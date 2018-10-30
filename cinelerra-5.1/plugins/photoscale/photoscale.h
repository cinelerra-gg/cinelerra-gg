
/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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

#ifndef PHOTOSCALE_H
#define PHOTOSCALE_H

class PhotoScaleMain;
class PhotoScaleEngine;
class PhotoScaleWindow;

#include "bchash.inc"
#include "guicast.h"
#include "loadbalance.h"
#include "mutex.h"
#include "overlayframe.h"
#include "pluginvclient.h"
#include "thread.h"
#include "vframe.inc"




class PhotoScaleSizeText : public BC_TextBox
{
public:
	PhotoScaleSizeText(PhotoScaleMain *plugin,
		PhotoScaleWindow *gui,
		int x,
		int y,
		int w,
		int *output);
	~PhotoScaleSizeText();
	int handle_event();
	PhotoScaleMain *plugin;
	PhotoScaleWindow *gui;
	int *output;
};

class PhotoScaleFile : public BC_Radial
{
public:
	PhotoScaleFile(PhotoScaleMain *plugin,
		PhotoScaleWindow *gui,
		int x,
		int y);
	int handle_event();
	PhotoScaleMain *plugin;
	PhotoScaleWindow *gui;
};

class PhotoScaleScan : public BC_Radial
{
public:
	PhotoScaleScan(PhotoScaleMain *plugin,
		PhotoScaleWindow *gui,
		int x,
		int y);
	int handle_event();
	PhotoScaleMain *plugin;
	PhotoScaleWindow *gui;
};

class PhotoScaleWindow : public PluginClientWindow
{
public:
	PhotoScaleWindow(PhotoScaleMain *plugin);
	~PhotoScaleWindow();

	void create_objects();

	PhotoScaleMain *plugin;
// Output size w, h
	PhotoScaleSizeText *output_size[2];
	PhotoScaleFile *file;
	PhotoScaleScan *scan;
};





class PhotoScaleSwapExtents : public BC_Button
{
public:
	PhotoScaleSwapExtents(PhotoScaleMain *plugin,
		PhotoScaleWindow *gui,
		int x,
		int y);
	int handle_event();
	PhotoScaleMain *plugin;
	PhotoScaleWindow *gui;
};







class PhotoScaleConfig
{
public:
	PhotoScaleConfig();

	int equivalent(PhotoScaleConfig &that);
	void copy_from(PhotoScaleConfig &that);
	void interpolate(PhotoScaleConfig &prev,
		PhotoScaleConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);

// Output size
	int width, height;
	int use_file;
};







class PhotoScaleEngine : public LoadServer
{
public:
	PhotoScaleEngine(PhotoScaleMain *plugin, int cpus);
	~PhotoScaleEngine();

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	PhotoScaleMain *plugin;

	int top_border;
	int bottom_border;
	int left_border;
	int right_border;
};


class PhotoScalePackage : public LoadPackage
{
public:
	PhotoScalePackage();
	int side;
};



class PhotoScaleUnit : public LoadClient
{
public:
	PhotoScaleUnit(PhotoScaleEngine *server);
	~PhotoScaleUnit();
	void process_package(LoadPackage *package);
	PhotoScaleEngine *server;
};





class PhotoScaleMain : public PluginVClient
{
public:
	PhotoScaleMain(PluginServer *server);
	~PhotoScaleMain();

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	PLUGIN_CLASS_MEMBERS(PhotoScaleConfig)

	int need_reconfigure;

private:
	PhotoScaleEngine *engine;
	OverlayFrame *overlayer;
	VFrame *input_frame;
};



#endif
