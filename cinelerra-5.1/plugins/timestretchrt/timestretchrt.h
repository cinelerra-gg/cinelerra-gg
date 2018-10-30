
/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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

#ifndef RESAMPLERT_H
#define RESAMPLERT_H


#include "guicast.h"
#include "pluginaclient.h"
#include "resample.h"
#include "../timestretch/timestretchengine.inc"



class TimeStretchRT;
class TimeStretchRTWindow;

class TimeStretchRTConfig
{
public:
	TimeStretchRTConfig();
	void boundaries();
	int equivalent(TimeStretchRTConfig &src);
	void copy_from(TimeStretchRTConfig &src);
	void interpolate(TimeStretchRTConfig &prev,
		TimeStretchRTConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
//	double scale;
	double num;
	double denom;
	int size;
};


class TimeStretchRTScale : public BC_TumbleTextBox
{
public:
	TimeStretchRTScale(TimeStretchRTWindow *window,
		TimeStretchRT *plugin,
		int x,
		int y,
		double *value);
	int handle_event();
	TimeStretchRT *plugin;
	double *value;
};


class TimeStretchRTSize : public BC_TumbleTextBox
{
public:
	TimeStretchRTSize(TimeStretchRTWindow *window, TimeStretchRT *plugin, int x, int y);

	int handle_event();

	TimeStretchRT *plugin;
};

class TimeStretchRTWindow : public PluginClientWindow
{
public:
	TimeStretchRTWindow(TimeStretchRT *plugin);
	~TimeStretchRTWindow();
	void create_objects();

	TimeStretchRT *plugin;
	TimeStretchRTScale *num;
	TimeStretchRTScale *denom;
	TimeStretchRTSize *size;
};




class TimeStretchRT : public PluginAClient
{
public:
	TimeStretchRT(PluginServer *server);
	~TimeStretchRT();

	PLUGIN_CLASS_MEMBERS(TimeStretchRTConfig)

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_realtime();
	int is_synthesis();
	int process_buffer(int64_t size,
		Samples *buffer,
		int64_t start_position,
		int sample_rate);

	TimeStretchEngine *engine;
	int64_t source_start;
	int64_t dest_start;
	int need_reconfigure;
};


#endif

