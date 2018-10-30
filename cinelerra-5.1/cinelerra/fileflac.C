
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "FLAC/stream_decoder.h"
#include "FLAC/stream_encoder.h"

#define IS_FILEFLAC
#include "asset.h"
#include "bcsignals.h"
#include "bitspopup.h"
#include "byteorder.h"
#include "clip.h"
#include "file.h"
#include "fileflac.h"
#include "filesystem.h"
#include "guicast.h"
#include "language.h"
#include "mwindow.inc"


FileFLAC::FileFLAC(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset_parameters();
	if(asset->format == FILE_UNKNOWN) asset->format = FILE_FLAC;
	asset->byte_order = 0;
}

FileFLAC::~FileFLAC()
{
	close_file();
}

void FileFLAC::get_parameters(BC_WindowBase *parent_window,
	Asset *asset, BC_WindowBase* &format_window,
	int audio_options, int video_options, EDL *edl)
{
	if(audio_options)
	{
		FLACConfigAudio *window = new FLACConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}



int FileFLAC::check_sig(Asset *asset, char *test)
{
	if(test[0] == 'f' &&
		test[1] == 'L' &&
		test[2] == 'a' &&
		test[3] == 'C')
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int FileFLAC::reset_parameters_derived()
{
	pcm_history = 0;
	flac_decode = 0;
	flac_encode = 0;
	temp_buffer = 0;
	temp_allocated = 0;
	temp_channels = 0;
	initialized = 0;
	is_reading = 0;
	return 0;
}


static FLAC__StreamDecoderWriteStatus write_callback(
	const FLAC__StreamDecoder *decoder,
	const FLAC__Frame *frame,
	const FLAC__int32 * const buffer[],
	void *client_data)
{
	FileFLAC *ptr = (FileFLAC*)client_data;

	if(!ptr->initialized)
	{
		ptr->initialized = 1;
		ptr->asset->audio_data = 1;
		ptr->asset->channels = FLAC__stream_decoder_get_channels(ptr->flac_decode);
		ptr->asset->bits = FLAC__stream_decoder_get_bits_per_sample(ptr->flac_decode);
		ptr->asset->sample_rate = FLAC__stream_decoder_get_sample_rate(ptr->flac_decode);
		ptr->asset->audio_length = FLAC__stream_decoder_get_total_samples(ptr->flac_decode);
		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}
	else
	if(ptr->is_reading)
	{
		int fragment = FLAC__stream_decoder_get_blocksize(ptr->flac_decode);
		ptr->samples_read += fragment;
//printf("write_callback 2 %d\n", fragment);

		if(ptr->temp_allocated < fragment)
		{
			if(ptr->temp_buffer)
			{
				for(int i = 0; i < ptr->temp_channels; i++)
				{
					delete [] ptr->temp_buffer[i];
				}
				delete [] ptr->temp_buffer;
			}

			ptr->temp_channels = ptr->asset->channels;
			ptr->temp_buffer = new int32_t*[ptr->temp_channels];
			for(int i = 0; i < ptr->temp_channels; i++)
			{
				ptr->temp_buffer[i] = new int32_t[fragment];
			}
			ptr->temp_allocated = fragment;
		}

		float audio_max = (float)0x7fff;
		if(FLAC__stream_decoder_get_bits_per_sample(ptr->flac_decode) == 24)
			audio_max = (float)0x7fffff;

		int nchannels = FLAC__stream_decoder_get_channels(ptr->flac_decode);
		if( nchannels > ptr->temp_channels ) nchannels = ptr->temp_channels;
		for(int j = 0; j < nchannels; j++)
		{
			float *dst = (float*)ptr->temp_buffer[j];
			int32_t *src = (int32_t*)buffer[j];
			for(int i = 0; i < fragment; i++)
			{
				*dst++ = (float)*src++ / audio_max;
			}
		}

		ptr->append_history((float**)ptr->temp_buffer, fragment);

//printf("write_callback 3\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}


static void metadata_callback(const FLAC__StreamDecoder *decoder,
	const FLAC__StreamMetadata *metadata,
	void *client_data)
{
//	printf("metadata_callback\n");
}

static void error_callback(const FLAC__StreamDecoder *decoder,
	FLAC__StreamDecoderErrorStatus status,
	void *client_data)
{
//	printf("error_callback\n");
}





int FileFLAC::open_file(int rd, int wr)
{
	int result = 0;

	if(rd)
	{
		file_bytes = FileSystem::get_size(asset->path);
		flac_decode = FLAC__stream_decoder_new();
		FLAC__stream_decoder_init_file(flac_decode,
			asset->path,
			write_callback,
			metadata_callback,
			error_callback,
			this);

		initialized = 0;
		while(!initialized)
		{
			if(!FLAC__stream_decoder_process_single(flac_decode)) break;
		}

		if(!initialized)
			result = 1;
		else
		{
			FLAC__stream_decoder_seek_absolute(flac_decode, 0);
		}
	}

	if(wr)
	{
		flac_encode = FLAC__stream_encoder_new();
		FLAC__stream_encoder_set_channels(flac_encode, asset->channels);
		FLAC__stream_encoder_set_bits_per_sample(flac_encode, asset->bits);
		FLAC__stream_encoder_set_sample_rate(flac_encode, asset->sample_rate);
		FLAC__stream_encoder_init_file(flac_encode,
			asset->path,
			0,
			0);
	}

	return result;
}


int FileFLAC::close_file_derived()
{
	if(flac_decode)
	{
		FLAC__stream_decoder_finish(flac_decode);
		FLAC__stream_decoder_delete(flac_decode);
	}

	if(flac_encode)
	{
		FLAC__stream_encoder_finish(flac_encode);
		FLAC__stream_encoder_delete(flac_encode);
	}


	if(temp_buffer)
	{
		for(int i = 0; i < temp_channels; i++)
			delete [] temp_buffer[i];
		delete [] temp_buffer;
	}
	return 0;
}


int FileFLAC::write_samples(double **buffer, int64_t len)
{
// Create temporary buffer of samples
	if(temp_allocated < len)
	{
		if(temp_buffer)
		{
			for(int i = 0; i < asset->channels; i++)
			{
				delete [] temp_buffer[i];
			}
			delete [] temp_buffer;
		}

		temp_buffer = new int32_t*[asset->channels];
		for(int i = 0; i < asset->channels; i++)
		{
			temp_buffer[i] = new int32_t[len];
		}
		temp_allocated = len;
	}

	float audio_max = (float)0x7fff;

	switch(asset->bits)
	{
		case 24:
			audio_max = (float)0x7fffff;
			break;
	}

	for(int i = 0; i < asset->channels; i++)
	{
		int32_t *dst = temp_buffer[i];
		double *src = buffer[i];
		for(int j = 0; j < len; j++)
		{
			double sample = *src++ * audio_max;
			CLAMP(sample, -audio_max, audio_max);
			*dst++ = (int32_t)sample;
		}
	}

	int result = FLAC__stream_encoder_process(flac_encode,
		temp_buffer,
		len);

	return !result;
}

int FileFLAC::read_samples(double *buffer, int64_t len)
{
	int result = 0;
	update_pcm_history(len);

	if(decode_end != decode_start)
	{
		FLAC__stream_decoder_seek_absolute(flac_decode, decode_start);
		decode_end = decode_start;
	}

	samples_read = 0;
	is_reading = 1;

	while(samples_read < decode_len)
	{
		FLAC__uint64 position;
		FLAC__stream_decoder_get_decode_position(flac_decode, &position);
// EOF
		if((int64_t)position >= file_bytes) break;
		if(!FLAC__stream_decoder_process_single(flac_decode)) {
			result = 1;
			break;
		}
	}
	is_reading = 0;

	if(!result)
	{
		read_history(buffer,
			file->current_sample,
			file->current_channel,
			len);
	}

//printf("FileFLAC::read_samples 10\n");
	return result;
}










FLACConfigAudio::FLACConfigAudio(BC_WindowBase *parent_window,
	Asset *asset)
 : BC_Window(_(PROGRAM_NAME ": Audio Compression"),
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	350,
	170,
	-1,
	-1,
	0,
	0,
	1)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

FLACConfigAudio::~FLACConfigAudio()
{
}

void FLACConfigAudio::create_objects()
{
	int x = 10, y = 10;
	lock_window("FLACConfigAudio::create_objects");
	bits_popup = new BitsPopup(this, x, y, &asset->bits, 0, 0, 0, 0, 0);
	bits_popup->create_objects();

	add_subwindow(new BC_OKButton(this));
	show_window(1);
	unlock_window();
}

int FLACConfigAudio::close_event()
{
	set_done(0);
	return 1;
}

