
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

#include "bcsignals.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "ctracking.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mainclock.h"
#include "maincursor.h"
#include "mtimebar.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "zoombar.h"

CTracking::CTracking(MWindow *mwindow, CWindow *cwindow)
 : Tracking(mwindow)
{
	this->cwindow = cwindow;
}

CTracking::~CTracking()
{
}

PlaybackEngine* CTracking::get_playback_engine()
{
	return cwindow->playback_engine;
}

int CTracking::start_playback(double new_position, int active)
{
	mwindow->gui->set_playing_back(1);
	if( active )
		mwindow->edl->local_session->set_playback_start(new_position);
	return Tracking::start_playback(new_position);
}

int CTracking::stop_playback()
{
	mwindow->gui->set_playing_back(0);
	Tracking::stop_playback();
	mwindow->edl->local_session->set_playback_end(get_tracking_position());
	return 0;
}

#define SCROLL_THRESHOLD .5


int CTracking::update_scroll(double position)
{
	int updated_scroll = 0;

	if(mwindow->edl->session->view_follows_playback)
	{
		TimelinePane *pane = mwindow->gui->get_focused_pane();
		double seconds_per_pixel = (double)mwindow->edl->local_session->zoom_sample /
			mwindow->edl->session->sample_rate;
		double half_canvas = seconds_per_pixel *
			pane->canvas->get_w() / 2;
		double midpoint = mwindow->edl->local_session->view_start[pane->number] *
			seconds_per_pixel +
			half_canvas;

		if(get_playback_engine()->command->get_direction() == PLAY_FORWARD)
		{
			double left_boundary = midpoint + SCROLL_THRESHOLD * half_canvas;
			double right_boundary = midpoint + half_canvas;

			if(position > left_boundary &&
				position < right_boundary)
			{
				int pixels = Units::to_int64((position - left_boundary) *
						mwindow->edl->session->sample_rate /
						mwindow->edl->local_session->zoom_sample);
				if(pixels)
				{
					mwindow->move_right(pixels);
//printf("CTracking::update_scroll 1 %d\n", pixels);
					updated_scroll = 1;
				}
			}
		}
		else
		{
			double right_boundary = midpoint - SCROLL_THRESHOLD * half_canvas;
			double left_boundary = midpoint - half_canvas;

			if(position < right_boundary &&
				position > left_boundary &&
				mwindow->edl->local_session->view_start[pane->number] > 0)
			{
				int pixels = Units::to_int64((right_boundary - position) *
						mwindow->edl->session->sample_rate /
						mwindow->edl->local_session->zoom_sample);
				if(pixels)
				{
					mwindow->move_left(pixels);
					updated_scroll = 1;
				}
			}
		}
	}

	return updated_scroll;
}

void CTracking::update_tracker(double position)
{
	int updated_scroll = 0;
	cwindow->gui->lock_window("CTracking::update_tracker 1");

// This is going to boost the latency but we need to update the timebar
	cwindow->gui->timebar->update(1);
//	cwindow->gui->timebar->flash();
	cwindow->gui->unlock_window();

// Update mwindow cursor
	mwindow->gui->lock_window("CTracking::update_tracker 2");

	mwindow->edl->local_session->set_selectionstart(position);
	mwindow->edl->local_session->set_selectionend(position);

	updated_scroll = update_scroll(position);

	mwindow->gui->mainclock->update(position);
	mwindow->gui->update_patchbay();
	mwindow->gui->update_timebar(0);

	if(!updated_scroll)
	{
		mwindow->gui->update_cursor();
		// we just need to update clocks, not everything
		mwindow->gui->zoombar->update_clocks();

		for(int i = 0; i < TOTAL_PANES; i++)
			if(mwindow->gui->pane[i])
				mwindow->gui->pane[i]->canvas->flash(0);
	}

	mwindow->gui->flush();
	mwindow->gui->unlock_window();

// Plugin GUI's hold lock on mwindow->gui here during user interface handlers.
	mwindow->update_plugin_guis();


	update_meters((int64_t)(position * mwindow->edl->session->sample_rate));
}

void CTracking::draw()
{
}
