/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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
#include "awindowgui.h"
#include "awindow.h"
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "clipedit.h"
#include "commercials.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "gwindow.h"
#include "gwindowgui.h"
#include "keyframe.h"
#include "keyframes.h"
#include "language.h"
#include "labels.h"
#include "levelwindow.h"
#include "localsession.h"
#include "mainclock.h"
#include "maincursor.h"
#include "mainerror.h"
#include "mainindexes.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "maskautos.h"
#include "mtimebar.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "panauto.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "pluginset.h"
#include "recordlabel.h"
#include "samplescroll.h"
#include "trackcanvas.h"
#include "track.h"
#include "trackscroll.h"
#include "tracks.h"
#include "transition.h"
#include "transportque.h"
#include "units.h"
#include "undostackitem.h"
#include "vplayback.h"
#include "vwindow.h"
#include "vwindowgui.h"
#include "zoombar.h"
#include "zwindow.h"
#include "automation.h"
#include "maskautos.h"

#include <string.h>

void MWindow::add_audio_track_entry(int above, Track *dst)
{
	undo->update_undo_before();
	add_audio_track(above, dst);
	save_backup();
	undo->update_undo_after(_("add track"), LOAD_ALL);

	restart_brender();
	gui->update(1, NORMAL_DRAW, 0, 0, 1, 0, 0);
	gui->activate_timeline();

//	gui->get_scrollbars(0);
//	gui->canvas->draw();
//	gui->patchbay->update();
//	gui->cursor->draw(1);
//	gui->canvas->flash();
//	gui->canvas->activate();
	cwindow->refresh_frame(CHANGE_EDL);
}

void MWindow::add_video_track_entry(Track *dst)
{
	undo->update_undo_before();
	add_video_track(1, dst);
	undo->update_undo_after(_("add track"), LOAD_ALL);

	restart_brender();

	gui->update(1, NORMAL_DRAW, 0, 0, 1, 0, 0);
	gui->activate_timeline();
//	gui->get_scrollbars(0);
//	gui->canvas->draw();
//	gui->patchbay->update();
//	gui->cursor->draw(1);
//	gui->canvas->flash();
//	gui->canvas->activate();
	cwindow->refresh_frame(CHANGE_EDL);
	save_backup();
}

void MWindow::add_subttl_track_entry(Track *dst)
{
	undo->update_undo_before();
	add_subttl_track(1, dst);
	undo->update_undo_after(_("add track"), LOAD_ALL);

	restart_brender();

	gui->update(1, NORMAL_DRAW, 0, 0, 1, 0, 0);
	gui->activate_timeline();
//	gui->get_scrollbars(0);
//	gui->canvas->draw();
//	gui->patchbay->update();
//	gui->cursor->draw(1);
//	gui->canvas->flash();
//	gui->canvas->activate();
	cwindow->refresh_frame(CHANGE_EDL);
	save_backup();
}


int MWindow::add_audio_track(int above, Track *dst)
{
	edl->tracks->add_audio_track(above, dst);
	edl->tracks->update_y_pixels(theme);
	save_backup();
	return 0;
}

int MWindow::add_video_track(int above, Track *dst)
{
	edl->tracks->add_video_track(above, dst);
	edl->tracks->update_y_pixels(theme);
	save_backup();
	return 0;
}

int MWindow::add_subttl_track(int above, Track *dst)
{
	edl->tracks->add_subttl_track(above, dst);
	edl->tracks->update_y_pixels(theme);
	save_backup();
	return 0;
}

void MWindow::asset_to_all()
{
	if( !session->drag_assets->size() ) return;
	Indexable *indexable = session->drag_assets->get(0);

//	if( indexable->have_video() )
	{
		int w, h;

		undo->update_undo_before();

// Get w and h
		w = indexable->get_w();
		h = indexable->get_h();
		double new_framerate = session->drag_assets->get(0)->get_frame_rate();
		double old_framerate = edl->session->frame_rate;
		int old_samplerate = edl->session->sample_rate;
		int new_samplerate = session->drag_assets->get(0)->get_sample_rate();


		if( indexable->have_video() ) {
			edl->session->output_w = w;
			edl->session->output_h = h;
			edl->session->frame_rate = new_framerate;
			create_aspect_ratio(
				edl->session->aspect_w,
				edl->session->aspect_h,
				w, h);

			for( Track *current=edl->tracks->first; current; current=NEXT ) {
				if( current->data_type == TRACK_VIDEO /* &&
					current->record */  ) {
					current->track_w = w;
					current->track_h = h;
				}
			}


			if( ((edl->session->output_w % 4) ||
				(edl->session->output_h % 4)) &&
				edl->session->playback_config->vconfig->driver == PLAYBACK_X11_GL ) {
				MainError::show_error(
					_("This project's dimensions are not multiples of 4 so\n"
					"it can't be rendered by OpenGL."));
			}

// Get aspect ratio
			if( defaults->get("AUTOASPECT", 0) ) {
				create_aspect_ratio(
					edl->session->aspect_w,
					edl->session->aspect_h,
					w, h);
			}
		}

		if( indexable->have_audio() ) {
			edl->session->sample_rate = new_samplerate;
			edl->resample(old_framerate, new_framerate, TRACK_VIDEO);
			edl->resample(old_samplerate, new_samplerate, TRACK_AUDIO);
		}

		save_backup();

		undo->update_undo_after(_("asset to all"), LOAD_ALL);
		restart_brender();
		gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 0);
		sync_parameters(CHANGE_ALL);
	}
}

void MWindow::asset_to_size()
{
	if( !session->drag_assets->size() ) return;
	Indexable *indexable = session->drag_assets->get(0);

	if( indexable->have_video() ) {
		int w, h;
		undo->update_undo_before();

// Get w and h
		w = indexable->get_w();
		h = indexable->get_h();
		edl->session->output_w = w;
		edl->session->output_h = h;

		if( ((edl->session->output_w % 4) ||
			(edl->session->output_h % 4)) &&
			edl->session->playback_config->vconfig->driver == PLAYBACK_X11_GL ) {
			MainError::show_error(
				_("This project's dimensions are not multiples of 4 so\n"
				"it can't be rendered by OpenGL."));
		}

// Get aspect ratio
		if( defaults->get("AUTOASPECT", 0) ) {
			create_aspect_ratio(edl->session->aspect_w,
				edl->session->aspect_h,
				w,
				h);
		}

		save_backup();

		undo->update_undo_after(_("asset to size"), LOAD_ALL);
		restart_brender();
		sync_parameters(CHANGE_ALL);
	}
}


void MWindow::asset_to_rate()
{
	if( session->drag_assets->size() &&
		session->drag_assets->get(0)->have_video() ) {
		double new_framerate = session->drag_assets->get(0)->get_frame_rate();
		double old_framerate = edl->session->frame_rate;
		undo->update_undo_before();

		edl->session->frame_rate = new_framerate;
		edl->resample(old_framerate, new_framerate, TRACK_VIDEO);

		save_backup();

		undo->update_undo_after(_("asset to rate"), LOAD_ALL);
		restart_brender();
		gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 0);
		sync_parameters(CHANGE_ALL);
	}
}


void MWindow::clear_entry()
{
	undo->update_undo_before();
	clear(1);

	edl->optimize();
	save_backup();
	undo->update_undo_after(_("clear"), LOAD_EDITS | LOAD_TIMEBAR);

	restart_brender();
	update_plugin_guis();
	gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 0);
	cwindow->update(1, 0, 0, 0, 1);
	cwindow->refresh_frame(CHANGE_EDL);
}

void MWindow::clear(int clear_handle)
{
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	if( clear_handle || !EQUIV(start, end) ) {
		edl->clear(start,
			end,
			edl->session->labels_follow_edits,
			edl->session->plugins_follow_edits,
			edl->session->autos_follow_edits);
	}
}

void MWindow::update_gui(int changed_edl)
{
	restart_brender();
	update_plugin_guis();
	if( changed_edl ) {
		gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 0);
		cwindow->update(1, 0, 0, 0, 1);
		cwindow->refresh_frame(CHANGE_EDL);
	}
	else {
		gui->draw_overlays(1);
		sync_parameters(CHANGE_PARAMS);
		gui->update_patchbay();
		cwindow->update(1, 0, 0);
	}
}

void MWindow::set_automation_mode(int mode)
{
	undo->update_undo_before();
	speed_before();
	edl->tracks->set_automation_mode(
		edl->local_session->get_selectionstart(),
		edl->local_session->get_selectionend(),
		mode);
	int changed_edl = speed_after(1);
	save_backup();
	char string[BCSTRLEN];
	sprintf(string,"set %s", FloatAuto::curve_name(mode));
	undo->update_undo_after(string,
		!changed_edl ? LOAD_AUTOMATION :
			LOAD_AUTOMATION + LOAD_EDITS + LOAD_TIMEBAR);
	update_gui(changed_edl);
}

void MWindow::clear_automation()
{
	undo->update_undo_before();
	speed_before();
	edl->tracks->clear_automation(edl->local_session->get_selectionstart(),
		edl->local_session->get_selectionend());
	int changed_edl = speed_after(1);
	save_backup();
	undo->update_undo_after(_("clear keyframes"),
		!changed_edl ? LOAD_AUTOMATION :
			LOAD_AUTOMATION + LOAD_EDITS + LOAD_TIMEBAR);
	update_gui(changed_edl);
}

int MWindow::clear_default_keyframe()
{
	undo->update_undo_before();
	speed_before();
	edl->tracks->clear_default_keyframe();
	int changed_edl = speed_after(1);
	save_backup();
	undo->update_undo_after(_("clear default keyframe"),
		!changed_edl ? LOAD_AUTOMATION :
			LOAD_AUTOMATION + LOAD_EDITS + LOAD_TIMEBAR);
	update_gui(changed_edl);
	return 0;
}

void MWindow::clear_labels()
{
	undo->update_undo_before();
	clear_labels(edl->local_session->get_selectionstart(),
		edl->local_session->get_selectionend());
	undo->update_undo_after(_("clear labels"), LOAD_TIMEBAR);

	gui->update_timebar(1);
	cwindow->update(0, 0, 0, 0, 1);
	save_backup();
}

int MWindow::clear_labels(double start, double end)
{
	edl->labels->clear(start, end, 0);
	return 0;
}

void MWindow::concatenate_tracks()
{
	undo->update_undo_before();
	edl->tracks->concatenate_tracks(edl->session->plugins_follow_edits,
		edl->session->autos_follow_edits);
	save_backup();
	undo->update_undo_after(_("concatenate tracks"), LOAD_EDITS);

	restart_brender();
	gui->update(1, NORMAL_DRAW, 0, 0, 1, 0, 0);
	cwindow->refresh_frame(CHANGE_EDL);
}


void MWindow::copy()
{
	copy(edl->local_session->get_selectionstart(),
		edl->local_session->get_selectionend());
}

