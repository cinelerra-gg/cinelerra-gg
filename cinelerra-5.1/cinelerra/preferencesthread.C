
/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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

#include "aboutprefs.h"
#include "appearanceprefs.h"
#include "asset.h"
#include "audiodevice.inc"
#include "bcsignals.h"
#include "bctrace.h"
#include "cache.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "fonts.h"
#include "interfaceprefs.h"
#include "keys.h"
#include "language.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "mainclock.h"
#include "mainerror.h"
#include "mbuttons.h"
#include "meterpanel.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "performanceprefs.h"
#include "playbackengine.h"
#include "playbackprefs.h"
#include "preferences.h"
#include "recordprefs.h"
#include "shbtnprefs.h"
#include "theme.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "vwindow.h"
#include "vwindowgui.h"

#include <string.h>



#define WIDTH 860
#define HEIGHT 700


PreferencesMenuitem::PreferencesMenuitem(MWindow *mwindow)
 : BC_MenuItem(_("Preferences..."), _("Shift-P"), 'P')
{
	this->mwindow = mwindow;

	set_shift(1);
	thread = new PreferencesThread(mwindow);
}

PreferencesMenuitem::~PreferencesMenuitem()
{
	delete thread;
}


int PreferencesMenuitem::handle_event()
{
	mwindow->gui->unlock_window();
	thread->start();
	mwindow->gui->lock_window("PreferencesMenuitem::handle_event");
	return 1;
}




PreferencesThread::PreferencesThread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	window = 0;
	thread_running = 0;
}

PreferencesThread::~PreferencesThread()
{
	close_window();
}

BC_Window* PreferencesThread::new_gui()
{

	preferences = new Preferences;
	edl = new EDL;
	edl->create_objects();
	current_dialog = mwindow->defaults->get("DEFAULTPREF", 0);
	preferences->copy_from(mwindow->preferences);
	edl->copy_session(mwindow->edl);
	redraw_indexes = 0;
	redraw_meters = 0;
	redraw_times = 0;
	redraw_overlays = 0;
	close_assets = 0;
	reload_plugins = 0;
	//int need_new_indexes = 0;
	rerender = 0;

 	mwindow->gui->lock_window("NewThread::new_gui");
	int scr_x = mwindow->gui->get_screen_x(0, -1);
	int scr_w = mwindow->gui->get_screen_w(0, -1);
	int scr_h = mwindow->gui->get_screen_h(0, -1);

	int w = WIDTH, h = HEIGHT;
	int min_w = mwindow->theme->preferencescategory_x;
	for(int i = 0; i < CATEGORIES; i++) {
		min_w += PreferencesButton::calculate_w(mwindow->gui, category_to_text(i)) -
			mwindow->theme->preferences_category_overlap;
	}
	if( w < min_w ) w = min_w;
	int x = scr_x + scr_w / 2 - w / 2;
	int y = scr_h / 2 - h / 2;

	window = new PreferencesWindow(mwindow, this, x, y, w, h);
	window->create_objects();
	mwindow->gui->unlock_window();

	thread_running = 1;
	return window;
}

void PreferencesThread::handle_close_event(int result)
{
	thread_running = 0;
	if(!result)
	{
		apply_settings();
		mwindow->save_defaults();
	}

	window = 0;
	delete preferences;
	edl->Garbage::remove_user();
	preferences = 0;
	edl = 0;

	mwindow->defaults->update("DEFAULTPREF", current_dialog);
	if( mwindow->restart() )
		mwindow->gui->set_done(0);
}



int PreferencesThread::update_framerate()
{
	if(thread_running)
	{
		lock_gui("PreferencesThread::update_framerate");
		PreferencesWindow *window = (PreferencesWindow*)get_gui();
		if(window) window->update_framerate();
		unlock_gui();
	}
	return 0;
}


void PreferencesThread::update_rates()
{
	if(thread_running)
	{
		lock_gui("PreferencesThread::update_framerate");
		PreferencesWindow *window = (PreferencesWindow*)get_gui();
		if(window) window->update_rates();
		unlock_gui();
	}
}

