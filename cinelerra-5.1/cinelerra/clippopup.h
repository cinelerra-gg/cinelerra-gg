
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

#ifndef CLIPPOPUP_H
#define CLIPPOPUP_H

#include "assetedit.inc"
#include "awindowgui.inc"
#include "clippopup.inc"
#include "edl.inc"
#include "guicast.h"
#include "assets.inc"
#include "mwindow.inc"



class ClipPopup : public BC_PopupMenu
{
public:
	ClipPopup(MWindow *mwindow, AWindowGUI *gui);
	~ClipPopup();

	void create_objects();
// Set mainsession with the current selections
	int update();
	void paste_assets();
	void match_size();
	void match_rate();
	void match_all();

	MWindow *mwindow;
	AWindowGUI *gui;


	ClipPopupInfo *info;
	ClipPopupView *view;
	ClipPopupViewWindow *view_window;
	AWindowListFormat *format;
};

class ClipPopupInfo : public BC_MenuItem
{
public:
	ClipPopupInfo(MWindow *mwindow, ClipPopup *popup);
	~ClipPopupInfo();

	int handle_event();
	int button_press_event();

	MWindow *mwindow;
	ClipPopup *popup;
};

class ClipPopupSort : public BC_MenuItem
{
public:
	ClipPopupSort(MWindow *mwindow, ClipPopup *popup);
	~ClipPopupSort();

	int handle_event();

	MWindow *mwindow;
	ClipPopup *popup;
};

class ClipPopupView : public BC_MenuItem
{
public:
	ClipPopupView(MWindow *mwindow, ClipPopup *popup);
	~ClipPopupView();

	int handle_event();

	MWindow *mwindow;
	ClipPopup *popup;
};


class ClipPopupViewWindow : public BC_MenuItem
{
public:
	ClipPopupViewWindow(MWindow *mwindow, ClipPopup *popup);
	~ClipPopupViewWindow();

	int handle_event();

	MWindow *mwindow;
	ClipPopup *popup;
};

class ClipPopupCopy : public BC_MenuItem
{
public:
	ClipPopupCopy(MWindow *mwindow, ClipPopup *popup);
	~ClipPopupCopy();

	int handle_event();

	MWindow *mwindow;
	ClipPopup *popup;
};

class ClipPopupPaste : public BC_MenuItem
{
public:
	ClipPopupPaste(MWindow *mwindow, ClipPopup *popup);
	~ClipPopupPaste();

	int handle_event();

	MWindow *mwindow;
	ClipPopup *popup;
};

class ClipMatchSize : public BC_MenuItem
{
public:
	ClipMatchSize(MWindow *mwindow, ClipPopup *popup);

	int handle_event();

	MWindow *mwindow;
	ClipPopup *popup;
};

class ClipMatchRate : public BC_MenuItem
{
public:
	ClipMatchRate(MWindow *mwindow, ClipPopup *popup);

	int handle_event();

	MWindow *mwindow;
	ClipPopup *popup;
};

class ClipMatchAll : public BC_MenuItem
{
public:
	ClipMatchAll(MWindow *mwindow, ClipPopup *popup);

	int handle_event();

	MWindow *mwindow;
	ClipPopup *popup;
};

class ClipPopupDelete : public BC_MenuItem
{
public:
	ClipPopupDelete(MWindow *mwindow, ClipPopup *popup);
	~ClipPopupDelete();

	int handle_event();

	MWindow *mwindow;
	ClipPopup *popup;
};


class ClipPasteToFolder : public BC_MenuItem
{
public:
        ClipPasteToFolder(MWindow *mwindow);

        int handle_event();
        MWindow *mwindow;
};

class ClipListFormat : public BC_MenuItem
{
public:
	ClipListFormat(MWindow *mwindow, ClipListMenu *menu);

	int handle_event();
	MWindow *mwindow;
	ClipListMenu *menu;
};

class ClipListMenu : public BC_PopupMenu
{
public:
	ClipListMenu(MWindow *mwindow, AWindowGUI *gui);
	~ClipListMenu();

	void create_objects();
	void update();
	AWindowListFormat *format;
	MWindow *mwindow;
	AWindowGUI *gui;
};

class ClipPopupNest : public BC_MenuItem
{
public:
	ClipPopupNest(MWindow *mwindow, ClipPopup *popup);
	~ClipPopupNest();

	int handle_event();

	MWindow *mwindow;
	ClipPopup *popup;
};

class ClipPopupUnNest : public BC_MenuItem
{
public:
	ClipPopupUnNest(MWindow *mwindow, ClipPopup *popup);
	~ClipPopupUnNest();

	int handle_event();

	MWindow *mwindow;
	ClipPopup *popup;
};

#endif