int MWindow::copy(double start, double end)
{
	if( start == end ) return 1;

	FileXML file;
	edl->copy(start, end, 0, &file, "", 1);
	const char *file_string = file.string();
	long file_length = strlen(file_string);
	gui->to_clipboard(file_string, file_length, BC_PRIMARY_SELECTION);
	gui->to_clipboard(file_string, file_length, SECONDARY_SELECTION);
	save_backup();
	return 0;
}

int MWindow::copy_automation()
{
	FileXML file;
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	edl->tracks->copy_automation(start, end, &file, 0, 1);
	const char *file_string = file.string();
	long file_length = strlen(file_string);
	gui->to_clipboard(file_string, file_length, BC_PRIMARY_SELECTION);
	gui->to_clipboard(file_string, file_length, SECONDARY_SELECTION);
	return 0;
}

int MWindow::copy_default_keyframe()
{
	FileXML file;
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	edl->tracks->copy_automation(start, end, &file, 1, 0);
	const char *file_string = file.string();
	long file_length = strlen(file_string);
	gui->to_clipboard(file_string, file_length, BC_PRIMARY_SELECTION);
	gui->to_clipboard(file_string, file_length, SECONDARY_SELECTION);
	return 0;
}


// Uses cropping coordinates in edl session to crop and translate video.
// We modify the projector since camera automation depends on the track size.
void MWindow::crop_video()
{

	undo->update_undo_before();
// Clamp EDL crop region
	if( edl->session->crop_x1 > edl->session->crop_x2 ) {
		edl->session->crop_x1 ^= edl->session->crop_x2;
		edl->session->crop_x2 ^= edl->session->crop_x1;
		edl->session->crop_x1 ^= edl->session->crop_x2;
	}
	if( edl->session->crop_y1 > edl->session->crop_y2 ) {
		edl->session->crop_y1 ^= edl->session->crop_y2;
		edl->session->crop_y2 ^= edl->session->crop_y1;
		edl->session->crop_y1 ^= edl->session->crop_y2;
	}

	float old_projector_x = (float)edl->session->output_w / 2;
	float old_projector_y = (float)edl->session->output_h / 2;
	float new_projector_x = (float)(edl->session->crop_x1 + edl->session->crop_x2) / 2;
	float new_projector_y = (float)(edl->session->crop_y1 + edl->session->crop_y2) / 2;
	float projector_offset_x = -(new_projector_x - old_projector_x);
	float projector_offset_y = -(new_projector_y - old_projector_y);

	edl->tracks->translate_projector(projector_offset_x, projector_offset_y);

	edl->session->output_w = edl->session->crop_x2 - edl->session->crop_x1;
	edl->session->output_h = edl->session->crop_y2 - edl->session->crop_y1;
	edl->session->crop_x1 = 0;
	edl->session->crop_y1 = 0;
	edl->session->crop_x2 = edl->session->output_w;
	edl->session->crop_y2 = edl->session->output_h;

// Recalculate aspect ratio
	if( defaults->get("AUTOASPECT", 0) ) {
		create_aspect_ratio(edl->session->aspect_w,
			edl->session->aspect_h,
			edl->session->output_w,
			edl->session->output_h);
	}

	undo->update_undo_after(_("crop"), LOAD_ALL);

	restart_brender();
	cwindow->refresh_frame(CHANGE_ALL);
	save_backup();
}

void MWindow::cut()
{
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	if( EQUIV(start,end) )
		blade(start);
	else
		cut(start, end);
}

void MWindow::blade(double position)
{
	undo->update_undo_before();
	edl->blade(position);
	edl->optimize();
	save_backup();
	undo->update_undo_after(_("blade"), LOAD_EDITS | LOAD_TIMEBAR);
	restart_brender();
	update_plugin_guis();
	gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 0);
	cwindow->update(1, 0, 0, 0, 1);
	awindow->gui->async_update_assets();
	cwindow->refresh_frame(CHANGE_EDL);
}

void MWindow::cut(double start, double end, double new_position)
{
	undo->update_undo_before();
	copy(start, end);
	edl->clear(start, end,
		edl->session->labels_follow_edits,
		edl->session->plugins_follow_edits,
		edl->session->autos_follow_edits);

	edl->optimize();
	save_backup();
	undo->update_undo_after(_("split | cut"), LOAD_EDITS | LOAD_TIMEBAR);
	if( new_position >= 0 ) {
		edl->local_session->set_selectionstart(new_position);
		edl->local_session->set_selectionend(new_position);
	}
	restart_brender();
	update_plugin_guis();
	gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 0);
	cwindow->update(1, 0, 0, 0, 1);
	awindow->gui->async_update_assets();
	cwindow->refresh_frame(CHANGE_EDL);
}

void MWindow::cut_left_edit()
{
	double start_pos = edl->local_session->get_selectionstart(1);
	double position = edl->prev_edit(start_pos);
	if( position < start_pos )
		cut(position, start_pos, position);
}

void MWindow::cut_right_edit()
{
	double end_pos = edl->local_session->get_selectionend(1);
	double position = edl->next_edit(end_pos);
	if( end_pos < position )
		cut(end_pos, position, end_pos);
}

void MWindow::cut_left_label()
{
	double start_pos = edl->local_session->get_selectionstart(1);
	Label *left_label = edl->labels->prev_label(start_pos);
	if( !left_label ) return;
	double position = left_label->position;
	if( position < start_pos )
		cut(position, start_pos, position);
}

void MWindow::cut_right_label()
{
	double end_pos = edl->local_session->get_selectionend(1);
	Label *right_label = edl->labels->next_label(end_pos);
	if( !right_label ) return;
	double position = right_label->position;
	if( end_pos < position )
		cut(end_pos, position, end_pos);
}

int MWindow::cut_automation()
{
	undo->update_undo_before();
	speed_before();
	copy_automation();
	edl->tracks->clear_automation(edl->local_session->get_selectionstart(),
		edl->local_session->get_selectionend());
	int changed_edl = speed_after(1);
	save_backup();
	undo->update_undo_after(_("cut keyframes"),
		!changed_edl ? LOAD_AUTOMATION :
			LOAD_AUTOMATION + LOAD_EDITS + LOAD_TIMEBAR);
	update_gui(changed_edl);
	return 0;
}

int MWindow::cut_default_keyframe()
{

	undo->update_undo_before();
	speed_before();
	copy_default_keyframe();
	edl->tracks->clear_default_keyframe();
	int changed_edl = speed_after(1);
	save_backup();
	undo->update_undo_after(_("cut default keyframe"),
		!changed_edl ? LOAD_AUTOMATION :
			LOAD_AUTOMATION + LOAD_EDITS + LOAD_TIMEBAR);
	update_gui(changed_edl);
	return 0;
}


void MWindow::delete_track()
{
	if( edl->tracks->last )
		delete_track(edl->tracks->last);
}

void MWindow::delete_tracks()
{
	undo->update_undo_before();
	edl->tracks->delete_tracks();
	undo->update_undo_after(_("delete tracks"), LOAD_ALL);
	save_backup();

	restart_brender();
	update_plugin_states();

	gui->update(1, NORMAL_DRAW, 1, 0, 1, 0, 0);
	cwindow->refresh_frame(CHANGE_EDL);
}

void MWindow::delete_track(Track *track)
{
	undo->update_undo_before();
	edl->tracks->delete_track(track);
	undo->update_undo_after(_("delete track"), LOAD_ALL);

	restart_brender();
	update_plugin_states();

	gui->update(1, NORMAL_DRAW, 1, 0, 1, 0, 0);
	cwindow->refresh_frame(CHANGE_EDL);
	save_backup();
}


// Insert data from clipboard
void MWindow::insert(double position, FileXML *file,
	int edit_labels, int edit_plugins, int edit_autos,
	EDL *parent_edl, Track *first_track, int overwrite)
{
// For clipboard pasting make the new edl use a separate session
// from the master EDL.  Then it can be resampled to the master rates.
// For splice, overwrite, and dragging need same session to get the assets.
	EDL *edl = new EDL(parent_edl);
	ArrayList<EDL*> new_edls;
	uint32_t load_flags = LOAD_ALL;


	new_edls.append(edl);
	edl->create_objects();




	if( parent_edl ) load_flags &= ~LOAD_SESSION;
	if( !edl->session->autos_follow_edits ) load_flags &= ~LOAD_AUTOMATION;
	if( !edl->session->labels_follow_edits ) load_flags &= ~LOAD_TIMEBAR;

	edl->load_xml(file, load_flags);


//printf("MWindow::insert %f\n", edl->local_session->clipboard_length);



	paste_edls(&new_edls, LOADMODE_PASTE, first_track, position,
		edit_labels, edit_plugins, edit_autos, overwrite);
// if( vwindow->edl )
// printf("MWindow::insert 5 %f %f\n",
// vwindow->edl->local_session->in_point,
// vwindow->edl->local_session->out_point);
	new_edls.remove_all();
	edl->Garbage::remove_user();
//printf("MWindow::insert 6 %p\n", vwindow->get_edl());
}

void MWindow::insert_effects_canvas(double start,
	double length)
{
	Track *dest_track = session->track_highlighted;
	if( !dest_track ) return;

	undo->update_undo_before();

	for( int i=0; i<session->drag_pluginservers->total; ++i ) {
		PluginServer *plugin = session->drag_pluginservers->values[i];
		insert_effect(plugin->title, 0, dest_track,
			i == 0 ? session->pluginset_highlighted : 0,
			start, length, PLUGIN_STANDALONE);
	}

	save_backup();
	undo->update_undo_after(_("insert effect"), LOAD_EDITS | LOAD_PATCHES);
	restart_brender();
	sync_parameters(CHANGE_EDL);
// GUI updated in TrackCanvas, after current_operations are reset
}

void MWindow::insert_effects_cwindow(Track *dest_track)
{
	if( !dest_track ) return;

	undo->update_undo_before();

	double start = 0;
	double length = dest_track->get_length();

	if( edl->local_session->get_selectionend() >
		edl->local_session->get_selectionstart() ) {
		start = edl->local_session->get_selectionstart();
		length = edl->local_session->get_selectionend() -
			edl->local_session->get_selectionstart();
	}

	for( int i=0; i<session->drag_pluginservers->total; ++i ) {
		PluginServer *plugin = session->drag_pluginservers->values[i];
		insert_effect(plugin->title, 0, dest_track, 0,
			start, length, PLUGIN_STANDALONE);
	}

	save_backup();
	undo->update_undo_after(_("insert effect"), LOAD_EDITS | LOAD_PATCHES);
	restart_brender();
	sync_parameters(CHANGE_EDL);
	gui->update(1, NORMAL_DRAW, 0, 0, 1, 0, 0);
}

void MWindow::insert_effect(char *title,
	SharedLocation *shared_location,
	int data_type,
	int plugin_type,
	int single_standalone)
{
	Track *current = edl->tracks->first;
	SharedLocation shared_location_local;
	shared_location_local.copy_from(shared_location);
	int first_track = 1;
	for( ; current; current=NEXT ) {
		if( current->data_type == data_type &&
			current->record ) {
			insert_effect(title, &shared_location_local,
				current, 0, 0, 0, plugin_type);

			if( first_track ) {
				if( plugin_type == PLUGIN_STANDALONE && single_standalone ) {
					plugin_type = PLUGIN_SHAREDPLUGIN;
					shared_location_local.module = edl->tracks->number_of(current);
					shared_location_local.plugin = current->plugin_set.total - 1;
				}
				first_track = 0;
			}
		}
	}
}


