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
#include "edge.h"
#include "edgewindow.h"
#include "theme.h"

EdgeWindow::EdgeWindow(Edge *plugin)
 : PluginClientWindow(plugin,
 	320,
	120,
	320,
	120,
	0)
{
	this->plugin = plugin;
}

EdgeWindow::~EdgeWindow()
{
}

void EdgeWindow::create_objects()
{
	int x = 10, y = 10;
	int margin = plugin->get_theme()->widget_border;
	BC_Title *title;



	add_subwindow(title = new BC_Title(x,
		y,
		_("Amount:")));
	y += title->get_h() + margin;
	add_subwindow(amount = new EdgeAmount(this,
		x,
		y,
		get_w() - x * 2));
	y += amount->get_h() + margin;

	show_window(1);
}







EdgeAmount::EdgeAmount(EdgeWindow *gui,
	int x,
	int y,
	int w)
 : BC_ISlider(x,
 	y,
	0,
	w,
	w,
	0,
	10,
	gui->plugin->config.amount,
	0)
{
	this->gui = gui;
}

int EdgeAmount::handle_event()
{
	gui->plugin->config.amount = get_value();
	gui->plugin->send_configure_change();
	return 1;
}






