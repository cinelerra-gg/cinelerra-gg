
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

#ifndef MOTIONSCAN_H
#define MOTIONSCAN_H


#include "arraylist.h"
//#include "../downsample/downsampleengine.inc"
#include "loadbalance.h"
#include "vframe.inc"
#include <stdint.h>

class MotionScan;

#define OVERSAMPLE 4

class MotionScanPackage : public LoadPackage
{
public:
	MotionScanPackage();

// For multiple blocks
// Position of stationary block
	int block_x1, block_y1, block_x2, block_y2;
// Range of positions to scan
	int scan_x1, scan_y1, scan_x2, scan_y2;
	int dx;
	int dy;
	int64_t max_difference;
	int64_t min_difference;
	int64_t min_pixel;
	int is_border;
	int valid;
// For single block
	int step;
	int64_t difference1;
	int64_t difference2;
// Search position to nearest pixel
	int search_x;
	int search_y;
// Subpixel of search position
	int sub_x;
	int sub_y;
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
	MotionScanUnit(MotionScan *server);
	~MotionScanUnit();

	void process_package(LoadPackage *package);
	int64_t get_cache(int x, int y);
	void put_cache(int x, int y, int64_t difference);

	MotionScan *server;

	ArrayList<MotionScanCache*> cache;
	Mutex *cache_lock;
};

class MotionScan : public LoadServer
{
public:
	MotionScan(int total_clients,
		int total_packages);
	~MotionScan();

	friend class MotionScanUnit;

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
// Test for identical frames before scanning
	void set_test_match(int value);

// Invoke the motion engine for a search
	void scan_frame(VFrame *previous_frame, // Frame before motion
		VFrame *current_frame, // Frame after motion
		int global_range_w, int global_range_h, int global_block_w, int global_block_h,
		double block_x, double block_y, int frame_type, int tracking_type, int action_type,
		int horizontal_only, int vertical_only, int source_position, int total_steps,
		int total_dx, int total_dy, int global_origin_x, int global_origin_y,
		int load_ok=0, int load_dx=0, int load_dy=0);
	int64_t get_cache(int x, int y);
	void put_cache(int x, int y, int64_t difference);

	static int64_t abs_diff(unsigned char *prev_ptr, unsigned char *current_ptr,
		int row_bytes, int w, int h, int color_model);
	static int64_t abs_diff_sub(unsigned char *prev_ptr, unsigned char *current_ptr,
		int row_bytes, int w, int h, int color_model, int sub_x, int sub_y);

	static void clamp_scan(int w, int h,
		int *block_x1, int *block_y1, int *block_x2, int *block_y2,
		int *scan_x1, int *scan_y1, int *scan_x2, int *scan_y2,
		int use_absolute);

// Change between previous frame and current frame multiplied by
// OVERSAMPLE
	int dx_result, dy_result;

	enum { // action_type
		TRACK,
		STABILIZE,
		TRACK_PIXEL,
		STABILIZE_PIXEL,
		NOTHING
	};

	enum { // tracking_type
		CALCULATE,
		SAVE,
		LOAD,
		NO_CALCULATE
	};

	enum { // frame_type
		TRACK_SINGLE,
		TRACK_PREVIOUS,
		PREVIOUS_SAME_BLOCK
	};

private:
// Pointer to downsampled frame before motion
	VFrame *previous_frame;
// Pointer to downsampled frame after motion
	VFrame *current_frame;
// Frames passed from user
	VFrame *previous_frame_arg;
	VFrame *current_frame_arg;
// Downsampled frames
	VFrame *downsampled_previous;
	VFrame *downsampled_current;
// Test for identical frames before processing
// Faster to skip it if the frames are usually different
	int test_match;
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
	int edge_steps;
	int y_steps;
	int x_steps;
	int subpixel;
	int horizontal_only;
	int vertical_only;
	int global_origin_x;
	int global_origin_y;

	ArrayList<MotionScanCache*> cache;
	Mutex *cache_lock;
//	DownSampleServer *downsample;
};



#endif
