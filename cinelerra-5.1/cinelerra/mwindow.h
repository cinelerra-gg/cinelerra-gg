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

#ifndef MWINDOW_H
#define MWINDOW_H

#include <stdio.h>
#include <stdint.h>

#include "apatchgui.h"
#include "arraylist.h"
#include "asset.inc"
#include "assets.inc"
#include "audiodevice.inc"
#include "autos.inc"
#include "awindow.inc"
#include "batchrender.inc"
#include "bcwindowbase.inc"
#include "bdcreate.inc"
#include "brender.inc"
#include "cache.inc"
#include "channel.inc"
#include "channeldb.inc"
#include "commercials.inc"
#include "cwindow.inc"
#include "bchash.inc"
#include "devicedvbinput.inc"
#include "devicempeginput.inc"
#include "dvdcreate.inc"
#include "edit.inc"
#include "edl.inc"
#include "edlsession.inc"
#include "exportedl.inc"
#include "filesystem.inc"
#include "filexml.inc"
#include "framecache.inc"
#include "gwindow.inc"
#include "indexable.inc"
#include "keyframegui.inc"
#include "levelwindow.inc"
#include "loadmode.inc"
#include "mainerror.inc"
#include "mainindexes.inc"
#include "mainprogress.inc"
#include "mainsession.inc"
#include "mainundo.inc"
#include "maxchannels.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "new.inc"
#include "patchbay.inc"
#include "playback3d.inc"
#include "playbackengine.inc"
#include "plugin.inc"
#include "pluginfclient.inc"
#include "pluginserver.inc"
#include "pluginset.inc"
#include "preferences.inc"
#include "preferencesthread.inc"
#include "proxy.inc"
#include "record.inc"
#include "recordlabel.inc"
#include "render.inc"
#include "sharedlocation.inc"
#include "sighandler.inc"
#include "splashgui.inc"
#include "theme.inc"
#include "thread.h"
#include "threadloader.inc"
#include "timebar.inc"
#include "timebomb.h"
#include "tipwindow.inc"
#include "track.inc"
#include "tracking.inc"
#include "tracks.inc"
#include "transition.inc"
#include "transportque.inc"
#include "videowindow.inc"
#include "vpatchgui.h"
#include "vwindow.inc"
#include "zwindow.inc"
#include "wwindow.inc"
#include "wavecache.inc"

#define FONT_SEARCHPATH "fonts"

// All entry points for commands except for window locking should be here.
// This allows scriptability.

class MWindow : public Thread
{
public:
	MWindow();
	~MWindow();

// ======================================== initialization commands
	void create_objects(int want_gui,
		int want_new,
		char *config_path);
	void show_splash();
	void hide_splash();
	void start();
	void run();

	int run_script(FileXML *script);
	int new_project();
	int delete_project(int flash = 1);
	void quit();
	int restart() { return restart_status; }

