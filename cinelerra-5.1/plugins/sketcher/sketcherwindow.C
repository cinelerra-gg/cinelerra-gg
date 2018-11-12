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

#define COLOR_W 30
#define COLOR_H 30

const char *SketcherCurve::types[] = {
	N_("off"),
	N_("line"),
	N_("smooth"),
};
const char *SketcherCurve::pens[] = {
	N_("box"),
	N_("+"),
	N_("/"),
	N_("X"),
};

SketcherCurveTypeItem::SketcherCurveTypeItem(int ty)
 : BC_MenuItem(_(cv_type[ty]))
{
	this->ty = ty;
}
int SketcherCurveTypeItem::handle_event()
{
	SketcherCurveType *popup = (SketcherCurveType*)get_popup_menu();
	popup->update(ty);
	SketcherWindow *gui = popup->gui;
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci >= 0 && ci < config.curves.size() ) {
		SketcherCurve *cv = config.curves[ci];
		cv->ty = ty;
		gui->curve_list->update(ci);
		gui->send_configure_change();
	}
	return 1;
}

SketcherCurveType::SketcherCurveType(SketcherWindow *gui, int x, int y, int ty)
 : BC_PopupMenu(x,y,64,_(cv_type[ty]))
{
	this->gui = gui;
}
void SketcherCurveType::create_objects()
{
	int n = sizeof(cv_type)/sizeof(cv_type[0]);
	for( int ty=0; ty<n; ++ty )
		add_item(new SketcherCurveTypeItem(ty));
}
void SketcherCurveType::update(int ty)
{
	set_text(_(cv_type[ty]));
}


SketcherCurvePenItem::SketcherCurvePenItem(int pen)
 : BC_MenuItem(_(cv_pen[pen]))
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
}
void SketcherCurvePen::create_objects()
{
	int n = sizeof(cv_pen)/sizeof(cv_pen[0]);
	for( int pen=0; pen<n; ++pen )
		add_item(new SketcherCurvePenItem(pen));
}
void SketcherCurvePen::update(int pen)
{
	set_text(_(cv_pen[pen]));
}


SketcherCurveColor::SketcherCurveColor(SketcherWindow *gui, int x, int y, int w)
 : BC_Button(x, y, w, vframes)
{
	this->gui = gui;
	this->color = BLACK;
	for( int i=0; i<3; ++i ) {
		vframes[i] = new VFrame(w, w, BC_RGBA8888);
		vframes[i]->clear_frame();
	}
}

SketcherCurveColor::~SketcherCurveColor()
{
	for( int i=0; i<3; ++i )
		delete vframes[i];
}

void SketcherCurveColor::set_color(int color)
{
	this->color = color;
	int r = (color>>16) & 0xff;
	int g = (color>>8) & 0xff;
	int b = (color>>0) & 0xff;
	for( int i=0; i<3; ++i ) {
		VFrame *vframe = vframes[i];
		int ww = vframe->get_w(), hh = vframe->get_h();
		int cx = (ww+1)/2, cy = hh/2;
		double cc = (cx*cx + cy*cy) / 4.;
		uint8_t *bp = vframe->get_data(), *dp = bp;
		uint8_t *ep = dp + vframe->get_data_size();
		int rr = r, gg = g, bb = b;
		int bpl = vframe->get_bytes_per_line();
		switch( i ) {
		case BUTTON_UP:
			break;
		case BUTTON_UPHI:
			if( (rr+=48) > 0xff ) rr = 0xff;
			if( (gg+=48) > 0xff ) gg = 0xff;
			if( (bb+=48) > 0xff ) bb = 0xff;
			break;
		case BUTTON_DOWNHI:
			if( (rr-=48) < 0x00 ) rr = 0x00;
			if( (gg-=48) < 0x00 ) gg = 0x00;
			if( (bb-=48) < 0x00 ) bb = 0x00;
			break;
		}
		while( dp < ep ) {
			int yy = (dp-bp) / bpl, xx = ((dp-bp) % bpl) >> 2;
			int dy = cy - yy, dx = cx - xx;
			double s = dx*dx + dy*dy - cc;
			double ss = s < 0 ? 1 : s >= cc ? 0 : 1 - s/cc;
			int aa = ss * 0xff;
			*dp++ = rr; *dp++ = gg; *dp++ = bb; *dp++ = aa;
		}
	}
	set_images(vframes);
}

