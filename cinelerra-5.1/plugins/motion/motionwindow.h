
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

class MasterLayer : public BC_PopupMenu
{
public:
	MasterLayer(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	MotionMain *plugin;
	MotionWindow *gui;
};

class ActionType : public BC_PopupMenu
{
public:
	ActionType(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	MotionMain *plugin;
	MotionWindow *gui;
};

class TrackingType : public BC_PopupMenu
{
public:
	TrackingType(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	MotionMain *plugin;
	MotionWindow *gui;
};

class TrackDirection : public BC_PopupMenu
{
public:
	TrackDirection(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionWindow *gui);
	static void from_text(int *horizontal_only, int *vertical_only, char *text);
	static char* to_text(int horizontal_only, int vertical_only);
	MotionMain *plugin;
	MotionWindow *gui;
};


class TrackSingleFrame : public BC_Radial
{
public:
	TrackSingleFrame(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	MotionMain *plugin;
	MotionWindow *gui;
};

class TrackFrameNumber : public BC_TextBox
{
public:
	TrackFrameNumber(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	MotionMain *plugin;
	MotionWindow *gui;
};

class TrackPreviousFrame : public BC_Radial
{
public:
	TrackPreviousFrame(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	MotionMain *plugin;
	MotionWindow *gui;
};

class PreviousFrameSameBlock : public BC_Radial
{
public:
	PreviousFrameSameBlock(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	MotionMain *plugin;
	MotionWindow *gui;
};

class GlobalRange : public BC_IPot
{
public:
	GlobalRange(MotionMain *plugin, int x, int y, int *value);
	int handle_event();
	MotionMain *plugin;
	int *value;
};

class RotationRange : public BC_IPot
{
public:
	RotationRange(MotionMain *plugin, int x, int y);
	int handle_event();
	MotionMain *plugin;
};

class RotationCenter : public BC_IPot
{
public:
	RotationCenter(MotionMain *plugin, int x, int y);
	int handle_event();
	MotionMain *plugin;
};

class BlockSize : public BC_IPot
{
public:
	BlockSize(MotionMain *plugin, int x, int y, int *value);
	int handle_event();
	MotionMain *plugin;
	int *value;
};

class MotionBlockX : public BC_FPot
{
public:
	MotionBlockX(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};

class MotionBlockY : public BC_FPot
{
public:
	MotionBlockY(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};

class MotionBlockXText : public BC_TextBox
{
public:
	MotionBlockXText(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};

class MotionBlockYText : public BC_TextBox
{
public:
	MotionBlockYText(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};

class GlobalSearchPositions : public BC_PopupMenu
{
public:
	GlobalSearchPositions(MotionMain *plugin, int x, int y, int w);
	void create_objects();
	int handle_event();
	MotionMain *plugin;
};

class RotationSearchPositions : public BC_PopupMenu
{
public:
	RotationSearchPositions(MotionMain *plugin, int x, int y, int w);
	void create_objects();
	int handle_event();
	MotionMain *plugin;
};

class MotionMagnitude : public BC_IPot
{
public:
	MotionMagnitude(MotionMain *plugin, int x, int y);
	int handle_event();
	MotionMain *plugin;
};

class MotionRMagnitude : public BC_IPot
{
public:
	MotionRMagnitude(MotionMain *plugin, int x, int y);
	int handle_event();
	MotionMain *plugin;
};

class MotionReturnSpeed : public BC_IPot
{
public:
	MotionReturnSpeed(MotionMain *plugin, int x, int y);
	int handle_event();
	MotionMain *plugin;
};


class MotionRReturnSpeed : public BC_IPot
{
public:
	MotionRReturnSpeed(MotionMain *plugin, int x, int y);
	int handle_event();
	MotionMain *plugin;
};


class MotionDrawVectors : public BC_CheckBox
{
public:
	MotionDrawVectors(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	MotionMain *plugin;
	MotionWindow *gui;
};

class AddTrackedFrameOffset : public BC_CheckBox
{
public:
	AddTrackedFrameOffset(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};

class MotionTrackingFile : public BC_TextBox
{
public:
	MotionTrackingFile(MotionMain *plugin, const char *filename,
		MotionWindow *gui, int x, int y);
	int handle_event();
	MotionMain *plugin;
	MotionWindow *gui;
};

class MotionGlobal : public BC_CheckBox
{
public:
	MotionGlobal(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};

class MotionRotate : public BC_CheckBox
{
public:
	MotionRotate(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};



class MotionWindow : public PluginClientWindow
{
public:
	MotionWindow(MotionMain *plugin);
	~MotionWindow();

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
	MotionBlockX *block_x;
	MotionBlockY *block_y;
	MotionBlockXText *block_x_text;
	MotionBlockYText *block_y_text;
	GlobalSearchPositions *global_search_positions;
	RotationSearchPositions *rotation_search_positions;
	MotionMagnitude *magnitude;
	MotionRMagnitude *rotate_magnitude;
	MotionReturnSpeed *return_speed;
	MotionRReturnSpeed *rotate_return_speed;
	ActionType *action_type;
	MotionDrawVectors *vectors;
	MotionTrackingFile *tracking_file;
	MotionGlobal *global;
	MotionRotate *rotate;
	AddTrackedFrameOffset *addtrackedframeoffset;
	TrackSingleFrame *track_single;
	TrackFrameNumber *track_frame_number;
	TrackPreviousFrame *track_previous;
	PreviousFrameSameBlock *previous_same;
	MasterLayer *master_layer;
	TrackingType *tracking_type;
	TrackDirection *track_direction;
	BC_Title *pef_title;

	MotionMain *plugin;
};


