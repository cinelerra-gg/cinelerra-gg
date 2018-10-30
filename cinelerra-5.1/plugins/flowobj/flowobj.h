/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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
 */

#ifndef FLOWOBJ_H
#define FLOWOBJ_H

#include "pluginvclient.h"
#include "loadbalance.h"
#include "overlayframe.h"

#include "opencv2/core/types.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/video/video.hpp"

#include "opencv2/core/types_c.h"
#include <opencv2/calib3d/calib3d_c.h>

#include <vector>

using namespace std;
using namespace cv;

//#define _RANSAC
#ifdef _RANSAC
#include <opencv2/videostab/motion_core.hpp>
#include <opencv2/videostab/global_motion.hpp>

using namespace videostab;
#endif

class FlowObjConfig;
class FlowObjRemapPackage;
class FlowObjRemapEngine;
class FlowObjRemapUnit;
class FlowObj;


class FlowObjConfig
{
public:
	FlowObjConfig();

	int equivalent(FlowObjConfig &that);
	void copy_from(FlowObjConfig &that);
	void interpolate(FlowObjConfig &prev, FlowObjConfig &next, 
		long prev_frame, long next_frame, long current_frame);
	void limits();
	int draw_vectors;
	int do_stabilization;
	int settling_speed;
// percent
	int block_size;
	int search_radius;
};

class FlowObjRemapPackage : public LoadPackage
{
public:
	FlowObjRemapPackage();

	int row1, row2;
};

class FlowObjRemapEngine : public LoadServer
{
public:
	FlowObjRemapEngine(FlowObj *plugin, int cpus);
	~FlowObjRemapEngine();

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	FlowObj *plugin;
};

class FlowObjRemapUnit : public LoadClient
{
public:
	FlowObjRemapUnit(FlowObjRemapEngine *engine, FlowObj *plugin);
	~FlowObjRemapUnit();

        void process_package(LoadPackage *pkg);
        void process_remap(FlowObjRemapPackage *pkg);

        FlowObjRemapEngine *engine;
        FlowObj *plugin;
};

class FlowObj : public PluginVClient
{
public:
	FlowObj(PluginServer *server);
	~FlowObj();
// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS2(FlowObjConfig)
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int process_buffer(VFrame *frame, int64_t start_position, double frame_rate);
	void to_mat(Mat &mat, int mcols, int mrows,
		VFrame *inp, int ix,int iy, int mcolor_model);
	void from_mat(VFrame *out, int ox, int oy, int ow, int oh,
		Mat &mat, int mcolor_model);

	long prev_position, next_position;
	int width, height, color_model;
	VFrame *input, *output, *accum;

	Mat *flow_mat;
        FlowObjRemapEngine *flow_engine;
	OverlayFrame *overlay;

//opencv
        typedef vector<Point2f> ptV;

        BC_CModel cvmodel;
	Mat accum_matrix;
	double accum_matrix_mem[9];
	double x_accum, y_accum, angle_accum;
	Mat prev_mat, next_mat;
	ptV *prev_corners, *next_corners;
};

#endif
