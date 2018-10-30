
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

#include "automation.inc"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "track.h"
#include "localsession.h"
#include "transportque.inc"

FloatAutos::FloatAutos(EDL *edl,
				Track *track,
				float default_)
 : Autos(edl, track)
{
	this->default_ = default_;
	type = AUTOMATION_TYPE_FLOAT;
}

FloatAutos::~FloatAutos()
{
}

void FloatAutos::set_automation_mode(int64_t start, int64_t end, int mode)
{
	FloatAuto *current = (FloatAuto*)first;
	while(current)
	{
// Is current auto in range?
		if(current->position >= start && current->position < end)
		{
			current->change_curve_mode((FloatAuto::t_mode)mode);
		}
		current = (FloatAuto*)NEXT;
	}
}

void FloatAutos::draw_joining_line(BC_SubWindow *canvas, int vertical, int center_pixel, int x1, int y1, int x2, int y2)
{
	if(vertical)
		canvas->draw_line(center_pixel - y1, x1, center_pixel - y2, x2);
	else
		canvas->draw_line(x1, center_pixel + y1, x2, center_pixel + y2);
}

Auto* FloatAutos::new_auto()
{
	FloatAuto *result = new FloatAuto(edl, this);
	result->set_value(default_);
	return result;
}

int FloatAutos::get_testy(float slope, int cursor_x, int ax, int ay)
{
	return (int)(slope * (cursor_x - ax)) + ay;
}

int FloatAutos::automation_is_constant(int64_t start,
	int64_t length,
	int direction,
	double &constant)
{
	int total_autos = total();
	int64_t end;
	if(direction == PLAY_FORWARD)
	{
		end = start + length;
	}
	else
	{
		end = start + 1;
		start -= length;
	}


// No keyframes on track
	if(total_autos == 0)
	{
		constant = ((FloatAuto*)default_auto)->get_value();
		return 1;
	}
	else
// Only one keyframe on track.
	if(total_autos == 1)
	{
		constant = ((FloatAuto*)first)->get_value();
		return 1;
	}
	else
// Last keyframe is before region
	if(last->position <= start)
	{
		constant = ((FloatAuto*)last)->get_value();
		return 1;
	}
	else
// First keyframe is after region
	if(first->position > end)
	{
		constant = ((FloatAuto*)first)->get_value();
		return 1;
	}

// Scan sequentially
	int64_t prev_position = -1;
	for(Auto *current = first; current; current = NEXT)
	{
		int test_current_next = 0;
		int test_previous_current = 0;
		FloatAuto *float_current = (FloatAuto*)current;

// keyframes before and after region but not in region
		if(prev_position >= 0 &&
			prev_position < start &&
			current->position >= end)
		{
// Get value now in case change doesn't occur
			constant = float_current->get_value();
			test_previous_current = 1;
		}
		prev_position = current->position;

// Keyframe occurs in the region
		if(!test_previous_current &&
			current->position < end &&
			current->position >= start)
		{

// Get value now in case change doesn't occur
			constant = float_current->get_value();

// Keyframe has neighbor
			if(current->previous)
			{
				test_previous_current = 1;
			}

			if(current->next)
			{
				test_current_next = 1;
			}
		}

		if(test_current_next)
		{
//printf("FloatAutos::automation_is_constant 1 %d\n", start);
			FloatAuto *float_next = (FloatAuto*)current->next;

// Change occurs between keyframes
			if( !EQUIV(float_current->get_value(), float_next->get_value()) ||
				!EQUIV(float_current->get_control_out_value(), 0) ||
				!EQUIV(float_next->get_control_in_value(), 0))
			{
				return 0;
			}
		}

		if(test_previous_current)
		{
			FloatAuto *float_previous = (FloatAuto*)current->previous;

// Change occurs between keyframes
			if(!EQUIV(float_current->get_value(), float_previous->get_value()) ||
				!EQUIV(float_current->get_control_in_value(), 0) ||
				!EQUIV(float_previous->get_control_out_value(), 0))
			{
// printf("FloatAutos::automation_is_constant %d %d %d %f %f %f %f\n",
// start,
// float_previous->position,
// float_current->position,
// float_previous->get_value(),
// float_current->get_value(),
// float_previous->get_control_out_value(),
// float_current->get_control_in_value());
				return 0;
			}
		}
	}

// Got nothing that changes in the region.
	return 1;
}

double FloatAutos::get_automation_constant(int64_t start, int64_t end)
{
	Auto *current_auto, *before = 0, *after = 0;

// quickly get autos just outside range
	get_neighbors(start, end, &before, &after);

// no auto before range so use first
	if(before)
		current_auto = before;
	else
		current_auto = first;

// no autos at all so use default value
	if(!current_auto) current_auto = default_auto;

	return ((FloatAuto*)current_auto)->get_value();
}


