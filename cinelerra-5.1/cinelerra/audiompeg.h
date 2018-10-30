
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

#ifndef AUDIOMPEG_H
#define AUDIOMPEG_H


#include "audiodevice.h"
#include "devicempeginput.inc"
#include "audiompeg.inc"
#include "vdevicempeg.inc"



// This reads audio from the DVB input and output uncompressed audio only.
// Used for the LiveAudio plugin and previewing.




class AudioMPEG : public AudioLowLevel
{
public:
	AudioMPEG(AudioDevice *device);
	~AudioMPEG();

	friend class VDeviceMPEG;
	virtual DeviceMPEGInput *get_mpeg_input() = 0;

	int open_input();
	int close_all();
	int write_buffer(char *buffer, int size);
	int read_buffer(char *buffer, int size);
	int64_t device_position();
	int flush_device();
	int interrupt_playback();
	double device_timestamp();
	int start_toc(const char *path, const char *toc_path);
	int start_record(int fd, int bsz);
	int stop_record();
	int total_audio_channels();
	DeviceMPEGInput *mpeg_device() { return mpeg_input; }

	int audio_open;
	DeviceMPEGInput *mpeg_input;
};







#endif
