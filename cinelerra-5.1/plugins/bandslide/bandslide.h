
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

#ifndef BANDSLIDE_H
#define BANDSLIDE_H

class BandSlideMain;
class BandSlideWindow;
class BandSlideConfig;

#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"




class BandSlideCount : public BC_TumbleTextBox
{
public:
	BandSlideCount(BandSlideMain *plugin,
		BandSlideWindow *window,
		int x,
		int y);
	int handle_event();
	BandSlideMain *plugin;
	BandSlideWindow *window;
};

class BandSlideIn : public BC_Radial
{
public:
	BandSlideIn(BandSlideMain *plugin,
		BandSlideWindow *window,
		int x,
		int y);
	int handle_event();
	BandSlideMain *plugin;
	BandSlideWindow *window;
};

class BandSlideOut : public BC_Radial
{
public:
	BandSlideOut(BandSlideMain *plugin,
		BandSlideWindow *window,
		int x,
		int y);
	int handle_event();
	BandSlideMain *plugin;
	BandSlideWindow *window;
};




class BandSlideWindow : public PluginClientWindow
{
public:
	BandSlideWindow(BandSlideMain *plugin);
	void create_objects();
	BandSlideMain *plugin;
	BandSlideCount *count;
	BandSlideIn *in;
	BandSlideOut *out;
};


class BandSlideConfig
{
public:
};

class BandSlideMain : public PluginVClient
{
public:
	BandSlideMain(PluginServer *server);
	~BandSlideMain();
	PLUGIN_CLASS_MEMBERS(BandSlideConfig);

// required for all realtime plugins
	int process_realtime(VFrame *incoming, VFrame *outgoing);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int uses_gui();
	int is_transition();

	int bands;
	int direction;
};

#endif
