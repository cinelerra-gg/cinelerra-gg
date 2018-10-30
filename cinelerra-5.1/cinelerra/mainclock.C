
/*
 * CINELERRA
 * Copyright (C) 1997-2017 Adam Williams <broadcast at earthling dot net>
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

#include "edl.h"
#include "edlsession.h"
#include "fonts.h"
#include "mainclock.h"
#include "mwindow.h"
#include "theme.h"

MainClock::MainClock(MWindow *mwindow, int x, int y, int w)
 : BC_Title(x, y, "", CLOCKFONT, //MEDIUM_7SEGMENT,
	mwindow->theme->clock_fg_color, 0, w)
{
	this->mwindow = mwindow;
	position_offset = 0;
	set_bg_color(mwindow->theme->clock_bg_color);
}

MainClock::~MainClock()
{
}

void MainClock::set_position_offset(double value)
{
	position_offset = value;
}

void MainClock::update(double position)
{
	lock_window("MainClock::update");
	char string[BCTEXTLEN];
	position += position_offset;
	Units::totext(string, position,
		mwindow->edl->session->time_format,
		mwindow->edl->session->sample_rate,
		mwindow->edl->session->frame_rate,
		mwindow->edl->session->frames_per_foot);
	BC_Title::update(string);
	unlock_window();
}

void MainClock::clear()
{
	lock_window("MainClock::clear");
	BC_Title::update("");
	unlock_window();
}

