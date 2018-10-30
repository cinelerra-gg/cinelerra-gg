
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

#include "asset.h"
#include "assets.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mainclock.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "renderengine.h"
#include "mainclock.h"
#include "vplayback.h"
#include "vrender.h"
#include "vtimebar.h"
#include "vtracking.h"
#include "vwindow.h"
#include "vwindowgui.h"


VTracking::VTracking(MWindow *mwindow, VWindow *vwindow)
 : Tracking(mwindow)
{
	this->vwindow = vwindow;
}

VTracking::~VTracking()
{
}

PlaybackEngine* VTracking::get_playback_engine()
{
	return vwindow->playback_engine;
}

void VTracking::update_tracker(double position)
{
	if( !vwindow->get_edl() ) return;
	vwindow->gui->lock_window("VTracking::update_tracker");
	vwindow->get_edl()->local_session->set_selectionstart(position);
	vwindow->get_edl()->local_session->set_selectionend(position);
	vwindow->gui->clock->update(position);
// This is going to boost the latency but we need to update the timebar
	vwindow->gui->timebar->update(1);
	vwindow->gui->unlock_window();
	update_meters((int64_t)(position * mwindow->edl->session->sample_rate));
}

int VTracking::start_playback(double new_position)
{

	return Tracking::start_playback(new_position);
}

void VTracking::update_meters(int64_t position)
{
	double output_levels[MAXCHANNELS];

	int do_audio = get_playback_engine()->get_output_levels(output_levels,
		position);
	if(do_audio)
	{
		vwindow->gui->lock_window("VTracking::update_meters");
		vwindow->gui->meters->update(output_levels);
		vwindow->gui->unlock_window();
	}
}

void VTracking::stop_meters()
{
	vwindow->gui->lock_window("VTracking::stop_meters");
	vwindow->gui->meters->stop_meters();
	vwindow->gui->unlock_window();
}

void VTracking::draw()
{
}
