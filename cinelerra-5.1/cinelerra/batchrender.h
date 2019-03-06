
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

#ifndef BATCHRENDER_H
#define BATCHRENDER_H

#include "arraylist.h"
#include "asset.inc"
#include "batchrender.inc"
#include "bcbutton.h"
#include "bcdialog.h"
#include "browsebutton.inc"
#include "filexml.inc"
#include "formattools.h"
#include "indexable.inc"
#include "keyframe.inc"
#include "mwindow.inc"
#include "preferences.inc"
#include "render.inc"
#include "timeentry.h"

enum {
	ENABLED_COL, LABELED_COL, FARMED_COL,
	OUTPUT_COL, EDL_COL, ELAPSED_COL,
	BATCHRENDER_COLUMNS
};

class BatchRenderMenuItem : public BC_MenuItem
{
public:
	BatchRenderMenuItem(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};



class BatchRenderJob
{
public:
	BatchRenderJob(const char *tag,
		Preferences *preferences, int labeled, int farmed);
	BatchRenderJob(Preferences *preferences, int labeled, int farmed);
	BatchRenderJob(const char *tag);
	virtual ~BatchRenderJob();
	virtual BatchRenderJob *copy();
	virtual void load(FileXML *file);
	virtual void save(FileXML *file);
	virtual int get_strategy();
	virtual char *create_script(EDL *edl, ArrayList<Indexable *> *idxbls);
	void copy_from(BatchRenderJob *src);

	const char *tag;
// Source EDL to render
	char edl_path[BCTEXTLEN];
// Destination file for output
	Asset *asset;
	int labeled, farmed;
	int enabled;
// Amount of time elapsed in last render operation
	double elapsed;
	Preferences *preferences;
};








class BatchRenderThread : public BC_DialogThread
{
public:
	BatchRenderThread(MWindow *mwindow=0);
	~BatchRenderThread();
	void handle_close_event(int result);
	BC_Window* new_gui();

	int test_edl_files();
	void calculate_dest_paths(ArrayList<char*> *paths,
		Preferences *preferences);
	void reset(const char *path=0);
// Load batch rendering jobs
	void load_jobs(char *path, Preferences *preferences);
// Not applicable to western civilizations
	void save_jobs(char *path=0);
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);
// Create path for persistent storage functions
	char* create_path(char *string);
	void new_job();
	void delete_job();
	void update_selected_edl();
	void use_current_edl();
// Conditionally returns the job or the default job based on current_job
	BatchRenderJob* get_current_job();
	Asset* get_current_asset();
	char* get_current_edl();
// For command line usage
	void start_rendering(char *config_path, char *batch_path);
// For GUI usage
	void start_rendering();
	void stop_rendering();
// Highlight the currently rendering job.
	void update_active(int number);
	void update_done(int number, int create_list, double elapsed_time);
	void move_batch(int src, int dst);
	void start(int do_farmed, int do_labeled);
	static void trap_hook(FILE *fp, void *vp);

	MWindow *mwindow;
	char batch_path[BCTEXTLEN];
	double current_start;
	double current_end;
	BatchRenderJob *default_job;
	ArrayList<BatchRenderJob*> jobs;
	BC_Hash *boot_defaults;
	Preferences *preferences;
	Render *render;
	BatchRenderGUI *gui;

	static const char *column_titles[BATCHRENDER_COLUMNS];
	static int column_widths[BATCHRENDER_COLUMNS];
	int list_width[BATCHRENDER_COLUMNS];
	int do_farmed;
	int do_labeled;
// job being edited
	int current_job;
// job being rendered
	int rendering_job;
	int is_rendering;
	int warn;
};



class BatchRenderEDLPath : public BC_TextBox
{
public:
	BatchRenderEDLPath(BatchRenderThread *thread, int x, int y, int w,
		char *text);
	int handle_event();
	BatchRenderThread *thread;
};


class BatchRenderCurrentEDL : public BC_GenericButton
{
public:
	BatchRenderCurrentEDL(BatchRenderThread *thread, int x, int y);
	int handle_event();
	BatchRenderThread *thread;
};


class BatchRenderUpdateEDL : public BC_GenericButton
{
public:
	BatchRenderUpdateEDL(BatchRenderThread *thread, int x, int y);
	int handle_event();
	BatchRenderThread *thread;
};


class BatchRenderNew : public BC_GenericButton
{
public:
	BatchRenderNew(BatchRenderThread *thread, int x, int y);
	int handle_event();
	BatchRenderThread *thread;
};

