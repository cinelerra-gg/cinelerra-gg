
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
#include "rescale.h"
#include "scaleratio.h"
#include "scaleratiowin.h"
#include "mwindow.h"


ScaleRatioWin::ScaleRatioWin(ScaleRatioMain *client)
 : PluginClientWindow(client, 400, 320, 400, 320, 0)
{
	this->client = client;
}

ScaleRatioWin::~ScaleRatioWin()
{
}

void ScaleRatioWin::create_objects()
{
	int x = 10, y = 10;
	int x0 = x, x1 = x0 + 50;
	int y0 = y;
	client->load_configuration();

	add_tool(new BC_Title(x0, y0, _("In R:")));
	add_tool(in_r = new ScaleRatioRatio(this, client, x1, y0, &client->config.in_r));
	in_r->create_objects();
	y0 += in_r->get_th() + 10;

	add_tool(new BC_Title(x0, y0, _("In W:")));
	in_w = new ScaleRatioCoord(this, client, x1, y0, 0, &client->config.in_w);
	in_w->create_objects();
	y0 += in_w->get_h() + 8;

	add_tool(new BC_Title(x0, y0, _("In H:")));
	in_h = new ScaleRatioCoord(this, client, x1, y0, 0, &client->config.in_h);
	in_h->create_objects();

	x0 = get_w()/2;  y0 = y;
	x1 = x0 + 50;

	add_tool(new BC_Title(x0, y0, _("Out R:")));
	add_tool(out_r = new ScaleRatioRatio(this, client, x1, y0, &client->config.out_r));
	out_r->create_objects();
	y0 += out_r->get_th() + 10;

	add_tool(new BC_Title(x0, y0, _("Out W:")));
	out_w = new ScaleRatioCoord(this, client, x1, y0, 0, &client->config.out_w);
	out_w->create_objects();
	y0 += out_w->get_h() + 8;

	add_tool(new BC_Title(x0, y0, _("Out H:")));
	out_h = new ScaleRatioCoord(this, client, x1, y0, 0, &client->config.out_h);
	out_h->create_objects();
	y0 += out_h->get_h() + 8;

	y = y0 + 5;
	add_tool(apply_button = new ScaleRatioApply(this, x, y));
	int x2 = x + apply_button->get_w() + 50;
	add_tool(type_popup = new ScaleRatioType(this, x2, y, &client->config.type));
	type_popup->create_objects();
	y += apply_button->get_h() + 30;

	x0 = x;  y0 = y;
	x1 = x0 + 50;

	add_tool(new BC_Title(x0, y0, _("Src X:")));
	src_x = new ScaleRatioCoord(this, client, x1, y0, 1, &client->config.src_x);
	src_x->create_objects();
	y0 += 30;

	add_tool(new BC_Title(x0, y0, _("Src Y:")));
	src_y = new ScaleRatioCoord(this, client, x1, y0, 1, &client->config.src_y);
	src_y->create_objects();
	y0 += 30;


	add_tool(new BC_Title(x0, y0, _("Src W:")));
	src_w = new ScaleRatioCoord(this, client, x1, y0, 0, &client->config.src_w);
	src_w->create_objects();
	y0 += 30;

	add_tool(new BC_Title(x0, y0, _("Src H:")));
	src_h = new ScaleRatioCoord(this, client, x1, y0, 0, &client->config.src_h);
	src_h->create_objects();
	y0 += 30;

	x0 = get_w()/2;
	x1 = x0 + 50;
	y0 = y;
	add_tool(new BC_Title(x0, y0, _("Dst X:")));
	dst_x = new ScaleRatioCoord(this, client, x1, y0, 1, &client->config.dst_x);
	dst_x->create_objects();
	y0 += 30;

	add_tool(new BC_Title(x0, y0, _("Dst Y:")));
	dst_y = new ScaleRatioCoord(this, client, x1, y0, 1, &client->config.dst_y);
	dst_y->create_objects();
	y0 += 30;

	add_tool(new BC_Title(x0, y0, _("Dst W:")));
	dst_w = new ScaleRatioCoord(this, client, x1, y0, 0, &client->config.dst_w);
	dst_w->create_objects();
	y0 += 30;

	add_tool(new BC_Title(x0, y0, _("Dst H:")));
	dst_h = new ScaleRatioCoord(this, client, x1, y0, 0, &client->config.dst_h);
	dst_h->create_objects();
	y0 += 30;

	show_window();
	flush();
}



ScaleRatioCoord::ScaleRatioCoord(ScaleRatioWin *win,
	ScaleRatioMain *client, int x, int y, int s, float *value)
 : BC_TumbleTextBox(win, (int)*value, (int)-10000*s, (int)10000, x, y, 100)
{
	this->client = client;
	this->win = win;
	this->value = value;
}

