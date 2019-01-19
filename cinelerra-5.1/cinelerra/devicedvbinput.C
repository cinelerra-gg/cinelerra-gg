// jam job dvb file testing
//  1) define TEST_DATA as mpeg ts file "name.ts"
//  2) mod record.C Record::resync(), comment out: record_channel->drain_audio()
//  3) enable audio delay in audioidevice.C  AudioDevice::run_input, #if 0 <-> #if 1
//#define TEST_DATA "/tmp/xx.ts"

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

#ifdef HAVE_DVB
#include "chantables.h"
#include "devicedvbinput.h"
#include "devicempeginput.h"
#include "recordconfig.h"
#include "signalstatus.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>
#include <sys/ioctl.h>


DVBInputStatus::DVBInputStatus(DeviceDVBInput *dvb_input)
 : Thread(1, 0, 0)
{
	this->dvb_input = dvb_input;
	signal_status = 0;
	done = 1;
}

DVBInputStatus::~DVBInputStatus()
{
	stop();
	if( signal_status ) signal_status->disconnect();
}

void DVBInputStatus::stop()
{
	if( done ) return;
	done = 1;
	Thread::cancel();
	Thread::join();
}

void DVBInputStatus::start()
{
	if( !done ) return;
	done = 0;
	Thread::start();
}

void DVBInputStatus::lock_dvb()
{
	Thread::disable_cancel();
	dvb_input->dvb_lock->lock("DVBInputStatus::lock_dvb");
}

void DVBInputStatus::unlock_dvb()
{
	dvb_input->dvb_lock->unlock();
	Thread::enable_cancel();
}

void DVBInputStatus::run()
{
	lock_dvb();

	while( !done ) {
		dvb_input->status_dev();
		update();
		unlock_dvb();
		usleep(250000);
		lock_dvb();
	}

	unlock_dvb();
}

void DVBInputStatus::update()
{
	if( !signal_status ) return;
	signal_status->lock_window("DVBInputStatus::update");
	signal_status->update();
	signal_status->unlock_window();
}


DeviceDVBInput::DeviceDVBInput(const char *name, int no)
 : DeviceMPEGInput(name, no)
{
	frontend_fd = -1;
	demux_fd = -1;
	dvb_fd = -1;
	reset_signal();
	dvb_locked = 0;
	dvb_lock = new Mutex("DeviceDVBInput::dvb_lock", 0);
	sprintf(frontend_path, "%s/frontend%d", name, no);
	sprintf(demux_path, "%s/demux%d", name, no);
	sprintf(dvb_path, "%s/dvr%d", name, no);
	dvb_input_status = new DVBInputStatus(this);
}

DeviceMPEGInput *DeviceDVBInput::
NewDVBInput(const char *name, int no)
{
	return (DeviceMPEGInput *) new DeviceDVBInput(name, no);
}

DeviceDVBInput::~DeviceDVBInput()
{
	close_dev();
	delete dvb_input_status;
	delete dvb_lock;
	dvb_close(1);
}


DeviceMPEGInput* DeviceDVBInput::get_mpeg_input(VideoDevice *device)
{
	DeviceMPEGInput* mpeg_device = get_mpeg_video(device, NewDVBInput,
		device->in_config->dvb_in_adapter, device->in_config->dvb_in_device);
	if( !mpeg_device ) return 0;
	device->channel->has_subchan = 1;
	device->channel->has_scanning = 1;
	device->channel->use_frequency = 1;
	mpeg_device->set_device_number(0);
	Channel *input_src = device->new_input_source((char*)"input0");
	input_src->device_index = 0;
	input_src->tuner = 0;
	return mpeg_device;
}

DeviceMPEGInput* DeviceDVBInput::get_mpeg_input(AudioDevice *device)
{
	return get_mpeg_audio(device, NewDVBInput,
		device->in_config->dvb_in_adapter, device->in_config->dvb_in_device);
}


