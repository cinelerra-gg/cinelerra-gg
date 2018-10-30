
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "audiodevice.h"
#include "clip.h"
#include "playbackconfig.h"
#include "recordconfig.h"
#include "bcprogressbox.h"
#include "bcsignals.h"
#include "bctimer.h"
#include "condition.h"
#include "dcoffset.h"
#include "samples.h"
#include "mutex.h"

#include <string.h>


#define STORE(k) \
  double v = fabs(sample); \
  if(v > 1) { ++over_count; sample = sample>0 ? 1 : -1; } \
  if(v > max[ich]) max[ich] = v; \
  input_channel[k] = sample

#define GET_8BIT(i) ((double)(buffer[(i)]))
#define GET_16BIT(i) ((double)(*(int16_t*)&buffer[(i)]))
#define GET_24BIT(i) ((i&1) ? \
    ((double)((*(uint8_t*)&buffer[i]) | (*(int16_t*)&buffer[i+1] << 8))) : \
    ((double)((*(uint16_t*)&buffer[i]) | (*(int8_t*)&buffer[i+2] << 16))))
#define GET_32BIT(i) ((double)(*(int32_t *)&buffer[(i)]))

#define GET_8BITS(j,k)  { double sample = gain*GET_8BIT(k);  STORE(j); }
#define GET_16BITS(j,k) { double sample = gain*GET_16BIT(k); STORE(j); }
#define GET_24BITS(j,k) { double sample = gain*GET_24BIT(k); STORE(j); }
#define GET_32BITS(j,k) { double sample = gain*GET_32BIT(k); STORE(j); }

