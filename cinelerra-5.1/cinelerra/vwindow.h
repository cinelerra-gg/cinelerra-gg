
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

#ifndef VWINDOW_H
#define VWINDOW_H

#include "asset.inc"
#include "bcdialog.h"
#include "clipedit.inc"
#include "edl.inc"
#include "indexable.inc"
#include "mwindow.inc"
#include "thread.h"
#include "transportque.inc"
#include "vplayback.inc"
#include "vtracking.inc"
#include "vwindowgui.inc"

class VWindow : public BC_DialogThread
{
public:
	VWindow(MWindow *mwindow);
	~VWindow();

	void handle_done_event(int result);
	void handle_close_event(int result);
	BC_Window* new_gui();

	void load_defaults();
	void create_objects();
// Change source to asset, creating a new EDL
	void change_source(Indexable *indexable);
// Change source to EDL
	void change_source(EDL *edl);
// Change source to 1 of master EDL's vwindow EDLs after a load.
	void change_source(int number);
// Returns private EDL of VWindow
// If an asset is dropped in, a new VWindow EDL is created in the master EDL
// and this points to it.
// If a clip is dropped in, it points to the clip EDL.
	EDL* get_edl();
// Returns last argument of change_source or 0 if it was an EDL
//  Asset* get_asset();
	Indexable* get_source();
	void update(int do_timebar);

	void update_position(int change_type = CHANGE_NONE);
	int update_position(double position);
	void set_inpoint();
	void set_outpoint();
	void unset_inoutpoint();
	void copy(int all);
	void splice_selection();
	void overwrite_selection();
	void delete_source(int do_main_edl, int update_gui);
	void goto_start();
	void goto_end();
	void stop_playback(int wait);
	void interrupt_playback(int wait);

	VTracking *playback_cursor;

// Number of source in GUI list
	MWindow *mwindow;
	VWindow *vwindow;
	VWindowGUI *gui;
	VPlayback *playback_engine;
	ClipEdit *clip_edit;
// Pointer to object being played back in the main EDL.
	EDL *edl;

// Pointer to source for accounting
	Indexable *indexable;
};


#endif
