
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

#include "samples.h"
#include "timestretchengine.h"


#include <stdio.h>
#include <string.h>
#include <unistd.h>





TimeStretchEngine::TimeStretchEngine(double scale, int sample_rate, int window_time)
{
	output = 0;
	output_allocation = 0;
	input = 0;
	input_allocation = 0;
	reset();
	update(scale, sample_rate, window_time);
}

TimeStretchEngine::~TimeStretchEngine()
{
	if(output) delete [] output;
	if(input) delete [] input;
}

void TimeStretchEngine::reset()
{
	input_size = 0;
	input_sample = 0;
	output_size = 0;
	output_sample = 0;
}

void TimeStretchEngine::update(double scale, int sample_rate, int window_time)
{
	this->scale = scale;
	this->sample_rate = sample_rate;
	this->window_time = window_time;

	window_size = (int64_t)sample_rate * (int64_t)window_time / (int64_t)1000;
	window_skirt = window_size / 2;
//printf("TimeStretchEngine::update %d %d\n", __LINE__, window_size);
	if(output) delete [] output;
	if(input) delete [] input;
//printf("TimeStretchEngine::update %d %d\n", __LINE__, window_size);

	output = 0;
	output_allocation = 0;
	input = 0;
	input_allocation = 0;
}


void TimeStretchEngine::overlay(double *out, double *in, int size, int skirt)
{
// Fade in
	for(int i = 0; i < skirt; i++)
	{
		*out = *out * (1.0 - (double)i / skirt) + *in * ((double)i / skirt);
		out++;
		in++;
	}

// Center
	for(int i = 0; i < size - skirt; i++)
	{
		*out++ = *in++;
	}

// New skirt
	for(int i = 0; i < skirt; i++)
	{
		*out++ = *in++;
	}
}

int TimeStretchEngine::process(Samples *in_buffer, int in_size)
{
// printf("TimeStretchEngine::process %d in_buffer=%p in_size=%d\n",
// __LINE__,
// in_buffer,
// in_size);
// Stack on input buffer
	if(!input || input_size + in_size > input_allocation)
	{
		int new_input_allocation = input_size + in_size;
		double *new_input = new double[new_input_allocation];
		if(input)
		{
			memcpy(new_input, input, input_size * sizeof(double));
			delete [] input;
		}
		input = new_input;
		input_allocation = new_input_allocation;
	}

// printf("TimeStretchEngine::process %d input=%p input_size=%d input_allocation=%d\n",
// __LINE__,
// input,
// input_size,
// input_allocation);

	memcpy(input + input_size, in_buffer->get_data(), in_size * sizeof(double));
	input_size += in_size;

//printf("TimeStretchEngine::process %d\n", __LINE__);
// Overlay windows from input buffer into output buffer
	int done = 0;
	do
	{
		int64_t current_out_sample = output_sample + output_size;
		int64_t current_in_sample = (int64_t)((double)current_out_sample / scale);

		if(current_in_sample - input_sample + window_size + window_skirt > input_size)
		{
// Shift input buffer so the fragment that would have been copied now will be
// in the next iteration.


// printf("TimeStretchEngine::process %d %lld %d\n",
// __LINE__,
// current_in_sample - input_sample,
// input_size);

			if(current_in_sample - input_sample < input_size)
			{
				memcpy(input,
					input + current_in_sample - input_sample,
					(input_size - (current_in_sample - input_sample)) * sizeof(double));
				input_size -= current_in_sample - input_sample;
				input_sample = current_in_sample;
			}

			done = 1;
//printf("TimeStretchEngine::process %d input_size=%d\n", __LINE__, input_size);
		}
		else
		{
//printf("TimeStretchEngine::process %d\n", __LINE__);
// Allocate output buffer
			if(output_size + window_size + window_skirt > output_allocation)
			{
				int new_allocation = output_size + window_size + window_skirt;
				double *new_output = new double[new_allocation];
				bzero(new_output, new_allocation * sizeof(double));
				if(output)
				{
					memcpy(new_output,
						output,
						(output_size + window_skirt) * sizeof(double));
// printf("TimeStretchEngine::process %d this=%p output=%p new_allocation=%d\n",
// __LINE__,
// this,
// output,
// new_allocation);
					delete [] output;
//printf("TimeStretchEngine::process %d\n", __LINE__);
				}
				output = new_output;
				output_allocation = new_allocation;
			}

// Overlay new window
			overlay(output + output_size,
				input + current_in_sample - input_sample,
				window_size,
				window_skirt);
			output_size += window_size;
//printf("TimeStretchEngine::process 30 %d\n", output_size);
		}
	}while(!done);

//printf("TimeStretchEngine::process %d %d\n", __LINE__, output_size);
	return output_size;
}

void TimeStretchEngine::read_output(Samples *buffer, int size)
{
	memcpy(buffer->get_data(), output, size * sizeof(double));
	memcpy(output, output + size, (output_size + window_skirt - size) * sizeof(double));
	output_size -= size;
	output_sample += size;
}

int TimeStretchEngine::get_output_size()
{
	return output_size;
}

double* TimeStretchEngine::get_samples()
{
	return output;
}









