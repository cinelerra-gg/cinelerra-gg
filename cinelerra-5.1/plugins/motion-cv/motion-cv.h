
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
#include "bchash.inc"
#include "filexml.inc"
#include "keyframe.inc"
#include "loadbalance.h"
#include "overlayframe.inc"
#include "pluginvclient.h"
#include "rotateframe.inc"
#include "vframe.inc"

class MotionCVMain;
class MotionCVWindow;
class MotionCVScan;
class RotateCVScan;


#define OVERSAMPLE 4


// Limits of global range in percent
#define MIN_RADIUS 1
#define MAX_RADIUS 50

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

#define TRACKING_FILE "/tmp/motion-cv"

class MotionCVConfig
{
public:
	MotionCVConfig();

	int equivalent(MotionCVConfig &that);
	void copy_from(MotionCVConfig &that);
	void interpolate(MotionCVConfig &prev, MotionCVConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame);
	void boundaries();

	int block_count;
	int global_range_w;
	int global_range_h;
	int rotation_range;
	int magnitude;
	int return_speed;
	int draw_vectors;
// Percent of image size
	int global_block_w;
	int global_block_h;
	int rotation_block_w;
	int rotation_block_h;
// Number of search positions in each refinement of the log search
	int global_positions;
	int rotate_positions;
// Block position in percentage 0 - 100
	double block_x;
	double block_y;

	int horizontal_only;
	int vertical_only;
	int global;
	int rotate;
	int addtrackedframeoffset;
	char tracking_file[BCTEXTLEN];
// Track or stabilize, single pixel, scan only, or nothing
	int mode1;
// Recalculate, no calculate, save, or load coordinates from disk
	int mode2;
// Track a single frame, previous frame, or previous frame same block
	int mode3;
	enum
	{
// mode1
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
// mode3
		TRACK_SINGLE,
		TRACK_PREVIOUS,
		PREVIOUS_SAME_BLOCK
	};
// Number of single frame to track relative to timeline start
	int64_t track_frame;
// Master layer
	int bottom_is_master;
};




class MotionCVMain : public PluginVClient
{
public:
	MotionCVMain(PluginServer *server);
	~MotionCVMain();

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

	PLUGIN_CLASS_MEMBERS2(MotionCVConfig)


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
	static void draw_pixel(VFrame *frame, int x, int y);
	static void draw_line(VFrame *frame, int x1, int y1, int x2, int y2);
	void draw_arrow(VFrame *frame, int x1, int y1, int x2, int y2);

// Number of the previous reference frame on the timeline.
	int64_t previous_frame_number;
// The frame compared with the previous frame to get the motion.
// It is moved to compensate for motion and copied to the previous_frame.
	VFrame *temp_frame;
	MotionCVScan *engine;
	RotateCVScan *motion_rotate;
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

	char cache_file[BCTEXTLEN];
	FILE *cache_fp, *active_fp;
	void reset_cache_file();
	int open_cache_file();
	void close_cache_file();
	int load_cache_line();
	int locate_cache_line(int64_t key);
	int get_cache_line(int64_t key);
	int put_cache_line(const char *line);
	char cache_line[BCSTRLEN];
	int64_t cache_key, active_key;
// add constant frame offset values
	int dx_offset, dy_offset;
	int64_t tracking_frame;
// save/load result values
	int load_ok;
	int save_dx, load_dx;
	int save_dy, load_dy;
	float save_dt, load_dt;
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



class MotionCvVVFrame : public VFrame
{
public:
	MotionCvVVFrame(VFrame *vfrm, int n);
	int draw_pixel(int x, int y);
	int n;
};























class MotionCVScanPackage : public LoadPackage
{
public:
	MotionCVScanPackage();

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

class MotionCVScanCache
{
public:
	MotionCVScanCache(int x, int y, int64_t difference);
	int x, y;
	int64_t difference;
};

class MotionCVScanUnit : public LoadClient
{
public:
	MotionCVScanUnit(MotionCVScan *server, MotionCVMain *plugin);
	~MotionCVScanUnit();

	void process_package(LoadPackage *package);
	int64_t get_cache(int x, int y);
	void put_cache(int x, int y, int64_t difference);

	MotionCVScan *server;
	MotionCVMain *plugin;

	ArrayList<MotionCVScanCache*> cache;
	Mutex *cache_lock;
};

class MotionCVScan : public LoadServer
{
public:
	MotionCVScan(MotionCVMain *plugin,
		int total_clients,
		int total_packages);
	~MotionCVScan();

	friend class MotionCVScanUnit;

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

// Invoke the motion engine for a search
// Frame before motion
	void scan_frame(VFrame *previous_frame,
// Frame after motion
		VFrame *current_frame);
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
	MotionCVMain *plugin;

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
	int subpixel;


	ArrayList<MotionCVScanCache*> cache;
	Mutex *cache_lock;
};













class RotateCVScanPackage : public LoadPackage
{
public:
	RotateCVScanPackage();
	float angle;
	int64_t difference;
};

class RotateCVScanCache
{
public:
	RotateCVScanCache(float angle, int64_t difference);
	float angle;
	int64_t difference;
};

class RotateCVScanUnit : public LoadClient
{
public:
	RotateCVScanUnit(RotateCVScan *server, MotionCVMain *plugin);
	~RotateCVScanUnit();

	void process_package(LoadPackage *package);

	RotateCVScan *server;
	MotionCVMain *plugin;
	AffineEngine *rotater;
	VFrame *temp;
};

class RotateCVScan : public LoadServer
{
public:
	RotateCVScan(MotionCVMain *plugin,
		int total_clients,
		int total_packages);
	~RotateCVScan();

	friend class RotateCVScanUnit;

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

// Invoke the motion engine for a search
// Frame before rotation
	float scan_frame(VFrame *previous_frame,
// Frame after rotation
		VFrame *current_frame,
// Pivot
		int block_x,
		int block_y);
	int64_t get_cache(float angle);
	void put_cache(float angle, int64_t difference);


// Angle result
	float result;

private:
	VFrame *previous_frame;
// Frame after motion
	VFrame *current_frame;

	MotionCVMain *plugin;
	int skip;

// Pivot
	int block_x;
	int block_y;
// Block to rotate
	int block_x1;
	int block_x2;
	int block_y1;
	int block_y2;
// Area to compare
	int scan_x;
	int scan_y;
	int scan_w;
	int scan_h;
// Range of angles to compare
	float scan_angle1, scan_angle2;
	int total_steps;

	ArrayList<RotateCVScanCache*> cache;
	Mutex *cache_lock;
};




#endif






