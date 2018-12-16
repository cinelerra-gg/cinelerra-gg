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
#include "sketcher.h"
#include "sketcherwindow.h"
#include "cstrdup.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "keys.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginserver.h"
#include "theme.h"
#include "track.h"

#define AltMask Mod1Mask

#define COLOR_W 40
#define COLOR_H 24

const char *SketcherPoint::types[] = {
	N_("off"),
	N_("line"),
	N_("curve"),
	N_("fill"),
};
const char *SketcherCurve::pens[] = {
	N_("off"),
	N_("box"),
	N_("+"),
	N_("/"),
	N_("X"),
};


SketcherCurvePenItem::SketcherCurvePenItem(int pen)
 : BC_MenuItem(_(SketcherCurve::pens[pen]))
{
	this->pen = pen;
}
int SketcherCurvePenItem::handle_event()
{
	SketcherCurvePen *popup = (SketcherCurvePen*)get_popup_menu();
	popup->update(pen);
	SketcherWindow *gui = popup->gui;
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci >= 0 && ci < config.curves.size() ) {
		SketcherCurve *cv = config.curves[ci];
		cv->pen = pen;
		gui->curve_list->update(ci);
		gui->send_configure_change();
	}
	return 1;
}

SketcherCurvePen::SketcherCurvePen(SketcherWindow *gui, int x, int y, int pen)
 : BC_PopupMenu(x,y,72,_(cv_pen[pen]))
{
	this->gui = gui;
	this->pen = pen;
}
void SketcherCurvePen::create_objects()
{
	int n = sizeof(cv_pen)/sizeof(cv_pen[0]);
	for( int pen=0; pen<n; ++pen )
		add_item(new SketcherCurvePenItem(pen));
}
void SketcherCurvePen::update(int pen)
{
	set_text(_(cv_pen[this->pen=pen]));
}


SketcherCurveColor::SketcherCurveColor(SketcherWindow *gui,
		int x, int y, int w, int h, int color, int alpha)
 : ColorBoxButton(_("Curve Color"), x, y, w, h, color, alpha, 1)
{
	this->gui = gui;
	this->color = CV_COLOR;
	for( int i=0; i<3; ++i ) {
		vframes[i] = new VFrame(w, h, BC_RGB888);
		vframes[i]->clear_frame();
	}
}

SketcherCurveColor::~SketcherCurveColor()
{
	for( int i=0; i<3; ++i )
		delete vframes[i];
}

void SketcherCurveColor::handle_done_event(int result)
{
	if( result ) color = orig_color | (~orig_alpha<<24);
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci >= 0 && ci < config.curves.size() ) {
		SketcherCurve *cv = config.curves[ci];
		cv->color = color;
		gui->curve_list->update(ci);
		gui->send_configure_change();
	}
}

int SketcherCurveColor::handle_new_color(int color, int alpha)
{
	color |= ~alpha<<24;  this->color = color;
	gui->lock_window("SketcherCurveColor::update_gui");
	update_gui(color);
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci >= 0 ) {
		SketcherCurve *cv = config.curves[ci];
		cv->color = color;
		gui->curve_list->update(ci);
		gui->send_configure_change();
	}
	gui->unlock_window();
	return 1;
}


SketcherCoord::SketcherCoord(SketcherWindow *gui, int x, int y,
		coord output, coord mn, coord mx)
 : BC_TumbleTextBox(gui, output, mn, mx, x, y, 64, 1)
{
	this->gui = gui;
	set_increment(1);
}
SketcherCoord::~SketcherCoord()
{
}

SketcherNum::SketcherNum(SketcherWindow *gui, int x, int y,
		int output, int mn, int mx)
 : BC_TumbleTextBox(gui, output, mn, mx, x, y, 54)
{
	this->gui = gui;
	set_increment(1);
}
SketcherNum::~SketcherNum()
{
}

int SketcherPointX::handle_event()
{
	if( !SketcherCoord::handle_event() ) return 0;
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci >= 0 && ci < config.curves.size() ) {
		SketcherCurve *cv = config.curves[ci];
		SketcherPointList *point_list = gui->point_list;
		int pi = config.pt_selected;
		SketcherPoints &points = cv->points;
		if( pi >= 0 && pi < points.size() ) {
			coord v = atof(get_text());
			points[pi]->x = v;
			point_list->set_point(pi, PT_X, v);
			point_list->update_list(pi);
			gui->send_configure_change();
		}
	}
	return 1;
}
int SketcherPointY::handle_event()
{
	if( !SketcherCoord::handle_event() ) return 0;
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci >= 0 && ci < config.curves.size() ) {
		SketcherCurve *cv = config.curves[ci];
		SketcherPointList *point_list = gui->point_list;
		int pi = config.pt_selected;
		SketcherPoints &points = cv->points;
		if( pi >= 0 && pi < points.size() ) {
			coord v = atof(get_text());
			points[pi]->y = v;
			point_list->set_point(pi, PT_Y, v);
			point_list->update_list(pi);
			gui->send_configure_change();
		}
	}
	return 1;
}

int SketcherPointId::handle_event()
{
	if( !SketcherNum::handle_event() ) return 0;
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci >= 0 && ci < config.curves.size() ) {
		SketcherCurve *cv = config.curves[ci];
		SketcherPointList *point_list = gui->point_list;
		int pi = config.pt_selected;
		SketcherPoints &points = cv->points;
		if( pi >= 0 && pi < points.size() ) {
			int id = atoi(get_text());
			points[pi]->id = id;
			point_list->set_point(pi, PT_ID, id);
			point_list->update_list(pi);
			gui->send_configure_change();
		}
	}
	return 1;
}

