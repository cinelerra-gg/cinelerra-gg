
/*
 * CINELERRA
 * Copyright (C) 2008-2017 Adam Williams <broadcast at earthling dot net>
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

#include "bcsignals.h"
#include "canvas.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "keys.h"
#include "language.h"
#include "mainsession.h"
#include "mwindowgui.h"
#include "mutex.h"
#include "mwindow.h"
#include "playback3d.h"
#include "videodevice.h"
#include "vframe.h"



Canvas::Canvas(MWindow *mwindow,
	BC_WindowBase *subwindow,
	int x,
	int y,
	int w,
	int h,
	int output_w,
	int output_h,
	int use_scrollbars)
{
	reset();

	if(w < 10) w = 10;
	if(h < 10) h = 10;
	this->mwindow = mwindow;
	this->subwindow = subwindow;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	this->output_w = output_w;
	this->output_h = output_h;
	this->use_scrollbars = use_scrollbars;
	this->canvas_auxwindow = 0;
	this->scr_w0 = subwindow->get_screen_w(0, 0);
	this->root_w = subwindow->get_root_w(0);
	this->root_h = subwindow->get_root_h(0);
	canvas_lock = new Condition(1, "Canvas::canvas_lock", 1);
}

Canvas::~Canvas()
{
	if(refresh_frame) delete refresh_frame;
	delete canvas_menu;
 	if(yscroll) delete yscroll;
 	if(xscroll) delete xscroll;
	delete canvas_subwindow;
	delete canvas_fullscreen;
	delete canvas_lock;
}

void Canvas::reset()
{
	use_scrollbars = 0;
	output_w = 0;
	output_h = 0;
	xscroll = 0;
	yscroll = 0;
	refresh_frame = 0;
	canvas_subwindow = 0;
	canvas_fullscreen = 0;
	is_processing = 0;
	is_fullscreen = 0;
	cursor_inside = 0;
}

BC_WindowBase *Canvas::lock_canvas(const char *loc)
{
	canvas_lock->lock(loc);
	BC_WindowBase *wdw = get_canvas();
	if( wdw ) wdw->lock_window(loc);
	return wdw;
}

void Canvas::unlock_canvas()
{
	BC_WindowBase *wdw = get_canvas();
	if( wdw ) wdw->unlock_window();
	canvas_lock->unlock();
}

BC_WindowBase* Canvas::get_canvas()
{
	if(get_fullscreen() && canvas_fullscreen)
		return canvas_fullscreen;
	return canvas_auxwindow ? canvas_auxwindow : canvas_subwindow;
}

void Canvas::use_auxwindow(BC_WindowBase *aux)
{
	canvas_auxwindow = aux;
}

void Canvas::use_cwindow()
{
	canvas_menu->use_cwindow();
	fullscreen_menu->use_cwindow();
}

void Canvas::use_rwindow()
{
	canvas_menu->use_rwindow();
}

void Canvas::use_vwindow()
{
	canvas_menu->use_vwindow();
}

int Canvas::get_fullscreen()
{
	return is_fullscreen;
}

void Canvas::set_fullscreen(int value)
{
	is_fullscreen = value;
}

// Get dimensions given a zoom
void Canvas::calculate_sizes(float aspect_ratio,
	int output_w,
	int output_h,
	float zoom,
	int &w,
	int &h)
{
// Horizontal stretch
	if((float)output_w / output_h <= aspect_ratio)
	{
		w = (int)((float)output_h * aspect_ratio * zoom);
		h = (int)((float)output_h * zoom);
	}
	else
// Vertical stretch
	{
		h = (int)((float)output_w / aspect_ratio * zoom);
		w = (int)((float)output_w * zoom);
	}
}

float Canvas::get_x_offset(EDL *edl,
	int single_channel,
	float zoom_x,
	float conformed_w,
	float conformed_h)
{
	if(use_scrollbars)
	{
		if(xscroll)
		{
// If the projection is smaller than the canvas, this forces it in the center.
//			if(conformed_w < w_visible)
//				return -(float)(w_visible - conformed_w) / 2;

			return (float)get_xscroll();
		}
		else
			return ((float)-get_canvas()->get_w() / zoom_x +
				edl->session->output_w) / 2;
	}
	else
	{
		int out_w, out_h;
		int canvas_w = get_canvas()->get_w();
		int canvas_h = get_canvas()->get_h();
		out_w = canvas_w;
		out_h = canvas_h;

		if((float)out_w / out_h > conformed_w / conformed_h)
		{
			out_w = (int)(out_h * conformed_w / conformed_h + 0.5);
		}

		if(out_w < canvas_w)
			return -(canvas_w - out_w) / 2 / zoom_x;
	}

	return 0;
}

float Canvas::get_y_offset(EDL *edl,
	int single_channel,
	float zoom_y,
	float conformed_w,
	float conformed_h)
{
	if(use_scrollbars)
	{
		if(yscroll)
		{
// If the projection is smaller than the canvas, this forces it in the center.
//			if(conformed_h < h_visible)
//				return -(float)(h_visible - conformed_h) / 2;

			return (float)get_yscroll();
		}
		else
			return ((float)-get_canvas()->get_h() / zoom_y +
				edl->session->output_h) / 2;
	}
	else
	{
		int out_w, out_h;
		int canvas_w = get_canvas()->get_w();
		int canvas_h = get_canvas()->get_h();
		out_w = canvas_w;
		out_h = canvas_h;

		if((float)out_w / out_h <= conformed_w / conformed_h)
		{
			out_h = (int)((float)out_w / (conformed_w / conformed_h) + 0.5);
		}

//printf("Canvas::get_y_offset 1 %d %d %f\n", out_h, canvas_h, -((float)canvas_h - out_h) / 2);
		if(out_h < canvas_h)
			return -((float)canvas_h - out_h) / 2 / zoom_y;
	}

	return 0;
}

// This may not be used anymore
void Canvas::check_boundaries(EDL *edl, int &x, int &y, float &zoom)
{
	if(x + w_visible > w_needed) x = w_needed - w_visible;
	if(y + h_visible > h_needed) y = h_needed - h_visible;

	if(x < 0) x = 0;
	if(y < 0) y = 0;
}

void Canvas::update_scrollbars(int flush)
{
	if(use_scrollbars)
	{
		if(xscroll) xscroll->update_length(w_needed, get_xscroll(), w_visible, flush);
		if(yscroll) yscroll->update_length(h_needed, get_yscroll(), h_visible, flush);
	}
}

void Canvas::get_zooms(EDL *edl,
	int single_channel,
	float &zoom_x,
	float &zoom_y,
	float &conformed_w,
	float &conformed_h)
{
	edl->calculate_conformed_dimensions(single_channel,
		conformed_w,
		conformed_h);

	if(use_scrollbars)
	{
		zoom_x = get_zoom() *
			conformed_w /
			edl->session->output_w;
		zoom_y = get_zoom() *
			conformed_h /
			edl->session->output_h;
	}
	else
	{
		int out_w, out_h;
		int canvas_w = get_canvas()->get_w();
		int canvas_h = get_canvas()->get_h();

		out_w = canvas_w;
		out_h = canvas_h;

		if((float)out_w / out_h > conformed_w / conformed_h)
		{
			out_w = (int)((float)out_h * conformed_w / conformed_h + 0.5);
		}
		else
		{
			out_h = (int)((float)out_w / (conformed_w / conformed_h) + 0.5);
		}

		zoom_x = (float)out_w / edl->session->output_w;
		zoom_y = (float)out_h / edl->session->output_h;
//printf("get zooms 2 %d %d %f %f\n", canvas_w, canvas_h, conformed_w, conformed_h);
	}
}

// Convert a coordinate on the canvas to a coordinate on the output
void Canvas::canvas_to_output(EDL *edl, int single_channel, float &x, float &y)
{
	float zoom_x, zoom_y, conformed_w, conformed_h;
	get_zooms(edl, single_channel, zoom_x, zoom_y, conformed_w, conformed_h);

//printf("Canvas::canvas_to_output y=%f zoom_y=%f y_offset=%f\n",
//	y, zoom_y, get_y_offset(edl, single_channel, zoom_y, conformed_w, conformed_h));

	x = (float)x / zoom_x + get_x_offset(edl, single_channel, zoom_x, conformed_w, conformed_h);
	y = (float)y / zoom_y + get_y_offset(edl, single_channel, zoom_y, conformed_w, conformed_h);
}

void Canvas::output_to_canvas(EDL *edl, int single_channel, float &x, float &y)
{
	float zoom_x, zoom_y, conformed_w, conformed_h;
	get_zooms(edl, single_channel, zoom_x, zoom_y, conformed_w, conformed_h);

//printf("Canvas::output_to_canvas x=%f zoom_x=%f x_offset=%f\n", x, zoom_x, get_x_offset(edl, single_channel, zoom_x, conformed_w));

	x = (float)zoom_x * (x - get_x_offset(edl, single_channel, zoom_x, conformed_w, conformed_h));
	y = (float)zoom_y * (y - get_y_offset(edl, single_channel, zoom_y, conformed_w, conformed_h));
}



void Canvas::get_transfers(EDL *edl,
	float &output_x1, float &output_y1, float &output_x2, float &output_y2,
	float &canvas_x1, float &canvas_y1, float &canvas_x2, float &canvas_y2,
	int canvas_w, int canvas_h)
{
//printf("Canvas::get_transfers %d canvas_w=%d canvas_h=%d\n", 
// __LINE__,  canvas_w, canvas_h);
// automatic canvas size detection
	if(canvas_w < 0) canvas_w = get_canvas()->get_w();
	if(canvas_h < 0) canvas_h = get_canvas()->get_h();

// Canvas is zoomed to a portion of the output frame
	if(use_scrollbars)
	{
		float in_x1, in_y1, in_x2, in_y2;
		float out_x1, out_y1, out_x2, out_y2;
		float zoom_x, zoom_y, conformed_w, conformed_h;

		get_zooms(edl, 0, zoom_x, zoom_y, conformed_w, conformed_h);
		out_x1 = 0;
		out_y1 = 0;
		out_x2 = canvas_w;
		out_y2 = canvas_h;
		in_x1 = 0;
		in_y1 = 0;
		in_x2 = canvas_w;
		in_y2 = canvas_h;

		canvas_to_output(edl, 0, in_x1, in_y1);
		canvas_to_output(edl, 0, in_x2, in_y2);

//printf("Canvas::get_transfers 1 %.0f %.0f %.0f %.0f -> %.0f %.0f %.0f %.0f\n",
//in_x1, in_y1, in_x2, in_y2, out_x1, out_y1, out_x2, out_y2);

		if(in_x1 < 0)
		{
			out_x1 += -in_x1 * zoom_x;
			in_x1 = 0;
		}

		if(in_y1 < 0)
		{
			out_y1 += -in_y1 * zoom_y;
			in_y1 = 0;
		}

		int output_w = get_output_w(edl);
		int output_h = get_output_h(edl);

		if(in_x2 > output_w)
		{
			out_x2 -= (in_x2 - output_w) * zoom_x;
			in_x2 = output_w;
		}

		if(in_y2 > output_h)
		{
			out_y2 -= (in_y2 - output_h) * zoom_y;
			in_y2 = output_h;
		}
// printf("Canvas::get_transfers 2 %.0f %.0f %.0f %.0f -> %.0f %.0f %.0f %.0f\n",
// 			in_x1, in_y1, in_x2, in_y2, out_x1, out_y1, out_x2, out_y2);

		output_x1 = in_x1;
		output_y1 = in_y1;
		output_x2 = in_x2;
		output_y2 = in_y2;
		canvas_x1 = out_x1;
		canvas_y1 = out_y1;
		canvas_x2 = out_x2;
		canvas_y2 = out_y2;

// Center on canvas
//		if(!scrollbars_exist())
//		{
//			out_x = canvas_w / 2 - out_w / 2;
//			out_y = canvas_h / 2 - out_h / 2;
//		}

	}
	else
// The output frame is normalized to the canvas
	{
// Default canvas coords fill the entire canvas
		canvas_x1 = 0;
		canvas_y1 = 0;
		canvas_x2 = canvas_w;
		canvas_y2 = canvas_h;

		if(edl)
		{
// Use EDL aspect ratio to shrink one of the canvas dimensions
			float out_w = canvas_x2 - canvas_x1;
			float out_h = canvas_y2 - canvas_y1;
			if(out_w / out_h > edl->get_aspect_ratio())
			{
				out_w = (int)(out_h * edl->get_aspect_ratio() + 0.5);
				canvas_x1 = canvas_w / 2 - out_w / 2;
			}
			else
			{
				out_h = (int)(out_w / edl->get_aspect_ratio() + 0.5);
				canvas_y1 = canvas_h / 2 - out_h / 2;
// printf("Canvas::get_transfers %d canvas_h=%d out_h=%f canvas_y1=%f\n",
// __LINE__,
// canvas_h,
// out_h,
// canvas_y1);
			}
			canvas_x2 = canvas_x1 + out_w;
			canvas_y2 = canvas_y1 + out_h;

// Get output frame coords from EDL
			output_x1 = 0;
			output_y1 = 0;
			output_x2 = get_output_w(edl);
			output_y2 = get_output_h(edl);
		}
		else
// No EDL to get aspect ratio or output frame coords from
		{
			output_x1 = 0;
			output_y1 = 0;
			output_x2 = this->output_w;
			output_y2 = this->output_h;
		}
	}

// Clamp to minimum value
	output_x1 = MAX(0, output_x1);
	output_y1 = MAX(0, output_y1);
	output_x2 = MAX(output_x1, output_x2);
	output_y2 = MAX(output_y1, output_y2);
	canvas_x1 = MAX(0, canvas_x1);
	canvas_y1 = MAX(0, canvas_y1);
	canvas_x2 = MAX(canvas_x1, canvas_x2);
	canvas_y2 = MAX(canvas_y1, canvas_y2);
// printf("Canvas::get_transfers %d %f,%f %f,%f -> %f,%f %f,%f\n",
// __LINE__,
// output_x1,
// output_y1,
// output_x2,
// output_y2,
// canvas_x1,
// canvas_y1,
// canvas_x2,
// canvas_y2);
}

int Canvas::scrollbars_exist()
{
	return(use_scrollbars && (xscroll || yscroll));
}

int Canvas::get_output_w(EDL *edl)
{
	return !edl ? 0 : edl->session->output_w;
}

int Canvas::get_output_h(EDL *edl)
{
	return !edl ? 0 : edl->session->output_h;
}



void Canvas::get_scrollbars(EDL *edl,
	int &canvas_x,
	int &canvas_y,
	int &canvas_w,
	int &canvas_h)
{
	int need_xscroll = 0;
	int need_yscroll = 0;
//	int done = 0;
	float zoom_x, zoom_y, conformed_w, conformed_h;

	if(edl)
	{
		w_needed = edl->session->output_w;
		h_needed = edl->session->output_h;
		w_visible = w_needed;
		h_visible = h_needed;
	}
//printf("Canvas::get_scrollbars 1 %d %d\n", get_xscroll(), get_yscroll());

	if( use_scrollbars ) {
		w_needed = edl->session->output_w;
		h_needed = edl->session->output_h;
		get_zooms(edl, 0, zoom_x, zoom_y, conformed_w, conformed_h);
//printf("Canvas::get_scrollbars 2 %d %d\n", get_xscroll(), get_yscroll());

		w_visible = (int)(canvas_w / zoom_x);
		h_visible = (int)(canvas_h / zoom_y);
		if( w_needed > w_visible ) {
			need_xscroll = 1;
			canvas_h -= BC_ScrollBar::get_span(SCROLL_HORIZ);
		}

		if( h_needed > h_visible ) {
			need_yscroll = 1;
			canvas_w -= BC_ScrollBar::get_span(SCROLL_VERT);
		}
//printf("Canvas::get_scrollbars %d %d %d %d %d %d\n", canvas_w, canvas_h, w_needed, h_needed, w_visible, h_visible);
//printf("Canvas::get_scrollbars 3 %d %d\n", get_xscroll(), get_yscroll());

		w_visible = (int)(canvas_w / zoom_x);
		h_visible = (int)(canvas_h / zoom_y);
	}

	if( need_xscroll ) {
		if( !xscroll ) {
			xscroll = new CanvasXScroll(edl, this, canvas_x, canvas_y + canvas_h,
					w_needed, get_xscroll(), w_visible, canvas_w);
			subwindow->add_subwindow(xscroll);
			xscroll->show_window(0);
		}
		else
			xscroll->reposition_window(canvas_x, canvas_y + canvas_h, canvas_w);

		if( xscroll->get_length() != w_needed ||
		    xscroll->get_handlelength() != w_visible )
			xscroll->update_length(w_needed, get_xscroll(), w_visible, 0);
	}
	else if( xscroll ) {
		delete xscroll;  xscroll = 0;
	}
//printf("Canvas::get_scrollbars 4 %d %d\n", get_xscroll(), get_yscroll());

	if( need_yscroll ) {
		if( !yscroll ) {
			yscroll = new CanvasYScroll(edl, this, canvas_x + canvas_w, canvas_y,
					h_needed, get_yscroll(), h_visible, canvas_h);
			subwindow->add_subwindow(yscroll);
			yscroll->show_window(0);
		}
		else
			yscroll->reposition_window(canvas_x + canvas_w, canvas_y, canvas_h);

		if( yscroll->get_length() != edl->session->output_h ||
		    yscroll->get_handlelength() != h_visible )
			yscroll->update_length(h_needed, get_yscroll(), h_visible, 0);
	}
	else if( yscroll ) {
		delete yscroll;  yscroll = 0;
	}
//printf("Canvas::get_scrollbars 5 %d %d\n", get_xscroll(), get_yscroll());
}


void Canvas::update_geometry(EDL *edl, int x, int y, int w, int h)
{
	int redraw = 0;
	if( this->x != x || this->y != y ||
	    this->w != w || this->h != h ) redraw = 1;
	if( !redraw ) {
		int vx = x, vy = y, vw = w, vh = h;
		get_scrollbars(edl, vx, vy, vw, vh);
		if( vx != view_x || vy != view_y ||
		    vw != view_w || vh != view_h ) redraw = 1;
	}
	if( !redraw ) return;
	reposition_window(edl, x, y, w, h);
}

void Canvas::reposition_window(EDL *edl, int x, int y, int w, int h)
{
	this->x = view_x = x;  this->y = view_y = y;
	this->w = view_w = w;  this->h = view_h = h;
//printf("Canvas::reposition_window 1\n");
	get_scrollbars(edl, view_x, view_y, view_w, view_h);
//printf("Canvas::reposition_window %d %d %d %d\n", view_x, view_y, view_w, view_h);
	if(canvas_subwindow)
	{
		canvas_subwindow->reposition_window(view_x, view_y, view_w, view_h);

// Need to clear out the garbage in the back
		if(canvas_subwindow->get_video_on())
		{
			canvas_subwindow->set_color(BLACK);
			canvas_subwindow->draw_box(0, 0,
				get_canvas()->get_w(), get_canvas()->get_h());
			canvas_subwindow->flash(0);
		}
	}
	refresh(0);
}

// must hold window lock
int Canvas::refresh(int flush)
{
	BC_WindowBase *window = get_canvas();
	if( !window ) return 0;
// relock in lock order to prevent deadlock
	window->unlock_window();
	lock_canvas("Canvas::refresh");
	draw_refresh(flush);
	canvas_lock->unlock();
	return 1;
}
// must not hold locks
int Canvas::redraw(int flush)
{
	lock_canvas("Canvas::redraw");
	draw_refresh(flush);
	unlock_canvas();
	return 1;
}

void Canvas::set_cursor(int cursor)
{
	get_canvas()->set_cursor(cursor, 0, 1);
}

int Canvas::get_cursor_x()
{
	return get_canvas()->get_cursor_x();
}

int Canvas::get_cursor_y()
{
	return get_canvas()->get_cursor_y();
}

int Canvas::get_buttonpress()
{
	return get_canvas()->get_buttonpress();
}


void Canvas::create_objects(EDL *edl)
{
	view_x = x;
	view_y = y;
	view_w = w;
	view_h = h;
	get_scrollbars(edl, view_x, view_y, view_w, view_h);

	subwindow->unlock_window();
	create_canvas();
	subwindow->lock_window("Canvas::create_objects");

	subwindow->add_subwindow(canvas_menu = new CanvasPopup(this));
	canvas_menu->create_objects();

	subwindow->add_subwindow(fullscreen_menu = new CanvasFullScreenPopup(this));
	fullscreen_menu->create_objects();

}

int Canvas::button_press_event()
{
	int result = 0;

	if(get_canvas()->get_buttonpress() == 3)
	{
		if(get_fullscreen())
			fullscreen_menu->activate_menu();
		else
			canvas_menu->activate_menu();
		result = 1;
	}

	return result;
}

void Canvas::start_single()
{
	is_processing = 1;
	status_event();
}

void Canvas::stop_single()
{
	is_processing = 0;
	status_event();
}

void Canvas::start_video()
{
	if(get_canvas())
	{
		get_canvas()->start_video();
		status_event();
	}
}

void Canvas::stop_video()
{
	if(get_canvas())
	{
		get_canvas()->stop_video();
		status_event();
	}
}


void Canvas::start_fullscreen()
{
	set_fullscreen(1);
	create_canvas();
}

void Canvas::stop_fullscreen()
{
	set_fullscreen(0);
	create_canvas();
}

void Canvas::create_canvas()
{
	canvas_lock->lock("Canvas::create_canvas");
	int video_on = 0;
	BC_WindowBase *wdw = 0;
	if( !get_fullscreen() ) {
// Enter windowed
		if( canvas_fullscreen ) {
			canvas_fullscreen->lock_window("Canvas::create_canvas 1");
			video_on = canvas_fullscreen->get_video_on();
			if( video_on ) canvas_fullscreen->stop_video();
			canvas_fullscreen->hide_window();
			canvas_fullscreen->unlock_window();
		}
		if( !canvas_auxwindow && !canvas_subwindow ) {
			subwindow->add_subwindow(canvas_subwindow = new CanvasOutput(this,
				view_x, view_y, view_w, view_h));
		}
		wdw = get_canvas();
		wdw->lock_window("Canvas::create_canvas 2");
	}
	else {
// Enter fullscreen
		wdw = canvas_auxwindow ? canvas_auxwindow : canvas_subwindow;
		wdw->lock_window("Canvas::create_canvas 3");
		video_on = wdw->get_video_on();
		if( video_on ) wdw->stop_video();
		int x, y, w, h;
		wdw->get_fullscreen_geometry(x, y, w, h);
		wdw->unlock_window();
		if( canvas_fullscreen ) {
			if( x != canvas_fullscreen->get_x() ||
			    y != canvas_fullscreen->get_y() ||
			    w != canvas_fullscreen->get_w() ||
			    h != canvas_fullscreen->get_h() ) {
				delete canvas_fullscreen;
				canvas_fullscreen = 0;
			}
		}
		if( !canvas_fullscreen )
			canvas_fullscreen = new CanvasFullScreen(this, w, h);
		wdw = canvas_fullscreen;
		wdw->lock_window("Canvas::create_canvas 4");
		wdw->show_window();
		wdw->sync_display();
		wdw->reposition_window(x, y);
	}

	if( !video_on )
		draw_refresh(1);
	else
		wdw->start_video();
	wdw->focus();
	wdw->unlock_window();
	canvas_lock->unlock();
}



int Canvas::cursor_leave_event_base(BC_WindowBase *caller)
{
	int result = 0;
	if(cursor_inside) result = cursor_leave_event();
	cursor_inside = 0;
	return result;
}

int Canvas::cursor_enter_event_base(BC_WindowBase *caller)
{
	int result = 0;
	if(caller->is_event_win() && caller->cursor_inside())
	{
		cursor_inside = 1;
		result = cursor_enter_event();
	}
	return result;
}

int Canvas::button_press_event_base(BC_WindowBase *caller)
{
	if(caller->is_event_win() && caller->cursor_inside())
	{
		return button_press_event();
	}
	return 0;
}

int Canvas::keypress_event(BC_WindowBase *caller)
{
	int key = caller->get_keypress();
	switch( key ) {
	case 'f':
		caller->unlock_window();
		if(get_fullscreen())
			stop_fullscreen();
		else
			start_fullscreen();
		caller->lock_window("Canvas::keypress_event 1");
		break;
	case ESC:
		caller->unlock_window();
		if(get_fullscreen())
			stop_fullscreen();
		caller->lock_window("Canvas::keypress_event 2");
		break;
	default:
		return 0;
	}
	return 1;
}

void Canvas::update_refresh(VideoDevice *device, VFrame *output_frame)
{
	int best_color_model = output_frame->get_color_model();
	int use_opengl =
		device->out_config->driver == PLAYBACK_X11_GL &&
		output_frame->get_opengl_state() != VFrame::RAM;

// OpenGL does YUV->RGB in the compositing step
	if( use_opengl )
		best_color_model = BC_RGB888;
	else if( BC_CModels::has_alpha(best_color_model) ) {
		best_color_model =
			BC_CModels::is_float(best_color_model ) ?
				BC_RGB_FLOAT :
			BC_CModels::is_yuv(best_color_model ) ?
				( BC_CModels::calculate_pixelsize(best_color_model) > 8 ?
					BC_YUV161616 : BC_YUV888 ) :
				( BC_CModels::calculate_pixelsize(best_color_model) > 8 ?
					BC_RGB161616 : BC_RGB888 ) ;
	}
	int out_w = output_frame->get_w();
	int out_h = output_frame->get_h();
	if( refresh_frame &&
	   (refresh_frame->get_w() != out_w ||
	    refresh_frame->get_h() != out_h ||
	    refresh_frame->get_color_model() != best_color_model ) ) {
// x11 direct render uses BC_BGR8888, use tranfer_from to remap
		delete refresh_frame;  refresh_frame = 0;
	}

	if( !refresh_frame ) {
		refresh_frame =
			new VFrame(out_w, out_h, best_color_model);
	}

	if( use_opengl ) {
		unlock_canvas();
		mwindow->playback_3d->copy_from(this, refresh_frame, output_frame, 0);
		lock_canvas("Canvas::update_refresh");
	}
	else
		refresh_frame->transfer_from(output_frame, -1);
}

void Canvas::clear(int flash)
{
	BC_WindowBase *cwdw = get_canvas();
	if( !cwdw )  return;
	cwdw->set_bg_color(BLACK);
	cwdw->clear_box(0,0, cwdw->get_w(), cwdw->get_h());
	if( flash ) cwdw->flash();
}


CanvasOutput::CanvasOutput(Canvas *canvas,
    int x,
    int y,
    int w,
    int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->canvas = canvas;
}

CanvasOutput::~CanvasOutput()
{
}

int CanvasOutput::cursor_leave_event()
{
	return canvas->cursor_leave_event_base(canvas->get_canvas());
}

int CanvasOutput::cursor_enter_event()
{
	return canvas->cursor_enter_event_base(canvas->get_canvas());
}

int CanvasOutput::button_press_event()
{
	return canvas->button_press_event_base(canvas->get_canvas());
}

int CanvasOutput::button_release_event()
{
	return canvas->button_release_event();
}

int CanvasOutput::cursor_motion_event()
{
	return canvas->cursor_motion_event();
}

int CanvasOutput::keypress_event()
{
	return canvas->keypress_event(canvas->get_canvas());
}



CanvasFullScreen::CanvasFullScreen(Canvas *canvas, int w, int h)
 : BC_FullScreen(canvas->subwindow, w, h, BLACK, 0, 0, 0)
{
	this->canvas = canvas;
}

CanvasFullScreen::~CanvasFullScreen()
{
}


CanvasXScroll::CanvasXScroll(EDL *edl, Canvas *canvas, int x, int y,
	int length, int position, int handle_length, int pixels)
 : BC_ScrollBar(x, y, SCROLL_HORIZ, pixels, length, position, handle_length)
{
	this->canvas = canvas;
}

CanvasXScroll::~CanvasXScroll()
{
}

int CanvasXScroll::handle_event()
{
	canvas->update_zoom(get_value(), canvas->get_yscroll(), canvas->get_zoom());
	return canvas->refresh(1);
}


CanvasYScroll::CanvasYScroll(EDL *edl, Canvas *canvas, int x, int y,
	int length, int position, int handle_length, int pixels)
 : BC_ScrollBar(x, y, SCROLL_VERT, pixels, length, position, handle_length)
{
	this->canvas = canvas;
}

CanvasYScroll::~CanvasYScroll()
{
}

int CanvasYScroll::handle_event()
{
	canvas->update_zoom(canvas->get_xscroll(), get_value(), canvas->get_zoom());
	return canvas->refresh(1);
}


CanvasFullScreenPopup::CanvasFullScreenPopup(Canvas *canvas)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->canvas = canvas;
}


void CanvasFullScreenPopup::create_objects()
{
	add_item(new CanvasSubWindowItem(canvas));
}

void CanvasFullScreenPopup::use_cwindow()
{
	add_item(new CanvasPopupAuto(canvas));
}

CanvasSubWindowItem::CanvasSubWindowItem(Canvas *canvas)
 : BC_MenuItem(_("Windowed"), "f", 'f')
{
	this->canvas = canvas;
}

int CanvasSubWindowItem::handle_event()
{
// It isn't a problem to delete the canvas from in here because the event
// dispatcher is the canvas subwindow.
	canvas->subwindow->unlock_window();
	canvas->stop_fullscreen();
	canvas->subwindow->lock_window("CanvasSubWindowItem::handle_event");
	return 1;
}


CanvasPopup::CanvasPopup(Canvas *canvas)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->canvas = canvas;
}

CanvasPopup::~CanvasPopup()
{
}

CanvasZoomSize::CanvasZoomSize(Canvas *canvas)
 : BC_MenuItem(_("Zoom..."))
{
	this->canvas = canvas;
}

CanvasSizeSubMenu::CanvasSizeSubMenu(CanvasZoomSize *zoom_size)
{
	this->zoom_size = zoom_size;
}

void CanvasPopup::create_objects()
{
	add_item(new BC_MenuItem("-"));
	add_item(new CanvasFullScreenItem(canvas));

	CanvasZoomSize *zoom_size = new CanvasZoomSize(canvas);
	add_item(zoom_size);
	CanvasSizeSubMenu *submenu = new CanvasSizeSubMenu(zoom_size);
	zoom_size->add_submenu(submenu);

	submenu->add_submenuitem(new CanvasPopupSize(canvas, _("Zoom 25%"), 0.25));
	submenu->add_submenuitem(new CanvasPopupSize(canvas, _("Zoom 33%"), 0.33));
	submenu->add_submenuitem(new CanvasPopupSize(canvas, _("Zoom 50%"), 0.5));
	submenu->add_submenuitem(new CanvasPopupSize(canvas, _("Zoom 75%"), 0.75));
	submenu->add_submenuitem(new CanvasPopupSize(canvas, _("Zoom 100%"), 1.0));
	submenu->add_submenuitem(new CanvasPopupSize(canvas, _("Zoom 150%"), 1.5));
	submenu->add_submenuitem(new CanvasPopupSize(canvas, _("Zoom 200%"), 2.0));
	submenu->add_submenuitem(new CanvasPopupSize(canvas, _("Zoom 300%"), 3.0));
	submenu->add_submenuitem(new CanvasPopupSize(canvas, _("Zoom 400%"), 4.0));
}

void CanvasPopup::use_cwindow()
{
	add_item(new CanvasPopupAuto(canvas));
	add_item(new CanvasPopupResetCamera(canvas));
	add_item(new CanvasPopupResetProjector(canvas));
	add_item(new CanvasPopupCameraKeyframe(canvas));
	add_item(new CanvasPopupProjectorKeyframe(canvas));
	add_item(toggle_controls = new CanvasToggleControls(canvas));
}

void CanvasPopup::use_rwindow()
{
	add_item(new CanvasPopupResetTranslation(canvas));
}

void CanvasPopup::use_vwindow()
{
	add_item(new CanvasPopupRemoveSource(canvas));
}


CanvasPopupAuto::CanvasPopupAuto(Canvas *canvas)
 : BC_MenuItem(_("Zoom Auto"))
{
	this->canvas = canvas;
}

int CanvasPopupAuto::handle_event()
{
	canvas->zoom_auto();
	return 1;
}


CanvasPopupSize::CanvasPopupSize(Canvas *canvas, char *text, float percentage)
 : BC_MenuItem(text)
{
	this->canvas = canvas;
	this->percentage = percentage;
}
CanvasPopupSize::~CanvasPopupSize()
{
}
int CanvasPopupSize::handle_event()
{
	canvas->zoom_resize_window(percentage);
	return 1;
}


CanvasPopupResetCamera::CanvasPopupResetCamera(Canvas *canvas)
 : BC_MenuItem(_("Reset camera"), _("F11"), KEY_F11)
{
	this->canvas = canvas;
}
int CanvasPopupResetCamera::handle_event()
{
	canvas->reset_camera();
	return 1;
}

CanvasPopupResetProjector::CanvasPopupResetProjector(Canvas *canvas)
 : BC_MenuItem(_("Reset projector"), _("F12"), KEY_F12)
{
	this->canvas = canvas;
}
int CanvasPopupResetProjector::handle_event()
{
	canvas->reset_projector();
	return 1;
}


CanvasPopupCameraKeyframe::CanvasPopupCameraKeyframe(Canvas *canvas)
 : BC_MenuItem(_("Camera keyframe"), _("Shift-F11"), KEY_F11)
{
	this->canvas = canvas;
	set_shift(1);
}
int CanvasPopupCameraKeyframe::handle_event()
{
	canvas->camera_keyframe();
	return 1;
}

CanvasPopupProjectorKeyframe::CanvasPopupProjectorKeyframe(Canvas *canvas)
 : BC_MenuItem(_("Projector keyframe"), _("Shift-F12"), KEY_F12)
{
	this->canvas = canvas;
	set_shift(1);
}
int CanvasPopupProjectorKeyframe::handle_event()
{
	canvas->projector_keyframe();
	return 1;
}



CanvasPopupResetTranslation::CanvasPopupResetTranslation(Canvas *canvas)
 : BC_MenuItem(_("Reset translation"))
{
	this->canvas = canvas;
}
int CanvasPopupResetTranslation::handle_event()
{
	canvas->reset_translation();
	return 1;
}



CanvasToggleControls::CanvasToggleControls(Canvas *canvas)
 : BC_MenuItem(calculate_text(canvas->get_cwindow_controls()))
{
	this->canvas = canvas;
}
int CanvasToggleControls::handle_event()
{
	canvas->toggle_controls();
	set_text(calculate_text(canvas->get_cwindow_controls()));
	return 1;
}

char* CanvasToggleControls::calculate_text(int cwindow_controls)
{
	if(!cwindow_controls)
		return _("Show controls");
	else
		return _("Hide controls");
}







CanvasFullScreenItem::CanvasFullScreenItem(Canvas *canvas)
 : BC_MenuItem(_("Fullscreen"), "f", 'f')
{
	this->canvas = canvas;
}
int CanvasFullScreenItem::handle_event()
{
	canvas->subwindow->unlock_window();
	canvas->start_fullscreen();
	canvas->subwindow->lock_window("CanvasFullScreenItem::handle_event");
	return 1;
}









CanvasPopupRemoveSource::CanvasPopupRemoveSource(Canvas *canvas)
 : BC_MenuItem(_("Close source"))
{
	this->canvas = canvas;
}
int CanvasPopupRemoveSource::handle_event()
{
	canvas->close_source();
	return 1;
}