ScaleRatioCoord::~ScaleRatioCoord()
{
}

int ScaleRatioCoord::handle_event()
{
	*value = atof(get_text());
	client->send_configure_change();
	return 1;
}


ScaleRatioTumbler::ScaleRatioTumbler(ScaleRatioRatio *ratio, int value, int x, int y)
 : BC_TumbleTextBox(ratio->win, value, 0, 10000, x, y, 45)
{
	this->ratio = ratio;
}

ScaleRatioTumbler::~ScaleRatioTumbler()
{
}

int ScaleRatioTumbler::handle_event()
{
	ratio->update_ratio();
	return 1;
}

ScaleRatioRatio::ScaleRatioRatio(ScaleRatioWin *win,
	ScaleRatioMain *client, int x, int y, float *value)
 : BC_TextBox(x, y, 100, 1, *value)
{
	this->client = client;
	this->win = win;
	this->value = value;
	this->ah = 0;
	this->aw = 0;
}

ScaleRatioRatio::~ScaleRatioRatio()
{
	delete ah;
	delete aw;
}

int ScaleRatioRatio::handle_event()
{
	*value = atof(get_text());
	float fah = 0, faw = 0;
	MWindow::create_aspect_ratio(faw, fah, *value*1000000, 1000000);
	ah->update((int64_t)fah);
	aw->update((int64_t)faw);
	win->update_config();
	win->update_gui();
	return 1;
}

void ScaleRatioRatio::create_objects()
{
	int tx = BC_TextBox::get_x();
	int ty = BC_TextBox::get_y();
	int x = tx;
	int y = ty + BC_TextBox::get_h() + 5;
	float faw = 0, fah = 0;
	MWindow::create_aspect_ratio(faw, fah, *value*1000000, 1000000);
	aw = new ScaleRatioTumbler(this, faw, x, y);
	aw->create_objects();
	x += aw->get_w() + 5;
	ah = new ScaleRatioTumbler(this, fah, x, y);
	ah->create_objects();
	x += ah->get_w();
	y += ah->get_h();
	tw = x - tx;
	int w = BC_TextBox::get_w();
	if( tw < w ) tw = w;
	th = y - ty;
}

void ScaleRatioRatio::update_ratio()
{
	float fah = atof(ah->get_text());
	float faw = atof(aw->get_text());
	*value = fah > 0 ? faw / fah : 1.f;
	win->update_config();
	win->update_gui();
}

ScaleRatioTypeItem::ScaleRatioTypeItem(ScaleRatioType *popup, int type, const char *text)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->type = type;
}

ScaleRatioTypeItem::~ScaleRatioTypeItem()
{
}

int ScaleRatioTypeItem::handle_event()
{
	popup->win->client->config.type = type;
	popup->set_value(type);
	return popup->handle_event();
}


ScaleRatioType::ScaleRatioType(ScaleRatioWin *win, int x, int y, int *value)
 : BC_PopupMenu(x, y, 180, "", 1)
{
	this->win = win;
	this->value = value;
}

ScaleRatioType::~ScaleRatioType()
{
}

void ScaleRatioType::create_objects()
{
	for( int i=1; i<Rescale::n_scale_types; ++i )
		add_item(new ScaleRatioTypeItem(this, i, Rescale::scale_types[i]));
	set_value(*value);
}

void ScaleRatioType::update(int v)
{
	set_value(*value = v);
}

int ScaleRatioType::handle_event()
{
	win->update_config();
	win->update_gui();
	return 1;
}

int ScaleRatioApply::handle_event()
{
	win->update_config();
	win->update_gui();
	win->client->send_configure_change();
	return 1;
}

void ScaleRatioWin::update_config()
{
	ScaleRatioConfig *conf = &client->config;
	Rescale in(conf->in_w, conf->in_h, conf->in_r);
	Rescale out(conf->out_w, conf->out_h, conf->out_r);
	in.rescale(out,conf->type, conf->src_w,conf->src_h, conf->dst_w,conf->dst_h);
}

void ScaleRatioWin::update_gui()
{
	ScaleRatioConfig *conf = &client->config;
	type_popup->update(conf->type);
	in_r->update(conf->in_r);
	out_r->update(conf->out_r);
	in_w->update(conf->in_w);
	in_h->update(conf->in_h);
	src_x->update(conf->src_x);
	src_y->update(conf->src_y);
	src_w->update(conf->src_w);
	src_h->update(conf->src_h);
	out_w->update(conf->out_w);
	out_h->update(conf->out_h);
	dst_x->update(conf->dst_x);
	dst_y->update(conf->dst_y);
	dst_w->update(conf->dst_w);
	dst_h->update(conf->dst_h);
}

