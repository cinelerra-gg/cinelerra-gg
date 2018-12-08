
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

#include "condition.h"
#include "cstrdup.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "module.h"
#include "mutex.h"
#include "plugin.h"
#include "plugindialog.h"
#include "pluginserver.h"
#include "theme.h"
#include "track.h"
#include "tracks.h"
#include "transition.h"


PluginDialogThread::PluginDialogThread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	plugin = 0;
}

PluginDialogThread::~PluginDialogThread()
{
	close_window();
}

void PluginDialogThread::start_window(Track *track,
	Plugin *plugin, const char *title, int is_mainmenu, int data_type)
{
	if(!BC_DialogThread::is_running())
	{
// At this point, the caller should hold the main window mutex.
//		mwindow->gui->lock_window("PluginDialogThread::start_window");
		this->track = track;
		this->data_type = data_type;
		this->plugin = plugin;
		this->is_mainmenu = is_mainmenu;
		single_standalone = mwindow->edl->session->single_standalone;

		if(plugin)
		{
			plugin->calculate_title(plugin_title, 0);
			this->shared_location = plugin->shared_location;
			this->plugin_type = plugin->plugin_type;
		}
		else
		{
			this->plugin_title[0] = 0;
			this->shared_location.plugin = -1;
			this->shared_location.module = -1;
			this->plugin_type = PLUGIN_NONE;
		}

		strcpy(this->window_title, title);
		mwindow->gui->unlock_window();

		BC_DialogThread::start();
		mwindow->gui->lock_window("PluginDialogThread::start_window");
	}
}

BC_Window* PluginDialogThread::new_gui()
{
	mwindow->gui->lock_window("PluginDialogThread::new_gui");
	int x = mwindow->gui->get_abs_cursor_x(0) -
		mwindow->session->plugindialog_w / 2;
	int y = mwindow->gui->get_abs_cursor_y(0) -
		mwindow->session->plugindialog_h / 2;
	plugin_type = 0;
	PluginDialog *window = new PluginDialog(mwindow,
		this,
		window_title,
		x,
		y);
	window->create_objects();
	mwindow->gui->unlock_window();
	return window;
}

void PluginDialogThread::handle_done_event(int result)
{
	PluginDialog *window = (PluginDialog*)BC_DialogThread::get_gui();
	if(window->selected_available >= 0)
	{
		window->attach_new(window->selected_available);
	}
	else
	if(window->selected_shared >= 0)
	{
		window->attach_shared(window->selected_shared);
	}
	else
	if(window->selected_modules >= 0)
	{
		window->attach_module(window->selected_modules);
	}
	if( mwindow->edl )
		mwindow->edl->session->single_standalone = single_standalone;
}

void PluginDialogThread::handle_close_event(int result)
{
	if(!result)
	{
		if(plugin_type)
		{
			mwindow->gui->lock_window("PluginDialogThread::run 3");


			mwindow->undo->update_undo_before();
			if(is_mainmenu)
			{
				mwindow->insert_effect(plugin_title,
					&shared_location,
					data_type,
					plugin_type,
					single_standalone);
			}
			else
			{
				if(plugin)
				{
					if(mwindow->edl->tracks->plugin_exists(plugin))
					{
						plugin->change_plugin(plugin_title,
							&shared_location,
							plugin_type);
					}
				}
				else
				{
					if(mwindow->edl->tracks->track_exists(track))
					{
						mwindow->insert_effect(plugin_title,
										&shared_location,
										track,
										0,
										0,
										0,
										plugin_type);
					}
				}
			}

			mwindow->save_backup();
			mwindow->undo->update_undo_after(_("attach effect"), LOAD_EDITS | LOAD_PATCHES);
			mwindow->restart_brender();
			mwindow->update_plugin_states();
			mwindow->sync_parameters(CHANGE_EDL);
			mwindow->gui->update(1, NORMAL_DRAW, 0, 0, 1, 0, 0);
			mwindow->gui->unlock_window();
		}
	}
	plugin = 0;
}









PluginDialog::PluginDialog(MWindow *mwindow,
	PluginDialogThread *thread,
	const char *window_title,
	int x,
	int y)
 : BC_Window(window_title,
 	x,
	y,
	mwindow->session->plugindialog_w,
	mwindow->session->plugindialog_h,
	510,
	415,
	1,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
	single_standalone = 0;
}