SketcherCurveWidth::SketcherCurveWidth(SketcherWindow *gui, int x, int y, int width)
 : SketcherNum(gui, x, y, width, 0, 255)
{
	this->width = width;
}
SketcherCurveWidth::~SketcherCurveWidth()
{
}

int SketcherCurveWidth::handle_event()
{
	if( !SketcherNum::handle_event() ) return 0;
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci >= 0 && ci < config.curves.size() ) {
		SketcherCurve *cv = config.curves[ci];
		int v = atoi(get_text());
		cv->width = v;
		gui->curve_list->update(ci);
		gui->send_configure_change();
	}
	return 1;
}

void SketcherCurveWidth::update(int width)
{
	SketcherNum::update(this->width=width);
}


SketcherWindow::SketcherWindow(Sketcher *plugin)
 : PluginClientWindow(plugin, 380, 620, 380, 620, 0)
{
	this->plugin = plugin;
	this->title_pen = 0;  this->curve_pen = 0;
	this->title_color = 0; this->curve_color = 0;
	this->drag = 0;
	this->new_curve = 0;  this->del_curve = 0;
	this->curve_up = 0;   this->curve_dn = 0;
	this->title_x = 0;    this->point_x = 0;
	this->title_y = 0;    this->point_y = 0;
	this->title_id = 0;   this->point_id = 0;
	this->new_point = 0;  this->del_point = 0;
	this->point_up = 0;   this->point_dn = 0;
	this->point_list = 0; this->notes0 = 0;
	this->notes1 = 0;     this->notes2 = 0;

	position = -1;
	track_w = track_h -1;
	cursor_x = cursor_y = -1;
	output_x = output_y = -1;
	last_x = last_y = -1;
	projector_x = projector_y = projector_z = -1;
	state = 0;  dragging = 0;
	new_points = 0;
	pending_motion = 0;
	pending_config = 0;
}

SketcherWindow::~SketcherWindow()
{
	delete curve_width;
	delete point_x;
	delete point_y;
}

