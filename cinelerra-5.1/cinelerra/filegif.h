
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

#include "file.inc"
#include "filelist.h"
#include "vframe.inc"

// This header file is representative of any single frame file format.

class FileGIF : public FileList
{
public:
	FileGIF(Asset *asset, File *file);
	~FileGIF();

	static int get_best_colormodel(Asset *asset, int driver);
	static int check_sig(Asset *asset);
	int colormodel_supported(int colormodel);

	int read_frame_header(char *path);
	int read_frame(VFrame *output, VFrame *input);

	unsigned char *data;
	int offset;
	int size;
};


#endif
