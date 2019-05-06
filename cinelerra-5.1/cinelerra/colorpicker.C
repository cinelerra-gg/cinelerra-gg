
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#include "bcbutton.h"
#include "bccapture.h"
#include "bccolors.h"
#include "bcdisplayinfo.h"
#include "colorpicker.h"
#include "condition.h"
#include "keys.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.h"
#include "bccolors.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>

#define PALETTE_DATA "palette.dat"

ColorPicker::ColorPicker(int do_alpha, const char *title)
 : BC_DialogThread()
{
	this->title = title;
	this->do_alpha = do_alpha;
	this->ok_cancel = 0;
	this->output = this->orig_color = BLACK;
	this->alpha = this->orig_alpha = 255;
}

ColorPicker::~ColorPicker()
{
	close_window();
}

void ColorPicker::start_window(int output, int alpha, int ok_cancel)
{
	if( running() ) {
		ColorWindow *gui = (ColorWindow *)get_gui();
		if( gui ) {
			gui->lock_window("ColorPicker::start_window");
			gui->raise_window(1);
			gui->unlock_window();
		}
		return;
	}
	this->orig_color = output;
	this->orig_alpha = alpha;
	this->output = output;
	this->alpha = alpha;
	this->ok_cancel = ok_cancel;
	start();
}

BC_Window* ColorPicker::new_gui()
{
	char window_title[BCTEXTLEN];
	strcpy(window_title, _(PROGRAM_NAME ": "));
	strcat(window_title, title ? title : _("Color Picker"));
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x() + 25;
	int y = display_info.get_abs_cursor_y() - 100;
	int w = 540, h = 330;
	if( ok_cancel )
		h += bmax(BC_OKButton::calculate_h(),BC_CancelButton::calculate_h());
	int root_w = display_info.get_root_w(), root_h = display_info.get_root_h();
	if( x+w > root_w ) x = root_w - w;
	if( y+h > root_h ) y = root_h - h;
	if( x < 0 ) x = 0;
	if( y < 0 ) y = 0;
	ColorWindow *gui = new ColorWindow(this, x, y, w, h, window_title);
	gui->create_objects();
	return gui;
}

void ColorPicker::update_gui(int output, int alpha)
{
	ColorWindow *gui = (ColorWindow *)get_gui();
	if( !gui ) return;
	gui->lock_window("ColorPicker::update_gui");
	this->output = output;
	this->alpha = alpha;
	gui->change_values();
	gui->update_display();
	gui->unlock_window();
}

int ColorPicker::handle_new_color(int output, int alpha)
{
	printf("ColorPicker::handle_new_color undefined.\n");
	return 1;
}


ColorWindow::ColorWindow(ColorPicker *thread, int x, int y, int w, int h, const char *title)
 : BC_Window(title, x, y, w, h, w, h, 0, 0, 1)
{
	this->thread = thread;
	wheel = 0;
	wheel_value = 0;
	output = 0;

	hue = 0; sat = 0; val = 0;
	red = 0; grn = 0; blu = 0;
	lum = 0; c_r = 0; c_b = 0;
	alpha = 0;

	hsv_h = 0;  hsv_s = 0;  hsv_v = 0;
	rgb_r = 0;  rgb_g = 0;  rgb_b = 0;
	yuv_y = 0;  yuv_u = 0;  yuv_v = 0;
	aph_a = 0;

	button_grabbed = 0;
}
ColorWindow::~ColorWindow()
{
	delete hsv_h;  delete hsv_s;  delete hsv_v;
	delete rgb_r;  delete rgb_g;  delete rgb_b;
	delete yuv_y;  delete yuv_u;  delete yuv_v;
	delete aph_a;

	if( button_grabbed ) {
		ungrab_buttons();
		ungrab_cursor();
	}
	update_history(rgb888());
	save_history();
}