	int load_defaults();
	int save_defaults();
	int set_filename(const char *filename);
// Total vertical pixels in timeline
	int get_tracks_height();
// Total horizontal pixels in timeline
	int get_tracks_width();
// Show windows
	void show_vwindow();
	void show_awindow();
	void show_lwindow();
	void show_cwindow();
	void show_gwindow();
	void hide_gwindow();
	void restore_windows();
	void save_layout(int no);
	void load_layout(int no);
	int tile_windows(int window_config);
	char *get_cwindow_display();
	void set_screens(int value);
	int asset_to_edl(EDL *new_edl,
		Asset *new_asset,
		RecordLabels *labels = 0);

// Entry point to insert assets and insert edls.  Called by TrackCanvas
// and AssetPopup when assets are dragged in from AWindow.
// Takes the drag vectors from MainSession and
// pastes either assets or clips depending on which is full.
// Returns 1 if the vectors were full
	int paste_assets(double position, Track *dest_track, int overwrite);

// Insert the assets at a point in the EDL.  Called by menueffects,
// render, and CWindow drop but recording calls paste_edls directly for
// labels.
	void load_assets(ArrayList<Indexable*> *new_assets,
		double position,
		int load_mode,
		Track *first_track /* = 0 */,
		RecordLabels *labels /* = 0 */,
		int edit_labels,
		int edit_plugins,
		int edit_autos,
		int overwrite);
	int paste_edls(ArrayList<EDL*> *new_edls,
		int load_mode,
		Track *first_track /* = 0 */,
		double current_position /* = -1 */,
		int edit_labels,
		int edit_plugins,
		int edit_autos,
		int overwrite);
// Reset everything for a load
	void update_project(int load_mode);
	void update_vwindow();
// Fit selected time to horizontal display range
	void fit_selection();
	void selected_to_clipboard();
// Fit selected autos to the vertical display range
	void fit_autos(int doall);
	void change_currentautorange(int autogrouptype, int increment, int changemax);
	void expand_autos(int changeall, int domin, int domax);
	void shrink_autos(int changeall, int domin, int domax);
// move the window to include the cursor
	void find_cursor();
// Search plugindb and put results in argument
	static void search_plugindb(int do_audio,
		int do_video,
		int is_realtime,
		int is_transition,
		int is_theme,
		ArrayList<PluginServer*> &results);
// Find the plugin whose title matches title and return it
	static PluginServer* scan_plugindb(char *title,
		int data_type);
	static void fix_plugin_title(char *title);
	static int plugin_exists(const char *plugin_path, ArrayList<PluginServer*> &plugins);
	static int plugin_exists(char *plugin_path);
	void dump_plugindb(FILE *fp);
	void stop_playback(int wait);
	void stop_transport();

	void queue_mixers(EDL *edl, int command, int wait_tracking,
		int use_inout, int update_refresh, int toggle_audio, int loop_play);
	ZWindow *create_mixer(Indexable *indexable);
	void create_mixers();
	void refresh_mixers(int dir=1);
	void stop_mixers();
	void close_mixers(int destroy=1);
	void open_mixers();
	ZWindow *get_mixer(Mixer *&mixer);
	void del_mixer(ZWindow *zwindow);
	int mixer_track_active(Track *track);
	void update_mixer_tracks();
	void start_mixer();
	int select_zwindow(ZWindow *zwindow);
	void tile_mixers();

	int load_filenames(ArrayList<char*> *filenames,
		int load_mode = LOADMODE_REPLACE,
// Cause the project filename on the top of the window to be updated.
// Not wanted for loading backups.
		int update_filename = 1);


// Print out plugins which are referenced in the EDL but not loaded.
	void test_plugins(EDL *new_edl, char *path);

	int interrupt_indexes();  // Stop index building

	int redraw_time_dependancies();     // after reconfiguring the time format, sample rate, frame rate

// =========================================== movement

	void next_time_format();
	void prev_time_format();
	void time_format_common();
	int reposition_timebar(int new_pixel, int new_height);
	int expand_sample();
	int zoom_in_sample();
	int zoom_sample(int64_t zoom_sample);
	void zoom_autos(float min, float max);
	void zoom_amp(int64_t zoom_amp);
	void zoom_track(int64_t zoom_track);
	int fit_sample();
	int move_left(int64_t distance = 0);
	int move_right(int64_t distance = 0);
	void move_up(int64_t distance = 0);
	void move_down(int64_t distance = 0);
	int find_selection(double position, int scroll_display = 0);
	void toggle_camera_xyz();
	void toggle_projector_xyz();

// seek to labels
// shift_down must be passed by the caller because different windows call
// into this
	int next_label(int shift_down);
	int prev_label(int shift_down);
// seek to edit handles
	int next_edit_handle(int shift_down);
	int prev_edit_handle(int shift_down);
// seek to keyframes
	int nearest_plugin_keyframe(int shift_down, int dir);
// offset is pixels to add to track_start
	void trackmovement(int offset, int pane_number);
// view_start is pixels
	int samplemovement(int64_t view_start, int pane_number);
	void select_all();
	int goto_start();
	int goto_end();
	int goto_position(double position);
	int expand_y();
	int zoom_in_y();
	int expand_t();
	int zoom_in_t();
	void split_x();
	void split_y();
	void crop_video();
	void update_plugins();
// Call after every edit operation
	void save_backup();
	void load_backup();
	void show_plugin(Plugin *plugin);
	void hide_plugin(Plugin *plugin, int lock);
	void hide_plugins();
	void delete_plugin(PluginServer *plugin);
// Update plugins with configuration changes.
// Called by TrackCanvas::cursor_motion_event.
	void update_plugin_guis(int do_keyframe_guis = 1);
	void update_plugin_states();
	void update_plugin_titles();
// Called by Attachmentpoint during playback.
// Searches for matching plugin and renders data in it.
	void render_plugin_gui(void *data, Plugin *plugin);
	void render_plugin_gui(void *data, int size, Plugin *plugin);

// Called from PluginVClient::process_buffer
// Returns 1 if a GUI for the plugin is open so OpenGL routines can determine if
// they can run.
	int plugin_gui_open(Plugin *plugin);

