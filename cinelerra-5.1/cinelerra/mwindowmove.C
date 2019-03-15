
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
#include "clip.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "keyframe.h"
#include "labels.h"
#include "localsession.h"
#include "maincursor.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mtimebar.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "plugin.h"
#include "pluginset.h"
#include "resourcethread.h"
#include "samplescroll.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "track.h"
#include "transportque.h"
#include "zoombar.h"


void MWindow::update_plugins()
{
// Show plugins which are visible and hide plugins which aren't
// Update plugin pointers in plugin servers
}


int MWindow::expand_sample()
{
	if(gui)
	{
		if(edl->local_session->zoom_sample < 0x100000)
		{
			edl->local_session->zoom_sample *= 2;
			gui->zoombar->sample_zoom->update(edl->local_session->zoom_sample);
			zoom_sample(edl->local_session->zoom_sample);
		}
	}
	return 0;
}

int MWindow::zoom_in_sample()
{
	if(gui)
	{
		if(edl->local_session->zoom_sample > 1)
		{
			edl->local_session->zoom_sample /= 2;
			gui->zoombar->sample_zoom->update(edl->local_session->zoom_sample);
			zoom_sample(edl->local_session->zoom_sample);
		}
	}
	return 0;
}

int MWindow::zoom_sample(int64_t zoom_sample)
{
	CLAMP(zoom_sample, 1, 0x100000);
	edl->local_session->zoom_sample = zoom_sample;
	find_cursor();

	TimelinePane *pane = gui->get_focused_pane();
	samplemovement(edl->local_session->view_start[pane->number], pane->number);
	return 0;
}

void MWindow::find_cursor()
{
	TimelinePane *pane = gui->get_focused_pane();
	edl->local_session->view_start[pane->number] =
		Units::round((edl->local_session->get_selectionend(1) +
		edl->local_session->get_selectionstart(1)) /
		2 *
		edl->session->sample_rate /
		edl->local_session->zoom_sample -
		(double)pane->canvas->get_w() /
		2);

	if(edl->local_session->view_start[pane->number] < 0)
		edl->local_session->view_start[pane->number] = 0;
}


void MWindow::fit_selection()
{
	if(EQUIV(edl->local_session->get_selectionstart(1),
		edl->local_session->get_selectionend(1)))
	{
		double total_samples = edl->tracks->total_length() *
			edl->session->sample_rate;
		TimelinePane *pane = gui->get_focused_pane();
		for(edl->local_session->zoom_sample = 1;
			pane->canvas->get_w() * edl->local_session->zoom_sample < total_samples;
			edl->local_session->zoom_sample *= 2)
			;
	}
	else
	{
		double total_samples = (edl->local_session->get_selectionend(1) -
			edl->local_session->get_selectionstart(1)) *
			edl->session->sample_rate;
		TimelinePane *pane = gui->get_focused_pane();
		for(edl->local_session->zoom_sample = 1;
			pane->canvas->get_w() * edl->local_session->zoom_sample < total_samples;
			edl->local_session->zoom_sample *= 2)
			;
	}

	edl->local_session->zoom_sample = MIN(0x100000,
		edl->local_session->zoom_sample);
	zoom_sample(edl->local_session->zoom_sample);
}


