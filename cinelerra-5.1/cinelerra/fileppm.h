
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

#ifndef FILEPPM_H
#define FILEPPM_H

#include "edl.inc"
#include "filelist.h"
#include "fileppm.inc"

class FilePPM : public FileList
{
public:
	FilePPM(Asset *asset, File *file);
	~FilePPM();

	void reset();
	static int check_sig(Asset *asset);
	int read_ppm(FILE *fp, VFrame *frame);
	int use_path();

	int read_frame(VFrame *frame, char *path);
	int colormodel_supported(int colormodel);
	static int get_best_colormodel(Asset *asset, int driver);
	int check_frame_header(FILE *fp);
	int read_frame_header(char *path);
	int write_frame(VFrame *frame, VFrame *output, FrameWriterUnit *unit);
	FrameWriterUnit* new_writer_unit(FrameWriter *writer);
	static void get_parameters(BC_WindowBase *parent_window,
		Asset *asset, BC_WindowBase* &format_window,
		int audio_options, int video_options, EDL *edl);
};

class PPMConfigVideo : public BC_Window
{
public:
	PPMConfigVideo(BC_WindowBase *gui, Asset *asset);
	void create_objects();

	BC_WindowBase *gui;
	Asset *asset;
};

class PPMUnit : public FrameWriterUnit
{
public:
	PPMUnit(FilePPM *file, FrameWriter *writer);
	~PPMUnit();

	FilePPM *file;
	VFrame *temp_frame;
};

#endif
