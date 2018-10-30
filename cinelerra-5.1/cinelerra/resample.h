
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


// Generic resampling module

#ifndef RESAMPLE_H
#define RESAMPLE_H

#define BPC 160
#define BLACKSIZE 25

#include "resample.inc"
#include "samples.inc"
#include <stdint.h>

class Resample
{
public:
	Resample();
	virtual ~Resample();

// All positions are in file sample rate
// This must reverse the buffer during reverse playback
// so the filter always renders forward.
	virtual int read_samples(Samples *buffer, int64_t start, int64_t len);

// Resample from the file handler and store in *output.
// Returns 1 if the input reader failed.
	int resample(Samples *samples,
		int64_t out_len,
		int in_rate,
		int out_rate,
// Starting sample in output samplerate
// If reverse, the end of the buffer.
		int64_t out_position,
		int direction);

	int get_direction();

	static void reverse_buffer(double *buffer, int64_t len);

// Reset after seeking
	void reset();

private:
	double blackman(int i, double offset, double fcn, int l);
// Query output temp
	int get_output_size();
//	void read_output(Samples *output, int size);
// Resamples input and dumps it to output_temp
	void resample_chunk(Samples *input,
		int64_t in_len,
		int in_rate,
		int out_rate);
	int read_chunk(Samples *input,
		int64_t len);

// History buffer for resampling.
	double *old;
	double itime;



// Unaligned resampled output
	double *output_temp;


// Total samples in unaligned output
	int64_t output_size;

	int direction;
// Allocation of unaligned output
	int64_t output_allocation;
// input chunk
	Samples *input;
// Position of source in source sample rate.
	int64_t input_position;
	int64_t input_size;
	int64_t output_position;
	int resample_init;
// Last sample ratio configured to
	double last_ratio;
	double blackfilt[2 * BPC + 1][BLACKSIZE];
};

#endif
