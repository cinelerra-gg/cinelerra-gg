
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

#ifndef RADIALBLUR_H
#define RADIALBLUR_H


#include <math.h>
#include <stdint.h>
#include <string.h>


#include "affine.h"
#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "theme.h"
#include "vframe.h"

#define RESET_DEFAULT_SETTINGS 10
#define RESET_ALL     0
#define RESET_XSLIDER 1
#define RESET_YSLIDER 2
#define RESET_ANGLE   3
#define RESET_STEPS   4

class RadialBlurMain;
class RadialBlurWindow;
class RadialBlurEngine;
class RadialBlurReset;
class RadialBlurDefaultSettings;
class RadialBlurSliderClr;



class RadialBlurConfig
{
public:
	RadialBlurConfig();

	void reset(int clear);
	int equivalent(RadialBlurConfig &that);
	void copy_from(RadialBlurConfig &that);
	void interpolate(RadialBlurConfig &prev,
		RadialBlurConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame);

	int x;
	int y;
	int steps;
	int angle;
	int r;
	int g;
	int b;
	int a;
};



class RadialBlurSize : public BC_ISlider
{
public:
	RadialBlurSize(RadialBlurMain *plugin,
		int x,
		int y,
		int *output,
		int min,
		int max);
	int handle_event();
	RadialBlurMain *plugin;
	int *output;
};

class RadialBlurToggle : public BC_CheckBox
{
public:
	RadialBlurToggle(RadialBlurMain *plugin,
		int x,
		int y,
		int *output,
		char *string);
	int handle_event();
	RadialBlurMain *plugin;
	int *output;
};

class RadialBlurReset : public BC_GenericButton
{
public:
	RadialBlurReset(RadialBlurMain *plugin, RadialBlurWindow *gui, int x, int y);
	~RadialBlurReset();
	int handle_event();
	RadialBlurMain *plugin;
	RadialBlurWindow *gui;
};

class RadialBlurDefaultSettings : public BC_GenericButton
{
public:
	RadialBlurDefaultSettings(RadialBlurMain *plugin, RadialBlurWindow *gui, int x, int y, int w);
	~RadialBlurDefaultSettings();
	int handle_event();
	RadialBlurMain *plugin;
	RadialBlurWindow *gui;
};

class RadialBlurSliderClr : public BC_Button
{
public:
	RadialBlurSliderClr(RadialBlurMain *plugin, RadialBlurWindow *gui, int x, int y, int w, int clear);
	~RadialBlurSliderClr();
	int handle_event();
	RadialBlurMain *plugin;
	RadialBlurWindow *gui;
	int clear;
};

class RadialBlurWindow : public PluginClientWindow
{
public:
	RadialBlurWindow(RadialBlurMain *plugin);
	~RadialBlurWindow();

	void create_objects();
	void update_gui(int clear);

	RadialBlurSize *x, *y, *steps, *angle;
	RadialBlurToggle *r, *g, *b, *a;
	RadialBlurMain *plugin;
	RadialBlurReset *reset;
	RadialBlurDefaultSettings *default_settings;
	RadialBlurSliderClr *xClr;
	RadialBlurSliderClr *yClr;
	RadialBlurSliderClr *angleClr;
	RadialBlurSliderClr *stepsClr;
};






class RadialBlurMain : public PluginVClient
{
public:
	RadialBlurMain(PluginServer *server);
	~RadialBlurMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int handle_opengl();

	PLUGIN_CLASS_MEMBERS(RadialBlurConfig)

	VFrame *input, *output, *temp;
	RadialBlurEngine *engine;
// Rotate engine only used for OpenGL
	AffineEngine *rotate;
};

class RadialBlurPackage : public LoadPackage
{
public:
	RadialBlurPackage();
	int y1, y2;
};

class RadialBlurUnit : public LoadClient
{
public:
	RadialBlurUnit(RadialBlurEngine *server, RadialBlurMain *plugin);
	void process_package(LoadPackage *package);
	RadialBlurEngine *server;
	RadialBlurMain *plugin;
};

class RadialBlurEngine : public LoadServer
{
public:
	RadialBlurEngine(RadialBlurMain *plugin,
		int total_clients,
		int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	RadialBlurMain *plugin;
};

#endif
