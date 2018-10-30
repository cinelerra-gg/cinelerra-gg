
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

#ifndef DEINTERWINDOW_H
#define DEINTERWINDOW_H


class DeInterlaceThread;
class DeInterlaceWindow;

#include "guicast.h"
#include "mutex.h"
#include "deinterlace.h"
#include "pluginclient.h"


class DeInterlaceOption;
class DeInterlaceAdaptive;
class DeInterlaceThreshold;

class DeInterlaceWindow : public PluginClientWindow
{
public:
	DeInterlaceWindow(DeInterlaceMain *client);
	~DeInterlaceWindow();

	void create_objects();
	int set_mode(int mode, int recursive);
	void get_status_string(char *string, int changed_rows);

	DeInterlaceMain *client;
	DeInterlaceOption *odd_fields;
	DeInterlaceOption *even_fields;
	DeInterlaceOption *average_fields;
	DeInterlaceOption *swap_odd_fields;
	DeInterlaceOption *swap_even_fields;
	DeInterlaceOption *avg_even;
	DeInterlaceOption *avg_odd;
	DeInterlaceOption *none;
//	DeInterlaceAdaptive *adaptive;
//	DeInterlaceThreshold *threshold;
//	BC_Title *status;
};

class DeInterlaceOption : public BC_Radial
{
public:
	DeInterlaceOption(DeInterlaceMain *client,
		DeInterlaceWindow *window,
		int output,
		int x,
		int y,
		char *text);
	~DeInterlaceOption();
	int handle_event();

	DeInterlaceMain *client;
	DeInterlaceWindow *window;
	int output;
};

class DeInterlaceAdaptive : public BC_CheckBox
{
public:
	DeInterlaceAdaptive(DeInterlaceMain *client, int x, int y);
	int handle_event();
	DeInterlaceMain *client;
};

class DeInterlaceThreshold : public BC_IPot
{
public:
	DeInterlaceThreshold(DeInterlaceMain *client, int x, int y);
	int handle_event();
	DeInterlaceMain *client;
};




#endif
