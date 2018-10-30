
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





#include "audiompeg.h"
#include "channel.h"
#include "chantables.h"
#include "condition.h"
#include "cstrdup.h"
#include "devicempeginput.h"
#include "edl.h"
#include "edlsession.h"
#include "filempeg.h"
#include "linklist.h"
#include "mwindow.h"
#include "preferences.h"
#include "picture.h"
#include "recordconfig.h"
#include "vdevicempeg.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#ifdef HAVE_VIDEO4LINUX2
#include <linux/videodev2.h>
#endif

Condition DeviceMPEGInput::in_mpeg_lock(1,"DeviceMPEGInput::in_mpeg_lock");
DeviceMPEGList DeviceMPEGInput::in_mpeg;

DeviceMPEGInput::DeviceMPEGInput(const char *name, int no)
 : Garbage("DeviceMPEGInput")
{
//printf("new DeviceMPEGInput\n");
	reset();
	dev_name = cstrdup(name);
	device_number = no;
	decoder_lock = new Mutex("DeviceMPEGInput::decoder_lock", 0);
	video_lock = new Mutex("DeviceMPEGInput::video_lock", 0);
	audio_lock = new Mutex("DeviceMPEGInput::audio_lock", 0);
	toc_builder = new DeviceMPEG_TOC_Builder(this);
}

DeviceMPEGInput::~DeviceMPEGInput()
{
//printf("delete DeviceMPEGInput\n");
	delete toc_builder;
	delete src;
	delete toc;
	delete channel;
	delete decoder_lock;
	delete video_lock;
	delete audio_lock;
	delete [] dev_name;
}


void DeviceMPEGInput::reset()
{
	dev_name = 0;
	device_number = 0;
	video_device = 0;
	audio_device = 0;
	channel = 0;
	decoder_lock = 0;
	video_lock = 0;
	audio_lock = 0;
	src = 0;
	toc = 0;
	toc_builder = 0;
	toc_pos = 0;
	record_fd = -1;
	total_buffers = 0;
	device_buffers = 0;
	buffer_valid = 0;
	streamon = 0;
	color_model = -1;
	audio_inited = 0;
	video_inited = 0;
	captioning = -1;
	audio_stream = video_stream = -1;
	width = height = 0;
	framerate = 0.;
	channels = sample_bits = samplerate = 0;
	total_vstreams = total_achannels = 0;
}

DeviceMPEGInput* DeviceMPEGInput::get_mpeg_device(
	DeviceMPEGInput* (*new_device)(const char *name, int no),
	const char *name, int no)
{
	DeviceMPEGInput *current = in_mpeg.first;
	while( current ) {
		if( current->device_number != no ) continue;
		if( !strcmp(current->dev_name, name) ) break;
 		current = NEXT;
	}
	if( !current )
	{
		current = new_device(name, no);
		in_mpeg.append(current);
	}
	else
		current->add_user();
	return current;
}


DeviceMPEGInput* DeviceMPEGInput::get_mpeg_video(VideoDevice *video_device,
	DeviceMPEGInput* (*new_device)(const char *name, int no),
	const char *name, int no)
{
//printf("get_mpeg_video\n");
	in_mpeg_lock.lock("DeviceMPEGInput::get_mpeg_video");
	DeviceMPEGInput* device = get_mpeg_device(new_device, name, no);
	if( !device->video_device )
		device->video_device = video_device;
	else
		device = 0;
	in_mpeg_lock.unlock();
	return device;
}

DeviceMPEGInput* DeviceMPEGInput::get_mpeg_audio(AudioDevice *audio_device,
	DeviceMPEGInput* (*new_device)(const char *name, int no),
	const char *name, int no)
{
//printf("get_mpeg_audio\n");
	in_mpeg_lock.lock("DeviceMPEGInput::get_mpeg_audio");
	DeviceMPEGInput* device = get_mpeg_device(new_device, name, no);
	if( !device->audio_device )
		device->audio_device = audio_device;
	else
		device = 0;
	in_mpeg_lock.unlock();
	return device;
}

void DeviceMPEGInput::put_mpeg_video()
{
//printf("put_mpeg_video\n");
	in_mpeg_lock.lock("DeviceMPEGInput::put_mpeg_video");
	video_device = 0;
	remove_user();
	in_mpeg_lock.unlock();
}

void DeviceMPEGInput::put_mpeg_audio()
{
//printf("put_mpeg_audio\n");
	in_mpeg_lock.lock("DeviceMPEGInput::put_mpeg_audio");
	audio_device = 0;
	remove_user();
	in_mpeg_lock.unlock();
}


