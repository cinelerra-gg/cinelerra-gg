
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

#ifndef _1080TO480_H
#define _1080TO480_H


#include "overlayframe.inc"
#include "pluginvclient.h"

class _1080to480Main;
class _1080to480Option;

class _1080to480Window : public PluginClientWindow
{
public:
	_1080to480Window(_1080to480Main *client);
	~_1080to480Window();

	void create_objects();
	int set_first_field(int first_field, int send_event);

	_1080to480Main *client;
	_1080to480Option *odd_first;
	_1080to480Option *even_first;
};


class _1080to480Option : public BC_Radial
{
public:
	_1080to480Option(_1080to480Main *client,
		_1080to480Window *window,
		int output,
		int x,
		int y,
		char *text);
	int handle_event();

	_1080to480Main *client;
	_1080to480Window *window;
	int output;
};

class _1080to480Config
{
public:
	_1080to480Config();

	int equivalent(_1080to480Config &that);
	void copy_from(_1080to480Config &that);
	void interpolate(_1080to480Config &prev,
		_1080to480Config &next,
		long prev_frame,
		long next_frame,
		long current_frame);

	int first_field;
};

class _1080to480Main : public PluginVClient
{
public:
	_1080to480Main(PluginServer *server);
	~_1080to480Main();
	PLUGIN_CLASS_MEMBERS(_1080to480Config)


// required for all realtime plugins
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	void reduce_field(VFrame *output, VFrame *input, int src_field, int dst_field);

	VFrame *temp;
};


#endif