void MWindow::insert_effect(char *title,
	SharedLocation *shared_location,
	Track *track,
	PluginSet *plugin_set,
	double start,
	double length,
	int plugin_type)
{
	KeyFrame *default_keyframe = 0;
	PluginServer *server = 0;
// Get default keyframe
	if( plugin_type == PLUGIN_STANDALONE ) {
		default_keyframe = new KeyFrame;
		server = new PluginServer(*scan_plugindb(title, track->data_type));

		server->open_plugin(0, preferences, edl, 0);
		server->save_data(default_keyframe);
	}
// Insert plugin object
	track->insert_effect(title, shared_location,
		default_keyframe, plugin_set,
		start, length, plugin_type);
	track->optimize();

	if( plugin_type == PLUGIN_STANDALONE ) {
		server->close_plugin();
		delete server;
		delete default_keyframe;
	}
}

int MWindow::modify_edithandles()
{
	undo->update_undo_before();
	edl->modify_edithandles(session->drag_start,
		session->drag_position,
		session->drag_handle,
		edl->session->edit_handle_mode[session->drag_button],
		edl->session->labels_follow_edits,
		edl->session->plugins_follow_edits,
		edl->session->autos_follow_edits);

	finish_modify_handles();
//printf("MWindow::modify_handles 1\n");
	return 0;
}

int MWindow::modify_pluginhandles()
{
	undo->update_undo_before();

	edl->modify_pluginhandles(session->drag_start,
		session->drag_position,
		session->drag_handle,
		edl->session->edit_handle_mode[session->drag_button],
		edl->session->labels_follow_edits,
		edl->session->autos_follow_edits,
		session->trim_edits);

	finish_modify_handles();

	return 0;
}


// Common to edithandles and plugin handles
void MWindow::finish_modify_handles()
{
	int edit_mode = edl->session->edit_handle_mode[session->drag_button];

	if( (session->drag_handle == 1 && edit_mode != MOVE_NO_EDITS) ||
		(session->drag_handle == 0 && edit_mode == MOVE_ONE_EDIT) ) {
//printf("MWindow::finish_modify_handles %d\n", __LINE__);
		edl->local_session->set_selectionstart(session->drag_position);
		edl->local_session->set_selectionend(session->drag_position);
	}
	else
	if( edit_mode != MOVE_NO_EDITS ) {
//printf("MWindow::finish_modify_handles %d\n", __LINE__);
		edl->local_session->set_selectionstart(session->drag_start);
		edl->local_session->set_selectionend(session->drag_start);
	}

// clamp the selection to 0
	if( edl->local_session->get_selectionstart(1) < 0 ) {
		edl->local_session->set_selectionstart(0);
		edl->local_session->set_selectionend(0);
	}
	undo->update_undo_after(_("drag handle"), LOAD_EDITS | LOAD_TIMEBAR);

	save_backup();
	restart_brender();
	sync_parameters(CHANGE_EDL);
	update_plugin_guis();
	gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 0);
// label list can be modified
	awindow->gui->async_update_assets();
	cwindow->update(1, 0, 0, 0, 1);
}

void MWindow::match_output_size(Track *track)
{
	undo->update_undo_before();
	track->track_w = edl->session->output_w;
	track->track_h = edl->session->output_h;
	save_backup();
	undo->update_undo_after(_("match output size"), LOAD_ALL);

	restart_brender();
	sync_parameters(CHANGE_EDL);
}


EDL *MWindow::selected_edits_to_clip(int packed,
		double *start_position, Track **start_track,
		int edit_labels, int edit_autos, int edit_plugins)
{
	double start = DBL_MAX, end = DBL_MIN;
	Track *first_track=0, *last_track = 0;
	for( Track *track=edl->tracks->first; track; track=track->next ) {
		if( !track->record ) continue;
		int empty = 1;
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			if( !edit->is_selected || edit->silence() ) continue;
			double edit_pos = track->from_units(edit->startproject);
			if( start > edit_pos ) start = edit_pos;
			if( end < (edit_pos+=edit->length) ) end = edit_pos;
			empty = 0;
		}
		if( empty ) continue;
		if( !first_track ) first_track = track;
		last_track = track;
	}
	if( start_position ) *start_position = start;
	if( start_track ) *start_track = first_track;
	if( !first_track ) return 0;
	EDL *new_edl = new EDL();
	new_edl->create_objects();
	new_edl->copy_session(edl);
	const char *text = _("new_edl edit");
	new_edl->set_path(text);
	strcpy(new_edl->local_session->clip_title, text);
	strcpy(new_edl->local_session->clip_notes, text);
	new_edl->session->video_tracks = 0;
	new_edl->session->audio_tracks = 0;
	for( Track *track=edl->tracks->first; track; track=track->next ) {
		if( !track->record ) continue;
		if( first_track ) {
			if( first_track != track ) continue;
			first_track = 0;
		}
		Track *new_track = 0;
		if( !packed )
			new_track = new_edl->add_new_track(track->data_type);
		int64_t start_pos = track->to_units(start, 0);
		int64_t end_pos = track->to_units(end, 0);
		int64_t startproject = 0;
		Edit *edit = track->edits->first;
		for( ; edit; edit=edit->next ) {
			if( !edit->is_selected || edit->silence() ) continue;
			if( edit->startproject < start_pos ) continue;
			if( edit->startproject >= end_pos ) break;
			int64_t edit_start_pos = edit->startproject;
			int64_t edit_end_pos = edit->startproject + edit->length;
			if( !new_track )
				new_track = new_edl->add_new_track(track->data_type);
			int64_t edit_pos = edit_start_pos - start_pos;
			if( !packed && edit_pos > startproject ) {
				Edit *silence = new Edit(new_edl, new_track);
				silence->startproject = startproject;
				silence->length = edit_pos - startproject;
				new_track->edits->append(silence);
				startproject = edit_pos;
			}
			int64_t clip_start_pos = startproject;
			Edit *clip_edit = new Edit(new_edl, new_track);
			clip_edit->copy_from(edit);
			clip_edit->startproject = startproject;
			startproject += clip_edit->length;
			new_track->edits->append(clip_edit);
			if( edit_labels ) {
				double edit_start = track->from_units(edit_start_pos);
				double edit_end = track->from_units(edit_end_pos);
				double clip_start = new_track->from_units(clip_start_pos);
				Label *label = edl->labels->first;
				for( ; label; label=label->next ) {
					if( label->position < edit_start ) continue;
					if( label->position >= edit_end ) break;
					double clip_position = label->position - edit_start + clip_start;
					Label *clip_label = new_edl->labels->first;
					while( clip_label && clip_label->position<clip_position )
						clip_label = clip_label->next;
					if( clip_label && clip_label->position == clip_position ) continue;
					Label *new_label = new Label(new_edl,
						new_edl->labels, clip_position, label->textstr);
					new_edl->labels->insert_before(clip_label, new_label);
				}
			}
			if( edit_autos ) {
				Automation *automation = track->automation;
				Automation *new_automation = new_track->automation;
				for( int i=0; i<AUTOMATION_TOTAL; ++i ) {
					Autos *autos = automation->autos[i];
					if( !autos ) continue;
					Autos *new_autos = new_automation->autos[i];
					new_autos->default_auto->copy_from(autos->default_auto);
					Auto *aut0 = autos->first;
					for( ; aut0; aut0=aut0->next ) {
						if( aut0->position < edit_start_pos ) continue;
						if( aut0->position >= edit_end_pos ) break;
						Auto *new_auto = new_autos->new_auto();
						new_auto->copy_from(aut0);
						int64_t clip_position = aut0->position - edit_start_pos + clip_start_pos;
						new_auto->position = clip_position;
						new_autos->append(new_auto);
					}
				}
			}
			if( edit_plugins ) {
				while( new_track->plugin_set.size() < track->plugin_set.size() )
					new_track->plugin_set.append(0);
				for( int i=0; i<track->plugin_set.total; ++i ) {
					PluginSet *plugin_set = track->plugin_set[i];
					if( !plugin_set ) continue;
					PluginSet *new_plugin_set = new_track->plugin_set[i];
					if( !new_plugin_set ) {
						new_plugin_set = new PluginSet(new_edl, new_track);
						new_track->plugin_set[i] = new_plugin_set;
					}
					Plugin *plugin = (Plugin*)plugin_set->first;
					int64_t startplugin = new_plugin_set->length();
					for( ; plugin ; plugin=(Plugin*)plugin->next ) {
						if( plugin->silence() ) continue;
						int64_t plugin_start_pos = plugin->startproject;
						int64_t plugin_end_pos = plugin_start_pos + plugin->length;
						if( plugin_end_pos < start_pos ) continue;
						if( plugin_start_pos > end_pos ) break;
						if( plugin_start_pos < edit_start_pos )
							plugin_start_pos = edit_start_pos;
						if( plugin_end_pos > edit_end_pos )
							plugin_end_pos = edit_end_pos;
						if( plugin_start_pos >= plugin_end_pos ) continue;
						int64_t plugin_pos = plugin_start_pos - start_pos;
						if( !packed && plugin_pos > startplugin ) {
							Plugin *silence = new Plugin(new_edl, new_track, "");
							silence->startproject = startplugin;
							silence->length = plugin_pos - startplugin;
							new_plugin_set->append(silence);
							startplugin = plugin_pos;
						}
						Plugin *new_plugin = new Plugin(new_edl, new_track, plugin->title);
						new_plugin->copy_base(plugin);
						new_plugin->startproject = startplugin;
						new_plugin->length = plugin_end_pos - plugin_start_pos;
						startplugin += new_plugin->length;
						new_plugin_set->append(new_plugin);
						KeyFrames *keyframes = plugin->keyframes;
						KeyFrames *new_keyframes = new_plugin->keyframes;
						new_keyframes->default_auto->copy_from(keyframes->default_auto);
						new_keyframes->default_auto->position = new_plugin->startproject;
						KeyFrame *keyframe = (KeyFrame*)keyframes->first;
						for( ; keyframe; keyframe=(KeyFrame*)keyframe->next ) {
							if( keyframe->position < edit_start_pos ) continue;
							if( keyframe->position >= edit_end_pos ) break;
							KeyFrame *clip_keyframe = new KeyFrame(new_edl, new_keyframes);
							clip_keyframe->copy_from(keyframe);
							int64_t key_position = keyframe->position - start_pos;
							if( packed )
								key_position += new_plugin->startproject - plugin_pos;
							clip_keyframe->position = key_position;
							new_keyframes->append(clip_keyframe);
						}
					}
				}
			}
		}
		if( last_track == track ) break;
	}
	return new_edl;
}

