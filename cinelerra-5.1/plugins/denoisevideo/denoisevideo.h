
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

#ifndef DENOISEVIDEO_H
#define DENOISEVIDEO_H

class DenoiseVideo;
class DenoiseVideoWindow;

#include "bcdisplayinfo.h"
#include "bchash.inc"
#include "pluginvclient.h"
#include "vframe.inc"



class DenoiseVideoConfig
{
public:
	DenoiseVideoConfig();

	int equivalent(DenoiseVideoConfig &that);
	void copy_from(DenoiseVideoConfig &that);
	void interpolate(DenoiseVideoConfig &prev,
		DenoiseVideoConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame);

	int frames;
	float threshold;
	int count_changed;
	int do_r, do_g, do_b, do_a;
};




class DenoiseVideoFrames : public BC_ISlider
{
public:
	DenoiseVideoFrames(DenoiseVideo *plugin, int x, int y);
	int handle_event();
	DenoiseVideo *plugin;
};


class DenoiseVideoThreshold : public BC_TumbleTextBox
{
public:
	DenoiseVideoThreshold(DenoiseVideo *plugin,
		DenoiseVideoWindow *gui,
		int x,
		int y);
	int handle_event();
	DenoiseVideo *plugin;
};

class DenoiseVideoToggle : public BC_CheckBox
{
public:
	DenoiseVideoToggle(DenoiseVideo *plugin,
		DenoiseVideoWindow *gui,
		int x,
		int y,
		int *output,
		char *text);
	int handle_event();
	DenoiseVideo *plugin;
	int *output;
};

class DenoiseVideoCountChanged : public BC_Radial
{
public:
	DenoiseVideoCountChanged(DenoiseVideo *plugin,
		DenoiseVideoWindow *gui,
		int x,
		int y);
	int handle_event();
	DenoiseVideo *plugin;
	DenoiseVideoWindow *gui;
};

class DenoiseVideoCountSame : public BC_Radial
{
public:
	DenoiseVideoCountSame(DenoiseVideo *plugin,
		DenoiseVideoWindow *gui,
		int x,
		int y);
	int handle_event();
	DenoiseVideo *plugin;
	DenoiseVideoWindow *gui;
};

class DenoiseVideoWindow : public PluginClientWindow
{
public:
	DenoiseVideoWindow(DenoiseVideo *plugin);

	void create_objects();

	DenoiseVideo *plugin;
	DenoiseVideoFrames *frames;
	DenoiseVideoThreshold *threshold;
	DenoiseVideoToggle *do_r, *do_g, *do_b, *do_a;
	DenoiseVideoCountChanged *count_changed;
	DenoiseVideoCountSame *count_same;
};



class DenoiseVideo : public PluginVClient
{
public:
	DenoiseVideo(PluginServer *server);
	~DenoiseVideo();

	PLUGIN_CLASS_MEMBERS(DenoiseVideoConfig)
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	float *accumulation;
};



#endif
