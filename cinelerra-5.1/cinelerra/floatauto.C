
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

#include "autos.h"
#include "clip.h"
#include "edl.h"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "language.h"
#include "localsession.h"
#include "transportque.inc"
#include "automation.inc"

FloatAuto::FloatAuto(EDL *edl, FloatAutos *autos)
 : Auto(edl, (Autos*)autos)
{
	value = 0;
	control_in_value = 0;
	control_out_value = 0;
	control_in_position = 0;
	control_out_position = 0;
	pos_valid = -1; //"dirty"
	curve_mode = FREE;
//  note: in most cases the curve_mode value is set
//        by the method interpolate_from() rsp. copy_from()
}

FloatAuto::~FloatAuto()
{
	// as we are going away, the neighbouring float auto nodes
	// need to re-adjust their ctrl point positions and curves
	if (next)
		((FloatAuto*)next)->curve_dirty();
	if (previous)
		((FloatAuto*)previous)->curve_dirty();
}

int FloatAuto::operator==(Auto &that)
{
	return identical((FloatAuto*)&that);
}


int FloatAuto::operator==(FloatAuto &that)
{
	return identical((FloatAuto*)&that);
}

int FloatAuto::identical(FloatAuto *src)
{
	return EQUIV(value, src->value) &&
		EQUIV(control_in_value, src->control_in_value) &&
		EQUIV(control_out_value, src->control_out_value);
		// ctrl positions ignored, as they may depend on neighbours
		// curve_mode is ignored, no recalculations
}

// exactly equals
int FloatAuto::equals(FloatAuto *that)
{
	return this->value == that->value &&
		this->control_in_value == that->control_in_value &&
		this->control_out_value == that->control_out_value &&
		this->control_in_position == that->control_in_position &&
		this->control_out_position == that->control_out_position &&
		this->curve_mode == that->curve_mode;
}


/* Note: the following is essentially display-code and has been moved to:
 *  TrackCanvas::value_to_percentage(float auto_value, int autogrouptype)
 *
float FloatAuto::value_to_percentage()
{
}
float FloatAuto::value_to_percentage()
{
}
float FloatAuto::value_to_percentage()
{
}
*/


void FloatAuto::copy_from(Auto *that)
{
	copy_from((FloatAuto*)that);
}

void FloatAuto::copy_from(FloatAuto *that)
{
	Auto::copy_from(that);
	this->value = that->value;
	this->control_in_value = that->control_in_value;
	this->control_out_value = that->control_out_value;
	this->control_in_position = that->control_in_position;
	this->control_out_position = that->control_out_position;
	this->curve_mode = that->curve_mode;
// note: literate copy, no recalculations
}

inline
void FloatAuto::handle_automatic_curve_after_copy()
// in most cases, we don't want to use the manual curve modes
// of the left neighbour used as a template for interpolation.
// Rather, we (re)set to automatically smoothed curves. Note
// auto generated nodes (while tweaking values) indeed are
// inserted by using this "interpolation" approach, thus making
// this defaulting to auto-smooth curves very important.
{
	if(curve_mode == FREE || curve_mode == TFREE)
	{
		this->curve_mode = SMOOTH;
	}
}


int FloatAuto::interpolate_from(Auto *a1, Auto *a2, int64_t pos, Auto *templ)
// bézier interpolates this->value and curves for the given position
// between the positions of a1 and a2. If a1 or a2 are omitted, they default
// to this->previous and this->next. If this FloatAuto has automatic curves,
// this may trigger re-adjusting of this and its neighbours in this->autos.
// Note while a1 and a2 need not be members of this->autos, automatic
// readjustments are always done to the neighbours in this->autos.
// If the template is given, it will be used to fill out this
// objects fields prior to interpolating.
{
	if(!a1) a1 = previous;
	if(!a2) a2 = next;
	Auto::interpolate_from(a1, a2, pos, templ);
	if( !templ ) handle_automatic_curve_after_copy();
	if( curve_mode == SMOOTH && a1 && a2 &&
	    a1->is_floatauto() && a2->is_floatauto() &&
	    a1->position <= pos && pos <= a2->position ) {
		// set this->value using bézier interpolation if possible
		FloatAuto *left = (FloatAuto*)a1;
		FloatAuto *right = (FloatAuto*)a2;
		if( pos != position ) { // this may trigger smoothing
			this->adjust_to_new_coordinates(pos,
				FloatAutos::calculate_bezier(left, right, pos));
		}
		float new_slope = FloatAutos::calculate_bezier_derivation(left, right, pos);
		this->set_control_in_value(new_slope * control_in_position);
		this->set_control_out_value(new_slope * control_out_position);
		return 1; //return true: interpolated indeed...
	}

	adjust_ctrl_positions(); // implies adjust_curves()
	return 0; // unable to interpolate
}


