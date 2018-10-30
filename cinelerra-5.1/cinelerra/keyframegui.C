
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

#include "bchash.h"
#include "bcsignals.h"
#include "edl.h"
#include "file.h"
#include "filesystem.h"
#include "keyframe.h"
#include "keyframes.h"
#include "keyframegui.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugin.h"
#include "preferences.h"
#include "presets.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"


KeyFrameThread::KeyFrameThread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	plugin = 0;
	keyframe = 0;
	keyframe_data = new ArrayList<BC_ListBoxItem*>[KEYFRAME_COLUMNS];
	plugin_title[0] = 0;
	is_factory = 0;
	preset_text[0] = 0;
	window_title[0] = 0;
	column_titles[0] = (char*)_("Parameter");
	column_titles[1] = (char*)_("Value");
	column_width[0] = 0;
	column_width[1] = 0;
	presets_data = new ArrayList<BC_ListBoxItem*>;
	presets_db = new PresetsDB;
}

KeyFrameThread::~KeyFrameThread()
{
	for( int i=0; i<KEYFRAME_COLUMNS; ++i )
		keyframe_data[i].remove_all_objects();
	delete [] keyframe_data;
	presets_data->remove_all_objects();
	delete presets_data;
	is_factories.remove_all();
	preset_titles.remove_all_objects();
}


#ifdef EDIT_KEYFRAME
void KeyFrameThread::update_values()
{
// Get the current selection before deleting the tables
	int selection = -1;
	for( int i=0; i<keyframe_data[0].size(); ++i ) {
		if( keyframe_data[0].get(i)->get_selected() ) {
			selection = i;
			break;
		}
	}

	for( int i=0; i<KEYFRAME_COLUMNS; ++i )
		keyframe_data[i].remove_all_objects();


// Must lock main window to read keyframe
	mwindow->gui->lock_window("KeyFrameThread::update_values");
	if( !plugin || !mwindow->edl->tracks->plugin_exists(plugin) ) {
		mwindow->gui->unlock_window();
		return;
	}

	KeyFrame *keyframe = 0;
	if( this->keyframe && plugin->keyframe_exists(this->keyframe) ) {
// If user edited a specific keyframe, use it.
		keyframe = this->keyframe;
	}
	else
	if( plugin->track ) {
// Use currently highlighted keyframe
		keyframe = plugin->get_prev_keyframe(
			plugin->track->to_units(
				mwindow->edl->local_session->get_selectionstart(1), 0),
			PLAY_FORWARD);
	}

	if( keyframe ) {
		BC_Hash hash;
		char *text = 0, *data = 0;
		keyframe->get_contents(&hash, &text, &data);
		
		for( int i=0; i<hash.size(); ++i ) {
			keyframe_data[0].append(new BC_ListBoxItem(hash.get_key(i)));
			keyframe_data[1].append(new BC_ListBoxItem(hash.get_value(i)));
		}
		if( text ) {
			keyframe_data[0].append(new BC_ListBoxItem((char*)"TEXT"));
			keyframe_data[1].append(new BC_ListBoxItem(text));
		}
		if( data ) {
			keyframe_data[0].append(new BC_ListBoxItem((char*)"DATA"));
			keyframe_data[1].append(new BC_ListBoxItem(data));
		}
		delete [] text;
		delete [] data;
	}

	column_width[0] = mwindow->session->keyframedialog_column1;
	column_width[1] = mwindow->session->keyframedialog_column2;
	if( selection >= 0 && selection < keyframe_data[0].size() ) {
		for( int i=0; i<KEYFRAME_COLUMNS; ++i )
			keyframe_data[i].get(selection)->set_selected(1);
	}
	mwindow->gui->unlock_window();
}
#endif


