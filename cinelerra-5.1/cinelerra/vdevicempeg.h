
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

#ifndef VDEVICEMPEG_H
#define VDEVICEMPEG_H

#include "condition.inc"
#include "mutex.inc"
#include "vdevicebase.h"
//#include <linux/types.h>
#ifdef HAVE_VIDEO4LINUX2
#include <linux/videodev2.h>
#endif
#include "devicempeginput.inc"
#include "videodevice.inc"
#include "vdevicempeg.inc"



class VDeviceMPEG : public VDeviceBase
{
public:
	VDeviceMPEG(VideoDevice *device);
	~VDeviceMPEG();

	virtual DeviceMPEGInput *get_mpeg_input() = 0;

	int initialize();
	int open_input(char *name=0);
	int close_all();
	int get_best_colormodel(Asset *asset);
	int read_buffer(VFrame *frame);

	int drop_frames(int frames);
	double device_timestamp();
	int start_toc(const char *path, const char *toc_path);
	int start_record(int fd, int bsz);
	int stop_record();
	int total_video_streams();
	int set_channel(Channel *channel);
	int set_captioning(int mode);
	int has_signal();
	int create_channeldb(ArrayList<Channel*> *channeldb);
	DeviceMPEGInput *mpeg_device() { return mpeg_input; }

        int get_video_pid(int track);
        int get_video_info(int track, int &pid, double &framerate,
                int &width, int &height, char *title);
        int get_thumbnail(int stream, int64_t &position,
                unsigned char *&thumbnail, int &ww, int &hh);
        int set_skimming(int track, int skim, skim_fn fn, void *vp);

	int video_open;
	DeviceMPEGInput *mpeg_input;
};

#endif


