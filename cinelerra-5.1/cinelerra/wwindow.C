
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

#include "bcdisplayinfo.h"
#include "bcbutton.h"
#include "language.h"
#include "mwindow.h"
#include "preferences.h"
#include "wwindow.h"


WWindow::WWindow(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->result = 0;
}

WWindow::~WWindow()
{
	close_window();
}

void WWindow::show_warning(int *do_warning, const char *warn_text)
{
	if( running() ) return;
	this->do_warning = do_warning;
	this->warn_text = warn_text;
	this->result = 0;
	start();
}

void WWindow::handle_close_event(int result)
{
	this->result = result;
	gui = 0;
}

BC_Window* WWindow::new_gui()
{
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x();
	int y = display_info.get_abs_cursor_y();
	WWindowGUI *gui = new WWindowGUI(this, x, y);
	gui->create_objects();
	return gui;
}

int WWindow::wait_result()
{
	if( !running() ) return -1;
	join();
	return result;
}

WWindowGUI::WWindowGUI(WWindow *thread, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Warning"), x, y, 640, 100, 640, 100, 0, 0, 1)
{
	this->thread = thread;
}

void WWindowGUI::create_objects()
{
	lock_window("WWindowGUI::create_objects");
	int x = 10, y = 10;
	add_subwindow(new BC_TextBox(x, y, get_w()-50, 3, thread->warn_text));
	y = get_h() - 30;
	add_subwindow(new WDisable(this, x, y));
	y = get_h() - BC_CancelButton::calculate_h() - 10;
	x = get_w() - BC_CancelButton::calculate_w() - 10;
	add_subwindow(new BC_CancelButton(x, y));
	show_window();
	unlock_window();
}

WDisable::WDisable(WWindowGUI *gui, int x, int y)
 : BC_CheckBox(x, y, !*gui->thread->do_warning, _("Don't show this warning again."))
{
	this->gui = gui;
}

int WDisable::handle_event()
{
	*gui->thread->do_warning = !get_value();
	return 1;
}


