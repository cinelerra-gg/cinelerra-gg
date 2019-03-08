
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

#include "bcbitmap.h"
#include "bcclipboard.h"
#include "bcdisplay.h"
#include "bcdisplayinfo.h"
#include "bcmenubar.h"
#include "bcpixmap.h"
#include "bcpopup.h"
#include "bcpopupmenu.h"
#include "bcrepeater.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsubwindow.h"
#include "bcsynchronous.h"
#include "bctimer.h"
#include "bcwindowbase.h"
#include "bcwindowevents.h"
#include "bccmodels.h"
#include "bccolors.h"
#include "condition.h"
#include "cursors.h"
#include "bchash.h"
#include "fonts.h"
#include "keys.h"
#include "language.h"
#include "mutex.h"
#include "sizes.h"
#include "vframe.h"
#include "workarounds.h"

#ifdef HAVE_GL
#include <GL/gl.h>
#endif
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <typeinfo>

#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xvlib.h>
#include <X11/extensions/shape.h>
#include <X11/XF86keysym.h>
#include <X11/Sunkeysym.h>

BC_ResizeCall::BC_ResizeCall(int w, int h)
{
	this->w = w;
	this->h = h;
}







int BC_WindowBase::shm_completion_event = -1;



BC_Resources BC_WindowBase::resources;

Window XGroupLeader = 0;

Mutex BC_KeyboardHandlerLock::keyboard_listener_mutex("keyboard_listener",0);
ArrayList<BC_KeyboardHandler*> BC_KeyboardHandler::listeners;

BC_WindowBase::BC_WindowBase()
{
//printf("BC_WindowBase::BC_WindowBase 1\n");
	BC_WindowBase::initialize();
}

BC_WindowBase::~BC_WindowBase()
{
#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_WindowBase::~BC_WindowBase");
#else
	if(window_type == MAIN_WINDOW)
		lock_window("BC_WindowBase::~BC_WindowBase");
#endif

#ifdef HAVE_LIBXXF86VM
	if(window_type == VIDMODE_SCALED_WINDOW && vm_switched) {
		restore_vm();
	}
#endif
	is_deleting = 1;

	hide_tooltip();
	if(window_type != MAIN_WINDOW)
	{
// stop event input
		XSelectInput(top_level->display, this->win, 0);
		XSync(top_level->display,0);
#ifndef SINGLE_THREAD
		top_level->dequeue_events(win);
#endif
// drop active window refs to this
		if(top_level->active_menubar == this) top_level->active_menubar = 0;
		if(top_level->active_popup_menu == this) top_level->active_popup_menu = 0;
		if(top_level->active_subwindow == this) top_level->active_subwindow = 0;
// drop motion window refs to this
		if(top_level->motion_events && top_level->last_motion_win == this->win)
			top_level->motion_events = 0;

// Remove pointer from parent window to this
		parent_window->subwindows->remove(this);
	}

	if(grab_active) grab_active->active_grab = 0;
	if(icon_window) delete icon_window;
	if(window_type == POPUP_WINDOW)
		parent_window->remove_popup(this);

// Delete the subwindows
	if(subwindows)
	{
		while(subwindows->total)
		{
// Subwindow removes its own pointer
			delete subwindows->values[0];
		}
		delete subwindows;
	}

	delete pixmap;

//printf("delete glx=%08x, win=%08x %s\n", (unsigned)glx_win, (unsigned)win, title);
#ifdef HAVE_GL
	if( get_resources()->get_synchronous() && glx_win != 0 ) {
		get_resources()->get_synchronous()->delete_window(this);
	}
#endif
	XDestroyWindow(top_level->display, win);

	if(bg_pixmap && !shared_bg_pixmap) delete bg_pixmap;
	if(icon_pixmap) delete icon_pixmap;
	if(temp_bitmap) delete temp_bitmap;
	top_level->active_bitmaps.remove_buffers(this);
	if(_7segment_pixmaps)
	{
		for(int i = 0; i < TOTAL_7SEGMENT; i++)
			delete _7segment_pixmaps[i];

		delete [] _7segment_pixmaps;
	}



	if(window_type == MAIN_WINDOW)
	{
		XFreeGC(display, gc);
		static XFontStruct *BC_WindowBase::*xfont[] = {
			 &BC_WindowBase::smallfont,
			 &BC_WindowBase::mediumfont,
			 &BC_WindowBase::largefont,
			 &BC_WindowBase::bigfont,
			 &BC_WindowBase::clockfont,
		};
		for( int i=sizeof(xfont)/sizeof(xfont[0]); --i>=0; )
			XFreeFont(display, this->*xfont[i]);

#ifdef HAVE_XFT
// prevents a bug when Xft closes with unrefd fonts
		FcPattern *defaults = FcPatternCreate();
		FcPatternAddInteger(defaults, "maxunreffonts", 0);
		XftDefaultSet(display, defaults);

		static void *BC_WindowBase::*xft_font[] = {
			 &BC_WindowBase::smallfont_xft,
			 &BC_WindowBase::mediumfont_xft,
			 &BC_WindowBase::largefont_xft,
			 &BC_WindowBase::bigfont_xft,
			 &BC_WindowBase::bold_smallfont_xft,
			 &BC_WindowBase::bold_mediumfont_xft,
			 &BC_WindowBase::bold_largefont_xft,
			 &BC_WindowBase::clockfont_xft,
		};
		for( int i=sizeof(xft_font)/sizeof(xft_font[0]); --i>=0; ) {
			XftFont *xft = (XftFont *)(this->*xft_font[i]);
			if( xft ) xftFontClose (display, xft);
		}
#endif
		finit_im();
		flush();
		sync_display();

		if( xinerama_info )
			XFree(xinerama_info);
		xinerama_screens = 0;
		xinerama_info = 0;
		if( xvideo_port_id >= 0 )
			XvUngrabPort(display, xvideo_port_id, CurrentTime);

		unlock_window();
// Must be last reference to display.
// _XftDisplayInfo needs a lock.
		get_resources()->create_window_lock->lock("BC_WindowBase::~BC_WindowBase");
		XCloseDisplay(display);
		get_resources()->create_window_lock->unlock();

// clipboard uses a different display connection
		clipboard->stop_clipboard();
		delete clipboard;
	}

	resize_history.remove_all_objects();

#ifndef SINGLE_THREAD
	common_events.remove_all_objects();
	delete event_lock;
	delete event_condition;
	delete init_lock;
#else
	top_level->window_lock = 0;
	BC_Display::unlock_display();
#endif
	delete cursor_timer;

#if HAVE_GL
	if( glx_fbcfgs_window ) XFree(glx_fbcfgs_window);
	if( glx_fbcfgs_pbuffer) XFree(glx_fbcfgs_pbuffer);
	if( glx_fbcfgs_pixmap ) XFree(glx_fbcfgs_pixmap);
#endif

	UNSET_ALL_LOCKS(this)
}

int BC_WindowBase::initialize()
{
	done = 0;
	done_set = 0;
	window_running = 0;
	display_lock_owner = 0;
	test_keypress = 0;
	keys_return[0] = 0;
	is_deleting = 0;
	window_lock = 0;
	resend_event_window = 0;
	x = 0;
	y = 0;
	w = 0;
	h = 0;
	bg_color = -1;
	line_width = 1;
	line_dashes = 0;
	top_level = 0;
	parent_window = 0;
	subwindows = 0;
	xinerama_info = 0;
	xinerama_screens = 0;
	xvideo_port_id = -1;
	video_on = 0;
	motion_events = 0;
	resize_events = 0;
	translation_events = 0;
	ctrl_mask = shift_mask = alt_mask = 0;
	cursor_x = cursor_y = button_number = 0;
	button_down = 0;
	button_pressed = 0;
	button_time1 = 0;
	button_time2 = 0;
	button_time3 = 0;
	double_click = 0;
	triple_click = 0;
	event_win = 0;
	last_motion_win = 0;
	key_pressed = 0;
	active_grab = 0;
	grab_active = 0;
	active_menubar = 0;
	active_popup_menu = 0;
	active_subwindow = 0;
	cursor_entered = 0;
	pixmap = 0;
	bg_pixmap = 0;
	_7segment_pixmaps = 0;
	tooltip_text = 0;
	force_tooltip = 0;
//	next_repeat_id = 0;
	tooltip_popup = 0;
	current_font = MEDIUMFONT;
	current_color = BLACK;
	current_cursor = ARROW_CURSOR;
	hourglass_total = 0;
	is_dragging = 0;
	shared_bg_pixmap = 0;
	icon_pixmap = 0;
	icon_window = 0;
	window_type = MAIN_WINDOW;
	translation_count = 0;
	x_correction = y_correction = 0;
	temp_bitmap = 0;
	tooltip_on = 0;
	temp_cursor = 0;
	toggle_value = 0;
	toggle_drag = 0;
	has_focus = 0;
	is_hourglass = 0;
	is_transparent = 0;
#ifdef HAVE_LIBXXF86VM
	vm_switched = 0;
#endif
	input_method = 0;
	input_context = 0;

	smallfont = 0;
	mediumfont = 0;
	largefont = 0;
	bigfont = 0;
	clockfont = 0;

	smallfont_xft = 0;
	mediumfont_xft = 0;
	largefont_xft = 0;
	bigfont_xft = 0;
	clockfont_xft = 0;

	bold_smallfont_xft = 0;
	bold_mediumfont_xft = 0;
	bold_largefont_xft = 0;
#ifdef SINGLE_THREAD
	completion_lock = new Condition(0, "BC_WindowBase::completion_lock");
#else
// Need these right away since put_event is called before run_window sometimes.
	event_lock = new Mutex("BC_WindowBase::event_lock");
	event_condition = new Condition(0, "BC_WindowBase::event_condition");
	init_lock = new Condition(0, "BC_WindowBase::init_lock");
#endif

	cursor_timer = new Timer;
	event_thread = 0;
#ifdef HAVE_GL
	glx_fbcfgs_window = 0;	n_fbcfgs_window = 0;
	glx_fbcfgs_pbuffer = 0;	n_fbcfgs_pbuffer = 0;
	glx_fbcfgs_pixmap = 0;	n_fbcfgs_pixmap = 0;

	glx_fb_config = 0;
	glx_win_context = 0;
	glx_win = 0;
#endif

	flash_enabled = 1;
	win = 0;
	return 0;
}



#define DEFAULT_EVENT_MASKS EnterWindowMask | \
			LeaveWindowMask | \
			ButtonPressMask | \
			ButtonReleaseMask | \
			PointerMotionMask | \
			FocusChangeMask


int BC_WindowBase::create_window(BC_WindowBase *parent_window, const char *title,
		int x, int y, int w, int h, int minw, int minh, int allow_resize,
		int private_color, int hide, int bg_color, const char *display_name,
		int window_type, BC_Pixmap *bg_pixmap, int group_it)
{
	XSetWindowAttributes attr;
	unsigned long mask;
	XSizeHints size_hints;
	int root_w;
	int root_h;
#ifdef HAVE_LIBXXF86VM
	int vm;
#endif

	id = get_resources()->get_id();
	if(parent_window) top_level = parent_window->top_level;
	if( top_level ) lock_window("BC_WindowBase::create_window");
	get_resources()->create_window_lock->lock("BC_WindowBase::create_window");

#ifdef HAVE_LIBXXF86VM
	if(window_type == VIDMODE_SCALED_WINDOW)
		closest_vm(&vm,&w,&h);
#endif

	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	this->bg_color = bg_color;
	this->window_type = window_type;
	this->hidden = hide;
	this->private_color = private_color;
	this->parent_window = parent_window;
	this->bg_pixmap = bg_pixmap;
	this->allow_resize = allow_resize;
	if(display_name)
		strcpy(this->display_name, display_name);
	else
		this->display_name[0] = 0;

	put_title(title);
	if(bg_pixmap) shared_bg_pixmap = 1;

	subwindows = new BC_SubWindowList;

	if(window_type == MAIN_WINDOW)
	{
		top_level = this;
		parent_window = this;


#ifdef SINGLE_THREAD
		display = BC_Display::get_display(display_name);
		BC_Display::lock_display("BC_WindowBase::create_window");
//		BC_Display::display_global->new_window(this);
#else

// get the display connection

// This function must be the first Xlib
// function a multi-threaded program calls
		XInitThreads();
		display = init_display(display_name);
		if( shm_completion_event < 0 ) shm_completion_event =
			ShmCompletion + XShmGetEventBase(display);
#endif
		lock_window("BC_WindowBase::create_window 1");

		screen = DefaultScreen(display);
		rootwin = RootWindow(display, screen);
// window placement boundaries
		if( !xinerama_screens && XineramaIsActive(display) )
			xinerama_info = XineramaQueryScreens(display, &xinerama_screens);
		root_w = get_root_w(0);
		root_h = get_root_h(0);

#if HAVE_GL
		vis = get_glx_visual(display);
		if( !vis )
#endif
			vis = DefaultVisual(display, screen);

		default_depth = DefaultDepth(display, screen);

		client_byte_order = (*(const u_int32_t*)"a   ") & 0x00000001;
		server_byte_order = (XImageByteOrder(display) == MSBFirst) ? 0 : 1;


// This must be done before fonts to know if antialiasing is available.
		init_colors();
// get the resources
		if(resources.use_shm < 0) resources.initialize_display(this);
		x_correction = BC_DisplayInfo::get_left_border();
		y_correction = BC_DisplayInfo::get_top_border();

// clamp window placement
		if(this->x + this->w + x_correction > root_w)
			this->x = root_w - this->w - x_correction;
		if(this->y + this->h + y_correction > root_h)
			this->y = root_h - this->h - y_correction;
		if(this->x < 0) this->x = 0;
		if(this->y < 0) this->y = 0;

		if(this->bg_color == -1)
			this->bg_color = resources.get_bg_color();

// printf("bcwindowbase 1 %s\n", title);
// if(window_type == MAIN_WINDOW) sleep(1);
// printf("bcwindowbase 10\n");
		init_fonts();
		init_gc();
		init_cursors();

// Create the window
		mask = CWEventMask | CWBackPixel | CWColormap | CWCursor;

		attr.event_mask = DEFAULT_EVENT_MASKS |
			StructureNotifyMask |
			KeyPressMask |
			KeyReleaseMask;

		attr.background_pixel = get_color(this->bg_color);
		attr.colormap = cmap;
		attr.cursor = get_cursor_struct(ARROW_CURSOR);

		win = XCreateWindow(display, rootwin,
			this->x, this->y, this->w, this->h, 0,
			top_level->default_depth, InputOutput,
			vis, mask, &attr);
		XGetNormalHints(display, win, &size_hints);

		size_hints.flags = PSize | PMinSize | PMaxSize;
		size_hints.width = this->w;
		size_hints.height = this->h;
		size_hints.min_width = allow_resize ? minw : this->w;
		size_hints.max_width = allow_resize ? 32767 : this->w;
		size_hints.min_height = allow_resize ? minh : this->h;
		size_hints.max_height = allow_resize ? 32767 : this->h;
		if(x > -BC_INFINITY && x < BC_INFINITY)
		{
			size_hints.flags |= PPosition;
			size_hints.x = this->x;
			size_hints.y = this->y;
		}
		XSetWMProperties(display, win, 0, 0, 0, 0, &size_hints, 0, 0);
		get_atoms();
		set_title(title);
#ifndef SINGLE_THREAD
		clipboard = new BC_Clipboard(this);
		clipboard->start_clipboard();
#endif


		if (group_it)
		{
			Atom ClientLeaderXAtom;
			if (XGroupLeader == 0)
				XGroupLeader = win;
			const char *instance_name = "cinelerra";
			const char *class_name = "Cinelerra";
			XClassHint *class_hints = XAllocClassHint();
			class_hints->res_name = (char*)instance_name;
			class_hints->res_class = (char*)class_name;
			XSetClassHint(top_level->display, win, class_hints);
			XFree(class_hints);
			ClientLeaderXAtom = XInternAtom(display, "WM_CLIENT_LEADER", True);
			XChangeProperty(display, win, ClientLeaderXAtom, XA_WINDOW, 32,
				PropModeReplace, (unsigned char *)&XGroupLeader, true);
		}
		init_im();
		set_icon(get_resources()->default_icon);
	}

#ifdef HAVE_LIBXXF86VM
	if(window_type == VIDMODE_SCALED_WINDOW && vm != -1)
	{
		scale_vm (vm);
		vm_switched = 1;
	}
#endif

#ifdef HAVE_LIBXXF86VM
	if(window_type == POPUP_WINDOW || window_type == VIDMODE_SCALED_WINDOW)
#else
	if(window_type == POPUP_WINDOW)
#endif
	{
		mask = CWEventMask | CWBackPixel | CWColormap |
			CWOverrideRedirect | CWSaveUnder | CWCursor;

		attr.event_mask = DEFAULT_EVENT_MASKS |
			KeyPressMask |
			KeyReleaseMask;

		if(this->bg_color == -1)
			this->bg_color = resources.get_bg_color();
		attr.background_pixel = top_level->get_color(bg_color);
		attr.colormap = top_level->cmap;
		if(top_level->is_hourglass)
			attr.cursor = top_level->get_cursor_struct(HOURGLASS_CURSOR);
		else
			attr.cursor = top_level->get_cursor_struct(ARROW_CURSOR);
		attr.override_redirect = True;
		attr.save_under = True;

		win = XCreateWindow(top_level->display,
			top_level->rootwin, this->x, this->y, this->w, this->h, 0,
			top_level->default_depth, InputOutput, top_level->vis, mask,
			&attr);
		top_level->add_popup(this);
	}

	if(window_type == SUB_WINDOW)
	{
		mask = CWEventMask | CWBackPixel | CWCursor;
		attr.event_mask = DEFAULT_EVENT_MASKS;
		attr.background_pixel = top_level->get_color(this->bg_color);
		if(top_level->is_hourglass)
			attr.cursor = top_level->get_cursor_struct(HOURGLASS_CURSOR);
		else
			attr.cursor = top_level->get_cursor_struct(ARROW_CURSOR);
		win = XCreateWindow(top_level->display,
			parent_window->win, this->x, this->y, this->w, this->h, 0,
			top_level->default_depth, InputOutput, top_level->vis, mask,
			&attr);
		init_window_shape();
		if(!hidden) XMapWindow(top_level->display, win);
	}

// Create pixmap for all windows
	pixmap = new BC_Pixmap(this, this->w, this->h);

// Set up options for main window
	if(window_type == MAIN_WINDOW)
	{
		if(get_resources()->bg_image && !bg_pixmap && bg_color < 0)
		{
			this->bg_pixmap = new BC_Pixmap(this,
				get_resources()->bg_image,
				PIXMAP_OPAQUE);
		}

		if(!hidden) show_window();

	}

	draw_background(0, 0, this->w, this->h);

	flash(-1, -1, -1, -1, 0);

// Set up options for popup window
#ifdef HAVE_LIBXXF86VM
	if(window_type == POPUP_WINDOW || window_type == VIDMODE_SCALED_WINDOW)
#else
	if(window_type == POPUP_WINDOW)
#endif
	{
		init_window_shape();
		if(!hidden) show_window();
	}
	get_resources()->create_window_lock->unlock();
	unlock_window();

	return 0;
}

