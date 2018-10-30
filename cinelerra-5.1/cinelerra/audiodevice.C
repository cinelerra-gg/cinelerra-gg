
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
#ifdef HAVE_FIREWIRE
#include "audio1394.h"
#endif
#include "audioalsa.h"
#include "audiodvb.h"
#include "audiov4l2mpeg.h"
#include "audioesound.h"
#include "audiooss.h"
#include "asset.h"
#include "bctimer.h"
#include "condition.h"
#include "dcoffset.h"
#include "edl.h"
#include "edlsession.h"
#include "mainmenu.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "recordconfig.h"
#include "recordgui.h"
#include "record.h"
#include "sema.h"


AudioLowLevel::AudioLowLevel(AudioDevice *device)
{
	this->device = device;
}

AudioLowLevel::~AudioLowLevel()
{
}


AudioDevice::AudioDevice(MWindow *mwindow)
{
	initialize();
	this->mwindow = mwindow;
	this->out_config = new AudioOutConfig();
	this->in_config = new AudioInConfig;
	this->vconfig = new VideoInConfig;
	device_lock = new Mutex("AudioDevice::device_lock",1);
	timer_lock = new Mutex("AudioDevice::timer_lock");
	buffer_lock = new Mutex("AudioDevice::buffer_lock");
	polling_lock = new Condition(0, "AudioDevice::polling_lock");
	playback_timer = new Timer;
	record_timer = new Timer;
	for(int i = 0; i < TOTAL_AUDIO_BUFFERS; i++) {
		output_buffer_t *obfr = &output[i];
		obfr->play_lock = new Condition(0, "AudioDevice::play_lock");
		obfr->arm_lock = new Condition(1, "AudioDevice::arm_lock");
	}
	reset_input();
	total_samples_read = 0;
	total_samples_input = 0;
	reset_output();
	total_samples_written = 0;
	total_samples_output = 0;
}

AudioDevice::~AudioDevice()
{
	stop_audio(0);
	delete out_config;
	delete in_config;
	delete vconfig;
	delete timer_lock;
	for( int i=0; i < TOTAL_AUDIO_BUFFERS; ++i ) {
		output_buffer_t *obfr = &output[i];
		delete obfr->play_lock;
		delete obfr->arm_lock;
	}
	delete playback_timer;
	delete record_timer;
	delete buffer_lock;
	delete polling_lock;
	delete device_lock;
}

int AudioDevice::initialize()
{
	for(int i = 0; i < TOTAL_AUDIO_BUFFERS; i++) {
		input[i].buffer = 0;
		output[i].buffer = 0;
	}
	audio_in = audio_out = 0;
	in_bits = out_bits = 0;
	in_channels = out_channels = 0;
	in_samples = out_samples = 0;
	in_samplerate = out_samplerate = 0;
	in_realtime = out_realtime = 0;
	lowlevel_out = lowlevel_in = 0;
	in51_2 = out51_2 = 0;
	in_config_updated = 0;
	rec_dither = play_dither = 0;
	rec_gain = play_gain = 1.;
	r = w = 0;
	software_position_info = 0;
	vdevice = 0;
	sharing = 0;
	return 0;
}


void AudioThread::initialize()
{
	device = 0;
	arun = 0;
	aend = 0;
	startup_lock = 0;
	started = 0;
}


AudioThread::
AudioThread(AudioDevice *device,
	void (AudioDevice::*arun)(), void (AudioDevice::*aend)())
 : Thread(1, 0, 0)
{
	initialize();
	startup_lock = new Condition(0, "AudioThread::startup_lock");
	this->device = device;
	this->arun = arun;
	this->aend = aend;
	Thread::start();
}

AudioThread::~AudioThread()
{
	stop(0);
	delete startup_lock;
}

void AudioThread::stop(int wait)
{
	if( !wait ) (device->*aend)();
	startup();
	Thread::join();
}

