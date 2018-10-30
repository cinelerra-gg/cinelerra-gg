
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#ifndef HISTOGRAMCONFIG_H
#define HISTOGRAMCONFIG_H


#include "histogram.inc"
#include "histogramconfig.inc"
#include "linklist.h"
#include <stdint.h>


class HistogramConfig
{
public:
	HistogramConfig();

	int equivalent(HistogramConfig &that);
	void copy_from(HistogramConfig &that);
	void interpolate(HistogramConfig &prev,
		HistogramConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
// Used by constructor and reset button
	void reset(int do_mode);
	void reset_points(int colors_only);
	void boundaries();
	void dump();

// Range 0 - 1.0
// Input points
	float low_input[HISTOGRAM_MODES];
	float high_input[HISTOGRAM_MODES];
	float gamma[HISTOGRAM_MODES];
// Output points
	float low_output[HISTOGRAM_MODES];
	float high_output[HISTOGRAM_MODES];

	int automatic;
	int automatic_v;
	float threshold;
	int plot;
	int split;
};


#endif



