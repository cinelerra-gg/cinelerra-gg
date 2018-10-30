/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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
 * *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */



#include "interpolatevideo.h"
#include "interpolatewindow.h"
#include "language.h"
#include "theme.h"








InterpolateVideoWindow::InterpolateVideoWindow(InterpolateVideo *plugin)
 : PluginClientWindow(plugin,
	250,
	250,
	250,
	250,
	0)
{
	this->plugin = plugin;
}

InterpolateVideoWindow::~InterpolateVideoWindow()
{
}

void InterpolateVideoWindow::create_objects()
{
	int x = 10, y = 10;

	BC_Title *title;


	add_subwindow(title = new BC_Title(x, y, _("Input frames per second:")));
	y += title->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(rate = new InterpolateVideoRate(plugin,
		this,
		x,
		y));
	add_subwindow(rate_menu = new InterpolateVideoRateMenu(plugin,
		this,
		x + rate->get_w() + 5,
		y));
	y += rate->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(keyframes = new InterpolateVideoKeyframes(plugin,
		this,
		x,
		y));
	y += keyframes->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(flow = new InterpolateVideoFlow(plugin,
		this,
		x,
		y));

	y += flow->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(vectors = new InterpolateVideoVectors(plugin,
		this,
		x,
		y));

	y += vectors->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(radius_title = new BC_Title(x, y, _("Search radius:")));
	add_subwindow(radius = new InterpolateVideoRadius(plugin,
		this,
		x + radius_title->get_w() + plugin->get_theme()->widget_border,
		y));

	y += radius->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(size_title = new BC_Title(x, y, _("Macroblock size:")));
	add_subwindow(size = new InterpolateVideoSize(plugin,
		this,
		x + size_title->get_w() + plugin->get_theme()->widget_border,
		y));


	update_enabled();
	show_window();
}

void InterpolateVideoWindow::update_enabled()
{
	if(plugin->config.use_keyframes)
	{
		rate->disable();
	}
	else
	{
		rate->enable();
	}

	if(plugin->config.optic_flow)
	{
		vectors->enable();
		radius->enable();
		size->enable();
		radius_title->set_color(get_resources()->default_text_color);
		size_title->set_color(get_resources()->default_text_color);
	}
	else
	{
		vectors->disable();
		radius->disable();
		size->disable();
		radius_title->set_color(get_resources()->disabled_text_color);
		size_title->set_color(get_resources()->disabled_text_color);
	}
}













InterpolateVideoRate::InterpolateVideoRate(InterpolateVideo *plugin,
	InterpolateVideoWindow *gui,
	int x,
	int y)
 : BC_TextBox(x,
	y,
	90,
	1,
	(float)plugin->config.input_rate)
{
	this->plugin = plugin;
	this->gui = gui;
}

int InterpolateVideoRate::handle_event()
{
	plugin->config.input_rate = Units::atoframerate(get_text());
	plugin->send_configure_change();
	return 1;
}




InterpolateVideoRateMenu::InterpolateVideoRateMenu(InterpolateVideo *plugin,
	InterpolateVideoWindow *gui,
	int x,
	int y)
 : BC_ListBox(x,
 	y,
	100,
	200,
	LISTBOX_TEXT,
	&plugin->get_theme()->frame_rates,
	0,
	0,
	1,
	0,
	1)
{
	this->plugin = plugin;
	this->gui = gui;
}

int InterpolateVideoRateMenu::handle_event()
{
	char *text = get_selection(0, 0)->get_text();
	plugin->config.input_rate = atof(text);
	gui->rate->update(text);
	plugin->send_configure_change();
	return 1;
}




InterpolateVideoKeyframes::InterpolateVideoKeyframes(InterpolateVideo *plugin,
	InterpolateVideoWindow *gui,
	int x,
	int y)
 : BC_CheckBox(x,
 	y,
	plugin->config.use_keyframes,
	_("Use keyframes as input"))
{
	this->plugin = plugin;
	this->gui = gui;
}
int InterpolateVideoKeyframes::handle_event()
{
	plugin->config.use_keyframes = get_value();
	gui->update_enabled();
	plugin->send_configure_change();
	return 1;
}




InterpolateVideoFlow::InterpolateVideoFlow(InterpolateVideo *plugin,
	InterpolateVideoWindow *gui,
	int x,
	int y)
 : BC_CheckBox(x,
 	y,
	plugin->config.optic_flow,
	_("Use optic flow"))
{
	this->plugin = plugin;
	this->gui = gui;
}
int InterpolateVideoFlow::handle_event()
{
	plugin->config.optic_flow = get_value();
	gui->update_enabled();
	plugin->send_configure_change();
	return 1;
}



InterpolateVideoVectors::InterpolateVideoVectors(InterpolateVideo *plugin,
	InterpolateVideoWindow *gui,
	int x,
	int y)
 : BC_CheckBox(x,
 	y,
	plugin->config.draw_vectors,
	_("Draw motion vectors"))
{
	this->plugin = plugin;
	this->gui = gui;
}
int InterpolateVideoVectors::handle_event()
{
	plugin->config.draw_vectors = get_value();
	plugin->send_configure_change();
	return 1;
}






InterpolateVideoRadius::InterpolateVideoRadius(InterpolateVideo *plugin,
	InterpolateVideoWindow *gui,
	int x,
	int y)
 : BC_IPot(x,
 	y,
	plugin->config.search_radius,
	MIN_SEARCH_RADIUS,
	MAX_SEARCH_RADIUS)
{
	this->plugin = plugin;
	this->gui = gui;
}
int InterpolateVideoRadius::handle_event()
{
	plugin->config.search_radius = get_value();
	plugin->send_configure_change();
	return 1;
}









InterpolateVideoSize::InterpolateVideoSize(InterpolateVideo *plugin,
	InterpolateVideoWindow *gui,
	int x,
	int y)
 : BC_IPot(x,
 	y,
	plugin->config.macroblock_size,
	MIN_MACROBLOCK_SIZE,
	MAX_MACROBLOCK_SIZE)
{
	this->plugin = plugin;
	this->gui = gui;
}
int InterpolateVideoSize::handle_event()
{
	plugin->config.macroblock_size = get_value();
	plugin->send_configure_change();
	return 1;
}








