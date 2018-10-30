
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

#ifndef BCBUTTON_H
#define BCBUTTON_H

#include "bcbitmap.inc"
#include "bcsubwindow.h"
#include "vframe.inc"

#include <stdint.h>

#define BUTTON_UP 0
#define BUTTON_UPHI 1
#define BUTTON_DOWNHI 2

class BC_Button : public BC_SubWindow
{
public:
	BC_Button(int x, int y, VFrame **data);
	BC_Button(int x, int y, int w, VFrame **data);
	virtual ~BC_Button();

	friend class BC_GenericButton;

	virtual int handle_event() { return 0; };
	int repeat_event(int64_t repeat_id);
	virtual int draw_face(int flash = 1);
	void disable();
	void enable();

	int initialize();
	virtual int set_images(VFrame **data);
	int cursor_enter_event();
	int cursor_leave_event();
	int button_press_event();
	virtual int button_release_event();
	virtual int cursor_motion_event();
	int update_bitmaps(VFrame **data);
	int reposition_window(int x, int y);
	void set_underline(int number);
	int get_status();


private:

	BC_Pixmap *images[3];
	VFrame **data;
	int status;
	int w_argument;
	int underline_number;
	int enabled;
};




class BC_GenericButton : public BC_Button
{
public:
	BC_GenericButton(int x, int y,
		const char *text, VFrame **data = 0, int color=-1);
	BC_GenericButton(int x, int y, int w,
		const char *text, VFrame **data = 0, int color=-1);
	int set_images(VFrame **data);
	void text_color(int color);
	int draw_face(int flash = 1);
	static int calculate_w(BC_WindowBase *gui, const char *text);
	static int calculate_h();
	const char *get_text () { return text; }
private:
	int color;
	char text[BCTEXTLEN];
};

class BC_OKTextButton : public BC_GenericButton
{
public:
	BC_OKTextButton(BC_WindowBase *parent_window);
	virtual int resize_event(int w, int h);
	virtual int handle_event();
	virtual int keypress_event();
	BC_WindowBase *parent_window;
};

class BC_CancelTextButton : public BC_GenericButton
{
public:
	BC_CancelTextButton(BC_WindowBase *parent_window);
	virtual int resize_event(int w, int h);
	virtual int handle_event();
	virtual int keypress_event();
	BC_WindowBase *parent_window;
};

class BC_OKButton : public BC_Button
{
public:
	BC_OKButton(int x, int y);
	BC_OKButton(BC_WindowBase *parent_window);
	BC_OKButton(BC_WindowBase *parent_window, VFrame **images);
	static int calculate_h();
	static int calculate_w();
	virtual int resize_event(int w, int h);
	virtual int handle_event();
	virtual int keypress_event();
};

class BC_CancelButton : public BC_Button
{
public:
	BC_CancelButton(int x, int y);
	BC_CancelButton(BC_WindowBase *parent_window);
	BC_CancelButton(BC_WindowBase *parent_window, VFrame **images);
	static int calculate_h();
	static int calculate_w();
	virtual int resize_event(int w, int h);
	virtual int handle_event();
	virtual int keypress_event();
};

#endif
