#ifndef _FILEMEDIADB_H_
#define _FILEMEDIADB_H_
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

#include "asset.inc"
#include "filebase.h"
#include "file.inc"
#include "filemediadb.inc"
#include "vframe.inc"



class FileMediaDb : public FileBase
{
public:
	int clip_id;
	int swidth, sheight;
	int seq_no, frame_id, frames;
	char title[BCTEXTLEN];
	double framerate, offset;

        static int check_sig(Asset *asset);
	int open_file(int rd, int wr);
	int close_file();

	int get_best_colormodel(Asset *asset, int driver);
	int colormodel_supported(int colormodel);
	int set_video_position(int64_t pos);
	int64_t get_memory_usage();

	int write_frames(VFrame ***frames, int len);
	int read_frame(VFrame *frame);

        FileMediaDb(Asset *asset, File *file);
        ~FileMediaDb();
};

#endif