class BatchRenderDelete : public BC_GenericButton
{
public:
	BatchRenderDelete(BatchRenderThread *thread, int x, int y);
	int handle_event();
	BatchRenderThread *thread;
};



class BatchRenderSaveList : public BC_GenericButton, public Thread
{
public:
	BatchRenderSaveList(BatchRenderThread *thread, int x, int y);
	~BatchRenderSaveList();
	int handle_event();
	BatchRenderThread *thread;
	BC_FileBox *gui;
	void run();
	void stop();
	virtual int keypress_event();
	Mutex *startup_lock;
};

class BatchRenderLoadList : public BC_GenericButton, public Thread
{
public:
	BatchRenderLoadList(BatchRenderThread *thread, int x, int y);
	~BatchRenderLoadList();
	int handle_event();
	BatchRenderThread *thread;
	BC_FileBox *gui;
	void run();
	void stop();
	virtual int keypress_event();
	Mutex *startup_lock;
};



class BatchRenderList : public BC_ListBox
{
public:
	BatchRenderList(BatchRenderThread *thread, int x, int y, int w, int h);
	int handle_event();
	int selection_changed();
	int column_resize_event();
	int drag_start_event();
	int drag_motion_event();
	int drag_stop_event();
	int dragging_item;
	BatchRenderThread *thread;
};
class BatchRenderStart : public BC_GenericButton
{
public:
	BatchRenderStart(BatchRenderThread *thread, int x, int y);
	int handle_event();
	BatchRenderThread *thread;
};

class BatchRenderStop : public BC_GenericButton
{
public:
	BatchRenderStop(BatchRenderThread *thread, int x, int y);
	int handle_event();
	BatchRenderThread *thread;
};

class BatchRenderWarning : public BC_CheckBox
{
public:
	BatchRenderWarning(BatchRenderThread *thread, int x, int y);
	int handle_event();
	BatchRenderThread *thread;
};

class BatchRenderCancel : public BC_GenericButton
{
public:
	BatchRenderCancel(BatchRenderThread *thread, int x, int y);
	int handle_event();
	int keypress_event();
	BatchRenderThread *thread;
};


class BatchFormat : public FormatTools
{
public:
	BatchFormat(MWindow *mwindow, BatchRenderGUI *gui, Asset *asset);
	~BatchFormat();

	int handle_event();

	BatchRenderGUI *gui;
	MWindow *mwindow;
};

class BatchRenderUseFarm : public BC_CheckBox
{
public:
	BatchRenderUseFarm(BatchRenderThread *thread, int x, int y, int *output);
	int handle_event();
	void update(int *output);

	BatchRenderThread *thread;
	int *output;
};


class BatchRenderGUI : public BC_Window
{
public:
	BatchRenderGUI(MWindow *mwindow,
		BatchRenderThread *thread,
		int x,
		int y,
		int w,
		int h);
	~BatchRenderGUI();

	void create_objects();
	int resize_event(int w, int h);
	int translation_event();
	int close_event();
	void create_list(int update_widget);
	void change_job();
	void button_enable();
	void button_disable();

	ArrayList<BC_ListBoxItem*> list_items[BATCHRENDER_COLUMNS];
	const char *list_titles[BATCHRENDER_COLUMNS];
	int list_width[BATCHRENDER_COLUMNS], list_columns;

	MWindow *mwindow;
	BatchRenderThread *thread;
	BC_Title *output_path_title;
	BatchFormat *format_tools;
	BrowseButton *edl_path_browse;
	BatchRenderEDLPath *edl_path_text;
	BC_Title *edl_path_title;
//	BC_Title *status_title;
//	BC_Title *status_text;
//	BC_ProgressBar *progress_bar;
	BC_Title *list_title;
	BC_Title *batch_path;
	BatchRenderNew *new_batch;
	BatchRenderDelete *delete_batch;
	BatchRenderSaveList *savelist_batch;
	BatchRenderLoadList *loadlist_batch;
	BatchRenderWarning *warning;
	BatchRenderList *batch_list;
	BatchRenderStart *start_button;
	BatchRenderStop *stop_button;
	BatchRenderCancel *cancel_button;
	BatchRenderCurrentEDL *use_current_edl;
	BatchRenderUpdateEDL *update_selected_edl;
	BatchRenderUseFarm *use_renderfarm;
};


#endif
