
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


#include "bcsignals.h"
#include "language.h"
#include "mainprogress.h"
#include "mwindow.h"
#include "statusbar.h"
#include "theme.h"
#include "vframe.h"



StatusBar::StatusBar(MWindow *mwindow, MWindowGUI *gui)
 : BC_SubWindow(mwindow->theme->mstatus_x,
 	mwindow->theme->mstatus_y,
	mwindow->theme->mstatus_w,
	mwindow->theme->mstatus_h)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

StatusBar::~StatusBar()
{
}



void StatusBar::create_objects()
{
//printf("StatusBar::create_objects 1\n");
	int x = 10; //int y = 5;
//printf("StatusBar::create_objects 1\n");
	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	add_subwindow(status_text = new BC_Title(mwindow->theme->mstatus_message_x,
		mwindow->theme->mstatus_message_y,
		"",
		MEDIUMFONT,
		mwindow->theme->message_normal));
	x = get_w() - 290;
// printf("StatusBar::create_objects %d: 0x%08x\n",
// __LINE__, mwindow->theme->message_normal);
	add_subwindow(main_progress =
		new BC_ProgressBar(mwindow->theme->mstatus_progress_x,
			mwindow->theme->mstatus_progress_y,
			mwindow->theme->mstatus_progress_w,
			mwindow->theme->mstatus_progress_w));
	x += main_progress->get_w() + 5;
//printf("StatusBar::create_objects 1\n");
	add_subwindow(main_progress_cancel =
		new StatusBarCancel(mwindow,
			mwindow->theme->mstatus_cancel_x,
			mwindow->theme->mstatus_cancel_y));
//printf("StatusBar::create_objects 1\n");
	reset_default_message();
	flash();
}

void StatusBar::resize_event()
{
	int x = 10; //int y = 1;


	reposition_window(mwindow->theme->mstatus_x,
		mwindow->theme->mstatus_y,
		mwindow->theme->mstatus_w,
		mwindow->theme->mstatus_h);

	draw_top_background(get_parent(), 0, 0, get_w(), get_h());


	status_text->reposition_window(mwindow->theme->mstatus_message_x,
		mwindow->theme->mstatus_message_y);

	x = get_w() - 290;
	main_progress->reposition_window(mwindow->theme->mstatus_progress_x,
		mwindow->theme->mstatus_progress_y);

	x += main_progress->get_w() + 5;
	main_progress_cancel->reposition_window(mwindow->theme->mstatus_cancel_x,
		mwindow->theme->mstatus_cancel_y);

	flash(0);
}

void StatusBar::show_message(const char *text, int color)
{
	int mx = mwindow->theme->mstatus_message_x;
	int my = mwindow->theme->mstatus_message_y;
	int tx = status_text->get_x(), th = status_text->get_h();
	if( color >= 0 ) {
		set_color(color);
		int bb = th/4, bh = th - bb*2;
		draw_box(mx+bb,my+bb, bh,bh);
		flash(mx,my, th,th);  mx += 5;
		if( (mx+=th) != tx )
			status_text->reposition_window(mx, my);
	}
	else if( tx != mx ) {
		draw_top_background(get_parent(), mx,my, th,th);
		flash(mx,my, th,th);
		status_text->reposition_window(mx, my);
	}
	color = mwindow->theme->message_normal;
	status_text->set_color(color);
	status_text->update(text);
}
void StatusBar::reset_default_message()
{
	status_color = -1;
	strcpy(default_msg, _("Welcome to Cinelerra."));
}
void StatusBar::update_default_message()
{
	status_color = status_text->get_color();
	strcpy(default_msg, status_text->get_text());
}
void StatusBar::default_message()
{
	show_message(default_msg, status_color);
}

StatusBarCancel::StatusBarCancel(MWindow *mwindow, int x, int y)
 : BC_Button(x, y, mwindow->theme->statusbar_cancel_data)
{
	this->mwindow = mwindow;
	set_tooltip(_("Cancel operation"));
}
int StatusBarCancel::handle_event()
{
	mwindow->mainprogress->cancelled = 1;
	return 1;
}
