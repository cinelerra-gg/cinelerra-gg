
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

#include "bcsignals.h"
#include "file.h"
#include "filesystem.h"
#include "ffmpeg.h"
#include "formatpopup.h"
#include "language.h"


FormatPopup::FormatPopup(int x, int y, int do_audio, int do_video, int use_brender)
 : BC_ListBox(x, y, 200, 200, LISTBOX_TEXT, 0, 0, 0, 1, 0, 1)
{
	this->do_audio = do_audio;
	this->do_video = do_video;
	this->use_brender = use_brender;
	set_tooltip(_("Change file format"));
}

void FormatPopup::post_item(int format)
{
	if( (do_audio && File::renders_audio(format)) ||
	    (do_video && File::renders_video(format)) )
		format_items.append(new BC_ListBoxItem(File::formattostr(format)));
}

void FormatPopup::create_objects()
{
	if(!use_brender) {
		post_item(FILE_FFMPEG);
#ifdef HAVE_LIBZMPEG
		post_item(FILE_AC3);
#endif
		post_item(FILE_AIFF);
		post_item(FILE_AU);
		post_item(FILE_FLAC);
	}

	if(!use_brender)
		post_item(FILE_JPEG);
	post_item(FILE_JPEG_LIST);


	if(!use_brender) {
		post_item(FILE_GIF);
		post_item(FILE_GIF_LIST);
#ifdef HAVE_OPENEXR
		post_item(FILE_EXR);
		post_item(FILE_EXR_LIST);
#endif
		post_item(FILE_WAV);
		post_item(FILE_RAWDV);
#ifdef HAVE_LIBZMPEG
		post_item(FILE_AMPEG);
		post_item(FILE_VMPEG);
#endif
		post_item(FILE_PCM);
	}

	if(!use_brender)
		post_item(FILE_PNG);
	post_item(FILE_PNG_LIST);
	if(!use_brender)
		post_item(FILE_PPM);
	post_item(FILE_PPM_LIST);
	if(!use_brender)
		post_item(FILE_TGA);
	post_item(FILE_TGA_LIST);
	if(!use_brender)
		post_item(FILE_TIFF);
	post_item(FILE_TIFF_LIST);

	update(&format_items, 0, 0, 1);
}

FormatPopup::~FormatPopup()
{
	format_items.remove_all_objects();
}

int FormatPopup::handle_event()
{
	return 1;
}


FFMPEGPopup::FFMPEGPopup(int x, int y)
 : BC_ListBox(x, y, 100, 200, LISTBOX_TEXT, 0, 0, 0, 1, 0, 1)
{
	set_tooltip(_("Set ffmpeg file type"));
}

void FFMPEGPopup::create_objects()
{
	static const char *dirs[] = { "audio", "video", };
	for( int i=0; i<(int)(sizeof(dirs)/sizeof(dirs[0])); ++i ) {
		FileSystem fs;
		char option_path[BCTEXTLEN];
		FFMPEG::set_option_path(option_path, dirs[i]);
		fs.update(option_path);
		int total_files = fs.total_files();
		for( int j=0; j<total_files; ++j ) {
			const char *name = fs.get_entry(j)->get_name();
			const char *ext = strrchr(name,'.');
			if( !ext ) continue;
			if( !strcmp("dfl", ++ext) ) continue;
			if( !strcmp("opts", ext) ) continue;
			int k = ffmpeg_types.size();
			while( --k >= 0 && strcmp(ffmpeg_types[k]->get_text(), ext) );
			if( k >= 0 ) continue;
			ffmpeg_types.append(new BC_ListBoxItem(ext));
        	}
        }

	BC_ListBoxItem::sort_items(ffmpeg_types);
	update(&ffmpeg_types, 0, 0, 1);
}

FFMPEGPopup::~FFMPEGPopup()
{
	ffmpeg_types.remove_all_objects();
}

int FFMPEGPopup::handle_event()
{
	return 0;
}


