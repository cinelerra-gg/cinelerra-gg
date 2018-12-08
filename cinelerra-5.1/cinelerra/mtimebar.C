
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

#include "bcsignals.h"
#include "clip.h"
#include "cplayback.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mainclock.h"
#include "maincursor.h"
#include "mainsession.h"
#include "mbuttons.h"
#include "mtimebar.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "patchbay.h"
#include "preferences.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "zoombar.h"



MTimeBar::MTimeBar(MWindow *mwindow,
	MWindowGUI *gui,
	int x,
	int y,
	int w,
	int h)
 : TimeBar(mwindow, gui, x, y, w, h)
{
	this->gui = gui;
	this->pane = 0;
}

MTimeBar::MTimeBar(MWindow *mwindow,
	TimelinePane *pane,
	int x,
	int y,
	int w,
	int h)
 : TimeBar(mwindow, mwindow->gui, x, y, w, h)
{
	this->gui = mwindow->gui;
	this->pane = pane;
}

void MTimeBar::create_objects()
{
	gui->add_subwindow(menu = new TimeBarPopup(mwindow));
	menu->create_objects();

	TimeBar::create_objects();
}


double MTimeBar::pixel_to_position(int pixel)
{
	return (double)pixel *
		mwindow->edl->local_session->zoom_sample /
		mwindow->edl->session->sample_rate +
		(double)mwindow->edl->local_session->view_start[pane->number] *
		mwindow->edl->local_session->zoom_sample /
		mwindow->edl->session->sample_rate;
}


int64_t MTimeBar::position_to_pixel(double position)
{
	return (int64_t)(position *
		mwindow->edl->session->sample_rate /
		mwindow->edl->local_session->zoom_sample -
		mwindow->edl->local_session->view_start[pane->number]);
}


void MTimeBar::stop_transport()
{
	gui->stop_transport("MTimeBar::stop_transport");
}

