
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
#include "deinterwindow.h"
#include "language.h"








DeInterlaceWindow::DeInterlaceWindow(DeInterlaceMain *client)
 : PluginClientWindow(client,
	200,
	250,
	200,
	250,
	0)
{
	this->client = client;
}

DeInterlaceWindow::~DeInterlaceWindow()
{
}

void DeInterlaceWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, _("Select lines to keep")));
	y += 25;
	add_tool(none = new DeInterlaceOption(client, this, DEINTERLACE_NONE, x, y, _("Do nothing")));
	y += 25;
	add_tool(odd_fields = new DeInterlaceOption(client, this, DEINTERLACE_EVEN, x, y, _("Odd lines")));
	y += 25;
	add_tool(even_fields = new DeInterlaceOption(client, this, DEINTERLACE_ODD, x, y, _("Even lines")));
	y += 25;
	add_tool(average_fields = new DeInterlaceOption(client, this, DEINTERLACE_AVG, x, y, _("Average lines")));
	y += 25;
	add_tool(swap_odd_fields = new DeInterlaceOption(client, this, DEINTERLACE_SWAP_ODD, x, y, _("Swap odd fields")));
	y += 25;
	add_tool(swap_even_fields = new DeInterlaceOption(client, this, DEINTERLACE_SWAP_EVEN, x, y, _("Swap even fields")));
	y += 25;
	add_tool(avg_even = new DeInterlaceOption(client, this, DEINTERLACE_AVG_EVEN, x, y, _("Average even lines")));

// 	draw_line(170, y + 5, 190, y + 5);
// 	draw_line(190, y + 5, 190, y + 70);
// 	draw_line(150, y + 70, 190, y + 70);
 	y += 25;
 	add_tool(avg_odd = new DeInterlaceOption(client, this, DEINTERLACE_AVG_ODD, x, y, _("Average odd lines")));
// 	draw_line(170, y + 5, 190, y + 5);
// 	y += 30;
//	add_tool(adaptive = new DeInterlaceAdaptive(client, x, y));
//	add_tool(threshold = new DeInterlaceThreshold(client, x + 100, y));
//	y += 50;
//	char string[BCTEXTLEN];
//	get_status_string(string, 0);
//	add_tool(status = new BC_Title(x, y, string));
	show_window();
}


void DeInterlaceWindow::get_status_string(char *string, int changed_rows)
{
	sprintf(string, _("Changed rows: %d\n"), changed_rows);
}

int DeInterlaceWindow::set_mode(int mode, int recursive)
{
	none->update(mode == DEINTERLACE_NONE);
	odd_fields->update(mode == DEINTERLACE_EVEN);
	even_fields->update(mode == DEINTERLACE_ODD);
	average_fields->update(mode == DEINTERLACE_AVG);
	swap_odd_fields->update(mode == DEINTERLACE_SWAP_ODD);
	swap_even_fields->update(mode == DEINTERLACE_SWAP_EVEN);
	avg_even->update(mode == DEINTERLACE_AVG_EVEN);
	avg_odd->update(mode == DEINTERLACE_AVG_ODD);

	client->config.mode = mode;

	if(!recursive)
		client->send_configure_change();
	return 0;
}


DeInterlaceOption::DeInterlaceOption(DeInterlaceMain *client,
		DeInterlaceWindow *window,
		int output,
		int x,
		int y,
		char *text)
 : BC_Radial(x, y, client->config.mode == output, text)
{
	this->client = client;
	this->window = window;
	this->output = output;
}

DeInterlaceOption::~DeInterlaceOption()
{
}
int DeInterlaceOption::handle_event()
{
	window->set_mode(output, 0);
	return 1;
}


// DeInterlaceAdaptive::DeInterlaceAdaptive(DeInterlaceMain *client, int x, int y)
//  : BC_CheckBox(x, y, client->config.adaptive, _("Adaptive"))
// {
// 	this->client = client;
// }
// int DeInterlaceAdaptive::handle_event()
// {
// 	client->config.adaptive = get_value();
// 	client->send_configure_change();
// 	return 1;
// }
//
//
//
// DeInterlaceThreshold::DeInterlaceThreshold(DeInterlaceMain *client, int x, int y)
//  : BC_IPot(x, y, client->config.threshold, 0, 100)
// {
// 	this->client = client;
// }
// int DeInterlaceThreshold::handle_event()
// {
// 	client->config.threshold = get_value();
// 	client->send_configure_change();
// 	return 1;
// }
//




