
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

#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "maincursor.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "mainsession.h"
#include "theme.h"
#include "timelinepane.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "trackscroll.h"

TrackScroll::TrackScroll(MWindow *mwindow,
	MWindowGUI *gui,
	int x,
	int y,
	int h)
 : BC_ScrollBar(x,
 	y,
	SCROLL_VERT,
	h,
	0,
	0,
	0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->pane = 0;
	old_position = 0;
}

TrackScroll::TrackScroll(MWindow *mwindow,
	TimelinePane *pane,
	int x,
	int y,
	int h)
 : BC_ScrollBar(x,
 	y,
	SCROLL_VERT,
	h,
	0,
	0,
	0)
{
	this->mwindow = mwindow;
	this->gui = mwindow->gui;
	this->pane = pane;
	old_position = 0;
}

TrackScroll::~TrackScroll()
{
}

long TrackScroll::get_distance()
{
	return get_value() - old_position;
}

int TrackScroll::update()
{
	return 0;
}

int TrackScroll::resize_event()
{
	reposition_window(mwindow->theme->mvscroll_x,
		mwindow->theme->mvscroll_y,
		mwindow->theme->mvscroll_h);
	update();
	return 0;
}

int TrackScroll::resize_event(int x, int y, int h)
{
	reposition_window(x,
		y,
		h);
	update();
	return 0;
}

int TrackScroll::flip_vertical(int top, int bottom)
{
	return 0;
}

void TrackScroll::set_position()
{
	int64_t max_pos = mwindow->edl->get_tracks_height(mwindow->theme) - pane->view_h;
	if( max_pos < 0 ) max_pos = 0;
	if( mwindow->edl->local_session->track_start[pane->number] > max_pos )
		mwindow->edl->local_session->track_start[pane->number] = max_pos;
	if( pane->number == TOP_RIGHT_PANE ) {
		if( mwindow->edl->local_session->track_start[TOP_LEFT_PANE] > max_pos )
			mwindow->edl->local_session->track_start[TOP_LEFT_PANE] = max_pos;
	}
	else if( pane->number == BOTTOM_RIGHT_PANE ) {
		if( mwindow->edl->local_session->track_start[BOTTOM_LEFT_PANE] > max_pos )
			mwindow->edl->local_session->track_start[BOTTOM_LEFT_PANE] = max_pos;
	}
	update_length(
		mwindow->edl->get_tracks_height(mwindow->theme),
		mwindow->edl->local_session->track_start[pane->number],
		pane->view_h,
		0);
}

int TrackScroll::handle_event()
{
	int64_t distance = get_value() -
		mwindow->edl->local_session->track_start[pane->number];
	mwindow->trackmovement(distance, pane->number);
// 	mwindow->edl->local_session->track_start[pane->number] = get_value();
// 	if(pane->number == TOP_RIGHT_PANE)
// 		mwindow->edl->local_session->track_start[TOP_LEFT_PANE] =
// 			mwindow->edl->local_session->track_start[pane->number];
// 	else
// 	if(pane->number == BOTTOM_RIGHT_PANE)
// 		mwindow->edl->local_session->track_start[BOTTOM_LEFT_PANE] =
// 			mwindow->edl->local_session->track_start[pane->number];
//
// 	mwindow->edl->tracks->update_y_pixels(mwindow->theme);
// 	mwindow->gui->draw_canvas(0, 0);
// 	mwindow->gui->draw_cursor(1);
// 	mwindow->gui->update_patchbay();
// 	mwindow->gui->flash_canvas(1);
// Scrollbar must be active to trap button release events
//	mwindow->gui->canvas->activate();
	old_position = get_value();
	return 1;
}
