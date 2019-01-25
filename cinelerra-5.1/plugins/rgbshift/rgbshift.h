
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

#ifndef RGBSHIFT_H
#define RGBSHIFT_H


#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "bccolors.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>


class RGBShiftEffect;
class RGBShiftWindow;
class RGBShiftReset;


class RGBShiftConfig
{
public:
	RGBShiftConfig();

	void reset();
	void copy_from(RGBShiftConfig &src);
	int equivalent(RGBShiftConfig &src);
	void interpolate(RGBShiftConfig &prev,
		RGBShiftConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame);

	int r_dx, r_dy, g_dx, g_dy, b_dx, b_dy;
};

class RGBShiftLevel : public BC_ISlider
{
public:
	RGBShiftLevel(RGBShiftEffect *plugin, int *output, int x, int y);
	int handle_event();
	RGBShiftEffect *plugin;
	int *output;
};

class RGBShiftReset : public BC_GenericButton
{
public:
	RGBShiftReset(RGBShiftEffect *plugin, RGBShiftWindow *window, int x, int y);
	~RGBShiftReset();
	int handle_event();
	RGBShiftEffect *plugin;
	RGBShiftWindow *window;
};

class RGBShiftWindow : public PluginClientWindow
{
public:
	RGBShiftWindow(RGBShiftEffect *plugin);
	void create_objects();
	void update();
	RGBShiftLevel *r_dx, *r_dy, *g_dx, *g_dy, *b_dx, *b_dy;
	RGBShiftEffect *plugin;
	RGBShiftReset *reset;
};



class RGBShiftEffect : public PluginVClient
{
	VFrame *temp_frame;
public:
	RGBShiftEffect(PluginServer *server);
	~RGBShiftEffect();


	PLUGIN_CLASS_MEMBERS(RGBShiftConfig)
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
};

#endif
