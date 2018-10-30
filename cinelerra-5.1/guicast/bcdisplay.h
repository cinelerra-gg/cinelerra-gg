
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

#ifndef BCDISPLAY_H
#define BCDISPLAY_H

#include "arraylist.h"
#include "bcclipboard.inc"
#include "bcrepeater.inc"
#include "bcwindowbase.inc"
#include "bcwindowevents.inc"
#include "condition.inc"
#include "mutex.inc"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xatom.h>
#ifdef HAVE_XFT
#include <X11/Xft/Xft.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <pthread.h>

// For single threaded event handling


#ifdef SINGLE_THREAD
class BC_Display
{
public:
	BC_Display(const char *display_name);
	~BC_Display();

	static Display* get_display(const char *name);
	void new_window(BC_WindowBase *window);
	void delete_window(BC_WindowBase *window);
	int is_first(BC_WindowBase *window);
	int is_event_win(XEvent *event, BC_WindowBase *window);
	void handle_event();
	int get_event_count(BC_WindowBase *window);
	int get_event_count();
	XEvent* get_event();
	void set_repeat(BC_WindowBase *window, int64_t duration);
	void unset_repeat(BC_WindowBase *window, int64_t duration);
	void unset_all_repeaters(BC_WindowBase *window);
	void unlock_repeaters(int64_t duration);
	void arm_repeat(int64_t duration);
	void arm_completion(BC_WindowBase *window);
	static void lock_display(const char *location);
	static void unlock_display();
	int get_display_locked();
	void loop();
	void put_event(XEvent *event);
	void dump_windows();

// Common events coming from X server and repeater.
	ArrayList<XEvent*> common_events;
// Locks for common events
// Locking order:
// 1) event_condition
// 2) event_lock
	Mutex *event_lock;
	Condition *event_condition;
// 1) table_lock
// 2) lock_window
//	Mutex *table_lock;
	ArrayList<BC_WindowBase*> windows;
	BC_WindowEvents *event_thread;
	int done;
	int motion_events;
	int resize_events;
	int translation_events;
// Array of repeaters for multiple repeating objects.
	ArrayList<BC_Repeater*> repeaters;

	Display *display;
	static BC_Display *display_global;
	static pthread_mutex_t display_lock;
	int window_locked;
// Copy of atoms which every window has.
	Atom DelWinXAtom;
	Atom ProtoXAtom;
	Atom RepeaterXAtom;
	Atom SetDoneXAtom;


	BC_Clipboard *clipboard;
};
#endif




#endif