void MWindow::fit_autos(int all)
{
	float min = 0, max = 0;
	double start, end;

// Test all autos
	if(EQUIV(edl->local_session->get_selectionstart(1),
		edl->local_session->get_selectionend(1)))
	{
		start = 0;
		end = edl->tracks->total_length();
	}
	else
// Test autos in highlighting only
	{
		start = edl->local_session->get_selectionstart(1);
		end = edl->local_session->get_selectionend(1);
	}

	int forstart = edl->local_session->zoombar_showautotype;
	int forend   = edl->local_session->zoombar_showautotype + 1;

	if( all ) {
		forstart = 0;
		forend   = AUTOGROUPTYPE_COUNT;
	}

	for (int i = forstart; i < forend; i++)
	{
// Adjust min and max
		edl->tracks->get_automation_extents(&min, &max, start, end, i);
//printf("MWindow::fit_autos %d %f %f results in ", i, min, max);

		float range = max - min;
		switch (i)
		{
		case AUTOGROUPTYPE_AUDIO_FADE:
			if (range < 1) {
				min = MIN(min, edl->local_session->automation_mins[i]);
				max = MAX(max, edl->local_session->automation_maxs[i]);
				if( min >= max-0.1 ) { min = -80.0; min = 6.0; }
			}
			break;
		case AUTOGROUPTYPE_VIDEO_FADE:
			if (range < 1) {
				min = MIN(min, edl->local_session->automation_mins[i]);
				max = MAX(max, edl->local_session->automation_maxs[i]);
				if( min >= max-0.1 ) { min = 0.0; min = 100.0; }
			}
			break;
		case AUTOGROUPTYPE_ZOOM:
			if (range < 0.001) {
				min = floor(min*50)/100;
				max = floor(max*200)/100;
			}
			break;
		case AUTOGROUPTYPE_SPEED:
			if (range < 0.001) {
				min = floor(min*5)/100;
				max = floor(max*300)/100;
			}
			break;
		case AUTOGROUPTYPE_X:
		case AUTOGROUPTYPE_Y:
			if (range < 1) {
				float scale = bmin(edl->session->output_w, edl->session->output_h);
				min = floor((min+max)/2) - 0.5*scale;
				max = floor((min+max)/2) + 0.5*scale;
			}
			break;
		}
//printf("%f %f\n", min, max);
		if (!Automation::autogrouptypes_fixedrange[i])
		{
			edl->local_session->automation_mins[i] = min;
			edl->local_session->automation_maxs[i] = max;
		}
	}

// Show range in zoombar
	gui->zoombar->update();

// Draw
	gui->draw_overlays(1);
}


void MWindow::change_currentautorange(int autogrouptype, int increment, int changemax)
{
	float val;
	if (changemax) {
		val = edl->local_session->automation_maxs[autogrouptype];
	} else {
		val = edl->local_session->automation_mins[autogrouptype];
	}

	if (increment)
	{
		switch (autogrouptype) {
		case AUTOGROUPTYPE_AUDIO_FADE:
			val += 2;
			break;
		case AUTOGROUPTYPE_VIDEO_FADE:
			val += 1;
			break;
		case AUTOGROUPTYPE_ZOOM:
		case AUTOGROUPTYPE_SPEED:
			if (val == 0)
				val = 0.001;
			else
				val = val*2;
			break;
		case AUTOGROUPTYPE_X:
		case AUTOGROUPTYPE_Y:
			val = floor(val + 5);
			break;
		}
	}
	else
	{ // decrement
		switch (autogrouptype) {
		case AUTOGROUPTYPE_AUDIO_FADE:
			val -= 2;
			break;
		case AUTOGROUPTYPE_VIDEO_FADE:
			val -= 1;
			break;
		case AUTOGROUPTYPE_ZOOM:
		case AUTOGROUPTYPE_SPEED:
			if (val > 0) val = val/2;
			break;
		case AUTOGROUPTYPE_X:
		case AUTOGROUPTYPE_Y:
			val = floor(val-5);
			break;
		}
	}

	AUTOMATIONVIEWCLAMPS(val, autogrouptype);

	if (changemax) {
		if (val > edl->local_session->automation_mins[autogrouptype])
			edl->local_session->automation_maxs[autogrouptype] = val;
	}
	else
	{
		if (val < edl->local_session->automation_maxs[autogrouptype])
			edl->local_session->automation_mins[autogrouptype] = val;
	}
}


