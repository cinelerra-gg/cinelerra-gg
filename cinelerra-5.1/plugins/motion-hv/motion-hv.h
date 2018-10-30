
/*
 * CINELERRA
 * Copyright (C) 2016 Adam Williams <broadcast at earthling dot net>
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

#ifndef MOTION_H
#define MOTION_H

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "affine.inc"
#include "bchash.inc"
#include "filexml.inc"
#include "keyframe.inc"
#include "loadbalance.h"
#include "motionscan-hv.inc"
#include "motionwindow-hv.inc"
#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"

class MotionHVMain;
class MotionHVWindow;




// Limits of global range in percent
#define MIN_RADIUS 1
#define MAX_RADIUS 100

// Limits of rotation range in degrees
#define MIN_ROTATION 1
#define MAX_ROTATION 25

// Limits of block size in percent.
#define MIN_BLOCK 1
#define MAX_BLOCK 100

// Limits of block count
#define MIN_BLOCKS 1
#define MAX_BLOCKS 200

// Precision of rotation
#define MIN_ANGLE 0.0001


class MotionHVConfig
{
public:
	MotionHVConfig();

	int equivalent(MotionHVConfig &that);
	void copy_from(MotionHVConfig &that);
	void interpolate(MotionHVConfig &prev,
		MotionHVConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
	void boundaries();

	int block_count;
	int global_range_w;
	int global_range_h;
// Range of angles above and below center rotation angle to search
	int rotation_range;
// Center angle of rotation search
	int rotation_center;
	int magnitude;
	int rotate_magnitude;
	int return_speed;
	int rotate_return_speed;
	int draw_vectors;
// Percent of image size
	int global_block_w;
	int global_block_h;
//	int rotation_block_w;
//	int rotation_block_h;
// Number of search positions in each refinement of the log search
//	int global_positions;
//	int rotate_positions;
// Block position in percentage 0 - 100
	double block_x;
	double block_y;

	int horizontal_only;
	int vertical_only;
	int global;
	int rotate;
// Track or stabilize, single pixel, scan only, or nothing
	int action_type;
// Recalculate, no calculate, save, or load coordinates from disk
	int tracking_type;
// Track a single frame, previous frame, or previous frame same block
	int tracking_object;

#if 0
	enum
	{
// action_type
		TRACK,
		STABILIZE,
		TRACK_PIXEL,
		STABILIZE_PIXEL,
		NOTHING,
// mode2
		RECALCULATE,
		SAVE,
		LOAD,
		NO_CALCULATE,
// tracking_object
		TRACK_SINGLE,
		TRACK_PREVIOUS,
		PREVIOUS_SAME_BLOCK
	};
#endif


// Number of single frame to track relative to timeline start
	int64_t track_frame;
// Master layer
	int bottom_is_master;
};




class MotionHVMain : public PluginVClient
{
public:
	MotionHVMain(PluginServer *server);
	~MotionHVMain();

	int process_buffer(VFrame **frame,
		int64_t start_position,
		double frame_rate);
	void process_global();
	void process_rotation();
	void draw_vectors(VFrame *frame);
	int is_multichannel();
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
// Calculate frame to copy from and frame to move
	void calculate_pointers(VFrame **frame, VFrame **src, VFrame **dst);
	void allocate_temp(int w, int h, int color_model);

	PLUGIN_CLASS_MEMBERS2(MotionHVConfig)


	static void draw_pixel(VFrame *frame, int x, int y);
	static void draw_line(VFrame *frame, int x1, int y1, int x2, int y2);
	void draw_arrow(VFrame *frame, int x1, int y1, int x2, int y2);

// Number of the previous reference frame on the timeline.
	int64_t previous_frame_number;
// The frame compared with the previous frame to get the motion.
// It is moved to compensate for motion and copied to the previous_frame.
	VFrame *temp_frame;
	MotionHVScan *engine;
//	RotateScan *motion_rotate;
	OverlayFrame *overlayer;
	AffineEngine *rotate_engine;

// Accumulation of all global tracks since the plugin start.
// Multiplied by OVERSAMPLE.
	int total_dx;
	int total_dy;

// Rotation motion tracking
	float total_angle;

// Current motion vector for drawing vectors
	int current_dx;
	int current_dy;
	float current_angle;



// Oversampled current frame for motion estimation
	int32_t *search_area;
	int search_size;


// The layer to track motion in.
	int reference_layer;
// The layer to apply motion in.
	int target_layer;

// Pointer to the source and destination of each operation.
// These are fully allocated buffers.

// The previous reference frame for global motion tracking
	VFrame *prev_global_ref;
// The current reference frame for global motion tracking
	VFrame *current_global_ref;
// The input target frame for global motion tracking
	VFrame *global_target_src;
// The output target frame for global motion tracking
	VFrame *global_target_dst;

// The previous reference frame for rotation tracking
	VFrame *prev_rotate_ref;
// The current reference frame for rotation tracking
	VFrame *current_rotate_ref;
// The input target frame for rotation tracking.
	VFrame *rotate_target_src;
// The output target frame for rotation tracking.
	VFrame *rotate_target_dst;

// The output of process_buffer
	VFrame *output_frame;
	int w;
	int h;
};


class MotionHvVVFrame : public VFrame
{
public:
	MotionHvVVFrame(VFrame *vfrm, int n);
	int draw_pixel(int x, int y);
	int n;
};










#endif






