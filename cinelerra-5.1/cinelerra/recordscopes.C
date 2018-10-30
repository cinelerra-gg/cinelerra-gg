/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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



#include "bctheme.h"
#include "bccolors.h"
#include "language.h"
#include "mainsession.h"
#include "mutex.h"
#include "preferences.h"
#include "recordmonitor.h"
#include "recordscopes.h"
#include "theme.h"

RecordScopeThread::RecordScopeThread(MWindow *mwindow, RecordMonitor *record_monitor)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->record_monitor = record_monitor;
	scope_gui = 0;
	gui_lock = new Mutex("RecordScopeThread::gui_lock");
}

RecordScopeThread::~RecordScopeThread()
{
	close_window();
	delete gui_lock;
}

void RecordScopeThread::handle_done_event(int result)
{
	gui_lock->lock("RecordScopeThread::handle_done_event");
	scope_gui = 0;
	gui_lock->unlock();

	record_monitor->window->lock_window("RecordScopeThread::handle_done_event");
	record_monitor->window->scope_toggle->update(0);
	record_monitor->window->unlock_window();
}

BC_Window* RecordScopeThread::new_gui()
{
	RecordScopeGUI *gui = new RecordScopeGUI(mwindow, record_monitor);
	gui->create_objects();
	scope_gui = gui;
	return gui;
}

void RecordScopeThread::process(VFrame *output_frame)
{
	if(record_monitor->scope_thread)
	{
		record_monitor->scope_thread->gui_lock->lock("RecordScopeThread::process");
		if(record_monitor->scope_thread->scope_gui)
		{
			RecordScopeGUI *gui = record_monitor->scope_thread->scope_gui;
			gui->process(output_frame);
		}



		record_monitor->scope_thread->gui_lock->unlock();
	}
}









RecordScopeGUI::RecordScopeGUI(MWindow *mwindow,
	RecordMonitor *record_monitor)
 : ScopeGUI(mwindow->theme,
 	mwindow->session->scope_x,
	mwindow->session->scope_y,
	mwindow->session->scope_w,
	mwindow->session->scope_h,
	mwindow->preferences->processors)
{
	this->mwindow = mwindow;
	this->record_monitor = record_monitor;
}

RecordScopeGUI::~RecordScopeGUI()
{
}

void RecordScopeGUI::create_objects()
{
	use_hist = mwindow->session->use_hist;
	use_wave = mwindow->session->use_wave;
	use_vector = mwindow->session->use_vector;
	use_hist_parade = mwindow->session->use_hist_parade;
	use_wave_parade = mwindow->session->use_wave_parade;
	ScopeGUI::create_objects();
}


void RecordScopeGUI::toggle_event()
{
	mwindow->session->use_hist = use_hist;
	mwindow->session->use_wave = use_wave;
	mwindow->session->use_vector = use_vector;
	mwindow->session->use_hist_parade = use_hist_parade;
	mwindow->session->use_wave_parade = use_wave_parade;
}

int RecordScopeGUI::translation_event()
{
	ScopeGUI::translation_event();
	mwindow->session->scope_x = get_x();
	mwindow->session->scope_y = get_y();
	return 0;
}

int RecordScopeGUI::resize_event(int w, int h)
{
	ScopeGUI::resize_event(w, h);
	mwindow->session->scope_w = w;
	mwindow->session->scope_h = h;
	return 0;
}





ScopeEnable::ScopeEnable(MWindow *mwindow, RecordMonitor *record_monitor, int x, int y)
 : BC_Toggle(x,
 	y,
	mwindow->theme ? mwindow->theme->get_image_set("scope_toggle") : 0,
	mwindow->session->record_scope)
{
	this->record_monitor = record_monitor;
	this->mwindow = mwindow;
	set_tooltip(_("View scope"));
}

ScopeEnable::~ScopeEnable()
{
}


int ScopeEnable::handle_event()
{
	unlock_window();
	mwindow->session->record_scope = get_value();

	if(mwindow->session->record_scope)
	{
		record_monitor->scope_thread->start();
	}
	else
	{
		record_monitor->scope_thread->close_window();
	}

	lock_window("ScopeEnable::handle_event");
	return 1;
}
