#define TEXT_MARGIN 4
#define TICK_SPACING 5
#define LINE_MARGIN 3
#define TICK_MARGIN 16
void MTimeBar::draw_time()
{

	char string[BCTEXTLEN];
	int sample_rate = mwindow->edl->session->sample_rate;
	double frame_rate = mwindow->edl->session->frame_rate;
// Seconds between text markings
	double text_interval = 3600.0;
// Seconds between tick marks
	double tick_interval = 3600.0;


// Calculate tick mark spacing, number spacing, and starting point based
// on zoom, time format, project settings.

// If the time format is for audio, mark round numbers of samples based on
// samplerate.
// Fow low zoom, mark tens of samples.

// If the time format is for video, mark round number of frames based on
// framerate.
// For low zoom, mark individual frames.

	//int64_t windowspan = mwindow->edl->local_session->zoom_sample * get_w();

	draw_range();





// Number of seconds per pixel
	double time_per_pixel = (double)mwindow->edl->local_session->zoom_sample /
		sample_rate;
// Seconds in each frame
	double frame_seconds = (double)1.0 / frame_rate;
// Starting time of view in seconds.
	double view_start = mwindow->edl->local_session->view_start[pane->number] *
		time_per_pixel;
// Ending time of view in seconds
	double view_end = (double)(mwindow->edl->local_session->view_start[pane->number] +
		get_w()) * time_per_pixel;
// Get minimum distance between text marks
	int min_pixels1 = get_text_width(MEDIUMFONT,
		Units::totext(string,
			view_start,
			mwindow->edl->session->time_format,
			sample_rate,
			mwindow->edl->session->frame_rate,
			mwindow->edl->session->frames_per_foot)) + TEXT_MARGIN;
	int min_pixels2 = get_text_width(MEDIUMFONT,
		Units::totext(string,
			view_end,
			mwindow->edl->session->time_format,
			sample_rate,
			mwindow->edl->session->frame_rate,
			mwindow->edl->session->frames_per_foot)) + TEXT_MARGIN;
	int min_pixels = (int)MAX(min_pixels1, min_pixels2);


// Minimum seconds between text marks
	double min_time = (double)min_pixels *
		mwindow->edl->local_session->zoom_sample /
		sample_rate;


// Get first text mark on or before window start
	int64_t starting_mark = 0;

	int progression = 1;

// Default text spacing
	text_interval = 0.5;
	double prev_text_interval = 1.0;

	while(text_interval >= min_time)
	{
		prev_text_interval = text_interval;
		if(progression == 0)
		{
			text_interval /= 2;
			progression++;
		}
		else
		if(progression == 1)
		{
			text_interval /= 2;
			progression++;
		}
		else
		if(progression == 2)
		{
			text_interval /= 2.5;
			progression = 0;
		}
	}

	text_interval = prev_text_interval;

	if(1 >= min_time)
		;
	else
	if(2 >= min_time)
		text_interval = 2;
	else
	if(5 >= min_time)
		text_interval = 5;
	else
	if(10 >= min_time)
		text_interval = 10;
	else
	if(15 >= min_time)
		text_interval = 15;
	else
	if(20 >= min_time)
		text_interval = 20;
	else
	if(30 >= min_time)
		text_interval = 30;
	else
	if(60 >= min_time)
		text_interval = 60;
	else
	if(120 >= min_time)
		text_interval = 120;
	else
	if(300 >= min_time)
		text_interval = 300;
	else
	if(600 >= min_time)
		text_interval = 600;
	else
	if(1200 >= min_time)
		text_interval = 1200;
	else
	if(1800 >= min_time)
		text_interval = 1800;
	else
	if(3600 >= min_time)
		text_interval = 3600;

// Set text interval
	switch(mwindow->edl->session->time_format)
	{
		case TIME_FEET_FRAMES:
		{
			double foot_seconds = frame_seconds * mwindow->edl->session->frames_per_foot;
			if(frame_seconds >= min_time)
				text_interval = frame_seconds;
			else
			if(foot_seconds / 8.0 > min_time)
				text_interval = frame_seconds * mwindow->edl->session->frames_per_foot / 8.0;
			else
			if(foot_seconds / 4.0 > min_time)
				text_interval = frame_seconds * mwindow->edl->session->frames_per_foot / 4.0;
			else
			if(foot_seconds / 2.0 > min_time)
				text_interval = frame_seconds * mwindow->edl->session->frames_per_foot / 2.0;
			else
			if(foot_seconds > min_time)
				text_interval = frame_seconds * mwindow->edl->session->frames_per_foot;
			else
			if(foot_seconds * 2 >= min_time)
				text_interval = foot_seconds * 2;
			else
			if(foot_seconds * 5 >= min_time)
				text_interval = foot_seconds * 5;
			else
			{

				for(int factor = 10, progression = 0; factor <= 100000; )
				{
					if(foot_seconds * factor >= min_time)
					{
						text_interval = foot_seconds * factor;
						break;
					}

					if(progression == 0)
					{
						factor = (int)(factor * 2.5);
						progression++;
					}
					else
					if(progression == 1)
					{
						factor = (int)(factor * 2);
						progression++;
					}
					else
					if(progression == 2)
					{
						factor = (int)(factor * 2);
						progression = 0;
					}
				}

			}
			break;
		}

		case TIME_FRAMES:
		case TIME_HMSF:
// One frame per text mark
			if(frame_seconds >= min_time)
				text_interval = frame_seconds;
			else
			if(frame_seconds * 2 >= min_time)
				text_interval = frame_seconds * 2;
			else
			if(frame_seconds * 5 >= min_time)
				text_interval = frame_seconds * 5;
			else
			{

				for(int factor = 10, progression = 0; factor <= 100000; )
				{
					if(frame_seconds * factor >= min_time)
					{
						text_interval = frame_seconds * factor;
						break;
					}

					if(progression == 0)
					{
						factor = (int)(factor * 2.5);
						progression++;
					}
					else
					if(progression == 1)
					{
						factor = (int)(factor * 2);
						progression++;
					}
					else
					if(progression == 2)
					{
						factor = (int)(factor * 2);
						progression = 0;
					}
				}

			}
			break;

		default:
			break;
	}

// Sanity
	while(text_interval < min_time)
	{
		text_interval *= 2;
	}

// Set tick interval
	tick_interval = text_interval;

	switch(mwindow->edl->session->time_format)
	{
		case TIME_HMSF:
		case TIME_FEET_FRAMES:
		case TIME_FRAMES:
			if(frame_seconds / time_per_pixel > TICK_SPACING)
				tick_interval = frame_seconds;
			break;
	}

// Get first text mark on or before window start
	starting_mark = (int64_t)((double)mwindow->edl->local_session->view_start[pane->number] *
		time_per_pixel / text_interval);

	double start_position = (double)starting_mark * text_interval;
	int64_t iteration = 0;


//printf("text_interval=%f\n", text_interval);
	while(start_position + text_interval * iteration < view_end)
	{
		double position1 = start_position + text_interval * iteration;
		int pixel = (int64_t)(position1 / time_per_pixel) -
			mwindow->edl->local_session->view_start[pane->number];
		int pixel1 = pixel;

		Units::totext(string,
			position1,
			mwindow->edl->session->time_format,
			sample_rate,
			mwindow->edl->session->frame_rate,
			mwindow->edl->session->frames_per_foot);
	 	set_color(get_resources()->default_text_color);
		set_font(MEDIUMFONT);

		draw_text(pixel + TEXT_MARGIN, get_text_ascent(MEDIUMFONT), string);
		draw_line(pixel, LINE_MARGIN, pixel, get_h() - 2);

		double position2 = start_position + text_interval * (iteration + 1);
		int pixel2 = (int64_t)(position2 / time_per_pixel) -
			mwindow->edl->local_session->view_start[pane->number];

		for(double tick_position = position1;
			tick_position < position2;
			tick_position += tick_interval)
		{
			pixel = (int64_t)(tick_position / time_per_pixel) -
				mwindow->edl->local_session->view_start[pane->number];
			if(labs(pixel - pixel1) > 1 &&
				labs(pixel - pixel2) > 1)
				draw_line(pixel, TICK_MARGIN, pixel, get_h() - 2);
		}
		iteration++;
	}


}

