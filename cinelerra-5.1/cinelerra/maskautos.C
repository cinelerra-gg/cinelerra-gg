
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
#include "localsession.h"
#include "maskauto.h"
#include "maskautos.h"
#include "track.h"
#include "transportque.inc"

MaskAutos::MaskAutos(EDL *edl,
	Track *track)
 : Autos(edl, track)
{
	type = AUTOMATION_TYPE_MASK;
}

MaskAutos::~MaskAutos()
{
}




void MaskAutos::update_parameter(MaskAuto *src)
{
	double selection_start = edl->local_session->get_selectionstart(0);
	double selection_end = edl->local_session->get_selectionend(0);

// Selection is always aligned to frame for masks

// Create new keyframe if auto keyframes or replace entire keyframe.
	if(selection_start == selection_end)
	{
// Search for keyframe to write to
		MaskAuto *dst = (MaskAuto*)get_auto_for_editing();

		dst->copy_data(src);
	}
	else
// Replace changed parameter in all selected keyframes.
	{
// Search all keyframes in selection but don't create a new one.
		int64_t start = track->to_units(selection_start, 0);
		int64_t end = track->to_units(selection_end, 0);
		Auto *current_auto = 0;
		MaskAuto *current = 0;
		current = (MaskAuto*)get_prev_auto(start,
			PLAY_FORWARD,
			current_auto,
			1);

// The first one determines the changed parameters since it is the one displayed
// in the GUI.
		MaskAuto *first = current;

// Update the first one last, so it is available for comparisons to the changed one.
		for(current = (MaskAuto*)NEXT;
			current && current->position < end;
			current = (MaskAuto*)NEXT)
		{
			current->update_parameter(first,
				src);
		}
		first->copy_data(src);
	}
}



void MaskAutos::get_points(ArrayList<MaskPoint*> *points,
	int submask,
	int64_t position,
	int direction)
{
	MaskAuto *begin = 0, *end = 0;
	position = (direction == PLAY_FORWARD) ? position : (position - 1);

// Get auto before and after position
	for(MaskAuto* current = (MaskAuto*)last;
		current;
		current = (MaskAuto*)PREVIOUS)
	{
		if(current->position <= position)
		{
			begin = current;
			end = NEXT ? (MaskAuto*)NEXT : current;
			break;
		}
	}

// Nothing before position found
	if(!begin)
	{
		begin = end = (MaskAuto*)first;
	}

// Nothing after position found
	if(!begin)
	{
		begin = end = (MaskAuto*)default_auto;
	}


	SubMask *mask1 = begin->get_submask(submask);
	SubMask *mask2 = end->get_submask(submask);

	points->remove_all_objects();
	int total_points = MAX(mask1->points.size(), mask2->points.size());
	for(int i = 0; i < total_points; i++)
	{
		MaskPoint *point = new MaskPoint;
		MaskPoint point1;
		int need_point1 = 1;
		MaskPoint point2;
		int need_point2 = 1;

		if(i < mask1->points.size())
		{
			point1.copy_from(*mask1->points.get(i));
			need_point1 = 0;
		}

		if(i < mask2->points.size())
		{
			point2.copy_from(*mask2->points.get(i));
			need_point2 = 0;
		}

		if(need_point1) point1.copy_from(point2);
		if(need_point2) point2.copy_from(point1);

		avg_points(point,
			&point1,
			&point2,
			position,
			begin->position,
			end->position);
		points->append(point);
	}
}


float MaskAutos::get_feather(int64_t position, int direction)
{
	Auto *begin = 0, *end = 0;
	position = (direction == PLAY_FORWARD) ? position : (position - 1);

	get_prev_auto(position, PLAY_FORWARD, begin, 1);
	get_next_auto(position, PLAY_FORWARD, end, 1);

	double weight = 0.0;
	if(end->position != begin->position)
		weight = (double)(position - begin->position) / (end->position - begin->position);

	return ((MaskAuto*)begin)->feather * (1.0 - weight) + ((MaskAuto*)end)->feather * weight;
}