int PreferencesThread::apply_settings()
{
// Compare sessions

	PlaybackConfig *this_playback_config = edl->session->playback_config;
	AudioOutConfig *this_aconfig = this_playback_config->aconfig;
	VideoOutConfig *this_vconfig = this_playback_config->vconfig;
	PlaybackConfig *playback_config = mwindow->edl->session->playback_config;
	AudioOutConfig *aconfig = playback_config->aconfig;
	VideoOutConfig *vconfig = playback_config->vconfig;

	rerender =
		edl->session->need_rerender(mwindow->edl->session) ||
		(preferences->force_uniprocessor != mwindow->preferences->force_uniprocessor) ||
		this_playback_config->active_config != playback_config->active_config ||
		(*this_aconfig != *aconfig) || (*this_vconfig != *vconfig) ||
		!preferences->brender_asset->equivalent(*mwindow->preferences->brender_asset, 0, 1, edl);

	if( preferences->yuv_color_space != mwindow->preferences->yuv_color_space ||
	    preferences->yuv_color_range != mwindow->preferences->yuv_color_range ) {
		YUV::yuv.yuv_set_colors(
			preferences->yuv_color_space,
			preferences->yuv_color_range);
		rerender = 1;
	}

	if( preferences->highlight_inverse != mwindow->preferences->highlight_inverse ) {
		mwindow->gui->lock_window("PreferencesThread::apply_settings 0");
		mwindow->gui->hide_cursor(0);
		mwindow->gui->unlock_window();
		redraw_overlays = 1;
	}
	PreferencesWindow *window = (PreferencesWindow*)get_gui();
	if( window ) window->unlock_window();
	mwindow->stop_brender();
	if( window ) window->lock_window("PreferencesThread::apply_settings 5");

	if( strcmp(preferences->theme, mwindow->preferences->theme) ||
	    strcmp(preferences->plugin_icons, mwindow->preferences->plugin_icons) ||
	    preferences->awindow_picon_h != mwindow->preferences->awindow_picon_h ||
	    preferences->vicon_size != mwindow->preferences->vicon_size ||
	    preferences->vicon_color_mode != mwindow->preferences->vicon_color_mode )
		mwindow->restart_status = -1;  // reconstruct/restart program
	if( strcmp(preferences->lv2_path, mwindow->preferences->lv2_path) != 0 )
		reload_plugins = 1;
	if( reload_plugins ) {
		MWindow::remove_plugin_index();
		File::setenv_path("LV2_PATH", preferences->lv2_path, 1);
		mwindow->restart_status = -1;
	}

	mwindow->edl->copy_session(edl, 1);
	mwindow->preferences->copy_from(preferences);

	BC_Signals::set_catch_segv(mwindow->preferences->trap_sigsegv);
	BC_Signals::set_catch_intr(mwindow->preferences->trap_sigintr);
	BC_WindowBase::get_resources()->popupmenu_btnup = mwindow->preferences->popupmenu_btnup;
	BC_WindowBase::get_resources()->grab_input_focus = mwindow->preferences->grab_input_focus;
	BC_WindowBase::get_resources()->textbox_focus_policy = mwindow->preferences->textbox_focus_policy;
	if( mwindow->preferences->trap_sigsegv || mwindow->preferences->trap_sigintr ) {
		BC_Trace::enable_locks();
	}
	else {
		BC_Trace::disable_locks();
	}

	mwindow->reset_android_remote();
	int ffmpeg_early_probe = mwindow->preferences->get_file_probe_armed("FFMPEG_Early");
	mwindow->gui->ffmpeg_toggle->update(ffmpeg_early_probe);
	mwindow->gui->ffmpeg_toggle->set_tooltip(ffmpeg_early_probe ?
		FFMPEG_EARLY_TIP : FFMPEG_LATE_TIP);
	mwindow->gui->mainshbtns->load(mwindow->preferences);
	mwindow->init_brender();

//edl->session->recording_format->dump();
//mwindow->edl->session->recording_format->dump();

	if(((mwindow->edl->session->output_w % 4) ||
		(mwindow->edl->session->output_h % 4)) &&
		mwindow->edl->session->playback_config->vconfig->driver == PLAYBACK_X11_GL)
	{
		MainError::show_error(
			_("This project's dimensions are not multiples of 4 so\n"
			"it can't be rendered by OpenGL."));
	}


	if(redraw_meters)
	{
		mwindow->cwindow->gui->lock_window("PreferencesThread::apply_settings");
		mwindow->cwindow->gui->meters->change_format(edl->session->meter_format,
			edl->session->min_meter_db,
			edl->session->max_meter_db);
		mwindow->cwindow->gui->unlock_window();



		for(int i = 0; i < mwindow->vwindows.size(); i++) {
			VWindow *vwindow = mwindow->vwindows.get(i);
			if( !vwindow->is_running() ) continue;
			vwindow->gui->lock_window("PreferencesThread::apply_settings");
			vwindow->gui->meters->change_format(edl->session->meter_format,
				edl->session->min_meter_db,
				edl->session->max_meter_db);
			vwindow->gui->unlock_window();

		}


		mwindow->gui->lock_window("PreferencesThread::apply_settings 1");
		mwindow->gui->set_meter_format(edl->session->meter_format,
			edl->session->min_meter_db,
			edl->session->max_meter_db);
		mwindow->gui->unlock_window();



		mwindow->lwindow->gui->lock_window("PreferencesThread::apply_settings");
		mwindow->lwindow->gui->panel->change_format(edl->session->meter_format,
			edl->session->min_meter_db,
			edl->session->max_meter_db);
		mwindow->lwindow->gui->unlock_window();
	}

	if(redraw_overlays)
	{
		mwindow->gui->lock_window("PreferencesThread::apply_settings 2");
		mwindow->gui->show_cursor(0);
		mwindow->gui->draw_overlays(1);
		mwindow->gui->unlock_window();
	}

	if(redraw_times)
	{
		mwindow->gui->lock_window("PreferencesThread::apply_settings 3");
		mwindow->gui->update(0, 0, 1, 0, 0, 1, 0);
		mwindow->gui->redraw_time_dependancies();
		mwindow->gui->unlock_window();
	}

	if(rerender)
	{
//printf("PreferencesThread::apply_settings 1\n");
// This doesn't stop and restart, only reloads the assets before
// the next play command.
		mwindow->cwindow->refresh_frame(CHANGE_ALL, mwindow->edl);
//printf("PreferencesThread::apply_settings 10\n");
	}

	if(redraw_times || redraw_overlays)
	{
		mwindow->gui->lock_window("PreferencesThread::apply_settings 4");
		mwindow->gui->flush();
		mwindow->gui->unlock_window();
	}

	return 0;
}

