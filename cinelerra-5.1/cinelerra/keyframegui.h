
/*
 * CINELERRA
 * Copyright (C) 2017 Adam Williams <broadcast at earthling dot net>
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

#ifndef KEYFRAMEGUI_H
#define KEYFRAMEGUI_H


#include "bcdialog.h"
#include "guicast.h"
#include "presets.inc"

class KeyFrameWindow;




#define KEYFRAME_COLUMNS 2
// Enable editing of detailed keyframe parameters.
#define EDIT_KEYFRAME

class KeyFrameThread : public BC_DialogThread
{
public:
	KeyFrameThread(MWindow *mwindow);
	~KeyFrameThread();


	BC_Window* new_gui();
	void start_window(Plugin *plugin, KeyFrame *keyframe);
	void handle_done_event(int result);
	void handle_close_event(int result);
	void update_values();
	void save_value(char *value);
	void save_preset(const char *title, int is_factory);
	void delete_preset(const char *title, int is_factory);
	void apply_preset(const char *title, int is_factory);
	void apply_value();
	void calculate_preset_list();
	void update_gui(int update_value_text = 1);
	void close_window();

	ArrayList<BC_ListBoxItem*> *keyframe_data;
	Plugin *plugin;
	KeyFrame *keyframe;
	MWindow *mwindow;
	char window_title[BCTEXTLEN];
	char plugin_title[BCTEXTLEN];

// the currently selected preset is a factory preset
	int is_factory;
// title of the currently selected preset from the DB
	char preset_text[BCTEXTLEN];
	
	const char *column_titles[KEYFRAME_COLUMNS];
	int column_width[KEYFRAME_COLUMNS];
// list of preset text to display
	ArrayList<BC_ListBoxItem*> *presets_data;
// flag for each preset shown
	ArrayList<int> is_factories;
// title of each preset shown
	ArrayList<char*> preset_titles;
	PresetsDB *presets_db;
};

#ifdef EDIT_KEYFRAME


class KeyFrameList : public BC_ListBox
{
public:
	KeyFrameList(KeyFrameThread *thread,
		KeyFrameWindow *window,
		int x,
		int y,
		int w, 
		int h);
	int selection_changed();
	int handle_event();
	int column_resize_event();
	KeyFrameThread *thread;
	KeyFrameWindow *window;
};
#endif

class KeyFramePresetsList : public BC_ListBox
{
public:
	KeyFramePresetsList(KeyFrameThread *thread,
		KeyFrameWindow *window,
		int x,
		int y,
		int w, 
		int h);
	int selection_changed();
	int handle_event();
	KeyFrameThread *thread;
	KeyFrameWindow *window;
};

class KeyFramePresetsText : public BC_TextBox
{
public:
	KeyFramePresetsText(KeyFrameThread *thread,
		KeyFrameWindow *window,
		int x,
		int y,
		int w);
	int handle_event();
	KeyFrameThread *thread;
	KeyFrameWindow *window;
};


class KeyFramePresetsDelete : public BC_GenericButton
{
public:
	KeyFramePresetsDelete(KeyFrameThread *thread,
		KeyFrameWindow *window,
		int x,
		int y);
	int handle_event();
	KeyFrameThread *thread;
	KeyFrameWindow *window;
};

class KeyFramePresetsSave : public BC_GenericButton
{
public:
	KeyFramePresetsSave(KeyFrameThread *thread,
		KeyFrameWindow *window,
		int x,
		int y);
	int handle_event();
	KeyFrameThread *thread;
	KeyFrameWindow *window;
};

class KeyFramePresetsApply : public BC_GenericButton
{
public:
	KeyFramePresetsApply(KeyFrameThread *thread,
		KeyFrameWindow *window,
		int x,
		int y);
	int handle_event();
	KeyFrameThread *thread;
	KeyFrameWindow *window;
};

/*
 * class KeyFrameText : public BC_TextBox
 * {
 * public:
 * 	KeyFrameText(KeyFrameThread *thread,
 * 		KeyFrameWindow *window,
 * 		int x,
 * 		int y,
 * 		int w);
 * 	int handle_event();
 * 	KeyFrameThread *thread;
 * 	KeyFrameWindow *window;
 * };
 */

#ifdef EDIT_KEYFRAME


class KeyFrameValue : public BC_TextBox
{
public:
	KeyFrameValue(KeyFrameThread *thread,
		KeyFrameWindow *window,
		int x,
		int y,
		int w);
	int handle_event();
	KeyFrameThread *thread;
	KeyFrameWindow *window;
};

class KeyFrameAll : public BC_CheckBox
{
public:
	KeyFrameAll(KeyFrameThread *thread,
		KeyFrameWindow *window,
		int x,
		int y);
	int handle_event();
	KeyFrameThread *thread;
	KeyFrameWindow *window;
};

#endif


class KeyFramePresetsOK : public BC_OKButton
{
public:
	KeyFramePresetsOK(KeyFrameThread *thread,
		KeyFrameWindow *window);
	int keypress_event();
	KeyFrameThread *thread;
	KeyFrameWindow *window;
};



class KeyFrameWindow : public BC_Window
{
public:
	KeyFrameWindow(MWindow *mwindow,
		KeyFrameThread *thread,
		int x,
		int y,
		char *title);
	void create_objects();
	int resize_event(int w, int h);
	void update_editing();

#ifdef EDIT_KEYFRAME

// List of parameters, values, & whether the parameter is defined by the current keyframe.
	KeyFrameList *keyframe_list;
// The text area of the plugin
//	KeyFrameText *keyframe_text;
// Value text of the current parameter
	KeyFrameValue *value_text;
	KeyFrameAll *all_toggle;
#endif
	
	KeyFramePresetsList *preset_list;
	KeyFramePresetsText *preset_text;
	KeyFramePresetsDelete *delete_preset;
	KeyFramePresetsSave *save_preset;
	KeyFramePresetsApply *apply_preset;

#ifdef EDIT_KEYFRAME

	BC_Title *title1;
//	BC_Title *title2;
	BC_Title *title3;
#endif

	BC_Title *title4;
	BC_Title *title5;

	MWindow *mwindow;
	KeyFrameThread *thread;
};




#endif





