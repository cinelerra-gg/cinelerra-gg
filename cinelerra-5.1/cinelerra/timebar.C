
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

#include "awindow.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "clip.h"
#include "cplayback.h"
#include "cursors.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "fonts.h"
#include "labels.h"
#include "labeledit.h"
#include "localsession.h"
#include "maincursor.h"
#include "mainundo.h"
#include "mbuttons.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "preferences.h"
#include "recordlabel.h"
#include "localsession.h"
#include "mainsession.h"
#include "theme.h"
#include "timebar.h"
#include "timelinepane.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "units.h"
#include "vframe.h"
#include "vwindow.h"
#include "vwindowgui.h"
#include "zoombar.h"


LabelGUI::LabelGUI(MWindow *mwindow, TimeBar *timebar,
	int64_t pixel, int y,
	double position, VFrame **data)
 : BC_Toggle(translate_pixel(mwindow, pixel), y,
		data ? data : mwindow->theme->label_toggle, 0)
{
	this->mwindow = mwindow;
	this->timebar = timebar;
	this->gui = 0;
	this->pixel = pixel;
	this->position = position;
	this->label = 0;
}

LabelGUI::~LabelGUI()
{
	if( timebar->drag_label == this )
		timebar->drag_label = 0;
}

int LabelGUI::get_y(MWindow *mwindow, TimeBar *timebar)
{
	return timebar->get_h() -
		mwindow->theme->label_toggle[0]->get_h();
}

int LabelGUI::translate_pixel(MWindow *mwindow, int pixel)
{
	int result = pixel - mwindow->theme->label_toggle[0]->get_w() / 2;
	return result;
}

void LabelGUI::reposition(int flush)
{
	reposition_window(translate_pixel(mwindow, pixel),
		BC_Toggle::get_y());
}

int LabelGUI::button_press_event()
{
	int result = test_drag_label(1);

	if( this->is_event_win() && get_buttonpress() == 3 ) {
		if( label ) {
			int cur_x, cur_y;
			get_abs_cursor(cur_x, cur_y, 0);
			timebar->label_edit->start(label, cur_x, cur_y);
		}
		result = 1;
	} else {
		result = BC_Toggle::button_press_event();
	}
	if( label )
		set_tooltip(this->label->textstr);
	return result;
}

int LabelGUI::button_release_event()
{
	int ret = BC_Toggle::button_release_event();
	test_drag_label(0);
	return ret;
}

int LabelGUI::test_drag_label(int press)
{
	if( is_event_win() && get_buttonpress() == 1 ) {
		switch( timebar->current_operation ) {
		case TIMEBAR_NONE:
			if( press && get_value() ) {
				timebar->current_operation = TIMEBAR_DRAG_LABEL;
				timebar->drag_label = this;
				set_cursor(HSEPARATE_CURSOR, 0, 0);
				mwindow->undo->update_undo_before(_("drag label"), this);
				return 1;
			}
			break;
		case TIMEBAR_DRAG_LABEL:
			if( !press ) {
				timebar->current_operation = TIMEBAR_NONE;
				timebar->drag_label = 0;
				set_cursor(ARROW_CURSOR, 0, 0);
				mwindow->undo->update_undo_after(_("drag label"), LOAD_TIMEBAR);
				mwindow->awindow->gui->async_update_assets(); // labels folder
			}
			break;
		}
	}
	return 0;
}

int LabelGUI::handle_event()
{
	timebar->select_label(position);
	return 1;
}

void LabelGUI::update_value()
{
	EDL *edl = timebar->get_edl();
	double start = edl->local_session->get_selectionstart(1);
	double end = edl->local_session->get_selectionend(1);
	int v = ( label->position >= start && end >= label->position ) ||
	    edl->equivalent(label->position, start) ||
	    edl->equivalent(label->position, end) ||
	    timebar->drag_label == this ? 1 : 0;
	update(v);
}


InPointGUI::InPointGUI(MWindow *mwindow, TimeBar *timebar,
	int64_t pixel, double position)
 : LabelGUI(mwindow, timebar,
	pixel, get_y(mwindow, timebar),
	position, mwindow->theme->in_point)
{
//printf("InPointGUI::InPointGUI %d %d\n", pixel, get_y(mwindow, timebar));
}
InPointGUI::~InPointGUI()
{
}
int InPointGUI::get_y(MWindow *mwindow, TimeBar *timebar)
{
	int result;
	result = timebar->get_h() -
		mwindow->theme->in_point[0]->get_h();
	return result;
}


