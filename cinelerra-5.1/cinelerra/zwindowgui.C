
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#include "bcwindow.h"
#include "canvas.h"
#include "edl.h"
#include "edlsession.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mbuttons.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "theme.h"
#include "tracks.h"
#include "zwindow.h"
#include "zwindowgui.h"

ZWindowGUI::ZWindowGUI(MWindow *mwindow, ZWindow *zwindow, Mixer *mixer)
 : BC_Window(zwindow->title, mixer->x, mixer->y, mixer->w, mixer->h,
	100, 75, 1, 1, 0)
{
	this->mwindow = mwindow;
	this->zwindow = zwindow;

	canvas = 0;
	playback_engine = 0;
	highlighted = 0;
}

ZWindowGUI::~ZWindowGUI()
{
	delete playback_engine;
	delete canvas;
}

void ZWindowGUI::create_objects()
{
	lock_window("ZWindowGUI::create_objects");

	canvas = new ZWindowCanvas(mwindow, this, 10,10, get_w()-20,get_h()-20);
	canvas->create_objects(mwindow->edl);
	playback_engine = new PlaybackEngine(mwindow, canvas);
	playback_engine->create_objects();

	deactivate();
	show_window();
	unlock_window();
}

int ZWindowGUI::resize_event(int w, int h)
{
	canvas->reposition_window(0, 10,10, w-20,h-20);
	zwindow->reposition(get_x(), get_y(), w, h);
	BC_WindowBase::resize_event(w, h);
	return 1;
}
int ZWindowGUI::translation_event()
{
	return resize_event(get_w(),get_h());
}

int ZWindowGUI::close_event()
{
	set_done(0);
	return 1;
}

int ZWindowGUI::keypress_event()
{
	int key = get_keypress();
	if( key == 'w' || key == 'W' ) {
		close_event();
		return 1;
	}
	unlock_window();
	int result = 1;
	switch( key ) {
	case 'f':
		if( mwindow->session->zwindow_fullscreen )
			canvas->stop_fullscreen();
		else
			canvas->start_fullscreen();
		break;
	case ESC:
		if( mwindow->session->zwindow_fullscreen )
			canvas->stop_fullscreen();
		break;
	default:
		mwindow->gui->lock_window("ZWindowGUI::keypress_event");
		result = mwindow->gui->mbuttons->transport->do_keypress(key);
		mwindow->gui->unlock_window();
	}

	lock_window("ZWindowGUI::keypress_event 1");
	return result;
}

int ZWindowGUI::button_press_event()
{
	mwindow->select_zwindow(zwindow);
	if( get_double_click() ) {
		unlock_window();
		mwindow->gui->lock_window("ZWindowGUI::button_press_event 0");
		LocalSession *local_session = mwindow->edl->local_session;
		double start = local_session->get_selectionstart(1);
		double end = local_session->get_selectionend(1);
		if( EQUIV(start, end) ) {
			start = mwindow->edl->tracks->total_recordable_length();
			if( start < 0 ) start = end;
		}
		if( (end-start) > 1e-4 ) {
			LocalSession *zlocal_session = zwindow->edl->local_session;
			zlocal_session->set_selectionstart(end);
			zlocal_session->set_selectionend(end);
			zlocal_session->set_inpoint(start);
			zlocal_session->set_outpoint(end);
			double orig_inpoint = local_session->get_inpoint();
			double orig_outpoint = local_session->get_outpoint();
			local_session->set_selectionstart(end);
			local_session->set_selectionend(end);
			local_session->set_inpoint(start);
			local_session->set_outpoint(end);
			mwindow->overwrite(zwindow->edl, 0);
			local_session->set_inpoint(orig_inpoint);
			local_session->set_outpoint(orig_outpoint);
			mwindow->gui->update_timebar(1);
		}
		mwindow->gui->unlock_window();
		lock_window("ZWindowGUI::button_press_event 1");
	}
	if( !canvas->get_canvas() ) return 0;
	return canvas->button_press_event_base(canvas->get_canvas());
}

int ZWindowGUI::cursor_leave_event()
{
	if( !canvas->get_canvas() ) return 0;
	return canvas->cursor_leave_event_base(canvas->get_canvas());
}

int ZWindowGUI::cursor_enter_event()
{
	if( !canvas->get_canvas() ) return 0;
	return canvas->cursor_enter_event_base(canvas->get_canvas());
}

int ZWindowGUI::button_release_event()
{
	if( !canvas->get_canvas() ) return 0;
	return canvas->button_release_event();
}

int ZWindowGUI::cursor_motion_event()
{
	BC_WindowBase *cvs = canvas->get_canvas();
	if( !cvs ) return 0;
	cvs->unhide_cursor();
	return canvas->cursor_motion_event();
}

int ZWindowGUI::draw_overlays()
{
	BC_WindowBase *cvs = canvas->get_canvas();
	if( !cvs || cvs->get_video_on() ) return 0;
	if( highlighted != zwindow->highlighted ) {
		highlighted = zwindow->highlighted;
		cvs->set_color(WHITE);
		cvs->set_inverse();
		cvs->draw_rectangle(0, 0, cvs->get_w(), cvs->get_h());
		cvs->draw_rectangle(1, 1, cvs->get_w() - 2, cvs->get_h() - 2);
		cvs->set_opaque();
	}
	return 1;
}

ZWindowCanvas::ZWindowCanvas(MWindow *mwindow, ZWindowGUI *gui,
		int x, int y, int w, int h)
 : Canvas(mwindow, gui, x,y, w,h, 0,0,0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

void ZWindowCanvas::close_source()
{
	gui->unlock_window();
	gui->zwindow->zgui->playback_engine->interrupt_playback(1);
	gui->lock_window("ZWindowCanvas::close_source");
	EDL *edl = gui->zwindow->edl;
	if( edl ) {
		gui->zwindow->edl = 0;
		edl->remove_user();
	}
}


void ZWindowCanvas::draw_refresh(int flush)
{
	EDL *edl = gui->zwindow->edl;
	BC_WindowBase *cvs = get_canvas();
	int dirty = 0;

	if( !cvs->get_video_on() ) {
		cvs->clear_box(0, 0, cvs->get_w(), cvs->get_h());
		gui->highlighted = 0;
		dirty = 1;
	}
	if( !cvs->get_video_on() && refresh_frame && edl ) {
		float in_x1, in_y1, in_x2, in_y2;
		float out_x1, out_y1, out_x2, out_y2;
		get_transfers(edl,
			in_x1, in_y1, in_x2, in_y2,
			out_x1, out_y1, out_x2, out_y2);
		cvs->draw_vframe(refresh_frame,
			(int)out_x1, (int)out_y1, (int)(out_x2 - out_x1), (int)(out_y2 - out_y1),
			(int)in_x1, (int)in_y1, (int)(in_x2 - in_x1), (int)(in_y2 - in_y1), 0);
		dirty = 1;
	}
	
	if( gui->draw_overlays() )
		dirty = 1;

	if( dirty )
		cvs->flash(flush);
}

int ZWindowCanvas::get_fullscreen()
{
	return mwindow->session->zwindow_fullscreen;
}

void ZWindowCanvas::set_fullscreen(int value)
{
	mwindow->session->zwindow_fullscreen = value;
}