Display* BC_WindowBase::init_display(const char *display_name)
{
	Display* display;

	if(display_name && display_name[0] == 0) display_name = NULL;
	if((display = XOpenDisplay(display_name)) == NULL) {
		printf("BC_WindowBase::init_display: cannot connect to X server %s\n",
			display_name);
		if(getenv("DISPLAY") == NULL) {
			printf(_("'DISPLAY' environment variable not set.\n"));
			exit(1);
		}
// Try again with default display.
		if((display = XOpenDisplay(0)) == NULL) {
			printf("BC_WindowBase::init_display: cannot connect to default X server.\n");
			exit(1);
		}
 	}

	static int xsynch = -1;
	if( xsynch < 0 ) {
		const char *cp = getenv("CIN_XSYNCH");
		xsynch = !cp ? 0 : atoi(cp);
	}
	if( xsynch > 0 )
		XSynchronize(display, True);

	return display;
}

Display* BC_WindowBase::get_display()
{
	return top_level->display;
}

int BC_WindowBase::get_screen()
{
	return top_level->screen;
}

int BC_WindowBase::run_window()
{
	done_set = done = 0;
	return_value = 0;


// Events may have been sent before run_window so can't initialize them here.

#ifdef SINGLE_THREAD
	set_repeat(get_resources()->tooltip_delay);
	BC_Display::display_global->new_window(this);

// If the first window created, run the display loop in this thread.
	if(BC_Display::display_global->is_first(this))
	{
		BC_Display::unlock_display();
		BC_Display::display_global->loop();
	}
	else
	{
		BC_Display::unlock_display();
		completion_lock->lock("BC_WindowBase::run_window");
	}

	BC_Display::lock_display("BC_WindowBase::run_window");
	BC_Display::display_global->delete_window(this);

	unset_all_repeaters();
	hide_tooltip();
	BC_Display::unlock_display();

#else // SINGLE_THREAD



// Start tooltips
	set_repeat(get_resources()->tooltip_delay);

// Start X server events
	event_thread = new BC_WindowEvents(this);
	event_thread->start();

// Release wait lock
	window_running = 1;
	init_lock->unlock();

// Handle common events
	while(!done)
	{
		dispatch_event(0);
	}

	unset_all_repeaters();
	hide_tooltip();
	delete event_thread;
	event_thread = 0;
	event_condition->reset();
	common_events.remove_all_objects();
	window_running = 0;
	done = 0;

#endif // SINGLE_THREAD

	return return_value;
}

int BC_WindowBase::get_key_masks(unsigned int key_state)
{
// printf("BC_WindowBase::get_key_masks %llx\n",
// event->xkey.state);
	ctrl_mask = (key_state & ControlMask) ? 1 : 0;  // ctrl key down
	shift_mask = (key_state & ShiftMask) ? 1 : 0;   // shift key down
	alt_mask = (key_state & Mod1Mask) ? 1 : 0;      // alt key down
	return 0;
}


void BC_WindowBase::add_keyboard_listener(int(BC_WindowBase::*handler)(BC_WindowBase *))
{
	BC_KeyboardHandlerLock set;
	BC_KeyboardHandler::listeners.append(new BC_KeyboardHandler(handler, this));
}

void BC_WindowBase::del_keyboard_listener(int(BC_WindowBase::*handler)(BC_WindowBase *))
{
	BC_KeyboardHandlerLock set;
	int i = BC_KeyboardHandler::listeners.size();
	while( --i >= 0 && BC_KeyboardHandler::listeners[i]->handler!=handler );
	if( i >= 0 ) BC_KeyboardHandler::listeners.remove_object_number(i);
}

int BC_KeyboardHandler::run_event(BC_WindowBase *wp)
{
 	int result = (win->*handler)(wp);
	return result;
}

int BC_KeyboardHandler::run_listeners(BC_WindowBase *wp)
{
	int result = 0;
	BC_KeyboardHandlerLock set;
	for( int i=0; !result && i<listeners.size(); ++i ) {
		BC_KeyboardHandler *listener = listeners[i];
		result = listener->run_event(wp);
	}
	return result;
}

void BC_KeyboardHandler::kill_grabs()
{
	BC_KeyboardHandlerLock set;
	for( int i=0; i<listeners.size(); ++i ) {
		BC_WindowBase *win = listeners[i]->win;
		if( win->get_window_type() != POPUP_WINDOW ) continue;
		((BC_Popup *)win)->ungrab_keyboard();
	}
}

void BC_ActiveBitmaps::reque(XEvent *event)
{
	XShmCompletionEvent *shm_ev = (XShmCompletionEvent *)event;
	ShmSeg shmseg = shm_ev->shmseg;
	Drawable drawable = shm_ev->drawable;
//printf("BC_BitmapImage::reque %08lx\n",shmseg);
	active_lock.lock("BC_BitmapImage::reque");
	BC_BitmapImage *bfr = first;
	while( bfr && bfr->get_shmseg() != shmseg ) bfr = bfr->next;
	if( bfr && bfr->drawable == drawable )
		remove_pointer(bfr);
	active_lock.unlock();
	if( !bfr ) {
// sadly, X reports two drawable completions and creates false reporting, so no boobytrap
//		printf("BC_BitmapImage::reque missed shmseg %08x, drawable %08x\n",
//			 (int)shmseg, (int)drawable);
		return;
	}
	if( bfr->drawable != drawable ) return;
	if( bfr->is_zombie() ) { --BC_Bitmap::zombies; delete bfr; return; }
	bfr->bitmap->reque(bfr);
}

void BC_ActiveBitmaps::insert(BC_BitmapImage *bfr, Drawable pixmap)
{
	active_lock.lock("BC_BitmapImage::insert");
	bfr->drawable = pixmap;
	append(bfr);
	active_lock.unlock();
}

void BC_ActiveBitmaps::remove_buffers(BC_WindowBase *wdw)
{
	active_lock.lock("BC_ActiveBitmaps::remove");
	for( BC_BitmapImage *nxt=0, *bfr=first; bfr; bfr=nxt ) {
		nxt = bfr->next;
		if( bfr->is_zombie() ) { --BC_Bitmap::zombies; delete bfr; continue; }
		if( bfr->bitmap->parent_window == wdw ) remove_pointer(bfr);
	}
	active_lock.unlock();
}

BC_ActiveBitmaps::BC_ActiveBitmaps()
{
}

BC_ActiveBitmaps::~BC_ActiveBitmaps()
{
}



int BC_WindowBase::keysym_lookup(XEvent *event)
{
	for( int i = 0; i < KEYPRESSLEN; ++i ) keys_return[i] = 0;
	for( int i = 0; i < 4; ++i ) wkey_string[i] = 0;

	if( event->xany.send_event && !event->xany.serial ) {
		keysym = (KeySym) event->xkey.keycode;
		keys_return[0] = keysym;
		return 0;
	}
	wkey_string_length = 0;

	if( input_context ) {
		wkey_string_length = XwcLookupString(input_context,
			(XKeyEvent*)event, wkey_string, 4, &keysym, 0);
//printf("keysym_lookup 1 %d %d %lx %x %x %x %x\n", wkey_string_length, keysym,
//  wkey_string[0], wkey_string[1], wkey_string[2], wkey_string[3]);

		Status stat;
		int ret = Xutf8LookupString(input_context, (XKeyEvent*)event,
				keys_return, KEYPRESSLEN, &keysym, &stat);
//printf("keysym_lookup 2 %d %d %lx %x %x\n", ret, stat, keysym, keys_return[0], keys_return[1]);
		if( stat == XLookupBoth ) return ret;
		if( stat == XLookupKeySym ) return 0;
	}
	int ret = XLookupString((XKeyEvent*)event, keys_return, KEYPRESSLEN, &keysym, 0);
	wkey_string_length = ret;
	for( int i=0; i<ret; ++i ) wkey_string[i] = keys_return[i];
	return ret;
}

pthread_t locking_task = (pthread_t)-1L;
int locking_event = -1;
int locking_message = -1;

