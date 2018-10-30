
/*
 * CINELERRA
 * Copyright (C) 2004 Andraz Tori
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

#include "bcdisplayinfo.h"
#include "editpanel.h"
#include "language.h"
#include "manualgoto.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "keys.h"

ManualGoto::ManualGoto(MWindow *mwindow, EditPanel *panel)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->panel = panel;
	gotowindow = 0;
}

ManualGoto::~ManualGoto()
{
	close_window();
}

BC_Window *ManualGoto::new_gui()
{
	BC_DisplayInfo dpy_info;
 	int x = dpy_info.get_abs_cursor_x() - 250 / 2;
	int y = dpy_info.get_abs_cursor_y() - 80 / 2;
	gotowindow = new ManualGotoWindow(this, x, y);
	gotowindow->create_objects();
	double position = panel->get_position();
	gotowindow->reset_data(position);
	gotowindow->show_window();
	return gotowindow;
}

void ManualGoto::handle_done_event(int result)
{
	if( result ) return;
	double current_position = panel->get_position();
	double new_position = gotowindow->get_new_position();
	char modifier = gotowindow->signtitle->get_text()[0];
	switch( modifier ) {
	case '+':  new_position = current_position + new_position;  break;
	case '-':  new_position = current_position - new_position;  break;
	default: break;
	}
	panel->set_position(new_position);
}


ManualGotoWindow::ManualGotoWindow(ManualGoto *mango, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Goto position"), x, y,
	250, 80, 250, 80, 0, 0, 1)
{
	this->mango = mango;
}

ManualGotoWindow::~ManualGotoWindow()
{
}

void ManualGotoWindow::reset_data(double position)
{
	lock_window();
	update_position(position);
	signtitle->update("=");
	unlock_window();
}

double ManualGotoWindow::get_new_position()
{
	int64_t hh = atoi(hours->get_text());
	int mm = atoi(minutes->get_text());
	int ss = atoi(seconds->get_text());
	int ms = atoi(msecs->get_text());

	double seconds = ((((hh * 60) + mm) * 60) + ss) + (1.0 * ms) / 1000;
	return seconds;
}

void ManualGotoWindow::update_position(double position)
{
	if( position < 0 ) position = 0;
	int64_t pos = position;
	int64_t hour = (pos / 3600);
	int minute = (pos / 60 - hour * 60);
	int second = pos - hour * 3600 - minute * 60;
	int thousandths = ((int64_t)(position * 1000)) % 1000;

	hours->update((int)hour);
	minutes->update(minute);
	seconds->update(second);
	msecs->update(thousandths);
}

void ManualGotoWindow::create_objects()
{
	lock_window("ManualGotoWindow::create_objects");
	int x = 76, y = 5;

	BC_Title *title = new BC_Title(x - 2, y, _("hour  min     sec     msec"), SMALLFONT);
	add_subwindow(title);  y += title->get_h() + 3;

	signtitle = new BC_Title(x - 17, y, "=", LARGEFONT);
	add_subwindow(signtitle);
	hours = new ManualGotoNumber(this, x, y, 16, 9, "%i");
	add_subwindow(hours);    x += hours->get_w() + 4;
	minutes = new ManualGotoNumber(this, x, y, 26, 59, "%02i");
	add_subwindow(minutes);  x += minutes->get_w() + 4;
	seconds = new ManualGotoNumber(this, x, y, 26, 59, "%02i");
	add_subwindow(seconds);  x += seconds->get_w() + 4;
	msecs = new ManualGotoNumber(this, x, y, 34, 999, "%03i");
	add_subwindow(msecs);
	y += hours->get_h() + 10;

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	unlock_window();
}


ManualGotoNumber::ManualGotoNumber(ManualGotoWindow *window, int x, int y, int w,
	int max, const char *format)
 : BC_TextBox(x, y, w, 1, "")
{
	this->window = window;
	this->max = max;
	this->format = format;
}

int ManualGotoNumber::handle_event()
{
	return 1;
}

void ManualGotoNumber::update(int64_t v)
{
	char text[BCTEXTLEN];
	if( v < 0 ) v = atoll(get_text());
	if( v > max ) v = max;
	sprintf(text, format, v);
	BC_TextBox::update(text);
}

int ManualGotoNumber::keypress_event()
{
	int key = get_keypress();
	if( key == '+' || key == '-' || key == '=' ) {
		window->signtitle->update(key);
		return 1;
	}
	if( key == '.' || key == TAB ) {
		window->cycle_textboxes(1);
		return 1;
	}

	int chars = 1, m = max;
	while( (m /= 10) > 0 ) ++chars;
	int in_textlen = strlen(get_text());
	if( in_textlen >= chars )
		BC_TextBox::update("");
	int result = BC_TextBox::keypress_event();
	int out_textlen = strlen(get_text());
	if( out_textlen >= chars && get_ibeam_letter() >= chars )
		window->cycle_textboxes(1);
	return result;
}


