
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

#include "audiodevice.inc"
#include "bchash.h"
#include "playbackconfig.h"
#include "recordconfig.h"
#include "videodevice.inc"
#include <string.h>





AudioInConfig::AudioInConfig()
{
#ifdef HAVE_ALSA
	driver = AUDIO_ALSA;
#else
	driver = AUDIO_OSS;
#endif
	for(int i = 0; i < MAXDEVICES; i++)
	{
		oss_enable[i] = (i == 0);
		sprintf(oss_in_device[i], "/dev/dsp");
	}
	oss_in_bits = 16;
	firewire_port = 0;
	firewire_channel = 63;
	strcpy(firewire_path, "/dev/raw1394");
	esound_in_server[0] = 0;
	esound_in_port = 0;

	sprintf(alsa_in_device, "default");
	alsa_in_bits = 16;
	in_samplerate = 48000;
	strcpy(dvb_in_adapter,"/dev/dvb/adapter0");
	dvb_in_device = 0;
	dvb_in_bits = 16;
	v4l2_in_bits = 16;
	channels = 2;
	follow_audio = 1;
	map51_2 = 1;
	rec_gain = 1.0;
}

AudioInConfig::~AudioInConfig()
{
}

int AudioInConfig::is_duplex(AudioInConfig *in, AudioOutConfig *out)
{
	if(in->driver == out->driver)
	{
		switch(in->driver)
		{
			case AUDIO_OSS:
			case AUDIO_OSS_ENVY24:
				return (!strcmp(in->oss_in_device[0], out->oss_out_device[0]) &&
					in->oss_in_bits == out->oss_out_bits);
				break;

// ALSA always opens 2 devices
			case AUDIO_ALSA:
				return 0;
				break;
		}
	}

	return 0;
}


void AudioInConfig::copy_from(AudioInConfig *src)
{
	driver = src->driver;

	firewire_port = src->firewire_port;
	firewire_channel = src->firewire_channel;
	strcpy(firewire_path, src->firewire_path);

	strcpy(esound_in_server, src->esound_in_server);
	esound_in_port = src->esound_in_port;

	for(int i = 0; i < MAXDEVICES; i++)
	{
		oss_enable[i] = src->oss_enable[i];
		strcpy(oss_in_device[i], src->oss_in_device[i]);
		oss_in_bits = src->oss_in_bits;
	}

	strcpy(alsa_in_device, src->alsa_in_device);
	alsa_in_bits = src->alsa_in_bits;
	in_samplerate = src->in_samplerate;
	strcpy(dvb_in_adapter, src->dvb_in_adapter);
	dvb_in_device = src->dvb_in_device;
	dvb_in_bits = src->dvb_in_bits;
	v4l2_in_bits = src->v4l2_in_bits;
	channels = src->channels;
	follow_audio = src->follow_audio;
	map51_2 = src->map51_2;
	rec_gain = src->rec_gain;
}

AudioInConfig& AudioInConfig::operator=(AudioInConfig &that)
{
	copy_from(&that);
	return *this;
}

int AudioInConfig::load_defaults(BC_Hash *defaults)
{
	driver = defaults->get("R_AUDIOINDRIVER", driver);
	firewire_port = defaults->get("R_AFIREWIRE_IN_PORT", firewire_port);
	firewire_channel = defaults->get("R_AFIREWIRE_IN_CHANNEL", firewire_channel);
	defaults->get("R_AFIREWIRE_IN_PATH", firewire_path);
	for(int i = 0; i < MAXDEVICES; i++)
	{
		oss_enable[i] = defaults->getf(oss_enable[i], "R_OSS_ENABLE_%d", i);
		defaults->getf(oss_in_device[i], "R_OSS_IN_DEVICE_%d", i);
	}
	oss_in_bits = defaults->get("R_OSS_IN_BITS", oss_in_bits);
	defaults->get("R_ESOUND_IN_SERVER", esound_in_server);
	esound_in_port = defaults->get("R_ESOUND_IN_PORT", esound_in_port);

	defaults->get("R_ALSA_IN_DEVICE", alsa_in_device);
	alsa_in_bits = defaults->get("R_ALSA_IN_BITS", alsa_in_bits);
	in_samplerate = defaults->get("R_IN_SAMPLERATE", in_samplerate);
	defaults->get("R_AUDIO_DVB_IN_ADAPTER", dvb_in_adapter);
	dvb_in_device = defaults->get("R_AUDIO_DVB_IN_DEVICE", dvb_in_device);
	dvb_in_bits = defaults->get("R_DVB_IN_BITS", dvb_in_bits);
	v4l2_in_bits = defaults->get("R_V4L2_IN_BITS", v4l2_in_bits);
	channels = defaults->get("R_IN_CHANNELS", channels);
	follow_audio = defaults->get("R_FOLLOW_AUDIO", follow_audio);
	map51_2 = defaults->get("R_AUDIO_IN_MAP51_2", map51_2);
	rec_gain = defaults->get("R_AUDIO_IN_GAIN", rec_gain);
	return 0;
}

