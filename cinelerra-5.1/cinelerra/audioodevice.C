
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
#include "playbackconfig.h"
#include "recordconfig.h"
#include "bctimer.h"
#include "condition.h"
#include "mutex.h"
#include "samples.h"
#include "sema.h"

#include <string.h>

#define NO_DITHER 0
#define DITHER dither()

// count of on bits
inline static unsigned int on_bits(unsigned int n) {
  n = (n & 0x55555555) + ((n >> 1) & 0x55555555);
  n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
  n = (n & 0x0f0f0f0f) + ((n >> 4) & 0x0f0f0f0f);
  n += n >> 8;  n += n >> 16;  //ok, fldsz > 5 bits
  return n & 0x1f;
}

// assumes RAND_MAX is (2**n)-1 and n<=32 bits
static int random_bits = on_bits(RAND_MAX);
static int dither_random = 0;
static long dither_bits = 0;

static inline int dither()
{
	if( --dither_bits < 0 ) {
		dither_random = random();
		dither_bits = random_bits;
	}
	return (dither_random>>dither_bits) & 1;
}

static inline int audio_clip(int64_t v, int mx)
{
  if( v > mx ) return mx;
  if( v < -mx-1 ) return -mx-1;
  return v;
}

#define FETCH1(m,dither) \
  audio_clip((output_channel[j]*gain)-dither, m)

#define FETCH51(m, dither) \
  audio_clip((gain*(center[j] + 2*front[j] + \
    2*back[j] + subwoof[j]))-dither, m)

#define PUT_8BIT_SAMPLES(n,dither) { \
  sample = FETCH##n(0x7f,dither); \
  *(int8_t*)&buffer[k] = sample; \
}

#define PUT_16BIT_SAMPLES(n,dither) { \
  sample = FETCH##n(0x7fff,dither); \
  *(int16_t*)&buffer[k] = sample; \
}

#define PUT_24BIT_SAMPLES(n,dither) { \
  sample = FETCH##n(0x7fffff,dither); \
  if( (k&1) ) { \
    *(int8_t*)&buffer[k] = sample; \
    *(int16_t*)&buffer[k+1] = (sample >> 8); \
  } else { \
    *(int16_t*)&buffer[k] = sample; \
    *(int8_t*)&buffer[k+2] = (sample >> 16); \
  } \
}

#define PUT_32BIT_SAMPLES(n,dither) { \
  sample = FETCH##n(0x7fffffff,dither); \
  *(int32_t*)&buffer[k] = sample; \
}

int AudioDevice::write_buffer(double **data,
	int channels, int samples, double bfr_time)
{
// find free buffer to fill
	if(playback_interrupted) return 0;
	int64_t sample_position = total_samples_written;
	total_samples_written += samples;
	int buffer_num = arm_buffer_num;
	if( ++arm_buffer_num >= TOTAL_AUDIO_BUFFERS ) arm_buffer_num = 0;
	arm_buffer(buffer_num, data, channels, samples,
		sample_position, bfr_time);
	return 0;
}

int AudioDevice::set_last_buffer()
{
	output_buffer_t *obfr = &output[arm_buffer_num];
	if( !obfr ) return 0;
	if( ++arm_buffer_num >= TOTAL_AUDIO_BUFFERS ) arm_buffer_num = 0;
	obfr->arm_lock->lock("AudioDevice::set_last_buffer");
	obfr->last_buffer = 1;
	obfr->play_lock->unlock();
	return 0;
}

