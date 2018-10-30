
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
#include "bcsignals.h"
#include "clip.h"
#include "compressor.h"
#include "cursors.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "samples.h"
#include "theme.h"
#include "units.h"
#include "vframe.h"

#include <math.h>
#include <string.h>





REGISTER_PLUGIN(CompressorEffect)






// More potential compressor algorithms:
// Use single reaction time parameter.  Negative reaction time uses
// readahead.  Positive reaction time uses slope.

// Smooth input stage if readahead.
// Determine slope from current smoothed sample to every sample in readahead area.
// Once highest slope is found, count of number of samples remaining until it is
// reached.  Only search after this count for the next highest slope.
// Use highest slope to determine smoothed value.

// Smooth input stage if not readahead.
// For every sample, calculate slope needed to reach current sample from
// current smoothed value in the reaction time.  If higher than current slope,
// make it the current slope and count number of samples remaining until it is
// reached.  If this count is met and no higher slopes are found, base slope
// on current sample when count is met.

// Gain stage.
// For every sample, calculate gain from smoothed input value.





CompressorEffect::CompressorEffect(PluginServer *server)
 : PluginAClient(server)
{
	reset();

}

CompressorEffect::~CompressorEffect()
{

	delete_dsp();
	levels.remove_all();
}

void CompressorEffect::delete_dsp()
{
	if(input_buffer)
	{
		for(int i = 0; i < PluginClient::total_in_buffers; i++)
			delete input_buffer[i];
		delete [] input_buffer;
	}


	input_buffer = 0;
	input_size = 0;
	input_allocated = 0;
}


void CompressorEffect::reset()
{
	input_buffer = 0;
	input_size = 0;
	input_allocated = 0;
	input_start = 0;

	next_target = 1.0;
	previous_target = 1.0;
	target_samples = 1;
	target_current_sample = -1;
	current_value = 1.0;
}

const char* CompressorEffect::plugin_title() { return N_("Compressor"); }
int CompressorEffect::is_realtime() { return 1; }
int CompressorEffect::is_multichannel() { return 1; }



void CompressorEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	config.levels.remove_all();
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("COMPRESSOR"))
			{
				config.reaction_len = input.tag.get_property("REACTION_LEN", config.reaction_len);
				config.decay_len = input.tag.get_property("DECAY_LEN", config.decay_len);
				config.trigger = input.tag.get_property("TRIGGER", config.trigger);
				config.smoothing_only = input.tag.get_property("SMOOTHING_ONLY", config.smoothing_only);
				config.input = input.tag.get_property("INPUT", config.input);
			}
			else
			if(input.tag.title_is("LEVEL"))
			{
				double x = input.tag.get_property("X", (double)0);
				double y = input.tag.get_property("Y", (double)0);
				compressor_point_t point = { x, y };

				config.levels.append(point);
			}
		}
	}
}

void CompressorEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("COMPRESSOR");
	output.tag.set_property("TRIGGER", config.trigger);
	output.tag.set_property("REACTION_LEN", config.reaction_len);
	output.tag.set_property("DECAY_LEN", config.decay_len);
	output.tag.set_property("SMOOTHING_ONLY", config.smoothing_only);
	output.tag.set_property("INPUT", config.input);
	output.append_tag();
	output.tag.set_title("/COMPRESSOR");
	output.append_tag();
	output.append_newline();


	for(int i = 0; i < config.levels.total; i++)
	{
		output.tag.set_title("LEVEL");
		output.tag.set_property("X", config.levels.values[i].x);
		output.tag.set_property("Y", config.levels.values[i].y);

		output.append_tag();
		output.tag.set_title("/LEVEL");
		output.append_tag();
		output.append_newline();
	}

	output.terminate_string();
}


void CompressorEffect::update_gui()
{
	if( !thread ) return;
	CompressorWindow *window = (CompressorWindow*)thread->window;
// load_configuration,read_data deletes levels
	window->lock_window("CompressorEffect::update_gui");
	if( load_configuration() )
		window->update();
	window->unlock_window();
}


LOAD_CONFIGURATION_MACRO(CompressorEffect, CompressorConfig)
NEW_WINDOW_MACRO(CompressorEffect, CompressorWindow)




