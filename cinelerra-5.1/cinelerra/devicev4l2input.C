
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

#include "bctimer.h"
#include "channel.h"
#include "chantables.h"
#include "condition.h"
#include "devicev4l2input.h"
#include "devicempeginput.h"
#include "picture.h"
#include "preferences.h"
#include "recordconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_VIDEO4LINUX2
#include <linux/videodev2.h>
#endif
#include <sys/ioctl.h>
#include <sys/mman.h>


DeviceV4L2Input::DeviceV4L2Input(const char *name, int no)
 : DeviceV4L2Base(), DeviceMPEGInput(name, no)
{
	device_channel = 0;
	status_v4l2 = new DeviceV4L2Status(this);
	status_v4l2->start();
}

DeviceMPEGInput *DeviceV4L2Input::
NewV4L2Input(const char *name, int no)
{
	return (DeviceMPEGInput *) new DeviceV4L2Input(name, no);
}

DeviceV4L2Input::~DeviceV4L2Input()
{
	delete status_v4l2;
}

DeviceMPEGInput* DeviceV4L2Input::get_mpeg_input(VideoDevice *device)
{
	DeviceMPEGInput *mpeg_device = get_mpeg_video(device, NewV4L2Input, "video", 0);
	if( mpeg_device )
	{
		device->channel->use_norm = 1;
		device->channel->use_input = 1;
		mpeg_device->set_device_number(0);
		Channel *channel = device->new_input_source((char*)"video0");
		channel->device_index = 0;
		channel->tuner = 0;
	}
	return mpeg_device;
}


DeviceMPEGInput* DeviceV4L2Input::get_mpeg_input(AudioDevice *device)
{
	DeviceMPEGInput *mpeg_device = get_mpeg_audio(device, NewV4L2Input, "video", 0);
	return mpeg_device;
}

int DeviceV4L2Input::mpeg_fd()
{
	return v4l2_fd();
}

int DeviceV4L2Input::open_dev(int color_model)
{
	return DeviceV4L2Base::open_dev(color_model);
}

void DeviceV4L2Input::close_dev()
{
	DeviceV4L2Base::close_dev();
}

int DeviceV4L2Input::status_dev()
{
	return DeviceV4L2Base::status_dev();
}

int DeviceV4L2Input::start_dev()
{
	return 0;
}

int DeviceV4L2Input::stop_dev()
{
	return 0;
}

VideoDevice *DeviceV4L2Input::v4l2_device()
{
	return video_device;
}


DeviceV4L2Status::DeviceV4L2Status(DeviceV4L2Input *in_v4l2)
 : Thread(1, 0, 0)
{
        this->in_v4l2 = in_v4l2;
}

DeviceV4L2Status::~DeviceV4L2Status()
{
        done = 1;
        Thread::cancel();
        Thread::join();
}

void DeviceV4L2Status::run()
{
        done = 0;
        Thread::enable_cancel();
        while( !done )
        {
		Thread::disable_cancel();
		in_v4l2->status_dev();
		Thread::enable_cancel();
		sleep(3);
	}
}

#endif