OutPointGUI::OutPointGUI(MWindow *mwindow, TimeBar *timebar,
	int64_t pixel, double position)
 : LabelGUI(mwindow, timebar,
	pixel, get_y(mwindow, timebar),
	position, mwindow->theme->out_point)
{
//printf("OutPointGUI::OutPointGUI %d %d\n", pixel, get_y(mwindow, timebar));
}
OutPointGUI::~OutPointGUI()
{
}
int OutPointGUI::get_y(MWindow *mwindow, TimeBar *timebar)
{
	return timebar->get_h() -
		mwindow->theme->out_point[0]->get_h();
}


PresentationGUI::PresentationGUI(MWindow *mwindow, TimeBar *timebar,
	int64_t pixel, double position)
 : LabelGUI(mwindow, timebar, pixel, get_y(mwindow, timebar), position)
{
}
PresentationGUI::~PresentationGUI()
{
}

TimeBar::TimeBar(MWindow *mwindow, BC_WindowBase *gui,
	int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h)
{
//printf("TimeBar::TimeBar %d %d %d %d\n", x, y, w, h);
	this->gui = gui;
	this->mwindow = mwindow;
	this->drag_label = 0;
	label_edit = new LabelEdit(mwindow, mwindow->awindow, 0);
	pane = 0;
	highlighted = 0;
}

TimeBar::~TimeBar()
{
	delete in_point;
	delete out_point;
	delete label_edit;
	labels.remove_all_objects();
	presentations.remove_all_objects();
}

void TimeBar::create_objects()
{
	in_point = 0;
	out_point = 0;
//printf("TimeBar::create_objects %d\n", __LINE__);
	current_operation = TIMEBAR_NONE;
	set_cursor(UPRIGHT_ARROW_CURSOR, 0, 0);
	update(0);
}


int64_t TimeBar::position_to_pixel(double position)
{
	get_edl_length();
	return (int64_t)(position / time_per_pixel);
}


double TimeBar::pixel_to_position(int pixel)
{
	if( pane ) {
		pixel += get_edl()->local_session->view_start[pane->number];
	}

	return (double)pixel *
		get_edl()->local_session->zoom_sample /
		get_edl()->session->sample_rate;
}

void TimeBar::update_labels()
{
	int output = 0;
	EDL *edl = get_edl();

	if( edl ) {
		for( Label *current=edl->labels->first; current; current=NEXT ) {
			int64_t pixel = position_to_pixel(current->position);
			if( pixel >= 0 && pixel < get_w()  ) {
// Create new label
				if( output >= labels.total ) {
					LabelGUI *new_label;
					add_subwindow(new_label =
						new LabelGUI(mwindow,
							this,
							pixel,
							LabelGUI::get_y(mwindow, this),
							current->position));
					new_label->set_cursor(INHERIT_CURSOR, 0, 0);
					new_label->set_tooltip(current->textstr);
					new_label->label = current;
					labels.append(new_label);
				}
				else
// Reposition old label
				{
					LabelGUI *gui = labels.values[output];
					if( gui->pixel != pixel ) {
						gui->pixel = pixel;
						gui->reposition(0);
					}
//					else {
//						gui->draw_face(1,0);
//					}

					labels.values[output]->position = current->position;
					labels.values[output]->set_tooltip(current->textstr);
					labels.values[output]->label = current;
				}

				labels.values[output++]->update_value();
			}
		}
	}

// Delete excess labels
	while(labels.total > output)
	{
		labels.remove_object();
	}

// Get the labels to show
	show_window(0);
}

void TimeBar::update_highlights()
{
	EDL *edl = get_edl();
	if( !edl ) return;
	for( int i = 0; i < labels.total; i++ ) {
		labels.values[i]->update_value();
	}

	if( edl->equivalent(edl->local_session->get_inpoint(),
			edl->local_session->get_selectionstart(1)) ||
		edl->equivalent(edl->local_session->get_inpoint(),
			edl->local_session->get_selectionend(1)) ) {
		if( in_point ) in_point->update(1);
	}
	else
		if( in_point ) in_point->update(0);

	if( edl->equivalent(edl->local_session->get_outpoint(),
			edl->local_session->get_selectionstart(1)) ||
		edl->equivalent(edl->local_session->get_outpoint(),
			edl->local_session->get_selectionend(1)) ) {
		if( out_point ) out_point->update(1);
	}
	else
		if( out_point ) out_point->update(0);

	draw_inout_highlight();
}

