
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
#include "filebase.h"
#include "file.h"
#include "filedb.h"
#include "mediadb.h"
#include "mwindow.h"
#include "preferences.h"
#include "vframe.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>


FileDB::FileDB(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset_parameters();
	mdb = new MediaDb();
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_DB;
	clip_id = -1;
	swidth = (SWIDTH+1) & ~1;
	sheight = (SHEIGHT+1) & ~1;
	frame_id = clip_size = -1;
	seq_no = 0;
	prefix_size = suffix_offset = -1;
	framerate = -1;
	vframe = 0;
}

FileDB::~FileDB()
{
	close_file();
	delete mdb;
}

int FileDB::check_sig(Asset *asset)
{
	return !strncmp(asset->path,"/db:",4);
}

int FileDB::open_file(int rd, int wr)
{
	int result = 0;
	if( mdb->openDb() ) return 1;
	mdb->detachDb();

	if(rd) {
		char *cp = 0;
		clip_id = strtol(asset->path+4,&cp,0);
		if( !cp || cp == asset->path+4 || *cp != 0 ) result = 1;
		if( !result ) result = mdb->clip_id(clip_id);
		if( !result ) {
			framerate = mdb->clip_framerate();
			clip_size = mdb->clip_size();
			prefix_size = mdb->clip_prefix_size();
			suffix_offset = mdb->clip_frames() - clip_size;
			asset->audio_data = 0;
			asset->video_data = 1;
			asset->actual_width = swidth;
			asset->actual_height = sheight;
			if( !asset->layers ) asset->layers = 1;
			if( !asset->width ) asset->width = asset->actual_width;
			if( !asset->height ) asset->height = asset->actual_height;
			if( !asset->video_length ) asset->video_length = clip_size;
			if( !asset->frame_rate ) asset->frame_rate = framerate;
		}
	}

	if(!result && wr ) {
		result = 1;
	}

	if( result ) {
		mdb->closeDb();
	}
	return result;
}

int FileDB::close_file()
{
	reset_parameters();
	FileBase::close_file();
	mdb->closeDb();
	delete vframe;
	vframe = 0;
	return 0;
}

int FileDB::get_best_colormodel(Asset *asset, int driver)
{
	return BC_YUV420P;
}

int FileDB::colormodel_supported(int colormodel)
{
	return colormodel;
}


int FileDB::set_video_position(int64_t pos)
{
	if( pos < 0 || pos >= asset->video_length )
		return 1;
	seq_no = pos;
	return 0;
}

int64_t FileDB::get_memory_usage()
{
	return 0;
}



int FileDB::write_frames(VFrame ***frames, int len)
{
	int result = 0;
	return result;
}


int FileDB::read_frame(VFrame *frame)
{
	int sw, sh;
	mdb->attachDb();
	int result = seq_no < clip_size ? 0 : 1;
	if( !result ) {
		int n = seq_no++;
		if( !n ) { frame_id = -1; }
		else if( n >= prefix_size ) n += suffix_offset;
		result = mdb->get_sequences(clip_id, n);
		if( !result && mdb->timeline_sequence_no() == n )
			frame_id = mdb->timeline_frame_id();
	}
	VFrame *fp = frame->get_w() == swidth && frame->get_h() == sheight &&
			frame->get_color_model() == BC_YUV420P ? frame :
		!vframe ? (vframe = new VFrame(swidth,sheight,BC_YUV420P,0)) :
		vframe;
	if( !result ) {
		if( frame_id < 0 )
			memset(fp->get_y(), 0, swidth*sheight);
		else
			result = mdb->get_image(frame_id, fp->get_y(), sw,sh);
	}
//printf("seq_no=%d, result=%d\n",seq_no,result);
	mdb->detachDb();
	if( !result ) {
		memset(fp->get_u(),0x80,swidth/2 * sheight/2);
		memset(fp->get_v(),0x80,swidth/2 * sheight/2);
	}
	if( !result && fp == vframe ) {
		BC_CModels::transfer(frame->get_rows(), fp->get_rows(),
			frame->get_y(), frame->get_u(), frame->get_v(),
			fp->get_y(), fp->get_u(), fp->get_v(),
			0, 0, fp->get_w(), fp->get_h(),
			0, 0, frame->get_w(), frame->get_h(),
			fp->get_color_model(), frame->get_color_model(), 0,
			fp->get_bytes_per_line(), swidth);
	}
	return result;
}


