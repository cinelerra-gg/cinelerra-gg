
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

#ifdef HAVE_VIDEO4LINUX2

#include "asset.h"
#include "condition.h"
#include "file.inc"
#include "mwindow.h"
#include "record.h"
#include "vdevicev4l2mpeg.h"
#include "videodevice.h"
#include "devicev4l2input.h"


#include <unistd.h>
#include <stdint.h>
#include <string.h>

VDeviceV4L2MPEG::VDeviceV4L2MPEG(VideoDevice *device)
 : VDeviceMPEG(device)
{
}

VDeviceV4L2MPEG::~VDeviceV4L2MPEG()
{
}

DeviceMPEGInput *VDeviceV4L2MPEG::get_mpeg_input()
{
	return DeviceV4L2Input::get_mpeg_input(device);
}

int VDeviceV4L2MPEG::open_input()
{
	if( !video_open ) {
		if( !mpeg_input ) {
			mpeg_input = get_mpeg_input();
			if( !mpeg_input ) return 1;
		}
		mpeg_input->video_reset();
		DeviceV4L2Input *v4l2_input = (DeviceV4L2Input *)mpeg_input;
		v4l2_input->get_sources();
		video_open = 1;
	}
	return 0;
}

#endif
