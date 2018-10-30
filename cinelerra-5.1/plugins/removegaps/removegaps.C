
/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "removegaps.h"
#include "samples.h"
#include "theme.h"
#include "transportque.inc"

#include <string.h>


// Maximum seconds to skip, if end of file
#define MAX_SKIPPED_DURATION 5.0
#define MIN_GAP_DURATION 0.01
#define MAX_GAP_DURATION 1.0
#define MIN_GAP_THRESHOLD INFINITYGAIN
#define MAX_GAP_THRESHOLD 0.0

REGISTER_PLUGIN(RemoveGaps);



RemoveGapsConfig::RemoveGapsConfig()
{
	threshold = -20.0;
	duration = 0.1;
}


int RemoveGapsConfig::equivalent(RemoveGapsConfig &src)
{
	return EQUIV(threshold, src.threshold) &&
		EQUIV(duration, src.duration);
}

void RemoveGapsConfig::copy_from(RemoveGapsConfig &src)
{
	this->threshold = src.threshold;
	this->duration = src.duration;
}

void RemoveGapsConfig::interpolate(RemoveGapsConfig &prev,
	RemoveGapsConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	this->threshold = prev.threshold;
	this->duration = prev.duration;
	boundaries();
}

void RemoveGapsConfig::boundaries()
{
	CLAMP(threshold, MIN_GAP_THRESHOLD, MAX_GAP_THRESHOLD);
	CLAMP(duration, MIN_GAP_DURATION, MAX_GAP_DURATION);
}




RemoveGapsWindow::RemoveGapsWindow(RemoveGaps *plugin)
 : PluginClientWindow(plugin,
	320,
	160,
	320,
	160,
	0)
{
	this->plugin = plugin;
}

RemoveGapsWindow::~RemoveGapsWindow()
{
}

void RemoveGapsWindow::create_objects()
{
	int x = 10, y = 10;
	BC_Title *title;

	add_subwindow(title = new BC_Title(x, y, _("Threshold of gap (DB):")));

	add_subwindow(threshold = new RemoveGapsThreshold(this,
		plugin,
		x + title->get_w() + plugin->get_theme()->widget_border,
		y));
	y += threshold->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(title = new BC_Title(x, y, _("Max duration of gap (Seconds):")));
	add_subwindow(duration = new RemoveGapsDuration(this,
		plugin,
		x + title->get_w() + plugin->get_theme()->widget_border,
		y));
	show_window(1);
}






RemoveGapsThreshold::RemoveGapsThreshold(RemoveGapsWindow *window,
	RemoveGaps *plugin,
	int x,
	int y)
 : BC_FPot(x,
 	y,
 	plugin->config.threshold,
	MIN_GAP_THRESHOLD,
	MAX_GAP_THRESHOLD)
{
	this->plugin = plugin;
	set_precision(0.1);
}

int RemoveGapsThreshold::handle_event()
{
	plugin->config.threshold = get_value();
	plugin->send_configure_change();
	return 1;
}








RemoveGapsDuration::RemoveGapsDuration(RemoveGapsWindow *window,
	RemoveGaps *plugin,
	int x,
	int y)
 : BC_FPot(x,
 	y,
 	plugin->config.duration,
	MIN_GAP_DURATION,
	MAX_GAP_DURATION)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int RemoveGapsDuration::handle_event()
{
	plugin->config.duration = get_value();
	plugin->send_configure_change();
	return 1;
}







RemoveGaps::RemoveGaps(PluginServer *server)
 : PluginAClient(server)
{
	need_reconfigure = 1;
	source_start = 0;
	dest_start = 0;
	gap_length = 0;
	temp_position = 0;
	temp = 0;
}


RemoveGaps::~RemoveGaps()
{
	delete temp;
}

const char* RemoveGaps::plugin_title() { return N_("Remove Gaps"); }
int RemoveGaps::is_realtime() { return 1; }

NEW_WINDOW_MACRO(RemoveGaps, RemoveGapsWindow)

LOAD_CONFIGURATION_MACRO(RemoveGaps, RemoveGapsConfig)


int RemoveGaps::process_buffer(int64_t size,
	Samples *buffer,
	int64_t start_position,
	int sample_rate)
{
	need_reconfigure |= load_configuration();


	if(need_reconfigure || start_position != dest_start)
	{
		source_start = start_position;
		temp_position = 0;
		need_reconfigure = 0;
	}

	dest_start = start_position;
	double *buffer_samples = buffer->get_data();
	double *temp_samples = !temp ? 0 : temp->get_data();

	double threshold = DB::fromdb(config.threshold);
	int64_t duration = (int64_t)(config.duration * sample_rate);
	for(int i = 0; i < size; )
	{
		if(!temp || temp_position >= temp->get_allocated())
		{
// Expand temp buffer if smaller than buffer read
			if(!temp)
			{
				temp = new Samples(get_buffer_size());
				temp_samples = temp->get_data();
			}
			temp_position = 0;

// Fill new temp buffer
			read_samples(temp,
				0,
				sample_rate,
				source_start,
				get_buffer_size());
			if(get_direction() == PLAY_FORWARD)
				source_start += size;
			else
				source_start -= size;
		}

		double sample = temp_samples[temp_position];
		if(fabs(sample) < threshold)
		{
			gap_length++;
		}
		else
		{
			gap_length = 0;
		}

// Only copy sample if gap below duration or no more samples to read
		if(gap_length < duration || gap_length > MAX_SKIPPED_DURATION * sample_rate)
		{
			buffer_samples[i++] = sample;
		}

		temp_position++;
	}


	if(get_direction() == PLAY_FORWARD)
		dest_start += size;
	else
		dest_start -= size;

	return 0;
}

void RemoveGaps::render_stop()
{
	need_reconfigure = 1;
}




void RemoveGaps::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("REMOVEGAPS");
	output.tag.set_property("DURATION", config.duration);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.append_tag();
	output.tag.set_title("/REMOVEGAPS");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void RemoveGaps::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	while(!input.read_tag())
	{
		if(input.tag.title_is("REMOVEGAPS"))
		{
			config.duration = input.tag.get_property("DURATION", config.duration);
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
		}
	}

}

void RemoveGaps::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("RemoveGaps::update_gui");
			((RemoveGapsWindow*)thread->window)->duration->update((float)config.duration);
			((RemoveGapsWindow*)thread->window)->threshold->update((float)config.threshold);
			thread->window->unlock_window();
		}
	}
}