int AudioInConfig::save_defaults(BC_Hash *defaults)
{
	defaults->update("R_AUDIOINDRIVER", driver);
	defaults->update("R_AFIREWIRE_IN_PORT", firewire_port);
	defaults->update("R_AFIREWIRE_IN_CHANNEL", firewire_channel);
	defaults->update("R_AFIREWIRE_IN_PATH", firewire_path);

	for(int i = 0; i < MAXDEVICES; i++)
	{
		defaults->updatef(oss_enable[i], "R_OSS_ENABLE_%d", i);
		defaults->updatef(oss_in_device[i], "R_OSS_IN_DEVICE_%d", i);
	}

	defaults->update("R_OSS_IN_BITS", oss_in_bits);
	defaults->update("R_ESOUND_IN_SERVER", esound_in_server);
	defaults->update("R_ESOUND_IN_PORT", esound_in_port);

	defaults->update("R_ALSA_IN_DEVICE", alsa_in_device);
	defaults->update("R_ALSA_IN_BITS", alsa_in_bits);
	defaults->update("R_IN_SAMPLERATE", in_samplerate);
	defaults->update("R_AUDIO_DVB_IN_ADAPTER", dvb_in_adapter);
	defaults->update("R_AUDIO_DVB_IN_DEVICE", dvb_in_device);
	defaults->update("R_DVB_IN_BITS", dvb_in_bits);
	defaults->update("R_V4L2_IN_BITS", v4l2_in_bits);
	defaults->update("R_IN_CHANNELS", channels);
	defaults->update("R_FOLLOW_AUDIO", follow_audio);
	defaults->update("R_AUDIO_IN_MAP51_2", map51_2);
	defaults->update("R_AUDIO_IN_GAIN", rec_gain);
	return 0;
}






const char *VideoInConfig::default_video_device = "/dev/video0";

VideoInConfig::VideoInConfig()
{
	driver = VIDEO4LINUX2;
	sprintf(v4l2_in_device, "%s", "/dev/video0");
	sprintf(v4l2jpeg_in_device, "%s", "/dev/video0");
	v4l2jpeg_in_fields = 2;
	sprintf(v4l2mpeg_in_device, "%s", "/dev/video0");
	strcpy(dvb_in_adapter, "/dev/dvb/adapter0");
	dvb_in_device = 0;
	sprintf(screencapture_display, "%s", "");
	firewire_port = 0;
	firewire_channel = 63;
	sprintf(firewire_path, "/dev/raw1394");
// number of frames to read from device during video recording.
//	capture_length = 15;
	capture_length = 2;
	w = 720;
	h = 480;
	in_framerate = 29.97;
	follow_video = 1;
}

VideoInConfig::~VideoInConfig()
{
}

const char *VideoInConfig::get_path()
{
	switch(driver) {
	case CAPTURE_JPEG_WEBCAM:
	case CAPTURE_YUYV_WEBCAM:
	case VIDEO4LINUX2: return v4l2_in_device;
	case VIDEO4LINUX2JPEG: return v4l2jpeg_in_device;
	case VIDEO4LINUX2MPEG: return v4l2mpeg_in_device;
	case CAPTURE_DVB: return dvb_in_adapter;
	}
	return default_video_device;
}

