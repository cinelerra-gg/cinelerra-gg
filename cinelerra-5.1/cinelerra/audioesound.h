
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

#ifndef AUDIOESOUND_H
#define AUDIOESOUND_H

#include "audiodevice.inc"

#ifdef HAVE_ESOUND

class AudioESound : public AudioLowLevel
{
public:
	AudioESound(AudioDevice *device);
	~AudioESound();

	int open_input();
	int open_output();
	int write_buffer(char *buffer, int size);
	int read_buffer(char *buffer, int size);
	int close_all();
	int64_t device_position();
	int flush_device();
	int interrupt_playback();

private:
	int get_bit_flag(int bits);
	int get_channels_flag(int channels);
	char* translate_device_string(char *server, int port);
	int esd_in, esd_out;
	int esd_in_fd, esd_out_fd;
	char device_string[1024];
};

#endif
#endif
