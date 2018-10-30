/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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
 * *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef INTERPOLATEWINDOW_H
#define INTERPOLATEWINDOW_H

#include "guicast.h"
#include "interpolatevideo.inc"
#include "interpolatewindow.inc"
#include "pluginvclient.h"



class InterpolateVideoRate : public BC_TextBox
{
public:
	InterpolateVideoRate(InterpolateVideo *plugin,
		InterpolateVideoWindow *gui,
		int x,
		int y);
	int handle_event();
	InterpolateVideo *plugin;
	InterpolateVideoWindow *gui;
};

class InterpolateVideoRateMenu : public BC_ListBox
{
public:
	InterpolateVideoRateMenu(InterpolateVideo *plugin,
		InterpolateVideoWindow *gui,
		int x,
		int y);
	int handle_event();
	InterpolateVideo *plugin;
	InterpolateVideoWindow *gui;
};

class InterpolateVideoFlow : public BC_CheckBox
{
public:
	InterpolateVideoFlow(InterpolateVideo *plugin,
		InterpolateVideoWindow *gui,
		int x,
		int y);
	int handle_event();
	InterpolateVideoWindow *gui;
	InterpolateVideo *plugin;
};

class InterpolateVideoVectors : public BC_CheckBox
{
public:
	InterpolateVideoVectors(InterpolateVideo *plugin,
		InterpolateVideoWindow *gui,
		int x,
		int y);
	int handle_event();
	InterpolateVideoWindow *gui;
	InterpolateVideo *plugin;
};

class InterpolateVideoRadius : public BC_IPot
{
public:
	InterpolateVideoRadius(InterpolateVideo *plugin,
		InterpolateVideoWindow *gui,
		int x,
		int y);
	int handle_event();
	InterpolateVideoWindow *gui;
	InterpolateVideo *plugin;
};

class InterpolateVideoSize : public BC_IPot
{
public:
	InterpolateVideoSize(InterpolateVideo *plugin,
		InterpolateVideoWindow *gui,
		int x,
		int y);
	int handle_event();
	InterpolateVideoWindow *gui;
	InterpolateVideo *plugin;
};

class InterpolateVideoKeyframes : public BC_CheckBox
{
public:
	InterpolateVideoKeyframes(InterpolateVideo *plugin,
		InterpolateVideoWindow *gui,
		int x,
		int y);
	int handle_event();
	InterpolateVideoWindow *gui;
	InterpolateVideo *plugin;
};

class InterpolateVideoWindow : public PluginClientWindow
{
public:
	InterpolateVideoWindow(InterpolateVideo *plugin);
	~InterpolateVideoWindow();

	void create_objects();
	void update_enabled();

	ArrayList<BC_ListBoxItem*> frame_rates;
	InterpolateVideo *plugin;

	InterpolateVideoRate *rate;
	InterpolateVideoRateMenu *rate_menu;
	InterpolateVideoKeyframes *keyframes;
	InterpolateVideoFlow *flow;
	InterpolateVideoVectors *vectors;
	BC_Title *radius_title;
	InterpolateVideoRadius *radius;
	BC_Title *size_title;
	InterpolateVideoSize *size;

};




#endif