int DeviceDVBInput::wait_signal(int ms, int trys)
{
	Timer timer;
	for( int retry=trys; --retry>=0; ) {
		timer.update();
		if( !dvb_status() && has_signal() ) break;
		int dt = ms - timer.get_difference();
		if( dt > 0 ) Timer::delay(dt);
	}
	return has_signal() ? 0 : 1;
}


int DeviceDVBBuffer::read(int retries, int usec, int bsz)
{
	while( --retries >= 0 ) {
		struct timeval tv;  tv.tv_sec = 0;  tv.tv_usec = usec;
		fd_set rd_fd;  FD_ZERO(&rd_fd);  FD_SET(fd, &rd_fd);
		fd_set er_fd;  FD_ZERO(&er_fd);  FD_SET(fd, &er_fd);
		int ret = select(fd+1, &rd_fd, 0, &er_fd, &tv);
		if( ret < 0 ) break;
		if( !ret ) continue;
		if( !FD_ISSET(fd, &er_fd) ) {
			select(0, 0, 0, 0, &tv);
			continue;
		}
		if( !FD_ISSET(fd, &rd_fd) ) continue;
		if( (ret=::read(fd, bfr, bsz)) > 0 )
			return sz = ret;
	}
	return -1;
}

int DeviceDVBInput::dvb_sync()
{
	const int SYNC = 0x47, TS_BLKSZ = 188;
	const int BSZ = 100*TS_BLKSZ, XSZ = BSZ*5;
	DeviceDVBBuffer data(dvb_fd, BSZ);

	int nsync = 0, xfr = 0;
	while( xfr < XSZ ) {
		if( data.read(5, 200000, BSZ) < 0 ) break;
		int ofs = 0;
		while( ofs < data.size() ) {
			nsync = 0;
			int i = ofs;
			while( i<data.size() && data[i]!=SYNC ) ++i;
			if( i >= data.size() ) break;
			ofs = i;
			while( (ofs+=TS_BLKSZ)<data.size() && data[ofs]==SYNC ) {
				++nsync;  i = ofs;
			}
		}
		if( nsync >= 32 && (ofs-=data.size()) >= 0 ) {
			while( ofs > 0 ) {
				if( data.read(5, 200000, ofs) < 0 ) return 1;
				ofs -= data.size();
			}
			return 0;
		}
		xfr += data.size();
	}
	return 1;
}

