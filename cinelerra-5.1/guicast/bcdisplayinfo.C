
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
#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "clip.h"
#include "language.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>

#define TEST_SIZE 128
#define TEST_DSIZE 28
#define TEST_SIZE2 164

int BC_DisplayInfo::top_border = -1;
int BC_DisplayInfo::left_border = -1;
int BC_DisplayInfo::bottom_border = -1;
int BC_DisplayInfo::right_border = -1;
int BC_DisplayInfo::auto_reposition_x = -1;
int BC_DisplayInfo::auto_reposition_y = -1;


BC_DisplayInfo::BC_DisplayInfo(const char *display_name, int show_error)
{
	screen = -1;
	xinerama_screens = -1;
	xinerama_info = 0;
	init_window(display_name, show_error);
}

BC_DisplayInfo::~BC_DisplayInfo()
{
	if( xinerama_info ) XFree(xinerama_info);
#ifndef SINGLE_THREAD
	XCloseDisplay(display);
#endif
}


void BC_DisplayInfo::parse_geometry(char *geom, int *x, int *y, int *width, int *height)
{
	XParseGeometry(geom, x, y, (unsigned int*)width, (unsigned int*)height);
}


int BC_DisplayInfo::get_xinerama_screens()
{
	if( xinerama_screens < 0 ) {
		xinerama_screens = 0;
		if( XineramaIsActive(display) )
			xinerama_info = XineramaQueryScreens(display, &xinerama_screens);
	}
	return xinerama_screens;
}

int BC_DisplayInfo::xinerama_geometry(int screen, int &x, int &y, int &w, int &h)
{
	int screens = get_xinerama_screens();
	if( !screens ) return 1;
	if( screen >= 0 ) {
		int k = screens;
		while( --k >= 0 && xinerama_info[k].screen_number != screen );
		if( k < 0 ) return 1;
		x = xinerama_info[k].x_org;  w = xinerama_info[k].width;
		y = xinerama_info[k].y_org;  h = xinerama_info[k].height;
	}
	else {
		int sx0 = INT_MAX, sx1 = INT_MIN;
		int sy0 = INT_MAX, sy1 = INT_MIN;
		for( int i=0; i<screens; ++i ) {
			int x0 = xinerama_info[i].x_org;
			int x1 = x0 + xinerama_info[i].width;
			if( sx0 > x0 ) sx0 = x0;
			if( sx1 < x1 ) sx1 = x1;
		 	int y0 = xinerama_info[i].y_org;
			int y1 = y0 + xinerama_info[i].height;
			if( sy0 > y0 ) sy0 = y0;
			if( sy1 < y1 ) sy1 = y1;
		}
		x = sx0;  w = sx1 - sx0;
		y = sy0;  h = sy1 - sy0;
	}
	return 0;
}

static void get_top_coords(Display *display, Window win, int &px,int &py, int &tx,int &ty)
{
	Window *pcwin = 0;  unsigned int ncwin = 0;
	Window cwin = 0, pwin = 0, root = 0;
	XQueryTree(display, win, &root, &pwin, &pcwin, &ncwin);
	if( pcwin ) XFree(pcwin);
	XTranslateCoordinates(display, pwin, root, 0,0, &px,&py, &cwin);
//printf(" win=%lx, px/py=%d/%d\n", win, px,py);

	int nx = px, ny = py;  pwin = win;
	for( int i=5; --i>=0; ) {
		win = pwin;  pwin = 0;  pcwin = 0;  ncwin = 0;
		Window rwin = 0;
// XQuerytTree has been known to fail here
		XQueryTree(display, win, &rwin, &pwin, &pcwin, &ncwin);
		if( pcwin ) XFree(pcwin);
		if( !rwin || rwin != root || pwin == root ) break;
		XTranslateCoordinates(display, pwin, root, 0,0, &nx,&ny, &cwin);
//printf(" win=%lx, nx/ny=%d/%d\n", win, nx,ny);
	}
	tx = nx;  ty = ny;
}


