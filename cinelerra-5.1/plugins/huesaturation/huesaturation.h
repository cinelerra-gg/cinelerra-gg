
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

#ifndef HUESATURATION_H
#define HUESATURATION_H

#include "bccolors.h"
#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "loadbalance.h"
#include "bccolors.h"
#include "playback3d.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>


class HueEffect;
class HueWindow;
class HueReset;

#define MINHUE -180
#define MAXHUE 180
#define MINSATURATION -100
#define MAXSATURATION 100
#define MINVALUE -100
#define MAXVALUE 100






class HueConfig
{
public:
	HueConfig();

	void copy_from(HueConfig &src);
	int equivalent(HueConfig &src);
	void reset();
	void interpolate(HueConfig &prev,
		HueConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame);
	float hue, saturation, value;
};

class HueSlider : public BC_FSlider
{
public:
	HueSlider(HueEffect *plugin, int x, int y, int w);
	int handle_event();
	HueEffect *plugin;
};

class SaturationSlider : public BC_FSlider
{
public:
	SaturationSlider(HueEffect *plugin, int x, int y, int w);
	int handle_event();
	char* get_caption();
	HueEffect *plugin;
	char string[BCTEXTLEN];
};

class ValueSlider : public BC_FSlider
{
public:
	ValueSlider(HueEffect *plugin, int x, int y, int w);
	int handle_event();
	char* get_caption();
	HueEffect *plugin;
	char string[BCTEXTLEN];
};

class HueReset : public BC_GenericButton
{
public:
	HueReset(HueEffect *plugin, HueWindow *gui, int x, int y);
	~HueReset();
	int handle_event();
	HueEffect *plugin;
	HueWindow *gui;
};

class HueWindow : public PluginClientWindow
{
public:
	HueWindow(HueEffect *plugin);
	void create_objects();
	void update();
	HueEffect *plugin;
	HueSlider *hue;
	SaturationSlider *saturation;
	ValueSlider *value;
	HueReset *reset;
};


class HueEngine : public LoadServer
{
public:
	HueEngine(HueEffect *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	HueEffect *plugin;
};

class HuePackage : public LoadPackage
{
public:
	HuePackage();
	int row1, row2;
};

class HueUnit : public LoadClient
{
public:
	HueUnit(HueEffect *plugin, HueEngine *server);
	void process_package(LoadPackage *package);
	HueEffect *plugin;
};

class HueEffect : public PluginVClient
{
public:
	HueEffect(PluginServer *server);
	~HueEffect();


	PLUGIN_CLASS_MEMBERS(HueConfig);
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int handle_opengl();

	VFrame *input, *output;
	HueEngine *engine;
};

#endif