	void show_keyframe_gui(Plugin *plugin);
	void hide_keyframe_guis();
	void hide_keyframe_gui(Plugin *plugin);
	void update_keyframe_guis();


// ============================= editing commands ========================

// Map each recordable audio track to the desired pattern
	void map_audio(int pattern);
	void remap_audio(int pattern);
	enum
	{
		AUDIO_5_1_TO_2,
		AUDIO_1_TO_1
	};
	void add_audio_track_entry(int above, Track *dst);
	int add_audio_track(int above, Track *dst);
	void add_clip_to_edl(EDL *edl);
	void add_video_track_entry(Track *dst = 0);
	int add_video_track(int above, Track *dst);
	void add_subttl_track_entry(Track *dst = 0);
	int add_subttl_track(int above, Track *dst);

	void asset_to_all();
	void asset_to_size();
	void asset_to_rate();
// Entry point for clear operations.
	void clear_entry();
// Clears active region in EDL.
// If clear_handle, edit boundaries are cleared if the range is 0.
// Called by paste, record, menueffects, render, and CWindow drop.
	void clear(int clear_handle);
	void clear_labels();
	int clear_labels(double start, double end);
	void concatenate_tracks();
	void copy();
	int copy(double start, double end);
	void cut();
	void blade(double position);
	void cut(double start, double end, double new_position=-1);
// cut edit from current position to handle/label
	void cut_left_edit();
	void cut_right_edit();
	void cut_left_label();
	void cut_right_label();

// Calculate aspect ratio from pixel counts
	static int create_aspect_ratio(float &w, float &h, int width, int height);
// Calculate defaults path
	static void create_defaults_path(char *string, const char *config_file);

	void delete_track();
	void delete_track(Track *track);
	void delete_tracks();
	int feather_edits(int64_t feather_samples, int audio, int video);
	int64_t get_feather(int audio, int video);
	float get_aspect_ratio();
	void insert(double position,
		FileXML *file,
		int edit_labels,
		int edit_plugins,
		int edit_autos,
		EDL *parent_edl /* = 0 */);

// TrackCanvas calls this to insert multiple effects from the drag_pluginservers
// into pluginset_highlighted.
	void insert_effects_canvas(double start,
		double length);

// CWindow calls this to insert multiple effects from
// the drag_pluginservers array.
	void insert_effects_cwindow(Track *dest_track);

// Attach new effect to all recordable tracks
// single_standalone - attach 1 standalone on the first track and share it with
// other tracks
	void insert_effect(char *title,
		SharedLocation *shared_location,
		int data_type,
		int plugin_type,
		int single_standalone);

// This is called multiple times by the above functions.
// It can't sync parameters.
	void insert_effect(char *title,
		SharedLocation *shared_location,
		Track *track,
		PluginSet *plugin_set,
		double start,
		double length,
		int plugin_type);

