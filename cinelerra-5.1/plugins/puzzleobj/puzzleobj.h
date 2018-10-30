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

#ifndef PUZZLEOBJ_H
#define PUZZLEOBJ_H

#include "pluginvclient.h"
#include "loadbalance.h"
#include "mutex.inc"

#include "opencv2/core/types.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/photo.hpp"
#include "opencv2/ximgproc.hpp"
#include "opencv2/video/video.hpp"

#include <vector>


using namespace std;
using namespace cv;
using namespace cv::ximgproc;

class PuzzleObjConfig;
class PuzzleObj;


class PuzzleObjConfig
{
public:
	PuzzleObjConfig();
	int pixels;
	int iterations;

	int equivalent(PuzzleObjConfig &that);
	void copy_from(PuzzleObjConfig &that);
	void interpolate(PuzzleObjConfig &prev, PuzzleObjConfig &next, 
		long prev_frame, long next_frame, long current_frame);
	void limits();
};

class PuzzleObj : public PluginVClient
{
public:
	PuzzleObj(PluginServer *server);
	~PuzzleObj();
	PLUGIN_CLASS_MEMBERS2(PuzzleObjConfig)
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int process_buffer(VFrame *frame, int64_t start_position, double frame_rate);
	void to_mat(Mat &mat, int mcols, int mrows,
		VFrame *inp, int ix,int iy, int mcolor_model);

	int width, height, color_model;
	VFrame *input, *output;

	int ss_width, ss_height, ss_channels;
	int ss_pixels, ss_levels, ss_prior, ss_hist_bins;

	Ptr<SuperpixelSEEDS> sseeds;
	Mat next_img;
};

#endif
