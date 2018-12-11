
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
	int update(Track *track);

	MWindow *mwindow;
	MWindowGUI *gui;
	Track *track;
	TrackPopupResize *resize_option;
	TrackPopupMatchSize *matchsize_option;
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

class TrackPopupFindAsset : public BC_MenuItem
{
public:
	TrackPopupFindAsset(MWindow *mwindow, TrackPopup *popup);
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


#endif
