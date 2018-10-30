
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
#include "delayvideo.h"
#include "filexml.h"
#include "language.h"
#include "vframe.h"




#include <string.h>






REGISTER_PLUGIN(DelayVideo)





DelayVideoConfig::DelayVideoConfig()
{
	length = 0;
}

int DelayVideoConfig::equivalent(DelayVideoConfig &that)
{
	return EQUIV(length, that.length);
}

void DelayVideoConfig::copy_from(DelayVideoConfig &that)
{
	length = that.length;
}

void DelayVideoConfig::interpolate(DelayVideoConfig &prev,
		DelayVideoConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame)
{
	this->length = prev.length;
}



DelayVideoWindow::DelayVideoWindow(DelayVideo *plugin)
 : PluginClientWindow(plugin,
	210,
	120,
	210,
	120,
	0)
{
	this->plugin = plugin;
}

DelayVideoWindow::~DelayVideoWindow()
{
}


void DelayVideoWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, _("Delay seconds:")));
	y += 20;
	slider = new DelayVideoSlider(this, plugin, x, y);
	slider->create_objects();
	show_window();
}


void DelayVideoWindow::update_gui()
{
	char string[BCTEXTLEN];
	sprintf(string, "%.04f", plugin->config.length);
	slider->update(string);
}












DelayVideoSlider::DelayVideoSlider(DelayVideoWindow *window,
	DelayVideo *plugin,
	int x,
	int y)
 : BC_TumbleTextBox(window,
 	(float)plugin->config.length,
	(float)0,
	(float)10,
	x,
	y,
	150)
{
	this->plugin = plugin;
	set_increment(0.1);
}

int DelayVideoSlider::handle_event()
{
	plugin->config.length = atof(get_text());
	plugin->send_configure_change();
	return 1;
}

















DelayVideo::DelayVideo(PluginServer *server)
 : PluginVClient(server)
{
	reset();
}

DelayVideo::~DelayVideo()
{
	if(buffer)
	{
//printf("DelayVideo::~DelayVideo 1\n");
		for(int i = 0; i < allocation; i++)
			delete buffer[i];
//printf("DelayVideo::~DelayVideo 1\n");

		delete [] buffer;
//printf("DelayVideo::~DelayVideo 1\n");
	}
}

void DelayVideo::reset()
{
	thread = 0;
	need_reconfigure = 1;
	buffer = 0;
	allocation = 0;
}

void DelayVideo::reconfigure()
{
	int new_allocation = 1 + (int)(config.length * PluginVClient::project_frame_rate);
	VFrame **new_buffer = new VFrame*[new_allocation];
	int reuse = MIN(new_allocation, allocation);

	for(int i = 0; i < reuse; i++)
	{
		new_buffer[i] = buffer[i];
	}

	for(int i = reuse; i < new_allocation; i++)
	{
		new_buffer[i] = new VFrame(input->get_w(), input->get_h(),
			PluginVClient::project_color_model, 0);
	}

	for(int i = reuse; i < allocation; i++)
	{
		delete buffer[i];
	}

	if(buffer) delete [] buffer;


	buffer = new_buffer;
	allocation = new_allocation;

	need_reconfigure = 0;
}

int DelayVideo::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
//printf("DelayVideo::process_realtime 1 %d\n", config.length);
	this->input = input_ptr;
	this->output = output_ptr;
	need_reconfigure += load_configuration();
	CLAMP(config.length, 0, 10);

//printf("DelayVideo::process_realtime 2 %d\n", config.length);
	if(need_reconfigure) reconfigure();
//printf("DelayVideo::process_realtime 3 %d %d\n", config.length, allocation);

	buffer[allocation - 1]->copy_from(input_ptr);
	output_ptr->copy_from(buffer[0]);

	VFrame *temp = buffer[0];
	for(int i = 0; i < allocation - 1; i++)
	{
		buffer[i] = buffer[i + 1];
	}

	buffer[allocation - 1] = temp;
//printf("DelayVideo::process_realtime 4\n");


	return 0;
}

int DelayVideo::is_realtime()
{
	return 1;
}

const char* DelayVideo::plugin_title() { return N_("Delay Video"); }

LOAD_CONFIGURATION_MACRO(DelayVideo, DelayVideoConfig)
NEW_WINDOW_MACRO(DelayVideo, DelayVideoWindow)


void DelayVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("DELAYVIDEO");
	output.tag.set_property("LENGTH", (double)config.length);
	output.append_tag();
	output.tag.set_title("/DELAYVIDEO");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void DelayVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("DELAYVIDEO"))
			{
				config.length = input.tag.get_property("LENGTH", (double)config.length);
			}
		}
	}
}

void DelayVideo::update_gui()
{
	if(thread)
	{
		load_configuration();
		((DelayVideoWindow*)thread->window)->lock_window();
		((DelayVideoWindow*)thread->window)->slider->update(config.length);
		((DelayVideoWindow*)thread->window)->unlock_window();
	}
}







