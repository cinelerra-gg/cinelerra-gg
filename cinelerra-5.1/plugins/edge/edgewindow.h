/*
 * CINELERRA
 * Copyright (C) 2008-2015 Adam Williams <broadcast at earthling dot net>
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

#ifndef EDGEWINDOW_H
#define EDGEWINDOW_H


#include "guicast.h"

class Edge;
class EdgeWindow;


class EdgeAmount : public BC_ISlider
{
public:
	EdgeAmount(EdgeWindow *gui,
		int x,
		int y,
		int w);
	int handle_event();
	EdgeWindow *gui;
};

class EdgeWindow : public PluginClientWindow
{
public:
	EdgeWindow(Edge *plugin);
	~EdgeWindow();

	void create_objects();

	Edge *plugin;
	EdgeAmount *amount;
};



#endif

