
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
#include "bcresources.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "bcwindowbase.inc"
#include <string.h>
#include <unistd.h>

BC_Clipboard::BC_Clipboard(BC_WindowBase *window)
 : Thread(1, 0, 0)
{
	this->window = window;
	const char *display_name = window->display_name;

#ifdef SINGLE_THREAD
	in_display = out_display = BC_Display::get_display(display_name);
#else
	in_display = BC_WindowBase::init_display(display_name);
	out_display = BC_WindowBase::init_display(display_name);
#endif

	completion_atom = XInternAtom(out_display, "BC_CLOSE_EVENT", False);
	xa_primary = XA_PRIMARY;
	clipboard = XInternAtom(out_display, "CLIPBOARD", False);
	targets = XInternAtom(out_display, "TARGETS", False);
	string_type = !BC_Resources::locale_utf8 ? XA_STRING :
		 XInternAtom(out_display, "UTF8_STRING", False);
	in_win = XCreateSimpleWindow(in_display,
		DefaultRootWindow(in_display), 0, 0, 1, 1, 0, 0, 0);
	out_win = XCreateSimpleWindow(out_display,
		DefaultRootWindow(out_display), 0, 0, 1, 1, 0, 0, 0);

	for( int i=0; i<CLIP_BUFFERS; ++i ) {
		data_buffer[i] = 0;
		data_length[i] = 0;
	}
}

BC_Clipboard::~BC_Clipboard()
{
	for( int i=0; i<CLIP_BUFFERS; ++i ) {
		delete [] data_buffer[i];
	}
	XDestroyWindow(in_display, in_win);
	XCloseDisplay(in_display);
	XDestroyWindow(out_display, out_win);
	XCloseDisplay(out_display);
}

int BC_Clipboard::start_clipboard()
{
#ifndef SINGLE_THREAD
	Thread::start();
#endif
	return 0;
}

int BC_Clipboard::stop_clipboard()
{
// if closing clipboard with selection, move data to CUT_BUFFER0
	char *data = 0;  int len = 0;
	for( int i=0; !data && i<CLIP_BUFFERS; ++i ) {
		data = data_buffer[i];  len = data_length[i];
	}
	if( data ) XStoreBuffer(out_display, data, len, 0);
#ifdef SINGLE_THREAD
	XFlush(in_display);
#else
	XFlush(in_display);
	XFlush(out_display);
#endif
// Must use a different display handle to send events.
	const char *display_name = window->display_name;
	Display *display = BC_WindowBase::init_display(display_name);
	XEvent event;  memset(&event, 0, sizeof(event));
	XClientMessageEvent *ptr = (XClientMessageEvent*)&event;

	event.type = ClientMessage;
	ptr->message_type = completion_atom;
	ptr->format = 32;
	XSendEvent(display, out_win, 0, 0, &event);
	XFlush(display);
	XCloseDisplay(display);

	Thread::join();
	return 0;
}

void BC_Clipboard::run()
{
	XEvent event;
	int done = 0;
#ifndef SINGLE_THREAD
	int x_fd = ConnectionNumber(out_display);
#endif

	while(!done) {
#ifndef SINGLE_THREAD
// see bcwindowevents.C regarding XNextEvent
		fd_set x_fds;
		FD_ZERO(&x_fds);
		FD_SET(x_fd, &x_fds);
		struct timeval tv;
		tv.tv_sec = 0;  tv.tv_usec = 200000;
		select(x_fd + 1, &x_fds, 0, 0, &tv);
		XLockDisplay(out_display);

		while( XPending(out_display) ) {
#endif
			XNextEvent(out_display, &event);

#ifdef SINGLE_THREAD
			BC_Display::lock_display("BC_Clipboard::run");
#endif
			switch( event.type ) {
			case ClientMessage:
				if( event.xclient.message_type == completion_atom )
					done = 1;
				break;

			case SelectionRequest:
				handle_selectionrequest(&event.xselectionrequest);
				break;

			case SelectionClear: {
				Atom selection = event.xselectionclear.selection;
				int idx =
					selection == xa_primary ? CLIP_PRIMARY :
					selection == clipboard  ? CLIP_CLIPBOARD : -1 ;
				if( idx < 0 ) break;
				delete [] data_buffer[idx];
				data_buffer[idx] = 0;
				data_length[idx] = 0;
				Window win = event.xselectionclear.window;
#ifndef SINGLE_THREAD
				XUnlockDisplay(out_display);
#endif
				window->lock_window("BC_Clipboard::run");
				window->do_selection_clear(win);
				window->unlock_window();
#ifndef SINGLE_THREAD
				XLockDisplay(out_display);
#endif
				break; }
			}
#ifdef SINGLE_THREAD
			BC_Display::unlock_display();
#else
		}
		XUnlockDisplay(out_display);
#endif
	}
}

long BC_Clipboard::from_clipboard(char *data, long maxlen, int clipboard_num)
{
	if( !data || maxlen <= 0 ) return -1;
	data[0] = 0;
	char *bfr;
	long len = from_clipboard(clipboard_num, bfr, maxlen);
	if( len >= maxlen ) len = maxlen-1;
	if( bfr && len >= 0 ) {
		strncpy(data, bfr, len);
		data[len] = 0;
	}
	if( bfr ) XFree(bfr);
	return len;
}