void MWindow::selected_edits_to_clipboard(int packed)
{
	EDL *new_edl = selected_edits_to_clip(packed, 0, 0,
		edl->session->labels_follow_edits,
		edl->session->autos_follow_edits,
		edl->session->plugins_follow_edits);
	if( !new_edl ) return;
	double length = new_edl->tracks->total_length();
	FileXML file;
	new_edl->copy(0, length, 1, &file, "", 1);
	const char *file_string = file.string();
	long file_length = strlen(file_string);
	gui->to_clipboard(file_string, file_length, BC_PRIMARY_SELECTION);
	gui->to_clipboard(file_string, file_length, SECONDARY_SELECTION);
	new_edl->remove_user();
}

void MWindow::delete_edit(Edit *edit, const char *msg, int collapse)
{
	ArrayList<Edit*> edits;
	edits.append(edit);
	delete_edits(&edits, msg, collapse);
}

void MWindow::delete_edits(ArrayList<Edit*> *edits, const char *msg, int collapse)
{
	if( !edits->size() ) return;
	undo->update_undo_before();
	edl->delete_edits(edits, collapse);
	save_backup();
	undo->update_undo_after(msg, LOAD_EDITS);

	restart_brender();
	cwindow->refresh_frame(CHANGE_EDL);
	update_plugin_guis();
	gui->update(1, NORMAL_DRAW, 1, 0, 0, 0, 0);
}

void MWindow::delete_edits(int collapse)
{
	ArrayList<Edit*> edits;
	edl->tracks->get_selected_edits(&edits);
	delete_edits(&edits,_("del edit"), collapse);
}

// collapse - delete from timeline, not collapse replace with silence
// packed - omit unselected from selection, unpacked - replace unselected with silence
void MWindow::cut_selected_edits(int collapse, int packed)
{
	selected_edits_to_clipboard(packed);
	ArrayList<Edit*> edits;
	edl->tracks->get_selected_edits(&edits);
	delete_edits(&edits, _("cut edit"), collapse);
}


void MWindow::move_edits(ArrayList<Edit*> *edits,
		Track *track,
		double position,
		int behaviour)
{
	undo->update_undo_before();

	edl->tracks->move_edits(edits,
		track,
		position,
		edl->session->labels_follow_edits,
		edl->session->plugins_follow_edits,
		edl->session->autos_follow_edits,
		behaviour);

	save_backup();
	undo->update_undo_after(_("move edit"), LOAD_ALL);

	restart_brender();
	cwindow->refresh_frame(CHANGE_EDL);

	update_plugin_guis();
	gui->update(1, NORMAL_DRAW, 1, 0, 0, 0, 0);
}

void MWindow::paste_edits(EDL *clip, Track *first_track, double position, int overwrite,
		int edit_edits, int edit_labels, int edit_autos, int edit_plugins)
{
	if( edit_labels ) {
		Label *edl_label = edl->labels->first;
		for( Label *label=clip->labels->first; label; label=label->next ) {
			double label_pos = position + label->position;
			int exists = 0;
			while( edl_label &&
				!(exists=edl->equivalent(edl_label->position, label_pos)) &&
				edl_label->position < position ) edl_label = edl_label->next;
			if( exists ) continue;
			edl->labels->insert_before(edl_label,
				new Label(edl, edl->labels, label_pos, label->textstr));
		}
	}

	if( !first_track )
		first_track = edl->tracks->first;
	Track *src = clip->tracks->first;
	for( Track *track=first_track; track && src; track=track->next ) {
		if( !track->record ) continue;
		int64_t pos = track->to_units(position, 0);
		if( edit_edits ) {
			for( Edit *edit=src->edits->first; edit; edit=edit->next ) {
				if( edit->silence() ) continue;
				int64_t start = pos + edit->startproject;
				int64_t end = start + edit->length;
				if( overwrite )
					track->edits->clear(start, end);
				Edit *dst = track->edits->insert_new_edit(start);
				dst->copy_from(edit);
				dst->startproject = start;
				dst->is_selected = 1;
				while( (dst=dst->next) != 0 )
					dst->startproject += edit->length;
			}
		}
		if( edit_autos ) {
			for( int i=0; i<AUTOMATION_TOTAL; ++i ) {
				Autos *src_autos = src->automation->autos[i];
				if( !src_autos ) continue;
				Autos *autos = track->automation->autos[i];
				for( Auto *aut0=src_autos->first; aut0; aut0=aut0->next ) {
					int64_t auto_pos = pos + aut0->position;
					autos->insert_auto(auto_pos, aut0);
				}
			}
		}
		if( edit_plugins ) {
			for( int i=0; i<src->plugin_set.size(); ++i ) {
				PluginSet *plugin_set = src->plugin_set[i];
				if( !plugin_set ) continue;
				while( i >= track->plugin_set.size() )
					track->plugin_set.append(0);
				PluginSet *dst_plugin_set = track->plugin_set[i];
				if( !dst_plugin_set ) {
					dst_plugin_set = new PluginSet(edl, track);
					track->plugin_set[i] = dst_plugin_set;
				}
				Plugin *plugin = (Plugin *)plugin_set->first;
				if( plugin ) track->expand_view = 1;
				for( ; plugin; plugin=(Plugin *)plugin->next ) {
					int64_t start = pos + plugin->startproject;
					int64_t end = start + plugin->length;
					if( overwrite )
						dst_plugin_set->clear(start, end, 1);
					Plugin *dst = dst_plugin_set->insert_plugin(
						plugin->title, start, end-start,
						plugin->plugin_type, &plugin->shared_location,
						(KeyFrame*)plugin->keyframes->default_auto, 0);
					KeyFrame *keyframe = (KeyFrame*)plugin->keyframes->first;
					for( ; keyframe; keyframe=(KeyFrame*)keyframe->next ) {
						int64_t keyframe_pos = pos + keyframe->position;
						dst->keyframes->insert_auto(keyframe_pos, keyframe);
					}
				}
			}
		}
		track->optimize();
		src = src->next;
	}
}

void MWindow::paste_clipboard(Track *first_track, double position, int overwrite,
		int edit_edits, int edit_labels, int edit_autos, int edit_plugins)
{
	int64_t len = gui->clipboard_len(BC_PRIMARY_SELECTION);
	if( !len ) return;
	char *string = new char[len];
	gui->from_clipboard(string, len, BC_PRIMARY_SELECTION);
	FileXML file;
	file.read_from_string(string);
	delete [] string;
	EDL *clip = new EDL();
	clip->create_objects();
	if( !clip->load_xml(&file, LOAD_ALL) ) {
		undo->update_undo_before();
		paste_edits(clip, first_track, position, overwrite,
			edit_edits, edit_labels, edit_autos, edit_plugins);
		save_backup();
		undo->update_undo_after(_("paste clip"), LOAD_ALL);
		restart_brender();
		cwindow->refresh_frame(CHANGE_EDL);

		update_plugin_guis();
		gui->update(1, NORMAL_DRAW, 1, 0, 0, 0, 0);
	}
	clip->remove_user();
}

void MWindow::move_group(EDL *group, Track *first_track, double position)
{
	undo->update_undo_before();

	ArrayList<Edit *>edits;
	edl->tracks->get_selected_edits(&edits);
	edl->delete_edits(&edits, 0);
	paste_edits(group, first_track, position, 1, 1, 1, 1, 1);
// big debate over whether to do this, must either clear selected, or no tweaking
//	edl->tracks->clear_selected_edits();

	save_backup();
	undo->update_undo_after(_("move group"), LOAD_ALL);
	restart_brender();
	cwindow->refresh_frame(CHANGE_EDL);

	update_plugin_guis();
	gui->update(1, NORMAL_DRAW, 1, 0, 0, 0, 0);
}

void MWindow::move_effect(Plugin *plugin, Track *track, int64_t position)
{
	undo->update_undo_before();
	edl->tracks->move_effect(plugin, track, position);
	save_backup();
	undo->update_undo_after(_("paste effect"), LOAD_ALL);

	restart_brender();
	cwindow->refresh_frame(CHANGE_EDL);
	update_plugin_guis();
	gui->update(1, NORMAL_DRAW, 0, 0, 0, 0, 0);
}

void MWindow::move_effect(Plugin *plugin, PluginSet *plugin_set, int64_t position)
{
	undo->update_undo_before();
	edl->tracks->move_effect(plugin, plugin_set, position);
	save_backup();
	undo->update_undo_after(_("move effect"), LOAD_ALL);

	restart_brender();
	cwindow->refresh_frame(CHANGE_EDL);
	update_plugin_guis();
	gui->update(1, NORMAL_DRAW, 0, 0, 0, 0, 0);
}

void MWindow::move_plugins_up(PluginSet *plugin_set)
{

	undo->update_undo_before();
	plugin_set->track->move_plugins_up(plugin_set);

	save_backup();
	undo->update_undo_after(_("move effect up"), LOAD_ALL);
	restart_brender();
	gui->update(1, NORMAL_DRAW, 0, 0, 0, 0, 0);
	sync_parameters(CHANGE_EDL);
}

void MWindow::move_plugins_down(PluginSet *plugin_set)
{
	undo->update_undo_before();

	plugin_set->track->move_plugins_down(plugin_set);

	save_backup();
	undo->update_undo_after(_("move effect down"), LOAD_ALL);
	restart_brender();
	gui->update(1, NORMAL_DRAW, 0, 0, 0, 0, 0);
	sync_parameters(CHANGE_EDL);
}

