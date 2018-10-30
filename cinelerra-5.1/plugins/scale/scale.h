
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

#ifndef SCALE_H
#define SCALE_H

// the simplest plugin possible

class ScaleMain;
class ScaleWidth;
class ScaleHeight;
class ScaleConstrain;
class ScaleThread;
class ScaleWin;

#include "bchash.h"
#include "guicast.h"
#include "mwindow.inc"
#include "mutex.h"
#include "new.h"
#include "scalewin.h"
#include "overlayframe.h"
#include "pluginvclient.h"

#define FIXED_SCALE 0
#define FIXED_SIZE 1

class ScaleConfig
{
public:
	ScaleConfig();

	void copy_from(ScaleConfig &src);
	int equivalent(ScaleConfig &src);
	void interpolate(ScaleConfig &prev,
		ScaleConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);

	int type;
	float x_factor, y_factor;
	int width, height;
	int constrain;
};


class ScaleXFactor : public BC_TumbleTextBox
{
public:
	ScaleXFactor(ScaleWin *win, ScaleMain *client, int x, int y);
	~ScaleXFactor();
	int handle_event();

	ScaleMain *client;
	ScaleWin *win;
	int enabled;
};

class ScaleWidth : public BC_TumbleTextBox
{
public:
	ScaleWidth(ScaleWin *win, ScaleMain *client, int x, int y);
	~ScaleWidth();
	int handle_event();

	ScaleMain *client;
	ScaleWin *win;
	int enabled;
};

class ScaleYFactor : public BC_TumbleTextBox
{
public:
	ScaleYFactor(ScaleWin *win, ScaleMain *client, int x, int y);
	~ScaleYFactor();
	int handle_event();

	ScaleMain *client;
	ScaleWin *win;
	int enabled;
};

class ScaleHeight : public BC_TumbleTextBox
{
public:
	ScaleHeight(ScaleWin *win, ScaleMain *client, int x, int y);
	~ScaleHeight();
	int handle_event();

	ScaleMain *client;
	ScaleWin *win;
	int enabled;
};

class ScaleUseScale : public BC_Radial
{
public:
	ScaleUseScale(ScaleWin *win, ScaleMain *client, int x, int y);
	~ScaleUseScale();
	int handle_event();

	ScaleWin *win;
	ScaleMain *client;
};

class ScaleUseSize : public BC_Radial
{
public:
	ScaleUseSize(ScaleWin *win, ScaleMain *client, int x, int y);
	~ScaleUseSize();
	int handle_event();

	ScaleWin *win;
	ScaleMain *client;
};

class ScaleConstrain : public BC_CheckBox
{
public:
	ScaleConstrain(ScaleMain *client, int x, int y);
	~ScaleConstrain();
	int handle_event();

	ScaleMain *client;
};

class ScaleWin : public PluginClientWindow
{
public:
	ScaleWin(ScaleMain *client);
	~ScaleWin();

	void create_objects();

	ScaleMain *client;
	ScaleXFactor *x_factor;
	ScaleYFactor *y_factor;
	ScaleWidth *width;
	ScaleHeight *height;
	FrameSizePulldown *pulldown;
	ScaleUseScale *use_scale;
	ScaleUseSize *use_size;
	ScaleConstrain *constrain;
};



class ScaleMain : public PluginVClient
{
public:
	ScaleMain(PluginServer *server);
	~ScaleMain();

// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS(ScaleConfig)
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	void calculate_transfer(VFrame *frame,
		float &in_x1,
		float &in_x2,
		float &in_y1,
		float &in_y2,
		float &out_x1,
		float &out_x2,
		float &out_y1,
		float &out_y2);
	int handle_opengl();
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void set_type(int type);


	PluginServer *server;
	OverlayFrame *overlayer;   // To scale images
};


#endif