PluginDialog::~PluginDialog()
{
	lock_window("PluginDialog::~PluginDialog");
	standalone_data.remove_all_objects();

	shared_data.remove_all_objects();

	module_data.remove_all_objects();

	plugin_locations.remove_all_objects();

	module_locations.remove_all_objects();

	delete standalone_list;
	delete shared_list;
	delete module_list;
	unlock_window();
}

void PluginDialog::create_objects()
{
//	int use_default = 1;
	mwindow->theme->get_plugindialog_sizes();
	lock_window("PluginDialog::create_objects");

// GET A LIST OF ALL THE PLUGINS AVAILABLE
	mwindow->search_plugindb(thread->data_type == TRACK_AUDIO,
		thread->data_type == TRACK_VIDEO,
		1, 0, 0, plugindb);

	mwindow->edl->get_shared_plugins(thread->track,
		&plugin_locations,
		thread->is_mainmenu,
		thread->data_type);
	mwindow->edl->get_shared_tracks(thread->track,
		&module_locations,
		thread->is_mainmenu,
		thread->data_type);

// Construct listbox items
	for(int i = 0; i < plugin_locations.total; )
	{
		Track *track = mwindow->edl->tracks->number(plugin_locations.values[i]->module);
		char *track_title = track->title;
		int number = plugin_locations.values[i]->plugin;
		double start = mwindow->edl->local_session->get_selectionstart(1);
		Plugin *plugin = track->get_current_plugin(start, number, PLAY_FORWARD, 1, 0);
		if( !plugin ) { plugin_locations.remove_object_number(i);  continue; }
		char string[BCTEXTLEN];
		const char *plugin_title = _(plugin->title);
		snprintf(string, sizeof(string), "%s: %s", track_title, plugin_title);
		shared_data.append(new BC_ListBoxItem(string));
		++i;
	}
	for(int i = 0; i < module_locations.total; i++)
	{
		Track *track = mwindow->edl->tracks->number(module_locations.values[i]->module);
		module_data.append(new BC_ListBoxItem(track->title));
	}





// Create widgets
	add_subwindow(standalone_title = new BC_Title(mwindow->theme->plugindialog_new_x,
		mwindow->theme->plugindialog_new_y - 20,
		_("Plugins:")));
	int x1 = mwindow->theme->plugindialog_new_x, y1 = mwindow->theme->plugindialog_new_y;
	int w1 = mwindow->theme->plugindialog_new_w, h1 = mwindow->theme->plugindialog_new_h;
	add_subwindow(search_text = new PluginDialogSearchText(this, x1, y1, w1));
	int dy = search_text->get_h() + 10;
	y1 += dy;  h1 -= dy;
	load_plugin_list(0);

	add_subwindow(standalone_list = new PluginDialogNew(this,
		&standalone_data, x1, y1, w1, h1));
//
// 	if(thread->plugin)
// 		add_subwindow(standalone_change = new PluginDialogChangeNew(mwindow,
// 			this,
// 			mwindow->theme->plugindialog_newattach_x,
// 			mwindow->theme->plugindialog_newattach_y));
// 	else
// 		add_subwindow(standalone_attach = new PluginDialogAttachNew(mwindow,
// 			this,
// 			mwindow->theme->plugindialog_newattach_x,
// 			mwindow->theme->plugindialog_newattach_y));
//

	add_subwindow(shared_title = new BC_Title(mwindow->theme->plugindialog_shared_x,
		mwindow->theme->plugindialog_shared_y - 20,
		_("Shared effects:")));
	add_subwindow(shared_list = new PluginDialogShared(this,
		&shared_data,
		mwindow->theme->plugindialog_shared_x,
		mwindow->theme->plugindialog_shared_y,
		mwindow->theme->plugindialog_shared_w,
		mwindow->theme->plugindialog_shared_h));
// 	if(thread->plugin)
//       add_subwindow(shared_change = new PluginDialogChangeShared(mwindow,
//          this,
//          mwindow->theme->plugindialog_sharedattach_x,
//          mwindow->theme->plugindialog_sharedattach_y));
//    else
// 		add_subwindow(shared_attach = new PluginDialogAttachShared(mwindow,
// 			this,
// 			mwindow->theme->plugindialog_sharedattach_x,
// 			mwindow->theme->plugindialog_sharedattach_y));
//

	add_subwindow(module_title = new BC_Title(mwindow->theme->plugindialog_module_x,
		mwindow->theme->plugindialog_module_y - 20,
		_("Shared tracks:")));
	add_subwindow(module_list = new PluginDialogModules(this,
		&module_data,
		mwindow->theme->plugindialog_module_x,
		mwindow->theme->plugindialog_module_y,
		mwindow->theme->plugindialog_module_w,
		mwindow->theme->plugindialog_module_h));
// 	if(thread->plugin)
//       add_subwindow(module_change = new PluginDialogChangeModule(mwindow,
//          this,
//          mwindow->theme->plugindialog_moduleattach_x,
//          mwindow->theme->plugindialog_moduleattach_y));
//    else
// 		add_subwindow(module_attach = new PluginDialogAttachModule(mwindow,
// 			this,
// 			mwindow->theme->plugindialog_moduleattach_x,
// 			mwindow->theme->plugindialog_moduleattach_y));
//


// Add option for the menu invocation
// 	add_subwindow(file_title = new BC_Title(
// 		mwindow->theme->menueffect_file_x,
// 		mwindow->theme->menueffect_file_y,
// 		_("One standalone effect is attached to the first track.\n"
// 		"Shared effects are attached to the remaining tracks.")));

	if(thread->is_mainmenu)
		add_subwindow(single_standalone = new PluginDialogSingle(this,
			mwindow->theme->plugindialog_new_x + BC_OKButton::calculate_w() + 10,
			mwindow->theme->plugindialog_new_y +
				mwindow->theme->plugindialog_new_h +
				get_text_height(MEDIUMFONT)));



	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

	selected_available = -1;
	selected_shared = -1;
	selected_modules = -1;

	show_window();
	flush();
	unlock_window();
}

