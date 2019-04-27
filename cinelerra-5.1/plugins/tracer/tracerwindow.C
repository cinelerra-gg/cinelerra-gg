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
 : PluginClientWindow(plugin, 400, 420, 400, 420, 0)
{
	this->plugin = plugin;
	this->title_x = 0;    this->point_x = 0;
	this->title_y = 0;    this->point_y = 0;
	this->new_point = 0;  this->del_point = 0;
	this->point_up = 0;   this->point_dn = 0;
	this->drag = 0;       this->draw = 0;
	this->button_no = 0;  this->invert = 0;
	this->title_r = 0;    this->title_s = 0;
	this->feather = 0;    this->radius = 0;
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
	int hot_point = plugin->config.selected;
	add_subwindow(title_x = new BC_Title(x, y, _("X:")));
	int x1 = x + title_x->get_w() + margin;
	TracerPoint *pt = hot_point >= 0 ? plugin->config.points[hot_point] : 0;
	point_x = new TracerPointX(this, x1, y, !pt ? 0 : pt->x);
	point_x->create_objects();
	x1 += point_x->get_w() + margin;
	add_subwindow(new_point = new TracerNewPoint(this, plugin, x1, y));
	x1 += new_point->get_w() + margin;
	add_subwindow(point_up = new TracerPointUp(this, x1, y));
	y += point_x->get_h() + margin;
	add_subwindow(title_y = new BC_Title(x, y, _("Y:")));
	x1 = x + title_y->get_w() + margin;
	point_y = new TracerPointY(this, x1, y, !pt ? 0 : pt->y);
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
	int y1 = y + 3;
	add_subwindow(reset = new TracerReset(this, plugin, x1, y1));
	y1 += reset->get_h() + margin;
	add_subwindow(invert = new TracerInvert(this, plugin, x1, y1));
	y += drag->get_h() + margin + 15;

	x1 = x + 80;
	add_subwindow(title_r = new BC_Title(x, y, _("Feather:")));
	add_subwindow(feather = new TracerFeather(this, x1, y, 150));
	y += feather->get_h() + margin;
	add_subwindow(title_s = new BC_Title(x, y, _("Radius:")));
	add_subwindow(radius = new TracerRadius(this, x1, y, 150));
	y += radius->get_h() + margin + 5;

	add_subwindow(point_list = new TracerPointList(this, plugin, x, y));
	point_list->update(plugin->config.selected);
	y += point_list->get_h() + 10;

	add_subwindow(new BC_Title(x, y, _(
		"Btn1: select/drag point\n"
		"Btn2: drag all points\n"
		"Btn3: add point on nearest line\n"
		"Btn3: shift: append point to end\n"
		"Wheel: rotate, centered on cursor\n"
		"Wheel: shift: scale, centered on cursor\n")));
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

	if( !button_no ) {
		if( cx < 0 || cx >= canvas->view_w ||
		    cy < 0 || cy >= canvas->view_h )
			return 0;
	}

	switch( event->type ) {
	case ButtonPress:
		if( button_no ) return 0;
		button_no = event->xbutton.button;
		break;
	case ButtonRelease:
		if( !button_no ) return 0;
		button_no = 0;
		return 1;
	case MotionNotify:
		if( !button_no ) return 0;
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

	switch( event->type ) {
	case ButtonPress: {
		float s = 1.02;
		float th = M_PI/360.f; // .5 deg per wheel_btn
		int shift_down = event->xbutton.state & ShiftMask;
		switch( button_no ) {
		case WHEEL_DOWN:
			s = 0.98;
			th = -th;  // fall thru
		case WHEEL_UP: {
			// shift_down scale, !shift_down rotate
			float st = sin(th), ct = cos(th);
			int sz = points.size();
			for( int i=0; i<sz; ++i ) {
				TracerPoint *pt = points[i];
				float px = pt->x - output_x, py = pt->y - output_y;
				float nx = shift_down ? px*s : px*ct + py*st;
				float ny = shift_down ? py*s : py*ct - px*st;
				point_list->set_point(i, PT_X, pt->x = nx + output_x);
				point_list->set_point(i, PT_Y, pt->y = ny + output_y);
			}
			point_list->update(-1);
			button_no = 0;
			break; }
		case RIGHT_BUTTON: {
			// shift_down adds to end
			int sz = !shift_down ? points.size() : 0;
			int k = !shift_down ? -1 : points.size()-1;
			float mx = FLT_MAX;
			for( int i=0; i<sz; ++i ) {
				// pt on line pt[i+0]..pt[i+1] nearest cx,cy
				TracerPoint *pt0 = points[i+0];
				TracerPoint *pt1 = i+1<sz ? points[i+1] : points[0];
				float x0 = pt0->x, y0 = pt0->y;
				float x1 = pt1->x, y1 = pt1->y;
				float dx = x1-x0, dy = y1-y0;
				float rr = dx*dx + dy*dy;
				if( !rr ) continue;
				float u = ((x1-output_x)*dx + (y1-output_y)*dy) / rr;
				if( u < 0 || u > 1 ) continue;  // past endpts
				float x = x0*u + x1*(1-u);
				float y = y0*u + y1*(1-u);
				dx = output_x-x;  dy = output_y-y;
				float dd = dx*dx + dy*dy;	// d**2 closest approach
				if( mx > dd ) { mx = dd;  k = i; }
			}
			TracerPoint *pt = points[sz=plugin->new_point()];
			int hot_point = k+1;
			for( int i=sz; i>hot_point; --i ) points[i] = points[i-1];
			points[hot_point] = pt;
			pt->x = output_x;  pt->y = output_y;
			point_list->update(hot_point);
			break; }
		case LEFT_BUTTON: {
			int hot_point = -1, sz = points.size();
			if( sz > 0 ) {
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
		}
		break; }
	case MotionNotify: {
		switch( button_no ) {
		case LEFT_BUTTON: {
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
		case MIDDLE_BUTTON: {
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
		break; }
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
	int sz = plugin->config.points.size();
	if( k < 0 || k >= sz ) k = -1;
	plugin->config.selected = k;
	update_selection(&cols[0], k);
	int xpos = get_xposition(), ypos = get_yposition();
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
	}
	update_list(k);
}

void TracerWindow::update_gui()
{
	TracerConfig &config = plugin->config;
	drag->update(config.drag);
	draw->update(config.draw);
	fill->update(config.fill);
	feather->update(config.feather);
	radius->update(config.radius);
	invert->update(config.invert);
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

TracerFeather::TracerFeather(TracerWindow *gui, int x, int y, int w)
 : BC_ISlider(x,y,0,w,w, -50,50, gui->plugin->config.feather)
{
	this->gui = gui;
}
int TracerFeather::handle_event()
{
	gui->plugin->config.feather = get_value();
	gui->send_configure_change();
	return 1;
}

TracerRadius::TracerRadius(TracerWindow *gui, int x, int y, int w)
 : BC_FSlider(x,y, 0,w,w, -5.f,5.f, gui->plugin->config.radius)
{
	this->gui = gui;
}
int TracerRadius::handle_event()
{
	gui->plugin->config.radius = get_value();
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
		gui->point_list->update(--hot_point);
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
	if( !config.drag ) {
		MWindow *mwindow = plugin->server->mwindow;
		CWindowGUI *cwindow_gui = mwindow->cwindow->gui;
		if( gui->grab(cwindow_gui) )
			config.drag = 1;
		else
			gui->drag->flicker(10,50);
	}
	config.draw = 1;
	config.fill = 0;
	config.invert = 0;
	config.feather = 0;
	config.radius = 1;
	config.selected = -1;
	TracerPoints &points = plugin->config.points;
	points.remove_all_objects();
	gui->point_list->update(-1);
	gui->update_gui();
	gui->send_configure_change();
	return 1;
}

TracerInvert::TracerInvert(TracerWindow *gui, Tracer *plugin, int x, int y)
 : BC_CheckBox(x, y, gui->plugin->config.invert, _("Invert"))
{
	this->gui = gui;
	this->plugin = plugin;
}
TracerInvert::~TracerInvert()
{
}
int TracerInvert::handle_event()
{
	plugin->config.invert = get_value();
	gui->send_configure_change();
	return 1;
}

