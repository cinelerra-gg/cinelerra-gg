
/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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



class ResampleRT;
class ResampleRTWindow;

class ResampleRTConfig
{
public:
	ResampleRTConfig();
	void boundaries();
	int equivalent(ResampleRTConfig &src);
	void copy_from(ResampleRTConfig &src);
	void interpolate(ResampleRTConfig &prev,
		ResampleRTConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
// was scale
	double num;
	double denom;
};


class ResampleRTNum : public BC_TumbleTextBox
{
public:
	ResampleRTNum(ResampleRTWindow *window,
		ResampleRT *plugin,
		int x,
		int y);
	int handle_event();
	ResampleRT *plugin;
};

class ResampleRTDenom : public BC_TumbleTextBox
{
public:
	ResampleRTDenom(ResampleRTWindow *window,
		ResampleRT *plugin,
		int x,
		int y);
	int handle_event();
	ResampleRT *plugin;
};

class ResampleRTWindow : public PluginClientWindow
{
public:
	ResampleRTWindow(ResampleRT *plugin);
	~ResampleRTWindow();
	void create_objects();

	ResampleRT *plugin;
	ResampleRTNum *num;
	ResampleRTDenom *denom;
};


class ResampleRTResample : public Resample
{
public:
	ResampleRTResample(ResampleRT *plugin);
	int read_samples(Samples *buffer, int64_t start, int64_t len);
	ResampleRT *plugin;
};



class ResampleRT : public PluginAClient
{
public:
	ResampleRT(PluginServer *server);
	~ResampleRT();

	PLUGIN_CLASS_MEMBERS(ResampleRTConfig)

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_realtime();
	int is_synthesis();
	int process_buffer(int64_t size,
		Samples *buffer,
		int64_t start_position,
		int sample_rate);
	void render_stop();

// Start of current resample region in input sample rate
	int64_t source_start;
// Start of playback in output sample rate.
	int64_t dest_start;
// End of playback in output sample rate
	int64_t dest_end;
	int need_reconfigure;
	int prev_scale;
	ResampleRTResample *resample;
};


#endif