int PluginDialog::resize_event(int w, int h)
{
	mwindow->session->plugindialog_w = w;
	mwindow->session->plugindialog_h = h;
	mwindow->theme->get_plugindialog_sizes();


	standalone_title->reposition_window(mwindow->theme->plugindialog_new_x,
		mwindow->theme->plugindialog_new_y - 20);
	int x1 = mwindow->theme->plugindialog_new_x, y1 = mwindow->theme->plugindialog_new_y;
	int w1 = mwindow->theme->plugindialog_new_w, h1 = mwindow->theme->plugindialog_new_h;
	search_text->reposition_window(x1, y1, w1);
	int dy = search_text->get_h() + 10;
	y1 += dy;  h1 -= dy;
	standalone_list->reposition_window(x1, y1, w1, h1);

// 	if(standalone_attach)
// 		standalone_attach->reposition_window(mwindow->theme->plugindialog_newattach_x,
// 			mwindow->theme->plugindialog_newattach_y);
// 	else
// 		standalone_change->reposition_window(mwindow->theme->plugindialog_newattach_x,
// 			mwindow->theme->plugindialog_newattach_y);

	shared_title->reposition_window(mwindow->theme->plugindialog_shared_x,
		mwindow->theme->plugindialog_shared_y - 20);
	shared_list->reposition_window(mwindow->theme->plugindialog_shared_x,
		mwindow->theme->plugindialog_shared_y,
		mwindow->theme->plugindialog_shared_w,
		mwindow->theme->plugindialog_shared_h);
// 	if(shared_attach)
// 		shared_attach->reposition_window(mwindow->theme->plugindialog_sharedattach_x,
// 			mwindow->theme->plugindialog_sharedattach_y);
// 	else
// 		shared_change->reposition_window(mwindow->theme->plugindialog_sharedattach_x,
// 			mwindow->theme->plugindialog_sharedattach_y);
//




	module_title->reposition_window(mwindow->theme->plugindialog_module_x,
		mwindow->theme->plugindialog_module_y - 20);
	module_list->reposition_window(mwindow->theme->plugindialog_module_x,
		mwindow->theme->plugindialog_module_y,
		mwindow->theme->plugindialog_module_w,
		mwindow->theme->plugindialog_module_h);
// 	if(module_attach)
// 		module_attach->reposition_window(mwindow->theme->plugindialog_moduleattach_x,
// 			mwindow->theme->plugindialog_moduleattach_y);
// 	else
// 		module_change->reposition_window(mwindow->theme->plugindialog_moduleattach_x,
// 			mwindow->theme->plugindialog_moduleattach_y);


	if(single_standalone)
		single_standalone->reposition_window(
			mwindow->theme->plugindialog_new_x + BC_OKButton::calculate_w() + 10,
			mwindow->theme->plugindialog_new_y + mwindow->theme->plugindialog_new_h +
				get_text_height(MEDIUMFONT));

	flush();
	return 0;
}

