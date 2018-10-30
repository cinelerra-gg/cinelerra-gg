
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

#ifndef TRANSITIONPOPUP_H
#define TRANSITIONPOPUP_H

#include "bcdialog.h"
#include "guicast.h"
#include "mwindowgui.inc"
#include "plugin.inc"
#include "transition.inc"
#include "transitionpopup.inc"

class TransitionPopupOn;
class TransitionPopupShow;
class TransitionPopupAttach;
class TransitionPopupDetach;
class TransitionPopupLength;
class TransitionLengthText;

class TransitionLengthThread : public BC_DialogThread
{
public:
	TransitionLengthThread(MWindow *mwindow);
	~TransitionLengthThread();

	void start(Transition *transition,
		double length);
	BC_Window* new_gui();
	void handle_close_event(int result);

	MWindow *mwindow;

	Transition *transition;
	double length;
	double orig_length;
};


class TransitionLengthDialog : public BC_Window
{
public:
	TransitionLengthDialog(MWindow *mwindow,
		TransitionLengthThread *thread,
		int x,
		int y);
	~TransitionLengthDialog();

	void create_objects();
	int close_event();

	MWindow *mwindow;
	TransitionLengthThread *thread;
	TransitionLengthText *text;
	double length;
};

class TransitionLengthText : public BC_TumbleTextBox
{
public:
	TransitionLengthText(MWindow *mwindow,
		TransitionLengthDialog *gui,
		int x,
		int y);
	int handle_event();
	MWindow *mwindow;
	TransitionLengthDialog *gui;
};


class TransitionPopup : public BC_PopupMenu
{
public:
	TransitionPopup(MWindow *mwindow, MWindowGUI *gui);
	~TransitionPopup();

	void create_objects();
	int update(Transition *transition);

// Acquired through the update command as the plugin currently being operated on
// Can't be dereferenced.
	Transition *transition;
	double length;

// Set when the user clicks a transition.
	MWindow *mwindow;
	MWindowGUI *gui;

// Needed for loading updates
	TransitionPopupOn *on;
	TransitionPopupShow *show;
	TransitionPopupAttach *attach;
	TransitionPopupDetach *detach;
	TransitionPopupLength *length_item;
	TransitionLengthThread *length_thread;
};


class TransitionPopupAttach : public BC_MenuItem
{
public:
	TransitionPopupAttach(MWindow *mwindow, TransitionPopup *popup);
	~TransitionPopupAttach();

	int handle_event();
	MWindow *mwindow;
	TransitionPopup *popup;
};

class TransitionPopupDetach : public BC_MenuItem
{
public:
	TransitionPopupDetach(MWindow *mwindow, TransitionPopup *popup);
	~TransitionPopupDetach();

	int handle_event();
	MWindow *mwindow;
	TransitionPopup *popup;
};

class TransitionPopupShow : public BC_MenuItem
{
public:
	TransitionPopupShow(MWindow *mwindow, TransitionPopup *popup);
	~TransitionPopupShow();

	int handle_event();
	MWindow *mwindow;
	TransitionPopup *popup;
};

class TransitionPopupOn : public BC_MenuItem
{
public:
	TransitionPopupOn(MWindow *mwindow, TransitionPopup *popup);
	~TransitionPopupOn();

	int handle_event();
	MWindow *mwindow;
	TransitionPopup *popup;
};

class TransitionPopupLength : public BC_MenuItem
{
public:
	TransitionPopupLength(MWindow *mwindow, TransitionPopup *popup);
	~TransitionPopupLength();

	int handle_event();
	MWindow *mwindow;
	TransitionPopup *popup;
};

#endif