void TimeBar::draw_inout_highlight()
{
	EDL *edl = get_edl();
	if( !edl->local_session->inpoint_valid() ) return;
	if( !edl->local_session->outpoint_valid() ) return;
	double in_position = edl->local_session->get_inpoint();
	double out_position = edl->local_session->get_outpoint();
	if( in_position >= out_position ) return;
	int in_x = position_to_pixel(in_position);
	int out_x = position_to_pixel(out_position);
	CLAMP(in_x, 0, get_w());
	CLAMP(out_x, 0, get_w());
	set_color(mwindow->theme->inout_highlight_color);
	int lw = 5;
	set_line_width(lw);
	set_inverse();
	draw_line(in_x, get_h()-2*lw, out_x, get_h()-2*lw);
	set_opaque();
	set_line_width(1);
}

void TimeBar::update_points()
{
	EDL *edl = get_edl();
	int64_t pixel = !edl ? 0 :
		position_to_pixel(edl->local_session->get_inpoint());

	if( in_point ) {
		if( edl && edl->local_session->inpoint_valid() &&
		    pixel >= 0 && pixel < get_w() ) {
			if( !EQUIV(edl->local_session->get_inpoint(), in_point->position) ||
			    in_point->pixel != pixel ) {
				in_point->pixel = pixel;
				in_point->position = edl->local_session->get_inpoint();
				in_point->reposition(0);
			}
			else {
				in_point->draw_face(1, 0);
			}
		}
		else {
			delete in_point;
			in_point = 0;
		}
	}
	else
	if( edl && edl->local_session->inpoint_valid() &&
	    pixel >= 0 && pixel < get_w() ) {
		add_subwindow(in_point = new InPointGUI(mwindow,
			this, pixel, edl->local_session->get_inpoint()));
		in_point->set_cursor(ARROW_CURSOR, 0, 0);
	}

	pixel = !edl ? 0 :
		 position_to_pixel(edl->local_session->get_outpoint());

	if( out_point ) {
		if( edl && edl->local_session->outpoint_valid() &&
		    pixel >= 0 && pixel < get_w()) {
			if( !EQUIV(edl->local_session->get_outpoint(), out_point->position) ||
			    out_point->pixel != pixel ) {
				out_point->pixel = pixel;
				out_point->position = edl->local_session->get_outpoint();
				out_point->reposition(0);
			}
			else {
				out_point->draw_face(1, 0);
			}
		}
		else {
			delete out_point;
			out_point = 0;
		}
	}
	else
	if( edl && edl->local_session->outpoint_valid() &&
	    pixel >= 0 && pixel < get_w() ) {
		add_subwindow(out_point = new OutPointGUI(mwindow,
			this, pixel, edl->local_session->get_outpoint()));
		out_point->set_cursor(ARROW_CURSOR, 0, 0);
	}

//	flush();
}

void TimeBar::update_clock(double position)
{
}

void TimeBar::update(int flush)
{
	draw_time();
// Need to redo these when range is drawn to get the background updated.
	update_labels();
	update_points();


 	EDL *edl = get_edl();
	int64_t pixel = -1;
	int x = get_relative_cursor_x();
// Draw highlight position
	if( edl && (highlighted || current_operation == TIMEBAR_DRAG) &&
	    x >= 0 && x < get_w() ) {
//printf("TimeBar::update %d %d\n", __LINE__, x);
		double position = pixel_to_position(x);

		position = mwindow->edl->align_to_frame(position, 0);
		pixel = position_to_pixel(position);
		update_clock(position);
	}

	if( pixel < 0 ) {
		double position = test_highlight();
		if( position >= 0 ) pixel = position_to_pixel(position);
	}


	if( pixel >= 0 && pixel < get_w() ) {
		set_color(mwindow->theme->timebar_cursor_color);
		set_line_dashes(1);
//printf("TimeBar::update %d pane=%d pixel=%jd\n", __LINE__, pane->number, pixel);
		draw_line(pixel, 0, pixel, get_h());
		set_line_dashes(0);
	}


 	if( edl ) {
		double playback_start = edl->local_session->playback_start;
		if( playback_start >= 0 ) {
	 		int64_t pixel = position_to_pixel(playback_start);
 			set_color(mwindow->theme->timebar_cursor_color ^ 0x0000ff);
	 		draw_line(pixel, 0, pixel, get_h());
			double playback_end = edl->local_session->playback_end;
			if( playback_end > playback_start ) {
		 		pixel = position_to_pixel(playback_end);
 				set_color(mwindow->theme->timebar_cursor_color ^ 0x00ff00);
		 		draw_line(pixel, 0, pixel, get_h());
			}
		}

		double position = edl->local_session->get_selectionstart(1);
 		int64_t pixel = position_to_pixel(position);
// Draw insertion point position.
		int color = mwindow->theme->timebar_cursor_color;
		if( mwindow->preferences->forward_render_displacement )
			color ^= 0x00ffff;
		set_color(color);
 		draw_line(pixel, 0, pixel, get_h());
 	}

	update_highlights();

// Get the labels to show
	show_window(0);
	flash(flush);
//printf("TimeBar::update %d this=%p %d\n", __LINE__, this, current_operation);
}