float FloatAutos::get_value(int64_t position,
	int direction,
	FloatAuto* &previous,
	FloatAuto* &next)
{
// Calculate bezier equation at position
	previous = (FloatAuto*)get_prev_auto(position, direction, (Auto* &)previous, 0);
	next = (FloatAuto*)get_next_auto(position, direction, (Auto* &)next, 0);

// Constant
	if(!next && !previous) return ((FloatAuto*)default_auto)->get_value();
	if(!previous) return next->get_value();
	if(!next) return previous->get_value();
	if(next == previous) return previous->get_value();

	if(direction == PLAY_FORWARD)
	{
		if(EQUIV(previous->get_value(), next->get_value())) {
			if( (previous->curve_mode == FloatAuto::LINEAR &&
			     next->curve_mode == FloatAuto::LINEAR) ||
			    (EQUIV(previous->get_control_out_value(), 0) &&
			     EQUIV(next->get_control_in_value(), 0))) {
				return previous->get_value();
			}
		}
	}
	else if(direction == PLAY_REVERSE) {
		if(EQUIV(previous->get_value(), next->get_value())) {
			if( (previous->curve_mode == FloatAuto::LINEAR &&
			     next->curve_mode == FloatAuto::LINEAR) ||
			    (EQUIV(previous->get_control_in_value(), 0) &&
			     EQUIV(next->get_control_out_value(), 0))) {
				return previous->get_value();
			}
		}
	}
// at this point: previous and next not NULL, positions differ, value not constant.

	return calculate_bezier(previous, next, position);
}


float FloatAutos::calculate_bezier(FloatAuto *previous, FloatAuto *next, int64_t position)
{
	if(next->position - previous->position == 0) return previous->get_value();

	float y0 = previous->get_value();
	float y3 = next->get_value();

// control points
	float y1 = previous->get_value() + previous->get_control_out_value();
	float y2 = next->get_value() + next->get_control_in_value();
	float t = (float)(position - previous->position) /
			(next->position - previous->position);

 	float tpow2 = t * t;
	float tpow3 = t * t * t;
	float invt = 1 - t;
	float invtpow2 = invt * invt;
	float invtpow3 = invt * invt * invt;

	float result = (  invtpow3 * y0
		+ 3 * t     * invtpow2 * y1
		+ 3 * tpow2 * invt     * y2
		+     tpow3            * y3);
//printf("FloatAutos::get_value(t=%5.3f)->%6.2f   (prev,pos,next)=(%d,%d,%d)\n", t, result, previous->position, position, next->position);

	return result;
}


float FloatAutos::calculate_bezier_derivation(FloatAuto *previous, FloatAuto *next, int64_t position)
// calculate the slope of the interpolating bezier function at given position.
// computed slope is based on the actual position scale (in frames or samples)
{
	float scale = next->position - previous->position;
	if( scale == 0 ) {
		if( !previous->get_control_out_position() )
			return 0;
		return previous->get_control_out_value() / previous->get_control_out_position();
	}
	float y0 = previous->get_value();
	float y3 = next->get_value();

// control points
	float y1 = previous->get_value() + previous->get_control_out_value();
	float y2 = next->get_value() + next->get_control_in_value();
// normalized scale
	float t = (float)(position - previous->position) / scale;

 	float tpow2 = t * t;
	float invt = 1 - t;
	float invtpow2 = invt * invt;

	float slope = 3 * (
		- invtpow2              * y0
		- invt * ( 2*t - invt ) * y1
		+ t    * ( 2*invt - t ) * y2
		+ tpow2                 * y3
		);

	return slope / scale;
}



void FloatAutos::get_extents(float *min,
	float *max,
	int *coords_undefined,
	int64_t unit_start,
	int64_t unit_end)
{
	if(!edl)
	{
		printf("FloatAutos::get_extents edl == NULL\n");
		return;
	}

	if(!track)
	{
		printf("FloatAutos::get_extents track == NULL\n");
		return;
	}

// Use default auto
	if(!first)
	{
		FloatAuto *current = (FloatAuto*)default_auto;
		if(*coords_undefined)
		{
			*min = *max = current->get_value();
			*coords_undefined = 0;
		}

		*min = MIN(current->get_value(), *min);
		*max = MAX(current->get_value(), *max);
	}

// Test all handles
	for(FloatAuto *current = (FloatAuto*)first; current; current = (FloatAuto*)NEXT)
	{
		if(current->position >= unit_start && current->position < unit_end)
		{
			if(*coords_undefined)
			{
				*min = *max = current->get_value();
				*coords_undefined = 0;
			}

			*min = MIN(current->get_value(), *min);
			*min = MIN(current->get_value() + current->get_control_in_value(), *min);
			*min = MIN(current->get_value() + current->get_control_out_value(), *min);

			*max = MAX(current->get_value(), *max);
			*max = MAX(current->get_value() + current->get_control_in_value(), *max);
			*max = MAX(current->get_value() + current->get_control_out_value(), *max);
		}
	}

// Test joining regions
	FloatAuto *prev = 0;
	FloatAuto *next = 0;
	int64_t unit_step = edl->local_session->zoom_sample;
	if(track->data_type == TRACK_VIDEO)
		unit_step = (int64_t)(unit_step *
			edl->session->frame_rate /
			edl->session->sample_rate);
	unit_step = MAX(unit_step, 1);
	for(int64_t position = unit_start;
		position < unit_end;
		position += unit_step)
	{
		float value = get_value(position,PLAY_FORWARD,prev,next);
		if(*coords_undefined)
		{
			*min = *max = value;
			*coords_undefined = 0;
		}
		else
		{
			*min = MIN(value, *min);
			*max = MAX(value, *max);
		}
	}
}

