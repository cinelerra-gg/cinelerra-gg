
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
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "bccolors.h"
#include "pluginvclient.h"
#include "swapframes.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>




REGISTER_PLUGIN(SwapFrames)







SwapFramesConfig::SwapFramesConfig()
{
	reset();
}

void SwapFramesConfig::reset()
{
	on = 1;
	swap_even = 1;
}

void SwapFramesConfig::copy_from(SwapFramesConfig &src)
{
	on = src.on;
	swap_even = src.swap_even;
}

int SwapFramesConfig::equivalent(SwapFramesConfig &src)
{
	return on == src.on &&
		swap_even == src.swap_even;
}

void SwapFramesConfig::interpolate(SwapFramesConfig &prev,
	SwapFramesConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	on = prev.on;
	swap_even = prev.swap_even;
}










SwapFramesOn::SwapFramesOn(SwapFrames *plugin,
	int x, int y)
 : BC_CheckBox(x,
 	y,
	plugin->config.on,
	_("Enabled"))
{
	this->plugin = plugin;
}

int SwapFramesOn::handle_event()
{
	plugin->config.on = get_value();
	plugin->send_configure_change();
	return 1;
}






SwapFramesEven::SwapFramesEven(SwapFrames *plugin,
	SwapFramesWindow *gui,
	int x,
	int y)
 : BC_Radial(x,
	y,
	plugin->config.swap_even,
	_("Swap 0-1, 2-3, 4-5..."))
{
	this->plugin = plugin;
	this->gui = gui;
}

int SwapFramesEven::handle_event()
{
	plugin->config.swap_even = 1;
	gui->swap_odd->update(0);
	plugin->send_configure_change();
	return 1;
}






SwapFramesOdd::SwapFramesOdd(SwapFrames *plugin,
	SwapFramesWindow *gui,
	int x,
	int y)
 : BC_Radial(x,
	y,
	!plugin->config.swap_even,
	_("Swap 1-2, 3-4, 5-6..."))
{
	this->plugin = plugin;
	this->gui = gui;
}

int SwapFramesOdd::handle_event()
{
	plugin->config.swap_even = 0;
	gui->swap_even->update(0);
	plugin->send_configure_change();
	return 1;
}







SwapFramesReset::SwapFramesReset(SwapFrames *plugin, SwapFramesWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->gui = gui;
}
SwapFramesReset::~SwapFramesReset()
{
}
int SwapFramesReset::handle_event()
{
	plugin->config.reset();
	gui->update();
	plugin->send_configure_change();
	return 1;
}





SwapFramesWindow::SwapFramesWindow(SwapFrames *plugin)
 : PluginClientWindow(plugin,
	260,
	135,
	260,
	135,
	0)
{
	this->plugin = plugin;
}

void SwapFramesWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(on = new SwapFramesOn(plugin, x, y));
	y += on->get_h() + 5;
	BC_Bar *bar;
	add_subwindow(bar = new BC_Bar(x, y, get_w() - x * 2));
	y += bar->get_h() + 5;
	add_subwindow(swap_even = new SwapFramesEven(plugin,
		this,
		x,
		y));
	y += swap_even->get_h() + 5;
	add_subwindow(swap_odd = new SwapFramesOdd(plugin,
		this,
		x,
		y));

	y += 35;
	add_subwindow(reset = new SwapFramesReset(plugin, this, x, y));

	show_window();
}

// for Reset button
void SwapFramesWindow::update()
{
	on->update(plugin->config.swap_even);
	swap_even->update(plugin->config.swap_even);
	swap_odd->update(!plugin->config.swap_even);
}











SwapFrames::SwapFrames(PluginServer *server)
 : PluginVClient(server)
{

	buffer = 0;
	buffer_position = -1;
	prev_frame = -1;
}

SwapFrames::~SwapFrames()
{

	delete buffer;
}

const char* SwapFrames::plugin_title() { return N_("Swap Frames"); }
int SwapFrames::is_realtime() { return 1; }

NEW_WINDOW_MACRO(SwapFrames, SwapFramesWindow)
LOAD_CONFIGURATION_MACRO(SwapFrames, SwapFramesConfig)

void SwapFrames::update_gui()
{
	if(thread)
	{
		thread->window->lock_window();
		if(load_configuration())
		{
			((SwapFramesWindow*)thread->window)->on->update(config.on);
			((SwapFramesWindow*)thread->window)->swap_even->update(config.swap_even);
			((SwapFramesWindow*)thread->window)->swap_odd->update(!config.swap_even);
		}
		thread->window->unlock_window();
	}
}


void SwapFrames::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("SWAPFRAMES");
	output.tag.set_property("ON", config.on);
	output.tag.set_property("SWAP_EVEN", config.swap_even);
	output.append_tag();
	output.tag.set_title("/SWAPFRAMES");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void SwapFrames::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	while(!input.read_tag())
	{
		if(input.tag.title_is("SWAPFRAMES"))
		{
			config.on = input.tag.get_property("ON", config.on);
			config.swap_even = input.tag.get_property("SWAP_EVEN", config.swap_even);
		}
	}
}


int SwapFrames::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();
	int64_t new_position = start_position;

	if(config.on)
	{
		if(config.swap_even)
		{
			if(new_position % 2)
				new_position--;
			else
				new_position++;
		}
		else
		{
			if(new_position % 2)
				new_position++;
			else
				new_position--;
		}
	}

// Recall a frame
//printf("SwapFrames::process_buffer %d new_position=%lld\n", __LINE__, new_position);
	if(buffer && buffer_position == new_position)
	{
//printf("SwapFrames::process_buffer %d\n", __LINE__);
		frame->copy_from(buffer);
	}
	else
// Skipped a frame
	if(new_position > prev_frame + 1)
	{
//printf("SwapFrames::process_buffer %d\n", __LINE__);
		if(!buffer)
			buffer = new VFrame(frame->get_w(), frame->get_h(),
				frame->get_color_model(), 0);
		buffer_position = new_position - 1;
		read_frame(buffer,
			0,
			buffer_position,
			frame_rate,
			0);
		read_frame(frame,
			0,
			new_position,
			frame_rate,
			get_use_opengl());
		prev_frame = new_position;
	}
// Read new frame
	else
	{
//printf("SwapFrames::process_buffer %d\n", __LINE__);
		read_frame(frame,
			0,
			new_position,
			frame_rate,
			get_use_opengl());
		prev_frame = new_position;
	}


	return 0;
}

