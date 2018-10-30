
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

#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

#include "audioalsa.inc"
#include "audioconfig.inc"
#include "audiodvb.inc"
#include "audiodevice.inc"
#include "audioesound.inc"
#include "audiooss.inc"
#include "audiov4l2mpeg.inc"
#include "bctimer.inc"
#include "binary.h"
#include "condition.inc"
#include "dcoffset.inc"
#include "device1394output.inc"
#include "devicedvbinput.inc"
#include "devicempeginput.inc"
#include "maxchannels.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "preferences.inc"
#include "recordgui.inc"
#include "samples.inc"
#include "sema.inc"
#include "thread.h"
#include "videodevice.inc"
#ifdef HAVE_FIREWIRE
#include "audio1394.inc"
#include "device1394output.inc"
#include "vdevice1394.inc"
#endif

class AudioThread : public Thread
{
	AudioDevice *device;
	void (AudioDevice::*arun)();
	void (AudioDevice::*aend)();
	Condition *startup_lock;
	int started;
public:
	AudioThread(AudioDevice *device,
		void (AudioDevice::*arun)(),
		void (AudioDevice::*aend)());
	~AudioThread();
	void initialize();
	void run();
	void startup();
	void stop(int wait);
};

class AudioDevice
{
	friend class RecordAudio;
public:
// MWindow is required where global input is used, to get the pointer.
	AudioDevice(MWindow *mwindow = 0);
	~AudioDevice();

	friend class AudioALSA;
	friend class AudioMPEG;
	friend class AudioDVB;
	friend class AudioV4L2MPEG;
	friend class AudioOSS;
	friend class AudioESound;
	friend class Audio1394;
	friend class VDevice1394;
	friend class Device1394Output;
	friend class DeviceMPEGInput;
	friend class DeviceDVBInput;
	friend class DeviceV4L2Input;

	int open_input(AudioInConfig *config,
		VideoInConfig *vconfig,
		int rate,
		int samples,
		int channels,
		int realtime);
	int open_output(AudioOutConfig *config,
		int rate,
		int samples,
		int channels,
		int realtime);
	int open_monitor(AudioOutConfig *config, int mode);
	int close_all();

// ================================ recording

// read from the record device
// Conversion between double and int is done in AudioDevice
	int read_buffer(Samples **input, int channels,
		int samples, int *over, double *max, int input_offset = 0);
	void start_recording();
// If a firewire device crashed
	int interrupt_crash();

// ================================== dc offset

// writes to whichever buffer is free or blocks until one becomes free
	int write_buffer(double **output, int channels, int samples, double bfr_time=-1.);
// Stop threads
	int stop_audio(int wait);

// After the last buffer is written call this to terminate.
// A separate buffer for termination is required since the audio device can be
// finished without writing a single byte
	int set_last_buffer();

// start the thread processing buffers
	int start_playback();
	int is_monitoring() { return monitoring && monitor_open; }
	void set_monitoring(int mode);
// record menu auto config update
	void auto_update(int channels, int samplerate, int bits);
	int config_updated();
	void config_update();
// record raw stream on file descr fd, to close use fd=-1
//  bsz records up to bsz bytes of past buffer data in first_write
	int start_toc(const char *path, const char *toc_path);
	int start_record(int fd, int bsz=0);
	int stop_record();
	int total_audio_channels();
	DeviceMPEGInput *mpeg_device();

// interrupt the playback thread
	int interrupt_playback();
// drop the current record buffer and wait for resume
	void interrupt_recording();
	void resume_recording();
	void set_play_dither(int status);
	void set_rec_gain(double gain=1.);
	void set_play_gain(double gain=1.);
// set software positioning on or off
	void set_software_positioning(int status = 1);
// total samples played
//  + audio offset from configuration if playback
	int64_t current_position();
	int64_t samples_output();
	double get_itimestamp();
	double get_otimestamp();
	double device_timestamp();
	void set_vdevice(VideoDevice *vdevice) { this->vdevice = vdevice; }
	int get_interrupted() { return playback_interrupted; }
	int get_ichannels() { return in_channels; }
	int get_ochannels() { return out_channels; }
	int get_ibits() { return in_bits; }
	int get_obits() { return out_bits; }
	int get_irate() { return in_samplerate; }
	int get_orate() { return out_samplerate; }
	int get_irealtime() { return in_realtime; }
	int get_orealtime() { return out_realtime; }
	int get_device_buffer() { return device_buffer; }
	int64_t get_samples_written() { return total_samples_written; }
	int64_t get_samples_output() { return total_samples_output; }
	int64_t get_samples_read() { return total_samples_read; }
	int64_t get_samples_input() { return total_samples_input; }
	int get_idmix() { return in51_2; }
	int get_odmix() { return out51_2; }
	int get_play_gain() { return play_gain; }
	int get_record_gain() { return rec_gain; }
private:
	int initialize();
	int reset_output();
	int reset_input();
// Create a lowlevel driver out of the driver ID
	int create_lowlevel(AudioLowLevel* &lowlevel, int driver,int in);
	int arm_buffer(int buffer, double **output, int channels,
		 int samples, int64_t sample_position, double bfr_time);
	int read_inactive() {
		int result = !is_recording || recording_interrupted || in_config_updated;
		return result;
	}
// Override configured parameters depending on the driver
	int in_samplerate, out_samplerate;
	int in_bits, out_bits;
	int in_channels, out_channels;
	int in_samples, out_samples;
	int in_realtime, out_realtime;
	int in51_2, out51_2;
	int in_config_updated;

// open mode
	int r, w;
// Video device to pass data to if the same device handles video
	VideoDevice *vdevice;
// bits in output file 1 or 0
	int rec_dither, play_dither;
	double rec_gain, play_gain;
	int sharing;
	int monitoring;
	int monitor_open;
// background loop for playback
	AudioThread *audio_out;
	void run_output();
	void end_output();
	void stop_output(int wait);
// background loop for recording
	AudioThread *audio_in;
	void run_input();
	void end_input();
	void stop_input();
// writes input to output, follows input config
	void monitor_buffer(Samples **input, int channels,
		 int samples, int ioffset, double bfr_time);