void SketcherWindow::create_objects()
{
	int x = 10, y = 10, dy = 0, x1, y1;
	int margin = plugin->get_theme()->widget_border;
	BC_Title *title;
	int ci = plugin->config.cv_selected;
	if( ci < 0 || ci >= plugin->config.curves.size() )
		ci = plugin->new_curve();
	SketcherCurve *cv = plugin->config.curves[ci];

	reset_curves = new SketcherResetCurves(this, plugin, x1=x, y+3);
	add_subwindow(reset_curves);	dy = bmax(dy,reset_curves->get_h());
	x1 += reset_curves->get_w() + 2*margin;
	const char *curve_text = _("Curve");
	title_width = new BC_Title(x1, y, _("Width:"));
	add_subwindow(title_width);	dy = bmax(dy,title_width->get_h());
	x1 += title_width->get_w() + margin;
	curve_width = new SketcherCurveWidth(this, x1, y, cv->width);
	curve_width->create_objects();
	y += dy + 2*margin;		dy = 0;

	x1 = get_w()-x - BC_Title::calculate_w(this, curve_text, LARGEFONT);
	y1 = y-margin - BC_Title::calculate_h(this, curve_text, LARGEFONT);
	title = new BC_Title(x1, y1, curve_text, LARGEFONT,
		get_resources()->menu_highlighted_fontcolor);
	add_subwindow(title);		dy = bmax(dy,title->get_h());
	curve_list = new SketcherCurveList(this, plugin, x, y);
	add_subwindow(curve_list);	dy = bmax(dy,curve_list->get_h());
	y += dy + margin;		dy = 0;

	new_curve = new SketcherNewCurve(this, plugin, x1=x, y);
	add_subwindow(new_curve);	dy = bmax(dy,new_curve->get_h());
	x1 += new_curve->get_w() + margin;
	curve_up = new SketcherCurveUp(this, x1, y);
	add_subwindow(curve_up);	dy = bmax(dy,curve_up->get_h());
	x1 += curve_up->get_w() + 4*margin;
	y1 = BC_Title::calculate_h(this, _("Pen:"));
	title_pen = new BC_Title(x1+30, y+dy-y1, _("Pen:"));
	add_subwindow(title_pen);	dy = bmax(dy,title_pen->get_h());
	int x2 = (get_w()+x1)/2 + 20;
	y1 = BC_Title::calculate_h(this, _("Color:"));
	title_color = new BC_Title(x2, y+dy-y1, _("Color:"));
	add_subwindow(title_color);	dy = bmax(dy,title_color->get_h());
	y += dy + margin;		dy = 0;

	del_curve = new SketcherDelCurve(this, plugin, x1=x, y);
	add_subwindow(del_curve);	dy = bmax(dy,del_curve->get_h());
	x1 += del_curve->get_w() + margin;
	curve_dn = new SketcherCurveDn(this, x1, y);
	add_subwindow(curve_dn);	dy = bmax(dy,curve_dn->get_h());
	x1 += curve_dn->get_w() + 4*margin;
	curve_pen = new SketcherCurvePen(this, x1, y, cv->pen);
	add_subwindow(curve_pen);	dy = bmax(dy,curve_pen->get_h());
	curve_pen->create_objects();
	curve_color = new SketcherCurveColor(this, x2, y, COLOR_W, COLOR_H,
		cv->color&0xffffff, (~cv->color>>24)&0xff);
	add_subwindow(curve_color);	dy = bmax(dy,curve_color->get_h());
	y += dy + margin;  dy = 0;
	curve_list->update(ci);

	BC_Bar *bar;
	bar = new BC_Bar(x, y, get_w()-2*x);
	add_subwindow(bar);		dy = bmax(dy,bar->get_h());
	bar = new BC_Bar(x, y+=dy, get_w()-2*x);
	add_subwindow(bar);		dy = bmax(dy,bar->get_h());
	y += dy + 2*margin;

	int pi = plugin->config.pt_selected;
	SketcherPoint *pt = pi >= 0 && pi < cv->points.size() ? cv->points[pi] : 0;
	reset_points = new SketcherResetPoints(this, plugin, x1=x, y+3);
	add_subwindow(reset_points);	dy = bmax(dy,reset_points->get_h());
	x1 += reset_points->get_w() + 2*margin; 
	if( plugin->config.drag ) {
		if( !grab(plugin->server->mwindow->cwindow->gui) ) {
			eprintf("drag enabled, but compositor already grabbed\n");
			plugin->config.drag = 0;
		}
	}
	drag = new SketcherDrag(this, x1, y);
	add_subwindow(drag);		dy = bmax(dy,drag->get_h());
	x1 += drag->get_w() + 2*margin;
	int arc = pt ? pt->arc : ARC_LINE;
	point_type = new SketcherPointType(this, x1, y, arc);
	add_subwindow(point_type);	dy = bmax(dy,point_type->get_h());
	point_type->create_objects();
	y += dy + margin;  dy = 0;

	const char *point_text = _("Point");
	x1 = get_w()-x - BC_Title::calculate_w(this, point_text, LARGEFONT);
	y1 = y-margin - BC_Title::calculate_h(this, point_text, LARGEFONT);
	add_subwindow(title = new BC_Title(x1, y1, point_text, LARGEFONT,
		get_resources()->menu_highlighted_fontcolor));
	point_list = new SketcherPointList(this, plugin, x, y);
	add_subwindow(point_list);	dy = bmax(dy,point_list->get_h());
	y += dy + margin;		dy = 0;

	new_point = new SketcherNewPoint(this, plugin, x1=x, y);
	add_subwindow(new_point);	dy = bmax(dy,new_point->get_h());
	x1 += new_point->get_w() + margin;
	point_up = new SketcherPointUp(this, x1, y);
	add_subwindow(point_up);	dy = bmax(dy,point_up->get_h());
	x1 += point_up->get_w() + 2*margin;
	title_x = new BC_Title(x1, y, _("X:"));
	add_subwindow(title_x);		dy = bmax(dy,title_x->get_h());
	x1 += title_x->get_w() + margin;
	point_x = new SketcherPointX(this, x1, y, !pt ? 0.f : pt->x);
	point_x->create_objects();	dy = bmax(dy, point_x->get_h());
	x2 = x1 + point_x->get_w() + 2*margin + 10;
	y1 = BC_Title::calculate_h(this, _("ID:"));
	title_id = new BC_Title(x2+16, y+dy-y1, _("ID:"));
	add_subwindow(title_id);	dy = bmax(dy, title_id->get_h());
	y += dy + margin;  dy = 0;

	del_point = new SketcherDelPoint(this, plugin, x1=x, y);
	add_subwindow(del_point);	dy = bmax(dy,del_point->get_h());
	x1 += del_point->get_w() + margin;
	point_dn = new SketcherPointDn(this, x1, y);
	add_subwindow(point_dn);	dy = bmax(dy,point_dn->get_h());
	x1 += point_dn->get_w() + 2*margin;
	title_y = new BC_Title(x1, y, _("Y:"));
	add_subwindow(title_y);		dy = bmax(dy,title_y->get_h());
	x1 += title_y->get_w() + margin;
	point_y = new SketcherPointY(this, x1, y, !pt ? 0.f : pt->y);
	point_y->create_objects();	dy = bmax(dy, point_y->get_h());
	point_id = new SketcherPointId(this, x2, y, !pt ? 0 : pt->id);
	point_id->create_objects();	dy = bmax(dy, point_id->get_h());
	y += dy + margin + 5;		dy = 0;
	point_list->update(pi);

	bar = new BC_Bar(x, y, get_w()-2*x);
	add_subwindow(bar);		dy = bmax(dy,bar->get_h());
	y += dy + 2*margin;

	add_subwindow(notes0 = new BC_Title(x, y,
		 _("\n"
		   "Shift=\n"
		   "None=\n"
		   "Ctrl=\n"
		   "Alt=\n"
		   "Ctrl+Shift=")));	dy = bmax(dy, notes0->get_h());
	add_subwindow(notes1 = new BC_Title(x+100, y,
		 _("     LMB\n"
		   "new line point\n"
		   "select point\n"
		   "drag point\n"
		   "drag all curves\n"
		   "new fill point"))); dy = bmax(dy, notes1->get_h());
	add_subwindow(notes2 = new BC_Title(x+220, y,
		 _("      RMB\n"
		   "new arc point\n"
		   "select curve\n"
		   "drag curve\n"
		   "new curve\n"
		   "new off point"))); dy = bmax(dy, notes2->get_h());
	y += dy + margin + 10;

	add_subwindow(notes3 = new BC_Title(x, y,
		   "Key DEL= delete point, +Shift= delete curve\n"));
	show_window(1);
}

