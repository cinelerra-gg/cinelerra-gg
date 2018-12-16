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

#include "sketcher.h"
#include "guicast.h"
#include "colorpicker.h"

class Sketcher;
class SketcherCoord;
class SketcherNum;
class SketcherCurveTypeItem;
class SketcherCurveType;
class SketcherCurvePenItem;
class SketcherCurvePen;
class SketcherCurveColor;
class SketcherNewCurve;
class SketcherDelCurve;
class SketcherCurveUp;
class SketcherCurveDn;
class SketcherCurveWidth;
class SketcherCurveList;
class SketcherPointX;
class SketcherPointY;
class SketcherPointId;
class SketcherDrag;
class SketcherPointTypeItem;
class SketcherPointType;
class SketcherPointList;
class SketcherNewPoint;
class SketcherDelPoint;
class SketcherPointUp;
class SketcherPointDn;
class SketcherResetCurves;
class SketcherResetPoints;
class SketcherWindow;


class SketcherCoord : public BC_TumbleTextBox
{
public:
	SketcherWindow *gui;

	SketcherCoord(SketcherWindow *gui, int x, int y, coord output,
		coord mn=-32767, coord mx=32767);
	~SketcherCoord();
	int update(coord v) { return BC_TumbleTextBox::update((coord)v); }
};

class SketcherNum : public BC_TumbleTextBox
{
public:
	SketcherWindow *gui;

	SketcherNum(SketcherWindow *gui, int x, int y, int output,
		int mn=-32767, int mx=32767);
	~SketcherNum();
	int update(int v) { return BC_TumbleTextBox::update((int64_t)v); }
};

class SketcherCurvePenItem : public BC_MenuItem
{
public:
	SketcherCurvePenItem(int pen);
	int handle_event();
	int pen;
};

class SketcherCurvePen : public BC_PopupMenu
{
public:
	SketcherCurvePen(SketcherWindow *gui, int x, int y, int pen);
	void create_objects();
	void update(int pen);

	SketcherWindow *gui;
	int pen;
};

class SketcherCurveColor : public ColorBoxButton
{
public:
	SketcherCurveColor(SketcherWindow *gui,
		int x, int y, int w, int h, int color, int alpha);
	~SketcherCurveColor();

	int handle_new_color(int color, int alpha);
	void handle_done_event(int result);

	int color;
	VFrame *vframes[3];
	SketcherWindow *gui;
};

class SketcherNewCurve : public BC_GenericButton
{
public:
	SketcherNewCurve(SketcherWindow *gui, Sketcher *plugin, int x, int y);
	~SketcherNewCurve();

	int handle_event();

	SketcherWindow *gui;
	Sketcher *plugin;
};

class SketcherDelCurve : public BC_GenericButton
{
public:
	SketcherDelCurve(SketcherWindow *gui, Sketcher *plugin, int x, int y);
	~SketcherDelCurve();

	int handle_event();

	Sketcher *plugin;
	SketcherWindow *gui;
};

class SketcherCurveUp : public BC_GenericButton
{
public:
	SketcherCurveUp(SketcherWindow *gui, int x, int y);
	~SketcherCurveUp();

	int handle_event();

	SketcherWindow *gui;
};

class SketcherCurveDn : public BC_GenericButton
{
public:
	SketcherCurveDn(SketcherWindow *gui, int x, int y);
	~SketcherCurveDn();

	int handle_event();

	SketcherWindow *gui;
};

class SketcherCurveWidth : public SketcherNum
{
public:
	SketcherCurveWidth(SketcherWindow *gui, int x, int y, int width);
	~SketcherCurveWidth();

	int handle_event();
	void update(int width);
	int width;
};

class SketcherCurveList : public BC_ListBox
{
public:
	SketcherCurveList(SketcherWindow *gui, Sketcher *plugin, int x, int y);
	~SketcherCurveList();

	int handle_event();
	int selection_changed();
	int column_resize_event();
	ArrayList<BC_ListBoxItem*> cols[CV_SZ];
	void clear();
	void add_curve(const char *id, const char *pen,
		const char *width, const char *color, const char *alpha);
	void del_curve(int i);
	void set_selected(int k);
	void update(int k);
	void update_list(int k);

	SketcherWindow *gui;
	Sketcher *plugin;
	const char *col_titles[CV_SZ];
	int col_widths[CV_SZ];
};


class SketcherPointX : public SketcherCoord
{
public:
	SketcherPointX(SketcherWindow *gui, int x, int y, float output)
	 : SketcherCoord(gui, x, y, output) {}
	~SketcherPointX() {}

	int handle_event();
};
class SketcherPointY : public SketcherCoord
{
public:
	SketcherPointY(SketcherWindow *gui, int x, int y, float output)
	 : SketcherCoord(gui, x, y, output) {}
	~SketcherPointY() {}

	int handle_event();
};

