
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

#ifndef MASKAUTO_H
#define MASKAUTO_H


#include "arraylist.h"
#include "auto.h"
#include "maskauto.inc"
#include "maskautos.inc"

class MaskPoint
{
public:
	MaskPoint();

	int operator==(MaskPoint& ptr);
	MaskPoint& operator=(MaskPoint& ptr);
	void copy_from(MaskPoint &ptr);

	float x, y;
// Incoming acceleration
	float control_x1, control_y1;
// Outgoing acceleration
	float control_x2, control_y2;
};

class SubMask
{
public:
	SubMask(MaskAuto *keyframe, int no);
	~SubMask();

// Don't use ==
	int operator==(SubMask& ptr);
	int equivalent(SubMask& ptr);
	void copy_from(SubMask& ptr);
	void load(FileXML *file);
	void copy(FileXML *file);
	void dump();

	char name[BCSTRLEN];
	ArrayList<MaskPoint*> points;
	MaskAuto *keyframe;
};

class MaskAuto : public Auto
{
public:
	MaskAuto(EDL *edl, MaskAutos *autos);
	~MaskAuto();

	int operator==(Auto &that);
	int operator==(MaskAuto &that);
	int identical(MaskAuto *src);
	void load(FileXML *file);
	void copy(int64_t start, int64_t end, FileXML *file, int default_auto);
	void copy_from(Auto *src);
	int interpolate_from(Auto *a1, Auto *a2, int64_t position, Auto *templ=0);
	void copy_from(MaskAuto *src);
// Copy data but not position
	void copy_data(MaskAuto *src);
	void get_points(ArrayList<MaskPoint*> *points,
		int submask);
	void set_points(ArrayList<MaskPoint*> *points,
		int submask);

// Copy parameters to this which differ between ref & src
	void update_parameter(MaskAuto *ref, MaskAuto *src);

	void dump();
// Retrieve submask with clamping
	SubMask* get_submask(int number);
// Translates all submasks
	void translate_submasks(float translate_x, float translate_y);
// scale all submasks
	void scale_submasks(int orig_scale, int new_scale);


	ArrayList<SubMask*> masks;
// These are constant for the entire track
	int mode;
	float feather;
// 0 - 100
	int value;
	int apply_before_plugins;
	int disable_opengl_masking;
};




#endif
