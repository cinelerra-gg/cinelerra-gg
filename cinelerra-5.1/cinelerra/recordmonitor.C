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

#include "asset.h"
#include "bcdialog.h"
#include "bcsignals.h"
#include "channelpicker.h"
#include "condition.h"
#include "cursors.h"
#include "devicedvbinput.h"
#ifdef HAVE_DV
#include "libdv.h"
#endif
#include "edl.h"
#include "edlsession.h"
#include "keys.h"
#include "language.h"
#include "mainsession.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "record.h"
#include "recordconfig.h"
#include "recordgui.h"
#include "recordscopes.h"
#include "recordtransport.h"
#include "recordmonitor.h"
#include "signalstatus.h"
#include "theme.h"
#include "videodevice.inc"
#include "vframe.h"
#include "videodevice.h"
#include "vdevicedvb.h"


RecordMonitor::RecordMonitor(MWindow *mwindow, Record *record)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->record = record;
	device = 0;
	thread = 0;
	scope_thread = 0;
}


RecordMonitor::~RecordMonitor()
{
	delete thread;
	window->set_done(0);
	Thread::join();
	if( device ) {
		device->close_all();
		delete device;
	}
	delete scope_thread;
	delete window;
}

void RecordMonitor::create_objects()
{
	int min_w = 150;

	if( !record->default_asset->video_data )
		min_w = MeterPanel::get_meters_width(mwindow->theme,
			record->default_asset->channels, 1);

	window = new RecordMonitorGUI(mwindow, record, this, min_w);
	window->create_objects();

	if( record->default_asset->video_data ) {
// Configure the output for record monitoring
		VideoOutConfig config;
		device = new VideoDevice;

// Override default device for X11 drivers
		if(mwindow->edl->session->playback_config->vconfig->driver ==
			PLAYBACK_X11_XV) config.driver = PLAYBACK_X11_XV;
		config.x11_use_fields = 0;
		device->open_output(&config,
			record->default_asset->frame_rate,
			record->default_asset->width,
			record->default_asset->height,
			window->canvas, 0);

		scope_thread = new RecordScopeThread(mwindow, this);

		if(mwindow->session->record_scope) {
			scope_thread->start();
		}

		thread = new RecordMonitorThread(mwindow, record, this);
		thread->start_playback();
	}

	Thread::start();
}


void RecordMonitor::run()
{
	window->run_window();
	close_threads();
}

void RecordMonitor::close_threads()
{
	if(window->channel_picker) window->channel_picker->close_threads();
}

int RecordMonitor::update(VFrame *vframe)
{
	return thread->write_frame(vframe);
}

void RecordMonitor::update_channel(char *text)
{
	if( window->channel_picker )
		window->channel_picker->channel_text->update(text);
}

int RecordMonitor::get_mbuttons_height()
{
	return RECBUTTON_HEIGHT;
}

void RecordMonitor::fix_size(int &w, int &h, int width_given, float aspect_ratio)
{
	w = width_given;
	h = (int)((float)width_given / aspect_ratio);
}

float RecordMonitor::get_scale(int w)
{
	if( mwindow->edl->get_aspect_ratio() >
		(float)record->frame_w / record->frame_h ) {
		return (float)w /
			((float)record->frame_h *
			mwindow->edl->get_aspect_ratio());
	}
	else {
		return (float)w / record->frame_w;
	}
}

int RecordMonitor::get_canvas_height()
{
	return window->get_h() - get_mbuttons_height();
}

int RecordMonitor::get_channel_x()
{
//	return 240;
	return 5;
}

int RecordMonitor::get_channel_y()
{
	return 2;
}

void RecordMonitor::stop_playback()
{
	if( !thread || thread->finished() ) return;
	window->enable_signal_status(0);
	if( thread ) {
		thread->stop_playback();
	}
}

void RecordMonitor::start_playback()
{
	if( thread ) {
		thread->output_lock->reset();
		thread->start_playback();
	}
	window->enable_signal_status(1);
}


void RecordMonitor::reconfig()
{
	stop_playback();
	VideoOutConfig config = *device->out_config;
	device->close_all();
	device->open_output(&config,
		record->default_asset->frame_rate,
		record->default_asset->width,
		record->default_asset->height,
		window->canvas, 0);
	start_playback();
	redraw();
}

void RecordMonitor::redraw()
{
	if( thread && window && thread->record->video_window_open )
		window->redraw();
}

void RecordMonitor::display_vframe(VFrame *in, int x, int y,
		int alpha, double secs, double scale)
{
	if( !thread ) return;
	thread->display_vframe(in, x, y, alpha, secs, scale);
}

