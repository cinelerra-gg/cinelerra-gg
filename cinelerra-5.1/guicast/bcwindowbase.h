
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

#ifndef BCWINDOWBASE_H
#define BCWINDOWBASE_H

// Window types
#define MAIN_WINDOW 0
#define SUB_WINDOW 1
#define POPUP_WINDOW 2

#ifdef HAVE_LIBXXF86VM
#define VIDMODE_SCALED_WINDOW 3
#endif

#define TOOLTIP_MARGIN 2
#define BC_INFINITY 65536

#include "arraylist.h"
#include "bcbar.inc"
#include "bcbitmap.inc"
#include "bcbutton.inc"
#include "bccapture.inc"
#include "bcclipboard.inc"
#include "bccmodels.inc"
#include "bcdisplay.inc"
#include "bcdragwindow.inc"
#include "bcfilebox.inc"
#include "bckeyboard.h"
#include "bclistbox.inc"
#include "bcmenubar.inc"
#include "bcmeter.inc"
#include "bcpan.inc"
#include "bcpbuffer.inc"
#include "bcpixmap.inc"
#include "bcpopup.inc"
#include "bcpopupmenu.inc"
#include "bcpot.inc"
#include "bcprogress.inc"
#include "bcrepeater.inc"
#include "bcresources.inc"
#include "bcscrollbar.inc"
#include "bcslider.inc"
#include "bcsubwindow.inc"
#include "bcsynchronous.inc"
#include "bctextbox.inc"
#include "bctimer.inc"
#include "bctitle.inc"
#include "bctoggle.inc"
#include "bctrace.inc"
#include "bctumble.inc"
#include "bcwindow.inc"
#include "bcwindowbase.inc"
#include "bcwindowevents.inc"
#include "condition.inc"
#include "bchash.inc"
#include "linklist.h"
#include "mutex.inc"
#include "vframe.inc"


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
#ifdef HAVE_LIBXXF86VM
#include <X11/extensions/xf86vmode.h>
#endif
#include <X11/extensions/Xinerama.h>
#ifdef HAVE_GL
#define GL_GLEXT_PROTOTYPES
#include <GL/glx.h>
#endif



#ifdef HAVE_GL
//typedef void* GLXContext;
#endif

class BC_ResizeCall
{
public:
	BC_ResizeCall(int w, int h);
	int w, h;
};

typedef XClientMessageEvent xatom_event;


class BC_ActiveBitmaps : public List<BC_BitmapImage> {
	Mutex active_lock;
public:
	void reque(XEvent *ev);
	void insert(BC_BitmapImage *image, Drawable pixmap);
	void remove_buffers(BC_WindowBase *wdw);

	BC_ActiveBitmaps();
	~BC_ActiveBitmaps();
};


// Windows, subwindows, popupwindows inherit from this
class BC_WindowBase : public trace_info
{
public:
	BC_WindowBase();
	virtual ~BC_WindowBase();

	friend class BC_Bar;
	friend class BC_BitmapImage;
	friend class BC_XImage;
	friend class BC_XShmImage;
	friend class BC_XvImage;
	friend class BC_XvShmImage;
	friend class BC_Bitmap;
	friend class BC_Button;
	friend class BC_GenericButton;
	friend class BC_Capture;
	friend class BC_Clipboard;
	friend class BC_Display;
	friend class BC_DragWindow;
	friend class BC_FileBox;
	friend class BC_FullScreen;
	friend class BC_ListBox;
	friend class BC_Menu;
	friend class BC_MenuBar;
	friend class BC_MenuItem;
	friend class BC_MenuPopup;
	friend class BC_Meter;
	friend class BC_Pan;
	friend class BC_PBuffer;
	friend class BC_Pixmap;
	friend class BC_PixmapSW;
	friend class BC_Popup;
	friend class BC_PopupMenu;
	friend class BC_Pot;
	friend class BC_ProgressBar;
	friend class BC_Repeater;
	friend class BC_Resources;
	friend class BC_ScrollBar;
	friend class BC_Slider;
	friend class BC_SubWindow;
	friend class BC_Synchronous;
	friend class BC_TextBox;
	friend class BC_Title;
	friend class BC_Toggle;
	friend class BC_Tumbler;
	friend class BC_Window;
	friend class BC_WindowEvents;
	friend class Shuttle;

// Main loop
	int run_window();
// Terminal event dispatchers
	virtual int close_event();
	virtual int resize_event(int w, int h);
	virtual int repeat_event(int64_t duration) { return 0; };
	virtual int focus_in_event() { return 0; };
	virtual int focus_out_event() { return 0; };
	virtual int button_press_event() { return 0; };
	virtual int button_release_event() { return 0; };
	virtual int cursor_motion_event() { return 0; };
	virtual int cursor_leave_event();
	virtual int cursor_enter_event();
	virtual int keypress_event() { return 0; };
	virtual int keyrelease_event() { return 0; };
	virtual int translation_event() { return 0; };
	virtual int drag_start_event() { return 0; };
	virtual int drag_motion_event() { return 0; };
	virtual int drag_stop_event() { return 0; };
	virtual int uses_text() { return 0; };
	virtual int selection_clear_event() { return 0; }
// Only if opengl is enabled
	virtual int expose_event() { return 0; };
	virtual int grab_event(XEvent *event) { return 0; };
	virtual void create_objects() { return; };

