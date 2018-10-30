
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

#ifndef METERPANEL_H
#define METERPANEL_H

#include "guicast.h"
#include "mwindow.inc"
#include "theme.inc"

class MeterReset;
class MeterMeter;

class MeterPanel
{
public:
	MeterPanel(MWindow *mwindow,
		BC_WindowBase *subwindow,
		int x,
		int y,
		int w, /* Ignored if -1 */
		int h,
		int meter_count,
		int visible,
		int use_headroom /* = 0 */,
		int use_titles);
	virtual ~MeterPanel();

	void create_objects();
	int set_meters(int meter_count, int visible);
	static int get_meters_width(Theme *theme, int meter_count, int visible);
	void reposition_window(int x,
		int y,
		int w,  /* Ignored if -1 */
		int h);
	int get_title_x(int number, const char *text);
	int get_reset_x();
	int get_reset_y();
	int get_meter_h();
// Calculate meter width without border
	int get_meter_w(int number);
	void update(double *levels);
	void init_meters(int dmix);
	void update_peak(int number, float value);
	void stop_meters();
	void change_format(int mode, int min, int max);
	virtual int change_status_event(int new_status);
	void reset_meters();

	MWindow *mwindow;
	BC_WindowBase *subwindow;
	ArrayList<MeterMeter*> meters;
	ArrayList<BC_Title*> meter_titles;
	ArrayList<float> meter_peaks;
	MeterReset *reset;
	int meter_count;
	int visible;
	int x, y, w, h;
	int use_headroom;
	int use_titles;
};


class MeterReset : public BC_Button
{
public:
	MeterReset(MWindow *mwindow, MeterPanel *panel, int x, int y);
	~MeterReset();
	int handle_event();
	MWindow *mwindow;
	MeterPanel *panel;
};

class MeterShow : public BC_Toggle
{
public:
	MeterShow(MWindow *mwindow, MeterPanel *panel, int x, int y);
	~MeterShow();
	int handle_event();
	MWindow *mwindow;
	MeterPanel *panel;
};

class MeterMeter : public BC_Meter
{
public:
	MeterMeter(MWindow *mwindow,
		MeterPanel *panel,
		int x,
		int y,
		int w,
		int h,
		int titles);
	~MeterMeter();

	int button_press_event();

	MWindow *mwindow;
	MeterPanel *panel;
};

#endif