int BC_WindowBase::dispatch_event(XEvent *event)
{
	Window tempwin;
	int result;
	XClientMessageEvent *ptr;
	int cancel_resize, cancel_translation;
	volatile static int debug = 0;

	key_pressed = 0;

#ifndef SINGLE_THREAD
// If an event is waiting get it, otherwise
// wait for next event only if there are no compressed events.
	if(get_event_count() ||
		(!motion_events && !resize_events && !translation_events))
	{
		event = get_event();
// Lock out window deletions
		lock_window("BC_WindowBase::dispatch_event 1");
locking_event = event->type;
locking_task = pthread_self();
locking_message = event->xclient.message_type;
	}
	else
// Handle compressed events
	{
		lock_window("BC_WindowBase::dispatch_event 2");
		if(resize_events)
			dispatch_resize_event(last_resize_w, last_resize_h);
		if(motion_events)
			dispatch_motion_event();
		if(translation_events)
			dispatch_translation_event();

		unlock_window();
		return 0;
	}

#endif




if( debug && event->type != ClientMessage ) {
 static const char *event_names[] = {
  "Reply", "Error", "KeyPress", "KeyRelease", "ButtonPress", "ButtonRelease", "MotionNotify",
  "EnterNotify", "LeaveNotify", "FocusIn", "FocusOut", "KeymapNotify", "Expose", "GraphicsExpose",
  "NoExpose", "VisibilityNotify", "CreateNotify", "DestroyNotify", "UnmapNotify", "MapNotify",
  "MapRequest", "ReparentNotify", "ConfigureNotify", "ConfigureRequest", "GravityNotify",
  "ResizeRequest", "CirculateNotify", "CirculateRequest", "PropertyNotify", "SelectionClear",
  "SelectionRequest", "SelectionNotify", "ColormapNotify", "ClientMessage", "MappingNotify",
  "GenericEvent", "LASTEvent",
 };
 const int nevents = sizeof(event_names)/sizeof(event_names[0]);

 printf("BC_WindowBase::dispatch_event %d %s %p %d (%s)\n", __LINE__,
  title, event, event->type, event->type>=0 && event->type<nevents ?
   event_names[event->type] : "Unknown");
}

	if( active_grab ) {
		unlock_window();
		active_grab->lock_window("BC_WindowBase::dispatch_event 3");
		result = active_grab->grab_event(event);
		active_grab->unlock_window();
		if( result ) return result;
		lock_window("BC_WindowBase::dispatch_event 4");
	}

	switch(event->type) {
	case ClientMessage:
// Clear the resize buffer
		if(resize_events) dispatch_resize_event(last_resize_w, last_resize_h);
// Clear the motion buffer since this can clear the window
		if(motion_events)
		{
			dispatch_motion_event();
		}

		ptr = (XClientMessageEvent*)event;


		if(ptr->message_type == ProtoXAtom &&
			(Atom)ptr->data.l[0] == DelWinXAtom)
		{
			close_event();
		}
		else
		if(ptr->message_type == RepeaterXAtom)
		{
			dispatch_repeat_event(ptr->data.l[0]);
// Make sure the repeater still exists.
// 				for(int i = 0; i < repeaters.total; i++)
// 				{
// 					if(repeaters.values[i]->repeat_id == ptr->data.l[0])
// 					{
// 						dispatch_repeat_event_master(ptr->data.l[0]);
// 						break;
// 					}
// 				}
		}
		else
		if(ptr->message_type == SetDoneXAtom)
		{
			done = 1;
			} else
			{ // We currently use X marshalling for xatom events, we can switch to something else later
				receive_custom_xatoms((xatom_event *)ptr);
		}
		break;

	case FocusIn:
		has_focus = 1;
		dispatch_focus_in();
		break;

	case FocusOut:
		has_focus = 0;
		dispatch_focus_out();
		break;

// Maximized
	case MapNotify:
		break;

// Minimized
	case UnmapNotify:
		break;

	case ButtonPress:
		if(motion_events)
		{
			dispatch_motion_event();
		}
		get_key_masks(event->xbutton.state);
		cursor_x = event->xbutton.x;
		cursor_y = event->xbutton.y;
		button_number = event->xbutton.button;

//printf("BC_WindowBase::dispatch_event %d %d\n", __LINE__, button_number);
		event_win = event->xany.window;
		if (button_number < 6) {
			if(button_number < 4)
				button_down = 1;
			button_pressed = event->xbutton.button;
			button_time1 = button_time2;
			button_time2 = button_time3;
			button_time3 = event->xbutton.time;
			drag_x = cursor_x;
			drag_y = cursor_y;
			drag_win = event_win;
			drag_x1 = cursor_x - get_resources()->drag_radius;
			drag_x2 = cursor_x + get_resources()->drag_radius;
			drag_y1 = cursor_y - get_resources()->drag_radius;
			drag_y2 = cursor_y + get_resources()->drag_radius;

			if((long)(button_time3 - button_time1) < resources.double_click * 2)
			{
				triple_click = 1;
				button_time3 = button_time2 = button_time1 = 0;
			}
			if((long)(button_time3 - button_time2) < resources.double_click)
			{
				double_click = 1;
//				button_time3 = button_time2 = button_time1 = 0;
			}
			else
			{
				triple_click = 0;
				double_click = 0;
			}

			dispatch_button_press();
		}
		break;

	case ButtonRelease:
		if(motion_events)
		{
			dispatch_motion_event();
		}
		get_key_masks(event->xbutton.state);
		button_number = event->xbutton.button;
		event_win = event->xany.window;
		if (button_number < 6)
		{
			if(button_number < 4)
				button_down = 0;
//printf("BC_WindowBase::dispatch_event %d %d\n", __LINE__, button_number);

			dispatch_button_release();
		}
		break;

	case Expose:
		event_win = event->xany.window;
		dispatch_expose_event();
		break;

	case MotionNotify:
		get_key_masks(event->xmotion.state);
// Dispatch previous motion event if this is a subsequent motion from a different window
		if(motion_events && last_motion_win != event->xany.window)
		{
			dispatch_motion_event();
		}

// Buffer the current motion
		motion_events = 1;
		last_motion_state = event->xmotion.state;
		last_motion_x = event->xmotion.x;
		last_motion_y = event->xmotion.y;
		last_motion_win = event->xany.window;
		break;

	case ConfigureNotify:
// printf("BC_WindowBase::dispatch_event %d win=%p this->win=%p\n",
// __LINE__,
// event->xany.window,
// win);
// dump_windows();
		XTranslateCoordinates(top_level->display,
			top_level->win,
			top_level->rootwin,
			0,
			0,
			&last_translate_x,
			&last_translate_y,
			&tempwin);
		last_resize_w = event->xconfigure.width;
		last_resize_h = event->xconfigure.height;

		cancel_resize = 0;
		cancel_translation = 0;

// Resize history prevents responses to recursive resize requests
		for(int i = 0; i < resize_history.total && !cancel_resize; i++)
		{
			if(resize_history.values[i]->w == last_resize_w &&
				resize_history.values[i]->h == last_resize_h)
			{
				delete resize_history.values[i];
				resize_history.remove_number(i);
				cancel_resize = 1;
			}
		}

		if(last_resize_w == w && last_resize_h == h)
			cancel_resize = 1;

		if(!cancel_resize)
		{
			resize_events = 1;
		}

		if((last_translate_x == x && last_translate_y == y))
			cancel_translation = 1;

		if(!cancel_translation)
		{
			translation_events = 1;
		}

		translation_count++;
		break;

	case KeyPress:
		get_key_masks(event->xkey.state);
		keys_return[0] = 0;  keysym = -1;
		if(XFilterEvent(event, win)) {
			break;
		}
		if( keysym_lookup(event) < 0 ) {
			printf("keysym %x\n", (uint32_t)keysym);
			break;
		}

//printf("BC_WindowBase::dispatch_event %d keysym=0x%x\n",
//__LINE__,
//keysym);

// block out control keys
		if(keysym > 0xffe0 && keysym < 0xffff) break;
// block out Alt_GR key
		if(keysym == 0xfe03) break;

		if(test_keypress)
			 printf("BC_WindowBase::dispatch_event %x\n", (uint32_t)keysym);

#ifdef X_HAVE_UTF8_STRING
//It's Ascii or UTF8?
//		if (keysym != 0xffff &&	(keys_return[0] & 0xff) >= 0x7f )
//printf("BC_WindowBase::dispatch_event %d %02x%02x\n", __LINE__, keys_return[0], keys_return[1]);

		if( ((keys_return[1] & 0xff) > 0x80) &&
		    ((keys_return[0] & 0xff) > 0xC0) ) {
//printf("BC_WindowBase::dispatch_event %d\n", __LINE__);
 			key_pressed = keysym & 0xff;
 		}
		else {
#endif
// shuttle speed codes
		if( keysym >= SKEY_MIN && keysym <= SKEY_MAX ) {
			key_pressed = keysym;
		}
		else switch( keysym ) {
// block out extra keys
		case XK_Alt_L:
		case XK_Alt_R:
		case XK_Shift_L:
		case XK_Shift_R:
		case XK_Control_L:
		case XK_Control_R:
			key_pressed = 0;
			break;

// Translate key codes
		case XK_Return:		key_pressed = RETURN;	break;
		case XK_Up:		key_pressed = UP;	break;
		case XK_Down:		key_pressed = DOWN;	break;
		case XK_Left:		key_pressed = LEFT;	break;
		case XK_Right:		key_pressed = RIGHT;	break;
		case XK_Next:		key_pressed = PGDN;	break;
		case XK_Prior:		key_pressed = PGUP;	break;
		case XK_BackSpace:	key_pressed = BACKSPACE; break;
		case XK_Escape:		key_pressed = ESC;	break;
		case XK_Tab:
			if(shift_down())
				key_pressed = LEFTTAB;
			else
				key_pressed = TAB;
			break;
		case XK_ISO_Left_Tab:	key_pressed = LEFTTAB;	break;
		case XK_underscore:	key_pressed = '_';	break;
		case XK_asciitilde:	key_pressed = '~';	break;
		case XK_Delete:		key_pressed = DELETE;	break;
		case XK_Home:		key_pressed = HOME;	break;
		case XK_End:		key_pressed = END;	break;

// number pad
		case XK_KP_Enter:	key_pressed = KPENTER;	break;
		case XK_KP_Add:		key_pressed = KPPLUS;	break;
		case XK_KP_Subtract:	key_pressed = KPMINUS;	break;
		case XK_KP_Multiply:	key_pressed = KPSTAR;	break;
		case XK_KP_Divide:	key_pressed = KPSLASH;	break;
		case XK_KP_1:
		case XK_KP_End:		key_pressed = KP1;	break;
		case XK_KP_2:
		case XK_KP_Down:	key_pressed = KP2;	break;
		case XK_KP_3:
		case XK_KP_Page_Down:	key_pressed = KP3;	break;
		case XK_KP_4:
		case XK_KP_Left:	key_pressed = KP4;	break;
		case XK_KP_5:
		case XK_KP_Begin:	key_pressed = KP5;	break;
		case XK_KP_6:
		case XK_KP_Right:	key_pressed = KP6;	break;
		case XK_KP_7:
		case XK_KP_Home:	key_pressed = KP7;	break;
		case XK_KP_8:
		case XK_KP_Up:		key_pressed = KP8;	break;
		case XK_KP_9:
		case XK_KP_Page_Up:	key_pressed = KP9;	break;
		case XK_KP_0:
		case XK_KP_Insert:	key_pressed = KPINS;	break;
		case XK_KP_Decimal:
		case XK_KP_Delete:	key_pressed = KPDEL;	break;

		case XK_F1:		key_pressed = KEY_F1;	break;
		case XK_F2:		key_pressed = KEY_F2;	break;
		case XK_F3:		key_pressed = KEY_F3;	break;
		case XK_F4:		key_pressed = KEY_F4;	break;
		case XK_F5:		key_pressed = KEY_F5;	break;
		case XK_F6:		key_pressed = KEY_F6;	break;
		case XK_F7:		key_pressed = KEY_F7;	break;
		case XK_F8:		key_pressed = KEY_F8;	break;
		case XK_F9:		key_pressed = KEY_F9;	break;
		case XK_F10:		key_pressed = KEY_F10;	break;
		case XK_F11:		key_pressed = KEY_F11;	break;
		case XK_F12:		key_pressed = KEY_F12;	break;

		case XK_Menu:		key_pressed = KPMENU;	break;  /* menu */
// remote control
// above	case XK_KP_Enter:	key_pressed = KPENTER;	break;  /* check */
		case XF86XK_MenuKB:	key_pressed = KPMENU;	break;  /* menu */
// intercepted	case XF86XK_PowerDown: key_pressed = KPPOWER;	break;  /* Power */
		case XF86XK_Launch1:	key_pressed = KPTV;	break;  /* TV */
		case XF86XK_Launch2:	key_pressed = KPDVD;	break;  /* DVD */
// intercepted	case XF86XK_WWW:	key_pressed = KPWWEB;	break;  /* WEB */
		case XF86XK_Launch3:	key_pressed = KPBOOK;	break;  /* book */
		case XF86XK_Launch4:	key_pressed = KPHAND;	break;  /* hand */
		case XF86XK_Reply:	key_pressed = KPTMR;	break;  /* timer */
		case SunXK_Front:	key_pressed = KPMAXW;	break;  /* max */
// above	case XK_Left:		key_pressed = LEFT;	break;  /* left */
// above	case XK_Right:		key_pressed = RIGHT;	break;  /* right */
// above	case XK_Down:		key_pressed = DOWN;	break;  /* down */
// above	case XK_Up:		key_pressed = UP;	break;  /* up */
// above	case XK_SPACE:		key_pressed = KPSPACE;	break;  /* ok */
// intercepted	case XF86XK_AudioRaiseVolume: key_pressed = KPVOLU;	break;  /* VOL + */
// intercepted	case XF86XK_AudioMute: key_pressed = KPMUTE;	break;  /* MUTE */
// intercepted	case XF86XK_AudioLowerVolume: key_pressed = KPVOLD;	break;  /* VOL - */
		case XF86XK_ScrollUp:	key_pressed = KPCHUP;	break;  /* CH + */
		case XF86XK_ScrollDown:	key_pressed = KPCHDN;	break;  /* CH - */
		case XF86XK_AudioRecord: key_pressed = KPRECD;	break;  /* ( o) red */
		case XF86XK_Forward:	key_pressed = KPPLAY;	break;  /* ( >) */
		case XK_Redo:		key_pressed = KPFWRD;	break;  /* (>>) */
		case XF86XK_Back:	key_pressed = KPBACK;	break;  /* (<<) */
		case XK_Cancel:		key_pressed = KPSTOP;	break;  /* ([]) */
		case XK_Pause:		key_pressed = KPAUSE;	break;  /* ('') */

		default:
			key_pressed = keysym & 0xff;
#ifdef X_HAVE_UTF8_STRING
//printf("BC_WindowBase::dispatch_event %d\n", __LINE__);
			keys_return[1] = 0;
#endif
			break;
		}
#ifdef X_HAVE_UTF8_STRING
		}
		key_pressed_utf8 = keys_return;
#endif


		result = 0;
		if( top_level == this )
			result = BC_KeyboardHandler::run_listeners(this);

//printf("BC_WindowBase::dispatch_event %d %d %x\n", shift_down(), alt_down(), key_pressed);
		if( !result )
			result = dispatch_keypress_event();
// Handle some default keypresses
		if(!result)
		{
			if(key_pressed == 'w' ||
				key_pressed == 'W')
			{
				close_event();
			}
		}
		break;

	case KeyRelease:
		XLookupString((XKeyEvent*)event, keys_return, 1, &keysym, 0);
		dispatch_keyrelease_event();
// printf("BC_WindowBase::dispatch_event KeyRelease keysym=0x%x keystate=0x%lld\n",
// keysym, event->xkey.state);
		break;

	case LeaveNotify:
		if( event->xcrossing.mode != NotifyNormal ) break;
		cursor_entered = 0;
		event_win = event->xany.window;
		dispatch_cursor_leave();
		break;

	case EnterNotify:
		if( event->xcrossing.mode != NotifyNormal ) break;

		if( !cursor_entered ) {
			for( int i=0; i<popups.size(); ++i ) {  // popups always take focus
				if( popups[i]->win == event->xcrossing.window )
				cursor_entered = 1;
			}
			if( !cursor_entered && get_resources()->grab_input_focus &&
			    !event->xcrossing.focus && event->xcrossing.window == win ) {
				cursor_entered = 1;
			}
			if( cursor_entered )
				focus();
		}
		event_win = event->xany.window;
		cursor_x = event->xcrossing.x;
		cursor_y = event->xcrossing.y;
		dispatch_cursor_enter();
		break;

	default:
		break;
	}
//printf("100 %s %p %d\n", title, event, event->type);
//if(event->type != ClientMessage) dump();

#ifndef SINGLE_THREAD
	unlock_window();
	if(event) {
		if( resend_event_window ) {
			resend_event_window->put_event(event);
			resend_event_window = 0;
		}
		else
			delete event;
	}
#else
//	if(done) completion_lock->unlock();
#endif

if(debug) printf("BC_WindowBase::dispatch_event this=%p %d\n", this, __LINE__);
	return 0;
}

int BC_WindowBase::dispatch_expose_event()
{
	int result = 0;
	for(int i = 0; i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_expose_event();
	}

// Propagate to user
	if(!result) expose_event();
	return result;
}

int BC_WindowBase::dispatch_resize_event(int w, int h)
{
// Can't store new w and h until the event is handles
// because bcfilebox depends on the old w and h to
// reposition widgets.
	if( window_type == MAIN_WINDOW ) {
		flash_enabled = 0;
		resize_events = 0;

		delete pixmap;
		pixmap = new BC_Pixmap(this, w, h);
		clear_box(0, 0, w, h);
	}

// Propagate to subwindows
	for(int i = 0; i < subwindows->total; i++) {
		subwindows->values[i]->dispatch_resize_event(w, h);
	}

// Propagate to user
	resize_event(w, h);

	if( window_type == MAIN_WINDOW ) {
		this->w = w;
		this->h = h;
		dispatch_flash();
		flush();
	}
	return 0;
}

int BC_WindowBase::dispatch_flash()
{
	flash_enabled = 1;
	for(int i = 0; i < subwindows->total; i++)
		subwindows->values[i]->dispatch_flash();
	return flash(0);
}

int BC_WindowBase::dispatch_translation_event()
{
	translation_events = 0;
	if(window_type == MAIN_WINDOW)
	{
		prev_x = x;
		prev_y = y;
		x = last_translate_x;
		y = last_translate_y;
// Correct for window manager offsets
		x -= x_correction;
		y -= y_correction;
	}

	for(int i = 0; i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_translation_event();
	}

	translation_event();
	return 0;
}

int BC_WindowBase::dispatch_motion_event()
{
	int result = 0;
	unhide_cursor();

	if(top_level == this)
	{
		motion_events = 0;
		event_win = last_motion_win;
		get_key_masks(last_motion_state);

// Test for grab
		if(get_button_down() && !active_menubar && !active_popup_menu)
		{
			if(!result)
			{
				cursor_x = last_motion_x;
				cursor_y = last_motion_y;
				result = dispatch_drag_motion();
			}

			if(!result &&
				(last_motion_x < drag_x1 || last_motion_x >= drag_x2 ||
				last_motion_y < drag_y1 || last_motion_y >= drag_y2))
			{
				cursor_x = drag_x;
				cursor_y = drag_y;

				result = dispatch_drag_start();
			}
		}

		cursor_x = last_motion_x;
		cursor_y = last_motion_y;

// printf("BC_WindowBase::dispatch_motion_event %d %p %p %p\n",
// __LINE__,
// active_menubar,
// active_popup_menu,
// active_subwindow);

		if(active_menubar && !result) result = active_menubar->dispatch_motion_event();
		if(active_popup_menu && !result) result = active_popup_menu->dispatch_motion_event();
		if(active_subwindow && !result) result = active_subwindow->dispatch_motion_event();
	}

// Dispatch in stacking order
	for(int i = subwindows->size() - 1; i >= 0 && !result; i--)
	{
		result = subwindows->values[i]->dispatch_motion_event();
	}

	if(!result) result = cursor_motion_event();    // give to user
	return result;
}

int BC_WindowBase::dispatch_keypress_event()
{
	int result = 0;
	if(top_level == this)
	{
		if(active_subwindow) result = active_subwindow->dispatch_keypress_event();
	}

	for(int i = 0; i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_keypress_event();
	}

	if(!result) result = keypress_event();

	return result;
}

int BC_WindowBase::dispatch_keyrelease_event()
{
	int result = 0;
	if(top_level == this)
	{
		if(active_subwindow) result = active_subwindow->dispatch_keyrelease_event();
	}

	for(int i = 0; i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_keyrelease_event();
	}

	if(!result) result = keyrelease_event();

	return result;
}

int BC_WindowBase::dispatch_focus_in()
{
	for(int i = 0; i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_focus_in();
	}

	focus_in_event();

	return 0;
}

int BC_WindowBase::dispatch_focus_out()
{
	for(int i = 0; i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_focus_out();
	}

	focus_out_event();

	return 0;
}

int BC_WindowBase::get_has_focus()
{
	return top_level->has_focus;
}

int BC_WindowBase::get_deleting()
{
	if(is_deleting) return 1;
	if(parent_window && parent_window->get_deleting()) return 1;
	return 0;
}

int BC_WindowBase::dispatch_button_press()
{
	int result = 0;


	if(top_level == this)
	{
		if(active_menubar) result = active_menubar->dispatch_button_press();
		if(active_popup_menu && !result) result = active_popup_menu->dispatch_button_press();
		if(active_subwindow && !result) result = active_subwindow->dispatch_button_press();
	}

	for(int i = 0; i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_button_press();
	}

	if(!result) result = button_press_event();


	return result;
}

int BC_WindowBase::dispatch_button_release()
{
	int result = 0;
	if(top_level == this)
	{
		if(active_menubar) result = active_menubar->dispatch_button_release();
		if(active_popup_menu && !result) result = active_popup_menu->dispatch_button_release();
		if(active_subwindow && !result) result = active_subwindow->dispatch_button_release();
		if(!result && button_number != 4 && button_number != 5)
			result = dispatch_drag_stop();
	}

	for(int i = 0; i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_button_release();
	}

	if(!result)
	{
		result = button_release_event();
	}

	return result;
}


int BC_WindowBase::dispatch_repeat_event(int64_t duration)
{

// all repeat event handlers get called and decide based on activity and duration
// whether to respond
	for(int i = 0; i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_repeat_event(duration);
	}


	repeat_event(duration);



// Unlock next repeat signal
	if(window_type == MAIN_WINDOW)
	{
#ifdef SINGLE_THREAD
		BC_Display::display_global->unlock_repeaters(duration);
#else
		for(int i = 0; i < repeaters.total; i++)
		{
			if(repeaters.values[i]->delay == duration)
			{
				repeaters.values[i]->repeat_lock->unlock();
			}
		}
#endif
	}
	return 0;
}

void BC_WindowBase::unhide_cursor()
{
	if(is_transparent)
	{
		is_transparent = 0;
		if(top_level->is_hourglass)
			set_cursor(HOURGLASS_CURSOR, 1, 0);
		else
			set_cursor(current_cursor, 1, 0);
	}
	cursor_timer->update();
}


void BC_WindowBase::update_video_cursor()
{
	if(video_on && !is_transparent)
	{
		if(cursor_timer->get_difference() > VIDEO_CURSOR_TIMEOUT && !is_transparent)
		{
			is_transparent = 1;
			set_cursor(TRANSPARENT_CURSOR, 1, 1);
			cursor_timer->update();
		}
	}
	else
	{
		cursor_timer->update();
	}
}


int BC_WindowBase::dispatch_cursor_leave()
{
	unhide_cursor();

	for(int i = 0; i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_cursor_leave();
	}

	cursor_leave_event();
	return 0;
}

int BC_WindowBase::dispatch_cursor_enter()
{
	int result = 0;

	unhide_cursor();

	if(active_menubar) result = active_menubar->dispatch_cursor_enter();
	if(!result && active_popup_menu) result = active_popup_menu->dispatch_cursor_enter();
	if(!result && active_subwindow) result = active_subwindow->dispatch_cursor_enter();

	for(int i = 0; !result && i < subwindows->total; i++)
	{
		result = subwindows->values[i]->dispatch_cursor_enter();
	}

	if(!result) result = cursor_enter_event();
	return result;
}