	int get_window_type() { return window_type; }
// Wait until event loop is running
	void init_wait();
	int is_running() { return window_running; }
	int is_hidden() { return hidden; }
// Check if a hardware accelerated colormodel is available and reserve it
	int accel_available(int color_model, int lock_it);
	void get_input_context();
	void close_input_context();
// Get color model adjusted for byte order and pixel size
	int get_color_model();
// return the colormap pixel of the color for all bit depths
	int get_color(int64_t color);
// return the currently selected color
	int64_t get_color();
	virtual int show_window(int flush = 1);
	virtual int hide_window(int flush = 1);
	int get_hidden();
	int get_video_on();
// Shouldn't deference a pointer to delete a window if a parent is
// currently being deleted.  This returns 1 if any parent is being deleted.
	int get_deleting();


//============================= OpenGL functions ===============================
// OpenGL functions must be called from inside a BC_Synchronous command.
// Create openGL context and bind it to the current window.
// If it's called inside start_video/stop_video, the context is bound to the window.
// If it's called outside start_video/stop_video, the context is bound to the pixmap.
// Must be called at the beginning of any opengl routine to make sure
// the context is current.
// No locking is performed.
	void enable_opengl();
	void disable_opengl();
	void flip_opengl();

// Calls the BC_Synchronous version of the function with the window_id.
// Not run in OpenGL thread because it has its own lock.
	unsigned int get_shader(char *title, int *got_it);
	void put_shader(unsigned int handle, char *title);
	int get_opengl_server_version();

	int flash(int x, int y, int w, int h, int flush = 1);
	int flash(int flush = 1);
	void flush();
	void sync_display();
// Lock out other threads
	int lock_window(const char *location = 0);
	int unlock_window();
	int get_window_lock();
	int break_lock();

	BC_MenuBar* add_menubar(BC_MenuBar *menu_bar);
	BC_WindowBase* add_subwindow(BC_WindowBase *subwindow);
	BC_WindowBase* add_tool(BC_WindowBase *subwindow);

// Use this to get events for the popup window.
// Events are not propagated to the popup window.
	BC_WindowBase* add_popup(BC_WindowBase *window);
	void remove_popup(BC_WindowBase *window);