// little endian byte order
int AudioDevice::arm_buffer(int buffer_num, double **data, int channels,
	int samples, int64_t sample_position, double bfr_time)
{
	if(!is_playing_back || playback_interrupted) return 1;
// wait for buffer to become available for writing
	output_buffer_t *obfr = &output[buffer_num];
	obfr->arm_lock->lock("AudioDevice::arm_buffer");
	if(!is_playing_back || playback_interrupted) return 1;

	int bits = get_obits();
	int device_channels = get_ochannels();
	int frame = device_channels * (bits / 8);
	int fragment_size = frame * samples;
	if( fragment_size > obfr->allocated ) {
		delete [] obfr->buffer;
		obfr->buffer = new char[fragment_size];
		obfr->allocated = fragment_size;
	}

	char *buffer = obfr->buffer;
	obfr->size = fragment_size;
	obfr->bfr_time = bfr_time;
	obfr->sample_position = sample_position;
	int map51_2 = out51_2 && channels == 6 && device_channels == 2;
	double gain = ((1u<<(bits-1))-1) * play_gain;
	if( map51_2 ) gain *= 0.2;

	for( int och=0; och<device_channels; ++och ) {
		int sample;
		int k = och * bits / 8;

		if( map51_2 ) {
			double *front = data[och];
			double *center = data[2];
			double *subwoof = data[3];
			double *back = data[och+4];
			if( play_dither ) {
				switch( bits ) {
				case 8: for( int j=0; j<samples; ++j,k+=frame )
						PUT_8BIT_SAMPLES(51,DITHER)
					break;
				case 16: for( int j=0; j<samples; ++j,k+=frame )
						PUT_16BIT_SAMPLES(51,DITHER)
					break;
				case 24: for( int j=0; j<samples; ++j,k+=frame )
						PUT_24BIT_SAMPLES(51,DITHER)
					break;
				case 32: for( int j=0; j<samples; ++j,k+=frame )
						PUT_32BIT_SAMPLES(51,DITHER)
					break;
				}
			}
			else {
				switch( bits ) {
				case 8: for( int j=0; j<samples; ++j,k+=frame )
						PUT_8BIT_SAMPLES(51,NO_DITHER)
					break;
				case 16: for( int j=0; j<samples; ++j,k+=frame )
						PUT_16BIT_SAMPLES(51,NO_DITHER)
					break;
				case 24: for( int j=0; j<samples; ++j,k+=frame )
						PUT_24BIT_SAMPLES(51,NO_DITHER)
					break;
				case 32: for( int j=0; j<samples; ++j,k+=frame )
						PUT_32BIT_SAMPLES(51,NO_DITHER)
					break;
				}
			}
		}
		else {
			double *output_channel = data[och % channels];
			if( play_dither ) {
				switch( bits ) {
				case 8: for( int j=0; j<samples; ++j,k+=frame )
						PUT_8BIT_SAMPLES(1,DITHER)
					break;
				case 16: for( int j=0; j<samples; ++j,k+=frame )
						PUT_16BIT_SAMPLES(1,DITHER)
					break;
				case 24: for( int j=0; j<samples; ++j,k+=frame )
						PUT_24BIT_SAMPLES(1,DITHER)
					break;
				case 32: for( int j=0; j<samples; ++j,k+=frame )
						PUT_32BIT_SAMPLES(1,DITHER)
					break;
				}
			}
			else {
				switch( bits ) {
				case 8: for( int j=0; j<samples; ++j,k+=frame )
						PUT_8BIT_SAMPLES(1,NO_DITHER)
					break;
				case 16: for( int j=0; j<samples; ++j,k+=frame )
						PUT_16BIT_SAMPLES(1,NO_DITHER)
					break;
				case 24: for( int j=0; j<samples; ++j,k+=frame )
						PUT_24BIT_SAMPLES(1,NO_DITHER)
					break;
				case 32: for( int j=0; j<samples; ++j,k+=frame )
						PUT_32BIT_SAMPLES(1,NO_DITHER)
					break;
				}
			}
		}
	}
// make buffer available for playback
	obfr->play_lock->unlock();
	return 0;
}

void AudioDevice::end_output()
{
	is_playing_back = 0;
	monitoring = 0;
	if( lowlevel_out )
		lowlevel_out->interrupt_playback();
	for( int i=0; i<TOTAL_AUDIO_BUFFERS; ++i ) {
		output_buffer_t *obfr = &output[i];
		obfr->play_lock->unlock();
		obfr->arm_lock->unlock();
	}
}