void MTimeBar::draw_range()
{
	int x1 = 0, x2 = 0;
	if( mwindow->brender_active && mwindow->preferences->use_brender &&
	    mwindow->edl->tracks->total_playable_vtracks() ) {
		double time_per_pixel = (double)mwindow->edl->local_session->zoom_sample /
			mwindow->edl->session->sample_rate;
		x1 = (int)(mwindow->edl->session->brender_start / time_per_pixel) -
			mwindow->edl->local_session->view_start[pane->number];
		x2 = (int)(mwindow->session->brender_end / time_per_pixel) -
			mwindow->edl->local_session->view_start[pane->number];
	}

	if(x2 > x1 && x1 < get_w() && x2 > 0) {
		draw_top_background(get_parent(), 0, 0, x1, get_h());
		draw_3segmenth(x1, 0, x2 - x1, mwindow->theme->get_image("timebar_brender"));
		draw_top_background(get_parent(), x2, 0, get_w() - x2, get_h());
	}
	else {
		draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	}

//  	int64_t pixel = position_to_pixel(
//  		mwindow->edl->local_session->get_selectionstart(1));
//
//  	set_color(mwindow->theme->timebar_cursor_colorg);
//  	draw_line(pixel, 0, pixel, get_h());
//printf("MTimeBar::draw_range %f %f\n", mwindow->session->brender_end, time_per_pixel);
}

void MTimeBar::select_label(double position)
{
	stop_transport();

	EDL *edl = mwindow->edl;
	position = edl->align_to_frame(position, 1);

	if(shift_down())
	{
		if(position > edl->local_session->get_selectionend(1) / 2 +
			edl->local_session->get_selectionstart(1) / 2)
		{

			edl->local_session->set_selectionend(position);
		}
		else
		{
			edl->local_session->set_selectionstart(position);
		}
	}
	else
	{
		edl->local_session->set_selectionstart(position);
		edl->local_session->set_selectionend(position);
	}

// Que the CWindow
	mwindow->cwindow->update(1, 0, 0, 0, 1);

	gui->hide_cursor(0);
	gui->draw_cursor(1);
	gui->flash_canvas(1);
	gui->zoombar->update();
	update_highlights();
	activate_timeline();
}


int MTimeBar::resize_event()
{
	reposition_window(mwindow->theme->mtimebar_x,
		mwindow->theme->mtimebar_y,
		mwindow->theme->mtimebar_w,
		mwindow->theme->mtimebar_h);
	update(0);
	return 1;
}

int MTimeBar::resize_event(int x, int y, int w, int h)
{
	reposition_window(x,
		y,
		w,
		h);
	update(0);
	return 1;
}

// int MTimeBar::test_preview(int buttonpress)
// {
// 	int result = 0;
// 	return result;
// }
//


void MTimeBar::handle_mwindow_drag()
{
//printf("TimeBar::cursor_motion_event %d %d\n", __LINE__, current_operation);
	int relative_cursor_x = pane->canvas->get_relative_cursor_x();
	if(relative_cursor_x >= pane->canvas->get_w() ||
		relative_cursor_x < 0)
	{
		pane->canvas->start_dragscroll();
	}
	else
	if(relative_cursor_x < pane->canvas->get_w() &&
		relative_cursor_x >= 0)
	{
		pane->canvas->stop_dragscroll();
	}

	update(0);
}

void MTimeBar::update(int flush)
{
	TimeBar::update(flush);
}

void MTimeBar::update_clock(double position)
{
	if(!mwindow->cwindow->playback_engine->is_playing_back)
		gui->mainclock->update(position);
}

void MTimeBar::update_cursor()
{
	int rx = get_relative_cursor_x();
	double position = pixel_to_position(rx);

	position = mwindow->edl->align_to_frame(position, 0);
	position = MAX(0, position);

	mwindow->select_point(position);
	update(1);
}

