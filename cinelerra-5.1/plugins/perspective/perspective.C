
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

#include "affine.h"
#include "cursors.h"
#include "language.h"
#include "perspective.h"

#define PERSPECTIVE_WIDTH 400
#define PERSPECTIVE_HEIGHT 600

REGISTER_PLUGIN(PerspectiveMain)


PerspectiveConfig::PerspectiveConfig()
{
	x1 = 0;    y1 = 0;
	x2 = 100;  y2 = 0;
	x3 = 100;  y3 = 100;
	x4 = 0;    y4 = 100;
	mode = AffineEngine::PERSPECTIVE;
	smoothing = AffineEngine::AF_DEFAULT;
	window_w = PERSPECTIVE_WIDTH;
	window_h = PERSPECTIVE_HEIGHT;
	current_point = 0;
	forward = 1;
	view_x = view_y = 0;
	view_zoom = 1;
}

int PerspectiveConfig::equivalent(PerspectiveConfig &that)
{
	return
		EQUIV(x1, that.x1) &&
		EQUIV(y1, that.y1) &&
		EQUIV(x2, that.x2) &&
		EQUIV(y2, that.y2) &&
		EQUIV(x3, that.x3) &&
		EQUIV(y3, that.y3) &&
		EQUIV(x4, that.x4) &&
		EQUIV(y4, that.y4) &&
		mode == that.mode &&
		smoothing == that.smoothing &&
		forward == that.forward;
// not tracking:
//		view_x == that.view_x &&
//		view_y == that.view_y &&
//		view_zoom == that.view_zoom &&
}

void PerspectiveConfig::copy_from(PerspectiveConfig &that)
{
	x1 = that.x1;  y1 = that.y1;
	x2 = that.x2;  y2 = that.y2;
	x3 = that.x3;  y3 = that.y3;
	x4 = that.x4;  y4 = that.y4;
	mode = that.mode;
	smoothing = that.smoothing;
	window_w = that.window_w;
	window_h = that.window_h;
	current_point = that.current_point;
	forward = that.forward;
	view_x = that.view_x;
	view_y = that.view_y;
	view_zoom = that.view_zoom;
}

void PerspectiveConfig::interpolate(PerspectiveConfig &prev, PerspectiveConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->x1 = prev.x1 * prev_scale + next.x1 * next_scale;
	this->y1 = prev.y1 * prev_scale + next.y1 * next_scale;
	this->x2 = prev.x2 * prev_scale + next.x2 * next_scale;
	this->y2 = prev.y2 * prev_scale + next.y2 * next_scale;
	this->x3 = prev.x3 * prev_scale + next.x3 * next_scale;
	this->y3 = prev.y3 * prev_scale + next.y3 * next_scale;
	this->x4 = prev.x4 * prev_scale + next.x4 * next_scale;
	this->y4 = prev.y4 * prev_scale + next.y4 * next_scale;
	mode = prev.mode;
	smoothing = prev.smoothing;
	forward = prev.forward;
	view_x = prev.view_x;
	view_y = prev.view_y;
	view_zoom = prev.view_zoom;
}


PerspectiveWindow::PerspectiveWindow(PerspectiveMain *plugin)
 : PluginClientWindow(plugin,
	plugin->config.window_w, plugin->config.window_h,
	plugin->config.window_w, plugin->config.window_h, 0)
{
//printf("PerspectiveWindow::PerspectiveWindow 1 %d %d\n", plugin->config.window_w, plugin->config.window_h);
	this->plugin = plugin;
}

PerspectiveWindow::~PerspectiveWindow()
{
}

void PerspectiveWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(canvas = new PerspectiveCanvas(this,
		x, y, get_w() - 20, get_h() - 290));
	canvas->set_cursor(CROSS_CURSOR, 0, 0);
	y += canvas->get_h() + 10;
	add_subwindow(new BC_Title(x, y, _("Current X:")));
	x += 80;
	this->x = new PerspectiveCoord(this,
		x, y, plugin->get_current_x(), 1);
	this->x->create_objects();
	x += 140;
	add_subwindow(new BC_Title(x, y, _("Y:")));
	x += 20;
	this->y = new PerspectiveCoord(this,
		x, y, plugin->get_current_y(), 0);
	this->y->create_objects();
	x = 10;   y += 30;
	add_subwindow(mode_perspective = new PerspectiveMode(this,
		x, y, AffineEngine::PERSPECTIVE, _("Perspective")));
	x += 120;
	add_subwindow(mode_sheer = new PerspectiveMode(this,
		x, y, AffineEngine::SHEER, _("Sheer")));
	x += 100;
	add_subwindow(affine = new PerspectiveAffine(this, x, y));
	affine->create_objects();
	x = 10;  y += 30;
	add_subwindow(mode_stretch = new PerspectiveMode(this,
		x, y, AffineEngine::STRETCH, _("Stretch")));
	x += 120;
	add_subwindow(new PerspectiveReset(this, x, y));
	update_canvas();

	x = 10;   y += 30;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Zoom view:")));
	int x1 = x + title->get_w() + 10, w1 = get_w() - x1 - 10;
	add_subwindow(zoom_view = new PerspectiveZoomView(this, x1, y, w1));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Perspective direction:")));
	x += 170;
	add_subwindow(forward = new PerspectiveDirection(this,
		x, y, 1, _("Forward")));
	x += 100;
	add_subwindow(reverse = new PerspectiveDirection(this,
		x, y, 0, _("Reverse")));
	x = 10;  y += 40;
	add_subwindow(title = new BC_Title(x, y, _("Alt/Shift:")));
	add_subwindow(new BC_Title(x+100, y, _("Button1 Action:")));
	y += title->get_h() + 5;
	add_subwindow(new BC_Title(x, y,
		"  0 / 0\n"
		"  0 / 1\n"
		"  1 / 0\n"
		"  1 / 1"));
	add_subwindow(new BC_Title(x+100, y,
	      _("Translate endpoint\n"
		"Zoom image\n"
		"Translate image\n"
		"Translate view")));
	show_window();
}


int PerspectiveWindow::resize_event(int w, int h)
{
	return 1;
}

PerspectiveZoomView::PerspectiveZoomView(PerspectiveWindow *gui,
	int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, -2., 2.,
	log10(gui->plugin->config.view_zoom < 0.01 ?
		0.01 : gui->plugin->config.view_zoom))
{
	this->gui = gui;
	set_precision(0.001);
	set_tooltip(_("Zoom"));
}
PerspectiveZoomView::~PerspectiveZoomView()
{
}
int PerspectiveZoomView::handle_event()
{
	float value = get_value();
	BC_FSlider::update(value);
	PerspectiveMain *plugin = gui->plugin;
	float view_zoom = plugin->config.view_zoom;
	double new_zoom = pow(10.,value);
	double scale = new_zoom / view_zoom;
	plugin->config.view_zoom = new_zoom;
	plugin->config.view_x *= scale;
	plugin->config.view_y *= scale;
	gui->update_canvas();
	plugin->send_configure_change();
	return 1;
}

char *PerspectiveZoomView::get_caption()
{
	double value = get_value();
	int frac = value >= 0. ? 1 : value >= -1. ? 2 : 3;
	double zoom = pow(10., value);
	char *caption = BC_Slider::get_caption();
	sprintf(caption, "%.*f", frac, zoom);
	return caption;
}

void PerspectiveZoomView::update(float zoom)
{
	if( zoom < 0.01 ) zoom = 0.01;
	float value = log10f(zoom);
	BC_FSlider::update(value);
}

