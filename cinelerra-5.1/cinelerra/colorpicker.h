
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

#ifndef COLORPICKER_H
#define COLORPICKER_H

#include "bcbutton.h"
#include "bcdialog.h"
#include "bctextbox.h"
#include "bcsubwindow.h"
#include "clip.h"
#include "condition.inc"
#include "guicast.h"
#include "mutex.inc"
#include "thread.h"
#include "vframe.inc"

#define PALLETTE_HISTORY_SIZE 16

class ColorWindow;
class PaletteWheel;
class PaletteWheelValue;
class PaletteOutput;
class PaletteHue;
class PaletteSat;
class PaletteVal;
class PaletteRed;
class PaletteGrn;
class PaletteBlu;
class PaletteLum;
class PaletteCr;
class PaletteCb;
class PaletteAlpha;
class PaletteHSV;
class PaletteRGB;
class PaletteYUV;
class PaletteAPH;
class PaletteHexButton;
class PaletteHex;
class PaletteGrabButton;
class PaletteHistory;

class ColorPicker : public BC_DialogThread
{
public:
	ColorPicker(int do_alpha = 0, const char *title = 0);
	~ColorPicker();

	void start_window(int output, int alpha, int do_okcancel=0);
	virtual int handle_new_color(int output, int alpha);
	void update_gui(int output, int alpha);
	BC_Window* new_gui();

	int orig_color, orig_alpha;
	int output, alpha;
	int do_alpha, do_okcancel;
	const char *title;
};

class ColorWindow : public BC_Window
{
public:
	ColorWindow(ColorPicker *thread, int x, int y, int w, int h, const char *title);
	~ColorWindow();

	void create_objects();
	void change_values();
	int close_event();
	void update_display();
	void update_rgb();
	void update_hsv();
	void update_yuv();
	int handle_event();
	void get_screen_sample();
	int cursor_motion_event();
	int button_press_event();
	int button_release_event();

	struct { float r, g, b; } rgb;
	struct { float y, u, v; } yuv;
	struct { float h, s, v; } hsv;
	float aph;
	void update_rgb(float r, float g, float b);
	void update_hsv(float h, float s, float v);
	void update_yuv(float y, float u, float v);
	void update_rgb_hex(const char *hex);
	int rgb888();

	ColorPicker *thread;
	PaletteWheel *wheel;
	PaletteWheelValue *wheel_value;
	PaletteOutput *output;
	PaletteHue *hue;
	PaletteSat *sat;
	PaletteVal *val;
	PaletteRed *red;
	PaletteGrn *grn;
	PaletteBlu *blu;
	PaletteLum *lum;
	PaletteCr  *c_r;
	PaletteCb  *c_b;
	PaletteAlpha *alpha;

	PaletteHSV *hsv_h, *hsv_s, *hsv_v;
	PaletteRGB *rgb_r, *rgb_g, *rgb_b;
	PaletteYUV *yuv_y, *yuv_u, *yuv_v;
	PaletteAPH *aph_a;

	PaletteHexButton *hex_btn;
	PaletteHex *hex_box;
	PaletteGrabButton *grab_btn;
	PaletteHistory *history;

	VFrame *value_bitmap;
	int button_grabbed;

	int palette_history[PALLETTE_HISTORY_SIZE];
	void load_history();
	void save_history();
	void update_history(int color);
	void update_history();
};


class PaletteWheel : public BC_SubWindow
{
public:
	PaletteWheel(ColorWindow *window, int x, int y);
	~PaletteWheel();
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();

	void create_objects();
	int draw(float hue, float saturation);
	int get_angle(float x1, float y1, float x2, float y2);
	float torads(float angle);
	ColorWindow *window;
	float oldhue;
	float oldsaturation;
	int button_down;
};

class PaletteWheelValue : public BC_SubWindow
{
public:
	PaletteWheelValue(ColorWindow *window, int x, int y);
	~PaletteWheelValue();
	void create_objects();
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();
	int draw(float hue, float saturation, float value);
	ColorWindow *window;
	int button_down;
// Gradient
	VFrame *frame;
};

class PaletteOutput : public BC_SubWindow
{
public:
	PaletteOutput(ColorWindow *window, int x, int y);
	~PaletteOutput();
	void create_objects();
	int handle_event();
	int draw();
	ColorWindow *window;
};