	static BC_Resources* get_resources();
// User must create synchronous object first
	static BC_Synchronous* get_synchronous();
	static int shm_completion_event;
// bckeyboard / remote control
	virtual int keyboard_listener(BC_WindowBase *wp) { return 0; }
	void add_keyboard_listener(int(BC_WindowBase::*handler)(BC_WindowBase *));
	void del_keyboard_listener(int(BC_WindowBase::*handler)(BC_WindowBase *));
	int resend_event(BC_WindowBase *window);
// Dimensions
	virtual int get_w() { return w; }
	virtual int get_h() { return h; }
	virtual int get_x() { return x; }
	virtual int get_y() { return y; }
	int get_root_w(int lock_display);
	int get_root_h(int lock_display);
	XineramaScreenInfo *get_xinerama_info(int screen);
	void get_fullscreen_geometry(int &wx, int &wy, int &ww, int &wh);
	int get_screen_w(int lock_display, int screen);
	int get_screen_h(int lock_display, int screen);
	int get_screen_x(int lock_display, int screen);
	int get_screen_y(int lock_display, int screen);
// Get current position
	void get_abs_cursor(int &abs_x, int &abs_y, int lock_window=0);
	int get_abs_cursor_x(int lock_window=0);
	int get_abs_cursor_y(int lock_window=0);
	void get_pop_cursor(int &px, int &py, int lock_window=0);
	int get_pop_cursor_x(int lock_window=0);
	int get_pop_cursor_y(int lock_window=0);
	void get_relative_cursor(int &x, int &y, int lock_window=0);
	int get_relative_cursor_x(int lock_window=0);
	int get_relative_cursor_y(int lock_window=0);
	void get_root_coordinates(int x, int y, int *abs_x, int *abs_y);
	void get_win_coordinates(int abs_x, int abs_y, int *x, int *y);
// Return 1 if cursor is over an unobscured part of this window.
// An argument is provided for excluding a drag popup
	int get_cursor_over_window();
// Return 1 if cursor is above/inside window
	int cursor_above();
// For traversing windows... return 1 if this or any subwindow is win
 	int match_window(Window win);

// 1 or 0 if a button is down
	int get_button_down();
// Number of button pressed 1 - 5
	int get_buttonpress();
	int get_has_focus();
	int get_dragging();
	wchar_t* get_wkeystring(int *length = 0);
	int get_keypress();
	int get_keysym() { return keysym; }
#ifdef X_HAVE_UTF8_STRING
	char* get_keypress_utf8();
#endif
	int keysym_lookup(XEvent *event);
// Get cursor position of last event
	int get_cursor_x();
	int get_cursor_y();
// Cursor position of drag start
	int get_drag_x();
	int get_drag_y();
	int alt_down();
	int shift_down();
	int ctrl_down();
	int get_double_click();
	int get_triple_click();
// Bottom right corner
	int get_x2();
	int get_y2();
	int get_bg_color();
	void set_bg_color(int color);
	BC_Pixmap* get_bg_pixmap();
	int get_text_ascent(int font);
	int get_text_descent(int font);
	int get_text_height(int font, const char *text = 0);
	int get_text_width(int font, const char *text, int length = -1);
	int get_text_width(int font, const wchar_t *text, int length = -1);
	BC_Clipboard* get_clipboard();
	void set_dragging(int value);
	int set_w(int w);
	int set_h(int h);
	BC_WindowBase* get_top_level();
	BC_WindowBase* get_parent();
// Event happened in this window
	int is_event_win();
	int cursor_inside();
// Deactivate everything and activate this subwindow
	virtual int activate();
// Deactivate this subwindow
	virtual int deactivate();
	void set_active_subwindow(BC_WindowBase *subwindow);
// Get value of toggle value when dragging a selection
	int get_toggle_value();
// Get if toggle is being dragged
	int get_toggle_drag();

// Set the gc to the color
	void set_color(int64_t color);
	void set_line_width(int value);
	void set_line_dashes(int value);
	int get_bgcolor();
	int get_current_font();
	void set_font(int font);
// Set the cursor to a macro from cursors.h
// Set override if the caller is enabling hourglass or hiding the cursor
	void set_cursor(int cursor, int override /* = 0 */, int flush);
// Set the cursor to a character in the X cursor library.  Used by test.C
	void set_x_cursor(int cursor);
	int get_cursor();
// Shows the cursor after it's hidden by video playback
	void unhide_cursor();
// Called by video updating routines to hide the cursor after a timeout
	void update_video_cursor();

// Entry point for starting hourglass.
// Converts all cursors and saves the previous cursor.
	void start_hourglass();
	void stop_hourglass();

// Recursive part of hourglass commands.
	void start_hourglass_recursive();
	void stop_hourglass_recursive();
// images, picons...
	BC_Pixmap *create_pixmap(VFrame *vframe);

// Drawing
	void copy_area(int x1, int y1, int x2, int y2, int w, int h, BC_Pixmap *pixmap = 0);
	void clear_box(int x, int y, int w, int h, BC_Pixmap *pixmap = 0);
	void draw_box(int x, int y, int w, int h, BC_Pixmap *pixmap = 0);
	void draw_circle(int x, int y, int w, int h, BC_Pixmap *pixmap = 0);
	void draw_arc(int x, int y, int w, int h,
		int start_angle, int angle_length, BC_Pixmap *pixmap = 0);
	void draw_disc(int x, int y, int w, int h, BC_Pixmap *pixmap = 0);
	void draw_text(int x, int y, const char *text, int length = -1, BC_Pixmap *pixmap = 0);
	void draw_utf8_text(int x, int y, const char *text, int length = -1, BC_Pixmap *pixmap = 0);
	void draw_text_line(int x, int y, const char *text, int len, BC_Pixmap *pixmap = 0);
	void draw_xft_text(int x, int y, const char *text, int len,
		BC_Pixmap *pixmap = 0, int is_utf8 = 0);
	void draw_xft_text(int x, int y, const wchar_t *text,
		int length, BC_Pixmap *pixmap);
	int draw_single_text(int draw, int font,
		int x, int y, const wchar_t *text, int length = -1, BC_Pixmap *pixmap = 0);
// truncate the text to a ... version that fits in the width, using the current_font
	void truncate_text(char *result, const char *text, int w);
	void draw_center_text(int x, int y, const char *text, int length = -1);
	void draw_line(int x1, int y1, int x2, int y2, BC_Pixmap *pixmap = 0);
	void draw_polygon(ArrayList<int> *x, ArrayList<int> *y, BC_Pixmap *pixmap = 0);
	void fill_polygon(ArrayList<int> *x, ArrayList<int> *y, BC_Pixmap *pixmap = 0);
	void draw_rectangle(int x, int y, int w, int h);
	void draw_3segment(int x, int y, int w, int h, BC_Pixmap *left_image,
		BC_Pixmap *mid_image, BC_Pixmap *right_image, BC_Pixmap *pixmap = 0);
	void draw_3segment(int x, int y, int w, int h, VFrame *left_image,
		VFrame *mid_image, VFrame *right_image, BC_Pixmap *pixmap = 0);
// For drawing a changing level
	void draw_3segmenth(int x, int y, int w, int total_x, int total_w,
		VFrame *image, BC_Pixmap *pixmap);
	void draw_3segmenth(int x, int y, int w, int total_x, int total_w,
		BC_Pixmap *src, BC_Pixmap *dst = 0);
	void draw_3segmentv(int x, int y, int h, int total_y, int total_h,
		BC_Pixmap *src, BC_Pixmap *dst = 0);
	void draw_3segmentv(int x, int y, int h, int total_y, int total_h,
		VFrame *src, BC_Pixmap *dst = 0);
// For drawing a single level
	void draw_3segmenth(int x, int y, int w, VFrame *image, BC_Pixmap *pixmap = 0);
	void draw_3segmenth(int x, int y, int w, BC_Pixmap *src, BC_Pixmap *dst = 0);
	void draw_3segmentv(int x, int y, int h, BC_Pixmap *src, BC_Pixmap *dst = 0);
	void draw_3segmentv(int x, int y, int h, VFrame *src, BC_Pixmap *dst = 0);
	void draw_9segment(int x, int y, int w, int h, VFrame *src, BC_Pixmap *dst = 0);
	void draw_9segment(int x, int y, int w, int h, BC_Pixmap *src, BC_Pixmap *dst = 0);
	void draw_3d_box(int x, int y, int w, int h, int light1, int light2,
		int middle, int shadow1, int shadow2, BC_Pixmap *pixmap = 0);
	void draw_3d_box(int x, int y, int w, int h, int is_down, BC_Pixmap *pixmap = 0);
	void draw_3d_border(int x, int y, int w, int h,
		int light1, int light2, int shadow1, int shadow2);
	void draw_3d_border(int x, int y, int w, int h, int is_down);
	void draw_colored_box(int x, int y, int w, int h, int down, int highlighted);
	void draw_check(int x, int y);
	void draw_triangle_down_flat(int x, int y, int w, int h);
	void draw_triangle_up(int x, int y, int w, int h,
		int light1, int light2, int middle, int shadow1, int shadow2);
	void draw_triangle_down(int x, int y, int w, int h,
		int light1, int light2, int middle, int shadow1, int shadow2);
	void draw_triangle_left(int x, int y, int w, int h,
		int light1, int light2, int middle, int shadow1, int shadow2);
	void draw_triangle_right(int x, int y, int w, int h,
		int light1, int light2, int middle, int shadow1, int shadow2);
// Set the gc to opaque
	void set_opaque();
	void set_inverse();
	void set_background(VFrame *bitmap);
// Change the window title.
	void put_title(const char *text);
	void set_title(const char *text, int utf8=1);
	const char *get_title();
// Change the window title.  The title is translated internally.
	void start_video();
	void stop_video();
	int get_id();
	void set_done(int return_value);
	void close(int return_value);
// Reroute toplevel events
	int grab(BC_WindowBase *window);
	int ungrab(BC_WindowBase *window);
	int grab_event_count();
// Grab button events
	int grab_buttons();
	void ungrab_buttons();
	void grab_cursor();
	void ungrab_cursor();
// Get a bitmap to draw on the window with
	BC_Bitmap* new_bitmap(int w, int h, int color_model = -1);
// Draw a bitmap on the window
	void draw_bitmap(BC_Bitmap *bitmap, int dont_wait,
		int dest_x = 0, int dest_y = 0, int dest_w = 0, int dest_h = 0,
		int src_x = 0, int src_y = 0, int src_w = 0, int src_h = 0,
		BC_Pixmap *pixmap = 0);
	void draw_pixel(int x, int y, BC_Pixmap *pixmap = 0);
// Draw a pixmap on the window
	void draw_pixmap(BC_Pixmap *pixmap,
		int dest_x = 0, int dest_y = 0, int dest_w = -1, int dest_h = -1,
		int src_x = 0, int src_y = 0, BC_Pixmap *dst = 0);
// Draw a vframe on the window
	void draw_vframe(VFrame *frame,
		int dest_x = 0, int dest_y = 0, int dest_w = -1, int dest_h = -1,
		int src_x = 0, int src_y = 0, int src_w = 0, int src_h = 0,
		BC_Pixmap *pixmap = 0);
	void draw_border(char *text, int x, int y, int w, int h);
// Draw a region of the background
	void draw_top_background(BC_WindowBase *parent_window, int x, int y, int w, int h, BC_Pixmap *pixmap = 0);
	void draw_top_tiles(BC_WindowBase *parent_window, int x, int y, int w, int h);
	void draw_background(int x, int y, int w, int h);
	void draw_tiles(BC_Pixmap *tile, int origin_x, int origin_y,
		int x, int y, int w, int h);
	void slide_left(int distance);
	void slide_right(int distance);
	void slide_up(int distance);
	void slide_down(int distance);
	void flicker(int n=3, int ms=66);
	void focus();

