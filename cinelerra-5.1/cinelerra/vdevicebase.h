
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

#ifndef VDEVICEBASE_H
#define VDEVICEBASE_H
#include <sys/time.h>

#include "asset.inc"
#include "assets.inc"
#include "channel.inc"
#include "devicempeginput.inc"
#include "edl.inc"
#include "file.inc"
#include "guicast.h"
#include "picture.inc"
#include "videodevice.inc"

class VDeviceBase
{
public:
	VDeviceBase(VideoDevice *device);
	virtual ~VDeviceBase();

	virtual int open_input() { return 1; };
	virtual int close_all() { return 1; };
	virtual int has_signal() { return 0; };
	virtual int create_channeldb(ArrayList<Channel*> *channeldb) { return 1; };
	virtual int read_buffer(VFrame *frame) { return 1; };
	virtual int write_buffer(VFrame *output, EDL *edl) { return 1; };
	virtual void new_output_buffer(VFrame **output, int colormodel, EDL *edl) {};
	virtual ArrayList<int>* get_render_strategies() { return 0; };
	virtual int get_shared_data(unsigned char *data, long size) { return 0; };
	virtual int stop_sharing() { return 0; };
	virtual int interrupt_crash() { return 0; };
// Extra work must sometimes be done in here to set up the device.
	virtual int get_best_colormodel(Asset *asset);
	virtual int drop_frames(int frames) { return 1; };
	virtual int set_channel(Channel *channel) { return 0; };
	virtual int set_captioning(int mode) { return 0; };
	virtual int set_picture(PictureConfig *picture) { return 0; };
	virtual double device_timestamp() {
		struct timeval tv;  gettimeofday(&tv, 0);
		return tv.tv_sec + tv.tv_usec / 1000000.0;
	}
	virtual int start_toc(const char *path, const char *toc_path) { return -1; }
	virtual int start_record(int fd, int bsz=0) { return -1; }
	virtual int stop_record() { return -1; }
	virtual int pause_input_device(int v) { return -1; }
	virtual int total_video_streams() { return 1; }
	virtual DeviceMPEGInput *mpeg_device() { return 0; }

	virtual int open_output() { return 1; };
	virtual int output_visible() { return 0; };
	virtual int start_playback() { return 1; };
	virtual int stop_playback() { return 1; };
	virtual BC_Bitmap* get_bitmap() { return 0; };
// Most Linux video drivers don't work.
// Called by KeepaliveThread when the device appears to be stuck.
// Should restart the device if that's what it takes to get it to work.
	virtual void goose_input() {};

// Called by Record::run to fix compression for certain devices.
// Not saved as default asset.
	virtual void fix_asset(Asset *asset) {};
// skimming
	virtual int get_video_pid(int track) { return -1; }
        virtual int get_video_info(int track, int &pid, double &framerate,
                int &width, int &height, char *title=0) { return -1; }
        virtual int get_thumbnail(int stream, int64_t &position,
                unsigned char *&thumbnail, int &ww, int &hh) { return -1; }
        virtual int set_skimming(int track, int skim, skim_fn fn, void *vp) { return -1; }

	VideoDevice *device;
};

#endif