void BC_DisplayInfo::test_window(int &x_out,
	int &y_out,
	int &x_out2,
	int &y_out2,
	int x_in,
	int y_in)
{
#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_DisplayInfo::test_window");
#endif

	x_out = 0;
	y_out = 0;
        int x_out1 = 0;
        int y_out1 = 0;
	x_out2 = 0;
	y_out2 = 0;

	unsigned long mask = CWEventMask | CWWinGravity | CWBackPixel;
	XSetWindowAttributes attr;
	attr.event_mask = StructureNotifyMask;
	attr.win_gravity = SouthEastGravity;
	attr.background_pixel = BlackPixel(display,screen);
	Window win = XCreateWindow(display, rootwin,
			x_in, y_in, TEST_SIZE, TEST_SIZE,
			0, default_depth, InputOutput,
			vis, mask, &attr);
	XSizeHints size_hints;
	XGetNormalHints(display, win, &size_hints);
	size_hints.flags = PPosition | PSize;
	size_hints.x = x_in;
	size_hints.y = y_in;
	size_hints.width = TEST_SIZE;
	size_hints.height = TEST_SIZE;
	XSetStandardProperties(display, win,
		"x", "x", None, 0, 0, &size_hints);
	XClearWindow(display, win);
	XMapWindow(display, win);
	XFlush(display);  XSync(display, 0);  usleep(100000);

	XEvent event;
	int state = 0;

	while( state < 3 ) {
		XNextEvent(display, &event);
//printf("BC_DisplayInfo::test_window 1 event=%d %d\n", event.type, XPending(display));
 		if( event.xany.window != win ) continue;
		if( event.type != ConfigureNotify ) continue;
		Window cwin = 0;
		int rx = 0, ry = 0, px = 0, py = 0, tx = 0, ty = 0;
//printf("BC_DisplayInfo::test_window 1 state=%d x=%d y=%d w=%d h=%d bw=%d sev=%d\n",
//  state, event.xconfigure.x, event.xconfigure.y,
//  event.xconfigure.width, event.xconfigure.height,
//  event.xconfigure.border_width, event.xconfigure.send_event);
		get_top_coords(display,win, px,py, tx,ty);
//printf("x_in,y_in=%d,%d dx,dy=%d,%d\n", x_in,y_in, x_in-tx,y_in-ty);
		switch( state ) {
		case 0: // Get creation config
			XTranslateCoordinates(display, win, rootwin, 0,0, &rx,&ry, &cwin);
			x_out = rx - x_in;
			y_out = ry - y_in;
			XMoveResizeWindow(display, win, x_in,y_in, TEST_SIZE2,TEST_SIZE2);
			XFlush(display);  XSync(display, 0);  usleep(100000);
			++state;
			break;
		case 1: // Get moveresize resizing
			XTranslateCoordinates(display, win, rootwin, 0,0, &rx,&ry, &cwin);
			x_out1 = px;
			y_out1 = py;
			x_in += TEST_DSIZE;  y_in += TEST_DSIZE;
			XMoveResizeWindow(display, win, x_in,y_in, TEST_SIZE2,TEST_SIZE2);
			XFlush(display);  XSync(display, 0);  usleep(100000);
			++state;
			break;
		case 2: // Get moveresize move
			XTranslateCoordinates(display, win, rootwin, 0,0, &rx,&ry, &cwin);
			x_out2 = px - x_out1 - TEST_DSIZE;
			y_out2 = py - y_out1 - TEST_DSIZE;
			++state;
			break;
		}
 	}
//printf("\nBC_DisplayInfo::test_window 3 x0,y0=%d,%d, x1,y1=%d,%d, x2,y2=%d,%d\n",
//  x_out,y_out, x_out1,y_out1, x_out2,y_out2);
//printf("\nx_in,y_in=%d,%d\n", x_in,y_in);

	XDestroyWindow(display, win);
	XFlush(display);
	XSync(display, 0);

	x_out = MAX(0, MIN(x_out, 48));
	y_out = MAX(0, MIN(y_out, 48));

#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#endif
}

void BC_DisplayInfo::init_borders()
{
	if(top_border < 0)
	{
		BC_DisplayInfo display_info;
		display_info.test_window(left_border, top_border,
			auto_reposition_x, auto_reposition_y, 100, 100);
		right_border = left_border;
		bottom_border = left_border;
//printf("BC_DisplayInfo::init_borders border=%d %d auto=%d %d\n",
//  left_border, top_border, auto_reposition_x, auto_reposition_y);
	}
}


int BC_DisplayInfo::get_top_border()
{
	init_borders();
	return top_border;
}

int BC_DisplayInfo::get_left_border()
{
	init_borders();
	return left_border;
}

int BC_DisplayInfo::get_right_border()
{
	init_borders();
	return right_border;
}

int BC_DisplayInfo::get_bottom_border()
{
	init_borders();
	return bottom_border;
}

void BC_DisplayInfo::init_window(const char *display_name, int show_error)
{
	if(display_name && display_name[0] == 0) display_name = NULL;

#ifdef SINGLE_THREAD
	display = BC_Display::get_display(display_name);
#else

// This function must be the first Xlib
// function a multi-threaded program calls
	XInitThreads();

	if((display = XOpenDisplay(display_name)) == NULL)
	{
		if(!show_error) return;
		fprintf(stderr,_("BC_DisplayInfo::init_window: cannot open display \"%s\".\n"),
			display_name ? display_name : "");
		if(getenv("DISPLAY") == NULL)
    			fprintf(stderr, _("'DISPLAY' environment variable not set.\n"));
		if((display = XOpenDisplay(0)) == NULL) {
			fprintf(stderr,_("BC_DisplayInfo::init_window: cannot connect to X server.\n"));
			exit(1);
		}
 	}
#endif // SINGLE_THREAD

#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_DisplayInfo::init_window");
#endif
	screen = DefaultScreen(display);
	rootwin = RootWindow(display, screen);
	vis = DefaultVisual(display, screen);
	default_depth = DefaultDepth(display, screen);
#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#endif // SINGLE_THREAD
}


int BC_DisplayInfo::get_root_w()
{
#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_DisplayInfo::get_root_w");
#endif
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	int result = WidthOfScreen(screen_ptr);
#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#endif
	return result;
}

int BC_DisplayInfo::get_root_h()
{
#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_DisplayInfo::get_root_h");
#endif
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	int result = HeightOfScreen(screen_ptr);
#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#endif
	return result;
}

int BC_DisplayInfo::get_abs_cursor_x()
{
	int abs_x, abs_y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_DisplayInfo::get_abs_cursor_x");
#endif
	XQueryPointer(display,
	   rootwin,
	   &temp_win,
	   &temp_win,
       &abs_x,
	   &abs_y,
	   &win_x,
	   &win_y,
	   &temp_mask);
#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#endif
	return abs_x;
}

int BC_DisplayInfo::get_abs_cursor_y()
{
	int abs_x, abs_y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_DisplayInfo::get_abs_cursor_y");
#endif
	XQueryPointer(display,
	   rootwin,
	   &temp_win,
	   &temp_win,
       &abs_x,
	   &abs_y,
	   &win_x,
	   &win_y,
	   &temp_mask);
#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#endif
	return abs_y;
}


int BC_DisplayInfo::get_screen_count()
{
	return XScreenCount(display);
}


const char *BC_DisplayInfo::host_display_name(const char *display_name)
{
	return XDisplayName(display_name);
}