int CompressorEffect::process_buffer(int64_t size,
		Samples **buffer,
		int64_t start_position,
		int sample_rate)
{
	load_configuration();

// Calculate linear transfer from db
	levels.remove_all();
	for(int i = 0; i < config.levels.total; i++)
	{
		levels.append();
		levels.values[i].x = DB::fromdb(config.levels.values[i].x);
		levels.values[i].y = DB::fromdb(config.levels.values[i].y);
	}
	min_x = DB::fromdb(config.min_db);
	min_y = DB::fromdb(config.min_db);
	max_x = 1.0;
	max_y = 1.0;


	int reaction_samples = (int)(config.reaction_len * sample_rate + 0.5);
	int decay_samples = (int)(config.decay_len * sample_rate + 0.5);
	int trigger = CLIP(config.trigger, 0, PluginAClient::total_in_buffers - 1);

	CLAMP(reaction_samples, -1000000, 1000000);
	CLAMP(decay_samples, reaction_samples, 1000000);
	CLAMP(decay_samples, 1, 1000000);
	if(labs(reaction_samples) < 1) reaction_samples = 1;
	if(labs(decay_samples) < 1) decay_samples = 1;

	int total_buffers = get_total_buffers();
	if(reaction_samples >= 0)
	{
		if(target_current_sample < 0) target_current_sample = reaction_samples;
		for(int i = 0; i < total_buffers; i++)
		{
			read_samples(buffer[i],
				i,
				sample_rate,
				start_position,
				size);
		}

		double current_slope = (next_target - previous_target) /
			reaction_samples;
		double *trigger_buffer = buffer[trigger]->get_data();
		for(int i = 0; i < size; i++)
		{
// Get slope required to reach current sample from smoothed sample over reaction
// length.
			double sample = 0.;
			switch(config.input)
			{
				case CompressorConfig::MAX:
				{
					double max = 0;
					for(int j = 0; j < total_buffers; j++)
					{
						sample = fabs(buffer[j]->get_data()[i]);
						if(sample > max) max = sample;
					}
					sample = max;
					break;
				}

				case CompressorConfig::TRIGGER:
					sample = fabs(trigger_buffer[i]);
					break;

				case CompressorConfig::SUM:
				{
					double max = 0;
					for(int j = 0; j < total_buffers; j++)
					{
						sample = fabs(buffer[j]->get_data()[i]);
						max += sample;
					}
					sample = max;
					break;
				}
			}

			double new_slope = (sample - current_value) /
				reaction_samples;

// Slope greater than current slope
			if(new_slope >= current_slope &&
				(current_slope >= 0 ||
				new_slope >= 0))
			{
				next_target = sample;
				previous_target = current_value;
				target_current_sample = 0;
				target_samples = reaction_samples;
				current_slope = new_slope;
			}
			else
			if(sample > next_target && current_slope < 0)
			{
				next_target = sample;
				previous_target = current_value;
				target_current_sample = 0;
				target_samples = decay_samples;
				current_slope = (sample - current_value) / decay_samples;
			}
// Current smoothed sample came up without finding higher slope
			if(target_current_sample >= target_samples)
			{
				next_target = sample;
				previous_target = current_value;
				target_current_sample = 0;
				target_samples = decay_samples;
				current_slope = (sample - current_value) / decay_samples;
			}

// Update current value and store gain
			current_value = (next_target * target_current_sample +
				previous_target * (target_samples - target_current_sample)) /
				target_samples;

			target_current_sample++;

			if(config.smoothing_only)
			{
				for(int j = 0; j < total_buffers; j++)
					buffer[j]->get_data()[i] = current_value;
			}
			else
			{
				double gain = calculate_gain(current_value);
				for(int j = 0; j < total_buffers; j++)
				{
					buffer[j]->get_data()[i] *= gain;
				}
			}
		}
	}
	else
	{
		if(target_current_sample < 0) target_current_sample = target_samples;
		int64_t preview_samples = -reaction_samples;

// Start of new buffer is outside the current buffer.  Start buffer over.
		if(start_position < input_start ||
			start_position >= input_start + input_size)
		{
			input_size = 0;
			input_start = start_position;
		}
		else
// Shift current buffer so the buffer starts on start_position
		if(start_position > input_start &&
			start_position < input_start + input_size)
		{
			if(input_buffer)
			{
				int len = input_start + input_size - start_position;
				for(int i = 0; i < total_buffers; i++)
				{
					memcpy(input_buffer[i]->get_data(),
						input_buffer[i]->get_data() + (start_position - input_start),
						len * sizeof(double));
				}
				input_size = len;
				input_start = start_position;
			}
		}

// Expand buffer to handle preview size
		if(size + preview_samples > input_allocated)
		{
			Samples **new_input_buffer = new Samples*[total_buffers];
			for(int i = 0; i < total_buffers; i++)
			{
				new_input_buffer[i] = new Samples(size + preview_samples);
				if(input_buffer)
				{
					memcpy(new_input_buffer[i]->get_data(),
						input_buffer[i]->get_data(),
						input_size * sizeof(double));
					delete input_buffer[i];
				}
			}
			if(input_buffer) delete [] input_buffer;

			input_allocated = size + preview_samples;
			input_buffer = new_input_buffer;
		}

// Append data to input buffer to construct readahead area.
#define MAX_FRAGMENT_SIZE 131072
		while(input_size < size + preview_samples)
		{
			int fragment_size = MAX_FRAGMENT_SIZE;
			if(fragment_size + input_size > size + preview_samples)
				fragment_size = size + preview_samples - input_size;
			for(int i = 0; i < total_buffers; i++)
			{
				input_buffer[i]->set_offset(input_size);
//printf("CompressorEffect::process_buffer %d %p %d\n", __LINE__, input_buffer[i], input_size);
				read_samples(input_buffer[i],
					i,
					sample_rate,
					input_start + input_size,
					fragment_size);
				input_buffer[i]->set_offset(0);
			}
			input_size += fragment_size;
		}


		double current_slope = (next_target - previous_target) /
			target_samples;
		double *trigger_buffer = input_buffer[trigger]->get_data();
		for(int i = 0; i < size; i++)
		{
// Get slope from current sample to every sample in preview_samples.
// Take highest one or first one after target_samples are up.

// For optimization, calculate the first slope we really need.
// Assume every slope up to the end of preview_samples has been calculated and
// found <= to current slope.
            int first_slope = preview_samples - 1;
// Need new slope immediately
			if(target_current_sample >= target_samples)
				first_slope = 1;
			for(int j = first_slope;
				j < preview_samples;
				j++)
			{
				double sample = 0.;
				switch(config.input)
				{
					case CompressorConfig::MAX:
					{
						double max = 0;
						for(int k = 0; k < total_buffers; k++)
						{
							sample = fabs(input_buffer[k]->get_data()[i + j]);
							if(sample > max) max = sample;
						}
						sample = max;
						break;
					}

					case CompressorConfig::TRIGGER:
						sample = fabs(trigger_buffer[i + j]);
						break;

					case CompressorConfig::SUM:
					{
						double max = 0;
						for(int k = 0; k < total_buffers; k++)
						{
							sample = fabs(input_buffer[k]->get_data()[i + j]);
							max += sample;
						}
						sample = max;
						break;
					}
				}






				double new_slope = (sample - current_value) /
					j;
// Got equal or higher slope
				if(new_slope >= current_slope &&
					(current_slope >= 0 ||
					new_slope >= 0))
				{
					target_current_sample = 0;
					target_samples = j;
					current_slope = new_slope;
					next_target = sample;
					previous_target = current_value;
				}
				else
				if(sample > next_target && current_slope < 0)
				{
					target_current_sample = 0;
					target_samples = decay_samples;
					current_slope = (sample - current_value) /
						decay_samples;
					next_target = sample;
					previous_target = current_value;
				}

// Hit end of current slope range without finding higher slope
				if(target_current_sample >= target_samples)
				{
					target_current_sample = 0;
					target_samples = decay_samples;
					current_slope = (sample - current_value) / decay_samples;
					next_target = sample;
					previous_target = current_value;
				}
			}

// Update current value and multiply gain
			current_value = (next_target * target_current_sample +
				previous_target * (target_samples - target_current_sample)) /
				target_samples;
//buffer[0][i] = current_value;
			target_current_sample++;

			if(config.smoothing_only)
			{
				for(int j = 0; j < total_buffers; j++)
				{
					buffer[j]->get_data()[i] = current_value;
				}
			}
			else
			{
				double gain = calculate_gain(current_value);
				for(int j = 0; j < total_buffers; j++)
				{
					buffer[j]->get_data()[i] = input_buffer[j]->get_data()[i] * gain;
				}
			}
		}



	}





	return 0;
}