int BC_WindowBase::cursor_enter_event()
{
	return 0;
}

int BC_WindowBase::cursor_leave_event()
{
	return 0;
}

int BC_WindowBase::close_event()
{
	set_done(1);
	return 1;
}

int BC_WindowBase::dispatch_drag_start()
{
	int result = 0;
	if(active_menubar) result = active_menubar->dispatch_drag_start();
	if(!result && active_popup_menu) result = active_popup_menu->dispatch_drag_start();
	if(!result && active_subwindow) result = active_subwindow->dispatch_drag_start();

	for(int i = 0; i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_drag_start();
	}

	if(!result) result = is_dragging = drag_start_event();
	return result;
}

int BC_WindowBase::dispatch_drag_stop()
{
	int result = 0;

	for(int i = 0; i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_drag_stop();
	}

	if(is_dragging && !result)
	{
		drag_stop_event();
		is_dragging = 0;
		result = 1;
	}

	return result;
}

int BC_WindowBase::dispatch_drag_motion()
{
	int result = 0;
	for(int i = 0; i < subwindows->total && !result; i++)
	{
		result = subwindows->values[i]->dispatch_drag_motion();
	}

	if(is_dragging && !result)
	{
		drag_motion_event();
		result = 1;
	}

	return result;
}


int BC_WindowBase::show_tooltip(const char *text, int x, int y, int w, int h)
{
// default text
	int forced = !text ? force_tooltip : 1;
	if( !text ) text = tooltip_text;
	if( !text || (!forced && !get_resources()->tooltips_enabled) ) {
		top_level->hide_tooltip();
		return 1;
	}
// default w,h
	if(w < 0) w = get_text_width(MEDIUMFONT, text)  + TOOLTIP_MARGIN * 2;
	if(h < 0) h = get_text_height(MEDIUMFONT, text) + TOOLTIP_MARGIN * 2;
// default x,y (win relative)
	if( x < 0 ) x = get_w();
	if( y < 0 ) y = get_h();
	int wx, wy;
	get_root_coordinates(x, y, &wx, &wy);
// keep the tip inside the window/display
	int x0 = top_level->get_x(), x1 = x0 + top_level->get_w();
	int x2 = top_level->get_screen_x(0, -1) + top_level->get_screen_w(0, -1);
	if( x1 > x2 ) x1 = x2;
	if( wx < x0 ) wx = x0;
	if( wx >= (x1-=w) ) wx = x1;
	int y0 = top_level->get_y(), y1 = y0 + top_level->get_h();
	int y2 = top_level->get_root_h(0);
	if( y1 > y2 ) y1 = y2;
	if( wy < y0 ) wy = y0;
	if( wy >= (y1-=h) ) wy = y1;
// avoid tip under cursor (flickers)
	int abs_x, abs_y;
	get_abs_cursor(abs_x,abs_y, 0);
	if( wx < abs_x && abs_x < wx+w && wy < abs_y && abs_y < wy+h ) {
		if( wx-abs_x < wy-abs_y )
			wx = abs_x+1;
		else
			wy = abs_y+1;
	}
	if( !tooltip_on ) {
		tooltip_on = 1;
		tooltip_popup = new BC_Popup(top_level, wx, wy, w, h,
				get_resources()->tooltip_bg_color);
	}
	else
		tooltip_popup->reposition_window(wx, wy, w, h);

	draw_tooltip(text);
	tooltip_popup->flash();
	tooltip_popup->flush();
	return 0;
}

int BC_WindowBase::hide_tooltip()
{
	if(subwindows)
		for(int i = 0; i < subwindows->total; i++)
		{
			subwindows->values[i]->hide_tooltip();
		}

	if(tooltip_on)
	{
		tooltip_on = 0;
		delete tooltip_popup;
		tooltip_popup = 0;
	}
	return 0;
}

const char *BC_WindowBase::get_tooltip()
{
	return tooltip_text;
}

int BC_WindowBase::set_tooltip(const char *text)
{
	tooltip_text = text;

// Update existing tooltip if it is visible
	if(tooltip_on)
	{
		draw_tooltip();
		tooltip_popup->flash();
	}
	return 0;
}
// signal the event handler to repeat
int BC_WindowBase::set_repeat(int64_t duration)
{
	if(duration <= 0)
	{
		printf("BC_WindowBase::set_repeat duration=%jd\n", duration);
		return 0;
	}
	if(window_type != MAIN_WINDOW) return top_level->set_repeat(duration);

#ifdef SINGLE_THREAD
	BC_Display::display_global->set_repeat(this, duration);
#else
// test repeater database for duplicates
	for(int i = 0; i < repeaters.total; i++)
	{
// Already exists
		if(repeaters.values[i]->delay == duration)
		{
			repeaters.values[i]->start_repeating(this);
			return 0;
		}
	}

	BC_Repeater *repeater = new BC_Repeater(this, duration);
	repeater->initialize();
	repeaters.append(repeater);
	repeater->start_repeating();
#endif
	return 0;
}

int BC_WindowBase::unset_repeat(int64_t duration)
{
	if(window_type != MAIN_WINDOW) return top_level->unset_repeat(duration);

#ifdef SINGLE_THREAD
	BC_Display::display_global->unset_repeat(this, duration);
#else
	for(int i = 0; i < repeaters.total; i++)
	{
		if(repeaters.values[i]->delay == duration)
		{
			repeaters.values[i]->stop_repeating();
		}
	}
#endif
	return 0;
}


int BC_WindowBase::unset_all_repeaters()
{
#ifdef SINGLE_THREAD
	BC_Display::display_global->unset_all_repeaters(this);
#else
	for(int i = 0; i < repeaters.total; i++)
	{
		repeaters.values[i]->stop_repeating();
	}
	repeaters.remove_all_objects();
#endif
	return 0;
}

// long BC_WindowBase::get_repeat_id()
// {
// 	return top_level->next_repeat_id++;
// }

XEvent *BC_WindowBase::new_xevent()
{
	XEvent *event = new XEvent;
	memset(event, 0, sizeof(*event));
	return event;
}

#ifndef SINGLE_THREAD
int BC_WindowBase::arm_repeat(int64_t duration)
{
	XEvent *event = new_xevent();
	XClientMessageEvent *ptr = (XClientMessageEvent*)event;
	ptr->type = ClientMessage;
	ptr->message_type = RepeaterXAtom;
	ptr->format = 32;
	ptr->data.l[0] = duration;

// Couldn't use XSendEvent since it locked up randomly.
	put_event(event);
	return 0;
}
#endif

int BC_WindowBase::receive_custom_xatoms(xatom_event *event)
{
	return 0;
}

int BC_WindowBase::send_custom_xatom(xatom_event *event)
{
#ifndef SINGLE_THREAD
	XEvent *myevent = new_xevent();
	XClientMessageEvent *ptr = (XClientMessageEvent*)myevent;
	ptr->type = ClientMessage;
	ptr->message_type = event->message_type;
	ptr->format = event->format;
	ptr->data.l[0] = event->data.l[0];
	ptr->data.l[1] = event->data.l[1];
	ptr->data.l[2] = event->data.l[2];
	ptr->data.l[3] = event->data.l[3];
	ptr->data.l[4] = event->data.l[4];

	put_event(myevent);
#endif
	return 0;
}



Atom BC_WindowBase::create_xatom(const char *atom_name)
{
	return XInternAtom(display, atom_name, False);
}

int BC_WindowBase::get_atoms()
{
	SetDoneXAtom =  XInternAtom(display, "BC_REPEAT_EVENT", False);
	RepeaterXAtom = XInternAtom(display, "BC_CLOSE_EVENT", False);
	DestroyAtom =   XInternAtom(display, "BC_DESTROY_WINDOW", False);
	DelWinXAtom =   XInternAtom(display, "WM_DELETE_WINDOW", False);
	if( (ProtoXAtom = XInternAtom(display, "WM_PROTOCOLS", False)) != 0 )
		XChangeProperty(display, win, ProtoXAtom, XA_ATOM, 32,
			PropModeReplace, (unsigned char *)&DelWinXAtom, True);
	return 0;

}


void BC_WindowBase::init_cursors()
{
	arrow_cursor = XCreateFontCursor(display, XC_top_left_arrow);
	cross_cursor = XCreateFontCursor(display, XC_crosshair);
	ibeam_cursor = XCreateFontCursor(display, XC_xterm);
	vseparate_cursor = XCreateFontCursor(display, XC_sb_v_double_arrow);
	hseparate_cursor = XCreateFontCursor(display, XC_sb_h_double_arrow);
	move_cursor = XCreateFontCursor(display, XC_fleur);
	left_cursor = XCreateFontCursor(display, XC_sb_left_arrow);
	right_cursor = XCreateFontCursor(display, XC_sb_right_arrow);
	upright_arrow_cursor = XCreateFontCursor(display, XC_arrow);
	upleft_resize_cursor = XCreateFontCursor(display, XC_top_left_corner);
	upright_resize_cursor = XCreateFontCursor(display, XC_top_right_corner);
	downleft_resize_cursor = XCreateFontCursor(display, XC_bottom_left_corner);
	downright_resize_cursor = XCreateFontCursor(display, XC_bottom_right_corner);
	hourglass_cursor = XCreateFontCursor(display, XC_watch);
	grabbed_cursor = create_grab_cursor();

	static char cursor_data[] = { 0,0,0,0, 0,0,0,0 };
	Colormap colormap = DefaultColormap(display, screen);
	Pixmap pixmap_bottom = XCreateBitmapFromData(display,
		rootwin, cursor_data, 8, 8);
	XColor black, dummy;
	XAllocNamedColor(display, colormap, "black", &black, &dummy);
	transparent_cursor = XCreatePixmapCursor(display,
		pixmap_bottom, pixmap_bottom, &black, &black, 0, 0);
//	XDefineCursor(display, win, transparent_cursor);
	XFreePixmap(display, pixmap_bottom);
}

int BC_WindowBase::evaluate_color_model(int client_byte_order, int server_byte_order, int depth)
{
	int color_model = BC_TRANSPARENCY;
	switch(depth)
	{
		case 8:
			color_model = BC_RGB8;
			break;
		case 16:
			color_model = (server_byte_order == client_byte_order) ? BC_RGB565 : BC_BGR565;
			break;
		case 24:
			color_model = server_byte_order ? BC_BGR888 : BC_RGB888;
			break;
		case 32:
			color_model = server_byte_order ? BC_BGR8888 : BC_ARGB8888;
			break;
	}
	return color_model;
}

int BC_WindowBase::init_colors()
{
	total_colors = 0;
	current_color_value = current_color_pixel = 0;

// Get the real depth
	char *data = 0;
	XImage *ximage;
	ximage = XCreateImage(top_level->display,
					top_level->vis,
					top_level->default_depth,
					ZPixmap,
					0,
					data,
					16,
					16,
					8,
					0);
	bits_per_pixel = ximage->bits_per_pixel;
	XDestroyImage(ximage);

	color_model = evaluate_color_model(client_byte_order,
		server_byte_order,
		bits_per_pixel);
// Get the color model
	switch(color_model)
	{
		case BC_RGB8:
			if(private_color) {
				cmap = XCreateColormap(display, rootwin, vis, AllocNone);
				create_private_colors();
			}
			else {
	 		  	cmap = DefaultColormap(display, screen);
				create_shared_colors();
			}

			allocate_color_table();
			break;

		default:
 			//cmap = DefaultColormap(display, screen);
 			cmap = XCreateColormap(display, rootwin, vis, AllocNone );
			break;
	}
	return 0;
}

int BC_WindowBase::create_private_colors()
{
	int color;
	total_colors = 256;

	for(int i = 0; i < 255; i++)
	{
		color = (i & 0xc0) << 16;
		color += (i & 0x38) << 10;
		color += (i & 0x7) << 5;
		color_table[i][0] = color;
	}
	create_shared_colors();        // overwrite the necessary colors on the table
	return 0;
}


int BC_WindowBase::create_color(int color)
{
	if(total_colors == 256)
	{
// replace the closest match with an exact match
		color_table[get_color_rgb8(color)][0] = color;
	}
	else
	{
// add the color to the table
		color_table[total_colors][0] = color;
		total_colors++;
	}
	return 0;
}

int BC_WindowBase::create_shared_colors()
{
	create_color(BLACK);
	create_color(WHITE);

	create_color(LTGREY);
	create_color(MEGREY);
	create_color(MDGREY);
	create_color(DKGREY);

	create_color(LTCYAN);
	create_color(MECYAN);
	create_color(MDCYAN);
	create_color(DKCYAN);

	create_color(LTGREEN);
	create_color(GREEN);
	create_color(DKGREEN);

	create_color(LTPINK);
	create_color(PINK);
	create_color(RED);

	create_color(LTBLUE);
	create_color(BLUE);
	create_color(DKBLUE);

	create_color(LTYELLOW);
	create_color(MEYELLOW);
	create_color(MDYELLOW);
	create_color(DKYELLOW);

	create_color(LTPURPLE);
	create_color(MEPURPLE);
	create_color(MDPURPLE);
	create_color(DKPURPLE);

	create_color(FGGREY);
	create_color(MNBLUE);
	create_color(ORANGE);
	create_color(FTGREY);

	return 0;
}

Cursor BC_WindowBase::create_grab_cursor()
{
	int iw = 23, iw1 = iw-1, iw2 = iw/2;
	int ih = 23, ih1 = ih-1, ih2 = ih/2;
	VFrame grab(iw,ih,BC_RGB888);
	grab.clear_frame();
	grab.set_pixel_color(RED);   // fg
	grab.draw_smooth(iw2,0,   iw1,0,   iw1,ih2);
	grab.draw_smooth(iw1,ih2, iw1,ih1, iw2,ih1);
	grab.draw_smooth(iw2,ih1, 0,ih1,   0,ih2);
	grab.draw_smooth(0,ih2,   0,0,     iw2,0);
	grab.set_pixel_color(WHITE); // bg
	grab.draw_line(0,ih2,     iw2-2,ih2);
	grab.draw_line(iw2+2,ih2, iw1,ih2);
	grab.draw_line(iw2,0,     iw2,ih2-2);
	grab.draw_line(iw2,ih2+2, iw2,ih1);

	int bpl = (iw+7)/8, isz = bpl * ih;
	char img[isz];  memset(img, 0, isz);
	char msk[isz];  memset(msk, 0, isz);
	unsigned char **rows = grab.get_rows();
	for( int iy=0; iy<ih; ++iy ) {
		char *op = img + iy*bpl;
		char *mp = msk + iy*bpl;
		unsigned char *ip = rows[iy];
		for( int ix=0; ix<iw; ++ix,ip+=3 ) {
			if( ip[0] ) mp[ix>>3] |= (1<<(ix&7));
			if( !ip[1] ) op[ix>>3] |= (1<<(ix&7));
		}
	}
	unsigned long white_pix = WhitePixel(display, screen);
	unsigned long black_pix = BlackPixel(display, screen);
	Pixmap img_xpm = XCreatePixmapFromBitmapData(display, rootwin,
		img, iw,ih, white_pix,black_pix, 1);
	Pixmap msk_xpm = XCreatePixmapFromBitmapData(display, rootwin,
		msk, iw,ih, white_pix,black_pix, 1);

	XColor fc, bc;
	fc.flags = bc.flags = DoRed | DoGreen | DoBlue;
	fc.red = 0xffff; fc.green = fc.blue = 0;  // fg
	bc.red = 0xffff; bc.green = 0xffff; bc.blue = 0x0000;     // bg
	Cursor cursor = XCreatePixmapCursor(display, img_xpm,msk_xpm, &fc,&bc, iw2,ih2);
	XFreePixmap(display, img_xpm);
	XFreePixmap(display, msk_xpm);
	return cursor;
}

int BC_WindowBase::allocate_color_table()
{
	int red, green, blue, color;
	XColor col;

	for(int i = 0; i < total_colors; i++)
	{
		color = color_table[i][0];
		red = (color & 0xFF0000) >> 16;
		green = (color & 0x00FF00) >> 8;
		blue = color & 0xFF;

		col.flags = DoRed | DoGreen | DoBlue;
		col.red   = red<<8   | red;
		col.green = green<<8 | green;
		col.blue  = blue<<8  | blue;

		XAllocColor(display, cmap, &col);
		color_table[i][1] = col.pixel;
	}

	XInstallColormap(display, cmap);
	return 0;
}

int BC_WindowBase::init_window_shape()
{
	if(bg_pixmap && bg_pixmap->use_alpha())
	{
		XShapeCombineMask(top_level->display,
			this->win, ShapeBounding, 0, 0,
			bg_pixmap->get_alpha(), ShapeSet);
	}
	return 0;
}


int BC_WindowBase::init_gc()
{
	unsigned long gcmask;
	gcmask = GCFont | GCGraphicsExposures;

	XGCValues gcvalues;
	gcvalues.font = mediumfont->fid;        // set the font
	gcvalues.graphics_exposures = 0;        // prevent expose events for every redraw
	gc = XCreateGC(display, rootwin, gcmask, &gcvalues);

// gcmask = GCCapStyle | GCJoinStyle;
// XGetGCValues(display, gc, gcmask, &gcvalues);
// printf("BC_WindowBase::init_gc %d %d %d\n", __LINE__, gcvalues.cap_style, gcvalues.join_style);
	return 0;
}