void MWindow::expand_autos(int changeall, int domin, int domax)
{
	if (changeall)
		for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
			if (domin) change_currentautorange(i, 1, 0);
			if (domax) change_currentautorange(i, 1, 1);
		}
	else
	{
		if (domin) change_currentautorange(edl->local_session->zoombar_showautotype, 1, 0);
		if (domax) change_currentautorange(edl->local_session->zoombar_showautotype, 1, 1);
	}
	gui->zoombar->update_autozoom();
	gui->draw_overlays(0);
	gui->update_patchbay();
	gui->flash_canvas(1);
}

void MWindow::shrink_autos(int changeall, int domin, int domax)
{
	if (changeall)
		for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
			if (domin) change_currentautorange(i, 0, 0);
			if (domax) change_currentautorange(i, 0, 1);
		}
	else
	{
		if (domin) change_currentautorange(edl->local_session->zoombar_showautotype, 0, 0);
		if (domax) change_currentautorange(edl->local_session->zoombar_showautotype, 0, 1);
	}
	gui->zoombar->update_autozoom();
	gui->draw_overlays(0);
	gui->update_patchbay();
	gui->flash_canvas(1);
}


void MWindow::zoom_autos(float min, float max)
{
	int i = edl->local_session->zoombar_showautotype;
	edl->local_session->automation_mins[i] = min;
	edl->local_session->automation_maxs[i] = max;
	gui->zoombar->update_autozoom();
	gui->draw_overlays(1);
}


void MWindow::zoom_amp(int64_t zoom_amp)
{
	edl->local_session->zoom_y = zoom_amp;
	gui->draw_canvas(0, 0);
	gui->flash_canvas(0);
	gui->update_patchbay();
	gui->flush();
}

void MWindow::zoom_track(int64_t zoom_track)
{
// scale waveforms
	edl->local_session->zoom_y = (int64_t)((float)edl->local_session->zoom_y *
		zoom_track /
		edl->local_session->zoom_track);
	CLAMP(edl->local_session->zoom_y, MIN_AMP_ZOOM, MAX_AMP_ZOOM);

// scale tracks
	double scale = (double)zoom_track / edl->local_session->zoom_track;
	edl->local_session->zoom_track = zoom_track;

// shift row position
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		edl->local_session->track_start[i] *= scale;
	}
	edl->tracks->update_y_pixels(theme);
	gui->draw_trackmovement();
//printf("MWindow::zoom_track %d %d\n", edl->local_session->zoom_y, edl->local_session->zoom_track);
}

void MWindow::trackmovement(int offset, int pane_number)
{
	edl->local_session->track_start[pane_number] += offset;
	if(edl->local_session->track_start[pane_number] < 0)
		edl->local_session->track_start[pane_number] = 0;

	if(pane_number == TOP_RIGHT_PANE ||
		pane_number == TOP_LEFT_PANE)
	{
		edl->local_session->track_start[TOP_LEFT_PANE] =
			edl->local_session->track_start[TOP_RIGHT_PANE] =
			edl->local_session->track_start[pane_number];
	}
	else
	if(pane_number == BOTTOM_RIGHT_PANE ||
		pane_number == BOTTOM_LEFT_PANE)
	{
		edl->local_session->track_start[BOTTOM_LEFT_PANE] =
			edl->local_session->track_start[BOTTOM_RIGHT_PANE] =
			edl->local_session->track_start[pane_number];
	}


	edl->tracks->update_y_pixels(theme);
	gui->draw_trackmovement();
}

void MWindow::move_up(int64_t distance)
{
	TimelinePane *pane = gui->get_focused_pane();
	if(distance == 0) distance = edl->local_session->zoom_track;

	trackmovement(-distance, pane->number);
}

void MWindow::move_down(int64_t distance)
{
	TimelinePane *pane = gui->get_focused_pane();
	if(distance == 0) distance = edl->local_session->zoom_track;

	trackmovement(distance, pane->number);
}

