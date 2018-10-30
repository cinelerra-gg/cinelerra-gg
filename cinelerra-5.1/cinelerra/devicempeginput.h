
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

#ifndef DEVICEMPEGINPUT_H
#define DEVICEMPEGINPUT_H

#include "devicempeginput.inc"
#include "audiodevice.h"
#include "audiompeg.inc"
#include "file.inc"
#include "videodevice.h"
#include "vdevicempeg.inc"
#include "condition.h"
#include "garbage.h"
#include "linklist.h"
#include "channel.h"
#include "vframe.h"
#include "mutex.h"
#include "record.inc"
#include "mwindow.inc"
//#include "recordconfig.inc"
#include <stdint.h>
#include "thread.h"
#include "libzmpeg3.h"

#define INPUT_SAMPLES 131072
#define BUFFER_TIMEOUT 500000


// This is the common interface for the V4L2 recording devices.
// It handles the network connection to the V4L2 dev.

class DeviceMPEGList : public List<DeviceMPEGInput>
{
public:
	DeviceMPEGList() {}
	~DeviceMPEGList() { first=last=0; } // no deletes
};

class DeviceMPEG_TOC_Builder : public Thread
{
	int done;
	DeviceMPEGInput *mpeg_dev;
public:
	DeviceMPEG_TOC_Builder(DeviceMPEGInput *mpeg_dev);
	~DeviceMPEG_TOC_Builder();
	void stop(int wait=0);
	void start();
	void run();
};


class DeviceMPEGInput : public Garbage, public ListItem<DeviceMPEGInput>
{
	friend class DeviceDVBInput;
	friend class DeviceV4L2Input;
	friend class DeviceMPEG_TOC_Builder;

	static Condition in_mpeg_lock;
	static DeviceMPEGList in_mpeg;

	static DeviceMPEGInput* get_mpeg_device(
		DeviceMPEGInput* (*new_device)(const char *name, int no),
	        const char *name, int no);

	int wait_signal(double tmo, int trys);
	int get_stream(int reopen=0);
	int get_channeldb(ArrayList<Channel*> *channeldb);
	Channel *add_channel( ArrayList<Channel*> *channeldb,
		char *name, int element, int major, int minor,
		int vstream, int astream, char *enc);
	int get_channel_table() { return channel ? channel->freqtable : -1; }
	int get_channel_input() { return channel ? channel->input : -1; }
	int get_channel_tuner() { return channel ? channel->tuner : -1; }
	static int get_dev_cmodel(int colormodel);

	virtual int mpeg_fd() = 0;
	virtual int open_dev(int color_model) = 0;
	virtual void close_dev() = 0;
	virtual int status_dev() = 0;

	const char *dev_name;
	int device_number;
	VideoDevice *video_device;
	AudioDevice *audio_device;
	Channel *channel;
	Mutex *decoder_lock;
	Mutex *video_lock;
	Mutex *audio_lock;

	zmpeg3_t *src;
	zmpeg3_t *toc;
	DeviceMPEG_TOC_Builder *toc_builder;
	int tick_toc();
	int64_t toc_pos;

	int color_model;
	int audio_inited, video_inited;
	int audio_stream, video_stream;
	int record_fd, captioning;
	int total_vstreams, total_achannels;

	int height, width;
	double framerate;
	int channels, sample_bits, samplerate;

        VFrame **device_buffers;
        int *buffer_valid;
        int streamon;
        int total_buffers;
public:
	DeviceMPEGInput(const char *name, int no);
	~DeviceMPEGInput();

	void reset();
	int open_input();
	int drop_frames(int frames);
	int read_buffer(VFrame *data);
	int read_audio(char *data, int samples);
	int video_width() { return width; }
	int video_height() { return height; }
	double video_framerate() { return framerate; }
	int audio_channels() { return channels; }
	int audio_sample_rate() { return samplerate; }
	int audio_sample_bits() { return sample_bits; }
	int total_video_streams() { return total_vstreams; }
	int total_audio_channels() { return total_achannels; }
	int get_channel() { return channel ? channel->entry : -1; }
	const char *channel_title() { return !channel ? "--" : channel->title; }
	int get_device_number() { return device_number; }
	zmpeg3_t *get_src() { return !src_stream() ? src : 0; }
	void put_src() { src_unlock(); }
	int colormodel();
	void set_device_number(int dev) { device_number = dev; }
	int set_channel(Channel *channel);
	void set_captioning(int mode);
	MWindow *get_mwindow();
	int create_channeldb(ArrayList<Channel*> *channeldb);
	int src_stream(Mutex *stream=0);
	zmpeg3_t *src_lock();
	void src_unlock();
	double audio_timestamp();
	double video_timestamp();
	int start_toc(const char *path, const char *toc_path);
	int start_record(int fd, int bsz);
	int stop_record();
	int subchannel_count();
	int subchannel_definition(int subchan, char *name,
	        int &major, int &minor, int &total_astreams, int &total_vstreams);
	int subchannel_video_stream(int subchan, int vstream);
	int subchannel_audio_stream(int subchan, int astream, char *enc=0);
        int get_video_pid(int track);
        int get_video_info(int track, int &pid, double &framerate,
                int &width, int &height, char *title=0);
        int get_thumbnail(int stream, int64_t &position,
                unsigned char *&thumbnail, int &ww, int &hh);
        int set_skimming(int track, int skim, skim_fn fn, void *vp);

	static DeviceMPEGInput* get_mpeg_video(VideoDevice *video_device,
       		DeviceMPEGInput* (*new_device)(const char *name, int no),
	        const char *name, int no);
	static DeviceMPEGInput* get_mpeg_audio(AudioDevice *audio_device,
       		DeviceMPEGInput* (*new_device)(const char *name, int no),
	        const char *name, int no);

	void put_mpeg_video();
	void put_mpeg_audio();
	void audio_reset() { audio_inited = 0; }
	void video_reset() { video_inited = 0; }
	virtual int has_signal() { return 1; }
};

#endif
