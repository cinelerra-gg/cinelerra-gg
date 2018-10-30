
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

#ifndef ZOOM_H
#define ZOOM_H

class ZoomMain;
class ZoomWindow;

#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"


class ZoomLimit : public BC_TumbleTextBox
{
public:
	ZoomLimit(ZoomMain *plugin, ZoomWindow *window, float *output, int x, int y, int w);
	int handle_event();
	ZoomMain *plugin;
	float *output;
};


class ZoomWindow : public PluginClientWindow
{
public:
	ZoomWindow(ZoomMain *plugin);
	~ZoomWindow();

	void create_objects();


	ZoomMain *plugin;
	ZoomLimit *limit_x;
	ZoomLimit *limit_y;
};


class ZoomMain : public PluginVClient
{
public:
	ZoomMain(PluginServer *server);
	~ZoomMain();

// For user configuration
	int load_configuration();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	PluginClientWindow* new_window();


// Transition processing
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int uses_gui();
	int is_transition();
	int is_video();
	const char* plugin_title();
	int handle_opengl();

	float magnification_x;
	float magnification_y;
	float max_magnification_x;
	float max_magnification_y;
	OverlayFrame *overlayer;
	VFrame *temp;
	float in_x, in_y, in_w, in_h;
	int is_before;
};

#endif



