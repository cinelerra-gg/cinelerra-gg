
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

#include "clip.h"
#include "bchash.h"
#include "playbackconfig.h"
#include "videodevice.inc"
#include <string.h>

AudioOutConfig::AudioOutConfig()
{
	fragment_size = 16384;
#ifdef HAVE_ALSA
	driver = AUDIO_ALSA;
#else
	driver = AUDIO_OSS;
#endif

	audio_offset = 0.0;
	map51_2 = 0;
	play_gain = 1.0;

	oss_out_bits = 16;
	for(int i = 0; i < MAXDEVICES; i++)
	{
		oss_enable[i] = (i == 0);
		sprintf(oss_out_device[i], "/dev/dsp");
	}

	esound_out_server[0] = 0;
	esound_out_port = 0;

	sprintf(alsa_out_device, "default");
	alsa_out_bits = 16;
	interrupt_workaround = 0;

	firewire_channel = 63;
	firewire_port = 0;
	strcpy(firewire_path, "/dev/video1394");
	firewire_syt = 30000;

	dv1394_channel = 63;
	dv1394_port = 0;
	strcpy(dv1394_path, "/dev/dv1394");
	dv1394_syt = 30000;
}

AudioOutConfig::~AudioOutConfig()
{
}



int AudioOutConfig::operator!=(AudioOutConfig &that)
{
	return !(*this == that);
}

int AudioOutConfig::operator==(AudioOutConfig &that)
{
	return
		fragment_size == that.fragment_size &&
		driver == that.driver &&
		EQUIV(audio_offset, that.audio_offset) &&
		play_gain == that.play_gain &&

		!strcmp(oss_out_device[0], that.oss_out_device[0]) &&
		(oss_out_bits == that.oss_out_bits) &&



		!strcmp(esound_out_server, that.esound_out_server) &&
		(esound_out_port == that.esound_out_port) &&



		!strcmp(alsa_out_device, that.alsa_out_device) &&
		(alsa_out_bits == that.alsa_out_bits) &&
		(interrupt_workaround == that.interrupt_workaround) &&

		firewire_channel == that.firewire_channel &&
		firewire_port == that.firewire_port &&
		firewire_syt == that.firewire_syt &&
		!strcmp(firewire_path, that.firewire_path) &&

		dv1394_channel == that.dv1394_channel &&
		dv1394_port == that.dv1394_port &&
		dv1394_syt == that.dv1394_syt &&
		!strcmp(dv1394_path, that.dv1394_path);
}



AudioOutConfig& AudioOutConfig::operator=(AudioOutConfig &that)
{
	copy_from(&that);
	return *this;
}

void AudioOutConfig::copy_from(AudioOutConfig *src)
{
	if( this == src ) return;
	fragment_size = src->fragment_size;
	driver = src->driver;
	audio_offset = src->audio_offset;
	map51_2 = src->map51_2;
	play_gain = src->play_gain;

	strcpy(esound_out_server, src->esound_out_server);
	esound_out_port = src->esound_out_port;
	for(int i = 0; i < MAXDEVICES; i++)
	{
		oss_enable[i] = src->oss_enable[i];
		strcpy(oss_out_device[i], src->oss_out_device[i]);
	}
	oss_out_bits = src->oss_out_bits;

	strcpy(alsa_out_device, src->alsa_out_device);
	alsa_out_bits = src->alsa_out_bits;
	interrupt_workaround = src->interrupt_workaround;

	firewire_channel = src->firewire_channel;
	firewire_port = src->firewire_port;
	firewire_syt = src->firewire_syt;
	strcpy(firewire_path, src->firewire_path);

	dv1394_channel = src->dv1394_channel;
	dv1394_port = src->dv1394_port;
	dv1394_syt = src->dv1394_syt;
	strcpy(dv1394_path, src->dv1394_path);
}