const char* PreferencesThread::category_to_text(int category)
{
	PlaybackConfig *playback_config = edl->session->playback_config;
	switch(category)
	{
		case PLAYBACK_A:
			return playback_config->active_config==0 ?
				 _("*Playback A") : _("Playback A");
		case PLAYBACK_B:
			return playback_config->active_config==1 ?
				 _("*Playback B") : _("Playback B");
		case RECORD:
			return _("Recording");
		case PERFORMANCE:
			return _("Performance");
		case INTERFACE:
			return _("Interface");
		case APPEARANCE:
			return _("Appearance");
		case ABOUT:
			return _("About");
	}
	return "";
}

int PreferencesThread::text_to_category(const char *category)
{
SET_TRACE
	int min_result = -1, result, result_num = 0;
	for(int i = 0; i < CATEGORIES; i++)
	{
		result = labs(strcmp(category_to_text(i), category));
		if(result < min_result || min_result < 0)
		{
			min_result = result;
			result_num = i;
		}
	}
SET_TRACE
	return result_num;
}








PreferencesWindow::PreferencesWindow(MWindow *mwindow,
	PreferencesThread *thread, int x, int y, int w, int h)
 : BC_Window(_(PROGRAM_NAME ": Preferences"), x,y, w,h,w,h, 1)
{
	this->mwindow = mwindow;
	this->thread = thread;
	dialog = 0;
	category = 0;
}