double CompressorEffect::calculate_output(double x)
{
	if(x > 0.999) return 1.0;

	for(int i = levels.total - 1; i >= 0; i--)
	{
		if(levels.values[i].x <= x)
		{
			if(i < levels.total - 1)
			{
				return levels.values[i].y +
					(x - levels.values[i].x) *
					(levels.values[i + 1].y - levels.values[i].y) /
					(levels.values[i + 1].x - levels.values[i].x);
			}
			else
			{
				return levels.values[i].y +
					(x - levels.values[i].x) *
					(max_y - levels.values[i].y) /
					(max_x - levels.values[i].x);
			}
		}
	}

	if(levels.total)
	{
		return min_y +
			(x - min_x) *
			(levels.values[0].y - min_y) /
			(levels.values[0].x - min_x);
	}
	else
		return x;
}


double CompressorEffect::calculate_gain(double input)
{
//  	double x_db = DB::todb(input);
//  	double y_db = config.calculate_db(x_db);
//  	double y_linear = DB::fromdb(y_db);
	double y_linear = calculate_output(input);
	double gain;
	if(input != 0)
		gain = y_linear / input;
	else
		gain = 100000;
	return gain;
}










CompressorConfig::CompressorConfig()
{
	reaction_len = 1.0;
	min_db = -80.0;
	min_x = min_db;
	min_y = min_db;
	max_x = 0;
	max_y = 0;
	trigger = 0;
	input = CompressorConfig::TRIGGER;
	smoothing_only = 0;
	decay_len = 1.0;
}

