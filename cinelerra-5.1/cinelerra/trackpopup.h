
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

#ifndef __TRACKPOPUP_H__
#define __TRACKPOPUP_H__

#include "guicast.h"
#include "colorpicker.h"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "plugindialog.inc"
#include "resizetrackthread.inc"
#include "track.inc"
#include "trackpopup.inc"


class TrackPopup : public BC_PopupMenu
{
public:
	TrackPopup(MWindow *mwindow, MWindowGUI *gui);
	~TrackPopup();

	void create_objects();
	int activate_menu(Track *track, Edit *edit,
		PluginSet *pluginset, Plugin *plugin, double position);

	MWindow *mwindow;
	MWindowGUI *gui;
	TrackPopupResize *resize_option;
	TrackPopupMatchSize *matchsize_option;

	Track *track;
	Edit *edit;
	Plugin *plugin;
	PluginSet *pluginset;
	double position;
};

class TrackPopupMatchSize : public BC_MenuItem
{
public:
	TrackPopupMatchSize(MWindow *mwindow, TrackPopup *popup);
	~TrackPopupMatchSize();
	int handle_event();
	MWindow *mwindow;
	TrackPopup *popup;
};

class TrackPopupResize : public BC_MenuItem
{
public:
	TrackPopupResize(MWindow *mwindow, TrackPopup *popup);
	~TrackPopupResize();
	int handle_event();
	MWindow *mwindow;
	TrackPopup *popup;
	ResizeTrackThread *dialog_thread;
};

class TrackPopupDeleteTrack : public BC_MenuItem
{
public:
	TrackPopupDeleteTrack(MWindow *mwindow, TrackPopup *popup);
	int handle_event();
	MWindow *mwindow;
	TrackPopup *popup;
};

class TrackPopupAddTrack : public BC_MenuItem
{
public:
	TrackPopupAddTrack(MWindow *mwindow, TrackPopup *popup);
	int handle_event();
	MWindow *mwindow;
	TrackPopup *popup;
};

class TrackAttachEffect : public BC_MenuItem
{
public:
	TrackAttachEffect(MWindow *mwindow, TrackPopup *popup);
	~TrackAttachEffect();

	int handle_event();

	MWindow *mwindow;
	TrackPopup *popup;
	PluginDialogThread *dialog_thread;
};

class TrackMoveUp : public BC_MenuItem
{
public:
	TrackMoveUp(MWindow *mwindow, TrackPopup *popup);
	~TrackMoveUp();

	int handle_event();

	MWindow *mwindow;
	TrackPopup *popup;
};

class TrackMoveDown : public BC_MenuItem
{
public:
	TrackMoveDown(MWindow *mwindow, TrackPopup *popup);
	~TrackMoveDown();

	int handle_event();

	MWindow *mwindow;
	TrackPopup *popup;
};

class TrackPopupFindAsset : public BC_MenuItem
{
public:
	TrackPopupFindAsset(MWindow *mwindow, TrackPopup *popup);
	int handle_event();
	MWindow *mwindow;
	TrackPopup *popup;
};

class TrackPopupUserTitle : public BC_MenuItem
{
public:
	TrackPopupUserTitle(MWindow *mwindow, TrackPopup *popup);
	~TrackPopupUserTitle();

	int handle_event();

	MWindow *mwindow;
	TrackPopup *popup;
	TrackUserTitleDialogThread *dialog_thread;
};

class TrackPopupUserTitleText : public BC_TextBox
{
public:
	TrackPopupUserTitleText(TrackPopupUserTitleWindow *window,
		MWindow *mwindow, int x, int y, const char *text);
	~TrackPopupUserTitleText();
	int handle_event();

	MWindow *mwindow;
	TrackPopupUserTitleWindow *window;
};

class TrackPopupUserTitleWindow : public BC_Window
{
public:
	TrackPopupUserTitleWindow(MWindow *mwindow, TrackPopup *popup, int wx, int wy);
	~TrackPopupUserTitleWindow();

	void create_objects();
	void handle_close_event(int result);

	TrackPopupUserTitleText *title_text;
	MWindow *mwindow;
	TrackPopup *popup;
	char new_text[BCTEXTLEN];
};

class TrackUserTitleDialogThread : public BC_DialogThread
{
public:
	TrackUserTitleDialogThread(TrackPopupUserTitle *edit_title);
	~TrackUserTitleDialogThread();

	void handle_close_event(int result);
	void handle_done_event(int result);
	BC_Window* new_gui();
	void start(int wx, int wy);

	int wx, wy;
	TrackPopupUserTitle *edit_title;
	TrackPopupUserTitleWindow *window;
};


class TrackPopupTitleColor : public BC_MenuItem
{
public:
	TrackPopupTitleColor(MWindow *mwindow, TrackPopup *popup);
	~TrackPopupTitleColor();

	int handle_event();

	MWindow *mwindow;
	TrackPopup *popup;
	TrackTitleColorPicker *color_picker;
};

class TrackTitleColorDefault : public BC_GenericButton
{
public:
	TrackTitleColorDefault(TrackTitleColorPicker *color_picker, int x, int y);
	int handle_event();

	TrackTitleColorPicker *color_picker;
};

class TrackTitleColorPicker : public ColorPicker
{
public:
	TrackTitleColorPicker(TrackPopup *popup, int color);
	~TrackTitleColorPicker();
	void create_objects(ColorWindow *gui);
	int handle_new_color(int color, int alpha);
	void handle_done_event(int result);

	TrackPopup *popup;
	int color;
};


class TrackPopupShow : public BC_MenuItem
{
public:
	TrackPopupShow(MWindow *mwindow, TrackPopup *popup);
	~TrackPopupShow();

	int handle_event();

	MWindow *mwindow;
	TrackPopup *popup;
	TrackShowDialogThread *dialog_thread;
};

class TrackShowDialogThread : public BC_DialogThread
{
public:
	TrackShowDialogThread(TrackPopupShow *edit_show);
	~TrackShowDialogThread();
	BC_Window* new_gui();
	void start(int wx, int wy);
	void handle_close_event(int result);

	int wx, wy;
	TrackPopupShow *edit_show;
	TrackPopupShowWindow *window;
};

class TrackPopupShowText : public BC_TextBox
{
public:
	TrackPopupShowText(TrackPopupShowWindow *window,
		MWindow *mwindow, int x, int y, const char *text);
	~TrackPopupShowText();

	TrackPopupShowWindow *window;
	MWindow *mwindow;
};

class TrackPopupShowWindow : public BC_Window
{
public:
	TrackPopupShowWindow(MWindow *mwindow, TrackPopup *popup, int wx, int wy);
	~TrackPopupShowWindow();

	void create_objects();

	TrackPopupShowText *show_text;
	MWindow *mwindow;
	TrackPopup *popup;
};

#endif