int TimeBar::delete_project()
{
//	labels->delete_all();
	return 0;
}

int TimeBar::save(FileXML *xml)
{
//	labels->save(xml);
	return 0;
}




void TimeBar::draw_time()
{
}

EDL* TimeBar::get_edl()
{
	return mwindow->edl;
}



void TimeBar::draw_range()
{


//printf("TimeBar::draw_range %d %p\n", __LINE__, get_edl());
 	if( has_preview() && get_edl() ) {
 		int x1, x2;
		get_preview_pixels(x1, x2);

//printf("TimeBar::draw_range %f %d %d\n", edl_length, x1, x2);
		draw_3segmenth(0, 0, x1, mwindow->theme->timebar_view_data);
 		draw_top_background(get_parent(), x1, 0, x2 - x1, get_h());
		draw_3segmenth(x2, 0, get_w() - x2, mwindow->theme->timebar_view_data);

		set_color(BLACK);
		draw_line(x1, 0, x1, get_h());
		draw_line(x2, 0, x2, get_h());


 		EDL *edl = get_edl();
 		if( edl ) {
 			int64_t pixel = position_to_pixel(
 				edl->local_session->get_selectionstart(1));
// Draw insertion point position if this timebar belongs to a window which
// has something other than the master EDL.
 			set_color(mwindow->theme->timebar_cursor_color);
 			draw_line(pixel, 0, pixel, get_h());
 		}
 	}
 	else
		draw_top_background(get_parent(), 0, 0, get_w(), get_h());
}

void TimeBar::select_label(double position)
{
}



int TimeBar::draw()
{
	return 0;
}

double TimeBar::get_edl_length()
{
	edl_length = get_edl() ? get_edl()->tracks->total_length() : 0;
	int w1 = get_w()-1;
	time_per_pixel = !EQUIV(edl_length, 0) ? edl_length/w1 : w1;
	return edl_length;
}

int TimeBar::get_preview_pixels(int &x1, int &x2)
{
	x1 = 0;  x2 = get_w();
	get_edl_length();
	EDL *edl = get_edl();
	if( edl && !EQUIV(edl_length, 0) ) {
		double preview_start = edl->local_session->preview_start;
		double preview_end = edl->local_session->preview_end;
		if( preview_end < 0 || preview_end > edl_length )
			preview_end = edl_length;
		if( preview_end >= preview_start ) {
			x1 = (int)(preview_start / time_per_pixel);
			x2 = (int)(preview_end / time_per_pixel);
		}
	}
	return 0;
}


