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

#ifndef __CRIKEYWINDOW_H__
#define __CRIKEYWINDOW_H__

#include "guicast.h"

class CriKey;
class CriKeyWindow;
class CriKeyNum;
class CriKeyPointX;
class CriKeyPointY;
class CriKeyDrawMode;
class CriKeyDrawModeItem;
class CriKeyThreshold;
class CriKeyDrag;
class CriKeyPointList;
class CriKeyNewPoint;
class CriKeyDelPoint;
class CriKeyPointUp;
class CriKeyPointDn;
class CriKeyReset;


class CriKeyNum : public BC_TumbleTextBox
{
public:
	CriKeyWindow *gui;

	CriKeyNum(CriKeyWindow *gui, int x, int y, float output);
	~CriKeyNum();
};
class CriKeyPointX : public CriKeyNum
{
public:
	CriKeyPointX(CriKeyWindow *gui, int x, int y, float output)
	 : CriKeyNum(gui, x, y, output) {}
	~CriKeyPointX() {}

	int handle_event();
};
class CriKeyPointY : public CriKeyNum
{
public:
	CriKeyPointY(CriKeyWindow *gui, int x, int y, float output)
	 : CriKeyNum(gui, x, y, output) {}
	~CriKeyPointY() {}

	int handle_event();
};

class CriKeyDrawMode : public BC_PopupMenu
{
	const char *draw_modes[DRAW_MODES];
public:
	CriKeyDrawMode(CriKeyWindow *gui, int x, int y);

	void create_objects();
	void update(int mode, int send=1);
	CriKeyWindow *gui;
	int mode;
};
class CriKeyDrawModeItem : public BC_MenuItem
{
public:
	CriKeyDrawModeItem(const char *txt, int id)
	: BC_MenuItem(txt) { this->id = id; }

	int handle_event();
	CriKeyWindow *gui;
	int id;
};

class CriKeyThreshold : public BC_FSlider
{
public:
	CriKeyThreshold(CriKeyWindow *gui, int x, int y, int w);
	int handle_event();
	int wheel_event(int v);
	CriKeyWindow *gui;
};

class CriKeyDrag : public BC_CheckBox
{
public:
	CriKeyDrag(CriKeyWindow *gui, int x, int y);

	int handle_event();
	CriKeyWindow *gui;
};

class CriKeyPointList : public BC_ListBox
{
public:
	CriKeyPointList(CriKeyWindow *gui, CriKey *plugin, int x, int y);
	~CriKeyPointList();

	int handle_event();
	int selection_changed();
	int column_resize_event();
	ArrayList<BC_ListBoxItem*> cols[PT_SZ];
	void clear();
	void new_point(const char *ep, const char *xp, const char *yp,
		const char *tp, const char *tag);
	void del_point(int i);
	void set_point(int i, int c, float v);
	void set_point(int i, int c, const char *cp);
	int set_selected(int k);
	void update(int k);
	void update_list(int k);


	CriKeyWindow *gui;
	CriKey *plugin;
	const char *titles[PT_SZ];
	int widths[PT_SZ];
};

class CriKeyNewPoint : public BC_GenericButton
{
public:
	CriKeyNewPoint(CriKeyWindow *gui, CriKey *plugin, int x, int y);
	~CriKeyNewPoint();

	int handle_event();

	CriKeyWindow *gui;
	CriKey *plugin;
};

class CriKeyDelPoint : public BC_GenericButton
{
public:
	CriKeyDelPoint(CriKeyWindow *gui, CriKey *plugin, int x, int y);
	~CriKeyDelPoint();

	int handle_event();

	CriKey *plugin;
	CriKeyWindow *gui;
};

class CriKeyPointUp : public BC_GenericButton
{
public:
	CriKeyPointUp(CriKeyWindow *gui, int x, int y);
	~CriKeyPointUp();

	int handle_event();

	CriKeyWindow *gui;
};

class CriKeyPointDn : public BC_GenericButton
{
public:
	CriKeyPointDn(CriKeyWindow *gui, int x, int y);
	~CriKeyPointDn();

	int handle_event();

	CriKeyWindow *gui;
};

class CriKeyReset : public BC_GenericButton
{
public:
	CriKeyReset(CriKeyWindow *gui, CriKey *plugin, int x, int y);
	~CriKeyReset();

	int handle_event();

	CriKey *plugin;
	CriKeyWindow *gui;
};


class CriKeyWindow : public PluginClientWindow
{
public:
	CriKeyWindow(CriKey *plugin);
	~CriKeyWindow();

	void create_objects();
	void update_gui();
	void start_color_thread();
	int grab_event(XEvent *event);
	void done_event(int result);
	int check_configure_change(int ret);
	void send_configure_change();

	CriKey *plugin;
	CriKeyThreshold *threshold;
	CriKeyDrawMode *draw_mode;

	BC_Title *title_x, *title_y;
	CriKeyPointX *point_x;
	CriKeyPointY *point_y;
	CriKeyNewPoint *new_point;
	CriKeyDelPoint *del_point;
	CriKeyPointUp *point_up;
	CriKeyPointDn *point_dn;
	int dragging, pending_config;
	float last_x, last_y;
	CriKeyDrag *drag;
	CriKeyPointList *point_list;
	CriKeyReset *reset;
	BC_Title *notes;
};

#endif

