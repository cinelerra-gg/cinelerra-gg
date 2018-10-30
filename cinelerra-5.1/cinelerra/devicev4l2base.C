
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
#include "clip.h"
#include "condition.h"
#include "devicev4l2base.h"
#include "mutex.h"
#include "picture.h"
#include "preferences.h"
#include "recordconfig.h"
#include "videodevice.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>


DeviceV4L2BufferQ::
DeviceV4L2BufferQ(int qsize)
{
	reset();
	bfr_lock = new Mutex("DeviceV4L2BufferQ::bfr_lock");
	buffers = new int[limit=qsize];
}

DeviceV4L2BufferQ::
~DeviceV4L2BufferQ()
{
	delete [] buffers;
	delete bfr_lock;
}

int DeviceV4L2BufferQ::q(int bfr)
{
	int result = 0;
	bfr_lock->lock("DeviceV4L2Buffer::q");
	int in1 = in+1;
	if( in1 >= limit ) in1 = 0;
	if( in1 != out )
	{
		buffers[in] = bfr;
		in = in1;
	}
	else
		result = -1;
	bfr_lock->unlock();
	return result;
}

int DeviceV4L2BufferQ::dq(Condition *time_lock, int time_out)
{
	int result = -1;
	bfr_lock->lock(" DeviceV4L2Buffer::dq 0");
	time_lock->reset();
	if( in == out )
	{
		bfr_lock->unlock();
		if( time_lock->timed_lock(time_out, "DeviceV4L2Buffer::dq 1") )
			printf("DeviceV4L2Buffer::wait time_out\n");
		bfr_lock->lock(" DeviceV4L2Buffer::dq 2");
	}
	if( in != out )
	{
		result = buffers[out];
		int out1 = out+1;
		if( out1 >= limit ) out1 = 0;
		out = out1;
	}
	bfr_lock->unlock();
	return result;
}

int DeviceV4L2BufferQ::dq()
{
	int result = -1;
	bfr_lock->lock(" DeviceV4L2Buffer::dq");
	if( in != out )
	{
		result = buffers[out];
		int out1 = out+1;
		if( out1 >= limit ) out1 = 0;
		out = out1;
	}
	bfr_lock->unlock();
	return result;
}



DeviceV4L2Base::DeviceV4L2Base()
 : Thread(1, 0, 0)
{
	v4l2_lock = new Mutex("DeviceV4L2Input::v4l2_lock", 0);
	qbfrs_lock = new Mutex("DeviceV4L2Base::qbfrs_lock");
	video_lock = new Condition(0, "DeviceV4L2Base::video_lock", 1);
	dev_fd = -1;
	iwidth = 0;
	iheight = 0;
	color_model = -1;
	device_buffers = 0;
	device_channel = 0;
	total_buffers = 0;
	streamon = 0;
	dev_status = 0;
	opened = 0;
	q_bfrs = 0;
	done = 0;
	put_thread = 0;
}

DeviceV4L2Base::~DeviceV4L2Base()
{
	close_dev();
	delete video_lock;
	delete qbfrs_lock;
	delete v4l2_lock;
}



DeviceV4L2Put::DeviceV4L2Put(DeviceV4L2Base *v4l2)
 : Thread(1, 0, 0)
{
	this->v4l2 = v4l2;
	done = 0;
	putq = new DeviceV4L2BufferQ(256);
	buffer_lock = new Condition(0, "DeviceV4L2Put::buffer_lock");
}

DeviceV4L2Put::~DeviceV4L2Put()
{
	if(Thread::running())
	{
		done = 1;
		buffer_lock->unlock();
		Thread::cancel();
	}
	Thread::join();
	delete buffer_lock;
	delete putq;
}


