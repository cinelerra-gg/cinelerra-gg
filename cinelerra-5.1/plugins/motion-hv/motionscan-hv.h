
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

#ifndef MOTIONSCAN_H
#define MOTIONSCAN_H


#include "affine.inc"
#include "loadbalance.h"
#include "motioncache-hv.inc"
#include "vframe.inc"
#include <stdint.h>

class MotionHVScan;

#define OVERSAMPLE 4
#define MOTION_FILE "/tmp/m"
#define ROTATION_FILE "/tmp/r"

class MotionHVScanPackage : public LoadPackage
{
public:
	MotionHVScanPackage();

// For multiple blocks
// Position of stationary block after downsampling
	int block_x1, block_y1, block_x2, block_y2;
// index of rotated frame
	int angle_step;

	int dx;
	int dy;
	int64_t max_difference;
	int64_t min_difference;
	int64_t min_pixel;
	int is_border;
	int valid;
	int64_t difference1;
	int64_t difference2;
// Search position of current package to nearest pixel with downsampling
	int search_x;
	int search_y;
// Subpixel of search position
	int sub_x;
	int sub_y;
};

class MotionHVScanUnit : public LoadClient
{
public:
	MotionHVScanUnit(MotionHVScan *server);
	~MotionHVScanUnit();

	void process_package(LoadPackage *package);
	void subpixel(MotionHVScanPackage *pkg);
	void single_pixel(MotionHVScanPackage *pkg);

	MotionHVScan *server;
};

class MotionHVScan : public LoadServer
{
public:
	MotionHVScan(int total_clients,
		int total_packages);
	~MotionHVScan();

	friend class MotionHVScanUnit;

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
// Test for identical frames before scanning
	void set_test_match(int value);
	void set_cache(MotionHVCache *cache);

// Invoke the motion engine for a search
// Frame before motion
	void scan_frame(VFrame *previous_frame,
		VFrame *current_frame,
		int global_range_w, // in pixels
		int global_range_h,
		int global_block_w, // in pixels
		int global_block_h,
		int block_x, // in pixels
		int block_y,
		int frame_type,
		int tracking_type,
		int action_type,
		int horizontal_only,
		int vertical_only,
		int source_position,
		int total_dx, // in pixels * OVERSAMPLE
		int total_dy,
		int global_origin_x, // in pixels
		int global_origin_y,
		int do_motion,
		int do_rotate,
		double rotation_center, // in deg
		double rotation_range);

	static int64_t abs_diff(unsigned char *prev_ptr,
		unsigned char *current_ptr,
		int row_bytes,
		int w,
		int h,
		int color_model);
	static int64_t abs_diff_sub(unsigned char *prev_ptr,
		unsigned char *current_ptr,
		int row_bytes,
		int w,
		int h,
		int color_model,
		int sub_x,
		int sub_y);


	static void clamp_scan(int w,
		int h,
		int *block_x1,
		int *block_y1,
		int *block_x2,
		int *block_y2,
		int *scan_x1,
		int *scan_y1,
		int *scan_x2,
		int *scan_y2,
		int use_absolute);

// Change between previous frame and current frame multiplied by
// OVERSAMPLE
	int dx_result;
	int dy_result;
	float dr_result;

	enum
	{
// action_type
		TRACK,
		STABILIZE,
		TRACK_PIXEL,
		STABILIZE_PIXEL,
		NOTHING
	};

	enum
	{
// tracking_type
		CALCULATE,
		SAVE,
		LOAD,
		NO_CALCULATE
	};

	enum
	{
// frame_type
		TRACK_SINGLE,
		TRACK_PREVIOUS,
		PREVIOUS_SAME_BLOCK
	};

private:
	void downsample_frame(VFrame *dst,
		VFrame *src,
		int downsample);
	void pixel_search(int &x_result, int &y_result, double &r_result);
	void subpixel_search(int &x_result, int &y_result);
	double step_to_angle(int step, double center);

// 	double calculate_variance(unsigned char *current_ptr,
// 		int row_bytes,
// 		int w,
// 		int h,
// 		int color_model);
	double calculate_range(unsigned char *current_ptr,
		int row_bytes,
		int w,
		int h,
		int color_model);


	MotionHVCache *downsample_cache;
	int shared_downsample;
	AffineEngine *rotater;
// Pointer to downsampled frame before motion
	VFrame *previous_frame;
// Pointer to downsampled frame after motion
	VFrame *current_frame;
// Frames passed from user
	VFrame *previous_frame_arg;
	VFrame *current_frame_arg;
// rotated versions of current_frame
	VFrame **rotated_current;
// allocation of rotated_current array, a copy of angle_steps
	int total_rotated;
// Test for identical frames before processing
// Faster to skip it if the frames are usually different
	int test_match;
	int skip;
// macroblocks didn't have enough data
	int failed;
// For single block
	int block_x1;
	int block_x2;
	int block_y1;
	int block_y2;
	int scan_w;
	int scan_h;
	int scan_x1;
	int scan_y1;
	int scan_x2;
	int scan_y2;
	double scan_angle1, scan_angle2;
	int y_steps;
	int x_steps;
	int angle_steps;
// in deg
	double angle_step;
	int subpixel;
	int horizontal_only;
	int vertical_only;
	int global_origin_x;
	int global_origin_y;
	int action_type;
	int current_downsample;
	int downsampled_w;
	int downsampled_h;
	int total_steps;
	int do_motion;
	int do_rotate;
	int rotation_pass;
// in deg
	double rotation_center;
	double rotation_range;
};



#endif
