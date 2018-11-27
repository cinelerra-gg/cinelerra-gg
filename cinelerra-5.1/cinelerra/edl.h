
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

#ifndef EDL_H
#define EDL_H

#include <stdio.h>
#include <stdint.h>

#include "asset.inc"
#include "assets.inc"
#include "autoconf.inc"
#include "bchash.inc"
#include "binfolder.h"
#include "edit.inc"
#include "edits.inc"
#include "edl.inc"
#include "edlsession.inc"
#include "filexml.inc"
#include "indexable.h"
#include "indexstate.inc"
#include "labels.inc"
#include "localsession.inc"
#include "maxchannels.h"
#include "mutex.inc"
#include "clipedls.h"
#include "playabletracks.inc"
#include "playbackconfig.h"
#include "pluginserver.h"
#include "preferences.inc"
#include "recordlabel.inc"
#include "sharedlocation.inc"
#include "theme.inc"
#include "tracks.inc"
#include "vedit.inc"
#include "zwindow.h"

// Loading and saving are built on load and copy except for automation:

// Storage:
// Load: load new -> paste into master
// Save: copy all of master
// Undo: selective load into master
// Copy: copy from master
// Paste: load new -> paste into master
// Copy automation: copy just automation from master
// Paste automation: paste functions in automation


class EDL : public Indexable
{
public:
	EDL(EDL *parent_edl = 0);
	~EDL();

	void create_objects();
	EDL& operator=(EDL &edl);

// Load configuration and track counts
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);
// Clip default settings to boundaries.
	void boundaries();
// Create tracks using existing configuration
	int create_default_tracks();
	int load_xml(FileXML *file, uint32_t load_flags);
	int read_xml(FileXML *file, uint32_t load_flags);
	int save_xml(FileXML *xml, const char *output_path);
	int load_audio_config(FileXML *file, int append_mode, uint32_t load_flags);
	int load_video_config(FileXML *file, int append_mode, uint32_t load_flags);

// Return 1 if rendering requires a virtual console.
	int get_use_vconsole(VEdit* *playable_edit,
		int64_t position,
		int direction,
		PlayableTracks *playable_tracks);

// Convert position to frame boundry times
	double frame_align(double position, int round);
// frame align if cursor alignment is enabled
	double align_to_frame(double position, int round);
// get position under cursor in pane
	double get_cursor_position(int cursor_x, int pane_no);

// increase track w/h to at least session w/h
	void retrack();
// Scale all sample values since everything is locked to audio
	void rechannel();
	void resample(double old_rate, double new_rate, int data_type);

	int copy(double start, double end, int all,
		FileXML *file, const char *output_path, int rewind_it);
	int copy(int all, FileXML *file, const char *output_path, int rewind_it);

	int copy_clip(double start, double end, int all,
		FileXML *file, const char *output_path, int rewind_it);
	int copy_clip(int all, FileXML *file, const char *output_path, int rewind_it);

	int copy_nested_edl(double start, double end, int all,
		FileXML *file, const char *output_path, int rewind_it);
	int copy_nested_edl(int all, FileXML *file, const char *output_path, int rewind_it);

	int copy_vwindow_edl(double start, double end, int all,
		FileXML *file, const char *output_path, int rewind_it);
	int copy_vwindow_edl(int all, FileXML *file, const char *output_path, int rewind_it);

	void copy_tracks(EDL *edl);
// Copies project path, folders, EDLSession, and LocalSession from edl argument.
// session_only - used by preferences and format specify
// whether to only copy EDLSession
	void copy_session(EDL *edl, int session_only = 0);
	int copy_all(EDL *edl);
	void copy_assets(EDL *edl);
	void copy_clips(EDL *edl);
	void copy_nested(EDL *edl);
	void copy_mixers(EDL *edl);
// Copy pan and fade settings from edl
	void synchronize_params(EDL *edl);
// Determine if the positions are equivalent if they're within half a frame
// of each other.
	int equivalent(double position1, double position2);
// Determine if the EDL's produce equivalent video output to the old EDL.
// The new EDL is this and the old EDL is the argument.
// Return the number of seconds from the beginning of this which are
// equivalent to the argument.
// If they're completely equivalent, -1 is returned;
// This is used by BRender + BatchRender.
	double equivalent_output(EDL *edl);
// Set project path for filename prefixes in the assets
	void set_path(const char *path);
// Set points and labels
	void set_inpoint(double position);
	void set_outpoint(double position);
	void unset_inoutpoint();
// Redraw resources during index builds
	void set_index_file(Indexable *indexable);