long BC_Clipboard::clipboard_len(int clipboard_num)
{
	char *bfr;
	long len = from_clipboard(clipboard_num, bfr, 0);
	if( bfr ) XFree(bfr);
	return len < 0 ? 0 : len+1;
}

long BC_Clipboard::from_clipboard(int clipboard_num, char *&bfr, long maxlen)
{
#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_Clipboard::from_clipboard");
#else
	XLockDisplay(in_display);
#endif

	bfr = 0;
	long len = 0;
	if( clipboard_num < CLIP_BUFFER0 ) {
		Atom selection = clipboard_num == CLIP_PRIMARY ? xa_primary : clipboard;
		Atom target = string_type, property = selection;
		XConvertSelection(in_display, selection, target, property, in_win, CurrentTime);

		XEvent event;
		do {
			XNextEvent(in_display, &event);
		} while( event.type != SelectionNotify && event.type != None );

		if( event.type == SelectionNotify && property == event.xselection.property ) {
			unsigned long size = 0, items = 0;
			Atom prop_type = 0;  int bits_per_item = 0;
			XGetWindowProperty(in_display, in_win, property, 0, (maxlen+3)/4,
				False, AnyPropertyType, &prop_type, &bits_per_item,
				&items, &size, (unsigned char**)&bfr);
			len = !prop_type ? -1 :
				!maxlen ? size :
				(items*bits_per_item + 7)/8;
		}
		else
			clipboard_num = CLIP_BUFFER0;
	}
	if( clipboard_num >= CLIP_BUFFER0 ) {
		int idx = clipboard_num - CLIP_BUFFER0, size = 0;
		bfr = XFetchBuffer(in_display, &size, idx);
		len = size;
	}

#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#else
	XUnlockDisplay(in_display);
#endif
	return len;
}

int BC_Clipboard::to_clipboard(BC_WindowBase *owner, const char *data, long len, int clipboard_num)
{
	if( !data || len < 0 ) return -1;
#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_Clipboard::to_clipboard");
#else
	XLockDisplay(out_display);
#endif

	if( clipboard_num < CLIP_BUFFER0 ) {
		char *bfr = data_buffer[clipboard_num];
		if( data_length[clipboard_num] != len ) {
			delete [] bfr;  bfr = new char[len];
			data_buffer[clipboard_num] = bfr;
			data_length[clipboard_num] = len;
		}
		memcpy(bfr, data, len);
		Atom selection = clipboard_num == CLIP_PRIMARY ? xa_primary : clipboard;
// this is not supposed to be necessary according to the man page
		Window cur = XGetSelectionOwner(out_display, selection);
		if( cur != owner->win && cur != None )
			XSetSelectionOwner(out_display, selection, None, CurrentTime);
		XSetSelectionOwner(out_display, selection, owner->win, CurrentTime);
		XFlush(out_display);
	}
	else {
		int idx = clipboard_num - CLIP_BUFFER0;
		XStoreBuffer(out_display, data, len, idx);
	}

#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#else
	XUnlockDisplay(out_display);
#endif
	return 0;
}

int BC_Clipboard::handle_request_string(XSelectionRequestEvent *xev)
{
	int idx =
		xev->selection == xa_primary ? CLIP_PRIMARY :
		xev->selection == clipboard  ? CLIP_CLIPBOARD : -1 ;
	if( idx < 0 ) return 0;
	char *data = data_buffer[idx];
	if( !data ) return 0;
	int len = data_length[idx];

	XChangeProperty(out_display, xev->requestor,
		xev->property, string_type, 8, PropModeReplace,
		(unsigned char*)data, len);
	return 1;
}

int BC_Clipboard::handle_request_targets(XSelectionRequestEvent *xev)
{
	Atom target_atoms[] = { targets, string_type };
	int ntarget_atoms = sizeof(target_atoms)/sizeof(target_atoms[0]);
	XChangeProperty(out_display, xev->requestor,
		xev->property, XA_ATOM, 32, PropModeReplace,
		(unsigned char*)target_atoms, ntarget_atoms);
	return 1;
}

void BC_Clipboard::handle_selectionrequest(XSelectionRequestEvent *xev)
{
	XEvent reply;  memset(&reply, 0, sizeof(reply));
// 'None' tells the client that the request was denied
	reply.xselection.property  =
	    (xev->target == string_type && handle_request_string(xev)) ||
	    (xev->target == targets && handle_request_targets(xev)) ?
		xev->property : None;
	reply.xselection.type      = SelectionNotify;
	reply.xselection.display   = xev->display;
	reply.xselection.requestor = xev->requestor;
	reply.xselection.selection = xev->selection;
	reply.xselection.target    = xev->target;
	reply.xselection.time      = xev->time;

	XSendEvent(out_display, xev->requestor, 0, 0, &reply);
	XFlush(out_display);
}

