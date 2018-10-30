
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
#include "bchash.h"
#include "bcsignals.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "pluginvclient.h"
#include "transportque.h"

#include <string.h>

class ReverseVideo;

class ReverseVideoConfig
{
public:
	ReverseVideoConfig();
	int enabled;
};


class ReverseVideoEnabled : public BC_CheckBox
{
public:
	ReverseVideoEnabled(ReverseVideo *plugin,
		int x,
		int y);
	int handle_event();
	ReverseVideo *plugin;
};

class ReverseVideoWindow : public PluginClientWindow
{
public:
	ReverseVideoWindow(ReverseVideo *plugin);
	~ReverseVideoWindow();
	void create_objects();

	ReverseVideo *plugin;
	ReverseVideoEnabled *enabled;
};


class ReverseVideo : public PluginVClient
{
public:
	ReverseVideo(PluginServer *server);
	~ReverseVideo();

	PLUGIN_CLASS_MEMBERS(ReverseVideoConfig)

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_realtime();
	int process_buffer(VFrame *frame,
			int64_t start_position,
			double frame_rate);

	int64_t input_position;
};







REGISTER_PLUGIN(ReverseVideo);



ReverseVideoConfig::ReverseVideoConfig()
{
	enabled = 1;
}





ReverseVideoWindow::ReverseVideoWindow(ReverseVideo *plugin)
 : PluginClientWindow(plugin,
	210,
	160,
	200,
	160,
	0)
{
	this->plugin = plugin;
}

ReverseVideoWindow::~ReverseVideoWindow()
{
}

void ReverseVideoWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(enabled = new ReverseVideoEnabled(plugin,
		x,
		y));
	show_window();
	flush();
}












ReverseVideoEnabled::ReverseVideoEnabled(ReverseVideo *plugin,
	int x,
	int y)
 : BC_CheckBox(x,
	y,
	plugin->config.enabled,
	_("Enabled"))
{
	this->plugin = plugin;
}

int ReverseVideoEnabled::handle_event()
{
	plugin->config.enabled = get_value();
	plugin->send_configure_change();
	return 1;
}









ReverseVideo::ReverseVideo(PluginServer *server)
 : PluginVClient(server)
{

}


ReverseVideo::~ReverseVideo()
{

}

const char* ReverseVideo::plugin_title() { return N_("Reverse video"); }
int ReverseVideo::is_realtime() { return 1; }


NEW_WINDOW_MACRO(ReverseVideo, ReverseVideoWindow)


int ReverseVideo::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	load_configuration();

	if(config.enabled)
		read_frame(frame,
			0,
			input_position,
			frame_rate,
			0);
	else
		read_frame(frame,
			0,
			start_position,
			frame_rate,
			0);
	return 0;
}




int ReverseVideo::load_configuration()
{
	KeyFrame *prev_keyframe, *next_keyframe;
	next_keyframe = get_next_keyframe(get_source_position());
	prev_keyframe = get_prev_keyframe(get_source_position());
// Previous keyframe stays in config object.
	read_data(prev_keyframe);

	int64_t prev_position = edl_to_local(prev_keyframe->position);
	int64_t next_position = edl_to_local(next_keyframe->position);

	if(prev_position == 0 && next_position == 0)
	{
		next_position = prev_position = get_source_start();
	}

// Get range to flip in requested rate
	int64_t range_start = prev_position;
	int64_t range_end = next_position;

// Between keyframe and edge of range or no keyframes
	if(range_start == range_end)
	{
// Between first keyframe and start of effect
		if(get_source_position() >= get_source_start() &&
			get_source_position() < range_start)
		{
			range_start = get_source_start();
		}
		else
// Between last keyframe and end of effect
		if(get_source_position() >= range_start &&
			get_source_position() < get_source_start() + get_total_len())
		{
			range_end = get_source_start() + get_total_len();
		}
		else
		{
// Should never get here
			;
		}
	}


// Convert start position to new direction
	if(get_direction() == PLAY_FORWARD)
	{
//printf("ReverseVideo::load_configuration %d %ld %ld %ld %ld %ld\n",
//	__LINE__,
//	get_source_position(),
//	get_source_start(),
//	get_total_len(),
//	range_start,
//	range_end);
		input_position = get_source_position() - range_start;
		input_position = range_end - input_position - 1;
	}
	else
	{
		input_position = range_end - get_source_position();
		input_position = range_start + input_position + 1;
	}
// printf("ReverseVideo::load_configuration 2 start=%lld end=%lld current=%lld input=%lld\n",
// range_start,
// range_end,
// get_source_position(),
// input_position);

	return 0;
}


void ReverseVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("REVERSEVIDEO");
	output.tag.set_property("ENABLED", config.enabled);
	output.append_tag();
	output.tag.set_title("/REVERSEVIDEO");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void ReverseVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	while(!input.read_tag())
	{
		if(input.tag.title_is("REVERSEVIDEO"))
		{
			config.enabled = input.tag.get_property("ENABLED", config.enabled);
		}
	}
}

void ReverseVideo::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		((ReverseVideoWindow*)thread->window)->enabled->update(config.enabled);
		thread->window->unlock_window();
	}
}