void SketcherCurveColor::update_gui(int color)
{
	set_color(color);
	draw_face();
}

int SketcherCurveColor::handle_event()
{
	gui->start_color_thread(this);
	return 1;
}

SketcherCurveColorPicker::SketcherCurveColorPicker(SketcherWindow *gui, SketcherCurveColor *color_button)
 : ColorPicker(0, _("Color"))
{
	this->gui = gui;
	this->color_button = color_button;
	this->color = 0;
	color_update = new SketcherCurveColorThread(this);
}

SketcherCurveColorPicker::~SketcherCurveColorPicker()
{
	delete color_update;
}

void SketcherCurveColorPicker::start(int color)
{
	start_window(color, 0, 1);
	color_update->start();
}

void SketcherCurveColorPicker::handle_done_event(int result)
{
	color_update->stop();
	gui->lock_window("SketcherCurveColorPicker::handle_done_event");
	if( result ) color = orig_color;
	color_button->update_gui(color);
	gui->unlock_window();
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci >= 0 && ci < config.curves.size() ) {
		SketcherCurve *cv = config.curves[ci];
		cv->color = color;
		gui->curve_list->update(ci);
		gui->send_configure_change();
	}
}

int SketcherCurveColorPicker::handle_new_color(int color, int alpha)
{
	this->color = color;
	color_update->update_lock->unlock();
	return 1;
}

void SketcherCurveColorPicker::update_gui()
{
	gui->lock_window("SketcherCurveColorPicker::update_gui");
	color_button->update_gui(color);
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci >= 0 ) {
		SketcherCurve *cv = config.curves[ci];
		cv->color = color;
		gui->curve_list->update(ci);
		gui->send_configure_change();
	}
	gui->unlock_window();
}

SketcherCurveColorThread::SketcherCurveColorThread(SketcherCurveColorPicker *color_picker)
 : Thread(1, 0, 0)
{
	this->color_picker = color_picker;
	this->update_lock = new Condition(0,"SketcherCurveColorThread::update_lock");
	done = 1;
}

SketcherCurveColorThread::~SketcherCurveColorThread()
{
	stop();
	delete update_lock;
}

void SketcherCurveColorThread::start()
{
	if( done ) {
		done = 0;
		Thread::start();
	}
}

void SketcherCurveColorThread::stop()
{
	if( !done ) {
		done = 1;
		update_lock->unlock();
		join();
	}
}

void SketcherCurveColorThread::run()
{
	while( !done ) {
		update_lock->lock("SketcherCurveColorThread::run");
		if( done ) break;
		color_picker->update_gui();
	}
}


SketcherNum::SketcherNum(SketcherWindow *gui, int x, int y, int output,
		int mn, int mx)
 : BC_TumbleTextBox(gui, output, mn, mx, x, y, 64)
{
	this->gui = gui;
	set_increment(1);
}

SketcherNum::~SketcherNum()
{
}

int SketcherPointX::handle_event()
{
	if( !SketcherNum::handle_event() ) return 0;
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci >= 0 && ci < config.curves.size() ) {
		SketcherCurve *cv = config.curves[ci];
		SketcherPointList *point_list = gui->point_list;
		int hot_point = point_list->get_selection_number(0, 0);
		SketcherPoints &points = cv->points;
		if( hot_point >= 0 && hot_point < points.size() ) {
			int v = atoi(get_text());
			points[hot_point]->x = v;
			point_list->set_point(hot_point, PT_X, v);
			point_list->update_list(hot_point);
			gui->send_configure_change();
		}
	}
	return 1;
}
int SketcherPointY::handle_event()
{
	if( !SketcherNum::handle_event() ) return 0;
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci >= 0 && ci < config.curves.size() ) {
		SketcherCurve *cv = config.curves[ci];
		SketcherPointList *point_list = gui->point_list;
		int hot_point = point_list->get_selection_number(0, 0);
		SketcherPoints &points = cv->points;
		if( hot_point >= 0 && hot_point < points.size() ) {
			int v = atoi(get_text());
			points[hot_point]->y = v;
			point_list->set_point(hot_point, PT_Y, v);
			point_list->update_list(hot_point);
			gui->send_configure_change();
		}
	}
	return 1;
}

