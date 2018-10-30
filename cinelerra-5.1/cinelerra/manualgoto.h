
/*
 * CINELERRA
 * Copyright (C) 2004 Andraz Tori
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

#ifndef MANUALGOTO_H
#define MANUALGOTO_H

#include "bcwindowbase.h"
#include "bcdialog.h"
#include "mwindow.inc"
#include "editpanel.inc"

class ManualGoto;
class ManualGotoWindow;
class ManualGotoNumber;

class ManualGoto : public BC_DialogThread
{
public:
	ManualGoto(MWindow *mwindow, EditPanel *panel);
	~ManualGoto();

	EditPanel *panel;
	MWindow *mwindow;
	ManualGotoWindow *gotowindow;

	BC_Window *new_gui();
	void handle_done_event(int result);

};


class ManualGotoWindow : public BC_Window
{
public:
	ManualGotoWindow(ManualGoto *mango, int x, int y);
	~ManualGotoWindow();

	void create_objects();
	void reset_data(double position);
	double get_new_position();
	void update_position(double position);

	ManualGoto *mango;
	BC_Title *signtitle;
	ManualGotoNumber *hours;
	ManualGotoNumber *minutes;
	ManualGotoNumber *seconds;
	ManualGotoNumber *msecs;
};



class ManualGotoNumber : public BC_TextBox
{
public:
	ManualGotoNumber(ManualGotoWindow *window, int x, int y, int w,
		int max, const char *format);
	int handle_event();
	ManualGotoWindow *window;
	int keypress_event();
	void update(int64_t number);

	int min, max;
	const char *format;
};

#endif
