
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

#ifndef MBUTTONS_H
#define MBUTTONS_H

class KeyFrameButton;
class ExpandX;
class ZoomX;
class ExpandY;
class ZoomY;
class ExpandTrack;
class ZoomTrack;
class ExpandVideo;
class MainEditing;
class ZoomVideo;
class LabelButton;
class Cut;
class Copy;
class Paste;
class MainFFMpegToggle;

#include "editpanel.h"
#include "guicast.h"
#include "labelnavigate.inc"
#include "mbuttons.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "playtransport.h"
#include "record.inc"




class MButtons : public BC_SubWindow
{
public:
	MButtons(MWindow *mwindow, MWindowGUI *gui);
	~MButtons();

	void create_objects();
	int resize_event();
	int keypress_event();
	void update();

	MWindowGUI *gui;
	MWindow *mwindow;
	PlayTransport *transport;
	MainEditing *edit_panel;
	MainFFMpegToggle *ffmpeg_toggle;
};

class MainTransport : public PlayTransport
{
public:
	MainTransport(MWindow *mwindow, MButtons *mbuttons, int x, int y);
	void goto_start();
	void goto_end();
};

class MainEditing : public EditPanel
{
public:
	MainEditing(MWindow *mwindow, MButtons *mbuttons, int x, int y);
	virtual ~MainEditing() {}

	double get_position();
	void set_position(double position);
	void set_click_to_play(int v);

	void panel_stop_transport();
	void panel_toggle_label();
	void panel_next_label(int cut);
	void panel_prev_label(int cut);
	void panel_prev_edit(int cut);
	void panel_next_edit(int cut);
	void panel_copy_selection();
	void panel_overwrite_selection();
	void panel_splice_selection();
	void panel_set_inpoint();
	void panel_set_outpoint();
	void panel_unset_inoutpoint();
	void panel_to_clip();
	void panel_cut();
	void panel_paste();
	void panel_fit_selection();
	void panel_fit_autos(int all);
	void panel_set_editing_mode(int mode);
	void panel_set_auto_keyframes(int v);
	void panel_set_labels_follow_edits(int v);

	MWindow *mwindow;
	MButtons *mbuttons;
};

#endif