void RecordMonitor::undisplay_vframe()
{
	if( !thread ) return;
	thread->undisplay_vframe();
}

RecordMonitorGUI::RecordMonitorGUI(MWindow *mwindow,
	Record *record, RecordMonitor *thread, int min_w)
 : BC_Window(_(PROGRAM_NAME ": Video in"),
 			mwindow->session->rmonitor_x,
			mwindow->session->rmonitor_y,
 			mwindow->session->rmonitor_w,
 			mwindow->session->rmonitor_h,
			min_w, 50, 1, 1, 1, -1,
			mwindow->get_cwindow_display())
{
//printf("%d %d\n", mwindow->session->rmonitor_w, mwindow->theme->rmonitor_meter_x);
	this->mwindow = mwindow;
	this->thread = thread;
	this->record = record;
#ifdef HAVE_FIREWIRE
	avc = 0;
	avc1394_transport = 0;
	avc1394transport_title = 0;
	avc1394transport_timecode = 0;
	avc1394transport_thread = 0;
#endif
	bitmap = 0;
	channel_picker = 0;
	reverse_interlace = 0;
	meters = 0;
	canvas = 0;
	cursor_toggle = 0;
	big_cursor_toggle = 0;
	current_operation = MONITOR_NONE;
	signal_status = 0;
}

RecordMonitorGUI::~RecordMonitorGUI()
{
	lock_window("RecordMonitorGUI::~RecordMonitorGUI");
#ifdef HAVE_DVB
	delete signal_status;
#endif
	delete canvas;
	delete cursor_toggle;
	delete big_cursor_toggle;
	delete bitmap;
	if( channel_picker ) delete channel_picker;
#ifdef HAVE_FIREWIRE
	delete avc1394transport_thread;
	delete avc;
	delete avc1394_transport;
	delete avc1394transport_title;
#endif
	delete meters;
	unlock_window();
}