int BC_WindowBase::init_fonts()
{
	if( !(smallfont = XLoadQueryFont(display, _(resources.small_font))) )
		if( !(smallfont = XLoadQueryFont(display, _(resources.small_font2))) )
			smallfont = XLoadQueryFont(display, "fixed");
	if( !(mediumfont = XLoadQueryFont(display, _(resources.medium_font))) )
		if( !(mediumfont = XLoadQueryFont(display, _(resources.medium_font2))) )
			mediumfont = XLoadQueryFont(display, "fixed");
	if( !(largefont = XLoadQueryFont(display, _(resources.large_font))) )
		if( !(largefont = XLoadQueryFont(display, _(resources.large_font2))) )
			largefont = XLoadQueryFont(display, "fixed");
	if( !(bigfont = XLoadQueryFont(display, _(resources.big_font))) )
		if( !(bigfont = XLoadQueryFont(display, _(resources.big_font2))) )
			bigfont = XLoadQueryFont(display, "fixed");

	if((clockfont = XLoadQueryFont(display, _(resources.clock_font))) == NULL)
		if((clockfont = XLoadQueryFont(display, _(resources.clock_font2))) == NULL)
			clockfont = XLoadQueryFont(display, "fixed");

	init_xft();
	if(get_resources()->use_fontset)
	{
		char **m, *d;
		int n;

// FIXME: should check the m,d,n values
		smallfontset = XCreateFontSet(display, resources.small_fontset, &m, &n, &d);
		if( !smallfontset )
			smallfontset = XCreateFontSet(display, "fixed,*", &m, &n, &d);
		mediumfontset = XCreateFontSet(display, resources.medium_fontset, &m, &n, &d);
		if( !mediumfontset )
			mediumfontset = XCreateFontSet(display, "fixed,*", &m, &n, &d);
		largefontset = XCreateFontSet(display, resources.large_fontset, &m, &n, &d);
		if( !largefontset )
			largefontset = XCreateFontSet(display, "fixed,*", &m, &n, &d);
		bigfontset = XCreateFontSet(display, resources.big_fontset, &m, &n, &d);
		if( !bigfontset )
			bigfontset = XCreateFontSet(display, "fixed,*", &m, &n, &d);
		clockfontset = XCreateFontSet(display, resources.clock_fontset, &m, &n, &d);
		if( !clockfontset )
			clockfontset = XCreateFontSet(display, "fixed,*", &m, &n, &d);
		if(clockfontset && bigfontset && largefontset && mediumfontset && smallfontset) {
			curr_fontset = mediumfontset;
			get_resources()->use_fontset = 1;
		}
		else {
			curr_fontset = 0;
			get_resources()->use_fontset = 0;
		}
	}

	return 0;
}

void BC_WindowBase::init_xft()
{
#ifdef HAVE_XFT
	if( !get_resources()->use_xft ) return;
// apparently, xft is not reentrant, more than this is needed
static Mutex xft_init_lock("BC_WindowBase::xft_init_lock", 0);
xft_init_lock.lock("BC_WindowBase::init_xft");
	if(!(smallfont_xft =
		(resources.small_font_xft[0] == '-' ?
			xftFontOpenXlfd(display, screen, resources.small_font_xft) :
			xftFontOpenName(display, screen, resources.small_font_xft))) )
		if(!(smallfont_xft =
			xftFontOpenXlfd(display, screen, resources.small_font_xft2)))
			smallfont_xft = xftFontOpenXlfd(display, screen, "fixed");
	if(!(mediumfont_xft =
		(resources.medium_font_xft[0] == '-' ?
			xftFontOpenXlfd(display, screen, resources.medium_font_xft) :
			xftFontOpenName(display, screen, resources.medium_font_xft))) )
		if(!(mediumfont_xft =
			xftFontOpenXlfd(display, screen, resources.medium_font_xft2)))
			mediumfont_xft = xftFontOpenXlfd(display, screen, "fixed");
	if(!(largefont_xft =
		(resources.large_font_xft[0] == '-' ?
			xftFontOpenXlfd(display, screen, resources.large_font_xft) :
			xftFontOpenName(display, screen, resources.large_font_xft))) )
		if(!(largefont_xft =
			xftFontOpenXlfd(display, screen, resources.large_font_xft2)))
			largefont_xft = xftFontOpenXlfd(display, screen, "fixed");
	if(!(bigfont_xft =
		(resources.big_font_xft[0] == '-' ?
			xftFontOpenXlfd(display, screen, resources.big_font_xft) :
			xftFontOpenName(display, screen, resources.big_font_xft))) )
		if(!(bigfont_xft =
			xftFontOpenXlfd(display, screen, resources.big_font_xft2)))
			bigfont_xft = xftFontOpenXlfd(display, screen, "fixed");
	if(!(clockfont_xft =
		(resources.clock_font_xft[0] == '-' ?
			xftFontOpenXlfd(display, screen, resources.clock_font_xft) :
			xftFontOpenName(display, screen, resources.clock_font_xft))) )
		clockfont_xft = xftFontOpenXlfd(display, screen, "fixed");


	if(!(bold_smallfont_xft =
		(resources.small_b_font_xft[0] == '-' ?
			xftFontOpenXlfd(display, screen, resources.small_b_font_xft) :
			xftFontOpenName(display, screen, resources.small_b_font_xft))) )
		bold_smallfont_xft = xftFontOpenXlfd(display, screen, "fixed");
	if(!(bold_mediumfont_xft =
		(resources.medium_b_font_xft[0] == '-' ?
			xftFontOpenXlfd(display, screen, resources.medium_b_font_xft) :
			xftFontOpenName(display, screen, resources.medium_b_font_xft))) )
		bold_mediumfont_xft = xftFontOpenXlfd(display, screen, "fixed");
	if(!(bold_largefont_xft =
		(resources.large_b_font_xft[0] == '-' ?
			xftFontOpenXlfd(display, screen, resources.large_b_font_xft) :
			xftFontOpenName(display, screen, resources.large_b_font_xft))) )
		bold_largefont_xft = xftFontOpenXlfd(display, screen, "fixed");

	if( !smallfont_xft || !mediumfont_xft || !largefont_xft || !bigfont_xft ||
	    !bold_largefont_xft || !bold_mediumfont_xft || !bold_largefont_xft ||
	    !clockfont_xft ) {
		printf("BC_WindowBase::init_fonts: no xft fonts found:"
			" %s=%p\n %s=%p\n %s=%p\n %s=%p\n %s=%p\n %s=%p\n %s=%p\n %s=%p\n",
			resources.small_font_xft, smallfont_xft,
			resources.medium_font_xft, mediumfont_xft,
			resources.large_font_xft, largefont_xft,
			resources.big_font_xft, bigfont_xft,
			resources.clock_font_xft, clockfont_xft,
			resources.small_b_font_xft, bold_smallfont_xft,
			resources.medium_b_font_xft, bold_mediumfont_xft,
			resources.large_b_font_xft, bold_largefont_xft);
		get_resources()->use_xft = 0;
		exit(1);
	}
// _XftDisplayInfo needs a lock.
	xftDefaultHasRender(display);
xft_init_lock.unlock();
#endif // HAVE_XFT
}

void BC_WindowBase::init_im()
{
	XIMStyles *xim_styles;
	XIMStyle xim_style;

	if(!(input_method = XOpenIM(display, NULL, NULL, NULL)))
	{
		printf("BC_WindowBase::init_im: Could not open input method.\n");
		exit(1);
	}
	if(XGetIMValues(input_method, XNQueryInputStyle, &xim_styles, NULL) ||
			xim_styles == NULL)
	{
		printf("BC_WindowBase::init_im: Input method doesn't support any styles.\n");
		XCloseIM(input_method);
		exit(1);
	}

	xim_style = 0;
	for(int z = 0;  z < xim_styles->count_styles;  z++)
	{
		if(xim_styles->supported_styles[z] == (XIMPreeditNothing | XIMStatusNothing))
		{
			xim_style = xim_styles->supported_styles[z];
			break;
		}
	}
	XFree(xim_styles);

	if(xim_style == 0)
	{
		printf("BC_WindowBase::init_im: Input method doesn't support the style we need.\n");
		XCloseIM(input_method);
		exit(1);
	}

	input_context = XCreateIC(input_method, XNInputStyle, xim_style,
		XNClientWindow, win, XNFocusWindow, win, NULL);
	if(!input_context)
	{
		printf("BC_WindowBase::init_im: Failed to create input context.\n");
		XCloseIM(input_method);
		exit(1);
	}
}

void BC_WindowBase::finit_im()
{
	if( input_context ) {
		XDestroyIC(input_context);
		input_context = 0;
	}
	if( input_method ) {
		XCloseIM(input_method);
		input_method = 0;
	}
}


int BC_WindowBase::get_color(int64_t color)
{
// return pixel of color
// use this only for drawing subwindows not for bitmaps
	 int i, test, difference;

	switch(color_model)
	{
	case BC_RGB8:
		if(private_color)
			return get_color_rgb8(color);
// test last color looked up
		if(current_color_value == color)
			return current_color_pixel;

// look up in table
		current_color_value = color;
		for(i = 0; i < total_colors; i++)
		{
			if(color_table[i][0] == color)
			{
				current_color_pixel = color_table[i][1];
				return current_color_pixel;
			}
		}

// find nearest match
		difference = 0xFFFFFF;

		for(i = 0; i < total_colors; i++)
		{
			test = abs((int)(color_table[i][0] - color));

			if(test < difference)
			{
				current_color_pixel = color_table[i][1];
				difference = test;
			}
		}
		return current_color_pixel;

	case BC_RGB565:
		return get_color_rgb16(color);

	case BC_BGR565:
		return get_color_bgr16(color);

	case BC_RGB888:
	case BC_BGR888:
		return client_byte_order == server_byte_order ?
			color : get_color_bgr24(color);

	default:
		return color;
	}
	return 0;
}

int BC_WindowBase::get_color_rgb8(int color)
{
	int pixel;

	pixel = (color & 0xc00000) >> 16;
	pixel += (color & 0xe000) >> 10;
	pixel += (color & 0xe0) >> 5;
	return pixel;
}

int64_t BC_WindowBase::get_color_rgb16(int color)
{
	int64_t result;
	result = (color & 0xf80000) >> 8;
	result += (color & 0xfc00) >> 5;
	result += (color & 0xf8) >> 3;

	return result;
}

int64_t BC_WindowBase::get_color_bgr16(int color)
{
	int64_t result;
	result = (color & 0xf80000) >> 19;
	result += (color & 0xfc00) >> 5;
	result += (color & 0xf8) << 8;

	return result;
}

int64_t BC_WindowBase::get_color_bgr24(int color)
{
	int64_t result;
	result = (color & 0xff) << 16;
	result += (color & 0xff00);
	result += (color & 0xff0000) >> 16;
	return result;
}

void BC_WindowBase::start_video()
{
	cursor_timer->update();
	video_on = 1;
//	set_color(BLACK);
//	draw_box(0, 0, get_w(), get_h());
//	flash();
}

void BC_WindowBase::stop_video()
{
	video_on = 0;
	unhide_cursor();
}



int64_t BC_WindowBase::get_color()
{
	return top_level->current_color;
}

void BC_WindowBase::set_color(int64_t color)
{
	top_level->current_color = color;
	XSetForeground(top_level->display,
		top_level->gc,
		top_level->get_color(color));
}

void BC_WindowBase::set_opaque()
{
	XSetFunction(top_level->display, top_level->gc, GXcopy);
}

void BC_WindowBase::set_inverse()
{
	XSetFunction(top_level->display, top_level->gc, GXxor);
}

void BC_WindowBase::set_line_width(int value)
{
	this->line_width = value;
	XSetLineAttributes(top_level->display, top_level->gc, value,	/* line_width */
		line_dashes == 0 ? LineSolid : LineOnOffDash,		/* line_style */
		line_dashes == 0 ? CapRound : CapNotLast,		/* cap_style */
		JoinMiter);						/* join_style */

	if(line_dashes > 0) {
		const char dashes = line_dashes;
		XSetDashes(top_level->display, top_level->gc, 0, &dashes, 1);
	}

// XGCValues gcvalues;
// unsigned long gcmask;
// gcmask = GCCapStyle | GCJoinStyle;
// XGetGCValues(top_level->display, top_level->gc, gcmask, &gcvalues);
// printf("BC_WindowBase::set_line_width %d %d %d\n", __LINE__, gcvalues.cap_style, gcvalues.join_style);
}

void BC_WindowBase::set_line_dashes(int value)
{
	line_dashes = value;
// call XSetLineAttributes
	set_line_width(line_width);
}


Cursor BC_WindowBase::get_cursor_struct(int cursor)
{
	switch(cursor)
	{
		case ARROW_CURSOR:         return top_level->arrow_cursor;
		case CROSS_CURSOR:         return top_level->cross_cursor;
		case IBEAM_CURSOR:         return top_level->ibeam_cursor;
		case VSEPARATE_CURSOR:     return top_level->vseparate_cursor;
		case HSEPARATE_CURSOR:     return top_level->hseparate_cursor;
		case MOVE_CURSOR:          return top_level->move_cursor;
		case LEFT_CURSOR:          return top_level->left_cursor;
		case RIGHT_CURSOR:         return top_level->right_cursor;
		case UPRIGHT_ARROW_CURSOR: return top_level->upright_arrow_cursor;
		case UPLEFT_RESIZE:        return top_level->upleft_resize_cursor;
		case UPRIGHT_RESIZE:       return top_level->upright_resize_cursor;
		case DOWNLEFT_RESIZE:      return top_level->downleft_resize_cursor;
		case DOWNRIGHT_RESIZE:     return top_level->downright_resize_cursor;
		case HOURGLASS_CURSOR:     return top_level->hourglass_cursor;
		case TRANSPARENT_CURSOR:   return top_level->transparent_cursor;
		case GRABBED_CURSOR:       return top_level->grabbed_cursor;
	}
	return 0;
}

void BC_WindowBase::set_cursor(int cursor, int override, int flush)
{
// inherit cursor from parent
	if(cursor < 0)
	{
		XUndefineCursor(top_level->display, win);
		current_cursor = cursor;
	}
	else
// don't change cursor if overridden
	if((!top_level->is_hourglass && !is_transparent) ||
		override)
	{
		XDefineCursor(top_level->display, win, get_cursor_struct(cursor));
		if(flush) this->flush();
	}

	if(!override) current_cursor = cursor;
}

void BC_WindowBase::set_x_cursor(int cursor)
{
	temp_cursor = XCreateFontCursor(top_level->display, cursor);
	XDefineCursor(top_level->display, win, temp_cursor);
	current_cursor = cursor;
	flush();
}

int BC_WindowBase::get_cursor()
{
	return current_cursor;
}

void BC_WindowBase::start_hourglass()
{
	top_level->start_hourglass_recursive();
	top_level->flush();
}

void BC_WindowBase::stop_hourglass()
{
	top_level->stop_hourglass_recursive();
	top_level->flush();
}

void BC_WindowBase::start_hourglass_recursive()
{
	if(this == top_level)
	{
		hourglass_total++;
		is_hourglass = 1;
	}

	if(!is_transparent)
	{
		set_cursor(HOURGLASS_CURSOR, 1, 0);
		for(int i = 0; i < subwindows->total; i++)
		{
			subwindows->values[i]->start_hourglass_recursive();
		}
	}
}

void BC_WindowBase::stop_hourglass_recursive()
{
	if(this == top_level)
	{
		if(hourglass_total == 0) return;
		top_level->hourglass_total--;
	}

	if(!top_level->hourglass_total)
	{
		top_level->is_hourglass = 0;

// Cause set_cursor to perform change
		if(!is_transparent)
			set_cursor(current_cursor, 1, 0);

		for(int i = 0; i < subwindows->total; i++)
		{
			subwindows->values[i]->stop_hourglass_recursive();
		}
	}
}




XFontStruct* BC_WindowBase::get_font_struct(int font)
{
// Clear out unrelated flags
	if(font & BOLDFACE) font ^= BOLDFACE;

	switch(font) {
		case SMALLFONT:  return top_level->smallfont;  break;
		case MEDIUMFONT: return top_level->mediumfont; break;
		case LARGEFONT:  return top_level->largefont;  break;
		case BIGFONT:    return top_level->bigfont;    break;
		case CLOCKFONT:  return top_level->clockfont;  break;
	}
	return 0;
}

XFontSet BC_WindowBase::get_fontset(int font)
{
	XFontSet fs = 0;

	if(get_resources()->use_fontset)
	{
		switch(font & 0xff) {
			case SMALLFONT:  fs = top_level->smallfontset; break;
			case MEDIUMFONT: fs = top_level->mediumfontset; break;
			case LARGEFONT:  fs = top_level->largefontset; break;
			case BIGFONT:    fs = top_level->bigfontset;   break;
			case CLOCKFONT: fs = top_level->clockfontset; break;
		}
	}

	return fs;
}

