
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

#ifndef SLIDE_H
#define SLIDE_H

class SlideMain;
class SlideWindow;


#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"




class SlideLeft : public BC_Radial
{
public:
	SlideLeft(SlideMain *plugin,
		SlideWindow *window,
		int x,
		int y);
	int handle_event();
	SlideMain *plugin;
	SlideWindow *window;
};

class SlideRight : public BC_Radial
{
public:
	SlideRight(SlideMain *plugin,
		SlideWindow *window,
		int x,
		int y);
	int handle_event();
	SlideMain *plugin;
	SlideWindow *window;
};

class SlideIn : public BC_Radial
{
public:
	SlideIn(SlideMain *plugin,
		SlideWindow *window,
		int x,
		int y);
	int handle_event();
	SlideMain *plugin;
	SlideWindow *window;
};

class SlideOut : public BC_Radial
{
public:
	SlideOut(SlideMain *plugin,
		SlideWindow *window,
		int x,
		int y);
	int handle_event();
	SlideMain *plugin;
	SlideWindow *window;
};





class SlideWindow : public PluginClientWindow
{
public:
	SlideWindow(SlideMain *plugin);
	void create_objects();
	SlideMain *plugin;
	SlideLeft *left;
	SlideRight *right;
	SlideIn *in;
	SlideOut *out;
};




class SlideMain : public PluginVClient
{
public:
	SlideMain(PluginServer *server);
	~SlideMain();

// required for all realtime plugins
	int load_configuration();
	int process_realtime(VFrame *incoming, VFrame *outgoing);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int uses_gui();
	int is_transition();
	int is_video();
	const char* plugin_title();
	PluginClientWindow* new_window();

	int motion_direction;
	int direction;
};

#endif
