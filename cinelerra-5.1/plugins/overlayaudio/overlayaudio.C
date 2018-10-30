
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
#include "language.h"
#include "pluginaclient.h"
#include "samples.h"
#include "theme.h"
#include <string.h>


class OverlayAudioWindow;
class OverlayAudio;

#define AUDIO_TRANSFER_TYPES 2


class OverlayAudioConfig
{
public:
	OverlayAudioConfig();
	int equivalent(OverlayAudioConfig &that);
	void copy_from(OverlayAudioConfig &that);
	void interpolate(OverlayAudioConfig &prev,
		OverlayAudioConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
	static const char* output_to_text(int output_layer);
	static const char* mode_to_text(int mode);
	int output_track;
	enum
	{
		TOP,
		BOTTOM
	};


	int mode;
	enum
	{
		ADD,
		MULTIPLY
	};
};

class OutputTrack : public BC_PopupMenu
{
public:
	OutputTrack(OverlayAudio *plugin, int x, int y);
	void create_objects();
	int handle_event();
	OverlayAudio *plugin;
};


class OverlayMode : public BC_PopupMenu
{
public:
	OverlayMode(OverlayAudio *plugin,
		int x,
		int y);
	void create_objects();
	int handle_event();
	OverlayAudio *plugin;
};

class OverlayAudioWindow : public PluginClientWindow
{
public:
	OverlayAudioWindow(OverlayAudio *plugin);

	void create_objects();


	OverlayAudio *plugin;
	OutputTrack *output;
	OverlayMode *mode;
};



class OverlayAudio : public PluginAClient
{
public:
	OverlayAudio(PluginServer *server);
	~OverlayAudio();

	int is_multichannel();
	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_buffer(int64_t size,
		Samples **buffer,
		int64_t start_position,
		int sample_rate);
	void update_gui();


	PLUGIN_CLASS_MEMBERS2(OverlayAudioConfig)
};








OverlayAudioConfig::OverlayAudioConfig()
{
	output_track = OverlayAudioConfig::TOP;
	mode = OverlayAudioConfig::ADD;

}

int OverlayAudioConfig::equivalent(OverlayAudioConfig &that)
{
	return that.output_track == output_track &&
		that.mode == mode;
}

void OverlayAudioConfig::copy_from(OverlayAudioConfig &that)
{
	output_track = that.output_track;
	mode = that.mode;
}

void OverlayAudioConfig::interpolate(OverlayAudioConfig &prev,
	OverlayAudioConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	output_track = prev.output_track;
	mode = prev.mode;
}

const char* OverlayAudioConfig::output_to_text(int output_layer)
{
	switch(output_layer)
	{
		case OverlayAudioConfig::TOP:    return _("Top");
		case OverlayAudioConfig::BOTTOM: return _("Bottom");
	}
	return "";
}


const char* OverlayAudioConfig::mode_to_text(int mode)
{
	switch(mode)
	{
		case OverlayAudioConfig::ADD:    return _("Add");
		case OverlayAudioConfig::MULTIPLY: return _("Multiply");
	}
	return "";
}







OverlayAudioWindow::OverlayAudioWindow(OverlayAudio *plugin)
 : PluginClientWindow(plugin,
	400,
	100,
	400,
	100,
	0)
{
	this->plugin = plugin;
}

void OverlayAudioWindow::create_objects()
{
	int x = 10, y = 10;
	int x1 = x;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Output track:")));
	x += title->get_w() + plugin->get_theme()->widget_border;
	add_subwindow(output = new OutputTrack(plugin, x, y));
	output->create_objects();

	y += output->get_h() + plugin->get_theme()->widget_border;
	x = x1;
	add_subwindow(title = new BC_Title(x, y, _("Mode:")));
	x += title->get_w() + plugin->get_theme()->widget_border;
	add_subwindow(mode = new OverlayMode(plugin, x, y));
	mode->create_objects();


	show_window();
}






OutputTrack::OutputTrack(OverlayAudio *plugin, int x , int y)
 : BC_PopupMenu(x,
 	y,
	100,
	OverlayAudioConfig::output_to_text(plugin->config.output_track),
	1)
{
	this->plugin = plugin;
}

void OutputTrack::create_objects()
{
	add_item(new BC_MenuItem(
		OverlayAudioConfig::output_to_text(
			OverlayAudioConfig::TOP)));
	add_item(new BC_MenuItem(
		OverlayAudioConfig::output_to_text(
			OverlayAudioConfig::BOTTOM)));
}

int OutputTrack::handle_event()
{
	char *text = get_text();

	if(!strcmp(text,
		OverlayAudioConfig::output_to_text(
			OverlayAudioConfig::TOP)))
		plugin->config.output_track = OverlayAudioConfig::TOP;
	else
	if(!strcmp(text,
		OverlayAudioConfig::output_to_text(
			OverlayAudioConfig::BOTTOM)))
		plugin->config.output_track = OverlayAudioConfig::BOTTOM;

	plugin->send_configure_change();
	return 1;
}












OverlayMode::OverlayMode(OverlayAudio *plugin,
	int x,
	int y)
 : BC_PopupMenu(x,
 	y,
	150,
	OverlayAudioConfig::mode_to_text(plugin->config.mode),
	1)
{
	this->plugin = plugin;
}

void OverlayMode::create_objects()
{
	for(int i = 0; i < AUDIO_TRANSFER_TYPES; i++)
		add_item(new BC_MenuItem(OverlayAudioConfig::mode_to_text(i)));
}

int OverlayMode::handle_event()
{
	char *text = get_text();

	for(int i = 0; i < AUDIO_TRANSFER_TYPES; i++)
	{
		if(!strcmp(text, OverlayAudioConfig::mode_to_text(i)))
		{
			plugin->config.mode = i;
			break;
		}
	}

	plugin->send_configure_change();
	return 1;
}









REGISTER_PLUGIN(OverlayAudio)




OverlayAudio::OverlayAudio(PluginServer *server)
 : PluginAClient(server)
{

}

OverlayAudio::~OverlayAudio()
{

}

const char* OverlayAudio::plugin_title() { return N_("Overlay"); }
int OverlayAudio::is_realtime() { return 1; }
int OverlayAudio::is_multichannel() { return 1; }



void OverlayAudio::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("OVERLAY"))
			{
				config.output_track = input.tag.get_property("OUTPUT", config.output_track);
				config.mode = input.tag.get_property("MODE", config.mode);
			}
		}
	}
}