#define GET_NBIT(sz,n,k,ich) \
  (GET_##n##BIT(k) + \
 2*GET_##n##BIT((k)+sz*(1+(ich))) + \
 2*GET_##n##BIT((k)+sz*(3+(ich))) + \
   GET_##n##BIT((k)+sz*5))

#define GET_8BITZ(j,k,ich)  { double sample = gain*GET_NBIT(1,8,k,ich);  STORE(j); }
#define GET_16BITZ(j,k,ich) { double sample = gain*GET_NBIT(2,16,k,ich); STORE(j); }
#define GET_24BITZ(j,k,ich) { double sample = gain*GET_NBIT(3,24,k,ich); STORE(j); }
#define GET_32BITZ(j,k,ich) { double sample = gain*GET_NBIT(4,32,k,ich); STORE(j); }

int AudioDevice::read_buffer(Samples **data, int channels,
	int samples, int *over, double *max, int input_offset)
{
	for( int i=0; i<channels; ++i ) {
		over[i] = 0;  max[i] = 0.;
	}
	int input_channels = get_ichannels();
	int map51_2 = in51_2 && channels == 2 && input_channels == 6;
	int bits = get_ibits();
	int frame_size = input_channels * bits / 8;
	int fragment_size = samples * frame_size;
	int result = !fragment_size ? 1 : 0;
	double gain = bits ? rec_gain / ((1<<(bits-1))-1) : 0.;
	if( map51_2 ) gain *= 0.2;

	while( !result && fragment_size > 0 ) {
		if( (result=read_inactive()) ) break;
		polling_lock->lock("AudioDevice::read_buffer");
		if( (result=read_inactive()) ) break;
		if( !input_buffer_count ) continue;

		input_buffer_t *ibfr = &input[input_buffer_out];
		if( input_buffer_offset >= ibfr->size ) {
			// guarentee the buffer has finished loading
			if( input_buffer_in == input_buffer_out ) continue;
			buffer_lock->lock("AudioDevice::read_buffer 1");
			--input_buffer_count;
			buffer_lock->unlock();
			if( ++input_buffer_out >= TOTAL_AUDIO_BUFFERS )
				input_buffer_out = 0;
			ibfr = &input[input_buffer_out];
			input_buffer_offset = 0;
		}
		char *buffer = ibfr->buffer + input_buffer_offset;
		int ibfr_remaining = ibfr->size - input_buffer_offset;

		int xfr_size = MIN(fragment_size, ibfr_remaining);
		int xfr_samples = xfr_size / frame_size;

		for( int ich=0; ich<channels; ++ich ) {
			int over_count = 0;
			double *input_channel = data[ich]->get_data() + input_offset;
			if( map51_2 ) {
				int k = 0;
				switch( bits ) {
				case 8: for(int j=0; j<xfr_samples; ++j,k+=frame_size)
						GET_8BITZ(j,k,ich)
					break;
				case 16: for(int j=0; j<xfr_samples; ++j,k+=frame_size)
						GET_16BITZ(j,k,ich)
					break;
				case 24: for(int j=0; j<xfr_samples; ++j,k+=frame_size)
						GET_24BITZ(j,k,ich)
					break;
				case 32: for(int j=0; j<xfr_samples; ++j,k+=frame_size)
						GET_32BITZ(j,k,ich)
					break;
				}
			}
			else {
				int k = (ich % input_channels) * bits / 8;
				switch( bits) {
				case 8: for(int j=0; j<xfr_samples; ++j,k+=frame_size)
						GET_8BITS(j,k)
					break;
				case 16: for(int j=0; j<xfr_samples; ++j,k+=frame_size)
						GET_16BITS(j,k)
					break;
				case 24: for(int j=0; j<xfr_samples; ++j,k+=frame_size)
						GET_24BITS(j,k)
					break;
				case 32: for(int j=0; j<xfr_samples; ++j,k+=frame_size)
						GET_32BITS(j,k)
					break;
				}
			}
			over[ich] = over_count >= 3 ? 1 : 0;
		}

		if( monitoring ) {
			int sample_offset = input_buffer_offset / frame_size;
			double buffer_time = ibfr->timestamp +
				(double) sample_offset / in_samplerate;
			monitor_buffer(data, channels,
				xfr_samples, input_offset, buffer_time);
		}

		input_offset += xfr_samples;
		input_buffer_offset += xfr_size;
		fragment_size -= xfr_size;
	}

	if( !result ) {
		total_samples_read += samples;
		record_timer->update();
	}

	return result;
}


void AudioDevice::run_input()
{
	while( is_recording ) {
// Get available input buffer
		input_buffer_t *ibfr = &input[input_buffer_in];
		char *data = &ibfr->buffer[ibfr->size];
		if( !ibfr->size || ibfr->timestamp < 0. )
			ibfr->timestamp = lowlevel_in->device_timestamp();
		int frame_size = get_ichannels() * get_ibits() / 8;
		int fragment_size = in_samples * frame_size;
		int result = lowlevel_in->read_buffer(data, fragment_size);
		if( !result ) {
			total_samples_input += in_samples;
			buffer_lock->lock("AudioDevice::run_input 2");
			if( !ibfr->size )
				++input_buffer_count;
			ibfr->size += fragment_size;

			if( ibfr->size > INPUT_BUFFER_BYTES-fragment_size ) {
#if 0
// jam job dvb file testing, enable code
while( is_recording ) {
	if( input_buffer_count < TOTAL_AUDIO_BUFFERS ) break;
	buffer_lock->unlock();
	Timer::delay(250);
	buffer_lock->lock("AudioDevice::run_input 3");
}
#endif
				if( input_buffer_count < TOTAL_AUDIO_BUFFERS ) {
					if( ++input_buffer_in >= TOTAL_AUDIO_BUFFERS )
						input_buffer_in = 0;
				}
				else {
					--input_buffer_count;
					printf("AudioDevice::run_input: buffer overflow\n");
					result = 1;
				}
				ibfr = &input[input_buffer_in];
				ibfr->size = 0;
				ibfr->timestamp = -1.;
			}
			buffer_lock->unlock();
			polling_lock->unlock();
		}
		if( result ) {
			perror("AudioDevice::run_input");
			usleep(250000);
		}
	}
}

void AudioDevice::end_input()
{
	is_recording = 0;
	polling_lock->unlock();
	buffer_lock->reset();
}

int AudioDevice::reset_input()
{
	for( int i=0; i<TOTAL_AUDIO_BUFFERS; ++i ) {
		input_buffer_t *ibfr = &input[i];
		if( ibfr->buffer ) {
			delete [] ibfr->buffer;
			ibfr->buffer = 0;
		}
		ibfr->size = 0;
		ibfr->timestamp = -1.;
	}
	input_buffer_count = 0;
	input_buffer_in = 0;
	input_buffer_out = 0;
	input_buffer_offset = 0;
	is_recording = 0;
	recording_interrupted = 0;
	buffer_lock->reset();
	polling_lock->reset();
	monitoring = 0;
	monitor_open = 0;
	return 0;
}

void AudioDevice::start_recording()
{
	reset_input();
	is_recording = 1;
	for( int i=0; i<TOTAL_AUDIO_BUFFERS; ++i ) {
		input[i].buffer = new char[INPUT_BUFFER_BYTES];
		input[i].size = 0;
	}
	record_timer->update();
	audio_in = new AudioThread(this,
		&AudioDevice::run_input,&AudioDevice::end_input);
	audio_in->set_realtime(get_irealtime());
	audio_in->startup();
}

void AudioDevice::interrupt_recording()
{
	recording_interrupted = 1;
	polling_lock->unlock();
}

void AudioDevice::resume_recording()
{
	recording_interrupted = 0;
}

void AudioDevice::set_rec_gain(double gain)
{
	rec_gain = gain * in_config->rec_gain;
}

void AudioDevice::set_monitoring(int mode)
{
	interrupt_playback();
	monitoring = mode;
	if( mode )
		start_playback();
}

void AudioDevice::monitor_buffer(Samples **data, int channels,
	 int samples, int ioffset, double bfr_time)
{
	if( !monitoring || !out_config ) return;
	if( !in_samplerate || !in_samples || !in_bits || !in_channels ) return;
	int ochannels = out_config->map51_2 && channels == 6 ? 2 : channels;
	if( !monitor_open
		/* follow input config, except for channels */
		 || in_samplerate != out_samplerate ||
		 in_bits != out_bits || ochannels != out_channels ||
		 in_samples != out_samples || in_realtime != out_realtime ) {
		interrupt_playback();
		if( lowlevel_out ) {
			lowlevel_out->close_all();
			delete lowlevel_out;
			lowlevel_out = 0;
		}
		int ret = open_output(out_config,
			in_samplerate, in_samples, channels, in_realtime );
		monitor_open = !ret ? 1 : 0;
		if( monitor_open ) {
			start_playback();
			monitoring = 1;
		}
	}
	if( is_monitoring() ) {
		double *offset_data[channels];
		for( int i=0; i<channels; ++i )
			offset_data[i] = data[i]->get_data() + ioffset;
		write_buffer(offset_data, channels, samples, bfr_time);
	}
}


