
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

#ifndef HISTOGRAMWINDOW_H
#define HISTOGRAMWINDOW_H



#include "histogram.inc"
#include "histogramwindow.inc"
#include "pluginvclient.h"



class HistogramSlider : public BC_SubWindow
{
public:
	HistogramSlider(HistogramMain *plugin,
		HistogramWindow *gui,
		int x,
		int y,
		int w,
		int h,
		int is_input);

	void update();
	int input_to_pixel(float input);

	int operation;
	enum
	{
		NONE,
		DRAG_INPUT,
		DRAG_MIN_OUTPUT,
		DRAG_MAX_OUTPUT,
	};
	int is_input;
	HistogramMain *plugin;
	HistogramWindow *gui;
};

class HistogramParade : public BC_Toggle
{
public:
	HistogramParade(HistogramMain *plugin,
		HistogramWindow *gui,
		int x,
		int y,
		int value);
	int handle_event();
	HistogramMain *plugin;
	HistogramWindow *gui;
	int value;
};

class HistogramCarrot : public BC_Toggle
{
public:
	HistogramCarrot(HistogramMain *plugin,
		HistogramWindow *gui,
		int x,
		int y);
	virtual ~HistogramCarrot();

	void update();
	float* get_value();
	int cursor_motion_event();
	int button_release_event();
	int button_press_event();

	int starting_x;
	int offset_x;
	int offset_y;
	int drag_operation;
	HistogramMain *plugin;
	HistogramWindow *gui;
};


class HistogramAuto : public BC_CheckBox
{
public:
	HistogramAuto(HistogramMain *plugin,
		int x,
		int y);
	int handle_event();
	HistogramMain *plugin;
};

class HistogramPlot : public BC_CheckBox
{
public:
	HistogramPlot(HistogramMain *plugin,
		int x,
		int y);
	int handle_event();
	HistogramMain *plugin;
};

class HistogramSplit : public BC_CheckBox
{
public:
	HistogramSplit(HistogramMain *plugin,
		int x,
		int y);
	int handle_event();
	HistogramMain *plugin;
};

class HistogramMode : public BC_Radial
{
public:
	HistogramMode(HistogramMain *plugin,
		int x,
		int y,
		int value,
		char *text);
	int handle_event();
	HistogramMain *plugin;
	int value;
};

class HistogramReset : public BC_GenericButton
{
public:
	HistogramReset(HistogramMain *plugin,
		int x,
		int y);
	int handle_event();
	HistogramMain *plugin;
};

class HistogramText : public BC_TumbleTextBox
{
public:
	HistogramText(HistogramMain *plugin,
		HistogramWindow *gui,
		int x,
		int y);

	int handle_event();
	void update();
	float* get_value();

	HistogramMain *plugin;
	HistogramWindow *gui;
};

class HistogramCanvas : public BC_SubWindow
{
public:
	HistogramCanvas(HistogramMain *plugin,
		HistogramWindow *gui,
		int x,
		int y,
		int w,
		int h);
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();
	HistogramMain *plugin;
	HistogramWindow *gui;
};

class HistogramWindow : public PluginClientWindow
{
public:
	HistogramWindow(HistogramMain *plugin);
	~HistogramWindow();

	void create_objects();
	void update(int do_canvases,
		int do_carrots,
		int do_text,
		int do_toggles);
	void draw_canvas_mode(int mode, int color, int y, int h);
	void update_canvas();
	int keypress_event();
	int resize_event(int w, int h);

	void get_point_extents(HistogramPoint *current,
		int *x1,
		int *y1,
		int *x2,
		int *y2,
		int *x,
		int *y);

	HistogramSlider *output;
	HistogramAuto *automatic;
	HistogramMode *mode_v, *mode_r, *mode_g, *mode_b /*,  *mode_a */;
	HistogramParade *parade_on, *parade_off;
	HistogramText *low_output;
	HistogramText *high_output;
	HistogramText *threshold;
	HistogramText *low_input;
	HistogramText *high_input;
	HistogramText *gamma;
	HistogramCanvas *canvas;
	HistogramCarrot *low_input_carrot;
	HistogramCarrot *gamma_carrot;
	HistogramCarrot *high_input_carrot;
	HistogramCarrot *low_output_carrot;
	HistogramCarrot *high_output_carrot;
	HistogramReset *reset;
	BC_Title *canvas_title1;
	BC_Title *canvas_title2;
	BC_Title *threshold_title;
	BC_Bar *bar;

// Value to change with keypresses
	float *active_value;
	HistogramMain *plugin;
	int canvas_w;
	int canvas_h;
	int title1_x;
	int title2_x;
	int title3_x;
	int title4_x;
	HistogramPlot *plot;
	HistogramSplit *split;
};







#endif
