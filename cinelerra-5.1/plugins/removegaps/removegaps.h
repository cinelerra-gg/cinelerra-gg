
/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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

#ifndef REMOVEGAPS_H
#define REMOVEGAPS_H


#include "guicast.h"
#include "pluginaclient.h"
#include "resample.h"



class RemoveGaps;
class RemoveGapsWindow;

class RemoveGapsConfig
{
public:
	RemoveGapsConfig();
	void boundaries();
	int equivalent(RemoveGapsConfig &src);
	void copy_from(RemoveGapsConfig &src);
	void interpolate(RemoveGapsConfig &prev,
		RemoveGapsConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
// Threshold to be considered a gap in DB
	double threshold;
// Duration below threshold required to remove a gap in seconds
	double duration;
};


class RemoveGapsThreshold : public BC_FPot
{
public:
	RemoveGapsThreshold(RemoveGapsWindow *window,
		RemoveGaps *plugin,
		int x,
		int y);
	int handle_event();
	RemoveGaps *plugin;
};

class RemoveGapsDuration : public BC_FPot
{
public:
	RemoveGapsDuration(RemoveGapsWindow *window,
		RemoveGaps *plugin,
		int x,
		int y);
	int handle_event();
	RemoveGaps *plugin;
};

class RemoveGapsWindow : public PluginClientWindow
{
public:
	RemoveGapsWindow(RemoveGaps *plugin);
	~RemoveGapsWindow();
	void create_objects();

	RemoveGaps *plugin;
	RemoveGapsThreshold *threshold;
	RemoveGapsDuration *duration;
};



class RemoveGaps : public PluginAClient
{
public:
	RemoveGaps(PluginServer *server);
	~RemoveGaps();

	PLUGIN_CLASS_MEMBERS2(RemoveGapsConfig)

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_realtime();
	int process_buffer(int64_t size,
		Samples *buffer,
		int64_t start_position,
		int sample_rate);
	void render_stop();

	Samples *temp;
// Position in temp buffer of input
	int temp_position;

// Position being read
	int64_t source_start;
// Position being written, to detect a seek operation
	int64_t dest_start;
// Current samples below threshold
	int64_t gap_length;
	int need_reconfigure;
};


#endif