#ifdef HAVE_XFT
XftFont* BC_WindowBase::get_xft_struct(int font)
{
	switch(font) {
		case SMALLFONT:    return (XftFont*)top_level->smallfont_xft;
		case MEDIUMFONT:   return (XftFont*)top_level->mediumfont_xft;
		case LARGEFONT:    return (XftFont*)top_level->largefont_xft;
		case BIGFONT:      return (XftFont*)top_level->bigfont_xft;
		case CLOCKFONT:    return (XftFont*)top_level->clockfont_xft;
		case MEDIUMFONT_3D: return (XftFont*)top_level->bold_mediumfont_xft;
		case SMALLFONT_3D:  return (XftFont*)top_level->bold_smallfont_xft;
		case LARGEFONT_3D:  return (XftFont*)top_level->bold_largefont_xft;
	}

	return 0;
}
#endif


int BC_WindowBase::get_current_font()
{
	return top_level->current_font;
}

void BC_WindowBase::set_font(int font)
{
	top_level->current_font = font;

#ifdef HAVE_XFT
	if(get_resources()->use_xft) {}
	else
#endif
	if(get_resources()->use_fontset) {
		set_fontset(font);
	}

	if(get_font_struct(font))
	{
		XSetFont(top_level->display, top_level->gc, get_font_struct(font)->fid);
	}

	return;
}

void BC_WindowBase::set_fontset(int font)
{
	XFontSet fs = 0;

	if(get_resources()->use_fontset) {
		switch(font) {
			case SMALLFONT:  fs = top_level->smallfontset; break;
			case MEDIUMFONT: fs = top_level->mediumfontset; break;
			case LARGEFONT:  fs = top_level->largefontset; break;
			case BIGFONT:    fs = top_level->bigfontset;   break;
			case CLOCKFONT:  fs = top_level->clockfontset; break;
		}
	}

	curr_fontset = fs;
}


XFontSet BC_WindowBase::get_curr_fontset(void)
{
	if(get_resources()->use_fontset)
		return curr_fontset;
	return 0;
}

int BC_WindowBase::get_single_text_width(int font, const char *text, int length)
{
#ifdef HAVE_XFT
	if(get_resources()->use_xft && get_xft_struct(font))
	{
		XGlyphInfo extents;
#ifdef X_HAVE_UTF8_STRING
		if(get_resources()->locale_utf8)
		{
			xftTextExtentsUtf8(top_level->display,
				get_xft_struct(font),
				(const XftChar8 *)text,
				length,
				&extents);
		}
		else
#endif
		{
			xftTextExtents8(top_level->display,
				get_xft_struct(font),
				(const XftChar8 *)text,
				length,
				&extents);
		}
		return extents.xOff;
	}
	else
#endif
	if(get_resources()->use_fontset && top_level->get_fontset(font))
		return XmbTextEscapement(top_level->get_fontset(font), text, length);
	else
	if(get_font_struct(font))
		return XTextWidth(get_font_struct(font), text, length);
	else
	{
		int w = 0;
		switch(font)
		{
			case MEDIUM_7SEGMENT:
				return get_resources()->medium_7segment[0]->get_w() * length;
				break;

			default:
				return 0;
		}
		return w;
	}
}

int BC_WindowBase::get_text_width(int font, const char *text, int length)
{
	int i, j, w = 0, line_w = 0;
	if(length < 0) length = strlen(text);

	for(i = 0, j = 0; i <= length; i++)
	{
		line_w = 0;
		if(text[i] == '\n')
		{
			line_w = get_single_text_width(font, &text[j], i - j);
			j = i + 1;
		}
		else
		if(text[i] == 0)
		{
			line_w = get_single_text_width(font, &text[j], length - j);
		}
		if(line_w > w) w = line_w;
	}

	if(i > length && w == 0)
	{
		w = get_single_text_width(font, text, length);
	}

	return w;
}

int BC_WindowBase::get_text_width(int font, const wchar_t *text, int length)
{
	int i, j, w = 0;
	if( length < 0 ) length = wcslen(text);

	for( i=j=0; i<length && text[i]; ++i ) {
		if( text[i] != '\n' ) continue;
		if( i > j ) {
			int lw = get_single_text_width(font, &text[j], i-j);
			if( w < lw ) w = lw;
		}
		j = i + 1;
	}
	if( j < length ) {
		int lw = get_single_text_width(font, &text[j], length-j);
		if( w < lw ) w = lw;
	}

	return w;
}

int BC_WindowBase::get_text_ascent(int font)
{
#ifdef HAVE_XFT
	XftFont *fstruct;
	if( (fstruct = get_xft_struct(font)) != 0 )
		return fstruct->ascent;
#endif
	if(get_resources()->use_fontset && top_level->get_fontset(font))
	{
	        XFontSetExtents *extents;

		extents = XExtentsOfFontSet(top_level->get_fontset(font));
		return -extents->max_logical_extent.y;
	}

	if(get_font_struct(font))
		return top_level->get_font_struct(font)->ascent;

	switch(font) {
		case MEDIUM_7SEGMENT:
			return get_resources()->medium_7segment[0]->get_h();
	}
	return 0;
}

int BC_WindowBase::get_text_descent(int font)
{
#ifdef HAVE_XFT
	XftFont *fstruct;
	if( (fstruct = get_xft_struct(font)) != 0 )
		return fstruct->descent;
#endif
	if(get_resources()->use_fontset && top_level->get_fontset(font)) {
		XFontSetExtents *extents;
		extents = XExtentsOfFontSet(top_level->get_fontset(font));
	        return (extents->max_logical_extent.height
			+ extents->max_logical_extent.y);
	}

	if(get_font_struct(font))
		return top_level->get_font_struct(font)->descent;

	return 0;
}

int BC_WindowBase::get_text_height(int font, const char *text)
{
	int rowh;
#ifdef HAVE_XFT
	XftFont *fstruct;
	if( (fstruct = get_xft_struct(font)) != 0 )
		rowh = fstruct->height;
	else
#endif
		rowh = get_text_ascent(font) + get_text_descent(font);

	if(!text) return rowh;

// Add height of lines
	int h = 0, i, length = strlen(text);
	for(i = 0; i <= length; i++)
	{
		if(text[i] == '\n')
			h++;
		else
		if(text[i] == 0)
			h++;
	}
	return h * rowh;
}

BC_Bitmap* BC_WindowBase::new_bitmap(int w, int h, int color_model)
{
	if(color_model < 0) color_model = top_level->get_color_model();
	return new BC_Bitmap(top_level, w, h, color_model);
}

void BC_WindowBase::init_wait()
{
#ifndef SINGLE_THREAD
	if(window_type != MAIN_WINDOW)
		top_level->init_wait();
	init_lock->lock("BC_WindowBase::init_wait");
	init_lock->unlock();
#endif
}

int BC_WindowBase::accel_available(int color_model, int lock_it)
{
	if( window_type != MAIN_WINDOW )
		return top_level->accel_available(color_model, lock_it);
	if( lock_it )
		lock_window("BC_WindowBase::accel_available");

	switch(color_model) {
	case BC_YUV420P:
		grab_port_id(color_model);
		break;

	case BC_YUV422:
		grab_port_id(color_model);
		break;

	default:
		break;
	}

	if( lock_it )
		unlock_window();
//printf("BC_WindowBase::accel_available %d %d\n", color_model, xvideo_port_id);
	return xvideo_port_id >= 0 ? 1 : 0;
}


int BC_WindowBase::grab_port_id(int color_model)
{
	if( !get_resources()->use_xvideo ||	// disabled
	    !get_resources()->use_shm )		// Only local server is fast enough.
		return -1;
	if( xvideo_port_id >= 0 )
		return xvideo_port_id;

	unsigned int ver, rev, reqBase, eventBase, errorBase;
	if( Success != XvQueryExtension(display, // XV extension is available
		    &ver, &rev, &reqBase, &eventBase, &errorBase) )
		return -1;

// XV adaptors are available
	unsigned int numAdapt = 0;
	XvAdaptorInfo *info = 0;
	XvQueryAdaptors(display, DefaultRootWindow(display), &numAdapt, &info);
	if( !numAdapt )
		return -1;

// Translate from color_model to X color model
	int x_color_model = BC_CModels::bc_to_x(color_model);

// Get adaptor with desired color model
	for( int i = 0; i < (int)numAdapt && xvideo_port_id == -1; i++) {
		if( !(info[i].type & XvImageMask) || !info[i].num_ports ) continue;
// adaptor supports XvImages
		int numFormats = 0, numPorts = info[i].num_ports;
		XvImageFormatValues *formats =
			XvListImageFormats(display, info[i].base_id, &numFormats);
		if( !formats ) continue;

		for( int j=0; j<numFormats && xvideo_port_id<0; ++j ) {
			if( formats[j].id != x_color_model ) continue;
// this adaptor supports the desired format, grab a port
			for( int k=0; k<numPorts; ++k ) {
				if( Success == XvGrabPort(top_level->display,
					info[i].base_id+k, CurrentTime) ) {
//printf("BC_WindowBase::grab_port_id %llx\n", info[i].base_id);
					xvideo_port_id = info[i].base_id + k;
					break;
				}
			}
		}
		XFree(formats);
	}

	XvFreeAdaptorInfo(info);

	return xvideo_port_id;
}


int BC_WindowBase::show_window(int flush)
{
	for(int i = 0; i < subwindows->size(); i++)
	{
		subwindows->get(i)->show_window(0);
	}

	XMapWindow(top_level->display, win);
	if(flush) XFlush(top_level->display);
//	XSync(top_level->display, 0);
	hidden = 0;
	return 0;
}

int BC_WindowBase::hide_window(int flush)
{
	for(int i = 0; i < subwindows->size(); i++)
	{
		subwindows->get(i)->hide_window(0);
	}

	XUnmapWindow(top_level->display, win);
	if(flush) this->flush();
	hidden = 1;
	return 0;
}

BC_MenuBar* BC_WindowBase::add_menubar(BC_MenuBar *menu_bar)
{
	subwindows->append((BC_SubWindow*)menu_bar);

	menu_bar->parent_window = this;
	menu_bar->top_level = this->top_level;
	menu_bar->initialize();
	return menu_bar;
}

BC_WindowBase* BC_WindowBase::add_popup(BC_WindowBase *window)
{
//printf("BC_WindowBase::add_popup window=%p win=%p\n", window, window->win);
	if(this != top_level) return top_level->add_popup(window);
	popups.append(window);
	return window;
}

void BC_WindowBase::remove_popup(BC_WindowBase *window)
{
//printf("BC_WindowBase::remove_popup %d size=%d window=%p win=%p\n", __LINE__, popups.size(), window, window->win);
	if(this != top_level)
		top_level->remove_popup(window);
	else
		popups.remove(window);
//printf("BC_WindowBase::remove_popup %d size=%d window=%p win=%p\n", __LINE__, popups.size(), window, window->win);
}


BC_WindowBase* BC_WindowBase::add_subwindow(BC_WindowBase *subwindow)
{
	subwindows->append(subwindow);

	if(subwindow->bg_color == -1) subwindow->bg_color = this->bg_color;

// parent window must be set before the subwindow initialization
	subwindow->parent_window = this;
	subwindow->top_level = this->top_level;

// Execute derived initialization
	subwindow->initialize();
	return subwindow;
}


BC_WindowBase* BC_WindowBase::add_tool(BC_WindowBase *subwindow)
{
	return add_subwindow(subwindow);
}

int BC_WindowBase::flash(int x, int y, int w, int h, int flush)
{
	if( !top_level->flash_enabled ) return 0;
//printf("BC_WindowBase::flash %d %d %d %d %d\n", __LINE__, w, h, this->w, this->h);
	set_opaque();
	XSetWindowBackgroundPixmap(top_level->display, win, pixmap->opaque_pixmap);
	if(x >= 0)
	{
		XClearArea(top_level->display, win, x, y, w, h, 0);
	}
	else
	{
		XClearWindow(top_level->display, win);
	}

	if(flush)
		this->flush();
	return 0;
}

int BC_WindowBase::flash(int flush)
{
	flash(-1, -1, -1, -1, flush);
	return 0;
}

void BC_WindowBase::flush()
{
	//if(!get_window_lock())
	//	printf("BC_WindowBase::flush %s not locked\n", top_level->title);
	// X gets hosed if Flush/Sync are not user locked (at libX11-1.1.5 / libxcb-1.1.91)
	//   _XReply deadlocks in condition_wait waiting for xlib lock when waiters!=-1
	int locked  = get_window_lock();
	if( !locked ) lock_window("BC_WindowBase::flush");
	XFlush(top_level->display);
	if( !locked ) unlock_window();
}

void BC_WindowBase::sync_display()
{
	int locked  = get_window_lock();
	if( !locked ) lock_window("BC_WindowBase::sync_display");
	XSync(top_level->display, False);
	if( !locked ) unlock_window();
}

int BC_WindowBase::get_window_lock()
{
#ifdef SINGLE_THREAD
	return BC_Display::display_global->get_display_locked();
#else
	return top_level->window_lock;
#endif
}

int BC_WindowBase::lock_window(const char *location)
{
	if(top_level && top_level != this)
	{
		top_level->lock_window(location);
	}
	else
	if(top_level)
	{
		SET_LOCK(this, title, location);
#ifdef SINGLE_THREAD
		BC_Display::lock_display(location);
#else
		XLockDisplay(top_level->display);
		top_level->display_lock_owner = pthread_self();
#endif
		SET_LOCK2
		++top_level->window_lock;
	}
	else
	{
		printf("BC_WindowBase::lock_window top_level NULL\n");
	}
	return 0;
}

int BC_WindowBase::unlock_window()
{
	if(top_level && top_level != this)
	{
		top_level->unlock_window();
	}
	else
	if(top_level)
	{
		UNSET_LOCK(this);
		if( !top_level->window_lock ) {
			printf("BC_WindowBase::unlock_window %s not locked\n", title);
			booby();
		}
		if( top_level->window_lock > 0 )
			if( --top_level->window_lock == 0 )
				top_level->display_lock_owner = 0;
#ifdef SINGLE_THREAD
		BC_Display::unlock_display();
#else
		XUnlockDisplay(top_level->display);
#endif
	}
	else
	{
		printf("BC_WindowBase::unlock_window top_level NULL\n");
	}
	return 0;
}

int BC_WindowBase::break_lock()
{
	if( !top_level ) return 0;
	if( top_level != this ) return top_level->break_lock();
	if( top_level->display_lock_owner != pthread_self() ) return 0;
	if( top_level->window_lock != 1 ) return 0;
	UNSET_LOCK(this);
	window_lock = 0;
	display_lock_owner = 0;
#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#else
	XUnlockDisplay(display);
#endif
	return 1;
}

void BC_WindowBase::set_done(int return_value)
{
	if(done_set) return;
	done_set = 1;
	if(window_type != MAIN_WINDOW)
		top_level->set_done(return_value);
	else
	{
#ifdef SINGLE_THREAD
		this->return_value = return_value;
		BC_Display::display_global->arm_completion(this);
		completion_lock->unlock();
#else // SINGLE_THREAD
		init_wait();
		if( !event_thread ) return;
		XEvent *event = new_xevent();
		XClientMessageEvent *ptr = (XClientMessageEvent*)event;
		event->type = ClientMessage;
		ptr->message_type = SetDoneXAtom;
		ptr->format = 32;
		this->return_value = return_value;
// May lock up here because XSendEvent doesn't work too well
// asynchronous with XNextEvent.
// This causes BC_WindowEvents to forward a copy of the event to run_window where
// it is deleted.
// Deletion of event_thread is done at the end of BC_WindowBase::run_window() - by calling the destructor
		put_event(event);
#endif
	}
}

void BC_WindowBase::close(int return_value)
{
	hide_window();  flush();
	set_done(return_value);
}

int BC_WindowBase::grab(BC_WindowBase *window)
{
	if( window->active_grab && this != window->active_grab ) return 0;
	window->active_grab = this;
	this->grab_active = window;
	return 1;
}
int BC_WindowBase::ungrab(BC_WindowBase *window)
{
	if( window->active_grab && this != window->active_grab ) return 0;
	window->active_grab = 0;
	this->grab_active = 0;
	return 1;
}
int BC_WindowBase::grab_event_count()
{
	int result = 0;
#ifndef SINGLE_THREAD
	result = grab_active->get_event_count();
#endif
	return result;
}
int BC_WindowBase::grab_buttons()
{
	XSync(top_level->display, False);
	if( XGrabButton(top_level->display, AnyButton, AnyModifier,
			top_level->rootwin, True, ButtonPressMask | ButtonReleaseMask,
			GrabModeAsync, GrabModeSync, None, None) == GrabSuccess ) {
		set_active_subwindow(this);
		return 0;
	}
	return 1;
}
void BC_WindowBase::ungrab_buttons()
{
	XUngrabButton(top_level->display, AnyButton, AnyModifier, top_level->rootwin);
	set_active_subwindow(0);
	unhide_cursor();
}
void BC_WindowBase::grab_cursor()
{
	Cursor cursor_grab = get_cursor_struct(GRABBED_CURSOR);
	XGrabPointer(top_level->display, top_level->rootwin, True,
		PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
		GrabModeAsync, GrabModeAsync, None, cursor_grab, CurrentTime);
}
void BC_WindowBase::ungrab_cursor()
{
	XUngrabPointer(top_level->display, CurrentTime);
}

