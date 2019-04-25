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
#include "tracer.h"
#include "tracerwindow.h"
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

TracerNum::TracerNum(TracerWindow *gui, int x, int y, float output)
 : BC_TumbleTextBox(gui, output, -32767.0f, 32767.0f, x, y, 120)
{
	this->gui = gui;
	set_increment(1);
	set_precision(1);
}

TracerNum::~TracerNum()
{
}

int TracerPointX::handle_event()
{
	if( !TracerNum::handle_event() ) return 0;
	TracerPointList *point_list = gui->point_list;
	int hot_point = point_list->get_selection_number(0, 0);
	TracerPoints &points = gui->plugin->config.points;
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
int TracerPointY::handle_event()
{
	if( !TracerNum::handle_event() ) return 0;
	TracerPointList *point_list = gui->point_list;
	int hot_point = point_list->get_selection_number(0, 0);
	TracerPoints &points = gui->plugin->config.points;
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

TracerWindow::TracerWindow(Tracer *plugin)
 : PluginClientWindow(plugin, 400, 300, 400, 300, 0)
{
	this->plugin = plugin;
	this->title_x = 0;    this->point_x = 0;
	this->title_y = 0;    this->point_y = 0;
	this->new_point = 0;  this->del_point = 0;
	this->point_up = 0;   this->point_dn = 0;
	this->drag = 0;       this->draw = 0;
	this->dragging = 0;
	this->title_r = 0;    this->title_s = 0;
	this->radius = 0;     this->scale = 0;
	this->last_x = 0;     this->last_y = 0;
	this->point_list = 0; this->pending_config = 0;
}

TracerWindow::~TracerWindow()
{
	delete point_x;
	delete point_y;
}

void TracerWindow::create_objects()
{
	int x = 10, y = 10;
	int margin = plugin->get_theme()->widget_border;
	TracerPoint *pt = plugin->config.points[plugin->config.selected];
	add_subwindow(title_x = new BC_Title(x, y, _("X:")));
	int x1 = x + title_x->get_w() + margin;
	point_x = new TracerPointX(this, x1, y, pt->x);
	point_x->create_objects();
	x1 += point_x->get_w() + margin;
	add_subwindow(new_point = new TracerNewPoint(this, plugin, x1, y));
	x1 += new_point->get_w() + margin;
	add_subwindow(point_up = new TracerPointUp(this, x1, y));
	y += point_x->get_h() + margin;
	add_subwindow(title_y = new BC_Title(x, y, _("Y:")));
	x1 = x + title_y->get_w() + margin;
	point_y = new TracerPointY(this, x1, y, pt->y);
	point_y->create_objects();
	x1 += point_y->get_w() + margin;
	add_subwindow(del_point = new TracerDelPoint(this, plugin, x1, y));
	x1 += del_point->get_w() + margin;
	add_subwindow(point_dn = new TracerPointDn(this, x1, y));
	y += point_y->get_h() + margin + 10;

	add_subwindow(drag = new TracerDrag(this, x, y));
	if( plugin->config.drag ) {
		if( !grab(plugin->server->mwindow->cwindow->gui) )
			eprintf("drag enabled, but compositor already grabbed\n");
	}
	x1 = x + drag->get_w() + margin + 20;
	add_subwindow(draw = new TracerDraw(this, x1, y));
	x1 += draw->get_w() + margin + 20;
	add_subwindow(fill = new TracerFill(this, x1, y));
	x1 += drag->get_w() + margin + 20;
	add_subwindow(reset = new TracerReset(this, plugin, x1, y+3));
	y += drag->get_h() + margin;

	add_subwindow(title_r = new BC_Title(x1=x, y, _("Radius:")));
	x1 += title_r->get_w() + margin;
	add_subwindow(radius = new TracerRadius(this, x1, y, 100));
	x1 += radius->get_w() + margin + 20;
	add_subwindow(title_s = new BC_Title(x1, y, _("Scale:")));
	x1 += title_s->get_w() + margin;
	add_subwindow(scale = new TracerScale(this, x1, y, 100));
	y += radius->get_h() + margin + 20;

	add_subwindow(point_list = new TracerPointList(this, plugin, x, y));
	point_list->update(plugin->config.selected);
//	y += point_list->get_h() + 10;

	show_window(1);
}

void TracerWindow::send_configure_change()
{
	pending_config = 0;
	plugin->send_configure_change();
}

int TracerWindow::grab_event(XEvent *event)
{
	int ret = do_grab_event(event);
	if( pending_config && !grab_event_count() )
		send_configure_change();
	return ret;
}

int TracerWindow::do_grab_event(XEvent *event)
{
	switch( event->type ) {
	case ButtonPress: break;
	case ButtonRelease: break;
	case MotionNotify: break;
	default:
		return 0;
	}

	MWindow *mwindow = plugin->server->mwindow;
	CWindowGUI *cwindow_gui = mwindow->cwindow->gui;
	CWindowCanvas *canvas = cwindow_gui->canvas;
	int cx, cy;  cwindow_gui->get_relative_cursor(cx, cy);
	cx -= canvas->view_x;
	cy -= canvas->view_y;

	if( !dragging ) {
		if( cx < 0 || cx >= canvas->view_w ||
		    cy < 0 || cy >= canvas->view_h )
			return 0;
	}

	switch( event->type ) {
	case ButtonPress:
		if( dragging ) return 0;
		dragging = event->xbutton.state & ShiftMask ? -1 : 1;
		break;
	case ButtonRelease:
		if( !dragging ) return 0;
		dragging = 0;
		return 1;
	case MotionNotify:
		if( !dragging ) return 0;
		break;
	default:
		return 0;
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
	TracerPoints &points = plugin->config.points;

	if( dragging > 0 ) {
		switch( event->type ) {
		case ButtonPress: {
			int button_no = event->xbutton.button;
			int hot_point = -1;
			if( button_no == RIGHT_BUTTON ) {
				hot_point = plugin->new_point();
				TracerPoint *pt = points[hot_point];
				pt->x = output_x;  pt->y = output_y;
				point_list->update(hot_point);
				break;
			}
			int sz = points.size();
			if( hot_point < 0 && sz > 0 ) {
				TracerPoint *pt = points[hot_point=0];
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
				TracerPoint *pt = points[hot_point];
				point_list->set_point(hot_point, PT_X, pt->x = output_x);
				point_list->set_point(hot_point, PT_Y, pt->y = output_y);
				point_list->update_list(hot_point);
			}
			break; }
		case MotionNotify: {
			int hot_point = point_list->get_selection_number(0, 0);
			if( hot_point >= 0 && hot_point < points.size() ) {
				TracerPoint *pt = points[hot_point];
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
				TracerPoint *pt = points[i];
				point_list->set_point(i, PT_X, pt->x += dx);
				point_list->set_point(i, PT_Y, pt->y += dy);
			}
			int hot_point = point_list->get_selection_number(0, 0);
			if( hot_point >= 0 && hot_point < sz ) {
				TracerPoint *pt = points[hot_point];
				point_x->update(pt->x);
				point_y->update(pt->y);
				point_list->update_list(hot_point);
			}
			break; }
		}
	}

	last_x = output_x;  last_y = output_y;
	pending_config = 1;
	return 1;
}

void TracerWindow::done_event(int result)
{
	ungrab(client->server->mwindow->cwindow->gui);
}

TracerPointList::TracerPointList(TracerWindow *gui, Tracer *plugin, int x, int y)
 : BC_ListBox(x, y, 360, 130, LISTBOX_TEXT)
{
	this->gui = gui;
	this->plugin = plugin;
	titles[PT_X] = _("X");    widths[PT_X] = 90;
	titles[PT_Y] = _("Y");    widths[PT_Y] = 90;
}
TracerPointList::~TracerPointList()
{
	clear();
}
void TracerPointList::clear()
{
	for( int i=PT_SZ; --i>=0; )
		cols[i].remove_all_objects();
}

int TracerPointList::column_resize_event()
{
	for( int i=PT_SZ; --i>=0; )
		widths[i] = get_column_width(i);
	return 1;
}

int TracerPointList::handle_event()
{
	int hot_point = get_selection_number(0, 0);
	const char *x_text = "", *y_text = "";
	TracerPoints &points = plugin->config.points;

	int sz = points.size();
	if( hot_point >= 0 && sz > 0 ) {
		x_text = gui->point_list->cols[PT_X].get(hot_point)->get_text();
		y_text = gui->point_list->cols[PT_Y].get(hot_point)->get_text();
	}
	gui->point_x->update(x_text);
	gui->point_y->update(y_text);
	update(hot_point);
	gui->send_configure_change();
	return 1;
}

int TracerPointList::selection_changed()
{
	handle_event();
	return 1;
}

void TracerPointList::new_point(const char *xp, const char *yp)
{
	cols[PT_X].append(new BC_ListBoxItem(xp));
	cols[PT_Y].append(new BC_ListBoxItem(yp));
}

void TracerPointList::del_point(int i)
{
	for( int sz1=cols[0].size()-1, c=PT_SZ; --c>=0; )
		cols[c].remove_object_number(sz1-i);
}

void TracerPointList::set_point(int i, int c, float v)
{
	char s[BCSTRLEN]; sprintf(s,"%0.4f",v);
	set_point(i,c,s);
}
void TracerPointList::set_point(int i, int c, const char *cp)
{
	cols[c].get(i)->set_text(cp);
}

int TracerPointList::set_selected(int k)
{
	TracerPoints &points = plugin->config.points;
	int sz = points.size();
	if( !sz ) return -1;
	bclamp(k, 0, sz-1);
	update_selection(&cols[0], k);
	return k;
}
void TracerPointList::update_list(int k)
{
	int xpos = get_xposition(), ypos = get_yposition();
	if( k < 0 ) k = get_selection_number(0, 0);
	update_selection(&cols[0], k);
	BC_ListBox::update(&cols[0], &titles[0],&widths[0],PT_SZ, xpos,ypos,k);
	center_selection();
}
void TracerPointList::update(int k)
{
	clear();
	TracerPoints &points = plugin->config.points;
	int sz = points.size();
	for( int i=0; i<sz; ++i ) {
		TracerPoint *pt = points[i];
		char xtxt[BCSTRLEN];  sprintf(xtxt,"%0.4f", pt->x);
		char ytxt[BCSTRLEN];  sprintf(ytxt,"%0.4f", pt->y);
		new_point(xtxt, ytxt);
	}
	if( k >= 0 && k < sz ) {
		gui->point_x->update(gui->point_list->cols[PT_X].get(k)->get_text());
		gui->point_y->update(gui->point_list->cols[PT_Y].get(k)->get_text());
		plugin->config.selected = k;
	}

	update_list(k);
}

void TracerWindow::update_gui()
{
	TracerConfig &config = plugin->config;
	drag->update(config.drag);
	draw->update(config.draw);
	fill->update(config.fill);
	radius->update(config.radius);
	scale->update(config.scale);
	drag->update(config.drag);
	point_list->update(-1);
}


TracerPointUp::TracerPointUp(TracerWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Up"))
{
	this->gui = gui;
}
TracerPointUp::~TracerPointUp()
{
}

int TracerPointUp::handle_event()
{
	TracerPoints &points = gui->plugin->config.points;
	int sz = points.size();
	int hot_point = gui->point_list->get_selection_number(0, 0);

	if( sz > 1 && hot_point > 0 ) {
		TracerPoint *&pt0 = points[hot_point];
		TracerPoint *&pt1 = points[--hot_point];
		TracerPoint *t = pt0;  pt0 = pt1;  pt1 = t;
		gui->point_list->update(hot_point);
	}
	gui->send_configure_change();
	return 1;
}

TracerPointDn::TracerPointDn(TracerWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Dn"))
{
	this->gui = gui;
}
TracerPointDn::~TracerPointDn()
{
}

int TracerPointDn::handle_event()
{
	TracerPoints &points = gui->plugin->config.points;
	int sz = points.size();
	int hot_point = gui->point_list->get_selection_number(0, 0);
	if( sz > 1 && hot_point < sz-1 ) {
		TracerPoint *&pt0 = points[hot_point];
		TracerPoint *&pt1 = points[++hot_point];
		TracerPoint *t = pt0;  pt0 = pt1;  pt1 = t;
		gui->point_list->update(hot_point);
	}
	gui->send_configure_change();
	return 1;
}

TracerDrag::TracerDrag(TracerWindow *gui, int x, int y)
 : BC_CheckBox(x, y, gui->plugin->config.drag, _("Drag"))
{
	this->gui = gui;
}
int TracerDrag::handle_event()
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

TracerDraw::TracerDraw(TracerWindow *gui, int x, int y)
 : BC_CheckBox(x, y, gui->plugin->config.draw, _("Draw"))
{
	this->gui = gui;
}
int TracerDraw::handle_event()
{
	gui->plugin->config.draw = get_value();
	gui->send_configure_change();
	return 1;
}

TracerFill::TracerFill(TracerWindow *gui, int x, int y)
 : BC_CheckBox(x, y, gui->plugin->config.fill, _("Fill"))
{
	this->gui = gui;
}
int TracerFill::handle_event()
{
	gui->plugin->config.fill = get_value();
	gui->send_configure_change();
	return 1;
}

TracerRadius::TracerRadius(TracerWindow *gui, int x, int y, int w)
 : BC_ISlider(x,y,0,w,w, -50,50, gui->plugin->config.radius)
{
	this->gui = gui;
}
int TracerRadius::handle_event()
{
	gui->plugin->config.radius = get_value();
	gui->send_configure_change();
	return 1;
}

TracerScale::TracerScale(TracerWindow *gui, int x, int y, int w)
 : BC_FSlider(x,y, 0,w,w, 1.f,10.f, gui->plugin->config.scale)
{
	this->gui = gui;
}
int TracerScale::handle_event()
{
	gui->plugin->config.scale = get_value();
	gui->send_configure_change();
	return 1;
}

TracerNewPoint::TracerNewPoint(TracerWindow *gui, Tracer *plugin, int x, int y)
 : BC_GenericButton(x, y, 80, _("New"))
{
	this->gui = gui;
	this->plugin = plugin;
}
TracerNewPoint::~TracerNewPoint()
{
}
int TracerNewPoint::handle_event()
{
	int k = plugin->new_point();
	gui->point_list->update(k);
	gui->send_configure_change();
	return 1;
}

TracerDelPoint::TracerDelPoint(TracerWindow *gui, Tracer *plugin, int x, int y)
 : BC_GenericButton(x, y, 80, C_("Del"))
{
	this->gui = gui;
	this->plugin = plugin;
}
TracerDelPoint::~TracerDelPoint()
{
}
int TracerDelPoint::handle_event()
{
	int hot_point = gui->point_list->get_selection_number(0, 0);
	TracerPoints &points = plugin->config.points;
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

TracerReset::TracerReset(TracerWindow *gui, Tracer *plugin, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->gui = gui;
	this->plugin = plugin;
}
TracerReset::~TracerReset()
{
}
int TracerReset::handle_event()
{
	TracerConfig &config = plugin->config;
	config.drag = 0;
	config.draw = 0;
	config.fill = 0;
	config.radius = 0;
	config.scale = 1;
	config.selected = 0;
	TracerPoints &points = plugin->config.points;
	points.remove_all_objects();
	plugin->new_point();
	gui->point_list->update(0);
	gui->update_gui();
	gui->send_configure_change();
	return 1;
}