void MWindow::move_track_down(Track *track)
{
	undo->update_undo_before();
	edl->tracks->move_track_down(track);
	save_backup();
	undo->update_undo_after(_("move track down"), LOAD_ALL);

	restart_brender();
	gui->update(1, NORMAL_DRAW, 0, 0, 1, 0, 0);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_tracks_down()
{
	undo->update_undo_before();
	edl->tracks->move_tracks_down();
	save_backup();
	undo->update_undo_after(_("move tracks down"), LOAD_ALL);

	restart_brender();
	gui->update(1, NORMAL_DRAW, 0, 0, 1, 0, 0);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_track_up(Track *track)
{
	undo->update_undo_before();
	edl->tracks->move_track_up(track);
	save_backup();
	undo->update_undo_after(_("move track up"), LOAD_ALL);
	restart_brender();
	gui->update(1, NORMAL_DRAW, 0, 0, 1, 0, 0);
	sync_parameters(CHANGE_EDL);
	save_backup();
}

void MWindow::move_tracks_up()
{
	undo->update_undo_before();
	edl->tracks->move_tracks_up();
	save_backup();
	undo->update_undo_after(_("move tracks up"), LOAD_ALL);
	restart_brender();
	gui->update(1, NORMAL_DRAW, 0, 0, 1, 0, 0);
	sync_parameters(CHANGE_EDL);
}


void MWindow::mute_selection()
{
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	if( start != end ) {
		undo->update_undo_before();
		edl->clear(start, end, 0,
			edl->session->plugins_follow_edits,
			edl->session->autos_follow_edits);
		edl->local_session->set_selectionend(end);
		edl->local_session->set_selectionstart(start);
		edl->paste_silence(start, end, 0,
			edl->session->plugins_follow_edits,
			edl->session->autos_follow_edits);

		save_backup();
		undo->update_undo_after(_("mute"), LOAD_EDITS);

		restart_brender();
		update_plugin_guis();
		gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 0);
		cwindow->refresh_frame(CHANGE_EDL);
	}
}


void MWindow::overwrite(EDL *source, int all)
{
	FileXML file;

	LocalSession *src = source->local_session;
	double src_start = all ? 0 :
		src->inpoint_valid() ? src->get_inpoint() :
		src->outpoint_valid() ? 0 :
			src->get_selectionstart();
	double src_end = all ? source->tracks->total_length() :
		src->outpoint_valid() ? src->get_outpoint() :
		src->inpoint_valid() ? source->tracks->total_length() :
			src->get_selectionend();
	double overwrite_len = src_end - src_start;
	double dst_start = edl->local_session->get_selectionstart();
	double dst_len = edl->local_session->get_selectionend() - dst_start;

	undo->update_undo_before();
	if( !EQUIV(dst_len, 0) && (dst_len < overwrite_len) ) {
// in/out points or selection present and shorter than overwrite range
// shorten the copy range
		overwrite_len = dst_len;
	}

	source->copy(src_start, src_start + overwrite_len, 0, &file, "", 1);

// HACK around paste_edl get_start/endselection on its own
// so we need to clear only when not using both io points
// FIXME: need to write simple overwrite_edl to be used for overwrite function
	if( edl->local_session->get_inpoint() < 0 ||
		edl->local_session->get_outpoint() < 0 )
		edl->clear(dst_start, dst_start + overwrite_len, 0, 0, 0);

	paste(dst_start, dst_start + overwrite_len, &file, 0, 0, 0, 0, 0);

	edl->local_session->set_selectionstart(dst_start + overwrite_len);
	edl->local_session->set_selectionend(dst_start + overwrite_len);

	save_backup();
	undo->update_undo_after(_("overwrite"), LOAD_EDITS);

	restart_brender();
	update_plugin_guis();
	gui->update(1, NORMAL_DRAW, 1, 1, 0, 1, 0);
	sync_parameters(CHANGE_EDL);
}

// For splice and overwrite
int MWindow::paste(double start, double end, FileXML *file,
	int edit_labels, int edit_plugins, int edit_autos,
	Track *first_track, int overwrite)
{
	clear(0);

// Want to insert with assets shared with the master EDL.
	insert(start, file,
		edit_labels, edit_plugins, edit_autos,
		edl, first_track, overwrite);

	return 0;
}

// For editing using insertion point
void MWindow::paste()
{
	paste(edl->local_session->get_selectionstart(), 0, 1, 0);
}

void MWindow::paste(double start, Track *first_track, int clear_selection, int overwrite)
{
	//double end = edl->local_session->get_selectionend();
	int64_t len = gui->clipboard_len(BC_PRIMARY_SELECTION);

	if( len ) {
		char *string = new char[len];
		undo->update_undo_before();
		gui->from_clipboard(string, len, BC_PRIMARY_SELECTION);
		FileXML file;
		file.read_from_string(string);
		if( clear_selection ) clear(0);

		insert(start, &file,
			edl->session->labels_follow_edits,
			edl->session->plugins_follow_edits,
			edl->session->autos_follow_edits,
			0, first_track, overwrite);

		edl->optimize();
		delete [] string;

		save_backup();

		undo->update_undo_after(_("paste"), LOAD_EDITS | LOAD_TIMEBAR);
		restart_brender();
		update_plugin_guis();
		gui->update(1, FORCE_REDRAW, 1, 1, 0, 1, 0);
		awindow->gui->async_update_assets();
		sync_parameters(CHANGE_EDL);
	}

}

int MWindow::paste_assets(double position, Track *dest_track, int overwrite)
{
	int result = 0;
	undo->update_undo_before();

	if( session->drag_assets->total ) {
		load_assets(session->drag_assets,
			position, LOADMODE_PASTE, dest_track, 0,
			edl->session->labels_follow_edits,
			edl->session->plugins_follow_edits,
			edl->session->autos_follow_edits,
			overwrite);
		result = 1;
	}

	if( session->drag_clips->total ) {
		paste_edls(session->drag_clips,
			LOADMODE_PASTE, dest_track, position,
			edl->session->labels_follow_edits,
			edl->session->plugins_follow_edits,
			edl->session->autos_follow_edits,
			overwrite);
		result = 1;
	}

	save_backup();

	undo->update_undo_after(_("paste assets"), LOAD_EDITS);
	restart_brender();
	gui->update(1, FORCE_REDRAW, 1, 0, 0, 1, 0);
	sync_parameters(CHANGE_EDL);
	return result;
}

void MWindow::load_assets(ArrayList<Indexable*> *new_assets,
	double position, int load_mode, Track *first_track, RecordLabels *labels,
	int edit_labels, int edit_plugins, int edit_autos, int overwrite)
{
	if( load_mode == LOADMODE_RESOURCESONLY )
		load_mode = LOADMODE_ASSETSONLY;
const int debug = 0;
if( debug ) printf("MWindow::load_assets %d\n", __LINE__);
	if( position < 0 ) position = edl->local_session->get_selectionstart();

	ArrayList<EDL*> new_edls;
	for( int i=0; i<new_assets->total; ++i ) {
		Indexable *indexable = new_assets->get(i);
		if( indexable->is_asset ) {
			remove_asset_from_caches((Asset*)indexable);
		}
		EDL *new_edl = new EDL;
		new_edl->create_objects();
		new_edl->copy_session(edl);
		if( !indexable->is_asset ) {
			EDL *nested_edl = (EDL*)indexable;
			new_edl->create_nested(nested_edl);
			new_edl->set_path(indexable->path);
		}
		else {
			Asset *asset = (Asset*)indexable;
			asset_to_edl(new_edl, asset);
		}
		new_edls.append(new_edl);

		if( labels ) {
			for( RecordLabel *label=labels->first; label; label=label->next ) {
				new_edl->labels->toggle_label(label->position, label->position);
			}
		}
	}
if( debug ) printf("MWindow::load_assets %d\n", __LINE__);

	paste_edls(&new_edls, load_mode, first_track, position,
		edit_labels, edit_plugins, edit_autos, overwrite);
if( debug ) printf("MWindow::load_assets %d\n", __LINE__);

	save_backup();
	for( int i=0; i<new_edls.size(); ++i )
		new_edls.get(i)->Garbage::remove_user();

if( debug ) printf("MWindow::load_assets %d\n", __LINE__);
}

int MWindow::paste_automation()
{
	int64_t len = gui->clipboard_len(BC_PRIMARY_SELECTION);

	if( len ) {
		undo->update_undo_before();
		speed_before();
		char *string = new char[len];
		gui->from_clipboard(string, len, BC_PRIMARY_SELECTION);
		FileXML file;
		file.read_from_string(string);
		delete [] string;
		double start = edl->local_session->get_selectionstart();
		double end = edl->local_session->get_selectionend();
		edl->tracks->clear_automation(start, end);
		edl->tracks->paste_automation(start, &file, 0, 1,
			edl->session->typeless_keyframes);
		int changed_edl = speed_after(1);
		save_backup();
		undo->update_undo_after(_("paste keyframes"),
			!changed_edl ? LOAD_AUTOMATION :
				LOAD_AUTOMATION + LOAD_EDITS + LOAD_TIMEBAR);
		update_gui(changed_edl);
	}

	return 0;
}

int MWindow::paste_default_keyframe()
{
	int64_t len = gui->clipboard_len(BC_PRIMARY_SELECTION);

	if( len ) {
		undo->update_undo_before();
		speed_before();
		char *string = new char[len];
		gui->from_clipboard(string, len, BC_PRIMARY_SELECTION);
		FileXML file;
		file.read_from_string(string);
		delete [] string;
		double start = edl->local_session->get_selectionstart();
		edl->tracks->paste_automation(start, &file, 1, 0,
			edl->session->typeless_keyframes);
//		edl->tracks->paste_default_keyframe(&file);
		int changed_edl = speed_after(1);
		undo->update_undo_after(_("paste default keyframe"),
			!changed_edl ? LOAD_AUTOMATION :
				LOAD_AUTOMATION + LOAD_EDITS + LOAD_TIMEBAR);
		save_backup();
		update_gui(changed_edl);
	}

	return 0;
}


// Insert edls with project deletion and index file generation.
int MWindow::paste_edls(ArrayList<EDL*> *new_edls, int load_mode,
	Track *first_track, double current_position,
	int edit_labels, int edit_plugins, int edit_autos,
	int overwrite)
{

	ArrayList<Track*> destination_tracks;
	int need_new_tracks = 0;

//PRINT_TRACE
	if( !new_edls->total ) return 0;

//PRINT_TRACE
//	double original_length = edl->tracks->total_length();
//	double original_preview_end = edl->local_session->preview_end;
//PRINT_TRACE

// Delete current project
	if( load_mode == LOADMODE_REPLACE ||
	    load_mode == LOADMODE_REPLACE_CONCATENATE ) {
		reset_caches();
		edl->save_defaults(defaults);
		hide_plugins();
		edl->Garbage::remove_user();
		edl = new EDL;
		edl->create_objects();
		edl->copy_session(new_edls->values[0]);
		edl->copy_mixers(new_edls->values[0]);
		gui->mainmenu->update_toggles(0);
		gui->unlock_window();
		gwindow->gui->update_toggles(1);
		gui->lock_window("MWindow::paste_edls");

// Insert labels for certain modes constitutively
		edit_labels = 1;
		edit_plugins = 1;
		edit_autos = 1;
// Force reset of preview
//		original_length = 0;
//		original_preview_end = -1;
	}


//PRINT_TRACE

// Create new tracks in master EDL
	if( load_mode == LOADMODE_REPLACE ||
	    load_mode == LOADMODE_REPLACE_CONCATENATE ||
	    load_mode == LOADMODE_NEW_TRACKS ) {

		need_new_tracks = 1;
		for( int i=0; i<new_edls->total; ++i ) {
			EDL *new_edl = new_edls->values[i];
			for( Track *current=new_edl->tracks->first; current; current=NEXT ) {
				switch( current->data_type ) {
				case TRACK_VIDEO:
					edl->tracks->add_video_track(0, 0);
					if( current->draw ) edl->tracks->last->draw = 1;
					break;
				case TRACK_AUDIO:
					edl->tracks->add_audio_track(0, 0);
					break;
				case TRACK_SUBTITLE:
					edl->tracks->add_subttl_track(0, 0);
					break;
				default:
					continue;
				}
				destination_tracks.append(edl->tracks->last);
			}

// Base track count on first EDL only for concatenation
			if( load_mode == LOADMODE_REPLACE_CONCATENATE ) break;
		}
		edl->session->highlighted_track = edl->tracks->total() - 1;
	}
	else
// Recycle existing tracks of master EDL
	if( load_mode == LOADMODE_CONCATENATE ||
	    load_mode == LOADMODE_PASTE ||
	    load_mode == LOADMODE_NESTED ) {
//PRINT_TRACE

// The point of this is to shift forward labels after the selection so they can
// then be shifted back to their original locations without recursively
// shifting back every paste.
		if( (load_mode == LOADMODE_PASTE || load_mode == LOADMODE_NESTED) &&
		    edl->session->labels_follow_edits )
			edl->labels->clear(edl->local_session->get_selectionstart(),
					   edl->local_session->get_selectionend(), 1);

		Track *current = first_track ? first_track : edl->tracks->first;
		for( ; current; current=NEXT ) {
			if( current->record ) {
				destination_tracks.append(current);
			}
		}
//PRINT_TRACE

	}
//PRINT_TRACE
	int destination_track = 0;
	double *paste_position = new double[destination_tracks.total];

// Iterate through the edls
	for( int i=0; i<new_edls->total; ++i ) {

		EDL *new_edl = new_edls->values[i];
		double edl_length = new_edl->local_session->clipboard_length ?
			new_edl->local_session->clipboard_length :
			new_edl->tracks->total_length();
// printf("MWindow::paste_edls 2 %f %f\n",
// new_edl->local_session->clipboard_length,
// new_edl->tracks->total_length());
// new_edl->dump();
//PRINT_TRACE

// Convert EDL to master rates
		new_edl->resample(new_edl->session->sample_rate,
			edl->session->sample_rate,
			TRACK_AUDIO);
		new_edl->resample(new_edl->session->frame_rate,
			edl->session->frame_rate,
			TRACK_VIDEO);
//PRINT_TRACE
// Add assets and prepare index files
		for( Asset *new_asset=new_edl->assets->first;
		     new_asset; new_asset=new_asset->next ) {
			mainindexes->add_next_asset(0, new_asset);
		}
// Capture index file status from mainindex test
		edl->update_assets(new_edl);
//PRINT_TRACE
// Get starting point of insertion.  Need this to paste labels.
		switch( load_mode ) {
		case LOADMODE_REPLACE:
		case LOADMODE_NEW_TRACKS:
			current_position = 0;
			break;

		case LOADMODE_CONCATENATE:
		case LOADMODE_REPLACE_CONCATENATE:
			destination_track = 0;
	  	 	if( destination_tracks.total )
				current_position = destination_tracks.values[0]->get_length();
			else
				current_position = 0;
			break;

		case LOADMODE_PASTE:
		case LOADMODE_NESTED:
			destination_track = 0;
			if( i == 0 ) {
				for( int j=0; j<destination_tracks.total; ++j ) {
					paste_position[j] = (current_position >= 0) ?
						current_position :
						edl->local_session->get_selectionstart();
				}
			}
			break;

		case LOADMODE_RESOURCESONLY:
			edl->add_clip(new_edl);
			break;
		}
//PRINT_TRACE
// Insert edl
		if( load_mode != LOADMODE_RESOURCESONLY &&
		    load_mode != LOADMODE_ASSETSONLY ) {
// Insert labels
//printf("MWindow::paste_edls %f %f\n", current_position, edl_length);
			if( load_mode == LOADMODE_PASTE ||
			    load_mode == LOADMODE_NESTED )
				edl->labels->insert_labels(new_edl->labels,
					destination_tracks.total ? paste_position[0] : 0.0,
					edl_length,
					edit_labels);
			else
				edl->labels->insert_labels(new_edl->labels,
					current_position,
					edl_length,
					edit_labels);
//PRINT_TRACE
			double total_length = new_edl->tracks->total_length();
			for( Track *new_track=new_edl->tracks->first;
			     new_track; new_track=new_track->next ) {
// Get destination track of same type as new_track
				for( int k = 0;
				     k < destination_tracks.total &&
				     destination_tracks.values[destination_track]->data_type != new_track->data_type;
				     ++k, ++destination_track ) {
					if( destination_track >= destination_tracks.total - 1 )
						destination_track = 0;
				}

// Insert data into destination track
				if( destination_track < destination_tracks.total &&
				    destination_tracks.values[destination_track]->data_type == new_track->data_type ) {
					Track *track = destination_tracks.values[destination_track];

// Replace default keyframes if first EDL and new tracks were created.
// This means data copied from one track and pasted to another won't retain
// the camera position unless it's a keyframe.  If it did, previous data in the
// track might get unknowingly corrupted.  Ideally we would detect when differing
// default keyframes existed and create discrete keyframes for both.
					int replace_default = (i == 0) && need_new_tracks;

//printf("MWindow::paste_edls 1 %d\n", replace_default);
// Insert new track at current position
					switch( load_mode ) {
					case LOADMODE_REPLACE_CONCATENATE:
					case LOADMODE_CONCATENATE:
						current_position = track->get_length();
						break;

					case LOADMODE_PASTE:
					case LOADMODE_NESTED:
						current_position = paste_position[destination_track];
						paste_position[destination_track] += new_track->get_length();
						break;
					}
					if( overwrite ) {
						double length = overwrite >= 0 ?
							new_track->get_length() : total_length;
						track->clear(current_position,
							current_position + length,
							1, // edit edits
							edit_labels, edit_plugins, edit_autos,
							0); // trim edits
					}
//PRINT_TRACE
					track->insert_track(new_track, current_position, replace_default,
						edit_plugins, edit_autos, edl_length);
//PRINT_TRACE
				}

// Get next destination track
				destination_track++;
				if( destination_track >= destination_tracks.total )
					destination_track = 0;
			}
		}

		if( load_mode == LOADMODE_PASTE ||
		    load_mode == LOADMODE_NESTED )
			current_position += edl_length;
	}


// Move loading of clips and vwindow to the end - this fixes some
// strange issue, for index not being shown
// Assume any paste operation from the same EDL won't contain any clips.
// If it did it would duplicate every clip here.
	for( int i=0; i<new_edls->total; ++i ) {
		EDL *new_edl = new_edls->get(i);

		for( int j=0; j<new_edl->clips.size(); ++j ) {
			edl->add_clip(new_edl->clips[j]);
		}
		for( int j=0; j<new_edl->nested_edls.size(); ++j ) {
			edl->nested_edls.get_nested(new_edl->nested_edls[j]);
		}

		if( new_edl->total_vwindow_edls() ) {
//			if( edl->vwindow_edl )
//				edl->vwindow_edl->Garbage::remove_user();
//			edl->vwindow_edl = new EDL(edl);
//			edl->vwindow_edl->create_objects();
//			edl->vwindow_edl->copy_all(new_edl->vwindow_edl);

			for( int j=0; j<new_edl->total_vwindow_edls(); ++j ) {
				EDL *vwindow_edl = new EDL(edl);
				vwindow_edl->create_objects();
				vwindow_edl->copy_all(new_edl->get_vwindow_edl(j));
				edl->append_vwindow_edl(vwindow_edl, 0);
			}
		}
	}

	if( paste_position ) delete [] paste_position;

// This is already done in load_filenames and everything else that uses paste_edls
//	update_project(load_mode);

// Fix preview range
//	if( EQUIV(original_length, original_preview_end) )
//	{
//		edl->local_session->preview_end = edl->tracks->total_length();
//	}

// Start examining next batch of index files
	mainindexes->start_build();

// Don't save a backup after loading since the loaded file is on disk already.
//PRINT_TRACE
	return 0;
}

void MWindow::paste_silence()
{
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	if( EQUIV(start, end) ) {
		if( edl->session->frame_rate > 0 )
			end += 1./edl->session->frame_rate;
	}
	undo->update_undo_before(_("silence"), this);
	edl->paste_silence(start, end,
		edl->session->labels_follow_edits,
		edl->session->plugins_follow_edits,
		edl->session->autos_follow_edits);
	edl->optimize();
	save_backup();
	undo->update_undo_after(_("silence"), LOAD_EDITS | LOAD_TIMEBAR);

	update_plugin_guis();
	restart_brender();
	gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 0);
	cwindow->update(1, 0, 0, 0, 1);
	cwindow->refresh_frame(CHANGE_EDL);
}

