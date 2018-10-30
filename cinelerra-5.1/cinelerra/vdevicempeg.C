
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

#include "asset.h"
#include "condition.h"
#include "file.inc"
#include "mwindow.h"
#include "record.h"
#include "videodevice.h"
#include "vdevicempeg.h"
#include "devicempeginput.h"


#include <unistd.h>
#include <stdint.h>
#include <string.h>

VDeviceMPEG::VDeviceMPEG(VideoDevice *device)
 : VDeviceBase(device)
{
	video_open = 0;
	mpeg_input = 0;
}

VDeviceMPEG::~VDeviceMPEG()
{
	close_all();
}

int VDeviceMPEG::open_input(char *name)
{
	if( video_open ) return 0;
	/* this is more like - try to open, since a broadcast signal may */
	/*  take time to lock, or not even be there at all.  On success, */
	/*  it updates the input config with the actual stream config. */
	if( !mpeg_input ) {
		mpeg_input = get_mpeg_input();
		if( !mpeg_input ) return 1;
		device->channel->has_subchan = 1;
		device->channel->has_scanning = 1;
		device->channel->use_frequency = 1;
		if( name != 0 )
		{
			Channel *channel = device->new_input_source(name);
			channel->device_index = mpeg_input->get_device_number();
			channel->tuner = 0;
		}
	}
	int ret = 1;
	if( !mpeg_input->src_stream() ) {
		int width = mpeg_input->video_width();
		int height = mpeg_input->video_height();
		double rate = mpeg_input->video_framerate();
		device->auto_update(rate, width, height);
		device->capturing = 1;
		video_open = 1;
		mpeg_input->src_unlock();
		ret = 0;
	}
	return ret;
}

int VDeviceMPEG::close_all()
{
	video_open = 0;
	if( mpeg_input ) {
		mpeg_input->put_mpeg_video();
		mpeg_input = 0;
	}
	return 0;
}

int VDeviceMPEG::read_buffer(VFrame *frame)
{
	if( !video_open && open_input() ) return 1;
	int result = mpeg_input->read_buffer(frame);
	return result;
}

int VDeviceMPEG::get_best_colormodel(Asset *asset)
{
	int result = mpeg_input ? mpeg_input->colormodel() : BC_YUV422P;
	return result;
}

int VDeviceMPEG::drop_frames(int frames)
{
	return !mpeg_input ? 1 : mpeg_input->drop_frames(frames);
}

double VDeviceMPEG::device_timestamp()
{
	if( !video_open || !mpeg_input ) return -1.;
	return mpeg_input->video_timestamp();
}

int VDeviceMPEG::start_toc(const char *path, const char *toc_path)
{
	return !mpeg_input ? -1 : mpeg_input->start_toc(path, toc_path);
}

int VDeviceMPEG::start_record(int fd, int bsz)
{
	return !mpeg_input ? -1 : mpeg_input->start_record(fd, bsz);
}

int VDeviceMPEG::stop_record()
{
	return !mpeg_input ? -1 : mpeg_input->stop_record();
}

int VDeviceMPEG::total_video_streams()
{
	return !mpeg_input ? -1 : mpeg_input->total_video_streams();
}


int VDeviceMPEG::set_channel(Channel *channel)
{
	device->channel_changed = 0;
	return !mpeg_input ? -1 : mpeg_input->set_channel(channel);
}

int VDeviceMPEG::set_captioning(int mode)
{
	return !mpeg_input ? -1 : (mpeg_input->set_captioning(mode), 0);
}

int VDeviceMPEG::has_signal()
{
	return !mpeg_input ? 0 : mpeg_input->has_signal();
}


int VDeviceMPEG::create_channeldb(ArrayList<Channel*> *channeldb)
{
	return !mpeg_input ? 1 : mpeg_input->
		create_channeldb(channeldb);
}


int VDeviceMPEG::get_video_pid(int track)
{
	return !mpeg_input ? 1 : mpeg_input->get_video_pid(track);
}

int VDeviceMPEG::get_video_info(int track, int &pid,
		double &framerate, int &width, int &height, char *title)
{
	return !mpeg_input ? 1 : mpeg_input->
		get_video_info(track, pid, framerate, width, height, title);
}

int VDeviceMPEG::get_thumbnail(int stream, int64_t &position,
		unsigned char *&thumbnail, int &ww, int &hh)
{
	return !mpeg_input ? 1 : mpeg_input->
		get_thumbnail(stream, position, thumbnail, ww, hh);
}

int VDeviceMPEG::set_skimming(int track, int skim, skim_fn fn, void *vp)
{
	return !mpeg_input ? 1 : mpeg_input->
		set_skimming(track, skim, fn, vp);
}

