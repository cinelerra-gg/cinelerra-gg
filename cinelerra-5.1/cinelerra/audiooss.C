
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

#include "audioconfig.h"
#include "audiodevice.h"
#include "audiooss.h"
#include "clip.h"
#include "condition.h"
#include "errno.h"
#include "language.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "recordconfig.h"
#include "mutex.h"

#include <string.h>

#ifdef HAVE_OSS

// These are only available in commercial OSS

#ifndef AFMT_S32_LE
#define AFMT_S32_LE 	 0x00001000
#define AFMT_S32_BE 	 0x00002000
#endif


// Synchronie multiple devices by using threads

OSSThread::OSSThread(AudioOSS *device)
 : Thread(1, 0, 0)
{
	rd = 0;
	wr = 0;
	done = 0;
	this->device = device;
	input_lock = new Condition(0, "OSSThread::input_lock");
	output_lock = new Condition(1, "OSSThread::output_lock");
	read_lock = new Condition(0, "OSSThread::read_lock");
	write_lock = new Condition(0, "OSSThread::write_lock");

	// delay / timing
	bytes_written = 0;
	timer = new Timer;
	delay = 0;
	timer_lock = new Mutex("OSSThread::timer_lock");
	timer->update();
}

OSSThread::~OSSThread()
{
	done = 1;
	input_lock->unlock();
	Thread::join();
	delete input_lock;
	delete output_lock;
	delete read_lock;
	delete write_lock;
	delete timer_lock;
	delete timer;
}

void OSSThread::run()
{
// this makes the longest (blocking) write 1024 samples
//  if this is not done, the video can be jerky
	AudioDevice *audio_device = device->device;
	int frame_size = audio_device->get_ochannels() * audio_device->get_obits()/8;
	int maxsz = 1024*frame_size, blksz = maxsz;
	ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &blksz);
	if( blksz > maxsz ) blksz = maxsz;

	while(!done) {
		input_lock->lock("OSSThread::run 1");
		if(done) return;
		if(rd) {
			read(fd, data, bytes);
			read_lock->unlock();
		}
		else if(wr) {
			if( !bytes_written )
				timer->update();
			int count = bytes;
			unsigned char *bp = data;
			while( count > 0 && !done ) {
				Thread::enable_cancel();
				int len = count>blksz ? blksz : count;
				int ret = write(fd, bp, len);
				Thread::disable_cancel();
				if( ret < 0 ) break;
				bp += ret;
				count -= ret;
				int delay = 0;
				timer_lock->lock("OSSThread::run");
				if( !ioctl(fd, SNDCTL_DSP_GETODELAY, &delay) )
					this->delay = delay;
				bytes_written += ret;
				timer->update();
				timer_lock->unlock();
			}
			write_lock->unlock();
		}
		output_lock->unlock();
	}
}

void OSSThread::write_data(int fd, unsigned char *data, int bytes)
{
	output_lock->lock("OSSThread::write_data");
	wr = 1;
	rd = 0;
	this->data = data;
	this->bytes = bytes;
	this->fd = fd;
	input_lock->unlock();
}

void OSSThread::read_data(int fd, unsigned char *data, int bytes)
{
	output_lock->lock("OSSThread::read_data");
	wr = 0;
	rd = 1;
	this->data = data;
	this->bytes = bytes;
	this->fd = fd;
	input_lock->unlock();
}

void OSSThread::wait_read()
{
	read_lock->lock("OSSThread::wait_read");
}

void OSSThread::wait_write()
{
	write_lock->lock("OSSThread::wait_write");
}


// bytes not samples
int64_t OSSThread::device_position()
{
	timer_lock->lock("OSSThread::device_position");
	int64_t ret = bytes_written - delay;
	timer_lock->unlock();
	return ret;
}


AudioOSS::AudioOSS(AudioDevice *device)
 : AudioLowLevel(device)
{
	for(int i = 0; i < MAXDEVICES; i++) {
		dsp_in[i] = dsp_out[i] = -1;
		thread[i] = 0;
		data[i] = 0;
		data_allocated[i] = 0;
	}
}

