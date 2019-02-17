
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

#include "automation.h"
#include "autos.h"
#include "bcsignals.h"
#include "canvas.h"
#include "clip.h"
#include "cpanel.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "cursors.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "cwindowtool.h"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainmenu.h"
#include "mainundo.h"
#include "mainsession.h"
#include "maskauto.h"
#include "maskautos.h"
#include "mbuttons.h"
#include "meterpanel.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "mwindow.h"
#include "playback3d.h"
#include "playtransport.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "vtrack.h"


static double my_zoom_table[] =
{
	0.25,
	0.33,
	0.50,
	0.75,
	1.0,
	1.5,
	2.0,
	3.0,
	4.0
};

static int total_zooms = sizeof(my_zoom_table) / sizeof(double);


CWindowGUI::CWindowGUI(MWindow *mwindow, CWindow *cwindow)
 : BC_Window(_(PROGRAM_NAME ": Compositor"),
 	mwindow->session->cwindow_x,
    mwindow->session->cwindow_y,
    mwindow->session->cwindow_w,
    mwindow->session->cwindow_h,
    100,
    100,
    1,
    1,
    1,
	BC_WindowBase::get_resources()->bg_color,
	mwindow->get_cwindow_display())
{
	this->mwindow = mwindow;
	this->cwindow = cwindow;
	affected_track = 0;
	affected_x = affected_y = affected_z = 0;
	mask_keyframe = 0;
	orig_mask_keyframe = new MaskAuto(0, 0);
	affected_point = 0;
	x_offset = y_offset = 0;
	x_origin = y_origin = 0;
	current_operation = CWINDOW_NONE;
	tool_panel = 0;
	active = 0;
	inactive = 0;
	crop_handle = -1; crop_translate = 0;
	crop_origin_x = crop_origin_y = 0;
	crop_origin_x1 = crop_origin_y1 = 0;
	crop_origin_x2 = crop_origin_y2 = 0;
	eyedrop_visible = 0;
	eyedrop_x = eyedrop_y = 0;
	ruler_origin_x = ruler_origin_y = 0;
	ruler_handle = -1; ruler_translate = 0;
	center_x = center_y = center_z = 0;
	control_in_x = control_in_y = 0;
	control_out_x = control_out_y = 0;
	translating_zoom = 0;
	highlighted = 0;
}

CWindowGUI::~CWindowGUI()
{
	if(tool_panel) delete tool_panel;
 	delete meters;
 	delete composite_panel;
 	delete canvas;
 	delete transport;
 	delete edit_panel;
 	delete zoom_panel;
	delete active;
	delete inactive;
	delete orig_mask_keyframe;
}

void CWindowGUI::create_objects()
{
	lock_window("CWindowGUI::create_objects");
	set_icon(mwindow->theme->get_image("cwindow_icon"));

	active = new BC_Pixmap(this, mwindow->theme->get_image("cwindow_active"));
	inactive = new BC_Pixmap(this, mwindow->theme->get_image("cwindow_inactive"));

	mwindow->theme->get_cwindow_sizes(this, mwindow->session->cwindow_controls);
	mwindow->theme->draw_cwindow_bg(this);
	flash();

// Meters required by composite panel
	meters = new CWindowMeters(mwindow,
		this,
		mwindow->theme->cmeter_x,
		mwindow->theme->cmeter_y,
		mwindow->theme->cmeter_h);
	meters->create_objects();


	composite_panel = new CPanel(mwindow,
		this,
		mwindow->theme->ccomposite_x,
		mwindow->theme->ccomposite_y,
		mwindow->theme->ccomposite_w,
		mwindow->theme->ccomposite_h);
	composite_panel->create_objects();

	canvas = new CWindowCanvas(mwindow, this);

	canvas->create_objects(mwindow->edl);
	canvas->use_cwindow();


	add_subwindow(timebar = new CTimeBar(mwindow,
		this,
		mwindow->theme->ctimebar_x,
		mwindow->theme->ctimebar_y,
		mwindow->theme->ctimebar_w,
		mwindow->theme->ctimebar_h));
	timebar->create_objects();

#ifdef USE_SLIDER
 	add_subwindow(slider = new CWindowSlider(mwindow,
 		cwindow,
		mwindow->theme->cslider_x,
 		mwindow->theme->cslider_y,
 		mwindow->theme->cslider_w));
#endif

	transport = new CWindowTransport(mwindow,
		this,
		mwindow->theme->ctransport_x,
		mwindow->theme->ctransport_y);
	transport->create_objects();
#ifdef USE_SLIDER
	transport->set_slider(slider);
#endif

	edit_panel = new CWindowEditing(mwindow, cwindow);
	edit_panel->set_meters(meters);
	edit_panel->create_objects();

// 	add_subwindow(clock = new MainClock(mwindow,
// 		mwindow->theme->ctime_x,
// 		mwindow->theme->ctime_y));

	zoom_panel = new CWindowZoom(mwindow,
		this,
		mwindow->theme->czoom_x,
		mwindow->theme->czoom_y,
		mwindow->theme->czoom_w);
	zoom_panel->create_objects();
	zoom_panel->zoom_text->add_item(new BC_MenuItem(auto_zoom = _(AUTO_ZOOM)));
	if( !mwindow->edl->session->cwindow_scrollbars )
		zoom_panel->set_text(auto_zoom);

// 	destination = new CWindowDestination(mwindow,
// 		this,
// 		mwindow->theme->cdest_x,
// 		mwindow->theme->cdest_y);
// 	destination->create_objects();

// Must create after meter panel
	tool_panel = new CWindowTool(mwindow, this);
	tool_panel->Thread::start();

	set_operation(mwindow->edl->session->cwindow_operation);
	draw_status(0);
	unlock_window();
}

int CWindowGUI::translation_event()
{
	mwindow->session->cwindow_x = get_x();
	mwindow->session->cwindow_y = get_y();
	return 0;
}

int CWindowGUI::resize_event(int w, int h)
{
	mwindow->session->cwindow_x = get_x();
	mwindow->session->cwindow_y = get_y();
	mwindow->session->cwindow_w = w;
	mwindow->session->cwindow_h = h;

	mwindow->theme->get_cwindow_sizes(this, mwindow->session->cwindow_controls);
	mwindow->theme->draw_cwindow_bg(this);
	flash(0);

	composite_panel->reposition_buttons(mwindow->theme->ccomposite_x,
		mwindow->theme->ccomposite_y, mwindow->theme->ccomposite_h);

	canvas->reposition_window(mwindow->edl,
		mwindow->theme->ccanvas_x,
		mwindow->theme->ccanvas_y,
		mwindow->theme->ccanvas_w,
		mwindow->theme->ccanvas_h);

	timebar->resize_event();

#ifdef USE_SLIDER
 	slider->reposition_window(mwindow->theme->cslider_x,
 		mwindow->theme->cslider_y,
 		mwindow->theme->cslider_w);
// Recalibrate pointer motion range
	slider->set_position();
#endif

	transport->reposition_buttons(mwindow->theme->ctransport_x,
		mwindow->theme->ctransport_y);

	edit_panel->reposition_buttons(mwindow->theme->cedit_x,
		mwindow->theme->cedit_y);

//	clock->reposition_window(mwindow->theme->ctime_x,
//		mwindow->theme->ctime_y);

	zoom_panel->reposition_window(mwindow->theme->czoom_x,
		mwindow->theme->czoom_y);

// 	destination->reposition_window(mwindow->theme->cdest_x,
// 		mwindow->theme->cdest_y);

	meters->reposition_window(mwindow->theme->cmeter_x,
		mwindow->theme->cmeter_y,
		-1,
		mwindow->theme->cmeter_h);

	draw_status(0);

	BC_WindowBase::resize_event(w, h);
	return 1;
}

int CWindowGUI::button_press_event()
{
	if( current_operation == CWINDOW_NONE &&
	    mwindow->edl != 0 && canvas->get_canvas() &&
	    mwindow->edl->session->cwindow_click2play &&
	    canvas->get_canvas()->get_cursor_over_window() ) {
		switch( get_buttonpress() ) {
		case LEFT_BUTTON:
			if( !cwindow->playback_engine->is_playing_back ) {
				double length = mwindow->edl->tracks->total_playable_length();
				double position = cwindow->playback_engine->get_tracking_position();
				if( position >= length ) transport->goto_start();
			}
			return transport->forward_play->handle_event();
		case MIDDLE_BUTTON:
			if( !cwindow->playback_engine->is_playing_back ) {
				double position = cwindow->playback_engine->get_tracking_position();
				if( position <= 0 ) transport->goto_end();
			}
			return transport->reverse_play->handle_event();
		case RIGHT_BUTTON:  // activates popup
			break;
		case WHEEL_UP:
			return transport->frame_forward_play->handle_event();
		case WHEEL_DOWN:
			return transport->frame_reverse_play->handle_event();
		}
	}
	if(canvas->get_canvas())
		return canvas->button_press_event_base(canvas->get_canvas());
	return 0;
}

int CWindowGUI::cursor_leave_event()
{
	if(canvas->get_canvas())
		return canvas->cursor_leave_event_base(canvas->get_canvas());
	return 0;
}

int CWindowGUI::cursor_enter_event()
{
	if(canvas->get_canvas())
		return canvas->cursor_enter_event_base(canvas->get_canvas());
	return 0;
}

int CWindowGUI::button_release_event()
{
	if(canvas->get_canvas())
		return canvas->button_release_event();
	return 0;
}

int CWindowGUI::cursor_motion_event()
{
	if(canvas->get_canvas())
	{
		canvas->get_canvas()->unhide_cursor();
		return canvas->cursor_motion_event();
	}
	return 0;
}







void CWindowGUI::draw_status(int flush)
{
	if( (canvas->get_canvas() && canvas->get_canvas()->get_video_on()) ||
		canvas->is_processing )
	{
		draw_pixmap(active,
			mwindow->theme->cstatus_x,
			mwindow->theme->cstatus_y);
	}
	else
	{
		draw_pixmap(inactive,
			mwindow->theme->cstatus_x,
			mwindow->theme->cstatus_y);
	}


// printf("CWindowGUI::draw_status %d %d %d\n", __LINE__, mwindow->theme->cstatus_x,
// 		mwindow->theme->cstatus_y);
	flash(mwindow->theme->cstatus_x,
		mwindow->theme->cstatus_y,
		active->get_w(),
		active->get_h(),
		flush);
}

float CWindowGUI::get_auto_zoom()
{
	float conformed_w, conformed_h;
	mwindow->edl->calculate_conformed_dimensions(0, conformed_w, conformed_h);
	float zoom_x = canvas->w / conformed_w;
	float zoom_y = canvas->h / conformed_h;
	return zoom_x < zoom_y ? zoom_x : zoom_y;
}

void CWindowGUI::zoom_canvas(double value, int update_menu)
{
	float x = 0, y = 0;
	float zoom = !value ? get_auto_zoom() : value;
	EDL *edl = mwindow->edl;
	edl->session->cwindow_scrollbars = !value ? 0 : 1;
	if( value ) {
		float cx = 0.5f * canvas->w;  x = cx;
		float cy = 0.5f * canvas->h;  y = cy;
		canvas->canvas_to_output(edl, 0, x, y);
		canvas->update_zoom(0, 0, zoom);
		float zoom_x, zoom_y, conformed_w, conformed_h;
		canvas->get_zooms(edl, 0, zoom_x, zoom_y, conformed_w, conformed_h);
		x -= cx / zoom_x;
		y -= cy / zoom_y;

	}
	canvas->update_zoom((int)(x+0.5), (int)(y+0.5), zoom);

	if( update_menu )
		zoom_panel->update(value);
	if( mwindow->edl->session->cwindow_operation == CWINDOW_ZOOM )
		composite_panel->cpanel_zoom->update(zoom);

	canvas->reposition_window(mwindow->edl,
		mwindow->theme->ccanvas_x, mwindow->theme->ccanvas_y,
		mwindow->theme->ccanvas_w, mwindow->theme->ccanvas_h);
	canvas->draw_refresh();
}

void CWindowGUI::set_operation(int value)
{
	switch( value ) {
	case CWINDOW_TOOL_WINDOW:
		mwindow->edl->session->tool_window = !mwindow->edl->session->tool_window;
		composite_panel->operation[CWINDOW_TOOL_WINDOW]->update(mwindow->edl->session->tool_window);
		tool_panel->update_show_window();
		return;
	case CWINDOW_TITLESAFE:
		mwindow->edl->session->safe_regions = !mwindow->edl->session->safe_regions;
		composite_panel->operation[CWINDOW_TITLESAFE]->update(mwindow->edl->session->safe_regions);
		value = mwindow->edl->session->cwindow_operation;
		break;
	default:
		mwindow->edl->session->cwindow_operation = value;
		composite_panel->set_operation(value);
		break;
	}

	edit_panel->update();
	tool_panel->start_tool(value);
	canvas->draw_refresh();
}

void CWindowGUI::update_tool()
{
	tool_panel->update_values();
}

int CWindowGUI::close_event()
{
	cwindow->hide_window();
	return 1;
}


