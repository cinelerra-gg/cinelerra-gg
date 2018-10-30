
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
#include "pluginaclient.h"
#include "samples.h"
#include "transportque.h"

#include <string.h>

class LoopAudio;

class LoopAudioConfig
{
public:
	LoopAudioConfig();
	int64_t samples;
};


class LoopAudioSamples : public BC_TextBox
{
public:
	LoopAudioSamples(LoopAudio *plugin,
		int x,
		int y);
	int handle_event();
	LoopAudio *plugin;
};

class LoopAudioWindow : public PluginClientWindow
{
public:
	LoopAudioWindow(LoopAudio *plugin);
	~LoopAudioWindow();
	void create_objects();
	LoopAudio *plugin;
	LoopAudioSamples *samples;
};



class LoopAudio : public PluginAClient
{
public:
	LoopAudio(PluginServer *server);
	~LoopAudio();

	PLUGIN_CLASS_MEMBERS(LoopAudioConfig)

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_realtime();
	int is_synthesis();
	int process_buffer(int64_t size,
		Samples *buffer,
		int64_t start_position,
		int sample_rate);
};







REGISTER_PLUGIN(LoopAudio);



LoopAudioConfig::LoopAudioConfig()
{
	samples = 48000;
}





LoopAudioWindow::LoopAudioWindow(LoopAudio *plugin)
 : PluginClientWindow(plugin,
	210,
	160,
	200,
	160,
	0)
{
	this->plugin = plugin;
}

LoopAudioWindow::~LoopAudioWindow()
{
}

void LoopAudioWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, _("Samples to loop:")));
	y += 20;
	add_subwindow(samples = new LoopAudioSamples(plugin,
		x,
		y));
	show_window();
	flush();
}









LoopAudioSamples::LoopAudioSamples(LoopAudio *plugin,
	int x,
	int y)
 : BC_TextBox(x,
	y,
	100,
	1,
	plugin->config.samples)
{
	this->plugin = plugin;
	set_precision(2);
}

int LoopAudioSamples::handle_event()
{
	plugin->config.samples = atol(get_text());
	plugin->config.samples = MAX(1, plugin->config.samples);
	plugin->send_configure_change();
	return 1;
}









LoopAudio::LoopAudio(PluginServer *server)
 : PluginAClient(server)
{

}


LoopAudio::~LoopAudio()
{

}

const char* LoopAudio::plugin_title() { return N_("Loop audio"); }
int LoopAudio::is_realtime() { return 1; }
int LoopAudio::is_synthesis() { return 1; }



NEW_WINDOW_MACRO(LoopAudio, LoopAudioWindow)


int LoopAudio::process_buffer(int64_t size,
	Samples *buffer,
	int64_t start_position,
	int sample_rate)
{
	int64_t current_position = start_position;
	int step = (get_direction() == PLAY_FORWARD) ? 1 : -1;
	int fragment_size;
	int64_t current_loop_end;

//printf("LoopAudio::process_buffer 1 %lld %d\n", start_position, size);

	current_position = start_position;

	for(int i = 0; i < size; i += fragment_size)
	{
// Truncate to end of buffer
		fragment_size = MIN(size - i, size);

		int64_t current_loop_position;

// Truncate to next keyframe
		if(get_direction() == PLAY_FORWARD)
		{
			KeyFrame *next_keyframe = get_next_keyframe(current_position);
			int64_t next_position = edl_to_local(next_keyframe->position);
			if(next_position > current_position)
				fragment_size = MIN(fragment_size, next_position - current_position);

// Get start of current loop
			KeyFrame *prev_keyframe = get_prev_keyframe(current_position);
			int64_t prev_position = edl_to_local(prev_keyframe->position);
			if(prev_position == 0)
				prev_position = get_source_start();
			read_data(prev_keyframe);

// Get start of fragment in current loop
			current_loop_position = prev_position +
				((current_position - prev_position) %
					config.samples);
			while(current_loop_position < prev_position) current_loop_position += config.samples;
			while(current_loop_position >= prev_position + config.samples) current_loop_position -= config.samples;

// Truncate fragment to end of loop
			current_loop_end = current_position -
				current_loop_position +
				prev_position +
				config.samples;
			fragment_size = MIN(current_loop_end - current_position,
				fragment_size);
		}
		else
		{
			KeyFrame *next_keyframe = get_prev_keyframe(current_position);
			int64_t next_position = edl_to_local(next_keyframe->position);
			if(next_position < current_position)
				fragment_size = MIN(fragment_size, current_position - next_position);

			KeyFrame *prev_keyframe = get_next_keyframe(current_position);
			int64_t prev_position = edl_to_local(prev_keyframe->position);
			if(prev_position == 0)
				prev_position = get_source_start() + get_total_len();
			read_data(prev_keyframe);

			current_loop_position = prev_position -
				((prev_position - current_position) %
					  config.samples);
			while(current_loop_position <= prev_position - config.samples) current_loop_position += config.samples;
			while(current_loop_position > prev_position) current_loop_position -= config.samples;

// Truncate fragment to end of loop
			current_loop_end = current_position +
				prev_position -
				current_loop_position -
				config.samples;
			fragment_size = MIN(current_position - current_loop_end,
				fragment_size);
		}


// printf("LoopAudio::process_buffer 100 %lld %lld %lld %d\n",
// current_position, current_loop_position, current_loop_end, fragment_size);
		int offset = buffer->get_offset();
		buffer->set_offset(offset + i);
		read_samples(buffer,
			0,
			sample_rate,
			current_loop_position,
			fragment_size);
		buffer->set_offset(offset);


		current_position += step * fragment_size;
	}


	return 0;
}




int LoopAudio::load_configuration()
{
	KeyFrame *prev_keyframe;
	int64_t old_samples = config.samples;
	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);
	return old_samples != config.samples;
}


void LoopAudio::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("LOOPAUDIO");
	output.tag.set_property("SAMPLES", config.samples);
	output.append_tag();
	output.tag.set_title("/LOOPAUDIO");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void LoopAudio::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	while(!input.read_tag())
	{
		if(input.tag.title_is("LOOPAUDIO"))
		{
			config.samples = input.tag.get_property("SAMPLES", config.samples);
		}
	}
}

void LoopAudio::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		((LoopAudioWindow*)thread->window)->samples->update(config.samples);
		thread->window->unlock_window();
	}
}