void RecordMonitorGUI::create_objects()
{
// y offset for video canvas if we have the transport controls
	lock_window("RecordMonitorGUI::create_objects");
	int driver = mwindow->edl->session->vconfig_in->driver;
	int do_channel = (driver == CAPTURE_DVB ||
			driver == VIDEO4LINUX2 ||
			driver == VIDEO4LINUX2JPEG ||
			driver == VIDEO4LINUX2MPEG ||
			driver == CAPTURE_JPEG_WEBCAM ||
			driver == CAPTURE_YUYV_WEBCAM);
	int do_cursor = driver == SCREENCAPTURE;
	int do_scopes = do_channel || driver == SCREENCAPTURE;
	int do_interlace = driver == VIDEO4LINUX2JPEG;
	int background_done = 0;
	int do_audio = record->default_asset->audio_data;
	int do_video = record->default_asset->video_data;
	int do_meters = record->metering_audio;
	int channels = !do_meters ? 0 : record->default_asset->channels;

	mwindow->theme->get_rmonitor_sizes(do_meters, do_video,
		do_channel || do_scopes, do_interlace, 0, channels);


	if(do_video) {
#ifdef HAVE_FIREWIRE
		if( driver == CAPTURE_FIREWIRE || driver == CAPTURE_IEC61883 ) {
			avc = new AVC1394Control;
			if( avc->device > -1 ) {
				mwindow->theme->get_rmonitor_sizes(do_meters, do_video,
					 do_channel, do_interlace, 1, channels);
				mwindow->theme->draw_rmonitor_bg(this);
				background_done = 1;

				avc1394_transport = new AVC1394Transport(mwindow,
					avc,
					this,
					mwindow->theme->rmonitor_tx_x,
					mwindow->theme->rmonitor_tx_y);
				avc1394_transport->create_objects();

				add_subwindow(avc1394transport_timecode =
					new BC_Title(avc1394_transport->x_end,
						mwindow->theme->rmonitor_tx_y + 10,
						"00:00:00:00",
						MEDIUM_7SEGMENT,
						BLACK));

				avc1394transport_thread =
					new AVC1394TransportThread(avc1394transport_timecode,
						avc);

				avc1394transport_thread->start();

			}
		}
#endif

		if( !background_done ) {
			mwindow->theme->draw_rmonitor_bg(this);
			background_done = 1;
		}

		mwindow->theme->rmonitor_canvas_w = MAX(10, mwindow->theme->rmonitor_canvas_w);
		mwindow->theme->rmonitor_canvas_h = MAX(10, mwindow->theme->rmonitor_canvas_h);
		canvas = new RecordMonitorCanvas(mwindow, this, record,
			mwindow->theme->rmonitor_canvas_x,
			mwindow->theme->rmonitor_canvas_y,
			mwindow->theme->rmonitor_canvas_w,
			mwindow->theme->rmonitor_canvas_h);
		canvas->create_objects(0);
		canvas->use_rwindow();

#ifdef HAVE_DVB
		if( driver == CAPTURE_DVB ) {
			int ssw = SignalStatus::calculate_w(this);
			signal_status = new SignalStatus(this, get_w()-ssw-3, 0);
			add_subwindow(signal_status);
			signal_status->create_objects();
		}
#endif

		int x = mwindow->theme->widget_border;
		int y = mwindow->theme->widget_border;
		if( do_channel ) {
			channel_picker = new RecordChannelPicker(mwindow,
				record, thread, this, record->channeldb,
				mwindow->theme->rmonitor_channel_x,
				mwindow->theme->rmonitor_channel_y);
			channel_picker->create_objects();
			x += channel_picker->get_w() + mwindow->theme->widget_border;
		}
		if( driver == VIDEO4LINUX2JPEG ) {
			add_subwindow(reverse_interlace = new ReverseInterlace(record,
				mwindow->theme->rmonitor_interlace_x,
				mwindow->theme->rmonitor_interlace_y));
			x += reverse_interlace->get_w() + mwindow->theme->widget_border;
		}

		if( do_scopes ) {
			scope_toggle = new ScopeEnable(mwindow, thread, x, y);
			add_subwindow(scope_toggle);
			x += scope_toggle->get_w() + mwindow->theme->widget_border;
		}

		if( do_cursor ) {
			add_subwindow(cursor_toggle = new DoCursor(record,
				x, 
				y));
			x += cursor_toggle->get_w() + mwindow->theme->widget_border;
			add_subwindow(big_cursor_toggle = new DoBigCursor(record,
				x, 
				y));
			x += big_cursor_toggle->get_w() + mwindow->theme->widget_border;
		}

		add_subwindow(monitor_menu = new BC_PopupMenu(0, 0, 0, "", 0));
		monitor_menu->add_item(new RecordMonitorFullsize(mwindow, this));
	}


	if( !background_done ) {
		mwindow->theme->draw_rmonitor_bg(this);
		background_done = 1;
	}

	if( do_audio ) {
		meters = new MeterPanel(mwindow,
			this,
			mwindow->theme->rmonitor_meter_x,
			mwindow->theme->rmonitor_meter_y,
			record->default_asset->video_data ? -1 : mwindow->theme->rmonitor_meter_w,
			mwindow->theme->rmonitor_meter_h,
			channels, do_meters, 1, 0);
		meters->create_objects();
	}
	unlock_window();
}

int RecordMonitorGUI::button_press_event()
{
	if( canvas && canvas->get_fullscreen() && canvas->get_canvas())
		return canvas->button_press_event_base(canvas->get_canvas());

	if( get_buttonpress() == 2 ) {
		return 0;
	}
	else
// Right button
	if( get_buttonpress() == 3 ) {
		monitor_menu->activate_menu();
		return 1;
	}
	return 0;
}

int RecordMonitorGUI::cursor_leave_event()
{
	if(canvas && canvas->get_canvas())
		return canvas->cursor_leave_event_base(canvas->get_canvas());
	return 0;
}

int RecordMonitorGUI::cursor_enter_event()
{
	if( canvas && canvas->get_canvas() )
		return canvas->cursor_enter_event_base(canvas->get_canvas());
	return 0;
}

int RecordMonitorGUI::button_release_event()
{
	if( canvas && canvas->get_canvas() )
		return canvas->button_release_event();
	return 0;
}

int RecordMonitorGUI::cursor_motion_event()
{
	if( canvas && canvas->get_canvas() ) {
		canvas->get_canvas()->unhide_cursor();
		return canvas->cursor_motion_event();
	}
	return 0;
}