// Add assets from the src to the destination
	void update_assets(EDL *src);
	void optimize();
// return next/prev edit starting from position
	double next_edit(double position);
	double prev_edit(double position);
// Debug
	int dump(FILE *fp=stdout);
	static int next_id();
// folders
	BinFolder *get_folder(int no);
	int get_folder_number(const char *title);
	const char *get_folder_name(int no);
	int new_folder(const char *title, int is_clips);
	int delete_folder(const char *title);

	void modify_edithandles(double oldposition,
		double newposition,
		int currentend,
		int handle_mode,
		int edit_labels,
		int edit_plugins,
		int edit_autos);

	void modify_pluginhandles(double oldposition,
		double newposition,
		int currentend,
		int handle_mode,
		int edit_labels,
		int edit_autos,
		Edits *trim_edits);

	int trim_selection(double start,
		double end,
		int edit_labels,
		int edit_plugins,
		int edit_autos);

// Editing functions
	int copy_assets(double start, double end,
		FileXML *file, int all, const char *output_path);
	int copy(double start, double end, int all,
		const char *closer, FileXML *file,
		const char *output_path, int rewind_it);
	void copy_indexables(EDL *edl);
	EDL *new_nested(EDL *edl, const char *path);
	EDL *create_nested_clip(EDL *nested);
	void create_nested(EDL *nested);
	void paste_silence(double start, double end,
		int edit_labels /* = 1 */,
		int edit_plugins,
		int edit_autos);
	int in_use(Indexable *indexable);
	void remove_from_project(ArrayList<Indexable*> *assets);
	void remove_from_project(ArrayList<EDL*> *clips);
	int blade(double position);
	int clear(double start, double end,
		int clear_labels,
		int clear_plugins,
		int edit_autos);
	void deglitch(double position);
// Insert the asset at a point in the EDL
	void insert_asset(Asset *asset,
		EDL *nested_edl,
		double position,
		Track *first_track = 0,
		RecordLabels *labels = 0);
// Insert the clip at a point in the EDL
	int insert_clips(ArrayList<EDL*> *new_edls, int load_mode, Track *first_track = 0);
// Add a copy of EDL* to the clip array.  Returns the copy.
	EDL* add_clip(EDL *edl);

	void get_shared_plugins(Track *source, ArrayList<SharedLocation*> *plugin_locations,
		int omit_recordable, int data_type);
	void get_shared_tracks(Track *track, ArrayList<SharedLocation*> *module_locations,
		int omit_recordable, int data_type);

    int get_tracks_height(Theme *theme);
    int64_t get_tracks_width();
// Return dimensions for canvas if smaller dimensions has zoom of 1
	void calculate_conformed_dimensions(int single_channel, float &w, float &h);
// Get the total output size scaled to aspect ratio
	void output_dimensions_scaled(int &w, int &h);
	float get_aspect_ratio();


// For Indexable
	int get_audio_channels();
	int get_sample_rate();
	int64_t get_audio_samples();
	int have_audio();
	int have_video();
	int get_w();
	int get_h();
	double get_frame_rate();
	int get_video_layers();
	int64_t get_video_frames();

	EDL* get_vwindow_edl(int number);
	int total_vwindow_edls();
	void remove_vwindow_edls();
	void remove_vwindow_edl(EDL *edl);
// Adds to list of EDLs & increase garbage collection counter
// Does nothing if EDL already exists
	void append_vwindow_edl(EDL *edl, int increase_counter);
	void rescale_proxy(int orig_scale, int new_scale);
	void set_proxy(int new_scale, int use_scaler,
		ArrayList<Indexable*> *orig_assets, ArrayList<Indexable*> *proxy_assets);
	void add_proxy(int use_scaler,
		ArrayList<Indexable*> *orig_assets, ArrayList<Indexable*> *proxy_assets);
	Asset *get_proxy_asset();

// Titles of all subfolders
	BinFolders folders;
// Clips, Nested EDLs
	ClipEDLs clips, nested_edls;
// EDLs being shown in VWindows
	ArrayList<EDL*> vwindow_edls;
// is the vwindow_edl shared and therefore should not be deleted in destructor
//	int vwindow_edl_shared;
	Mixers mixers;

// Media files
// Shared between all EDLs
	Assets *assets;

	Tracks *tracks;
	Labels *labels;
// Shared between all EDLs in a tree, for projects.
	EDLSession *session;
// Specific to this EDL, for clips.
	LocalSession *local_session;

// Use parent Assets if nonzero
	EDL *parent_edl;
};

#endif
