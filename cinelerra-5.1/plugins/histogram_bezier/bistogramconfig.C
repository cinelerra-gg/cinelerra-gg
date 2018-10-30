
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
#include "bistogramconfig.h"
#include "units.h"

#include <math.h>


HistogramPoint::HistogramPoint()
 : ListItem<HistogramPoint>()
{
	x = 0;  y = 0;
	xoffset_left = -0.05;
	xoffset_right = 0.05;
	gradient = 1.0;

}

HistogramPoint::~HistogramPoint()
{
}

int HistogramPoint::equivalent(HistogramPoint *src)
{
	return EQUIV(x, src->x) && EQUIV(y, src->y) &&
		EQUIV(xoffset_left, src->xoffset_left) &&
		EQUIV(xoffset_right, src->xoffset_right) &&
		EQUIV(gradient, src->gradient);
}

void HistogramPoint::copy_from(HistogramPoint *that)
{
	x = that->x;  y = that->y;
	xoffset_left = that->xoffset_left;
	xoffset_right = that->xoffset_right;
	gradient = that->gradient;

}


HistogramPoints::HistogramPoints()
 : List<HistogramPoint>()
{
	clear();
}

HistogramPoints::~HistogramPoints()
{
}

void HistogramPoints::clear()
{
	while( last ) delete last;
	insert(0.0,0.0);
	first->gradient = 1.0;
	first->xoffset_left = 0.0;
	first->xoffset_right = 0.05;
	insert(1.0,1.0);
	last->gradient = 1.0;
	last->xoffset_left = -0.05;
	last->xoffset_right = 0.0;
}

HistogramPoint* HistogramPoints::insert(float x, float y)
{
	HistogramPoint *current = first;

// Get existing point after new point
	while( current ) {
		if( current->x > x ) break;
		current = NEXT;
	}

// Insert new point before current point
	HistogramPoint *new_point = new HistogramPoint;
	if( current )
		insert_before(current, new_point);
	else
		append(new_point);

	new_point->x = x;
	new_point->y = y;
	return new_point;
}

void HistogramPoints::boundaries()
{
	HistogramPoint *current = first;
	while( current ) {
		CLAMP(current->x, 0.0, 1.0);
		CLAMP(current->y, 0.0, 1.0);
		current = NEXT;
	}
}

int HistogramPoints::equivalent(HistogramPoints *src)
{
	HistogramPoint *current_this = first;
	HistogramPoint *current_src = src->first;
	while( current_this && current_src ) {
		if(!current_this->equivalent(current_src)) return 0;
		current_this = current_this->next;
		current_src = current_src->next;
	}

	return current_this || current_src ? 0 : 1;
}

void HistogramPoints::copy_from(HistogramPoints *src)
{
	while(last)
		delete last;
	HistogramPoint *current = src->first;
	while( current ) {
		HistogramPoint *new_point = new HistogramPoint;
		new_point->copy_from(current);
		append(new_point);
		current = NEXT;
	}
}

int HistogramPoints::cmprx(HistogramPoint *ap, HistogramPoint *bp)
{
	return ap->x < bp->x ? -1 : ap->x == bp->x ? 0 : 1;
}

void HistogramPoints::interpolate(HistogramPoints *prev, HistogramPoints *next,
		double prev_scale, double next_scale)
{
	HistogramPoint *current_prev = prev->first;
	HistogramPoint *current_next = next->first;

	HistogramPoint *current = first;
	while( current_prev || current_next ) {
		if( !current ) {
			current = new HistogramPoint;
			append(current);
		}
		if( !current_next ) {
			current->copy_from(current_prev);
			current_prev = current_prev->next;
		}
		else if( !current_prev ) {
			current->copy_from(current_next);
			current_next = current_next->next;
		}
		else {
			current->x = current_prev->x * prev_scale +
					current_next->x * next_scale;
			current->y = current_prev->y * prev_scale +
					current_next->y * next_scale;
			current->gradient = current_prev->gradient * prev_scale +
					current_next->gradient * next_scale;
			current->xoffset_left = current_prev->xoffset_left * prev_scale +
					current_next->xoffset_left * next_scale;
			current->xoffset_right = current_prev->xoffset_right * prev_scale +
					current_next->xoffset_right * next_scale;
			current_prev = current_prev->next;
			current_next = current_next->next;
		}
		current = current->next;
	}

	while( current ) {
		HistogramPoint *next_point = current->next;
		delete current;  current = next_point;
	}
	sort(cmprx);
}