int TimeBar::test_preview(int buttonpress)
{
	int result = 0;


	if( get_edl() && cursor_inside() && buttonpress >= 0 ) {
		int x1, x2, x = get_relative_cursor_x();
		get_preview_pixels(x1, x2);
//printf("TimeBar::test_preview %d %d %d\n", x1, x2, x);
// Inside left handle
		if( x >= x1 - HANDLE_W && x < x1 + HANDLE_W &&
// Ignore left handle if both handles are up against the left side
		    x2 > HANDLE_W ) {
			if( buttonpress ) {
				current_operation = TIMEBAR_DRAG_LEFT;
				start_position = get_edl()->local_session->preview_start;
				start_cursor_x = x;
			}
			else if( get_cursor() != LEFT_CURSOR )
				set_cursor(LEFT_CURSOR, 0, 1);
			result = 1;
		}
// Inside right handle
		else if( x >= x2 - HANDLE_W && x < x2 + HANDLE_W &&
// Ignore right handle if both handles are up against the right side
		    x1 < get_w() - HANDLE_W ) {
			if( buttonpress ) {
				current_operation = TIMEBAR_DRAG_RIGHT;
				start_position = get_edl()->local_session->preview_end;
				if( start_position < 0 || start_position > edl_length )
					start_position = edl_length;
				start_cursor_x = x;
			}
			else if( get_cursor() != RIGHT_CURSOR )
				set_cursor(RIGHT_CURSOR, 0, 1);
			result = 1;
		}
// Inside preview
		else if( get_button_down() && get_buttonpress() == 3 &&
		    x >= x1 && x < x2 ) {
			if( buttonpress ) {
				current_operation = TIMEBAR_DRAG_CENTER;
				starting_start_position = get_edl()->local_session->preview_start;
				starting_end_position = get_edl()->local_session->preview_end;
				if( starting_end_position < 0 || starting_end_position > edl_length )
					starting_end_position = edl_length;
				start_cursor_x = x;
			}
			if( get_cursor() != HSEPARATE_CURSOR )
				set_cursor(HSEPARATE_CURSOR, 0, 1);
			result = 1;
		}
	}

	if( !result && get_cursor() != ARROW_CURSOR )
		set_cursor(ARROW_CURSOR, 0, 1);


	return result;
}

int TimeBar::move_preview(int &redraw)
{
	int result = 0, x = get_relative_cursor_x();
	switch( current_operation ) {
	case TIMEBAR_DRAG_LEFT: {
		get_edl()->local_session->preview_start =
			start_position + time_per_pixel * (x - start_cursor_x);
		double preview_end = get_edl()->local_session->preview_end;
		if( preview_end < 0 || preview_end > edl_length )
			preview_end = get_edl()->local_session->preview_end = edl_length;
		CLAMP(get_edl()->local_session->preview_start, 0, preview_end);
		result = 1;
		break; }
	case TIMEBAR_DRAG_RIGHT: {
		double preview_end = get_edl()->local_session->preview_end =
			start_position + time_per_pixel * (x - start_cursor_x);
		double preview_start = get_edl()->local_session->preview_start;
		if( preview_end >= edl_length && !preview_start ) {
			get_edl()->local_session->preview_end = -1;
			if( preview_start > preview_end )
				preview_start = get_edl()->local_session->preview_start = preview_end;
		}
		else
			CLAMP(get_edl()->local_session->preview_end, preview_start, edl_length);
		result = 1;
		break; }
	case TIMEBAR_DRAG_CENTER: {
		double dt = time_per_pixel * (x - start_cursor_x);
		get_edl()->local_session->preview_start = starting_start_position + dt;
		get_edl()->local_session->preview_end = starting_end_position + dt;
		if( get_edl()->local_session->preview_start < 0 ) {
			get_edl()->local_session->preview_end -= get_edl()->local_session->preview_start;
			get_edl()->local_session->preview_start = 0;
		}
		else
		if( get_edl()->local_session->preview_end > edl_length ) {
			get_edl()->local_session->preview_start -= get_edl()->local_session->preview_end - edl_length;
			get_edl()->local_session->preview_end = edl_length;
		}
		result = 1;
		break; }
	}

//printf("TimeBar::move_preview %d %d\n", __LINE__, current_operation);

	if( result ) {
		update_preview();
		redraw = 1;
	}
//printf("TimeBar::move_preview %d %d\n", __LINE__, current_operation);

	return result;
}

void TimeBar::update_preview()
{
}

int TimeBar::samplemovement()
{
	return 0;
}

void TimeBar::stop_playback()
{
}

int TimeBar::button_press_event()
{
	int result = 0;
	if( is_event_win() && cursor_above() ) {
		if( has_preview() && get_buttonpress() == 3 ) {
			result = test_preview(1);
		}
// Change time format
		else if( !is_vwindow() && ctrl_down() ) {
			if( get_buttonpress() == 1 )
				mwindow->next_time_format();
			else
			if( get_buttonpress() == 2 )
				mwindow->prev_time_format();
			result = 1;
		}
		else if( get_buttonpress() == 1 ) {
			stop_playback();

// Select region between two labels
			if( !is_vwindow() && get_double_click() ) {
				int x = get_relative_cursor_x();
				double position = pixel_to_position(x);
// Test labels
				select_region(position);
			}
			else {

// Reposition highlight cursor
				update_cursor();
				current_operation = TIMEBAR_DRAG;
				activate_timeline();
			}
			result = 1;
		}
	}
	return result;
}