void VideoInConfig::copy_from(VideoInConfig *src)
{
	driver = src->driver;
	strcpy(v4l2_in_device, src->v4l2_in_device);
	v4l2jpeg_in_fields = src->v4l2jpeg_in_fields;
	strcpy(v4l2jpeg_in_device, src->v4l2jpeg_in_device);
	strcpy(v4l2mpeg_in_device, src->v4l2mpeg_in_device);
	strcpy(dvb_in_adapter, src->dvb_in_adapter);
	dvb_in_device = src->dvb_in_device;
	strcpy(screencapture_display, src->screencapture_display);
	firewire_port = src->firewire_port;
	firewire_channel = src->firewire_channel;
	strcpy(firewire_path, src->firewire_path);
	capture_length = src->capture_length;
	w = src->w;
	h = src->h;
	in_framerate = src->in_framerate;
	follow_video = src->follow_video;
}

VideoInConfig& VideoInConfig::operator=(VideoInConfig &that)
{
	copy_from(&that);
	return *this;
}

int VideoInConfig::load_defaults(BC_Hash *defaults)
{
	driver = defaults->get("R_VIDEO_IN_DRIVER", driver);
	defaults->get("R_V4L2_IN_DEVICE", v4l2_in_device);
	defaults->get("R_V4L2JPEG_IN_DEVICE", v4l2jpeg_in_device);
	v4l2jpeg_in_fields = defaults->get("R_V4L2JPEG_IN_FIELDS", v4l2jpeg_in_fields);
	defaults->get("R_V4L2MPEG_IN_DEVICE", v4l2mpeg_in_device);
	defaults->get("R_VIDEO_DVB_IN_ADAPTER", dvb_in_adapter);
	dvb_in_device = defaults->get("R_VIDEO_DVB_IN_DEVICE", dvb_in_device);
	defaults->get("R_SCREENCAPTURE_DISPLAY", screencapture_display);
	firewire_port = defaults->get("R_VFIREWIRE_IN_PORT", firewire_port);
	firewire_channel = defaults->get("R_VFIREWIRE_IN_CHANNEL", firewire_channel);
	defaults->get("R_VFIREWIRE_IN_PATH", firewire_path);
	capture_length = defaults->get("R_VIDEO_CAPTURE_LENGTH", capture_length);
	w = defaults->get("RECORD_W", w);
	h = defaults->get("RECORD_H", h);
	in_framerate = defaults->get("R_IN_FRAMERATE", in_framerate);
	follow_video = defaults->get("R_FOLLOW_VIDEO", follow_video);
	return 0;
}

int VideoInConfig::save_defaults(BC_Hash *defaults)
{
	defaults->update("R_VIDEO_IN_DRIVER", driver);
	defaults->update("R_V4L2_IN_DEVICE", v4l2_in_device);
	defaults->update("R_V4L2JPEG_IN_DEVICE", v4l2jpeg_in_device);
	defaults->update("R_V4L2JPEG_IN_FIELDS", v4l2jpeg_in_fields);
	defaults->update("R_V4L2MPEG_IN_DEVICE", v4l2mpeg_in_device);
	defaults->update("R_VIDEO_DVB_IN_ADAPTER", dvb_in_adapter);
	defaults->update("R_VIDEO_DVB_IN_DEVICE", dvb_in_device);
	defaults->update("R_SCREENCAPTURE_DISPLAY", screencapture_display);
	defaults->update("R_VFIREWIRE_IN_PORT", firewire_port);
	defaults->update("R_VFIREWIRE_IN_CHANNEL", firewire_channel);
	defaults->update("R_VFIREWIRE_IN_PATH", firewire_path);
	defaults->update("R_VIDEO_CAPTURE_LENGTH", capture_length);
	defaults->update("RECORD_W", w);
	defaults->update("RECORD_H", h);
	defaults->update("R_IN_FRAMERATE", in_framerate);
	defaults->update("R_FOLLOW_VIDEO", follow_video);
	return 0;
}

