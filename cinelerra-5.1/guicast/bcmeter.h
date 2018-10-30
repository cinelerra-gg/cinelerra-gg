
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

#ifndef BCMETER_H
#define BCMETER_H

#include "bcmeter.inc"
#include "bcsubwindow.h"
#include "units.h"

#define METER_TYPES 2

#define METER_DB 0
#define METER_INT 1
#define METER_VERT 0
#define METER_HORIZ 1

// Distance from subwindow border to top and bottom tick mark
#define METER_MARGIN 0

class BC_Meter : public BC_SubWindow
{
public:
	BC_Meter(int x,
		int y,
		int orientation,
		int pixels,
		int min, /* = -40, */
		int max,
		int mode, /* = METER_DB, */
		int use_titles, /* = 0, */
		int span, /* = -1 width for vertical mode only */
		int downmix = 0);
	virtual ~BC_Meter();

	int initialize();
	int set_delays(
// Number of updates before over dissappears
		int over_delay, /* = 150, */
// Number of updates before peak updates
		int peak_delay /* = 15 */);
	void set_images(VFrame **data);
	int region_pixel(int region);
	int region_pixels(int region);
	virtual int button_press_event();

	static int get_title_w();
	static int get_meter_w();
	int update(float new_value, int over, int dmix=-1);
	int reposition_window(int x,
		int y,
		int span /* = -1 for vertical mode only */,
		int pixels);
	int reset(int dmix=-1);
	int reset_over();
	int change_format(int mode, int min, int max);

private:
	void draw_titles(int flush);
	void draw_face(int flush);
	int level_to_pixel(float level);
	void get_divisions();

	BC_Pixmap *images[TOTAL_METER_IMAGES];
	int orientation;
// Number of pixels in the longest dimension
	int pixels;
// Width if variable width
	int span;
	int low_division;
	int medium_division;
	int high_division;
	int use_titles;
// Tick mark positions
	ArrayList<int> tick_pixels;
// Title positions
	ArrayList<int> title_pixels;
	ArrayList<char*> db_titles;
	float level, peak;
	int mode;
	DB db;
	int peak_timer;






	int peak_pixel, level_pixel, peak_pixel1, peak_pixel2;
	int over_count, over_timer;
	int min, max;
	int downmix;
	int over_delay;       // Number of updates the over warning lasts.
	int peak_delay;       // Number of updates the peak lasts.
};

#endif
