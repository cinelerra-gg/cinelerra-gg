
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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

#include "bcsignals.h"
#include "clip.h"
#include "resample.h"
#include "samples.h"
#include "transportque.inc"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Resampling from Lame

Resample::Resample()
{
	old = new double[BLACKSIZE];
	resample_init = 0;
	last_ratio = 0;
	output_temp = 0;
	output_size = 0;
	output_allocation = 0;
	input_size = RESAMPLE_CHUNKSIZE;
	input_position = 0;
	input = new Samples(input_size + 1);
	output_position = 0;
	itime = 0;
	direction = PLAY_FORWARD;
}


Resample::~Resample()
{
	delete [] old;
	delete [] output_temp;
	delete input;
}

int Resample::read_samples(Samples *buffer, int64_t start, int64_t len)
{
	return 0;
}

int Resample::get_direction()
{
	return direction;
}


void Resample::reset()
{
	resample_init = 0;
	output_size = 0;
	output_position = 0;
	input_position = 0;
}

double Resample::blackman(int i, double offset, double fcn, int l)
{
  /* This algorithm from:
SIGNAL PROCESSING ALGORITHMS IN FORTRAN AND C
S.D. Stearns and R.A. David, Prentice-Hall, 1992
  */

	double bkwn;
	double wcn = (M_PI * fcn);
	double dly = l / 2.0;
	double x = i-offset;
	if(x < 0) x = 0;
	else
	if(x > l) x = l;

	bkwn = 0.42 - 0.5 * cos((x * 2) * M_PI /l) + 0.08 * cos((x * 4) * M_PI /l);
	if(fabs(x - dly) < 1e-9)
  		return wcn / M_PI;
    else
    	return (sin((wcn * (x - dly))) / (M_PI * (x - dly)) * bkwn);
}


int Resample::get_output_size()
{
	return output_size;
}

// void Resample::read_output(double *output, int size)
// {
// 	memcpy(output, output_temp, size * sizeof(double));
// // Shift leftover forward
// 	for(int i = size; i < output_size; i++)
// 		output_temp[i - size] = output_temp[i];
// 	output_size -= size;
// }



void Resample::resample_chunk(Samples *input_buffer,
	int64_t in_len,
	int in_rate,
	int out_rate)
{
	double resample_ratio = (double)in_rate / out_rate;
  	int filter_l;
	double fcn, intratio;
	double offset, xvalue;
	int num_used;
	int i, j, k;
	double *input = input_buffer->get_data();
//printf("Resample::resample_chunk %d in_len=%jd input_size=%d\n",
// __LINE__, in_len, input_size);

  	intratio = (fabs(resample_ratio - floor(.5 + resample_ratio)) < .0001);
	fcn = .90 / resample_ratio;
	if(fcn > .90) fcn = .90;
	filter_l = BLACKSIZE - 6;
/* must be odd */
	if(0 == filter_l % 2 ) --filter_l;

/* if resample_ratio = int, filter_l should be even */
  	filter_l += (int)intratio;

// Blackman filter initialization must be called whenever there is a
// sampling ratio change
	if(!resample_init || last_ratio != resample_ratio)
	{
		resample_init = 1;
		itime = 0;
		bzero(old, sizeof(double) * BLACKSIZE);

// precompute blackman filter coefficients
    	for (j = 0; j <= 2 * BPC; ++j)
		{
			for(j = 0; j <= 2 * BPC; j++)
			{
				offset = (double)(j - BPC) / (2 * BPC);
				for(i = 0; i <= filter_l; i++)
				{
					blackfilt[j][i] = blackman(i, offset, fcn, filter_l);
				}
			}
		}
	}

// Main loop
	double *inbuf_old = old;
	for(k = 0; 1; k++)
	{
		double time0;
		int joff;

		time0 = k * resample_ratio;
		j = (int)floor(time0 - itime);

//		if(j + filter_l / 2 >= input_size) break;
		if(j + filter_l / 2 >= in_len) break;

/* blackman filter.  by default, window centered at j+.5(filter_l%2) */
/* but we want a window centered at time0.   */
		offset = (time0 - itime - (j + .5 * (filter_l % 2)));
		joff = (int)floor((offset * 2 * BPC) + BPC + .5);
		xvalue = 0;


		for(i = 0; i <= filter_l; i++)
		{
			int j2 = i + j - filter_l / 2;
//printf("j2=%d\n", j2);
			double y = ((j2 < 0) ? inbuf_old[BLACKSIZE + j2] : input[j2]);

			xvalue += y * blackfilt[joff][i];
		}


		if(output_allocation <= output_size)
		{
			double *new_output = 0;
			int64_t new_allocation = output_allocation ? (output_allocation * 2) : 16384;
			new_output = new double[new_allocation];
			if(output_temp)
			{
				bcopy(output_temp, new_output, output_allocation * sizeof(double));
				delete [] output_temp;
			}

			output_temp = new_output;
			output_allocation = new_allocation;
		}

		output_temp[output_size++] = xvalue;
	}

	num_used = MIN(in_len, j + filter_l / 2);
	itime += num_used - k * resample_ratio;
	for(i = 0; i < BLACKSIZE; i++)
		inbuf_old[i] = input[num_used + i - BLACKSIZE];

	last_ratio = resample_ratio;

}

