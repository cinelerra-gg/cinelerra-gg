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


#ifndef RECORDSCOPES_H
#define RECORDSCOPES_H

#include "bcdialog.h"
#include "guicast.h"
#include "loadbalance.h"
#include "mwindow.h"
#include "recordmonitor.inc"
#include "recordscopes.inc"
#include "scopewindow.h"



class RecordScopeThread : public BC_DialogThread
{
public:
	RecordScopeThread(MWindow *mwindow, RecordMonitor *record_monitor);
	~RecordScopeThread();

	void handle_done_event(int result);
	BC_Window* new_gui();
	void process(VFrame *output_frame);

	MWindow *mwindow;
	RecordMonitor *record_monitor;
	RecordScopeGUI *scope_gui;
	Mutex *gui_lock;
	VFrame *output_frame;
};



class RecordScopeGUI : public ScopeGUI
{
public:
	RecordScopeGUI(MWindow *mwindow,
		RecordMonitor *record_monitor);
	~RecordScopeGUI();

	void create_objects();
	void toggle_event();
	int translation_event();
	int resize_event(int w, int h);

	MWindow *mwindow;
	RecordMonitor *record_monitor;
};





class ScopeEnable : public BC_Toggle
{
public:
	ScopeEnable(MWindow *mwindow, RecordMonitor *record_monitor, int x, int y);
	~ScopeEnable();
	int handle_event();
	RecordMonitor *record_monitor;
	MWindow *mwindow;
};


#endif