void OverlayAudio::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("OVERLAY");
	output.tag.set_property("OUTPUT", config.output_track);
	output.tag.set_property("MODE", config.mode);
	output.append_tag();
	output.tag.set_title("/OVERLAY");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}



void OverlayAudio::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("OverlayAudio::update_gui");
			((OverlayAudioWindow*)thread->window)->output->set_text(
				OverlayAudioConfig::output_to_text(config.output_track));
			((OverlayAudioWindow*)thread->window)->mode->set_text(
				OverlayAudioConfig::mode_to_text(config.mode));
			thread->window->unlock_window();
		}
	}
}

NEW_WINDOW_MACRO(OverlayAudio, OverlayAudioWindow)
LOAD_CONFIGURATION_MACRO(OverlayAudio, OverlayAudioConfig)


int OverlayAudio::process_buffer(int64_t size,
	Samples **buffer,
	int64_t start_position,
	int sample_rate)
{
	load_configuration();


	int output_track = 0;
	if(config.output_track == OverlayAudioConfig::BOTTOM)
		output_track = get_total_buffers() - 1;

// Direct copy the output track
	read_samples(buffer[output_track],
		output_track,
		sample_rate,
		start_position,
		size);

// Add remaining tracks
	Samples *output_buffer = buffer[output_track];
	double *output_samples = output_buffer->get_data();
	for(int i = 0; i < get_total_buffers(); i++)
	{
		if(i != output_track)
		{
			Samples *input_buffer = buffer[i];
			read_samples(buffer[i],
				i,
				sample_rate,
				start_position,
				size);
			double *input_samples = input_buffer->get_data();

			switch(config.mode)
			{
				case OverlayAudioConfig::ADD:
					for(int j = 0; j < size; j++)
					{
						output_samples[j] += input_samples[j];
					}
					break;


				case OverlayAudioConfig::MULTIPLY:
					for(int j = 0; j < size; j++)
					{
						output_samples[j] *= input_samples[j];
					}
					break;
			}
		}
	}

	return 0;
}






