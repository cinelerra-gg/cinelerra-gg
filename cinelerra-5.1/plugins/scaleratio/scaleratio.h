
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

#ifndef TRANSLATE_H
#define TRANSLATE_H

// the simplest plugin possible

class ScaleRatioMain;

#include "bchash.h"
#include "mutex.h"
#include "scaleratiowin.h"
#include "overlayframe.h"
#include "pluginvclient.h"

class ScaleRatioConfig
{
public:
	ScaleRatioConfig();
	int equivalent(ScaleRatioConfig &that);
	void copy_from(ScaleRatioConfig &that);
	void interpolate(ScaleRatioConfig &prev, ScaleRatioConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame);

	int type;
	float in_w, in_h, in_r;
	float out_w, out_h, out_r;
	float src_x, src_y, src_w, src_h;
	float dst_x, dst_y, dst_w, dst_h;
};


class ScaleRatioMain : public PluginVClient
{
public:
	ScaleRatioMain(PluginServer *server);
	~ScaleRatioMain();

	PLUGIN_CLASS_MEMBERS(ScaleRatioConfig)
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int handle_opengl();

	OverlayFrame *overlayer;
	VFrame *temp_frame;
};


#endif