	int cycle_textboxes(int amount);

	int raise_window(int do_flush = 1);
	int lower_window(int do_flush = 1);
	void set_force_tooltip(int v);
	void set_tooltips(int v);
	int resize_window(int w, int h);
	int reposition_window(int x, int y);
	int reposition_window(int x, int y, int w, int h);
	int reposition_window_relative(int dx, int dy);
	int reposition_window_relative(int dx, int dy, int w, int h);
// Cause a repeat event to be dispatched every duration.
// duration is milliseconds
	int set_repeat(int64_t duration);
// Stop a repeat event from being dispatched.
	int unset_repeat(int64_t duration);
	const char *get_tooltip();
	int set_tooltip(const char *text);
	virtual int show_tooltip(const char *text, int x=-1, int y=-1, int w = -1, int h = -1);
	int show_tooltip(int w=-1, int h=-1) { return show_tooltip(0, -1, -1, w, h); }
	int hide_tooltip();
	int set_icon(VFrame *data);
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);

#ifdef HAVE_LIBXXF86VM
// Mode switch methods.
   void closest_vm(int *vm, int *width, int *height);
   void scale_vm(int vm);
   void restore_vm();
#endif

	Atom to_clipboard(const char *data, long len, int clipboard_num);
	long from_clipboard(char *data, long maxlen, int clipboard_num);
	long clipboard_len(int clipboard_num);

	int test_keypress;
  	char keys_return[KEYPRESSLEN];