void DeviceV4L2Put::run()
{
	while(!done)
	{
		buffer_lock->lock("DeviceV4L2Put::run");
		if(done) break;
		int bfr = putq->dq();
		if( bfr < 0 ) continue;
		struct v4l2_buffer arg;
		memset(&arg, 0, sizeof(arg));
		arg.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		arg.index = bfr;
		arg.memory = V4L2_MEMORY_MMAP;

		Thread::enable_cancel();
		v4l2->qbfrs_lock->lock("DeviceV4L2Put::run");
// This locks up if there's no signal.
		if( v4l2->vioctl(VIDIOC_QBUF, &arg) < 0 )
			perror("DeviceV4L2Put::run 1 VIDIOC_QBUF");
		else
			++v4l2->q_bfrs;
		v4l2->qbfrs_lock->unlock();
		Thread::disable_cancel();
	}
}




int DeviceV4L2Base::get_sources()
{
	v4l2_lock->lock("DeviceV4L2Base::get_sources");
	VideoDevice *video_device = v4l2_device();
	if( !video_device ) return 1;
	const char *dev_path = video_device->in_config->get_path();
	int vfd = open(dev_path, O_RDWR+O_CLOEXEC);
	if(vfd < 0)
	{
		perror("DeviceV4L2Base::get_sources open failed");
		printf("DeviceV4L2Base::get_sources path: %s\n", dev_path);
		return 1;
	}

// Get the Inputs

	for(int i = 0; i < 20; ++i)
	{
		struct v4l2_input arg;
		memset(&arg, 0, sizeof(arg));
		arg.index = i;

		if(ioctl(vfd, VIDIOC_ENUMINPUT, &arg) < 0) break;
		Channel *channel = video_device->new_input_source((char*)arg.name);
		channel->device_index = i;
		channel->tuner = arg.type == V4L2_INPUT_TYPE_TUNER ? arg.tuner : -1;
	}

// Get the picture controls
	for(int i = V4L2_CID_BASE; i < V4L2_CID_LASTP1; i++)
	{
		struct v4l2_queryctrl arg;
		memset(&arg, 0, sizeof(arg));
		arg.id = i;
// This returns errors for unsupported controls which is what we want.
		if(!ioctl(vfd, VIDIOC_QUERYCTRL, &arg))
		{
// Test if control exists already
			if(!video_device->picture->get_item((const char*)arg.name, arg.id))
			{
// Add control
				PictureItem *item = video_device->picture->new_item((const char*)arg.name);
				item->device_id = arg.id;
				item->min = arg.minimum;
				item->max = arg.maximum;
				item->step = arg.step;
				item->default_ = arg.default_value;
				item->type = arg.type;
				item->value = arg.default_value;
			}
		}
	}

// Load defaults for picture controls
	video_device->picture->load_defaults();

	close(vfd);
	v4l2_lock->unlock();
	return 0;
}

int DeviceV4L2Base::vioctl(unsigned long int req, void *arg)
{
	int result = ioctl(dev_fd, req, arg);
	return result;
}