MWindow *DeviceMPEGInput::get_mwindow()
{
	in_mpeg_lock.lock("DeviceMPEGInput::get_mwindow");
	MWindow *mwindow = video_device ? video_device->mwindow :
		  audio_device ? audio_device->mwindow : 0;
	in_mpeg_lock.unlock();
	return mwindow;
}

int DeviceMPEGInput::get_stream(int reopen)
{
	int ret = 0;
	if( src ) {
//printf("DeviceMPEGInput::get_stream delete src %p\n",src);
		delete src;  src = 0;
		audio_stream = video_stream = -1;
	}
	if( reopen ) close_dev();
	if( !channel ) ret = 1;
	if( !ret ) ret = open_dev(color_model);
	if( !ret ) ret = status_dev();
	if( !ret ) ret = has_signal() ? 0 : 1;
	if( !ret ) {
//struct stat st; fstat(mpeg_fd(),&st); printf("stat %08x/%08x\n",(int)st.st_dev,(int)st.st_rdev);
//		src = new zmpeg3_t(mpeg_fd(), ret, !st.st_rdev ? 0 : ZIO_THREADED+ZIO_NONBLOCK);
		src = new zmpeg3_t(mpeg_fd(), ret, ZIO_THREADED+ZIO_NONBLOCK);
//printf("DeviceMPEGInput::get_stream new src %p\n",src);
		if( !ret && channel->audio_stream >= src->total_astreams() ) ret = 1;
		if( !ret && channel->video_stream >= src->total_vstreams() ) ret = 1;
		if( !ret && record_fd >= 0 ) src->start_record(record_fd, 0);
		if( !ret ) {
			audio_stream = channel->audio_stream;
			video_stream = channel->video_stream;
			MWindow *mwindow = get_mwindow();
			int cpus = mwindow ? mwindow->preferences->real_processors : 1;
			src->set_cpus(cpus);
			src->show_subtitle(video_stream, captioning);
			height = src->video_height(video_stream);
			if( height < 0 ) height = 0;
			width = src->video_width(video_stream);
			if( width < 0 ) width = 0;
			framerate = src->frame_rate(video_stream);
			if( framerate < 0. ) framerate = 0.;
			channels = src->audio_channels(audio_stream);
			if( channels < 0 ) channels = 0;
			samplerate = src->sample_rate(audio_stream);
			if( samplerate < 0 ) samplerate = 0;
			sample_bits = sizeof(short)*8;
			int dev_cmodel = src->colormodel(video_stream);
			printf("mpeg video %dx%d @%6.2ffps %s  "
				"audio %dchs, %d rate %d bits\n",
				height, width, framerate,
				FileMPEG::zmpeg3_cmdl_name(dev_cmodel),
				channels, samplerate, sample_bits);
			total_vstreams = src->total_vstreams();
			total_achannels = 0;
			int total_astreams = src->total_astreams();
			for( int stream=0; stream<total_astreams; ++stream )
				total_achannels +=  src->audio_channels(stream);
		}
		else {
			fprintf(stderr,"DeviceMPEGInput::get_stream open failed (%d)\n", ret);
			delete src;  src = 0;
		}
	}
	if( ret ) close_dev();
	return ret;
}

int DeviceMPEGInput::src_stream(Mutex *stream)
{
	int ret = 0;
	decoder_lock->lock();
	if( !src ) {
		audio_lock->lock();
		video_lock->lock();
		ret = get_stream();
		video_lock->unlock();
		audio_lock->unlock();
	}
	if( !ret && stream ) stream->lock();
	if( ret || stream ) decoder_lock->unlock();
	return ret;
}

zmpeg3_t *
DeviceMPEGInput::src_lock()
{
	decoder_lock->lock();
	if( src ) return src;
	decoder_lock->unlock();
	return 0;
}

void DeviceMPEGInput::src_unlock()
{
	decoder_lock->unlock();
}

int DeviceMPEGInput::get_dev_cmodel(int color_model)
{
	int result = FileMPEG::zmpeg3_cmdl(color_model);
	if( result < 0 )
		printf("DeviceMPEGInput::get_dev_cmodel: unknown color_model for libzmpeg3\n");
	return result;
}

int DeviceMPEGInput::colormodel()
{
	return color_model>=0 ? color_model : BC_YUV420P;
}

