
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
#include "colorpicker.h"
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

class EditPopupFindAsset : public BC_MenuItem
{
public:
	EditPopupFindAsset(MWindow *mwindow, EditPopup *popup);
	int handle_event();
	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupUserTitle : public BC_MenuItem
{
public:
	EditPopupUserTitle(MWindow *mwindow, EditPopup *popup);
	~EditPopupUserTitle();

	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
	EditUserTitleDialogThread *dialog_thread;
};

class EditPopupUserTitleText : public BC_TextBox
{
public:
	EditPopupUserTitleText(EditPopupUserTitleWindow *window,
		MWindow *mwindow, int x, int y, const char *text);
	~EditPopupUserTitleText();
	int handle_event();

	MWindow *mwindow;
	EditPopupUserTitleWindow *window;
};

class EditPopupUserTitleWindow : public BC_Window
{
public:
	EditPopupUserTitleWindow(MWindow *mwindow, EditPopup *popup, int wx, int wy);
	~EditPopupUserTitleWindow();

	void create_objects();
	void handle_close_event(int result);

	EditPopupUserTitleText *title_text;
	MWindow *mwindow;
	EditPopup *popup;
	char new_text[BCTEXTLEN];
};

class EditUserTitleDialogThread : public BC_DialogThread
{
public:
	EditUserTitleDialogThread(EditPopupUserTitle *edit_title);
	~EditUserTitleDialogThread();

	void handle_close_event(int result);
	void handle_done_event(int result);
	BC_Window* new_gui();
	void start(int wx, int wy);

	int wx, wy;
	EditPopupUserTitle *edit_title;
	EditPopupUserTitleWindow *window;
};


class EditPopupTitleColor : public BC_MenuItem
{
public:
	EditPopupTitleColor(MWindow *mwindow, EditPopup *popup);
	~EditPopupTitleColor();

	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
	EditTitleColorPicker *color_picker;
};

class EditTitleColorDefault : public BC_GenericButton
{
public:
	EditTitleColorDefault(EditTitleColorPicker *color_picker, int x, int y);
	int handle_event();

	EditTitleColorPicker *color_picker;
};

class EditTitleColorPicker : public ColorPicker
{
public:
	EditTitleColorPicker(EditPopup *popup, int color);
	~EditTitleColorPicker();
	void create_objects(ColorWindow *gui);
	int handle_new_color(int color, int alpha);
	void handle_done_event(int result);

	EditPopup *popup;
	int color;
};


class EditPopupShow : public BC_MenuItem
{
public:
	EditPopupShow(MWindow *mwindow, EditPopup *popup);
	~EditPopupShow();

	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
	EditShowDialogThread *dialog_thread;
};

class EditShowDialogThread : public BC_DialogThread
{
public:
	EditShowDialogThread(EditPopupShow *edit_show);
	~EditShowDialogThread();
	BC_Window* new_gui();
	void start(int wx, int wy);
	void handle_close_event(int result);

	int wx, wy;
	EditPopupShow *edit_show;
	EditPopupShowWindow *window;
};

class EditPopupShowText : public BC_TextBox
{
public:
	EditPopupShowText(EditPopupShowWindow *window,
		MWindow *mwindow, int x, int y, const char *text);
	~EditPopupShowText();

	EditPopupShowWindow *window;
	MWindow *mwindow;
};

class EditPopupShowWindow : public BC_Window
{
public:
	EditPopupShowWindow(MWindow *mwindow, EditPopup *popup, int wx, int wy);
	~EditPopupShowWindow();

	void create_objects();

	EditPopupShowText *show_text;
	MWindow *mwindow;
	EditPopup *popup;
};

#endif