void CompressorConfig::copy_from(CompressorConfig &that)
{
	this->reaction_len = that.reaction_len;
	this->decay_len = that.decay_len;
	this->min_db = that.min_db;
	this->min_x = that.min_x;
	this->min_y = that.min_y;
	this->max_x = that.max_x;
	this->max_y = that.max_y;
	this->trigger = that.trigger;
	this->input = that.input;
	this->smoothing_only = that.smoothing_only;
	levels.remove_all();
	for(int i = 0; i < that.levels.total; i++)
		this->levels.append(that.levels.values[i]);
}

int CompressorConfig::equivalent(CompressorConfig &that)
{
	if(!EQUIV(this->reaction_len, that.reaction_len) ||
		!EQUIV(this->decay_len, that.decay_len) ||
		this->trigger != that.trigger ||
		this->input != that.input ||
		this->smoothing_only != that.smoothing_only)
		return 0;
	if(this->levels.total != that.levels.total) return 0;
	for(int i = 0;
		i < this->levels.total && i < that.levels.total;
		i++)
	{
		compressor_point_t *this_level = &this->levels.values[i];
		compressor_point_t *that_level = &that.levels.values[i];
		if(!EQUIV(this_level->x, that_level->x) ||
			!EQUIV(this_level->y, that_level->y))
			return 0;
	}
	return 1;
}

void CompressorConfig::interpolate(CompressorConfig &prev,
	CompressorConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	copy_from(prev);
}

int CompressorConfig::total_points()
{
	if(!levels.total)
		return 1;
	else
		return levels.total;
}

void CompressorConfig::dump()
{
	printf("CompressorConfig::dump\n");
	for(int i = 0; i < levels.total; i++)
	{
		printf("	%f %f\n", levels.values[i].x, levels.values[i].y);
	}
}


double CompressorConfig::get_y(int number)
{
	if(!levels.total)
		return 1.0;
	else
	if(number >= levels.total)
		return levels.values[levels.total - 1].y;
	else
		return levels.values[number].y;
}

double CompressorConfig::get_x(int number)
{
	if(!levels.total)
		return 0.0;
	else
	if(number >= levels.total)
		return levels.values[levels.total - 1].x;
	else
		return levels.values[number].x;
}

double CompressorConfig::calculate_db(double x)
{
	if(x > -0.001) return 0.0;

	for(int i = levels.total - 1; i >= 0; i--)
	{
		if(levels.values[i].x <= x)
		{
			if(i < levels.total - 1)
			{
				return levels.values[i].y +
					(x - levels.values[i].x) *
					(levels.values[i + 1].y - levels.values[i].y) /
					(levels.values[i + 1].x - levels.values[i].x);
			}
			else
			{
				return levels.values[i].y +
					(x - levels.values[i].x) *
					(max_y - levels.values[i].y) /
					(max_x - levels.values[i].x);
			}
		}
	}

	if(levels.total)
	{
		return min_y +
			(x - min_x) *
			(levels.values[0].y - min_y) /
			(levels.values[0].x - min_x);
	}
	else
		return x;
}


