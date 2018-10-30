
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

#include "guicast.h"
#include "motion.inc"


class MotionGlobal : public BC_CheckBox
{
public:
	MotionGlobal(MotionMain2 *plugin,
		MotionWindow *gui,
		int x,
		int y,
		int *value,
		char *text);
	int handle_event();
	MotionWindow *gui;
	MotionMain2 *plugin;
	int *value;
};

class MasterLayer : public BC_PopupMenu
{
public:
	MasterLayer(MotionMain2 *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	MotionMain2 *plugin;
	MotionWindow *gui;
};

class Action : public BC_PopupMenu
{
public:
	Action(MotionMain2 *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	MotionMain2 *plugin;
	MotionWindow *gui;
};

class Calculation : public BC_PopupMenu
{
public:
	Calculation(MotionMain2 *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	MotionMain2 *plugin;
	MotionWindow *gui;
};

class TrackingDirection : public BC_PopupMenu
{
public:
	TrackingDirection(MotionMain2 *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionWindow *gui);
	static void from_text(int *horizontal_only, int *vertical_only, char *text);
	static char* to_text(int horizontal_only, int vertical_only);
	MotionMain2 *plugin;
	MotionWindow *gui;
};


class TrackSingleFrame : public BC_Radial
{
public:
	TrackSingleFrame(MotionMain2 *plugin,
		MotionWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionMain2 *plugin;
	MotionWindow *gui;
};

class TrackFrameNumber : public BC_TextBox
{
public:
	TrackFrameNumber(MotionMain2 *plugin,
		MotionWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionMain2 *plugin;
	MotionWindow *gui;
};

class TrackPreviousFrame : public BC_Radial
{
public:
	TrackPreviousFrame(MotionMain2 *plugin,
		MotionWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionMain2 *plugin;
	MotionWindow *gui;
};

class PreviousFrameSameBlock : public BC_Radial
{
public:
	PreviousFrameSameBlock(MotionMain2 *plugin,
		MotionWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionMain2 *plugin;
	MotionWindow *gui;
};

class MotionPot : public BC_IPot
{
public:
	MotionPot(MotionMain2 *plugin,
		int x,
		int y,
		int *value,
		int min,
		int max);
	int handle_event();
	MotionMain2 *plugin;
	int *value;
};

class MotionBlockX : public BC_FPot
{
public:
	MotionBlockX(MotionMain2 *plugin,
		MotionWindow *gui,
		int x,
		int y,
		int number);
	int handle_event();
	MotionWindow *gui;
	MotionMain2 *plugin;
	int number;
};

class MotionBlockY : public BC_FPot
{
public:
	MotionBlockY(MotionMain2 *plugin,
		MotionWindow *gui,
		int x,
		int y,
		int number);
	int handle_event();
	MotionWindow *gui;
	MotionMain2 *plugin;
	int number;
};

class MotionBlockXText : public BC_TextBox
{
public:
	MotionBlockXText(MotionMain2 *plugin,
		MotionWindow *gui,
		int x,
		int y,
		int number);
	int handle_event();
	MotionWindow *gui;
	MotionMain2 *plugin;
	int number;
};

class MotionBlockYText : public BC_TextBox
{
public:
	MotionBlockYText(MotionMain2 *plugin,
		MotionWindow *gui,
		int x,
		int y,
		int number);
	int handle_event();
	MotionWindow *gui;
	MotionMain2 *plugin;
	int number;
};

class GlobalSearchPositions : public BC_PopupMenu
{
public:
	GlobalSearchPositions(MotionMain2 *plugin,
		int x,
		int y,
		int w);
	void create_objects();
	int handle_event();
	MotionMain2 *plugin;
};



class MotionMagnitude : public BC_IPot
{
public:
	MotionMagnitude(MotionMain2 *plugin,
		int x,
		int y);
	int handle_event();
	MotionMain2 *plugin;
};

class MotionReturnSpeed : public BC_IPot
{
public:
	MotionReturnSpeed(MotionMain2 *plugin,
		int x,
		int y);
	int handle_event();
	MotionMain2 *plugin;
};



class MotionDrawVectors : public BC_CheckBox
{
public:
	MotionDrawVectors(MotionMain2 *plugin,
		MotionWindow *gui,
		int x,
		int y,
		int number);
	int handle_event();
	MotionMain2 *plugin;
	MotionWindow *gui;
	int number;
};




class MotionWindow : public PluginClientWindow
{
public:
	MotionWindow(MotionMain2 *plugin);
	~MotionWindow();

	void create_objects();
	void update_mode();
	char* get_radius_title();

	MotionGlobal *global[TOTAL_POINTS];
	MotionPot *global_range_w[TOTAL_POINTS];
	MotionPot *global_range_h[TOTAL_POINTS];
	MotionPot *global_block_w[TOTAL_POINTS];
	MotionPot *global_block_h[TOTAL_POINTS];
	MotionPot *global_origin_x[TOTAL_POINTS];
	MotionPot *global_origin_y[TOTAL_POINTS];
	MotionBlockX *block_x[TOTAL_POINTS];
	MotionBlockY *block_y[TOTAL_POINTS];
	MotionBlockXText *block_x_text[TOTAL_POINTS];
	MotionBlockYText *block_y_text[TOTAL_POINTS];
	MotionDrawVectors *vectors[TOTAL_POINTS];

	GlobalSearchPositions *global_search_positions;
	MotionMagnitude *magnitude;
	MotionReturnSpeed *return_speed;
	TrackSingleFrame *track_single;
	TrackFrameNumber *track_frame_number;
	TrackPreviousFrame *track_previous;
	PreviousFrameSameBlock *previous_same;
	MasterLayer *master_layer;
	Action *action;
	Calculation *calculation;
	TrackingDirection *tracking_direction;

	MotionMain2 *plugin;
};