int RecordMonitorGUI::keypress_event()
{
	int result = 0;

	switch(get_keypress()) {
	case LEFT:
		if( !ctrl_down() ) {
			record->record_gui->set_translation(--(record->video_x), record->video_y, record->video_zoom);
		}
		else {
			record->video_zoom -= 0.1;
			record->record_gui->set_translation(record->video_x, record->video_y, record->video_zoom);
		}
		result = 1;
		break;
	case RIGHT:
		if( !ctrl_down() ) {
			record->record_gui->set_translation(++(record->video_x), record->video_y, record->video_zoom);
		}
		else {
			record->video_zoom += 0.1;
			record->record_gui->set_translation(record->video_x, record->video_y, record->video_zoom);
		}
		result = 1;
		break;
	case UP:
		if( !ctrl_down() ) {
			record->record_gui->set_translation(record->video_x, --(record->video_y), record->video_zoom);
		}
		else {
			record->video_zoom -= 0.1;
			record->record_gui->set_translation(record->video_x, record->video_y, record->video_zoom);
		}
		result = 1;
		break;
	case DOWN:
		if( !ctrl_down() ) {
			record->record_gui->set_translation(record->video_x, ++(record->video_y), record->video_zoom);
		}
		else {
			record->video_zoom += 0.1;
			record->record_gui->set_translation(record->video_x, record->video_y, record->video_zoom);
		}
		result = 1;
		break;
	case 'w':
		close_event();
		break;

	default:
		if(canvas) result = canvas->keypress_event(this);
#ifdef HAVE_FIREWIRE
		if(!result && avc1394_transport)
			result = avc1394_transport->keypress_event(get_keypress());
#endif
		break;
	}

	return result;
}


int RecordMonitorGUI::translation_event()
{
//printf("MWindowGUI::translation_event 1 %d %d\n", get_x(), get_y());
	mwindow->session->rmonitor_x = get_x();
	mwindow->session->rmonitor_y = get_y();
	return 0;
}

int RecordMonitorGUI::resize_event(int w, int h)
{
	int driver = mwindow->edl->session->vconfig_in->driver;
	int do_channel = (driver == CAPTURE_DVB ||
			driver == VIDEO4LINUX2 ||
			driver == VIDEO4LINUX2JPEG ||
			driver == VIDEO4LINUX2MPEG ||
			driver == CAPTURE_JPEG_WEBCAM ||
			driver == CAPTURE_YUYV_WEBCAM);
	int do_scopes = do_channel || driver == SCREENCAPTURE;
	int do_interlace = (driver == VIDEO4LINUX2JPEG);
	int do_avc = 0;
#ifdef HAVE_FIREWIRE
	do_avc = avc1394_transport ? 1 : 0;
#endif
	int do_meters = meters && record->default_asset->audio_data &&
		record->metering_audio;
	int do_video = record->default_asset->video_data;

	mwindow->session->rmonitor_x = get_x();
	mwindow->session->rmonitor_y = get_y();
	mwindow->session->rmonitor_w = w;
	mwindow->session->rmonitor_h = h;

	mwindow->theme->get_rmonitor_sizes(do_meters, do_video,
		do_channel || do_scopes, do_interlace, do_avc,
		record->default_asset->channels);
	mwindow->theme->draw_rmonitor_bg(this);


// 	record_transport->reposition_window(mwindow->theme->rmonitor_tx_x,
// 		mwindow->theme->rmonitor_tx_y);
#ifdef HAVE_FIREWIRE
	if(avc1394_transport)
	{
		avc1394_transport->reposition_window(mwindow->theme->rmonitor_tx_x,
			mwindow->theme->rmonitor_tx_y);
	}
#endif

	
	if( channel_picker ) {
		channel_picker->reposition();
	}
	if( reverse_interlace ) {
		reverse_interlace->reposition_window(
			reverse_interlace->get_x(),
			reverse_interlace->get_y());
	}
	if( cursor_toggle ) {
		cursor_toggle->reposition_window(
			cursor_toggle->get_x(),
			cursor_toggle->get_y());
		big_cursor_toggle->reposition_window(
			big_cursor_toggle->get_x(),
			big_cursor_toggle->get_y());
	}

	if( canvas && do_video ) {
		canvas->reposition_window(0,
			mwindow->theme->rmonitor_canvas_x,
			mwindow->theme->rmonitor_canvas_y,
			mwindow->theme->rmonitor_canvas_w,
			mwindow->theme->rmonitor_canvas_h);
	}

	if( do_meters ) {
		meters->reposition_window(mwindow->theme->rmonitor_meter_x,
			mwindow->theme->rmonitor_meter_y,
			do_video ? -1 : mwindow->theme->rmonitor_meter_w,
			mwindow->theme->rmonitor_meter_h);
		meters->set_meters(record->default_asset->channels,1);
	}
	else if( meters ) {
		meters->set_meters(0,0);
	}

	set_title();
	BC_WindowBase::resize_event(w, h);
	flash();
	return 1;
}

