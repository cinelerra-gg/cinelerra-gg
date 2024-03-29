
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
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "pluginaclient.h"
#include "samples.h"
#include "transportque.h"

#include <string.h>

class ReverseAudio;

class ReverseAudioConfig
{
public:
	ReverseAudioConfig();
	int enabled;
};


class ReverseAudioEnabled : public BC_CheckBox
{
public:
	ReverseAudioEnabled(ReverseAudio *plugin,
		int x,
		int y);
	int handle_event();
	ReverseAudio *plugin;
};

class ReverseAudioWindow : public PluginClientWindow
{
public:
	ReverseAudioWindow(ReverseAudio *plugin);
	~ReverseAudioWindow();
	void create_objects();

	ReverseAudio *plugin;
	ReverseAudioEnabled *enabled;
};



class ReverseAudio : public PluginAClient
{
public:
	ReverseAudio(PluginServer *server);
	~ReverseAudio();

	PLUGIN_CLASS_MEMBERS(ReverseAudioConfig)

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_realtime();
	int process_buffer(int64_t size,
		Samples *buffer,
		int64_t start_position,
		int sample_rate);

	int64_t input_position;
	int fragment_size;
};







REGISTER_PLUGIN(ReverseAudio);



ReverseAudioConfig::ReverseAudioConfig()
{
	enabled = 1;
}





ReverseAudioWindow::ReverseAudioWindow(ReverseAudio *plugin)
 : PluginClientWindow(plugin, 265, 60, 265, 60, 0)
{
	this->plugin = plugin;
}

ReverseAudioWindow::~ReverseAudioWindow()
{
}

void ReverseAudioWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(enabled = new ReverseAudioEnabled(plugin,
		x,
		y));
	show_window();
	flush();
}






ReverseAudioEnabled::ReverseAudioEnabled(ReverseAudio *plugin,
	int x,
	int y)
 : BC_CheckBox(x,
	y,
	plugin->config.enabled,
	_("Enabled"))
{
	this->plugin = plugin;
}

int ReverseAudioEnabled::handle_event()
{
	plugin->config.enabled = get_value();
	plugin->send_configure_change();
	return 1;
}









ReverseAudio::ReverseAudio(PluginServer *server)
 : PluginAClient(server)
{

}


ReverseAudio::~ReverseAudio()
{

}

const char* ReverseAudio::plugin_title() { return N_("Reverse audio"); }
int ReverseAudio::is_realtime() { return 1; }


NEW_WINDOW_MACRO(ReverseAudio, ReverseAudioWindow)


int ReverseAudio::process_buffer(int64_t size,
	Samples *buffer,
	int64_t start_position,
	int sample_rate)
{
	for(int i = 0; i < size; i += fragment_size)
	{
		fragment_size = size - i;
		load_configuration();
		if(config.enabled)
		{
			int offset = buffer->get_offset();
			buffer->set_offset(offset + i);
			read_samples(buffer,
				0,
				sample_rate,
				input_position,
				fragment_size);
			buffer->set_offset(offset);
			double *buffer_samples = buffer->get_data();

			for(int start = i, end = i + fragment_size - 1;
				end > start;
				start++, end--)
			{
				double temp = buffer_samples[start];
				buffer_samples[start] = buffer_samples[end];
				buffer_samples[end] = temp;
			}
		}
		else
		{
			int offset = buffer->get_offset();
			buffer->set_offset(offset + i);
			read_samples(buffer,
				0,
				sample_rate,
				start_position,
				fragment_size);
			buffer->set_offset(offset);
		}

		if(get_direction() == PLAY_FORWARD)
			start_position += fragment_size;
		else
			start_position -= fragment_size;
	}


	return 0;
}




int ReverseAudio::load_configuration()
{
	KeyFrame *prev_keyframe, *next_keyframe;
	next_keyframe = get_next_keyframe(get_source_position());
	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(next_keyframe);
// Previous keyframe stays in config object.
	read_data(prev_keyframe);

	int64_t prev_position = edl_to_local(prev_keyframe->position);
	int64_t next_position = edl_to_local(next_keyframe->position);

// printf("ReverseAudio::load_configuration 1 %lld %lld %lld %lld\n",
// prev_position,
// next_position,
// prev_keyframe->position,
// next_keyframe->position);
// Defeat default keyframe
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
// Truncate next buffer to keyframe
		if(range_end - get_source_position() < fragment_size)
			fragment_size = range_end - get_source_position();
		input_position = get_source_position() - range_start;
		input_position = range_end - input_position - fragment_size;
	}
	else
	{
		if(get_source_position() - range_start < fragment_size)
			fragment_size = get_source_position() - range_start;
		input_position = range_end - get_source_position();
		input_position = range_start + input_position + fragment_size;
	}
// printf("ReverseAudio::load_configuration 20 start=%lld end=%lld current=%lld input=%lld\n",
// range_start,
// range_end,
// get_source_position(),
// input_position);

	return 0;
}


void ReverseAudio::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("REVERSEAUDIO");
	output.tag.set_property("ENABLED", config.enabled);
	output.append_tag();
	output.tag.set_title("/REVERSEAUDIO");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void ReverseAudio::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	while(!input.read_tag())
	{
		if(input.tag.title_is("REVERSEAUDIO"))
		{
			config.enabled = input.tag.get_property("ENABLED", config.enabled);
		}
	}
}

void ReverseAudio::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		((ReverseAudioWindow*)thread->window)->enabled->update(config.enabled);
		thread->window->unlock_window();
	}
}