void FloatAuto::change_curve_mode(t_mode new_mode)
{
	if(new_mode == TFREE && !(control_in_position && control_out_position))
		new_mode = FREE; // only if curves on both sides...

	curve_mode = new_mode;
	adjust_curves();
}

void FloatAuto::toggle_curve_mode()
{
	switch (curve_mode) {
	case SMOOTH:	change_curve_mode(TFREE);  break;
	case LINEAR:	change_curve_mode(FREE);   break;
	case TFREE :	change_curve_mode(LINEAR); break;
	case FREE  :	change_curve_mode(SMOOTH); break;
	}
}


void FloatAuto::set_value(float newvalue)
{
	this->value=newvalue;
	this->adjust_curves();
	if(previous) ((FloatAuto*)previous)->adjust_curves();
	if(next)     ((FloatAuto*)next)->adjust_curves();
}

void FloatAuto::set_control_in_value(float newvalue)
{
	switch(curve_mode) {
	case TFREE:	control_out_value = control_out_position*newvalue / control_in_position;
	case FREE:	control_in_value = newvalue;
	default:	return; // otherwise calculated automatically...
	}
}

void FloatAuto::set_control_out_value(float newvalue)
{
	switch(curve_mode) {
	case TFREE:	control_in_value = control_in_position*newvalue / control_out_position;
	case FREE:	control_out_value=newvalue;
	default:	return;
	}
}



inline int sgn(float value) { return (value == 0)?  0 : (value < 0) ? -1 : 1; }

inline float weighted_mean(float v1, float v2, float w1, float w2){
	if(0.000001 > fabs(w1 + w2))
		return 0;
	else
		return (w1 * v1 + w2 * v2) / (w1 + w2);
}




void FloatAuto::adjust_curves()
// recalculates curves if current mode
// implies automatic adjustment of curves
{
	if(!autos) return;

	if(curve_mode == SMOOTH) {
		// normally, one would use the slope of chord between the neighbours.
		// but this could cause the curve to overshot extremal automation nodes.
		// (e.g when setting a fade node at zero, the curve could go negative)
		// we can interpret the slope of chord as a weighted mean value, where
		// the length of the interval is used as weight; we just use other
		// weights: intervall length /and/ reciprocal of slope. So, if the
		// connection to one of the neighbours has very low slope this will
		// dominate the calculated curve slope at this automation node.
		// if the slope goes beyond the zero line, e.g if left connection
		// has positive and right connection has negative slope, then
		// we force the calculated curve to be horizontal.
		float s, dxl, dxr, sl, sr;
		calculate_slope((FloatAuto*) previous, this, sl, dxl);
		calculate_slope(this, (FloatAuto*) next, sr, dxr);

		if(0 < sgn(sl) * sgn(sr))
		{
			float wl = fabs(dxl) * (fabs(1.0/sl) + 1);
			float wr = fabs(dxr) * (fabs(1.0/sr) + 1);
			s = weighted_mean(sl, sr, wl, wr);
		}
		else s = 0; // fixed hoizontal curve

		control_in_value = s * control_in_position;
		control_out_value = s * control_out_position;
	}

	else if(curve_mode == LINEAR) {
		float g, dx;
		if(previous)
		{
			calculate_slope(this, (FloatAuto*)previous, g, dx);
			control_in_value = g * dx / 3;
		}
		if(next)
		{
			calculate_slope(this, (FloatAuto*)next, g, dx);
			control_out_value = g * dx / 3;
	}	}

	else if(curve_mode == TFREE && control_in_position && control_out_position) {
		float gl = control_in_value / control_in_position;
		float gr = control_out_value / control_out_position;
		float wl = fabs(control_in_value);
		float wr = fabs(control_out_value);
		float g = weighted_mean(gl, gr, wl, wr);

		control_in_value = g * control_in_position;
		control_out_value = g * control_out_position;
	}
}