int RecordMonitorGUI::redraw()
{
	lock_window("RecordMonitorGUI::redraw");
	int w = mwindow->session->rmonitor_w;
	int h = mwindow->session->rmonitor_h;
	int result = resize_event(w, h);
	unlock_window();
	return result;
}

int RecordMonitorGUI::set_title()
{
return 0;
	char string[1024];
	int scale;

	scale = (int)(thread->get_scale(thread->record->video_window_w) * 100 + 0.5);

	sprintf(string, _(PROGRAM_NAME ": Video in %d%%"), scale);
	BC_Window::set_title(string);
	return 0;
}

int RecordMonitorGUI::close_event()
{
	thread->record->set_video_monitoring(0);
	thread->record->set_audio_monitoring(0);
	thread->record->video_window_open = 0;
	unlock_window();
	lock_window("RecordMonitorGUI::close_event");
	hide_window();
	return 0;
}

int RecordMonitorGUI::create_bitmap()
{
	if(bitmap && (bitmap->get_w() != get_w() ||
			bitmap->get_h() != thread->get_canvas_height())) {
		delete bitmap;
		bitmap = 0;
	}

	if( !bitmap && canvas ) {
//		bitmap = canvas->new_bitmap(get_w(), thread->get_canvas_height());
	}
	return 0;
}


DoCursor::DoCursor(Record *record, int x, int y)
 : BC_CheckBox(x, y, record->do_cursor, _("Record cursor"))
{
	this->record = record;
}

DoCursor::~DoCursor()
{
}

int DoCursor::handle_event()
{
	record->do_cursor = get_value();
	return 0;
}


DoBigCursor::DoBigCursor(Record *record, int x, int y)
 : BC_CheckBox(x, y, record->do_big_cursor, _("Big cursor"))
{
	this->record = record;
}

DoBigCursor::~DoBigCursor()
{
}

int DoBigCursor::handle_event()
{
	record->do_big_cursor = get_value();
	return 0;
}


void RecordMonitorGUI::enable_signal_status(int enable)
{
#ifdef HAVE_DVB
	if( !signal_status ) return;
	signal_status->lock_window("RecordMonitorGUI::enable_signal_status");
	if( !enable )
		signal_status->hide_window();
	else
		signal_status->show_window();
	signal_status->unlock_window();
	DeviceDVBInput *dvb_input = record->dvb_device();
	if( dvb_input )
		dvb_input->set_signal_status(!enable ? 0 : signal_status);
#endif
}

void RecordMonitorGUI::
display_video_text(int x, int y, const char *text, int font,
	int bg_color, int color, int alpha, double secs, double scale)
{
	lock_window("RecordMonitorGUI::display_text");
	set_font(font);
	int ch = get_text_height(font);
	int h = get_text_height(font,text) + ch/2;
	int w = get_text_width(font, text) + ch;
	BC_Pixmap pixmap(this, w, h);
	set_opaque();
	set_color(bg_color);
	draw_box(0, 0, w, h, &pixmap);
	set_color(color);
	draw_text(ch/2, ch, text, strlen(text), &pixmap);
	BC_Bitmap bitmap(this, w, h, BC_RGB888, 0);
	VFrame in(&bitmap, w, h, BC_RGB888, -1);
	Drawable drawable = pixmap.get_pixmap();
	bitmap.read_drawable(drawable, 0, 0, &in);
	unlock_window();
	record->display_vframe(&in, x, y, alpha, secs, scale);
}

ReverseInterlace::ReverseInterlace(Record *record, int x, int y)
 : BC_CheckBox(x, y, record->reverse_interlace, _("Swap fields"))
{
	this->record = record;
}

ReverseInterlace::~ReverseInterlace()
{
}

int ReverseInterlace::handle_event()
{
	record->reverse_interlace = get_value();
	return 0;
}

RecordMonitorCanvas::RecordMonitorCanvas(MWindow *mwindow,
	RecordMonitorGUI *window, Record *record,
	int x, int y, int w, int h)
 : Canvas(mwindow, window, x, y, w, h,
	record->default_asset->width,
	record->default_asset->height,
	0)
{
	this->window = window;
	this->mwindow = mwindow;
	this->record = record;
//printf("RecordMonitorCanvas::RecordMonitorCanvas 1 %d %d %d %d\n",
//x, y, w, h);
//printf("RecordMonitorCanvas::RecordMonitorCanvas 2\n");
}

RecordMonitorCanvas::~RecordMonitorCanvas()
{
}

int RecordMonitorCanvas::get_output_w()
{
	return record->default_asset->width;
}

int RecordMonitorCanvas::get_output_h()
{
	return record->default_asset->height;
}


