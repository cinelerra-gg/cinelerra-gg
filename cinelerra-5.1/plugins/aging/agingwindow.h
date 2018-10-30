
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

#ifndef AGINGWINDOW_H
#define AGINGWINDOW_H

#include "guicast.h"

class AgingThread;
class AgingWindow;

#include "filexml.h"
#include "mutex.h"
#include "aging.h"

class AgingCheckBox;
class AgingISlider;

class AgingWindow : public PluginClientWindow
{
public:
	AgingWindow(AgingMain *plugin);
	~AgingWindow();

	void create_objects();
	AgingMain *plugin;

	AgingCheckBox *color;
	AgingCheckBox *scratches;
	AgingISlider *scratch_count;
	AgingCheckBox *pits;
	AgingISlider *pit_count;
	AgingCheckBox *dust;
	AgingISlider *dust_count;
};

class AgingISlider : public BC_ISlider
{
public:
	AgingISlider(AgingWindow *win,
		int x, int y, int w, int min, int max, int *output);
	~AgingISlider();
	int handle_event();

	AgingWindow *win;
	int *output;
};

class AgingCheckBox : public BC_CheckBox
{
public:
	AgingCheckBox(AgingWindow *win, int x, int y, int *output, const char *text);
	int handle_event();

	AgingWindow *win;
};

#endif
