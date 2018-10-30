
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

class MotionCVMain;
class MotionCVWindow;
class MotionCVScan;
class RotateCVScan;

class MasterLayer : public BC_PopupMenu
{
public:
	MasterLayer(MotionCVMain *plugin, MotionCVWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionCVWindow *gui);
	static int from_text(char *text);
	static const char* to_text(int mode);
	MotionCVMain *plugin;
	MotionCVWindow *gui;
};

class Mode1 : public BC_PopupMenu
{
public:
	Mode1(MotionCVMain *plugin, MotionCVWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionCVWindow *gui);
	static int from_text(char *text);
	static const char* to_text(int mode);
	MotionCVMain *plugin;
	MotionCVWindow *gui;
};

class Mode2 : public BC_PopupMenu
{
public:
	Mode2(MotionCVMain *plugin, MotionCVWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionCVWindow *gui);
	static int from_text(char *text);
	static const char* to_text(int mode);
	MotionCVMain *plugin;
	MotionCVWindow *gui;
};

class Mode3 : public BC_PopupMenu
{
public:
	Mode3(MotionCVMain *plugin, MotionCVWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionCVWindow *gui);
	static void from_text(int *horizontal_only, int *vertical_only, char *text);
	static const char* to_text(int horizontal_only, int vertical_only);
	MotionCVMain *plugin;
	MotionCVWindow *gui;
};


class TrackSingleFrame : public BC_Radial
{
public:
	TrackSingleFrame(MotionCVMain *plugin,
		MotionCVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionCVMain *plugin;
	MotionCVWindow *gui;
};

class TrackFrameNumber : public BC_TextBox
{
public:
	TrackFrameNumber(MotionCVMain *plugin,
		MotionCVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionCVMain *plugin;
	MotionCVWindow *gui;
};

class TrackPreviousFrame : public BC_Radial
{
public:
	TrackPreviousFrame(MotionCVMain *plugin,
		MotionCVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionCVMain *plugin;
	MotionCVWindow *gui;
};

class PreviousFrameSameBlock : public BC_Radial
{
public:
	PreviousFrameSameBlock(MotionCVMain *plugin,
		MotionCVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionCVMain *plugin;
	MotionCVWindow *gui;
};

class GlobalRange : public BC_IPot
{
public:
	GlobalRange(MotionCVMain *plugin,
		int x,
		int y,
		int *value);
	int handle_event();
	MotionCVMain *plugin;
	int *value;
};

class RotationRange : public BC_IPot
{
public:
	RotationRange(MotionCVMain *plugin,
		int x,
		int y);
	int handle_event();
	MotionCVMain *plugin;
};

class BlockSize : public BC_IPot
{
public:
	BlockSize(MotionCVMain *plugin,
		int x,
		int y,
		int *value);
	int handle_event();
	MotionCVMain *plugin;
	int *value;
};

class MotionCVBlockX : public BC_FPot
{
public:
	MotionCVBlockX(MotionCVMain *plugin,
		MotionCVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionCVWindow *gui;
	MotionCVMain *plugin;
};

class MotionCVBlockY : public BC_FPot
{
public:
	MotionCVBlockY(MotionCVMain *plugin,
		MotionCVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionCVWindow *gui;
	MotionCVMain *plugin;
};

class MotionCVBlockXText : public BC_TextBox
{
public:
	MotionCVBlockXText(MotionCVMain *plugin,
		MotionCVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionCVWindow *gui;
	MotionCVMain *plugin;
};

class MotionCVBlockYText : public BC_TextBox
{
public:
	MotionCVBlockYText(MotionCVMain *plugin,
		MotionCVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionCVWindow *gui;
	MotionCVMain *plugin;
};

class GlobalSearchPositions : public BC_PopupMenu
{
public:
	GlobalSearchPositions(MotionCVMain *plugin,
		int x,
		int y,
		int w);
	void create_objects();
	int handle_event();
	MotionCVMain *plugin;
};

class RotationSearchPositions : public BC_PopupMenu
{
public:
	RotationSearchPositions(MotionCVMain *plugin,
		int x,
		int y,
		int w);
	void create_objects();
	int handle_event();
	MotionCVMain *plugin;
};

class MotionCVMagnitude : public BC_IPot
{
public:
	MotionCVMagnitude(MotionCVMain *plugin,
		int x,
		int y);
	int handle_event();
	MotionCVMain *plugin;
};

class MotionCVReturnSpeed : public BC_IPot
{
public:
	MotionCVReturnSpeed(MotionCVMain *plugin,
		int x,
		int y);
	int handle_event();
	MotionCVMain *plugin;
};



class MotionCVDrawVectors : public BC_CheckBox
{
public:
	MotionCVDrawVectors(MotionCVMain *plugin,
		MotionCVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionCVMain *plugin;
	MotionCVWindow *gui;
};

class AddTrackedFrameOffset : public BC_CheckBox
{
public:
	AddTrackedFrameOffset(MotionCVMain *plugin,
		MotionCVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionCVWindow *gui;
	MotionCVMain *plugin;
};

class MotionCVTrackingFile : public BC_TextBox
{
public:
	MotionCVTrackingFile(MotionCVMain *plugin, const char *filename,
		MotionCVWindow *gui, int x, int y);
	int handle_event();
	MotionCVMain *plugin;
	MotionCVWindow *gui;
};

class MotionCVGlobal : public BC_CheckBox
{
public:
	MotionCVGlobal(MotionCVMain *plugin,
		MotionCVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionCVWindow *gui;
	MotionCVMain *plugin;
};

class MotionCVRotate : public BC_CheckBox
{
public:
	MotionCVRotate(MotionCVMain *plugin,
		MotionCVWindow *gui,
		int x,
		int y);
	int handle_event();
	MotionCVWindow *gui;
	MotionCVMain *plugin;
};



class MotionCVWindow : public PluginClientWindow
{
public:
	MotionCVWindow(MotionCVMain *plugin);
	~MotionCVWindow();

	void create_objects();
	void update_mode();
	char* get_radius_title();

	GlobalRange *global_range_w;
	GlobalRange *global_range_h;
	RotationRange *rotation_range;
	BlockSize *global_block_w;
	BlockSize *global_block_h;
	BlockSize *rotation_block_w;
	BlockSize *rotation_block_h;
	MotionCVBlockX *block_x;
	MotionCVBlockY *block_y;
	MotionCVBlockXText *block_x_text;
	MotionCVBlockYText *block_y_text;
	GlobalSearchPositions *global_search_positions;
	RotationSearchPositions *rotation_search_positions;
	MotionCVMagnitude *magnitude;
	MotionCVReturnSpeed *return_speed;
	Mode1 *mode1;
	MotionCVDrawVectors *vectors;
	MotionCVTrackingFile *tracking_file;
	MotionCVGlobal *global;
	MotionCVRotate *rotate;
	AddTrackedFrameOffset *addtrackedframeoffset;
	TrackSingleFrame *track_single;
	TrackFrameNumber *track_frame_number;
	TrackPreviousFrame *track_previous;
	PreviousFrameSameBlock *previous_same;
	MasterLayer *master_layer;
	Mode2 *mode2;
	Mode3 *mode3;
	BC_Title *pef_title;

	MotionCVMain *plugin;
};


