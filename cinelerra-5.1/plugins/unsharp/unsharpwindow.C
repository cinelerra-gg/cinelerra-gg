
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
#include "theme.h"
#include "unsharp.h"
#include "unsharpwindow.h"


UnsharpWindow::UnsharpWindow(UnsharpMain *plugin)
 : PluginClientWindow(plugin, 285, 170, 285, 170, 0)
{
	this->plugin = plugin;
}

UnsharpWindow::~UnsharpWindow()
{
}

void UnsharpWindow::create_objects()
{
	int x = 10, y = 10, x1 = 90;
	int x2 = 0; int clrBtn_w = 50;
	int defaultBtn_w = 100;
	BC_Title *title;

	add_subwindow(title = new BC_Title(x, y + 10, _("Radius:")));
	add_subwindow(radius = new UnsharpRadius(plugin, x + x1, y));
	x2 = 285 - 10 - clrBtn_w;
	add_subwindow(radiusClr = new UnsharpSliderClr(plugin, this, x2, y + 10, clrBtn_w, RESET_RADIUS));

	y += 40;
	add_subwindow(title = new BC_Title(x, y + 10, _("Amount:")));
	add_subwindow(amount = new UnsharpAmount(plugin, x + x1, y));
	add_subwindow(amountClr = new UnsharpSliderClr(plugin, this, x2, y + 10, clrBtn_w, RESET_AMOUNT));

	y += 40;
	add_subwindow(title = new BC_Title(x, y + 10, _("Threshold:")));
	add_subwindow(threshold = new UnsharpThreshold(plugin, x + x1, y));
	add_subwindow(thresholdClr = new UnsharpSliderClr(plugin, this, x2, y + 10, clrBtn_w, RESET_THRESHOLD));

	y += 50;
	add_subwindow(reset = new UnsharpReset(plugin, this, x, y));
	add_subwindow(default_settings = new UnsharpDefaultSettings(plugin, this,
		(285 - 10 - defaultBtn_w), y, defaultBtn_w));

	show_window();
	flush();
}


void UnsharpWindow::update_gui(int clear)
{
	switch(clear) {
		case RESET_RADIUS : radius->update(plugin->config.radius);
			break;
		case RESET_AMOUNT : amount->update(plugin->config.amount);
			break;
		case RESET_THRESHOLD : threshold->update(plugin->config.threshold);
			break;
		case RESET_ALL :
		case RESET_DEFAULT_SETTINGS :
		default:
			radius->update(plugin->config.radius);
			amount->update(plugin->config.amount);
			threshold->update(plugin->config.threshold);
			break;
	}
}










UnsharpRadius::UnsharpRadius(UnsharpMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.radius, 0.1, 120)
{
	this->plugin = plugin;
}
int UnsharpRadius::handle_event()
{
	plugin->config.radius = get_value();
	plugin->send_configure_change();
	return 1;
}

UnsharpAmount::UnsharpAmount(UnsharpMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.amount, 0, 5)
{
	this->plugin = plugin;
}
int UnsharpAmount::handle_event()
{
	plugin->config.amount = get_value();
	plugin->send_configure_change();
	return 1;
}

UnsharpThreshold::UnsharpThreshold(UnsharpMain *plugin, int x, int y)
 : BC_IPot(x, y, plugin->config.threshold, 0, 255)
{
	this->plugin = plugin;
}
int UnsharpThreshold::handle_event()
{
	plugin->config.threshold = get_value();
	plugin->send_configure_change();
	return 1;
}

UnsharpReset::UnsharpReset(UnsharpMain *plugin, UnsharpWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->window = window;
}
UnsharpReset::~UnsharpReset()
{
}
int UnsharpReset::handle_event()
{
	plugin->config.reset(RESET_ALL);
	window->update_gui(RESET_ALL);
	plugin->send_configure_change();
	return 1;
}

UnsharpDefaultSettings::UnsharpDefaultSettings(UnsharpMain *plugin, UnsharpWindow *window, int x, int y, int w)
 : BC_GenericButton(x, y, w, _("Default"))
{
	this->plugin = plugin;
	this->window = window;
}
UnsharpDefaultSettings::~UnsharpDefaultSettings()
{
}
int UnsharpDefaultSettings::handle_event()
{
	plugin->config.reset(RESET_DEFAULT_SETTINGS);
	window->update_gui(RESET_DEFAULT_SETTINGS);
	plugin->send_configure_change();
	return 1;
}

UnsharpSliderClr::UnsharpSliderClr(UnsharpMain *plugin, UnsharpWindow *window, int x, int y, int w, int clear)
 : BC_Button(x, y, w, plugin->get_theme()->get_image_set("reset_button"))
{
	this->plugin = plugin;
	this->window = window;
	this->clear = clear;
}
UnsharpSliderClr::~UnsharpSliderClr()
{
}
int UnsharpSliderClr::handle_event()
{
	// clear==1 ==> Radius slider
	// clear==2 ==> Amount slider
	// clear==3 ==> Threshold slider
	plugin->config.reset(clear);
	window->update_gui(clear);
	plugin->send_configure_change();
	return 1;
}