// Return values: 0 = OK, 1 = unreported error, 2 = error reported to stderr
int DeviceDVBInput::dvb_open()
{
	int ret = 0;
#ifndef TEST_DATA
#ifdef HAVE_DVB
	if( frontend_fd < 0 &&
	   (frontend_fd = ::open(frontend_path, O_RDWR)) < 0 ) {
		fprintf(stderr, "DeviceDVBInput::dvb_open %s: %s\n",
			frontend_path, strerror(errno));
		ret = 2;
	}
	if( !ret && ioctl(frontend_fd, FE_GET_INFO, &fe_info) < 0 ) {
		fprintf(stderr,
			"DeviceDVBInput::dvb_open FE_GET_INFO: %s\n",
			strerror(errno));
		ret = 2;
	}

	pwr_min = snr_min = 0;
	pwr_max = snr_max = 65535;

// Set frequency
	int index = -1, table = -1;
	if( !ret && (index = get_channel()) < 0 ) ret = 1;
	if( !ret && (table = get_channel_table()) < 0 ) ret = 1;
	if( !ret && table >= CHANLIST_SIZE ) ret = 1;

	struct dvb_frontend_parameters frontend_param;
	if( !ret ) {
		uint32_t frequency = chanlists[table].list[index].freq * 1000;
		if( frequency < fe_info.frequency_min ||
			frequency > fe_info.frequency_max )  {
			fprintf(stderr,
				"DeviceDVBInput::dvb_open channel %s frequency %d out of range %d-%d\n",
				chanlists[table].list[index].name, frequency, fe_info.frequency_min, fe_info.frequency_max);
			 ret = 2;
		}
		memset(&frontend_param, 0, sizeof(frontend_param));
		frontend_param.frequency = frequency;
	}
	if( !ret ) {
		switch( table ) {
		case NTSC_DVB:
		case NTSC_BCAST:
		case NTSC_HRC:
		case NTSC_BCAST_JP:
			frontend_param.u.vsb.modulation = VSB_8;
			break;
		case PAL_EUROPE:
		case PAL_E_EUROPE:
		case PAL_ITALY:
		case PAL_NEWZEALAND:
		case PAL_AUSTRALIA:
		case PAL_IRELAND:
		case CATV_DVB:
		case NTSC_CABLE:
		case NTSC_CABLE_JP:
			frontend_param.u.vsb.modulation = QAM_AUTO;
			break;
		default:
			fprintf(stderr,
				"DeviceDVBInput::dvb_open bad table index=%d\n", table);
			ret = 2;
			break;
		}
	}

	if( !ret && ioctl(frontend_fd, FE_SET_FRONTEND, &frontend_param) < 0 ) {
		fprintf(stderr,
			"DeviceDVBInput::dvb_open FE_SET_FRONTEND frequency=%d: %s\n",
			frontend_param.frequency, strerror(errno));
		ret = 2;
	}

	if( !ret && wait_signal(333,3) ) {
		fprintf(stderr,
			 "DeviceDVBInput::dvb_open: no signal, index=%d, channel %s, frequency=%d\n",
			index, chanlists[table].list[index].name, frontend_param.frequency);
		ret = 2;
	}

	if( !ret && ioctl(frontend_fd, FE_GET_FRONTEND, &frontend_param) ) {
		fprintf(stderr,
			"DeviceDVBInput::dvb_open FE_GET_FRONTEND: %s\n",
			strerror(errno));
		ret = 2;
	}
	if( !ret ) { // goofy quirks
		if( !strcmp("Samsung S5H1409 QAM/8VSB Frontend", fe_info.name) ) {
		        switch(frontend_param.u.vsb.modulation) {
		        case QAM_64:  snr_min = 200;  snr_max = 300;  break;
		        case QAM_256: snr_min = 260;  snr_max = 400;  break;
		        case VSB_8:   snr_min = 111;  snr_max = 300;  break;
		        default: break;
			}
			pwr_min = snr_min;  pwr_max = snr_max;
		}
		else if( !strcmp("Auvitek AU8522 QAM/8VSB Frontend", fe_info.name) ) {
		        switch(frontend_param.u.vsb.modulation) {
		        case QAM_64:  snr_min = 190;  snr_max = 290;  break;
		        case QAM_256: snr_min = 280;  snr_max = 400;  break;
		        case VSB_8:   snr_min = 115;  snr_max = 270;  break;
		        default: break;
			}
			pwr_min = snr_min;  pwr_max = snr_max;
		}
	}

	if( !ret && (demux_fd = ::open(demux_path, O_RDWR)) < 0 ) {
		fprintf(stderr,
			"DeviceDVBInput::dvb_open %s for video: %s\n",
			demux_path,
			strerror(errno));
		ret = 2;
	}

// Setting exactly one PES filter to 0x2000 dumps the entire
// transport stream.
	if( !ret ) {
		struct dmx_pes_filter_params pesfilter;
		memset(&pesfilter,0,sizeof(pesfilter));
		pesfilter.pid = 0x2000;
		pesfilter.input = DMX_IN_FRONTEND;
		pesfilter.output = DMX_OUT_TS_TAP;
		pesfilter.pes_type = DMX_PES_OTHER;
		pesfilter.flags = DMX_IMMEDIATE_START;
		if( ioctl(demux_fd, DMX_SET_PES_FILTER, &pesfilter) < 0 ) {
			fprintf(stderr,
		"DeviceDVBInput::dvb_open DMX_SET_PES_FILTER for raw: %s\n",
				strerror(errno));
			ret = 1;
		}
	}
// Open transport stream for reading
	if( !ret && (dvb_fd = ::open(dvb_path, O_RDONLY+O_NONBLOCK)) < 0 ) {
		fprintf(stderr, "DeviceDVBInput::dvb_open %s: %s\n",
			dvb_path, strerror(errno));
		ret = 1;
	}
#else
	dvb_fd = -1;
	ret = 1;
#endif
#else
	dvb_fd = open(TEST_DATA,O_RDONLY);
	if( dvb_fd < 0 ) ret = 1;
#endif
	reset_signal();
	dvb_locked = 0;
	if( !ret ) ret = dvb_sync();
	if( !ret ) ret = dvb_status();
	if( ret ) dvb_close();
	return ret;
}


