
/*
 * CINELERRA
 * Copyright (C) 2008-2014 Adam Williams <broadcast at earthling dot net>
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

#ifndef TRACKCANVAS_H
#define TRACKCANVAS_H

#include "asset.inc"
#include "auto.inc"
#include "autos.inc"
#include "bctimer.inc"
#include "edit.inc"
#include "edithandles.inc"
#include "edl.inc"
#include "floatauto.inc"
#include "floatautos.inc"
#include "intauto.inc"
#include "guicast.h"
#include "indexable.h"
#include "keyframe.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "plugin.inc"
#include "pluginset.inc"
#include "plugintoggles.inc"
#include "resourcepixmap.inc"
#include "timelinepane.inc"
#include "track.inc"
#include "trackcanvas.inc"
#include "tracks.inc"
#include "transitionhandles.inc"
#include "keyframe.inc"

#include <stdarg.h>

class TrackCanvas : public BC_SubWindow
{
public:
	TrackCanvas(MWindow *mwindow, MWindowGUI *gui);
	TrackCanvas(MWindow *mwindow, TimelinePane *pane, int x, int y, int w, int h);
	~TrackCanvas();

	void create_objects();
	void resize_event();
	int drag_start_event();
	int cursor_leave_event();
	int keypress_event();
	void draw_resources(int mode = 0,
		int indexes_only = 0,     // Redraw only certain audio resources with indexes
		Indexable *indexable = 0);
	void draw_highlight_rectangle(int x, int y, int w, int h);
	void draw_highlight_insertion(int x, int y, int w, int h);
	void draw_playback_cursor();
	void draw_highlighting();
	void draw_keyframe_reticle();
	int draw_hairline(Auto *auto_keyframe, int color, int show);

// User can either call draw or draw_overlays to copy a fresh
// canvas and just draw the overlays over it
	void draw_overlays();
	void update_handles();
// Convert edit coords to transition coords
	void get_transition_coords(Edit *edit,
		int64_t &x, int64_t &y, int64_t &w, int64_t &h);
	void get_handle_coords(Edit *edit,
		int64_t &x,
		int64_t &y,
		int64_t &w,
		int64_t &h,
		int side);
	int get_drag_values(float *percentage,
		int64_t *position,
		int do_clamp,
		int cursor_x,
		int cursor_y,
		Auto *current);
	void draw_title(Edit *edit,
		int64_t edit_x,
		int64_t edit_y,
		int64_t edit_w,
		int64_t edit_h);
	void draw_automation();
	void draw_hard_edges();
	void draw_inout_points();
	void draw_auto(Auto *current,
		int x,
		int y,
		int center_pixel,
		int zoom_track);
	void draw_floatauto(FloatAuto *current,
		int x,
		int y,
		int in_x,
		int in_y,
		int out_x,
		int out_y,
		int center_pixel,
		int zoom_track,
		int color);
	int test_auto(Auto *current,
		int x,
		int y,
		int center_pixel,
		int zoom_track,
		int cursor_x,
		int cursor_y,
		int buttonpress);
	int test_floatauto(FloatAuto *current,
		int x,
		int y,
		int in_x,
		int in_y,
		int out_x,
		int out_y,
		int center_pixel,
		int zoom_track,
		int cursor_x,
		int cursor_y,
		int buttonpress,
		int autogrouptype);
	void draw_floatline(int center_pixel,
		FloatAuto *previous,
		FloatAuto *current,
		FloatAutos *autos,
		double unit_start,
		double zoom_units,
		double yscale,
		int ax,
		int ay,
		int ax2,
		int ay2,
		int color,
		int autogrouptype);
	int test_floatline(int center_pixel,
		FloatAutos *autos,
		double unit_start,
		double zoom_units,
		double yscale,
		int x1,
		int x2,
		int cursor_x,
		int cursor_y,
		int buttonpress,
		int autogrouptype);
	void draw_toggleline(int center_pixel,
		int ax,
		int ay,
		int ax2,
		int ay2);
	int test_toggleline(Autos *autos,
		int center_pixel,
		int x1,
		int y1,
		int x2,
		int y2,
		int cursor_x,
		int cursor_y,
		int buttonpress);
	int do_keyframes(int cursor_x,
		int cursor_y,
		int draw,
		int buttonpress,
		int &new_cursor,
		int &update_cursor,
		int &rerender);

	int do_float_autos(Track *track,
		Autos *autos,
		int cursor_x,
		int cursor_y,
		int draw,
		int buttonpress,
		int x_offset,
		int y_offset,
		int color,
		Auto* &auto_instance,
		int autogrouptype);
	int do_int_autos(Track *track,
		Autos *autos,
		int cursor_x,
		int cursor_y,
		int draw,
		int buttonpress,
		int x_offset,
		int y_offset,
		int color,
		Auto * &auto_instance);
	int do_autos(Track *track,
		Autos *autos,
		int cursor_x,
		int cursor_y,
		int draw,
		int buttonpress,
		BC_Pixmap *pixmap,
		Auto* &auto_instance,
		int &rerender);
	int do_plugin_autos(Track *track,
		int cursor_x,
		int cursor_y,
		int draw,
		int buttonpress,
		Plugin* &keyframe_plugin,
		KeyFrame* &keyframe_instance);


	void calculate_viewport(Track *track,
		double &view_start,
		double &unit_start,
		double &view_end,
		double &unit_end,
		double &yscale,
		int &center_pixel,
		double &zoom_sample,
		double &zoom_units);

// Convert percentage position inside track to value.
// if is_toggle is 1, the result is either 0 or 1.
// if reference is nonzero and a FloatAuto,
//     the result is made relative to the value in reference.
	float percentage_to_value(float percentage,
		int is_toggle,
		Auto *reference,
		int autogrouptype);

// Get x and y of a FloatAuto relative to center_pixel
	void calculate_auto_position(double *x,
		double *y,
		double *in_x,
		double *in_y,
		double *out_x,
		double *out_y,
		Auto *current,
		double unit_start,
		double zoom_units,
		double yscale,
		int autogrouptype);

	void fill_ganged_autos(int all, float change, Track *skip, FloatAuto *fauto);
	void update_ganged_autos(float change, Track *skip, FloatAuto *fauto);
	void clear_ganged_autos();

	void draw_brender_range();
	void draw_loop_points();
	void draw_transitions();
	void draw_drag_handle();
	void draw_plugins();
	void refresh_plugintoggles();
	void update_edit_handles(Edit *edit, int64_t edit_x, int64_t edit_y, int64_t edit_w, int64_t edit_h);
	void update_transitions();
	void update_keyframe_handles(Track *track);
// Draw everything to synchronize with the view.
	void draw(int mode, int hide_cursor);
// Draw resources during index building
	void draw_indexes(Indexable *indexable);
// Get location of edit on screen without boundary checking
	void edit_dimensions(Edit *edit, int64_t &x, int64_t &y, int64_t &w, int64_t &h);
	void track_dimensions(Track *track, int64_t &x, int64_t &y, int64_t &w, int64_t &h);
	void plugin_dimensions(Plugin *plugin, int64_t &x, int64_t &y, int64_t &w, int64_t &h);
	void get_pixmap_size(Edit *edit, int64_t edit_x, int64_t edit_w, int64_t &pixmap_x, int64_t &pixmap_w, int64_t &pixmap_h);
	ResourcePixmap* create_pixmap(Edit *edit, int64_t edit_x, int64_t pixmap_x, int64_t pixmap_w, int64_t pixmap_h);
	void update_cursor(int flush);
// Get edit and handle the cursor is over
	int do_edit_handles(int cursor_x,
		int cursor_y,
		int button_press,
		int &rerender,
		int &update_overlay,
		int &new_cursor,
		int &update_cursor);
// Get plugin and handle the cursor if over
	int do_plugin_handles(int cursor_x,
		int cursor_y,
		int button_press,
		int &rerender,
		int &update_overlay,
		int &new_cursor,
		int &update_cursor);
// Get edit the cursor is over
	int do_edits(int cursor_x,
		int cursor_y,
		int button_press,
		int drag_start,
		int &redraw,
		int &rerender,
		int &new_cursor,
		int &update_cursor);
	int do_tracks(int cursor_x,
		int cursor_y,
		int button_press);
	int test_resources(int cursor_x, int cursor_y);
	int do_plugins(int cursor_x,
		int cursor_y,
		int drag_start,
		int button_press,
		int &redraw,
		int &rerender);
	int do_transitions(int cursor_x,
		int cursor_y,
		int button_press,
		int &new_cursor,
		int &update_cursor);
	void draw_cropped_line(int x1,
		int y1,
		int x2,
		int y2,
		int min_y,
		int max_y);
	int button_press_event();
	int button_release_event();
	int cursor_update(int in_motion);
	int cursor_motion_event();
	int activate();
	int deactivate();
	int repeat_event(int64_t duration);
	void start_dragscroll();
	void stop_dragscroll();
	int start_selection(double position);
	int drag_motion_event();
	int drag_stop_event();
	int drag_motion(Track **over_track,
		Edit **over_edit,
		PluginSet **over_pluginset,
		Plugin **over_plugin);
	int drag_stop(int *redraw);
	int64_t drop_edit_position (int *is_insertion, Edit *moved_edit, int64_t moved_edit_length);
	int64_t drop_plugin_position(PluginSet *plugin_set, Plugin *moved_plugin);
	void end_edithandle_selection();
	void end_pluginhandle_selection();
// Number of seconds spanned by the trackcanvas
	double time_visible();
	void update_drag_handle();
	int update_drag_edit();
	int render_handle_frame(EDL *edl, int64_t pos, int mode);
	int update_drag_floatauto(int cursor_x, int cursor_y);
	int update_drag_toggleauto(int cursor_x, int cursor_y);
	int update_drag_auto(int cursor_x, int cursor_y);
	int update_drag_pluginauto(int cursor_x, int cursor_y);
	void show_message(Auto *current, int show_curve_type, const char *fmt, ...);

// Update status bar to reflect drag operation
	void update_drag_caption();

	int get_title_h();
	int resource_h();

// Display hourglass if timer expired
	void test_timer();
// get the relevant patchbay by traversing the panes
//	Patchbay* get_patchbay();

	MWindow *mwindow;
	MWindowGUI *gui;
	TimelinePane *pane;
// Allows overlays to get redrawn without redrawing the resources
	BC_Pixmap *background_pixmap;
	BC_Pixmap *transition_pixmap;
	EditHandles *edit_handles;
//	TransitionHandles *transition_handles;
	BC_Pixmap *keyframe_pixmap;
	BC_Pixmap *camerakeyframe_pixmap;
	BC_Pixmap *modekeyframe_pixmap;
	BC_Pixmap *pankeyframe_pixmap;
	BC_Pixmap *projectorkeyframe_pixmap;
	BC_Pixmap *maskkeyframe_pixmap;

	int active;
// Currently in a drag scroll operation
	int drag_scroll;
// Don't stop hourglass if it was never started before the operation.
	int hourglass_enabled;
// position used by timebar, when the timebar needs to draw a highlight
	double timebar_position;

// Temporary for picon drawing
	VFrame *temp_picon;
// Timer for hourglass
	Timer *resource_timer;
	Timer *render_timer;

// Plugin toggle interfaces
	ArrayList<PluginOn*> plugin_on_toggles;
	ArrayList<PluginShow*> plugin_show_toggles;
	ArrayList<PluginPresetEdit*> preset_edit_buttons;

	static int auto_operations[];
// event handlers
	void draw_paste_destination();

	void draw_floatauto_ctrlpoint(int x, int y, int cp_x, int cp_y,
		int center_pixel, int zoom_track, int color);

	float value_to_percentage(float auto_value, int autogrouptype);
	// transforms automation value into current display coords
	// dependant on current automation display range for given kind of automation

// ====================================== cursor selection type

	double selection_midpoint;        // division between current ends
	int snapped;			// drag handle snapping
	EDL *speed_edl;			// drag speed handle start edl
};

#endif