AudioOSS::~AudioOSS()
{
}

int AudioOSS::open_input()
{
	device->in_bits = device->in_config->oss_in_bits;
// 24 bits not available in OSS
	if(device->in_bits == 24) device->in_bits = 32;

	for(int i = 0; i < MAXDEVICES; i++) {
		if(device->in_config->oss_enable[i]) {
//printf("AudioOSS::open_input 10\n");
			dsp_in[i] = open(device->in_config->oss_in_device[i], O_RDONLY/* | O_NDELAY*/);
//printf("AudioOSS::open_input 20\n");
			if(dsp_in[i] < 0) {
				fprintf(stderr, "AudioOSS::open_input %s: %s\n",
					device->in_config->oss_in_device[i], strerror(errno));
				close_all();
				return 1;
			}

			int format = get_fmt(device->in_config->oss_in_bits);
			int buffer_info = sizetofrag(device->in_samples,
				device->get_ichannels(),
				device->in_config->oss_in_bits);

			set_cloexec_flag(dsp_in[i], 1);

// For the ice1712 the buffer must be maximum or no space will be allocated.
			if(device->idriver == AUDIO_OSS_ENVY24)
				buffer_info = 0x7fff000f;
			int ret = 1;
			if(ioctl(dsp_in[i], SNDCTL_DSP_SETFRAGMENT, &buffer_info))
				fprintf(stderr, _("%s failed\n"), "SNDCTL_DSP_SETFRAGMENT");
			else if(ioctl(dsp_in[i], SNDCTL_DSP_SETFMT, &format) < 0)
				fprintf(stderr, _("%s failed\n"), "SNDCTL_DSP_SETFMT");
			else {
				int channels = device->get_ichannels();
				if(ioctl(dsp_in[i], SNDCTL_DSP_CHANNELS, &channels) < 0)
					fprintf(stderr, _("%s failed\n"), "SNDCTL_DSP_CHANNELS");
				else if(ioctl(dsp_in[i], SNDCTL_DSP_SPEED, &device->in_samplerate) < 0)
					fprintf(stderr, _("%s failed\n"), "SNDCTL_DSP_SPEED");
				else
					ret = 0;
			}
			if( ret ) {
				close_all();
				return ret;
			}

			audio_buf_info recinfo;
			ioctl(dsp_in[i], SNDCTL_DSP_GETISPACE, &recinfo);

//printf("AudioOSS::open_input fragments=%d fragstotal=%d fragsize=%d bytes=%d\n",
//	recinfo.fragments, recinfo.fragstotal, recinfo.fragsize, recinfo.bytes);

			thread[i] = new OSSThread(this);
			thread[i]->start();
		}
	}
	return 0;
}

int AudioOSS::open_output()
{
	device->out_bits = device->out_config->oss_out_bits;
// OSS only supports 8, 16, and 32
	if(device->out_bits == 24) device->out_bits = 32;

	for(int i = 0; i < MAXDEVICES; i++) {
		if(device->out_config->oss_enable[i]) {
// Linux 2.4.18 no longer supports allocating the maximum buffer size.
// Need the shrink fragment size in preferences until it works.
			dsp_out[i] =
				open(device->out_config->oss_out_device[i],
					O_WRONLY /*| O_NDELAY*/);
			if(dsp_out[i] < 0) {
				perror("AudioOSS::open_output");
				close_all();
				return 1;
			}

			int format = get_fmt(device->out_config->oss_out_bits);
			int buffer_info = sizetofrag(device->out_samples,
				device->get_ochannels(),
				device->out_config->oss_out_bits);

			set_cloexec_flag(dsp_out[i], 1);

// For the ice1712 the buffer must be maximum or no space will be allocated.
			if(device->odriver == AUDIO_OSS_ENVY24)
				buffer_info = 0x7fff000f;
			int ret = 1;
			if(ioctl(dsp_out[i], SNDCTL_DSP_SETFRAGMENT, &buffer_info))
				fprintf(stderr,"SNDCTL_DSP_SETFRAGMENT 2 failed.\n");
			else if(ioctl(dsp_out[i], SNDCTL_DSP_SETFMT, &format) < 0)
				fprintf(stderr,"SNDCTL_DSP_SETFMT 2 failed\n");
			else {
				int channels = device->get_ochannels();
				if(ioctl(dsp_out[i], SNDCTL_DSP_CHANNELS, &channels) < 0)
					fprintf(stderr,"SNDCTL_DSP_CHANNELS 2 failed\n");
				else if(ioctl(dsp_out[i], SNDCTL_DSP_SPEED, &device->out_samplerate) < 0)
					fprintf(stderr,"SNDCTL_DSP_SPEED 2 failed\n");
				else
					ret = 0;
			}
			if( ret ) {
				close_all();
				return ret;
			}

			audio_buf_info playinfo;
			ioctl(dsp_out[i], SNDCTL_DSP_GETOSPACE, &playinfo);
// printf("AudioOSS::open_output fragments=%d fragstotal=%d fragsize=%d bytes=%d\n",
// playinfo.fragments, playinfo.fragstotal, playinfo.fragsize, playinfo.bytes);
			device->device_buffer = playinfo.bytes;
			thread[i] = new OSSThread(this);
			thread[i]->start();
		}
	}
	return 0;
}