HistogramConfig::HistogramConfig()
{
	reset(1);
}

void HistogramConfig::reset(int do_mode)
{
	reset_points();
	for( int i = 0; i < HISTOGRAM_MODES; i++ ) {
		output_min[i] = 0.0;
		output_max[i] = 1.0;
	}

	if( do_mode ) {
		automatic = 0;
		threshold = 1.0;
		split = 0;
		smoothMode = 0;
	}
}

void HistogramConfig::reset_points()
{
	for( int i=0; i<HISTOGRAM_MODES; ++i )
		points[i].clear();
}


void HistogramConfig::boundaries()
{
	for( int i=0; i<HISTOGRAM_MODES; ++i ) {
		points[i].boundaries();
		CLAMP(output_min[i], HIST_MIN_INPUT, HIST_MAX_INPUT);
		CLAMP(output_max[i], HIST_MIN_INPUT, HIST_MAX_INPUT);
		output_min[i] = Units::quantize(output_min[i], PRECISION);
		output_max[i] = Units::quantize(output_max[i], PRECISION);
	}
	CLAMP(threshold, 0, 1);
}

int HistogramConfig::equivalent(HistogramConfig &that)
{
	for( int i=0; i<HISTOGRAM_MODES; ++i ) {
		if( !points[i].equivalent(&that.points[i]) ||
		    !EQUIV(output_min[i], that.output_min[i]) ||
		    !EQUIV(output_max[i], that.output_max[i]) ) return 0;
	}

	if( automatic != that.automatic ||
	    split != that.split ||
	    smoothMode != that.smoothMode ||
	    !EQUIV(threshold, that.threshold) ) return 0;

	return 1;
}

void HistogramConfig::copy_from(HistogramConfig &that)
{
	for( int i=0; i<HISTOGRAM_MODES; ++i ) {
		points[i].copy_from(&that.points[i]);
		output_min[i] = that.output_min[i];
		output_max[i] = that.output_max[i];
	}

	automatic = that.automatic;
	threshold = that.threshold;
	split = that.split;
	smoothMode = that.smoothMode;
}

void HistogramConfig::interpolate(HistogramConfig &prev, HistogramConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = 1.0 - next_scale;

	for( int i=0; i<HISTOGRAM_MODES; ++i ) {
		points[i].interpolate(&prev.points[i], &next.points[i], prev_scale, next_scale);
		output_min[i] = prev.output_min[i] * prev_scale + next.output_min[i] * next_scale;
		output_max[i] = prev.output_max[i] * prev_scale + next.output_max[i] * next_scale;
	}

	threshold = prev.threshold * prev_scale + next.threshold * next_scale;
	automatic = prev.automatic;
	split = prev.split;
	smoothMode = prev.smoothMode;
}


void HistogramConfig::dump()
{
	printf("HistogramConfig::dump: automatic=%d, threshold=%f, split=%d, smoothMode=%d\n",
		automatic, threshold, split, smoothMode);
	static const char *mode_name[] = { "red","grn","blu","val" };
	for( int j=0; j<HISTOGRAM_MODES; ++j ) {
		printf("mode[%s]:\n", mode_name[j]);
		HistogramPoints *points = &this->points[j];
		HistogramPoint *current = points->first;
		while( current ) {
			printf("%f,%f (@%f l%f,r%f)\n", current->x, current->y,
				current->gradient, current->xoffset_left, current->xoffset_right);
			fflush(stdout);
			current = NEXT;
		}
	}
}

