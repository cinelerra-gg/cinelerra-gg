
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

#ifndef INVERT_H
#define INVERT_H

// the simplest plugin possible

class InvertMain;

#include "bcbase.h"
#include "invertwindow.h"
#include "pluginvclient.h"


class InvertMain : public PluginVClient
{
public:
	InvertMain();
	~InvertMain();

// required for all realtime plugins
	int process_realtime(long size, VFrame **input_ptr, VFrame **output_ptr);
	int plugin_is_realtime();
	int plugin_is_multi_channel();
	const char* plugin_title();
	PluginClientWindow* new_window();
	int start_realtime();
	int stop_realtime();
	int save_data(char *text);
	int read_data(char *text);

// parameters needed for invert
	int invert;


private:
// Utilities used by invert.
};


#endif