void PerspectiveWindow::update_canvas()
{
	int cw = canvas->get_w(), ch = canvas->get_h();
	canvas->clear_box(0, 0, cw, ch);
	int x1, y1, x2, y2, x3, y3, x4, y4;
	calculate_canvas_coords(x1, y1, x2, y2, x3, y3, x4, y4);
	float x0 = cw / 2.0f, y0 = ch / 2.0f;
	float view_zoom = plugin->config.view_zoom;
	float view_x = plugin->config.view_x, view_y = plugin->config.view_y;
	x1 = (x1-x0) * view_zoom + view_x + x0;
	y1 = (y1-y0) * view_zoom + view_y + y0;
	x2 = (x2-x0) * view_zoom + view_x + x0;
	y2 = (y2-y0) * view_zoom + view_y + y0;
	x3 = (x3-x0) * view_zoom + view_x + x0;
	y3 = (y3-y0) * view_zoom + view_y + y0;
	x4 = (x4-x0) * view_zoom + view_x + x0;
	y4 = (y4-y0) * view_zoom + view_y + y0;

	canvas->set_color(RED);
	int vx1 = x0 - x0 * view_zoom + view_x;
	int vy1 = y0 - y0 * view_zoom + view_y;
	int vx2 = vx1 + cw * view_zoom - 1;
	int vy2 = vy1 + ch * view_zoom - 1;
	canvas->draw_line(vx1, vy1, vx2, vy1);
	canvas->draw_line(vx2, vy1, vx2, vy2);
	canvas->draw_line(vx2, vy2, vx1, vy2);
	canvas->draw_line(vx1, vy2, vx1, vy1);

//printf("PerspectiveWindow::update_canvas %d,%d %d,%d %d,%d %d,%d\n",
// x1, y1, x2, y2, x3, y3, x4, y4);
// Draw divisions
	canvas->set_color(WHITE);

#define DIVISIONS 10
	for( int i=0; i<=DIVISIONS; ++i ) {
		canvas->draw_line( // latitude
			x1 + (x4 - x1) * i / DIVISIONS,
			y1 + (y4 - y1) * i / DIVISIONS,
			x2 + (x3 - x2) * i / DIVISIONS,
			y2 + (y3 - y2) * i / DIVISIONS);
		canvas->draw_line( // longitude
			x1 + (x2 - x1) * i / DIVISIONS,
			y1 + (y2 - y1) * i / DIVISIONS,
			x4 + (x3 - x4) * i / DIVISIONS,
			y4 + (y3 - y4) * i / DIVISIONS);
	}

// Corners
#define RADIUS 5
	if(plugin->config.current_point == 0)
		canvas->draw_disc(x1 - RADIUS, y1 - RADIUS, RADIUS * 2, RADIUS * 2);
	else
		canvas->draw_circle(x1 - RADIUS, y1 - RADIUS, RADIUS * 2, RADIUS * 2);

	if(plugin->config.current_point == 1)
		canvas->draw_disc(x2 - RADIUS, y2 - RADIUS, RADIUS * 2, RADIUS * 2);
	else
		canvas->draw_circle(x2 - RADIUS, y2 - RADIUS, RADIUS * 2, RADIUS * 2);

	if(plugin->config.current_point == 2)
		canvas->draw_disc(x3 - RADIUS, y3 - RADIUS, RADIUS * 2, RADIUS * 2);
	else
		canvas->draw_circle(x3 - RADIUS, y3 - RADIUS, RADIUS * 2, RADIUS * 2);

	if(plugin->config.current_point == 3)
		canvas->draw_disc(x4 - RADIUS, y4 - RADIUS, RADIUS * 2, RADIUS * 2);
	else
		canvas->draw_circle(x4 - RADIUS, y4 - RADIUS, RADIUS * 2, RADIUS * 2);

	canvas->flash();
}

void PerspectiveWindow::update_mode()
{
	mode_perspective->update(plugin->config.mode == AffineEngine::PERSPECTIVE);
	mode_sheer->update(plugin->config.mode == AffineEngine::SHEER);
	mode_stretch->update(plugin->config.mode == AffineEngine::STRETCH);
	forward->update(plugin->config.forward);
	reverse->update(!plugin->config.forward);
}

void PerspectiveWindow::update_coord()
{
	x->update(plugin->get_current_x());
	y->update(plugin->get_current_y());
}

void PerspectiveWindow::update_view_zoom()
{
	zoom_view->update(plugin->config.view_zoom);
}

void PerspectiveWindow::reset_view()
{
	plugin->config.view_x = plugin->config.view_y = 0;
	zoom_view->update(plugin->config.view_zoom = 1);
	update_canvas();
	plugin->send_configure_change();
}