	Mutex *device_lock;
	class input_buffer_t {
	public:
		char *buffer;
		int size;
		double timestamp;
	} input[TOTAL_AUDIO_BUFFERS];
	class output_buffer_t {
	public:
		char *buffer;
		int allocated, size;
		int last_buffer;
		int64_t sample_position;
		double bfr_time;
		Condition *play_lock;
		Condition *arm_lock;
	} output[TOTAL_AUDIO_BUFFERS];

	Mutex *timer_lock;
// Get buffer_lock to delay before locking to allow read_buffer to lock it.
	Mutex *buffer_lock;
	Condition *polling_lock;
	int arm_buffer_num;

// samples in buffer
	double last_buffer_time;
	int position_correction;
	int device_buffer;
// prevent the counter from going backwards
	int last_position;
	Timer *playback_timer;
	Timer *record_timer;
// Current operation
	int is_playing_back;
	int is_recording;
	int global_timer_started;
	int software_position_info;
	int playback_interrupted;
	int recording_interrupted;
	int idriver, odriver;

// Multiple data paths can be opened simultaneously by Record when monitoring
	AudioLowLevel *lowlevel_in, *lowlevel_out;
	AudioOutConfig *out_config;
	AudioInConfig *in_config;
// Extra configuration if shared with video
	VideoInConfig *vconfig;

// Buffer being used by the hardware
	int input_buffer_count;
	int input_buffer_in;
	int input_buffer_out;
	int input_buffer_offset;
	int output_buffer_num;
// for position information
	int64_t total_samples_read;
	int64_t total_samples_input;
	int64_t total_samples_written;
	int64_t total_samples_output;
	MWindow *mwindow;
};


class AudioLowLevel
{
public:
	AudioLowLevel(AudioDevice *device);
	virtual ~AudioLowLevel();

	virtual int open_input() { return 1; };
	virtual int open_output() { return 1; };
	virtual int close_all() { return 1; };
	virtual int interrupt_crash() { return 0; };
	virtual int64_t device_position() { return -1; };
	virtual int64_t samples_output() { return -1; };
	virtual int write_buffer(char *buffer, int size) { return 1; };
	virtual int read_buffer(char *buffer, int size) { return -1; };
	virtual int flush_device() { return 1; };
	virtual int interrupt_playback() { return 1; };
	virtual double device_timestamp() {
		struct timeval tv;  gettimeofday(&tv, 0);
		return tv.tv_sec + tv.tv_usec / 1000000.0;
	}
	virtual int start_toc(const char *path, const char *toc_path) { return -1; }
	virtual int start_record(int fd, int bsz=0) { return -1; }
	virtual int stop_record() { return -1; }
	virtual int total_audio_channels() {
		return device ? device->get_ichannels() : 0;
	}
	virtual DeviceMPEGInput *mpeg_device() { return 0; }

	AudioDevice *device;
};


#endif
