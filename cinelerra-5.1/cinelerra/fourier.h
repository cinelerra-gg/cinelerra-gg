
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

#ifndef FOURIER_H
#define FOURIER_H


#include "samples.inc"

#include <stdint.h>


class FFT
{
	static uint8_t rev_bytes[256];
	static unsigned int reverse_bits(unsigned int index, unsigned int bits);
	static void bit_reverse(unsigned int samples,
    		double *real_in, double *imag_in,
    		double *real_out, double *imag_out);
public:
	FFT();
	virtual ~FFT();
	// can be in place, but imag_out must exist, imag_in optional
	int do_fft(int samples, // must be a power of 2
		int inverse,  		 // 0 = forward FFT, 1 = inverse
    		double *real_in, double *imag_in,	// complex input
	    	double *real_out, double *imag_out);	// complex output
	int do_fft(int samples, int inverse, double *real, double *imag) {
		return do_fft(samples, inverse, real, imag, real, imag);
	}
	static unsigned int samples_to_bits(unsigned int samples);
	static int symmetry(int size, double *freq_real, double *freq_imag);
	virtual int update_progress(int current_position);
};

class CrossfadeFFT : public FFT
{
public:
	CrossfadeFFT();
	virtual ~CrossfadeFFT();

	int reset();
	int initialize(int window_size);
	long get_delay();     // Number of samples fifo is delayed
	int reconfigure();
	int fix_window_size();
	int delete_fft();


// Read enough samples from input to generate the requested number of samples.
// output_sample - tells it if we've seeked and the output overflow is invalid.
// return - 0 on success 1 on failure
// output_sample - start of samples if forward.  End of samples if reverse.
//               It's always contiguous.
// output_ptr - if nonzero, output is put here
// direction - PLAY_FORWARD or PLAY_REVERSE
	int process_buffer(int64_t output_sample,
		long size,
		Samples *output_ptr,
		int direction);

// Called by process_buffer to read samples from input.
// Returns 1 on error or 0 on success.
	virtual int read_samples(int64_t output_sample,
		int samples,
		Samples *buffer);

// Process a window in the frequency domain
	virtual int signal_process();
// Process a window in the time domain after the frequency domain
	virtual int post_process();

// Size of a window.  Automatically fixed to a power of 2
	long window_size;

// Frequency domain output of FFT
	double *freq_real;
	double *freq_imag;
// Time domain output of FFT
	double *output_real;
	double *output_imag;

private:

// input for complete windows
	Samples *input_buffer;
// output for crossfaded windows with overflow
	double *output_buffer;

// samples in input_buffer
	long input_size;
// window_size
	long input_allocation;
// Samples in output buffer less window border
	long output_size;
// Space in output buffer including window border
	long output_allocation;
// Starting sample of output buffer in project
	int64_t output_sample;
// Starting sample of input buffer in project
	int64_t input_sample;
// Don't crossfade the first window
	int first_window;
};

#endif