int RecordMonitorCanvas::button_press_event()
{

	if(Canvas::button_press_event()) return 1;
	if( mwindow->edl->session->vconfig_in->driver == SCREENCAPTURE ) {
		window->current_operation = MONITOR_TRANSLATE;
		window->translate_x_origin = record->video_x;
		window->translate_y_origin = record->video_y;
		window->cursor_x_origin = get_cursor_x();
		window->cursor_y_origin = get_cursor_y();
	}

	return 0;
}

void RecordMonitorCanvas::zoom_resize_window(float percentage)
{
	int canvas_w, canvas_h;
	calculate_sizes(mwindow->edl->get_aspect_ratio(),
		record->default_asset->width, record->default_asset->height,
		percentage, canvas_w, canvas_h);
	int new_w, new_h;
	new_w = canvas_w + (window->get_w() - mwindow->theme->rmonitor_canvas_w);
	new_h = canvas_h + (window->get_h() - mwindow->theme->rmonitor_canvas_h);
	window->resize_window(new_w, new_h);
	window->resize_event(new_w, new_h);
}


int RecordMonitorCanvas::button_release_event()
{
	window->current_operation = MONITOR_NONE;
	return 0;
}

int RecordMonitorCanvas::cursor_motion_event()
{
//SET_TRACE
	if( window->current_operation == MONITOR_TRANSLATE ) {
//SET_TRACE
		record->set_translation(
			get_cursor_x() - window->cursor_x_origin + window->translate_x_origin,
			get_cursor_y() - window->cursor_y_origin + window->translate_y_origin);
//SET_TRACE
	}

	return 0;
}

int RecordMonitorCanvas::cursor_enter_event()
{
	if(mwindow->edl->session->vconfig_in->driver == SCREENCAPTURE)
		set_cursor(MOVE_CURSOR);
	return 0;
}

void RecordMonitorCanvas::reset_translation()
{
	record->set_translation(0, 0);
}

int RecordMonitorCanvas::keypress_event()
{
	if( !get_canvas() ) return 0;

	switch(get_canvas()->get_keypress()) {
	case LEFT:
		record->set_translation(--record->video_x, record->video_y);
		break;
	case RIGHT:
		record->set_translation(++record->video_x, record->video_y);
		break;
	case UP:
		record->set_translation(record->video_x, --record->video_y);
		break;
	case DOWN:
		record->set_translation(record->video_x, ++record->video_y);
		break;
	default:
		return 0;
	}
	return 1;
}


RecordMonitorFullsize::RecordMonitorFullsize(MWindow *mwindow,
	RecordMonitorGUI *window)
 : BC_MenuItem(_("Zoom 100%"))
{
	this->mwindow = mwindow;
	this->window = window;
}
int RecordMonitorFullsize::handle_event()
{
	return 1;
}








// ================================== slippery playback ============================


RecordMonitorThread::RecordMonitorThread(MWindow *mwindow,
	Record *record,
	RecordMonitor *record_monitor)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->record_monitor = record_monitor;
	this->record = record;
	this->ovly = 0;
	reset_parameters();
	output_lock = new Condition(1, "RecordMonitor::output_lock");
	input_lock = new Condition(1, "RecordMonitor::input_lock");
}

void RecordMonitorThread::reset_parameters()
{
	input_frame = 0;
	output_frame = 0;
	shared_data = 0;
	jpeg_engine = 0;
	dv_engine = 0;
	ready = 0;
	done = 1;
}


RecordMonitorThread::~RecordMonitorThread()
{
	stop_playback();
	if( input_frame && !shared_data )
		delete input_frame;
	delete ovly;
	delete output_lock;
	delete input_lock;
}

void RecordMonitorThread::init_output_format()
{
//printf("RecordMonitorThread::init_output_format 1\n");
	switch(mwindow->edl->session->vconfig_in->driver) {
	case SCREENCAPTURE:
		output_colormodel = record->vdevice->get_best_colormodel(record->default_asset);
		break;

	case VIDEO4LINUX2JPEG:
		jpeg_engine = new RecVideoMJPGThread(record, this,
			mwindow->edl->session->vconfig_in->v4l2jpeg_in_fields);
		jpeg_engine->start_rendering();
		output_colormodel = BC_YUV422P;
		break;

	case CAPTURE_FIREWIRE:
	case CAPTURE_IEC61883:
		dv_engine = new RecVideoDVThread(record, this);
		dv_engine->start_rendering();
		output_colormodel = BC_YUV422P;
		break;

	case CAPTURE_JPEG_WEBCAM:
		jpeg_engine = new RecVideoMJPGThread(record, this, 1);
		jpeg_engine->start_rendering();
		output_colormodel = BC_YUV420P;
		break;

	case CAPTURE_YUYV_WEBCAM:
		output_colormodel = BC_YUV422;
		break;


	case CAPTURE_DVB:
	case VIDEO4LINUX2:
	case VIDEO4LINUX2MPEG:
		output_colormodel = record->vdevice->get_best_colormodel(record->default_asset);
//printf("RecordMonitorThread::init_output_format 2 %d\n", output_colormodel);
		break;
	}
}