int AudioOSS::sizetofrag(int samples, int channels, int bits)
{
	int testfrag = 2, fragsize = 1, grain = 4;
	samples *= channels * bits / (8 * grain);
	while(testfrag < samples) {
		fragsize++;
		testfrag *= 2;
	}
//printf("AudioOSS::sizetofrag %d\n", fragsize);
	return (grain << 16) | fragsize;
}

int AudioOSS::get_fmt(int bits)
{
	switch(bits) {
	case 32: return AFMT_S32_LE; break;
	case 16: return AFMT_S16_LE; break;
	case 8:  return AFMT_S8;  break;
	}
	return AFMT_S16_LE;
}


int AudioOSS::close_all()
{
//printf("AudioOSS::close_all 1\n");
	for(int i = 0; i < MAXDEVICES; i++) {
		if(dsp_in[i] >= 0) {
			ioctl(dsp_in[i], SNDCTL_DSP_RESET, 0);
			close(dsp_in[i]);
			dsp_in[i] = -1;
		}

		if(dsp_out[i] >= 0) {
//printf("AudioOSS::close_all 2\n");
			ioctl(dsp_out[i], SNDCTL_DSP_RESET, 0);
			close(dsp_out[i]);
			dsp_out[i] = -1;
		}

		if(thread[i]) delete thread[i];
		if(data[i]) delete [] data[i];
	}
	return 0;
}

int AudioOSS::set_cloexec_flag(int desc, int value)
{
	int oldflags = fcntl (desc, F_GETFD, 0);
	if (oldflags < 0) return oldflags;
	if(value != 0)
		oldflags |= FD_CLOEXEC;
	else
		oldflags &= ~FD_CLOEXEC;
	return fcntl(desc, F_SETFD, oldflags);
}

int64_t AudioOSS::device_position()
{
	for (int i = 0; i < MAXDEVICES; i++) {
		if( thread[i] ) {
			int frame_size = device->get_ochannels() * device->get_obits()/8;
			int64_t pos = thread[i]->device_position() / frame_size;
			int64_t tmr = thread[i]->timer->get_scaled_difference(device->out_samplerate);
			return pos + tmr;
		}
	}
	count_info info;
	if(!ioctl(get_output(0), SNDCTL_DSP_GETOPTR, &info))
	{
//printf("AudioOSS::device_position %d %d %d\n", info.bytes, device->get_obits(), device->get_ochannels());
// workaround for ALSA OSS emulation driver's bug
// the problem is that if the first write to sound device was not full lenght fragment then
// _GETOPTR returns insanely large numbers at first moments of play
		if (info.bytes > 2100000000) return 0;
		int frame_size = device->get_ochannels() * device->get_obits()/8;
		return info.bytes / frame_size;
	}
	return 0;
}

