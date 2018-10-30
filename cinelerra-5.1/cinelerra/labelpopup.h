/*
 * CINELERRA
 * Copyright (C) 2006 Pierre Dumuid
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

#ifndef LABELPOPUP_H
#define LABELPOPUP_H

#include "assetedit.inc"
#include "assets.inc"
#include "awindowgui.inc"
#include "clippopup.inc"
#include "edl.inc"
#include "guicast.h"
#include "labelpopup.inc"
#include "mwindow.inc"


class LabelPopup : public BC_PopupMenu
{
public:
	LabelPopup(MWindow *mwindow, AWindowGUI *gui);
	~LabelPopup();

	void create_objects();
// Set mainsession with the current selections
	int update();

	MWindow *mwindow;
	AWindowGUI *gui;

	LabelPopupEdit *editlabel;
};

class LabelPopupEdit : public BC_MenuItem
{
public:
	LabelPopupEdit(MWindow *mwindow, AWindowGUI *gui);
	~LabelPopupEdit();

	int handle_event();

	MWindow *mwindow;
	AWindowGUI *gui;
};

class LabelPopupDelete : public BC_MenuItem
{
public:
	LabelPopupDelete(MWindow *mwindow, AWindowGUI *gui);
	~LabelPopupDelete();

	int handle_event();

	MWindow *mwindow;
	AWindowGUI *gui;
};

class LabelPopupGoTo : public BC_MenuItem
{
public:
	LabelPopupGoTo(MWindow *mwindow, AWindowGUI *gui);
	~LabelPopupGoTo();

	int handle_event();

	MWindow *mwindow;
	AWindowGUI *gui;
};

class LabelListMenu : public BC_PopupMenu
{
public:
	LabelListMenu(MWindow *mwindow, AWindowGUI *gui);
	~LabelListMenu();

	void create_objects();
	void update();

	MWindow *mwindow;
	AWindowGUI *gui;
	AWindowListFormat *format;
};

#endif
