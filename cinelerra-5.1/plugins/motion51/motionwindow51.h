
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

#include "guicast.h"
#include "motion51.inc"


class Motion51SampleSteps : public BC_PopupMenu
{
public:
	Motion51SampleSteps(Motion51Main *plugin, int x, int y, int w);
	void create_objects();
	int handle_event();
	Motion51Main *plugin;
};


class Motion51DrawVectors : public BC_CheckBox
{
public:
	Motion51DrawVectors(Motion51Main *plugin, Motion51Window *gui, int x, int y);
	int handle_event();
	Motion51Main *plugin;
	Motion51Window *gui;
};

class Motion51TrackingFile : public BC_TextBox
{
public:
	Motion51TrackingFile(Motion51Main *plugin, const char *filename,
		Motion51Window *gui, int x, int y, int w);
	int handle_event();
	Motion51Main *plugin;
	Motion51Window *gui;
};

class Motion51Limits : public BC_TumbleTextBox {
public:
	Motion51Limits(Motion51Main *plugin, Motion51Window *gui, int x, int y,
		const char *ttext, float *value, float min, float max, int ttbox_w);
	~Motion51Limits();

	void create_objects();
	int handle_event();

	Motion51Main *plugin;
	Motion51Window *gui;
	float *value;
	const char *ttext;
	BC_Title *title;

	int get_w() { return BC_TumbleTextBox::get_w()+5+title->get_w(); }
	int get_h() { return BC_TumbleTextBox::get_h(); }
};

class Motion51ResetConfig : public BC_GenericButton
{
public:
	Motion51ResetConfig(Motion51Main *plugin, Motion51Window *gui, int x, int y);
	int handle_event();
	Motion51Main *plugin;
	Motion51Window *gui;
};

class Motion51ResetTracking : public BC_GenericButton
{
public:
	Motion51ResetTracking(Motion51Main *plugin, Motion51Window *gui, int x, int y);
	int handle_event();
	Motion51Main *plugin;
	Motion51Window *gui;
};

class Motion51EnableTracking : public BC_CheckBox
{
public:
	Motion51EnableTracking(Motion51Main *plugin, Motion51Window *gui, int x, int y);
	int handle_event();
	Motion51Main *plugin;
	Motion51Window *gui;
};

class Motion51Window : public PluginClientWindow
{
public:
	Motion51Window(Motion51Main *plugin);
	~Motion51Window();

	void create_objects();
	void update_gui();

	Motion51Main *plugin;
	Motion51Limits *horiz_limit, *vert_limit, *twist_limit;
	Motion51Limits *shake_fade, *twist_fade;
	Motion51Limits *sample_r;
	Motion51SampleSteps *sample_steps;
	Motion51Limits *block_x, *block_y, *block_w, *block_h;
	Motion51DrawVectors *draw_vectors;
	Motion51TrackingFile *tracking_file;
	Motion51ResetConfig *reset_config;
	Motion51ResetTracking *reset_tracking;
	Motion51EnableTracking *enable_tracking;

	BC_Title *pef_title;
};