int MWindow::goto_end()
{
	TimelinePane *pane = gui->get_focused_pane();
	int64_t old_view_start = edl->local_session->view_start[pane->number];

	if(edl->tracks->total_length() > (double)pane->canvas->get_w() *
		edl->local_session->zoom_sample /
		edl->session->sample_rate)
	{
		edl->local_session->view_start[pane->number] =
			Units::round(edl->tracks->total_length() *
				edl->session->sample_rate /
				edl->local_session->zoom_sample -
				pane->canvas->get_w() /
				2);
	}
	else
	{
		edl->local_session->view_start[pane->number] = 0;
	}

	if(gui->shift_down())
	{
		edl->local_session->set_selectionend(edl->tracks->total_length());
	}
	else
	{
		edl->local_session->set_selectionstart(edl->tracks->total_length());
		edl->local_session->set_selectionend(edl->tracks->total_length());
	}

	if(edl->local_session->view_start[pane->number] != old_view_start)
	{
		samplemovement(edl->local_session->view_start[pane->number], pane->number);
		gui->draw_samplemovement();
	}

	update_plugin_guis();

	gui->update_patchbay();
	gui->update_cursor();
	gui->activate_timeline();
	gui->zoombar->update();
	gui->update_timebar(1);
	cwindow->update(1, 0, 0, 0, 1);
	return 0;
}

int MWindow::goto_start()
{
	TimelinePane *pane = gui->get_focused_pane();
	int64_t old_view_start = edl->local_session->view_start[pane->number];

	edl->local_session->view_start[pane->number] = 0;
	if(gui->shift_down())
	{
		edl->local_session->set_selectionstart(0);
	}
	else
	{
		edl->local_session->set_selectionstart(0);
		edl->local_session->set_selectionend(0);
	}

	if(edl->local_session->view_start[pane->number] != old_view_start)
	{
		samplemovement(edl->local_session->view_start[pane->number], pane->number);
		gui->draw_samplemovement();
	}

	update_plugin_guis();
	gui->update_patchbay();
	gui->update_cursor();
	gui->activate_timeline();
	gui->zoombar->update();
	gui->update_timebar(1);
	cwindow->update(1, 0, 0, 0, 1);
	return 0;
}

int MWindow::goto_position(double position)
{
	position = edl->align_to_frame(position, 0);
	if( position < 0 ) position = 0;
	select_point(position);
	gui->activate_timeline();
	return 0;
}

int MWindow::samplemovement(int64_t view_start, int pane_number)
{
	if( view_start < 0 ) view_start = 0;
	edl->local_session->view_start[pane_number] = view_start;
	if(edl->local_session->view_start[pane_number] < 0)
		edl->local_session->view_start[pane_number] = 0;

	if(pane_number == TOP_LEFT_PANE ||
		pane_number == BOTTOM_LEFT_PANE)
	{
		edl->local_session->view_start[TOP_LEFT_PANE] =
			edl->local_session->view_start[BOTTOM_LEFT_PANE] =
			edl->local_session->view_start[pane_number];
	}
	else
	{
		edl->local_session->view_start[TOP_RIGHT_PANE] =
			edl->local_session->view_start[BOTTOM_RIGHT_PANE] =
			edl->local_session->view_start[pane_number];
	}

	gui->draw_samplemovement();

	return 0;
}

int MWindow::move_left(int64_t distance)
{
	TimelinePane *pane = gui->get_focused_pane();
	if(!distance)
		distance = pane->canvas->get_w() /
			10;
	edl->local_session->view_start[pane->number] -= distance;
	samplemovement(edl->local_session->view_start[pane->number],
		pane->number);
	return 0;
}

int MWindow::move_right(int64_t distance)
{
	TimelinePane *pane = gui->get_focused_pane();
	if(!distance)
		distance = pane->canvas->get_w() /
			10;
	edl->local_session->view_start[pane->number] += distance;
	samplemovement(edl->local_session->view_start[pane->number],
		pane->number);
	return 0;
}

void MWindow::select_all()
{
	if( edl->local_session->get_selectionstart(1) == 0 &&
	    edl->local_session->get_selectionend(1) == edl->tracks->total_length() )
		edl->local_session->set_selectionend(0);
	else {
		edl->local_session->set_selectionstart(0);
		edl->local_session->set_selectionend(edl->tracks->total_length());
	}
	gui->update(0, NORMAL_DRAW, 1, 1, 0, 1, 0);
	gui->activate_timeline();
	cwindow->update(1, 0, 0, 0, 1);
	update_plugin_guis();
}