int SketcherCurveRadius::handle_event()
{
	if( !SketcherNum::handle_event() ) return 0;
	SketcherConfig &config = gui->plugin->config;
	int ci = config.cv_selected;
	if( ci >= 0 && ci < config.curves.size() ) {
		SketcherCurve *cv = config.curves[ci];
		int v = atoi(get_text());
		cv->radius = v;
		gui->curve_list->update(ci);
		gui->send_configure_change();
	}
	return 1;
}


SketcherWindow::SketcherWindow(Sketcher *plugin)
 : PluginClientWindow(plugin, 380, 580, 380, 580, 0)
{
	this->plugin = plugin;
	this->title_type = 0; this->curve_type = 0;
	this->title_pen = 0;  this->curve_pen = 0;
	this->title_color = 0; this->curve_color = 0;
	this->color_picker = 0; this->new_points = 0;
	this->new_curve = 0;  this->del_curve = 0;
	this->curve_up = 0;   this->curve_dn = 0;
	this->title_x = 0;    this->point_x = 0;
	this->title_y = 0;    this->point_y = 0;
	this->new_point = 0;  this->del_point = 0;
	this->point_up = 0;   this->point_dn = 0;
	this->drag = 0;       this->dragging = 0;
	this->last_x = 0;     this->last_y = 0;
	this->point_list = 0; this->pending_config = 0;
}

SketcherWindow::~SketcherWindow()
{
	delete curve_radius;
	delete point_x;
	delete point_y;
	delete color_picker;
}