int RecordMonitorThread::start_playback()
{
	ready = 1;
	done = 0;
	output_frame = 0;
	output_lock->lock("RecordMonitorThread::start_playback");
	Thread::start();
	return 0;
}

int RecordMonitorThread::stop_playback()
{
	if( done ) return 0;
	done = 1;
	output_lock->unlock();
	Thread::join();
//printf("RecordMonitorThread::stop_playback 1\n");

	switch(mwindow->edl->session->vconfig_in->driver) {
	case VIDEO4LINUX2JPEG:
		if( jpeg_engine ) {
			jpeg_engine->stop_rendering();
			delete jpeg_engine;
			jpeg_engine = 0;
		}
		break;

	case CAPTURE_FIREWIRE:
	case CAPTURE_IEC61883:
		if( dv_engine ) {
			dv_engine->stop_rendering();
			delete dv_engine;
			dv_engine = 0;
		}
		break;
	case CAPTURE_DVB:
	case VIDEO4LINUX2MPEG:
		break;
	}
//printf("RecordMonitorThread::stop_playback 4\n");

	return 0;
}

int RecordMonitorThread::write_frame(VFrame *new_frame)
{
	if( ready ) {
		ready = 0;
		shared_data = (new_frame->get_color_model() != BC_COMPRESSED);


// Need to wait until after Record creates the input device before starting monitor
// because the input device deterimes the output format.
// First time
		if( !output_frame ) init_output_format();
		if( !shared_data ) {
			if(!input_frame) input_frame = new VFrame;
			input_frame->allocate_compressed_data(new_frame->get_compressed_size());
			memcpy(input_frame->get_data(),
				new_frame->get_data(),
				new_frame->get_compressed_size());
			input_frame->set_compressed_size(new_frame->get_compressed_size());
			input_frame->set_field2_offset(new_frame->get_field2_offset());
		}
		else {
			input_lock->lock("RecordMonitorThread::write_frame");
			input_frame = new_frame;
		}
		output_lock->unlock();
	}
	return 0;
}

int RecordMonitorThread::render_jpeg()
{
//printf("RecordMonitorThread::render_jpeg 1\n");
	jpeg_engine->render_frame(input_frame, input_frame->get_compressed_size());
//printf("RecordMonitorThread::render_jpeg 2\n");
	return 0;
}

int RecordMonitorThread::render_dv()
{
	dv_engine->render_frame(input_frame, input_frame->get_compressed_size());
	return 0;
}

void RecordMonitorThread::render_uncompressed()
{
	output_frame->transfer_from(input_frame);
}

void RecordMonitorThread::show_output_frame()
{
	if( ovly && ovly->overlay(output_frame) )
		undisplay_vframe();
	record_monitor->device->write_buffer(output_frame, record->edl);
}


void RecordMonitorThread::unlock_input()
{
	if(shared_data) input_lock->unlock();
}

void RecordMonitorThread::lock_input()
{
	if(shared_data) input_lock->lock();
}

int RecordMonitorThread::render_frame()
{
	switch(mwindow->edl->session->vconfig_in->driver) {
	case VIDEO4LINUX2JPEG:
	case CAPTURE_JPEG_WEBCAM:
		render_jpeg();
		break;

	case CAPTURE_FIREWIRE:
	case CAPTURE_IEC61883:
		render_dv();
		break;

	default:
		render_uncompressed();
		break;
	}

	return 0;
}

void RecordMonitorThread::new_output_frame()
{
	record_monitor->device->new_output_buffer(&output_frame, 
		output_colormodel,
		record->edl);
}

void RecordMonitorThread::
display_vframe(VFrame *in, int x, int y, int alpha, double secs, double scale)
{
	delete ovly;
	int ticks = secs * SESSION->vconfig_in->in_framerate;
	scale *= SESSION->vconfig_in->h / 1080.;
	ovly = new RecVideoOverlay(in, x, y, ticks, scale, alpha/255.);
}