// for get_root_w/h
//   WidthOfScreen/HeightOfScreen of XDefaultScreenOfDisplay
//   this is the bounding box of all the screens

int BC_WindowBase::get_root_w(int lock_display)
{
	if(lock_display) lock_window("BC_WindowBase::get_root_w");
	Screen *def_screen = XDefaultScreenOfDisplay(top_level->display);
	int result = WidthOfScreen(def_screen);
	if(lock_display) unlock_window();
	return result;
}

int BC_WindowBase::get_root_h(int lock_display)
{
	if(lock_display) lock_window("BC_WindowBase::get_root_h");
	Screen *def_screen = XDefaultScreenOfDisplay(top_level->display);
	int result = HeightOfScreen(def_screen);
	if(lock_display) unlock_window();
	return result;
}

XineramaScreenInfo *
BC_WindowBase::get_xinerama_info(int screen)
{
	if( !xinerama_info || !xinerama_screens ) return 0;
	if( screen >= 0 ) {
		for( int i=0; i<xinerama_screens; ++i )
			if( xinerama_info[i].screen_number == screen )
				return &xinerama_info[i];
		return 0;
	}
	int top_x = get_x(), top_y = get_y();
	if(  BC_DisplayInfo::left_border >= 0 ) top_x +=  BC_DisplayInfo::left_border;
	if(  BC_DisplayInfo::top_border >= 0 ) top_y +=  BC_DisplayInfo::top_border;
	for( int i=0; i<xinerama_screens; ++i ) {
		int scr_y = top_y - xinerama_info[i].y_org;
		if( scr_y < 0 || scr_y >= xinerama_info[i].height ) continue;
		int scr_x = top_x - xinerama_info[i].x_org;
		if( scr_x >= 0 && scr_x < xinerama_info[i].width )
			return &xinerama_info[i];
	}
	return 0;
}

void BC_WindowBase::get_fullscreen_geometry(int &wx, int &wy, int &ww, int &wh)
{
	XineramaScreenInfo *info = top_level->get_xinerama_info(-1);
	if( info ) {
		wx = info->x_org;  wy = info->y_org;
		ww = info->width;  wh = info->height;
	}
	else {
		wx = get_screen_x(0, -1);
		wy = get_screen_y(0, -1);
		int scr_w0 = get_screen_w(0, 0);
		int root_w = get_root_w(0);
		int root_h = get_root_h(0);
		if( root_w > scr_w0 ) { // multi-headed
			if( wx >= scr_w0 ) {
				// assumes right side is the big one
				ww = root_w - scr_w0;
				wh = root_h;
			}
			else {
				// use same aspect ratio to compute left height
				ww = scr_w0;
				wh = (w*root_h) / (root_w-scr_w0);
			}
		}
		else {
			ww = root_w;
			wh = root_h;
		}
	}
}

int BC_WindowBase::get_screen_x(int lock_display, int screen)
{
	int result = -1;
	if(lock_display) lock_window("BC_WindowBase::get_screen_x");
	XineramaScreenInfo *info = top_level->get_xinerama_info(screen);
	if( !info ) {
		result = 0;
		int root_w = get_root_w(0);
		int root_h = get_root_h(0);
// Shift X based on position of current window if dual head
		if( (float)root_w/root_h > 1.8 ) {
			root_w = get_screen_w(0, 0);
			if( top_level->get_x() >= root_w )
				result = root_w;
		}
	}
	else
		result = info->x_org;
	if(lock_display) unlock_window();
	return result;
}

int BC_WindowBase::get_screen_y(int lock_display, int screen)
{
	if(lock_display) lock_window("BC_WindowBase::get_screen_y");
	XineramaScreenInfo *info = top_level->get_xinerama_info(screen);
	int result = !info ? 0 : info->y_org;
	if(lock_display) unlock_window();
	return result;
}

int BC_WindowBase::get_screen_w(int lock_display, int screen)
{
	int result = -1;
	if(lock_display) lock_window("BC_WindowBase::get_screen_w");
	XineramaScreenInfo *info = top_level->get_xinerama_info(screen);
	if( !info ) {
		int width = get_root_w(0);
		int height = get_root_h(0);
		if( (float)width/height > 1.8 ) {
			// If dual head, the screen width is > 16x9
			// but we only want to fill one screen
			// this code assumes the "big" screen is on the right
			int scr_w0 = width / 2;
			switch( height ) {
			case 600:  scr_w0 = 800;   break;
			case 720:  scr_w0 = 1280;  break;
			case 1024: scr_w0 = 1280;  break;
			case 1200: scr_w0 = 1600;  break;
			case 1080: scr_w0 = 1920;  break;
			}
			int scr_w1 = width - scr_w0;
			result = screen > 0 ? scr_w1 :
				screen == 0 ? scr_w0 :
				top_level->get_x() < scr_w0 ? scr_w0 : scr_w1;
		}
		else
			result = width;
	}
	else
		result = info->width;
	if(lock_display) unlock_window();
	return result;
}

int BC_WindowBase::get_screen_h(int lock_display, int screen)
{
	if(lock_display) lock_window("BC_WindowBase::get_screen_h");
	XineramaScreenInfo *info = top_level->get_xinerama_info(screen);
	int result = info ? info->height : get_root_h(0);
	if(lock_display) unlock_window();
	return result;
}

// Bottom right corner
int BC_WindowBase::get_x2()
{
	return w + x;
}

int BC_WindowBase::get_y2()
{
	return y + h;
}

int BC_WindowBase::get_video_on()
{
	return video_on;
}

int BC_WindowBase::get_hidden()
{
	return top_level->hidden;
}

int BC_WindowBase::cursor_inside()
{
	return (top_level->cursor_x >= 0 &&
			top_level->cursor_y >= 0 &&
			top_level->cursor_x < w &&
			top_level->cursor_y < h);
}

BC_WindowBase* BC_WindowBase::get_top_level()
{
	return top_level;
}

BC_WindowBase* BC_WindowBase::get_parent()
{
	return parent_window;
}

int BC_WindowBase::get_color_model()
{
	return top_level->color_model;
}

BC_Resources* BC_WindowBase::get_resources()
{
	return &BC_WindowBase::resources;
}

BC_Synchronous* BC_WindowBase::get_synchronous()
{
	return BC_WindowBase::resources.get_synchronous();
}

int BC_WindowBase::get_bg_color()
{
	return bg_color;
}

void BC_WindowBase::set_bg_color(int color)
{
	this->bg_color = color;
}

BC_Pixmap* BC_WindowBase::get_bg_pixmap()
{
	return bg_pixmap;
}

void BC_WindowBase::set_active_subwindow(BC_WindowBase *subwindow)
{
	top_level->active_subwindow = subwindow;
}

int BC_WindowBase::activate()
{
	return 0;
}

int BC_WindowBase::deactivate()
{
	if(window_type == MAIN_WINDOW)
	{
		if( top_level->active_menubar ) {
			top_level->active_menubar->deactivate();
			top_level->active_menubar = 0;
		}
		if( top_level->active_popup_menu ) {
			top_level->active_popup_menu->deactivate();
			top_level->active_popup_menu = 0;
		}
		if( top_level->active_subwindow ) {
			top_level->active_subwindow->deactivate();
			top_level->active_subwindow = 0;
		}
		if( top_level->motion_events && top_level->last_motion_win == this->win )
			top_level->motion_events = 0;

	}
	return 0;
}

int BC_WindowBase::cycle_textboxes(int amount)
{
	int result = 0;
	BC_WindowBase *new_textbox = 0;

	if(amount > 0)
	{
		BC_WindowBase *first_textbox = 0;
		find_next_textbox(&first_textbox, &new_textbox, result);
		if(!new_textbox) new_textbox = first_textbox;

	}
	else
	if(amount < 0)
	{
		BC_WindowBase *last_textbox = 0;
		find_prev_textbox(&last_textbox, &new_textbox, result);
		if(!new_textbox) new_textbox = last_textbox;

	}

	if(new_textbox != active_subwindow)
	{
		deactivate();
		new_textbox->activate();
	}

	return 0;
}

int BC_WindowBase::find_next_textbox(BC_WindowBase **first_textbox, BC_WindowBase **next_textbox, int &result)
{
// Search subwindows for textbox
	for(int i = 0; i < subwindows->total && result < 2; i++)
	{
		BC_WindowBase *test_subwindow = subwindows->values[i];
		test_subwindow->find_next_textbox(first_textbox, next_textbox, result);
	}

	if(result < 2)
	{
		if(uses_text())
		{
			if(!*first_textbox) *first_textbox = this;

			if(result < 1)
			{
				if(top_level->active_subwindow == this)
					result++;
			}
			else
			{
				result++;
				*next_textbox = this;
			}
		}
	}
	return 0;
}

int BC_WindowBase::find_prev_textbox(BC_WindowBase **last_textbox, BC_WindowBase **prev_textbox, int &result)
{
	if(result < 2)
	{
		if(uses_text())
		{
			if(!*last_textbox) *last_textbox = this;

			if(result < 1)
			{
				if(top_level->active_subwindow == this)
					result++;
			}
			else
			{
				result++;
				*prev_textbox = this;
			}
		}
	}

// Search subwindows for textbox
	for(int i = subwindows->total - 1; i >= 0 && result < 2; i--)
	{
		BC_WindowBase *test_subwindow = subwindows->values[i];
		test_subwindow->find_prev_textbox(last_textbox, prev_textbox, result);
	}
	return 0;
}

BC_Clipboard* BC_WindowBase::get_clipboard()
{
#ifdef SINGLE_THREAD
	return BC_Display::display_global->clipboard;
#else
	return top_level->clipboard;
#endif
}

Atom BC_WindowBase::to_clipboard(const char *data, long len, int clipboard_num)
{
	return get_clipboard()->to_clipboard(this, data, len, clipboard_num);
}

long BC_WindowBase::from_clipboard(char *data, long maxlen, int clipboard_num)
{
	return get_clipboard()->from_clipboard(data, maxlen, clipboard_num);
}

long BC_WindowBase::clipboard_len(int clipboard_num)
{
	return get_clipboard()->clipboard_len(clipboard_num);
}

int BC_WindowBase::do_selection_clear(Window win)
{
	top_level->event_win = win;
	return dispatch_selection_clear();
}

int BC_WindowBase::dispatch_selection_clear()
{
	int result = 0;
	for( int i=0; i<subwindows->total && !result; ++i )
		result = subwindows->values[i]->dispatch_selection_clear();
	if( !result )
		result = selection_clear_event();
	return result;
}


void BC_WindowBase::get_relative_cursor(int &x, int &y, int lock_window)
{
	int abs_x, abs_y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

	if(lock_window) this->lock_window("BC_WindowBase::get_relative_cursor");
	XQueryPointer(top_level->display, top_level->win,
	   &temp_win, &temp_win, &abs_x, &abs_y, &win_x, &win_y,
	   &temp_mask);

	XTranslateCoordinates(top_level->display, top_level->rootwin,
	   win, abs_x, abs_y, &x, &y, &temp_win);
	if(lock_window) this->unlock_window();
}
int BC_WindowBase::get_relative_cursor_x(int lock_window)
{
	int x, y;
	get_relative_cursor(x, y, lock_window);
	return x;
}
int BC_WindowBase::get_relative_cursor_y(int lock_window)
{
	int x, y;
	get_relative_cursor(x, y, lock_window);
	return y;
}

void BC_WindowBase::get_abs_cursor(int &abs_x, int &abs_y, int lock_window)
{
	int win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

	if(lock_window) this->lock_window("BC_WindowBase::get_abs_cursor");
	XQueryPointer(top_level->display, top_level->win,
		&temp_win, &temp_win, &abs_x, &abs_y, &win_x, &win_y,
		&temp_mask);
	if(lock_window) this->unlock_window();
}
int BC_WindowBase::get_abs_cursor_x(int lock_window)
{
	int abs_x, abs_y;
	get_abs_cursor(abs_x, abs_y, lock_window);
	return abs_x;
}
int BC_WindowBase::get_abs_cursor_y(int lock_window)
{
	int abs_x, abs_y;
	get_abs_cursor(abs_x, abs_y, lock_window);
	return abs_y;
}

void BC_WindowBase::get_pop_cursor(int &px, int &py, int lock_window)
{
	int margin = 100;
	get_abs_cursor(px, py, lock_window);
	if( px < margin ) px = margin;
	if( py < margin ) py = margin;
	int wd = get_screen_w(lock_window,-1) - margin;
	if( px > wd ) px = wd;
	int ht = get_screen_h(lock_window,-1) - margin;
	if( py > ht ) py = ht;
}
int BC_WindowBase::get_pop_cursor_x(int lock_window)
{
	int px, py;
	get_pop_cursor(px, py, lock_window);
	return px;
}
int BC_WindowBase::get_pop_cursor_y(int lock_window)
{
	int px, py;
	get_pop_cursor(px, py, lock_window);
	return py;
}

int BC_WindowBase::match_window(Window win)
{
	if (this->win == win) return 1;
	int result = 0;
	for(int i = 0; i < subwindows->total; i++) {
		result = subwindows->values[i]->match_window(win);
		if (result) return result;
	}
	return 0;

}

int BC_WindowBase::get_cursor_over_window()
{
	int abs_x, abs_y, win_x, win_y;
	unsigned int mask_return;
	Window root_return, child_return;

	int ret = XQueryPointer(top_level->display, top_level->rootwin,
		&root_return, &child_return, &abs_x, &abs_y,
		&win_x, &win_y, &mask_return);
	if( ret && child_return == None ) ret = 0;
	if( ret && win != child_return )
		ret = top_level->match_window(child_return);
// query pointer can return a window manager window with this top_level as a child
//  for kde this can be two levels deep
	unsigned int nchildren_return = 0;
	Window parent_return, *children_return = 0;
	Window top_win = top_level->win;
	while( !ret && top_win != top_level->rootwin && top_win != root_return &&
		XQueryTree(top_level->display, top_win, &root_return,
			&parent_return, &children_return, &nchildren_return) ) {
		if( children_return ) XFree(children_return);
		if( (top_win=parent_return) == child_return ) ret = 1;
	}
	return ret;
}

int BC_WindowBase::cursor_above()
{
	int rx, ry;
	get_relative_cursor(rx, ry);
	return rx < 0 || rx >= get_w() ||
		ry < 0 || ry >= get_h() ? 0 : 1;
}

int BC_WindowBase::get_drag_x()
{
	return top_level->drag_x;
}

int BC_WindowBase::get_drag_y()
{
	return top_level->drag_y;
}

int BC_WindowBase::get_cursor_x()
{
	return top_level->cursor_x;
}

int BC_WindowBase::get_cursor_y()
{
	return top_level->cursor_y;
}

int BC_WindowBase::dump_windows()
{
	printf("\tBC_WindowBase::dump_windows window=%p win=%p '%s', %dx%d+%d+%d %s\n",
		this, (void*)this->win, title, w,h,x,y, typeid(*this).name());
	for(int i = 0; i < subwindows->size(); i++)
		subwindows->get(i)->dump_windows();
	for(int i = 0; i < popups.size(); i++) {
		BC_WindowBase *p = popups[i];
		printf("\tBC_WindowBase::dump_windows popup=%p win=%p '%s', %dx%d+%d+%d %s\n",
			p, (void*)p->win, p->title, p->w,p->h,p->x,p->y, typeid(*p).name());
	}
	return 0;
}

int BC_WindowBase::is_event_win()
{
	return this->win == top_level->event_win;
}

void BC_WindowBase::set_dragging(int value)
{
	is_dragging = value;
}

int BC_WindowBase::get_dragging()
{
	return is_dragging;
}

int BC_WindowBase::get_buttonpress()
{
	return top_level->button_number;
}

int BC_WindowBase::get_button_down()
{
	return top_level->button_down;
}

int BC_WindowBase::alt_down()
{
	return top_level->alt_mask;
}

int BC_WindowBase::shift_down()
{
	return top_level->shift_mask;
}

int BC_WindowBase::ctrl_down()
{
	return top_level->ctrl_mask;
}

wchar_t* BC_WindowBase::get_wkeystring(int *length)
{
	if(length)
		*length = top_level->wkey_string_length;
	return top_level->wkey_string;
}

#ifdef X_HAVE_UTF8_STRING
char* BC_WindowBase::get_keypress_utf8()
{
	return top_level->key_pressed_utf8;
}
#endif


int BC_WindowBase::get_keypress()
{
	return top_level->key_pressed;
}

int BC_WindowBase::get_double_click()
{
	return top_level->double_click;
}

int BC_WindowBase::get_triple_click()
{
	return top_level->triple_click;
}

int BC_WindowBase::get_bgcolor()
{
	return bg_color;
}