void FloatAutos::set_proxy(int orig_scale, int new_scale)
{
	float orig_value;
	orig_value = ((FloatAuto*)default_auto)->value * orig_scale;
	((FloatAuto*)default_auto)->value = orig_value / new_scale;

	for( FloatAuto *current= (FloatAuto*)first; current; current=(FloatAuto*)NEXT ) {
		orig_value = current->value * orig_scale;
		current->value = orig_value / new_scale;
		orig_value = current->control_in_value * orig_scale;
		current->control_in_value = orig_value / new_scale;
		orig_value = current->control_out_value * orig_scale;
		current->control_out_value = orig_value / new_scale;
	}
}

void FloatAutos::dump()
{
	printf("	FloatAutos::dump %p\n", this);
	printf("	Default: position %jd value=%f\n",
		default_auto->position, ((FloatAuto*)default_auto)->get_value());
	for(Auto* current = first; current; current = NEXT)
	{
		printf("	position %jd value=%7.3f invalue=%7.3f outvalue=%7.3f %s\n",
			current->position,
			((FloatAuto*)current)->get_value(),
			((FloatAuto*)current)->get_control_in_value(),
			((FloatAuto*)current)->get_control_out_value(),
			FloatAuto::curve_name(((FloatAuto*)current)->curve_mode));
	}
}

double FloatAutos::automation_integral(int64_t start, int64_t length, int direction)
{
	if( direction == PLAY_REVERSE )
		start -= length;
	if( start < 0 ) {
		length += start;
		start = 0;
	}
	double value = 0;
	int64_t pos = start, len = length, end = pos + len;
	while( pos < end ) {
		int64_t prev_pos = 0, next_pos = end;
		Auto *zprev = 0, *znext = 0;
		FloatAuto *prev = (FloatAuto*)get_prev_auto(pos, direction, zprev, 0);
		if( prev ) prev_pos = prev->position;
		FloatAuto *next = (FloatAuto*)get_next_auto(pos, direction, znext, 0);
		if( next ) next_pos = next->position;
		if( !prev && !next ) prev = next = (FloatAuto*)default_auto;
		else if( !prev ) prev = next;
		else if( !next ) next = prev;

		double dt = next_pos - prev_pos;
		double t0 = (pos - prev_pos) / dt;
		if( (pos = next_pos) > end ) pos = end;
		double t1 = (pos - prev_pos) / dt;

		double y0 = prev->get_value(), y1 = y0 + prev->get_control_out_value();
		double y3 = next->get_value(), y2 = y3 + next->get_control_in_value();
		if( y0 != y1 || y1 != y2 || y2 != y3 ) {
// bezier definite integral t0..t1
			double f4 = -y0/4 + 3*y1/4 - 3*y2/4 + y3/4;
			double f3 = y0 - 2*y1 + y2;
			double f2 = -3*y0/2 + 3*y1/2;
			double f1 = y0, t = t0;
			double t2 = t*t, t3 = t2*t, t4 = t3*t;
			t0 = t4*f4 + t3*f3 + t2*f2 + t*f1;
			t = t1;  t2 = t*t;  t3 = t2*t;  t4 = t3*t;
			t1 = t4*f4 + t3*f3 + t2*f2 + t*f1;
		}
		else {
			t0 *= y0;  t1 *= y0;
		}
		value += dt * (t1 - t0);
	}

	return value + 1e-6;
}

int64_t FloatAutos::speed_position(double pos)
{
	double length = track->get_length();
	int64_t l = -1, r = track->to_units(length, 1);
	if( r < 1 ) r = 1;
	for( int i=32; --i >= 0 && automation_integral(0,r,PLAY_FORWARD) <= pos; r*=2 );
	for( int i=64; --i >= 0 && (r-l)>1; ) {
		int64_t m = (l + r) / 2;
		double t = automation_integral(0,m,PLAY_FORWARD) - pos;
		*(t >= 0 ? &r : &l) = m;
	}
	return r;
}

