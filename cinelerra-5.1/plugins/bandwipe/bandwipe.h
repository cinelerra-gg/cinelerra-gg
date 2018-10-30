
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

#ifndef BANDWIPE_H
#define BANDWIPE_H

class BandWipeMain;
class BandWipeWindow;
class BandWipeConfig;


#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"




class BandWipeCount : public BC_TumbleTextBox
{
public:
	BandWipeCount(BandWipeMain *plugin,
		BandWipeWindow *window,
		int x,
		int y);
	int handle_event();
	BandWipeMain *plugin;
	BandWipeWindow *window;
};

class BandWipeIn : public BC_Radial
{
public:
	BandWipeIn(BandWipeMain *plugin,
		BandWipeWindow *window,
		int x,
		int y);
	int handle_event();
	BandWipeMain *plugin;
	BandWipeWindow *window;
};

class BandWipeOut : public BC_Radial
{
public:
	BandWipeOut(BandWipeMain *plugin,
		BandWipeWindow *window,
		int x,
		int y);
	int handle_event();
	BandWipeMain *plugin;
	BandWipeWindow *window;
};




class BandWipeWindow : public PluginClientWindow
{
public:
	BandWipeWindow(BandWipeMain *plugin);
	void create_objects();
	BandWipeMain *plugin;
	BandWipeCount *count;
	BandWipeIn *in;
	BandWipeOut *out;
};



class BandWipeConfig
{
public:
};


class BandWipeMain : public PluginVClient
{
public:
	BandWipeMain(PluginServer *server);
	~BandWipeMain();

	PLUGIN_CLASS_MEMBERS(BandWipeConfig);

	int process_realtime(VFrame *incoming, VFrame *outgoing);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int uses_gui();
	int is_transition();

	int bands;
	int direction;
};

#endif
