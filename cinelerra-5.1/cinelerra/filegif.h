
/*
 * CINELERRA
 * Copyright (C) 2014 Adam Williams <broadcast at earthling dot net>
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

#ifndef FILEGIF_H
#define FILEGIF_H

#include "arraylist.h"
#include "file.inc"
#include "filelist.h"
#include "vframe.inc"

#include "gif_lib.h"

// This header file is representative of any single frame file format.

class FileGIF : public FileBase
{
public:
	FileGIF(Asset *asset, File *file);
	~FileGIF();

	static int get_best_colormodel(Asset *asset, int driver);
	static int check_sig(Asset *asset);
	int colormodel_supported(int colormodel);
	int open_file(int rd, int wr);
	int ropen_path(const char *path);
	int wopen_path(const char *path);
	int wopen_data(VFrame *frame);
	int open_gif();
	int close_file();
	int set_video_position(int64_t pos);

	int read_frame_header(char *path);
	int read_frame(VFrame *output);
	int read_next_image(VFrame *output);
	int scan_gif();
	int write_frames(VFrame ***frames, int len);
	int write_frame(VFrame *frame);

	int64_t offset;
	int err, eof;
	int fd, depth, writes;
	int rows, row_size;
	FILE *fp;
	GifFileType *gif_file;
	GifPixelType *bg;
	GifRowType *buffer;
	ArrayList<int64_t> file_pos;
	VFrame *output;
};

class FileGIFList : public FileList
{
public:
	FileGIFList(Asset *asset, File *file);
	~FileGIFList();

	static int check_sig(Asset *asset);
	int colormodel_supported(int colormodel);
	int get_best_colormodel(Asset *asset, int driver);
	int read_frame_header(char *path);
	int use_path() { return 1; }
	int read_frame(VFrame *output, char *path);
	int write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit);
	FrameWriterUnit* new_writer_unit(FrameWriter *writer);
// Need to override this method from FILEBASE class
	int verify_file_list();
};

class GIFUnit : public FrameWriterUnit
{
public:
	GIFUnit(FileGIFList *file, FrameWriter *writer);
	~GIFUnit();

	FileGIFList *file;
	VFrame *temp_frame;
};

#endif