int DeviceV4L2Base::v4l2_open(int color_model)
{
	VideoDevice *video_device = v4l2_device();
	if( !video_device ) return 1;

	struct v4l2_capability cap;
	memset(&cap, 0, sizeof(cap));
	if(vioctl(VIDIOC_QUERYCAP, &cap))
		perror("DeviceV4L2Base::v4l2_open VIDIOC_QUERYCAP");

// printf("DeviceV4L2Base::v4l2_open dev_fd=%d driver=%s card=%s bus_info=%s version=%d\n",
// dev_fd(), cap.driver, cap.card, cap.bus_info, cap.version);
// printf("    ");
// if( (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ) printf("VIDEO_CAPTURE ");
// if( (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)  ) printf("VIDEO_OUTPUT ");
// if( (cap.capabilities & V4L2_CAP_VIDEO_OVERLAY) ) printf("VIDEO_OVERLAY ");
// if( (cap.capabilities & V4L2_CAP_VBI_CAPTURE)   ) printf("VBI_CAPTURE ");
// if( (cap.capabilities & V4L2_CAP_VBI_OUTPUT)    ) printf("VBI_OUTPUT ");
// if( (cap.capabilities & V4L2_CAP_RDS_CAPTURE)   ) printf("RDS_CAPTURE ");
// if( (cap.capabilities & V4L2_CAP_TUNER)         ) printf("TUNER ");
// if( (cap.capabilities & V4L2_CAP_AUDIO)         ) printf("AUDIO ");
// if( (cap.capabilities & V4L2_CAP_RADIO)         ) printf("RADIO ");
// if( (cap.capabilities & V4L2_CAP_READWRITE)     ) printf("READWRITE ");
// if( (cap.capabilities & V4L2_CAP_ASYNCIO)       ) printf("ASYNCIO ");
// if( (cap.capabilities & V4L2_CAP_STREAMING)     ) printf("STREAMING ");
// printf("\n");

	int config_width = video_device->in_config->w;
	int config_height = video_device->in_config->h;
	int config_area = config_width * config_height;
	int best_width = config_width;
	int best_height = config_height;
	unsigned int best_format = 0;
	int best_merit = 0;
	int best_color_model = -1;
	int best_area = INT_MAX;
	int driver = video_device->in_config->driver;

	for( int i=0; i<20; ++i ) {
		struct v4l2_fmtdesc fmt;
		memset(&fmt,0,sizeof(fmt));
		fmt.index = i;
		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if( vioctl(VIDIOC_ENUM_FMT,&fmt) != 0 ) break;
		printf("DeviceV4L2Base::v4l2_open");
		// printf("format=\"%s\";",&fmt.description[0]);
		printf(" pixels=\"%4.4s\"; res=\"",(char*)&fmt.pixelformat);

		int merit = 0;
		int cmodel = -1;
	        switch(fmt.pixelformat)
		{
		case V4L2_PIX_FMT_YUYV:    cmodel = BC_YUV422;  merit = 4;  break;
		case V4L2_PIX_FMT_UYVY:    cmodel = BC_UVY422;  merit = 4;  break;
		case V4L2_PIX_FMT_Y41P:    cmodel = BC_YUV411P; merit = 1;  break;
		case V4L2_PIX_FMT_YVU420:  cmodel = BC_YUV420P; merit = 3;  break;
		case V4L2_PIX_FMT_YUV422P: cmodel = BC_YUV422P; merit = 4;  break;
		case V4L2_PIX_FMT_RGB24:   cmodel = BC_RGB888;  merit = 6;  break;
		case V4L2_PIX_FMT_MJPEG:
			cmodel = BC_COMPRESSED;
			merit = driver == VIDEO4LINUX2JPEG ||
				driver == CAPTURE_JPEG_WEBCAM ? 7 : 0;
			break;
		case V4L2_PIX_FMT_MPEG:
			cmodel = BC_YUV420P;
			merit = driver == VIDEO4LINUX2MPEG ? 7 : 2;
			break;
		}
		if( cmodel >= 0 && merit > best_merit )
		{
			best_merit = merit;
			best_format = fmt.pixelformat;
			best_color_model = cmodel;
		}

		for( int n=0; n<20; ++n ) {
			struct v4l2_frmsizeenum fsz;
			memset(&fsz,0,sizeof(fsz));
			fsz.index = n;
			fsz.pixel_format = fmt.pixelformat;
			if( vioctl(VIDIOC_ENUM_FRAMESIZES,&fsz) != 0 ) break;
			int w = fsz.discrete.width, h = fsz.discrete.height;
			if( n > 0 ) printf(" ");
			printf("%dx%d",w,h);
			int area = w * h;
			if( area > config_area ) continue;
			int diff = abs(area-config_area);
			if( diff < best_area ) {
				best_area = diff;
				best_width = w;
				best_height = h;
			}
		}
		printf("\"\n");
	}

	if( !best_format )
	{
		printf("DeviceV4L2Base::v4l2_open cant determine best_format\n");
		switch(driver)
		{
		case VIDEO4LINUX2JPEG:
			best_format = V4L2_PIX_FMT_MJPEG;
			break;
		case VIDEO4LINUX2MPEG:
			best_format = V4L2_PIX_FMT_MPEG;
			break;
		default:
			best_color_model = color_model;
			best_format = cmodel_to_device(color_model);
			break;
		}
		printf("DeviceV4L2Base::v4l2_open ");
		printf(_(" attempting format %4.4s\n"),
			(char *)&best_format);
	}
	if(driver == VIDEO4LINUX2JPEG && best_format != V4L2_PIX_FMT_MJPEG)
	{
		printf("DeviceV4L2Base::v4l2_open ");
		printf(_("jpeg driver and best_format not mjpeg (%4.4s)\n"),
			(char *)&best_format);
		return 1;
	}
	if(driver == VIDEO4LINUX2MPEG && best_format != V4L2_PIX_FMT_MPEG)
	{
		printf("DeviceV4L2Base::v4l2_open ");
		printf(_("mpeg driver and best_format not mpeg (%4.4s)\n"),
			(char *)&best_format);
		return 1;
	}
	if(config_width != best_width || config_height != best_height)
	{
		printf("DeviceV4L2Base::v4l2_open ");
		printf(_("config geom %dx%d != %dx%d best_geom\n"),
			config_width, config_height, best_width, best_height);
	}

// Set up frame rate
	struct v4l2_streamparm v4l2_parm;
	memset(&v4l2_parm, 0, sizeof(v4l2_parm));
	v4l2_parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(vioctl(VIDIOC_G_PARM, &v4l2_parm) < 0)
		perror("DeviceV4L2Base::v4l2_open VIDIOC_G_PARM");
	if(v4l2_parm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
	{
		v4l2_parm.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;
		v4l2_parm.parm.capture.timeperframe.numerator = 1;
		v4l2_parm.parm.capture.timeperframe.denominator =
			(unsigned long)((float)1 /
			video_device->frame_rate * 10000000);
		if(vioctl(VIDIOC_S_PARM, &v4l2_parm) < 0)
			perror("DeviceV4L2Base::v4l2_open VIDIOC_S_PARM");

		if(vioctl(VIDIOC_G_PARM, &v4l2_parm) < 0)
			perror("DeviceV4L2Base::v4l2_open VIDIOC_G_PARM");
	}

	printf("v4l2 s_fmt %dx%d %4.4s\n", best_width, best_height, (char*)&best_format);
// Set up data format
	struct v4l2_format v4l2_params;
	memset(&v4l2_params, 0, sizeof(v4l2_params));
	v4l2_params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(vioctl(VIDIOC_G_FMT, &v4l2_params) < 0)
		perror("DeviceV4L2Base::v4l2_open VIDIOC_G_FMT 1");
	v4l2_params.fmt.pix.width = best_width;
	v4l2_params.fmt.pix.height = best_height;
	v4l2_params.fmt.pix.pixelformat = best_format;
	if(vioctl(VIDIOC_S_FMT, &v4l2_params) < 0)
		perror("DeviceV4L2Base::v4l2_open VIDIOC_S_FMT");
	memset(&v4l2_params, 0, sizeof(v4l2_params));
	v4l2_params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(vioctl(VIDIOC_G_FMT, &v4l2_params) < 0)
		perror("DeviceV4L2Base::v4l2_open VIDIOC_G_FMT 2");
	if( v4l2_params.fmt.pix.pixelformat != best_format )
	{
		printf("DeviceV4L2Base::v4l2_open  set format %4.4s != %4.4s best_format\n",
			 (char*)&v4l2_params.fmt.pix.pixelformat, (char*)&best_format);
		return 1;
	}
	iwidth = v4l2_params.fmt.pix.width;
	iheight = v4l2_params.fmt.pix.height;
	if( iwidth != best_width || iheight != best_height )
	{
		printf("DeviceV4L2Base::v4l2_open  set geom %dx%d != %dx%d best_geom\n",
			iwidth, iheight, best_width, best_height);
		return 1;
	}

// Set picture controls.  This driver requires the values to be set once to default
// values and then again to different values before it takes up the values.
// Unfortunately VIDIOC_S_CTRL resets the audio to mono in 2.6.7.
	PictureConfig *picture = video_device->picture;
	for(int i = 0; i < picture->controls.total; i++)
	{
		struct v4l2_queryctrl arg;
		memset(&arg, 0, sizeof(arg));
		PictureItem *item = picture->controls.values[i];
		arg.id = item->device_id;
		if(!vioctl(VIDIOC_QUERYCTRL, &arg))
		{
			struct v4l2_control ctrl_arg;
			memset(&ctrl_arg, 0, sizeof(ctrl_arg));
			ctrl_arg.id = item->device_id;
			ctrl_arg.value = 0;
			if(vioctl(VIDIOC_S_CTRL, &ctrl_arg) < 0)
				perror("DeviceV4L2Base::v4l2_open VIDIOC_S_CTRL");
		}
		else
		{
			printf("DeviceV4L2Base::v4l2_open VIDIOC_S_CTRL 1 %s/id %08x failed\n",
				item->name, item->device_id);
		}
	}


	for(int i = 0; i < picture->controls.total; i++)
	{
		struct v4l2_queryctrl arg;
		memset(&arg, 0, sizeof(arg));
		PictureItem *item = picture->controls.values[i];
		arg.id = item->device_id;

		if(!vioctl(VIDIOC_QUERYCTRL, &arg))
		{
			struct v4l2_control ctrl_arg;
			memset(&ctrl_arg, 0, sizeof(ctrl_arg));
			ctrl_arg.id = item->device_id;
			ctrl_arg.value = item->value;
			if(vioctl(VIDIOC_S_CTRL, &ctrl_arg) < 0)
				perror("DeviceV4L2Base::v4l2_open VIDIOC_S_CTRL");
		}
		else
		{
			printf("DeviceV4L2Base::v4l2_open VIDIOC_S_CTRL 2 %s/id %08x failed\n",
				item->name, item->device_id);
		}
	}


// Set input
	Channel *video_channel = video_device->channel;
	ArrayList<Channel*> *inputs = video_device->get_inputs();
	int input = video_channel->input;
	device_channel = input >= 0 && input < inputs->total ?
		inputs->values[input] : 0;
	if(!device_channel)
	{
// Try first input
		if(inputs->total)
		{
			device_channel = inputs->values[0];
			printf("DeviceV4L2Base::v4l2_open user channel not found.  Using %s\n",
				device_channel->device_name);
		}
		else
		{
			printf("DeviceV4L2Base::v4l2_open channel \"%s\" not found.\n",
				video_channel->title);
		}
	}

// Translate input to API structures
	input = device_channel ? device_channel->device_index : 0;
	if(vioctl(VIDIOC_S_INPUT, &input) < 0)
		perror("DeviceV4L2Base::v4l2_open VIDIOC_S_INPUT");

// Set norm
	v4l2_std_id std_id = 0;
	switch(video_channel->norm)
	{
		case NTSC: std_id = V4L2_STD_NTSC; break;
		case PAL: std_id = V4L2_STD_PAL; break;
		case SECAM: std_id = V4L2_STD_SECAM; break;
		default: std_id = V4L2_STD_NTSC_M; break;
	}
	if(vioctl(VIDIOC_S_STD, &std_id))
		perror("DeviceV4L2Base::v4l2_open VIDIOC_S_STD");

	int dev_tuner = device_channel ? device_channel->tuner : -1;
	if( (cap.capabilities & V4L2_CAP_TUNER) && dev_tuner >= 0 )
	{
		struct v4l2_tuner tuner;
		memset(&tuner, 0, sizeof(tuner));
		tuner.index = dev_tuner;
		if(!vioctl(VIDIOC_G_TUNER, &tuner))
		{
// printf("DeviceV4L2Base::v4l2_open audmode=%d rxsubchans=%d\n",
//   tuner.audmode, tuner.rxsubchans);
			tuner.index = dev_tuner;
			tuner.type = V4L2_TUNER_ANALOG_TV;
			tuner.audmode = V4L2_TUNER_MODE_STEREO;
			tuner.rxsubchans = V4L2_TUNER_SUB_STEREO;
			if(vioctl(VIDIOC_S_TUNER, &tuner) < 0)
				perror("DeviceV4L2Base::v4l2_open VIDIOC_S_TUNER");
// Set frequency
			struct v4l2_frequency frequency;
			memset(&frequency, 0, sizeof(frequency));
			frequency.tuner = video_channel->tuner;
			frequency.type = V4L2_TUNER_ANALOG_TV;
			int table = video_channel->freqtable;
			int entry = video_channel->entry;
			frequency.frequency = (int)(chanlists[table].list[entry].freq * 0.016);
// printf("DeviceV4L2Base::v4l2_open tuner=%d freq=%d norm=%d\n",
//   video_channel->tuner, frequency.frequency, video_channel->norm);
			if(vioctl(VIDIOC_S_FREQUENCY, &frequency) < 0)
				perror("DeviceV4L2Base::v4l2_open VIDIOC_S_FREQUENCY");
		}
		else
			perror("DeviceV4L2Base::v4l2_open VIDIOC_G_TUNER");
	}

// Set compression
	if(color_model == BC_COMPRESSED)
	{
		struct v4l2_jpegcompression jpeg_arg;
		memset(&jpeg_arg, 0, sizeof(jpeg_arg));
		if(vioctl(VIDIOC_G_JPEGCOMP, &jpeg_arg) < 0)
			perror("DeviceV4L2Base::v4l2_open VIDIOC_G_JPEGCOMP");
		jpeg_arg.quality = video_device->quality / 2;
		if(vioctl(VIDIOC_S_JPEGCOMP, &jpeg_arg) < 0)
			perror("DeviceV4L2Base::v4l2_open VIDIOC_S_JPEGCOMP");
	}

	this->color_model = best_color_model;
	return 0;
}

int DeviceV4L2Base::v4l2_status()
{
	int result = 1;
	Channel *channel = device_channel;
	if( !channel )
	{
		dev_status = 0;
	}
	else if( channel->tuner >= 0 )
	{
		struct v4l2_tuner tuner;
		memset(&tuner,0,sizeof(tuner));
		tuner.index = channel->tuner;
		if( !vioctl(VIDIOC_G_TUNER, &tuner) )
		{
			dev_status = tuner.signal ? 1 : 0;
			result = 0;
		}
		else
			perror("DeviceV4L2Base::v4l2_status VIDIOC_S_TUNER");
       	}
	else
	{
		struct v4l2_input arg;
		memset(&arg, 0, sizeof(arg));
		arg.index = channel->device_index;
		if( !vioctl(VIDIOC_ENUMINPUT, &arg) )
		{
			dev_status = (arg.status &
			(V4L2_IN_ST_NO_POWER | V4L2_IN_ST_NO_SIGNAL)) ? 0 : 1;
			result = 0;
		}
		else
			perror("DeviceV4L2Base::v4l2_status VIDIOC_ENUMINPUT");
        }
	return result;
}

void DeviceV4L2Base::dev_close()
{
	if( dev_fd >= 0 )
	{
		::close(dev_fd);
		dev_fd = -1;
	}
}

int DeviceV4L2Base::dev_open()
{
	if( dev_fd < 0 )
	{
		VideoDevice *video_device = v4l2_device();
		if( !video_device )
		{
			printf("DeviceV4L2Base::dev_open: video_device not available\n");
			Timer::delay(250);
			return -1;
		}
		const char *dev_path = video_device->in_config->get_path();
		dev_fd = open(dev_path, O_RDWR);
		if( dev_fd < 0 )
		{
			perror("DeviceV4L2Base::dev_open: open_failed");
			printf("DeviceV4L2Base::dev_open: dev_path %s\n", dev_path);
			return 1;
		}
	        video_device->set_cloexec_flag(dev_fd, 1);
	}
	return 0;
}


int DeviceV4L2Base::open_dev(int color_model)
{
	v4l2_lock->lock("DeviceV4L2Base::open_dev");
	int result = 0;
	if( !opened )
	{
		result = dev_open();
		if( !result )
			result = v4l2_open(color_model);
		if( !result )
			result = start_dev();
		if( !result )
		{
			qbfrs_lock->reset();
			video_lock->reset();
			getq = new DeviceV4L2BufferQ(total_buffers+1);
			put_thread = new DeviceV4L2Put(this);
			put_thread->start();
			done = 0;
			Thread::start();
		}
		else
			printf("DeviceV4L2Base::open_dev failed\n");
	}
	if( result )
	{
		printf("DeviceV4L2Base::open_dev: adaptor open failed\n");
		stop_dev();
		dev_close();
	}
	else
		opened = 1;
	v4l2_lock->unlock();
	return result;
}

void DeviceV4L2Base::close_dev()
{
	v4l2_lock->lock("DeviceV4L2Base::close_dev");
	if( opened )
	{
		if(Thread::running())
		{
			done = 1;
			Thread::cancel();
		}
		Thread::join();
		if(put_thread)
		{
			delete put_thread;
			put_thread = 0;
		}
		stop_dev();
		dev_close();
		delete getq;
		getq = 0;
		opened = 0;
	}
	v4l2_lock->unlock();
}

int DeviceV4L2Base::status_dev()
{
        v4l2_lock->lock("DeviceV4L2Base::status_dev");
	int result = dev_fd >= 0 ? v4l2_status() : 1;
        v4l2_lock->unlock();
	return result;
}

unsigned int DeviceV4L2Base::cmodel_to_device(int color_model)
{
	switch(color_model)
	{
	case BC_COMPRESSED:	return V4L2_PIX_FMT_MJPEG;
	case BC_YUV422:		return V4L2_PIX_FMT_YUYV;
	case BC_UVY422:		return V4L2_PIX_FMT_UYVY;
	case BC_YUV411P:	return V4L2_PIX_FMT_Y41P;
	case BC_YUV420P:	return V4L2_PIX_FMT_YVU420;
	case BC_YUV422P:	return V4L2_PIX_FMT_YUV422P;
	case BC_RGB888:		return V4L2_PIX_FMT_RGB24;
	}
	return 0;
}

DeviceV4L2VFrame::DeviceV4L2VFrame(int dev_fd, unsigned long ofs, unsigned long sz)
{
	dev_data = (unsigned char*)mmap(NULL, dev_size=sz,
		PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, ofs);
	if(dev_data == MAP_FAILED) {
		perror("DeviceV4L2VFrame::DeviceV4L2VFrame: mmap");
		dev_data = 0;
	}
}


DeviceV4L2VFrame::~DeviceV4L2VFrame()
{
	munmap(dev_data, dev_size);
}

int DeviceV4L2Base::start_dev()
{
	VideoDevice *video_device = v4l2_device();
	if( !video_device ) return 1;

// Errors here are fatal.
	int req_buffers = video_device->in_config->capture_length;
	req_buffers = MAX(req_buffers, 2);
	req_buffers = MIN(req_buffers, 0xff);

	struct v4l2_requestbuffers requestbuffers;
	memset(&requestbuffers, 0, sizeof(requestbuffers));
//printf("DeviceV4L2Base::start_dev requested %d buffers\n", req_buffers);
	requestbuffers.count = req_buffers;
	requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	requestbuffers.memory = V4L2_MEMORY_MMAP;
	if(vioctl(VIDIOC_REQBUFS, &requestbuffers) < 0)
	{
		perror("DeviceV4L2Base::start_dev VIDIOC_REQBUFS");
		return 1;
	}
	int bfr_count = requestbuffers.count;
//printf("DeviceV4L2Base::start_dev got %d buffers\n", bfr_count);
	device_buffers = new DeviceV4L2VFrame *[bfr_count];
	memset(device_buffers, 0, bfr_count*sizeof(device_buffers[0]));

//printf("DeviceV4L2Base::start_dev color_model=%d\n", color_model);
	int y_offset = 0, line_size = -1;
	int u_offset = 0, v_offset = 0;

	if(color_model != BC_COMPRESSED)
	{
		switch(color_model)
		{
		case BC_YUV422P:
			u_offset = iwidth * iheight;
			v_offset = u_offset + u_offset / 2;
			break;
		case BC_YUV420P:
		case BC_YUV411P:
// In 2.6.7, the v and u are inverted for 420 but not 422
			v_offset = iwidth * iheight;
			u_offset = v_offset + v_offset / 4;
			break;
		}
	}

	total_buffers = 0;
	for(int i = 0; i < bfr_count; i++)
	{
		struct v4l2_buffer buffer;
		memset(&buffer, 0, sizeof(buffer));
		buffer.type = requestbuffers.type;
		buffer.index = i;
		if(vioctl(VIDIOC_QUERYBUF, &buffer) < 0)
		{
			perror("DeviceV4L2Base::start_dev VIDIOC_QUERYBUF");
			continue;
		}

		DeviceV4L2VFrame *dframe =
			new DeviceV4L2VFrame(dev_fd, buffer.m.offset, buffer.length);
		unsigned char *data = dframe->get_dev_data();
		if( !data ) continue;
		device_buffers[total_buffers++] = dframe;
		if(color_model != BC_COMPRESSED)
			dframe->reallocate(data, 0, y_offset, u_offset, v_offset,
				iwidth, iheight, color_model, line_size);
		else
			dframe->set_compressed_memory(data, 0, 0, dframe->get_dev_size());
	}

	q_bfrs = 0;
	for(int i = 0; i < total_buffers; i++)
	{
		struct v4l2_buffer buffer;
		memset(&buffer, 0, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;

		if(vioctl(VIDIOC_QBUF, &buffer) < 0)
		{
			perror("DeviceV4L2Base::start_dev VIDIOC_QBUF");
			continue;
		}
		++q_bfrs;
	}

	int streamon_arg = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(vioctl(VIDIOC_STREAMON, &streamon_arg) < 0)
	{
		perror("DeviceV4L2Base::start_dev VIDIOC_STREAMON");
		return 1;
	}

	streamon = 1;
	return 0;
}

int DeviceV4L2Base::stop_dev()
{
	if( streamon )
	{
		int streamoff_arg = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(vioctl(VIDIOC_STREAMOFF, &streamoff_arg) < 0)
			perror("DeviceV4L2Base::stop_stream_capture VIDIOC_STREAMOFF");
		streamon = 0;
	}

	for( int i = 0; i < total_buffers; i++ )
	{
		delete device_buffers[i];
	}

	struct v4l2_requestbuffers requestbuffers;
	memset(&requestbuffers, 0, sizeof(requestbuffers));
	requestbuffers.count = 0;
	requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	requestbuffers.memory = V4L2_MEMORY_MMAP;
	vioctl(VIDIOC_REQBUFS, &requestbuffers);

	delete [] device_buffers;
	device_buffers = 0;
	total_buffers = 0;
	return 0;
}


void DeviceV4L2Base::run()
{
	Thread::disable_cancel();
	int min_buffers = total_buffers / 2;
	if( min_buffers > 3 ) min_buffers = 3;

// Read buffers continuously
	while( !done )
	{
		int retry = 0;
		int qbfrs = q_bfrs;
		while( !done && (!qbfrs || (!retry && qbfrs < min_buffers)) )
		{
			Thread::enable_cancel();
			Timer::delay(10);
			Thread::disable_cancel();
			++retry;  qbfrs = q_bfrs;
		}
		if( done ) break;
		struct v4l2_buffer buffer;
		memset(&buffer, 0, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;

// The driver returns the first buffer not queued, so only one buffer
// can be unqueued at a time.
		Thread::enable_cancel();
		qbfrs_lock->lock("DeviceV4L2Base::run");
		int result = vioctl(VIDIOC_DQBUF, &buffer);
		if( !result ) --q_bfrs;
		qbfrs_lock->unlock();
		Thread::disable_cancel();
		if(result < 0)
		{
			perror("DeviceV4L2Base::run VIDIOC_DQBUF");
			Thread::enable_cancel();
			Timer::delay(10);
			Thread::disable_cancel();
			continue;
		}

// Get output frame
		int bfr = buffer.index;
// Set output frame data size/time, queue video
		VFrame *frame = device_buffers[bfr];
		if(color_model == BC_COMPRESSED)
			frame->set_compressed_size(buffer.bytesused);
		struct timeval *tv = &buffer.timestamp;
		double bfr_time = tv->tv_sec + tv->tv_usec / 1000000.;
		frame->set_timestamp(bfr_time);
		if( !getq->q(bfr) ) video_lock->unlock();
	}
}

#endif
