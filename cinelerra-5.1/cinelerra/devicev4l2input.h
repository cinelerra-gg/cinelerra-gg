
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

#ifndef DEVICEV4L2INPUT_H
#define DEVICEV4L2INPUT_H

#ifdef HAVE_VIDEO4LINUX2

#include "devicev4l2base.h"
#include "devicev4l2input.inc"
#include "devicempeginput.h"
#include "condition.h"
#include "audiodevice.h"
#include "videodevice.h"

#include <stdint.h>
#include "thread.h"
#include "libzmpeg3.h"

// stubs
class VDeviceV4L2;
class AudioV4L2;

class DeviceV4L2Status : public Thread
{
	int done;
	DeviceV4L2Input *in_v4l2;
public:
	DeviceV4L2Status(DeviceV4L2Input *in_v4l2);
	~DeviceV4L2Status();
	void run();
};

class DeviceV4L2Input : public DeviceV4L2Base, public DeviceMPEGInput
{
	friend class DeviceV4L2Status;
	DeviceV4L2Status *status_v4l2;
	Channel *device_channel;
public:
	DeviceV4L2Input(const char *name, int no);
	static DeviceMPEGInput *NewV4L2Input(const char *name, int no);
	~DeviceV4L2Input();

	static DeviceMPEGInput* get_mpeg_input(VideoDevice *device);
	static DeviceMPEGInput* get_mpeg_input(AudioDevice *device);
	void put_v4l2_video() { put_mpeg_video(); }
	void put_v4l2_audio() { put_mpeg_audio(); }

	int mpeg_fd();
	int open_dev(int color_model);
	void close_dev();
	int status_dev();
	int start_dev();
	int stop_dev();

	VideoDevice *v4l2_device();

	VDeviceV4L2 *v4l2_video() {
		return video_device ? (VDeviceV4L2 *)video_device->input_base : 0;
	}
	AudioV4L2 *v4l2_audio() {
		return audio_device ? (AudioV4L2 *)audio_device->lowlevel_in : 0;
	}
};

#endif
#endif