class SketcherPointId : public SketcherNum
{
public:
	SketcherPointId(SketcherWindow *gui, int x, int y, int output)
	 : SketcherNum(gui, x, y, output) {}
	~SketcherPointId() {}

	int handle_event();
};


class SketcherDrag : public BC_CheckBox
{
public:
	SketcherDrag(SketcherWindow *gui, int x, int y);

	int handle_event();
	SketcherWindow *gui;
};

class SketcherPointTypeItem : public BC_MenuItem
{
public:
	SketcherPointTypeItem(int arc);
	int handle_event();
	int arc;
};

class SketcherPointType : public BC_PopupMenu
{
public:
	SketcherPointType(SketcherWindow *gui, int x, int y, int arc);
	void create_objects();
	void update(int arc);

	SketcherWindow *gui;
	int type;
};


class SketcherPointList : public BC_ListBox
{
public:
	SketcherPointList(SketcherWindow *gui, Sketcher *plugin, int x, int y);
	~SketcherPointList();

	int handle_event();
	int selection_changed();
	int column_resize_event();
	ArrayList<BC_ListBoxItem*> cols[PT_SZ];
	void clear();
	void add_point(const char *id, const char *ty, const char *xp, const char *yp);
	void set_point(int i, int c, int v);
	void set_point(int i, int c, coord v);
	void set_point(int i, int c, const char *cp);
	void set_selected(int k);
	void update(int k);
	void update_list(int k);


	SketcherWindow *gui;
	Sketcher *plugin;
	const char *col_titles[PT_SZ];
	int col_widths[PT_SZ];
};

class SketcherNewPoint : public BC_GenericButton
{
public:
	SketcherNewPoint(SketcherWindow *gui, Sketcher *plugin, int x, int y);
	~SketcherNewPoint();

	int handle_event();

	SketcherWindow *gui;
	Sketcher *plugin;
};

class SketcherDelPoint : public BC_GenericButton
{
public:
	SketcherDelPoint(SketcherWindow *gui, Sketcher *plugin, int x, int y);
	~SketcherDelPoint();

	int handle_event();

	Sketcher *plugin;
	SketcherWindow *gui;
};

class SketcherPointUp : public BC_GenericButton
{
public:
	SketcherPointUp(SketcherWindow *gui, int x, int y);
	~SketcherPointUp();

	int handle_event();

	SketcherWindow *gui;
};

class SketcherPointDn : public BC_GenericButton
{
public:
	SketcherPointDn(SketcherWindow *gui, int x, int y);
	~SketcherPointDn();

	int handle_event();

	SketcherWindow *gui;
};

class SketcherResetCurves : public BC_GenericButton
{
public:
	SketcherResetCurves(SketcherWindow *gui, Sketcher *plugin, int x, int y);
	~SketcherResetCurves();

	int handle_event();

	Sketcher *plugin;
	SketcherWindow *gui;
};

class SketcherResetPoints : public BC_GenericButton
{
public:
	SketcherResetPoints(SketcherWindow *gui, Sketcher *plugin, int x, int y);
	~SketcherResetPoints();

	int handle_event();

	Sketcher *plugin;
	SketcherWindow *gui;
};


class SketcherWindow : public PluginClientWindow
{
public:
	SketcherWindow(Sketcher *plugin);
	~SketcherWindow();

	void create_objects();
	void done_event(int result);
	void update_gui();
	int grab_event(XEvent *event);
	int do_grab_event(XEvent *event);
	int grab_button_press(XEvent *event);
	int grab_cursor_motion();
	void send_configure_change();
	int keypress_event();

	Sketcher *plugin;

	BC_Title *title_pen, *title_color, *title_width;
	SketcherCurveType *curve_type;
	SketcherCurvePen *curve_pen;
	SketcherCurveColor *curve_color;
	SketcherNewCurve *new_curve;
	SketcherDelCurve *del_curve;
	SketcherCurveUp *curve_up;
	SketcherCurveDn *curve_dn;
	SketcherCurveWidth *curve_width;
	SketcherCurveList *curve_list;
	SketcherResetCurves *reset_curves;

	SketcherResetPoints *reset_points;
	SketcherDrag *drag;
	SketcherPointType *point_type;
	SketcherPointList *point_list;
	BC_Title *title_x, *title_y, *title_id;
	SketcherPointX *point_x;
	SketcherPointY *point_y;
	SketcherPointId *point_id;
	SketcherNewPoint *new_point;
	SketcherDelPoint *del_point;
	SketcherPointUp *point_up;
	SketcherPointDn *point_dn;
	int64_t position;
	float projector_x, projector_y, projector_z;
	int track_w, track_h;
	int new_points;
	float cursor_x, cursor_y;
	float output_x, output_y;
	int state, dragging;
	int pending_motion, pending_config;
	XEvent motion_event;
	float last_x, last_y;
	BC_Title *notes0, *notes1, *notes2, *notes3;
};
#endif

