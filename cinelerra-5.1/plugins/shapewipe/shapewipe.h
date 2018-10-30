
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

#ifndef SHAPEWIPE_H
#define SHAPEWIPE_H

class ShapeWipeMain;
class ShapeWipeWindow;

#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"

class ShapeWipeW2B : public BC_Radial
{
public:
	ShapeWipeW2B(ShapeWipeMain *plugin,
		ShapeWipeWindow *window,
		int x,
		int y);
	int handle_event();
	ShapeWipeMain *plugin;
	ShapeWipeWindow *window;
};

class ShapeWipeB2W : public BC_Radial
{
public:
	ShapeWipeB2W(ShapeWipeMain *plugin,
		ShapeWipeWindow *window,
		int x,
		int y);
	int handle_event();
	ShapeWipeMain *plugin;
	ShapeWipeWindow *window;
};


class ShapeWipeTumble : public BC_Tumbler
{
public:
	ShapeWipeTumble(ShapeWipeMain *client,
		ShapeWipeWindow *window,
		int x,
		int y);

	int handle_up_event();
	int handle_down_event();

	ShapeWipeMain *client;
	ShapeWipeWindow *window;
};

class ShapeWipeShape : public BC_PopupTextBox
{
public:
	ShapeWipeShape(ShapeWipeMain *client,
		ShapeWipeWindow *window,
		int x,
		int y,
		int text_w,
		int list_h);
	int handle_event();
	ShapeWipeMain *client;
	ShapeWipeWindow *window;
};

class ShapeWipeAntiAlias : public BC_CheckBox
{
public:
	ShapeWipeAntiAlias(ShapeWipeMain *plugin,
		ShapeWipeWindow *window,
		int x,
		int y);
	int handle_event();
	ShapeWipeMain *plugin;
	ShapeWipeWindow *window;
};


class ShapeWipePreserveAspectRatio : public BC_CheckBox
{
public:
	ShapeWipePreserveAspectRatio(ShapeWipeMain *plugin,
		ShapeWipeWindow *window,
		int x,
		int y);
	int handle_event();
	ShapeWipeMain *plugin;
	ShapeWipeWindow *window;
};


class ShapeWipeWindow : public PluginClientWindow
{
public:
	ShapeWipeWindow(ShapeWipeMain *plugin);
	~ShapeWipeWindow();
	void create_objects();
	void next_shape();
	void prev_shape();

	ShapeWipeMain *plugin;
	ShapeWipeW2B *left;
	ShapeWipeB2W *right;
//	ShapeWipeFilename *filename_widget;
	ShapeWipeTumble *shape_tumbler;
	ShapeWipeShape *shape_text;
	ArrayList<BC_ListBoxItem*> shapes;
};

class ShapeWipeMain : public PluginVClient
{
public:
	ShapeWipeMain(PluginServer *server);
	~ShapeWipeMain();

// required for all realtime plugins
	int load_configuration();
	int process_realtime(VFrame *incoming, VFrame *outgoing);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	PluginClientWindow* new_window();
	int uses_gui();
	int is_transition();
	const char* plugin_title();
	int read_pattern_image(int new_frame_width, int new_frame_height);
	void reset_pattern_image();
	void init_shapes();

	ArrayList<char*> shape_paths;
	ArrayList<char*> shape_titles;

	int shapes_initialized;
	int direction;
	char filename[BCTEXTLEN];
	char last_read_filename[BCTEXTLEN];
	char shape_name[BCTEXTLEN];
	char current_name[BCTEXTLEN];
	unsigned char **pattern_image;
	unsigned char min_value;
	unsigned char max_value;
	int frame_width;
	int frame_height;
	int antialias;
	int preserve_aspect;
	int last_preserve_aspect;
};

#endif
