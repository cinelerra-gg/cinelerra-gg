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

#ifndef MOVEOBJ_H
#define MOVEOBJ_H

#include "affine.inc"
#include "pluginvclient.h"

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

class MoveObjConfig
{
public:
	MoveObjConfig();

	int equivalent(MoveObjConfig &that);
	void copy_from(MoveObjConfig &that);
	void interpolate(MoveObjConfig &prev, MoveObjConfig &next, 
		long prev_frame, long next_frame, long current_frame);
	void limits();
	int draw_vectors;
	int do_stabilization;
	int settling_speed;
// percent
	int block_size;
	int search_radius;
};

class MoveObj : public PluginVClient
{
public:
	MoveObj(PluginServer *server);
	~MoveObj();
// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS2(MoveObjConfig)
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int process_buffer(VFrame *frame, int64_t start_position, double frame_rate);
	AffineEngine *affine;
	void to_mat(Mat &mat, int mcols, int mrows,
		VFrame *inp, int ix,int iy, int mcolor_model);

	long prev_position, next_position;

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