void SketcherWindow::create_objects()
{
	int x = 10, y = 10, x1, y1;
	int margin = plugin->get_theme()->widget_border;
	BC_Title *title;
	int ci = plugin->config.cv_selected;
	if( ci < 0 || ci >= plugin->config.curves.size() )
		ci = plugin->new_curve(0, 1, 0, BLACK);
	SketcherCurve *cv = plugin->config.curves[ci];
	add_subwindow(reset_curves = new SketcherResetCurves(this, plugin, x1=x, y+3));
	x1 += reset_curves->get_w() + 2*margin;
	const char *curve_text = _("Curve");
	add_subwindow(title_radius = new BC_Title(x1, y, _("Width:")));
	x1 += title_radius->get_w() + margin;
	curve_radius = new SketcherCurveRadius(this, x1, y, cv->radius);
	curve_radius->create_objects();
	y += reset_curves->get_h() + 2*margin;
	x1 = get_w()-x - BC_Title::calculate_w(this, curve_text, LARGEFONT);
	y1 = y-margin - BC_Title::calculate_h(this, curve_text, LARGEFONT);
	add_subwindow(title = new BC_Title(x1, y1, curve_text, LARGEFONT,
		get_resources()->menu_highlighted_fontcolor));
	add_subwindow(curve_list = new SketcherCurveList(this, plugin, x, y));
	y += curve_list->get_h() + margin;
	add_subwindow(title_type = new BC_Title(x, y, _("Type:")));
	x1 = x + title_type->get_w() + margin;
	add_subwindow(curve_type = new SketcherCurveType(this, x1, y, cv->ty));
	curve_type->create_objects();
	x1 += curve_type->get_w() + margin;
	add_subwindow(new_curve = new SketcherNewCurve(this, plugin, x1, y));
	x1 += new_curve->get_w() + margin;
	add_subwindow(curve_up = new SketcherCurveUp(this, x1, y));
	x1 += curve_up->get_w() + 2*margin;
	add_subwindow(title_color = new BC_Title(x1, y, _("Color:")));
	y += curve_type->get_h() + margin;

	add_subwindow(title_pen = new BC_Title(x, y, _("Pen:")));
	x1 = x + title_pen->get_w() + margin;
	add_subwindow(curve_pen = new SketcherCurvePen(this, x1, y, cv->pen));
	curve_pen->create_objects();
	x1 += curve_pen->get_w() + margin;
	add_subwindow(del_curve = new SketcherDelCurve(this, plugin, x1, y));
	x1 += del_curve->get_w() + margin;
	add_subwindow(curve_dn = new SketcherCurveDn(this, x1, y));
	x1 += curve_dn->get_w() + 2*margin;
	add_subwindow(curve_color = new SketcherCurveColor(this, x1, y, COLOR_W));
	curve_color->create_objects();
	curve_color->set_color(cv->color);
	curve_color->draw_face();
	y += COLOR_H + margin;
	curve_list->update(ci);

	BC_Bar *bar;
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += bar->get_h() + 2*margin;

	int pi = plugin->config.pt_selected;
	SketcherPoint *pt = pi >= 0 && pi < cv->points.size() ? cv->points[pi] : 0;
	add_subwindow(reset_points = new SketcherResetPoints(this, plugin, x1=x, y+3));
	x1 += reset_points->get_w() + 2*margin;
	add_subwindow(drag = new SketcherDrag(this, x1, y));
	y += drag->get_h() + margin;
	if( plugin->config.drag ) {
		if( !grab(plugin->server->mwindow->cwindow->gui) )
			eprintf("drag enabled, but compositor already grabbed\n");
	}
	const char *point_text = _("Point");
	x1 = get_w()-x - BC_Title::calculate_w(this, point_text, LARGEFONT);
	y1 = y-margin - BC_Title::calculate_h(this, point_text, LARGEFONT);
	add_subwindow(title = new BC_Title(x1, y1, point_text, LARGEFONT,
		get_resources()->menu_highlighted_fontcolor));
	add_subwindow(point_list = new SketcherPointList(this, plugin, x, y));

	y += point_list->get_h() + margin;
	add_subwindow(title_x = new BC_Title(x, y, _("X:")));
	x1 = x + title_x->get_w() + margin;
	point_x = new SketcherPointX(this, x1, y, !pt ? 0.f : pt->x);
	point_x->create_objects();
	x1 += point_x->get_w() + 2*margin;
	add_subwindow(new_point = new SketcherNewPoint(this, plugin, x1, y));
	x1 += new_point->get_w() + margin;
	add_subwindow(point_up = new SketcherPointUp(this, x1, y));
	y += point_x->get_h() + margin;
	add_subwindow(title_y = new BC_Title(x, y, _("Y:")));
	x1 = x + title_y->get_w() + margin;
	point_y = new SketcherPointY(this, x1, y, !pt ? 0.f : pt->y);
	point_y->create_objects();
	x1 += point_y->get_w() + 2*margin;
	add_subwindow(del_point = new SketcherDelPoint(this, plugin, x1, y));
	x1 += del_point->get_w() + margin;
	add_subwindow(point_dn = new SketcherPointDn(this, x1, y));
	y += point_y->get_h() + margin + 10;
	point_list->update(pi);

	add_subwindow(notes0 = new BC_Title(x, y,
		 _("\n"
		   "LMB=\n"
		   "Alt+LMB=\n"
		   "MMB=\n"
		   "DEL=\n")));
	add_subwindow(notes1 = new BC_Title(x+80, y,
		 _("     No Shift\n"
		   "select point\n"
		   "drag curve\n"
		   "next curve type\n"
		   "deletes point\n")));
	add_subwindow(notes2 = new BC_Title(x+200, y,
		 _("             Shift\n"
		   "append new points\n"
		   "drag image\n"
		   "append new curve\n"
		   "delete curve\n")));
	show_window(1);
}

void SketcherWindow::send_configure_change()
{
	pending_config = 0;
	plugin->send_configure_change();
}