void SketcherWindow::done_event(int result)
{
}

void SketcherWindow::send_configure_change()
{
	pending_config = 0;
	plugin->send_configure_change();
}


int SketcherWindow::grab_event(XEvent *event)
{
	int ret = do_grab_event(event);
	if( !grab_event_count() ) {
		if( grab_cursor_motion() )
			pending_config = 1;
		if( pending_config ) {
			last_x = output_x;  last_y = output_y;
			send_configure_change();
		}
	}
	return ret;
}

int SketcherWindow::do_grab_event(XEvent *event)
{
	switch( event->type ) {
	case ButtonPress: break;
	case ButtonRelease: break;
	case MotionNotify: break;
	case KeyPress:
		if( keysym_lookup(event) > 0 ) {
			switch( get_keysym() ) {
			case XK_Delete:
				pending_config = 1;
				return (event->xkey.state & ShiftMask) ?
					del_curve->handle_event() :
					del_point->handle_event() ;
			}
		} // fall thru
	default:
		return 0;
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
			return 0;
	}

	switch( event->type ) {
	case ButtonPress:
		if( dragging ) return 0;
		dragging = 1;
		break;
	case ButtonRelease:
		dragging = 0;
		break;
	case MotionNotify:
		if( dragging ) break;
	default: // fall thru
		return 0;
	}

	cursor_x = cx, cursor_y = cy;
	canvas->canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);
	position = plugin->get_source_position();
	Track *track = plugin->server->plugin->track;
	track_w = track->track_w;
	track_h = track->track_h;
	track->automation->get_projector(
		&projector_x, &projector_y, &projector_z,
		position, PLAY_FORWARD);
	projector_x += mwindow->edl->session->output_w / 2;
	projector_y += mwindow->edl->session->output_h / 2;
	output_x = (cursor_x - projector_x) / projector_z + track_w / 2;
	output_y = (cursor_y - projector_y) / projector_z + track_h / 2;
	state = event->xmotion.state;

	if( event->type == MotionNotify ) {
		memcpy(&motion_event, event, sizeof(motion_event));
		pending_motion = 1;
		return 1;
	}
	if( grab_cursor_motion() )
		pending_config = 1;

	switch( event->type ) {
	case ButtonPress:
		pending_config = grab_button_press(event);
		break;
	case ButtonRelease:
		new_points = 0;
		break;
	}

	return 1;
}

int SketcherWindow::grab_button_press(XEvent *event)
{
	SketcherConfig &config = plugin->config;
	int ci = config.cv_selected;
	if( ci < 0 || ci >= plugin->config.curves.size() )
		return 0;
	SketcherCurves &curves = config.curves;
	SketcherCurve *cv = curves[ci];
	SketcherPoints &points = cv->points;
	int pi = config.pt_selected;

	int button_no = event->xbutton.button;
	switch( button_no ) {
	case LEFT_BUTTON: {
		if( (state & ShiftMask) ) { // create new point/string
			++new_points;
			pi = plugin->new_point(cv,
				!(state & ControlMask) ? ARC_LINE : ARC_FILL,
				output_x, output_y, pi+1);
			point_list->update(pi);
			break;
		}
		SketcherPoint *pt = 0; // select point
		double dist = cv->nearest_point(pi, output_x,output_y);
		if( dist >= 0 ) {
			pt = points[pi];
			Track *track = plugin->server->plugin->track;
			int track_w = track->track_w, track_h = track->track_h;
			float px = (pt->x - track_w / 2) * projector_z + projector_x;
			float py = (pt->y - track_h / 2) * projector_z + projector_y;
			float pix = DISTANCE(px, py, cursor_x,cursor_y);
			if( (state & ControlMask) && pix >= HANDLE_W ) { pi = -1;  pt = 0; }
		}
		point_list->set_selected(pi);
		break; }
	case RIGHT_BUTTON: {
		if( (state & ShiftMask) ) { // create new curve point
			++new_points;
			pi = plugin->new_point(cv,
				!(state & ControlMask) ? ARC_CURVE : ARC_OFF,
				output_x, output_y, pi+1);
			point_list->update(pi);
			break;
		}
		if( (state & AltMask) ) { // create new curve
			ci = plugin->new_curve(cv->pen, cv->width, curve_color->color);
			curve_list->update(ci);
			point_list->update(-1);
			break;
		}
		SketcherPoint *pt = 0;
		double dist = config.nearest_point(ci, pi, output_x,output_y);
		if( dist >= 0 ) {
			pt = curves[ci]->points[pi];
			Track *track = plugin->server->plugin->track;
			int track_w = track->track_w, track_h = track->track_h;
			float px = (pt->x - track_w / 2) * projector_z + projector_x;
			float py = (pt->y - track_h / 2) * projector_z + projector_y;
			float pix = DISTANCE(px, py, cursor_x,cursor_y);
			if( (state & ControlMask) && pix >= HANDLE_W ) { ci = pi = -1;  pt = 0; }
		}
		if( pt ) {
			curve_list->update(ci);
			point_list->update(pi);
		}
		break; }
	}
	return 1;
}

