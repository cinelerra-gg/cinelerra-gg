
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
#include "libmjpeg.h"
#include "preferences.h"
#include "recordconfig.h"
#include "vdevicev4l2jpeg.h"
#include "vframe.h"
#include "videodevice.h"

#include <string.h>

VDeviceV4L2JPEG::VDeviceV4L2JPEG(VideoDevice *device)
 : VDeviceBase(device), DeviceV4L2Base()
{
}

VDeviceV4L2JPEG::~VDeviceV4L2JPEG()
{
	close_all();
}

int VDeviceV4L2JPEG::close_all()
{
	close_dev();
	return 0;
}

int VDeviceV4L2JPEG::open_input()
{
	int result = get_sources();
	return result;
}

int VDeviceV4L2JPEG::get_best_colormodel(Asset *asset)
{
	return BC_COMPRESSED;
}

int VDeviceV4L2JPEG::has_signal()
{
	return !status_dev() ? dev_signal() : 0;
}

int VDeviceV4L2JPEG::read_buffer(VFrame *frame)
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
			frame->set_timestamp(buffer->get_timestamp());
			frame->allocate_compressed_data(buffer->get_compressed_size());
			frame->set_compressed_size(buffer->get_compressed_size());

// Transfer fields to frame
			if(device->odd_field_first)
			{
				int field2_offset = mjpeg_get_field2((unsigned char*)buffer->get_data(),
					buffer->get_compressed_size());
				int field1_len = field2_offset;
				int field2_len = buffer->get_compressed_size() -
					field2_offset;

				memcpy(frame->get_data(),
					buffer->get_data() + field2_offset,
					field2_len);
				memcpy(frame->get_data() + field2_len,
					buffer->get_data(),
					field1_len);
			}
			else
			{
				bcopy(buffer->get_data(),
					frame->get_data(),
					buffer->get_compressed_size());
			}

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