int DeviceMPEGInput::read_buffer(VFrame *frame)
{
	int result = 1;
	if( !video_inited )
	{
        	color_model = frame->get_color_model();
        	if( (result=open_dev(color_model)) ) return result;
        	if( (result=src_stream()) ) return result;
                int width = video_width();
                int height = video_height();
                double rate = video_framerate();
                video_device->auto_update(rate, width, height);
                video_device->capturing = 1;
                src_unlock();
                video_inited = 1;
                return -1;
        }
	int dev_cmodel = get_dev_cmodel(color_model);
	if( dev_cmodel >= 0 ) {
		int w = frame->get_w();
		int h = frame->get_h();
		uint8_t *data = frame->get_data();
		int bpl = frame->get_bytes_per_line();
		int uvs = 0;
		switch( dev_cmodel ) {
		case zmpeg3_t::cmdl_YUV420P: uvs = 2;  break;
		case zmpeg3_t::cmdl_YUV422P: uvs = 1;  break;
		default: break;
		}
		int uvw = !uvs ? 0 : bpl / 2;
		int uvh = !uvs ? 0 : 2*h / uvs;
		uint8_t *rows[h + uvh], *rp = data;
		int n = 0;
		for( int i=0; i<h; ++i, rp+=bpl ) rows[n++] = rp;
		for( int i=0; i<uvh; ++i, rp+=uvw ) rows[n++] = rp;
		if( !src_stream(video_lock) ) {
			double timestamp = src->get_video_time(video_stream);
			frame->set_timestamp(timestamp);
			result = src->read_frame(&rows[0],
				0,0, width,height,
				w,h, dev_cmodel, video_stream);
			video_lock->unlock();
		}
		if( result )
			set_channel(channel);
	}
	return result;
}

int DeviceMPEGInput::read_audio(char *data, int size)
{
	int result = -1;
	if( !audio_inited )
	{
		if( (result=src_stream()) ) return result;
		int samplerate = audio_sample_rate();
		int channels = audio_channels();
		int bits = audio_sample_bits();
//printf("DeviceMPEGInput::read_audio activate  ch %d, rate %d, bits %d\n",
//  channels, samplerate, bits);
		audio_device->auto_update(channels, samplerate, bits);
		src_unlock();
		audio_inited = 1;
		return -1;
	}
	int frame = (sample_bits * channels) / 8;
	if( size > 0 && frame > 0 && audio_stream >= 0 &&
			!src_stream(audio_lock) ) {
		result = 0;
		short *sdata = (short*)data;
		int samples = size / frame;
		short bfr[samples];
		int chans = channels;
		for( int ch=0; ch<chans; ++ch ) {
			int ret = ch == 0 ?
		src->read_audio(&bfr[0], ch, samples, audio_stream) :
		src->reread_audio(&bfr[0], ch, samples, audio_stream);
			if( !ret ) {
				int k = ch;
				for( int i=0; i<samples; ++i,k+=chans )
					sdata[k] = bfr[i];
			}
			else {
				int k = ch;
				for( int i=0; i<samples; ++i,k+=chans )
					sdata[k] = 0;
				result = -1;
			}
		}
		audio_lock->unlock();
	}
	return result;
}

int DeviceMPEGInput::set_channel(Channel *channel)
{
//printf("DeviceMPEGInput::set_channel: enter %s\n",!channel ? "close" :
//  channel == this->channel ? "reset" : channel->title);
	decoder_lock->lock();
	audio_lock->lock();
	video_lock->lock();
	Channel *new_channel = 0;
	if( channel ) {
		new_channel = new Channel;
		new_channel->copy_settings(channel);
		new_channel->copy_usage(channel);
	}
	delete this->channel;
	this->channel = new_channel;
	audio_reset();
	video_reset();
	int ret = get_stream(1);
	video_lock->unlock();
	audio_lock->unlock();
	decoder_lock->unlock();
//printf("DeviceMPEGInput::set_channel: exit %d\n",has_lock);
	return !ret && !status_dev() && has_signal() ? 0 : 1;
}

void DeviceMPEGInput::set_captioning(int strk)
{
	decoder_lock->lock("DeviceMPEGInput::set_captioning");
	captioning = strk;
	if( src && video_stream >= 0 )
		src->show_subtitle(video_stream, strk);
	decoder_lock->unlock();
}

int DeviceMPEGInput::drop_frames(int frames)
{
	decoder_lock->lock("DeviceMPEGInput::drop_frames");
	double result = -1.;
	if( src && video_stream >= 0 )
		result = src->drop_frames(frames,video_stream);
	decoder_lock->unlock();
	return result;
}

double DeviceMPEGInput::audio_timestamp()
{
	decoder_lock->lock("DeviceMPEGInput::audio_timestamp");
	double result = -1.;
	if( src && audio_stream >= 0 )
		result = src->get_audio_time(audio_stream);
	decoder_lock->unlock();
	return result;
}

