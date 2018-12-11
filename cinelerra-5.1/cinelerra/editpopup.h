
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
#include "plugindialog.inc"
#include "resizetrackthread.inc"

class EditPopup : public BC_PopupMenu
{
public:
	EditPopup(MWindow *mwindow, MWindowGUI *gui);
	~EditPopup();

	void create_objects();
	int update(Edit *edit);

	MWindow *mwindow;
	MWindowGUI *gui;
	Edit *edit;
};


class EditPopupClear : public BC_MenuItem
{
public:
	EditPopupClear(MWindow *mwindow, EditPopup *popup);
	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupDelete : public BC_MenuItem
{
public:
	EditPopupDelete(MWindow *mwindow, EditPopup *popup);
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

class EditPopupCut : public BC_MenuItem
{
public:
	EditPopupCut(MWindow *mwindow, EditPopup *popup);
	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupCopyCut : public BC_MenuItem
{
public:
	EditPopupCopyCut(MWindow *mwindow, EditPopup *popup);
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

class EditPopupFindAsset : public BC_MenuItem
{
public:
	EditPopupFindAsset(MWindow *mwindow, EditPopup *popup);
	int handle_event();
	MWindow *mwindow;
	EditPopup *popup;
};

class EditPopupTitle : public BC_MenuItem
{
public:
	EditPopupTitle(MWindow *mwindow, EditPopup *popup);
	~EditPopupTitle();

	int handle_event();

	MWindow *mwindow;
	EditPopup *popup;
	EditTitleDialogThread *dialog_thread;
};

class EditTitleDialogThread : public BC_DialogThread
{
public:
	EditTitleDialogThread(EditPopupTitle *edit_title);
	~EditTitleDialogThread();
	BC_Window* new_gui();
	void start(int wx, int wy);
	void handle_close_event(int result);
	void handle_done_event(int result);

	int wx, wy;
	EditPopupTitle *edit_title;
	EditPopupTitleWindow *window;
};

class EditPopupTitleText : public BC_TextBox
{
public:
	EditPopupTitleText(EditPopupTitleWindow *window,
		MWindow *mwindow, int x, int y, const char *text);
	~EditPopupTitleText();
	int handle_event();

	MWindow *mwindow;
	EditPopupTitleWindow *window;
};

class EditPopupTitleWindow : public BC_Window
{
public:
	EditPopupTitleWindow(MWindow *mwindow, EditPopup *popup, int wx, int wy);
	~EditPopupTitleWindow();

	void create_objects();
	void handle_close_event(int result);

	EditPopupTitleText *title_text;
	MWindow *mwindow;
	EditPopup *popup;
	char new_text[BCTEXTLEN];
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
