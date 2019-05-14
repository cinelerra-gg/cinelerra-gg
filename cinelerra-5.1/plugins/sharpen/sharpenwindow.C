
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
#include "sharpenwindow.h"










SharpenWindow::SharpenWindow(SharpenMain *client)
 : PluginClientWindow(client, 280, 190, 280, 190, 0) //195 was 150
{
	this->client = client;
}

SharpenWindow::~SharpenWindow()
{
}

void SharpenWindow::create_objects()
{
	int x = 10, y = 10;
	int x1 = 0; int clrBtn_w = 50;
	int defaultBtn_w = 100;

	add_tool(new BC_Title(x, y, _("Sharpness")));
	y += 20;
	add_tool(sharpen_slider = new SharpenSlider(client, &(client->config.sharpness), x, y));
	x1 = x + sharpen_slider->get_w() + 10;
	add_subwindow(sharpen_sliderClr = new SharpenSliderClr(client, this, x1, y, clrBtn_w));

	y += 30;
	add_tool(sharpen_interlace = new SharpenInterlace(client, x, y));
	y += 30;
	add_tool(sharpen_horizontal = new SharpenHorizontal(client, x, y));
	y += 30;
	add_tool(sharpen_luminance = new SharpenLuminance(client, x, y));
	y += 40;
	add_tool(reset = new SharpenReset(client, this, x, y));
	add_subwindow(default_settings = new SharpenDefaultSettings(client, this,
		(280 - 10 - defaultBtn_w), y, defaultBtn_w));

	show_window();
	flush();
}

void SharpenWindow::update_gui(int clear)
{
	switch(clear) {
		case RESET_SHARPEN_SLIDER :
			sharpen_slider->update(client->config.sharpness);
			break;
		case RESET_ALL :
		case RESET_DEFAULT_SETTINGS :
		default:
			sharpen_slider->update(client->config.sharpness);
			sharpen_interlace->update(client->config.interlace);
			sharpen_horizontal->update(client->config.horizontal);
			sharpen_luminance->update(client->config.luminance);
			break;
	}
}

SharpenSlider::SharpenSlider(SharpenMain *client, float *output, int x, int y)
 : BC_ISlider(x,
 	y,
	0,
	200,
	200,
	0,
	MAXSHARPNESS,
	(int)*output,
	0,
	0,
	0)
{
	this->client = client;
	this->output = output;
}
SharpenSlider::~SharpenSlider()
{
}
int SharpenSlider::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}


SharpenInterlace::SharpenInterlace(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.interlace, _("Interlace"))
{
	this->client = client;
}
SharpenInterlace::~SharpenInterlace()
{
}
int SharpenInterlace::handle_event()
{
	client->config.interlace = get_value();
	client->send_configure_change();
	return 1;
}


SharpenHorizontal::SharpenHorizontal(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.horizontal, _("Horizontal only"))
{
	this->client = client;
}
SharpenHorizontal::~SharpenHorizontal()
{
}
int SharpenHorizontal::handle_event()
{
	client->config.horizontal = get_value();
	client->send_configure_change();
	return 1;
}


SharpenLuminance::SharpenLuminance(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.luminance, _("Luminance only"))
{
	this->client = client;
}
SharpenLuminance::~SharpenLuminance()
{
}
int SharpenLuminance::handle_event()
{
	client->config.luminance = get_value();
	client->send_configure_change();
	return 1;
}


SharpenReset::SharpenReset(SharpenMain *client, SharpenWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->client = client;
	this->gui = gui;
}
SharpenReset::~SharpenReset()
{
}
int SharpenReset::handle_event()
{
	client->config.reset(RESET_ALL);
	gui->update_gui(RESET_ALL);
	client->send_configure_change();
	return 1;
}


SharpenDefaultSettings::SharpenDefaultSettings(SharpenMain *client, SharpenWindow *gui, int x, int y, int w)
 : BC_GenericButton(x, y, w, _("Default"))
{
	this->client = client;
	this->gui = gui;
}
SharpenDefaultSettings::~SharpenDefaultSettings()
{
}
int SharpenDefaultSettings::handle_event()
{
	client->config.reset(RESET_DEFAULT_SETTINGS);
	gui->update_gui(RESET_DEFAULT_SETTINGS);
	client->send_configure_change();
	return 1;
}


SharpenSliderClr::SharpenSliderClr(SharpenMain *client, SharpenWindow *gui, int x, int y, int w)
 : BC_Button(x, y, w, client->get_theme()->get_image_set("reset_button"))
{
	this->client = client;
	this->gui = gui;
}
SharpenSliderClr::~SharpenSliderClr()
{
}
int SharpenSliderClr::handle_event()
{
	client->config.reset(RESET_SHARPEN_SLIDER);
	gui->update_gui(RESET_SHARPEN_SLIDER);
	client->send_configure_change();
	return 1;
}