void KeyFrameThread::start_window(Plugin *plugin, KeyFrame *keyframe)
{

	if( !BC_DialogThread::is_running() ) {
		if( !mwindow->edl->tracks->plugin_exists(plugin) ) return;
		this->keyframe = keyframe;
		this->plugin = plugin;
		this->preset_text[0] = 0;
		plugin->calculate_title(plugin_title, 0);
		sprintf(window_title, _("%s: %s Keyframe"), _(PROGRAM_NAME), plugin_title);

// Load all the presets from disk
		char path[BCTEXTLEN];
		FileSystem fs;
// system wide presets
		sprintf(path, "%s/%s", File::get_cindat_path(), FACTORY_FILE);
		fs.complete_path(path);
		presets_db->load_from_file(path, 1, 1);
// user presets
		sprintf(path, "%s/%s", File::get_config_path(), PRESETS_FILE);
		fs.complete_path(path);
		presets_db->load_from_file(path, 0, 0);

		calculate_preset_list();

#ifdef EDIT_KEYFRAME
		update_values();
#endif
		mwindow->gui->unlock_window();
		BC_DialogThread::start();
		mwindow->gui->lock_window("KeyFrameThread::start_window");
	}
	else {
		BC_DialogThread::start();
	}
}

BC_Window* KeyFrameThread::new_gui()
{
	mwindow->gui->lock_window("KeyFrameThread::new_gui");
	
	int x = mwindow->gui->get_abs_cursor_x(0) - 
		mwindow->session->plugindialog_w / 2;
	int y = mwindow->gui->get_abs_cursor_y(0) - 
		mwindow->session->plugindialog_h / 2;

	KeyFrameWindow *window = new KeyFrameWindow(mwindow, 
		this, 
		x, 
		y,
		window_title);

	window->create_objects();
	
	
	mwindow->gui->unlock_window();

	return window;
}

void KeyFrameThread::handle_done_event(int result)
{
// Apply the preset
	if( !result ) {
		apply_preset(preset_text, is_factory);
	}
}

void KeyFrameThread::handle_close_event(int result)
{
	plugin = 0;
	keyframe = 0;
}

void KeyFrameThread::close_window()
{
	lock_window("KeyFrameThread::close_window");
	if( get_gui() ) {
		get_gui()->lock_window("KeyFrameThread::close_window");
		get_gui()->set_done(1);
		get_gui()->unlock_window();
	}
	unlock_window();
}



void KeyFrameThread::calculate_preset_list()
{
	presets_data->remove_all_objects();
	is_factories.remove_all();
	preset_titles.remove_all_objects();
	int total_presets = presets_db->get_total_presets(plugin_title, 0);

// sort the list
	presets_db->sort(plugin_title);
	
	for( int i=0; i<total_presets; ++i ) {
		char text[BCTEXTLEN];
		char *orig_title = presets_db->get_preset_title( plugin_title, i);
		if( !orig_title ) continue;
		int is_factory = presets_db->get_is_factory(plugin_title, i);
		sprintf(text, "%s%s", is_factory ? "*" : "", orig_title);
		presets_data->append(new BC_ListBoxItem(text));

		preset_titles.append(strdup(orig_title));
		is_factories.append(is_factory);
	}
}


void KeyFrameThread::update_gui(int update_value_text)
{
#ifdef EDIT_KEYFRAME
	if( BC_DialogThread::is_running() ) {
		mwindow->gui->lock_window("KeyFrameThread::update_gui");
		update_values();
		mwindow->gui->unlock_window();

		lock_window("KeyFrameThread::update_gui");
		KeyFrameWindow *window = (KeyFrameWindow*)get_gui();
		if( window ) {
			window->lock_window("KeyFrameThread::update_gui");
			window->keyframe_list->update(keyframe_data,
				column_titles,
				column_width,
				KEYFRAME_COLUMNS,
				window->keyframe_list->get_xposition(),
				window->keyframe_list->get_yposition(),
				window->keyframe_list->get_highlighted_item());
			if( update_value_text &&
				window->keyframe_list->get_selection_number(0, 0) >= 0 &&
				window->keyframe_list->get_selection_number(0, 0) < keyframe_data[1].size() ) {
				window->value_text->update(
					keyframe_data[1].get(window->keyframe_list->get_selection_number(0, 0))->get_text());
			}
			window->unlock_window();
		}
		unlock_window();
	}
#endif
}

