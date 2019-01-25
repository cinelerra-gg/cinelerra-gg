
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
#include "translate.h"
#include "translatewin.h"













TranslateWin::TranslateWin(TranslateMain *client)
 : PluginClientWindow(client,
	300,
	250,
	300,
	250,
	0)
{
	this->client = client;
}

TranslateWin::~TranslateWin()
{
}

void TranslateWin::create_objects()
{
	int x = 10, y = 10;

	add_tool(new BC_Title(x, y, _("In X:")));
	y += 20;
	in_x = new TranslateCoord(this, client, x, y, &client->config.in_x);
	in_x->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, _("In Y:")));
	y += 20;
	in_y = new TranslateCoord(this, client, x, y, &client->config.in_y);
	in_y->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, _("In W:")));
	y += 20;
	in_w = new TranslateCoord(this, client, x, y, &client->config.in_w);
	in_w->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, _("In H:")));
	y += 20;
	in_h = new TranslateCoord(this, client, x, y, &client->config.in_h);
	in_h->create_objects();

	y += 40;
	add_tool(reset = new TranslateReset(client, this, x, y));

	x += 150;
	y = 10;
	add_tool(new BC_Title(x, y, _("Out X:")));
	y += 20;
	out_x = new TranslateCoord(this, client, x, y, &client->config.out_x);
	out_x->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, _("Out Y:")));
	y += 20;
	out_y = new TranslateCoord(this, client, x, y, &client->config.out_y);
	out_y->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, _("Out W:")));
	y += 20;
	out_w = new TranslateCoord(this, client, x, y, &client->config.out_w);
	out_w->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, _("Out H:")));
	y += 20;
	out_h = new TranslateCoord(this, client, x, y, &client->config.out_h);
	out_h->create_objects();
	y += 30;

	show_window();
	flush();
}

void TranslateWin::update()
{
	in_x->update((int64_t)client->config.in_x);
	in_y->update((int64_t)client->config.in_y);
	in_w->update((int64_t)client->config.in_w);
	in_h->update((int64_t)client->config.in_h);
	out_x->update((int64_t)client->config.out_x);
	out_y->update((int64_t)client->config.out_y);
	out_y->update((int64_t)client->config.out_y);
	out_w->update((int64_t)client->config.out_w);
	out_h->update((int64_t)client->config.out_h);
}



TranslateCoord::TranslateCoord(TranslateWin *win,
	TranslateMain *client,
	int x,
	int y,
	float *value)
 : BC_TumbleTextBox(win,
 	(int)*value,
	(int)0,
	(int)10000,
	x,
	y,
	100)
{
//printf("TranslateWidth::TranslateWidth %f\n", client->config.w);
	this->client = client;
	this->win = win;
	this->value = value;
}

TranslateCoord::~TranslateCoord()
{
}

int TranslateCoord::handle_event()
{
	*value = atof(get_text());

	client->send_configure_change();
	return 1;
}

TranslateReset::TranslateReset(TranslateMain *client, TranslateWin *win, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->client = client;
	this->win = win;
}
TranslateReset::~TranslateReset()
{
}
int TranslateReset::handle_event()
{
	client->config.reset();
	win->update();
	client->send_configure_change();
	return 1;
}