int CompressorConfig::set_point(double x, double y)
{
	for(int i = levels.total - 1; i >= 0; i--)
	{
		if(levels.values[i].x < x)
		{
			levels.append();
			i++;
			for(int j = levels.total - 2; j >= i; j--)
			{
				levels.values[j + 1] = levels.values[j];
			}
			levels.values[i].x = x;
			levels.values[i].y = y;

			return i;
		}
	}

	levels.append();
	for(int j = levels.total - 2; j >= 0; j--)
	{
		levels.values[j + 1] = levels.values[j];
	}
	levels.values[0].x = x;
	levels.values[0].y = y;
	return 0;
}

void CompressorConfig::remove_point(int number)
{
	for(int j = number; j < levels.total - 1; j++)
	{
		levels.values[j] = levels.values[j + 1];
	}
	levels.remove();
}

void CompressorConfig::optimize()
{
	int done = 0;

	while(!done)
	{
		done = 1;


		for(int i = 0; i < levels.total - 1; i++)
		{
			if(levels.values[i].x >= levels.values[i + 1].x)
			{
				done = 0;
				for(int j = i + 1; j < levels.total - 1; j++)
				{
					levels.values[j] = levels.values[j + 1];
				}
				levels.remove();
			}
		}

	}
}



























CompressorWindow::CompressorWindow(CompressorEffect *plugin)
 : PluginClientWindow(plugin,
	650,
	480,
	650,
	480,
	0)
{
	this->plugin = plugin;
}