int AudioOutConfig::load_defaults(BC_Hash *defaults, int active_config)
{
	char prefix[BCTEXTLEN];
	sprintf(prefix, "%c_", 'A'+active_config);

	fragment_size = defaults->getf(fragment_size, "%sFRAGMENT_SIZE", prefix);
	audio_offset = defaults->getf(audio_offset, "%sAUDIO_OFFSET", prefix);
	map51_2 = defaults->getf(map51_2, "%sAUDIO_OUT_MAP51_2", prefix);
	play_gain = defaults->getf(play_gain, "%sAUDIO_OUT_GAIN", prefix);
	driver = defaults->getf(driver, "%sAUDIO_OUT_DRIVER", prefix);

	for(int i = 0; i < MAXDEVICES; i++)
	{
		oss_enable[i] = defaults->getf(oss_enable[i], "%sOSS_ENABLE_%d", prefix, i);
		defaults->getf(oss_out_device[i], "%sOSS_OUT_DEVICE_%d", prefix, i);
	}
	oss_out_bits = defaults->getf(oss_out_bits, "%sOSS_OUT_BITS", prefix);

	defaults->getf(alsa_out_device, "%sALSA_OUT_DEVICE", prefix);
	alsa_out_bits = defaults->getf(alsa_out_bits, "%sALSA_OUT_BITS", prefix);
	interrupt_workaround = defaults->getf(interrupt_workaround, "%sALSA_INTERRUPT_WORKAROUND", prefix);

	defaults->getf(esound_out_server, "%sESOUND_OUT_SERVER", prefix);
	esound_out_port = defaults->getf(esound_out_port, "%sESOUND_OUT_PORT", prefix);

	firewire_channel = defaults->getf(firewire_channel, "%sAFIREWIRE_OUT_CHANNEL", prefix);
	firewire_port = defaults->getf(firewire_port, "%sAFIREWIRE_OUT_PORT", prefix);
	defaults->getf(firewire_path, "%sAFIREWIRE_OUT_PATH", prefix);
	firewire_syt = defaults->getf(firewire_syt, "%sAFIREWIRE_OUT_SYT", prefix);

	dv1394_channel = defaults->getf(dv1394_channel, "%sADV1394_OUT_CHANNEL", prefix);
	dv1394_port = defaults->getf(dv1394_port, "%sADV1394_OUT_PORT", prefix);
	defaults->getf(dv1394_path, "%sADV1394_OUT_PATH", prefix);
	dv1394_syt = defaults->getf(dv1394_syt, "%sADV1394_OUT_SYT", prefix);

	return 0;
}

int AudioOutConfig::save_defaults(BC_Hash *defaults, int active_config)
{
	char prefix[BCTEXTLEN];
	sprintf(prefix, "%c_", 'A'+active_config);

	defaults->updatef(fragment_size, "%sFRAGMENT_SIZE", prefix);
	defaults->updatef(audio_offset, "%sAUDIO_OFFSET", prefix);
	defaults->updatef(map51_2, "%sAUDIO_OUT_MAP51_2", prefix);
	defaults->updatef(play_gain, "%sAUDIO_OUT_GAIN", prefix);

	defaults->updatef(driver, "%sAUDIO_OUT_DRIVER", prefix);

	for(int i = 0; i < MAXDEVICES; i++)
	{
		defaults->updatef(oss_enable[i], "%sOSS_ENABLE_%d", prefix, i);
		defaults->updatef(oss_out_device[i], "%sOSS_OUT_DEVICE_%d", prefix, i);
	}
	defaults->updatef(oss_out_bits, "%sOSS_OUT_BITS", prefix);


	defaults->updatef(alsa_out_device, "%sALSA_OUT_DEVICE", prefix);
	defaults->updatef(alsa_out_bits, "%sALSA_OUT_BITS", prefix);
	defaults->updatef(interrupt_workaround, "%sALSA_INTERRUPT_WORKAROUND", prefix);

	defaults->updatef(esound_out_server, "%sESOUND_OUT_SERVER", prefix);
	defaults->updatef(esound_out_port, "%sESOUND_OUT_PORT", prefix);

	defaults->updatef(firewire_channel, "%sAFIREWIRE_OUT_CHANNEL", prefix);
	defaults->updatef(firewire_port, "%sAFIREWIRE_OUT_PORT", prefix);
	defaults->updatef(firewire_path, "%sAFIREWIRE_OUT_PATH", prefix);
	defaults->updatef(firewire_syt, "%sAFIREWIRE_OUT_SYT", prefix);


	defaults->updatef(dv1394_channel, "%sADV1394_OUT_CHANNEL", prefix);
	defaults->updatef(dv1394_port, "%sADV1394_OUT_PORT", prefix);
	defaults->updatef(dv1394_path, "%sADV1394_OUT_PATH", prefix);
	defaults->updatef(dv1394_syt, "%sADV1394_OUT_SYT", prefix);

	return 0;
}









