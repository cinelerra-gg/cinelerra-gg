
/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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

#ifndef RECORDMONITOR_H
#define RECORDMONITOR_H

#ifdef HAVE_FIREWIRE
#include "avc1394transport.h"
#endif
#include "bcdialog.h"
#include "canvas.h"
#include "channelpicker.inc"
#include "condition.inc"
#include "guicast.h"
#include "channelpicker.inc"
#include "libmjpeg.h"
#include "meterpanel.inc"
#include "preferences.inc"
#include "record.inc"
#include "recordgui.inc"
#include "recordscopes.inc"
#include "recordtransport.inc"
#include "recordmonitor.inc"
#include "signalstatus.inc"
#include "videodevice.inc"

class RecordMonitorThread;

class RecordMonitor : public Thread
{
public:
	RecordMonitor(MWindow *mwindow, Record *record);
	~RecordMonitor();

// Show a frame
	int update(VFrame *vframe);
// Update channel textbox
	void update_channel(char *text);

	MWindow *mwindow;
	Record *record;
// Thread for slippery monitoring
	RecordMonitorThread *thread;
	RecordMonitorGUI *window;
	VideoDevice *device;
// Fake config for monitoring
	VideoOutConfig *config;

	RecordScopeThread *scope_thread;

	void run();
	void close_threads();   // Stop all the child threads on exit
	void create_objects();
	void fix_size(int &w, int &h, int width_given, float aspect_ratio);
	float get_scale(int w);
	int get_mbuttons_height();
	int get_canvas_height();
	int get_channel_x();
	int get_channel_y();
	void reconfig();
	void redraw();
	void start_playback();
	void stop_playback();
	void display_vframe(VFrame *in, int x, int y,
		int alpha, double secs, double scale);
	void undisplay_vframe();
};

class ReverseInterlace;
class RecordMonitorCanvas;
class DoCursor;
class DoBigCursor;

class RecordMonitorGUI : public BC_Window
{
public:
	RecordMonitorGUI(MWindow *mwindow, Record *record,
		RecordMonitor *thread, int min_w);
	~RecordMonitorGUI();

	void create_objects();
	int cursor_leave_event();
	int cursor_enter_event();
	int button_release_event();
	int cursor_motion_event();

	void display_video_text(int x, int y, const char *text, int font,
		int bg_color, int color, int alpha, double secs, double scale);
	void enable_signal_status(int enable);

	MeterPanel *meters;
	Canvas *canvas;
//	RecordTransport *record_transport;
#ifdef HAVE_FIREWIRE
	AVC1394Transport *avc1394_transport;
	AVC1394TransportThread *avc1394transport_thread;
#endif
	RecordChannelPicker *channel_picker;
	ScopeEnable *scope_toggle;
	DoCursor *cursor_toggle;
	DoBigCursor *big_cursor_toggle;
	ReverseInterlace *reverse_interlace;
	int cursor_x_origin, cursor_y_origin;
	int translate_x_origin, translate_y_origin;
	BC_PopupMenu *monitor_menu;
	int current_operation;


	int translation_event();
	int button_press_event();
	int resize_event(int w, int h);
	int redraw();
	int set_title();
	int close_event();
	int create_bitmap();
	int keypress_event();

	MWindow *mwindow;
	BC_SubWindow *mbuttons;
	BC_Bitmap *bitmap;
	RecordMonitor *thread;
	Record *record;
#ifdef HAVE_FIREWIRE
	AVC1394Control *avc;
	BC_Title *avc1394transport_title;
	BC_Title *avc1394transport_timecode;
#endif
	SignalStatus *signal_status;
};


class RecVideoMJPGThread;
class RecVideoDVThread;
class RecVideoOverlay;

class RecordMonitorThread : public Thread
{
	void show_output_frame();
	void render_uncompressed();
	int render_jpeg();
	int render_dv();
	void process_scope();
	void process_hist();

	int ready;   // Ready to receive the next frame
	int done;
	RecVideoMJPGThread *jpeg_engine;
	RecVideoDVThread *dv_engine;
public:
	RecordMonitorThread(MWindow *mwindow,
		Record *record,
		RecordMonitor *record_monitor);
	~RecordMonitorThread();

	void reset_parameters();
	void run();
// Calculate the best output format based on input drivers
	void init_output_format();
	int start_playback();
	int stop_playback();
	int write_frame(VFrame *new_frame);
	int render_frame();
	void lock_input();
	void unlock_input();
	void new_output_frame();
	void display_vframe(VFrame *in, int x, int y,
		int alpha, double secs, double scale);
	void undisplay_vframe();
	int finished() { return done; }

// Input frame being rendered
	VFrame *input_frame;
// Frames for the rendered output
	VFrame *output_frame;
// Best color model given by device
	int output_colormodel;
// Block until new input data
	Condition *output_lock;
	Condition *input_lock;
	Record *record;
	RecordMonitor *record_monitor;
	MWindow *mwindow;
// If the input frame is the same data that the file handle uses
	int shared_data;
// overlay data
	RecVideoOverlay *ovly;
};

class RecordMonitorFullsize : public BC_MenuItem
{
public:
	RecordMonitorFullsize(MWindow *mwindow, RecordMonitorGUI *window);

	int handle_event();
	MWindow *mwindow;
	RecordMonitorGUI *window;
};

class RecordMonitorCanvas : public Canvas
{
public:
	RecordMonitorCanvas(MWindow *mwindow, RecordMonitorGUI *window,
		Record *record, int x, int y, int w, int h);
	~RecordMonitorCanvas();

	void zoom_resize_window(float percentage);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int cursor_enter_event();
	void reset_translation();
	int keypress_event();
	int get_output_w();
	int get_output_h();

	RecordMonitorGUI *window;
	MWindow *mwindow;
	Record *record;
};

class DoCursor : public BC_CheckBox
{
public:
	DoCursor(Record *record, int x, int y);
	~DoCursor();
	int handle_event();
	Record *record;
};

class DoBigCursor : public BC_CheckBox
{
public:
	DoBigCursor(Record *record, int x, int y);
	~DoBigCursor();
	int handle_event();
	Record *record;
};

class ReverseInterlace : public BC_CheckBox
{
public:
	ReverseInterlace(Record *record, int x, int y);
	~ReverseInterlace();
	int handle_event();
	Record *record;
};

class RecVideoMJPGThread
{
public:
	RecVideoMJPGThread(Record *record,
		RecordMonitorThread *thread,
		int fields);
	~RecVideoMJPGThread();

	int render_frame(VFrame *frame, long size);
	int start_rendering();
	int stop_rendering();

	RecordMonitorThread *thread;
	Record *record;

private:
	mjpeg_t *mjpeg;
	int fields;
};

class RecVideoDVThread
{
public:
	RecVideoDVThread(Record *record, RecordMonitorThread *thread);
	~RecVideoDVThread();

	int start_rendering();
	int stop_rendering();
	int render_frame(VFrame *frame, long size);

	RecordMonitorThread *thread;
	Record *record;

private:
// Don't want theme to need libdv to compile
	void *dv;
};

class RecVideoOverlay
{
	int x, y, ticks;
	float scale, alpha;
	VFrame *vframe;
public:
	RecVideoOverlay(VFrame *vframe, int x, int y, int ticks, float scale, float alpha);
	~RecVideoOverlay();

	int overlay(VFrame *out);
};


#endif