int BC_WindowBase::resize_window(int w, int h)
{
	if(this->w == w && this->h == h) return 0;

	if(window_type == MAIN_WINDOW && !allow_resize)
	{
		XSizeHints size_hints;
		size_hints.flags = PSize | PMinSize | PMaxSize;
		size_hints.width = w;
		size_hints.height = h;
		size_hints.min_width = w;
		size_hints.max_width = w;
		size_hints.min_height = h;
		size_hints.max_height = h;
		XSetNormalHints(top_level->display, win, &size_hints);
	}
	XResizeWindow(top_level->display, win, w, h);

	this->w = w;
	this->h = h;
	delete pixmap;
	pixmap = new BC_Pixmap(this, w, h);

// Propagate to menubar
	for(int i = 0; i < subwindows->total; i++)
	{
		subwindows->values[i]->dispatch_resize_event(w, h);
	}

	draw_background(0, 0, w, h);
	if(top_level == this && get_resources()->recursive_resizing)
		resize_history.append(new BC_ResizeCall(w, h));
	return 0;
}

// The only way for resize events to be propagated is by updating the internal w and h
int BC_WindowBase::resize_event(int w, int h)
{
	if(window_type == MAIN_WINDOW)
	{
		this->w = w;
		this->h = h;
	}
	return 0;
}

int BC_WindowBase::reposition_window(int x, int y)
{
	reposition_window(x, y, -1, -1);
	return 0;
}


int BC_WindowBase::reposition_window(int x, int y, int w, int h)
{
	int resize = 0;

// Some tools set their own dimensions before calling this, causing the
// resize check to skip.
	this->x = x;
	this->y = y;

	if(w > 0 && w != this->w)
	{
		resize = 1;
		this->w = w;
	}

	if(h > 0 && h != this->h)
	{
		resize = 1;
		this->h = h;
	}

//printf("BC_WindowBase::reposition_window %d %d %d\n", translation_count, x_correction, y_correction);

	if(this->w <= 0)
		printf("BC_WindowBase::reposition_window this->w == %d\n", this->w);
	if(this->h <= 0)
		printf("BC_WindowBase::reposition_window this->h == %d\n", this->h);

	if(translation_count && window_type == MAIN_WINDOW)
	{
// KDE shifts window right and down.
// FVWM leaves window alone and adds border around it.
		XMoveResizeWindow(top_level->display,
			win,
			x - BC_DisplayInfo::auto_reposition_x,
			y - BC_DisplayInfo::auto_reposition_y,
			this->w,
			this->h);
	}
	else
	{
		XMoveResizeWindow(top_level->display,
			win,
			x,
			y,
			this->w,
			this->h);
	}

	if(resize)
	{
		delete pixmap;
		pixmap = new BC_Pixmap(this, this->w, this->h);
		clear_box(0,0, this->w, this->h);
// Propagate to menubar
		for(int i = 0; i < subwindows->total; i++)
		{
			subwindows->values[i]->dispatch_resize_event(this->w, this->h);
		}

//		draw_background(0, 0, w, h);
	}

	return 0;
}

int BC_WindowBase::reposition_window_relative(int dx, int dy, int w, int h)
{
	return reposition_window(get_x()+dx, get_y()+dy, w, h);
}

int BC_WindowBase::reposition_window_relative(int dx, int dy)
{
	return reposition_window_relative(dx, dy, -1, -1);
}

void BC_WindowBase::set_tooltips(int v)
{
	get_resources()->tooltips_enabled = v;
}

void BC_WindowBase::set_force_tooltip(int v)
{
	force_tooltip = v;
}

int BC_WindowBase::raise_window(int do_flush)
{
	XRaiseWindow(top_level->display, win);
	if(do_flush) XFlush(top_level->display);
	return 0;
}

int BC_WindowBase::lower_window(int do_flush)
{
	XLowerWindow(top_level->display, win);
	if(do_flush) XFlush(top_level->display);
	return 0;
}

void BC_WindowBase::set_background(VFrame *bitmap)
{
	if(bg_pixmap && !shared_bg_pixmap) delete bg_pixmap;

	bg_pixmap = new BC_Pixmap(this, bitmap, PIXMAP_OPAQUE);
	shared_bg_pixmap = 0;
	draw_background(0, 0, w, h);
}

void BC_WindowBase::put_title(const char *text)
{
	char *cp = this->title, *ep = cp+sizeof(this->title)-1;
	for( const unsigned char *bp = (const unsigned char *)text; *bp && cp<ep; ++bp )
		*cp++ = *bp >= ' ' ? *bp : ' ';
	*cp = 0;
}

void BC_WindowBase::set_title(const char *text, int utf8)
{
// utf8>0: wm + net_wm, utf8=0: wm only,  utf<0: net_wm only
	put_title(text);
	const unsigned char *wm_title = (const unsigned char *)title;
	int title_len = strlen((const char *)title);
	if( utf8 >= 0 ) {
		Atom xa_wm_name = XA_WM_NAME;
		Atom xa_icon_name = XA_WM_ICON_NAME;
		Atom xa_string = XA_STRING;
		XChangeProperty(display, win, xa_wm_name, xa_string, 8,
				PropModeReplace, wm_title, title_len);
		XChangeProperty(display, win, xa_icon_name, xa_string, 8,
				PropModeReplace, wm_title, title_len);
	}
	if( utf8 != 0 ) {
		Atom xa_net_wm_name = XInternAtom(display, "_NET_WM_NAME", True);
		Atom xa_net_icon_name = XInternAtom(display, "_NET_WM_ICON_NAME", True);
		Atom xa_utf8_string = XInternAtom(display, "UTF8_STRING", True);
		XChangeProperty(display, win, xa_net_wm_name, xa_utf8_string, 8,
					PropModeReplace, wm_title, title_len);
		XChangeProperty(display, win, xa_net_icon_name, xa_utf8_string, 8,
					PropModeReplace, wm_title, title_len);
	}
	flush();
}

const char *BC_WindowBase::get_title()
{
	return title;
}

int BC_WindowBase::get_toggle_value()
{
	return toggle_value;
}

int BC_WindowBase::get_toggle_drag()
{
	return toggle_drag;
}

int BC_WindowBase::set_icon(VFrame *data)
{
	if(icon_pixmap) delete icon_pixmap;
	icon_pixmap = new BC_Pixmap(top_level, data, PIXMAP_ALPHA, 1);

	if(icon_window) delete icon_window;
	icon_window = new BC_Popup(this,
		(int)BC_INFINITY,
		(int)BC_INFINITY,
		icon_pixmap->get_w(),
		icon_pixmap->get_h(),
		-1,
		1, // All windows are hidden initially
		icon_pixmap);

	XWMHints wm_hints;
	wm_hints.flags = WindowGroupHint | IconPixmapHint | IconMaskHint | IconWindowHint;
	wm_hints.icon_pixmap = icon_pixmap->get_pixmap();
	wm_hints.icon_mask = icon_pixmap->get_alpha();
	wm_hints.icon_window = icon_window->win;
	wm_hints.window_group = XGroupLeader;

// for(int i = 0; i < 1000; i++)
// printf("02x ", icon_pixmap->get_alpha()->get_row_pointers()[0][i]);
// printf("\n");

	XSetWMHints(top_level->display, top_level->win, &wm_hints);
	XSync(top_level->display, 0);
	return 0;
}

int BC_WindowBase::set_w(int w)
{
	this->w = w;
	return 0;
}

int BC_WindowBase::set_h(int h)
{
	this->h = h;
	return 0;
}

int BC_WindowBase::load_defaults(BC_Hash *defaults)
{
	BC_Resources *resources = get_resources();
	char string[BCTEXTLEN];
	int newest_id = - 1;
	for(int i = 0; i < FILEBOX_HISTORY_SIZE; i++)
	{
		sprintf(string, "FILEBOX_HISTORY_PATH%d", i);
		resources->filebox_history[i].path[0] = 0;
		defaults->get(string, resources->filebox_history[i].path);
		sprintf(string, "FILEBOX_HISTORY_ID%d", i);
		resources->filebox_history[i].id = defaults->get(string, resources->get_id());
		if(resources->filebox_history[i].id > newest_id)
			newest_id = resources->filebox_history[i].id;
	}

	resources->filebox_id = newest_id + 1;
	resources->filebox_mode = defaults->get("FILEBOX_MODE", get_resources()->filebox_mode);
	resources->filebox_w = defaults->get("FILEBOX_W", get_resources()->filebox_w);
	resources->filebox_h = defaults->get("FILEBOX_H", get_resources()->filebox_h);
	resources->filebox_columntype[0] = defaults->get("FILEBOX_TYPE0", resources->filebox_columntype[0]);
	resources->filebox_columntype[1] = defaults->get("FILEBOX_TYPE1", resources->filebox_columntype[1]);
	resources->filebox_columntype[2] = defaults->get("FILEBOX_TYPE2", resources->filebox_columntype[2]);
	resources->filebox_columntype[3] = defaults->get("FILEBOX_TYPE3", resources->filebox_columntype[3]);
	resources->filebox_columnwidth[0] = defaults->get("FILEBOX_WIDTH0", resources->filebox_columnwidth[0]);
	resources->filebox_columnwidth[1] = defaults->get("FILEBOX_WIDTH1", resources->filebox_columnwidth[1]);
	resources->filebox_columnwidth[2] = defaults->get("FILEBOX_WIDTH2", resources->filebox_columnwidth[2]);
	resources->filebox_columnwidth[3] = defaults->get("FILEBOX_WIDTH3", resources->filebox_columnwidth[3]);
	resources->filebox_size_format = defaults->get("FILEBOX_SIZE_FORMAT", get_resources()->filebox_size_format);
	defaults->get("FILEBOX_FILTER", resources->filebox_filter);
	return 0;
}

int BC_WindowBase::save_defaults(BC_Hash *defaults)
{
	BC_Resources *resources = get_resources();
	char string[BCTEXTLEN];
	for(int i = 0; i < FILEBOX_HISTORY_SIZE; i++)
	{
		sprintf(string, "FILEBOX_HISTORY_PATH%d", i);
		defaults->update(string, resources->filebox_history[i].path);
		sprintf(string, "FILEBOX_HISTORY_ID%d", i);
		defaults->update(string, resources->filebox_history[i].id);
	}
	defaults->update("FILEBOX_MODE", resources->filebox_mode);
	defaults->update("FILEBOX_W", resources->filebox_w);
	defaults->update("FILEBOX_H", resources->filebox_h);
	defaults->update("FILEBOX_TYPE0", resources->filebox_columntype[0]);
	defaults->update("FILEBOX_TYPE1", resources->filebox_columntype[1]);
	defaults->update("FILEBOX_TYPE2", resources->filebox_columntype[2]);
	defaults->update("FILEBOX_TYPE3", resources->filebox_columntype[3]);
	defaults->update("FILEBOX_WIDTH0", resources->filebox_columnwidth[0]);
	defaults->update("FILEBOX_WIDTH1", resources->filebox_columnwidth[1]);
	defaults->update("FILEBOX_WIDTH2", resources->filebox_columnwidth[2]);
	defaults->update("FILEBOX_WIDTH3", resources->filebox_columnwidth[3]);
	defaults->update("FILEBOX_FILTER", resources->filebox_filter);
	defaults->update("FILEBOX_SIZE_FORMAT", get_resources()->filebox_size_format);
	return 0;
}



// For some reason XTranslateCoordinates can take a long time to return.
// We work around this by only calling it when the event windows are different.
void BC_WindowBase::translate_coordinates(Window src_w,
		Window dest_w,
		int src_x,
		int src_y,
		int *dest_x_return,
		int *dest_y_return)
{
	Window tempwin = 0;
//Timer timer;
//timer.update();
	if(src_w == dest_w)
	{
		*dest_x_return = src_x;
		*dest_y_return = src_y;
	}
	else
	{
		XTranslateCoordinates(top_level->display,
			src_w,
			dest_w,
			src_x,
			src_y,
			dest_x_return,
			dest_y_return,
			&tempwin);
//printf("BC_WindowBase::translate_coordinates 1 %lld\n", timer.get_difference());
	}
}

void BC_WindowBase::get_root_coordinates(int x, int y, int *abs_x, int *abs_y)
{
	translate_coordinates(win, top_level->rootwin, x, y, abs_x, abs_y);
}

void BC_WindowBase::get_win_coordinates(int abs_x, int abs_y, int *x, int *y)
{
	translate_coordinates(top_level->rootwin, win, abs_x, abs_y, x, y);
}






#ifdef HAVE_LIBXXF86VM
void BC_WindowBase::closest_vm(int *vm, int *width, int *height)
{
	int foo,bar;
	*vm = 0;
	if(XF86VidModeQueryExtension(top_level->display,&foo,&bar)) {
		int vm_count,i;
		XF86VidModeModeInfo **vm_modelines;
		XF86VidModeGetAllModeLines(top_level->display,
			XDefaultScreen(top_level->display), &vm_count,&vm_modelines);
		for( i = 0; i < vm_count; i++ ) {
			if( vm_modelines[i]->hdisplay < vm_modelines[*vm]->hdisplay &&
			    vm_modelines[i]->hdisplay >= *width )
				*vm = i;
		}
		display = top_level->display;
		if( vm_modelines[*vm]->hdisplay == *width )
			*vm = -1;
		else {
			*width = vm_modelines[*vm]->hdisplay;
			*height = vm_modelines[*vm]->vdisplay;
		}
	}
}

void BC_WindowBase::scale_vm(int vm)
{
	int foo,bar,dotclock;
	if( XF86VidModeQueryExtension(top_level->display,&foo,&bar) ) {
		int vm_count;
		XF86VidModeModeInfo **vm_modelines;
		XF86VidModeModeLine vml;
		XF86VidModeGetAllModeLines(top_level->display,
			XDefaultScreen(top_level->display), &vm_count,&vm_modelines);
		XF86VidModeGetModeLine(top_level->display,
			XDefaultScreen(top_level->display), &dotclock,&vml);
		orig_modeline.dotclock = dotclock;
		orig_modeline.hdisplay = vml.hdisplay;
		orig_modeline.hsyncstart = vml.hsyncstart;
		orig_modeline.hsyncend = vml.hsyncend;
		orig_modeline.htotal = vml.htotal;
		orig_modeline.vdisplay = vml.vdisplay;
		orig_modeline.vsyncstart = vml.vsyncstart;
		orig_modeline.vsyncend = vml.vsyncend;
		orig_modeline.vtotal = vml.vtotal;
		orig_modeline.flags = vml.flags;
		orig_modeline.privsize = vml.privsize;
		// orig_modeline.private = vml.private;
		XF86VidModeSwitchToMode(top_level->display,XDefaultScreen(top_level->display),vm_modelines[vm]);
		XF86VidModeSetViewPort(top_level->display,XDefaultScreen(top_level->display),0,0);
		XFlush(top_level->display);
	}
}

void BC_WindowBase::restore_vm()
{
	XF86VidModeSwitchToMode(top_level->display,XDefaultScreen(top_level->display),&orig_modeline);
	XFlush(top_level->display);
}
#endif


#ifndef SINGLE_THREAD
int BC_WindowBase::get_event_count()
{
	event_lock->lock("BC_WindowBase::get_event_count");
	int result = common_events.total;
	event_lock->unlock();
	return result;
}

XEvent* BC_WindowBase::get_event()
{
	XEvent *result = 0;
	while(!done && !result)
	{
		event_condition->lock("BC_WindowBase::get_event");
		event_lock->lock("BC_WindowBase::get_event");

		if(common_events.total && !done)
		{
			result = common_events.values[0];
			common_events.remove_number(0);
		}

		event_lock->unlock();
	}
	return result;
}

void BC_WindowBase::put_event(XEvent *event)
{
	event_lock->lock("BC_WindowBase::put_event");
	common_events.append(event);
	event_lock->unlock();
	event_condition->unlock();
}

void BC_WindowBase::dequeue_events(Window win)
{
	event_lock->lock("BC_WindowBase::dequeue_events");

	int out = 0, total = common_events.size();
	for( int in=0; in<total; ++in ) {
		if( common_events[in]->xany.window == win ) continue;
		common_events[out++] = common_events[in];
	}
	common_events.total = out;

	event_lock->unlock();
}

int BC_WindowBase::resend_event(BC_WindowBase *window)
{
	if( resend_event_window ) return 1;
	resend_event_window = window;
	return 0;
}

#else

int BC_WindowBase::resend_event(BC_WindowBase *window)
{
	return 1;
}

#endif // SINGLE_THREAD

int BC_WindowBase::get_id()
{
	return id;
}


BC_Pixmap *BC_WindowBase::create_pixmap(VFrame *vframe)
{
	int w = vframe->get_w(), h = vframe->get_h();
	BC_Pixmap *icon = new BC_Pixmap(this, w, h);
	icon->draw_vframe(vframe, 0,0, w,h, 0,0);
	return icon;
}


void BC_WindowBase::flicker(int n, int ms)
{
	int color = get_bg_color();
	for( int i=2*n; --i>=0; ) {
		set_inverse();		set_bg_color(WHITE);
		clear_box(0,0, w,h);	flash(1);
		sync_display();		Timer::delay(ms);
	}
	set_bg_color(color);
	set_opaque();
}

void BC_WindowBase::focus()
{
	XWindowAttributes xwa;
	XGetWindowAttributes(top_level->display, top_level->win, &xwa);
	if( xwa.map_state == IsViewable )
		XSetInputFocus(top_level->display, top_level->win, RevertToParent, CurrentTime);
}