const char *VideoOutConfig::default_video_device = "/dev/video0";

VideoOutConfig::VideoOutConfig()
{
	driver = PLAYBACK_X11;
	x11_host[0] = 0;
	x11_use_fields = USE_NO_FIELDS;

	firewire_channel = 63;
	firewire_port = 0;
	strcpy(firewire_path, "/dev/video1394");
	firewire_syt = 30000;

	dv1394_channel = 63;
	dv1394_port = 0;
	strcpy(dv1394_path, "/dev/dv1394");
	dv1394_syt = 30000;

	brightness = 32768;
	hue = 32768;
	color = 32768;
	contrast = 32768;
	whiteness = 32768;
	out_channel = -1;
	use_direct_x11 = 1;
}

VideoOutConfig::~VideoOutConfig()
{
}


int VideoOutConfig::operator!=(VideoOutConfig &that)
{
	return !(*this == that);
}

int VideoOutConfig::operator==(VideoOutConfig &that)
{
	return (driver == that.driver) &&
		!strcmp(x11_host, that.x11_host) &&
		(x11_use_fields == that.x11_use_fields) &&
		(use_direct_x11 == that.use_direct_x11) &&
		(brightness == that.brightness) &&
		(hue == that.hue) &&
		(color == that.color) &&
		(contrast == that.contrast) &&
		(whiteness == that.whiteness) &&

		(firewire_channel == that.firewire_channel) &&
		(firewire_port == that.firewire_port) &&
		!strcmp(firewire_path, that.firewire_path) &&
		(firewire_syt == that.firewire_syt) &&

		(dv1394_channel == that.dv1394_channel) &&
		(dv1394_port == that.dv1394_port) &&
		!strcmp(dv1394_path, that.dv1394_path) &&
		(dv1394_syt == that.dv1394_syt);
}






VideoOutConfig& VideoOutConfig::operator=(VideoOutConfig &that)
{
	copy_from(&that);
	return *this;
}

void VideoOutConfig::copy_from(VideoOutConfig *src)
{
	this->driver = src->driver;
	strcpy(this->x11_host, src->x11_host);
	this->x11_use_fields = src->x11_use_fields;
	this->use_direct_x11 = src->use_direct_x11;

	firewire_channel = src->firewire_channel;
	firewire_port = src->firewire_port;
	strcpy(firewire_path, src->firewire_path);
	firewire_syt = src->firewire_syt;

	dv1394_channel = src->dv1394_channel;
	dv1394_port = src->dv1394_port;
	strcpy(dv1394_path, src->dv1394_path);
	dv1394_syt = src->dv1394_syt;
}

const char *VideoOutConfig::get_path()
{
	switch(driver)
	{
		case PLAYBACK_X11:
		case PLAYBACK_X11_XV:
			return x11_host;
			break;
		case PLAYBACK_DV1394:
			return dv1394_path;
			break;
		case PLAYBACK_FIREWIRE:
			return firewire_path;
			break;
	};
	return default_video_device;
}