void DeviceDVBInput::dvb_close(int fe)
{
	if( dvb_fd >= 0 ) { ::close(dvb_fd); dvb_fd = -1; }
	if( demux_fd >= 0 ) { ::close(demux_fd); demux_fd = -1; }
	if( fe && frontend_fd >= 0 ) { ::close(frontend_fd); frontend_fd = -1; }
}


int DeviceDVBInput::dvb_status()
{
//note: this function can take > 500ms to execute  (slowww)
#ifndef TEST_DATA
	int locked = 0;
	reset_signal();
#ifdef HAVE_DVB
	int &fe = frontend_fd;
	if( fe < 0 ) return -1;
	fe_status_t st;  memset(&st, 0, sizeof(st));
	if( ioctl(fe, FE_READ_STATUS, &st) ) return 1;
	if( (st & FE_TIMEDOUT) != 0 ) return 1;
	signal_lck = (st & FE_HAS_LOCK) != 0 ? 1 : 0;
	signal_crr = (st & FE_HAS_CARRIER) != 0 ? 1 : 0;
	uint16_t power = 0;
	signal_pwr = ioctl(fe,FE_READ_SIGNAL_STRENGTH,&power) ? -1 :
		drange(power, pwr_min, pwr_max);
	uint16_t ratio = 0;
	signal_snr = ioctl(fe, FE_READ_SNR, &ratio) ? -1 :
		drange(ratio, snr_min, snr_max);
	uint32_t rate = 0;
	signal_ber = ioctl(fe, FE_READ_BER, &rate) ? -1 : rate;
	uint32_t errs = 0;
	signal_unc = ioctl(fe, FE_READ_UNCORRECTED_BLOCKS, &errs) ? -1 : errs;
	if( signal_lck && signal_ber >= 0 && signal_ber < 255 ) locked = 1;
	if( dvb_locked != locked ) {
		printf(_("** %scarrier, dvb_locked %s\n"),
			 signal_crr ? "" : _("no "), locked ? _("lock") : _("lost") );
	}
#endif
	dvb_locked = locked;
#endif
	return 0;
}

int DeviceDVBInput::drange(int v, int min, int max)
{
	if( (v-=min) < 0 || (max-=min) <= 0 ) return 0;
	return v>max ? 65535U : (65535U * v) / max;
}

void DeviceDVBInput::reset_signal()
{
	signal_pwr = signal_crr = signal_lck =
	signal_snr = signal_ber = signal_unc = -1;
}

int DeviceDVBInput::open_dev(int color_model)
{
	int ret = 0;
	dvb_lock->lock("DeviceDVBInput::open_dev");
	if( dvb_fd < 0 ) {
		ret = dvb_open();
		if( ret == 1 ) {
			printf("DeviceDVBInput::open_dev: adaptor %s open failed\n",
				dev_name);
		}
	}
	if( !ret )
		dvb_input_status->start();
	else
		dvb_close();
	dvb_lock->unlock();
	return ret;
}

void DeviceDVBInput::close_dev()
{
	dvb_lock->lock("DeviceDVBInput::close_dev");
	dvb_input_status->stop();
	dvb_close();
	dvb_lock->unlock();
}

int DeviceDVBInput::status_dev()
{
	int result = 0;
#ifndef TEST_DATA
	result = frontend_fd >= 0 ? dvb_status() : 1;
#endif
	return result;
}

void DeviceDVBInput::set_signal_status(SignalStatus *stat)
{
	dvb_lock->lock("DeviceDVBInput::set_signal_status");
	if( dvb_input_status->signal_status )
		dvb_input_status->signal_status->disconnect();
	dvb_input_status->signal_status = stat;
	if( stat ) stat->dvb_input = this;
	dvb_lock->unlock();
}

#endif
