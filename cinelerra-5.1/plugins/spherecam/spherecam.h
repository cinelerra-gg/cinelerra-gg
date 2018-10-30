
/*
 * CINELERRA
 * Copyright (C) 2017 Adam Williams <broadcast at earthling dot net>
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

#ifndef FUSE360_H
#define FUSE360_H


#include "affine.inc"
#include "bchash.inc"
#include "guicast.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "thread.h"


class SphereCamEngine;
class SphereCamGUI;
class SphereCamMain;
class SphereCamText;

#define EYES 2

class SphereCamSlider : public BC_FSlider
{
public:
	SphereCamSlider(SphereCamMain *client, 
		SphereCamGUI *gui,
		SphereCamText *text,
		float *output, 
		int x, 
		int y, 
		float min,
		float max);
	int handle_event();

	SphereCamGUI *gui;
	SphereCamMain *client;
	SphereCamText *text;
	float *output;
};

class SphereCamText : public BC_TextBox
{
public:
	SphereCamText(SphereCamMain *client, 
		SphereCamGUI *gui,
		SphereCamSlider *slider,
		float *output, 
		int x, 
		int y);
	int handle_event();

	SphereCamGUI *gui;
	SphereCamMain *client;
	SphereCamSlider *slider;
	float *output;
};


class SphereCamToggle : public BC_CheckBox
{
public:
	SphereCamToggle(SphereCamMain *client, 
		int *output, 
		int x, 
		int y,
		const char *text);
	int handle_event();

	SphereCamMain *client;
	int *output;
};


class SphereCamMode : public BC_PopupMenu
{
public:
	SphereCamMode(SphereCamMain *plugin,  
		SphereCamGUI *gui,
		int x,
		int y);
	int handle_event();
	void create_objects();
	static int calculate_w(SphereCamGUI *gui);
	static int from_text(char *text);
	static const char* to_text(int mode);
	void update(int mode);
	SphereCamMain *plugin;
	SphereCamGUI *gui;
};


class SphereCamGUI : public PluginClientWindow
{
public:
	SphereCamGUI(SphereCamMain *client);
	~SphereCamGUI();
	
	void create_objects();

	SphereCamMain *client;
	SphereCamToggle *enabled[EYES];
	SphereCamSlider *fov_slider[EYES];
	SphereCamText *fov_text[EYES];
	SphereCamSlider *radius_slider[EYES];
	SphereCamText *radius_text[EYES];
	SphereCamSlider *centerx_slider[EYES];
	SphereCamText *centerx_text[EYES];
	SphereCamSlider *centery_slider[EYES];
	SphereCamText *centery_text[EYES];
	SphereCamSlider *rotatex_slider[EYES];
	SphereCamText *rotatex_text[EYES];
	SphereCamSlider *rotatey_slider[EYES];
	SphereCamText *rotatey_text[EYES];
	SphereCamSlider *rotatez_slider[EYES];
	SphereCamText *rotatez_text[EYES];
	SphereCamSlider *feather_slider;
	SphereCamText *feather_text;
	
	SphereCamMode *mode;
	SphereCamToggle *draw_guides;
};

class SphereCamConfig
{
public:
	SphereCamConfig();
	int equivalent(SphereCamConfig &that);
	void copy_from(SphereCamConfig &that);
	void interpolate(SphereCamConfig &prev, 
		SphereCamConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void boundaries();


	int enabled[EYES];
// degrees 1-359
	float fov[EYES];
// radius of each eye 1-150
	float radius[EYES];
// center of 2 eyes  0-100
	float center_x[EYES];
	float center_y[EYES];
// rotation  0-359
	float rotate_x[EYES];
	float rotate_y[EYES];
	float rotate_z[EYES];
// amount to feather eye edges 0-50
	float feather;
	int draw_guides;
	int mode;
	enum
	{
// draw guides only
		DO_NOTHING,
// the standard algorithm
		EQUIRECT,
// alignment only
		ALIGN,
	};
};





class SphereCamPackage : public LoadPackage
{
public:
	SphereCamPackage();
	int row1, row2;
};


class SphereCamUnit : public LoadClient
{
public:
	SphereCamUnit(SphereCamEngine *engine, SphereCamMain *plugin);
	~SphereCamUnit();
	
	
	void process_package(LoadPackage *package);
	void process_equirect(SphereCamPackage *pkg);
	void process_align(SphereCamPackage *pkg);
	double calculate_max_z(double a, double r);

	
	SphereCamEngine *engine;
	SphereCamMain *plugin;
};

class SphereCamEngine : public LoadServer
{
public:
	SphereCamEngine(SphereCamMain *plugin);
	~SphereCamEngine();
	
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	
	SphereCamMain *plugin;
};

class SphereCamMain : public PluginVClient
{
public:
	SphereCamMain(PluginServer *server);
	~SphereCamMain();

	PLUGIN_CLASS_MEMBERS2(SphereCamConfig)
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	void draw_feather(int left, int right, int eye, int y);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void calculate_extents();
	double calculate_max_z(double a, int r);

// config values converted to pixels
	int w;
	int h;

// the center of the output regions, shifted by the rotations
	int output_x[EYES];
	int output_y[EYES];
// The output of each eye is split into 2 halves so it can wrap around
	int out_x1[EYES];
	int out_y1[EYES];
	int out_x2[EYES];
	int out_y2[EYES];
	int out_x3[EYES];
	int out_y3[EYES];
	int out_x4[EYES];
	int out_y4[EYES];
// the center of the input regions
	int input_x[EYES];
	int input_y[EYES];
// radius of the input regions for drawing the guide only
	int radius[EYES];
// pixels to add to the output regions
	int feather;

	SphereCamEngine *engine;
	AffineEngine *affine;
};



#endif