int VideoOutConfig::load_defaults(BC_Hash *defaults, int active_config)
{
	char prefix[BCTEXTLEN];
	sprintf(prefix, "%c_", 'A'+active_config);

	driver = defaults->getf(driver, "%sVIDEO_OUT_DRIVER", prefix);
	defaults->getf(x11_host, "%sX11_OUT_DEVICE", prefix);
	x11_use_fields = defaults->getf(x11_use_fields, "%sX11_USE_FIELDS", prefix);



	firewire_channel = defaults->getf(firewire_channel, "%sVFIREWIRE_OUT_CHANNEL", prefix);
	firewire_port = defaults->getf(firewire_port, "%sVFIREWIRE_OUT_PORT", prefix);
	defaults->getf(firewire_path, "%sVFIREWIRE_OUT_PATH", prefix);
	firewire_syt = defaults->getf(firewire_syt, "%sVFIREWIRE_OUT_SYT", prefix);

	dv1394_channel = defaults->getf(dv1394_channel, "%sVDV1394_OUT_CHANNEL", prefix);
	dv1394_port = defaults->getf(dv1394_port, "%sVDV1394_OUT_PORT", prefix);
	defaults->getf(dv1394_path, "%sVDV1394_OUT_PATH", prefix);
	dv1394_syt = defaults->getf(dv1394_syt, "%sVDV1394_OUT_SYT", prefix);
	return 0;
}

int VideoOutConfig::save_defaults(BC_Hash *defaults, int active_config)
{
	char prefix[BCTEXTLEN];
	sprintf(prefix, "%c_", 'A'+active_config);

	defaults->updatef(driver, "%sVIDEO_OUT_DRIVER", prefix);
	defaults->updatef(x11_host, "%sX11_OUT_DEVICE", prefix);
	defaults->updatef(x11_use_fields, "%sX11_USE_FIELDS", prefix);

	defaults->updatef(firewire_channel, "%sVFIREWIRE_OUT_CHANNEL", prefix);
	defaults->updatef(firewire_port, "%sVFIREWIRE_OUT_PORT", prefix);
	defaults->updatef(firewire_path, "%sVFIREWIRE_OUT_PATH", prefix);
	defaults->updatef(firewire_syt, "%sVFIREWIRE_OUT_SYT", prefix);

	defaults->updatef(dv1394_channel, "%sVDV1394_OUT_CHANNEL", prefix);
	defaults->updatef(dv1394_port, "%sVDV1394_OUT_PORT", prefix);
	defaults->updatef(dv1394_path, "%sVDV1394_OUT_PATH", prefix);
	defaults->updatef(dv1394_syt, "%sVDV1394_OUT_SYT", prefix);
	return 0;
}









PlaybackConfig::PlaybackConfig()
{
	aconfig = new AudioOutConfig;
	vconfig = new VideoOutConfig;
	sprintf(hostname, "localhost");
	active_config = 0;
	port = 23456;
}

PlaybackConfig::~PlaybackConfig()
{
	delete aconfig;
	delete vconfig;
}

PlaybackConfig& PlaybackConfig::operator=(PlaybackConfig &that)
{
	copy_from(&that);
	return *this;
}

void PlaybackConfig::copy_from(PlaybackConfig *src)
{
	active_config = src->active_config;
	aconfig->copy_from(src->aconfig);
	vconfig->copy_from(src->vconfig);
	strncpy(hostname, src->hostname, sizeof(hostname));
	port = src->port;
}

int PlaybackConfig::load_defaults(BC_Hash *defaults, int load_config)
{
	active_config = load_config >= 0 ? load_config :
		defaults->get("PLAYBACK_ACTIVE_CONFIG", active_config);
	aconfig->load_defaults(defaults, active_config);
	vconfig->load_defaults(defaults, active_config);
	defaults->get("PLAYBACK_HOSTNAME", hostname);
	port = defaults->get("PLAYBACK_PORT", port);
	return 0;
}

int PlaybackConfig::save_defaults(BC_Hash *defaults)
{
	defaults->update("PLAYBACK_ACTIVE_CONFIG", active_config);
	aconfig->save_defaults(defaults, active_config);
	vconfig->save_defaults(defaults, active_config);
	defaults->update("PLAYBACK_HOSTNAME", hostname);
	defaults->update("PLAYBACK_PORT", port);
	return 0;
}