	void match_output_size(Track *track);
	void delete_edit(Edit *edit, const char *msg, int collapse=0);
	void delete_edits(ArrayList<Edit*> *edits, const char *msg, int collapse=0);
	void delete_edits(int collapse=0);
	void cut_selected_edits(int collapse=0);
// Move edit to new position
	void move_edits(ArrayList<Edit*> *edits,
		Track *track,
		double position,
		int behaviour);       // behaviour: 0 - old style (cut and insert elswhere), 1- new style - (clear and overwrite elsewere)
// Move effect to position
	void move_effect(Plugin *plugin,
		Track *track,
		int64_t position);
	void move_effect(Plugin *plugin,
		PluginSet *plugin_set,
		int64_t position);
	void move_plugins_up(PluginSet *plugin_set);
	void move_plugins_down(PluginSet *plugin_set);
	void move_track_down(Track *track);
	void move_tracks_down();
	void move_track_up(Track *track);
	void move_tracks_up();
	void mute_selection();
	void new_folder(const char *new_folder, int is_clips);
	void delete_folder(char *folder);
// For clipboard commands
	void paste();
// For splice and overwrite
	void overwrite(EDL *source, int all);
	void splice(EDL *source, int all);
	int paste(double start, double end, FileXML *file,
		int edit_labels, int edit_plugins, int edit_autos);
	int paste_output(int64_t startproject, int64_t endproject,
		int64_t startsource_sample, int64_t endsource_sample,
		int64_t startsource_frame, int64_t endsource_frame,
		Asset *asset, RecordLabels *new_labels);
	void paste_silence();

// Detach single transition
	void detach_transition(Transition *transition);
// Detach all transitions in selection
	void detach_transitions();
// Attach dragged transition
	void paste_transition();
// Attach transition to all edits in selection
	void paste_transitions(int track_type, char *title);
// Attach transition dragged onto CWindow
	void paste_transition_cwindow(Track *dest_track);
// Attach default transition to single edit
	void paste_audio_transition();
	void paste_video_transition();
	void shuffle_edits();
	void reverse_edits();
	void align_edits();
	void set_edit_length(double length);
// Set length of single transition
	void set_transition_length(Transition *transition, double length);
// Set length in seconds of all transitions in active range
	void set_transition_length(double length);

	void remove_indexfile(Indexable *indexable);
	void rebuild_indices();
// Asset removal from caches
	void reset_caches();
	void remove_asset_from_caches(Asset *asset);
	void remove_assets_from_project(int push_undo /* = 0 */,
		int redraw /* 1 */,
		ArrayList<Indexable*> *drag_assets /* mwindow->session->drag_assets */,
		ArrayList<EDL*> *drag_clips /* mwindow->session->drag_clips */);
	void remove_assets_from_disk();
	void resize_track(Track *track, int w, int h);

	void set_automation_mode(int mode);
	void set_keyframe_type(int mode);
	void set_auto_keyframes(int value, int lock_mwindow, int lock_cwindow);
	void set_auto_visibility(Autos *autos, int value);
	void set_labels_follow_edits(int value);

// Update the editing mode
	int set_editing_mode(int new_editing_mode, int lock_mwindow, int lock_cwindow);
	void toggle_editing_mode();
	void set_inpoint(int is_mwindow);
	void set_outpoint(int is_mwindow);
	void unset_inoutpoint(int is_mwindow);
	void toggle_loop_playback();
	void trim_selection();
// Synchronize EDL settings with all playback engines depending on current
// operation.  Doesn't redraw anything.
	void sync_parameters(int change_type = CHANGE_PARAMS);
	void save_clip(EDL *new_edl, const char *txt);
	void to_clip(EDL *edl, const char *txt, int all);
	int toggle_label(int is_mwindow);
	void undo_entry(BC_WindowBase *calling_window_gui);
	void redo_entry(BC_WindowBase *calling_window_gui);
	void save_undo_data();
	void load_undo_data();
	int copy_target(const char *path, const char *target);
	int link_target(const char *real_path, const char *link_path, int relative);
	void save_project(const char *dir, int save_mode, int overwrite, int reload);

	int cut_automation();
	int copy_automation();
	int paste_automation();
	void clear_automation();
	int cut_default_keyframe();
	int copy_default_keyframe();
// Use paste_automation to paste the default keyframe in other position.
// Use paste_default_keyframe to replace the default keyframe with whatever is
// in the clipboard.
	int paste_default_keyframe();
	int clear_default_keyframe();

