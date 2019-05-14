
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

#ifndef SHIFTINTERLACE_H
#define SHIFTINTERLACE_H

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "pluginvclient.h"
#include "theme.h"
#include "vframe.h"



#include <stdint.h>
#include <string.h>


#define RESET_ALL 0
#define RESET_ODD_OFFSET  1
#define RESET_EVEN_OFFSET 2


class ShiftInterlaceWindow;
class ShiftInterlaceMain;

class ShiftInterlaceConfig
{
public:
	ShiftInterlaceConfig();

	void reset(int clear);
	int equivalent(ShiftInterlaceConfig &that);
	void copy_from(ShiftInterlaceConfig &that);
	void interpolate(ShiftInterlaceConfig &prev,
		ShiftInterlaceConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame);


	int odd_offset;
	int even_offset;
};


class ShiftInterlaceOdd : public BC_ISlider
{
public:
	ShiftInterlaceOdd(ShiftInterlaceMain *plugin, int x, int y);
	int handle_event();
	ShiftInterlaceMain *plugin;
};

class ShiftInterlaceEven : public BC_ISlider
{
public:
	ShiftInterlaceEven(ShiftInterlaceMain *plugin, int x, int y);
	int handle_event();
	ShiftInterlaceMain *plugin;
};

class ShiftInterlaceReset : public BC_GenericButton
{
public:
	ShiftInterlaceReset(ShiftInterlaceMain *plugin, ShiftInterlaceWindow *gui, int x, int y);
	~ShiftInterlaceReset();
	int handle_event();
	ShiftInterlaceMain *plugin;
	ShiftInterlaceWindow *gui;
};

class ShiftInterlaceSliderClr : public BC_Button
{
public:
	ShiftInterlaceSliderClr(ShiftInterlaceMain *plugin, ShiftInterlaceWindow *gui, int x, int y, int w, int clear);
	~ShiftInterlaceSliderClr();
	int handle_event();
	ShiftInterlaceMain *plugin;
	ShiftInterlaceWindow *gui;
	int clear;
};

class ShiftInterlaceWindow : public PluginClientWindow
{
public:
	ShiftInterlaceWindow(ShiftInterlaceMain *plugin);

	void create_objects();
	void update_gui(int clear);

	ShiftInterlaceOdd *odd_offset;
	ShiftInterlaceEven *even_offset;
	ShiftInterlaceMain *plugin;
	ShiftInterlaceReset *reset;
	ShiftInterlaceSliderClr *odd_offsetClr, *even_offsetClr;
};






class ShiftInterlaceMain : public PluginVClient
{
public:
	ShiftInterlaceMain(PluginServer *server);
	~ShiftInterlaceMain();

// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS(ShiftInterlaceConfig)
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);


	void shift_row(VFrame *input_frame,
		VFrame *output_frame,
		int offset,
		int row);


};

#endif
