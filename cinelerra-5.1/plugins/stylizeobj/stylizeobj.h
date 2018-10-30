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

#ifndef STYLIZEOBJ_H
#define STYLIZEOBJ_H

#include "pluginvclient.h"
#include "loadbalance.h"
#include "mutex.inc"

#include "opencv2/core/types.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/photo.hpp"
#include "opencv2/video/video.hpp"

#include <vector>

using namespace std;
using namespace cv;

enum {
 MODE_EDGE_SMOOTH,
 MODE_EDGE_RECURSIVE,
 MODE_DETAIL_ENHANCE,
 MODE_PENCIL_SKETCH,
 MODE_COLOR_SKETCH,
 MODE_STYLIZATION,
};

class StylizeObjConfig;
class StylizeObj;


class StylizeObjConfig
{
public:
	StylizeObjConfig();
	int mode;
	float smoothing, edge_strength, shade_factor;

	int equivalent(StylizeObjConfig &that);
	void copy_from(StylizeObjConfig &that);
	void interpolate(StylizeObjConfig &prev, StylizeObjConfig &next, 
		long prev_frame, long next_frame, long current_frame);
	void limits();
};

class StylizeObj : public PluginVClient
{
public:
	StylizeObj(PluginServer *server);
	~StylizeObj();
	PLUGIN_CLASS_MEMBERS2(StylizeObjConfig)
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int process_buffer(VFrame *frame, int64_t start_position, double frame_rate);
	void to_mat(Mat &mat, int mcols, int mrows,
		VFrame *inp, int ix,int iy, int mcolor_model);
	void from_mat(VFrame *out, int ox, int oy, int ow, int oh,
		Mat &mat, int mcolor_model);

	int width, height, color_model;
	VFrame *input, *output;

	Mat next_img;
};

#endif