int SketcherWindow::grab_cursor_motion()
{
	if( !pending_motion )
		return 0;
	pending_motion = 0;
	SketcherConfig &config = plugin->config;
	int ci = config.cv_selected;
	if( ci < 0 || ci >= plugin->config.curves.size() )
		return 0;
	SketcherCurves &curves = config.curves;
	SketcherCurve *cv = curves[ci];
	SketcherPoints &points = cv->points;
	int pi = config.pt_selected;

	if( (state & ShiftMask) ) {  // string of points
		if( (state & (Button1Mask|Button3Mask)) ) {
			SketcherPoint *pt = pi >= 0 && pi < points.size() ? points[pi] : 0;
			if( pt ) {
				float dist = DISTANCE(pt->x, pt->y, output_x, output_y);
				if( dist < get_w()*0.1 ) return 0; // tolerance w/10
			}
			++new_points;
			int arc = (state & Button1Mask) ? ARC_LINE : ARC_CURVE;
			pi = plugin->new_point(cv, arc, output_x, output_y, pi+1);
			point_list->update(pi);
		}
		return 1;
	}
	if( (state & Button1Mask) ) {
		if( (state & ControlMask) ) { // drag selected point
			SketcherPoint *pt = pi >= 0 && pi < points.size() ? points[pi] : 0;
			if( pt ) {
				point_list->set_point(pi, PT_X, pt->x = output_x);
				point_list->set_point(pi, PT_Y, pt->y = output_y);
				point_list->update_list(pi);
				point_x->update(pt->x);
				point_y->update(pt->y);
			}
			return 1;
		}
		if( (state & AltMask) ) { // drag all curves
			int dx = round(output_x - last_x);
			int dy = round(output_y - last_y);
			for( int i=0; i<curves.size(); ++i ) {
				SketcherCurve *crv = plugin->config.curves[i];
				int pts = crv->points.size();
				for( int k=0; k<pts; ++k ) {
					SketcherPoint *pt = crv->points[k];
					pt->x += dx;  pt->y += dy;
				}
			}
			SketcherPoint *pt = pi >= 0 && pi < points.size() ?
				points[pi] : 0;
			point_x->update(pt ? pt->x : 0.f);
			point_y->update(pt ? pt->y : 0.f);
			point_id->update(pt ? pt->id : 0);
			point_list->update(pi);
			return 1;
		}
		double dist = cv->nearest_point(pi, output_x,output_y);
		if( dist >= 0 )
			point_list->set_selected(pi);
		return 1;
	}
	if( (state & Button3Mask) ) {
		if( (state & (ControlMask | AltMask)) ) { // drag selected curve(s)
			int dx = round(output_x - last_x);
			int dy = round(output_y - last_y);
			for( int i=0; i<points.size(); ++i ) {
				SketcherPoint *pt = points[i];
				pt->x += dx;  pt->y += dy;
			}
			SketcherPoint *pt = pi >= 0 && pi < points.size() ?
				points[pi] : 0;
			point_x->update(pt ? pt->x : 0.f);
			point_y->update(pt ? pt->y : 0.f);
			point_id->update(pt ? pt->id : 0);
			point_list->update(pi);
			return 1;
		}
		double dist = config.nearest_point(ci, pi, output_x,output_y);
		if( dist >= 0 ) {
			curve_list->update(ci);
			point_list->update(pi);
		}
		return 1;
	}
	return 0;
}

int SketcherWindow::keypress_event()
{
	int key = get_keypress();
	switch( key ) {
	case DELETE: return shift_down() ?
			del_curve->handle_event() :
			del_point->handle_event() ;
	}
	return 0;
}


SketcherCurveList::SketcherCurveList(SketcherWindow *gui, Sketcher *plugin, int x, int y)
 : BC_ListBox(x, y, 360, 130, LISTBOX_TEXT)
{
	this->gui = gui;
	this->plugin = plugin;
	col_titles[CV_ID] = _("ID");      col_widths[CV_ID] = 64;
	col_titles[CV_RAD] = _("width");  col_widths[CV_RAD] = 64;
	col_titles[CV_PEN] = _("pen");    col_widths[CV_PEN] = 64;
	col_titles[CV_CLR] = _("color");  col_widths[CV_CLR] = 80;
	col_titles[CV_ALP] = _("alpha");  col_widths[CV_ALP] = 64;
}
SketcherCurveList::~SketcherCurveList()
{
	clear();
}
void SketcherCurveList::clear()
{
	for( int i=CV_SZ; --i>=0; )
		cols[i].remove_all_objects();
}

int SketcherCurveList::column_resize_event()
{
	for( int i=CV_SZ; --i>=0; )
		col_widths[i] = get_column_width(i);
	return 1;
}

int SketcherCurveList::handle_event()
{
	int ci = get_selection_number(0, 0);
	set_selected(ci);
	gui->point_list->update(0);
	gui->send_configure_change();
	return 1;
}

int SketcherCurveList::selection_changed()
{
	handle_event();
	return 1;
}

void SketcherCurveList::set_selected(int k)
{
	int ci = -1;
	if( k >= 0 && k < plugin->config.curves.size() ) {
		SketcherCurve *cv = plugin->config.curves[k];
		gui->curve_width->update(cv->width);
		gui->curve_pen->update(cv->pen);
		gui->curve_color->update_gui(cv->color);
		ci = k;
	}
	plugin->config.cv_selected = ci;
	update_list(ci);
}

void SketcherCurveList::update_list(int k)
{
	int xpos = get_xposition(), ypos = get_yposition();
	if( k >= 0 ) update_selection(&cols[0], k);
	BC_ListBox::update(&cols[0], &col_titles[0],&col_widths[0],CV_SZ, xpos,ypos,k);
	center_selection();
}

