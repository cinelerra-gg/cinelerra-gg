
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

#ifndef FLIPWINDOW_H
#define FLIPWINDOW_H


class FlipThread;
class FlipWindow;

#include "filexml.inc"
#include "flip.h"
#include "mutex.h"
#include "pluginvclient.h"


class FlipToggle;

class FlipWindow : public PluginClientWindow
{
public:
	FlipWindow(FlipMain *client);
	~FlipWindow();

	void create_objects();

	FlipMain *client;
	FlipToggle *flip_vertical;
	FlipToggle *flip_horizontal;
};

class FlipToggle : public BC_CheckBox
{
public:
	FlipToggle(FlipMain *client, int *output, char *string, int x, int y);
	~FlipToggle();
	int handle_event();

	FlipMain *client;
	int *output;
};


#endif
