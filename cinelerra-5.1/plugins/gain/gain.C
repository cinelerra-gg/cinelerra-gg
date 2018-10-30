
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

#include "clip.h"
#include "confirmsave.h"
#include "bchash.h"
#include "errorbox.h"
#include "filexml.h"
#include "language.h"
#include "gain.h"
#include "gainwindow.h"
#include "samples.h"

#include "vframe.h"

#include <string.h>


REGISTER_PLUGIN(Gain)


GainConfig::GainConfig()
{
	level = 0.0;
}

int GainConfig::equivalent(GainConfig &that)
{
	return EQUIV(level, that.level);
}

void GainConfig::copy_from(GainConfig &that)
{
	this->level = that.level;
}

void GainConfig::interpolate(GainConfig &prev,
	GainConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	level = prev.level * prev_scale + next.level * next_scale;
}









Gain::Gain(PluginServer *server)
 : PluginAClient(server)
{

}

Gain::~Gain()
{

}

const char* Gain::plugin_title() { return N_("Gain"); }
int Gain::is_realtime() { return 1; }


NEW_WINDOW_MACRO(Gain, GainWindow)
LOAD_CONFIGURATION_MACRO(Gain, GainConfig)

int Gain::process_realtime(int64_t size, Samples *input_ptr, Samples *output_ptr)
{
	load_configuration();

	double gain = db.fromdb(config.level);
	double *output_samples = output_ptr->get_data();
	double *input_samples = input_ptr->get_data();

	for(int64_t i = 0; i < size; i++)
	{
		output_samples[i] = input_samples[i] * gain;
	}

	return 0;
}




void Gain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause xml file to store data directly in text
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("GAIN");
	output.tag.set_property("LEVEL", config.level);
	output.append_tag();
	output.tag.set_title("/GAIN");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void Gain::read_data(KeyFrame *keyframe)
{
	FileXML input;
// cause xml file to read directly from text
	input.set_shared_input(keyframe->xbuf);
	int result = 0;

	result = input.read_tag();

	if(!result)
	{
		if(input.tag.title_is("GAIN"))
		{
			config.level = input.tag.get_property("LEVEL", config.level);
		}
	}
}

void Gain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		((GainWindow*)thread->window)->level->update(config.level);
		thread->window->unlock_window();
	}
}