void KeyFrameThread::save_preset(const char *title, int is_factory)
{
	get_gui()->unlock_window();
	mwindow->gui->lock_window("KeyFrameThread::save_preset");
	
// Test EDL for plugin existence
	if( !mwindow->edl->tracks->plugin_exists(plugin) ) {
		mwindow->gui->unlock_window();
		get_gui()->lock_window("KeyFrameThread::save_preset 2");
		return;
	}

// Get current plugin keyframe
	EDL *edl = mwindow->edl;
	Track *track = plugin->track;
	KeyFrame *keyframe = plugin->get_prev_keyframe(
			track->to_units(edl->local_session->get_selectionstart(1), 0), 
			PLAY_FORWARD);

// Send to database
	presets_db->save_preset(plugin_title, title, keyframe->get_data());

	mwindow->gui->unlock_window();
	get_gui()->lock_window("KeyFrameThread::save_preset 2");

// Update list
	calculate_preset_list();
	((KeyFrameWindow*)get_gui())->preset_list->update(presets_data,
		0, 0, 1);
}

void KeyFrameThread::delete_preset(const char *title, int is_factory)
{
	get_gui()->unlock_window();
	mwindow->gui->lock_window("KeyFrameThread::save_preset");
	
// Test EDL for plugin existence
	if( !mwindow->edl->tracks->plugin_exists(plugin) ) {
		mwindow->gui->unlock_window();
		get_gui()->lock_window("KeyFrameThread::delete_preset 1");
		return;
	}

	presets_db->delete_preset(plugin_title, title, is_factory);
	
	mwindow->gui->unlock_window();
	get_gui()->lock_window("KeyFrameThread::delete_preset 2");

// Update list
	calculate_preset_list();
	((KeyFrameWindow*)get_gui())->preset_list->update(presets_data,
		0, 0, 1);
}


void KeyFrameThread::apply_preset(const char *title, int is_factory)
{
	if( presets_db->preset_exists(plugin_title, title, is_factory) ) {
		get_gui()->unlock_window();
		mwindow->gui->lock_window("KeyFrameThread::apply_preset");

// Test EDL for plugin existence
		if( !mwindow->edl->tracks->plugin_exists(plugin) ) {
			mwindow->gui->unlock_window();
			get_gui()->lock_window("KeyFrameThread::apply_preset 1");
			return;
		}

		mwindow->undo->update_undo_before();

#ifdef USE_KEYFRAME_SPANNING
		KeyFrame keyframe;
		presets_db->load_preset(plugin_title, title, &keyframe, is_factory);
		plugin->keyframes->update_parameter(&keyframe);
#else
		KeyFrame *keyframe = plugin->get_keyframe();
		presets_db->load_preset(plugin_title, title, keyframe, is_factory);
#endif
		mwindow->save_backup();
		mwindow->undo->update_undo_after(_("apply preset"), LOAD_AUTOMATION); 

		mwindow->update_plugin_guis(0);
		mwindow->gui->draw_overlays(1);
		mwindow->sync_parameters(CHANGE_PARAMS);


		update_gui(1);
		mwindow->gui->unlock_window();
		get_gui()->lock_window("KeyFrameThread::apply_preset");
	}
}

#ifdef EDIT_KEYFRAME
void KeyFrameThread::apply_value()
{
	const char *text = 0, *data = 0;
	BC_Hash *hash = 0;
	KeyFrameWindow *window = (KeyFrameWindow*)get_gui();
	int selection = window->keyframe_list->get_selection_number(0, 0);
//printf("KeyFrameThread::apply_value %d %d\n", __LINE__, selection);
	if( selection < 0 ) return;
	
	if( selection == keyframe_data[0].size() - 2 )
		text = window->value_text->get_text();
	else if( selection == keyframe_data[0].size() - 1 )
		data = window->value_text->get_text();
	else {
		const char *key = keyframe_data[0].get(selection)->get_text();
		const char *value = window->value_text->get_text();
		hash = new BC_Hash();
		hash->update(key, value);
	}

	get_gui()->unlock_window();
	mwindow->gui->lock_window("KeyFrameThread::apply_value");
	if( plugin && mwindow->edl->tracks->plugin_exists(plugin) ) {
		mwindow->undo->update_undo_before();
		if( mwindow->session->keyframedialog_all ) {
// Search for all keyframes in selection but don't create a new one.
			Track *track = plugin->track;
			int64_t start = track->to_units(mwindow->edl->local_session->get_selectionstart(0), 0);
			int64_t end = track->to_units(mwindow->edl->local_session->get_selectionend(0), 0);
			int got_it = 0;
			KeyFrame *current = (KeyFrame*)plugin->keyframes->last;
			for( ; current; current=(KeyFrame*)PREVIOUS ) {
				got_it = 1;
				if( current && current->position < end ) {
				    current->update_parameter(hash, text, data);
// Stop at beginning of range
					if( current->position <= start ) break;
				}
			}

			if( !got_it ) {
				current = (KeyFrame*)plugin->keyframes->default_auto;
				current->update_parameter(hash, text, data);
			}
		}
		else {
// Create new keyframe if enabled
			KeyFrame *keyframe = plugin->get_keyframe();
			keyframe->update_parameter(hash, text, data);
		}
	}
	else {
printf("KeyFrameThread::apply_value %d: plugin doesn't exist\n", __LINE__);
	}

	mwindow->save_backup();
	mwindow->undo->update_undo_after(_("edit keyframe"), LOAD_AUTOMATION); 

	mwindow->update_plugin_guis(0);
	mwindow->gui->draw_overlays(1);
	mwindow->sync_parameters(CHANGE_PARAMS);
	mwindow->gui->unlock_window();

	update_gui(0);

	get_gui()->lock_window("KeyFrameThread::apply_value");
	delete hash;
}
#endif



