
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

#ifndef FORMATPOPUP_H
#define FORMATPOPUP_H



#include "guicast.h"
#include "formatpopup.inc"

class FormatPopup : public BC_ListBox
{
public:
	FormatPopup(int x, int y,
		int do_audio=1, int do_video=1,
		int use_brender=0);  // Show formats useful in background rendering
	~FormatPopup();

	void create_objects();
	void post_item(int format);
	virtual int handle_event();  // user copies text to value here
	ArrayList<BC_ListBoxItem*> format_items;
	int use_brender, do_audio, do_video;
};


class FFMPEGPopup : public BC_ListBox
{
public:
	FFMPEGPopup(int x, int y);
	~FFMPEGPopup();

	void create_objects();
	virtual int handle_event();
	ArrayList<BC_ListBoxItem*> ffmpeg_types;
};






#endif
