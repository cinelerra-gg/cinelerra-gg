
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
#include "language.h"
#include "mwindow.inc"
#include "splashgui.h"
#include "vframe.h"

#include <unistd.h>



SplashGUI::SplashGUI(VFrame *bg, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Loading"), x, y, bg->get_w(), bg->get_h(),
		-1, -1, 0, 0, 1, -1, "", 0)
{
	this->bg = bg;
}

SplashGUI::~SplashGUI()
{
	delete bg;
}

void SplashGUI::create_objects()
{
	lock_window("SplashGUI::create_objects");
	draw_vframe(bg, 0, 0);
	flash();
	show_window();
	operation = new BC_Title(5,
			get_h() - get_text_height(MEDIUMFONT) - 5,
			_("Loading..."), MEDIUMFONT, GREEN);
	add_subwindow(operation);
	unlock_window();
}

void SplashGUI::update_status(const char *text)
{
	lock_window("SplashGUI::update_status");
	operation->update(text);
	unlock_window();
}

