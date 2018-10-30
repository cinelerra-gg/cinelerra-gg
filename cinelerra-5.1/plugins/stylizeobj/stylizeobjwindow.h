/*
 * CINELERRA
 * Copyright (C) 2008-2014 Adam Williams <broadcast at earthling dot net>
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

#ifndef STYLIZEOBJWINDOW_H
#define STYLIZEOBJWINDOW_H


#include "guicast.h"

class StylizeObj;
class StylizeObjWindow;
class StylizeObjModeItem;
class StylizeObjMode;
class StylizeObjFSlider;

class StylizeObjModeItem : public BC_MenuItem
{
public:
	StylizeObjModeItem(StylizeObjMode *popup, int type, const char *text);
	~StylizeObjModeItem();
	int handle_event();

	StylizeObjMode *popup;
	int type;
};

class StylizeObjMode : public BC_PopupMenu
{
public:
	StylizeObjMode(StylizeObjWindow *win, int x, int y, int *value);
	~StylizeObjMode();
	void create_objects();
	int handle_event();
	void update(int v);
	void set_value(int v);

	StylizeObjWindow *win;
	int *value;
};

class StylizeObjFSlider : public BC_FSlider
{
public:
	StylizeObjFSlider(StylizeObjWindow *win,
		int x, int y, int w, float min, float max, float *output);
	~StylizeObjFSlider();
	int handle_event();

	StylizeObjWindow *win;
	float *output;
};

class StylizeObjWindow : public PluginClientWindow
{
public:
	StylizeObjWindow(StylizeObj *plugin);
	~StylizeObjWindow();

	void create_objects();
	void update_params();
	int x0, y0;

	StylizeObj *plugin;
	StylizeObjMode *mode;
	BC_Title *smooth_title;
	BC_Title *edge_title;
	BC_Title *shade_title;
	StylizeObjFSlider *smoothing;
	StylizeObjFSlider *edge_strength;
	StylizeObjFSlider *shade_factor;
};

#endif
