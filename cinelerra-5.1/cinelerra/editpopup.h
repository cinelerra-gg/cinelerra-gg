
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

#ifndef EDITPOPUP_H
#define EDITPOPUP_H

#include "guicast.h"
#include "edit.inc"
#include "editpopup.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "plugin.inc"
#include "pluginset.inc"
#include "plugindialog.inc"
#include "resizetrackthread.inc"
#include "track.inc"

class EditPopup : public BC_PopupMenu
{
public:
	EditPopup(MWindow *mwindow, MWindowGUI *gui);
	~EditPopup();

	void create_objects();
	int activate_menu(Track *track, Edit *edit,
		PluginSet *pluginset, Plugin *plugin, double position);

	MWindow *mwindow;
	MWindowGUI *gui;
	Track *track;
	Edit *edit;
	Plugin *plugin;
	PluginSet *pluginset;
	double position;
};

class EditPopupClearSelect : public BC_MenuItem
{
public:
	EditPopupClearSelect(MWindow *mwindow, EditPopup *popup);
	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupCopy : public BC_MenuItem
{
public:
	EditPopupCopy(MWindow *mwindow, EditPopup *popup);
	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupCopyPack : public BC_MenuItem
{
public:
	EditPopupCopyPack(MWindow *mwindow, EditPopup *popup);
	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupCut : public BC_MenuItem
{
public:
	EditPopupCut(MWindow *mwindow, EditPopup *popup);
	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupCutPack : public BC_MenuItem
{
public:
	EditPopupCutPack(MWindow *mwindow, EditPopup *popup);
	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupMute : public BC_MenuItem
{
public:
	EditPopupMute(MWindow *mwindow, EditPopup *popup);
	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupMutePack : public BC_MenuItem
{
public:
	EditPopupMutePack(MWindow *mwindow, EditPopup *popup);
	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupPaste : public BC_MenuItem
{
public:
	EditPopupPaste(MWindow *mwindow, EditPopup *popup);
	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupOverwrite : public BC_MenuItem
{
public:
	EditPopupOverwrite(MWindow *mwindow, EditPopup *popup);
	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupOverwritePlugins : public BC_MenuItem
{
public:
	EditPopupOverwritePlugins(MWindow *mwindow, EditPopup *popup);
	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

#endif
