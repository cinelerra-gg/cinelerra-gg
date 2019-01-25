
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

#ifndef TEMPORALSWAP_H
#define TEMPORALSWAP_H


#include "pluginvclient.h"


class SwapFrames;
class SwapFramesWindow;
class SwapFramesReset;

class SwapFramesConfig
{
public:
	SwapFramesConfig();

	void reset();
	int equivalent(SwapFramesConfig &that);
	void copy_from(SwapFramesConfig &that);
	void interpolate(SwapFramesConfig &prev,
		SwapFramesConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
	int on;
	int swap_even;
};


class SwapFramesEven : public BC_Radial
{
public:
	SwapFramesEven(SwapFrames *plugin,
		SwapFramesWindow *gui,
		int x,
		int y);
	int handle_event();
	SwapFrames *plugin;
	SwapFramesWindow *gui;
};


class SwapFramesOdd : public BC_Radial
{
public:
	SwapFramesOdd(SwapFrames *plugin,
		SwapFramesWindow *gui,
		int x,
		int y);
	int handle_event();
	SwapFrames *plugin;
	SwapFramesWindow *gui;
};

class SwapFramesOn : public BC_CheckBox
{
public:
	SwapFramesOn(SwapFrames *plugin, int x, int y);
	int handle_event();
	SwapFrames *plugin;
};

class SwapFramesReset : public BC_GenericButton
{
public:
	SwapFramesReset(SwapFrames *plugin, SwapFramesWindow *gui, int x, int y);
	~SwapFramesReset();
	int handle_event();
	SwapFrames *plugin;
	SwapFramesWindow *gui;
};


class SwapFramesWindow : public PluginClientWindow
{
public:
	SwapFramesWindow(SwapFrames *plugin);
	void create_objects();
	void update();
	SwapFramesOn *on;
	SwapFramesEven *swap_even;
	SwapFramesOdd *swap_odd;
	SwapFrames *plugin;
	SwapFramesReset *reset;
};


class SwapFrames : public PluginVClient
{
public:
	SwapFrames(PluginServer *server);
	~SwapFrames();

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	PLUGIN_CLASS_MEMBERS(SwapFramesConfig)


// Buffer out of order frames
	VFrame *buffer;
	int64_t buffer_position;
	int64_t prev_frame;
};


#endif