int PluginDialog::attach_new(int number)
{
	if(number >= 0 && number < plugindb.size())
	{
		strcpy(thread->plugin_title, plugindb.values[number]->title);
		thread->plugin_type = PLUGIN_STANDALONE;         // type is plugin
	}
	return 0;
}

int PluginDialog::attach_shared(int number)
{
	if(number >= 0 && number < plugin_locations.size())
	{
		thread->plugin_type = PLUGIN_SHAREDPLUGIN;         // type is shared plugin
		thread->shared_location = *(plugin_locations.values[number]); // copy location
	}
	return 0;
}

int PluginDialog::attach_module(int number)
{
	if(number >= 0 && number < module_locations.size())
	{
//		title->update(module_data.values[number]->get_text());
		thread->plugin_type = PLUGIN_SHAREDMODULE;         // type is module
		thread->shared_location = *(module_locations.values[number]); // copy location
	}
	return 0;
}

void PluginDialog::save_settings()
{
}







//
// PluginDialogTextBox::PluginDialogTextBox(PluginDialog *dialog, char *text, int x, int y)
//  : BC_TextBox(x, y, 200, 1, text)
// {
// 	this->dialog = dialog;
// }
// PluginDialogTextBox::~PluginDialogTextBox()
// { }
// int PluginDialogTextBox::handle_event()
// { }
//
// PluginDialogDetach::PluginDialogDetach(MWindow *mwindow, PluginDialog *dialog, int x, int y)
//  : BC_GenericButton(x, y, _("Detach"))
// {
// 	this->dialog = dialog;
// }
// PluginDialogDetach::~PluginDialogDetach()
// { }
// int PluginDialogDetach::handle_event()
// {
// //	dialog->title->update(_("None"));
// 	dialog->thread->plugin_type = 0;         // type is none
// 	dialog->thread->plugin_title[0] = 0;
// 	return 1;
// }
//













PluginDialogNew::PluginDialogNew(PluginDialog *dialog,
	ArrayList<BC_ListBoxItem*> *standalone_data,
	int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT, standalone_data)
{
	this->dialog = dialog;
}

PluginDialogNew::~PluginDialogNew() { }
int PluginDialogNew::handle_event()
{
// 	dialog->attach_new(get_selection_number(0, 0));
// 	deactivate();

	set_done(0);
	return 1;
}
int PluginDialogNew::selection_changed()
{
	int no = get_selection_number(0, 0);
	dialog->selected_available = no >= 0 && no < dialog->standalone_data.size() ?
		((PluginDialogListItem *)dialog->standalone_data[no])->item_no : -1;
	dialog->shared_list->set_all_selected(&dialog->shared_data, 0);
	dialog->shared_list->draw_items(1);
	dialog->selected_shared = -1;
	dialog->module_list->set_all_selected(&dialog->module_data, 0);
	dialog->module_list->draw_items(1);
	dialog->selected_modules = -1;
	return 1;
}

// PluginDialogAttachNew::PluginDialogAttachNew(MWindow *mwindow, PluginDialog *dialog, int x, int y)
//  : BC_GenericButton(x, y, _("Attach"))
// {
//  	this->dialog = dialog;
// }
// PluginDialogAttachNew::~PluginDialogAttachNew()
// {
// }
// int PluginDialogAttachNew::handle_event()
// {
// 	dialog->attach_new(dialog->selected_available);
// 	set_done(0);
// 	return 1;
// }
//
// PluginDialogChangeNew::PluginDialogChangeNew(MWindow *mwindow, PluginDialog *dialog, int x, int y)
//  : BC_GenericButton(x, y, _("Change"))
// {
//    this->dialog = dialog;
// }
// PluginDialogChangeNew::~PluginDialogChangeNew()
// {
// }
// int PluginDialogChangeNew::handle_event()
// {
//    dialog->attach_new(dialog->selected_available);
//    set_done(0);
//    return 1;
// }










PluginDialogShared::PluginDialogShared(PluginDialog *dialog,
	ArrayList<BC_ListBoxItem*> *shared_data,
	int x,
	int y,
	int w,
	int h)
 : BC_ListBox(x,
 	y,
	w,
	h,
	LISTBOX_TEXT,
	shared_data)
{
	this->dialog = dialog;
}
PluginDialogShared::~PluginDialogShared() { }
int PluginDialogShared::handle_event()
{
//	dialog->attach_shared(get_selection_number(0, 0));
//	deactivate();
	set_done(0);
	return 1;
}
int PluginDialogShared::selection_changed()
{
	dialog->selected_shared = get_selection_number(0, 0);


	dialog->standalone_list->set_all_selected(&dialog->standalone_data, 0);
	dialog->standalone_list->draw_items(1);
	dialog->module_list->set_all_selected(&dialog->module_data, 0);
	dialog->module_list->draw_items(1);
	dialog->selected_available = -1;
	dialog->selected_modules = -1;
	return 1;
}

