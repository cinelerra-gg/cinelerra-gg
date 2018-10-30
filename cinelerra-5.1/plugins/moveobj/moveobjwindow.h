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

#ifndef MOVEOBJWINDOW_H
#define MOVEOBJWINDOW_H


#include "guicast.h"

class MoveObj;
class MoveObjWindow;


class MoveObjVectors : public BC_CheckBox
{
public:
	MoveObjVectors(MoveObjWindow *gui, int x, int y);
	int handle_event();
	MoveObjWindow *gui;
};

class MoveObjDoStabilization : public BC_CheckBox
{
public:
	MoveObjDoStabilization(MoveObjWindow *gui, int x, int y);
	int handle_event();
	MoveObjWindow *gui;
};

class MoveObjBlockSize : public BC_IPot
{
public:
	MoveObjBlockSize(MoveObjWindow *gui, int x, int y);
	int handle_event();
	MoveObjWindow *gui;
};

class MoveObjSearchRadius : public BC_IPot
{
public:
	MoveObjSearchRadius(MoveObjWindow *gui, int x, int y);
	int handle_event();
	MoveObjWindow *gui;
};

class MoveObjMaxMovement : public BC_IPot
{
public:
	MoveObjMaxMovement(MoveObjWindow *gui, int x, int y);
	int handle_event();
	MoveObjWindow *gui;
};

class MoveObjSettling : public BC_IPot
{
public:
	MoveObjSettling(MoveObjWindow *gui, int x, int y);
	int handle_event();
	MoveObjWindow *gui;
};

class MoveObjWindow : public PluginClientWindow
{
public:
	MoveObjWindow(MoveObj *plugin);
	~MoveObjWindow();

	void create_objects();

	MoveObj *plugin;
	MoveObjVectors *vectors;
	MoveObjDoStabilization *do_stabilization;
	MoveObjBlockSize *block_size;
	MoveObjSearchRadius *search_radius;
// not implemented
//	MoveObjMaxMovement *max_movement;
	MoveObjSettling *settling_speed;
};

#endif
