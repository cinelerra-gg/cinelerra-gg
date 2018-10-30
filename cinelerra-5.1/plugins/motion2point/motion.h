
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

#ifndef MOTION_H
#define MOTION_H

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "affine.inc"
#include "motionscan-hv.inc"
#include "bchash.inc"
#include "filexml.inc"
#include "keyframe.inc"
#include "loadbalance.h"
#include "motionwindow.inc"
#include "overlayframe.inc"
#include "pluginvclient.h"
#include "rotateframe.inc"
#include "vframe.inc"

class MotionMain2;
class MotionWindow;
class RotateScan;


#define OVERSAMPLE 4
#define TOTAL_POINTS 2
// Point 0 is used for determining translation
#define TRANSLATION_POINT 0
// Point 1 is used for determining rotation and scaling
#define ROTATION_POINT 1

// Limits of global range in percent
#define MIN_RADIUS 1
#define MAX_RADIUS 50

// Limits of global origin in percent
#define MIN_ORIGIN -50
#define MAX_ORIGIN 50

// Limits of rotation range in degrees
#define MIN_ROTATION 1
#define MAX_ROTATION 25

// Limits of block size in percent.
#define MIN_BLOCK 1
#define MAX_BLOCK 100

// Limits of block count
#define MIN_BLOCKS 1
#define MAX_BLOCKS 200

#define MOTION_FILE "/tmp/m"
#define ROTATION_FILE "/tmp/r"

class MotionConfig
{
public:
	MotionConfig();

	int equivalent(MotionConfig &that);
	void copy_from(MotionConfig &that);
	void interpolate(MotionConfig &prev,
		MotionConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
	void boundaries();

// Enable each point
	int global[TOTAL_POINTS];
// Search range 1-100
	int global_range_w[TOTAL_POINTS];
	int global_range_h[TOTAL_POINTS];
// Search origin relative to block position -50-50
	int global_origin_x[TOTAL_POINTS];
	int global_origin_y[TOTAL_POINTS];
	int draw_vectors[TOTAL_POINTS];
// Size of comparison block.  Percent of image size
	int global_block_w[TOTAL_POINTS];
	int global_block_h[TOTAL_POINTS];
// Block position in percentage 0 - 100
	double block_x[TOTAL_POINTS];
	double block_y[TOTAL_POINTS];

// Number of search positions in each refinement of the log search
	int global_positions;
// Maximum position offset
	int magnitude;
	int return_speed;
// Track single direction only
	int horizontal_only;
	int vertical_only;
// Number of single frame to track relative to timeline start
	int64_t track_frame;
// Master layer
	int bottom_is_master;
// Track or stabilize, single pixel, scan only, or nothing
	int action;
// Recalculate, no calculate, save, or load coordinates from disk
	int calculation;
// Track a single frame, previous frame, or previous frame same block
	int tracking_object;
};




class MotionMain2 : public PluginVClient
{
public:
	MotionMain2(PluginServer *server);
	~MotionMain2();

	int process_buffer(VFrame **frame,
		int64_t start_position,
		double frame_rate);
	void scan_motion(int point);
	void apply_motion();
	void draw_vectors(VFrame *frame, int point);
	int is_multichannel();
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
// Calculate frame to copy from and frame to move
	void calculate_pointers(VFrame **frame, VFrame **src, VFrame **dst);
	void allocate_temp(int w, int h, int color_model);

	PLUGIN_CLASS_MEMBERS(MotionConfig)

	int64_t abs_diff(unsigned char *prev_ptr,
		unsigned char *current_ptr,
		int row_bytes,
		int w,
		int h,
		int color_model);
	int64_t abs_diff_sub(unsigned char *prev_ptr,
		unsigned char *current_ptr,
		int row_bytes,
		int w,
		int h,
		int color_model,
		int sub_x,
		int sub_y);

	static void draw_pixel(VFrame *frame, int x, int y);
	static void draw_line(VFrame *frame, int x1, int y1, int x2, int y2);
	void draw_arrow(VFrame *frame, int x1, int y1, int x2, int y2);
// Get start and end of current motion vector in pixels
	void get_current_vector(float *origin_x,
		float *origin_y,
		float *current_x1,
		float *current_y1,
		float *current_x2,
		float *current_y2,
		int point);

// Number of the previous reference frame on the timeline.
	int64_t previous_frame_number;
// The frame compared with the previous frame to get the motion.
// It is moved to compensate for motion and copied to the previous_frame.
	VFrame *temp_frame;
	MotionScan *engine;
	OverlayFrame *overlayer;
	AffineEngine *affine;

// Accumulation of all global tracks since the plugin start.
// Multiplied by OVERSAMPLE.
	int total_dx[TOTAL_POINTS];
	int total_dy[TOTAL_POINTS];



// Current motion vector for drawing vectors
	int current_dx[TOTAL_POINTS];
	int current_dy[TOTAL_POINTS];



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

// The output of process_buffer
	VFrame *output_frame;
	int w;
	int h;
};



class Motion2VVFrame : public VFrame
{
public:
	Motion2VVFrame(VFrame *vfrm, int n);
	int draw_pixel(int x, int y);
	int n;
};





















#if 0


class MotionScanPackage : public LoadPackage
{
public:
	MotionScanPackage();

// For multiple blocks
	int block_x1, block_y1, block_x2, block_y2;
	int scan_x1, scan_y1, scan_x2, scan_y2;
	int dx;
	int dy;
	int64_t max_difference;
	int64_t min_difference;
	int64_t min_pixel;
	int is_border;
	int valid;
// For single block
	int pixel;
	int64_t difference1;
	int64_t difference2;
};

class MotionScanCache
{
public:
	MotionScanCache(int x, int y, int64_t difference);
	int x, y;
	int64_t difference;
};

class MotionScanUnit : public LoadClient
{
public:
	MotionScanUnit(MotionScan *server, MotionMain2 *plugin);
	~MotionScanUnit();

	void process_package(LoadPackage *package);
	int64_t get_cache(int x, int y);
	void put_cache(int x, int y, int64_t difference);

	MotionScan *server;
	MotionMain2 *plugin;

	ArrayList<MotionScanCache*> cache;
	Mutex *cache_lock;
};

class MotionScan : public LoadServer
{
public:
	MotionScan(MotionMain2 *plugin,
		int total_clients,
		int total_packages);
	~MotionScan();

	friend class MotionScanUnit;

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

// Invoke the motion engine for a search
// Frame before motion
	void scan_frame(VFrame *previous_frame,
// Frame after motion
		VFrame *current_frame,
// Motion point
		int point);
	int64_t get_cache(int x, int y);
	void put_cache(int x, int y, int64_t difference);

// Change between previous frame and current frame multiplied by
// OVERSAMPLE
	int dx_result;
	int dy_result;

private:
	VFrame *previous_frame;
// Frame after motion
	VFrame *current_frame;
	MotionMain2 *plugin;
	int skip;
// For single block
	int block_x1;
	int block_x2;
	int block_y1;
	int block_y2;
	int scan_x1;
	int scan_y1;
	int scan_x2;
	int scan_y2;
	int total_pixels;
	int total_steps;
	int point;
	int subpixel;


	ArrayList<MotionScanCache*> cache;
	Mutex *cache_lock;
};

#endif // 0












#endif