RecVideoOverlay::
RecVideoOverlay(VFrame *vframe, int x, int y, int ticks, float scale, float alpha)
{
	this->x = x;
	this->y = y;
	this->ticks = ticks;
	this->scale = scale;
	this->alpha = alpha;
	this->vframe = new VFrame(*vframe);
}

RecVideoOverlay::
~RecVideoOverlay()
{
	delete vframe;
}

int RecVideoOverlay::
overlay(VFrame *out)
{
	VFrame *in = vframe;
	int xx = x * scale, yy = y * scale;
	int w = in->get_w(), h = in->get_h();
	int ww = w * scale, hh = h * scale;
	BC_CModels::transfer(out->get_rows(), in->get_rows(),
		out->get_y(), out->get_u(), out->get_v(),
		in->get_y(), in->get_u(), in->get_v(),
		0, 0, w, h, xx, yy, ww, hh,
		in->get_color_model(), out->get_color_model(), 0,
		in->get_bytes_per_line(), out->get_bytes_per_line());
	return ticks > 0 && --ticks == 0 ? 1 : 0;
}

void RecordMonitorThread::
undisplay_vframe()
{
	delete ovly;  ovly = 0;
}

void RecordMonitorThread::run()
{
//printf("RecordMonitorThread::run 1 %d\n", getpid());
	while(!done) {
// Wait for next frame
//SET_TRACE
		output_lock->lock("RecordMonitorThread::run");

		if(done) {
			unlock_input();
			return;
		}
//SET_TRACE
		new_output_frame();
//SET_TRACE
		render_frame();
//SET_TRACE
		record_monitor->scope_thread->process(output_frame);
//SET_TRACE
		show_output_frame();
//SET_TRACE
		unlock_input();
// Get next frame
		ready = 1;
	}
}



RecVideoMJPGThread::RecVideoMJPGThread(Record *record,
	RecordMonitorThread *thread,
	int fields)
{
	this->record = record;
	this->thread = thread;
	mjpeg = 0;
	this->fields = fields;
}

RecVideoMJPGThread::~RecVideoMJPGThread()
{
}

int RecVideoMJPGThread::start_rendering()
{
	mjpeg = mjpeg_new(record->default_asset->width,
		record->default_asset->height,
		fields);
//printf("RecVideoMJPGThread::start_rendering 1 %p\n", mjpeg);
	return 0;
}

int RecVideoMJPGThread::stop_rendering()
{
//printf("RecVideoMJPGThread::stop_rendering 1 %p\n", mjpeg);
	if(mjpeg) mjpeg_delete(mjpeg);
//printf("RecVideoMJPGThread::stop_rendering 2\n");
	return 0;
}

int RecVideoMJPGThread::render_frame(VFrame *frame, long size)
{
// printf("RecVideoMJPGThread::render_frame %d %02x%02x %02x%02x\n",
// frame->get_field2_offset(),
// frame->get_data()[0],
// frame->get_data()[1],
// frame->get_data()[frame->get_field2_offset()],
// frame->get_data()[frame->get_field2_offset() + 1]);
//frame->set_field2_offset(0);
	mjpeg_decompress(mjpeg,
		frame->get_data(),
		frame->get_compressed_size(),
		frame->get_field2_offset(),
		thread->output_frame->get_rows(),
		thread->output_frame->get_y(),
		thread->output_frame->get_u(),
		thread->output_frame->get_v(),
		thread->output_frame->get_color_model(),
		record->mwindow->preferences->processors);
	return 0;
}




RecVideoDVThread::RecVideoDVThread(Record *record, RecordMonitorThread *thread)
{
	this->record = record;
	this->thread = thread;
	dv = 0;
}

RecVideoDVThread::~RecVideoDVThread()
{
}


int RecVideoDVThread::start_rendering()
{
#ifdef HAVE_DV
	dv = dv_new();
#endif
	return 0;
}

int RecVideoDVThread::stop_rendering()
{
#ifdef HAVE_DV
	if( dv ) { dv_delete(((dv_t*)dv));  dv = 0; }
#endif
	return 0;
}

int RecVideoDVThread::render_frame(VFrame *frame, long size)
{
#ifdef HAVE_DV
	unsigned char *yuv_planes[3];
	yuv_planes[0] = thread->output_frame->get_y();
	yuv_planes[1] = thread->output_frame->get_u();
	yuv_planes[2] = thread->output_frame->get_v();
	dv_read_video(((dv_t*)dv),
		yuv_planes,
		frame->get_data(),
		frame->get_compressed_size(),
		thread->output_frame->get_color_model());
#endif
	return 0;
}

