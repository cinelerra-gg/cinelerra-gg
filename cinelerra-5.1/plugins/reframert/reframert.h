
/*
 * CINELERRA
 * Copyright (C) 2008-2016 Adam Williams <broadcast at earthling dot net>
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

#ifndef REFRAMERT_H
#define REFRAMERT_H


#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "pluginvclient.h"
#include "theme.h"
#include "transportque.h"


class ReframeRT;
class ReframeRTWindow;
class ReframeRTReset;

class ReframeRTConfig
{
public:
	ReframeRTConfig();
	void boundaries();
	int equivalent(ReframeRTConfig &src);
	void reset();
	void copy_from(ReframeRTConfig &src);
	void interpolate(ReframeRTConfig &prev,
		ReframeRTConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
// was scale
	double num;
	double denom;
	int stretch;
	int interp;
	int optic_flow;
};


class ReframeRTNum : public BC_TumbleTextBox
{
public:
	ReframeRTNum(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
};

class ReframeRTDenom : public BC_TumbleTextBox
{
public:
	ReframeRTDenom(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
};

class ReframeRTStretch : public BC_Radial
{
public:
	ReframeRTStretch(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
	ReframeRTWindow *gui;
};

class ReframeRTDownsample : public BC_Radial
{
public:
	ReframeRTDownsample(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
	ReframeRTWindow *gui;
};

class ReframeRTInterpolate : public BC_CheckBox
{
public:
	ReframeRTInterpolate(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
	ReframeRTWindow *gui;
};

class ReframeRTReset : public BC_GenericButton
{
public:
	ReframeRTReset(ReframeRT *plugin, ReframeRTWindow *gui, int x, int y);
	~ReframeRTReset();
	int handle_event();
	ReframeRT *plugin;
	ReframeRTWindow *gui;
};

class ReframeRTWindow : public PluginClientWindow
{
public:
	ReframeRTWindow(ReframeRT *plugin);
	~ReframeRTWindow();
	void create_objects();
	void update();
	ReframeRT *plugin;
	ReframeRTNum *num;
	ReframeRTDenom *denom;
	ReframeRTStretch *stretch;
	ReframeRTDownsample *downsample;
	ReframeRTInterpolate *interpolate;
	ReframeRTReset *reset;
};


class ReframeRT : public PluginVClient
{
public:
	ReframeRT(PluginServer *server);
	~ReframeRT();

	PLUGIN_CLASS_MEMBERS(ReframeRTConfig)

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_realtime();
	int is_synthesis();
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
};

#endif