
/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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

#ifndef MAINWINDOWGUI_H
#define MAINWINDOWGUI_H

#include "androidcontrol.inc"
#include "channelinfo.inc"
#include "cwindow.inc"
#include "editpopup.inc"
#include "guicast.h"
#include "indexable.inc"
#include "keyframepopup.inc"
#include "mainclock.inc"
#include "maincursor.inc"
#include "mainmenu.inc"
#include "mbuttons.inc"
#include "dbwindow.inc"
#include "mtimebar.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "panedividers.inc"
#include "patchbay.inc"
#include "pluginpopup.inc"
#include "record.inc"
#include "remotecontrol.h"
#include "record.inc"
#include "renderengine.inc"
#include "resourcepixmap.h"
#include "resourcethread.inc"
#include "samplescroll.inc"
#include "shbtnprefs.inc"
#include "statusbar.inc"
#include "swindow.inc"
#include "timelinepane.inc"
#include "track.inc"
#include "trackcanvas.inc"
#include "trackpopup.inc"
#include "trackscroll.inc"
#include "transitionpopup.inc"
#include "zoombar.inc"


class PaneButton : public BC_Button
{
public:
	PaneButton(MWindow *mwindow, int x, int y);
	int cursor_motion_event();
	int button_release_event();
	MWindow *mwindow;
};

class FFMpegToggle : public BC_Toggle
{
public:
	FFMpegToggle(MWindow *mwindow, MButtons *mbuttons, int x, int y);
	~FFMpegToggle();
	int handle_event();

	MWindow *mwindow;
	MButtons *mbuttons;
};

class ProxyToggle : public BC_Toggle
{
public:
	ProxyToggle(MWindow *mwindow, MButtons *mbuttons, int x, int y);
	~ProxyToggle();
	int handle_event();
	int keypress_event();
	void show();
	void hide();

	MWindow *mwindow;
	MButtons *mbuttons;
	int scaler_images;
};


class MWindowGUI : public BC_Window
{
public:
	MWindowGUI(MWindow *mwindow);
	~MWindowGUI();

	void create_objects();
//	void get_scrollbars(int flush);

// ======================== event handlers

// Replace with update
	void redraw_time_dependancies();

	int focus_in_event();
	int focus_out_event();

// do_canvas -
//   NO_DRAW disable canvas draw
//   IGNORE_THREAD to ignore picon thread
//   NORMAL_DRAW for incremental drawing of resources
//   FORCE_REDRAW for delete and redraw of resources
	void update(int scrollbars,
		int do_canvas,
		int timebar,
		int zoombar,
		int patchbay,
		int clock,
		int buttonbar);
	void draw_overlays(int flash_it);
	void draw_indexes(Indexable *indexable);
	void update_title(char *path);
	void update_timebar(int flush_it);
	void update_timebar_highlights();
	void update_patchbay();
	void update_proxy_toggle();
	void update_plugintoggles();
	void update_scrollbars(int flush);
	void draw_canvas(int redraw, int hide_cursor);
	void flash_canvas(int flush);
	int show_window(int flush=1);
	void deactivate_timeline();
	void activate_timeline();
	void reset_meters();
	void stop_meters();
	void update_meters(ArrayList<double> *module_levels);
	void draw_cursor(int do_plugintoggles);
	void show_cursor(int do_plugintoggles /* = 1 */);
	void hide_cursor(int do_plugintoggles /* = 1 */);
	void update_cursor();
	void set_playing_back(int value);
	void set_editing_mode(int flush);
	void set_meter_format(int mode, int min, int max);
	void update_mixers(Track *track, int v);
	void stop_transport(const char *lock_msg);

	int translation_event();
	int resize_event(int w, int h);          // handle a resize event
	int keypress_event();
	int keyboard_listener(BC_WindowBase *wp);
	int key_listener(int key);
	void use_android_remote(int on);
	int close_event();
	int quit();
	void stop_drawing();
	int save_defaults(BC_Hash *defaults);
	int menu_w();
	int menu_h();
// Draw on the status bar only.
	void show_message(const char *message, int color=-1);
	void update_default_message();
	void reset_default_message();
	void default_message();
	void show_error(char *message, int color = BLACK);
	int repeat_event(int64_t duration);
// Entry point for drag events in all windows
	int drag_motion();
	int drag_stop();
	void default_positions();
	int total_panes();
// 1 if there are 2 panes vertically
	int vertical_panes();
// 1 if there are 2 panes horizontally
	int horizontal_panes();

// get pane number where cursor updates should be drawn in,
// whether active or not
	TimelinePane* get_focused_pane();
	void start_x_pane_drag();
	void start_y_pane_drag();
	void handle_pane_drag();
	void stop_pane_drag();
	void delete_x_pane(int cursor_x);
	void create_x_pane(int cursor_x);
	void create_y_pane(int cursor_y);
	void delete_y_pane(int cursor_y);
	void update_pane_dividers();
// load panes from EDL
	void load_panes();
	void draw_samplemovement();
	void draw_trackmovement();


// Return if the area bounded by x1 and x2 is visible between view_x1 and view_x2
	static int visible(int64_t x1, int64_t x2, int64_t view_x1, int64_t view_x2);

	MWindow *mwindow;

// For drawing nested EDL's
	RenderEngine *render_engine;
// ID of nested EDL last drawn.
	int render_engine_id;
// sideshow apps
	Record *record;
	ChannelInfo *channel_info;
	DbWindow *db_window;
	SWindow *swindow;
// Popup menus
	TrackPopup *track_menu;
	EditPopup *edit_menu;
	PluginPopup *plugin_menu;
	KeyframePopup *keyframe_menu;
	KeyframeHidePopup *keyframe_hide;
	BC_SubWindow *keyvalue_popup;
	TransitionPopup *transition_menu;

	MainClock *mainclock;
	MButtons *mbuttons;
	FFMpegToggle *ffmpeg_toggle;
	ProxyToggle *proxy_toggle;
	PaneDivider *x_divider;
	PaneDivider *y_divider;
	TimelinePane *pane[TOTAL_PANES];
	ResourceThread *resource_thread;
	ArrayList<ResourcePixmap*> resource_pixmaps;


	MainMenu *mainmenu;
	MainShBtns *mainshbtns;
	ZoomBar *zoombar;
	StatusBar *statusbar;
	PaneButton *pane_button;
	BC_Popup *x_pane_drag;
	BC_Popup *y_pane_drag;
	int dragging_pane;
// Cursor used to select a region in all panes
//	MainCursor *cursor;
// Dimensions of canvas minus scrollbars
//	int view_w, view_h;
// pane number where cursor updates should be drawn in, whether active or not
	int focused_pane;


	BC_DragWindow *drag_popup;

//	PatchBay *patchbay;
//	MTimeBar *timebar;
//	SampleScroll *samplescroll;
//	TrackScroll *trackscroll;
//	TrackCanvas *canvas;

// remote control
	AndroidControl *android_control;
	RemoteControl *remote_control;
	CWindowRemoteHandler *cwindow_remote_handler;
	RecordRemoteHandler *record_remote_handler;
};

#endif