void TimeBar::activate_timeline()
{
	mwindow->gui->activate_timeline();
}

int TimeBar::cursor_motion_event()
{
	int result = 0;
	int redraw = 0;

	switch( current_operation ) {
	case TIMEBAR_DRAG_LEFT:
	case TIMEBAR_DRAG_RIGHT:
	case TIMEBAR_DRAG_CENTER:
		if( has_preview() )
			result = move_preview(redraw);
		break;

	case TIMEBAR_DRAG_LABEL:
		if( drag_label ) {
			EDL *edl = get_edl();
			int pixel = get_relative_cursor_x();
			double position = pixel_to_position(pixel);
			if( drag_label->label )
				drag_label->label->position = position;
			else if( drag_label == in_point ) {
				if( out_point && edl->local_session->outpoint_valid() ) {
					double out_pos = edl->local_session->get_outpoint();
					if( position > out_pos ) {
						edl->local_session->set_outpoint(position);
						drag_label = out_point;
						position = out_pos;
					}
				}
				edl->local_session->set_inpoint(position);
			}
			else if( drag_label == out_point ) {
				if( in_point && edl->local_session->inpoint_valid() ) {
					double in_pos = edl->local_session->get_inpoint();
					if( position < in_pos ) {
						edl->local_session->set_inpoint(position);
						drag_label = in_point;
						position = in_pos;
					}
				}
				edl->local_session->set_outpoint(position);
			}
		}
	// fall thru
	case TIMEBAR_DRAG:
		update_cursor();
		handle_mwindow_drag();
		result = 1;
		break;

	default:
		if( has_preview() )
			result = test_preview(0);
		break;
	}


	if( redraw ) {
		update(1);
	}

	return result;
}

int TimeBar::cursor_leave_event()
{
	if( highlighted ) {
		highlighted = 0;
		update(1);
	}
	return 0;
}

int TimeBar::button_release_event()
{
//printf("TimeBar::button_release_event %d %d\n", __LINE__, current_operation);
	int result = 0;
	int need_redraw = 0;
	switch( current_operation )
	{
		case TIMEBAR_DRAG:
			mwindow->gui->get_focused_pane()->canvas->stop_dragscroll();
			current_operation = TIMEBAR_NONE;
			need_redraw = 1;
			result = 1;
			break;

		default:
			if( current_operation != TIMEBAR_NONE ) {
				current_operation = TIMEBAR_NONE;
				result = 1;
			}
			break;
	}

	if( (!cursor_above() && highlighted) || need_redraw ) {
		highlighted = 0;
		update(1);
	}

	return result;
}

// Update the selection cursor during a dragging operation
void TimeBar::update_cursor()
{
}

void TimeBar::handle_mwindow_drag()
{
}

int TimeBar::select_region(double position)
{
	Label *start = 0, *end = 0, *current;
	for( current = get_edl()->labels->first; current; current = NEXT ) {
		if( current->position > position ) {
			end = current;
			break;
		}
	}

	for( current = get_edl()->labels->last ; current; current = PREVIOUS ) {
		if( current->position <= position ) {
			start = current;
			break;
		}
	}

// Select region
	if( end != start ) {
		if( !start )
			get_edl()->local_session->set_selectionstart(0);
		else
			get_edl()->local_session->set_selectionstart(start->position);

		if( !end )
			get_edl()->local_session->set_selectionend(get_edl()->tracks->total_length());
		else
			get_edl()->local_session->set_selectionend(end->position);
	}
	else
	if( end || start ) {
		get_edl()->local_session->set_selectionstart(start->position);
		get_edl()->local_session->set_selectionend(start->position);
	}

// Que the CWindow
	mwindow->cwindow->gui->lock_window("TimeBar::select_region");
	mwindow->cwindow->update(1, 0, 0);
	mwindow->cwindow->gui->unlock_window();
	mwindow->gui->lock_window("TimeBar::select_region");
	mwindow->gui->hide_cursor(0);
	mwindow->gui->draw_cursor(1);
	mwindow->gui->flash_canvas(0);
	mwindow->gui->activate_timeline();
	mwindow->gui->zoombar->update();
	mwindow->gui->unlock_window();
	update_highlights();
	return 0;
}




int TimeBar::delete_arrows()
{
	return 0;
}

double TimeBar::test_highlight()
{
	return -1;
}


