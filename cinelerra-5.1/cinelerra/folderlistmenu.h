
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

#ifndef FOLDERLISTMENU_H
#define FOLDERLISTMENU_H

#include "awindowgui.inc"
#include "edl.inc"
#include "guicast.h"
#include "mwindow.inc"

class FolderListFormat : public BC_MenuItem
{
public:
	FolderListFormat(MWindow *mwindow, FolderListMenu *menu);

	int handle_event();
	MWindow *mwindow;
	FolderListMenu *menu;
};

class FolderListSort : public BC_MenuItem
{
public:
	FolderListSort(MWindow *mwindow, FolderListMenu *menu);

	int handle_event();
	MWindow *mwindow;
	FolderListMenu *menu;
};

class FolderListNew : public BC_MenuItem
{
public:
	FolderListNew(MWindow *mwindow, FolderListMenu *menu,
		const char *text, int is_clips);

	int handle_event();
	MWindow *mwindow;
	FolderListMenu *menu;
	int is_clips;
};

class FolderListModify : public BC_MenuItem
{
public:
	FolderListModify(MWindow *mwindow, FolderListMenu *menu);

	int handle_event();
	MWindow *mwindow;
	FolderListMenu *menu;
};

class FolderListDelete : public BC_MenuItem
{
public:
	FolderListDelete(MWindow *mwindow, FolderListMenu *menu);

	int handle_event();
	MWindow *mwindow;
	FolderListMenu *menu;
};

class FolderListMenu : public BC_PopupMenu
{
public:
	FolderListMenu(MWindow *mwindow, AWindowGUI *gui);
	~FolderListMenu();

	void create_objects();
	void update_titles();

	FolderListFormat *format;

	MWindow *mwindow;
	AWindowGUI *gui;
};

#endif
