
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

#ifndef FILEFLAC_H
#define FILEFLAC_H

#include "bitspopup.inc"
#include "edl.inc"
#include "file.inc"
#include "filebase.h"

class FileFLAC : public FileBase
{
public:
	FileFLAC(Asset *asset, File *file);
	~FileFLAC();

	static void get_parameters(BC_WindowBase *parent_window,
		Asset *asset, BC_WindowBase* &format_window,
		int audio_options, int video_options, EDL *edl);
	int reset_parameters_derived();

	static int check_sig(Asset *asset, char *test);
	int open_file(int rd, int wr);
	int close_file_derived();
	int write_samples(double **buffer,
			int64_t len);

	int read_samples(double *buffer, int64_t len);

// Decoding
// Use parallel symbol types to eliminate any include conflicts
#ifdef IS_FILEFLAC
	FLAC__StreamDecoder *flac_decode;
	FLAC__StreamEncoder *flac_encode;
#else
	void *flac_decode;
	void *flac_encode;
#endif

	int64_t file_bytes;

	int32_t **temp_buffer;
	int temp_allocated;
	int temp_channels;

// State for reading callback
	int initialized;
	int samples_read;
	int is_reading;
};


class FLACConfigAudio;


class FLACConfigAudio : public BC_Window
{
public:
	FLACConfigAudio(BC_WindowBase *parent_window, Asset *asset);
	~FLACConfigAudio();

	void create_objects();
	int close_event();

	BitsPopup *bits_popup;
	BC_WindowBase *parent_window;
	Asset *asset;
};



#endif