private:
// Create a window
	virtual int create_window(BC_WindowBase *parent_window,
		const char *title, int x, int y, int w, int h,
		int minw, int minh, int allow_resize, int private_color,
		int hide, int bg_color, const char *display_name,
		int window_type, BC_Pixmap *bg_pixmap, int group_it);

	static Display* init_display(const char *display_name);
// Get display from top level
	Display* get_display();
	int get_screen();
	virtual int initialize();
	int get_atoms();
// Function to overload to receive customly defined atoms
	virtual int receive_custom_xatoms(xatom_event *event);

	void init_cursors();
	int init_colors();
	int init_window_shape();
	static int evaluate_color_model(int client_byte_order, int server_byte_order, int depth);
	int create_private_colors();
	int create_color(int color);
	int create_shared_colors();
	Cursor create_grab_cursor();
// Get width of a single line.  Used by get_text_width
	int get_single_text_width(int font, const char *text, int length);
	int get_single_text_width(int font, const wchar_t *text, int length);
	int allocate_color_table();
	int init_gc();
	int init_fonts();
	void init_xft();
	void init_im();
	void finit_im();
	int get_color_rgb8(int color);
	int64_t get_color_rgb16(int color);
	int64_t get_color_bgr16(int color);
	int64_t get_color_bgr24(int color);
	XFontStruct* get_font_struct(int font);
	XftFont* get_xft_struct(int font);
	Cursor get_cursor_struct(int cursor);
	XFontSet get_fontset(int font);
	XFontSet get_curr_fontset(void);
	void set_fontset(int font);
	int dispatch_event(XEvent *event);

	int get_key_masks(unsigned int key_state);

	int trigger_tooltip();
	int untrigger_tooltip();
	void draw_tooltip(const char *text=0);
	static XEvent *new_xevent();