int SketcherWindow::grab_event(XEvent *event)
{
	int ret = do_grab_event(event);
	if( pending_config && !grab_event_count() )
		send_configure_change();
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
		dragging = event->xbutton.state & Mod1Mask ? -1 : 1; // alt_down
		break;
	case ButtonRelease:
	case MotionNotify:
		if( !dragging ) return 0;
		break;
	default:
		return 0;
	}


	int ci = plugin->config.cv_selected;
	if( ci < 0 || ci >= plugin->config.curves.size() )
		return 1;

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
	SketcherCurve *cv = plugin->config.curves[ci];
	SketcherPoints &points = cv->points;
	int state = event->xmotion.state;

	switch( event->type ) {
	case ButtonPress: {
		if( dragging < 0 ) break;
		int hot_point = -1;
		int button_no = event->xbutton.button;
		if( button_no == LEFT_BUTTON ) {
// create new point string
			if( (state & ShiftMask) ) {
				++new_points;
				hot_point = plugin->new_point(cv, output_x, output_y);
				point_list->update(hot_point);
			}
			else {
// select point
				int sz = points.size();
				int last_point = hot_point;
				if( sz > 0 ) {
					SketcherPoint *pt = points[hot_point=0];
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
				if( hot_point != last_point ) {
					SketcherPoint *pt = 0;
					if( hot_point >= 0 && hot_point < sz ) {
						pt = points[hot_point];
						point_list->set_point(hot_point, PT_X, pt->x = output_x);
						point_list->set_point(hot_point, PT_Y, pt->y = output_y);
					}
					point_list->update_list(hot_point);
					point_x->update(pt ? pt->x : 0.f);
					point_y->update(pt ? pt->y : 0.f);
				}
			}
		}
		else if( button_no == MIDDLE_BUTTON ) {
			if( (state & ShiftMask) ) {
				int ci = plugin->new_curve(cv->ty, cv->radius, cv->pen, cv->color);
				curve_list->update(ci);
				point_list->update(-1);
			}
			else {
				int ty = cv->ty + 1;
				if( ty >= TYP_SZ ) ty = 0;
				cv->ty = ty;
				curve_type->update(ty);
				curve_list->update(ci);
			}
		}
		break; }
	case MotionNotify: {
		int hot_point = point_list->get_selection_number(0, 0);
		if( dragging < 0 ) {
			SketcherCurves &curves = plugin->config.curves;
			int dx = round(output_x - last_x);
			int dy = round(output_y - last_y);
			int mnc = (state & ShiftMask) || ci<0 ? 0 : ci;
			int mxc = (state & ShiftMask) ? curves.size() : ci+1;
			for( int i=mnc; i<mxc; ++i ) {
				SketcherCurve *crv = plugin->config.curves[i];
				int pts = crv->points.size();
				for( int k=0; k<pts; ++k ) {
					SketcherPoint *pt = crv->points[k];
					pt->x += dx;  pt->y += dy;
				}
			}
			SketcherPoint *pt = hot_point >= 0 && hot_point < points.size() ?
				points[hot_point] : 0;
			point_x->update(pt ? pt->x : 0.f);
			point_y->update(pt ? pt->y : 0.f);
			point_list->update(hot_point);
			break;
		}
		if( (state & Button1Mask) ) {
			SketcherPoint *pt = hot_point >= 0 && hot_point < points.size() ?
				points[hot_point] : 0;
			if( pt && pt->x == output_x && pt->y == output_y ) break;
			if( new_points ) {
				if( pt ) {
					float frac_w = DISTANCE(pt->x, pt->y, output_x, output_y) / get_w();
					if( frac_w < 0.01 ) break; // 1 percent w
				}
				if( (state & ShiftMask) ) {
					++new_points;
					hot_point = plugin->new_point(cv, output_x, output_y);
					point_list->update(hot_point);
				}
			}
			else if( pt ) {
				point_list->set_point(hot_point, PT_X, pt->x = output_x);
				point_list->set_point(hot_point, PT_Y, pt->y = output_y);
				point_list->update_list(hot_point);
				point_x->update(pt->x);
				point_y->update(pt->y);
			}
		}
		break; }
	case ButtonRelease: {
		new_points = 0;
		dragging = 0;
		break; }
	}

	last_x = output_x;  last_y = output_y;
	pending_config = 1;
	return 1;
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

void SketcherWindow::done_event(int result)
{
	ungrab(client->server->mwindow->cwindow->gui);
}

void SketcherWindow::start_color_thread(SketcherCurveColor *color_button)
{
	unlock_window();
	delete color_picker;
	color_picker = new SketcherCurveColorPicker(this, color_button);
	int color = BLACK, ci = plugin->config.cv_selected;
	if( ci >= 0 && ci < plugin->config.curves.size() ) {
		SketcherCurve *cv = plugin->config.curves[ci];
		color = cv->color;
	}
	color_picker->start(color);
	lock_window("SketcherWindow::start_color_thread");
}


SketcherCurveList::SketcherCurveList(SketcherWindow *gui, Sketcher *plugin, int x, int y)
 : BC_ListBox(x, y, 360, 130, LISTBOX_TEXT)
{
	this->gui = gui;
	this->plugin = plugin;
	titles[CV_ID] = _("id");  widths[CV_ID] = 64;
	titles[CV_TY] = _("type");  widths[CV_TY] = 64;
	titles[CV_RAD] = _("radius");  widths[CV_RAD] = 64;
	titles[CV_PEN] = _("pen");  widths[CV_PEN] = 64;
	titles[CV_CLR] = _("color");  widths[CV_CLR] = 64;
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
		widths[i] = get_column_width(i);
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
		gui->curve_type->update(cv->ty);
		gui->curve_radius->update(cv->radius);
		gui->curve_pen->update(cv->pen);
		gui->curve_color->update_gui(cv->color);
		ci = k;
	}
	plugin->config.cv_selected = ci;
	update_selection(&cols[0], ci);
	update_list(-1);
}