int CWindowGUI::keypress_event()
{
	int result = 0;
	int cwindow_operation = CWINDOW_NONE;

	switch( get_keypress() ) {
	case 'w':
	case 'W':
		close_event();
		result = 1;
		break;
	case '+':
	case '=':
		keyboard_zoomin();
		result = 1;
		break;
	case '-':
		keyboard_zoomout();
		result = 1;
		break;
	case 'f':
		unlock_window();
		if(mwindow->session->cwindow_fullscreen)
			canvas->stop_fullscreen();
		else
			canvas->start_fullscreen();
		lock_window("CWindowGUI::keypress_event 1");
		result = 1;
		break;
	case 'x':
		if( ctrl_down() || shift_down() || alt_down() ) break;
		unlock_window();
		mwindow->gui->lock_window("CWindowGUI::keypress_event 2");
		mwindow->cut();
		mwindow->gui->unlock_window();
		lock_window("CWindowGUI::keypress_event 2");
		result = 1;
		break;
	case DELETE:
		unlock_window();
		mwindow->gui->lock_window("CWindowGUI::keypress_event 2");
		mwindow->clear_entry();
		mwindow->gui->unlock_window();
		lock_window("CWindowGUI::keypress_event 3");
		result = 1;
		break;
	case ESC:
		unlock_window();
		if(mwindow->session->cwindow_fullscreen)
			canvas->stop_fullscreen();
		lock_window("CWindowGUI::keypress_event 4");
		result = 1;
		break;
	case LEFT:
		if( !ctrl_down() ) {
			int alt_down = this->alt_down();
			int shift_down = this->shift_down();
			unlock_window();
			stop_transport(0);
			mwindow->gui->lock_window("CWindowGUI::keypress_event 5");
			if( alt_down )
				mwindow->prev_edit_handle(shift_down);
			else
				mwindow->move_left();
			mwindow->gui->unlock_window();
			lock_window("CWindowGUI::keypress_event 6");
 			result = 1;
		}
		break;

	case ',':
		if( !ctrl_down() && !alt_down() ) {
			unlock_window();
			stop_transport(0);
			mwindow->gui->lock_window("CWindowGUI::keypress_event 7");
			mwindow->move_left();
			mwindow->gui->unlock_window();
			lock_window("CWindowGUI::keypress_event 8");
			result = 1;
		}
		break;

	case RIGHT:
		if( !ctrl_down() ) {
			int alt_down = this->alt_down();
			int shift_down = this->shift_down();
			unlock_window();
			stop_transport(0);
			mwindow->gui->lock_window("CWindowGUI::keypress_event 8");
			if( alt_down )
				mwindow->next_edit_handle(shift_down);
			else
				mwindow->move_right();
			mwindow->gui->unlock_window();
			lock_window("CWindowGUI::keypress_event 9");
			result = 1;
		}
		break;

	case '.':
		if( !ctrl_down() && !alt_down() ) {
			unlock_window();
			stop_transport(0);
			mwindow->gui->lock_window("CWindowGUI::keypress_event 10");
			mwindow->move_right();
			mwindow->gui->unlock_window();
			lock_window("CWindowGUI::keypress_event 11");
			result = 1;
		}
		break;
	}

	if( !result && cwindow_operation < 0 && ctrl_down() && shift_down() ) {
		switch( get_keypress() ) {
		case KEY_F1 ... KEY_F4: // mainmenu, load layout
			resend_event(mwindow->gui);
			result = 1;
			break;
		}
	}
	if( !result && !ctrl_down() ) {
		switch( get_keypress() ) {
		case KEY_F1:
			if( shift_down() ) {
				mwindow->toggle_camera_xyz();  result = 1;
			}
			else
				cwindow_operation = CWINDOW_PROTECT;
			break;
		case KEY_F2:
			if( shift_down() ) {
				mwindow->toggle_projector_xyz();  result = 1;
			}
			else
				cwindow_operation = CWINDOW_ZOOM;
			break;
		}
	}
	if( !result && cwindow_operation < 0 && !ctrl_down() && !shift_down() ) {
		switch( get_keypress() ) {
		case KEY_F3:	cwindow_operation = CWINDOW_MASK;	break;
		case KEY_F4:	cwindow_operation = CWINDOW_RULER;	break;
		case KEY_F5:	cwindow_operation = CWINDOW_CAMERA;	break;
		case KEY_F6:	cwindow_operation = CWINDOW_PROJECTOR;	break;
		case KEY_F7:	cwindow_operation = CWINDOW_CROP;	break;
		case KEY_F8:	cwindow_operation = CWINDOW_EYEDROP;	break;
		case KEY_F9:	cwindow_operation = CWINDOW_TOOL_WINDOW; break;
		case KEY_F10:	cwindow_operation = CWINDOW_TITLESAFE;	break;
		}
	}
	if( !result && cwindow_operation < 0 && !ctrl_down() ) {
		switch( get_keypress() ) {
		case KEY_F11:
			if( !shift_down() )
				canvas->reset_camera();
			else
				canvas->camera_keyframe();
			result = 1;
			break;
		case KEY_F12:
			if( !shift_down() )
				canvas->reset_projector();
			else
				canvas->projector_keyframe();
			result = 1;
			break;
		}
	}

	if( cwindow_operation >= 0 ) {
		set_operation(cwindow_operation);
		result = 1;
	}

	if( !result )
		result = transport->keypress_event();

	return result;
}


void CWindowGUI::reset_affected()
{
	affected_x = 0;
	affected_y = 0;
	affected_z = 0;
}

void CWindowGUI::keyboard_zoomin()
{
//	if(mwindow->edl->session->cwindow_scrollbars)
//	{
		zoom_panel->zoom_tumbler->handle_up_event();
//	}
//	else
//	{
//	}
}

void CWindowGUI::keyboard_zoomout()
{
//	if(mwindow->edl->session->cwindow_scrollbars)
//	{
		zoom_panel->zoom_tumbler->handle_down_event();
//	}
//	else
//	{
//	}
}

void CWindowGUI::sync_parameters(int change_type, int redraw, int overlay)
{
	if( redraw ) {
		update_tool();
		canvas->draw_refresh();
	}
	if( change_type < 0 && !overlay ) return;
	unlock_window();
	if( change_type >= 0 ) {
		mwindow->restart_brender();
		mwindow->sync_parameters(change_type);
	}
	if( overlay ) {
		mwindow->gui->lock_window("CWindow::camera_keyframe");
		mwindow->gui->draw_overlays(1);
		mwindow->gui->unlock_window();
	}
	lock_window("CWindowGUI::sync_parameters");
}

void CWindowGUI::drag_motion()
{
	if(get_hidden()) return;

	if(mwindow->session->current_operation != DRAG_ASSET &&
		mwindow->session->current_operation != DRAG_VTRANSITION &&
		mwindow->session->current_operation != DRAG_VEFFECT) return;
	int need_highlight = cursor_above() && get_cursor_over_window();
	if( highlighted == need_highlight ) return;
	highlighted = need_highlight;
	canvas->draw_refresh();
}

int CWindowGUI::drag_stop()
{
	int result = 0;
	if(get_hidden()) return 0;
	if( !highlighted ) return 0;
	if( mwindow->session->current_operation != DRAG_ASSET &&
	    mwindow->session->current_operation != DRAG_VTRANSITION &&
	    mwindow->session->current_operation != DRAG_VEFFECT) return 0;
	highlighted = 0;
	canvas->draw_refresh();
	result = 1;

	if(mwindow->session->current_operation == DRAG_ASSET)
	{
		if(mwindow->session->drag_assets->total ||
			mwindow->session->drag_clips->total)
		{
			mwindow->gui->lock_window("CWindowGUI::drag_stop 1");
			mwindow->undo->update_undo_before(_("insert assets"), 0);
		}

		if(mwindow->session->drag_assets->total)
		{
			mwindow->clear(0);
			mwindow->load_assets(mwindow->session->drag_assets,
				mwindow->edl->local_session->get_selectionstart(),
				LOADMODE_PASTE,
				mwindow->session->track_highlighted,
				0,
				mwindow->edl->session->labels_follow_edits,
				mwindow->edl->session->plugins_follow_edits,
				mwindow->edl->session->autos_follow_edits,
				0); // overwrite
		}

		if(mwindow->session->drag_clips->total)
		{
			mwindow->clear(0);
			mwindow->paste_edls(mwindow->session->drag_clips,
				LOADMODE_PASTE,
				mwindow->session->track_highlighted,
				mwindow->edl->local_session->get_selectionstart(),
				mwindow->edl->session->labels_follow_edits,
				mwindow->edl->session->plugins_follow_edits,
				mwindow->edl->session->autos_follow_edits,
				0); // overwrite
		}

		if(mwindow->session->drag_assets->total ||
			mwindow->session->drag_clips->total)
		{
			mwindow->save_backup();
			mwindow->restart_brender();
			mwindow->gui->update(1, NORMAL_DRAW, 1, 1, 0, 1, 0);
			mwindow->undo->update_undo_after(_("insert assets"), LOAD_ALL);
			mwindow->gui->unlock_window();
			sync_parameters(CHANGE_ALL);
		}
	}

	if(mwindow->session->current_operation == DRAG_VEFFECT)
	{
//printf("CWindowGUI::drag_stop 1\n");
		Track *affected_track = cwindow->calculate_affected_track();
//printf("CWindowGUI::drag_stop 2\n");

		mwindow->gui->lock_window("CWindowGUI::drag_stop 3");
		mwindow->insert_effects_cwindow(affected_track);
		mwindow->session->current_operation = NO_OPERATION;
		mwindow->gui->unlock_window();
	}

	if(mwindow->session->current_operation == DRAG_VTRANSITION)
	{
		Track *affected_track = cwindow->calculate_affected_track();
		mwindow->gui->lock_window("CWindowGUI::drag_stop 4");
		mwindow->paste_transition_cwindow(affected_track);
		mwindow->session->current_operation = NO_OPERATION;
		mwindow->gui->unlock_window();
	}

	return result;
}

void CWindowGUI::update_meters()
{
	if(mwindow->edl->session->cwindow_meter != meters->visible)
	{
		meters->set_meters(meters->meter_count, mwindow->edl->session->cwindow_meter);
		mwindow->theme->get_cwindow_sizes(this, mwindow->session->cwindow_controls);
		resize_event(get_w(), get_h());
	}
}

void CWindowGUI::stop_transport(const char *lock_msg)
{
	if( lock_msg ) unlock_window();
	mwindow->stop_transport();
	if( lock_msg ) lock_window(lock_msg);
}


CWindowEditing::CWindowEditing(MWindow *mwindow, CWindow *cwindow)
 : EditPanel(mwindow, cwindow->gui, CWINDOW_ID,
		mwindow->theme->cedit_x, mwindow->theme->cedit_y,
		mwindow->edl->session->editing_mode,
		0, // use_editing_mode
		1, // use_keyframe
		0, // use_splice
		0, // use_overwrite
		1, // use_copy
		1, // use_paste
		1, // use_undo
		0, // use_fit
		0, // locklabels
		1, // use_labels
		1, // use_toclip
		1, // use_meters
		1, // use_cut
		0, // use_commerical
		0, // use_goto
		1) // use_clk2play
{
	this->mwindow = mwindow;
	this->cwindow = cwindow;
}

#define relock_cm(s) \
 cwindow->gui->unlock_window(); \
 mwindow->gui->lock_window("CWindowEditing::" s)
#define relock_mc(s) \
 mwindow->gui->unlock_window(); \
 cwindow->gui->lock_window("CWindowEditing::" s)