// delete all repeater opjects for a close
	int unset_all_repeaters();

// Block and get event from common events.
	XEvent* get_event();
// Return number of events in table.
	int get_event_count();
// Put event in common events.
	void put_event(XEvent *event);
// remove events queued for win
	void dequeue_events(Window win);
// clear selection
	int do_selection_clear(Atom selection);

// Recursive event dispatchers
	int dispatch_resize_event(int w, int h);
	int dispatch_flash();
	int dispatch_focus_in();
	int dispatch_focus_out();
	int dispatch_motion_event();
	int dispatch_keypress_event();
	int dispatch_keyrelease_event();
	int dispatch_repeat_event(int64_t duration);
	int dispatch_repeat_event_master(int64_t duration);
	int dispatch_button_press();
	int dispatch_button_release();
	int dispatch_cursor_leave();
	int dispatch_cursor_enter();
	int dispatch_translation_event();
	int dispatch_drag_start();
	int dispatch_drag_motion();
	int dispatch_drag_stop();
	int dispatch_expose_event();
	int dispatch_selection_clear();

// Get the port ID for a color model or return -1 for failure
	int grab_port_id(int color_model);

	int find_next_textbox(BC_WindowBase **first_textbox, BC_WindowBase **next_textbox, int &result);
	int find_prev_textbox(BC_WindowBase **last_textbox, BC_WindowBase **prev_textbox, int &result);

	void xft_draw_string(XftColor *xft_color, XftFont *xft_font, int x, int y,
		const FcChar32 *fc, int len, BC_Pixmap *pixmap=0);

	void translate_coordinates(Window src_w, Window dest_w,
		int src_x, int src_y, int *dest_x_return, int *dest_y_return);

// Top level window above this window
	BC_WindowBase* top_level;
// Window just above this window
	BC_WindowBase* parent_window;
// list of window bases in this window
	BC_SubWindowList* subwindows;
// list of window bases in this window
	ArrayList<BC_WindowBase*> popups;
// Position of window
	int x, y, w, h;
// Default colors
	int light1, light2, medium, dark1, dark2, bg_color;