int MWindow::next_label(int shift_down)
{
	double position = edl->local_session->get_selectionend(1);
	double total_length = edl->tracks->total_length();
	Label *current = edl->labels->next_label(position);
	double new_position = current ? current->position : total_length;
// last playback endpoints as fake label positions
	double playback_start = edl->local_session->playback_start;
	double playback_end = edl->local_session->playback_end;
	if( playback_start > position && playback_start < new_position )
		new_position = playback_start;
	else if( playback_end > position && playback_end < new_position )
		new_position = playback_end;
	if( new_position > total_length )
		new_position = total_length;
	edl->local_session->set_selectionend(new_position);
//printf("MWindow::next_edit_handle %d\n", shift_down);
	if( !shift_down )
		edl->local_session->set_selectionstart(new_position);
	return find_selection(new_position);
}

int MWindow::prev_label(int shift_down)
{
	double position = edl->local_session->get_selectionstart(1);
	Label *current = edl->labels->prev_label(position);
	double new_position = current ? current->position : 0;
// last playback endpoints as fake label positions
	double playback_start = edl->local_session->playback_start;
	double playback_end = edl->local_session->playback_end;
	if( playback_end < position && playback_end > new_position )
		new_position = playback_end;
	else if( playback_start < position && playback_start > new_position )
		new_position = playback_start;
	if( new_position < 0 )
		new_position = 0;
	edl->local_session->set_selectionstart(new_position);
//printf("MWindow::next_edit_handle %d\n", shift_down);
	if( !shift_down )
		edl->local_session->set_selectionend(new_position);
	return find_selection(new_position);
}

int MWindow::next_edit_handle(int shift_down)
{
	double position = edl->local_session->get_selectionend(1);
	double new_position = edl->next_edit(position);
	double total_length = edl->tracks->total_length();
	if( new_position >= total_length )
		new_position = total_length;
	edl->local_session->set_selectionend(new_position);
//printf("MWindow::next_edit_handle %d\n", shift_down);
	if( !shift_down )
		edl->local_session->set_selectionstart(new_position);
	return find_selection(new_position);
}

int MWindow::prev_edit_handle(int shift_down)
{
	double position = edl->local_session->get_selectionstart(1);
	double new_position = edl->prev_edit(position);
	if( new_position < 0 )
		new_position = 0;
	edl->local_session->set_selectionstart(new_position);
	if( !shift_down )
		edl->local_session->set_selectionend(new_position);
	return find_selection(new_position);
}

int MWindow::nearest_plugin_keyframe(int shift_down, int dir)
{
	KeyFrame *keyframe = 0;
	double start = edl->local_session->get_selectionstart(1);
	double end = edl->local_session->get_selectionend(1);
	double position = dir == PLAY_FORWARD ? end : start, new_position = 0;
	for( Track *track=edl->tracks->first; track; track=track->next ) {
		if( !track->record ) continue;
		for( int i=0; i<track->plugin_set.size(); ++i ) {
			PluginSet *plugin_set = track->plugin_set[i];
			int64_t pos = track->to_units(position, 0);
			KeyFrame *key = plugin_set->nearest_keyframe(pos, dir);
			if( !key ) continue;
			double key_position = track->from_units(key->position);
			if( keyframe && (dir == PLAY_FORWARD ?
				key_position >= new_position :
				new_position >= key_position ) ) continue;
			keyframe = key;  new_position = key_position;
		}
	}

	new_position = keyframe ?
		keyframe->autos->track->from_units(keyframe->position) :
		dir == PLAY_FORWARD ? edl->tracks->total_length() : 0;

	if( !shift_down )
		start = end = new_position;
	else if( dir == PLAY_FORWARD )
		end = new_position;
	else
		start = new_position;

	edl->local_session->set_selectionstart(start);
	edl->local_session->set_selectionend(end);
	return find_selection(new_position);
}