	FloatAuto* get_float_auto(PatchGUI *patch,int idx);
	IntAuto* get_int_auto(PatchGUI *patch,int idx);
	PanAuto* get_pan_auto(PatchGUI *patch);
	PatchGUI *get_patchgui(Track *track);

	int modify_edithandles();
	int modify_pluginhandles();
	void finish_modify_handles();
	void rescale_proxy(EDL *clip, int orig_scale, int new_scale);
	void add_proxy(int use_scaler,
		ArrayList<Indexable*> *orig_assets,
		ArrayList<Indexable*> *proxy_assets);
	int render_proxy(ArrayList<Indexable *> &new_idxbls);
	void beep(double freq, double secs, double gain);
	int enable_proxy();
	int disable_proxy();
	int to_proxy(Asset *asset, int new_scale, int new_use_scaler);
	ProxyBeep *proxy_beep;

	void dump_plugins(FILE *fp=stdout);
	void dump_edl(FILE *fp=stdout);
	void dump_undo(FILE *fp=stdout);
	void dump_exe(FILE *fp=stdout);
	static void trap_hook(FILE *fp, void *vp);

	void reset_android_remote();

// Send new EDL to caches
	void age_caches();
	int optimize_assets();            // delete unused assets from the cache and assets

	void select_point(double position);
	int set_loop_boundaries();         // toggle loop playback and set boundaries for loop playback


	Playback3D *playback_3d;
	SplashGUI *splash_window;

// Main undo stack
	MainUndo *undo;
	BC_Hash *defaults;
	Assets *assets;
// CICaches for drawing timeline only
	CICache *audio_cache, *video_cache;
// Frame cache for drawing timeline only.
// Cache drawing doesn't wait for file decoding.
	FrameCache *frame_cache;
	WaveCache *wave_cache;
	Preferences *preferences;
	PreferencesThread *preferences_thread;
	MainSession *session;
	Theme *theme;
	MainIndexes *mainindexes;
	MainProgress *mainprogress;
	BRender *brender;
	char cin_lang[4];
	int brender_active;
	const char *default_standard;
	static Commercials *commercials;
	int commercial_active;
	int has_commercials();
// copy of edl created in speed_before, used in speed_after to normalize_speed
	EDL *speed_edl;

// Menu items
	ArrayList<ColormodelItem*> colormodels;
	ArrayList<InterlacemodeItem*>          interlace_project_modes;
	ArrayList<InterlacemodeItem*>          interlace_asset_modes;
	ArrayList<InterlacefixmethodItem*>     interlace_asset_fixmethods;

	int reset_meters();
	void resync_guis();

	int select_asset(Asset *asset, int vstream, int astream, int delete_tracks);
	int select_asset(int vtrack, int delete_tracks);

// Channel DB for playback only.  Record channel DB's are in record.C
	ChannelDB *channeldb_buz;
	ChannelDB *channeldb_v4l2jpeg;

// ====================================== plugins ==============================

// Contains file descriptors for all the dlopens
	static ArrayList<PluginServer*> *plugindb;
// Currently visible plugins
	int64_t plugin_visibility;
	ArrayList<PluginServer*> *plugin_guis;
// GUI Plugins to delete
	ArrayList<PluginServer*> *dead_plugins;
// Keyframe editors
	ArrayList<KeyFrameThread*> *keyframe_threads;

// Adjust sample position to line up with frames.
	int fix_timing(int64_t &samples_out,
		int64_t &frames_out,
		int64_t samples_in);


	CreateBD_Thread *create_bd;
	CreateDVD_Thread *create_dvd;
	BatchRenderThread *batch_render;
	Render *render;