// PluginDialogAttachShared::PluginDialogAttachShared(MWindow *mwindow,
// 	PluginDialog *dialog,
// 	int x,
// 	int y)
//  : BC_GenericButton(x, y, _("Attach"))
// {
// 	this->dialog = dialog;
// }
// PluginDialogAttachShared::~PluginDialogAttachShared() { }
// int PluginDialogAttachShared::handle_event()
// {
// 	dialog->attach_shared(dialog->selected_shared);
// 	set_done(0);
// 	return 1;
// }
//
// PluginDialogChangeShared::PluginDialogChangeShared(MWindow *mwindow,
//    PluginDialog *dialog,
//    int x,
//    int y)
//  : BC_GenericButton(x, y, _("Change"))
// {
//    this->dialog = dialog;
// }
// PluginDialogChangeShared::~PluginDialogChangeShared() { }
// int PluginDialogChangeShared::handle_event()
// {
//    dialog->attach_shared(dialog->selected_shared);
//    set_done(0);
//    return 1;
// }
//












PluginDialogModules::PluginDialogModules(PluginDialog *dialog,
	ArrayList<BC_ListBoxItem*> *module_data,
	int x,
	int y,
	int w,
	int h)
 : BC_ListBox(x,
 	y,
	w,
	h,
	LISTBOX_TEXT,
	module_data)
{
	this->dialog = dialog;
}
PluginDialogModules::~PluginDialogModules() { }
int PluginDialogModules::handle_event()
{
//	dialog->attach_module(get_selection_number(0, 0));
//	deactivate();

	set_done(0);
	return 1;
}
int PluginDialogModules::selection_changed()
{
	dialog->selected_modules = get_selection_number(0, 0);


	dialog->standalone_list->set_all_selected(&dialog->standalone_data, 0);
	dialog->standalone_list->draw_items(1);
	dialog->shared_list->set_all_selected(&dialog->shared_data, 0);
	dialog->shared_list->draw_items(1);
	dialog->selected_available = -1;
	dialog->selected_shared = -1;
	return 1;
}

void PluginDialog::load_plugin_list(int redraw)
{
	standalone_data.remove_all_objects();
	const char *text = search_text->get_text();

	for( int i=0; i<plugindb.total; ++i ) {
		const char *title = _(plugindb.values[i]->title);
		if( text && text[0] && !bstrcasestr(title, text) ) continue;
		standalone_data.append(new PluginDialogListItem(title, i));
	}

	if( redraw )
		standalone_list->draw_items(1);
}

PluginDialogSearchText::PluginDialogSearchText(PluginDialog *dialog, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, "")
{
	this->dialog = dialog;
}

int PluginDialogSearchText::handle_event()
{
	dialog->load_plugin_list(1);
	return 1;
}

PluginDialogSingle::PluginDialogSingle(PluginDialog *dialog, int x, int y)
 : BC_CheckBox(x,
 	y,
	dialog->thread->single_standalone,
	_("Attach single standalone and share others"))
{
	this->dialog = dialog;
}

int PluginDialogSingle::handle_event()
{
	dialog->thread->single_standalone = get_value();
	return 1;
}


// PluginDialogAttachModule::PluginDialogAttachModule(MWindow *mwindow,
// 	PluginDialog *dialog,
// 	int x,
// 	int y)
//  : BC_GenericButton(x, y, _("Attach"))
// {
// 	this->dialog = dialog;
// }
// PluginDialogAttachModule::~PluginDialogAttachModule() { }
// int PluginDialogAttachModule::handle_event()
// {
// 	dialog->attach_module(dialog->selected_modules);
// 	set_done(0);
// 	return 1;
// }
//
// PluginDialogChangeModule::PluginDialogChangeModule(MWindow *mwindow,
//    PluginDialog *dialog,
//    int x,
//    int y)
//  : BC_GenericButton(x, y, _("Change"))
// {
//    this->dialog = dialog;
// }
// PluginDialogChangeModule::~PluginDialogChangeModule() { }
// int PluginDialogChangeModule::handle_event()
// {
//    dialog->attach_module(dialog->selected_modules);
//    set_done(0);
//    return 1;
// }
//