void MWindow::detach_transition(Transition *transition)
{
	undo->update_undo_before();
	hide_plugin(transition, 1);
	int is_video = (transition->edit->track->data_type == TRACK_VIDEO);
	transition->edit->detach_transition();
	save_backup();
	undo->update_undo_after(_("detach transition"), LOAD_ALL);

	if( is_video ) restart_brender();
	gui->update(0, NORMAL_DRAW, 0, 0, 0, 0, 0);
	sync_parameters(CHANGE_EDL);
}

void MWindow::detach_transitions()
{
	gui->lock_window("MWindow::detach_transitions 1");

	undo->update_undo_before();
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	edl->tracks->clear_transitions(start, end);

	save_backup();
	undo->update_undo_after(_("detach transitions"), LOAD_EDITS);

	sync_parameters(CHANGE_EDL);
	gui->update(0, NORMAL_DRAW, 0, 0, 0, 0, 0);
	gui->unlock_window();
}

void MWindow::paste_transition()
{
// Only the first transition gets dropped.
 	PluginServer *server = session->drag_pluginservers->values[0];

	undo->update_undo_before();
	edl->tracks->paste_transition(server, session->edit_highlighted);
	save_backup();
	undo->update_undo_after(_("transition"), LOAD_EDITS);

	if( server->video ) restart_brender();
	sync_parameters(CHANGE_ALL);
}

void MWindow::paste_transitions(int track_type, char *title)
{
	gui->lock_window("MWindow::detach_transitions 1");

	undo->update_undo_before();
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	edl->tracks->paste_transitions(start, end, track_type, title);
	save_backup();
	undo->update_undo_after(_("attach transitions"), LOAD_EDITS);

	sync_parameters(CHANGE_EDL);
	gui->update(0, NORMAL_DRAW, 0, 0, 0, 0, 0);
	gui->unlock_window();
}

void MWindow::paste_transition_cwindow(Track *dest_track)
{
	PluginServer *server = session->drag_pluginservers->values[0];
	undo->update_undo_before();
	edl->tracks->paste_video_transition(server, 1);
	save_backup();
	undo->update_undo_after(_("transition"), LOAD_EDITS);
	restart_brender();
	gui->update(0, NORMAL_DRAW, 0, 0, 0, 0, 0);
	sync_parameters(CHANGE_ALL);
}

void MWindow::paste_audio_transition()
{
 	PluginServer *server = scan_plugindb(edl->session->default_atransition,
		TRACK_AUDIO);
	if( !server ) {
		char string[BCTEXTLEN];
		sprintf(string, _("No default transition %s found."), edl->session->default_atransition);
		gui->show_message(string);
		return;
	}

	undo->update_undo_before();
	edl->tracks->paste_audio_transition(server);
	save_backup();
	undo->update_undo_after(_("transition"), LOAD_EDITS);

	sync_parameters(CHANGE_EDL);
	gui->update(0, NORMAL_DRAW, 0, 0, 0, 0, 0);
}

