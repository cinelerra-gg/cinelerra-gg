
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

#ifndef BCCLIPBOARD_H
#define BCCLIPBOARD_H

#include "bcwindowbase.inc"
#include "thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>


enum {
        CLIP_PRIMARY,		// XA_PRIMARY
        CLIP_CLIPBOARD,		// Atom(CLIPBOARD)
        CLIP_BUFFER0,		// XA_CUT_BUFFER0
        CLIP_BUFFERS = CLIP_BUFFER0, // CUT_BUFFER api does not need storage
};
// loads CUT_BUFFER0 if window is closed while selection set

// The primary selection is filled by highlighting a region
#define PRIMARY_SELECTION CLIP_PRIMARY

// The secondary selection is filled by copying (textbox Ctrl-C)
#define SECONDARY_SELECTION CLIP_CLIPBOARD

// Storage for guicast only
#define BC_PRIMARY_SELECTION (CLIP_BUFFER0+2)

// The secondary selection has never been reliable either in Cinelerra
// or anything else.  We just use the CUT_BUFFER2 solution for any data
// not intended for use outside Cinelerra.

class BC_Clipboard : public Thread
{
public:
	BC_Clipboard(BC_WindowBase *window);
	~BC_Clipboard();

	int start_clipboard();
	void run();
	int stop_clipboard();
	int to_clipboard(BC_WindowBase *owner, const char *data, long len, int clipboard_num);
	long from_clipboard(char *data, long maxlen, int clipboard_num);
	long clipboard_len(int clipboard_num);

private:
	long from_clipboard(int clipboard_num, char *&bfr, long maxlen);
	void handle_selectionrequest(XSelectionRequestEvent *request);
	int handle_request_string(XSelectionRequestEvent *request);
	int handle_request_targets(XSelectionRequestEvent *request);

	BC_WindowBase *window;
	Display *in_display, *out_display;
	Window in_win, out_win;
	Atom xa_primary, clipboard, targets;
	Atom completion_atom, string_type;

	char *data_buffer[CLIP_BUFFERS];
	int data_length[CLIP_BUFFERS];
};

#endif