int Resample::read_chunk(Samples *input,
	int64_t len)
{
	int fragment = len;
	if(direction == PLAY_REVERSE &&
		input_position - len < 0)
	{
		fragment = input_position;
	}

	int result = read_samples(input, input_position, fragment);

	if(direction == PLAY_FORWARD)
	{
		input_position += fragment;
	}
	else
	{
		input_position -= fragment;
// Mute unused part of buffer
		if(fragment < len)
		{
			bzero(input->get_data() + fragment,
				(len - fragment) * sizeof(double));
		}
	}

	return result;
}


void Resample::reverse_buffer(double *buffer, int64_t len)
{
	double *ap = buffer;
	double *bp = ap + len;
	while( ap < --bp ) {
		double t = *ap;
		*ap++ = *bp;
		*bp = t;
	}
}


int Resample::resample(Samples *output,
	int64_t out_len,
	int in_rate,
	int out_rate,
	int64_t out_position,
	int direction)
{
	int result = 0;


//printf("Resample::resample 1 output_position=%jd out_position=%jd out_len=%jd\n",
// output_position, out_position, out_len);
// Changed position
	if(labs(this->output_position - out_position) > 0 ||
		direction != this->direction)
	{
		reset();

// Compute starting point in input rate.
		this->input_position = out_position * in_rate / out_rate;
		this->direction = direction;
	}


	int remaining_len = out_len;
	double *output_ptr = output->get_data();
	while(remaining_len > 0 && !result)
	{
// Drain output buffer
		if(output_size)
		{
			int fragment_len = output_size;
			if(fragment_len > remaining_len) fragment_len = remaining_len;

//printf("Resample::resample 1 %d %d\n", remaining_len, output_size);
			bcopy(output_temp, output_ptr, fragment_len * sizeof(double));

// Shift leftover forward
			for(int i = fragment_len; i < output_size; i++)
				output_temp[i - fragment_len] = output_temp[i];

			output_size -= fragment_len;
			remaining_len -= fragment_len;
			output_ptr += fragment_len;
		}

// Import new samples
//printf("Resample::resample 2 %d\n", remaining_len);
		if(remaining_len > 0)
		{
//printf("Resample::resample 3 input_size=%d out_position=%d\n", input_size, out_position);
			result = read_chunk(input, input_size);
			resample_chunk(input,
				input_size,
				in_rate,
				out_rate);
		}
	}


	if(direction == PLAY_FORWARD)
		this->output_position = out_position + out_len;
	else
		this->output_position = out_position - out_len;

//printf("Resample::resample 2 %d %d\n", this->output_position, out_position);
//printf("Resample::resample 2 %d %d\n", out_len, output_size);

//printf("Resample::resample 2 %d\n", output_size);
	return result;
}



