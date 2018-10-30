
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

#ifndef MASKENGINE_H
#define MASKENGINE_H


#include "condition.inc"
#include "loadbalance.h"
#include "maskautos.inc"
#include "maskauto.inc"
#include "mutex.inc"
#include "vframe.inc"


class MaskEngine;

// Values for step
enum
{
	DO_MASK,
	DO_X_FEATHER,
	DO_Y_FEATHER,
	DO_APPLY
};

class MaskPackage : public LoadPackage
{
public:
	MaskPackage();
	~MaskPackage();

	int start_x, end_x, start_y, end_y;
};

class MaskUnit : public LoadClient
{
public:
	MaskUnit(MaskEngine *engine);
	~MaskUnit();

	void process_package(LoadPackage *package);
	void draw_line_clamped(VFrame *frame,
		int x1, int y1, int x2, int y2, unsigned char value);
	void do_feather(VFrame *output, VFrame *input,
		double feather, int start_y, int end_y, int start_x, int end_x);
	void blur_strip(double *val_p, double *val_m,
		double *dst, double *src, int size, int max);

	double n_p[5], n_m[5];
	double d_p[5], d_m[5];
	double bd_p[5], bd_m[5];

	MaskEngine *engine;
	VFrame *temp;
};


class MaskEngine : public LoadServer
{
public:
	MaskEngine(int cpus);
	~MaskEngine();


	void do_mask(VFrame *output,
// Position relative to project, compensated for playback direction
		int64_t start_position_project,
		MaskAutos *keyframe_set,
		MaskAuto *keyframe,
		MaskAuto *default_auto);
	int points_equivalent(ArrayList<MaskPoint*> *new_points,
		ArrayList<MaskPoint*> *points);

	void delete_packages();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	VFrame *output;
// State of last mask
	VFrame *mask;
// Temporary for feathering
	VFrame *temp_mask;
	ArrayList<ArrayList<MaskPoint*>*> point_sets;
	int mode;
	int step;
	double feather;
	int recalculate;
	int value;
};



#endif