void SketcherCurveList::update(int k)
{
	clear();
	SketcherCurves &curves = plugin->config.curves;
	int sz = curves.size();
	for( int i=0; i<sz; ++i ) {
		SketcherCurve *cv = curves[i];
		char itxt[BCSTRLEN];  sprintf(itxt,"%d", cv->id);
		char ptxt[BCSTRLEN];  sprintf(ptxt,"%s", cv_pen[cv->pen]);
		char rtxt[BCSTRLEN];  sprintf(rtxt,"%d", cv->width);
		int color = cv->color;
		int r = (color>>16)&0xff;
		int g = (color>> 8)&0xff;
		int b = (color>> 0)&0xff;
		int a = (~color>>24)&0xff;
		char ctxt[BCSTRLEN];  sprintf(ctxt,"#%02x%02x%02x", r, g, b);
		char atxt[BCSTRLEN];  sprintf(atxt,"%5.3f", a/255.);
		add_curve(itxt, ptxt, rtxt, ctxt, atxt);
	}
	set_selected(k);
}

void SketcherCurveList::add_curve(const char *id, const char *pen,
		const char *width, const char *color, const char *alpha)
{
	cols[CV_ID].append(new BC_ListBoxItem(id));
	cols[CV_RAD].append(new BC_ListBoxItem(width));
	cols[CV_PEN].append(new BC_ListBoxItem(pen));
	cols[CV_CLR].append(new BC_ListBoxItem(color));
	cols[CV_ALP].append(new BC_ListBoxItem(alpha));
}

SketcherNewCurve::SketcherNewCurve(SketcherWindow *gui, Sketcher *plugin, int x, int y)
 : BC_GenericButton(x, y, 64, _("New"))
{
	this->gui = gui;
	this->plugin = plugin;
}
SketcherNewCurve::~SketcherNewCurve()
{
}
int SketcherNewCurve::handle_event()
{
	int pen = gui->curve_pen->pen;
	int color = gui->curve_color->color;
	int width = gui->curve_width->width;
	int ci = plugin->config.cv_selected;
	if( ci >= 0 && ci < plugin->config.curves.size() ) {
		SketcherCurve *cv = plugin->config.curves[ci];
		pen = cv->pen;  width = cv->width;  color = cv->color;
	}
	ci = plugin->new_curve(pen, width, color);
	gui->curve_list->update(ci);
	gui->point_list->update(-1);
	gui->send_configure_change();
	return 1;
}

SketcherDelCurve::SketcherDelCurve(SketcherWindow *gui, Sketcher *plugin, int x, int y)
 : BC_GenericButton(x, y, 64, C_("Del"))
{
	this->gui = gui;
	this->plugin = plugin;
}
SketcherDelCurve::~SketcherDelCurve()
{
}
int SketcherDelCurve::handle_event()
{
	SketcherConfig &config = plugin->config;
	int ci = config.cv_selected;
	SketcherCurves &curves = config.curves;
	if( ci >= 0 && ci < curves.size() ) {
		curves.remove_object_number(ci--);
		if( ci < 0 ) ci = 0;
		if( !curves.size() )
			ci = plugin->new_curve();
		gui->curve_list->update(ci);
		gui->point_list->update(-1);
		gui->send_configure_change();
	}
	return 1;
}