class PaletteHue : public BC_ISlider
{
public:
	PaletteHue(ColorWindow *window, int x, int y);
	~PaletteHue();
	int handle_event();
	ColorWindow *window;
};

class PaletteSat : public BC_FSlider
{
public:
	PaletteSat(ColorWindow *window, int x, int y);
	~PaletteSat();
	int handle_event();
	ColorWindow *window;
};

class PaletteVal : public BC_FSlider
{
public:
	PaletteVal(ColorWindow *window, int x, int y);
	~PaletteVal();
	int handle_event();
	ColorWindow *window;
};

class PaletteRed : public BC_FSlider
{
public:
	PaletteRed(ColorWindow *window, int x, int y);
	~PaletteRed();
	int handle_event();
	ColorWindow *window;
};

class PaletteGrn : public BC_FSlider
{
public:
	PaletteGrn(ColorWindow *window, int x, int y);
	~PaletteGrn();
	int handle_event();
	ColorWindow *window;
};

class PaletteBlu : public BC_FSlider
{
public:
	PaletteBlu(ColorWindow *window, int x, int y);
	~PaletteBlu();
	int handle_event();
	ColorWindow *window;
};

class PaletteAlpha : public BC_FSlider
{
public:
	PaletteAlpha(ColorWindow *window, int x, int y);
	~PaletteAlpha();
	int handle_event();
	ColorWindow *window;
};

class PaletteLum : public BC_FSlider
{
public:
	PaletteLum(ColorWindow *window, int x, int y);
	~PaletteLum();
	int handle_event();
	ColorWindow *window;
};

class PaletteCr : public BC_FSlider
{
public:
	PaletteCr(ColorWindow *window, int x, int y);
	~PaletteCr();
	int handle_event();
	ColorWindow *window;
};

class PaletteCb : public BC_FSlider
{
public:
	PaletteCb(ColorWindow *window, int x, int y);
	~PaletteCb();
	int handle_event();
	ColorWindow *window;
};

class PaletteNum : public BC_TumbleTextBox
{
public:
	ColorWindow *window;
	float *output;

	PaletteNum(ColorWindow *window, int x, int y,
			float &output, float min, float max);
	~PaletteNum();
	void update_output() { *output = atof(get_text()); }
	static int calculate_h() { return BC_Tumbler::calculate_h(); }
};

class PaletteRGB : public PaletteNum
{
public:
	PaletteRGB(ColorWindow *window, int x, int y,
			float &output, float min, float max)
	 : PaletteNum(window, x, y, output, min, max) {}
	int handle_event();
};

class PaletteYUV : public PaletteNum
{
public:
	PaletteYUV(ColorWindow *window, int x, int y,
			float &output, float min, float max)
	 : PaletteNum(window, x, y, output, min, max) {}
	int handle_event();
};

class PaletteHSV : public PaletteNum
{
public:
	PaletteHSV(ColorWindow *window, int x, int y,
			float &output, float min, float max)
	 : PaletteNum(window, x, y, output, min, max) {}
	int handle_event();
};

class PaletteAPH : public PaletteNum
{
public:
	PaletteAPH(ColorWindow *window, int x, int y,
			float &output, float min, float max)
	 : PaletteNum(window, x, y, output, min, max) {}
	int handle_event();
};

class PaletteHexButton : public BC_GenericButton
{
public:
	PaletteHexButton(ColorWindow *window, int x, int y);
	~PaletteHexButton();
	int handle_event();
	ColorWindow *window;
};

class PaletteHex : public BC_TextBox
{
public:
	PaletteHex(ColorWindow *window, int x, int y, const char *hex);
	~PaletteHex();
	int keypress_event();
	void update();
	ColorWindow *window;
};

class PaletteGrabButton : public BC_Button
{
public:
	PaletteGrabButton(ColorWindow *window, int x, int y);
	~PaletteGrabButton();
	int handle_event();

	ColorWindow *window;
	VFrame *vframes[3];
};

class PaletteHistory : public BC_SubWindow
{
public:
	PaletteHistory(ColorWindow *window, int x, int y);
	~PaletteHistory();
	void update(int flush=1);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int cursor_leave_event();
	int repeat_event(int64_t duration);

	ColorWindow *window;
	int button_down;
};

#endif