inline void FloatAuto::calculate_slope(FloatAuto *left, FloatAuto *right, float &dvdx, float &dx)
{
	dvdx=0; dx=0;
	if(!left || !right) return;

	dx = right->position - left->position;
	float dv = right->value - left->value;
	dvdx = (dx == 0) ? 0 : dv/dx;
}


void FloatAuto::adjust_ctrl_positions(FloatAuto *prev, FloatAuto *next)
// recalculates location of ctrl points to be
// always 1/3 and 2/3 of the distance to the
// next neighbours. The reason is: for this special
// distance the bézier function yields x(t) = t, i.e.
// we can use the y(t) as if it was a simple function y(x).

// This adjustment is done only on demand and involves
// updating neighbours and adjust_curves() as well.
{
	if(!prev && !next)
	{	// use current siblings
		prev = (FloatAuto*)this->previous;
		next = (FloatAuto*)this->next;
	}

	if(prev)
	{	set_ctrl_positions(prev, this);
		prev->adjust_curves();
	}
	else // disable curve on left side
		control_in_position = 0;

	if(next)
	{	set_ctrl_positions(this, next);
		next->adjust_curves();
	}
	else // disable right curve
		control_out_position = 0;

	this->adjust_curves();
	pos_valid = position;
// curves up-to-date
}



inline void redefine_curve(int64_t &old_pos, int64_t new_pos, float &ctrl_val)
{
	if(old_pos != 0)
		ctrl_val *= (float)new_pos / old_pos;
	old_pos = new_pos;
}


inline void FloatAuto::set_ctrl_positions(FloatAuto *prev, FloatAuto* next)
{
	int64_t distance = next->position - prev->position;
	redefine_curve(prev->control_out_position, +distance / 3,  prev->control_out_value);
	redefine_curve(next->control_in_position,  -distance / 3,  next->control_in_value);
}



void FloatAuto::adjust_to_new_coordinates(int64_t position, float value)
// define new position and value in one step, do necessary re-adjustments
{
	this->value = value;
	this->position = position;
	adjust_ctrl_positions();
}



int FloatAuto::value_to_str(char *string, float value)
{
	int j = 0, i = 0;
	if(value > 0)
		sprintf(string, "+%.2f", value);
	else
		sprintf(string, "%.2f", value);

// fix number
	if(value == 0)
	{
		j = 0;
		string[1] = 0;
	}
	else
	if(value < 1 && value > -1)
	{
		j = 1;
		string[j] = string[0];
	}
	else
	{
		j = 0;
		string[3] = 0;
	}

	while(string[j] != 0) string[i++] = string[j++];
	string[i] = 0;

	return 0;
}

void FloatAuto::copy(int64_t start, int64_t end, FileXML *file, int default_auto)
{
	file->tag.set_title("AUTO");
	if(default_auto)
		file->tag.set_property("POSITION", 0);
	else
		file->tag.set_property("POSITION", position - start);
	file->tag.set_property("VALUE", value);
	file->tag.set_property("CONTROL_IN_VALUE", control_in_value / 2.0); // compatibility, see below
	file->tag.set_property("CONTROL_OUT_VALUE", control_out_value / 2.0);
	file->tag.set_property("TANGENT_MODE", (int)curve_mode);
	file->append_tag();
	file->tag.set_title("/AUTO");
	file->append_tag();
	file->append_newline();
}

void FloatAuto::load(FileXML *file)
{
	value = file->tag.get_property("VALUE", value);
	control_in_value = file->tag.get_property("CONTROL_IN_VALUE", control_in_value);
	control_out_value = file->tag.get_property("CONTROL_OUT_VALUE", control_out_value);
	curve_mode = (t_mode)file->tag.get_property("TANGENT_MODE", (int)FREE);

	// Compatibility to old session data format:
	// Versions previous to the bezier auto patch (Jun 2006) applied a factor 2
	// to the y-coordinates of ctrl points while calculating the bezier function.
	// To retain compatibility, we now apply this factor while loading
	control_in_value *= 2.0;
	control_out_value *= 2.0;

// restore ctrl positions and adjust curves if necessary
	adjust_ctrl_positions();
}

const char *FloatAuto::curve_name(int curve_mode)
{
	switch( curve_mode ) {
	case FloatAuto::SMOOTH: return _("Smooth");
	case FloatAuto::LINEAR: return _("Linear");
	case FloatAuto::TFREE:  return _("Tangent");
       	case FloatAuto::FREE:   return _("Disjoint");
        }
        return _("Error");
}