SketcherCurveUp::SketcherCurveUp(SketcherWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Up"))
{
	this->gui = gui;
}
SketcherCurveUp::~SketcherCurveUp()
{
}

int SketcherCurveUp::handle_event()
{
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	SketcherCurves &curves = config.curves;
	if( ci > 0 && ci < curves.size() ) {
		SketcherCurve *&cv0 = curves[ci];
		SketcherCurve *&cv1 = curves[--ci];
		SketcherCurve *t = cv0;  cv0 = cv1;  cv1 = t;
		gui->curve_list->update(ci);
	}
	gui->send_configure_change();
	return 1;
}

SketcherCurveDn::SketcherCurveDn(SketcherWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Dn"))
{
	this->gui = gui;
}
SketcherCurveDn::~SketcherCurveDn()
{
}

int SketcherCurveDn::handle_event()
{
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	SketcherCurves &curves = config.curves;
	if( ci >= 0 && ci < curves.size()-1 ) {
		SketcherCurve *&cv0 = curves[ci];
		SketcherCurve *&cv1 = curves[++ci];
		SketcherCurve *t = cv0;  cv0 = cv1;  cv1 = t;
		gui->curve_list->update(ci);
	}
	gui->send_configure_change();
	return 1;
}


SketcherPointTypeItem::SketcherPointTypeItem(int arc)
 : BC_MenuItem(_(pt_type[arc]))
{
	this->arc = arc;
}
int SketcherPointTypeItem::handle_event()
{
	SketcherPointType *popup = (SketcherPointType*)get_popup_menu();
	popup->update(arc);
	SketcherWindow *gui = popup->gui;
	SketcherConfig &config = gui->plugin->config;
	SketcherCurves &curves = config.curves;
	int ci = config.cv_selected;
	if( ci < 0 || ci >= curves.size() )
		return 1;
	SketcherCurve *cv = curves[ci];
	SketcherPoints &points = cv->points;
	int pi = config.pt_selected;

	ArrayList<int> selected;
	for( int v,i=0; (v=gui->point_list->get_selection_number(0, i))>=0; ++i )
		selected.append(v);

	for( int i=selected.size(); --i>=0; ) {
		int k = selected[i];
		if( k < 0 || k >= points.size() ) continue;
		SketcherPoint *pt = cv->points[k];
		pt->arc = arc;
		gui->point_list->set_point(k, PT_TY, _(pt_type[arc]));
	}

	gui->point_list->update_list(pi);
	gui->send_configure_change();
	return 1;
}

SketcherPointType::SketcherPointType(SketcherWindow *gui, int x, int y, int arc)
 : BC_PopupMenu(x,y,64,_(pt_type[arc]))
{
	this->gui = gui;
	this->type = arc;
}
void SketcherPointType::create_objects()
{
	for( int arc=0; arc<PT_SZ; ++arc )
		add_item(new SketcherPointTypeItem(arc));
}
void SketcherPointType::update(int arc)
{
	set_text(_(pt_type[this->type=arc]));
}


SketcherPointList::SketcherPointList(SketcherWindow *gui, Sketcher *plugin, int x, int y)
 : BC_ListBox(x, y, 360, 130, LISTBOX_TEXT)
{
	this->gui = gui;
	this->plugin = plugin;
	col_titles[PT_ID] = _("ID");    col_widths[PT_ID] = 50;
	col_titles[PT_TY] = _("Type");  col_widths[PT_TY] = 80;
	col_titles[PT_X] = _("X");      col_widths[PT_X] = 90;
	col_titles[PT_Y] = _("Y");      col_widths[PT_Y] = 90;
	set_selection_mode(LISTBOX_MULTIPLE);
}
SketcherPointList::~SketcherPointList()
{
	clear();
}
void SketcherPointList::clear()
{
	for( int i=PT_SZ; --i>=0; )
		cols[i].remove_all_objects();
}

int SketcherPointList::column_resize_event()
{
	for( int i=PT_SZ; --i>=0; )
		col_widths[i] = get_column_width(i);
	return 1;
}

int SketcherPointList::handle_event()
{
	int pi = get_selection_number(0, 0);
	if( get_selection_number(0, 1) >= 0 ) pi = -1;
	set_selected(pi);
	gui->send_configure_change();
	return 1;
}

int SketcherPointList::selection_changed()
{
	handle_event();
	return 1;
}

void SketcherPointList::add_point(const char *id, const char *ty, const char *xp, const char *yp)
{
	cols[PT_ID].append(new BC_ListBoxItem(id));
	cols[PT_TY].append(new BC_ListBoxItem(ty));
	cols[PT_X].append(new BC_ListBoxItem(xp));
	cols[PT_Y].append(new BC_ListBoxItem(yp));
}

void SketcherPointList::set_point(int i, int c, int v)
{
	char stxt[BCSTRLEN];
	sprintf(stxt,"%d",v);
	set_point(i,c,stxt);
}
void SketcherPointList::set_point(int i, int c, coord v)
{
	char stxt[BCSTRLEN];
	sprintf(stxt,"%0.1f",v);
	set_point(i,c,stxt);
}
void SketcherPointList::set_point(int i, int c, const char *cp)
{
	cols[c].get(i)->set_text(cp);
}

void SketcherPointList::set_selected(int k)
{
	SketcherPoint *pt = 0;
	int ci = plugin->config.cv_selected, pi = -1;
	if( ci >= 0 && ci < plugin->config.curves.size() ) {
		SketcherCurve *cv = plugin->config.curves[ci];
		pt = k >= 0 && k < cv->points.size() ? cv->points[pi=k] : 0;
	}
	gui->point_type->update(pt ? pt->arc : ARC_OFF);
	gui->point_x->update(pt ? pt->x : 0.f);
	gui->point_y->update(pt ? pt->y : 0.f);
	gui->point_id->update(pt ? pt->id : 0);
	plugin->config.pt_selected = pi;
	update_list(pi);
}
void SketcherPointList::update_list(int k)
{
	int xpos = get_xposition(), ypos = get_yposition();
	if( k >= 0 ) update_selection(&cols[0], k);
	BC_ListBox::update(&cols[0], &col_titles[0],&col_widths[0],PT_SZ, xpos,ypos,k);
	center_selection();
}
void SketcherPointList::update(int k)
{
	clear();
	int ci = plugin->config.cv_selected, sz = 0;
	if( ci >= 0 && ci < plugin->config.curves.size() ) {
		SketcherCurve *cv = plugin->config.curves[ci];
		SketcherPoints &points = cv->points;
		sz = points.size();
		for( int i=0; i<sz; ++i ) {
			SketcherPoint *pt = points[i];
			char itxt[BCSTRLEN];  sprintf(itxt,"%d", pt->id);
			char ttxt[BCSTRLEN];  sprintf(ttxt,"%s", _(pt_type[pt->arc]));
			char xtxt[BCSTRLEN];  sprintf(xtxt,"%0.1f", pt->x);
			char ytxt[BCSTRLEN];  sprintf(ytxt,"%0.1f", pt->y);
			add_point(itxt, ttxt, xtxt, ytxt);
		}
	}
	set_selected(k);
}

void SketcherWindow::update_gui()
{
	SketcherConfig &config = plugin->config;
	int ci = config.cv_selected;
	int pi = config.pt_selected;
	curve_list->update(ci);
	point_list->update(pi);
	SketcherCurve *cv = ci >= 0 ? config.curves[ci] : 0;
	curve_width->update(cv ? cv->width : 1);
	curve_pen->update(cv ? cv->pen : PEN_SQUARE);
	curve_color->set_color(cv ? cv->color : CV_COLOR);
	SketcherPoint *pt = pi >= 0 ? cv->points[pi] : 0;
	point_x->update(pt ? pt->x : 0);
	point_y->update(pt ? pt->y : 0);
	point_id->update(pt ? pt->id : 0);
	drag->update(plugin->config.drag);
}


SketcherPointUp::SketcherPointUp(SketcherWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Up"))
{
	this->gui = gui;
}
SketcherPointUp::~SketcherPointUp()
{
}

int SketcherPointUp::handle_event()
{
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci < 0 || ci >= config.curves.size() )
		return 1;
	SketcherCurve *cv = config.curves[ci];
	SketcherPoints &points = cv->points;
	if( points.size() < 2 )
		return 1;
	int pi = config.pt_selected;

	ArrayList<int> selected;
	for( int v,i=0; (v=gui->point_list->get_selection_number(0, i))>=0; ++i )
		selected.append(v);

	for( int i=0; i<selected.size(); ++i ) {
		int k = selected[i];
		if( k <= 0 ) continue;
		if( k == pi ) --pi;
		SketcherPoint *&pt0 = points[k];
		SketcherPoint *&pt1 = points[--k];
		SketcherPoint *t = pt0;  pt0 = pt1;  pt1 = t;
	}
	gui->point_list->update(pi);
	gui->send_configure_change();
	return 1;
}

SketcherPointDn::SketcherPointDn(SketcherWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Dn"))
{
	this->gui = gui;
}
SketcherPointDn::~SketcherPointDn()
{
}

int SketcherPointDn::handle_event()
{
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci < 0 || ci >= config.curves.size() )
		return 1;
	SketcherCurve *cv = config.curves[ci];
	SketcherPoints &points = cv->points;
	int sz1 = points.size()-1;
	if( sz1 < 1 )
		return 1;
	int pi = config.pt_selected;

	ArrayList<int> selected;
	for( int v,i=0; (v=gui->point_list->get_selection_number(0, i))>=0; ++i )
		selected.append(v);

	for( int i=selected.size(); --i>=0; ) {
		int k = selected[i];
		if( k >= sz1 ) continue;
		if( k == pi ) ++pi;
		SketcherPoint *&pt0 = points[k];
		SketcherPoint *&pt1 = points[++k];
		SketcherPoint *t = pt0;  pt0 = pt1;  pt1 = t;
	}
	gui->point_list->update(pi);
	gui->send_configure_change();
	return 1;
}

SketcherDrag::SketcherDrag(SketcherWindow *gui, int x, int y)
 : BC_CheckBox(x, y, gui->plugin->config.drag, _("Drag"))
{
	this->gui = gui;
}
int SketcherDrag::handle_event()
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

SketcherNewPoint::SketcherNewPoint(SketcherWindow *gui, Sketcher *plugin, int x, int y)
 : BC_GenericButton(x, y, 64, _("New"))
{
	this->gui = gui;
	this->plugin = plugin;
}
SketcherNewPoint::~SketcherNewPoint()
{
}
int SketcherNewPoint::handle_event()
{
	int pi = plugin->config.pt_selected;
	int arc = gui->point_type->type;
	int k = plugin->new_point(pi+1, arc);
	gui->point_list->update(k);
	gui->send_configure_change();
	return 1;
}

SketcherDelPoint::SketcherDelPoint(SketcherWindow *gui, Sketcher *plugin, int x, int y)
 : BC_GenericButton(x, y, 64, C_("Del"))
{
	this->gui = gui;
	this->plugin = plugin;
}
SketcherDelPoint::~SketcherDelPoint()
{
}
int SketcherDelPoint::handle_event()
{
	SketcherConfig &config = gui->plugin->config;
	SketcherCurves &curves = config.curves;
	int ci = config.cv_selected;
	if( ci < 0 || ci >= curves.size() )
		return 1;
	SketcherCurve *cv = curves[ci];
	SketcherPoints &points = cv->points;
	int pi = config.pt_selected;

	ArrayList<int> selected;
	for( int v,i=0; (v=gui->point_list->get_selection_number(0, i))>=0; ++i )
		selected.append(v);

	for( int i=selected.size(); --i>=0; ) {
		int k = selected[i];
		if( k < 0 || k >= points.size() ) continue;
		points.remove_object_number(k);
		if( k == pi && --pi < 0 && points.size() > 0 ) pi = 0;
	}
	gui->point_list->update(pi);
	gui->send_configure_change();
	return 1;
}

SketcherResetCurves::SketcherResetCurves(SketcherWindow *gui, Sketcher *plugin, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->gui = gui;
	this->plugin = plugin;
}
SketcherResetCurves::~SketcherResetCurves()
{
}
int SketcherResetCurves::handle_event()
{
	SketcherConfig &config = plugin->config;
	config.curves.remove_all_objects();
	int ci = plugin->new_curve();
	gui->curve_list->update(ci);
	gui->point_list->update(-1);
	gui->send_configure_change();
	return 1;
}

SketcherResetPoints::SketcherResetPoints(SketcherWindow *gui, Sketcher *plugin, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->gui = gui;
	this->plugin = plugin;
}
SketcherResetPoints::~SketcherResetPoints()
{
}
int SketcherResetPoints::handle_event()
{
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci >= 0 && ci < config.curves.size() ) {
		SketcherCurve *cv = config.curves[ci];
		cv->points.remove_all_objects();
		gui->point_list->update(-1);
		gui->send_configure_change();
	}
	return 1;
}

