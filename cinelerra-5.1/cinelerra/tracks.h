
/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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

#ifndef TRACKS_H
#define TRACKS_H

#include <stdio.h>
#include <stdint.h>

#include "autoconf.h"
#include "edl.inc"
#include "file.inc"
#include "filexml.inc"
#include "linklist.h"
#include "pluginserver.inc"
#include "threadindexer.inc"
#include "track.h"
#include "trackcanvas.inc"
#include "transition.inc"



class Tracks : public List<Track>
{
public:
	Tracks();
	Tracks(EDL *edl);
	virtual ~Tracks();

	Tracks& operator=(Tracks &tracks);
	int load(FileXML *xml,
		int &track_offset,
		uint32_t load_flags);
	void move_edits(ArrayList<Edit*> *edits, Track *track, double position,
		int edit_labels, int edit_plugins, int edit_autos, int behaviour);
	void move_group(EDL *group, Track *first_track, double position);
	void move_effect(Plugin *plugin, Track *track, int64_t position);
	void move_effect(Plugin *plugin, PluginSet *plugin_set, int64_t position);

// Construct a list of all the recordable edits which start on position
	void clear_selected_edits();
	void select_affected_edits(double position, Track *start_track);
	void get_selected_edits(ArrayList<Edit*> *drag_edits);
	int next_group_id();
	int new_group(int id);
	int set_group_selected(int id, int v);
	int del_group(int id);

	void get_automation_extents(float *min,
		float *max,
		double start,
		double end,
		int autogrouptype);

	void equivalent_output(Tracks *tracks, double *result);

	int move_track_up(Track *track);        // move recordable tracks up
	int move_track_down(Track *track);      // move recordable tracks down
	int move_tracks_up();                   // move recordable tracks up
	int move_tracks_down();                 // move recordable tracks down
	void paste_audio_transition(PluginServer *server);
	void paste_video_transition(PluginServer *server, int first_track = 0);

// Only tests effects
	int plugin_exists(Plugin *plugin);
	int track_exists(Track *track);

	void paste_transition(PluginServer *server, Edit *dest_edit);
// Return the numbers of tracks with the play patch enabled
	int playable_audio_tracks();
	int playable_video_tracks();
// Return number of tracks with the record patch enabled
	int recordable_audio_tracks();
	int recordable_video_tracks();
	int total_audio_tracks();
	int total_video_tracks();
// return the longest track in all the tracks in seconds
 	double total_length();
 	double total_audio_length();
 	double total_video_length();
	double total_length_framealigned(double fps);
// Update y pixels after a zoom
	void update_y_pixels(Theme *theme);
// Total number of tracks where the following toggles are selected
	void select_all(int type,
		int value);
	void translate_projector(float offset_x, float offset_y);
		int total_of(int type);
// add a track
	Track* add_audio_track(int above, Track *dst_track);
	Track* add_video_track(int above, Track *dst_track);
	Track* add_subttl_track(int above, Track *dst_track);
//	Track* add_audio_track(int to_end = 1);
//	Track* add_video_track(int to_end = 1);
// delete any track
 	int delete_track(Track* track);
// detach shared effects referencing module
	int detach_shared_effects(int module);

	EDL *edl;





// Types for drag toggle behavior
	enum
	{
		NONE,
		PLAY,
		RECORD,
		GANG,
		DRAW,
		MUTE,
		EXPAND
	};









	int change_channels(int oldchannels, int newchannels);
	int dump(FILE *fp);



// Change references to shared modules in all tracks from old to new.
// If do_swap is true values of new are replaced with old.
	void change_modules(int old_location, int new_location, int do_swap);
// Append all the tracks to the end of the recordable tracks
	int concatenate_tracks(int edit_plugins, int edit_autos);
// Change references to shared plugins in all tracks
	void change_plugins(SharedLocation &old_location, SharedLocation &new_location, int do_swap);

	int delete_tracks();     // delete all the recordable tracks
	int delete_all_tracks();      // delete just the tracks

	void copy_from(Tracks *tracks);

// ================================== EDL editing
	int copy(double start,
		double end,
		int all,
		FileXML *file,
		const char *output_path = "");



	int copy_assets(FileXML *xml,
		double start,
		double end,
		int all);
	int blade(double position);
	int clear(double start, double end, int clear_plugins, int edit_autos);
	void clear_automation(double selectionstart,
		double selectionend);
	void set_automation_mode(double selectionstart,
		double selectionend,
		int mode);
	int clear_default_keyframe();
	int clear_handle(double start,
		double end,
		double &longest_distance,
		int clear_labels,
		int clear_plugins,
		int edit_autos);
	int copy_automation(double selectionstart,
		double selectionend,
		FileXML *file,
		int default_only,
		int autos_only);
//	int copy_default_keyframe(FileXML *file);
	void paste_automation(double selectionstart,
		FileXML *xml,
		int default_only,
		int active_only,
		int typeless);
//	int paste_default_keyframe(FileXML *file);
	int paste(int64_t start, int64_t end);
// all units are samples by default
	int paste_output(int64_t startproject,
				int64_t endproject,
				int64_t startsource_sample,
				int64_t endsource_sample,
				int64_t startsource_frame,
				int64_t endsource_frame,
				Asset *asset);
	int paste_silence(double start,
		double end,
		int edit_plugins,
		int edit_autos);
	int purge_asset(Asset *asset);
	int asset_used(Asset *asset);
// Transition popup
	int popup_transition(int cursor_x, int cursor_y);
	int select_auto(int cursor_x, int cursor_y);
	int move_auto(int cursor_x, int cursor_y, int shift_down);
	int modify_edithandles(double &oldposition,
		double &newposition,
		int currentend,
		int handle_mode,
		int edit_labels,
		int edit_plugins,
		int edit_autos);
	int modify_pluginhandles(double &oldposition,
		double &newposition,
		int currentend,
		int handle_mode,
		int edit_labels,
		int edit_autos,
		Edits *trim_edits);
	int select_handles();
	int select_region();
	int select_edit(int64_t cursor_position, int cursor_x, int cursor_y, int64_t &new_start, int64_t &new_end);
	int feather_edits(int64_t start, int64_t end, int64_t samples, int audio, int video);
	int64_t get_feather(int64_t selectionstart, int64_t selectionend, int audio, int video);
// Move edit boundaries and automation during a framerate change
	int scale_time(float rate_scale, int ignore_record, int scale_edits, int scale_autos, int64_t start, int64_t end);

	void clear_transitions(double start, double end);
	void shuffle_edits(double start, double end);
	void reverse_edits(double start, double end);
	void align_edits(double start, double end);
	void set_edit_length(double start, double end, double length);
	void set_transition_length(double start, double end, double length);
	void set_transition_length(Transition *transition, double length);
	void paste_transitions(double start, double end, int track_type, char* title);

// ================================== accounting

	int handles, titles;     // show handles or titles
	int show_output;         // what type of video to draw
	AutoConf auto_conf;      // which autos are visible
	int overlays_visible;
	double total_playable_length();     // Longest track.
// Used by equivalent_output
	int total_playable_vtracks();
	double total_recordable_length();   // Longest track with recording on
	int totalpixels();       // height of all tracks in pixels
	int number_of(Track *track);        // track number of pointer
	Track* number(int number);      // pointer to track number
	Track *get(int idx, int data_type);


private:
};

#endif