int AudioOSS::interrupt_playback()
{
//printf("AudioOSS::interrupt_playback 1\n");
	for(int i = 0; i < MAXDEVICES; i++) {
		if(thread[i]) {
			thread[i]->done = 1;
			thread[i]->input_lock->unlock();
			thread[i]->write_lock->unlock();
			thread[i]->cancel();
		}
	}
//printf("AudioOSS::interrupt_playback 100\n");
	return 0;
}

int64_t AudioOSS::samples_output()
{
	for( int i=0; i<MAXDEVICES; ++i ) {
		if( thread[i] ) {
			int frame_size = device->get_ochannels() * device->get_obits()/8;
			return thread[i]->bytes_written / frame_size;
		}
	}
	return 0;
}

int AudioOSS::read_buffer(char *buffer, int bytes)
{
	int sample_size = device->get_ibits() / 8;
	int out_frame_size = device->get_ichannels() * sample_size;
	int samples = bytes / out_frame_size;

//printf("AudioOSS::read_buffer 1 %d\n", bytes);
// Fill temp buffers
	for(int i = 0; i < MAXDEVICES; i++) {
		if(thread[i]) {
			int in_frame_size = device->get_ichannels() * sample_size;

			if(data[i] && data_allocated[i] < bytes) {
				delete [] data[i];
				data[i] = 0;
			}
			if(!data[i]) {
				data[i] = new unsigned char[bytes];
				data_allocated[i] = bytes;
			}

			thread[i]->read_data(get_input(i), data[i], samples * in_frame_size);
		}
	}

//printf("AudioOSS::read_buffer 1 %d\n", device->get_ibits());
	for(int i = 0, out_channel = 0; i < MAXDEVICES; i++) {
		if(thread[i]) {
			thread[i]->wait_read();

			for(int in_channel = 0;
				in_channel < device->get_ichannels();
				in_channel++) {
				int in_frame_size = device->get_ichannels() * sample_size;

				for(int k = 0; k < samples; k++) {
					for(int l = 0;
						l < sample_size;
						l++) {
						buffer[out_channel * sample_size + k * out_frame_size + l] =
							data[i][in_channel * sample_size + k * in_frame_size + l];
					}
				}
				out_channel++;
			}
		}
	}
//printf("AudioOSS::read_buffer 2\n");
	return 0;
}

int AudioOSS::write_buffer(char *buffer, int bytes)
{
	int sample_size = device->get_obits() / 8;
	int in_frame_size = device->get_ochannels() * sample_size;
	int samples = bytes / in_frame_size;

	for(int i = 0, in_channel = 0; i < MAXDEVICES; i++) {
		if( thread[i] ) {
			int out_frame_size = device->get_ochannels() * sample_size;
			if(data[i] && data_allocated[i] < bytes) {
				delete [] data[i];
				data[i] = 0;
			}
			if(!data[i]) {
				data[i] = new unsigned char[bytes];
				data_allocated[i] = bytes;
			}

			for(int out_channel = 0;
				out_channel < device->get_ochannels();
				out_channel++) {

				for(int k = 0; k < samples; k++) {
					for(int l = 0; l < sample_size; l++) {
						data[i][out_channel * sample_size + k * out_frame_size + l] =
							buffer[in_channel * sample_size + k * in_frame_size + l];
					}
				}
				in_channel++;
			}

			thread[i]->write_data(get_output(i), data[i], samples * out_frame_size);
		}
	}
	for(int i = 0; i < MAXDEVICES; i++) {
		if( thread[i] ) thread[i]->wait_write();
	}
	return 0;
}

int AudioOSS::flush_device()
{
	for(int i = 0; i < MAXDEVICES; i++)
		if(thread[i]) ioctl(get_output(i), SNDCTL_DSP_SYNC, 0);
	return 0;
}

int AudioOSS::get_output(int number)
{
	return device->w ? dsp_out[number] : -1;
}

int AudioOSS::get_input(int number)
{
	return device->r ? dsp_in[number] : -1;
}

#endif // HAVE_OSS
