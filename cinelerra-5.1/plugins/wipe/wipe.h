
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

#ifndef WIPE_H
#define WIPE_H

class WipeMain;
class WipeWindow;


#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"




class WipeLeft : public BC_Radial
{
public:
	WipeLeft(WipeMain *plugin,
		WipeWindow *window,
		int x,
		int y);
	int handle_event();
	WipeMain *plugin;
	WipeWindow *window;
};

class WipeRight : public BC_Radial
{
public:
	WipeRight(WipeMain *plugin,
		WipeWindow *window,
		int x,
		int y);
	int handle_event();
	WipeMain *plugin;
	WipeWindow *window;
};




class WipeWindow : public PluginClientWindow
{
public:
	WipeWindow(WipeMain *plugin);
	void create_objects();
	WipeMain *plugin;
	WipeLeft *left;
	WipeRight *right;
};




class WipeMain : public PluginVClient
{
public:
	WipeMain(PluginServer *server);
	~WipeMain();

// required for all realtime plugins
	int load_configuration();
	int process_realtime(VFrame *incoming, VFrame *outgoing);
	PluginClientWindow* new_window();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int uses_gui();
	int is_transition();
	int is_video();
	const char* plugin_title();

	int direction;
};

#endif
