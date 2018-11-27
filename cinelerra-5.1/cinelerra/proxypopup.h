
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

#ifndef PROXYPOPUP_H
#define PROXYPOPUP_H

#include "assetedit.inc"
#include "awindowgui.inc"
#include "proxypopup.inc"
#include "edl.inc"
#include "guicast.h"
#include "assets.inc"
#include "mwindow.inc"



class ProxyPopup : public BC_PopupMenu
{
public:
	ProxyPopup(MWindow *mwindow, AWindowGUI *gui);
	~ProxyPopup();

	void create_objects();
// Set mainsession with the current selections
	int update();
	void paste_assets();

	MWindow *mwindow;
	AWindowGUI *gui;

	ProxyPopupInfo *info;
	ProxyPopupView *view;
	ProxyPopupViewWindow *view_window;
	AWindowListFormat *format;
};

class ProxyPopupInfo : public BC_MenuItem
{
public:
	ProxyPopupInfo(MWindow *mwindow, ProxyPopup *popup);
	~ProxyPopupInfo();

	int handle_event();
	int button_press_event();

	MWindow *mwindow;
	ProxyPopup *popup;
};

class ProxyPopupSort : public BC_MenuItem
{
public:
	ProxyPopupSort(MWindow *mwindow, ProxyPopup *popup);
	~ProxyPopupSort();

	int handle_event();

	MWindow *mwindow;
	ProxyPopup *popup;
};

class ProxyPopupView : public BC_MenuItem
{
public:
	ProxyPopupView(MWindow *mwindow, ProxyPopup *popup);
	~ProxyPopupView();

	int handle_event();

	MWindow *mwindow;
	ProxyPopup *popup;
};


class ProxyPopupViewWindow : public BC_MenuItem
{
public:
	ProxyPopupViewWindow(MWindow *mwindow, ProxyPopup *popup);
	~ProxyPopupViewWindow();

	int handle_event();

	MWindow *mwindow;
	ProxyPopup *popup;
};

class ProxyPopupCopy : public BC_MenuItem
{
public:
	ProxyPopupCopy(MWindow *mwindow, ProxyPopup *popup);
	~ProxyPopupCopy();

	int handle_event();

	MWindow *mwindow;
	ProxyPopup *popup;
};

class ProxyPopupPaste : public BC_MenuItem
{
public:
	ProxyPopupPaste(MWindow *mwindow, ProxyPopup *popup);
	~ProxyPopupPaste();

	int handle_event();

	MWindow *mwindow;
	ProxyPopup *popup;
};

class ProxyPopupProjectRemove : public BC_MenuItem
{
public:
	ProxyPopupProjectRemove(MWindow *mwindow, ProxyPopup *popup);
	~ProxyPopupProjectRemove();

	int handle_event();

	MWindow *mwindow;
	ProxyPopup *popup;
};

class ProxyPopupDiskRemove : public BC_MenuItem
{
public:
	ProxyPopupDiskRemove(MWindow *mwindow, ProxyPopup *popup);
	~ProxyPopupDiskRemove();

	int handle_event();

	MWindow *mwindow;
	ProxyPopup *popup;
};

class ProxyListMenu : public BC_PopupMenu
{
public:
	ProxyListMenu(MWindow *mwindow, AWindowGUI *gui);
	~ProxyListMenu();

	void create_objects();
	void update();
	AWindowListFormat *format;
	AssetSelectUsed *select_used;
	MWindow *mwindow;
	AWindowGUI *gui;
};

#endif
