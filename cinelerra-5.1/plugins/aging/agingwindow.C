
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
#include "agingwindow.h"
#include "language.h"

AgingWindow::AgingWindow(AgingMain *plugin)
 : PluginClientWindow(plugin, 300, 180, 300, 180, 0)
{
	this->plugin = plugin;
}

AgingWindow::~AgingWindow()
{
}

void AgingWindow::create_objects()
{
	int x = 10, y = 10;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Aging:")));
	y += title->get_h() + 15;

	add_subwindow(color = new AgingCheckBox(this, x, y,
		&plugin->config.colorage, _("Grain")));
	y += color->get_h() + 5;

	add_subwindow(scratches = new AgingCheckBox(this, x, y,
		&plugin->config.scratch, _("Scratch")));
	add_subwindow(scratch_count = new AgingISlider(this, x+100, y, 180,
		0,SCRATCH_MAX, &plugin->config.scratch_lines));
	y += scratches->get_h() + 5;

	add_subwindow(pits = new AgingCheckBox(this, x, y,
		&plugin->config.pits, _("Pits")));
	add_subwindow(pit_count = new AgingISlider(this, x+100, y, 180,
		0,100, &plugin->config.pits_interval));
	y += pits->get_h() + 5;

	add_subwindow(dust = new AgingCheckBox(this, x, y,
		&plugin->config.dust, _("Dust")));
	add_subwindow(dust_count = new AgingISlider(this, x+100, y, 180,
		0,100, &plugin->config.dust_interval));

	show_window(1);
}


AgingISlider::AgingISlider(AgingWindow *win,
		int x, int y, int w, int min, int max, int *output)
 : BC_ISlider(x, y, 0, w, w, min, max, *output)
{
	this->win = win;
	this->output = output;
}

AgingISlider::~AgingISlider()
{
}

int AgingISlider::handle_event()
{
	int ret = BC_ISlider::handle_event();
	win->plugin->send_configure_change();
	return ret;
}

AgingCheckBox::AgingCheckBox(AgingWindow *win, int x, int y,
		int *output, const char *text)
 : BC_CheckBox(x, y, output, text)
{
	this->win = win;
}

int AgingCheckBox::handle_event()
{
	int ret = BC_CheckBox::handle_event();
	win->plugin->send_configure_change();
	return ret;
}