double DeviceMPEGInput::video_timestamp()
{
	decoder_lock->lock("DeviceMPEGInput::video_timestamp");
	double ret = -1.;
	if( src && video_stream >= 0 )
		ret = src->get_video_time(video_stream);
	decoder_lock->unlock();
	return ret;
}

int DeviceMPEGInput::start_toc(const char *path, const char *toc_path)
{
	if( !src || toc ) return -1;
	toc = src->start_toc(path, toc_path);
	toc_pos = 0;
	toc_builder->start();
	return toc ? 0 : 1;
}

int DeviceMPEGInput::tick_toc()
{
	if( record_fd < 0 || !src || !toc ) return 1;
	while( toc_pos < src->record_position() )
 	 	toc->do_toc(&toc_pos);
	return 0;
}

int DeviceMPEGInput::start_record(int fd, int bsz)
{
	record_fd = fd;
	return src ? src->start_record(fd, bsz) : -1;
}

int DeviceMPEGInput::stop_record()
{
	int ret = src ? src->stop_record() : -1;
	if( toc ) {
		toc_builder->stop(1);
		toc->stop_toc();
		toc = 0;
	}
	record_fd = -1;
	return ret;
}

int DeviceMPEGInput::subchannel_count()
{
#ifdef HAVE_DVB
	return src ? src->dvb.channel_count() : 0;
#else
	return 0;
#endif
}

int DeviceMPEGInput::subchannel_definition(int subchan, char *name,
	int &major, int &minor, int &total_astreams, int &total_vstreams)
{
#ifdef HAVE_DVB
	int result = src != 0 ? 0 : -1;
	if( !result ) result = src->dvb.get_channel(subchan, major, minor);
	if( !result ) result = src->dvb.get_station_id(subchan, &name[0]);
	if( !result ) result = src->dvb.total_astreams(subchan, total_astreams);
	if( !result ) result = src->dvb.total_vstreams(subchan, total_vstreams);
	return result;
#else
	return -1;
#endif
}

int DeviceMPEGInput::subchannel_video_stream(int subchan, int vstream)
{
#ifdef HAVE_DVB
	int result = src != 0 ? 0 : -1;
	if( !result && src->dvb.vstream_number(subchan, vstream, result) )
		result = -1;
	return result;
#else
	return -1;
#endif
}

int DeviceMPEGInput::subchannel_audio_stream(int subchan, int astream, char *enc)
{
#ifdef HAVE_DVB
	int result = src != 0 ? 0 : -1;
	if( src && src->dvb.astream_number(subchan, astream, result, enc) ) {
		enc[0] = 0; result = -1;
	}
	return result;
#else
	return -1;
#endif
}

int DeviceMPEGInput::get_video_pid(int track)
{
	return !src ? -1 : src->video_pid(track);
}

int DeviceMPEGInput::get_video_info(int track, int &pid, double &framerate,
                int &width, int &height, char *title)
{
	//caller of callback holds decoder_lock;
	if( !src ) return 1;
	pid = src->video_pid(track);
	framerate = src->frame_rate(track);
	width = src->video_width(track);
	height = src->video_height(track);
	if( !title ) return 0;
	*title = 0;
	int elements = src->dvb.channel_count();
	for( int n=0; n<elements; ++n ) {
		int major, minor, total_vstreams, vstream, vidx;
		if( src->dvb.get_channel(n, major, minor) ) continue;
		if( src->dvb.total_vstreams(n,total_vstreams) ) continue;
		for( vidx=0; vidx<total_vstreams; ++vidx ) {
			if( src->dvb.vstream_number(n,vidx,vstream) ) continue;
			if( vstream < 0 ) continue;
			if( vstream == track ) {
				sprintf(title, "%3d.%-3d", major, minor);
				return 0;
			}
		}
	}
	return 0;
}

int DeviceMPEGInput::get_thumbnail(int stream, int64_t &position,
		unsigned char *&thumbnail, int &ww, int &hh)
{
	if( !src ) return 1;
	return src->get_thumbnail(stream, position, thumbnail, ww, hh);
}

int DeviceMPEGInput::set_skimming(int track, int skim, skim_fn fn, void *vp)
{
	if( !src ) return 1;
	return !fn ? src->set_thumbnail_callback(track, 0, 0, 0, 0) :
		src->set_thumbnail_callback(track, skim, 1, fn, vp);
}


