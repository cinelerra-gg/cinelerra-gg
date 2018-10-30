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
#include "stylizeobj.h"
#include "stylizeobjwindow.h"
#include "theme.h"

StylizeObjWindow::StylizeObjWindow(StylizeObj *plugin)
 : PluginClientWindow(plugin, 320, 160, 320, 160, 0)
{
	this->plugin = plugin; 
	smoothing = 0;
	edge_strength = 0;
	shade_factor = 0;
	smooth_title = 0;
	edge_title = 0;
	shade_title = 0;
}

StylizeObjWindow::~StylizeObjWindow()
{
}

void StylizeObjWindow::create_objects()
{
	int x = 10, y = 10;
	BC_Title *title = new BC_Title(x, y, _("StylizeObj"));
	add_subwindow(title);
	y += title->get_h() + 10;
	int x1 = x;
	add_subwindow(title = new BC_Title(x1, y, _("Mode: ")));
	x1 += title->get_w() + 15;
	add_subwindow(mode = new StylizeObjMode(this, x1, y, &plugin->config.mode));
	mode->create_objects();
	y += mode->get_h() + 10;
	x0 = x;  y0 = y;
	update_params();
	show_window(1);
}

void StylizeObjWindow::update_params()
{
	int x = x0, y = y0;
	if( plugin->config.mode == MODE_PENCIL_SKETCH ||
	    plugin->config.mode == MODE_COLOR_SKETCH ) {
		int x1 = x + 80;
		if( !smooth_title )
			add_subwindow(smooth_title = new BC_Title(x,y,_("Smooth:")));
		if( !smoothing )
			add_subwindow(smoothing = new StylizeObjFSlider(this,
				x1,y,180, 0,100, &plugin->config.smoothing));
		y += smoothing->get_h() + 10;
		if( !edge_title )
			add_subwindow(edge_title = new BC_Title(x,y,_("Edges:")));
		if( !edge_strength )
			add_subwindow(edge_strength = new StylizeObjFSlider(this,
				x1,y,180, 0,100, &plugin->config.edge_strength));
		y += edge_strength->get_h() + 10;
		if( !shade_title )
			add_subwindow(shade_title = new BC_Title(x,y,_("Shade:")));
		if( !shade_factor )
			add_subwindow(shade_factor = new StylizeObjFSlider(this,
				x1,y,180, 0,100, &plugin->config.shade_factor));
		//y += shade_factor->get_h() + 10;
	}
	else {
		delete smooth_title;	smooth_title = 0;
		delete smoothing;	smoothing = 0;
		delete edge_title;	edge_title = 0;
		delete edge_strength;	edge_strength = 0;
		delete shade_title;	shade_title = 0;
		delete shade_factor;	shade_factor = 0;
	}
}

StylizeObjModeItem::StylizeObjModeItem(StylizeObjMode *popup, int type, const char *text)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->type = type;
}

StylizeObjModeItem::~StylizeObjModeItem()
{
}

int StylizeObjModeItem::handle_event()
{
	popup->update(type);
	popup->win->update_params();
	return popup->handle_event();
}

StylizeObjMode::StylizeObjMode(StylizeObjWindow *win, int x, int y, int *value)
 : BC_PopupMenu(x, y, 150, "", 1)
{
	this->win = win;
	this->value = value;
}

StylizeObjMode::~StylizeObjMode()
{
}

void StylizeObjMode::create_objects()
{
	add_item(new StylizeObjModeItem(this, MODE_EDGE_SMOOTH,    _("Edge smooth")));
	add_item(new StylizeObjModeItem(this, MODE_EDGE_RECURSIVE, _("Edge recursive")));
	add_item(new StylizeObjModeItem(this, MODE_DETAIL_ENHANCE, _("Detail Enhance")));
	add_item(new StylizeObjModeItem(this, MODE_PENCIL_SKETCH,  _("Pencil Sketch")));
	add_item(new StylizeObjModeItem(this, MODE_COLOR_SKETCH,   _("Color Sketch")));
	add_item(new StylizeObjModeItem(this, MODE_STYLIZATION,    _("Stylization")));
	set_value(*value);
}

int StylizeObjMode::handle_event()
{
	win->plugin->send_configure_change();
	return 1;
}

void StylizeObjMode::update(int v)
{
	set_value(*value = v);
}

void StylizeObjMode::set_value(int v)
{
	int i = total_items();
	while( --i >= 0 && ((StylizeObjModeItem*)get_item(i))->type != v );
	if( i >= 0 ) set_text(get_item(i)->get_text());
}

StylizeObjFSlider::StylizeObjFSlider(StylizeObjWindow *win,
		int x, int y, int w, float min, float max, float *output)
 : BC_FSlider(x, y, 0,w,w, min,max, *output)
{
        this->win = win;
        this->output = output;
}
StylizeObjFSlider::~StylizeObjFSlider()
{
}

int StylizeObjFSlider::handle_event()
{
	*output = get_value();
	win->plugin->send_configure_change();
        return 1;
}

