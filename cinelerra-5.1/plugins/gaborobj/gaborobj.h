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

#ifndef GABOROBJ_H
#define GABOROBJ_H

#include "pluginvclient.h"
#include "loadbalance.h"
#include "mutex.inc"

#include "opencv2/core/types.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/video/video.hpp"

#include <vector>

using namespace std;
using namespace cv;


class GaborObjConfig;
class GaborObj;


class GaborObjConfig
{
public:
	GaborObjConfig();

	int equivalent(GaborObjConfig &that);
	void copy_from(GaborObjConfig &that);
	void interpolate(GaborObjConfig &prev, GaborObjConfig &next, 
		long prev_frame, long next_frame, long current_frame);
	void limits();
};

class GaborObjFilterPackage : public LoadPackage
{
public:
	GaborObjFilterPackage();
	int i;
};

class GaborObjFilterEngine : public LoadServer
{
public:
	GaborObjFilterEngine(GaborObj *plugin, int cpus);
	~GaborObjFilterEngine();

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	GaborObj *plugin;
};

class GaborObjFilterUnit : public LoadClient
{
public:
	GaborObjFilterUnit(GaborObjFilterEngine *engine, GaborObj *plugin);
	~GaborObjFilterUnit();

        void process_package(LoadPackage *pkg);
        void process_filter(GaborObjFilterPackage *pkg);

        GaborObjFilterEngine *engine;
        GaborObj *plugin;
};

class GaborObj : public PluginVClient
{
public:
	GaborObj(PluginServer *server);
	~GaborObj();
	PLUGIN_CLASS_MEMBERS2(GaborObjConfig)
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
	VFrame *input, *output, *accum;
	::Mutex *remap_lock;

	Mat next_img;
	vector<Mat> filters;
	GaborObjFilterEngine *gabor_engine;
};

#endif
