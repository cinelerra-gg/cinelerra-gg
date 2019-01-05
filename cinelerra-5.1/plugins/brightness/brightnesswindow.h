
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

#ifndef BRIGHTNESSWINDOW_H
#define BRIGHTNESSWINDOW_H


class BrightnessThread;
class BrightnessWindow;
class BrightnessSlider;
class BrightnessLuma;
class BrightnessReset;

#include "brightness.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "thread.h"


class BrightnessWindow : public PluginClientWindow
{
public:
	BrightnessWindow(BrightnessMain *client);
	~BrightnessWindow();
	void update();
	void create_objects();

	BrightnessMain *client;
	BrightnessSlider *brightness;
	BrightnessSlider *contrast;
	BrightnessLuma *luma;
	BrightnessReset *reset;
};

class BrightnessSlider : public BC_FSlider
{
public:
	BrightnessSlider(BrightnessMain *client, float *output, int x, int y, int is_brightness);
	~BrightnessSlider();
	int handle_event();
	char* get_caption();

	BrightnessMain *client;
	float *output;
	int is_brightness;
	char string[BCTEXTLEN];
};

class BrightnessLuma : public BC_CheckBox
{
public:
	BrightnessLuma(BrightnessMain *client, int x, int y);
	~BrightnessLuma();
	int handle_event();

	BrightnessMain *client;
};

class BrightnessReset : public BC_GenericButton
{
public:
	BrightnessReset(BrightnessMain *client, BrightnessWindow *window, int x, int y);
	~BrightnessReset();
	int handle_event();
	BrightnessMain *client;
	BrightnessWindow *window;
};

#endif




