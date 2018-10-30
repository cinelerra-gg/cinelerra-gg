
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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

#ifndef GAIN_H
#define GAIN_H

class DCOffset;

#include "pluginaclient.h"

class DCOffsetConfig
{
public:
	DCOffsetConfig();
	int equivalent(DCOffsetConfig &that);
	void copy_from(DCOffsetConfig &that);
	void interpolate(DCOffsetConfig &prev,
		DCOffsetConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);

	double level;
};

class DCOffset : public PluginAClient
{
public:
	DCOffset(PluginServer *server);
	~DCOffset();

	int process_buffer(int64_t size,
		Samples *buffer,
		int64_t start_position,
		int sample_rate);

	const char* plugin_title();
	int is_realtime();

	int need_collection;
	Samples *reference;
	double offset;
};

#endif
