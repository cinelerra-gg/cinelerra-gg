
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
#include "brightnesswindow.h"
#include "language.h"
#include "theme.h"









BrightnessWindow::BrightnessWindow(BrightnessMain *client)
 : PluginClientWindow(client, 370, 155, 370, 155, 0)
{
	this->client = client;
}

BrightnessWindow::~BrightnessWindow()
{
}

void BrightnessWindow::create_objects()
{
	int x = 10, y = 10, x1 = x + 90;
	int x2 = 0; int clrBtn_w = 50;

	add_tool(new BC_Title(x, y, _("Brightness/Contrast")));
	y += 25;
	add_tool(new BC_Title(x, y,_("Brightness:")));
	add_tool(brightness = new BrightnessSlider(client,
		&(client->config.brightness),
		x1,
		y,
		1));
	x2 = x1 + brightness->get_w() + 10;
	add_subwindow(brightnessClr = new BrightnessSliderClr(client, this, x2, y, clrBtn_w, 1));
	y += 25;
	add_tool(new BC_Title(x, y, _("Contrast:")));
	add_tool(contrast = new BrightnessSlider(client,
		&(client->config.contrast),
		x1,
		y,
		0));

	add_subwindow(contrastClr = new BrightnessSliderClr(client, this, x2, y, clrBtn_w, 0));
	y += 30;
	add_tool(luma = new BrightnessLuma(client,
		x,
		y));

	y += 35;
	add_subwindow(reset = new BrightnessReset(client, this, x, y));

	show_window();
	flush();
}

// for Reset button
void BrightnessWindow::update_gui(int clear)
{
	switch(clear) {
		case RESET_CONTRAST : contrast->update(client->config.contrast);
			break;
		case RESET_BRIGHTNESS: brightness->update(client->config.brightness);
			break;
		case RESET_ALL :
		default:
			brightness->update(client->config.brightness);
			contrast->update(client->config.contrast);
			luma->update(client->config.luma);
			break;
	}
}

BrightnessSlider::BrightnessSlider(BrightnessMain *client,
	float *output,
	int x,
	int y,
	int is_brightness)
 : BC_FSlider(x,
 	y,
	0,
	200,
	200,
	-100,
	100,
	(int)*output)
{
	this->client = client;
	this->output = output;
	this->is_brightness = is_brightness;
}
BrightnessSlider::~BrightnessSlider()
{
}
int BrightnessSlider::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}

char* BrightnessSlider::get_caption()
{
	float fraction;
	if(is_brightness)
	{
		fraction = *output / 100;
	}
	else
	{
		fraction = (*output < 0) ?
			(*output + 100) / 100 :
			(*output + 25) / 25;
	}
	sprintf(string, "%0.4f", fraction);
	return string;
}


BrightnessLuma::BrightnessLuma(BrightnessMain *client,
	int x,
	int y)
 : BC_CheckBox(x,
 	y,
	client->config.luma,
	_("Boost luminance only"))
{
	this->client = client;
}
BrightnessLuma::~BrightnessLuma()
{
}
int BrightnessLuma::handle_event()
{
	client->config.luma = get_value();
	client->send_configure_change();
	return 1;
}


BrightnessReset::BrightnessReset(BrightnessMain *client, BrightnessWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->client = client;
	this->window = window;
}
BrightnessReset::~BrightnessReset()
{
}
int BrightnessReset::handle_event()
{
	client->config.reset(RESET_ALL); // clear=0 ==> reset all
	window->update_gui(RESET_ALL);
	client->send_configure_change();
	return 1;
}

BrightnessSliderClr::BrightnessSliderClr(BrightnessMain *client, BrightnessWindow *window, int x, int y, int w, int is_brightness)
 : BC_Button(x, y, w, client->get_theme()->get_image_set("reset_button"))
{
	this->client = client;
	this->window = window;
	this->is_brightness = is_brightness;
}
BrightnessSliderClr::~BrightnessSliderClr()
{
}
int BrightnessSliderClr::handle_event()
{
	// is_brightness==0 means Contrast slider   ==> "clear=1"
	// is_brightness==1 means Brightness slider ==> "clear=2"
	client->config.reset(is_brightness + 1);
	window->update_gui(is_brightness + 1);
	client->send_configure_change();
	return 1;
}

