
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

#include "clip.h"
#include "histogramconfig.h"
#include "units.h"

#include <math.h>



HistogramConfig::HistogramConfig()
{
	plot = 1;
	split = 0;

	reset(1);
}

void HistogramConfig::reset(int do_mode)
{
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		low_input[i] = 0.0;
		high_input[i] = 1.0;
		low_output[i] = 0.0;
		high_output[i] = 1.0;
		gamma[i] = 1.0;
	}

	if(do_mode)
	{
		automatic = 0;
		automatic_v = 0;
		threshold = 1.0;
	}
}

void HistogramConfig::reset_points(int colors_only)
{
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		if(i != HISTOGRAM_VALUE || !colors_only)
		{
			low_input[i] = 0.0;
			high_input[i] = 1.0;
		}
	}
}


void HistogramConfig::boundaries()
{
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		CLAMP(low_input[i], HIST_MIN_INPUT, HIST_MAX_INPUT);
		CLAMP(high_input[i], HIST_MIN_INPUT, HIST_MAX_INPUT);
		CLAMP(low_output[i], HIST_MIN_INPUT, HIST_MAX_INPUT);
		CLAMP(high_output[i], HIST_MIN_INPUT, HIST_MAX_INPUT);
		CLAMP(gamma[i], MIN_GAMMA, MAX_GAMMA);
		low_output[i] = Units::quantize(low_output[i], PRECISION);
		high_output[i] = Units::quantize(high_output[i], PRECISION);
	}
	CLAMP(threshold, 0, 1);
}

int HistogramConfig::equivalent(HistogramConfig &that)
{
// EQUIV isn't precise enough to detect changes in points
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
// 		if(!EQUIV(low_input[i], that.low_input[i]) ||
// 			!EQUIV(high_input[i], that.high_input[i]) ||
// 			!EQUIV(gamma[i], that.gamma[i]) ||
// 			!EQUIV(low_output[i], that.low_output[i]) ||
// 			!EQUIV(high_output[i], that.high_output[i])) return 0;
		if(low_input[i] != that.low_input[i] ||
			high_input[i] != that.high_input[i] ||
			gamma[i] != that.gamma[i] ||
			low_output[i] != that.low_output[i] ||
			high_output[i] != that.high_output[i]) return 0;
	}

	if(automatic != that.automatic ||
		automatic_v != that.automatic_v ||
		threshold != that.threshold) return 0;

	if(plot != that.plot ||
		split != that.split) return 0;

	return 1;
}

void HistogramConfig::copy_from(HistogramConfig &that)
{
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		low_input[i] = that.low_input[i];
		high_input[i] = that.high_input[i];
		gamma[i] = that.gamma[i];
		low_output[i] = that.low_output[i];
		high_output[i] = that.high_output[i];
	}

	automatic = that.automatic;
	automatic_v = that.automatic_v;
	threshold = that.threshold;
	plot = that.plot;
	split = that.split;
}

void HistogramConfig::interpolate(HistogramConfig &prev,
	HistogramConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = 1.0 - next_scale;

	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		low_input[i] = prev.low_input[i] * prev_scale + next.low_input[i] * next_scale;
		high_input[i] = prev.high_input[i] * prev_scale + next.high_input[i] * next_scale;
		gamma[i] = prev.gamma[i] * prev_scale + next.gamma[i] * next_scale;
		low_output[i] = prev.low_output[i] * prev_scale + next.low_output[i] * next_scale;
		high_output[i] = prev.high_output[i] * prev_scale + next.high_output[i] * next_scale;
	}

	threshold = prev.threshold * prev_scale + next.threshold * next_scale;
	automatic = prev.automatic;
	automatic_v = prev.automatic_v;
	plot = prev.plot;
	split = prev.split;


}


void HistogramConfig::dump()
{
	for(int j = 0; j < HISTOGRAM_MODES; j++)
	{
		printf("HistogramConfig::dump mode=%d plot=%d split=%d\n", j, plot, split);
	}
}