 	ExportEDL *exportedl;


// Master edl
	EDL *edl;
// Main Window GUI
	MWindowGUI *gui;
// Compositor
	CWindow *cwindow;
// Viewer
	Mutex *vwindows_lock;
	ArrayList<VWindow*> vwindows;
// Mixer
	Mutex *zwindows_lock;
	ArrayList<ZWindow*> zwindows;
// Asset manager
	AWindow *awindow;
// Automation window
	GWindow *gwindow;
// Tip of the day
	TipWindow *twindow;
// Warning window
	WWindow *wwindow;
	void show_warning(int *do_warning, const char *text);
	int wait_warning();
// Levels
	LevelWindow *lwindow;
	Mutex *run_lock;
// Lock during creation and destruction of GUI
	Mutex *plugin_gui_lock;
	Mutex *dead_plugin_lock;
	Mutex *keyframe_gui_lock;
// Lock during creation and destruction of brender so playback doesn't use it.
	Mutex *brender_lock;
// Initialize shared memory
	void init_shm(const char *pfn, int64_t min);
// Initialize channel DB's for playback
	void init_channeldb();
	void init_render();
	void init_exportedl();
// These three happen synchronously with each other
// Make sure this is called after synchronizing EDL's.
	void init_brender();
// Restart brender after testing its existence
	void restart_brender();
// Stops brender after testing its existence
	void stop_brender();
// This one happens asynchronously of the others.  Used by playback to
// see what frame is background rendered.
	int brender_available(int position);
	void set_brender_active(int v, int update=1);
	int put_commercial();
	void activate_commercial() { commercial_active = 1; }
	void commit_commercial();
	void undo_commercial();
	void cut_commercials();
	void update_gui(int changed_edl);
	int paste_subtitle_text(char *text, double start, double end);

	void init_error();
	void finit_error();
	static void init_defaults(BC_Hash* &defaults, char *config_path);
	void check_language();
	const char *default_std();
	void fill_preset_defaults(const char *preset, EDLSession *session);
	const char *get_preset_name(int index);
	void init_edl();
	void init_awindow();
	void init_gwindow();
	void init_tipwindow();
// Used by MWindow and RenderFarmClient
	static void get_plugin_path(char *path, const char *plug_dir, const char *fs_path);
	static int init_plugins(MWindow *mwindow, Preferences *preferences);
	static int init_ladspa_plugins(MWindow *mwindow, Preferences *preferences);
	static void init_plugin_tips(ArrayList<PluginServer*> &plugins, const char *lang);
	static int check_plugin_index(ArrayList<PluginServer*> &plugins,
		const char *plug_dir, const char *plug_path);
	static void init_plugin_index(MWindow *mwindow, Preferences *preferences,
		FILE *fp, const char *plugin_dir);
	static int init_ladspa_index(MWindow *mwindow, Preferences *preferences,
		FILE *fp, const char *plugin_dir);
	static void scan_plugin_index(MWindow *mwindow, Preferences *preferences,
		FILE *fp, const char *plug_dir, const char *plug_path, int &idx);
	static void init_ffmpeg();
	static void init_ffmpeg_index(MWindow *mwindow, Preferences *preferences, FILE *fp);
	static int load_plugin_index(MWindow *mwindow, FILE *fp, const char *plugin_dir);
	static PluginServer *new_ffmpeg_server(MWindow *mwindow, const char *name);
	static int init_lv2_index(MWindow *mwindow, Preferences *preferences, FILE *fp);
	static PluginServer *new_lv2_server(MWindow *mwindow, const char *name);
	static void remove_plugin_index();

	void init_preferences();
	void init_signals();
	void init_theme();
	void init_compositor();
	void init_levelwindow();
// Called when creating a new viewer to view footage
	VWindow* get_viewer(int start_it, int idx=-1);
	void init_cache();
	void init_menus();
	void init_indexes();
	void init_gui();
	void init_3d();
	void init_playbackcursor();
	void init_commercials();
	static void add_plugins(ArrayList<PluginServer*> &plugins);
	static void delete_plugins();
	void speed_before();
	int speed_after(int done);
	int normalize_speed(EDL *old_edl, EDL *new_edl);
//
	void clean_indexes();
//	TimeBomb timebomb;
	SigHandler *sighandler;
	int restart_status;
	int screens;
	int in_destructor;
};

#endif
