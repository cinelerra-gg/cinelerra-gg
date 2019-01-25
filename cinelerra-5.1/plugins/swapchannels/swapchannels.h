
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

#ifndef SWAPCHANNELS_H
#define SWAPCHANNELS_H

#include "bchash.inc"
#include "guicast.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "vframe.inc"





class SwapMain;
class SwapWindow;
class SwapReset;

#define RED_SRC 0
#define GREEN_SRC 1
#define BLUE_SRC 2
#define ALPHA_SRC 3
#define NO_SRC 4
#define MAX_SRC 5



class SwapConfig
{
public:
	SwapConfig();

	void reset_Config();
	int equivalent(SwapConfig &that);
	void copy_from(SwapConfig &that);

	int red;
	int green;
	int blue;
	int alpha;
};



class SwapMenu : public BC_PopupMenu
{
public:
	SwapMenu(SwapMain *client, int *output, int x, int y);


	int handle_event();
	void create_objects();

	SwapMain *client;
	int *output;
};


class SwapItem : public BC_MenuItem
{
public:
	SwapItem(SwapMenu *menu, const char *title);

	int handle_event();

	SwapMenu *menu;
	char *title;
};


class SwapReset : public BC_GenericButton
{
public:
	SwapReset(SwapMain *plugin, SwapWindow *gui, int x, int y);
	~SwapReset();
	int handle_event();
	SwapMain *plugin;
	SwapWindow *gui;
};


class SwapWindow : public PluginClientWindow
{
public:
	SwapWindow(SwapMain *plugin);
	~SwapWindow();

	void create_objects();

	SwapMain *plugin;
	SwapMenu *red;
	SwapMenu *green;
	SwapMenu *blue;
	SwapMenu *alpha;
	SwapReset *reset;
};







class SwapMain : public PluginVClient
{
public:
	SwapMain(PluginServer *server);
	~SwapMain();

	PLUGIN_CLASS_MEMBERS(SwapConfig)
// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int is_synthesis();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int handle_opengl();







	void reset();




// parameters needed for processor
	const char* output_to_text(int value);
	int text_to_output(const char *text);

	VFrame *temp;
};


#endif
