
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
#include "clip.h"
#include "language.h"
#include "mwindow.h"
#include "pluginserver.h"
#include "scale.h"



ScaleWin::ScaleWin(ScaleMain *client)
 : PluginClientWindow(client, 400, 100, 400, 100, 0)
{
	this->client = client;
}

ScaleWin::~ScaleWin()
{
	delete x_factor;
	delete y_factor;
	delete width;
	delete height;
}

void ScaleWin::create_objects()
{
	int x0 = 10, y0 = 10;
	int y1 = y0 + 25;
	int y2 = y1 + 25;
	BC_Title *title = new BC_Title(x0, y1, _("Scale:"));
	add_tool(title);
	int x1 = x0 + title->get_w() + 10;
	add_tool(use_scale = new ScaleUseScale(this, client, x1, y1));
	int x2 = x1 + use_scale->get_w() + 10;
	x_factor = new ScaleXFactor(this, client, x2, y1);
	x_factor->create_objects();
	int x3 = x2 + x_factor->get_w() + 20;
	y_factor = new ScaleYFactor(this, client, x3, y1);
	y_factor->create_objects();
	add_tool(constrain = new ScaleConstrain(client, x1, y2));


	add_tool(new BC_Title(x0, y0, _("Size:")));
	add_tool(use_size = new ScaleUseSize(this, client, x1, y0));
	width = new ScaleWidth(this, client, x2, y0);
	width->create_objects();
	int x = x2 + width->get_w() + 3;
	add_tool(new BC_Title(x, y0, _("x")));
	height= new ScaleHeight(this, client, x3, y0);
	height->create_objects();
	int x4 = x3 + height->get_w() + 15;
	add_tool(pulldown = new FrameSizePulldown(client->server->mwindow->theme,
			width->get_textbox(), height->get_textbox(), x4, y0));

	show_window();
	flush();
}

ScaleXFactor::ScaleXFactor(ScaleWin *win,
	ScaleMain *client, int x, int y)
 : BC_TumbleTextBox(win, (float)client->config.x_factor, 0., 100., x, y, 100)
{
//printf("ScaleXFactor::ScaleXFactor %f\n", client->config.x_factor);
	this->client = client;
	this->win = win;
	set_increment(0.1);
	enabled = 1;
}

ScaleXFactor::~ScaleXFactor()
{
}

int ScaleXFactor::handle_event()
{
	client->config.x_factor = atof(get_text());
	CLAMP(client->config.x_factor, 0, 100);

	if(client->config.constrain)
	{
		client->config.y_factor = client->config.x_factor;
		win->y_factor->update(client->config.y_factor);
	}

//printf("ScaleXFactor::handle_event 1 %f\n", client->config.x_factor);
	if(client->config.type == FIXED_SCALE && enabled) {
		client->send_configure_change();
	}
	return 1;
}




ScaleYFactor::ScaleYFactor(ScaleWin *win, ScaleMain *client, int x, int y)
 : BC_TumbleTextBox(win, (float)client->config.y_factor, 0., 100., x, y, 100)
{
	this->client = client;
	this->win = win;
	set_increment(0.1);
	enabled = 1;
}
ScaleYFactor::~ScaleYFactor()
{
}
int ScaleYFactor::handle_event()
{
	client->config.y_factor = atof(get_text());
	CLAMP(client->config.y_factor, 0, 100);

	if(client->config.constrain)
	{
		client->config.x_factor = client->config.y_factor;
		win->x_factor->update(client->config.x_factor);
	}

	if(client->config.type == FIXED_SCALE && enabled)
	{
		client->send_configure_change();
	}
	return 1;
}



ScaleWidth::ScaleWidth(ScaleWin *win,
	ScaleMain *client, int x, int y)
 : BC_TumbleTextBox(win, client->config.width, 0, 100000, x, y, 90)
{
//printf("ScaleWidth::ScaleWidth %f\n", client->config.x_factor);
	this->client = client;
	this->win = win;
	set_increment(10);
	enabled = 1;
}

ScaleWidth::~ScaleWidth()
{
}

int ScaleWidth::handle_event()
{
	client->config.width = atoi(get_text());
	if(client->config.type == FIXED_SIZE && enabled)
	{
		client->send_configure_change();
	}
//printf("ScaleWidth::handle_event 1 %f\n", client->config.x_factor);
	return 1;
}




ScaleHeight::ScaleHeight(ScaleWin *win, ScaleMain *client, int x, int y)
 : BC_TumbleTextBox(win, client->config.height, 0, 100000, x, y, 90)
{
	this->client = client;
	this->win = win;
	set_increment(10);
	enabled = 1;
}
ScaleHeight::~ScaleHeight()
{
}

int ScaleHeight::handle_event()
{
	client->config.height = atoi(get_text());
	if(client->config.type == FIXED_SIZE && enabled)
	{
		client->send_configure_change();
	}
	return 1;
}

ScaleUseScale::ScaleUseScale(ScaleWin *win, ScaleMain *client, int x, int y)
 : BC_Radial(x, y, client->config.type == FIXED_SCALE, "")
{
        this->win = win;
        this->client = client;
	set_tooltip(_("Use fixed scale"));
}
ScaleUseScale::~ScaleUseScale()
{
}
int ScaleUseScale::handle_event()
{
	client->set_type(FIXED_SCALE);
	return 1;
}

ScaleUseSize::ScaleUseSize(ScaleWin *win, ScaleMain *client, int x, int y)
 : BC_Radial(x, y, client->config.type == FIXED_SIZE, "")
{
        this->win = win;
        this->client = client;
	set_tooltip(_("Use fixed size"));
}
ScaleUseSize::~ScaleUseSize()
{
}
int ScaleUseSize::handle_event()
{
	client->set_type(FIXED_SIZE);
	return 1;
}



ScaleConstrain::ScaleConstrain(ScaleMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.constrain, _("Constrain ratio"))
{
	this->client = client;
}
ScaleConstrain::~ScaleConstrain()
{
}
int ScaleConstrain::handle_event()
{
	client->config.constrain = get_value();
	client->send_configure_change();
	return 1;
}