void AudioThread::run()
{
	startup_lock->lock();
	(device->*arun)();
}

void AudioThread::startup()
{
	if( !started ) {
		started = 1;
		startup_lock->unlock();
	}
}


void AudioDevice::stop_input()
{
	device_lock->lock("AudioDevice::stop_input");
	if( audio_in ) {
		audio_in->stop(0);
		delete audio_in;
		audio_in = 0;
	}
	reset_input();
	device_lock->unlock();
}

void AudioDevice::stop_output(int wait)
{
	if( !wait ) playback_interrupted = 1;
	device_lock->lock("AudioDevice::stop_output");
	if( audio_out ) {
		if( is_playing_back )
			audio_out->stop(wait);
		delete audio_out;
		audio_out = 0;
	}
	reset_output();
	device_lock->unlock();
}

int AudioDevice::stop_audio(int wait)
{
	if( audio_in ) stop_input();
	if( audio_out ) stop_output(wait);
	return 0;
}


int AudioDevice::create_lowlevel(AudioLowLevel* &lowlevel, int driver,int in)
{
	*(in ? &this->idriver : &this->odriver) = driver;

	if(!lowlevel) {
		switch(driver) {
#ifdef HAVE_OSS
		case AUDIO_OSS:
		case AUDIO_OSS_ENVY24:
			lowlevel = new AudioOSS(this);
			break;
#endif

#ifdef HAVE_ESOUND
		case AUDIO_ESOUND:
			lowlevel = new AudioESound(this);
			break;
#endif
		case AUDIO_NAS:
			break;

#ifdef HAVE_ALSA
		case AUDIO_ALSA:
			lowlevel = new AudioALSA(this);
			break;
#endif

#ifdef HAVE_FIREWIRE
		case AUDIO_1394:
		case AUDIO_DV1394:
		case AUDIO_IEC61883:
			lowlevel = new Audio1394(this);
			break;
#endif

#ifdef HAVE_DVB
		case AUDIO_DVB:
			lowlevel = new AudioDVB(this);
			break;
#endif

#ifdef HAVE_VIDEO4LINUX2
		case AUDIO_V4L2MPEG:
			lowlevel = new AudioV4L2MPEG(this);
			break;
#endif
		}
	}
	return 0;
}

int AudioDevice::open_input(AudioInConfig *config, VideoInConfig *vconfig,
	int rate, int samples, int channels, int realtime)
{
	device_lock->lock("AudioDevice::open_input");
	r = 1;
	this->in_config->copy_from(config);
	this->vconfig->copy_from(vconfig);
	in_samplerate = rate;
	in_samples = samples;
	in_realtime = realtime;
	in51_2 = config->map51_2 && channels == 2;
	if( in51_2 ) channels = 6;
	in_channels = channels;
        rec_gain = config->rec_gain;
	create_lowlevel(lowlevel_in, config->driver, 1);
	lowlevel_in->open_input();
	in_config_updated = 0;
	record_timer->update();
	device_lock->unlock();
	return 0;
}

int AudioDevice::open_output(AudioOutConfig *config,
	int rate, int samples, int channels, int realtime)
{
	device_lock->lock("AudioDevice::open_output");
	w = 1;
	*this->out_config = *config;
	out_samplerate = rate;
	out_samples = samples;
	out51_2 = config->map51_2 && channels == 6;
	if( out51_2 ) channels = 2;
	out_channels = channels;
	out_realtime = realtime;
        play_gain = config->play_gain;
	create_lowlevel(lowlevel_out, config->driver,0);
	int result = lowlevel_out ? lowlevel_out->open_output() : 0;
	device_lock->unlock();
	return result;
}

int AudioDevice::open_monitor(AudioOutConfig *config,int mode)
{
	device_lock->lock("AudioDevice::open_output");
	w = 1;
	*this->out_config = *config;
	monitoring = mode;
	monitor_open = 0;
	device_lock->unlock();
	return 0;
}



