
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

#include "asset.h"
#include "assets.h"
#include "byteorder.h"
#include "bccmodels.h"
#include "file.h"
#include "filebase.h"
#include "overlayframe.h"
#include "sizes.h"

#include <stdlib.h>
#include <string.h>

FileBase::FileBase(Asset *asset, File *file)
{
	this->file = file;
	this->asset = asset;
	internal_byte_order = get_byte_order();
	reset_parameters();
}

FileBase::~FileBase()
{
	close_file();
}

int FileBase::close_file()
{
	delete [] row_pointers_in;   row_pointers_in = 0;
	delete [] row_pointers_out;  row_pointers_out = 0;
	delete [] float_buffer;      float_buffer = 0;

	if( pcm_history ) {
		for(int i = 0; i < history_channels; i++)
			delete [] pcm_history[i];
		delete [] pcm_history;  pcm_history = 0;
	}

	close_file_derived();
	reset_parameters();
	return 0;
}

void FileBase::update_pcm_history(int64_t len)
{
	decode_start = 0;
	decode_len = 0;

	if( !pcm_history ) {
		history_channels = asset->channels;
		pcm_history = new double*[history_channels];
		for(int i = 0; i < history_channels; i++)
			pcm_history[i] = new double[HISTORY_MAX];
		history_start = 0;
		history_size = 0;
		history_allocated = HISTORY_MAX;
	}


//printf("FileBase::update_pcm_history current_sample=%jd history_start=%jd history_size=%jd\n",
//file->current_sample,
//history_start,
//history_size);
// Restart history.  Don't bother shifting history back.
	if(file->current_sample < history_start ||
		file->current_sample > history_start + history_size)
	{
		history_size = 0;
		history_start = file->current_sample;
		decode_start = file->current_sample;
		decode_len = len;
	}
	else
// Shift history forward to make room for new samples
	if(file->current_sample > history_start + HISTORY_MAX)
	{
		int diff = file->current_sample - (history_start + HISTORY_MAX);
		for(int i = 0; i < asset->channels; i++)
		{
			double *temp = pcm_history[i];
			memcpy(temp, temp + diff, (history_size - diff) * sizeof(double));
		}

		history_start += diff;
		history_size -= diff;

// Decode more data
		decode_start = history_start + history_size;
		decode_len = file->current_sample + len - (history_start + history_size);
	}
	else
// Starting somewhere in the buffer
	{
		decode_start = history_start + history_size;
		decode_len = file->current_sample + len - (history_start + history_size);
	}
}

void FileBase::append_history(float **new_data, int len)
{
	allocate_history(len);

	for(int i = 0; i < history_channels; i++)
	{
		double *output = pcm_history[i] + history_size;
		float *input = new_data[i];
		for(int j = 0; j < len; j++)
			*output++ = *input++;
	}

	history_size += len;
	decode_end += len;
}

void FileBase::append_history(short *new_data, int len)
{
	allocate_history(len);

	for(int i = 0; i < history_channels; i++)
	{
		double *output = pcm_history[i] + history_size;
		short *input = new_data + i;
		for(int j = 0; j < len; j++)
		{
			*output++ = (double)*input / 32768;
			input += history_channels;
		}
	}

	history_size += len;
	decode_end += len;
}

void FileBase::read_history(double *dst,
	int64_t start_sample,
	int channel,
	int64_t len)
{
	if(start_sample - history_start + len > history_size)
		len = history_size - (start_sample - history_start);
//printf("FileBase::read_history start_sample=%jd history_start=%jd history_size=%jd len=%jd\n",
//start_sample, history_start, history_size, len);
	double *input = pcm_history[channel] + start_sample - history_start;
	for(int i = 0; i < len; i++)
	{
		*dst++ = *input++;
	}
}

void FileBase::allocate_history(int len)
{
	if(history_size + len > history_allocated)
	{
		double **temp = new double*[history_channels];

		for(int i = 0; i < history_channels; i++)
		{
			temp[i] = new double[history_size + len];
			memcpy(temp[i], pcm_history[i], history_size * sizeof(double));
			delete [] pcm_history[i];
		}

		delete [] pcm_history;
		pcm_history = temp;
		history_allocated = history_size + len;
	}
}

int64_t FileBase::get_history_sample()
{
	return history_start + history_size;
}

int FileBase::set_dither()
{
	dither = 1;
	return 0;
}

void FileBase::reset_parameters()
{
	dither = 0;
	float_buffer = 0;
	row_pointers_in = 0;
	row_pointers_out = 0;
	prev_buffer_position = -1;
	prev_frame_position = -1;
	prev_len = 0;
	prev_bytes = 0;
	prev_track = -1;
	prev_layer = -1;
	ulawtofloat_table = 0;
	floattoulaw_table = 0;
	rd = wr = 0;
	pcm_history = 0;
	history_start = 0;
	history_size = 0;
	history_allocated = 0;
	history_channels = 0;
	decode_end = 0;

	delete_ulaw_tables();
	reset_parameters_derived();
}

void FileBase::get_mode(char *mode, int rd, int wr)
{
	if(rd && !wr) sprintf(mode, "rb");
	else if(!rd && wr) sprintf(mode, "wb");
	else if(rd && wr) {
		int exists = 0;
		FILE *stream = fopen(asset->path, "rb");
		if( stream ) {
			exists = 1;
			fclose(stream);
		}
		sprintf(mode, exists ? "rb+" : "wb+");
	}
}

int FileBase::get_best_colormodel(int driver, int vstream)
{
	return File::get_best_colormodel(asset, driver);
}


// ======================================= audio codecs

int FileBase::get_video_buffer(unsigned char **buffer, int depth)
{
// get a raw video buffer for writing or compression by a library
	if(!*buffer)
	{
// Video compression is entirely done in the library.
		int64_t bytes = asset->width * asset->height * depth;
		*buffer = new unsigned char[bytes];
	}
	return 0;
}

int FileBase::get_row_pointers(unsigned char *buffer, unsigned char ***pointers, int depth)
{
// This might be fooled if a new VFrame is created at the same address with a different height.
	if(*pointers && (*pointers)[0] != &buffer[0])
	{
		delete [] *pointers;
		*pointers = 0;
	}

	if(!*pointers)
	{
		*pointers = new unsigned char*[asset->height];
		for(int i = 0; i < asset->height; i++)
		{
			(*pointers)[i] = &buffer[i * asset->width * depth / 8];
		}
	}
	return 0;
}

int FileBase::match4(const char *in, const char *out)
{
	return  in[0] == out[0] && in[1] == out[1] &&
		in[2] == out[2] && in[3] == out[3] ? 1 : 0;
}

int FileBase::search_render_strategies(ArrayList<int>* render_strategies, int render_strategy)
{
	for(int i = 0; i < render_strategies->total; i++)
		if(render_strategies->values[i] == render_strategy) return 1;
	return 0;
}


int64_t FileBase::base_memory_usage()
{
//printf("FileBase::base_memory_usage %d\n", __LINE__);
	return !pcm_history ? 0 :
		history_allocated * history_channels * sizeof(double);
}


