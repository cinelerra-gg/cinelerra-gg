
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

#ifndef CTIMEBAR_H
#define CTIMEBAR_H




#include "cwindowgui.inc"
#include "timebar.h"





class CTimeBar : public TimeBar
{
public:
	CTimeBar(MWindow *mwindow,
		CWindowGUI *gui,
		int x,
		int y,
		int w,
		int h);

	int resize_event();
	EDL* get_edl();
	void draw_time();
//	void update_preview();
	void select_label(double position);
	double pixel_to_position(int pixel);
	void update_cursor();
	int has_preview() { return 1; }

	CWindowGUI *gui;
};


#endif
