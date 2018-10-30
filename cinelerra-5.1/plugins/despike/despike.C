
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
#include "despike.h"
#include "despikewindow.h"
#include "samples.h"

#include "vframe.h"

#include <string.h>



REGISTER_PLUGIN(Despike)



Despike::Despike(PluginServer *server)
 : PluginAClient(server)
{

	last_sample = 0;
}

Despike::~Despike()
{

}

const char* Despike::plugin_title() { return N_("Despike"); }
int Despike::is_realtime() { return 1; }

NEW_WINDOW_MACRO(Despike, DespikeWindow)

LOAD_CONFIGURATION_MACRO(Despike, DespikeConfig)


int Despike::process_realtime(int64_t size, Samples *input_ptr, Samples *output_ptr)
{
	load_configuration();

	double threshold = db.fromdb(config.level);
	double change = db.fromdb(config.slope);

//printf("Despike::process_realtime 1\n");
	double *output_samples = output_ptr->get_data();
	double *input_samples = input_ptr->get_data();
	for(int64_t i = 0; i < size; i++)
	{
		if(fabs(input_samples[i]) > threshold ||
			fabs(input_samples[i]) - fabs(last_sample) > change)
		{
			output_samples[i] = last_sample;
		}
		else
		{
			output_samples[i] = input_samples[i];
			last_sample = input_samples[i];
		}
	}
//printf("Despike::process_realtime 2\n");

	return 0;
}



void Despike::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause xml file to store data directly in text
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("DESPIKE");
	output.tag.set_property("LEVEL", config.level);
	output.tag.set_property("SLOPE", config.slope);
	output.append_tag();
	output.tag.set_title("/DESPIKE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void Despike::read_data(KeyFrame *keyframe)
{
	FileXML input;
// cause xml file to read directly from text
	input.set_shared_input(keyframe->xbuf);
	int result = 0;

	result = input.read_tag();

	if(!result)
	{
		if(input.tag.title_is("DESPIKE"))
		{
			config.level = input.tag.get_property("LEVEL", config.level);
			config.slope = input.tag.get_property("SLOPE", config.slope);
		}
	}
}

void Despike::update_gui()
{
	if(thread)
	{
		load_configuration();
		((DespikeWindow*)thread->window)->lock_window();
		((DespikeWindow*)thread->window)->level->update(config.level);
		((DespikeWindow*)thread->window)->slope->update(config.slope);
		((DespikeWindow*)thread->window)->unlock_window();
	}
}
















DespikeConfig::DespikeConfig()
{
	level = 0;
	slope = 0;
}

int DespikeConfig::equivalent(DespikeConfig &that)
{
	return EQUIV(level, that.level) &&
		EQUIV(slope, that.slope);
}

void DespikeConfig::copy_from(DespikeConfig &that)
{
	level = that.level;
	slope = that.slope;
}

void DespikeConfig::interpolate(DespikeConfig &prev,
		DespikeConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->level = prev.level * prev_scale + next.level * next_scale;
	this->slope = prev.slope * prev_scale + next.slope * next_scale;
}






