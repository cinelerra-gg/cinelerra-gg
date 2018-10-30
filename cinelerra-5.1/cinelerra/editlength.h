
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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

#ifndef EDITLENGTH_H
#define EDITLENGTH_H

#include "bcdialog.h"
#include "edit.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "editlength.inc"

class EditLengthText;

class EditLengthThread : public BC_DialogThread
{
public:
	EditLengthThread(MWindow *mwindow);
	~EditLengthThread();

	void start(Edit *edit);
	BC_Window* new_gui();
	void handle_close_event(int result);

	MWindow *mwindow;

	Edit *edit;
	double length;
	double orig_length;
};


class EditLengthDialog : public BC_Window
{
public:
	EditLengthDialog(MWindow *mwindow,
		EditLengthThread *thread,
		int x,
		int y);
	~EditLengthDialog();

	void create_objects();
	int close_event();

	MWindow *mwindow;
	EditLengthThread *thread;
	EditLengthText *text;
};

class EditLengthText : public BC_TumbleTextBox
{
public:
	EditLengthText(MWindow *mwindow,
		EditLengthDialog *gui,
		int x,
		int y);
	int handle_event();
	MWindow *mwindow;
	EditLengthDialog *gui;
};


#endif
