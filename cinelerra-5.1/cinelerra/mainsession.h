
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

#ifndef MAINSESSION_H
#define MAINSESSION_H

#include "asset.inc"
#include "assets.inc"
#include "auto.inc"
#include "bchash.inc"
#include "edit.inc"
#include "edits.inc"
#include "edl.inc"
#include "guicast.h"
#include "indexable.inc"
#include "mainsession.inc"
#include "maxchannels.h"
#include "mwindow.inc"
#include "plugin.inc"
#include "pluginset.inc"
#include "pluginserver.inc"
#include "track.inc"
#include "vwindowgui.inc"

// Options not in EDL but not changed in preferences
class MainSession
{
public:
	MainSession(MWindow *mwindow);
	~MainSession();

	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);
	void save_x11_host(int play_config, const char *x11_host);
	int set_default_x11_host(int win_config=-1);
	void default_window_positions(int window_config=0);
	void boundaries();
	Track *drag_handle_track();
	void update_clip_number();
	int load_file(const char *path);
	int save_file(const char *path);

// For drag and drop events
// The entire track where the dropped asset is going to go
	Track *track_highlighted;
// The edit after the point where the media is going to be dropped.
	Edit *edit_highlighted;
// The plugin set where the plugin is going to be dropped.
	PluginSet *pluginset_highlighted;
// The plugin after the point where the plugin is going to be dropped.
	Plugin *plugin_highlighted;
// Current drag operation
	int current_operation;

// Item being dragged
	ArrayList <PluginServer*> *drag_pluginservers;
	Plugin *drag_plugin;
// When trim should only affect the selected edits or plugins
	Edits *trim_edits;
	ArrayList<Indexable*> *drag_assets;
	ArrayList<EDL*> *drag_clips;
	Auto *drag_auto;
	ArrayList<Auto*> *drag_auto_gang;

// Edit whose handle is being dragged
	Edit *drag_edit;
// Selected edits clip of grouped drag
	EDL *drag_group;
// drag edit of selected edits
	Edit *drag_group_edit;
// cursor position in selected edits at start of drag
	double drag_group_position;
// top track of selected edits
	Track *drag_group_first_track;
// Edits who are being dragged
	ArrayList<Edit*> *drag_edits;
// Button pressed during drag
	int drag_button;
// Handle being dragged
	int drag_handle;
// Current position of drag cursor
	double drag_position;
// Starting position of drag cursor
	double drag_start;
// Cursor position when button was pressed
	int drag_origin_x, drag_origin_y;
// Value of keyframe when button was pressed
	float drag_start_percentage;
	long drag_start_position;
// Amount of data rendered, for drawing status in timebar
	double brender_end;
// Position of cursor in CWindow output.  Used by ruler.
	int cwindow_output_x, cwindow_output_y;

// Show controls in CWindow
	int cwindow_controls;

// Clip number for automatic title generation
	int clip_number;
// Next group id index
	int group_number;

// Audio session
	int changes_made;

// filename of the current project for window titling and saving
	char filename[BCTEXTLEN];

// title bar background blend
	float title_bar_alpha;

	int batchrender_x, batchrender_y, batchrender_w, batchrender_h;

// Window positions
	int window_config;
	char a_x11_host[BCSTRLEN], b_x11_host[BCSTRLEN];
// level window
	int lwindow_x, lwindow_y, lwindow_w, lwindow_h;
// main window
	int mwindow_x, mwindow_y, mwindow_w, mwindow_h;
// viewer
	int vwindow_x, vwindow_y, vwindow_w, vwindow_h;
// compositor
	int cwindow_x, cwindow_y, cwindow_w, cwindow_h;
	int ctool_x, ctool_y;
// asset window
	int awindow_x, awindow_y, awindow_w, awindow_h;
	int gwindow_x, gwindow_y;
// record monitor
	int rmonitor_x, rmonitor_y, rmonitor_w, rmonitor_h;
// record status
	int rwindow_x, rwindow_y, rwindow_w, rwindow_h;
// channelinfo
	int cswindow_x, cswindow_y, cswindow_w, cswindow_h;
// subtitle
	int swindow_x, swindow_y, swindow_w, swindow_h;
// binfolder filter window
	int bwindow_w, bwindow_h;
// error window
	int ewindow_w, ewindow_h;
// Channel edit window
	int channels_x, channels_y;
// Picture edit window
	int picture_x, picture_y;
// Recording scope window
	int scope_x, scope_y, scope_w, scope_h;
// Recording histogram window
	int histogram_x, histogram_y, histogram_w, histogram_h;
// Recording scopes enabled
	int record_scope;
// Recording scope parameters
	int use_hist;
	int use_wave;
	int use_vector;
	int use_hist_parade;
	int use_wave_parade;

	int afolders_w;
	int show_vwindow, show_awindow, show_cwindow, show_gwindow, show_lwindow;
	int plugindialog_w, plugindialog_h;
//	int presetdialog_w, presetdialog_h;
	int keyframedialog_w, keyframedialog_h;
	int keyframedialog_column1;
	int keyframedialog_column2;
	int keyframedialog_all;
	int menueffect_w, menueffect_h;
	int transitiondialog_w, transitiondialog_h;

	int cwindow_fullscreen;
	int rwindow_fullscreen;
	int vwindow_fullscreen;
	int zwindow_fullscreen;
	int selected_zwindow;

	double actual_frame_rate;

// Tip of the day
	int current_tip;

	MWindow *mwindow;
};

#endif