void PerspectiveWindow::calculate_canvas_coords(
	int &x1, int &y1, int &x2, int &y2,
	int &x3, int &y3, int &x4, int &y4)
{
	int w = canvas->get_w() - 1;
	int h = canvas->get_h() - 1;
	if( plugin->config.mode == AffineEngine::PERSPECTIVE ||
	    plugin->config.mode == AffineEngine::STRETCH ) {
		x1 = (int)(plugin->config.x1 * w / 100);
		y1 = (int)(plugin->config.y1 * h / 100);
		x2 = (int)(plugin->config.x2 * w / 100);
		y2 = (int)(plugin->config.y2 * h / 100);
		x3 = (int)(plugin->config.x3 * w / 100);
		y3 = (int)(plugin->config.y3 * h / 100);
		x4 = (int)(plugin->config.x4 * w / 100);
		y4 = (int)(plugin->config.y4 * h / 100);
	}
	else {
		x1 = (int)(plugin->config.x1 * w) / 100;
		y1 = 0;
		x2 = x1 + w;
		y2 = 0;
		x4 = (int)(plugin->config.x4 * w) / 100;
		y4 = h;
		x3 = x4 + w;
		y3 = h;
	}
}


PerspectiveCanvas::PerspectiveCanvas(PerspectiveWindow *gui,
	int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->gui = gui;
	state = PerspectiveCanvas::NONE;
}


int PerspectiveCanvas::button_press_event()
{
	if( is_event_win() && cursor_inside() ) {
// Set current point
		int cx = get_cursor_x(), cy = get_cursor_y();
		if( alt_down() && shift_down() ) {
			state = PerspectiveCanvas::DRAG_VIEW;
			start_x = cx;  start_y = cy;
			return 1;
		}

		PerspectiveMain *plugin = gui->plugin;
		int x1, y1, x2, y2, x3, y3, x4, y4;
		gui->calculate_canvas_coords(x1, y1, x2, y2, x3, y3, x4, y4);
		float cw = gui->canvas->get_w(), ch = gui->canvas->get_h();
		float x0 = cw / 2, y0 = ch / 2;
		float view_zoom = plugin->config.view_zoom;
		float view_x = plugin->config.view_x, view_y = plugin->config.view_y;
		int x = (cx-x0 - view_x) / view_zoom + x0;
		int y = (cy-y0 - view_y) / view_zoom + y0;
		float distance1 = DISTANCE(x, y, x1, y1);
		float distance2 = DISTANCE(x, y, x2, y2);
		float distance3 = DISTANCE(x, y, x3, y3);
		float distance4 = DISTANCE(x, y, x4, y4);
		float min = distance1;
		plugin->config.current_point = 0;
		if( distance2 < min ) {
			min = distance2;
			plugin->config.current_point = 1;
		}
		if( distance3 < min ) {
			min = distance3;
			plugin->config.current_point = 2;
		}
		if( distance4 < min ) {
			min = distance4;
			plugin->config.current_point = 3;
		}

		if( plugin->config.mode == AffineEngine::SHEER ) {
			if( plugin->config.current_point == 1 )
				plugin->config.current_point = 0;
			else if( plugin->config.current_point == 2 )
				plugin->config.current_point = 3;
		}
		start_x = x;
		start_y = y;
		if( alt_down() || shift_down() ) {
			state =  alt_down() ?
				PerspectiveCanvas::DRAG_FULL :
				PerspectiveCanvas::ZOOM;
// Get starting positions
			start_x1 = plugin->config.x1;
			start_y1 = plugin->config.y1;
			start_x2 = plugin->config.x2;
			start_y2 = plugin->config.y2;
			start_x3 = plugin->config.x3;
			start_y3 = plugin->config.y3;
			start_x4 = plugin->config.x4;
			start_y4 = plugin->config.y4;
		}
		else {
			state = PerspectiveCanvas::DRAG;
// Get starting positions
			start_x1 = plugin->get_current_x();
			start_y1 = plugin->get_current_y();
		}
		gui->update_coord();
		gui->update_canvas();
		return 1;
	}

	return 0;
}

int PerspectiveCanvas::button_release_event()
{
	if( state == PerspectiveCanvas::NONE ) return 0;
	state = PerspectiveCanvas::NONE;
	return 1;
}