PreferencesWindow::~PreferencesWindow()
{
	lock_window("PreferencesWindow::~PreferencesWindow");
	delete category;


	if(dialog) delete dialog;


	for(int i = 0; i < categories.total; i++)
		delete categories.values[i];
	unlock_window();
}

void PreferencesWindow::create_objects()
{
	BC_Button *button;

	lock_window("PreferencesWindow::create_objects");
	set_icon(mwindow->theme->get_image("mwindow_icon"));
	mwindow->theme->draw_preferences_bg(this);
	flash();

	int x = mwindow->theme->preferencescategory_x;
	int y = mwindow->theme->preferencescategory_y;
	for(int i = 0; i < CATEGORIES; i++)
	{
		add_subwindow(category_button[i] = new PreferencesButton(mwindow,
			thread, x, y, i, thread->category_to_text(i),
			(i == thread->current_dialog) ?
				mwindow->theme->get_image_set("category_button_checked") :
				mwindow->theme->get_image_set("category_button")));
		x += category_button[i]->get_w() -
			mwindow->theme->preferences_category_overlap;
	}


// 	for(int i = 0; i < CATEGORIES; i++)
// 		categories.append(new BC_ListBoxItem(thread->category_to_text(i)));
// 	category = new PreferencesCategory(mwindow,
// 		thread,
// 		mwindow->theme->preferencescategory_x,
// 		mwindow->theme->preferencescategory_y);
// 	category->create_objects();


	add_subwindow(button = new PreferencesOK(mwindow, thread));
	add_subwindow(new PreferencesApply(mwindow, thread));
	add_subwindow(new PreferencesCancel(mwindow, thread));

	set_current_dialog(thread->current_dialog);

	show_window();
	unlock_window();
}

int PreferencesWindow::update_framerate()
{
	lock_window("PreferencesWindow::update_framerate");
	if(thread->current_dialog < PreferencesThread::RECORD)
	{
		dialog->draw_framerate(1);
//		flash();
	}
	unlock_window();
	return 0;
}


void PreferencesWindow::update_rates()
{
	lock_window("PreferencesWindow::update_rates");
	if(thread->current_dialog == PreferencesThread::PERFORMANCE)
	{
		dialog->update_rates();
	}
	unlock_window();
}


int PreferencesWindow::set_current_dialog(int number)
{
	if(dialog) delete dialog;
	dialog = 0;
	thread->current_dialog = number;

//PRINT_TRACE
	PreferencesDialog *dialog2 = dialog;
	dialog = 0;
//PRINT_TRACE

// Redraw category buttons
	for(int i = 0; i < CATEGORIES; i++)
	{
		if(i == number)
		{
			category_button[i]->set_images(
				mwindow->theme->get_image_set("category_button_checked"));
		}
		else
		{
			category_button[i]->set_images(
				mwindow->theme->get_image_set("category_button"));
		}
		category_button[i]->draw_face(0);

// Copy face to background for next button's overlap.
// Still can't do state changes right.
	}


//PRINT_TRACE
	PlaybackConfig *playback_config = thread->edl->session->playback_config;
	switch(number)
	{
		case PreferencesThread::PLAYBACK_A:
		case PreferencesThread::PLAYBACK_B:
			playback_config->load_defaults(mwindow->defaults,
				number == PreferencesThread::PLAYBACK_A ? 0 : 1);
			add_subwindow(dialog = new PlaybackPrefs(mwindow, this, number));
			break;

		case PreferencesThread::RECORD:
			add_subwindow(dialog = new RecordPrefs(mwindow, this));
			break;

		case PreferencesThread::PERFORMANCE:
			add_subwindow(dialog = new PerformancePrefs(mwindow, this));
			break;

		case PreferencesThread::INTERFACE:
			add_subwindow(dialog = new InterfacePrefs(mwindow, this));
			break;

		case PreferencesThread::APPEARANCE:
			add_subwindow(dialog = new AppearancePrefs(mwindow, this));
			break;

		case PreferencesThread::ABOUT:
			add_subwindow(dialog = new AboutPrefs(mwindow, this));
			break;
	}

//PRINT_TRACE
	if(dialog)
	{
		dialog->draw_top_background(this, 0, 0, dialog->get_w(), dialog->get_h());
//printf("PreferencesWindow::set_current_dialog %d\n", __LINE__);
		dialog->create_objects();
//printf("PreferencesWindow::set_current_dialog %d\n", __LINE__);
		dialog->lower_window();
		dialog->show_window(0);
	}

	if(dialog2)
	{
		dialog2->hide_window(0);
		delete dialog2;
	}

	return 0;
}











