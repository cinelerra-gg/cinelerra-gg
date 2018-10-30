
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

#include "bcclipboard.h"
#include "bcdisplay.h"
#include "bcrepeater.h"
#include "bcsignals.h"
#include "bcsubwindow.h"
#include "bcwindowbase.h"
#include "bcwindowevents.h"
#include "condition.h"
#include "language.h"
#include "mutex.h"

#include <pthread.h>

#ifdef SINGLE_THREAD

pthread_mutex_t BC_Display::display_lock = PTHREAD_MUTEX_INITIALIZER;
BC_Display* BC_Display::display_global = 0;

BC_Display::BC_Display(const char *display_name)
{
	done = 0;
	motion_events = 0;
	resize_events = 0;
	translation_events = 0;
	window_locked = 0;
	clipboard = 0;

// This function must be the first Xlib
// function a multi-threaded program calls
	XInitThreads();

	if(display_name && display_name[0] == 0) display_name = NULL;
	if((display = XOpenDisplay(display_name)) == NULL)
	{
  		printf("BC_Display::BC_Display: cannot connect to X server %s\n",
			display_name);
  		if(getenv("DISPLAY") == NULL)
    	{
			printf(_("'DISPLAY' environment variable not set.\n"));
  			exit(1);
		}
		else
// Try again with default display.
		{
			if((display = XOpenDisplay(0)) == NULL)
			{
				printf("BC_Display::BC_Display: cannot connect to default X server.\n");
				exit(1);
			}
		}
 	}

// Create atoms for events
	SetDoneXAtom = XInternAtom(display, "BC_REPEAT_EVENT", False);
	RepeaterXAtom = XInternAtom(display, "BC_CLOSE_EVENT", False);
	DelWinXAtom = XInternAtom(display, "WM_DELETE_WINDOW", False);
	ProtoXAtom = XInternAtom(display, "WM_PROTOCOLS", False);


// Start event handling
	event_thread = new BC_WindowEvents(this);
	event_thread->start();


	event_lock = new Mutex("BC_Display::event_lock", 1);
	event_condition = new Condition(0, "BC_Display::event_condition");
}

BC_Display::~BC_Display()
{
}

Display* BC_Display::get_display(const char *name)
{
	pthread_mutex_lock(&display_lock);
	if(!display_global)
	{
		display_global = new BC_Display(name);
	}
	pthread_mutex_unlock(&display_lock);

	return display_global->display;
}

int BC_Display::is_first(BC_WindowBase *window)
{
	int result = 0;

	if(windows.size() && windows.values[0] == window) result = 1;

	return result;
}

void BC_Display::dump_windows()
{
	for(int i = 0; i < windows.size(); i++) {
		printf("BC_Display::dump_windows %d window=%p window->win=0x%08jx\n",
			i, windows.get(i), windows.get(i)->win);
	}
}

void BC_Display::new_window(BC_WindowBase *window)
{
//printf("BC_Display::new_window %d\n", __LINE__);
	if(!clipboard)
	{
		clipboard = new BC_Clipboard(window);
		clipboard->start_clipboard();
	}

//printf("BC_Display::new_window %d\n", __LINE__);
	windows.append(window);
//	dump_windows();
//printf("BC_Display::new_window %d\n", __LINE__);
}

void BC_Display::delete_window(BC_WindowBase *window)
{
//printf("BC_Display::delete_window %d\n", __LINE__);
	windows.remove(window);
//printf("BC_Display::delete_window %d\n", __LINE__);
}

// If the event happened in any subwindow
int BC_Display::is_event_win(XEvent *event, BC_WindowBase *window)
{
	Window event_win = event->xany.window;
//printf("BC_Display::is_event_win %d\n", __LINE__);
	if(event_win == 0 || window->win == event_win) return 1;
//printf("BC_Display::is_event_win %d\n", __LINE__);
	for(int i = 0; i < window->subwindows->size(); i++)
	{
		if(is_event_win(event, window->subwindows->get(i)))
		{
//printf("BC_Display::is_event_win %d\n", __LINE__);
			return 1;
		}
	}

// Test popups in the main window only
	if(window->window_type == MAIN_WINDOW)
	{
// Not all events are handled by popups.
		for(int i = 0; i < window->popups.size(); i++)
		{
			if(window->popups.get(i)->win == event_win &&
				event->type != ConfigureNotify) return 1;
		}
	}
//printf("BC_Display::is_event_win %d\n", __LINE__);
	return 0;
}

void BC_Display::loop()
{
	const int debug = 0;
if(debug) printf("BC_Display::loop %d\n", __LINE__);

	while(!done)
	{
// If an event is waiting, process it now.
if(debug) printf("BC_Display::loop %d\n", __LINE__);
		if(get_event_count())
		{
if(debug) printf("BC_Display::loop %d\n", __LINE__);
			handle_event();
if(debug) printf("BC_Display::loop %d\n", __LINE__);
		}
		else
// Otherwise, process all compressed events & get the next event.
		{
if(debug) printf("BC_Display::loop %d\n", __LINE__);
			lock_display("BC_Display::loop");
			for(int i = 0; i < windows.size(); i++)
			{
				BC_WindowBase *window = windows.get(i);
if(debug) printf("BC_Display::loop %d %d %d %d\n",
__LINE__,
window->resize_events,
window->motion_events,
window->translation_events);
				if(window->resize_events)
					window->dispatch_resize_event(window->last_resize_w,
						window->last_resize_h);
				if(window->motion_events)
					window->dispatch_motion_event();
if(debug) printf("BC_Display::loop %d\n", __LINE__);
				if(window->translation_events)
					window->dispatch_translation_event();
			}
if(debug) printf("BC_Display::loop %d\n", __LINE__);
			unlock_display();
if(debug) printf("BC_Display::loop %d\n", __LINE__);

			handle_event();
if(debug) printf("BC_Display::loop %d\n", __LINE__);
		}
	}
if(debug) printf("BC_Display::loop %d\n", __LINE__);
}

