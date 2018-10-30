
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
#include "timeavgwindow.h"



#define MAX_FRAMES 1024


TimeAvgWindow::TimeAvgWindow(TimeAvgMain *client)
 : PluginClientWindow(client, 250, 400, 250, 400, 0)
{
	this->client = client;
}

TimeAvgWindow::~TimeAvgWindow()
{
}

void TimeAvgWindow::create_objects()
{
	int x = 10, y = 10;
	BC_Bar *bar;
	BC_Title *title;

	add_tool(title = new BC_Title(x, y, _("Frame count:")));
	y += title->get_h() + 5;
	add_tool(total_frames = new TimeAvgSlider(client, x, y));
	y += 30;
	add_tool(paranoid = new TimeAvgParanoid(client, x, y));
	y += 30;
	add_tool(no_subtract = new TimeAvgNoSubtract(client, x, y));
	y += 30;
	add_tool(bar = new BC_Bar(x, y, get_w() - x * 2));
	y += bar->get_h() + 5;



	add_tool(avg = new TimeAvgAvg(client, this, x, y));
	y += 30;
	add_tool(accum = new TimeAvgAccum(client, this, x, y));
	y += 30;
	add_tool(bar = new BC_Bar(x, y, get_w() - x * 2));
	y += bar->get_h() + 5;



	add_tool(replace = new TimeAvgReplace(client, this, x, y));
	y += 30;
	add_tool(new BC_Title(x, y, _("Threshold:")));
	y += title->get_h() + 5;
	add_tool(threshold = new TimeThresholdSlider(client, x, y));
	y += 30;
	add_tool(new BC_Title(x, y, _("Border:")));
	y += title->get_h() + 5;
	add_tool(border = new TimeBorderSlider(client, x, y));
	y += 30;



	add_tool(bar = new BC_Bar(x, y, get_w() - x * 2));
	y += bar->get_h() + 5;


	add_tool(greater = new TimeAvgGreater(client, this, x, y));
	y += 30;
	add_tool(less = new TimeAvgLess(client, this, x, y));
	y += 30;

	update_toggles();
	show_window();
	flush();
}

void TimeAvgWindow::update_toggles()
{
	avg->update(client->config.mode == TimeAvgConfig::AVERAGE);
	accum->update(client->config.mode == TimeAvgConfig::ACCUMULATE);
	replace->update(client->config.mode == TimeAvgConfig::REPLACE);
	greater->update(client->config.mode == TimeAvgConfig::GREATER);
	less->update(client->config.mode == TimeAvgConfig::LESS);

// 	if(client->config.mode == TimeAvgConfig::AVERAGE ||
// 		client->config.mode == TimeAvgConfig::ACCUMULATE)
// 	{
// 		no_subtract->enable();
// 	}
// 	else
// 	{
// 		no_subtract->disable();
// 	}

	if(client->config.mode == TimeAvgConfig::REPLACE)
	{
		threshold->enable();
		border->enable();
	}
	else
	{
		threshold->disable();
		border->disable();
	}
}


TimeAvgSlider::TimeAvgSlider(TimeAvgMain *client, int x, int y)
 : BC_ISlider(x,
 	y,
	0,
	190,
	200,
	1,
	MAX_FRAMES,
	client->config.frames)
{
	this->client = client;
}
TimeAvgSlider::~TimeAvgSlider()
{
}
int TimeAvgSlider::handle_event()
{
	int result = get_value();
	if(result < 1) result = 1;
	client->config.frames = result;
	client->send_configure_change();
	return 1;
}



TimeThresholdSlider::TimeThresholdSlider(TimeAvgMain *client, int x, int y)
 : BC_ISlider(x,
 	y,
	0,
	190,
	200,
	1,
	255,
	client->config.threshold)
{
	this->client = client;
}
TimeThresholdSlider::~TimeThresholdSlider()
{
}
int TimeThresholdSlider::handle_event()
{
	int result = get_value();
	if(result < 1) result = 1;
	client->config.threshold = result;
	client->send_configure_change();
	return 1;
}


TimeBorderSlider::TimeBorderSlider(TimeAvgMain *client, int x, int y)
 : BC_ISlider(x,
 	y,
	0,
	190,
	200,
	0,
	8,
	client->config.border)
{
	this->client = client;
}
TimeBorderSlider::~TimeBorderSlider()
{
}
int TimeBorderSlider::handle_event()
{
	int result = get_value();
	if(result < 0) result = 0;
	client->config.border = result;
	client->send_configure_change();
	return 1;
}




TimeAvgAvg::TimeAvgAvg(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y)
 : BC_Radial(x, y,
	client->config.mode == TimeAvgConfig::AVERAGE,
	_("Average"))
{
	this->client = client;
	this->gui = gui;
}
int TimeAvgAvg::handle_event()
{
	int result = get_value(); (void)result;
	client->config.mode = TimeAvgConfig::AVERAGE;
	gui->update_toggles();
	client->send_configure_change();
	return 1;
}




TimeAvgAccum::TimeAvgAccum(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y)
 : BC_Radial(x, y,
	client->config.mode == TimeAvgConfig::ACCUMULATE,
	_("Accumulate"))
{
	this->client = client;
	this->gui = gui;
}
int TimeAvgAccum::handle_event()
{
	int result = get_value(); (void)result;
	client->config.mode = TimeAvgConfig::ACCUMULATE;
	gui->update_toggles();
	client->send_configure_change();
	return 1;
}



TimeAvgReplace::TimeAvgReplace(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y)
 : BC_Radial(x, y,
	client->config.mode == TimeAvgConfig::REPLACE,
	_("Replace"))
{
	this->client = client;
	this->gui = gui;
}
int TimeAvgReplace::handle_event()
{
	int result = get_value(); (void)result;
	client->config.mode = TimeAvgConfig::REPLACE;
	gui->update_toggles();
	client->send_configure_change();
	return 1;
}


TimeAvgGreater::TimeAvgGreater(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y)
 : BC_Radial(x, y,
	client->config.mode == TimeAvgConfig::GREATER,
	_("Greater"))
{
	this->client = client;
	this->gui = gui;
}
int TimeAvgGreater::handle_event()
{
	int result = get_value(); (void)result;
	client->config.mode = TimeAvgConfig::GREATER;
	gui->update_toggles();
	client->send_configure_change();
	return 1;
}


TimeAvgLess::TimeAvgLess(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y)
 : BC_Radial(x, y,
	client->config.mode == TimeAvgConfig::LESS,
	_("Less"))
{
	this->client = client;
	this->gui = gui;
}
int TimeAvgLess::handle_event()
{
	int result = get_value(); (void)result;
	client->config.mode = TimeAvgConfig::LESS;
	gui->update_toggles();
	client->send_configure_change();
	return 1;
}



TimeAvgParanoid::TimeAvgParanoid(TimeAvgMain *client, int x, int y)
 : BC_CheckBox(x, y,
	client->config.paranoid,
	_("Restart for every frame"))
{
	this->client = client;
}
int TimeAvgParanoid::handle_event()
{
	int result = get_value();
	client->config.paranoid = result;
	client->send_configure_change();
	return 1;
}





TimeAvgNoSubtract::TimeAvgNoSubtract(TimeAvgMain *client, int x, int y)
 : BC_CheckBox(x, y,
	client->config.nosubtract,
	_("Don't buffer frames"))
{
	this->client = client;
}
int TimeAvgNoSubtract::handle_event()
{
	int result = get_value();
	client->config.nosubtract = result;
	client->send_configure_change();
	return 1;
}



