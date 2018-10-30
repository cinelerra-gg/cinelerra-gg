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

#ifndef FLOWOBJWINDOW_H
#define FLOWOBJWINDOW_H


#include "guicast.h"

class FlowObj;
class FlowObjWindow;


class FlowObjVectors : public BC_CheckBox
{
public:
	FlowObjVectors(FlowObjWindow *gui, int x, int y);
	int handle_event();
	FlowObjWindow *gui;
};

class FlowObjDoStabilization : public BC_CheckBox
{
public:
	FlowObjDoStabilization(FlowObjWindow *gui, int x, int y);
	int handle_event();
	FlowObjWindow *gui;
};

class FlowObjBlockSize : public BC_IPot
{
public:
	FlowObjBlockSize(FlowObjWindow *gui, int x, int y);
	int handle_event();
	FlowObjWindow *gui;
};

class FlowObjSearchRadius : public BC_IPot
{
public:
	FlowObjSearchRadius(FlowObjWindow *gui, int x, int y);
	int handle_event();
	FlowObjWindow *gui;
};

class FlowObjMaxMovement : public BC_IPot
{
public:
	FlowObjMaxMovement(FlowObjWindow *gui, int x, int y);
	int handle_event();
	FlowObjWindow *gui;
};

class FlowObjSettling : public BC_IPot
{
public:
	FlowObjSettling(FlowObjWindow *gui, int x, int y);
	int handle_event();
	FlowObjWindow *gui;
};

class FlowObjWindow : public PluginClientWindow
{
public:
	FlowObjWindow(FlowObj *plugin);
	~FlowObjWindow();

	void create_objects();

	FlowObj *plugin;
	FlowObjVectors *vectors;
	FlowObjDoStabilization *do_stabilization;
	FlowObjBlockSize *block_size;
	FlowObjSearchRadius *search_radius;
// not implemented
//	FlowObjMaxMovement *max_movement;
	FlowObjSettling *settling_speed;
};

#endif
