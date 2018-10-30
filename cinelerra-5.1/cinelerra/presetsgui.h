
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

#ifndef PRESETSGUI_H
#define PRESETSGUI_H


// Presets for effects & transitions
#if 0

#include "bcdialog.h"
#include "guicast.h"
#include "mwindow.inc"
#include "plugin.inc"
#include "presets.inc"

class PresetsWindow;





class PresetsThread : public BC_DialogThread
{
public:
	PresetsThread(MWindow *mwindow);
	~PresetsThread();

	void calculate_list();
	void start_window(Plugin *plugin);
	BC_Window* new_gui();
	void handle_done_event(int result);
	void handle_close_event(int result);
	void save_preset(char *title);
	void delete_preset(char *title);
	void apply_preset(char *title);

	char window_title[BCTEXTLEN];
	char plugin_title[BCTEXTLEN];
	MWindow *mwindow;
	Plugin *plugin;
	ArrayList<BC_ListBoxItem*> *data;
	PresetsDB *presets_db;
};

class PresetsList : public BC_ListBox
{
public:
	PresetsList(PresetsThread *thread,
		PresetsWindow *window,
		int x,
		int y,
		int w, 
		int h);
	int selection_changed();
	int handle_event();
	PresetsThread *thread;
	PresetsWindow *window;
};

class PresetsText : public BC_TextBox
{
public:
	PresetsText(PresetsThread *thread,
		PresetsWindow *window,
		int x,
		int y,
		int w);
	int handle_event();
	PresetsThread *thread;
	PresetsWindow *window;
};


class PresetsDelete : public BC_GenericButton
{
public:
	PresetsDelete(PresetsThread *thread,
		PresetsWindow *window,
		int x,
		int y);
	int handle_event();
	PresetsThread *thread;
	PresetsWindow *window;
};

class PresetsSave : public BC_GenericButton
{
public:
	PresetsSave(PresetsThread *thread,
		PresetsWindow *window,
		int x,
		int y);
	int handle_event();
	PresetsThread *thread;
	PresetsWindow *window;
};

class PresetsApply : public BC_GenericButton
{
public:
	PresetsApply(PresetsThread *thread,
		PresetsWindow *window,
		int x,
		int y);
	int handle_event();
	PresetsThread *thread;
	PresetsWindow *window;
};

class PresetsOK : public BC_OKButton
{
public:
	PresetsOK(PresetsThread *thread,
		PresetsWindow *window);
	int keypress_event();
	PresetsThread *thread;
	PresetsWindow *window;
};


class PresetsWindow : public BC_Window
{
public:
	PresetsWindow(MWindow *mwindow,
		PresetsThread *thread,
		int x,
		int y,
		char *title_string);

	void create_objects();
	int resize_event(int w, int h);

	PresetsList *list;
	PresetsText *title_text;
	PresetsDelete *delete_button;
	PresetsSave *save_button;
	PresetsApply *apply_button;
	BC_Title *title1;
	BC_Title *title2;
	MWindow *mwindow;
	PresetsThread *thread;
};



#endif

#endif