void MWindow::paste_video_transition()
{
 	PluginServer *server = scan_plugindb(edl->session->default_vtransition,
		TRACK_VIDEO);
	if( !server ) {
		char string[BCTEXTLEN];
		sprintf(string, _("No default transition %s found."), edl->session->default_vtransition);
		gui->show_message(string);
		return;
	}

	undo->update_undo_before();

	edl->tracks->paste_video_transition(server);
	save_backup();
	undo->update_undo_after(_("transition"), LOAD_EDITS);

	sync_parameters(CHANGE_EDL);
	restart_brender();
	gui->update(0, NORMAL_DRAW, 0, 0, 0, 0, 0);
}

void MWindow::shuffle_edits()
{
	gui->lock_window("MWindow::shuffle_edits 1");

	undo->update_undo_before();
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();

	edl->tracks->shuffle_edits(start, end);

	save_backup();
	undo->update_undo_after(_("shuffle edits"), LOAD_EDITS | LOAD_TIMEBAR);

	sync_parameters(CHANGE_EDL);
	restart_brender();
	gui->update(0, NORMAL_DRAW, 1, 0, 0, 0, 0);
	gui->unlock_window();
}

void MWindow::reverse_edits()
{
	gui->lock_window("MWindow::reverse_edits 1");

	undo->update_undo_before();
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();

	edl->tracks->reverse_edits(start, end);

	save_backup();
	undo->update_undo_after(_("reverse edits"), LOAD_EDITS | LOAD_TIMEBAR);

	sync_parameters(CHANGE_EDL);
	restart_brender();
	gui->update(0, NORMAL_DRAW, 1, 0, 0, 0, 0);
	gui->unlock_window();
}

void MWindow::align_edits()
{
	gui->lock_window("MWindow::align_edits 1");

	undo->update_undo_before();
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();

	edl->tracks->align_edits(start, end);

	save_backup();
	undo->update_undo_after(_("align edits"), LOAD_EDITS | LOAD_TIMEBAR);

	sync_parameters(CHANGE_EDL);
	restart_brender();
	gui->update(0, NORMAL_DRAW, 1, 0, 0, 0, 0);
	gui->unlock_window();
}

void MWindow::set_edit_length(double length)
{
	gui->lock_window("MWindow::set_edit_length 1");

	undo->update_undo_before();
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();

	edl->tracks->set_edit_length(start, end, length);

	save_backup();
	undo->update_undo_after(_("edit length"), LOAD_EDITS | LOAD_TIMEBAR);

	sync_parameters(CHANGE_EDL);
	restart_brender();
	gui->update(0, NORMAL_DRAW, 1, 0, 0, 0, 0);
	gui->unlock_window();
}


void MWindow::set_transition_length(Transition *transition, double length)
{
	gui->lock_window("MWindow::set_transition_length 1");

	undo->update_undo_before();
	//double start = edl->local_session->get_selectionstart();
	//double end = edl->local_session->get_selectionend();

	edl->tracks->set_transition_length(transition, length);

	save_backup();
	undo->update_undo_after(_("transition length"), LOAD_EDITS);

	edl->session->default_transition_length = length;
	sync_parameters(CHANGE_PARAMS);
	gui->update(0, NORMAL_DRAW, 0, 0, 0, 0, 0);
	gui->unlock_window();
}

void MWindow::set_transition_length(double length)
{
	gui->lock_window("MWindow::set_transition_length 2");

	undo->update_undo_before();
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();

	edl->tracks->set_transition_length(start, end, length);

	save_backup();
	undo->update_undo_after(_("transition length"), LOAD_EDITS);

	edl->session->default_transition_length = length;
	sync_parameters(CHANGE_PARAMS);
	restart_brender();
	gui->update(0, NORMAL_DRAW, 0, 0, 0, 0, 0);
	gui->unlock_window();
}


void MWindow::redo_entry(BC_WindowBase *calling_window_gui)
{
	calling_window_gui->unlock_window();
	stop_playback(0);
	if( undo->redo_load_flags() & LOAD_SESSION )
		close_mixers();

	cwindow->gui->lock_window("MWindow::redo_entry 1");
	for( int i=0; i<vwindows.size(); ++i ) {
		if( vwindows.get(i)->is_running() ) {
			if( calling_window_gui != vwindows.get(i)->gui ) {
				vwindows.get(i)->gui->lock_window("MWindow::redo_entry 2");
			}
		}
	}
	gui->lock_window("MWindow::redo_entry 3");

	undo->redo();

	save_backup();
	restart_brender();
	update_plugin_states();
	update_plugin_guis();

	gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 1);
	gui->update_proxy_toggle();
	gui->unlock_window();
	cwindow->update(1, 1, 1, 1, 1);
	cwindow->gui->unlock_window();

	for( int i=0; i < vwindows.size(); ++i ) {
		if( vwindows.get(i)->is_running() ) {
			if( calling_window_gui != vwindows.get(i)->gui ) {
				vwindows.get(i)->gui->unlock_window();
			}
		}
	}

	awindow->gui->async_update_assets();

	cwindow->refresh_frame(CHANGE_ALL);
	calling_window_gui->lock_window("MWindow::redo_entry 4");
}


void MWindow::resize_track(Track *track, int w, int h)
{
	undo->update_undo_before();
// We have to move all maskpoints so they do not move in relation to image areas
	((MaskAutos*)track->automation->autos[AUTOMATION_MASK])->translate_masks(
		(w - track->track_w) / 2,
		(h - track->track_h) / 2);
	track->track_w = w;
	track->track_h = h;
	undo->update_undo_after(_("resize track"), LOAD_ALL);
	save_backup();

	restart_brender();
	sync_parameters(CHANGE_EDL);
}


void MWindow::set_inpoint(int is_mwindow)
{
	undo->update_undo_before();
	edl->set_inpoint(edl->local_session->get_selectionstart(1));
	save_backup();
	undo->update_undo_after(_("in point"), LOAD_TIMEBAR);

	if( !is_mwindow ) {
		gui->lock_window("MWindow::set_inpoint 1");
	}
	gui->update_timebar(1);
	if( !is_mwindow ) {
		gui->unlock_window();
	}

	if( is_mwindow ) {
		cwindow->gui->lock_window("MWindow::set_inpoint 2");
	}
	cwindow->gui->timebar->update(1);
	if( is_mwindow ) {
		cwindow->gui->unlock_window();
	}
}

void MWindow::set_outpoint(int is_mwindow)
{
	undo->update_undo_before();
	edl->set_outpoint(edl->local_session->get_selectionend(1));
	save_backup();
	undo->update_undo_after(_("out point"), LOAD_TIMEBAR);

	if( !is_mwindow ) {
		gui->lock_window("MWindow::set_outpoint 1");
	}
	gui->update_timebar(1);
	if( !is_mwindow ) {
		gui->unlock_window();
	}

	if( is_mwindow ) {
		cwindow->gui->lock_window("MWindow::set_outpoint 2");
	}
	cwindow->gui->timebar->update(1);
	if( is_mwindow ) {
		cwindow->gui->unlock_window();
	}
}

void MWindow::unset_inoutpoint(int is_mwindow)
{
	undo->update_undo_before();
	edl->unset_inoutpoint();
	save_backup();
	undo->update_undo_after(_("clear in/out"), LOAD_TIMEBAR);

	if( !is_mwindow ) {
		gui->lock_window("MWindow::unset_inoutpoint 1");
	}
	gui->update_timebar(1);
	if( !is_mwindow ) {
		gui->unlock_window();
	}

	if( is_mwindow ) {
		cwindow->gui->lock_window("MWindow::unset_inoutpoint 2");
	}
	cwindow->gui->timebar->update(1);
	if( is_mwindow ) {
		cwindow->gui->unlock_window();
	}
}

void MWindow::splice(EDL *source, int all)
{
	FileXML file;
	LocalSession *src = source->local_session;

	undo->update_undo_before();
	double source_start = all ? 0 :
		src->inpoint_valid() ? src->get_inpoint() :
		src->outpoint_valid() ? 0 : src->get_selectionstart();
	double source_end = all ? source->tracks->total_length() :
		src->outpoint_valid() ? src->get_outpoint() :
		src->inpoint_valid() ? source->tracks->total_length() :
			src->get_selectionend();
	source->copy(source_start, source_end, 1, &file, "", 1);
//file.dump();
	double start = edl->local_session->get_selectionstart();
	//double end = edl->local_session->get_selectionend();

	paste(start, start, &file,
		edl->session->labels_follow_edits,
		edl->session->plugins_follow_edits,
		edl->session->autos_follow_edits,
		0, 0);

// Position at end of clip
	edl->local_session->set_selectionstart(start + source_end - source_start);
	edl->local_session->set_selectionend(start + source_end - source_start);

	save_backup();
	undo->update_undo_after(_("splice"), LOAD_EDITS | LOAD_TIMEBAR);
	update_plugin_guis();
	restart_brender();
	gui->update(1, NORMAL_DRAW, 1, 1, 0, 1, 0);
	sync_parameters(CHANGE_EDL);
}

void MWindow::save_clip(EDL *new_edl, const char *txt)
{
	new_edl->local_session->set_selectionstart(0);
	new_edl->local_session->set_selectionend(0);
	sprintf(new_edl->local_session->clip_title, _("Clip %d"),
		session->clip_number++);
	char duration[BCTEXTLEN];
	Units::totext(duration, new_edl->tracks->total_length(),
		new_edl->session->time_format,
		new_edl->session->sample_rate,
		new_edl->session->frame_rate,
		new_edl->session->frames_per_foot);

	Track *track = new_edl->tracks->first;
	const char *path = edl->path;
	for( ; (!path || !*path) && track; track=track->next ) {
		if( !track->record ) continue;
		Edit *edit = track->edits->first;
		if( !edit ) continue;
		Indexable *indexable = edit->get_source();
		if( !indexable ) continue;
		path = indexable->path;
	}

        time_t now;  time(&now);
        struct tm dtm;   localtime_r(&now, &dtm);
	char *cp = new_edl->local_session->clip_notes;
	int n, sz = sizeof(new_edl->local_session->clip_notes)-1;
	if( txt && *txt ) {
		n = snprintf(cp, sz, "%s", txt);
		cp += n;  sz -= n;
	}
	n = snprintf(cp, sz,
		"%02d/%02d/%02d %02d:%02d:%02d,  +%s\n",
		dtm.tm_year+1900, dtm.tm_mon+1, dtm.tm_mday,
		dtm.tm_hour, dtm.tm_min, dtm.tm_sec, duration);
	cp += n;  sz -= n;
	if( path && *path ) {
	        FileSystem fs;
        	char title[BCTEXTLEN];
		fs.extract_name(title, path);
		n = snprintf(cp, sz, "%s", title);
		cp += n;  sz -= n;
	}
	cp[n] = 0;
	sprintf(new_edl->local_session->clip_icon,
		"clip_%02d%02d%02d-%02d%02d%02d.png",
		dtm.tm_year+1900, dtm.tm_mon+1, dtm.tm_mday,
		dtm.tm_hour, dtm.tm_min, dtm.tm_sec);
	new_edl->folder_no = AW_CLIP_FOLDER;
	edl->update_assets(new_edl);
	int cur_x, cur_y;
	gui->get_abs_cursor(cur_x, cur_y, 0);
	gui->unlock_window();

	awindow->clip_edit->create_clip(new_edl, cur_x, cur_y);
	new_edl->remove_user();

	gui->lock_window("MWindow::save_clip");
	save_backup();
}

