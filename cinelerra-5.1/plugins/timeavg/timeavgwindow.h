
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

#ifndef TIMEAVGWINDOW_H
#define TIMEAVGWINDOW_H


class TimeAvgThread;
class TimeAvgWindow;
class TimeAvgAvg;
class TimeAvgAccum;
class TimeAvgReplace;
class TimeAvgGreater;
class TimeAvgLess;
class TimeAvgParanoid;
class TimeAvgNoSubtract;
class TimeThresholdSlider;
class TimeBorderSlider;

#include "guicast.h"
#include "mutex.h"
#include "timeavg.h"


class TimeAvgSlider;

class TimeAvgWindow : public PluginClientWindow
{
public:
	TimeAvgWindow(TimeAvgMain *client);
	~TimeAvgWindow();

	void create_objects();
	void update_toggles();

	TimeAvgMain *client;
	TimeAvgSlider *total_frames;
	TimeThresholdSlider *threshold;
	TimeBorderSlider *border;
	TimeAvgAvg *avg;
	TimeAvgAccum *accum;
	TimeAvgReplace *replace;
	TimeAvgGreater *greater;
	TimeAvgLess *less;
	TimeAvgParanoid *paranoid;
	TimeAvgNoSubtract *no_subtract;
};

class TimeAvgSlider : public BC_ISlider
{
public:
	TimeAvgSlider(TimeAvgMain *client, int x, int y);
	~TimeAvgSlider();
	int handle_event();

	TimeAvgMain *client;
};

class TimeThresholdSlider : public BC_ISlider
{
public:
	TimeThresholdSlider(TimeAvgMain *client, int x, int y);
	~TimeThresholdSlider();
	int handle_event();

	TimeAvgMain *client;
};

class TimeBorderSlider : public BC_ISlider
{
public:
	TimeBorderSlider(TimeAvgMain *client, int x, int y);
	~TimeBorderSlider();
	int handle_event();

	TimeAvgMain *client;
};

class TimeAvgAvg : public BC_Radial
{
public:
	TimeAvgAvg(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y);
	int handle_event();
	TimeAvgMain *client;
	TimeAvgWindow *gui;
};

class TimeAvgAccum : public BC_Radial
{
public:
	TimeAvgAccum(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y);
	int handle_event();
	TimeAvgMain *client;
	TimeAvgWindow *gui;
};

class TimeAvgReplace : public BC_Radial
{
public:
	TimeAvgReplace(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y);
	int handle_event();
	TimeAvgMain *client;
	TimeAvgWindow *gui;
};

class TimeAvgGreater : public BC_Radial
{
public:
	TimeAvgGreater(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y);
	int handle_event();
	TimeAvgMain *client;
	TimeAvgWindow *gui;
};

class TimeAvgLess : public BC_Radial
{
public:
	TimeAvgLess(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y);
	int handle_event();
	TimeAvgMain *client;
	TimeAvgWindow *gui;
};

class TimeAvgParanoid : public BC_CheckBox
{
public:
	TimeAvgParanoid(TimeAvgMain *client, int x, int y);
	int handle_event();
	TimeAvgMain *client;
};

class TimeAvgNoSubtract : public BC_CheckBox
{
public:
	TimeAvgNoSubtract(TimeAvgMain *client, int x, int y);
	int handle_event();
	TimeAvgMain *client;
};

#endif