void BC_Display::handle_event()
{
const int debug = 0;
	XEvent *event = get_event();
if(debug)  printf("BC_Display::handle_event %d type=%d\n",
__LINE__,
event->type);

	lock_display("BC_Display::handle_event");
if(debug) printf("BC_Display::handle_event %d\n", __LINE__);
	for(int i = 0; i < windows.size(); i++)
	{
// Test if event was inside window
if(debug) printf("BC_Display::handle_event %d\n", __LINE__);
		BC_WindowBase *window = windows.get(i);
if(debug) printf("BC_Display::handle_event %d\n", __LINE__);
		if(is_event_win(event, window))
// Dispatch event
			window->dispatch_event(event);
if(debug) printf("BC_Display::handle_event %d\n", __LINE__);
	}
if(debug) printf("BC_Display::handle_event %d\n", __LINE__);
	unlock_display();
if(debug) printf("BC_Display::handle_event %d\n", __LINE__);

	delete event;
if(debug) printf("BC_Display::handle_event %d\n", __LINE__);
}

// Get pending events for the given window
int BC_Display::get_event_count(BC_WindowBase *window)
{
	int result = 0;
	event_lock->lock("BC_WindowBase::get_event_count 2");
	for(int i = 0; i < common_events.size(); i++)
	{
		XEvent *event = common_events.get(i);
		if(is_event_win(event, window)) result++;
	}
	event_lock->unlock();
	return result;
}

int BC_Display::get_event_count()
{
	event_lock->lock("BC_WindowBase::get_event_count 1");
	int result = common_events.size();
	event_lock->unlock();
	return result;
}

XEvent* BC_Display::get_event()
{
	XEvent *result = 0;
	while(!done && !result)
	{
//printf("BC_Display::get_event %d\n", __LINE__);
		event_condition->lock("BC_WindowBase::get_event");
//printf("BC_Display::get_event %d\n", __LINE__);
		event_lock->lock("BC_WindowBase::get_event");
//printf("BC_Display::get_event %d\n", __LINE__);

		if(common_events.total && !done)
		{
			result = common_events.values[0];
			common_events.remove_number(0);
		}

		event_lock->unlock();
	}
	return result;
}

void BC_Display::put_event(XEvent *event)
{
	event_lock->lock("BC_WindowBase::put_event");
	common_events.append(event);
	event_lock->unlock();
	event_condition->unlock();
}

void BC_Display::set_repeat(BC_WindowBase *window, int64_t duration)
{
return;
// test repeater database for duplicates
	for(int i = 0; i < repeaters.total; i++)
	{
// Matching delay already exists
		if(repeaters.values[i]->delay == duration)
		{
			repeaters.values[i]->start_repeating(window);
			return;
		}
	}

	BC_Repeater *repeater = new BC_Repeater(window, duration);
	repeater->initialize();
	repeaters.append(repeater);
    repeater->start_repeating();
}

void BC_Display::unset_repeat(BC_WindowBase *window, int64_t duration)
{
	for(int i = 0; i < repeaters.size(); i++)
	{
		if(repeaters.get(i)->delay == duration)
		{
			repeaters.get(i)->stop_repeating(window);
		}
	}
}

void BC_Display::unset_all_repeaters(BC_WindowBase *window)
{
	for(int i = 0; i < repeaters.total; i++)
	{
		repeaters.values[i]->stop_all_repeating(window);
	}
}

void BC_Display::arm_repeat(int64_t duration)
{
	XEvent *event = BC_WindowBase::new_xevent();
	XClientMessageEvent *ptr = (XClientMessageEvent*)event;
	event->xany.window = 0;
	ptr->type = ClientMessage;
	ptr->message_type = RepeaterXAtom;
	ptr->format = 32;
	ptr->data.l[0] = duration;

// Couldn't use XSendEvent since it locked up randomly.
	put_event(event);
}

void BC_Display::arm_completion(BC_WindowBase *window)
{
	XEvent *event = BC_WindowBase::new_xevent();
	XClientMessageEvent *ptr = (XClientMessageEvent*)event;
	event->xany.window = window->win;
	event->type = ClientMessage;
	ptr->message_type = SetDoneXAtom;
	ptr->format = 32;

	put_event(event);
}

void BC_Display::unlock_repeaters(int64_t duration)
{
	for(int i = 0; i < repeaters.size(); i++)
	{
		if(repeaters.get(i)->delay == duration)
		{
			repeaters.get(i)->repeat_lock->unlock();
		}
	}
}


void BC_Display::lock_display(const char *location)
{
	pthread_mutex_lock(&BC_Display::display_lock);
	++BC_Display::display_global->window_locked;
	pthread_mutex_unlock(&BC_Display::display_lock);

//printf("BC_Display::lock_display %d %s result=%d\n", __LINE__, location, result);
	XLockDisplay(BC_Display::display_global->display);
}

void BC_Display::unlock_display()
{
	pthread_mutex_lock(&BC_Display::display_lock);
	--BC_Display::display_global->window_locked;
//	if(BC_Display::display_global->window_locked < 0)
//		BC_Display::display_global->window_locked = 0;
	pthread_mutex_unlock(&BC_Display::display_lock);

//printf("BC_Display::unlock_display %d result=%d\n", __LINE__, result);
	XUnlockDisplay(BC_Display::display_global->display);
}


int BC_Display::get_display_locked()
{
	return BC_Display::display_global->window_locked;
}


#endif
