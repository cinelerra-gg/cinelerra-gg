
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef COLORBALANCEWINDOW_H
#define COLORBALANCEWINDOW_H

#define RESET_DEFAULT_SETTINGS 10
#define RESET_ALL     0
#define RESET_CYAN    1
#define RESET_MAGENTA 2
#define RESET_YELLOW  3

class ColorBalanceThread;
class ColorBalanceWindow;
class ColorBalanceSlider;
class ColorBalancePreserve;
class ColorBalanceLock;
class ColorBalanceWhite;
class ColorBalanceReset;
class ColorBalanceSliderClr;

#include "filexml.h"
#include "guicast.h"
#include "mutex.h"
#include "colorbalance.h"
#include "pluginclient.h"



class ColorBalanceWindow : public PluginClientWindow
{
public:
	ColorBalanceWindow(ColorBalanceMain *client);
	~ColorBalanceWindow();

	void create_objects();
	void update_gui(int clear);

	ColorBalanceMain *client;
	ColorBalanceSlider *cyan;
	ColorBalanceSlider *magenta;
	ColorBalanceSlider *yellow;
	ColorBalanceLock *lock_params;
	ColorBalancePreserve *preserve;
	ColorBalanceWhite *use_eyedrop;
	ColorBalanceReset *reset;
	ColorBalanceSliderClr *cyanClr;
	ColorBalanceSliderClr *magentaClr;
	ColorBalanceSliderClr *yellowClr;
};

class ColorBalanceSlider : public BC_ISlider
{
public:
	ColorBalanceSlider(ColorBalanceMain *client, float *output, int x, int y);
	~ColorBalanceSlider();
	int handle_event();
	char* get_caption();

	ColorBalanceMain *client;
	float *output;
	float old_value;
	char string[BCTEXTLEN];
};

class ColorBalancePreserve : public BC_CheckBox
{
public:
	ColorBalancePreserve(ColorBalanceMain *client, int x, int y);
    ~ColorBalancePreserve();

    int handle_event();
    ColorBalanceMain *client;
};

class ColorBalanceLock : public BC_CheckBox
{
public:
	ColorBalanceLock(ColorBalanceMain *client, int x, int y);
    ~ColorBalanceLock();

    int handle_event();
    ColorBalanceMain *client;
};

class ColorBalanceWhite : public BC_GenericButton
{
public:
	ColorBalanceWhite(ColorBalanceMain *plugin, ColorBalanceWindow *gui, int x, int y);
	int handle_event();
	ColorBalanceMain *plugin;
	ColorBalanceWindow *gui;
};

class ColorBalanceReset : public BC_GenericButton
{
public:
	ColorBalanceReset(ColorBalanceMain *plugin, ColorBalanceWindow *gui, int x, int y);
	int handle_event();
	ColorBalanceMain *plugin;
	ColorBalanceWindow *gui;
};

class ColorBalanceSliderClr : public BC_GenericButton
{
public:
	ColorBalanceSliderClr(ColorBalanceMain *plugin, ColorBalanceWindow *gui, int x, int y, int w, int clear);
	~ColorBalanceSliderClr();
	int handle_event();
	ColorBalanceMain *plugin;
	ColorBalanceWindow *gui;
	int clear;
};

#endif
