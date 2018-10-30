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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "language.h"
#include "puzzleobj.h"
#include "puzzleobjwindow.h"
#include "theme.h"

PuzzleObjWindow::PuzzleObjWindow(PuzzleObj *plugin)
 : PluginClientWindow(plugin, 320, 100, 320, 100, 0)
{
	this->plugin = plugin; 
	pixels = 0;
	iterations = 0;
	pixels_title = 0;
	iterations_title = 0;
}

PuzzleObjWindow::~PuzzleObjWindow()
{
}

void PuzzleObjWindow::create_objects()
{
	int x = 10, y = 10;
	BC_Title *title = new BC_Title(x, y, _("PuzzleObj"));
	add_subwindow(title);
	y += title->get_h() + 10;
	int x1 = x + 80;
	add_subwindow(pixels_title = new BC_Title(x,y,_("Pixels:")));
	add_subwindow(pixels = new PuzzleObjISlider(this,
			x1,y,180, 1,1000, &plugin->config.pixels));
	y += pixels->get_h() + 10;
	add_subwindow(iterations_title = new BC_Title(x,y,_("Iterations:")));
	add_subwindow(iterations = new PuzzleObjISlider(this,
			x1,y,180, 0,50, &plugin->config.iterations));
	show_window(1);
}

PuzzleObjISlider::PuzzleObjISlider(PuzzleObjWindow *win,
		int x, int y, int w, int min, int max, int *output)
 : BC_ISlider(x, y, 0,w,w, min,max, *output)
{
        this->win = win;
        this->output = output;
}
PuzzleObjISlider::~PuzzleObjISlider()
{
}

int PuzzleObjISlider::handle_event()
{
	*output = get_value();
	win->plugin->send_configure_change();
        return 1;
}