Channel *DeviceMPEGInput::add_channel( ArrayList<Channel*> *channeldb, char *name,
	int element, int major, int minor, int vstream, int astream, char *enc)
{
	Channel *ch = new Channel;
	ch->copy_settings(channel);
	sprintf(ch->title,"%d.%d %s", major, minor, &name[0]);
	sprintf(ch->device_name,"%d", get_channel());
	if( astream >= 0 && enc && enc[0] ) {
		char *cp = ch->title;  cp += strlen(cp);
		sprintf(cp," (%s)",&enc[0]);
	}
	ch->element = element;
	ch->video_stream = vstream;
	ch->audio_stream = astream;
	channeldb->append(ch);
	return ch;
}

//printf("   channel %s\n",channel->title);
int DeviceMPEGInput::get_channeldb(ArrayList<Channel*> *channeldb)
{
//printf(" get channel record\n");
	int count = subchannel_count();
	for( int subchan=0; subchan<count; ++subchan ) {
		char name[16], enc[8];
		int vstream = -1, video_stream = -1;
		int astream = -1, audio_stream = -1;
		int major, minor, total_astreams, total_vstreams;
		if( subchannel_definition(subchan,&name[0],
			major, minor, total_astreams, total_vstreams)) continue;
		if( total_astreams > 1 && total_vstreams > 1 )
			printf(_("DeviceMPEGInput::get_channeldb::element %d"
				" (id %d.%d) has %d/%d video/audio streams\n"),
				subchan, major, minor, total_vstreams, total_astreams);
		if( total_vstreams > 1 && total_astreams > 0 && (audio_stream =
			subchannel_audio_stream(subchan, astream=0, enc)) >= 0 ) {
			if( total_astreams > 1 )
				printf(_("  only first audio stream will be used\n"));
			for( int vstream=0; vstream<total_vstreams; ++vstream ) {
				int video_stream = subchannel_video_stream(subchan, vstream);
				if( video_stream < 0 ) continue;
				Channel *ch = add_channel(channeldb, name, subchan,
					major, minor, video_stream, audio_stream, enc);
				char *cp = ch->title;  cp += strlen(cp);
				sprintf(cp," v%d",vstream+1);
			}
			continue;
		}
		if( total_astreams > 1 && total_vstreams > 0 && (video_stream =
			subchannel_video_stream(subchan, vstream=0)) >= 0 ) {
			if( total_vstreams > 1 )
				printf(_("  only first video stream will be used\n"));
			for( int astream=0; astream<total_astreams; ++astream ) {
				int audio_stream = subchannel_audio_stream(subchan, astream, enc);
				if( audio_stream < 0 ) continue;
				Channel *ch = add_channel(channeldb, name, subchan,
					major, minor, video_stream, audio_stream, enc);
				char *cp = ch->title;  cp += strlen(cp);
				sprintf(cp," a%d",astream+1);
			}
			continue;
		}
		if( total_astreams > 0 || total_vstreams > 0 ) {
			astream = vstream = -1;
			video_stream = !total_vstreams ? -1 :
				subchannel_video_stream(subchan, vstream = 0);
			audio_stream = !total_astreams ? -1 :
				subchannel_audio_stream(subchan, astream = 0, enc);
			add_channel(channeldb, name, subchan,
				major, minor, video_stream, audio_stream, enc);
		}
	}
	return 0;
}

int DeviceMPEGInput::create_channeldb(ArrayList<Channel*> *channeldb)
{
	int ret = 1;
	if( !src_stream() ) {
		ret = get_channeldb(channeldb);
		src_unlock();
	}
	return ret;
}

DeviceMPEG_TOC_Builder::
DeviceMPEG_TOC_Builder(DeviceMPEGInput *mpeg_dev)
 : Thread(1, 0, 0)
{
	this->mpeg_dev = mpeg_dev;
	done = -1;
}

DeviceMPEG_TOC_Builder::
~DeviceMPEG_TOC_Builder()
{
	stop();
}


void DeviceMPEG_TOC_Builder::
stop(int wait)
{
	if( !done ) {
		done = 1;
		if( !wait ) Thread::cancel();
	}
	Thread::join();
}

void DeviceMPEG_TOC_Builder::
start()
{
	done = 0;
	Thread::start();
}

void DeviceMPEG_TOC_Builder::
run()
{
	Thread::enable_cancel();

	while( !done ) {
		Thread::disable_cancel();
		mpeg_dev->tick_toc();
		Thread::enable_cancel();
		sleep(1);
	}

	Thread::disable_cancel();
	mpeg_dev->tick_toc();
	done = -1;
}

