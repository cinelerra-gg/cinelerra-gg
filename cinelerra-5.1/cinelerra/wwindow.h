
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

#ifndef WARNWINDOW_H
#define WARNWINDOW_H

#include "bcwindowbase.h"
#include "bcdialog.h"
#include "bctoggle.h"
#include "guicast.h"
#include "mwindow.inc"

class WWindow;
class WWindowGUI;
class WDisable;


class WWindow : public BC_DialogThread
{
public:
	WWindow(MWindow *mwindow);
	~WWindow();
	void handle_close_event(int result);
	void show_warning(int *do_warning, const char *warn_text);
	BC_Window* new_gui();
	int wait_result();

	MWindow *mwindow;
	WWindowGUI *gui;
	int *do_warning;
	int result;
	const char *warn_text;
};

class WWindowGUI : public BC_Window
{
public:
	WWindowGUI(WWindow *thread, int x, int y);
	void create_objects();
	WWindow *thread;
};

class WDisable : public BC_CheckBox
{
public:
	WDisable(WWindowGUI *gui, int x, int y);
	int handle_event();
	WWindowGUI *gui;
};

#endif