int AudioDevice::interrupt_crash()
{
	if( lowlevel_in )
		lowlevel_in->interrupt_crash();
	stop_input();
	return 0;
}

double AudioDevice::get_itimestamp()
{
	buffer_lock->lock("AudioDevice::get_itimestamp");
	timer_lock->lock("AudioDevice::get_otimestamp");
	input_buffer_t *ibfr = &input[input_buffer_out];
	int frame = get_ichannels() * get_ibits() / 8;
	int64_t samples = frame ? input_buffer_offset / frame : 0;
	double timestamp = !in_samplerate || ibfr->timestamp < 0. ? -1. :
		ibfr->timestamp + (double) samples / in_samplerate;
	timer_lock->unlock();
	buffer_lock->unlock();
	return timestamp;
}

double AudioDevice::get_otimestamp()
{
	buffer_lock->lock("AudioDevice::get_otimestamp");
	timer_lock->lock("AudioDevice::get_otimestamp");
	int64_t unplayed_samples = total_samples_output - current_position();
	double unplayed_time = !out_samplerate || last_buffer_time < 0. ? -1. :
		(double)unplayed_samples / out_samplerate;
	double timestamp = last_buffer_time - unplayed_time;
	timer_lock->unlock();
	buffer_lock->unlock();
	return timestamp;
}

double AudioDevice::device_timestamp()
{
	return lowlevel_in ? lowlevel_in->device_timestamp() : -1.;
}

int AudioDevice::close_all()
{
	device_lock->lock("AudioDevice::close_all");
	stop_audio(0);
	if( lowlevel_in ) {
		lowlevel_in->close_all();
		delete lowlevel_in;
		lowlevel_in = 0;
	}
	if( lowlevel_out )
	{
		lowlevel_out->close_all();
		delete lowlevel_out;
		lowlevel_out = 0;
	}
	initialize();
	device_lock->unlock();
	return 0;
}

void AudioDevice::auto_update(int channels, int samplerate, int bits)
{
	if( !in_config || !in_config->follow_audio ) return;
	in_channels = channels;
	in_samplerate = samplerate;
	in_bits = bits;
	in_config_updated = 1;
	polling_lock->unlock();
}

int AudioDevice::config_updated()
{
	if( !in_config || !in_config->follow_audio ) return 0;
	return in_config_updated;
}

void AudioDevice::config_update()
{
	in_config_updated = 0;
	AudioInConfig *aconfig_in = mwindow->edl->session->aconfig_in;
	in51_2 = aconfig_in->map51_2 && in_channels == 6 ? 1 : 0;
	int ichannels = in51_2 ? 2 : in_channels;
	AudioOutConfig *aconfig_out = mwindow->edl->session->playback_config->aconfig;
	out51_2 = aconfig_out->map51_2 && ichannels == 6 ? 1 : 0;
	aconfig_in->in_samplerate = in_samplerate;
	aconfig_in->channels = ichannels;
	aconfig_in->oss_in_bits = in_bits;
	aconfig_in->alsa_in_bits = in_bits;
	total_samples_read = 0;
	total_samples_input = 0;
	monitor_open = 0;
}

int AudioDevice::start_toc(const char *path, const char *toc_path)
{
	return lowlevel_in ? lowlevel_in->start_toc(path, toc_path) : -1;
}

int AudioDevice::start_record(int fd, int bsz)
{
	return lowlevel_in ? lowlevel_in->start_record(fd, bsz) : -1;
}

int AudioDevice::stop_record()
{
	return lowlevel_in ? lowlevel_in->stop_record() : -1;
}

int AudioDevice::total_audio_channels()
{
	return lowlevel_in ? lowlevel_in->total_audio_channels() : 0;
}

DeviceMPEGInput *AudioDevice::mpeg_device()
{
	return lowlevel_in ? lowlevel_in->mpeg_device() : 0;
}

