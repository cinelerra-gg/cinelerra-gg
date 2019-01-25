
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

#ifndef RUMBLER_H
#define RUMBLER_H

#include "bcdisplayinfo.h"
#include "affine.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <math.h>
#include <string.h>
#include <stdint.h>


class Rumbler;
class RumblerConfig;
class RumblerRate;
class RumblerSeq;
class RumblerWindow;
class RumblerReset;


class RumblerConfig
{
public:
	RumblerConfig();
	void reset();
	float time_rumble, time_rate;
	float space_rumble, space_rate;
	int sequence;
	void copy_from(RumblerConfig &that);
	int equivalent(RumblerConfig &that);
	void interpolate(RumblerConfig &prev, RumblerConfig &next,
	        int64_t prev_frame, int64_t next_frame, int64_t current_frame);
};

class RumblerRate : public BC_TextBox
{
public:
	RumblerRate(Rumbler *plugin, RumblerWindow *gui,
		float &value, int x, int y);
	int handle_event();
	Rumbler *plugin;
	RumblerWindow *gui;
	float *value;
};

class RumblerSeq : public BC_TextBox
{
public:
	RumblerSeq(Rumbler *plugin, RumblerWindow *gui,
		int &value, int x, int y);
	int handle_event();
	Rumbler *plugin;
	RumblerWindow *gui;
	int *value;
};

class RumblerReset : public BC_GenericButton
{
public:
	RumblerReset(Rumbler *plugin, RumblerWindow *gui, int x, int y);
	~RumblerReset();
	int handle_event();
	Rumbler *plugin;
	RumblerWindow *gui;
};


class RumblerWindow : public PluginClientWindow
{
public:
	RumblerWindow(Rumbler *plugin);
	~RumblerWindow();
	void create_objects();
	void update();

	Rumbler *plugin;
	RumblerRate *time_rumble, *time_rate;
	RumblerRate *space_rumble, *space_rate;
	RumblerSeq *seq;
	RumblerReset *reset;
};

class Rumbler : public PluginVClient
{
public:
	Rumbler(PluginServer *server);
	~Rumbler();

	PLUGIN_CLASS_MEMBERS(RumblerConfig)

	int process_buffer(VFrame *frame, int64_t start_position, double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int handle_opengl();
	static double rumble(int64_t seq, double cur, double per, double amp);

	VFrame *input, *output, *temp;
	float x1,y1, x2,y2, x3,y3, x4,y4;
	AffineEngine *engine;
};

#endif
