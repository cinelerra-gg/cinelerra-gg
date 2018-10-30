
/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "theme.h"
#include "../timestretch/timestretchengine.h"
#include "timestretchrt.h"
#include "transportque.h"

#include <string.h>







REGISTER_PLUGIN(TimeStretchRT);



TimeStretchRTConfig::TimeStretchRTConfig()
{
//	scale = 1;
	num = 1;
	denom = 1;
	size = 40;
}


int TimeStretchRTConfig::equivalent(TimeStretchRTConfig &src)
{
//	return fabs(scale - src.scale) < 0.0001 && size == src.size;
	return fabs(num - src.num) < 0.0001 &&
		fabs(denom - src.denom) < 0.0001 && 
		size == src.size;
}

void TimeStretchRTConfig::copy_from(TimeStretchRTConfig &src)
{
	this->num = src.num;
	this->denom = src.denom;
//	this->scale = src.scale;
	this->size = src.size;
}

void TimeStretchRTConfig::interpolate(TimeStretchRTConfig &prev,
	TimeStretchRTConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	this->num = prev.num;
	this->denom = prev.denom;
//	this->scale = prev.scale;
	this->size = prev.size;
	boundaries();
}

void TimeStretchRTConfig::boundaries()
{
	if(num < 0.0001) num = 0.0001;
	if(denom < 0.0001) denom = 0.0001;
//	if(fabs(scale) < 0.01) scale = 0.01;
//	if(fabs(scale) > 100) scale = 100;
	if(size < 10) size = 10;
	if(size > 1000) size = 1000;
}




TimeStretchRTWindow::TimeStretchRTWindow(TimeStretchRT *plugin)
 : PluginClientWindow(plugin,
	210, 
	200, 
	200, 
	210,
	0)
{
	this->plugin = plugin;
}

TimeStretchRTWindow::~TimeStretchRTWindow()
{
}

void TimeStretchRTWindow::create_objects()
{
	int x = 10, y = 10;
	int margin = plugin->get_theme()->widget_border;

	BC_Title *title = 0;
	add_subwindow(title = new BC_Title(x, y, _("Input samples:")));
	y += title->get_h() + plugin->get_theme()->widget_border;
	num = new TimeStretchRTScale(this,
		plugin, 
		x, 
		y,
		&plugin->config.num);
	num->create_objects();
	y += num->get_h() + margin;

	add_subwindow(title = new BC_Title(x, y, _("Output samples:")));
	y += title->get_h() + plugin->get_theme()->widget_border;
	denom = new TimeStretchRTScale(this,
		plugin, 
		x, 
		y,
		&plugin->config.denom);
	denom->create_objects();

	y += denom->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Window size (ms):")));
	y += title->get_h() + margin;
	size = new TimeStretchRTSize(this,
		plugin,
		x,
		y);
	size->create_objects();

	show_window();
}






TimeStretchRTScale::TimeStretchRTScale(TimeStretchRTWindow *window,
	TimeStretchRT *plugin,
	int x,
	int y,
	double *value)
 : BC_TumbleTextBox(window,
 	(float)*value,
	(float)0.0001,
	(float)1000,
 	x,
	y,
	100)
{
	this->plugin = plugin;
	this->value = value;
	set_increment(0.01);
}

int TimeStretchRTScale::handle_event()
{
	*value = atof(get_text());
	plugin->config.boundaries();
	plugin->send_configure_change();
	return 1;
}




TimeStretchRTSize::TimeStretchRTSize(TimeStretchRTWindow *window,
	TimeStretchRT *plugin,
	int x,
	int y)
 : BC_TumbleTextBox(window,
 	plugin->config.size,
	10,
	1000,
 	x,
	y,
	100)
{
	this->plugin = plugin;
	set_increment(10);
}

int TimeStretchRTSize::handle_event()
{
	plugin->config.size = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}








TimeStretchRT::TimeStretchRT(PluginServer *server)
 : PluginAClient(server)
{
	need_reconfigure = 1;
	dest_start = -1;
	source_start = -1;
	engine = 0;
}


TimeStretchRT::~TimeStretchRT()
{
	delete engine;
}

const char* TimeStretchRT::plugin_title() { return N_("Time Stretch RT"); }
int TimeStretchRT::is_realtime() { return 1; }
int TimeStretchRT::is_synthesis() { return 1; }


NEW_WINDOW_MACRO(TimeStretchRT, TimeStretchRTWindow)

LOAD_CONFIGURATION_MACRO(TimeStretchRT, TimeStretchRTConfig)


int TimeStretchRT::process_buffer(int64_t size,
	Samples *buffer,
	int64_t start_position,
	int sample_rate)
{
	need_reconfigure = load_configuration();

	if(!engine) engine = new TimeStretchEngine(config.denom / config.num, 
		sample_rate,
		config.size);

	if(start_position != dest_start) need_reconfigure = 1;
	dest_start = start_position;

// Get start position of the input.
// Sample 0 is the keyframe position
	if(need_reconfigure)
	{
		int64_t prev_position = edl_to_local(
			get_prev_keyframe(
				get_source_position())->position);

		if(prev_position == 0)
		{
			prev_position = get_source_start();
		}

		source_start = (int64_t)((start_position - prev_position) * 
			config.num / config.denom) + prev_position;

		engine->reset();
		engine->update(config.denom / config.num, sample_rate, config.size);
		need_reconfigure = 0;
// printf("TimeStretchRT::process_buffer %d start_position=%lld prev_position=%lld scale=%f source_start=%lld\n", 
// __LINE__, 
// start_position,
// prev_position,
// config.scale,
// source_start);
	}

// process buffers until output length is reached
	int error = 0;
	while(!error && engine->get_output_size() < size)
	{

// printf("TimeStretchRT::process_buffer %d buffer=%p size=%lld source_start=%lld\n",
// __LINE__,
// buffer,
// size,
// source_start);
		read_samples(buffer,
			0,
			sample_rate,
			source_start,
			size);


		if(get_direction() == PLAY_FORWARD)
			source_start += size;
		else
			source_start -= size;

//printf("TimeStretchRT::process_buffer %d size=%d\n", __LINE__, size);
		engine->process(buffer, size);
//printf("TimeStretchRT::process_buffer %d\n", __LINE__);

	}
//printf("TimeStretchRT::process_buffer %d\n", __LINE__);


	engine->read_output(buffer, size);

//printf("TimeStretchRT::process_buffer %d\n", __LINE__);
	if(get_direction() == PLAY_FORWARD)
		dest_start += size;
	else
		dest_start -= size;

	return 0;
}





void TimeStretchRT::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("TIMESTRETCHRT");
	output.tag.set_property("NUM", config.num);
	output.tag.set_property("DENOM", config.denom);
	output.tag.set_property("SIZE", config.size);
	output.append_tag();
	output.tag.set_title("/TIMESTRETCHRT");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void TimeStretchRT::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	while(!input.read_tag())
	{
		if(input.tag.title_is("TIMESTRETCHRT"))
		{
			config.num = input.tag.get_property("NUM", config.num);
			config.denom = input.tag.get_property("DENOM", config.denom);
			config.size = input.tag.get_property("SIZE", config.size);
		}
	}
}

void TimeStretchRT::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{

			thread->window->lock_window("TimeStretchRT::update_gui");
			((TimeStretchRTWindow*)thread->window)->num->update((float)config.num);
			((TimeStretchRTWindow*)thread->window)->denom->update((float)config.denom);
			((TimeStretchRTWindow*)thread->window)->size->update((int64_t)config.size);
			thread->window->unlock_window();
		}
	}
}





