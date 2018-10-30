
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


class Fuse360Engine;
class Fuse360GUI;
class Fuse360Main;
class Fuse360Text;


class Fuse360Slider : public BC_FSlider
{
public:
	Fuse360Slider(Fuse360Main *client, 
		Fuse360GUI *gui,
		Fuse360Text *text,
		float *output, 
		int x, 
		int y, 
		float min,
		float max);
	int handle_event();

	Fuse360GUI *gui;
	Fuse360Main *client;
	Fuse360Text *text;
	float *output;
};

class Fuse360Text : public BC_TextBox
{
public:
	Fuse360Text(Fuse360Main *client, 
		Fuse360GUI *gui,
		Fuse360Slider *slider,
		float *output, 
		int x, 
		int y);
	int handle_event();

	Fuse360GUI *gui;
	Fuse360Main *client;
	Fuse360Slider *slider;
	float *output;
};


class Fuse360Toggle : public BC_CheckBox
{
public:
	Fuse360Toggle(Fuse360Main *client, 
		int *output, 
		int x, 
		int y,
		const char *text);
	int handle_event();

	Fuse360Main *client;
	int *output;
};


class Fuse360Mode : public BC_PopupMenu
{
public:
	Fuse360Mode(Fuse360Main *plugin,  
		Fuse360GUI *gui,
		int x,
		int y);
	int handle_event();
	void create_objects();
	static int calculate_w(Fuse360GUI *gui);
	static int from_text(char *text);
	static const char* to_text(int mode);
	void update(int mode);
	Fuse360Main *plugin;
	Fuse360GUI *gui;
};


class Fuse360GUI : public PluginClientWindow
{
public:
	Fuse360GUI(Fuse360Main *client);
	~Fuse360GUI();
	
	void create_objects();

	Fuse360Main *client;
//	Fuse360Slider *fov_slider;
//	Fuse360Text *fov_text;
	Fuse360Slider *radiusx_slider;
	Fuse360Text *radiusx_text;
	Fuse360Slider *radiusy_slider;
	Fuse360Text *radiusy_text;
//	Fuse360Slider *centerx_slider;
//	Fuse360Text *centerx_text;
//	Fuse360Slider *centery_slider;
//	Fuse360Text *centery_text;
	Fuse360Slider *distancex_slider;
	Fuse360Text *distancex_text;
	Fuse360Slider *distancey_slider;
	Fuse360Text *distancey_text;
	Fuse360Slider *translatex_slider;
	Fuse360Text *translatex_text;
	Fuse360Slider *feather_slider;
	Fuse360Text *feather_text;
	Fuse360Slider *rotation_slider;
	Fuse360Text *rotation_text;
	
	Fuse360Mode *mode;
	Fuse360Toggle *draw_guides;
};

class Fuse360Config
{
public:
	Fuse360Config();
	int equivalent(Fuse360Config &that);
	void copy_from(Fuse360Config &that);
	void interpolate(Fuse360Config &prev, 
		Fuse360Config &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void boundaries();



	float fov;
// radius of each eye
	float radius_x;
	float radius_y;
// amount to feather eye edges
	float feather;
// center of 2 eyes
	float center_x;
	float center_y;
// X distance between 2 eyes
	float distance_x;
// Y offset between 2 eyes
	float distance_y;
// translate output position
	float translate_x;
	float rotation;
	int draw_guides;
	int mode;
	enum
	{
// align guides only
		DO_NOTHING,
// rectilinear
		STRETCHXY,
// the standard blend
		STANDARD,
// don't stretch eyes
		BLEND
	};
};





class Fuse360Package : public LoadPackage
{
public:
	Fuse360Package();
	int row1, row2;
};


class Fuse360Unit : public LoadClient
{
public:
	Fuse360Unit(Fuse360Engine *engine, Fuse360Main *plugin);
	~Fuse360Unit();
	
	
	void process_package(LoadPackage *package);
	void process_stretch(Fuse360Package *pkg);
	void process_blend(Fuse360Package *pkg);
	void process_standard(Fuse360Package *pkg);
	double calculate_max_z(double a, double r);

	
	Fuse360Engine *engine;
	Fuse360Main *plugin;
};

class Fuse360Engine : public LoadServer
{
public:
	Fuse360Engine(Fuse360Main *plugin);
	~Fuse360Engine();
	
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	
	Fuse360Main *plugin;
};

class Fuse360Main : public PluginVClient
{
public:
	Fuse360Main(PluginServer *server);
	~Fuse360Main();

	PLUGIN_CLASS_MEMBERS2(Fuse360Config)
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void calculate_extents();
	double calculate_max_z(double a, int r);

	int w;
	int h;
	int center_x;
	int center_y;
	int center_x1;
	int center_y1;
	int center_x2;
	int center_y2;
	int radius_x;
	int radius_y;
	int distance_x;
	int distance_y;
	int feather;
	int feather_x1;
	int feather_x2;

	Fuse360Engine *engine;
	AffineEngine *affine;
};



#endif
