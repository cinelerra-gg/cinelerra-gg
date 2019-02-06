
/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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

#ifndef __ZWINDOW_H__
#define __ZWINDOW_H__

#include "arraylist.h"
#include "bcdialog.h"
#include "bcwindow.h"
#include "edl.inc"
#include "filexml.inc"
#include "mwindow.inc"
#include "playbackengine.inc"
#include "renderengine.inc"
#include "zwindowgui.inc"

class Mixer {
public:
	int idx;
	int x, y, w, h;
	ArrayList<int> mixer_ids;
	char title[BCSTRLEN];

	Mixer(int idx);
	void save(FileXML *file);
	int load(FileXML *file);
	void copy_from(Mixer &that);
	void set_title(const char *tp);
	void reposition(int x, int y, int w, int h);
};

class Mixers : public ArrayList<Mixer *>
{
public:
	Mixers();
	~Mixers();
	Mixer *new_mixer();
	Mixer *get_mixer(int idx);
	void del_mixer(int idx);
	void save(FileXML *file);
	int load(FileXML *file);
	void copy_from(Mixers &that);
};


class ZWindow : public BC_DialogThread
{
public:
	ZWindow(MWindow *mwindow);
	~ZWindow();

	void handle_done_event(int result);
	void handle_close_event(int result);
	void change_source(EDL *edl);
	void stop_playback(int wait);
	void handle_mixer(int command, int wait_tracking,
		int use_inout, int toggle_audio, int loop_play, float speed);
	void update_mixer_ids();
	void set_title(const char *tp);
	void reposition(int x, int y, int w, int h);

	BC_Window *new_gui();
	MWindow *mwindow;
	ZWindowGUI *zgui;
	EDL* edl;

	int idx, destroy;
	int highlighted;
	char title[BCTEXTLEN];
};

#endif
