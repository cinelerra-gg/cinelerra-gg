
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

#ifndef DEVICEV4L2BASE_H
#define DEVICEV4L2BASE_H

#ifdef HAVE_VIDEO4LINUX2

#include "channel.inc"
#include "condition.h"
#include "vframe.inc"
#include "devicev4l2base.inc"
#include "mutex.inc"
#include "thread.h"
#include "vframe.h"
#include "videodevice.inc"

class DeviceV4L2BufferQ
{
	volatile int in, out;
        int limit, *buffers;
        Mutex *bfr_lock;
public:
	DeviceV4L2BufferQ(int qsize);
	~DeviceV4L2BufferQ();
	int q(int bfr);
	int dq();
	int dq(Condition *time_lock, int time_out);
	int sz();
	void reset() { in = out = 0; }
};

// Another thread which puts back buffers asynchronously of the buffer
// grabber.  Because 2.6.7 drivers block the buffer enqueuer.
class DeviceV4L2Put : public Thread
{
	DeviceV4L2Base *v4l2;
public:
        DeviceV4L2Put(DeviceV4L2Base *v4l2);
        ~DeviceV4L2Put();

        void run();
	DeviceV4L2BufferQ *putq;
	Condition *buffer_lock;
        int done;
	void put_buffer(int bfr) {
		if( !putq->q(bfr) ) buffer_lock->unlock();
	}
};

class DeviceV4L2VFrame : public VFrame {
	unsigned char *dev_data;
	unsigned long dev_size;
public:
	unsigned char *get_dev_data() { return dev_data; }
	unsigned long get_dev_size() { return dev_size; }

	DeviceV4L2VFrame(int dev_fd, unsigned long ofs, unsigned long sz);
	~DeviceV4L2VFrame();
};

class DeviceV4L2Base : public Thread
{
	friend class DeviceV4L2Put;
	DeviceV4L2Put *put_thread;
	Mutex *v4l2_lock;
// Some of the drivers in 2.6.7 can't handle simultaneous QBUF and DQBUF calls.
        Mutex *qbfrs_lock;
	Condition *video_lock;
	DeviceV4L2VFrame **device_buffers;
        Channel *device_channel;
        DeviceV4L2BufferQ *getq;

        int done;
	int opened;
	int streamon;
	int dev_status;
	int total_buffers;
	volatile int q_bfrs;
// COMPRESSED or another color model the device should use.
        int color_model;
	int dev_fd;
	int iwidth;
	int iheight;

	int dev_open();
	void dev_close();
	int v4l2_open(int color_model);
	int v4l2_status();

	int vioctl (unsigned long int req, void *arg);
	static unsigned int cmodel_to_device(int color_model);
	void run();
public:
	DeviceV4L2Base();
	~DeviceV4L2Base();

        virtual VideoDevice *v4l2_device() = 0;

	int v4l2_fd() { return dev_fd; }
	int open_dev(int color_model);
	void close_dev();
	int status_dev();
	virtual int start_dev();
	virtual int stop_dev();
	int get_sources();
	int is_opened() { return opened; }
	int dev_signal() { return dev_status; }
	VFrame *device_buffer(int bfr) { return device_buffers[bfr]; }
	int get_color_model() { return color_model; }
	int get_buffer() { return getq->dq(video_lock,V4L2_BUFFER_TIMEOUT); }
	void put_buffer(int bfr) { put_thread->put_buffer(bfr); }
	int putq_sz() { return put_thread->putq->sz(); }
};

#endif
#endif