// Type of window defined above
	int window_type;
// keypress/pointer active grab
	BC_WindowBase *active_grab, *grab_active;
// Pointer to the active menubar in the window.
	BC_MenuBar* active_menubar;
// pointer to the active popup menu in the window
	BC_PopupMenu* active_popup_menu;
// pointer to the active subwindow
	BC_WindowBase* active_subwindow;
// pointer to the window to which to put the current event
	BC_WindowBase* resend_event_window;
// thread id of display locker
	pthread_t display_lock_owner;

// Window parameters
	int allow_resize;
	int hidden, private_color, bits_per_pixel, color_model;
	int server_byte_order, client_byte_order;
// number of colors in color table
	int total_colors;
// last color found in table
	int current_color_value, current_color_pixel;
// table for every color allocated
	int color_table[256][2];
// Turn on optimization
	int video_on;
// Event handler completion
	int done, done_set, window_running;
// Enter/Leave notify state
	int cursor_entered;
// Return value of event handler
	int return_value;
// Motion event compression
	int motion_events, last_motion_x, last_motion_y;
	unsigned int last_motion_state;
// window of buffered motion
	Window last_motion_win;
// Resize event compression
	int resize_events, last_resize_w, last_resize_h;
	int translation_events, last_translate_x, last_translate_y;
	int prev_x, prev_y;
// Since the window manager automatically translates the window at boot,
// use the first translation event to get correction factors
	int translation_count;
	int x_correction, y_correction;
// key data
	KeySym keysym;
// Key masks
	int ctrl_mask, shift_mask, alt_mask;
// Cursor motion information
	int cursor_x, cursor_y;
// Button status information
	int button_down, button_number;
// When button was pressed and whether it qualifies as a double click
	int64_t button_time1;
	int64_t button_time2;
	int64_t button_time3;
	int double_click;
	int triple_click;
// Which button is down.  1, 2, 3, 4, 5
	int button_pressed;
// Last key pressed
	int key_pressed;
	int wkey_string_length;
	wchar_t wkey_string[4];
#ifdef X_HAVE_UTF8_STRING
	char* key_pressed_utf8;
#endif
// During a selection drag involving toggles, set the same value for each toggle
	int toggle_value;
	int toggle_drag;
// Whether the window has the focus
	int has_focus;

	static BC_Resources resources;

#ifndef SINGLE_THREAD
// Array of repeaters for multiple repeating objects.
	ArrayList<BC_Repeater*> repeaters;
	int arm_repeat(int64_t duration);
#endif
// Text for tooltip if one exists
	const char *tooltip_text;
// tooltip forced for this window
	int force_tooltip;
// If the current window's tooltip is visible
	int tooltip_on;
// Repeat ID of tooltip
//	int64_t tooltip_id;
// Popup window for tooltip
	BC_Popup *tooltip_popup;
// If this subwindow has already shown a tooltip since the last EnterNotify
	int flash_enabled;


// Font sets
	XFontSet smallfontset, mediumfontset, largefontset, bigfontset, clockfontset;
	XFontSet curr_fontset;
// Fonts
	int current_font;
	XFontStruct *smallfont, *mediumfont, *largefont, *bigfont, *clockfont;
// Must be void so users don't need to include the wrong libpng version.
	void *smallfont_xft, *mediumfont_xft, *largefont_xft, *bigfont_xft, *clockfont_xft;
	void *bold_smallfont_xft, *bold_mediumfont_xft, *bold_largefont_xft;

	int line_width;
	int line_dashes;
	int64_t current_color;
// Coordinate of drag start
	int drag_x, drag_y;
// Boundaries the cursor must pass to start a drag
	int drag_x1, drag_x2, drag_y1, drag_y2;
// Dragging is specific to the subwindow
	int is_dragging;
// Don't delete the background pixmap
	int shared_bg_pixmap;
	char title[BCTEXTLEN];

// X Window parameters
	int screen;
	Window rootwin;
// windows previous events happened in
 	Window event_win, drag_win;
// selection clear
	Atom event_selection;
	Visual *vis;
	Colormap cmap;
// Name of display
	char display_name[BCTEXTLEN];