int MaskAutos::get_value(int64_t position, int direction)
{
	Auto *begin = 0, *end = 0;
	position = (direction == PLAY_FORWARD) ? position : (position - 1);

	get_prev_auto(position, PLAY_FORWARD, begin, 1);
	get_next_auto(position, PLAY_FORWARD, end, 1);

	double weight = 0.0;
	if(end->position != begin->position)
		weight = (double)(position - begin->position) / (end->position - begin->position);

	int result = (int)((double)((MaskAuto*)begin)->value * (1.0 - weight) +
		(double)((MaskAuto*)end)->value * weight + 0.5);
// printf("MaskAutos::get_value %d %d %f %d %f %d\n", __LINE__,
// ((MaskAuto*)begin)->value, 1.0 - weight, ((MaskAuto*)end)->value, weight, result);
	return result;
}


void MaskAutos::avg_points(MaskPoint *output, MaskPoint *input1, MaskPoint *input2,
		int64_t output_position, int64_t position1, int64_t position2)
{
	if(position2 == position1) {
		*output = *input1;
	}
	else {
		float fraction2 = (float)(output_position - position1) / (position2 - position1);
		float fraction1 = 1 - fraction2;
		output->x = input1->x * fraction1 + input2->x * fraction2;
		output->y = input1->y * fraction1 + input2->y * fraction2;
		output->control_x1 = input1->control_x1 * fraction1 + input2->control_x1 * fraction2;
		output->control_y1 = input1->control_y1 * fraction1 + input2->control_y1 * fraction2;
		output->control_x2 = input1->control_x2 * fraction1 + input2->control_x2 * fraction2;
		output->control_y2 = input1->control_y2 * fraction1 + input2->control_y2 * fraction2;
	}

}


Auto* MaskAutos::new_auto()
{
	return new MaskAuto(edl, this);
}

void MaskAutos::dump()
{
	printf("	MaskAutos::dump %p\n", this);
	printf("	Default: position %jd submasks %d\n",
		default_auto->position,
		((MaskAuto*)default_auto)->masks.total);
	((MaskAuto*)default_auto)->dump();
	for(Auto* current = first; current; current = NEXT)
	{
		printf("	position %jd masks %d\n",
			current->position,
			((MaskAuto*)current)->masks.total);
		((MaskAuto*)current)->dump();
	}
}

int MaskAutos::mask_exists(int64_t position, int direction)
{
	Auto *current = 0;
	position = (direction == PLAY_FORWARD) ? position : (position - 1);

	MaskAuto* keyframe = (MaskAuto*)get_prev_auto(position, direction, current);



	for(int i = 0; i < keyframe->masks.total; i++)
	{
		SubMask *mask = keyframe->get_submask(i);
		if(mask->points.total > 1)
			return 1;
	}
	return 0;
}

int MaskAutos::total_submasks(int64_t position, int direction)
{
	position = (direction == PLAY_FORWARD) ? position : (position - 1);
	for(MaskAuto* current = (MaskAuto*)last;
		current;
		current = (MaskAuto*)PREVIOUS)
	{
		if(current->position <= position)
		{
			return current->masks.total;
		}
	}

	return ((MaskAuto*)default_auto)->masks.total;
}


void MaskAutos::translate_masks(float translate_x, float translate_y)
{
	((MaskAuto *)default_auto)->translate_submasks(translate_x, translate_y);
	for(MaskAuto* current = (MaskAuto*)first;
		current;
		current = (MaskAuto*)NEXT)
	{
		current->translate_submasks(translate_x, translate_y);
	}
}

void MaskAutos::set_proxy(int orig_scale, int new_scale)
{
	((MaskAuto *)default_auto)->scale_submasks(orig_scale, new_scale);
	for( MaskAuto* current=(MaskAuto*)first; current; current=(MaskAuto*)NEXT ) {
		current->scale_submasks(orig_scale, new_scale);
	}
}