int PerspectiveCanvas::cursor_motion_event()
{
	if( state == PerspectiveCanvas::NONE ) return 0;
	PerspectiveMain *plugin = gui->plugin;
	int cx = get_cursor_x(), cy = get_cursor_y();
	if( state != PerspectiveCanvas::DRAG_VIEW ) {
		float cw = gui->canvas->get_w(), ch = gui->canvas->get_h();
		float x0 = cw / 2, y0 = ch / 2;
		float view_zoom = plugin->config.view_zoom;
		float view_x = plugin->config.view_x, view_y = plugin->config.view_y;
		int x = (cx-x0 - view_x) / view_zoom + x0;
		int y = (cy-y0 - view_y) / view_zoom + y0;
		int w1 = get_w() - 1, h1 = get_h() - 1;

		switch( state ) {
		case PerspectiveCanvas::DRAG:
			plugin->set_current_x((float)(x - start_x) / w1 * 100 + start_x1);
			plugin->set_current_y((float)(y - start_y) / h1 * 100 + start_y1);
			break;
		case PerspectiveCanvas::DRAG_FULL:
			plugin->config.x1 = ((float)(x - start_x) / w1 * 100 + start_x1);
			plugin->config.y1 = ((float)(y - start_y) / h1 * 100 + start_y1);
			plugin->config.x2 = ((float)(x - start_x) / w1 * 100 + start_x2);
			plugin->config.y2 = ((float)(y - start_y) / h1 * 100 + start_y2);
			plugin->config.x3 = ((float)(x - start_x) / w1 * 100 + start_x3);
			plugin->config.y3 = ((float)(y - start_y) / h1 * 100 + start_y3);
			plugin->config.x4 = ((float)(x - start_x) / w1 * 100 + start_x4);
			plugin->config.y4 = ((float)(y - start_y) / h1 * 100 + start_y4);
			break;
		case PerspectiveCanvas::ZOOM:
			float center_x = (start_x1 + start_x2 + start_x3 + start_x4) / 4;
			float center_y = (start_y1 + start_y2 + start_y3 + start_y4) / 4;
			float zoom = (float)(get_cursor_y() - start_y + 640) / 640;
			plugin->config.x1 = center_x + (start_x1 - center_x) * zoom;
			plugin->config.y1 = center_y + (start_y1 - center_y) * zoom;
			plugin->config.x2 = center_x + (start_x2 - center_x) * zoom;
			plugin->config.y2 = center_y + (start_y2 - center_y) * zoom;
			plugin->config.x3 = center_x + (start_x3 - center_x) * zoom;
			plugin->config.y3 = center_y + (start_y3 - center_y) * zoom;
			plugin->config.x4 = center_x + (start_x4 - center_x) * zoom;
			plugin->config.y4 = center_y + (start_y4 - center_y) * zoom;
			break;
		}
		gui->update_coord();
	}
	else {
		plugin->config.view_x += cx - start_x;
		plugin->config.view_y += cy - start_y;
		start_x = cx;  start_y = cy;
	}
	gui->update_canvas();
	plugin->send_configure_change();
	return 1;
}


PerspectiveCoord::PerspectiveCoord(PerspectiveWindow *gui,
	int x, int y, float value, int is_x)
 : BC_TumbleTextBox(gui, value, (float)-100, (float)200, x, y, 100)
{
	this->gui = gui;
	this->is_x = is_x;
}

int PerspectiveCoord::handle_event()
{
	PerspectiveMain *plugin = gui->plugin;
	float v = atof(get_text());
	if( is_x )
		plugin->set_current_x(v);
	else
		plugin->set_current_y(v);
	gui->update_canvas();
	plugin->send_configure_change();
	return 1;
}


PerspectiveReset::PerspectiveReset(PerspectiveWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->gui = gui;
}
int PerspectiveReset::handle_event()
{
	gui->reset_view();

	PerspectiveMain *plugin = gui->plugin;
	plugin->config.x1 = 0;    plugin->config.y1 = 0;
	plugin->config.x2 = 100;  plugin->config.y2 = 0;
	plugin->config.x3 = 100;  plugin->config.y3 = 100;
	plugin->config.x4 = 0;    plugin->config.y4 = 100;
	gui->update_canvas();
	gui->update_coord();
	plugin->send_configure_change();
	return 1;
}