void MWindow::to_clip(EDL *edl, const char *txt, int all)
{
	FileXML file;
	LocalSession *src = edl->local_session;

	gui->lock_window("MWindow::to_clip 1");
	double start = all ? 0 :
		src->inpoint_valid() ? src->get_inpoint() :
		src->outpoint_valid() ? 0 : src->get_selectionstart();
	double end = all ? edl->tracks->total_length() :
		src->outpoint_valid() ? src->get_outpoint() :
		src->inpoint_valid() ? edl->tracks->total_length() :
			src->get_selectionend();
	if( EQUIV(end, start) ) {
		start = 0;
		end = edl->tracks->total_length();
	}

// Don't copy all since we don't want the clips twice.
	edl->copy(start, end, 0, &file, "", 1);

	EDL *new_edl = new EDL(edl);
	new_edl->create_objects();
	new_edl->load_xml(&file, LOAD_ALL);
	save_clip(new_edl, txt);
	gui->unlock_window();
}

int MWindow::toggle_label(int is_mwindow)
{
	double position1, position2;
	undo->update_undo_before();

	if( cwindow->playback_engine->is_playing_back ) {
		position1 = position2 =
			cwindow->playback_engine->get_tracking_position();
	}
	else {
		position1 = edl->local_session->get_selectionstart(1);
		position2 = edl->local_session->get_selectionend(1);
	}

	position1 = edl->align_to_frame(position1, 0);
	position2 = edl->align_to_frame(position2, 0);

//printf("MWindow::toggle_label 1\n");

	edl->labels->toggle_label(position1, position2);
	save_backup();

	if( !is_mwindow ) {
		gui->lock_window("MWindow::toggle_label 1");
	}
	gui->update_timebar(0);
	gui->activate_timeline();
	gui->flush();
	if( !is_mwindow ) {
		gui->unlock_window();
	}

	if( is_mwindow ) {
		cwindow->gui->lock_window("MWindow::toggle_label 2");
	}
	cwindow->gui->timebar->update(1);
	if( is_mwindow ) {
		cwindow->gui->unlock_window();
	}

	awindow->gui->async_update_assets();

	undo->update_undo_after(_("label"), LOAD_TIMEBAR);
	return 0;
}

void MWindow::trim_selection()
{
	undo->update_undo_before();


	edl->trim_selection(edl->local_session->get_selectionstart(),
		edl->local_session->get_selectionend(),
		edl->session->labels_follow_edits,
		edl->session->plugins_follow_edits,
		edl->session->autos_follow_edits);

	save_backup();
	undo->update_undo_after(_("trim selection"), LOAD_EDITS | LOAD_TIMEBAR);
	update_plugin_guis();
	gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 0);
	cwindow->update(1, 0, 0, 0, 1);
	awindow->gui->async_update_assets();
	restart_brender();
	cwindow->refresh_frame(CHANGE_EDL);
}


void MWindow::undo_entry(BC_WindowBase *calling_window_gui)
{
	calling_window_gui->unlock_window();
	stop_playback(0);
	if( undo->undo_load_flags() & LOAD_SESSION )
		close_mixers();

	cwindow->gui->lock_window("MWindow::undo_entry 1");
	for( int i=0; i<vwindows.size(); ++i ) {
		if( vwindows.get(i)->is_running() ) {
			if( calling_window_gui != vwindows.get(i)->gui ) {
				vwindows.get(i)->gui->lock_window("MWindow::undo_entry 4");
			}
		}
	}
	gui->lock_window("MWindow::undo_entry 2");

	undo->undo();

	save_backup();
	restart_brender();
	update_plugin_states();
	update_plugin_guis();

	gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 1);
	gui->update_proxy_toggle();
	gui->unlock_window();
	cwindow->update(1, 1, 1, 1, 1);
	cwindow->gui->unlock_window();

	for( int i=0; i<vwindows.size(); ++i ) {
		if( vwindows.get(i)->is_running() ) {
			if( calling_window_gui != vwindows.get(i)->gui ) {
				vwindows.get(i)->gui->unlock_window();
			}
		}
	}

	awindow->gui->async_update_assets();

	cwindow->refresh_frame(CHANGE_ALL);
	calling_window_gui->lock_window("MWindow::undo_entry 4");
}


void MWindow::new_folder(const char *new_folder, int is_clips)
{
	undo->update_undo_before();
	if( edl->new_folder(new_folder, is_clips) ) {
		MainError::show_error(_("create new folder failed"));
	}
	undo->update_undo_after(_("new folder"), LOAD_ALL);
	awindow->gui->async_update_assets();
}

void MWindow::delete_folder(char *folder)
{
	undo->update_undo_before();
	if( edl->delete_folder(folder) < 0 ) {
		MainError::show_error(_("delete folder failed"));
	}
	undo->update_undo_after(_("del folder"), LOAD_ALL);
	awindow->gui->async_update_assets();
}

void MWindow::select_point(double position)
{
	gui->unlock_window();
	gui->stop_drawing();
	cwindow->stop_playback(0);
	gui->lock_window("MWindow::select_point");

	edl->local_session->set_selectionstart(position);
	edl->local_session->set_selectionend(position);

// Que the CWindow
	cwindow->update(1, 0, 0, 0, 1);

	update_plugin_guis();
	gui->update_patchbay();
	gui->hide_cursor(0);
	gui->draw_cursor(0);
	gui->mainclock->update(edl->local_session->get_selectionstart(1));
	gui->zoombar->update();
	gui->update_timebar(0);
	gui->flash_canvas(0);
	gui->flush();
}




void MWindow::map_audio(int pattern)
{
	undo->update_undo_before();
	remap_audio(pattern);
	undo->update_undo_after(
		pattern == MWindow::AUDIO_1_TO_1 ? _("map 1:1") : _("map 5.1:2"),
		LOAD_AUTOMATION);
	sync_parameters(CHANGE_PARAMS);
	gui->update(0, NORMAL_DRAW, 0, 0, 1, 0, 0);
}

void MWindow::remap_audio(int pattern)
{
	int current_channel = 0;
	int current_track = 0;
	for( Track *current=edl->tracks->first; current; current=NEXT ) {
		if( current->data_type == TRACK_AUDIO &&
			current->record ) {
			Autos *pan_autos = current->automation->autos[AUTOMATION_PAN];
			PanAuto *pan_auto = (PanAuto*)pan_autos->get_auto_for_editing(-1);

			for( int i=0; i < MAXCHANNELS; ++i ) {
				pan_auto->values[i] = 0.0;
			}

			if( pattern == MWindow::AUDIO_1_TO_1 ) {
				pan_auto->values[current_channel] = 1.0;
			}
			else
			if( pattern == MWindow::AUDIO_5_1_TO_2 ) {
				switch( current_track ) {
				case 0:
				case 4:
					pan_auto->values[0] = 1;
					break;
				case 1:
				case 5:
					pan_auto->values[1] = 1;
					break;
				case 2:
				case 3:
					pan_auto->values[0] = 0.5;
					pan_auto->values[1] = 0.5;
					break;
				}
			}

			BC_Pan::calculate_stick_position(edl->session->audio_channels,
				edl->session->achannel_positions, pan_auto->values,
				MAX_PAN, PAN_RADIUS, pan_auto->handle_x, pan_auto->handle_y);

			current_channel++;
			current_track++;
			if( current_channel >= edl->session->audio_channels )
				current_channel = 0;
		}
	}
}


void MWindow::rescale_proxy(EDL *clip, int orig_scale, int new_scale)
{
	edl->rescale_proxy(orig_scale, new_scale);
}

void MWindow::add_proxy(int use_scaler,
	ArrayList<Indexable*> *orig_assets, ArrayList<Indexable*> *proxy_assets)
{
	edl->add_proxy(use_scaler, orig_assets, proxy_assets);
}

void MWindow::cut_commercials()
{
#ifdef HAVE_COMMERCIAL
	undo->update_undo_before();
	commercials->scan_media();
	edl->optimize();
	save_backup();
	undo->update_undo_after(_("cut ads"), LOAD_EDITS | LOAD_TIMEBAR);

	restart_brender();
	update_plugin_guis();
	gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 0);
	cwindow->update(1, 0, 0, 0, 1);
	cwindow->refresh_frame(CHANGE_EDL);
#endif
}

int MWindow::normalize_speed(EDL *old_edl, EDL *new_edl)
{
	int result = 0;
	Track *old_track = old_edl->tracks->first;
	Track *new_track = new_edl->tracks->first;
	for( ; old_track && new_track; old_track=old_track->next, new_track=new_track->next ) {
		if( old_track->data_type != new_track->data_type ) continue;
		FloatAutos *old_speeds = (FloatAutos *)old_track->automation->autos[AUTOMATION_SPEED];
		FloatAutos *new_speeds = (FloatAutos *)new_track->automation->autos[AUTOMATION_SPEED];
		if( !old_speeds || !new_speeds ) continue;
		FloatAuto *old_speed = (FloatAuto *)old_speeds->first;
		FloatAuto *new_speed = (FloatAuto *)new_speeds->first;
		while( old_speed && new_speed && old_speed->equals(new_speed) ) {
			old_speed = (FloatAuto *)old_speed->next;
			new_speed = (FloatAuto *)new_speed->next;
		}
		Edit *old_edit = old_track->edits->first;
		Edit *new_edit = new_track->edits->first;
		for( ; old_edit && new_edit; old_edit=old_edit->next, new_edit=new_edit->next ) {
			int64_t edit_start = old_edit->startproject, edit_end = edit_start + old_edit->length;
			if( old_speed || new_speed ) {
				double orig_start = old_speeds->automation_integral(0, edit_start, PLAY_FORWARD);
				double orig_end   = old_speeds->automation_integral(0, edit_end, PLAY_FORWARD);
				edit_start = new_speeds->speed_position(orig_start);
				edit_end = new_speeds->speed_position(orig_end);
				result = 1;
			}
			new_edit->startproject = edit_start;
			new_edit->length = edit_end - edit_start;
		}
	}
	return result;
}

void MWindow::speed_before()
{
	if( !speed_edl ) {
		speed_edl = new EDL;
		speed_edl->create_objects();
	}
	speed_edl->copy_all(edl);
}

int MWindow::speed_after(int done)
{
	int result = 0;
	if( speed_edl ) {
		if( done >= 0 )
			result = normalize_speed(speed_edl, edl);
		if( done != 0 ) {
			speed_edl->remove_user();
			speed_edl = 0;
		}
	}
	return result;
}

