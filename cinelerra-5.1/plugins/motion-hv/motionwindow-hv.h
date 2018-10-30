
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
#include "motion-hv.inc"

class MasterLayer : public BC_PopupMenu
{
public:
	MasterLayer(MotionHVMain *plugin, MotionHVWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionHVWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	MotionHVMain *plugin;
	MotionHVWindow *gui;
};

class ActionType : public BC_PopupMenu
{
public:
	ActionType(MotionHVMain *plugin, MotionHVWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionHVWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	MotionHVMain *plugin;
	MotionHVWindow *gui;
};

class TrackingType : public BC_PopupMenu
{
public:
	TrackingType(MotionHVMain *plugin, MotionHVWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionHVWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	MotionHVMain *plugin;
	MotionHVWindow *gui;
};

class TrackDirection : public BC_PopupMenu
{
public:
	TrackDirection(MotionHVMain *plugin, MotionHVWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionHVWindow *gui);
	static void from_text(int *horizontal_only, int *vertical_only, char *text);
	static char* to_text(int horizontal_only, int vertical_only);
	MotionHVMain *plugin;
	MotionHVWindow *gui;
};


class TrackSingleFrame : public BC_Radial
{
public:
	TrackSingleFrame(MotionHVMain *plugin,
		MotionHVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionHVMain *plugin;
	MotionHVWindow *gui;
};

class TrackFrameNumber : public BC_TextBox
{
public:
	TrackFrameNumber(MotionHVMain *plugin,
		MotionHVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionHVMain *plugin;
	MotionHVWindow *gui;
};

class TrackPreviousFrame : public BC_Radial
{
public:
	TrackPreviousFrame(MotionHVMain *plugin,
		MotionHVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionHVMain *plugin;
	MotionHVWindow *gui;
};

class PreviousFrameSameBlock : public BC_Radial
{
public:
	PreviousFrameSameBlock(MotionHVMain *plugin,
		MotionHVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionHVMain *plugin;
	MotionHVWindow *gui;
};

class GlobalRange : public BC_IPot
{
public:
	GlobalRange(MotionHVMain *plugin,
		int x,
		int y,
		int *value);
	int handle_event();
	MotionHVMain *plugin;
	int *value;
};

class RotationRange : public BC_IPot
{
public:
	RotationRange(MotionHVMain *plugin,
		int x,
		int y);
	int handle_event();
	MotionHVMain *plugin;
};

class RotationCenter : public BC_IPot
{
public:
	RotationCenter(MotionHVMain *plugin,
		int x,
		int y);
	int handle_event();
	MotionHVMain *plugin;
};

class BlockSize : public BC_IPot
{
public:
	BlockSize(MotionHVMain *plugin,
		int x,
		int y,
		int *value);
	int handle_event();
	MotionHVMain *plugin;
	int *value;
};

class MotionHVBlockX : public BC_FPot
{
public:
	MotionHVBlockX(MotionHVMain *plugin,
		MotionHVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionHVWindow *gui;
	MotionHVMain *plugin;
};

class MotionHVBlockY : public BC_FPot
{
public:
	MotionHVBlockY(MotionHVMain *plugin,
		MotionHVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionHVWindow *gui;
	MotionHVMain *plugin;
};

class MotionHVBlockXText : public BC_TextBox
{
public:
	MotionHVBlockXText(MotionHVMain *plugin,
		MotionHVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionHVWindow *gui;
	MotionHVMain *plugin;
};

class MotionHVBlockYText : public BC_TextBox
{
public:
	MotionHVBlockYText(MotionHVMain *plugin,
		MotionHVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionHVWindow *gui;
	MotionHVMain *plugin;
};

// class GlobalSearchPositions : public BC_PopupMenu
// {
// public:
// 	GlobalSearchPositions(MotionHVMain *plugin,
// 		int x,
// 		int y,
// 		int w);
// 	void create_objects();
// 	int handle_event();
// 	MotionHVMain *plugin;
// };
//
// class RotationSearchPositions : public BC_PopupMenu
// {
// public:
// 	RotationSearchPositions(MotionHVMain *plugin,
// 		int x,
// 		int y,
// 		int w);
// 	void create_objects();
// 	int handle_event();
// 	MotionHVMain *plugin;
// };

class MotionHVMagnitude : public BC_IPot
{
public:
	MotionHVMagnitude(MotionHVMain *plugin,
		int x,
		int y);
	int handle_event();
	MotionHVMain *plugin;
};

class MotionHVRMagnitude : public BC_IPot
{
public:
	MotionHVRMagnitude(MotionHVMain *plugin,
		int x,
		int y);
	int handle_event();
	MotionHVMain *plugin;
};

class MotionHVReturnSpeed : public BC_IPot
{
public:
	MotionHVReturnSpeed(MotionHVMain *plugin,
		int x,
		int y);
	int handle_event();
	MotionHVMain *plugin;
};


class MotionHVRReturnSpeed : public BC_IPot
{
public:
	MotionHVRReturnSpeed(MotionHVMain *plugin,
		int x,
		int y);
	int handle_event();
	MotionHVMain *plugin;
};


class MotionHVDrawVectors : public BC_CheckBox
{
public:
	MotionHVDrawVectors(MotionHVMain *plugin,
		MotionHVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionHVMain *plugin;
	MotionHVWindow *gui;
};

class AddTrackedFrameOffset : public BC_CheckBox
{
public:
	AddTrackedFrameOffset(MotionHVMain *plugin,
		MotionHVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionHVWindow *gui;
	MotionHVMain *plugin;
};

// class MotionHVGlobal : public BC_CheckBox
// {
// public:
// 	MotionHVGlobal(MotionHVMain *plugin,
// 		MotionHVWindow *gui,
// 		int x,
// 		int y);
// 	int handle_event();
// 	MotionHVWindow *gui;
// 	MotionHVMain *plugin;
// };

class MotionHVRotate : public BC_CheckBox
{
public:
	MotionHVRotate(MotionHVMain *plugin,
		MotionHVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionHVWindow *gui;
	MotionHVMain *plugin;
};



class MotionHVWindow : public PluginClientWindow
{
public:
	MotionHVWindow(MotionHVMain *plugin);
	~MotionHVWindow();

	void create_objects();
	void update_mode();
	char* get_radius_title();

	GlobalRange *global_range_w;
	GlobalRange *global_range_h;
	RotationRange *rotation_range;
	RotationCenter *rotation_center;
	BlockSize *global_block_w;
	BlockSize *global_block_h;
	BlockSize *rotation_block_w;
	BlockSize *rotation_block_h;
	MotionHVBlockX *block_x;
	MotionHVBlockY *block_y;
	MotionHVBlockXText *block_x_text;
	MotionHVBlockYText *block_y_text;
//	GlobalSearchPositions *global_search_positions;
//	RotationSearchPositions *rotation_search_positions;
	MotionHVMagnitude *magnitude;
	MotionHVRMagnitude *rotate_magnitude;
	MotionHVReturnSpeed *return_speed;
	MotionHVRReturnSpeed *rotate_return_speed;
	ActionType *action_type;
	MotionHVDrawVectors *vectors;
//	MotionHVGlobal *global;
	MotionHVRotate *rotate;
	AddTrackedFrameOffset *addtrackedframeoffset;
	TrackSingleFrame *track_single;
	TrackFrameNumber *track_frame_number;
	TrackPreviousFrame *track_previous;
	PreviousFrameSameBlock *previous_same;
	MasterLayer *master_layer;
	TrackingType *tracking_type;
	TrackDirection *track_direction;

	MotionHVMain *plugin;
};









