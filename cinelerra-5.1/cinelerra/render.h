
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

#ifndef RENDER_H
#define RENDER_H


#include "asset.inc"
#include "batchrender.inc"
#include "bcdialog.h"
#include "bitspopup.h"
#include "browsebutton.h"
#include "cache.inc"
#include "compresspopup.h"
#include "condition.inc"
#include "bchash.inc"
#include "edit.inc"
#include "errorbox.inc"
#include "file.inc"
#include "formatpopup.inc"
#include "formattools.h"
#include "guicast.h"
#include "loadmode.inc"
#include "mainprogress.inc"
#include "maxchannels.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "packagedispatcher.inc"
#include "packagerenderer.h"
#include "playabletracks.inc"
#include "preferences.inc"
#include "bcprogressbox.inc"
#include "render.inc"
#include "track.inc"
#include "transportque.inc"
#include "vframe.inc"
#include "renderprofiles.inc"


class RenderItem : public BC_MenuItem
{
public:
	RenderItem(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class RenderProgress : public Thread
{
public:
	RenderProgress(MWindow *mwindow, Render *render);
	~RenderProgress();

	void run();


	MWindow *mwindow;
	Render *render;

	int64_t last_value;
};



class MainPackageRenderer : public PackageRenderer
{
public:
	MainPackageRenderer(Render *render);
	~MainPackageRenderer();


	int get_master();
	int get_result();
	void set_result(int value);
	void set_progress(int64_t value);
	int progress_cancelled();

	Render *render;
};

class RenderWindow;


class Render : public BC_DialogThread
{
public:
	Render(MWindow *mwindow);
	~Render();

// Start dialogue box and render interactively.
	void start_interactive();
// Start batch rendering jobs.
// A new thread is created and the rendering is interactive.
	void start_batches(ArrayList<BatchRenderJob*> *jobs);
// The batches are processed in the foreground in non interactive mode.
	void start_batches(ArrayList<BatchRenderJob*> *jobs,
		BC_Hash *boot_defaults, Preferences *batch_prefs);
// Called by BatchRender to stop the operation.
	void stop_operation();
	BC_Window* new_gui();

	void handle_done_event(int result);
	void handle_close_event(int result);
	void start_render();

	int load_defaults(Asset *asset);
	int save_defaults(Asset *asset);
	int load_profile(int profile_slot, Asset *asset);
	double get_render_range();
// force asset parameters regardless of window
// This should be integrated into the Asset Class.
	static int check_asset(EDL *edl, Asset &asset);
// strategy to conform with using renderfarm.
	static int get_strategy(int use_renderfarm, int use_labels);
	int get_strategy();
// Force filename to have a 0 padded number if rendering to a list.
	int check_numbering(Asset &asset);
	static void create_filename(char *path,
		char *default_path,
		int current_number,
		int total_digits,
		int number_start);
	static void get_starting_number(char *path,
		int &current_number,
		int &number_start,
		int &total_digits,
		int min_digits = 3);
	int direct_frame_copy(EDL *edl, int64_t &render_video_position, File *file);
	int direct_copy_possible(EDL *edl,
			int64_t current_position,
			Track* playable_track,  // The one track which is playable
			Edit* &playable_edit, // The edit which is playing
			File *file);   // Output file

	void start_progress();
	void stop_progress();
	void show_progress();

// Procedure the run function should use.
	int mode;
	enum
	{
		INTERACTIVE,
		BATCH
	};
// When batch rendering is cancelled from the batch dialog
	int batch_cancelled;


	int load_mode;
	int in_progress;
// Background compression must be disabled when direct frame copying and reenabled afterwards
	int direct_frame_copying;
// beep on done
	int beep;

	Preferences *preferences;
	VFrame *compressed_output;
	MainProgressBar *progress;
	RenderProgress *render_progress;
	RenderThread *thread;
	MWindow *mwindow;
	PlayableTracks *playable_tracks;
	PackageDispatcher *packages;
	Mutex *package_lock, *counter_lock;
	int use_labels;
	int range_type;
// Total selection to render in seconds
	double total_start, total_end;
// External Render farm checks this every frame.
	int result;
	Asset *default_asset;
// Asset containing the file format
	Asset *asset;
// Jobs pointer passed to start_batches
	ArrayList<BatchRenderJob*> *jobs;
// Used by batch rendering to wait until rendering is finished
	Condition *completion;

// Total samples updated by the render farm and the local renderer.
// This avoids rounding errors and complies with the use of samples for
// timing.
	int64_t total_rendered;
// Time used in last render
	double elapsed_time;

// Current open RenderWindow
	RenderWindow *render_window;

// For non interactive mode, maintain progress here.
	int64_t progress_max;
	Timer *progress_timer;
	int64_t last_eta;
};


class RenderThread : public Thread
{
public:
	RenderThread(MWindow *mwindow, Render *render);
	~RenderThread();

	void run();

	void render_single(int test_overwrite,
		Asset *asset,
		EDL *edl,
		int strategy,
		int range_type);

	MWindow *mwindow;
	Render *render;
};

class RenderToTracks;


class RenderRangeProject : public BC_Radial
{
public:
	RenderRangeProject(RenderWindow *rwindow, int value, int x, int y);
	int handle_event();
	RenderWindow *rwindow;
};

class RenderRangeSelection : public BC_Radial
{
public:
	RenderRangeSelection(RenderWindow *rwindow, int value, int x, int y);
	int handle_event();
	RenderWindow *rwindow;
};


class RenderRangeInOut : public BC_Radial
{
public:
	RenderRangeInOut(RenderWindow *rwindow, int value, int x, int y);
	int handle_event();
	RenderWindow *rwindow;
};


class RenderRange1Frame : public BC_Radial
{
public:
	RenderRange1Frame(RenderWindow *rwindow, int value, int x, int y);
	int handle_event();
	RenderWindow *rwindow;
};


class RenderFormat : public FormatTools
{
public:
	RenderFormat(MWindow *mwindow, BC_WindowBase *window, Asset *asset);
	~RenderFormat();
	void update_format();
};


class RenderBeepOnDone : public BC_CheckBox
{
public:
	RenderBeepOnDone(RenderWindow *rwindow, int x, int y);
	int handle_event();

	RenderWindow *rwindow;
};


class RenderWindow : public BC_Window
{
public:
	RenderWindow(MWindow *mwindow,
		Render *render,
		Asset *asset,
		int x,
		int y);
	~RenderWindow();

	void create_objects();
	void enable_render_range(int v);
	void update_range_type(int range_type);
	void load_profile(int profile_slot);

	RenderRangeProject *rangeproject;
	RenderRangeSelection *rangeselection;
	RenderRangeInOut *rangeinout;
	RenderRange1Frame *range1frame;
	RenderBeepOnDone *beep_on_done;

	RenderProfile *renderprofile;

	LoadMode *loadmode;
	RenderFormat *render_format;

	MWindow *mwindow;
	Render *render;
	Asset *asset;
};

#endif
