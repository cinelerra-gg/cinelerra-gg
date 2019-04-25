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

#ifndef __TRACERWINDOW_H__
#define __TRACERWINDOW_H__

#include "guicast.h"

class Tracer;
class TracerWindow;
class TracerNum;
class TracerPointX;
class TracerPointY;
class TracerDrag;
class TracerDraw;
class TracerFill;
class TracerRadius;
class TracerScale;
class TracerPointList;
class TracerNewPoint;
class TracerDelPoint;
class TracerPointUp;
class TracerPointDn;
class TracerReset;


class TracerNum : public BC_TumbleTextBox
{
public:
	TracerWindow *gui;

	TracerNum(TracerWindow *gui, int x, int y, float output);
	~TracerNum();
};
class TracerPointX : public TracerNum
{
public:
	TracerPointX(TracerWindow *gui, int x, int y, float output)
	 : TracerNum(gui, x, y, output) {}
	~TracerPointX() {}

	int handle_event();
};
class TracerPointY : public TracerNum
{
public:
	TracerPointY(TracerWindow *gui, int x, int y, float output)
	 : TracerNum(gui, x, y, output) {}
	~TracerPointY() {}

	int handle_event();
};

class TracerThreshold : public BC_FSlider
{
public:
	TracerThreshold(TracerWindow *gui, int x, int y, int w);
	int handle_event();
	int wheel_event(int v);
	TracerWindow *gui;
};

class TracerDrag : public BC_CheckBox
{
public:
	TracerDrag(TracerWindow *gui, int x, int y);

	int handle_event();
	TracerWindow *gui;
};

class TracerDraw : public BC_CheckBox
{
public:
	TracerDraw(TracerWindow *gui, int x, int y);

	int handle_event();
	TracerWindow *gui;
};

class TracerFill : public BC_CheckBox
{
public:
	TracerFill(TracerWindow *gui, int x, int y);

	int handle_event();
	TracerWindow *gui;
};

class TracerRadius : public BC_ISlider
{
public:
	TracerRadius(TracerWindow *gui, int x, int y, int w);

	int handle_event();
	TracerWindow *gui;
};

class TracerScale : public BC_FSlider
{
public:
	TracerScale(TracerWindow *gui, int x, int y, int w);

	int handle_event();
	TracerWindow *gui;
};

class TracerPointList : public BC_ListBox
{
public:
	TracerPointList(TracerWindow *gui, Tracer *plugin, int x, int y);
	~TracerPointList();

	int handle_event();
	int selection_changed();
	int column_resize_event();
	ArrayList<BC_ListBoxItem*> cols[PT_SZ];
	void clear();
	void new_point(const char *xp, const char *yp);
	void del_point(int i);
	void set_point(int i, int c, float v);
	void set_point(int i, int c, const char *cp);
	int set_selected(int k);
	void update(int k);
	void update_list(int k);


	TracerWindow *gui;
	Tracer *plugin;
	const char *titles[PT_SZ];
	int widths[PT_SZ];
};

class TracerNewPoint : public BC_GenericButton
{
public:
	TracerNewPoint(TracerWindow *gui, Tracer *plugin, int x, int y);
	~TracerNewPoint();

	int handle_event();

	TracerWindow *gui;
	Tracer *plugin;
};

class TracerDelPoint : public BC_GenericButton
{
public:
	TracerDelPoint(TracerWindow *gui, Tracer *plugin, int x, int y);
	~TracerDelPoint();

	int handle_event();

	Tracer *plugin;
	TracerWindow *gui;
};

class TracerPointUp : public BC_GenericButton
{
public:
	TracerPointUp(TracerWindow *gui, int x, int y);
	~TracerPointUp();

	int handle_event();

	TracerWindow *gui;
};

class TracerPointDn : public BC_GenericButton
{
public:
	TracerPointDn(TracerWindow *gui, int x, int y);
	~TracerPointDn();

	int handle_event();

	TracerWindow *gui;
};

class TracerReset : public BC_GenericButton
{
public:
	TracerReset(TracerWindow *gui, Tracer *plugin, int x, int y);
	~TracerReset();

	int handle_event();

	Tracer *plugin;
	TracerWindow *gui;
};


class TracerWindow : public PluginClientWindow
{
public:
	TracerWindow(Tracer *plugin);
	~TracerWindow();

	void create_objects();
	void update_gui();
	void start_color_thread();
	int grab_event(XEvent *event);
	int do_grab_event(XEvent *event);
	void done_event(int result);
	void send_configure_change();

	Tracer *plugin;
	BC_Title *title_x, *title_y;
	TracerPointX *point_x;
	TracerPointY *point_y;
	TracerNewPoint *new_point;
	TracerDelPoint *del_point;
	TracerPointUp *point_up;
	TracerPointDn *point_dn;
	int dragging, pending_config;
	float last_x, last_y;
	TracerDrag *drag;
	TracerDraw *draw;
	TracerFill *fill;
	BC_Title *title_r, *title_s;
	TracerRadius *radius;
	TracerScale *scale;
	TracerPointList *point_list;
	TracerReset *reset;
};

#endif

