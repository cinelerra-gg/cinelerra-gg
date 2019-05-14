
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

#ifndef ZOOMBLUR_H
#define ZOOMBLUR_H


#include "bcdisplayinfo.h"
#include "bcsignals.h"
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
#define RESET_RADIUS  3
#define RESET_STEPS   4

class ZoomBlurMain;
class ZoomBlurWindow;
class ZoomBlurEngine;
class ZoomBlurReset;
class ZoomBlurDefaultSettings;
class ZoomBlurSliderClr;



class ZoomBlurConfig
{
public:
	ZoomBlurConfig();

	void reset(int clear);
	int equivalent(ZoomBlurConfig &that);
	void copy_from(ZoomBlurConfig &that);
	void interpolate(ZoomBlurConfig &prev,
		ZoomBlurConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame);

	int x;
	int y;
	int radius;
	int steps;
	int r;
	int g;
	int b;
	int a;
};



class ZoomBlurSize : public BC_ISlider
{
public:
	ZoomBlurSize(ZoomBlurMain *plugin,
		int x,
		int y,
		int *output,
		int min,
		int max);
	int handle_event();
	ZoomBlurMain *plugin;
	int *output;
};

class ZoomBlurToggle : public BC_CheckBox
{
public:
	ZoomBlurToggle(ZoomBlurMain *plugin,
		int x,
		int y,
		int *output,
		char *string);
	int handle_event();
	ZoomBlurMain *plugin;
	int *output;
};

class ZoomBlurWindow : public PluginClientWindow
{
public:
	ZoomBlurWindow(ZoomBlurMain *plugin);
	~ZoomBlurWindow();

	void create_objects();
	void update_gui(int clear);

	ZoomBlurSize *x, *y, *radius, *steps;
	ZoomBlurToggle *r, *g, *b, *a;
	ZoomBlurMain *plugin;
	ZoomBlurReset *reset;
	ZoomBlurDefaultSettings *default_settings;
	ZoomBlurSliderClr *xClr, *yClr, *radiusClr, *stepsClr;
};

class ZoomBlurReset : public BC_GenericButton
{
public:
	ZoomBlurReset(ZoomBlurMain *plugin, ZoomBlurWindow *window, int x, int y);
	~ZoomBlurReset();
	int handle_event();
	ZoomBlurMain *plugin;
	ZoomBlurWindow *window;
};

class ZoomBlurDefaultSettings : public BC_GenericButton
{
public:
	ZoomBlurDefaultSettings(ZoomBlurMain *plugin, ZoomBlurWindow *window, int x, int y, int w);
	~ZoomBlurDefaultSettings();
	int handle_event();
	ZoomBlurMain *plugin;
	ZoomBlurWindow *window;
};


class ZoomBlurSliderClr : public BC_Button
{
public:
	ZoomBlurSliderClr(ZoomBlurMain *plugin, ZoomBlurWindow *window, int x, int y, int w, int clear);
	~ZoomBlurSliderClr();
	int handle_event();
	ZoomBlurMain *plugin;
	ZoomBlurWindow *window;
	int clear;
};



// Output coords for a layer of blurring
// Used for OpenGL only
class ZoomBlurLayer
{
public:
	ZoomBlurLayer() {};
	float x1, y1, x2, y2;
};

class ZoomBlurMain : public PluginVClient
{
public:
	ZoomBlurMain(PluginServer *server);
	~ZoomBlurMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int handle_opengl();

	PLUGIN_CLASS_MEMBERS(ZoomBlurConfig)

	void delete_tables();
	VFrame *input, *output, *temp;
	ZoomBlurEngine *engine;
	int **scale_y_table;
	int **scale_x_table;
	ZoomBlurLayer *layer_table;
	int table_entries;
	int need_reconfigure;
// The accumulation buffer is needed because 8 bits isn't precise enough
	unsigned char *accum;
};

class ZoomBlurPackage : public LoadPackage
{
public:
	ZoomBlurPackage();
	int y1, y2;
};

class ZoomBlurUnit : public LoadClient
{
public:
	ZoomBlurUnit(ZoomBlurEngine *server, ZoomBlurMain *plugin);
	void process_package(LoadPackage *package);
	ZoomBlurEngine *server;
	ZoomBlurMain *plugin;
};

class ZoomBlurEngine : public LoadServer
{
public:
	ZoomBlurEngine(ZoomBlurMain *plugin,
		int total_clients,
		int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	ZoomBlurMain *plugin;
};


#endif
