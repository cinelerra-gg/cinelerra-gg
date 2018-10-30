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
#include "flowobj.h"
#include "flowobjwindow.h"
#include "theme.h"

FlowObjWindow::FlowObjWindow(FlowObj *plugin)
 : PluginClientWindow(plugin, 320, 240, 320, 240, 0)
{
	this->plugin = plugin; 
}

FlowObjWindow::~FlowObjWindow()
{
}

void FlowObjWindow::create_objects()
{
	int x = 10, y = 10;
	int margin = plugin->get_theme()->widget_border;
	BC_Title *title;

	add_subwindow(vectors = new FlowObjVectors(this, x, y));
	y += vectors->get_h() + margin;
	add_subwindow(do_stabilization = new FlowObjDoStabilization(this,
		x, y));
	y += do_stabilization->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Block size:")));
	add_subwindow(block_size = new FlowObjBlockSize(this,
		x + title->get_w() + margin, y));
	y += block_size->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Search radius:")));
	add_subwindow(search_radius = new FlowObjSearchRadius(this,
		x + title->get_w() + margin, y));
	y += search_radius->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Settling speed:")));
	add_subwindow(settling_speed = new FlowObjSettling(this,
		x + title->get_w() + margin, y));

	show_window(1);
}

FlowObjVectors::FlowObjVectors(FlowObjWindow *gui, int x, int y)
 : BC_CheckBox(x, y, gui->plugin->config.draw_vectors, _("Draw vectors"))
{
	this->gui = gui;
}

int FlowObjVectors::handle_event()
{
	gui->plugin->config.draw_vectors = get_value();
	gui->plugin->send_configure_change();
	return 1;
}


FlowObjDoStabilization::FlowObjDoStabilization(FlowObjWindow *gui, int x, int y)
 : BC_CheckBox(x, y, gui->plugin->config.do_stabilization, _("Do stabilization"))
{
	this->gui = gui;
}

int FlowObjDoStabilization::handle_event()
{
	gui->plugin->config.do_stabilization = get_value();
	gui->plugin->send_configure_change();
	return 1;
}


FlowObjBlockSize::FlowObjBlockSize(FlowObjWindow *gui, int x, int y)
 : BC_IPot(x, y, (int64_t)gui->plugin->config.block_size, (int64_t)5, (int64_t)100)
{
	this->gui = gui;
}

int FlowObjBlockSize::handle_event()
{
	gui->plugin->config.block_size = (int)get_value();
	gui->plugin->send_configure_change();
	return 1;
}


FlowObjSearchRadius::FlowObjSearchRadius(FlowObjWindow *gui, int x, int y)
 : BC_IPot(x, y, (int64_t)gui->plugin->config.search_radius, (int64_t)1, (int64_t)100)
{
	this->gui = gui;
}

int FlowObjSearchRadius::handle_event()
{
	gui->plugin->config.search_radius = (int)get_value();
	gui->plugin->send_configure_change();
	return 1;
}


FlowObjSettling::FlowObjSettling(FlowObjWindow *gui, int x, int y)
 : BC_IPot(x, y, (int64_t)gui->plugin->config.settling_speed, (int64_t)0, (int64_t)100)
{
	this->gui = gui;
}

int FlowObjSettling::handle_event()
{
	gui->plugin->config.settling_speed = (int)get_value();
	gui->plugin->send_configure_change();
	return 1;
}