PerspectiveMode::PerspectiveMode(PerspectiveWindow *gui,
	int x, int y, int value, char *text)
 : BC_Radial(x, y, gui->plugin->config.mode == value, text)
{
	this->gui = gui;
	this->value = value;
}
int PerspectiveMode::handle_event()
{
	PerspectiveMain *plugin = gui->plugin;
	plugin->config.mode = value;
	gui->update_mode();
	gui->update_canvas();
	plugin->send_configure_change();
	return 1;
}


PerspectiveDirection::PerspectiveDirection(PerspectiveWindow *gui,
	int x, int y, int value, char *text)
 : BC_Radial(x, y, gui->plugin->config.forward == value, text)
{
	this->gui = gui;
	this->value = value;
}
int PerspectiveDirection::handle_event()
{
	PerspectiveMain *plugin = gui->plugin;
	plugin->config.forward = value;
	gui->update_mode();
	plugin->send_configure_change();
	return 1;
}


int PerspectiveAffineItem::handle_event()
{
	((PerspectiveAffine *)get_popup_menu())->update(id);
	return 1;
}

PerspectiveAffine::PerspectiveAffine(PerspectiveWindow *gui, int x, int y)
 : BC_PopupMenu(x, y, 100, "", 1)
{
	this->gui = gui;
	affine_modes[AffineEngine::AF_DEFAULT]     = _("default");
	affine_modes[AffineEngine::AF_NEAREST]     = _("Nearest");
	affine_modes[AffineEngine::AF_LINEAR]      = _("Linear");
	affine_modes[AffineEngine::AF_CUBIC]       = _("Cubic");
	mode = -1;
}
PerspectiveAffine::~PerspectiveAffine()
{
	int id = total_items();
	while( --id >= 0 )
		remove_item(get_item(id));
	for( int id=0; id<n_modes; ++id )
		delete affine_items[id];
}
void PerspectiveAffine::affine_item(int id)
{
	affine_items[id] = new PerspectiveAffineItem(affine_modes[id], id);
	add_item(affine_items[id]);
}

void PerspectiveAffine::create_objects()
{
	affine_item(AffineEngine::AF_DEFAULT);
	affine_item(AffineEngine::AF_NEAREST);
	affine_item(AffineEngine::AF_LINEAR);
	affine_item(AffineEngine::AF_CUBIC);
	PerspectiveMain *plugin = gui->plugin;
	update(plugin->config.smoothing, 0);
}

void PerspectiveAffine::update(int mode, int send)
{
	if( this->mode == mode ) return;
	this->mode = mode;
	set_text(affine_modes[mode]);
	PerspectiveMain *plugin = gui->plugin;
	plugin->config.smoothing = mode;
	if( send ) plugin->send_configure_change();
}

PerspectiveMain::PerspectiveMain(PluginServer *server)
 : PluginVClient(server)
{

	engine = 0;
	temp = 0;
}

PerspectiveMain::~PerspectiveMain()
{

	if(engine) delete engine;
	if(temp) delete temp;
}

const char* PerspectiveMain::plugin_title() { return N_("Perspective"); }
int PerspectiveMain::is_realtime() { return 1; }


NEW_WINDOW_MACRO(PerspectiveMain, PerspectiveWindow)

LOAD_CONFIGURATION_MACRO(PerspectiveMain, PerspectiveConfig)

void PerspectiveMain::update_gui()
{
	if( !thread ) return;
//printf("PerspectiveMain::update_gui 1\n");
	thread->window->lock_window();
	PerspectiveWindow *gui = (PerspectiveWindow*)thread->window;
//printf("PerspectiveMain::update_gui 2\n");
	load_configuration();
	gui->update_coord();
	gui->update_mode();
	gui->update_view_zoom();
	gui->update_canvas();
	thread->window->unlock_window();
//printf("PerspectiveMain::update_gui 3\n");
}





void PerspectiveMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("PERSPECTIVE");

	output.tag.set_property("X1", config.x1);
	output.tag.set_property("X2", config.x2);
	output.tag.set_property("X3", config.x3);
	output.tag.set_property("X4", config.x4);
	output.tag.set_property("Y1", config.y1);
	output.tag.set_property("Y2", config.y2);
	output.tag.set_property("Y3", config.y3);
	output.tag.set_property("Y4", config.y4);

	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("VIEW_X", config.view_x);
	output.tag.set_property("VIEW_Y", config.view_y);
	output.tag.set_property("VIEW_ZOOM", config.view_zoom);
	output.tag.set_property("SMOOTHING", config.smoothing);
	output.tag.set_property("FORWARD", config.forward);
	output.tag.set_property("WINDOW_W", config.window_w);
	output.tag.set_property("WINDOW_H", config.window_h);
	output.append_tag();
	output.tag.set_title("/PERSPECTIVE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void PerspectiveMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	int result = 0;

	while(!(result = input.read_tag()) ) {
		if(input.tag.title_is("PERSPECTIVE")) {
			config.x1 = input.tag.get_property("X1", config.x1);
			config.x2 = input.tag.get_property("X2", config.x2);
			config.x3 = input.tag.get_property("X3", config.x3);
			config.x4 = input.tag.get_property("X4", config.x4);
			config.y1 = input.tag.get_property("Y1", config.y1);
			config.y2 = input.tag.get_property("Y2", config.y2);
			config.y3 = input.tag.get_property("Y3", config.y3);
			config.y4 = input.tag.get_property("Y4", config.y4);

			config.mode = input.tag.get_property("MODE", config.mode);
			config.view_x = input.tag.get_property("VIEW_X", config.view_x);
			config.view_y = input.tag.get_property("VIEW_Y", config.view_y);
			config.view_zoom = input.tag.get_property("VIEW_ZOOM", config.view_zoom);
			config.smoothing = input.tag.get_property("SMOOTHING", config.smoothing);
			config.forward = input.tag.get_property("FORWARD", config.forward);
			config.window_w = input.tag.get_property("WINDOW_W", config.window_w);
			config.window_h = input.tag.get_property("WINDOW_H", config.window_h);
		}
	}
}

float PerspectiveMain::get_current_x()
{
	switch( config.current_point ) {
	case 0: return config.x1;
	case 1: return config.x2;
	case 2: return config.x3;
	case 3: return config.x4;
	}
	return 0;
}

float PerspectiveMain::get_current_y()
{
	switch( config.current_point ) {
	case 0: return config.y1;
	case 1: return config.y2;
	case 2: return config.y3;
	case 3: return config.y4;
	}
	return 0;
}

void PerspectiveMain::set_current_x(float value)
{
	switch( config.current_point ) {
	case 0: config.x1 = value; break;
	case 1: config.x2 = value; break;
	case 2: config.x3 = value; break;
	case 3: config.x4 = value; break;
	}
}

void PerspectiveMain::set_current_y(float value)
{
	switch( config.current_point ) {
	case 0: config.y1 = value; break;
	case 1: config.y2 = value; break;
	case 2: config.y3 = value; break;
	case 3: config.y4 = value; break;
	}
}

