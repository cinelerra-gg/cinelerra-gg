
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

#ifndef LINEARBLUR_H
#define LINEARBLUR_H


#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "vframe.h"

#define RESET_DEFAULT_SETTINGS 10
#define RESET_ALL    0
#define RESET_RADIUS 1
#define RESET_ANGLE  2
#define RESET_STEPS  3

class LinearBlurMain;
class LinearBlurWindow;
class LinearBlurEngine;
class LinearBlurReset;
class LinearBlurDefaultSettings;
class LinearBlurSliderClr;



class LinearBlurConfig
{
public:
	LinearBlurConfig();

	void reset(int clear);
	int equivalent(LinearBlurConfig &that);
	void copy_from(LinearBlurConfig &that);
	void interpolate(LinearBlurConfig &prev,
		LinearBlurConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame);

	int radius;
	int steps;
	int angle;
	int r;
	int g;
	int b;
	int a;
};



class LinearBlurSize : public BC_ISlider
{
public:
	LinearBlurSize(LinearBlurMain *plugin,
		int x,
		int y,
		int *output,
		int min,
		int max);
	int handle_event();
	LinearBlurMain *plugin;
	int *output;
};

class LinearBlurToggle : public BC_CheckBox
{
public:
	LinearBlurToggle(LinearBlurMain *plugin,
		int x,
		int y,
		int *output,
		char *string);
	int handle_event();
	LinearBlurMain *plugin;
	int *output;
};

class LinearBlurReset : public BC_GenericButton
{
public:
	LinearBlurReset(LinearBlurMain *plugin, LinearBlurWindow *gui, int x, int y);
	~LinearBlurReset();
	int handle_event();
	LinearBlurMain *plugin;
	LinearBlurWindow *gui;
};

class LinearBlurDefaultSettings : public BC_GenericButton
{
public:
	LinearBlurDefaultSettings(LinearBlurMain *plugin, LinearBlurWindow *gui, int x, int y, int w);
	~LinearBlurDefaultSettings();
	int handle_event();
	LinearBlurMain *plugin;
	LinearBlurWindow *gui;
};

class LinearBlurSliderClr : public BC_Button
{
public:
	LinearBlurSliderClr(LinearBlurMain *plugin, LinearBlurWindow *gui, int x, int y, int w, int clear);
	~LinearBlurSliderClr();
	int handle_event();
	LinearBlurMain *plugin;
	LinearBlurWindow *gui;
	int clear;
};

class LinearBlurWindow : public PluginClientWindow
{
public:
	LinearBlurWindow(LinearBlurMain *plugin);
	~LinearBlurWindow();

	void create_objects();
	void update_gui(int clear);

	LinearBlurSize *angle, *steps, *radius;
	LinearBlurToggle *r, *g, *b, *a;
	LinearBlurMain *plugin;
	LinearBlurReset *reset;
	LinearBlurDefaultSettings *default_settings;
	LinearBlurSliderClr *radiusClr;
	LinearBlurSliderClr *angleClr;
	LinearBlurSliderClr *stepsClr;
};





// Output coords for a layer of blurring
// Used for OpenGL only
class LinearBlurLayer
{
public:
	LinearBlurLayer() {};
	int x, y;
};

class LinearBlurMain : public PluginVClient
{
public:
	LinearBlurMain(PluginServer *server);
	~LinearBlurMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int handle_opengl();

	PLUGIN_CLASS_MEMBERS(LinearBlurConfig)

	void delete_tables();
	VFrame *input, *output, *temp;
	LinearBlurEngine *engine;
	int **scale_y_table;
	int **scale_x_table;
	LinearBlurLayer *layer_table;
	int table_entries;
	int need_reconfigure;
// The accumulation buffer is needed because 8 bits isn't precise enough
	unsigned char *accum;
};

class LinearBlurPackage : public LoadPackage
{
public:
	LinearBlurPackage();
	int y1, y2;
};

class LinearBlurUnit : public LoadClient
{
public:
	LinearBlurUnit(LinearBlurEngine *server, LinearBlurMain *plugin);
	void process_package(LoadPackage *package);
	LinearBlurEngine *server;
	LinearBlurMain *plugin;
};

class LinearBlurEngine : public LoadServer
{
public:
	LinearBlurEngine(LinearBlurMain *plugin,
		int total_clients,
		int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	LinearBlurMain *plugin;
};

#endif