int MWindow::find_selection(double position, int scroll_display)
{
	update_plugin_guis();
	TimelinePane *pane = gui->get_focused_pane();
	double pixel_zoom = (double)edl->session->sample_rate / edl->local_session->zoom_sample;
	if( !scroll_display ) {
		double timeline_start = edl->local_session->view_start[pane->number] / pixel_zoom;
		double timeline_end = timeline_start + pane->canvas->time_visible();
		if( position >= timeline_end || position < timeline_start )
			scroll_display = 1;
	}
	if( scroll_display ) {
		samplemovement((int64_t)(position * pixel_zoom - pane->canvas->get_w() / 2), pane->number);
		cwindow->update(1, 0, 0, 0, 0);
	}
	else {
		gui->update_patchbay();
		gui->update_timebar(0);
		gui->hide_cursor(0);
		gui->draw_cursor(0);
		gui->zoombar->update();
		gui->flash_canvas(1);
		cwindow->update(1, 0, 0, 0, 1);
	}
	return 0;
}

double MWindow::get_position()
{
        return edl->local_session->get_selectionstart(1);
}

void MWindow::set_position(double position)
{
        if( position != get_position() ) {
                if( position < 0 ) position = 0;
                edl->local_session->set_selectionstart(position);
                edl->local_session->set_selectionend(position);
                gui->lock_window();
                find_cursor();
                gui->update(1, NORMAL_DRAW, 1, 1, 1, 1, 0);
                gui->unlock_window();
                cwindow->update(1, 0, 0, 0, 0);
        }
}


int MWindow::expand_y()
{
	int result = edl->local_session->zoom_y * 2;
	result = MIN(result, MAX_AMP_ZOOM);
	zoom_amp(result);
	gui->zoombar->update();
	return 0;
}

int MWindow::zoom_in_y()
{
	int result = edl->local_session->zoom_y / 2;
	result = MAX(result, MIN_AMP_ZOOM);
	zoom_amp(result);
	gui->zoombar->update();
	return 0;
}

int MWindow::expand_t()
{
	int result = edl->local_session->zoom_track * 2;
	result = MIN(result, MAX_TRACK_ZOOM);
	zoom_track(result);
	gui->zoombar->update();
	return 0;
}

int MWindow::zoom_in_t()
{
	int result = edl->local_session->zoom_track / 2;
	result = MAX(result, MIN_TRACK_ZOOM);
	zoom_track(result);
	gui->zoombar->update();
	return 0;
}

void MWindow::split_x()
{
	gui->resource_thread->stop_draw(1);

	if(gui->pane[TOP_RIGHT_PANE])
	{
		gui->delete_x_pane(theme->mcanvas_w);
		edl->local_session->x_pane = -1;
	}
	else
	{
		gui->create_x_pane(theme->mcanvas_w / 2);
		edl->local_session->x_pane = theme->mcanvas_w / 2;
	}

	gui->mainmenu->update_toggles(0);
	gui->update_pane_dividers();
	gui->update_cursor();
	gui->draw_samplemovement();
// required to get new widgets to appear
	gui->show_window();

	gui->resource_thread->start_draw();
}

void MWindow::split_y()
{
	gui->resource_thread->stop_draw(1);
	if(gui->pane[BOTTOM_LEFT_PANE])
	{
		gui->delete_y_pane(theme->mcanvas_h);
		edl->local_session->y_pane = -1;
	}
	else
	{
		gui->create_y_pane(theme->mcanvas_h / 2);
		edl->local_session->y_pane = theme->mcanvas_h / 2;
	}

	gui->mainmenu->update_toggles(0);
	gui->update_pane_dividers();
	gui->update_cursor();
	gui->draw_trackmovement();
// required to get new widgets to appear
	gui->show_window();
	gui->resource_thread->start_draw();
}

