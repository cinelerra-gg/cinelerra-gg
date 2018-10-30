
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
#include "condition.h"
#include "devicempeginput.h"




AudioMPEG::AudioMPEG(AudioDevice *device)
 : AudioLowLevel(device)
{
	audio_open = 0;
	mpeg_input = 0;
}

AudioMPEG::~AudioMPEG()
{
	close_all();
}


int AudioMPEG::open_input()
{
	/* this is more like - try to open, since a broadcast signal may */
	/*  take time to lock, or not even be there at all.  On success, */
	/*  it may update the input config with the actual stream config. */
	int ret = 1;
	if( !mpeg_input )
	{
		mpeg_input = get_mpeg_input();
		if( !mpeg_input ) return 1;
	}
	device->in_samplerate = device->in_channels = device->in_bits = 0;
	if( !mpeg_input->src_stream() ) {
		int samplerate = mpeg_input->audio_sample_rate();
		int channels = mpeg_input->audio_channels();
		int bits = mpeg_input->audio_sample_bits();
		device->auto_update(channels, samplerate, bits);
		audio_open = 1;
		mpeg_input->src_unlock();
		ret = 0;
	}
	return ret;
}

int AudioMPEG::close_all()
{
	audio_open = 0;
	if( mpeg_input ) {
		mpeg_input->put_mpeg_audio();
		mpeg_input = 0;
	}
	return 0;
}

int AudioMPEG::write_buffer(char *buffer, int size)
{
	printf("AudioMPEG::write_position 10\n");
	return 0;
}

int AudioMPEG::read_buffer(char *buffer, int size)
{
	if( !audio_open ) { open_input(); return -1; }
	int result = mpeg_input->read_audio(buffer,size);
//printf("AudioMPEG::read_buffer(%p,%d) = %d\n", buffer, size, result);
	return result;
}

double AudioMPEG::device_timestamp()
{
	if( !audio_open || !mpeg_input ) return -1.;
	return mpeg_input->audio_timestamp();
}

int AudioMPEG::start_toc(const char *path, const char *toc_path)
{
	int result = mpeg_input ? mpeg_input->start_toc(path, toc_path) : -1;
	return result;
}

int AudioMPEG::start_record(int fd, int bsz)
{
	int result = mpeg_input ? mpeg_input->start_record(fd, bsz) : -1;
	return result;
}

int AudioMPEG::stop_record()
{
	int result = mpeg_input ? mpeg_input->stop_record() : -1;
	return result;
}

int AudioMPEG::total_audio_channels()
{
	int result = mpeg_input ? mpeg_input->total_audio_channels() : 0;
	return result;
}

int64_t AudioMPEG::device_position()
{
	printf("AudioMPEG::device_position 10\n");
	return 0;
}

int AudioMPEG::flush_device()
{
	printf("AudioMPEG::flush_device 10\n");
	return 0;
}

int AudioMPEG::interrupt_playback()
{
	printf("AudioMPEG::interrupt_playback 10\n");
	return 0;
}





