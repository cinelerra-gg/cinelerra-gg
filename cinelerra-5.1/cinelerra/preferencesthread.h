
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

#ifndef PREFERENCESTHREAD_H
#define PREFERENCESTHREAD_H

#include "bcdialog.h"
#include "edl.inc"
#include "guicast.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "preferences.inc"
#include "preferencesthread.inc"


class PreferencesMenuitem : public BC_MenuItem
{
public:
	PreferencesMenuitem(MWindow *mwindow);
	~PreferencesMenuitem();

	int handle_event();

	MWindow *mwindow;
	PreferencesThread *thread;
};

class PreferencesThread : public BC_DialogThread
{
public:
	PreferencesThread(MWindow *mwindow);
	~PreferencesThread();

	BC_Window* new_gui();
	void handle_close_event(int result);
// update rendering rates
	void update_rates();
// update playback rate
	int update_framerate();
	int apply_settings();
	const char* category_to_text(int category);
	int text_to_category(const char *category);

	int current_dialog;
	int thread_running;
	int redraw_indexes;
	int redraw_meters;
	int redraw_times;
	int redraw_overlays;
	int rerender;
	int close_assets;
	int reload_plugins;
	PreferencesWindow *window;
	MWindow *mwindow;
// Copy of mwindow preferences
	Preferences *preferences;
	EDL *edl;

// Categories
#define CATEGORIES 7
	enum
	{
		PLAYBACK_A,
		PLAYBACK_B,
		RECORD,
		PERFORMANCE,
		INTERFACE,
		APPEARANCE,
		ABOUT
	};
};

class PreferencesDialog : public BC_SubWindow
{
public:
	PreferencesDialog(MWindow *mwindow, PreferencesWindow *pwindow);
	virtual ~PreferencesDialog();

	virtual void create_objects() {}
// update playback rate
	virtual int draw_framerate(int flush) { return 0; }
// update rendering rates
	virtual void update_rates() {}
	virtual int show_window(int flush) { return BC_SubWindow::show_window(flush); }

	PreferencesWindow *pwindow;
	MWindow *mwindow;
	Preferences *preferences;
};

class PreferencesCategory;
class PreferencesButton;

class PreferencesWindow : public BC_Window
{
public:
	PreferencesWindow(MWindow *mwindow,
		PreferencesThread *thread,
		int x, int y, int w, int h);
	~PreferencesWindow();

	void create_objects();
	int delete_current_dialog();
	int set_current_dialog(int number);
	int update_framerate();
	void update_rates();
	void show_dialog() { dialog->show_window(0); }
	MWindow *mwindow;
	PreferencesThread *thread;
	ArrayList<BC_ListBoxItem*> categories;
	PreferencesCategory *category;
	PreferencesButton *category_button[CATEGORIES];

private:
	PreferencesDialog *dialog;
};

class PreferencesButton : public BC_GenericButton
{
public:
	PreferencesButton(MWindow *mwindow,
		PreferencesThread *thread,
		int x,
		int y,
		int category,
		const char *text,
		VFrame **images);

	int handle_event();

	MWindow *mwindow;
	PreferencesThread *thread;
	int category;
};

class PreferencesCategory : public BC_PopupTextBox
{
public:
	PreferencesCategory(MWindow *mwindow, PreferencesThread *thread, int x, int y);
	~PreferencesCategory();
	int handle_event();
	MWindow *mwindow;
	PreferencesThread *thread;
};

class PreferencesApply : public BC_GenericButton
{
public:
	PreferencesApply(MWindow *mwindow, PreferencesThread *thread);
	int handle_event();
	int resize_event(int w, int h);
	MWindow *mwindow;
	PreferencesThread *thread;
};

class PreferencesOK : public BC_GenericButton
{
public:
	PreferencesOK(MWindow *mwindow, PreferencesThread *thread);
	int keypress_event();
	int handle_event();
	int resize_event(int w, int h);
	MWindow *mwindow;
	PreferencesThread *thread;
};

class PreferencesCancel : public BC_GenericButton
{
public:
	PreferencesCancel(MWindow *mwindow, PreferencesThread *thread);
	int keypress_event();
	int handle_event();
	int resize_event(int w, int h);
	MWindow *mwindow;
	PreferencesThread *thread;
};


#endif
