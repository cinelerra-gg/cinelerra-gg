
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

#include "bcdisplay.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "bcwindowevents.h"
#include "bctimer.h"
#include "condition.h"
#include <unistd.h>

BC_WindowEvents::BC_WindowEvents(BC_WindowBase *window)
 : Thread(1, 0, 0)
{
	this->window = window;
	display = 0;
	done = 0;
}


BC_WindowEvents::BC_WindowEvents(BC_Display *display)
 : Thread(1, 0, 0)
{
	this->display = display;
	window = 0;
	done = 0;
}

BC_WindowEvents::~BC_WindowEvents()
{
// no more external events, send one last event into X to flush out all of the
//  pending events so that no straglers or shm leaks are possible.
//printf("BC_WindowEvents::~BC_WindowEvents %d %s\n", __LINE__, window->title);
	XSelectInput(window->display, window->win, 0);
	XEvent event;  memset(&event,0,sizeof(event));
	XClientMessageEvent *ptr = (XClientMessageEvent*)&event;
	event.type = ClientMessage;
	ptr->message_type = window->DestroyAtom;
	ptr->format = 32;
	XSendEvent(window->display, window->win, 0, 0, &event);
	window->flush();
	Thread::join();
//printf("BC_WindowEvents::~BC_WindowEvents %d %s\n", __LINE__, window->title);
}

void BC_WindowEvents::start()
{
	done = 0;
	Thread::start();
}


void BC_WindowEvents::run()
{
// Can't cancel in XNextEvent because X server never figures out it's not
// listening anymore and XCloseDisplay locks up.
	XEvent *event;
#ifndef SINGLE_THREAD
	int x_fd = ConnectionNumber(window->display);
#endif

	while(!done)
	{

// Can't cancel in XNextEvent because X server never figures out it's not
// listening anymore and XCloseDisplay locks up.
#ifdef SINGLE_THREAD
		event = new XEvent;
		XNextEvent(display->display, event);
#else
// This came from a linuxquestions post.
// We can get a file descriptor for the X display & use select instead of XNextEvent.
// The newest X11 library requires locking the display to use XNextEvent.
		fd_set x_fds;
		FD_ZERO(&x_fds);
		FD_SET(x_fd, &x_fds);
		struct timeval tv;
		tv.tv_sec = 0;  tv.tv_usec = 200000;
//printf("BC_WindowEvents::run %d %s\n", __LINE__, window->title);
		select(x_fd + 1, &x_fds, 0, 0, &tv);
		XLockDisplay(window->display);
		while(!done && XPending(window->display))
		{
			event = new XEvent;
			XNextEvent(window->display, event);
#endif
			if( event->type == BC_WindowBase::shm_completion_event ) {
				window->active_bitmaps.reque(event);
				delete event;
				continue;
			}
			if( event->type == ClientMessage ) {
				XClientMessageEvent *ptr = (XClientMessageEvent*)event;
				if( ptr->message_type == window->DestroyAtom ) {
					delete event;
					done = 1;
					break;
				}
			}
#ifndef SINGLE_THREAD
// HACK: delay is required to get the close event
			yield();
//if(window && event)
//printf("BC_WindowEvents::run %d %s %d\n", __LINE__, window->title, event->type);
			window->put_event(event);
		}
		XUnlockDisplay(window->display);
//printf("BC_WindowEvents::run %d %s\n", __LINE__, window->title);
#else
		display->put_event(event);
#endif

	}
}



