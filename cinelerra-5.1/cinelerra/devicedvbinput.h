
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

#ifndef DEVICEDVBINPUT_H
#define DEVICEDVBINPUT_H

#ifdef HAVE_DVB

#include "devicedvbinput.inc"
#include "devicempeginput.h"
#include "libzmpeg3.h"
#include "signalstatus.h"
#include "thread.h"
#include "videodevice.h"
#include "vdevicedvb.h"

#include <stdint.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>



class DVBInputStatus : public Thread
{
	friend class DeviceDVBInput;
	int done;
	DeviceDVBInput *dvb_input;
	SignalStatus *signal_status;
	void run();
	void lock_dvb();
	void unlock_dvb();
public:
	void start();
	void stop();
	void update();

	DVBInputStatus(DeviceDVBInput *dvb_input);
	~DVBInputStatus();
};

class DeviceDVBInput : public DeviceMPEGInput
{
	friend class DVBInputStatus;
	friend class SignalStatus;
	Mutex *dvb_lock;
	char frontend_path[BCTEXTLEN];
	char demux_path[BCTEXTLEN];
	char dvb_path[BCTEXTLEN];
	int frontend_fd;
	int demux_fd;
	int dvb_fd;
	int dvb_locked;
	int signal_pwr, signal_crr, signal_lck;
	int signal_snr, signal_ber, signal_unc;
	DVBInputStatus *dvb_input_status;
	struct dvb_frontend_info fe_info;
	uint32_t pwr_min, pwr_max;
	uint32_t snr_min, snr_max;

	void reset_signal();
	int wait_signal(int ms, int trys);
	int dvb_sync();
	int dvb_open();
	void dvb_close(int fe=0);
	int dvb_status();
	int drange(int v, int min, int max);
public:
	DeviceDVBInput(const char *name, int no);
	static DeviceMPEGInput *NewDVBInput(const char *name, int no);
	~DeviceDVBInput();

	static DeviceMPEGInput* get_mpeg_input(VideoDevice *device);
	static DeviceMPEGInput* get_mpeg_input(AudioDevice *device);
	void put_dvb_video() { put_mpeg_video(); }
	void put_dvb_audio() { put_mpeg_audio(); }

	int open_dev(int color_model);
	void close_dev();
	int status_dev();
	void set_signal_status(SignalStatus *stat);
	int has_signal() { return dvb_locked; }
	int mpeg_fd() { return dvb_fd; }

	VDeviceDVB *dvb_video() {
		return video_device ? (VDeviceDVB *)video_device->input_base : 0;
	}
	AudioDVB *dvb_audio() {
		return audio_device ? (AudioDVB *)audio_device->lowlevel_in : 0;
	}
};

class DeviceDVBBuffer
{
	int fd, sz, bsz;
	uint8_t *bfr;
public:
	DeviceDVBBuffer(int ifd, int isz) {
		fd = ifd;  sz = 0;
		bsz = isz;  bfr = new uint8_t[isz];
        }
	~DeviceDVBBuffer() { delete [] bfr; }

	int read(int retries, int usec, int bsz);
	int size() { return sz; }
	int operator [](int i) { return bfr[i]; }
};

#endif
#endif
