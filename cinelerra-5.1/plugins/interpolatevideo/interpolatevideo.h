/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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
 * *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef INTERPOLATEVIDEO_H
#define INTERPOLATEVIDEO_H

/*
 * #include "arraylist.h"
 * #include "bcdisplayinfo.h"
 *  *  *  * #include "keyframe.h"
 *  * #include "vframe.h"
 */

#include "opticflow.inc"
#include "pluginvclient.h"

#include <string.h>
#include <stdint.h>


// In pixels
#define MIN_SEARCH_RADIUS 4
#define MAX_SEARCH_RADIUS 256
#define MIN_MACROBLOCK_SIZE 8
#define MAX_MACROBLOCK_SIZE 256
#define MAX_SEARCH_STEPS 1024
// Enough packages to keep busy if macroblocks turn invalid,
// yet not bog down the mutexes
#define MAX_PACKAGES ((PluginClient::get_project_smp() + 1) * 16)

class InterpolateVideo;
class InterpolateVideoWindow;


class InterpolateVideoConfig
{
public:
	InterpolateVideoConfig();

	void copy_from(InterpolateVideoConfig *config);
	int equivalent(InterpolateVideoConfig *config);

// Frame rate of input
	double input_rate;
// If 1, use the keyframes as beginning and end frames and ignore input rate
	int use_keyframes;
	int optic_flow;
	int draw_vectors;
	int search_radius;
	int macroblock_size;
};




class InterpolateVideo : public PluginVClient
{
public:
	InterpolateVideo(PluginServer *server);
	~InterpolateVideo();

	PLUGIN_CLASS_MEMBERS2(InterpolateVideoConfig)

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	void fill_border(double frame_rate, int64_t start_position);
	void create_macroblocks();
	void draw_vectors(int processed);
	int angles_overlap(float dst2_angle1,
		float dst2_angle2,
		float dst1_angle1,
		float dst1_angle2);
	void blend_macroblock(int number);
	void optic_flow();
	void average();
	void draw_pixel(VFrame *frame, int x, int y);
	void draw_line(VFrame *frame, int x1, int y1, int x2, int y2);
	void draw_arrow(VFrame *frame,
		int x1,
		int y1,
		int x2,
		int y2);
	void draw_rect(VFrame *frame,
		int x1,
		int y1,
		int x2,
		int y2);

	ArrayList<OpticFlowMacroblock*> macroblocks;
	ArrayList<int> invalid_blocks;
	OpticFlow *optic_flow_engine;
	Warp *warp_engine;
	BlendMacroblock *blend_engine;
	int x_macroblocks;
	int y_macroblocks;
// Last motion scanned positions
	int64_t motion_number[2];



// beginning and end frames
	VFrame *frames[2];
// Last requested positions
	int64_t frame_number[2];
	int last_macroblock_size;
	int last_search_radius;
	int total_macroblocks;
// Last output position
	int64_t last_position;
	double last_rate;
	float lowest_fraction;

// Current requested positions
	int64_t range_start;
	int64_t range_end;

// Input rate determined by keyframe mode
	double active_input_rate;
};







#endif