#define panel_fn(fn, args, s) \
 CWindowEditing::fn args { relock_cm(#fn); s; relock_mc(#fn); }

// transmit lock to mwindow, and run mbutton->edit_panel->s
#define panel_btn(fn, args, s) \
 panel_fn(panel_##fn, args, mwindow->gui->mbuttons->edit_panel->panel_##s)


double CWindowEditing::get_position()
{
	relock_cm("get_position");
	double ret = mwindow->edl->local_session->get_selectionstart(1);
	relock_mc("get_position");
	return ret;
}

void CWindowEditing::set_position(double position)
{
	relock_cm("set_position");
	mwindow->gui->mbuttons->edit_panel->set_position(position);
	relock_mc("set_position");
}

void CWindowEditing::set_click_to_play(int v)
{
	relock_cm("set_position");
	mwindow->edl->session->cwindow_click2play = v;
	relock_mc("set_position");
	click2play->update(v);
}


void panel_btn(stop_transport,(), stop_transport())
void panel_btn(toggle_label,(), toggle_label())
void panel_btn(next_label,(int cut), next_label(cut))
void panel_btn(prev_label,(int cut), prev_label(cut))
void panel_btn(next_edit,(int cut), next_edit(cut))
void panel_btn(prev_edit,(int cut), prev_edit(cut))
void panel_fn(panel_copy_selection,(), mwindow->copy())
void CWindowEditing::panel_overwrite_selection() {} // not used
void CWindowEditing::panel_splice_selection() {} // not used
void panel_btn(set_inpoint,(), set_inpoint())
void panel_btn(set_outpoint,(), set_outpoint())
void panel_btn(unset_inoutpoint,(), unset_inoutpoint())
void panel_fn(panel_to_clip,(), mwindow->to_clip(mwindow->edl, _("main window: "), 0))
void panel_btn(cut,(), cut())
void panel_btn(paste,(), paste())
void panel_btn(fit_selection,(), fit_selection())
void panel_btn(fit_autos,(int all), fit_autos(all))
void panel_btn(set_editing_mode,(int mode), set_editing_mode(mode))
void panel_btn(set_auto_keyframes,(int v), set_auto_keyframes(v))
void panel_btn(set_labels_follow_edits,(int v), set_labels_follow_edits(v))


CWindowMeters::CWindowMeters(MWindow *mwindow,
	CWindowGUI *gui, int x, int y, int h)
 : MeterPanel(mwindow, gui, x, y, -1, h,
		mwindow->edl->session->audio_channels,
		mwindow->edl->session->cwindow_meter,
		0, 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowMeters::~CWindowMeters()
{
}

int CWindowMeters::change_status_event(int new_status)
{
	mwindow->edl->session->cwindow_meter = new_status;
	gui->update_meters();
	return 1;
}




CWindowZoom::CWindowZoom(MWindow *mwindow, CWindowGUI *gui, int x, int y, int w)
 : ZoomPanel(mwindow, gui, (double)mwindow->edl->session->cwindow_zoom,
	x, y, w, my_zoom_table, total_zooms, ZOOM_PERCENTAGE)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowZoom::~CWindowZoom()
{
}

void CWindowZoom::update(double value)
{
	char string[BCSTRLEN];
	const char *cp = string;
	if( value ) {
		this->value = value;
		int frac = value >= 1.0f ? 1 :
			   value >= 0.1f ? 2 :
			   value >= .01f ? 3 : 4;
		sprintf(string, "x %.*f", frac, value);
	}
	else
		cp = gui->auto_zoom;
	ZoomPanel::update(cp);
}

int CWindowZoom::handle_event()
{
	double value = !strcasecmp(gui->auto_zoom, get_text()) ? 0 : get_value();
	gui->zoom_canvas(value, 0);
	return 1;
}



#ifdef USE_SLIDER
CWindowSlider::CWindowSlider(MWindow *mwindow, CWindow *cwindow, int x, int y, int pixels)
 : BC_PercentageSlider(x,
			y,
			0,
			pixels,
			pixels,
			0,
			1,
			0)
{
	this->mwindow = mwindow;
	this->cwindow = cwindow;
	set_precision(0.00001);
}

CWindowSlider::~CWindowSlider()
{
}

int CWindowSlider::handle_event()
{
	cwindow->update_position((double)get_value());
	return 1;
}

void CWindowSlider::set_position()
{
	double new_length = mwindow->edl->tracks->total_length();
	update(mwindow->theme->cslider_w,
		mwindow->edl->local_session->get_selectionstart(1),
		0, new_length);
}


int CWindowSlider::increase_value()
{
	unlock_window();
	cwindow->gui->transport->handle_transport(SINGLE_FRAME_FWD);
	lock_window("CWindowSlider::increase_value");
	return 1;
}

int CWindowSlider::decrease_value()
{
	unlock_window();
	cwindow->gui->transport->handle_transport(SINGLE_FRAME_REWIND);
	lock_window("CWindowSlider::decrease_value");
	return 1;
}


// CWindowDestination::CWindowDestination(MWindow *mwindow, CWindowGUI *cwindow, int x, int y)
//  : BC_PopupTextBox(cwindow,
//  	&cwindow->destinations,
// 	cwindow->destinations.values[cwindow->cwindow->destination]->get_text(),
// 	x,
// 	y,
// 	70,
// 	200)
// {
// 	this->mwindow = mwindow;
// 	this->cwindow = cwindow;
// }
//
// CWindowDestination::~CWindowDestination()
// {
// }
//
// int CWindowDestination::handle_event()
// {
// 	return 1;
// }
#endif // USE_SLIDER






CWindowTransport::CWindowTransport(MWindow *mwindow,
	CWindowGUI *gui,
	int x,
	int y)
 : PlayTransport(mwindow,
	gui,
	x,
	y)
{
	this->gui = gui;
}

EDL* CWindowTransport::get_edl()
{
	return mwindow->edl;
}

void CWindowTransport::goto_start()
{
	gui->unlock_window();
	handle_transport(REWIND, 1);

	mwindow->gui->lock_window("CWindowTransport::goto_start 1");
	mwindow->goto_start();
	mwindow->gui->unlock_window();

	gui->lock_window("CWindowTransport::goto_start 2");
}

void CWindowTransport::goto_end()
{
	gui->unlock_window();
	handle_transport(GOTO_END, 1);

	mwindow->gui->lock_window("CWindowTransport::goto_end 1");
	mwindow->goto_end();
	mwindow->gui->unlock_window();

	gui->lock_window("CWindowTransport::goto_end 2");
}



CWindowCanvas::CWindowCanvas(MWindow *mwindow, CWindowGUI *gui)
 : Canvas(mwindow,
 	gui,
	mwindow->theme->ccanvas_x,
	mwindow->theme->ccanvas_y,
	mwindow->theme->ccanvas_w,
	mwindow->theme->ccanvas_h,
	0,
	0,
	mwindow->edl->session->cwindow_scrollbars)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

void CWindowCanvas::status_event()
{
	gui->draw_status(1);
}

int CWindowCanvas::get_fullscreen()
{
	return mwindow->session->cwindow_fullscreen;
}

void CWindowCanvas::set_fullscreen(int value)
{
	mwindow->session->cwindow_fullscreen = value;
}


void CWindowCanvas::update_zoom(int x, int y, float zoom)
{
	use_scrollbars = mwindow->edl->session->cwindow_scrollbars;

	mwindow->edl->session->cwindow_xscroll = x;
	mwindow->edl->session->cwindow_yscroll = y;
	mwindow->edl->session->cwindow_zoom = zoom;
}

void CWindowCanvas::zoom_auto()
{
	gui->zoom_canvas(0, 1);
}

int CWindowCanvas::get_xscroll()
{
	return mwindow->edl->session->cwindow_xscroll;
}

int CWindowCanvas::get_yscroll()
{
	return mwindow->edl->session->cwindow_yscroll;
}


float CWindowCanvas::get_zoom()
{
	return mwindow->edl->session->cwindow_zoom;
}

void CWindowCanvas::draw_refresh(int flush)
{
	if(get_canvas() && !get_canvas()->get_video_on())
	{

		if(refresh_frame && refresh_frame->get_w()>0 && refresh_frame->get_h()>0)
		{
			float in_x1, in_y1, in_x2, in_y2;
			float out_x1, out_y1, out_x2, out_y2;
			get_transfers(mwindow->edl,
				in_x1,
				in_y1,
				in_x2,
				in_y2,
				out_x1,
				out_y1,
				out_x2,
				out_y2);

			if(!EQUIV(out_x1, 0) ||
				!EQUIV(out_y1, 0) ||
				!EQUIV(out_x2, get_canvas()->get_w()) ||
				!EQUIV(out_y2, get_canvas()->get_h()))
			{
				get_canvas()->clear_box(0,
					0,
					get_canvas()->get_w(),
					get_canvas()->get_h());
			}

//printf("CWindowCanvas::draw_refresh %.2f %.2f %.2f %.2f -> %.2f %.2f %.2f %.2f\n",
//in_x1, in_y1, in_x2, in_y2, out_x1, out_y1, out_x2, out_y2);


			if(out_x2 > out_x1 &&
				out_y2 > out_y1 &&
				in_x2 > in_x1 &&
				in_y2 > in_y1)
			{
// input scaled from session to refresh frame coordinates
				int ow = get_output_w(mwindow->edl);
				int oh = get_output_h(mwindow->edl);
				int rw = refresh_frame->get_w();
				int rh = refresh_frame->get_h();
				float xs = (float)rw / ow;
				float ys = (float)rh / oh;
				in_x1 *= xs;  in_x2 *= xs;
				in_y1 *= ys;  in_y2 *= ys;
// Can't use OpenGL here because it is called asynchronously of the
// playback operation.
				get_canvas()->draw_vframe(refresh_frame,
						(int)out_x1,
						(int)out_y1,
						(int)(out_x2 - out_x1),
						(int)(out_y2 - out_y1),
						(int)in_x1,
						(int)in_y1,
						(int)(in_x2 - in_x1),
						(int)(in_y2 - in_y1),
						0);
			}
		}
		else
		{
			get_canvas()->clear_box(0,
				0,
				get_canvas()->get_w(),
				get_canvas()->get_h());
		}

		draw_overlays();
// allow last opengl write to complete before redraw
// tried sync_display, glFlush, glxMake*Current(0..)
usleep(20000);
		get_canvas()->flash(flush);
	}
//printf("CWindowCanvas::draw_refresh 10\n");
}

#define CROPHANDLE_W 10
#define CROPHANDLE_H 10

void CWindowCanvas::draw_crophandle(int x, int y)
{
	get_canvas()->draw_box(x, y, CROPHANDLE_W, CROPHANDLE_H);
}






#define CONTROL_W 10
#define CONTROL_H 10
#define FIRST_CONTROL_W 20
#define FIRST_CONTROL_H 20
#undef BC_INFINITY
#define BC_INFINITY 65536

#define RULERHANDLE_W 16
#define RULERHANDLE_H 16



int CWindowCanvas::do_ruler(int draw,
	int motion,
	int button_press,
	int button_release)
{
	int result = 0;
	float x1 = mwindow->edl->session->ruler_x1;
	float y1 = mwindow->edl->session->ruler_y1;
	float x2 = mwindow->edl->session->ruler_x2;
	float y2 = mwindow->edl->session->ruler_y2;
	float canvas_x1 = x1;
	float canvas_y1 = y1;
	float canvas_x2 = x2;
	float canvas_y2 = y2;
	float output_x = get_cursor_x();
	float output_y = get_cursor_y();
	float canvas_cursor_x = output_x;
	float canvas_cursor_y = output_y;

	canvas_to_output(mwindow->edl, 0, output_x, output_y);
	output_to_canvas(mwindow->edl, 0, canvas_x1, canvas_y1);
	output_to_canvas(mwindow->edl, 0, canvas_x2, canvas_y2);
	mwindow->session->cwindow_output_x = roundf(output_x);
	mwindow->session->cwindow_output_y = roundf(output_y);

	if(button_press && get_buttonpress() == 1)
	{
		gui->ruler_handle = -1;
		gui->ruler_translate = 0;
		if(gui->alt_down())
		{
			gui->ruler_translate = 1;
			gui->ruler_origin_x = x1;
			gui->ruler_origin_y = y1;
		}
		else
		if(canvas_cursor_x >= canvas_x1 - RULERHANDLE_W / 2 &&
			canvas_cursor_x < canvas_x1 + RULERHANDLE_W / 2 &&
			canvas_cursor_y >= canvas_y1 - RULERHANDLE_W &&
			canvas_cursor_y < canvas_y1 + RULERHANDLE_H / 2)
		{
			gui->ruler_handle = 0;
			gui->ruler_origin_x = x1;
			gui->ruler_origin_y = y1;
		}
		else
		if(canvas_cursor_x >= canvas_x2 - RULERHANDLE_W / 2 &&
			canvas_cursor_x < canvas_x2 + RULERHANDLE_W / 2 &&
			canvas_cursor_y >= canvas_y2 - RULERHANDLE_W &&
			canvas_cursor_y < canvas_y2 + RULERHANDLE_H / 2)
		{
			gui->ruler_handle = 1;
			gui->ruler_origin_x = x2;
			gui->ruler_origin_y = y2;
		}


// Start new selection
		if(!gui->ruler_translate &&
			(gui->ruler_handle < 0 ||
			(EQUIV(x2, x1) &&
			EQUIV(y2, y1))))
		{
// Hide previous
			do_ruler(1, 0, 0, 0);
			get_canvas()->flash();
			gui->ruler_handle = 1;
			mwindow->edl->session->ruler_x1 = output_x;
			mwindow->edl->session->ruler_y1 = output_y;
			mwindow->edl->session->ruler_x2 = output_x;
			mwindow->edl->session->ruler_y2 = output_y;
			gui->ruler_origin_x = mwindow->edl->session->ruler_x2;
			gui->ruler_origin_y = mwindow->edl->session->ruler_y2;
		}

		gui->x_origin = output_x;
		gui->y_origin = output_y;
		gui->current_operation = CWINDOW_RULER;
		gui->tool_panel->raise_window();
		result = 1;
	}

	if(motion)
	{
		if(gui->current_operation == CWINDOW_RULER)
		{
			if(gui->ruler_translate)
			{
// Hide ruler
				do_ruler(1, 0, 0, 0);
				float x_difference = mwindow->edl->session->ruler_x1;
				float y_difference = mwindow->edl->session->ruler_y1;
				mwindow->edl->session->ruler_x1 = output_x - gui->x_origin + gui->ruler_origin_x;
				mwindow->edl->session->ruler_y1 = output_y - gui->y_origin + gui->ruler_origin_y;
				x_difference -= mwindow->edl->session->ruler_x1;
				y_difference -= mwindow->edl->session->ruler_y1;
				mwindow->edl->session->ruler_x2 -= x_difference;
				mwindow->edl->session->ruler_y2 -= y_difference;
// Show ruler
				do_ruler(1, 0, 0, 0);
				get_canvas()->flash();
				gui->update_tool();
			}
			else
			switch(gui->ruler_handle)
			{
				case 0:
					do_ruler(1, 0, 0, 0);
					mwindow->edl->session->ruler_x1 = output_x - gui->x_origin + gui->ruler_origin_x;
					mwindow->edl->session->ruler_y1 = output_y - gui->y_origin + gui->ruler_origin_y;
					if(gui->alt_down() || gui->ctrl_down())
					{
						double angle_value = fabs(atan((mwindow->edl->session->ruler_y2 - mwindow->edl->session->ruler_y1) /
							(mwindow->edl->session->ruler_x2 - mwindow->edl->session->ruler_x1)) *
							360 /
							2 /
							M_PI);
						double distance_value =
							sqrt(SQR(mwindow->edl->session->ruler_x2 - mwindow->edl->session->ruler_x1) +
							SQR(mwindow->edl->session->ruler_y2 - mwindow->edl->session->ruler_y1));
						if(angle_value < 22)
							mwindow->edl->session->ruler_y1 = mwindow->edl->session->ruler_y2;
						else
						if(angle_value > 67)
							mwindow->edl->session->ruler_x1 = mwindow->edl->session->ruler_x2;
						else
						if(mwindow->edl->session->ruler_x1 < mwindow->edl->session->ruler_x2 &&
							mwindow->edl->session->ruler_y1 < mwindow->edl->session->ruler_y2)
						{
							mwindow->edl->session->ruler_x1 = mwindow->edl->session->ruler_x2 - distance_value / 1.414214;
							mwindow->edl->session->ruler_y1 = mwindow->edl->session->ruler_y2 - distance_value / 1.414214;
						}
						else
						if(mwindow->edl->session->ruler_x1 < mwindow->edl->session->ruler_x2 && mwindow->edl->session->ruler_y1 > mwindow->edl->session->ruler_y2)
						{
							mwindow->edl->session->ruler_x1 = mwindow->edl->session->ruler_x2 - distance_value / 1.414214;
							mwindow->edl->session->ruler_y1 = mwindow->edl->session->ruler_y2 + distance_value / 1.414214;
						}
						else
						if(mwindow->edl->session->ruler_x1 > mwindow->edl->session->ruler_x2 &&
							mwindow->edl->session->ruler_y1 < mwindow->edl->session->ruler_y2)
						{
							mwindow->edl->session->ruler_x1 = mwindow->edl->session->ruler_x2 + distance_value / 1.414214;
							mwindow->edl->session->ruler_y1 = mwindow->edl->session->ruler_y2 - distance_value / 1.414214;
						}
						else
						{
							mwindow->edl->session->ruler_x1 = mwindow->edl->session->ruler_x2 + distance_value / 1.414214;
							mwindow->edl->session->ruler_y1 = mwindow->edl->session->ruler_y2 + distance_value / 1.414214;
						}
					}
					do_ruler(1, 0, 0, 0);
					get_canvas()->flash();
					gui->update_tool();
					break;

				case 1:
					do_ruler(1, 0, 0, 0);
					mwindow->edl->session->ruler_x2 = output_x - gui->x_origin + gui->ruler_origin_x;
					mwindow->edl->session->ruler_y2 = output_y - gui->y_origin + gui->ruler_origin_y;
					if(gui->alt_down() || gui->ctrl_down())
					{
						double angle_value = fabs(atan((mwindow->edl->session->ruler_y2 - mwindow->edl->session->ruler_y1) /
							(mwindow->edl->session->ruler_x2 - mwindow->edl->session->ruler_x1)) *
							360 /
							2 /
							M_PI);
						double distance_value =
							sqrt(SQR(mwindow->edl->session->ruler_x2 - mwindow->edl->session->ruler_x1) +
							SQR(mwindow->edl->session->ruler_y2 - mwindow->edl->session->ruler_y1));
						if(angle_value < 22)
							mwindow->edl->session->ruler_y2 = mwindow->edl->session->ruler_y1;
						else
						if(angle_value > 67)
							mwindow->edl->session->ruler_x2 = mwindow->edl->session->ruler_x1;
						else
						if(mwindow->edl->session->ruler_x2 < mwindow->edl->session->ruler_x1 &&
							mwindow->edl->session->ruler_y2 < mwindow->edl->session->ruler_y1)
						{
							mwindow->edl->session->ruler_x2 = mwindow->edl->session->ruler_x1 - distance_value / 1.414214;
							mwindow->edl->session->ruler_y2 = mwindow->edl->session->ruler_y1 - distance_value / 1.414214;
						}
						else
						if(mwindow->edl->session->ruler_x2 < mwindow->edl->session->ruler_x1 &&
							mwindow->edl->session->ruler_y2 > mwindow->edl->session->ruler_y1)
						{
							mwindow->edl->session->ruler_x2 = mwindow->edl->session->ruler_x1 - distance_value / 1.414214;
							mwindow->edl->session->ruler_y2 = mwindow->edl->session->ruler_y1 + distance_value / 1.414214;
						}
						else
						if(mwindow->edl->session->ruler_x2 > mwindow->edl->session->ruler_x1 && mwindow->edl->session->ruler_y2 < mwindow->edl->session->ruler_y1)
						{
							mwindow->edl->session->ruler_x2 = mwindow->edl->session->ruler_x1 + distance_value / 1.414214;
							mwindow->edl->session->ruler_y2 = mwindow->edl->session->ruler_y1 - distance_value / 1.414214;
						}
						else
						{
							mwindow->edl->session->ruler_x2 = mwindow->edl->session->ruler_x1 + distance_value / 1.414214;
							mwindow->edl->session->ruler_y2 = mwindow->edl->session->ruler_y1 + distance_value / 1.414214;
						}
					}
					do_ruler(1, 0, 0, 0);
					get_canvas()->flash();
					gui->update_tool();
					break;
			}
//printf("CWindowCanvas::do_ruler 2 %f %f %f %f\n", gui->ruler_x1, gui->ruler_y1, gui->ruler_x2, gui->ruler_y2);
		}
		else
		{
// printf("CWindowCanvas::do_ruler 2 %f %f %f %f\n",
// canvas_cursor_x,
// canvas_cursor_y,
// canvas_x1,
// canvas_y1);
			if(canvas_cursor_x >= canvas_x1 - RULERHANDLE_W / 2 &&
				canvas_cursor_x < canvas_x1 + RULERHANDLE_W / 2 &&
				canvas_cursor_y >= canvas_y1 - RULERHANDLE_W &&
				canvas_cursor_y < canvas_y1 + RULERHANDLE_H / 2)
			{
				set_cursor(UPRIGHT_ARROW_CURSOR);
			}
			else
			if(canvas_cursor_x >= canvas_x2 - RULERHANDLE_W / 2 &&
				canvas_cursor_x < canvas_x2 + RULERHANDLE_W / 2 &&
				canvas_cursor_y >= canvas_y2 - RULERHANDLE_W &&
				canvas_cursor_y < canvas_y2 + RULERHANDLE_H / 2)
			{
				set_cursor(UPRIGHT_ARROW_CURSOR);
			}
			else
				set_cursor(CROSS_CURSOR);

// Update current position
			gui->update_tool();
		}
	}

// Assume no ruler measurement if 0 length
	if(draw && (!EQUIV(x2, x1) || !EQUIV(y2, y1)))
	{
		get_canvas()->set_inverse();
		get_canvas()->set_color(WHITE);
		get_canvas()->draw_line((int)canvas_x1,
			(int)canvas_y1,
			(int)canvas_x2,
			(int)canvas_y2);
		get_canvas()->draw_line(roundf(canvas_x1) - RULERHANDLE_W / 2,
			roundf(canvas_y1),
			roundf(canvas_x1) + RULERHANDLE_W / 2,
			roundf(canvas_y1));
		get_canvas()->draw_line(roundf(canvas_x1),
			roundf(canvas_y1) - RULERHANDLE_H / 2,
			roundf(canvas_x1),
			roundf(canvas_y1) + RULERHANDLE_H / 2);
		get_canvas()->draw_line(roundf(canvas_x2) - RULERHANDLE_W / 2,
			roundf(canvas_y2),
			roundf(canvas_x2) + RULERHANDLE_W / 2,
			roundf(canvas_y2));
		get_canvas()->draw_line(roundf(canvas_x2),
			roundf(canvas_y2) - RULERHANDLE_H / 2,
			roundf(canvas_x2),
			roundf(canvas_y2) + RULERHANDLE_H / 2);
		get_canvas()->set_opaque();
	}

	return result;
}


static inline double line_dist(float cx,float cy, float tx,float ty)
{
	double dx = tx-cx, dy = ty-cy;
	return sqrt(dx*dx + dy*dy);
}

static inline bool test_bbox(int cx, int cy, int tx, int ty)
{
//	printf("test_bbox %d,%d - %d,%d = %f\n",cx,cy,tx,ty,line_dist(cx,cy,tx,ty));
	return (llabs(cx-tx) < CONTROL_W/2 && llabs(cy-ty) < CONTROL_H/2);
}


int CWindowCanvas::do_mask(int &redraw, int &rerender,
		int button_press, int cursor_motion, int draw)
{
// Retrieve points from top recordable track
//printf("CWindowCanvas::do_mask 1\n");
	Track *track = gui->cwindow->calculate_affected_track();
//printf("CWindowCanvas::do_mask 2\n");

	if(!track) return 0;
//printf("CWindowCanvas::do_mask 3\n");

	MaskAutos *mask_autos = (MaskAutos*)track->automation->autos[AUTOMATION_MASK];
	int64_t position = track->to_units(
		mwindow->edl->local_session->get_selectionstart(1),
		0);
	ArrayList<MaskPoint*> points;

// Determine the points based on whether
// new keyframes will be generated or drawing is performed.
// If keyframe generation occurs, use the interpolated mask.
// If no keyframe generation occurs, use the previous mask.
	int use_interpolated = 0;
	if(button_press || cursor_motion) {
#ifdef USE_KEYFRAME_SPANNING
		double selection_start = mwindow->edl->local_session->get_selectionstart(0);
		double selection_end = mwindow->edl->local_session->get_selectionend(0);

		Auto *first = 0;
		mask_autos->get_prev_auto(track->to_units(selection_start, 0),
			PLAY_FORWARD, first, 1);
		Auto *last = 0;
		mask_autos->get_prev_auto(track->to_units(selection_end, 0),
			PLAY_FORWARD, last, 1);

		if(last == first && (!mwindow->edl->session->auto_keyframes))
			use_interpolated = 0;
		else
// If keyframe spanning occurs, use the interpolated points.
// If new keyframe is generated, use the interpolated points.
			use_interpolated = 1;

#else
		if(mwindow->edl->session->auto_keyframes)
			use_interpolated = 1;
#endif
	}
	else
		use_interpolated = 1;

	if(use_interpolated) {
// Interpolate the points to get exactly what is being rendered at this position.
		mask_autos->get_points(&points,
			mwindow->edl->session->cwindow_mask,
			position,
			PLAY_FORWARD);
	}
	else {
// Use the prev mask
		Auto *prev = 0;
		mask_autos->get_prev_auto(position,
			PLAY_FORWARD,
			prev,
			1);
		((MaskAuto*)prev)->get_points(&points,
			mwindow->edl->session->cwindow_mask);
	}

// Projector zooms relative to the center of the track output.
	float half_track_w = (float)track->track_w / 2;
	float half_track_h = (float)track->track_h / 2;
// Translate mask to projection
	float projector_x, projector_y, projector_z;
	track->automation->get_projector(
		&projector_x, &projector_y, &projector_z,
		position, PLAY_FORWARD);


// Get position of cursor relative to mask
	float cursor_x = get_cursor_x(), cursor_y = get_cursor_y();
	float mask_cursor_x = cursor_x, mask_cursor_y = cursor_y;
	canvas_to_output(mwindow->edl, 0, mask_cursor_x, mask_cursor_y);

	projector_x += mwindow->edl->session->output_w / 2;
	projector_y += mwindow->edl->session->output_h / 2;
	mask_cursor_x = (mask_cursor_x - projector_x) / projector_z + half_track_w;
	mask_cursor_y = (mask_cursor_y - projector_y) / projector_z + half_track_h;

// Fix cursor origin
	if(button_press) {
		gui->x_origin = mask_cursor_x;
		gui->y_origin = mask_cursor_y;
	}

	int result = 0;
// Points of closest line
	int shortest_point1 = -1;
	int shortest_point2 = -1;
// Closest point
	int shortest_point = -1;
// Distance to closest line
	float shortest_line_distance = BC_INFINITY;
// Distance to closest point
	float shortest_point_distance = BC_INFINITY;
	int selected_point = -1;
	int selected_control_point = -1;
	float selected_control_point_distance = BC_INFINITY;
	ArrayList<int> x_points;
	ArrayList<int> y_points;

	if(!cursor_motion) {
		if(draw) {
			get_canvas()->set_color(WHITE);
			get_canvas()->set_inverse();
		}
//printf("CWindowCanvas::do_mask 1 %d\n", points.size());

// Never draw closed polygon and a closed
// polygon is harder to add points to.
		for(int i = 0; i < points.size() && !result; i++) {
			MaskPoint *point1 = points.get(i);
			MaskPoint *point2 = (i >= points.size() - 1) ?
				points.get(0) : points.get(i + 1);
			if(button_press) {
				float point_distance1 = line_dist(point1->x,point1->y, mask_cursor_x,mask_cursor_y);
				if(point_distance1 < shortest_point_distance || shortest_point < 0) {
					shortest_point_distance = point_distance1;
					shortest_point = i;
				}

				float point_distance2 = line_dist(point2->x,point2->y, mask_cursor_x,mask_cursor_y);
				if(point_distance2 < shortest_point_distance || shortest_point < 0) {
					shortest_point_distance = point_distance2;
					shortest_point = (i >= points.size() - 1) ? 0 : (i + 1);
				}
			}

			int segments = 1 + line_dist(point1->x,point1->y, point2->x,point2->y);

//printf("CWindowCanvas::do_mask 1 %f, %f -> %f, %f projectorz=%f\n",
//point1->x, point1->y, point2->x, point2->y, projector_z);
			for(int j = 0; j <= segments && !result; j++) {
//printf("CWindowCanvas::do_mask 1 %f, %f -> %f, %f\n", x0, y0, x3, y3);
				float x0 = point1->x, y0 = point1->y;
				float x1 = point1->x + point1->control_x2;
				float y1 = point1->y + point1->control_y2;
				float x2 = point2->x + point2->control_x1;
				float y2 = point2->y + point2->control_y1;
				float x3 = point2->x, y3 = point2->y;
				float canvas_x0 = (x0 - half_track_w) * projector_z + projector_x;
				float canvas_y0 = (y0 - half_track_h) * projector_z + projector_y;
				float canvas_x1 = (x1 - half_track_w) * projector_z + projector_x;
				float canvas_y1 = (y1 - half_track_h) * projector_z + projector_y;
				float canvas_x2 = (x2 - half_track_w) * projector_z + projector_x;
				float canvas_y2 = (y2 - half_track_h) * projector_z + projector_y;
				float canvas_x3 = (x3 - half_track_w) * projector_z + projector_x;
				float canvas_y3 = (y3 - half_track_h) * projector_z + projector_y;

				float t = (float)j / segments;
				float tpow2 = t * t;
				float tpow3 = t * t * t;
				float invt = 1 - t;
				float invtpow2 = invt * invt;
				float invtpow3 = invt * invt * invt;

				float x = (           invtpow3 * x0
					+ 3 * t     * invtpow2 * x1
					+ 3 * tpow2 * invt     * x2
					+     tpow3            * x3);
				float y = (           invtpow3 * y0
					+ 3 * t     * invtpow2 * y1
					+ 3 * tpow2 * invt     * y2
					+     tpow3            * y3);
				float canvas_x = (x - half_track_w) * projector_z + projector_x;
				float canvas_y = (y - half_track_h) * projector_z + projector_y;
// Test new point addition
				if(button_press) {
					float line_distance = line_dist(x,y, mask_cursor_x,mask_cursor_y);

//printf("CWindowCanvas::do_mask 1 x=%f cursor_x=%f y=%f cursor_y=%f %f %f %d, %d\n",
//  x, cursor_x, y, cursor_y, line_distance, shortest_line_distance, shortest_point1, shortest_point2);
					if(line_distance < shortest_line_distance ||
						shortest_point1 < 0) {
						shortest_line_distance = line_distance;
						shortest_point1 = i;
						shortest_point2 = (i >= points.size() - 1) ? 0 : (i + 1);
//printf("CWindowCanvas::do_mask 2 %f %f %d, %d\n",
//  line_distance, shortest_line_distance, shortest_point1, shortest_point2);
					}

// Test existing point selection
// Test first point
					if(gui->ctrl_down()) {
						float distance = line_dist(x1,y1, mask_cursor_x,mask_cursor_y);

						if(distance < selected_control_point_distance) {
							selected_point = i;
							selected_control_point = 1;
							selected_control_point_distance = distance;
						}
					}
					else {
						if(!gui->shift_down()) {
							output_to_canvas(mwindow->edl, 0, canvas_x0, canvas_y0);
							if(test_bbox(cursor_x, cursor_y, canvas_x0, canvas_y0)) {
								selected_point = i;
							}
						}
						else {
							selected_point = shortest_point;
						}
					}
// Test second point
					if(gui->ctrl_down()) {
						float distance = line_dist(x2,y2, mask_cursor_x,mask_cursor_y);

//printf("CWindowCanvas::do_mask %d %f %f\n", i, distance, selected_control_point_distance);
						if(distance < selected_control_point_distance) {
							selected_point = (i < points.size() - 1 ? i + 1 : 0);
							selected_control_point = 0;
							selected_control_point_distance = distance;
						}
					}
					else if(i < points.size() - 1) {
						if(!gui->shift_down()) {
							output_to_canvas(mwindow->edl, 0, canvas_x3, canvas_y3);
							if(test_bbox(cursor_x, cursor_y, canvas_x3, canvas_y3)) {
								selected_point = (i < points.size() - 1 ? i + 1 : 0);
							}
						}
						else {
							selected_point = shortest_point;
						}
					}
				}

				output_to_canvas(mwindow->edl, 0, canvas_x, canvas_y);

				if(j > 0) {

					if(draw) { // Draw joining line
						x_points.append((int)canvas_x);
						y_points.append((int)canvas_y);
					}

					if(j == segments) {
						if(draw) { // Draw second anchor
							if(i < points.size() - 1) {
								if(i == gui->affected_point - 1)
									get_canvas()->draw_disc(
										(int)canvas_x - CONTROL_W / 2,
										(int)canvas_y - CONTROL_W / 2,
										CONTROL_W, CONTROL_H);
								else
									get_canvas()->draw_circle(
										(int)canvas_x - CONTROL_W / 2,
										(int)canvas_y - CONTROL_W / 2,
										CONTROL_W, CONTROL_H);
// char string[BCTEXTLEN];
// sprintf(string, "%d", (i < points.size() - 1 ? i + 1 : 0));
// canvas->draw_text((int)canvas_x + CONTROL_W, (int)canvas_y + CONTROL_W, string);
							}
// Draw second control point.
							output_to_canvas(mwindow->edl, 0, canvas_x2, canvas_y2);
							get_canvas()->draw_line(
								(int)canvas_x, (int)canvas_y,
								(int)canvas_x2, (int)canvas_y2);
							get_canvas()->draw_rectangle(
								(int)canvas_x2 - CONTROL_W / 2,
								(int)canvas_y2 - CONTROL_H / 2,
								CONTROL_W, CONTROL_H);
						}
					}
				}
				else {
// Draw first anchor
					if(i == 0 && draw) {
						char mask_label[BCSTRLEN];
						sprintf(mask_label, "%d",
							mwindow->edl->session->cwindow_mask);
						get_canvas()->draw_text(
							(int)canvas_x - FIRST_CONTROL_W,
							(int)canvas_y - FIRST_CONTROL_H,
							mask_label);

						get_canvas()->draw_disc(
							(int)canvas_x - FIRST_CONTROL_W / 2,
							(int)canvas_y - FIRST_CONTROL_H / 2,
							FIRST_CONTROL_W, FIRST_CONTROL_H);
					}

// Draw first control point.
					if(draw) {
						output_to_canvas(mwindow->edl, 0, canvas_x1, canvas_y1);
						get_canvas()->draw_line(
							(int)canvas_x, (int)canvas_y,
							(int)canvas_x1, (int)canvas_y1);
						get_canvas()->draw_rectangle(
							(int)canvas_x1 - CONTROL_W / 2,
							(int)canvas_y1 - CONTROL_H / 2,
							CONTROL_W, CONTROL_H);

						x_points.append((int)canvas_x);
						y_points.append((int)canvas_y);
					}
				}
//printf("CWindowCanvas::do_mask 1\n");

			}
		}
//printf("CWindowCanvas::do_mask 1\n");

		if(draw) {
			get_canvas()->draw_polygon(&x_points, &y_points);
			get_canvas()->set_opaque();
		}
//printf("CWindowCanvas::do_mask 1\n");
	}

	if(button_press && !result) {
		gui->affected_track = gui->cwindow->calculate_affected_track();

// Get keyframe outside the EDL to edit.  This must be rendered
// instead of the EDL keyframes when it exists.  Then it must be
// applied to the EDL keyframes on buttonrelease.
		if(gui->affected_track) {
#ifdef USE_KEYFRAME_SPANNING
// Make copy of current parameters in local keyframe
			gui->mask_keyframe =
				(MaskAuto*)gui->cwindow->calculate_affected_auto(
					mask_autos,
					0);
			gui->orig_mask_keyframe->copy_data(gui->mask_keyframe);
#else

			gui->mask_keyframe =
				(MaskAuto*)gui->cwindow->calculate_affected_auto(
					mask_autos,
					1);
#endif
		}
		SubMask *mask = gui->mask_keyframe->get_submask(mwindow->edl->session->cwindow_mask);


// Translate entire keyframe
		if(gui->alt_down() && mask->points.size()) {
			mwindow->undo->update_undo_before(_("mask translate"), 0);
			gui->current_operation = CWINDOW_MASK_TRANSLATE;
			gui->affected_point = 0;
		}
		else
// Existing point or control point was selected
		if(selected_point >= 0) {
			mwindow->undo->update_undo_before(_("mask adjust"), 0);
			gui->affected_point = selected_point;

			if(selected_control_point == 0)
				gui->current_operation = CWINDOW_MASK_CONTROL_IN;
			else
			if(selected_control_point == 1)
				gui->current_operation = CWINDOW_MASK_CONTROL_OUT;
			else
				gui->current_operation = mwindow->edl->session->cwindow_operation;
		}
		else // No existing point or control point was selected so create a new one
		if(!gui->ctrl_down() && !gui->alt_down()) {
			mwindow->undo->update_undo_before(_("mask point"), 0);
// Create the template
			MaskPoint *point = new MaskPoint;
			point->x = mask_cursor_x;
			point->y = mask_cursor_y;
			point->control_x1 = 0;
			point->control_y1 = 0;
			point->control_x2 = 0;
			point->control_y2 = 0;


			if(shortest_point2 < shortest_point1) {
				shortest_point2 ^= shortest_point1;
				shortest_point1 ^= shortest_point2;
				shortest_point2 ^= shortest_point1;
			}



// printf("CWindowGUI::do_mask 40\n");
// mwindow->edl->dump();
// printf("CWindowGUI::do_mask 50\n");



//printf("CWindowCanvas::do_mask 1 %f %f %d %d\n",
//	shortest_line_distance, shortest_point_distance, shortest_point1, shortest_point2);
//printf("CWindowCanvas::do_mask %d %d\n", shortest_point1, shortest_point2);

// Append to end of list
			if( shortest_point1 == shortest_point2 ||
			    labs(shortest_point1 - shortest_point2) > 1) {
#ifdef USE_KEYFRAME_SPANNING

				MaskPoint *new_point = new MaskPoint;
				points.append(new_point);
				*new_point = *point;
				gui->affected_point = points.size() - 1;

#else

// Need to apply the new point to every keyframe
				for(MaskAuto *current = (MaskAuto*)mask_autos->default_auto; current; ) {
					SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
					MaskPoint *new_point = new MaskPoint;
					submask->points.append(new_point);
					*new_point = *point;
					if(current == (MaskAuto*)mask_autos->default_auto)
						current = (MaskAuto*)mask_autos->first;
					else
						current = (MaskAuto*)NEXT;
				}
				gui->affected_point = mask->points.size() - 1;
#endif

				result = 1;
			}
			else
// Insert between 2 points, shifting back point 2
			if(shortest_point1 >= 0 && shortest_point2 >= 0) {

#ifdef USE_KEYFRAME_SPANNING
// In case the keyframe point count isn't synchronized with the rest of the keyframes,
// avoid a crash.
				if(points.size() >= shortest_point2) {
					MaskPoint *new_point = new MaskPoint;
					points.append(0);
					for(int i = points.size() - 1;
						i > shortest_point2;
						i--)
						points.values[i] = points.values[i - 1];
					points.values[shortest_point2] = new_point;

					*new_point = *point;
				}

#else

				for(MaskAuto *current = (MaskAuto*)mask_autos->default_auto; current; ) {
					SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
// In case the keyframe point count isn't synchronized with the rest of the keyframes,
// avoid a crash.
					if(submask->points.size() >= shortest_point2) {
						MaskPoint *new_point = new MaskPoint;
						submask->points.append(0);
						for(int i = submask->points.size() - 1;
							i > shortest_point2;
							i--)
							submask->points.values[i] = submask->points.values[i - 1];
						submask->points.values[shortest_point2] = new_point;

						*new_point = *point;
					}

					if(current == (MaskAuto*)mask_autos->default_auto)
						current = (MaskAuto*)mask_autos->first;
					else
						current = (MaskAuto*)NEXT;
				}

#endif
				gui->affected_point = shortest_point2;
				result = 1;
			}


// printf("CWindowGUI::do_mask 20\n");
// mwindow->edl->dump();
// printf("CWindowGUI::do_mask 30\n");

			if(!result) {
//printf("CWindowCanvas::do_mask 1\n");
// Create the first point.
#ifdef USE_KEYFRAME_SPANNING
				MaskPoint *new_point = new MaskPoint;
				points.append(new_point);
				*new_point = *point;
				gui->affected_point = points.size() - 1;
#else
				for(MaskAuto *current = (MaskAuto*)mask_autos->default_auto; current; ) {
					SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
					MaskPoint *new_point = new MaskPoint;
					submask->points.append(new_point);
					*new_point = *point;
					if(current == (MaskAuto*)mask_autos->default_auto)
						current = (MaskAuto*)mask_autos->first;
					else
						current = (MaskAuto*)NEXT;
				}
				gui->affected_point = points.size() - 1;
#endif

//printf("CWindowCanvas::do_mask 3 %d\n", mask->points.size());
			}

			gui->current_operation = mwindow->edl->session->cwindow_operation;
// Delete the template
			delete point;
		}

		result = 1;
		rerender = 1;
		redraw = 1;
	}

	if(button_press && result) {
#ifdef USE_KEYFRAME_SPANNING
		MaskPoint *point = points.values[gui->affected_point];
		gui->center_x = point->x;
		gui->center_y = point->y;
		gui->control_in_x = point->control_x1;
		gui->control_in_y = point->control_y1;
		gui->control_out_x = point->control_x2;
		gui->control_out_y = point->control_y2;
		gui->tool_panel->raise_window();
#else
		SubMask *mask = gui->mask_keyframe->get_submask(mwindow->edl->session->cwindow_mask);
		MaskPoint *point = mask->points.values[gui->affected_point];
		gui->center_x = point->x;
		gui->center_y = point->y;
		gui->control_in_x = point->control_x1;
		gui->control_in_y = point->control_y1;
		gui->control_out_x = point->control_x2;
		gui->control_out_y = point->control_y2;
		gui->tool_panel->raise_window();
#endif
	}

//printf("CWindowCanvas::do_mask 8\n");
	if(cursor_motion) {

#ifdef USE_KEYFRAME_SPANNING
// Must update the reference keyframes for every cursor motion
		gui->mask_keyframe =
			(MaskAuto*)gui->cwindow->calculate_affected_auto(
				mask_autos,
				0);
		gui->orig_mask_keyframe->copy_data(gui->mask_keyframe);
#endif

//printf("CWindowCanvas::do_mask %d %d\n", __LINE__, gui->affected_point);

		SubMask *mask = gui->mask_keyframe->get_submask(mwindow->edl->session->cwindow_mask);
		if( gui->affected_point >= 0 && gui->affected_point < mask->points.size() &&
			gui->current_operation != CWINDOW_NONE) {
//			mwindow->undo->update_undo_before(_("mask point"), this);
#ifdef USE_KEYFRAME_SPANNING
			MaskPoint *point = points.get(gui->affected_point);
#else
			MaskPoint *point = mask->points.get(gui->affected_point);
#endif
// 			canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);
//printf("CWindowCanvas::do_mask 9 %d %d\n", mask->points.size(), gui->affected_point);

			float last_x = point->x;
			float last_y = point->y;
			float last_control_x1 = point->control_x1;
			float last_control_y1 = point->control_y1;
			float last_control_x2 = point->control_x2;
			float last_control_y2 = point->control_y2;

			switch(gui->current_operation) {
				case CWINDOW_MASK:
//printf("CWindowCanvas::do_mask %d %d\n", __LINE__, gui->affected_point);
					point->x = mask_cursor_x - gui->x_origin + gui->center_x;
					point->y = mask_cursor_y - gui->y_origin + gui->center_y;
					break;

				case CWINDOW_MASK_CONTROL_IN:
					point->control_x1 = mask_cursor_x - gui->x_origin + gui->control_in_x;
					point->control_y1 = mask_cursor_y - gui->y_origin + gui->control_in_y;
					break;

				case CWINDOW_MASK_CONTROL_OUT:
					point->control_x2 = mask_cursor_x - gui->x_origin + gui->control_out_x;
					point->control_y2 = mask_cursor_y - gui->y_origin + gui->control_out_y;
					break;

				case CWINDOW_MASK_TRANSLATE:
#ifdef USE_KEYFRAME_SPANNING
					for(int i = 0; i < points.size(); i++) {
						points.values[i]->x += mask_cursor_x - gui->x_origin;
						points.values[i]->y += mask_cursor_y - gui->y_origin;
					}
#else
					for(int i = 0; i < mask->points.size(); i++) {
						mask->points.values[i]->x += mask_cursor_x - gui->x_origin;
						mask->points.values[i]->y += mask_cursor_y - gui->y_origin;
					}
#endif
					gui->x_origin = mask_cursor_x;
					gui->y_origin = mask_cursor_y;
					break;
			}

			if( !EQUIV(last_x, point->x) ||
				!EQUIV(last_y, point->y) ||
				!EQUIV(last_control_x1, point->control_x1) ||
				!EQUIV(last_control_y1, point->control_y1) ||
				!EQUIV(last_control_x2, point->control_x2) ||
				!EQUIV(last_control_y2, point->control_y2)) {
				rerender = 1;
				redraw = 1;
			}
		}
		else
		if(gui->current_operation == CWINDOW_NONE) {
//			printf("CWindowCanvas::do_mask %d\n", __LINE__);
			int over_point = 0;
			for(int i = 0; i < points.size() && !over_point; i++) {
				MaskPoint *point = points.get(i);
				float x0 = point->x;
				float y0 = point->y;
				float x1 = point->x + point->control_x1;
				float y1 = point->y + point->control_y1;
				float x2 = point->x + point->control_x2;
				float y2 = point->y + point->control_y2;
				float canvas_x0 = (x0 - half_track_w) * projector_z + projector_x;
				float canvas_y0 = (y0 - half_track_h) * projector_z + projector_y;

				output_to_canvas(mwindow->edl, 0, canvas_x0, canvas_y0);
				if(test_bbox(cursor_x, cursor_y, canvas_x0, canvas_y0)) {
					over_point = 1;
				}

				if(!over_point && gui->ctrl_down()) {
					float canvas_x1 = (x1 - half_track_w) * projector_z + projector_x;
					float canvas_y1 = (y1 - half_track_h) * projector_z + projector_y;
					output_to_canvas(mwindow->edl, 0, canvas_x1, canvas_y1);
					if(test_bbox(cursor_x, cursor_y, canvas_x1, canvas_y1)) {
						over_point = 1;
					}
					else {
						float canvas_x2 = (x2 - half_track_w) * projector_z + projector_x;
						float canvas_y2 = (y2 - half_track_h) * projector_z + projector_y;
						output_to_canvas(mwindow->edl, 0, canvas_x2, canvas_y2);
						if(test_bbox(cursor_x, cursor_y, canvas_x2, canvas_y2)) {
							over_point = 1;
						}
					}
				}
			}

			set_cursor( over_point ? ARROW_CURSOR : CROSS_CURSOR );
		}

		result = 1;
	}
//printf("CWindowCanvas::do_mask 2 %d %d %d\n", result, rerender, redraw);


#ifdef USE_KEYFRAME_SPANNING
// Must commit change after operation.
	if(rerender && track) {
// Swap EDL keyframe with original.
// Apply new values to keyframe span
		MaskAuto temp_keyframe(mwindow->edl, mask_autos);
		temp_keyframe.copy_data(gui->mask_keyframe);
// Apply interpolated points back to keyframe
		temp_keyframe.set_points(&points, mwindow->edl->session->cwindow_mask);
		gui->mask_keyframe->copy_data(gui->orig_mask_keyframe);
		mask_autos->update_parameter(&temp_keyframe);
	}
#endif

	points.remove_all_objects();
//printf("CWindowCanvas::do_mask 20\n");
	return result;
}

int CWindowCanvas::do_eyedrop(int &rerender, int button_press, int draw)
{
	int result = 0;
	int radius = mwindow->edl->session->eyedrop_radius;
	int row1 = 0;
	int row2 = 0;
	int column1 = 0;
	int column2 = 0;



	if(refresh_frame && refresh_frame->get_w()>0 && refresh_frame->get_h()>0)
	{

		if(draw)
		{
			row1 = gui->eyedrop_y - radius;
			row2 = gui->eyedrop_y + radius;
			column1 = gui->eyedrop_x - radius;
			column2 = gui->eyedrop_x + radius;

			CLAMP(row1, 0, refresh_frame->get_h() - 1);
			CLAMP(row2, 0, refresh_frame->get_h() - 1);
			CLAMP(column1, 0, refresh_frame->get_w() - 1);
			CLAMP(column2, 0, refresh_frame->get_w() - 1);

			if(row2 <= row1) row2 = row1 + 1;
			if(column2 <= column1) column2 = column1 + 1;

			float x1 = column1;
			float y1 = row1;
			float x2 = column2;
			float y2 = row2;

			output_to_canvas(mwindow->edl, 0, x1, y1);
			output_to_canvas(mwindow->edl, 0, x2, y2);
//printf("CWindowCanvas::do_eyedrop %d %f %f %f %f\n", __LINE__, x1, x2, y1, y2);

			if(x2 - x1 >= 1 && y2 - y1 >= 1)
			{
				get_canvas()->set_inverse();
				get_canvas()->set_color(WHITE);

				get_canvas()->draw_rectangle((int)x1,
					(int)y1,
					(int)(x2 - x1),
					(int)(y2 - y1));

				get_canvas()->set_opaque();
				get_canvas()->flash();
			}
			return 0;
		}
	}

	if(button_press)
	{
		gui->current_operation = CWINDOW_EYEDROP;
		gui->tool_panel->raise_window();
	}

	if(gui->current_operation == CWINDOW_EYEDROP)
	{
		mwindow->undo->update_undo_before(_("Eyedrop"), this);

// Get color out of frame.
// Doesn't work during playback because that bypasses the refresh frame.
		if(refresh_frame && refresh_frame->get_w()>0 && refresh_frame->get_h()>0)
		{
			float cursor_x = get_cursor_x();
			float cursor_y = get_cursor_y();
			canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);
			CLAMP(cursor_x, 0, refresh_frame->get_w() - 1);
			CLAMP(cursor_y, 0, refresh_frame->get_h() - 1);

			row1 = cursor_y - radius;
			row2 = cursor_y + radius;
			column1 = cursor_x - radius;
			column2 = cursor_x + radius;
			CLAMP(row1, 0, refresh_frame->get_h() - 1);
			CLAMP(row2, 0, refresh_frame->get_h() - 1);
			CLAMP(column1, 0, refresh_frame->get_w() - 1);
			CLAMP(column2, 0, refresh_frame->get_w() - 1);
			if(row2 <= row1) row2 = row1 + 1;
			if(column2 <= column1) column2 = column1 + 1;


// hide it
			if(gui->eyedrop_visible)
			{
				int temp;
				do_eyedrop(temp, 0, 1);
				gui->eyedrop_visible = 0;
			}

			gui->eyedrop_x = cursor_x;
			gui->eyedrop_y = cursor_y;

// show it
			{
				int temp;
				do_eyedrop(temp, 0, 1);
				gui->eyedrop_visible = 1;
			}

// Decompression coefficients straight out of jpeglib
#define V_TO_R    1.40200
#define V_TO_G    -0.71414

#define U_TO_G    -0.34414
#define U_TO_B    1.77200

#define GET_COLOR(type, components, max, do_yuv) \
{ \
	type *row = (type*)(refresh_frame->get_rows()[i]) + \
		j * components; \
	float red = (float)*row++ / max; \
	float green = (float)*row++ / max; \
	float blue = (float)*row++ / max; \
	if(do_yuv) \
	{ \
		float r = red + V_TO_R * (blue - 0.5); \
		float g = red + U_TO_G * (green - 0.5) + V_TO_G * (blue - 0.5); \
		float b = red + U_TO_B * (green - 0.5); \
		mwindow->edl->local_session->red += r; \
		mwindow->edl->local_session->green += g; \
		mwindow->edl->local_session->blue += b; \
		if(r > mwindow->edl->local_session->red_max) mwindow->edl->local_session->red_max = r; \
		if(g > mwindow->edl->local_session->green_max) mwindow->edl->local_session->green_max = g; \
		if(b > mwindow->edl->local_session->blue_max) mwindow->edl->local_session->blue_max = b; \
	} \
	else \
	{ \
		mwindow->edl->local_session->red += red; \
		mwindow->edl->local_session->green += green; \
		mwindow->edl->local_session->blue += blue; \
		if(red > mwindow->edl->local_session->red_max) mwindow->edl->local_session->red_max = red; \
		if(green > mwindow->edl->local_session->green_max) mwindow->edl->local_session->green_max = green; \
		if(blue > mwindow->edl->local_session->blue_max) mwindow->edl->local_session->blue_max = blue; \
	} \
}



			mwindow->edl->local_session->red = 0;
			mwindow->edl->local_session->green = 0;
			mwindow->edl->local_session->blue = 0;
			mwindow->edl->local_session->red_max = 0;
			mwindow->edl->local_session->green_max = 0;
			mwindow->edl->local_session->blue_max = 0;
			for(int i = row1; i < row2; i++)
			{
				for(int j = column1; j < column2; j++)
				{
					switch(refresh_frame->get_color_model())
					{
						case BC_YUV888:
							GET_COLOR(unsigned char, 3, 0xff, 1);
							break;
						case BC_YUVA8888:
							GET_COLOR(unsigned char, 4, 0xff, 1);
							break;
						case BC_YUV161616:
							GET_COLOR(uint16_t, 3, 0xffff, 1);
							break;
						case BC_YUVA16161616:
							GET_COLOR(uint16_t, 4, 0xffff, 1);
							break;
						case BC_RGB888:
							GET_COLOR(unsigned char, 3, 0xff, 0);
							break;
						case BC_RGBA8888:
							GET_COLOR(unsigned char, 4, 0xff, 0);
							break;
						case BC_RGB_FLOAT:
							GET_COLOR(float, 3, 1.0, 0);
							break;
						case BC_RGBA_FLOAT:
							GET_COLOR(float, 4, 1.0, 0);
							break;
					}
				}
			}

			mwindow->edl->local_session->red /= (row2 - row1) * (column2 - column1);
			mwindow->edl->local_session->green /= (row2 - row1) * (column2 - column1);
			mwindow->edl->local_session->blue /= (row2 - row1) * (column2 - column1);

		}
		else
		{
			mwindow->edl->local_session->red = 0;
			mwindow->edl->local_session->green = 0;
			mwindow->edl->local_session->blue = 0;
			gui->eyedrop_visible = 0;
		}


		gui->update_tool();



		result = 1;
// Can't rerender since the color value is from the output of any effect it
// goes into.
//		rerender = 1;
		mwindow->undo->update_undo_after(_("Eyedrop"), LOAD_SESSION);
	}

	return result;
}

int CWindowCanvas::need_overlays()
{
	if( mwindow->edl->session->safe_regions ) return 1;
	if( mwindow->edl->session->cwindow_scrollbars ) return 1;
	if( gui->highlighted ) return 1;
	switch( mwindow->edl->session->cwindow_operation ) {
		case CWINDOW_EYEDROP:
			if( ! gui->eyedrop_visible ) break;
		case CWINDOW_CAMERA:
		case CWINDOW_PROJECTOR:
		case CWINDOW_CROP:
		case CWINDOW_MASK:
		case CWINDOW_RULER:
			return 1;
	}
	return 0;
}

void CWindowCanvas::draw_overlays()
{
	if(mwindow->edl->session->safe_regions)
	{
		draw_safe_regions();
	}

	if(mwindow->edl->session->cwindow_scrollbars)
	{
// Always draw output rectangle
		float x1, y1, x2, y2;
		x1 = 0;
		x2 = mwindow->edl->session->output_w;
		y1 = 0;
		y2 = mwindow->edl->session->output_h;
		output_to_canvas(mwindow->edl, 0, x1, y1);
		output_to_canvas(mwindow->edl, 0, x2, y2);

		get_canvas()->set_inverse();
		get_canvas()->set_color(WHITE);

		get_canvas()->draw_rectangle((int)x1,
				(int)y1,
				(int)(x2 - x1),
				(int)(y2 - y1));

		get_canvas()->set_opaque();
	}

	if(gui->highlighted)
	{
		get_canvas()->set_color(WHITE);
		get_canvas()->set_inverse();
		get_canvas()->draw_rectangle(0, 0, get_canvas()->get_w(), get_canvas()->get_h());
		get_canvas()->draw_rectangle(1, 1, get_canvas()->get_w() - 2, get_canvas()->get_h() - 2);
		get_canvas()->set_opaque();
	}

	int temp1 = 0, temp2 = 0;
//printf("CWindowCanvas::draw_overlays 1 %d\n", mwindow->edl->session->cwindow_operation);
	switch(mwindow->edl->session->cwindow_operation)
	{
		case CWINDOW_CAMERA:
			draw_bezier(1);
			break;

		case CWINDOW_PROJECTOR:
			draw_bezier(0);
			break;

		case CWINDOW_CROP:
			draw_crop();
			break;

		case CWINDOW_MASK:
			do_mask(temp1, temp2, 0, 0, 1);
			break;

		case CWINDOW_RULER:
			do_ruler(1, 0, 0, 0);
			break;

		case CWINDOW_EYEDROP:
		if(gui->eyedrop_visible)
		{
			int rerender;
			do_eyedrop(rerender, 0, 1);
			gui->eyedrop_visible = 1;
			break;
		}
	}
}

void CWindowCanvas::draw_safe_regions()
{
	float action_x1, action_x2, action_y1, action_y2;
	float title_x1, title_x2, title_y1, title_y2;

	action_x1 = mwindow->edl->session->output_w / 2 - mwindow->edl->session->output_w / 2 * 0.9;
	action_x2 = mwindow->edl->session->output_w / 2 + mwindow->edl->session->output_w / 2 * 0.9;
	action_y1 = mwindow->edl->session->output_h / 2 - mwindow->edl->session->output_h / 2 * 0.9;
	action_y2 = mwindow->edl->session->output_h / 2 + mwindow->edl->session->output_h / 2 * 0.9;
	title_x1 = mwindow->edl->session->output_w / 2 - mwindow->edl->session->output_w / 2 * 0.8;
	title_x2 = mwindow->edl->session->output_w / 2 + mwindow->edl->session->output_w / 2 * 0.8;
	title_y1 = mwindow->edl->session->output_h / 2 - mwindow->edl->session->output_h / 2 * 0.8;
	title_y2 = mwindow->edl->session->output_h / 2 + mwindow->edl->session->output_h / 2 * 0.8;

	output_to_canvas(mwindow->edl, 0, action_x1, action_y1);
	output_to_canvas(mwindow->edl, 0, action_x2, action_y2);
	output_to_canvas(mwindow->edl, 0, title_x1, title_y1);
	output_to_canvas(mwindow->edl, 0, title_x2, title_y2);

	get_canvas()->set_inverse();
	get_canvas()->set_color(WHITE);

	get_canvas()->draw_rectangle((int)action_x1,
			(int)action_y1,
			(int)(action_x2 - action_x1),
			(int)(action_y2 - action_y1));
	get_canvas()->draw_rectangle((int)title_x1,
			(int)title_y1,
			(int)(title_x2 - title_x1),
			(int)(title_y2 - title_y1));

	get_canvas()->set_opaque();
}


void CWindowCanvas::create_keyframe(int do_camera)
{
	Track *affected_track = gui->cwindow->calculate_affected_track();
	if( affected_track ) {
		double pos = mwindow->edl->local_session->get_selectionstart(1);
		int64_t position = affected_track->to_units(pos, 0);
		int ix = do_camera ? AUTOMATION_CAMERA_X : AUTOMATION_PROJECTOR_X;
		int iy = do_camera ? AUTOMATION_CAMERA_Y : AUTOMATION_PROJECTOR_Y;
		int iz = do_camera ? AUTOMATION_CAMERA_Z : AUTOMATION_PROJECTOR_Z;
		FloatAuto *prev, *next;
		FloatAutos **autos = (FloatAutos**)affected_track->automation->autos;
		FloatAutos *x_autos = autos[ix];  prev = 0;  next = 0;
		float x_value = x_autos->get_value(position, PLAY_FORWARD, prev, next);
		FloatAutos *y_autos = autos[iy];  prev = 0;  next = 0;
		float y_value = y_autos->get_value(position, PLAY_FORWARD, prev, next);
		FloatAutos *z_autos = autos[iz];  prev = 0;  next = 0;
		float z_value = z_autos->get_value(position, PLAY_FORWARD, prev, next);
		FloatAuto *x_keyframe = 0, *y_keyframe = 0, *z_keyframe = 0;

		gui->cwindow->calculate_affected_autos(affected_track,
			&x_keyframe, &y_keyframe, &z_keyframe,
			do_camera, -1, -1, -1, 0);
		x_keyframe->set_value(x_value);
		y_keyframe->set_value(y_value);
		z_keyframe->set_value(z_value);

		gui->sync_parameters(CHANGE_PARAMS, 1, 1);
	}
}

void CWindowCanvas::camera_keyframe() { create_keyframe(1); }
void CWindowCanvas::projector_keyframe() { create_keyframe(0); }

void CWindowCanvas::reset_keyframe(int do_camera)
{
	Track *affected_track = gui->cwindow->calculate_affected_track();
	if( affected_track ) {
		FloatAuto *x_keyframe = 0, *y_keyframe = 0, *z_keyframe = 0;
		gui->cwindow->calculate_affected_autos(affected_track,
			&x_keyframe, &y_keyframe, &z_keyframe,
			do_camera, 1, 1, 1);

		x_keyframe->set_value(0);
		y_keyframe->set_value(0);
		z_keyframe->set_value(1);

		gui->sync_parameters(CHANGE_PARAMS, 1, 1);
	}
}

void CWindowCanvas::reset_camera() { reset_keyframe(1); }
void CWindowCanvas::reset_projector() { reset_keyframe(0); }


int CWindowCanvas::test_crop(int button_press, int &redraw)
{
	int result = 0;
	int handle_selected = -1;
	float x1 = mwindow->edl->session->crop_x1;
	float y1 = mwindow->edl->session->crop_y1;
	float x2 = mwindow->edl->session->crop_x2;
	float y2 = mwindow->edl->session->crop_y2;
	float cursor_x = get_cursor_x();
	float cursor_y = get_cursor_y();
	float canvas_x1 = x1;
	float canvas_y1 = y1;
	float canvas_x2 = x2;
	float canvas_y2 = y2;
	float canvas_cursor_x = cursor_x;
	float canvas_cursor_y = cursor_y;

	canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);
// Use screen normalized coordinates for hot spot tests.
	output_to_canvas(mwindow->edl, 0, canvas_x1, canvas_y1);
	output_to_canvas(mwindow->edl, 0, canvas_x2, canvas_y2);


	if(gui->current_operation == CWINDOW_CROP)
	{
		handle_selected = gui->crop_handle;
	}
	else
	if(canvas_cursor_x >= canvas_x1 && canvas_cursor_x < canvas_x1 + CROPHANDLE_W &&
		canvas_cursor_y >= canvas_y1 && canvas_cursor_y < canvas_y1 + CROPHANDLE_H)
	{
		handle_selected = 0;
		gui->crop_origin_x = x1;
		gui->crop_origin_y = y1;
	}
	else
	if(canvas_cursor_x >= canvas_x2 - CROPHANDLE_W && canvas_cursor_x < canvas_x2 &&
		canvas_cursor_y >= canvas_y1 && canvas_cursor_y < canvas_y1 + CROPHANDLE_H)
	{
		handle_selected = 1;
		gui->crop_origin_x = x2;
		gui->crop_origin_y = y1;
	}
	else
	if(canvas_cursor_x >= canvas_x1 && canvas_cursor_x < canvas_x1 + CROPHANDLE_W &&
		canvas_cursor_y >= canvas_y2 - CROPHANDLE_H && canvas_cursor_y < canvas_y2)
	{
		handle_selected = 2;
		gui->crop_origin_x = x1;
		gui->crop_origin_y = y2;
	}
	else
	if(canvas_cursor_x >= canvas_x2 - CROPHANDLE_W && canvas_cursor_x < canvas_x2 &&
		canvas_cursor_y >= canvas_y2 - CROPHANDLE_H && canvas_cursor_y < canvas_y2)
	{
		handle_selected = 3;
		gui->crop_origin_x = x2;
		gui->crop_origin_y = y2;
	}
	else
// Start new box
	{
		gui->crop_origin_x = cursor_x;
		gui->crop_origin_y = cursor_y;
	}

// printf("test crop %d %d\n",
// 	gui->current_operation,
// 	handle_selected);

// Start dragging.
	if(button_press)
	{
		if(gui->alt_down())
		{
			gui->crop_translate = 1;
			gui->crop_origin_x1 = x1;
			gui->crop_origin_y1 = y1;
			gui->crop_origin_x2 = x2;
			gui->crop_origin_y2 = y2;
		}
		else
			gui->crop_translate = 0;

		gui->current_operation = CWINDOW_CROP;
		gui->crop_handle = handle_selected;
		gui->x_origin = cursor_x;
		gui->y_origin = cursor_y;
		gui->tool_panel->raise_window();
		result = 1;

		if(handle_selected < 0 && !gui->crop_translate)
		{
			x2 = x1 = cursor_x;
			y2 = y1 = cursor_y;
			mwindow->edl->session->crop_x1 = (int)x1;
			mwindow->edl->session->crop_y1 = (int)y1;
			mwindow->edl->session->crop_x2 = (int)x2;
			mwindow->edl->session->crop_y2 = (int)y2;
			redraw = 1;
		}
	}
	else
// Translate all 4 points
	if(gui->current_operation == CWINDOW_CROP && gui->crop_translate)
	{
		x1 = cursor_x - gui->x_origin + gui->crop_origin_x1;
		y1 = cursor_y - gui->y_origin + gui->crop_origin_y1;
		x2 = cursor_x - gui->x_origin + gui->crop_origin_x2;
		y2 = cursor_y - gui->y_origin + gui->crop_origin_y2;

		mwindow->edl->session->crop_x1 = (int)x1;
		mwindow->edl->session->crop_y1 = (int)y1;
		mwindow->edl->session->crop_x2 = (int)x2;
		mwindow->edl->session->crop_y2 = (int)y2;
		result = 1;
		redraw = 1;
	}
	else
// Update dragging
	if(gui->current_operation == CWINDOW_CROP)
	{
		switch(gui->crop_handle)
		{
			case -1:
				x1 = gui->crop_origin_x;
				y1 = gui->crop_origin_y;
				x2 = gui->crop_origin_x;
				y2 = gui->crop_origin_y;
				if(cursor_x < gui->x_origin)
				{
					if(cursor_y < gui->y_origin)
					{
						x1 = cursor_x;
						y1 = cursor_y;
					}
					else
					if(cursor_y >= gui->y_origin)
					{
						x1 = cursor_x;
						y2 = cursor_y;
					}
				}
				else
				if(cursor_x  >= gui->x_origin)
				{
					if(cursor_y < gui->y_origin)
					{
						y1 = cursor_y;
						x2 = cursor_x;
					}
					else
					if(cursor_y >= gui->y_origin)
					{
						x2 = cursor_x;
						y2 = cursor_y;
					}
				}

// printf("test crop %d %d %d %d\n",
// 	mwindow->edl->session->crop_x1,
// 	mwindow->edl->session->crop_y1,
// 	mwindow->edl->session->crop_x2,
// 	mwindow->edl->session->crop_y2);
				break;
			case 0:
				x1 = cursor_x - gui->x_origin + gui->crop_origin_x;
				y1 = cursor_y - gui->y_origin + gui->crop_origin_y;
				break;
			case 1:
				x2 = cursor_x - gui->x_origin + gui->crop_origin_x;
				y1 = cursor_y - gui->y_origin + gui->crop_origin_y;
				break;
			case 2:
				x1 = cursor_x - gui->x_origin + gui->crop_origin_x;
				y2 = cursor_y - gui->y_origin + gui->crop_origin_y;
				break;
			case 3:
				x2 = cursor_x - gui->x_origin + gui->crop_origin_x;
				y2 = cursor_y - gui->y_origin + gui->crop_origin_y;
				break;
		}

		if(!EQUIV(mwindow->edl->session->crop_x1, x1) ||
			!EQUIV(mwindow->edl->session->crop_x2, x2) ||
			!EQUIV(mwindow->edl->session->crop_y1, y1) ||
			!EQUIV(mwindow->edl->session->crop_y2, y2))
		{
			if (x1 > x2)
			{
				float tmp = x1;
				x1 = x2;
				x2 = tmp;
				switch (gui->crop_handle)
				{
					case 0:	gui->crop_handle = 1; break;
					case 1:	gui->crop_handle = 0; break;
					case 2:	gui->crop_handle = 3; break;
					case 3:	gui->crop_handle = 2; break;
					default: break;
				}
			}

			if (y1 > y2)
			{
				float tmp = y1;
				y1 = y2;
				y2 = tmp;
				switch (gui->crop_handle)
				{
					case 0:	gui->crop_handle = 2; break;
					case 1:	gui->crop_handle = 3; break;
					case 2:	gui->crop_handle = 0; break;
					case 3:	gui->crop_handle = 1; break;
					default: break;
				}
			}

			mwindow->edl->session->crop_x1 = (int)x1;
			mwindow->edl->session->crop_y1 = (int)y1;
			mwindow->edl->session->crop_x2 = (int)x2;
			mwindow->edl->session->crop_y2 = (int)y2;
			result = 1;
			redraw = 1;
		}
	}
	else
// Update cursor font
	if(handle_selected >= 0)
	{
		switch(handle_selected)
		{
			case 0:
				set_cursor(UPLEFT_RESIZE);
				break;
			case 1:
				set_cursor(UPRIGHT_RESIZE);
				break;
			case 2:
				set_cursor(DOWNLEFT_RESIZE);
				break;
			case 3:
				set_cursor(DOWNRIGHT_RESIZE);
				break;
		}
		result = 1;
	}
	else
	{
		set_cursor(ARROW_CURSOR);
	}
#define CLAMP(x, y, z) ((x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x))))

	if(redraw)
	{
		CLAMP(mwindow->edl->session->crop_x1, 0, mwindow->edl->session->output_w);
		CLAMP(mwindow->edl->session->crop_x2, 0, mwindow->edl->session->output_w);
		CLAMP(mwindow->edl->session->crop_y1, 0, mwindow->edl->session->output_h);
		CLAMP(mwindow->edl->session->crop_y2, 0, mwindow->edl->session->output_h);
// printf("CWindowCanvas::test_crop %d %d %d %d\n",
// 	mwindow->edl->session->crop_x2,
// 	mwindow->edl->session->crop_y2,
// 	mwindow->edl->calculate_output_w(0),
// 	mwindow->edl->calculate_output_h(0));
	}
	return result;
}


void CWindowCanvas::draw_crop()
{
	get_canvas()->set_inverse();
	get_canvas()->set_color(WHITE);

	float x1 = mwindow->edl->session->crop_x1;
	float y1 = mwindow->edl->session->crop_y1;
	float x2 = mwindow->edl->session->crop_x2;
	float y2 = mwindow->edl->session->crop_y2;

	output_to_canvas(mwindow->edl, 0, x1, y1);
	output_to_canvas(mwindow->edl, 0, x2, y2);

	if(x2 - x1 && y2 - y1)
		get_canvas()->draw_rectangle((int)x1,
			(int)y1,
			(int)(x2 - x1),
			(int)(y2 - y1));

	draw_crophandle((int)x1, (int)y1);
	draw_crophandle((int)x2 - CROPHANDLE_W, (int)y1);
	draw_crophandle((int)x1, (int)y2 - CROPHANDLE_H);
	draw_crophandle((int)x2 - CROPHANDLE_W, (int)y2 - CROPHANDLE_H);
	get_canvas()->set_opaque();
}








void CWindowCanvas::draw_bezier(int do_camera)
{
	Track *track = gui->cwindow->calculate_affected_track();

	if(!track) return;

	float center_x;
	float center_y;
	float center_z;
	int64_t position = track->to_units(
		mwindow->edl->local_session->get_selectionstart(1),
		0);
	if( do_camera ) {
		track->automation->get_camera(&center_x,
			&center_y, &center_z, position, PLAY_FORWARD);
// follow image, not camera
		center_x = -center_x;  center_y = -center_y;
	}
	else
		track->automation->get_projector(&center_x,
			&center_y, &center_z, position, PLAY_FORWARD);

//	center_x += track->track_w / 2;
//	center_y += track->track_h / 2;
	center_x += mwindow->edl->session->output_w / 2;
	center_y += mwindow->edl->session->output_h / 2;
	float track_x1 = center_x - track->track_w / 2 * center_z;
	float track_y1 = center_y - track->track_h / 2 * center_z;
	float track_x2 = track_x1 + track->track_w * center_z;
	float track_y2 = track_y1 + track->track_h * center_z;

	output_to_canvas(mwindow->edl, 0, track_x1, track_y1);
	output_to_canvas(mwindow->edl, 0, track_x2, track_y2);

#define DRAW_PROJECTION(offset) \
	get_canvas()->draw_rectangle((int)track_x1 + offset, \
		(int)track_y1 + offset, \
		(int)(track_x2 - track_x1), \
		(int)(track_y2 - track_y1)); \
	get_canvas()->draw_line((int)track_x1 + offset,  \
		(int)track_y1 + offset, \
		(int)track_x2 + offset, \
		(int)track_y2 + offset); \
	get_canvas()->draw_line((int)track_x2 + offset,  \
		(int)track_y1 + offset, \
		(int)track_x1 + offset, \
		(int)track_y2 + offset); \


// Drop shadow
	get_canvas()->set_color(BLACK);
	DRAW_PROJECTION(1);

//	canvas->set_inverse();
	if(do_camera)
		get_canvas()->set_color(GREEN);
	else
		get_canvas()->set_color(RED);

	DRAW_PROJECTION(0);
//	canvas->set_opaque();

}

int CWindowCanvas::test_bezier(int button_press,
	int &redraw,
	int &redraw_canvas,
	int &rerender,
	int do_camera)
{
	int result = 0;

// Processing drag operation.
// Create keyframe during first cursor motion.
	if(!button_press)
	{

		float cursor_x = get_cursor_x();
		float cursor_y = get_cursor_y();
		canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);

		if(gui->current_operation == CWINDOW_CAMERA ||
			gui->current_operation == CWINDOW_PROJECTOR)
		{
			if(!gui->ctrl_down() && gui->shift_down() && !gui->translating_zoom)
			{
				gui->translating_zoom = 1;
				gui->reset_affected();
			}
			else
			if(!gui->ctrl_down() && !gui->shift_down() && gui->translating_zoom)
			{
				gui->translating_zoom = 0;
				gui->reset_affected();
			}

// Get target keyframe
			float last_center_x;
			float last_center_y;
			float last_center_z;
			int created;

			if(!gui->affected_x && !gui->affected_y && !gui->affected_z)
			{
				FloatAutos *affected_x_autos;
				FloatAutos *affected_y_autos;
				FloatAutos *affected_z_autos;
				if(!gui->affected_track) return 0;
				double position = mwindow->edl->local_session->get_selectionstart(1);
				int64_t track_position = gui->affected_track->to_units(position, 0);

				if(mwindow->edl->session->cwindow_operation == CWINDOW_CAMERA)
				{
					affected_x_autos = (FloatAutos*)gui->affected_track->automation->autos[AUTOMATION_CAMERA_X];
					affected_y_autos = (FloatAutos*)gui->affected_track->automation->autos[AUTOMATION_CAMERA_Y];
					affected_z_autos = (FloatAutos*)gui->affected_track->automation->autos[AUTOMATION_CAMERA_Z];
				}
				else
				{
					affected_x_autos = (FloatAutos*)gui->affected_track->automation->autos[AUTOMATION_PROJECTOR_X];
					affected_y_autos = (FloatAutos*)gui->affected_track->automation->autos[AUTOMATION_PROJECTOR_Y];
					affected_z_autos = (FloatAutos*)gui->affected_track->automation->autos[AUTOMATION_PROJECTOR_Z];
				}


				if(gui->translating_zoom)
				{
					FloatAuto *previous = 0;
					FloatAuto *next = 0;
					float new_z = affected_z_autos->get_value(
						track_position,
						PLAY_FORWARD,
						previous,
						next);
					gui->affected_z =
						(FloatAuto*)gui->cwindow->calculate_affected_auto(
							affected_z_autos, 1, &created, 0);
					if(created) {
						gui->affected_z->set_value(new_z);
						redraw_canvas = 1;
					}
				}
				else
				{
					FloatAuto *previous = 0;
					FloatAuto *next = 0;
					float new_x = affected_x_autos->get_value(
						track_position,
						PLAY_FORWARD,
						previous,
						next);
					previous = 0;
					next = 0;
					float new_y = affected_y_autos->get_value(
						track_position,
						PLAY_FORWARD,
						previous,
						next);
					gui->affected_x =
						(FloatAuto*)gui->cwindow->calculate_affected_auto(
							affected_x_autos, 1, &created, 0);
					if(created) {
						gui->affected_x->set_value(new_x);
						redraw_canvas = 1;
					}
					gui->affected_y =
						(FloatAuto*)gui->cwindow->calculate_affected_auto(
							affected_y_autos, 1, &created, 0);
					if(created) {
						gui->affected_y->set_value(new_y);
						redraw_canvas = 1;
					}
				}

				calculate_origin();

				if(gui->translating_zoom)
				{
					gui->center_z = gui->affected_z->get_value();
				}
				else
				{
					gui->center_x = gui->affected_x->get_value();
					gui->center_y = gui->affected_y->get_value();
				}

				rerender = 1;
				redraw = 1;
			}

			if(gui->translating_zoom)
			{
				last_center_z = gui->affected_z->get_value();
				float z = gui->center_z + (cursor_y - gui->y_origin) / 128;
				if( z < 0 ) z = 0;
				if(!EQUIV(last_center_z, z))
				{
					rerender = 1;
					redraw = 1;
					redraw_canvas = 1;
				}
				gui->affected_z->set_value(z);
			}
			else
			{
				last_center_x = gui->affected_x->get_value();
				last_center_y = gui->affected_y->get_value();
				float dx = cursor_x - gui->x_origin;
				float dy = cursor_y - gui->y_origin;
// follow image, not camera
				if(gui->current_operation == CWINDOW_CAMERA ) {
					dx = -dx;  dy = -dy;
				}
				float x = gui->center_x + dx;
				float y = gui->center_y + dy;
				gui->affected_x->set_value(x);
				gui->affected_y->set_value(y);
				if( !EQUIV(last_center_x, x) || !EQUIV(last_center_y, y) )
				{
					rerender = 1;
					redraw = 1;
					redraw_canvas = 1;
				}
				gui->affected_x->set_value(x);
				gui->affected_y->set_value(y);
			}
		}

		result = 1;
	}
	else
// Begin drag operation.  Don't create keyframe here.
	{
// Get affected track off of the first recordable video track.
// Calculating based on the alpha channel would require recording what layer
// each output pixel belongs to as they're rendered and stacked.  Forget it.
		gui->affected_track = gui->cwindow->calculate_affected_track();
		gui->reset_affected();

		if(gui->affected_track)
		{
			if( do_camera )
				mwindow->undo->update_undo_before(_("camera"), this);
			else
				mwindow->undo->update_undo_before(_("projector"), this);

			gui->current_operation =
				mwindow->edl->session->cwindow_operation;
			gui->tool_panel->raise_window();
			result = 1;
		}
	}

	return result;
}


int CWindowCanvas::test_zoom(int &redraw)
{
	int result = 0;
	float x, y;
	float zoom = 0;

	if( mwindow->edl->session->cwindow_scrollbars ) {
		if( *gui->zoom_panel->get_text() != 'x' ) {
// Find current zoom in table
			int idx = total_zooms;  float old_zoom = get_zoom();
			while( --idx >= 0 && !EQUIV(my_zoom_table[idx], old_zoom) );
			if( idx >= 0 ) {
				idx += get_buttonpress() == 5 ||
					 gui->ctrl_down() || gui->shift_down() ?  -1 : +1 ;
				CLAMP(idx, 0, total_zooms-1);
				zoom = my_zoom_table[idx];
			}
		}
		x = get_cursor_x();  y = get_cursor_y();
		if( !zoom ) {
			mwindow->edl->session->cwindow_scrollbars = 0;
			gui->zoom_panel->update(0);
			zoom = gui->get_auto_zoom();
		}
		else {
			gui->zoom_panel->ZoomPanel::update(zoom);
			float output_x = x, output_y = y;
			canvas_to_output(mwindow->edl, 0, output_x, output_y);
			x = output_x - x / zoom;
			y = output_y - y / zoom;
		}
	}
	else {
		mwindow->edl->session->cwindow_scrollbars = 1;
		x = (mwindow->edl->session->output_w - w) / 2;
		y = (mwindow->edl->session->output_h - h) / 2;
		zoom = 1;
	}
	update_zoom((int)x, (int)y, zoom);

	gui->composite_panel->cpanel_zoom->update(zoom);

	reposition_window(mwindow->edl,
			mwindow->theme->ccanvas_x, mwindow->theme->ccanvas_y,
			mwindow->theme->ccanvas_w, mwindow->theme->ccanvas_h);
	redraw = 1;  result = 1;

	return result;
}


void CWindowCanvas::calculate_origin()
{
	gui->x_origin = get_cursor_x();
	gui->y_origin = get_cursor_y();
//printf("CWindowCanvas::calculate_origin 1 %f %f\n", gui->x_origin, gui->y_origin);
	canvas_to_output(mwindow->edl, 0, gui->x_origin, gui->y_origin);
//printf("CWindowCanvas::calculate_origin 2 %f %f\n", gui->x_origin, gui->y_origin);
}


int CWindowCanvas::cursor_leave_event()
{
	set_cursor(ARROW_CURSOR);
	return 1;
}

int CWindowCanvas::cursor_enter_event()
{
	int redraw = 0;
	switch(mwindow->edl->session->cwindow_operation)
	{
		case CWINDOW_CAMERA:
		case CWINDOW_PROJECTOR:
			set_cursor(MOVE_CURSOR);
			break;
		case CWINDOW_ZOOM:
			set_cursor(MOVE_CURSOR);
			break;
		case CWINDOW_CROP:
			test_crop(0, redraw);
			break;
		case CWINDOW_PROTECT:
			set_cursor(ARROW_CURSOR);
			break;
		case CWINDOW_MASK:
		case CWINDOW_RULER:
			set_cursor(CROSS_CURSOR);
			break;
		case CWINDOW_EYEDROP:
			set_cursor(CROSS_CURSOR);
			break;
	}
	return 1;
}

int CWindowCanvas::cursor_motion_event()
{
	int redraw = 0, result = 0, rerender = 0, redraw_canvas = 0;


//printf("CWindowCanvas::cursor_motion_event %d current_operation=%d\n", __LINE__, gui->current_operation);
	switch(gui->current_operation)
	{
		case CWINDOW_SCROLL:
		{
			float zoom = get_zoom();
			float cursor_x = get_cursor_x();
			float cursor_y = get_cursor_y();

			float zoom_x, zoom_y, conformed_w, conformed_h;
			get_zooms(mwindow->edl, 0, zoom_x, zoom_y, conformed_w, conformed_h);
			cursor_x = (float)cursor_x / zoom_x + gui->x_offset;
			cursor_y = (float)cursor_y / zoom_y + gui->y_offset;



			int x = (int)(gui->x_origin - cursor_x + gui->x_offset);
			int y = (int)(gui->y_origin - cursor_y + gui->y_offset);

			update_zoom(x,
				y,
				zoom);
			update_scrollbars(0);
			redraw = 1;
			result = 1;
			break;
		}

		case CWINDOW_RULER:
			result = do_ruler(0, 1, 0, 0);
			break;

		case CWINDOW_CAMERA:
			result = test_bezier(0, redraw, redraw_canvas, rerender, 1);
			break;

		case CWINDOW_PROJECTOR:
			result = test_bezier(0, redraw, redraw_canvas, rerender, 0);
			break;


		case CWINDOW_CROP:
			result = test_crop(0, redraw);
// printf("CWindowCanvas::cursor_motion_event %d result=%d redraw=%d\n",
// __LINE__,
// result,
// redraw);
			break;

		case CWINDOW_MASK:
		case CWINDOW_MASK_CONTROL_IN:
		case CWINDOW_MASK_CONTROL_OUT:
		case CWINDOW_MASK_TRANSLATE:

			result = do_mask(redraw,
				rerender,
				0,
				1,
				0);
			break;

		case CWINDOW_EYEDROP:
			result = do_eyedrop(rerender, 0, 0);
			break;

		default:
			break;

	}


// cursor font changes
	if(!result)
	{
// printf("CWindowCanvas::cursor_motion_event %d cwindow_operation=%d\n",
// __LINE__,
// mwindow->edl->session->cwindow_operation);
		switch(mwindow->edl->session->cwindow_operation)
		{
			case CWINDOW_CROP:
				result = test_crop(0, redraw);
				break;
			case CWINDOW_RULER:
				result = do_ruler(0, 1, 0, 0);
				break;
			case CWINDOW_MASK:
				result = do_mask(redraw,
					rerender,
					0,
					1,
					0);
					break;
		}
	}

	int change_type = rerender ? CHANGE_PARAMS : -1;
	gui->sync_parameters(change_type, redraw, redraw_canvas);

	return result;
}

int CWindowCanvas::button_press_event()
{
	int result = 0;
	int redraw = 0;
	int redraw_canvas = 0;
	int rerender = 0;

	if(Canvas::button_press_event()) return 1;

	gui->translating_zoom = gui->shift_down();

	calculate_origin();
//printf("CWindowCanvas::button_press_event 2 %f %f\n", gui->x_origin, gui->y_origin, gui->x_origin, gui->y_origin);

	float zoom_x, zoom_y, conformed_w, conformed_h;
	get_zooms(mwindow->edl, 0, zoom_x, zoom_y, conformed_w, conformed_h);
	gui->x_offset = get_x_offset(mwindow->edl, 0, zoom_x, conformed_w, conformed_h);
	gui->y_offset = get_y_offset(mwindow->edl, 0, zoom_y, conformed_w, conformed_h);

// Scroll view
	if( mwindow->edl->session->cwindow_operation != CWINDOW_PROTECT &&
	    get_buttonpress() == 2 )
	{
		gui->current_operation = CWINDOW_SCROLL;
		result = 1;
	}
	else
// Adjust parameter
	{
		switch(mwindow->edl->session->cwindow_operation)
		{
			case CWINDOW_RULER:
				result = do_ruler(0, 0, 1, 0);
				break;

			case CWINDOW_CAMERA:
				result = test_bezier(1, redraw, redraw_canvas, rerender, 1);
				break;

			case CWINDOW_PROJECTOR:
				result = test_bezier(1, redraw, redraw_canvas, rerender, 0);
				break;

			case CWINDOW_ZOOM:
				result = test_zoom(redraw);
				break;

			case CWINDOW_CROP:
				result = test_crop(1, redraw);
				break;

			case CWINDOW_MASK:
				if(get_buttonpress() == 1)
					result = do_mask(redraw, rerender, 1, 0, 0);
				break;

			case CWINDOW_EYEDROP:
				result = do_eyedrop(rerender, 1, 0);
				break;
		}
	}

	int change_type = rerender ? CHANGE_PARAMS : -1;
	gui->sync_parameters(change_type, redraw, redraw_canvas);

	return result;
}

int CWindowCanvas::button_release_event()
{
	int result = 0;

	switch(gui->current_operation)
	{
		case CWINDOW_SCROLL:
			result = 1;
			break;

		case CWINDOW_RULER:
			do_ruler(0, 0, 0, 1);
			break;

		case CWINDOW_CAMERA:
			mwindow->undo->update_undo_after(_("camera"), LOAD_AUTOMATION);
			break;

		case CWINDOW_PROJECTOR:
			mwindow->undo->update_undo_after(_("projector"), LOAD_AUTOMATION);
			break;

		case CWINDOW_MASK:
		case CWINDOW_MASK_CONTROL_IN:
		case CWINDOW_MASK_CONTROL_OUT:
		case CWINDOW_MASK_TRANSLATE:
// Finish mask operation
			gui->mask_keyframe = 0;
			mwindow->undo->update_undo_after(_("mask"), LOAD_AUTOMATION);
			break;
		case CWINDOW_NONE:
			result = Canvas::button_release_event();
			break;
	}

	gui->current_operation = CWINDOW_NONE;
	return result;
}

void CWindowCanvas::zoom_resize_window(float percentage)
{
	int canvas_w, canvas_h;
	int new_w, new_h;


// Get required canvas size
	calculate_sizes(mwindow->edl->get_aspect_ratio(),
		mwindow->edl->session->output_w,
		mwindow->edl->session->output_h,
		percentage,
		canvas_w,
		canvas_h);

// Estimate window size from current borders
	new_w = canvas_w + (gui->get_w() - mwindow->theme->ccanvas_w);
	new_h = canvas_h + (gui->get_h() - mwindow->theme->ccanvas_h);

//printf("CWindowCanvas::zoom_resize_window %d %d %d\n", __LINE__, new_w, new_h);
	mwindow->session->cwindow_w = new_w;
	mwindow->session->cwindow_h = new_h;

	mwindow->theme->get_cwindow_sizes(gui,
		mwindow->session->cwindow_controls);

// Estimate again from new borders
	new_w = canvas_w + (mwindow->session->cwindow_w - mwindow->theme->ccanvas_w);
	new_h = canvas_h + (mwindow->session->cwindow_h - mwindow->theme->ccanvas_h);
//printf("CWindowCanvas::zoom_resize_window %d %d %d\n", __LINE__, new_w, new_h);

	gui->resize_window(new_w, new_h);
	gui->resize_event(new_w, new_h);
}

void CWindowCanvas::toggle_controls()
{
	mwindow->session->cwindow_controls = !mwindow->session->cwindow_controls;
	gui->resize_event(gui->get_w(), gui->get_h());
}

int CWindowCanvas::get_cwindow_controls()
{
	return mwindow->session->cwindow_controls;
}