void CompressorWindow::create_objects()
{
	int x = 35, y = 10;
	int control_margin = 130;

	add_subwindow(canvas = new CompressorCanvas(plugin,
		x,
		y,
		get_w() - x - control_margin - 10,
		get_h() - y - 70));
	canvas->set_cursor(CROSS_CURSOR, 0, 0);
	x = get_w() - control_margin;
	add_subwindow(new BC_Title(x, y, _("Reaction secs:")));
	y += 20;
	add_subwindow(reaction = new CompressorReaction(plugin, x, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Decay secs:")));
	y += 20;
	add_subwindow(decay = new CompressorDecay(plugin, x, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Trigger Type:")));
	y += 20;
	add_subwindow(input = new CompressorInput(plugin, x, y));
	input->create_objects();
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Trigger:")));
	y += 20;
	add_subwindow(trigger = new CompressorTrigger(plugin, x, y));
	if(plugin->config.input != CompressorConfig::TRIGGER) trigger->disable();
	y += 30;
	add_subwindow(smooth = new CompressorSmooth(plugin, x, y));
	y += 60;
	add_subwindow(clear = new CompressorClear(plugin, x, y));
	x = 10;
	y = get_h() - 40;
	add_subwindow(new BC_Title(x, y, _("Point:")));
	x += 50;
	add_subwindow(x_text = new CompressorX(plugin, x, y));
	x += 110;
	add_subwindow(new BC_Title(x, y, _("x")));
	x += 20;
	add_subwindow(y_text = new CompressorY(plugin, x, y));
	draw_scales();

	update_canvas();
	show_window();
}

void CompressorWindow::draw_scales()
{
	draw_3d_border(canvas->get_x() - 2,
		canvas->get_y() - 2,
		canvas->get_w() + 4,
		canvas->get_h() + 4,
		get_bg_color(),
		BLACK,
		MDGREY,
		get_bg_color());


	set_font(SMALLFONT);
	set_color(get_resources()->default_text_color);

#define DIVISIONS 8
	for(int i = 0; i <= DIVISIONS; i++)
	{
		int y = canvas->get_y() + 10 + canvas->get_h() / DIVISIONS * i;
		int x = canvas->get_x() - 30;
		char string[BCTEXTLEN];

		sprintf(string, "%.0f", (float)i / DIVISIONS * plugin->config.min_db);
		draw_text(x, y, string);

		int y1 = canvas->get_y() + canvas->get_h() / DIVISIONS * i;
		int y2 = canvas->get_y() + canvas->get_h() / DIVISIONS * (i + 1);
		for(int j = 0; j < 10; j++)
		{
			y = y1 + (y2 - y1) * j / 10;
			if(j == 0)
			{
				draw_line(canvas->get_x() - 10, y, canvas->get_x(), y);
			}
			else
			if(i < DIVISIONS)
			{
				draw_line(canvas->get_x() - 5, y, canvas->get_x(), y);
			}
		}
	}


	for(int i = 0; i <= DIVISIONS; i++)
	{
		int y = canvas->get_h() + 30;
		int x = canvas->get_x() + (canvas->get_w() - 10) / DIVISIONS * i;
		char string[BCTEXTLEN];

		sprintf(string, "%.0f", (1.0 - (float)i / DIVISIONS) * plugin->config.min_db);
		draw_text(x, y, string);

		int x1 = canvas->get_x() + canvas->get_w() / DIVISIONS * i;
		int x2 = canvas->get_x() + canvas->get_w() / DIVISIONS * (i + 1);
		for(int j = 0; j < 10; j++)
		{
			x = x1 + (x2 - x1) * j / 10;
			if(j == 0)
			{
				draw_line(x, canvas->get_y() + canvas->get_h(), x, canvas->get_y() + canvas->get_h() + 10);
			}
			else
			if(i < DIVISIONS)
			{
				draw_line(x, canvas->get_y() + canvas->get_h(), x, canvas->get_y() + canvas->get_h() + 5);
			}
		}
	}



	flash();
}

void CompressorWindow::update()
{
	update_textboxes();
	update_canvas();
}

void CompressorWindow::update_textboxes()
{
	if(atol(trigger->get_text()) != plugin->config.trigger)
		trigger->update((int64_t)plugin->config.trigger);
	if(strcmp(input->get_text(), CompressorInput::value_to_text(plugin->config.input)))
		input->set_text(CompressorInput::value_to_text(plugin->config.input));

	if(plugin->config.input != CompressorConfig::TRIGGER && trigger->get_enabled())
		trigger->disable();
	else
	if(plugin->config.input == CompressorConfig::TRIGGER && !trigger->get_enabled())
		trigger->enable();

	if(!EQUIV(atof(reaction->get_text()), plugin->config.reaction_len))
		reaction->update((float)plugin->config.reaction_len);
	if(!EQUIV(atof(decay->get_text()), plugin->config.decay_len))
		decay->update((float)plugin->config.decay_len);
	smooth->update(plugin->config.smoothing_only);
	if(canvas->current_operation == CompressorCanvas::DRAG)
	{
		x_text->update((float)plugin->config.levels.values[canvas->current_point].x);
		y_text->update((float)plugin->config.levels.values[canvas->current_point].y);
	}
}

#define POINT_W 10
void CompressorWindow::update_canvas()
{
	canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
	canvas->set_line_dashes(1);
	canvas->set_color(GREEN);

	for(int i = 1; i < DIVISIONS; i++)
	{
		int y = canvas->get_h() * i / DIVISIONS;
		canvas->draw_line(0, y, canvas->get_w(), y);

		int x = canvas->get_w() * i / DIVISIONS;
		canvas->draw_line(x, 0, x, canvas->get_h());
	}
	canvas->set_line_dashes(0);


	canvas->set_font(MEDIUMFONT);
	canvas->draw_text(plugin->get_theme()->widget_border,
		canvas->get_h() / 2,
		_("Output"));
	canvas->draw_text(canvas->get_w() / 2 - canvas->get_text_width(MEDIUMFONT, _("Input")) / 2,
		canvas->get_h() - plugin->get_theme()->widget_border,
		_("Input"));


	canvas->set_color(WHITE);
	canvas->set_line_width(2);

	double x_db = plugin->config.min_db;
	double y_db = plugin->config.calculate_db(x_db);
	int y1 = (int)(y_db / plugin->config.min_db * canvas->get_h());

	for(int i=1; i<canvas->get_w(); i++)
	{
		x_db = (1. - (double)i / canvas->get_w()) * plugin->config.min_db;
		y_db = plugin->config.calculate_db(x_db);
		int y2 = (int)(y_db / plugin->config.min_db * canvas->get_h());
		canvas->draw_line(i-1, y1, i, y2);
		y1 = y2;
	}
	canvas->set_line_width(1);

	int total = plugin->config.levels.total ? plugin->config.levels.total : 1;
	for(int i=0; i < total; i++)
	{
		x_db = plugin->config.get_x(i);
		y_db = plugin->config.get_y(i);

		int x = (int)((1. - x_db / plugin->config.min_db) * canvas->get_w());
		int y = (int)(y_db / plugin->config.min_db * canvas->get_h());

		canvas->draw_box(x - POINT_W / 2, y - POINT_W / 2, POINT_W, POINT_W);
	}

	canvas->flash();
}

int CompressorWindow::resize_event(int w, int h)
{
	return 1;
}








CompressorCanvas::CompressorCanvas(CompressorEffect *plugin, int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->plugin = plugin;
	current_operation = NONE;
	current_point = 0;
}

int CompressorCanvas::button_press_event()
{
// Check existing points
	if(is_event_win() && cursor_inside())
	{
		for(int i = 0; i < plugin->config.levels.total; i++)
		{
			double x_db = plugin->config.get_x(i);
			double y_db = plugin->config.get_y(i);

			int x = (int)(((double)1 - x_db / plugin->config.min_db) * get_w());
			int y = (int)(y_db / plugin->config.min_db * get_h());

			if(get_cursor_x() < x + POINT_W / 2 && get_cursor_x() >= x - POINT_W / 2 &&
				get_cursor_y() < y + POINT_W / 2 && get_cursor_y() >= y - POINT_W / 2)
			{
				current_operation = DRAG;
				current_point = i;
				return 1;
			}
		}



// Create new point
		double x_db = (double)(1 - (double)get_cursor_x() / get_w()) * plugin->config.min_db;
		double y_db = (double)get_cursor_y() / get_h() * plugin->config.min_db;

		current_point = plugin->config.set_point(x_db, y_db);
		current_operation = DRAG;
		((CompressorWindow*)plugin->thread->window)->update();
		plugin->send_configure_change();
		return 1;
	}
	return 0;
//plugin->config.dump();
}

int CompressorCanvas::button_release_event()
{
	if(current_operation == DRAG)
	{
		if(current_point > 0)
		{
			if(plugin->config.levels.values[current_point].x <
				plugin->config.levels.values[current_point - 1].x)
				plugin->config.remove_point(current_point);
		}

		if(current_point < plugin->config.levels.total - 1)
		{
			if(plugin->config.levels.values[current_point].x >=
				plugin->config.levels.values[current_point + 1].x)
				plugin->config.remove_point(current_point);
		}

		((CompressorWindow*)plugin->thread->window)->update();
		plugin->send_configure_change();
		current_operation = NONE;
		return 1;
	}

	return 0;
}

int CompressorCanvas::cursor_motion_event()
{
	if(current_operation == DRAG)
	{
		int x = get_cursor_x();
		int y = get_cursor_y();
		CLAMP(x, 0, get_w());
		CLAMP(y, 0, get_h());
		double x_db = (double)(1 - (double)x / get_w()) * plugin->config.min_db;
		double y_db = (double)y / get_h() * plugin->config.min_db;
		plugin->config.levels.values[current_point].x = x_db;
		plugin->config.levels.values[current_point].y = y_db;
		((CompressorWindow*)plugin->thread->window)->update();
		plugin->send_configure_change();
		return 1;
//plugin->config.dump();
	}
	else
// Change cursor over points
	if(is_event_win() && cursor_inside())
	{
		int new_cursor = CROSS_CURSOR;

		for(int i = 0; i < plugin->config.levels.total; i++)
		{
			double x_db = plugin->config.get_x(i);
			double y_db = plugin->config.get_y(i);

			int x = (int)(((double)1 - x_db / plugin->config.min_db) * get_w());
			int y = (int)(y_db / plugin->config.min_db * get_h());

			if(get_cursor_x() < x + POINT_W / 2 && get_cursor_x() >= x - POINT_W / 2 &&
				get_cursor_y() < y + POINT_W / 2 && get_cursor_y() >= y - POINT_W / 2)
			{
				new_cursor = UPRIGHT_ARROW_CURSOR;
				break;
			}
		}


		if(new_cursor != get_cursor())
		{
			set_cursor(new_cursor, 0, 1);
		}
	}
	return 0;
}





CompressorReaction::CompressorReaction(CompressorEffect *plugin, int x, int y)
 : BC_TextBox(x, y, 100, 1, (float)plugin->config.reaction_len)
{
	this->plugin = plugin;
}

int CompressorReaction::handle_event()
{
	plugin->config.reaction_len = atof(get_text());
	plugin->send_configure_change();
	return 1;
}

int CompressorReaction::button_press_event()
{
	if(is_event_win())
	{
		if(get_buttonpress() < 4) return BC_TextBox::button_press_event();
		if(get_buttonpress() == 4)
		{
			plugin->config.reaction_len += 0.1;
		}
		else
		if(get_buttonpress() == 5)
		{
			plugin->config.reaction_len -= 0.1;
		}
		update((float)plugin->config.reaction_len);
		plugin->send_configure_change();
		return 1;
	}
	return 0;
}

CompressorDecay::CompressorDecay(CompressorEffect *plugin, int x, int y)
 : BC_TextBox(x, y, 100, 1, (float)plugin->config.decay_len)
{
	this->plugin = plugin;
}
int CompressorDecay::handle_event()
{
	plugin->config.decay_len = atof(get_text());
	plugin->send_configure_change();
	return 1;
}

int CompressorDecay::button_press_event()
{
	if(is_event_win())
	{
		if(get_buttonpress() < 4) return BC_TextBox::button_press_event();
		if(get_buttonpress() == 4)
		{
			plugin->config.decay_len += 0.1;
		}
		else
		if(get_buttonpress() == 5)
		{
			plugin->config.decay_len -= 0.1;
		}
		update((float)plugin->config.decay_len);
		plugin->send_configure_change();
		return 1;
	}
	return 0;
}



CompressorX::CompressorX(CompressorEffect *plugin, int x, int y)
 : BC_TextBox(x, y, 100, 1, "")
{
	this->plugin = plugin;
}
int CompressorX::handle_event()
{
	int current_point = ((CompressorWindow*)plugin->thread->window)->canvas->current_point;
	if(current_point < plugin->config.levels.total)
	{
		plugin->config.levels.values[current_point].x = atof(get_text());
		((CompressorWindow*)plugin->thread->window)->update_canvas();
		plugin->send_configure_change();
	}
	return 1;
}



CompressorY::CompressorY(CompressorEffect *plugin, int x, int y)
 : BC_TextBox(x, y, 100, 1, "")
{
	this->plugin = plugin;
}
int CompressorY::handle_event()
{
	int current_point = ((CompressorWindow*)plugin->thread->window)->canvas->current_point;
	if(current_point < plugin->config.levels.total)
	{
		plugin->config.levels.values[current_point].y = atof(get_text());
		((CompressorWindow*)plugin->thread->window)->update_canvas();
		plugin->send_configure_change();
	}
	return 1;
}





CompressorTrigger::CompressorTrigger(CompressorEffect *plugin, int x, int y)
 : BC_TextBox(x, y, (int64_t)100, (int64_t)1, (int64_t)plugin->config.trigger)
{
	this->plugin = plugin;
}
int CompressorTrigger::handle_event()
{
	plugin->config.trigger = atol(get_text());
	plugin->send_configure_change();
	return 1;
}

int CompressorTrigger::button_press_event()
{
	if(is_event_win())
	{
		if(get_buttonpress() < 4) return BC_TextBox::button_press_event();
		if(get_buttonpress() == 4)
		{
			plugin->config.trigger++;
		}
		else
		if(get_buttonpress() == 5)
		{
			plugin->config.trigger--;
		}
		update((int64_t)plugin->config.trigger);
		plugin->send_configure_change();
		return 1;
	}
	return 0;
}





CompressorInput::CompressorInput(CompressorEffect *plugin, int x, int y)
 : BC_PopupMenu(x,
	y,
	100,
	CompressorInput::value_to_text(plugin->config.input),
	1)
{
	this->plugin = plugin;
}
int CompressorInput::handle_event()
{
	plugin->config.input = text_to_value(get_text());
	((CompressorWindow*)plugin->thread->window)->update();
	plugin->send_configure_change();
	return 1;
}

void CompressorInput::create_objects()
{
	for(int i = 0; i < 3; i++)
	{
		add_item(new BC_MenuItem(value_to_text(i)));
	}
}

const char* CompressorInput::value_to_text(int value)
{
	switch(value)
	{
		case CompressorConfig::TRIGGER: return _("Trigger");
		case CompressorConfig::MAX: return _("Maximum");
		case CompressorConfig::SUM: return _("Total");
	}

	return _("Trigger");
}

int CompressorInput::text_to_value(char *text)
{
	for(int i = 0; i < 3; i++)
	{
		if(!strcmp(value_to_text(i), text)) return i;
	}

	return CompressorConfig::TRIGGER;
}






CompressorClear::CompressorClear(CompressorEffect *plugin, int x, int y)
 : BC_GenericButton(x, y, _("Clear"))
{
	this->plugin = plugin;
}

int CompressorClear::handle_event()
{
	plugin->config.levels.remove_all();
//plugin->config.dump();
	((CompressorWindow*)plugin->thread->window)->update();
	plugin->send_configure_change();
	return 1;
}



CompressorSmooth::CompressorSmooth(CompressorEffect *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.smoothing_only, _("Smooth only"))
{
	this->plugin = plugin;
}

int CompressorSmooth::handle_event()
{
	plugin->config.smoothing_only = get_value();
	plugin->send_configure_change();
	return 1;
}




