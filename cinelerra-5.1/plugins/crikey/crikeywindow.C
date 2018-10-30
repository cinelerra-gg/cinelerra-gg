/*
 * CINELERRA
 * Copyright (C) 2014 Adam Williams <broadcast at earthling dot net>
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

#include "automation.h"
#include "bcdisplayinfo.h"
#include "clip.h"
#include "crikey.h"
#include "crikeywindow.h"
#include "cstrdup.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginserver.h"
#include "theme.h"
#include "track.h"

#define COLOR_W 50
#define COLOR_H 30

CriKeyNum::CriKeyNum(CriKeyWindow *gui, int x, int y, float output)
 : BC_TumbleTextBox(gui, output, -32767.0f, 32767.0f, x, y, 120)
{
	this->gui = gui;
	set_increment(1);
	set_precision(1);
}

CriKeyNum::~CriKeyNum()
{
}

int CriKeyPointX::handle_event()
{
	if( !CriKeyNum::handle_event() ) return 0;
	CriKeyPointList *point_list = gui->point_list;
	int hot_point = point_list->get_selection_number(0, 0);
	CriKeyPoints &points = gui->plugin->config.points;
	int sz = points.size();
	if( hot_point >= 0 && hot_point < sz ) {
		float v = atof(get_text());
		points[hot_point]->x = v;
		point_list->set_point(hot_point, PT_X, v);
	}
	point_list->update_list(hot_point);
	gui->send_configure_change();
	return 1;
}
int CriKeyPointY::handle_event()
{
	if( !CriKeyNum::handle_event() ) return 0;
	CriKeyPointList *point_list = gui->point_list;
	int hot_point = point_list->get_selection_number(0, 0);
	CriKeyPoints &points = gui->plugin->config.points;
	int sz = points.size();
	if( hot_point >= 0 && hot_point < sz ) {
		float v = atof(get_text());
		points[hot_point]->y = v;
		point_list->set_point(hot_point, PT_Y, v);
	}
	point_list->update_list(hot_point);
	gui->send_configure_change();
	return 1;
}

int CriKeyDrawModeItem::handle_event()
{
	((CriKeyDrawMode *)get_popup_menu())->update(id);
	return 1;
}
CriKeyDrawMode::CriKeyDrawMode(CriKeyWindow *gui, int x, int y)
 : BC_PopupMenu(x, y, 100, "", 1)
{
	this->gui = gui;
	draw_modes[DRAW_ALPHA]     = _("Alpha");
	draw_modes[DRAW_EDGE]      = _("Edge");
	draw_modes[DRAW_MASK]      = _("Mask");
	mode = -1;
}
void CriKeyDrawMode::create_objects()
{
	for( int i=0; i<DRAW_MODES; ++i )
		add_item(new CriKeyDrawModeItem(draw_modes[i], i));
	update(gui->plugin->config.draw_mode, 0);
}
void CriKeyDrawMode::update(int mode, int send)
{
	if( this->mode == mode ) return;
	this->mode = mode;
	set_text(draw_modes[mode]);
	gui->plugin->config.draw_mode = mode;
	if( send ) gui->send_configure_change();
}


CriKeyWindow::CriKeyWindow(CriKey *plugin)
 : PluginClientWindow(plugin, 380, 400, 380, 400, 0)
{
	this->plugin = plugin;
	this->title_x = 0;    this->point_x = 0;
	this->title_y = 0;    this->point_y = 0;
	this->new_point = 0;  this->del_point = 0;
	this->point_up = 0;   this->point_dn = 0;
	this->drag = 0;       this->dragging = 0;
	this->last_x = 0;     this->last_y = 0;
	this->point_list = 0; this->pending_config = 0;
}

CriKeyWindow::~CriKeyWindow()
{
}

void CriKeyWindow::create_objects()
{
	int x = 10, y = 10;
	int margin = plugin->get_theme()->widget_border;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y+5, _("Draw mode:")));
	int x1 = x + title->get_w() + 10 + margin;
	add_subwindow(draw_mode = new CriKeyDrawMode(this, x1, y));
	draw_mode->create_objects();
	y += draw_mode->get_h() + 10 + margin;

	CriKeyPoint *pt = plugin->config.points[plugin->config.selected];
	add_subwindow(title_x = new BC_Title(x, y, _("X:")));
	x1 = x + title_x->get_w() + margin;
	point_x = new CriKeyPointX(this, x1, y, pt->x);
	point_x->create_objects();
	x1 += point_x->get_w() + margin;
	add_subwindow(new_point = new CriKeyNewPoint(this, plugin, x1, y));
	x1 += new_point->get_w() + margin;
	add_subwindow(point_up = new CriKeyPointUp(this, x1, y));
	y += point_x->get_h() + margin;
	add_subwindow(title_y = new BC_Title(x, y, _("Y:")));
	x1 = x + title_y->get_w() + margin;
	point_y = new CriKeyPointY(this, x1, y, pt->y);
	point_y->create_objects();
	x1 += point_y->get_w() + margin;
	add_subwindow(del_point = new CriKeyDelPoint(this, plugin, x1, y));
	x1 += del_point->get_w() + margin;
	add_subwindow(point_dn = new CriKeyPointDn(this, x1, y));
	y += point_y->get_h() + margin + 10;
	add_subwindow(title = new BC_Title(x, y, _("Threshold:")));
	y += title->get_h() + margin;
	add_subwindow(threshold = new CriKeyThreshold(this, x, y, get_w() - x * 2));
	y += threshold->get_h() + margin;

	add_subwindow(drag = new CriKeyDrag(this, x, y));
	if( plugin->config.drag ) {
		if( !grab(plugin->server->mwindow->cwindow->gui) )
			eprintf("drag enabled, but compositor already grabbed\n");
	}
	x1 = x + drag->get_w() + margin + 32;
	add_subwindow(reset = new CriKeyReset(this, plugin, x1, y+3));
	y += drag->get_h() + margin;

	add_subwindow(point_list = new CriKeyPointList(this, plugin, x, y));
	point_list->update(plugin->config.selected);

	y += point_list->get_h() + 10;
	add_subwindow(notes = new BC_Title(x, y,
		 _("Right click in composer: create new point\n"
		   "Shift-left click in Enable field:\n"
		   "  if any off, turns all on\n"
		   "  if all on, turns rest off.")));
	show_window(1);
}

void CriKeyWindow::send_configure_change()
{
	pending_config = 0;
	plugin->send_configure_change();
}
int CriKeyWindow::check_configure_change(int ret)
{
	if( pending_config && !grab_event_count() )
		send_configure_change();
	return ret;
}

int CriKeyWindow::grab_event(XEvent *event)
{
	switch( event->type ) {
	case ButtonPress: break;
	case ButtonRelease: break;
	case MotionNotify: break;
	default:
		return check_configure_change(0);
	}

	MWindow *mwindow = plugin->server->mwindow;
	CWindowGUI *cwindow_gui = mwindow->cwindow->gui;
	CWindowCanvas *canvas = cwindow_gui->canvas;
	int cx, cy;  cwindow_gui->get_relative_cursor(cx, cy);
	cx -= mwindow->theme->ccanvas_x;
	cy -= mwindow->theme->ccanvas_y;

	if( !dragging ) {
		if( cx < 0 || cx >= mwindow->theme->ccanvas_w ||
		    cy < 0 || cy >= mwindow->theme->ccanvas_h )
			return check_configure_change(0);
	}

	switch( event->type ) {
	case ButtonPress:
		if( dragging ) return check_configure_change(0);
		if( event->xbutton.button == WHEEL_UP )  return threshold->wheel_event(1);
		if( event->xbutton.button == WHEEL_DOWN ) return threshold->wheel_event(-1);
		dragging = event->xbutton.state & ShiftMask ? -1 : 1;
		break;
	case ButtonRelease:
		if( !dragging ) return check_configure_change(0);
		dragging = 0;
		return 1;
	case MotionNotify:
		if( !dragging ) return check_configure_change(0);
		break;
	default:
		return check_configure_change(0);
	}

	float cursor_x = cx, cursor_y = cy;
	canvas->canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);
	int64_t position = plugin->get_source_position();
	float projector_x, projector_y, projector_z;
	Track *track = plugin->server->plugin->track;
	int track_w = track->track_w, track_h = track->track_h;
	track->automation->get_projector(
		&projector_x, &projector_y, &projector_z,
		position, PLAY_FORWARD);
	projector_x += mwindow->edl->session->output_w / 2;
	projector_y += mwindow->edl->session->output_h / 2;
	float output_x = (cursor_x - projector_x) / projector_z + track_w / 2;
	float output_y = (cursor_y - projector_y) / projector_z + track_h / 2;
	point_x->update((int64_t)(output_x));
	point_y->update((int64_t)(output_y));
	CriKeyPoints &points = plugin->config.points;

	if( dragging > 0 ) {
		switch( event->type ) {
		case ButtonPress: {
			int button_no = event->xbutton.button;
			int hot_point = -1;
			if( button_no == RIGHT_BUTTON ) {
				hot_point = plugin->new_point();
				CriKeyPoint *pt = points[hot_point];
				pt->x = output_x;  pt->y = output_y;
				point_list->update(hot_point);
				break;
			}
			int sz = points.size();
			if( hot_point < 0 && sz > 0 ) {
				CriKeyPoint *pt = points[hot_point=0];
				double dist = DISTANCE(output_x,output_y, pt->x,pt->y);
				for( int i=1; i<sz; ++i ) {
					pt = points[i];
					double d = DISTANCE(output_x,output_y, pt->x,pt->y);
					if( d >= dist ) continue;
					dist = d;  hot_point = i;
				}
				pt = points[hot_point];
				float px = (pt->x - track_w / 2) * projector_z + projector_x;
				float py = (pt->y - track_h / 2) * projector_z + projector_y;
				dist = DISTANCE(px, py, cursor_x,cursor_y);
				if( dist >= HANDLE_W ) hot_point = -1;
			}
			if( hot_point >= 0 && sz > 0 ) {
				CriKeyPoint *pt = points[hot_point];
				point_list->set_point(hot_point, PT_X, pt->x = output_x);
				for( int i=0; i<sz; ++i ) {
					pt = points[i];
					pt->e = i==hot_point ? !pt->e : 0;
					point_list->set_point(i, PT_E, pt->e ? "*" : "");
				}
				point_list->update_list(hot_point);
			}
			break; }
		case MotionNotify: {
			int hot_point = point_list->get_selection_number(0, 0);
			if( hot_point >= 0 && hot_point < points.size() ) {
				CriKeyPoint *pt = points[hot_point];
				if( pt->x == output_x && pt->y == output_y ) break;
				point_list->set_point(hot_point, PT_X, pt->x = output_x);
				point_list->set_point(hot_point, PT_Y, pt->y = output_y);
				point_x->update(pt->x);
				point_y->update(pt->y);
				point_list->update_list(hot_point);
			}
			break; }
		}
	}
	else {
		switch( event->type ) {
		case MotionNotify: {
			float dx = output_x - last_x, dy = output_y - last_y;
			int sz = points.size();
			for( int i=0; i<sz; ++i ) {
				CriKeyPoint *pt = points[i];
				point_list->set_point(i, PT_X, pt->x += dx);
				point_list->set_point(i, PT_Y, pt->y += dy);
			}
			int hot_point = point_list->get_selection_number(0, 0);
			if( hot_point >= 0 && hot_point < sz ) {
				CriKeyPoint *pt = points[hot_point];
				point_x->update(pt->x);
				point_y->update(pt->y);
				point_list->update_list(hot_point);
			}
			break; }
		}
	}

	last_x = output_x;  last_y = output_y;
	if( !grab_event_count() ) 
		send_configure_change();
	else
		pending_config = 1;
	return 1;
}

void CriKeyWindow::done_event(int result)
{
	ungrab(client->server->mwindow->cwindow->gui);
}

CriKeyPointList::CriKeyPointList(CriKeyWindow *gui, CriKey *plugin, int x, int y)
 : BC_ListBox(x, y, 360, 130, LISTBOX_TEXT)
{
	this->gui = gui;
	this->plugin = plugin;
	titles[PT_E] = _("E");    widths[PT_E] = 50;
	titles[PT_X] = _("X");    widths[PT_X] = 90;
	titles[PT_Y] = _("Y");    widths[PT_Y] = 90;
	titles[PT_T] = _("T");    widths[PT_T] = 70;
	titles[PT_TAG] = _("Tag");  widths[PT_TAG] = 50;
}
CriKeyPointList::~CriKeyPointList()
{
	clear();
}
void CriKeyPointList::clear()
{
	for( int i=PT_SZ; --i>=0; )
		cols[i].remove_all_objects();
}

int CriKeyPointList::column_resize_event()
{
	for( int i=PT_SZ; --i>=0; )
		widths[i] = get_column_width(i);
	return 1;
}

int CriKeyPointList::handle_event()
{
	int hot_point = get_selection_number(0, 0);
	const char *x_text = "", *y_text = "";
	float t = plugin->config.threshold;
	CriKeyPoints &points = plugin->config.points;

	int sz = points.size();
	if( hot_point >= 0 && sz > 0 ) {
		if( get_cursor_x() < widths[0] ) {
			if( shift_down() ) {
				int all_on = points[0]->e;
				for( int i=1; i<sz && all_on; ++i ) all_on = points[i]->e;
				int e = !all_on ? 1 : 0;
				for( int i=0; i<sz; ++i ) points[i]->e = e;
				points[hot_point]->e = 1;
			}
			else
				points[hot_point]->e = !points[hot_point]->e;
		}
		x_text = gui->point_list->cols[PT_X].get(hot_point)->get_text();
		y_text = gui->point_list->cols[PT_Y].get(hot_point)->get_text();
		t = points[hot_point]->t;
	}
	gui->point_x->update(x_text);
	gui->point_y->update(y_text);
	gui->threshold->update(t);
	update(hot_point);
	gui->send_configure_change();
	return 1;
}

int CriKeyPointList::selection_changed()
{
	handle_event();
	return 1;
}

void CriKeyPointList::new_point(const char *ep, const char *xp, const char *yp,
		const char *tp, const char *tag)
{
	cols[PT_E].append(new BC_ListBoxItem(ep));
	cols[PT_X].append(new BC_ListBoxItem(xp));
	cols[PT_Y].append(new BC_ListBoxItem(yp));
	cols[PT_T].append(new BC_ListBoxItem(tp));
	cols[PT_TAG].append(new BC_ListBoxItem(tag));
}

void CriKeyPointList::del_point(int i)
{
	for( int sz1=cols[0].size()-1, c=PT_SZ; --c>=0; )
		cols[c].remove_object_number(sz1-i);
}

void CriKeyPointList::set_point(int i, int c, float v)
{
	char s[BCSTRLEN]; sprintf(s,"%0.4f",v);
	set_point(i,c,s);
}
void CriKeyPointList::set_point(int i, int c, const char *cp)
{
	cols[c].get(i)->set_text(cp);
}

int CriKeyPointList::set_selected(int k)
{
	CriKeyPoints &points = plugin->config.points;
	int sz = points.size();
	if( !sz ) return -1;
	bclamp(k, 0, sz-1);
	for( int i=0; i<sz; ++i ) points[i]->e = 0;
	points[k]->e = 1;
	update_selection(&cols[0], k);
	return k;
}
void CriKeyPointList::update_list(int k)
{
	int xpos = get_xposition(), ypos = get_yposition();
	if( k < 0 ) k = get_selection_number(0, 0);
	update_selection(&cols[0], k);
	BC_ListBox::update(&cols[0], &titles[0],&widths[0],PT_SZ, xpos,ypos,k);
	center_selection();
}
void CriKeyPointList::update(int k)
{
	clear();
	CriKeyPoints &points = plugin->config.points;
	int sz = points.size();
	for( int i=0; i<sz; ++i ) {
		CriKeyPoint *pt = points[i];
		char etxt[BCSTRLEN];  sprintf(etxt,"%s", pt->e ? "*" : "");
		char xtxt[BCSTRLEN];  sprintf(xtxt,"%0.4f", pt->x);
		char ytxt[BCSTRLEN];  sprintf(ytxt,"%0.4f", pt->y);
		char ttxt[BCSTRLEN];  sprintf(ttxt,"%0.4f", pt->t);
		char ttag[BCSTRLEN];  sprintf(ttag,"%d", pt->tag);
		new_point(etxt, xtxt, ytxt, ttxt, ttag);
	}
	if( k >= 0 && k < sz ) {
		gui->point_x->update(gui->point_list->cols[PT_X].get(k)->get_text());
		gui->point_y->update(gui->point_list->cols[PT_Y].get(k)->get_text());
		plugin->config.selected = k;
	}

	update_list(k);
}

void CriKeyWindow::update_gui()
{
	draw_mode->update(plugin->config.draw_mode);
	threshold->update(plugin->config.threshold);
	drag->update(plugin->config.drag);
	point_list->update(-1);
}


CriKeyThreshold::CriKeyThreshold(CriKeyWindow *gui, int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, 0, 1, gui->plugin->config.threshold, 0)
{
	this->gui = gui;
	set_precision(0.005);
	set_pagination(0.01, 0.1);
}

int CriKeyThreshold::wheel_event(int v)
{
	if( v > 0 ) increase_value();
	else if( v < 0 ) decrease_value();
	handle_event();
	enable();
	return 1;
}

int CriKeyThreshold::handle_event()
{
	float v = get_value();
	gui->plugin->config.threshold = v;
	int hot_point = gui->point_list->get_selection_number(0, 0);
	if( hot_point >= 0 ) {
		CriKeyPoints &points = gui->plugin->config.points;
		CriKeyPoint *pt = points[hot_point];
		pt->t = v;  pt->e = 1;
		gui->point_list->update(hot_point);
	}
	gui->send_configure_change();
	return 1;
}


CriKeyPointUp::CriKeyPointUp(CriKeyWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Up"))
{
	this->gui = gui;
}
CriKeyPointUp::~CriKeyPointUp()
{
}

int CriKeyPointUp::handle_event()
{
	CriKeyPoints &points = gui->plugin->config.points;
	int sz = points.size();
	int hot_point = gui->point_list->get_selection_number(0, 0);

	if( sz > 1 && hot_point > 0 ) {
		CriKeyPoint *&pt0 = points[hot_point];
		CriKeyPoint *&pt1 = points[--hot_point];
		CriKeyPoint *t = pt0;  pt0 = pt1;  pt1 = t;
		gui->point_list->update(hot_point);
	}
	gui->send_configure_change();
	return 1;
}

CriKeyPointDn::CriKeyPointDn(CriKeyWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Dn"))
{
	this->gui = gui;
}
CriKeyPointDn::~CriKeyPointDn()
{
}

int CriKeyPointDn::handle_event()
{
	CriKeyPoints &points = gui->plugin->config.points;
	int sz = points.size();
	int hot_point = gui->point_list->get_selection_number(0, 0);
	if( sz > 1 && hot_point < sz-1 ) {
		CriKeyPoint *&pt0 = points[hot_point];
		CriKeyPoint *&pt1 = points[++hot_point];
		CriKeyPoint *t = pt0;  pt0 = pt1;  pt1 = t;
		gui->point_list->update(hot_point);
	}
	gui->send_configure_change();
	return 1;
}

CriKeyDrag::CriKeyDrag(CriKeyWindow *gui, int x, int y)
 : BC_CheckBox(x, y, gui->plugin->config.drag, _("Drag"))
{
	this->gui = gui;
}
int CriKeyDrag::handle_event()
{
	CWindowGUI *cwindow_gui = gui->plugin->server->mwindow->cwindow->gui;
	int value = get_value();
	if( value ) {
		if( !gui->grab(cwindow_gui) ) {
			update(value = 0);
			flicker(10,50);
		}
	}
	else
		gui->ungrab(cwindow_gui);
	gui->plugin->config.drag = value;
	gui->send_configure_change();
	return 1;
}

CriKeyNewPoint::CriKeyNewPoint(CriKeyWindow *gui, CriKey *plugin, int x, int y)
 : BC_GenericButton(x, y, 80, _("New"))
{
	this->gui = gui;
	this->plugin = plugin;
}
CriKeyNewPoint::~CriKeyNewPoint()
{
}
int CriKeyNewPoint::handle_event()
{
	int k = plugin->new_point();
	gui->point_list->update(k);
	gui->send_configure_change();
	return 1;
}

CriKeyDelPoint::CriKeyDelPoint(CriKeyWindow *gui, CriKey *plugin, int x, int y)
 : BC_GenericButton(x, y, 80, C_("Del"))
{
	this->gui = gui;
	this->plugin = plugin;
}
CriKeyDelPoint::~CriKeyDelPoint()
{
}
int CriKeyDelPoint::handle_event()
{
	int hot_point = gui->point_list->get_selection_number(0, 0);
	CriKeyPoints &points = plugin->config.points;
	if( hot_point >= 0 && hot_point < points.size() ) {
		plugin->config.del_point(hot_point);
		if( !points.size() ) plugin->new_point();
		int sz = points.size();
		if( hot_point >= sz && hot_point > 0 ) --hot_point;
		gui->point_list->update(hot_point);
		gui->send_configure_change();
	}
	return 1;
}

CriKeyReset::CriKeyReset(CriKeyWindow *gui, CriKey *plugin, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->gui = gui;
	this->plugin = plugin;
}
CriKeyReset::~CriKeyReset()
{
}
int CriKeyReset::handle_event()
{
	CriKeyPoints &points = plugin->config.points;
	points.remove_all_objects();
	plugin->new_point();
	gui->point_list->update(0);
	gui->send_configure_change();
	return 1;
}