// Display for all synchronous operations
	Display *display;
 	Window win;
	int xinerama_screens;
	XineramaScreenInfo *xinerama_info;
#ifdef HAVE_GL
	int glx_fb_configs(int *attrs, GLXFBConfig *&cfgs);
	int glx_test_fb_configs(int *attrs, GLXFBConfig *&cfgs,
		const char *msg, int &msgs);
	GLXFBConfig *glx_fbcfgs_window, *glx_window_fb_configs();
	int n_fbcfgs_window;
	GLXFBConfig *glx_fbcfgs_pbuffer, *glx_pbuffer_fb_configs();
	int n_fbcfgs_pbuffer;
	GLXFBConfig *glx_fbcfgs_pixmap, *glx_pixmap_fb_configs();
	int n_fbcfgs_pixmap;
	Visual *get_glx_visual(Display *display);

	void sync_lock(const char *cp);
	void sync_unlock();
	GLXWindow glx_window();

// The first context to be created and the one whose texture id
// space is shared with the other contexts.
	GLXContext glx_get_context();
	bool glx_make_current(GLXDrawable draw);
	bool glx_make_current(GLXDrawable draw, GLXContext glx_ctxt);

	GLXFBConfig glx_fb_config;
	GLXContext glx_win_context;
	GLXWindow glx_win;
#endif

	int window_lock;
	GC gc;
// Depth given by the X Server
	int default_depth;
	Atom DelWinXAtom;
	Atom DestroyAtom;
	Atom ProtoXAtom;
	Atom RepeaterXAtom;
	Atom SetDoneXAtom;
// Number of times start_hourglass was called
	int hourglass_total;
// Cursor set by last set_cursor which wasn't an hourglass or transparent.
	int current_cursor;
// If hourglass overrides current cursor.  Only effective in top level.
	int is_hourglass;
// If transparent overrides all cursors.  Only effective in subwindow.
	int is_transparent;
	Cursor arrow_cursor;
	Cursor cross_cursor;
	Cursor ibeam_cursor;
	Cursor vseparate_cursor;
	Cursor hseparate_cursor;
	Cursor move_cursor;
	Cursor temp_cursor;
	Cursor left_cursor;
	Cursor right_cursor;
	Cursor upright_arrow_cursor;
	Cursor upleft_resize_cursor;
	Cursor upright_resize_cursor;
	Cursor downleft_resize_cursor;
	Cursor downright_resize_cursor;
	Cursor hourglass_cursor;
	Cursor transparent_cursor;
	Cursor grabbed_cursor;

	int xvideo_port_id;
	ArrayList<BC_ResizeCall*> resize_history;
// Back buffer
	BC_Pixmap *pixmap;
// Background tile if tiled
	BC_Pixmap *bg_pixmap;
// Icon
	BC_Popup *icon_window;
	BC_Pixmap *icon_pixmap;
	BC_Pixmap **_7segment_pixmaps;
// Temporary
	BC_Bitmap *temp_bitmap;
	BC_ActiveBitmaps active_bitmaps;
// Clipboard
#ifndef SINGLE_THREAD
	BC_Clipboard *clipboard;
#endif

#ifdef HAVE_LIBXXF86VM
// Mode switch information.
   int vm_switched;
   XF86VidModeModeInfo orig_modeline;
#endif




#ifndef SINGLE_THREAD
// Common events coming from X server and repeater.
	ArrayList<XEvent*> common_events;
// Locks for common events
// Locking order:
// 1) event_condition
// 2) event_lock
	Mutex *event_lock;
	Condition *event_condition;
// Lock that waits until the event handler is running
	Condition *init_lock;
#else
	Condition *completion_lock;
#endif


	int dump_windows();


	BC_WindowEvents *event_thread;
	int is_deleting;
// Hide cursor when video is enabled
	Timer *cursor_timer;
// unique ID of window.
	int id;

	// Used to communicate with the input method (IM) server
	XIM input_method;
	// Used for retaining the state, properties, and semantics
	//  of communication with the input method (IM) server
	XIC input_context;

protected:
	Atom create_xatom(const char *atom_name);
	int send_custom_xatom(xatom_event *event);

};



#endif