PreferencesButton::PreferencesButton(MWindow *mwindow,
	PreferencesThread *thread,
	int x,
	int y,
	int category,
	const char *text,
	VFrame **images)
 : BC_GenericButton(x, y, text, images)
{
	this->mwindow = mwindow;
	this->thread = thread;
	this->category = category;
}

int PreferencesButton::handle_event()
{
	thread->window->set_current_dialog(category);
	return 1;
}









PreferencesDialog::PreferencesDialog(MWindow *mwindow,
	PreferencesWindow *pwindow)
 : BC_SubWindow(10,
 	40,
	pwindow->get_w() - 20,
 	pwindow->get_h() - BC_GenericButton::calculate_h() - 10 - 40)
{
	this->pwindow = pwindow;
	this->mwindow = mwindow;
	preferences = pwindow->thread->preferences;
}

PreferencesDialog::~PreferencesDialog()
{
}

// ============================== category window




PreferencesApply::PreferencesApply(MWindow *mwindow, PreferencesThread *thread)
 : BC_GenericButton(thread->window->get_w() / 2 - BC_GenericButton::calculate_w(thread->window, _("Apply")) / 2,
 	thread->window->get_h() - BC_GenericButton::calculate_h() - 10,
	_("Apply"))
{
	this->mwindow = mwindow;
	this->thread = thread;
}
int PreferencesApply::handle_event()
{
	thread->apply_settings();
	mwindow->save_defaults();
	return 1;
}
int PreferencesApply::resize_event(int w, int h)
{
	reposition_window(w/2 - get_w()/2, h-get_h()-10);
	return 1;
}




PreferencesOK::PreferencesOK(MWindow *mwindow, PreferencesThread *thread)
 : BC_GenericButton(10,
 	thread->window->get_h() - BC_GenericButton::calculate_h() - 10,
	_("OK"))
{
	this->mwindow = mwindow;
	this->thread = thread;
}
int PreferencesOK::keypress_event()
{
	if(get_keypress() == RETURN)
	{
		thread->window->set_done(0);
		return 1;
	}
	return 0;
}
int PreferencesOK::handle_event()
{
	thread->window->set_done(0);
	return 1;
}
int PreferencesOK::resize_event(int w, int h)
{
	reposition_window(10, h-get_h()-10);
	return 1;
}



PreferencesCancel::PreferencesCancel(MWindow *mwindow, PreferencesThread *thread)
 : BC_GenericButton(thread->window->get_w() - BC_GenericButton::calculate_w(thread->window, _("Cancel")) - 10,
 	thread->window->get_h() - BC_GenericButton::calculate_h() - 10,
 	_("Cancel"))
{
	this->mwindow = mwindow;
	this->thread = thread;
}
int PreferencesCancel::keypress_event()
{
	if(get_keypress() == ESC)
	{
		thread->window->set_done(1);
		return 1;
	}
	return 0;
}
int PreferencesCancel::handle_event()
{
	thread->window->set_done(1);
	return 1;
}
int PreferencesCancel::resize_event(int w, int h)
{
	reposition_window(w-get_w()-10, h-get_h()-10);
	return 1;
}










PreferencesCategory::PreferencesCategory(MWindow *mwindow, PreferencesThread *thread, int x, int y)
 : BC_PopupTextBox(thread->window,
		&thread->window->categories,
		thread->category_to_text(thread->current_dialog),
		x,
		y,
		200,
		150)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

PreferencesCategory::~PreferencesCategory()
{
}

int PreferencesCategory::handle_event()
{
SET_TRACE
	thread->window->set_current_dialog(thread->text_to_category(get_text()));
SET_TRACE
	return 1;
}
