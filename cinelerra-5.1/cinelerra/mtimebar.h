
/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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

#ifndef MTIMEBAR_H
#define MTIMEBAR_H


#include "mwindowgui.inc"
#include "timebar.h"
#include "timelinepane.h"


class TimeBarPopup;

class TimeBarPopupItem : public BC_MenuItem
{
public:
	TimeBarPopupItem(MWindow *mwindow,
		TimeBarPopup *menu,
		const char *text,
		int value);
	int handle_event();

	int value;
	MWindow *mwindow;
	TimeBarPopup *menu;
};

class TimeBarPopup : public BC_PopupMenu
{
public:
	TimeBarPopup(MWindow *mwindow);
	~TimeBarPopup();

	void create_objects();
	void update();

	TimeBarPopupItem *items[TOTAL_TIMEFORMATS];
	MWindow *mwindow;
};



class MTimeBar : public TimeBar
{
public:
	MTimeBar(MWindow *mwindow,
		MWindowGUI *gui,
		int x,
		int y,
		int w,
		int h);
	MTimeBar(MWindow *mwindow,
		TimelinePane *pane,
		int x,
		int y,
		int w,
		int h);

	void create_objects();
	void draw_time();
	void draw_range();
	void stop_transport();
	int resize_event();
	int resize_event(int x, int y, int w, int h);
	int button_press_event();
//	int test_preview(int buttonpress);
	int64_t position_to_pixel(double position);
	void select_label(double position);
	double pixel_to_position(int pixel);
	void handle_mwindow_drag();
	void update(int flush);
	void update_cursor();
	void update_clock(double position);
	double test_highlight();
	int repeat_event(int64_t duration);
	void activate_timeline();

	TimeBarPopup *menu;
	MWindowGUI *gui;
};



#endif
