
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

#include "language.h"
#include "whirlwindow.h"









WhirlWindow::WhirlWindow(WhirlMain *client)
 : PluginClientWindow(client, 210, 170, 200, 170, 0)
{ this->client = client; }

WhirlWindow::~WhirlWindow()
{
	delete angle_slider;
	delete pinch_slider;
	delete radius_slider;
	delete automation[0];
	delete automation[1];
	delete automation[2];
}

void WhirlWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, _("Angle")));
	add_tool(automation[0] = new AutomatedFn(client, this, x + 80, y, 0));
	y += 20;
	add_tool(angle_slider = new AngleSlider(client, x, y));
	y += 35;
	add_tool(new BC_Title(x, y, _("Pinch")));
	add_tool(automation[1] = new AutomatedFn(client, this, x + 80, y, 1));
	y += 20;
	add_tool(pinch_slider = new PinchSlider(client, x, y));
	y += 35;
	add_tool(new BC_Title(x, y, _("Radius")));
	add_tool(automation[2] = new AutomatedFn(client, this, x + 80, y, 2));
	y += 20;
	add_tool(radius_slider = new RadiusSlider(client, x, y));
}


AngleSlider::AngleSlider(WhirlMain *client, int x, int y)
 : BC_ISlider(x, y, 190, 30, 200, client->angle, -MAXANGLE, MAXANGLE, DKGREY, BLACK, 1)
{
	this->client = client;
}
AngleSlider::~AngleSlider()
{
}
int AngleSlider::handle_event()
{
	client->angle = get_value();
	client->send_configure_change();
	return 1;
}

PinchSlider::PinchSlider(WhirlMain *client, int x, int y)
 : BC_ISlider(x, y, 190, 30, 200, client->pinch, -MAXPINCH, MAXPINCH, DKGREY, BLACK, 1)
{
	this->client = client;
}
PinchSlider::~PinchSlider()
{
}
int PinchSlider::handle_event()
{
	client->pinch = get_value();
	client->send_configure_change();
	return 1;
}

RadiusSlider::RadiusSlider(WhirlMain *client, int x, int y)
 : BC_ISlider(x, y, 190, 30, 200, client->radius, 0, MAXRADIUS, DKGREY, BLACK, 1)
{
	this->client = client;
}
RadiusSlider::~RadiusSlider()
{
}
int RadiusSlider::handle_event()
{
	client->radius = get_value();
	client->send_configure_change();
	return 1;
}

AutomatedFn::AutomatedFn(WhirlMain *client, WhirlWindow *window, int x, int y, int number)
 : BC_CheckBox(x, y, 16, 16, client->automated_function == number, _("Automate"))
{
	this->client = client;
	this->window = window;
	this->number = number;
}

AutomatedFn::~AutomatedFn()
{
}

int AutomatedFn::handle_event()
{
	for(int i = 0; i < 3; i++)
	{
		if(i != number) window->automation[i]->update(0);
	}
	update(1);
	client->automated_function = number;
	client->send_configure_change();
	return 1;
}