int AudioDevice::reset_output()
{
	for( int i=0; i<TOTAL_AUDIO_BUFFERS; ++i ) {
		output_buffer_t *obfr = &output[i];
		if( obfr->buffer ) {
			delete [] obfr->buffer;
			obfr->buffer = 0;
		}
		obfr->allocated = 0;
		obfr->size = 0;
		obfr->last_buffer = 0;
		obfr->arm_lock->reset();
		obfr->play_lock->reset();
	}
	is_playing_back = 0;
	playback_interrupted = 0;
	monitoring = 0;
	monitor_open = 0;
	output_buffer_num = 0;
	arm_buffer_num = 0;
	last_position = 0;
	return 0;
}


void AudioDevice::set_play_gain(double gain)
{
	play_gain = gain * out_config->play_gain;
}

void AudioDevice::set_play_dither(int status)
{
	play_dither = status;
}

void AudioDevice::set_software_positioning(int status)
{
	software_position_info = status;
}

int AudioDevice::start_playback()
{
// arm buffer before doing this
	is_playing_back = 1;
	playback_interrupted = 0;
	total_samples_written = 0;
	total_samples_output = 0;
// zero timers
	playback_timer->update();
	last_position = 0;
// start output thread
	audio_out = new AudioThread(this,
		&AudioDevice::run_output,&AudioDevice::end_output);
	audio_out->set_realtime(get_orealtime());
	audio_out->startup();
	return 0;
}

int AudioDevice::interrupt_playback()
{
//printf("AudioDevice::interrupt_playback\n");
	stop_output(0);
	return 0;
}

int64_t AudioDevice::samples_output()
{
	int64_t result = !lowlevel_out ? -1 : lowlevel_out->samples_output();
	if( result < 0 ) result = total_samples_output;
	return result;
}


int64_t AudioDevice::current_position()
{
// try to get OSS position
	int64_t result = 0;

	if(w) {
		int frame = (get_obits() * get_ochannels()) / 8;
		if( !frame ) return 0;

// get hardware position
		if(!software_position_info && lowlevel_out) {
			result = lowlevel_out->device_position();
			int64_t sample_count = samples_output();
			if( result > sample_count )
				result = sample_count;
		}

// get software position
		if(result < 0 || software_position_info) {
			timer_lock->lock("AudioDevice::current_position");
			result = total_samples_output - device_buffer / frame;
			result += playback_timer->get_scaled_difference(get_orate());
			timer_lock->unlock();
			int64_t sample_count = samples_output();
			if( result > sample_count )
				result = sample_count;
		}

		if(result < last_position)
			result = last_position;
		else
			last_position = result;

		result -= (int64_t)(get_orate() * out_config->audio_offset);

	}
	else if(r) {
		result = total_samples_read +
			record_timer->get_scaled_difference(get_irate());
	}

	return result;
}

void AudioDevice::run_output()
{
	output_buffer_num = 0;
	playback_timer->update();

//printf("AudioDevice::run 1 %d\n", Thread::calculate_realtime());
	while( is_playing_back && !playback_interrupted ) {
// wait for buffer to become available
   		int buffer_num = output_buffer_num;
		if( ++output_buffer_num >= TOTAL_AUDIO_BUFFERS ) output_buffer_num = 0;
		output_buffer_t *obfr = &output[buffer_num];
		obfr->play_lock->lock("AudioDevice::run 1");
		if( !is_playing_back || playback_interrupted ) break;
		if( obfr->last_buffer ) { lowlevel_out->flush_device(); break; }
// get size for position information
// write converted buffer synchronously
		double bfr_time = obfr->bfr_time;
		int result = lowlevel_out->write_buffer(obfr->buffer, obfr->size);
// allow writing to the buffer
		obfr->arm_lock->unlock();
		if( !result ) {
			timer_lock->lock("AudioDevice::run 3");
			int frame = get_ochannels() * get_obits() / 8;
			int samples = frame ? obfr->size / frame : 0;
			total_samples_output += samples;
			last_buffer_time = bfr_time + (double)samples/out_samplerate;
			playback_timer->update();
			timer_lock->unlock();
		}
// inform user if the buffer write failed
		else if( result < 0 ) {
			perror("AudioDevice::write_buffer");
			usleep(250000);
		}
	}

	is_playing_back = 0;
}