double MTimeBar::test_highlight()
{
// Don't crash during initialization
	if(pane->canvas)
	{
		if(mwindow->session->current_operation == NO_OPERATION)
		{
			if(pane->canvas->is_event_win() &&
				pane->canvas->cursor_inside())
			{
				int cursor_x = pane->canvas->get_cursor_x();
				double position = (double)cursor_x *
					(double)mwindow->edl->local_session->zoom_sample /
					(double)mwindow->edl->session->sample_rate +
					(double)mwindow->edl->local_session->view_start[pane->number] *
					(double)mwindow->edl->local_session->zoom_sample /
					(double)mwindow->edl->session->sample_rate;
				pane->canvas->timebar_position = mwindow->edl->align_to_frame(position, 0);
			}

//printf("MTimeBar::test_highlight %d %d %f\n", __LINE__, pane->canvas->cursor_inside(), pane->canvas->timebar_position);
			return pane->canvas->timebar_position;
		}
		else
		if(mwindow->session->current_operation == SELECT_REGION ||
			mwindow->session->current_operation == DRAG_EDITHANDLE2)
		{
//printf("MTimeBar::test_highlight %d %f\n", __LINE__, mwindow->gui->canvas->timebar_position);
			return pane->canvas->timebar_position;
		}

		return -1;
	}
	else
	{
		return -1;
	}
}

int MTimeBar::repeat_event(int64_t duration)
{
	if(!pane->canvas->drag_scroll) return 0;
	if(duration != BC_WindowBase::get_resources()->scroll_repeat) return 0;

	int distance = 0;
	int x_movement = 0;
	int relative_cursor_x = pane->canvas->get_relative_cursor_x();
	if(current_operation == TIMEBAR_DRAG)
	{
		if(relative_cursor_x >= pane->canvas->get_w())
		{
			distance = relative_cursor_x - pane->canvas->get_w();
			x_movement = 1;
		}
		else
		if(relative_cursor_x < 0)
		{
			distance = relative_cursor_x;
			x_movement = 1;
		}



		if(x_movement)
		{
			update_cursor();
			mwindow->samplemovement(
				mwindow->edl->local_session->view_start[pane->number] + distance,
				pane->number);
		}
		return 1;
	}

	return 0;
}

int MTimeBar::button_press_event()
{
	int result = 0;

	if(is_event_win() && cursor_above() && get_buttonpress() == 3)
	{
		menu->update();
		menu->activate_menu();
		result = 1;
	}

	if(!result) return TimeBar::button_press_event();
	return result;
}


void MTimeBar::activate_timeline()
{
	pane->activate();
}






TimeBarPopupItem::TimeBarPopupItem(MWindow *mwindow,
	TimeBarPopup *menu,
	const char *text,
	int value)
 : BC_MenuItem(text)
{
	this->mwindow = mwindow;
	this->menu = menu;
	this->value = value;
}

int TimeBarPopupItem::handle_event()
{
	mwindow->edl->session->time_format = value;
	mwindow->gui->update(0, NO_DRAW, 1, 0, 0, 1, 0);
	mwindow->gui->redraw_time_dependancies();
	return 1;
}



TimeBarPopup::TimeBarPopup(MWindow *mwindow)
 : BC_PopupMenu(0,
		0,
		0,
		"",
		0)
{
	this->mwindow = mwindow;
}

TimeBarPopup::~TimeBarPopup()
{
}


void TimeBarPopup::create_objects()
{
	add_item(items[0] = new TimeBarPopupItem(mwindow,
		this,
		TIME_HMS_TEXT,
		TIME_HMS));
	add_item(items[1] = new TimeBarPopupItem(mwindow,
		this,
		TIME_HMSF_TEXT,
		TIME_HMSF));
	add_item(items[2] = new TimeBarPopupItem(mwindow,
		this,
		TIME_FRAMES_TEXT,
		TIME_FRAMES));
	add_item(items[3] = new TimeBarPopupItem(mwindow,
		this,
		TIME_SAMPLES_TEXT,
		TIME_SAMPLES));
	add_item(items[4] = new TimeBarPopupItem(mwindow,
		this,
		TIME_SAMPLES_HEX_TEXT,
		TIME_SAMPLES_HEX));
	add_item(items[5] = new TimeBarPopupItem(mwindow,
		this,
		TIME_SECONDS_TEXT,
		TIME_SECONDS));
	add_item(items[6] = new TimeBarPopupItem(mwindow,
		this,
		TIME_FEET_FRAMES_TEXT,
		TIME_FEET_FRAMES));
}

void TimeBarPopup::update()
{
	for(int i = 0; i < TOTAL_TIMEFORMATS; i++)
	{
		if(items[i]->value == mwindow->edl->session->time_format)
		{
			items[i]->set_checked(1);
		}
		else
		{
			items[i]->set_checked(0);
		}
	}
}