int PerspectiveMain::process_buffer(VFrame *frame,
	int64_t start_position, double frame_rate)
{
	/*int need_reconfigure =*/ load_configuration();
	int smoothing = config.smoothing;
// default smoothing uses opengl if possible
	int use_opengl = smoothing != AffineEngine::AF_DEFAULT ? 0 :
// Opengl does some funny business with stretching.
		config.mode == AffineEngine::PERSPECTIVE ||
		config.mode == AffineEngine::SHEER ? get_use_opengl() : 0;

	read_frame(frame, 0, start_position, frame_rate, use_opengl);

// Do nothing

	if( EQUIV(config.x1, 0)   && EQUIV(config.y1, 0) &&
	    EQUIV(config.x2, 100) && EQUIV(config.y2, 0) &&
	    EQUIV(config.x3, 100) && EQUIV(config.y3, 100) &&
	    EQUIV(config.x4, 0)   && EQUIV(config.y4, 100) )
		return 1;

	if( !engine ) {
		int cpus = get_project_smp() + 1;
		engine = new AffineEngine(cpus, cpus);
	}
	engine->set_interpolation(smoothing);

	if( use_opengl )
		return run_opengl();

	this->input = frame;
	this->output = frame;

	int w = frame->get_w(), need_w = w;
	int h = frame->get_h(), need_h = h;
	int color_model = frame->get_color_model();
	switch( config.mode ) {
	case AffineEngine::STRETCH:
		need_w *= AFFINE_OVERSAMPLE;
		need_h *= AFFINE_OVERSAMPLE;
	case AffineEngine::SHEER:
	case AffineEngine::PERSPECTIVE:
		if( temp ) {
			if( temp->get_w() != need_w || temp->get_h() != need_h ||
			    temp->get_color_model() != color_model ) {
				delete temp;  temp = 0;
			}
		}
		if( !temp )
			temp = new VFrame(need_w, need_h, color_model, 0);
		break;
	}
	switch( config.mode ) {
	case AffineEngine::STRETCH:
		temp->clear_frame();
		break;
	case AffineEngine::PERSPECTIVE:
	case AffineEngine::SHEER:
		temp->copy_from(input);
		input = temp;
		output->clear_frame();
		break;
	default:
		delete temp;  temp = 0;
		break;
	}

	engine->process(output, input, temp, config.mode,
		config.x1, config.y1, config.x2, config.y2,
		config.x3, config.y3, config.x4, config.y4,
		config.forward);

// Resample

	if( config.mode == AffineEngine::STRETCH ) {

#define RESAMPLE(tag, type, components, chroma_offset) \
case tag: { \
    int os = AFFINE_OVERSAMPLE, os2 = os*os; \
    for( int i=0; i<h; ++i ) { \
        type *out_row = (type*)output->get_rows()[i]; \
        type *in_row1 = (type*)temp->get_rows()[i * os]; \
        type *in_row2 = (type*)temp->get_rows()[i * os + 1]; \
        for( int j=0; j<w; ++j ) { \
            out_row[0] = \
                ( in_row1[0] + in_row1[components + 0] +  \
                  in_row2[0] + in_row2[components + 0] ) / os2; \
            out_row[1] = \
                ( in_row1[1] + in_row1[components + 1] + \
                  in_row2[1] + in_row2[components + 1] ) / os2; \
            out_row[2] = \
                ( in_row1[2] + in_row1[components + 2] + \
                  in_row2[2] + in_row2[components + 2] ) / os2; \
            if( components == 4 ) { \
                out_row[3] = \
		    ( in_row1[3] + in_row1[components + 3] +  \
                      in_row2[3] + in_row2[components + 3] ) / os2; \
            } \
            out_row += components; \
            in_row1 += components * os; \
            in_row2 += components * os; \
        } \
    } \
} break

		switch( frame->get_color_model() ) {
		RESAMPLE( BC_RGB_FLOAT, float, 3, 0 );
		RESAMPLE( BC_RGB888, unsigned char, 3, 0 );
		RESAMPLE( BC_RGBA_FLOAT, float, 4, 0 );
		RESAMPLE( BC_RGBA8888, unsigned char, 4, 0 );
		RESAMPLE( BC_YUV888, unsigned char, 3, 0x80 );
		RESAMPLE( BC_YUVA8888, unsigned char, 4, 0x80 );
		RESAMPLE( BC_RGB161616, uint16_t, 3, 0 );
		RESAMPLE( BC_RGBA16161616, uint16_t, 4, 0 );
		RESAMPLE( BC_YUV161616, uint16_t, 3, 0x8000 );
		RESAMPLE( BC_YUVA16161616, uint16_t, 4, 0x8000 );
		}
	}

	return 0;
}


int PerspectiveMain::handle_opengl()
{
#ifdef HAVE_GL
	engine->set_opengl(1);
	engine->process(get_output(), get_output(), get_output(), config.mode,
		config.x1, config.y1, config.x2, config.y2,
		config.x3, config.y3, config.x4, config.y4,
		config.forward);
	engine->set_opengl(0);
#endif
	return 0;
}