void SketcherCurveList::update_list(int k)
{
	int xpos = get_xposition(), ypos = get_yposition();
	if( k < 0 ) k = get_selection_number(0, 0);
	update_selection(&cols[0], k);
	BC_ListBox::update(&cols[0], &titles[0],&widths[0],CV_SZ, xpos,ypos,k);
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
		char ttxt[BCSTRLEN];  sprintf(ttxt,"%s", cv_type[cv->ty]);
		char rtxt[BCSTRLEN];  sprintf(rtxt,"%d", cv->radius);
		char ptxt[BCSTRLEN];  sprintf(ptxt,"%s", cv_pen[cv->pen]);
		int color = cv->color;
		int r = (color>>16)&0xff, g = (color>>8)&0xff, b = (color>>0)&0xff;
		char ctxt[BCSTRLEN];  sprintf(ctxt,"#%02x%02x%02x", r, g, b);
		add_curve(itxt, ttxt, rtxt, ptxt, ctxt);
	}
	set_selected(k);
}

void SketcherCurveList::add_curve(const char *id, const char *type,
		const char *radius, const char *pen, const char *color)
{
	cols[CV_ID].append(new BC_ListBoxItem(id));
	cols[CV_TY].append(new BC_ListBoxItem(type));
	cols[CV_RAD].append(new BC_ListBoxItem(radius));
	cols[CV_PEN].append(new BC_ListBoxItem(pen));
	cols[CV_CLR].append(new BC_ListBoxItem(color));
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
	int ty = 1, pen = 0, color = 0, radius = 1;
	int ci = plugin->config.cv_selected;
	if( ci >= 0 && ci < plugin->config.curves.size() ) {
		SketcherCurve *cv = plugin->config.curves[ci];
		ty = cv->ty;  pen = cv->pen;
		color = cv->color;  radius = cv->radius;
	}
	int k = plugin->new_curve(ty, radius, pen, color);
	gui->curve_list->update(k);
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
	int hot_curve = gui->curve_list->get_selection_number(0, 0);
	SketcherCurves &curves = plugin->config.curves;
	if( hot_curve >= 0 && hot_curve < curves.size() ) {
		curves.remove_object_number(hot_curve);
		if( --hot_curve < 0 )
			hot_curve = plugin->new_curve(0, 1, 0, BLACK);
		gui->curve_list->update(hot_curve);
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
	SketcherCurves &curves = gui->plugin->config.curves;
	int hot_curve = gui->curve_list->get_selection_number(0, 0);

	if( hot_curve > 0 && hot_curve < curves.size() ) {
		SketcherCurve *&cv0 = curves[hot_curve];
		SketcherCurve *&cv1 = curves[--hot_curve];
		SketcherCurve *t = cv0;  cv0 = cv1;  cv1 = t;
		gui->curve_list->update(hot_curve);
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
	SketcherCurves &curves = gui->plugin->config.curves;
	int hot_curve = gui->curve_list->get_selection_number(0, 0);

	if( hot_curve >= 0 && hot_curve < curves.size()-1 ) {
		SketcherCurve *&cv0 = curves[hot_curve];
		SketcherCurve *&cv1 = curves[++hot_curve];
		SketcherCurve *t = cv0;  cv0 = cv1;  cv1 = t;
		gui->curve_list->update(hot_curve);
	}
	gui->send_configure_change();
	return 1;
}


SketcherPointList::SketcherPointList(SketcherWindow *gui, Sketcher *plugin, int x, int y)
 : BC_ListBox(x, y, 360, 130, LISTBOX_TEXT)
{
	this->gui = gui;
	this->plugin = plugin;
	titles[PT_X] = _("X");    widths[PT_X] = 90;
	titles[PT_Y] = _("Y");    widths[PT_Y] = 90;
	titles[PT_ID] = _("ID");  widths[PT_ID] = 50;
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
		widths[i] = get_column_width(i);
	return 1;
}

int SketcherPointList::handle_event()
{
	int pi = get_selection_number(0, 0);
	set_selected(pi);
	gui->send_configure_change();
	return 1;
}

int SketcherPointList::selection_changed()
{
	handle_event();
	return 1;
}

void SketcherPointList::add_point(const char *id, const char *xp, const char *yp)
{
	cols[PT_ID].append(new BC_ListBoxItem(id));
	cols[PT_X].append(new BC_ListBoxItem(xp));
	cols[PT_Y].append(new BC_ListBoxItem(yp));
}

void SketcherPointList::set_point(int i, int c, int v)
{
	char stxt[BCSTRLEN];
	sprintf(stxt,"%d",v);
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
	gui->point_x->update(pt ? pt->x : 0.f);
	gui->point_y->update(pt ? pt->y : 0.f);
	plugin->config.pt_selected = pi;
	update_selection(&cols[0], pi);
	update_list(k);
}
void SketcherPointList::update_list(int k)
{
	int xpos = get_xposition(), ypos = get_yposition();
	if( k < 0 ) k = get_selection_number(0, 0);
	update_selection(&cols[0], k);
	BC_ListBox::update(&cols[0], &titles[0],&widths[0],PT_SZ, xpos,ypos,k);
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
			char xtxt[BCSTRLEN];  sprintf(xtxt,"%d", pt->x);
			char ytxt[BCSTRLEN];  sprintf(ytxt,"%d", pt->y);
			add_point(itxt, xtxt, ytxt);
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
	curve_radius->update(cv ? cv->radius : 1);
	curve_type->update(cv ? cv->ty : TYP_OFF);
	curve_pen->update(cv ? cv->pen : PEN_SQUARE);
	curve_color->set_color(cv ? cv->color : BLACK);
	SketcherPoint *pt = pi >= 0 ? cv->points[pi] : 0;
	point_x->update(pt ? pt->x : 0);
	point_y->update(pt ? pt->y : 0);
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
	int sz = points.size();
	int hot_point = gui->point_list->get_selection_number(0, 0);

	if( sz > 1 && hot_point > 0 ) {
		SketcherPoint *&pt0 = points[hot_point];
		SketcherPoint *&pt1 = points[--hot_point];
		SketcherPoint *t = pt0;  pt0 = pt1;  pt1 = t;
		gui->point_list->update(hot_point);
	}
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
	int sz = points.size();
	int hot_point = gui->point_list->get_selection_number(0, 0);
	if( sz > 1 && hot_point < sz-1 ) {
		SketcherPoint *&pt0 = points[hot_point];
		SketcherPoint *&pt1 = points[++hot_point];
		SketcherPoint *t = pt0;  pt0 = pt1;  pt1 = t;
		gui->point_list->update(hot_point);
	}
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
	int k = plugin->new_point();
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
	int ci = config.cv_selected;
	if( ci >= 0 && ci < config.curves.size() ) {
		SketcherCurve *cv = config.curves[ci];
		SketcherPoints &points = cv->points;
		int hot_point = gui->point_list->get_selection_number(0, 0);
		if( hot_point >= 0 && hot_point < points.size() ) {
			points.remove_object_number(hot_point);
			gui->point_list->update(--hot_point);
			gui->send_configure_change();
		}
	}
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
	int ci = plugin->new_curve(0, 1, 0, BLACK);
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

