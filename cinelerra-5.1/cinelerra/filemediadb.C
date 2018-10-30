
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
#include "commercials.h"
#include "filebase.h"
#include "file.h"
#include "filemediadb.h"
#include "mwindow.h"
#include "preferences.h"
#include "vframe.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>


FileMediaDb::FileMediaDb(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset_parameters();
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_MEDIADB;
	clip_id = -1;
	swidth = (SWIDTH+1) & ~1;
	sheight = (SHEIGHT+1) & ~1;
	seq_no = frame_id = frames = -1;
	framerate = offset = -1;;
	title[0] = 0;
}

FileMediaDb::~FileMediaDb()
{
	close_file();
}

int FileMediaDb::check_sig(Asset *asset)
{
	return !strncmp(asset->path,"/mediadb:",9);
}

int FileMediaDb::open_file(int rd, int wr)
{
	int result = 0;

	if(rd) {
		char *cp = 0;
		clip_id = strtol(asset->path+9,&cp,0);
		if( !cp || cp == asset->path+9 || *cp != 0 ) result = 1;
		if( !result ) result = MWindow::commercials->
			get_clip_set(clip_id, title, framerate, frames);
		if( !result ) {
			asset->audio_data = 0;
			asset->video_data = 1;
			asset->actual_width = swidth;
			asset->actual_height = sheight;
			if( !asset->layers ) asset->layers = 1;
			if( !asset->width ) asset->width = asset->actual_width;
			if( !asset->height ) asset->height = asset->actual_height;
			if( !asset->video_length ) asset->video_length = frames;
			if( !asset->frame_rate ) asset->frame_rate = framerate;
		}
	}

	if(!result && wr ) {
		result = 1;
	}

	return result;
}

int FileMediaDb::close_file()
{
	reset_parameters();

	FileBase::close_file();
	return 0;
}

int FileMediaDb::get_best_colormodel(Asset *asset, int driver)
{
	return BC_YUV420P;
}

int FileMediaDb::colormodel_supported(int colormodel)
{
	return BC_YUV420P;
}


int FileMediaDb::set_video_position(int64_t pos)
{
	if( pos < 0 || pos >= asset->video_length )
		return 1;
	seq_no = pos-1;
	return 0;
}

int64_t FileMediaDb::get_memory_usage()
{
	return 0;
}



int FileMediaDb::write_frames(VFrame ***frames, int len)
{
	int result = 0;
	return result;
}


int FileMediaDb::read_frame(VFrame *frame)
{
	int sw, sh;
	int result = MWindow::commercials->
		get_clip_seq_no(clip_id, seq_no, offset, frame_id);
	if( !result ) result = MWindow::commercials->
		get_image(frame_id, frame->get_y(), sw,sh);
	if( !result ) {
		memset(frame->get_u(),0x80,swidth/2 * sheight/2);
		memset(frame->get_v(),0x80,swidth/2 * sheight/2);
	}
	return result;
}


