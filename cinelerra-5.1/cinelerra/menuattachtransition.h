
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

#ifndef MENUATTACHTRANSITION_H
#define MENUATTACHTRANSITION_H


#include "bcdialog.h"
#include "guicast.h"
#include "mwindow.inc"
#include "plugindialog.inc"

class TransitionDialogName;
class TransitionDialogThread;
class TransitionDialog;

class MenuAttachTransition : public BC_MenuItem
{
public:
	MenuAttachTransition(MWindow *mwindow, int data_type);
	~MenuAttachTransition();

	int handle_event();

	MWindow *mwindow;
	int data_type;
	TransitionDialogThread *thread;
};


class TransitionSetDefault : public BC_GenericButton
{
public:
	TransitionSetDefault(TransitionDialog *window, int x, int y);
	~TransitionSetDefault();
	int handle_event();

	TransitionDialog *window;
};

class TransitionDefaultText : public BC_TextBox
{
public:
	TransitionDefaultText(TransitionDialog *window,
		int x, int y, int w, const char *text);
	~TransitionDefaultText();

	TransitionDialog *window;
};


class TransitionDialogThread : public BC_DialogThread
{
public:
	TransitionDialogThread(MWindow *mwindow, int data_type);
	~TransitionDialogThread();
	BC_Window* new_gui();
	void handle_close_event(int result);
	void start();

	char transition_title[BCTEXTLEN];
	MWindow *mwindow;
	int data_type;
	int number;
	ArrayList<BC_ListBoxItem*> transition_names;
};



class TransitionDialog : public BC_Window
{
public:
	TransitionDialog(MWindow *mwindow,
		TransitionDialogThread *thread,
		int x,
		int y);

	void create_objects();
	int resize_event(int w, int h);

	MWindow *mwindow;
	TransitionDialogThread *thread;
	TransitionDialogName *name_list;
	BC_Title *name_title;
	TransitionSetDefault *set_default;
	TransitionDefaultText *set_default_text;
};

class TransitionDialogName : public BC_ListBox
{
public:
	TransitionDialogName(TransitionDialogThread *thread,
		ArrayList<BC_ListBoxItem*> *standalone_data,
		int x,
		int y,
		int w,
		int h);
	int handle_event();
	int selection_changed();
	TransitionDialogThread *thread;
};




#endif


