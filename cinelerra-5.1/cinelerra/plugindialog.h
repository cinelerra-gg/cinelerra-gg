
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

#ifndef PLUGINDIALOG_H
#define PLUGINDIALOG_H

class PluginDialogTextBox;
class PluginDialogDetach;
class PluginDialogNew;
class PluginDialogShared;
class PluginDialogSearchText;
class PluginDialogModules;
class PluginDialogAttachNew;
class PluginDialogChangeNew;
class PluginDialogIn;
class PluginDialogOut;
class PluginDialogThru;
class PluginDialogSingle;
class PluginDialog;

#include "bcdialog.h"
#include "condition.inc"
#include "guicast.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "plugin.inc"
#include "pluginserver.inc"
#include "sharedlocation.h"
#include "thread.h"
#include "track.inc"
#include "transition.inc"

class PluginDialogThread : public BC_DialogThread
{
public:
	PluginDialogThread(MWindow *mwindow);
	~PluginDialogThread();

// Set up parameters for a transition menu.
	void start_window(Track *track,
		Plugin *plugin,
		const char *title,
		int is_mainmenu,
		int data_type);
	BC_Window* new_gui();
	void handle_done_event(int result);
	void handle_close_event(int result);

	MWindow *mwindow;
	Track *track;
	int data_type;
	Transition *transition;
// Plugin being modified if there is one
	Plugin *plugin;
	char window_title[BCTEXTLEN];
// If attaching from main menu
	int is_mainmenu;


// type of attached plugin
	int plugin_type;    // 0: none  1: plugin   2: shared plugin   3: module

// location of attached plugin if shared
	SharedLocation shared_location;

// Title of attached plugin if new
	char plugin_title[BCTEXTLEN];
// For the main menu invocation,
// attach 1 standalone on the first track and share it with other tracks
	int single_standalone;
};

class PluginDialogListItem : public BC_ListBoxItem
{
public:
	PluginDialogListItem(const char *item, int n)
	 : BC_ListBoxItem(item), item_no(n) {}
	int item_no;
};

class PluginDialog : public BC_Window
{
public:
	PluginDialog(MWindow *mwindow,
		PluginDialogThread *thread,
		const char *title,
		int x,
		int y);
	~PluginDialog();

	void create_objects();

	int attach_new(int number);
	int attach_shared(int number);
	int attach_module(int number);
	void save_settings();
	int resize_event(int w, int h);
	void load_plugin_list(int redraw);

	BC_Title *standalone_title;
	PluginDialogNew *standalone_list;
	BC_Title *shared_title;
	PluginDialogShared *shared_list;
	BC_Title *module_title;
	PluginDialogModules *module_list;
	PluginDialogSingle *single_standalone;
	PluginDialogSearchText *search_text;

	PluginDialogThru *thru;

	PluginDialogThread *thread;

	ArrayList<BC_ListBoxItem*> standalone_data;
	ArrayList<BC_ListBoxItem*> shared_data;
	ArrayList<BC_ListBoxItem*> module_data;
	ArrayList<SharedLocation*> plugin_locations; // locations of all shared plugins
	ArrayList<SharedLocation*> module_locations; // locations of all shared modules
	ArrayList<PluginServer*> plugindb;           // locations of all simple plugins, no need for memory freeing!

	int selected_available;
	int selected_shared;
	int selected_modules;

	MWindow *mwindow;
};


/*
 * class PluginDialogTextBox : public BC_TextBox
 * {
 * public:
 * 	PluginDialogTextBox(PluginDialog *dialog, char *text, int x, int y);
 * 	~PluginDialogTextBox();
 *
 * 	int handle_event();
 * 	PluginDialog *dialog;
 * };
 */

/*
 * class PluginDialogDetach : public BC_GenericButton
 * {
 * public:
 * 	PluginDialogDetach(MWindow *mwindow, PluginDialog *dialog, int x, int y);
 * 	~PluginDialogDetach();
 *
 * 	int handle_event();
 * 	PluginDialog *dialog;
 * };
 */

/*
 * class PluginDialogAttachNew : public BC_GenericButton
 * {
 * public:
 * 	PluginDialogAttachNew(MWindow *mwindow, PluginDialog *dialog, int x, int y);
 * 	~PluginDialogAttachNew();
 *
 * 	int handle_event();
 * 	PluginDialog *dialog;
 * };
 *
 * class PluginDialogChangeNew : public BC_GenericButton
 * {
 * public:
 *    PluginDialogChangeNew(MWindow *mwindow, PluginDialog *dialog, int x, int y);
 *    ~PluginDialogChangeNew();
 *
 *    int handle_event();
 *    PluginDialog *dialog;
 * };
 */


class PluginDialogNew : public BC_ListBox
{
public:
	PluginDialogNew(PluginDialog *dialog,
		ArrayList<BC_ListBoxItem*> *standalone_data,
		int x,
		int y,
		int w,
		int h);
	~PluginDialogNew();

	int handle_event();
	int selection_changed();
	PluginDialog *dialog;
};

class PluginDialogShared : public BC_ListBox
{
public:
	PluginDialogShared(PluginDialog *dialog,
		ArrayList<BC_ListBoxItem*> *shared_data,
		int x,
		int y,
		int w,
		int h);
	~PluginDialogShared();

	int handle_event();
	int selection_changed();
	PluginDialog *dialog;
};

class PluginDialogModules : public BC_ListBox
{
public:
	PluginDialogModules(PluginDialog *dialog,
		ArrayList<BC_ListBoxItem*> *module_data,
		int x,
		int y,
		int w,
		int h);
	~PluginDialogModules();

	int handle_event();
	int selection_changed();
	PluginDialog *dialog;
};

class PluginDialogSingle : public BC_CheckBox
{
public:
	PluginDialogSingle(PluginDialog *dialog, int x, int y);
	int handle_event();
	PluginDialog *dialog;
};

class PluginDialogSearchText : public BC_TextBox
{
public:
	PluginDialogSearchText(PluginDialog *dialog, int x, int y, int w);
	int handle_event();

	PluginDialog *dialog;
};

/*
 * class PluginDialogAttachShared : public BC_GenericButton
 * {
 * public:
 * 	PluginDialogAttachShared(MWindow *mwindow, PluginDialog *dialog, int x, int y);
 * 	~PluginDialogAttachShared();
 *
 * 	int handle_event();
 * 	PluginDialog *dialog;
 * };
 *
 * class PluginDialogChangeShared : public BC_GenericButton
 * {
 * public:
 *    PluginDialogChangeShared(MWindow *mwindow, PluginDialog *dialog, int x, int y);
 *    ~PluginDialogChangeShared();
 *
 *    int handle_event();
 *    PluginDialog *dialog;
 * };
 *
 *
 * class PluginDialogAttachModule : public BC_GenericButton
 * {
 * public:
 * 	PluginDialogAttachModule(MWindow *mwindow, PluginDialog *dialog, int x, int y);
 * 	~PluginDialogAttachModule();
 *
 * 	int handle_event();
 * 	PluginDialog *dialog;
 * };
 *
 * class PluginDialogChangeModule : public BC_GenericButton
 * {
 * public:
 *    PluginDialogChangeModule(MWindow *mwindow, PluginDialog *dialog, int x, int y);
 *    ~PluginDialogChangeModule();
 *
 *    int handle_event();
 *    PluginDialog *dialog;
 * }
 */



#endif
