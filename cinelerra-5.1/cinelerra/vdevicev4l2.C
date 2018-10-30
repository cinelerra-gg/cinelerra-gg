
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

#include "channel.h"
#include "file.h"
#include "preferences.h"
#include "recordconfig.h"
#include "vdevicev4l2.h"
#include "vframe.h"
#include "videodevice.h"



VDeviceV4L2::VDeviceV4L2(VideoDevice *device)
 : VDeviceBase(device), DeviceV4L2Base()
{
}

VDeviceV4L2::~VDeviceV4L2()
{
	close_all();
}

int VDeviceV4L2::close_all()
{
	close_dev();
	return 0;
}

int VDeviceV4L2::open_input()
{
	int result = get_sources();
	if( !result )
	{
		ArrayList<Channel*> *inputs = device->get_inputs();
		for( int i=0; i<inputs->size(); ++i ) {
			if( inputs->get(i)->tuner ) {
				device->channel->use_frequency = 1;
				device->channel->use_fine = 1;
			}
			else
				device->channel->use_input = 1;
		}
	}
	return result;
}

int VDeviceV4L2::get_best_colormodel(Asset *asset)
{
	int result = // BC_RGB888;
		File::get_best_colormodel(asset, device->in_config->driver);
	return result;
}

int VDeviceV4L2::has_signal()
{
	return !status_dev() ? dev_signal() : 0;
}

int VDeviceV4L2::read_buffer(VFrame *frame)
{
	int result = 0;

	if(is_opened())
	{
		if(device->channel_changed || device->picture_changed)
			close_dev();
	}

	if(!is_opened())
	{
		result = open_dev(frame->get_color_model());
		if( !result )
		{
			device->channel_changed = 0;
			device->picture_changed = 0;
		}
	}

// Get buffer from thread
	if( !result )
	{
		int bfr = get_buffer();
		if(bfr >= 0)
		{
			VFrame *buffer = device_buffer(bfr);
//printf("VDeviceV4L2::read_buffer %d %p %p\n",
//  __LINE__, frame->get_data(), buffer->get_data());
			frame->transfer_from(buffer);
			put_buffer(bfr);
		}
		else
			result = 1;
	}

	if( result )
		close_dev();

	return result;
}

#endif
