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

#ifndef LABELEDIT_H
#define LABELEDIT_H

#include "assetedit.inc"
#include "assets.inc"
#include "awindow.inc"
#include "awindowgui.inc"
#include "guicast.h"
#include "labeledit.inc"
#include "mwindow.inc"
#include "vwindow.inc"


class LabelEdit : public BC_DialogThread
{
public:
	LabelEdit(MWindow *mwindow, AWindow *awindow, VWindow *vwindow);
	~LabelEdit();

	BC_Window *new_gui();
	void start(Label *label, int x, int y);
	void handle_close_event(int result);
	void handle_done_event(int result);

// If it is being created or edited
	MWindow *mwindow;
	AWindow *awindow;
	VWindow *vwindow;

	Label *label;
	int x, y;
	LabelEditWindow *label_edit_window;
};

class LabelEditWindow : public BC_Window
{
public:
	LabelEditWindow(MWindow *mwindow, LabelEdit *thread);
	~LabelEditWindow();

	void create_objects();

	Label *label;
	MWindow *mwindow;
	LabelEdit *thread;
	BC_TextBox *textbox;
};

class LabelEditTitle : public BC_TextBox
{
public:
	LabelEditTitle(LabelEditWindow *window, int x, int y, int w);
	int handle_event();
	LabelEditWindow *window;
};

class LabelEditComments : public BC_TextBox
{
public:
	LabelEditComments(LabelEditWindow *window, int x, int y, int w, int rows);
	int handle_event();
	LabelEditWindow *window;
};

#endif