void ColorWindow::create_objects()
{
	int x0 = 10, y0 = 10;
	lock_window("ColorWindow::create_objects");
	change_values();

	int x = x0, y = y0;
	add_tool(wheel = new PaletteWheel(this, x, y));
	wheel->create_objects();

	x += 180;  add_tool(wheel_value = new PaletteWheelValue(this, x, y));
	wheel_value->create_objects();
	x = x0;
	y += 180;  add_tool(output = new PaletteOutput(this, x, y));
	output->create_objects();
	y += output->get_h() + 20;

	load_history();  int x1 = x;
	add_tool(hex_btn = new PaletteHexButton(this, x1, y));
	char hex[BCSTRLEN];  sprintf(hex,"%06x",thread->output);
	x1 += hex_btn->get_w() + 5;
	add_tool(hex_box = new PaletteHex(this, x1, y, hex));
	x1 += hex_box->get_w() + 15;
	add_tool(grab_btn = new PaletteGrabButton(this, x1, y));
	y += hex_box->get_h() + 15;
	add_tool(history = new PaletteHistory(this, 10, y));

	x += 240;
	add_tool(new BC_Title(x, y =y0, C_("H:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=25, C_("S:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=25, D_("colorpicker_value#V:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=40, C_("R:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=25, C_("G:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=25, C_("B:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=40, C_("Y:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=25, C_("U:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=25, D_("colorpicker_Cr#V:"), SMALLFONT));
	if( thread->do_alpha )
		add_tool(new BC_Title(x, y+=40, C_("A:"), SMALLFONT));
	x += 24;
	add_tool(hue = new PaletteHue(this, x, y= y0));
	add_tool(sat = new PaletteSat(this, x, y+=25));
	add_tool(val = new PaletteVal(this, x, y+=25));
	add_tool(red = new PaletteRed(this, x, y+=40));
	add_tool(grn = new PaletteGrn(this, x, y+=25));
	add_tool(blu = new PaletteBlu(this, x, y+=25));
	add_tool(lum = new PaletteLum(this, x, y+=40));
	add_tool(c_r = new PaletteCr (this, x, y+=25));
	add_tool(c_b = new PaletteCb (this, x, y+=25));
	if( thread->do_alpha )
		add_tool(alpha = new PaletteAlpha(this, x, y+=40));

	x += hue->get_w() + 10;
	hsv_h = new PaletteHSV(this, x,y= y0, hsv.h, 0, 360);
	hsv_h->create_objects();  hsv_h->set_tooltip(_("Hue"));
	hsv_s = new PaletteHSV(this, x,y+=25, hsv.s, 0, 1);
	hsv_s->create_objects();  hsv_s->set_tooltip(_("Saturation"));
	hsv_v = new PaletteHSV(this, x,y+=25, hsv.v, 0, 1);
	hsv_v->create_objects();  hsv_v->set_tooltip(_("Value"));
	rgb_r = new PaletteRGB(this, x,y+=40, rgb.r, 0, 1);
	rgb_r->create_objects();  rgb_r->set_tooltip(_("Red"));
	rgb_g = new PaletteRGB(this, x,y+=25, rgb.g, 0, 1);
	rgb_g->create_objects();  rgb_g->set_tooltip(_("Green"));
	rgb_b = new PaletteRGB(this, x,y+=25, rgb.b, 0, 1);
	rgb_b->create_objects();  rgb_b->set_tooltip(_("Blue"));
	yuv_y = new PaletteYUV(this, x,y+=40, yuv.y, 0, 1);
	yuv_y->create_objects();  yuv_y->set_tooltip(_("Luminance"));
	yuv_u = new PaletteYUV(this, x,y+=25, yuv.u, 0, 1);
	yuv_u->create_objects();  yuv_u->set_tooltip(_("Blue Luminance Difference"));
	yuv_v = new PaletteYUV(this, x,y+=25, yuv.v, 0, 1);
	yuv_v->create_objects();  yuv_v->set_tooltip(_("Red Luminance Difference"));
	if( thread->do_alpha ) {
		aph_a = new PaletteAPH(this, x,y+=40, aph, 0, 1);
		aph_a->create_objects();  aph_a->set_tooltip(_("Alpha"));
	}
	if( thread->ok_cancel ) {
		add_tool(new BC_OKButton(this));
		add_tool(new BC_CancelButton(this));
	}
	thread->create_objects(this);

	update_display();
	update_history();
	show_window(1);
	unlock_window();
}


void ColorWindow::change_values()
{
	float r = ((thread->output>>16) & 0xff) / 255.;
	float g = ((thread->output>>8)  & 0xff) / 255.;
	float b = ((thread->output>>0)  & 0xff) / 255.;
	rgb.r = r;  rgb.g = g;  rgb.b = b;
	aph = (float)thread->alpha / 255;
	update_rgb(rgb.r, rgb.g, rgb.b);
}


int ColorWindow::close_event()
{
	set_done(thread->ok_cancel ? 1 : 0);
	return 1;
}


void ColorWindow::update_rgb()
{
	update_rgb(rgb.r, rgb.g, rgb.b);
	update_display();
}
void ColorWindow::update_hsv()
{
	update_hsv(hsv.h, hsv.s, hsv.v);
	update_display();
}
void ColorWindow::update_yuv()
{
	update_yuv(yuv.y, yuv.u, yuv.v);
	update_display();
}

void ColorWindow::update_display()
{
	wheel->draw(wheel->oldhue, wheel->oldsaturation);
	wheel->oldhue = hsv.h;
	wheel->oldsaturation = hsv.s;
	wheel->draw(hsv.h, hsv.s);
	wheel->flash();
	wheel_value->draw(hsv.h, hsv.s, hsv.v);
	wheel_value->flash();
	output->draw();
	output->flash();

	hue->update((int)hsv.h);
	sat->update(hsv.s);
	val->update(hsv.v);

	red->update(rgb.r);
	grn->update(rgb.g);
	blu->update(rgb.b);

	lum->update(yuv.y);
	c_r->update(yuv.u);
	c_b->update(yuv.v);

	hsv_h->update(hsv.h);
	hsv_s->update(hsv.s);
	hsv_v->update(hsv.v);
	rgb_r->update(rgb.r);
	rgb_g->update(rgb.g);
	rgb_b->update(rgb.b);
	yuv_y->update(yuv.y);
	yuv_u->update(yuv.u);
	yuv_v->update(yuv.v);
	hex_box->update();

	if( thread->do_alpha ) {
		alpha->update(aph);
		aph_a->update(aph);
	}
}

int ColorWindow::handle_event()
{
	thread->handle_new_color(rgb888(), alpha8());
	return 1;
}

void ColorWindow::get_screen_sample()
{
	int cx, cy;
	get_abs_cursor(cx, cy);
	BC_Capture capture_bitmap(1, 1, 0);
	VFrame vframe(1,1,BC_RGB888);
	capture_bitmap.capture_frame(&vframe, cx,cy);
	unsigned char *data = vframe.get_data();
	rgb.r = data[0]/255.;  rgb.g = data[1]/255.;  rgb.b = data[2]/255.;
	update_rgb();
}

int ColorWindow::cursor_motion_event()
{
	if( button_grabbed && get_button_down() ) {
		get_screen_sample();
		return 1;
	}
	return 0;
}

int ColorWindow::button_press_event()
{
	if( button_grabbed ) {
		get_screen_sample();
		return 1;
	}
	return 0;
}

int ColorWindow::button_release_event()
{
	if( button_grabbed ) {
		ungrab_buttons();
		ungrab_cursor();
		grab_btn->enable();
		button_grabbed = 0;
		update_history();
		return handle_event();
	}
	return 1;
}

void ColorWindow::update_rgb_hex(const char *hex)
{
	unsigned color;
	if( sscanf(hex,"%x",&color) == 1 ) {
		if( thread->do_alpha ) {
			aph = ((color>>24) & 0xff) / 255.;
			aph_a->update(aph);
		}
		float r = ((color>>16) & 0xff) / 255.;
		float g = ((color>>8)  & 0xff) / 255.;
		float b = ((color>>0)  & 0xff) / 255.;
		rgb.r = r;  rgb.g = g;  rgb.b = b;
		update_rgb();
		update_history();
		handle_event();
	}
}


PaletteWheel::PaletteWheel(ColorWindow *window, int x, int y)
 : BC_SubWindow(x, y, 170, 170)
{
	this->window = window;
	oldhue = 0;
	oldsaturation = 0;
	button_down = 0;
}

PaletteWheel::~PaletteWheel()
{
}

int PaletteWheel::button_press_event()
{
	if( get_cursor_x() >= 0 && get_cursor_x() < get_w() &&
		get_cursor_y() >= 0 && get_cursor_y() < get_h() &&
		is_event_win() ) {
		button_down = 1;
		cursor_motion_event();
		return 1;
	}
	return 0;
}

int PaletteWheel::cursor_motion_event()
{
	int x1, y1, distance;
	if( button_down && is_event_win() ) {
		float h = get_angle(get_w()/2, get_h()/2, get_cursor_x(), get_cursor_y());
		bclamp(h, 0, 359.999);  window->hsv.h = h;
		x1 = get_w() / 2 - get_cursor_x();
		y1 = get_h() / 2 - get_cursor_y();
		distance = (int)sqrt(x1 * x1 + y1 * y1);
		float s = (float)distance / (get_w() / 2);
		bclamp(s, 0, 1);  window->hsv.s = s;
		window->hsv.v = 1;
		window->update_hsv();
		window->handle_event();
		return 1;
	}
	return 0;
}

int PaletteWheel::button_release_event()
{
	if( button_down ) {
		button_down = 0;
		return 1;
	}
	return 0;
}

void PaletteWheel::create_objects()
{
// Upper right
//printf("PaletteWheel::create_objects 1\n");
	float h, s, v = 1;
	float r, g, b;
	float x1, y1, x2, y2;
	float distance;
	int default_r, default_g, default_b;
	VFrame frame(0, -1, get_w(), get_h(), BC_RGBA8888, -1);
	x1 = get_w() / 2;
	y1 = get_h() / 2;
	default_r = (get_resources()->get_bg_color() & 0xff0000) >> 16;
	default_g = (get_resources()->get_bg_color() & 0xff00) >> 8;
	default_b = (get_resources()->get_bg_color() & 0xff);
//printf("PaletteWheel::create_objects 1\n");

	int highlight_r = (get_resources()->button_light & 0xff0000) >> 16;
	int highlight_g = (get_resources()->button_light & 0xff00) >> 8;
	int highlight_b = (get_resources()->button_light & 0xff);

	for( y2 = 0; y2 < get_h(); y2++ ) {
		unsigned char *row = (unsigned char*)frame.get_rows()[(int)y2];
		for( x2 = 0; x2 < get_w(); x2++ ) {
			distance = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
			if( distance > x1 ) {
				row[(int)x2 * 4] = default_r;
				row[(int)x2 * 4 + 1] = default_g;
				row[(int)x2 * 4 + 2] = default_b;
				row[(int)x2 * 4 + 3] = 0;
			}
			else
			if( distance > x1 - 1 ) {
				int r_i, g_i, b_i;
				if( get_h() - y2 < x2 ) {
					r_i = highlight_r;
					g_i = highlight_g;
					b_i = highlight_b;
				}
				else {
					r_i = 0;
					g_i = 0;
					b_i = 0;
				}

				row[(int)x2 * 4] = r_i;
				row[(int)x2 * 4 + 1] = g_i;
				row[(int)x2 * 4 + 2] = b_i;
				row[(int)x2 * 4 + 3] = 255;
			}
			else {
				h = get_angle(x1, y1, x2, y2);
				s = distance / x1;
				HSV::hsv_to_rgb(r, g, b, h, s, v);
				row[(int)x2 * 4] = (int)(r * 255);
				row[(int)x2 * 4 + 1] = (int)(g * 255);
				row[(int)x2 * 4 + 2] = (int)(b * 255);
				row[(int)x2 * 4 + 3] = 255;
			}
		}
	}
//printf("PaletteWheel::create_objects 1\n");

	draw_vframe(&frame,
		0,
		0,
		get_w(),
		get_h(),
		0,
		0,
		get_w(),
		get_h(),
		0);
//printf("PaletteWheel::create_objects 1\n");

	oldhue = window->hsv.h;
	oldsaturation = window->hsv.s;
//printf("PaletteWheel::create_objects 1\n");
	draw(oldhue, oldsaturation);
//printf("PaletteWheel::create_objects 1\n");
	flash();
//printf("PaletteWheel::create_objects 2\n");
}

float PaletteWheel::torads(float angle)
{
	return (float)angle / 360 * 2 * M_PI;
}


int PaletteWheel::draw(float hue, float saturation)
{
	int x, y, w, h;
	x = w = get_w() / 2;
	y = h = get_h() / 2;

	if( hue > 0 && hue < 90 ) {
		x = (int)(w - w * cos(torads(90 - hue)) * saturation);
		y = (int)(h - h * sin(torads(90 - hue)) * saturation);
	}
	else if( hue > 90 && hue < 180 ) {
		x = (int)(w - w * cos(torads(hue - 90)) * saturation);
		y = (int)(h + h * sin(torads(hue - 90)) * saturation);
	}
	else if( hue > 180 && hue < 270 ) {
		x = (int)(w + w * cos(torads(270 - hue)) * saturation);
		y = (int)(h + h * sin(torads(270 - hue)) * saturation);
	}
	else if( hue > 270 && hue < 360 ) {
		x = (int)(w + w * cos(torads(hue - 270)) * saturation);
		y = (int)(h - w * sin(torads(hue - 270)) * saturation);
	}
	else if( hue == 0 ) {
		x = w;
		y = (int)(h - h * saturation);
	}
	else if( hue == 90 ) {
		x = (int)(w - w * saturation);
		y = h;
	}
	else if( hue == 180 ) {
		x = w;
		y = (int)(h + h * saturation);
	}
	else if( hue == 270 ) {
		x = (int)(w + w * saturation);
		y = h;
	}

	set_inverse();
	set_color(WHITE);
	draw_circle(x - 5, y - 5, 10, 10);
	set_opaque();
	return 0;
}

int PaletteWheel::get_angle(float x1, float y1, float x2, float y2)
{
	float result = -atan2(x2 - x1, y1 - y2) * (360 / M_PI / 2);
	if( result < 0 )
		result += 360;
	return (int)result;
}

PaletteWheelValue::PaletteWheelValue(ColorWindow *window, int x, int y)
 : BC_SubWindow(x, y, 40, 170, BLACK)
{
	this->window = window;
	button_down = 0;
}
PaletteWheelValue::~PaletteWheelValue()
{
	delete frame;
}

void PaletteWheelValue::create_objects()
{
	frame = new VFrame(get_w(), get_h(), BC_RGB888);
	draw(window->hsv.h, window->hsv.s, window->hsv.v);
	flash();
}

int PaletteWheelValue::button_press_event()
{
//printf("PaletteWheelValue::button_press 1 %d\n", is_event_win());
	if( get_cursor_x() >= 0 && get_cursor_x() < get_w() &&
		get_cursor_y() >= 0 && get_cursor_y() < get_h() &&
		is_event_win() ) {
//printf("PaletteWheelValue::button_press 2\n");
		button_down = 1;
		cursor_motion_event();
		return 1;
	}
	return 0;
}

int PaletteWheelValue::cursor_motion_event()
{
	if( button_down && is_event_win() ) {
//printf("PaletteWheelValue::cursor_motion 1\n");
		float v = 1.0 - (float)(get_cursor_y() - 2) / (get_h() - 4);
		bclamp(v, 0, 1);  window->hsv.v = v;
		window->update_hsv();
		window->handle_event();
		return 1;
	}
	return 0;
}

int PaletteWheelValue::button_release_event()
{
	if( button_down ) {
//printf("PaletteWheelValue::button_release 1\n");
		button_down = 0;
		return 1;
	}
	return 0;
}

int PaletteWheelValue::draw(float hue, float saturation, float value)
{
	float r_f, g_f, b_f;
	int i, j, r, g, b;

	for( i = get_h() - 3; i >= 2; i-- ) {
		unsigned char *row = (unsigned char*)frame->get_rows()[i];
		HSV::hsv_to_rgb(r_f, g_f, b_f, hue, saturation,
			1.0 - (float)(i - 2) / (get_h() - 4));
		r = (int)(r_f * 255);
		g = (int)(g_f * 255);
		b = (int)(b_f * 255);
		for( j = 0; j < get_w(); j++ ) {
			row[j * 3] = r;
			row[j * 3 + 1] = g;
			row[j * 3 + 2] = b;
		}
	}

	draw_3d_border(0, 0, get_w(), get_h(), 1);
	draw_vframe(frame, 2, 2, get_w() - 4, get_h() - 4,
		2, 2, get_w() - 4, get_h() - 4, 0);
	set_color(BLACK);
	draw_line(2, get_h() - 3 - (int)(value * (get_h() - 5)),
		  get_w() - 3, get_h() - 3 - (int)(value * (get_h() - 5)));
//printf("PaletteWheelValue::draw %d %f\n", __LINE__, value);

	return 0;
}

PaletteOutput::PaletteOutput(ColorWindow *window, int x, int y)
 : BC_SubWindow(x, y, 180, 30, BLACK)
{
	this->window = window;
}
PaletteOutput::~PaletteOutput()
{
}


void PaletteOutput::create_objects()
{
	draw();
	flash();
}

int PaletteOutput::handle_event()
{
	return 1;
}

int PaletteOutput::draw()
{
	set_color(window->rgb888());
	draw_box(2, 2, get_w() - 4, get_h() - 4);
	draw_3d_border(0, 0, get_w(), get_h(), 1);
	return 0;
}

PaletteHue::PaletteHue(ColorWindow *window, int x, int y)
 : BC_ISlider(x, y, 0, 150, 200, 0, 359, (int)(window->hsv.h), 0)
{
	this->window = window;
}
PaletteHue::~PaletteHue()
{
}

int PaletteHue::handle_event()
{
	window->hsv.h = get_value();
	window->update_hsv();
	window->handle_event();
	return 1;
}

PaletteSat::PaletteSat(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1.0, window->hsv.s, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteSat::~PaletteSat()
{
}

int PaletteSat::handle_event()
{
	window->hsv.s = get_value();
	window->update_hsv();
	window->handle_event();
	return 1;
}


PaletteVal::PaletteVal(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1.0, window->hsv.v, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteVal::~PaletteVal()
{
}

int PaletteVal::handle_event()
{
	window->hsv.v = get_value();
	window->update_hsv();
	window->handle_event();
	return 1;
}


PaletteRed::PaletteRed(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->rgb.r, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteRed::~PaletteRed()
{
}

int PaletteRed::handle_event()
{
	window->rgb.r = get_value();
	window->update_rgb();
	window->handle_event();
	return 1;
}

PaletteGrn::PaletteGrn(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->rgb.g, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteGrn::~PaletteGrn()
{
}

int PaletteGrn::handle_event()
{
	window->rgb.g = get_value();
	window->update_rgb();
	window->handle_event();
	return 1;
}

PaletteBlu::PaletteBlu(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->rgb.b, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteBlu::~PaletteBlu()
{
}

int PaletteBlu::handle_event()
{
	window->rgb.b = get_value();
	window->update_rgb();
	window->handle_event();
	return 1;
}

PaletteAlpha::PaletteAlpha(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->aph, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteAlpha::~PaletteAlpha()
{
}

int PaletteAlpha::handle_event()
{
	window->aph = get_value();
	window->aph_a->update(window->aph);
	window->hex_box->update();
	window->handle_event();
	return 1;
}

PaletteLum::PaletteLum(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->yuv.y, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteLum::~PaletteLum()
{
}

int PaletteLum::handle_event()
{
	window->yuv.y = get_value();
	window->update_yuv();
	window->handle_event();
	return 1;
}

PaletteCr::PaletteCr(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->yuv.u, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteCr::~PaletteCr()
{
}

int PaletteCr::handle_event()
{
	window->yuv.u = get_value();
	window->update_yuv();
	window->handle_event();
	return 1;
}

PaletteCb::PaletteCb(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->yuv.v, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteCb::~PaletteCb()
{
}

int PaletteCb::handle_event()
{
	window->yuv.v = get_value();
	window->update_yuv();
	window->handle_event();
	return 1;
}

void ColorWindow::update_rgb(float r, float g, float b)
{
	{ float y, u, v;
	YUV::yuv.rgb_to_yuv_f(r, g, b, y, u, v);
	u += 0.5;  v += 0.5;
	bclamp(y, 0, 1);    yuv.y = y;
	bclamp(u, 0, 1);    yuv.u = u;
	bclamp(v, 0, 1);    yuv.v = v; }
	{ float h, s, v;
	HSV::rgb_to_hsv(r,g,b, h,s,v);
	bclamp(h, 0, 360);  hsv.h = h;
	bclamp(s, 0, 1);    hsv.s = s;
	bclamp(v, 0, 1);    hsv.v = v; }
}

void ColorWindow::update_yuv(float y, float u, float v)
{
	u -= 0.5;  v -= 0.5;
	{ float r, g, b;
	YUV::yuv.yuv_to_rgb_f(r, g, b, y, u, v);
	bclamp(r, 0, 1);   rgb.r = r;
	bclamp(g, 0, 1);   rgb.g = g;
	bclamp(b, 0, 1);   rgb.b = b;
	float h, s, v;
	HSV::rgb_to_hsv(r,g,b, h, s, v);
	bclamp(h, 0, 360); hsv.h = h;
	bclamp(s, 0, 1);   hsv.s = s;
	bclamp(v, 0, 1);   hsv.v = v; }
}

void ColorWindow::update_hsv(float h, float s, float v)
{
	{ float r, g, b;
	HSV::hsv_to_rgb(r,g,b, h,s,v);
	bclamp(r, 0, 1);   rgb.r = r;
	bclamp(g, 0, 1);   rgb.g = g;
	bclamp(b, 0, 1);   rgb.b = b;
	float y, u, v;
	YUV::yuv.rgb_to_yuv_f(r, g, b, y, u, v);
	u += 0.5;  v += 0.5;
	bclamp(y, 0, 1);   yuv.y = y;
	bclamp(u, 0, 1);   yuv.u = u;
	bclamp(v, 0, 1);   yuv.v = v; }
}

void ColorWindow::load_history()
{
	char history_path[BCTEXTLEN];
	MWindow::create_defaults_path(history_path,PALETTE_DATA);
	FILE *fp = fopen(history_path,"r");
	int i=0;
	if( fp ) {
		while( i < PALLETTE_HISTORY_SIZE ) {
			char line[BCSTRLEN];
			if( !fgets(line,sizeof(line)-1,fp) ) break;
			line[sizeof(line)-1] = 0;
			if( sscanf(line, "%x",&palette_history[i]) != 1 ) break;
			++i;
		}
		fclose(fp);
	}
	int r = 0, g = 0, b = 0;
	float v0 = 0, v1 = 1;
	while( i < PALLETTE_HISTORY_SIZE ) {
		int grey_code = i ^ (i>>1);
		r = 255 * ((grey_code&4) ? v0 : v1);
		g = 255 * ((grey_code&2) ? v0 : v1);
		b = 255 * ((grey_code&1) ? v0 : v1);
		int color = (r<<16) | (g<<8) | (b<<0);
		palette_history[i++] = color;
		if( i & 7 ) continue;
		v0 = 0.5f * (v0+.5f);
		v1 = 0.5f * (v1+.5f);
	}
}
void ColorWindow::save_history()
{
	char history_path[BCTEXTLEN];
	MWindow::create_defaults_path(history_path,PALETTE_DATA);
	FILE *fp = fopen(history_path,"w");
	if( fp ) {
		for( int i=0; i<PALLETTE_HISTORY_SIZE; ++i ) {
			fprintf(fp, "%06x\n", palette_history[i]);
		}
		fclose(fp);
	}
}
void ColorWindow::update_history(int color)
{
	int out = palette_history[0];
	palette_history[0] = color;
	for( int i=1; out != color && i<PALLETTE_HISTORY_SIZE; ++i ) {
		int in = out;
		out = palette_history[i];
		palette_history[i] = in;
	}
}
void ColorWindow::update_history()
{
	update_history(rgb888());
	history->update(0);
}
int ColorWindow::rgb888()
{
	int r = 255*rgb.r + 0.5, g = 255*rgb.g + 0.5, b = 255*rgb.b + 0.5;
	bclamp(r, 0, 255);  bclamp(g, 0, 255);  bclamp(b, 0, 255);
	return (r<<16) | (g<<8) | (b<<0);
}
int ColorWindow::alpha8()
{
	int a = 255*aph + 0.5;
	bclamp(a, 0, 255);
	return a;
}

PaletteNum::PaletteNum(ColorWindow *window, int x, int y,
	float &output, float min, float max)
 : BC_TumbleTextBox(window, output, min, max, x, y, 64)
{
	this->window = window;
	this->output = &output;
	set_increment(0.01);
	set_precision(2);
}

PaletteNum::~PaletteNum()
{
}


int PaletteHSV::handle_event()
{
	update_output();
	window->update_hsv();
	window->handle_event();
	return 1;
}

int PaletteRGB::handle_event()
{
	update_output();
	window->update_rgb();
	window->handle_event();
	return 1;
}

int PaletteYUV::handle_event()
{
	update_output();
	window->update_yuv();
	window->handle_event();
	return 1;
}

int PaletteAPH::handle_event()
{
	update_output();
	window->update_display();
	window->handle_event();
	return 1;
}

PaletteHexButton::PaletteHexButton(ColorWindow *window, int x, int y)
 : BC_GenericButton(x, y, 50, "#")
{
	this->window = window;
	set_tooltip(_("hex rgb color"));
}
PaletteHexButton::~PaletteHexButton()
{
}
int PaletteHexButton::handle_event()
{
	const char *hex = window->hex_box->get_text();
	window->update_rgb_hex(hex);
	return 1;
}

PaletteHex::PaletteHex(ColorWindow *window, int x, int y, const char *hex)
 : BC_TextBox(x, y, 100, 1, hex)
{
	this->window = window;
}
PaletteHex::~PaletteHex()
{
}
void PaletteHex::update()
{
	char hex[BCSTRLEN], *cp = hex;
	if( window->thread->do_alpha )
		cp += sprintf(cp,"%02x", window->alpha8());
	sprintf(cp,"%06x",window->rgb888());
	BC_TextBox::update(hex);
}

int PaletteHex::keypress_event()
{
	if( get_keypress() != RETURN )
		return BC_TextBox::keypress_event();
	window->update_rgb_hex(get_text());
	return 1;
}

#include "grabpick_up_png.h"
#include "grabpick_hi_png.h"
#include "grabpick_dn_png.h"

PaletteGrabButton::PaletteGrabButton(ColorWindow *window, int x, int y)
 : BC_Button(x, y, vframes)
{
	this->window = window;
	vframes[0] = new VFramePng(grabpick_up_png);
	vframes[1] = new VFramePng(grabpick_hi_png);
	vframes[2] = new VFramePng(grabpick_dn_png);
	set_tooltip(_("grab from anywhere picker"));
}
PaletteGrabButton::~PaletteGrabButton()
{
	for( int i=0; i<3; ++i )
		delete vframes[i];
}
int PaletteGrabButton::handle_event()
{
	if( window->grab_buttons() ) {
		grab_cursor();
		window->button_grabbed = 1;
		button_press_event(); // redraw face HI
	}
	return 1;
}

PaletteHistory::PaletteHistory(ColorWindow *window, int x, int y)
 : BC_SubWindow(x,y, 200, 24)
{
	this->window = window;
	button_down = 0;
	set_tooltip(_("color history"));
}
PaletteHistory::~PaletteHistory()
{
}
void PaletteHistory::update(int flush)
{
	int x1 = 0, x2 = 0;
	for( int i=0; i<PALLETTE_HISTORY_SIZE; x1=x2 ) {
		int rgb = window->palette_history[i];
		x2 = (++i * get_w())/PALLETTE_HISTORY_SIZE;
		draw_3d_box(x1,0,x2-x1,get_h(),WHITE,BLACK,rgb,LTBLUE,DKBLUE);
	}
	flash(flush);
}

int PaletteHistory::button_press_event()
{
	if( button_down || !is_event_win() ) return 0;
	button_down =  1;
	cursor_motion_event();
	return 1;
}
int PaletteHistory::button_release_event()
{
	if( !button_down || !is_event_win() ) return 0;
	cursor_motion_event();
	if( button_down > 0 ) {
		window->handle_event();
		window->update_display();
		window->update_history();
	}
	button_down =  0;
	return 1;
}
int PaletteHistory::cursor_motion_event()
{
	if( !button_down || !is_event_win() ) return 0;
	hide_tooltip();
	int pick = (PALLETTE_HISTORY_SIZE * get_cursor_x()) / get_w();
	bclamp(pick, 0, PALLETTE_HISTORY_SIZE-1);
	int color = window->palette_history[pick];
	float r = ((color>>16) & 0xff) / 255.;
	float g = ((color>>8)  & 0xff) / 255.;
	float b = ((color>>0)  & 0xff) / 255.;
	if( window->rgb.r != r || window->rgb.g != g || window->rgb.b != b ) {
		window->rgb.r = r;  window->rgb.g = g;  window->rgb.b = b;
		window->update_rgb();
	}
	return 1;
}

int PaletteHistory::cursor_leave_event()
{
	hide_tooltip();
	return 0;
}
int PaletteHistory::repeat_event(int64_t duration)
{
	int result = 0;

	if( duration == get_resources()->tooltip_delay &&
	    get_tooltip() && *get_tooltip() && cursor_above() ) {
		show_tooltip();
		result = 1;
	}
	return result;
}


ColorButton::ColorButton(const char *title,
	int x, int y, int w, int h,
	int color, int alpha, int ok_cancel)
 : BC_Button(x, y, w, vframes)
{
	this->title = title;
	this->color = this->orig_color = color;
	this->alpha = this->orig_alpha = alpha;
	this->ok_cancel = ok_cancel;

	for( int i=0; i<3; ++i ) {
		vframes[i] = new VFrame(w, h, BC_RGBA8888);
		vframes[i]->clear_frame();
	}
	color_picker = 0;
	color_thread = 0;
}

ColorButton::~ColorButton()
{
	delete color_thread;
	delete color_picker;
	for( int i=0; i<3; ++i )
		delete vframes[i];
}

void ColorButton::set_color(int color)
{
	printf("ColorButton::set_color %06x\n", color);
}
void ColorButton::handle_done_event(int result)
{
	color_thread->stop();
}
int ColorButton::handle_new_color(int color, int alpha)
{
	printf("ColorButton::handle_new_color %02x%06x\n", alpha, color);
	return 1;
}

int ColorButton::handle_event()
{
	unlock_window();
	delete color_picker;
	color_picker = new ColorButtonPicker(this);
	orig_color = color;  orig_alpha = alpha;
	color_picker->start_window(color, alpha, ok_cancel);
	if( !color_thread )
		color_thread = new ColorButtonThread(this);
	color_thread->start();
	lock_window("ColorButtonButton::start_color_thread");
	return 1;
}

void ColorButton::close_picker()
{
	if( color_thread ) color_thread->stop();
	delete color_thread;  color_thread = 0;
	delete color_picker;  color_picker = 0;
}

void ColorButton::update_gui(int color)
{
	set_color(color);
	draw_face();
}

ColorButtonPicker::ColorButtonPicker(ColorButton *color_button)
 : ColorPicker(color_button->alpha >= 0 ? 1 : 0, color_button->title)
{
	this->color_button = color_button;
}

ColorButtonPicker::~ColorButtonPicker()
{
}

void ColorButtonPicker::handle_done_event(int result)
{
	color_button->color_thread->stop();
	color_button->handle_done_event(result);
}

int ColorButtonPicker::handle_new_color(int color, int alpha)
{
	color_button->color = color;
	color_button->color_thread->update_lock->unlock();
	color_button->handle_new_color(color, alpha);
	return 1;
}

void ColorButtonPicker::update_gui()
{
	color_button->lock_window("ColorButtonPicker::update_gui");
	color_button->update_gui(color_button->color);
	color_button->unlock_window();
}

ColorButtonThread::ColorButtonThread(ColorButton *color_button)
 : Thread(1, 0, 0)
{
	this->color_button = color_button;
	this->update_lock = new Condition(0,"ColorButtonThread::update_lock");
	done = 1;
}

ColorButtonThread::~ColorButtonThread()
{
	stop();
	delete update_lock;
}

void ColorButtonThread::start()
{
	if( done ) {
		done = 0;
		Thread::start();
	}
}

void ColorButtonThread::stop()
{
	if( !done ) {
		done = 1;
		update_lock->unlock();
		join();
	}
}

void ColorButtonThread::run()
{
	ColorButtonPicker *color_picker = color_button->color_picker;
	color_picker->update_gui();
	while( !done ) {
		update_lock->lock("ColorButtonThread::run");
		if( done ) break;
		color_picker->update_gui();
	}
}


ColorBoxButton::ColorBoxButton(const char *title,
		int x, int y, int w, int h,
		int color, int alpha, int ok_cancel)
 : ColorButton(title, x, y, w, h, color, alpha, ok_cancel)
{
}
ColorBoxButton::~ColorBoxButton()
{
}

int ColorBoxButton::handle_new_color(int color, int alpha)
{
	return ColorButton::handle_new_color(color, alpha);
}
void ColorBoxButton::handle_done_event(int result)
{
	ColorButton::handle_done_event(result);
}
void ColorBoxButton::create_objects()
{
	update_gui(color);
}

void ColorBoxButton::set_color(int color)
{
	this->color = (color & 0xffffff);
	this->alpha = (~color>>24) & 0xff;
	int r = (color>>16) & 0xff;
	int g = (color>> 8) & 0xff;
	int b = (color>> 0) & 0xff;
	int color_model = vframes[0]->get_color_model();
	int bpp = BC_CModels::calculate_pixelsize(color_model);
	for( int i=0; i<3; ++i ) {
		VFrame *vframe = vframes[i];
		int ww = vframe->get_w(), hh = vframe->get_h();
		uint8_t **rows = vframe->get_rows();
		int rr = r, gg = g, bb = b;
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
		for( int y=0; y<hh; ++y ) {
			uint8_t *rp = rows[y];
			for( int x=0; x<ww; ++x ) {
				rp[0] = rr;  rp[1] = gg;  rp[2] = bb;
				if( bpp > 3 ) rp[3] = 0xff;
				rp += bpp;
			}
		}
	}
	set_images(vframes);
}

ColorCircleButton::ColorCircleButton(const char *title,
		int x, int y, int w, int h,
		int color, int alpha, int ok_cancel)
 : ColorButton(title, x, y, w, h, color, alpha, ok_cancel)
{
}
ColorCircleButton::~ColorCircleButton()
{
}
int ColorCircleButton::handle_new_color(int color, int alpha)
{
	return ColorButton::handle_new_color(color, alpha);
}
void ColorCircleButton::handle_done_event(int result)
{
	ColorButton::handle_done_event(result);
}
void ColorCircleButton::create_objects()
{
	update_gui(color);
}

void ColorCircleButton::set_color(int color)
{
	this->color = (color & 0xffffff);
	this->alpha = (~color>>24) & 0xff;
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