KeyFrameWindow::KeyFrameWindow(MWindow *mwindow,
	KeyFrameThread *thread,
	int x,
	int y,
	char *title_string)
 : BC_Window(title_string, 
 	x,
	y,
	mwindow->session->keyframedialog_w, 
	mwindow->session->keyframedialog_h, 
	320, 
	240,
	1,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

void KeyFrameWindow::create_objects()
{
	Theme *theme = mwindow->theme;

	theme->get_keyframedialog_sizes(this);
	thread->column_width[0] = mwindow->session->keyframedialog_column1;
	thread->column_width[1] = mwindow->session->keyframedialog_column2;
	lock_window("KeyFrameWindow::create_objects");

#ifdef EDIT_KEYFRAME


	add_subwindow(title1 = new BC_Title(theme->keyframe_list_x,
		theme->keyframe_list_y - BC_Title::calculate_h(this, (char*)"Py", LARGEFONT) - 
			theme->widget_border, _("Keyframe parameters:"), LARGEFONT));
	add_subwindow(keyframe_list = new KeyFrameList(thread,
		this,
		theme->keyframe_list_x,
		theme->keyframe_list_y,
		theme->keyframe_list_w, 
		theme->keyframe_list_h));
// 	add_subwindow(title2 = new BC_Title(theme->keyframe_text_x,
// 		theme->keyframe_text_y - BC_Title::calculate_h(this, "P") - theme->widget_border,
// 		_("Global Text:")));
// 	add_subwindow(keyframe_text = new KeyFrameText(thread,
// 		this,
// 		theme->keyframe_text_x,
// 		theme->keyframe_text_y,
// 		theme->keyframe_text_w));
	add_subwindow(title3 = new BC_Title(theme->keyframe_value_x,
		theme->keyframe_value_y - BC_Title::calculate_h(this, (char*)"P") -
			theme->widget_border, _("Edit value:")));
	add_subwindow(value_text = new KeyFrameValue(thread, this,
		theme->keyframe_value_x, theme->keyframe_value_y, theme->keyframe_value_w));
	add_subwindow(all_toggle = new KeyFrameAll(thread, this, 
		theme->keyframe_all_x, theme->keyframe_all_y));

#endif
	add_subwindow(title4 = new BC_Title(theme->presets_list_x, theme->presets_list_y - 
			BC_Title::calculate_h(this, (char*)"Py", LARGEFONT) - 
			theme->widget_border, _("Presets:"), LARGEFONT));
	add_subwindow(preset_list = new KeyFramePresetsList(thread, this,
		theme->presets_list_x, theme->presets_list_y,
		theme->presets_list_w, theme->presets_list_h));
	add_subwindow(title5 = new BC_Title(theme->presets_text_x,
		theme->presets_text_y - BC_Title::calculate_h(this, (char*)"P") - theme->widget_border,
		_("Preset title:")));
	add_subwindow(preset_text = new KeyFramePresetsText(thread, this,
		theme->presets_text_x, theme->presets_text_y, theme->presets_text_w));
	add_subwindow(delete_preset = new KeyFramePresetsDelete(thread, this,
		theme->presets_delete_x, theme->presets_delete_y));
	add_subwindow(save_preset = new KeyFramePresetsSave(thread, this,
		theme->presets_save_x, theme->presets_save_y));
	add_subwindow(apply_preset = new KeyFramePresetsApply(thread, this,
		theme->presets_apply_x, theme->presets_apply_y));

	add_subwindow(new KeyFramePresetsOK(thread, this));
	add_subwindow(new BC_CancelButton(this));

	show_window();
	unlock_window();
}

// called when going in & out of a factory preset
void KeyFrameWindow::update_editing()
{
	if( thread->is_factory ) {
		delete_preset->disable();
		save_preset->disable();
	}
	else {
		delete_preset->enable();
		save_preset->enable();
	}
}



int KeyFrameWindow::resize_event(int w, int h)
{
	Theme *theme = mwindow->theme;
	mwindow->session->keyframedialog_w = w;
	mwindow->session->keyframedialog_h = h;
	theme->get_keyframedialog_sizes(this);

#ifdef EDIT_KEYFRAME

	title1->reposition_window(theme->keyframe_list_x,
		theme->keyframe_list_y - BC_Title::calculate_h(this, (char*)"P") - theme->widget_border);
//	title2->reposition_window(theme->keyframe_text_x,
//		theme->keyframe_text_y - BC_Title::calculate_h(this, (char*)"P") - theme->widget_border);
	title3->reposition_window(theme->keyframe_value_x,
		theme->keyframe_value_y - BC_Title::calculate_h(this, (char*)"P") - theme->widget_border);
	keyframe_list->reposition_window(theme->keyframe_list_x,
		theme->keyframe_list_y,
		theme->keyframe_list_w, 
		theme->keyframe_list_h);
// 	text->reposition_window(theme->keyframe_text_x,
// 		theme->keyframe_text_y,
// 		theme->keyframe_text_w);
	value_text->reposition_window(theme->keyframe_value_x,
		theme->keyframe_value_y,
		theme->keyframe_value_w);
	all_toggle->reposition_window(theme->keyframe_all_x,
		theme->keyframe_all_y);

#endif // EDIT_KEYFRAME




	title4->reposition_window(theme->presets_list_x,
		theme->presets_list_y - BC_Title::calculate_h(this, (char*)"P") - theme->widget_border);
	title5->reposition_window(theme->presets_text_x,
		theme->presets_text_y - BC_Title::calculate_h(this, (char*)"P") - theme->widget_border);
	preset_list->reposition_window(theme->presets_list_x,
		theme->presets_list_y,
		theme->presets_list_w, 
		theme->presets_list_h);
	preset_text->reposition_window(theme->presets_text_x,
		theme->presets_text_y,
		theme->presets_text_w);
	delete_preset->reposition_window(theme->presets_delete_x,
		theme->presets_delete_y);
	save_preset->reposition_window(theme->presets_save_x,
		theme->presets_save_y);
	apply_preset->reposition_window(theme->presets_apply_x,
		theme->presets_apply_y);

	return 0;
}




#ifdef EDIT_KEYFRAME


KeyFrameList::KeyFrameList(KeyFrameThread *thread,
	KeyFrameWindow *window, int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT, thread->keyframe_data,
	thread->column_titles, thread->column_width, KEYFRAME_COLUMNS)
{
	this->thread = thread;
	this->window = window;
	set_master_column(1, 0);
}

int KeyFrameList::selection_changed()
{
	window->value_text->update(
		thread->keyframe_data[1].get(get_selection_number(0, 0))->get_text());
	return 0;
}

int KeyFrameList::handle_event()
{
	window->set_done(0);
	return 0;
}

int KeyFrameList::column_resize_event()
{
	thread->mwindow->session->keyframedialog_column1 = get_column_width(0);
	thread->mwindow->session->keyframedialog_column2 = get_column_width(1);
	return 1;
}




// KeyFrameText::KeyFrameText(KeyFrameThread *thread,
// 	KeyFrameWindow *window,
// 	int x,
// 	int y,
// 	int w)
//  : BC_TextBox(x, 
// 	y, 
// 	w, 
// 	1, 
// 	"")
// {
// 	this->thread = thread;
// 	this->window = window;
// }
// 
// int KeyFrameText::handle_event()
// {
// 	return 0;
// }



KeyFrameValue::KeyFrameValue(KeyFrameThread *thread,
	KeyFrameWindow *window,
	int x,
	int y,
	int w)
 : BC_TextBox(x, 
	y, 
	w, 
	1, 
	(char*)"")
{
	this->thread = thread;
	this->window = window;
}

int KeyFrameValue::handle_event()
{
	thread->apply_value();
	return 0;
}





KeyFrameAll::KeyFrameAll(KeyFrameThread *thread,
	KeyFrameWindow *window,
	int x,
	int y)
 : BC_CheckBox(x, 
 	y, 
	thread->mwindow->session->keyframedialog_all, 
	_("Apply to all selected keyframes"))
{
	this->thread = thread;
	this->window = window;
}

int KeyFrameAll::handle_event()
{
	thread->mwindow->session->keyframedialog_all = get_value();
	return 1;
}

#endif // EDIT_KEYFRAME










KeyFramePresetsList::KeyFramePresetsList(KeyFrameThread *thread,
	KeyFrameWindow *window,
	int x,
	int y,
	int w, 
	int h)
 : BC_ListBox(x, 
		y, 
		w, 
		h,
		LISTBOX_TEXT,
		thread->presets_data)
{
	this->thread = thread;
	this->window = window;
}

int KeyFramePresetsList::selection_changed()
{
	int number = get_selection_number(0, 0);
	if( number >= 0 ) {
		strcpy(thread->preset_text, thread->preset_titles.get(number));
		thread->is_factory = thread->is_factories.get(number);
// show title without factory symbol in the textbox
		window->preset_text->update(
			thread->presets_data->get(number)->get_text());
		window->update_editing();
	}
	
	return 0;
}

int KeyFramePresetsList::handle_event()
{
	thread->apply_preset(thread->preset_text, thread->is_factory);
	window->set_done(0);
	return 0;
}


KeyFramePresetsText::KeyFramePresetsText(KeyFrameThread *thread,
	KeyFrameWindow *window, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, thread->preset_text)
{
	this->thread = thread;
	this->window = window;
}

// user entered a title
int KeyFramePresetsText::handle_event()
{
	strcpy(thread->preset_text, get_text());
// once changed, it's now not a factory preset
	thread->is_factory = 0;
	window->update_editing();
	return 0;
}


KeyFramePresetsDelete::KeyFramePresetsDelete(KeyFrameThread *thread,
	KeyFrameWindow *window,
	int x,
	int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->thread = thread;
	this->window = window;
}

int KeyFramePresetsDelete::handle_event()
{
	if( !thread->is_factory ) {
		thread->delete_preset(thread->preset_text, thread->is_factory);
	}
	return 1;
}


KeyFramePresetsSave::KeyFramePresetsSave(KeyFrameThread *thread,
	KeyFrameWindow *window,
	int x,
	int y)
: BC_GenericButton(x, y, C_("Save"))
{
	this->thread = thread;
	this->window = window;
}

int KeyFramePresetsSave::handle_event()
{
	if( !thread->is_factory ) {
		thread->save_preset(thread->preset_text, thread->is_factory);
	}
	return 1;
}








KeyFramePresetsApply::KeyFramePresetsApply(KeyFrameThread *thread,
	KeyFrameWindow *window,
	int x,
	int y)
 : BC_GenericButton(x, y, _("Apply"))
{
	this->thread = thread;
	this->window = window;
}

int KeyFramePresetsApply::handle_event()
{
	thread->apply_preset(thread->preset_text, thread->is_factory);
	return 1;
}


KeyFramePresetsOK::KeyFramePresetsOK(KeyFrameThread *thread,
	KeyFrameWindow *window)
 : BC_OKButton(window)
{
	this->thread = thread;
	this->window = window;
}

int KeyFramePresetsOK::keypress_event()
{
	if( get_keypress() == RETURN ) {
// Apply the preset
		if( thread->presets_db->preset_exists(thread->plugin_title, 
			thread->preset_text, thread->is_factory) ) {
			window->set_done(0);
			return 1;
		}
		else {
			if( !thread->is_factory ) {
				thread->save_preset(thread->preset_text, thread->is_factory);
				return 1;
			}
		}
	}
	return 0;
}

