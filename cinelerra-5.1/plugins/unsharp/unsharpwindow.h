
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

#ifndef UNSHARPWINDOW_H
#define UNSHARPWINDOW_H

#include "guicast.h"
#include "unsharp.inc"
#include "unsharpwindow.inc"

#define RESET_DEFAULT_SETTINGS 10
#define RESET_ALL       0
#define RESET_RADIUS	1
#define RESET_AMOUNT    2
#define RESET_THRESHOLD 3


class UnsharpRadius : public BC_FPot
{
public:
	UnsharpRadius(UnsharpMain *plugin, int x, int y);
	int handle_event();
	UnsharpMain *plugin;
};

class UnsharpAmount : public BC_FPot
{
public:
	UnsharpAmount(UnsharpMain *plugin, int x, int y);
	int handle_event();
	UnsharpMain *plugin;
};

class UnsharpThreshold : public BC_IPot
{
public:
	UnsharpThreshold(UnsharpMain *plugin, int x, int y);
	int handle_event();
	UnsharpMain *plugin;
};

class UnsharpReset : public BC_GenericButton
{
public:
	UnsharpReset(UnsharpMain *plugin, UnsharpWindow *window, int x, int y);
	~UnsharpReset();
	int handle_event();
	UnsharpMain *plugin;
	UnsharpWindow *window;
};

class UnsharpDefaultSettings : public BC_GenericButton
{
public:
	UnsharpDefaultSettings(UnsharpMain *plugin, UnsharpWindow *window, int x, int y, int w);
	~UnsharpDefaultSettings();
	int handle_event();
	UnsharpMain *plugin;
	UnsharpWindow *window;
};

class UnsharpSliderClr : public BC_GenericButton
{
public:
	UnsharpSliderClr(UnsharpMain *plugin, UnsharpWindow *window, int x, int y, int w, int clear);
	~UnsharpSliderClr();
	int handle_event();
	UnsharpMain *plugin;
	UnsharpWindow *window;
	int clear;
};

class UnsharpWindow : public PluginClientWindow
{
public:
	UnsharpWindow(UnsharpMain *plugin);
	~UnsharpWindow();

	void create_objects();
	void update_gui(int clear);

	UnsharpRadius *radius;
	UnsharpAmount *amount;
	UnsharpThreshold *threshold;
	UnsharpMain *plugin;
	UnsharpReset *reset;
	UnsharpDefaultSettings *default_settings;
	UnsharpSliderClr *radiusClr;
	UnsharpSliderClr *amountClr;
	UnsharpSliderClr *thresholdClr;
};








#endif
