
/*
 * CINELERRA
 * Copyright (C) 2012 Adam Williams <broadcast at earthling dot net>
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

#include "clip.h"
//#include "../downsample/downsampleengine.h"
//#include "motion.h"
#include "motionscan.h"
#include "mutex.h"
#include "vframe.h"

#include <math.h>

// The module which does the actual scanning

MotionScanPackage::MotionScanPackage()
 : LoadPackage()
{
	valid = 1;
}

MotionScanUnit::MotionScanUnit(MotionScan *server)
 : LoadClient(server)
{
	this->server = server;
	cache_lock = new Mutex("MotionScanUnit::cache_lock");
}

MotionScanUnit::~MotionScanUnit()
{
	delete cache_lock;
}

void MotionScanUnit::process_package(LoadPackage *package)
{
	MotionScanPackage *pkg = (MotionScanPackage*)package;
	//int w = server->current_frame->get_w();
	//int h = server->current_frame->get_h();
	int color_model = server->current_frame->get_color_model();
	int pixel_size = BC_CModels::calculate_pixelsize(color_model);
	int row_bytes = server->current_frame->get_bytes_per_line();

// Single pixel
	if( !server->subpixel ) {
// Try cache
		pkg->difference1 = server->get_cache(pkg->search_x, pkg->search_y);
		if( pkg->difference1 < 0 ) {
//printf("MotionScanUnit::process_package 1 search_x=%d search_y=%d"
// " scan_x1=%d scan_y1=%d scan_x2=%d scan_y2=%d x_steps=%d y_steps=%d\n",
// pkg->search_x, pkg->search_y, pkg->scan_x1, pkg->scan_y1, pkg->scan_x2, pkg->scan_y2,
// server->x_steps, server->y_steps);
// Pointers to first pixel in each block
			unsigned char *prev_ptr =
			    server->previous_frame->get_rows()[pkg->search_y] +
				pkg->search_x * pixel_size;
			unsigned char *current_ptr =
			    server->current_frame->get_rows()[pkg->block_y1] +
				pkg->block_x1 * pixel_size;
// Scan block
			pkg->difference1 = MotionScan::abs_diff(prev_ptr, current_ptr, row_bytes,
				pkg->block_x2 - pkg->block_x1, pkg->block_y2 - pkg->block_y1,
				color_model);

// printf("MotionScanUnit::process_package %d search_x=%d search_y=%d diff=%lld\n",
// __LINE__, server->block_x1 - pkg->search_x, server->block_y1 - pkg->search_y, pkg->difference1);
			server->put_cache(pkg->search_x, pkg->search_y, pkg->difference1);
		}
	}
// Sub pixel
	else {
		unsigned char *prev_ptr =
		    server->previous_frame->get_rows()[pkg->search_y] +
			pkg->search_x * pixel_size;
		unsigned char *current_ptr =
		    server->current_frame->get_rows()[pkg->block_y1] +
			pkg->block_x1 * pixel_size;

// With subpixel, there are two ways to compare each position, one by shifting
// the previous frame and two by shifting the current frame.
		pkg->difference1 = MotionScan::abs_diff_sub(prev_ptr, current_ptr, row_bytes,
			pkg->block_x2 - pkg->block_x1, pkg->block_y2 - pkg->block_y1,
			color_model, pkg->sub_x, pkg->sub_y);
		pkg->difference2 =
		    MotionScan::abs_diff_sub(current_ptr, prev_ptr, row_bytes,
			pkg->block_x2 - pkg->block_x1, pkg->block_y2 - pkg->block_y1,
			color_model, pkg->sub_x, pkg->sub_y);
//printf("MotionScanUnit::process_package sub_x=%d sub_y=%d search_x=%d search_y=%d diff1=%lld diff2=%lld\n",
// sub_x, sub_y, search_x, search_y, pkg->difference1, pkg->difference2);
	}
}

int64_t MotionScanUnit::get_cache(int x, int y)
{
	int64_t result = -1;
	cache_lock->lock("MotionScanUnit::get_cache");
	for( int i = 0; i < cache.total; i++ ) {
		MotionScanCache *ptr = cache.values[i];
		if( ptr->x == x && ptr->y == y ) {
			result = ptr->difference;
			break;
		}
	}
	cache_lock->unlock();
	return result;
}

void MotionScanUnit::put_cache(int x, int y, int64_t difference)
{
	MotionScanCache *ptr = new MotionScanCache(x, y, difference);
	cache_lock->lock("MotionScanUnit::put_cache");
	cache.append(ptr);
	cache_lock->unlock();
}

MotionScan::MotionScan(int total_clients, int total_packages)
 : LoadServer( //1, 1
		total_clients, total_packages)
{
	test_match = 1;
	cache_lock = new Mutex("MotionScan::cache_lock");
	downsampled_previous = 0;
	downsampled_current = 0;
//	downsample = 0;
}

MotionScan::~MotionScan()
{
	delete cache_lock;
	delete downsampled_previous;
	delete downsampled_current;
//	delete downsample;
}

void MotionScan::init_packages()
{
// Set package coords
//printf("MotionScan::init_packages %d %d\n", __LINE__, get_total_packages());
	for( int i = 0; i < get_total_packages(); i++ ) {
		MotionScanPackage *pkg = (MotionScanPackage*)get_package(i);

		pkg->block_x1 = block_x1; pkg->block_x2 = block_x2;
		pkg->block_y1 = block_y1; pkg->block_y2 = block_y2;
		pkg->scan_x1 = scan_x1;   pkg->scan_x2 = scan_x2;
		pkg->scan_y1 = scan_y1;   pkg->scan_y2 = scan_y2;
		pkg->difference1 = 0;     pkg->difference2 = 0;
		pkg->step = i;            pkg->valid = 1;
		pkg->dx = pkg->dy = 0;

		if( !subpixel ) {
			pkg->search_x = pkg->scan_x1 +
				(pkg->step % x_steps) * (scan_x2 - scan_x1) / x_steps;
			pkg->search_y = pkg->scan_y1 +
				(pkg->step / x_steps) * (scan_y2 - scan_y1) / y_steps;
			pkg->sub_x = pkg->sub_y = 0;
		}
		else {
			pkg->sub_x = pkg->step % (OVERSAMPLE * 2);
			pkg->sub_y = pkg->step / (OVERSAMPLE * 2);

			if( horizontal_only ) pkg->sub_y = 0;
			if( vertical_only ) pkg->sub_x = 0;

			pkg->search_x = pkg->scan_x1 + pkg->sub_x / OVERSAMPLE + 1;
			pkg->search_y = pkg->scan_y1 + pkg->sub_y / OVERSAMPLE + 1;
			pkg->sub_x %= OVERSAMPLE;
			pkg->sub_y %= OVERSAMPLE;

// printf("MotionScan::init_packages %d i=%d search_x=%d search_y=%d sub_x=%d sub_y=%d\n",
// __LINE__, i, pkg->search_x, pkg->search_y, pkg->sub_x, pkg->sub_y);
		}

// printf("MotionScan::init_packages %d %d,%d %d,%d %d,%d\n",
// __LINE__, scan_x1, scan_x2, scan_y1, scan_y2, pkg->search_x, pkg->search_y);
	}
}

LoadClient* MotionScan::new_client()
{
	return new MotionScanUnit(this);
}

LoadPackage* MotionScan::new_package()
{
	return new MotionScanPackage;
}

void MotionScan::set_test_match(int value)
{
	this->test_match = value;
}

void MotionScan::scan_frame(VFrame *previous_frame, VFrame *current_frame,
		int global_range_w, int global_range_h,
		int global_block_w, int global_block_h,
		double block_x, double block_y, int frame_type,
		int tracking_type, int action_type,
		int horizontal_only, int vertical_only,
		int source_position, int total_steps, int total_dx,
		int total_dy, int global_origin_x, int global_origin_y,
		int load_ok, int load_dx, int load_dy)
{
	this->previous_frame_arg = previous_frame;
	this->current_frame_arg = current_frame;
	this->horizontal_only = horizontal_only;
	this->vertical_only = vertical_only;
	this->previous_frame = previous_frame_arg;
	this->current_frame = current_frame_arg;
	this->global_origin_x = global_origin_x;
	this->global_origin_y = global_origin_y;
	subpixel = 0;

	cache.remove_all_objects();

// Single macroblock
	int w = current_frame->get_w();
	int h = current_frame->get_h();

// Initial search parameters
	int scan_w = w * global_range_w / 100;
	int scan_h = h * global_range_h / 100;
	int block_w = w * global_block_w / 100;
	int block_h = h * global_block_h / 100;

// Location of block in previous frame
	block_x1 = (int)(w * block_x / 100 - block_w / 2);
	block_y1 = (int)(h * block_y / 100 - block_h / 2);
	block_x2 = (int)(w * block_x / 100 + block_w / 2);
	block_y2 = (int)(h * block_y / 100 + block_h / 2);

// Offset to location of previous block.  This offset needn't be very accurate
// since it's the offset of the previous image and current image we want.
	if( frame_type == MotionScan::TRACK_PREVIOUS ) {
		block_x1 += total_dx / OVERSAMPLE;
		block_y1 += total_dy / OVERSAMPLE;
		block_x2 += total_dx / OVERSAMPLE;
		block_y2 += total_dy / OVERSAMPLE;
	}

	skip = 0;

	switch( tracking_type ) {
// Don't calculate
	case MotionScan::NO_CALCULATE:
		dx_result = dy_result = 0;
		skip = 1;
		break;

	case MotionScan::LOAD:
	case MotionScan::SAVE:
		if( load_ok ) {
			dx_result = load_dx;
			dy_result = load_dy;
			skip = 1;
		}
		break;

// Scan from scratch
	default:
		skip = 0;
		break;
	}

	if( !skip && test_match ) {
		if( previous_frame->data_matches(current_frame) ) {
			printf("MotionScan::scan_frame: data matches. skipping.\n");
			dx_result = dy_result = 0;
			skip = 1;
		}
	}
// Perform scan
	if( !skip ) {
//printf("MotionScan::scan_frame %d\n", __LINE__);
// Location of block in current frame
		int origin_offset_x = this->global_origin_x * w / 100;
		int origin_offset_y = this->global_origin_y * h / 100;
		int x_result = block_x1 + origin_offset_x;
		int y_result = block_y1 + origin_offset_y;

//printf("MotionScan::scan_frame 1 %d %d %d %d %d %d %d %d\n",
// block_x1 + block_w / 2, block_y1 + block_h / 2,
// block_w, block_h, block_x1, block_y1, block_x2, block_y2);

		while(1) {
// Cache needs to be cleared if downsampling is used because the sums of
// different downsamplings can't be compared.
// Subpixel never uses the cache.
//			cache.remove_all_objects();
			scan_x1 = x_result - scan_w / 2;
			scan_y1 = y_result - scan_h / 2;
			scan_x2 = x_result + scan_w / 2;
			scan_y2 = y_result + scan_h / 2;

// Zero out requested values
			if( horizontal_only ) {
				scan_y1 = block_y1;
				scan_y2 = block_y1 + 1;
			}
			if( vertical_only ) {
				scan_x1 = block_x1;
				scan_x2 = block_x1 + 1;
			}
//printf("MotionScan::scan_frame 1 %d %d %d %d %d %d %d %d\n",
// block_x1, block_y1, block_x2, block_y2, scan_x1, scan_y1, scan_x2, scan_y2);
// Clamp the block coords before the scan so we get useful scan coords.
			clamp_scan(w, h, &block_x1, &block_y1, &block_x2,
				&block_y2, &scan_x1, &scan_y1, &scan_x2,
				&scan_y2, 0);
// printf("MotionScan::scan_frame 1 %d block_x1=%d block_y1=%d block_x2=%d block_y2=%d\n"
//  "    scan_x1=%d scan_y1=%d scan_x2=%d scan_y2=%d\n"
//  "    x_result=%d y_result=%d\n", __LINE__, block_x1, block_y1, block_x2, block_y2,
//      scan_x1, scan_y1, scan_x2, scan_y2, x_result, y_result);

// Give up if invalid coords.
			if (scan_y2 <= scan_y1 || scan_x2 <= scan_x1 ||
			    block_x2 <= block_x1 || block_y2 <= block_y1 )
				break;

// For subpixel, the top row and left column are skipped
			if( subpixel ) {

//printf("MotionScan::scan_frame %d %d %d\n", __LINE__, x_result, y_result);
// Scan every subpixel in a 2 pixel * 2 pixel square
				total_pixels = (2 * OVERSAMPLE) * (2 * OVERSAMPLE);

				this->total_steps = total_pixels;
// These aren't used in subpixel
				this->x_steps = OVERSAMPLE * 2;
				this->y_steps = OVERSAMPLE * 2;

				set_package_count(this->total_steps);
				process_packages();

// Get least difference
				int64_t min_difference = -1;
				for( int i = 0; i < get_total_packages(); i++ ) {
					MotionScanPackage *pkg = (MotionScanPackage *)get_package(i);
//printf("MotionScan::scan_frame %d search_x=%d search_y=%d sub_x=%d sub_y=%d diff1=%lld diff2=%lld\n",
//__LINE__, pkg->search_x, pkg->search_y, pkg->sub_x, pkg->sub_y, pkg->difference1, pkg->difference2);
					if( pkg->difference1 < min_difference ||
					    min_difference == -1 ) {
						min_difference = pkg->difference1;

// The sub coords are 1 pixel up & left of the block coords
						x_result = pkg->search_x * OVERSAMPLE + pkg->sub_x;
						y_result = pkg->search_y * OVERSAMPLE + pkg->sub_y;
// Fill in results
						dx_result = block_x1 * OVERSAMPLE - x_result;
						dy_result = block_y1 * OVERSAMPLE - y_result;
//printf("MotionScan::scan_frame %d dx_result=%d dy_result=%d diff=%lld\n",
//__LINE__, dx_result, dy_result, min_difference);
					}

					if( pkg->difference2 < min_difference ) {
						min_difference = pkg->difference2;

						x_result = pkg->search_x * OVERSAMPLE - pkg->sub_x;
						y_result = pkg->search_y * OVERSAMPLE - pkg->sub_y;

						dx_result = block_x1 * OVERSAMPLE - x_result;
						dy_result = block_y1 * OVERSAMPLE - y_result;
//printf("MotionScan::scan_frame %d dx_result=%d dy_result=%d diff=%lld\n",
//__LINE__, dx_result, dy_result, min_difference);
					}
				}

				break;
			}
// Single pixel
			else {
				total_pixels =
				    (scan_x2 - scan_x1) * (scan_y2 - scan_y1);
				this->total_steps =
				    MIN(total_steps, total_pixels);

				if( this->total_steps == total_pixels ) {
					x_steps = scan_x2 - scan_x1;
					y_steps = scan_y2 - scan_y1;
				}
				else {
					x_steps = (int)sqrt(this->total_steps);
					y_steps = (int)sqrt(this->total_steps);
				}

// Use downsampled images
// 				if( scan_x2 - scan_x1 > x_steps * 4 ||
//                                      scan_y2 - scan_y1 > y_steps * 4 )
//                              {
// printf("MotionScan::scan_frame %d total_pixels=%d total_steps=%d x_steps=%d y_steps=%d x y steps=%d\n",
// __LINE__,
// total_pixels,
// total_steps,
// x_steps,
// y_steps,
// x_steps * y_steps);
//
// 					if( !downsampled_previous ||
//                                              !downsampled_previous->equivalent(previous_frame_arg) )
//                                      {
// 						delete downsampled_previous;
// 						downsampled_previous = new VFrame(*previous_frame_arg);
// 					}
//
// 					if( !downsampled_current ||
//                                              !downsampled_current->equivalent(current_frame_arg) )
//                                      {
// 						delete downsampled_current;
// 						downsampled_current = new VFrame(*current_frame_arg);
// 					}
//
//
// 					if( !downsample )
// 						downsample = new DownSampleServer(get_total_clients(),
// 							get_total_clients());
// 					downsample->process_frame(downsampled_previous,
// 						previous_frame_arg, 1, 1, 1, 1,
// 						(scan_y2 - scan_y1) / y_steps,
// 						(scan_x2 - scan_x1) / x_steps,
// 						0, 0);
// 					downsample->process_frame(downsampled_current,
// 						current_frame_arg, 1, 1, 1, 1,
// 						(scan_y2 - scan_y1) / y_steps,
// 						(scan_x2 - scan_x1) / x_steps,
// 						0, 0);
// 					this->previous_frame = downsampled_previous;
// 					this->current_frame = downsampled_current;
// 				}

// printf("MotionScan::scan_frame %d this->total_steps=%d\n",
// __LINE__, this->total_steps);

				set_package_count(this->total_steps);
				process_packages();

// Get least difference
				int64_t min_difference = -1;
				for( int i = 0; i < get_total_packages(); i++ ) {
					MotionScanPackage *pkg = (MotionScanPackage *) get_package(i);
//printf("MotionScan::scan_frame %d search_x=%d search_y=%d sub_x=%d sub_y=%d diff1=%lld diff2=%lld\n",
// __LINE__, pkg->search_x, pkg->search_y, pkg->sub_x, pkg->sub_y, pkg->difference1, pkg->difference2);
					if (pkg->difference1 < min_difference
					    || min_difference == -1) {
						min_difference = pkg->difference1;
						x_result = pkg->search_x;
						y_result = pkg->search_y;
						x_result *= OVERSAMPLE;
						y_result *= OVERSAMPLE;
//printf("MotionScan::scan_frame %d x_result=%d y_result=%d diff=%lld\n",
//__LINE__, block_x1 * OVERSAMPLE - x_result, block_y1 * OVERSAMPLE - y_result, pkg->difference1);
					}
				}

// If a new search is required, rescale results back to pixels.
				if( this->total_steps >= total_pixels ) {
// Single pixel accuracy reached.  Now do exhaustive subpixel search.
					if( action_type == MotionScan::STABILIZE ||
						action_type == MotionScan::TRACK ||
						action_type == MotionScan::NOTHING ) {
//printf("MotionScan::scan_frame %d %d %d\n", __LINE__, x_result, y_result);
						x_result /= OVERSAMPLE;
						y_result /= OVERSAMPLE;
						scan_w = scan_h = 2;
						subpixel = 1;
					}
// Fill in results and quit
					else {
						dx_result = block_x1 * OVERSAMPLE - x_result;
						dy_result = block_y1 * OVERSAMPLE - y_result;
//printf("MotionScan::scan_frame %d %d %d\n", __LINE__, dx_result, dy_result);
						break;
					}
				}
// Reduce scan area and try again
				else {
					scan_w = (scan_x2 - scan_x1) / 2;
					scan_h = (scan_y2 - scan_y1) / 2;
					x_result /= OVERSAMPLE;
					y_result /= OVERSAMPLE;
				}
			}
		}
		dx_result = -dx_result;
		dy_result = -dy_result;
	}
//printf("MotionScan::scan_frame %d\n", __LINE__);

	if( vertical_only ) dx_result = 0;
	if( horizontal_only ) dy_result = 0;

//printf("MotionScan::scan_frame %d dx=%.2f dy=%.2f\n",
// __LINE__, (float)this->dx_result / OVERSAMPLE, (float)this->dy_result / OVERSAMPLE);
}

int64_t MotionScan::get_cache(int x, int y)
{
	int64_t result = -1;
	cache_lock->lock("MotionScan::get_cache");
	for( int i = 0; i < cache.total; i++ ) {
		MotionScanCache *ptr = cache.values[i];
		if( ptr->x == x && ptr->y == y ) {
			result = ptr->difference;
			break;
		}
	}
	cache_lock->unlock();
	return result;
}

void MotionScan::put_cache(int x, int y, int64_t difference)
{
	MotionScanCache *ptr = new MotionScanCache(x, y, difference);
	cache_lock->lock("MotionScan::put_cache");
	cache.append(ptr);
	cache_lock->unlock();
}

#define ABS_DIFF(model, type, temp_type, multiplier, components) case model: { \
	temp_type result_temp = 0; \
	for( int i = 0; i < h; i++ ) { \
		type *prev_row = (type*)prev_ptr; \
		type *current_row = (type*)current_ptr; \
		for( int j = 0; j < w; j++ ) { \
			for( int k = 0; k < 3; k++ ) { \
				temp_type difference; \
				difference = *prev_row++ - *current_row++; \
				if( difference < 0 ) \
					result_temp -= difference; \
				else \
					result_temp += difference; \
			} \
			if( components == 4 ) { \
				prev_row++; \
				current_row++; \
			} \
		} \
		prev_ptr += row_bytes; \
		current_ptr += row_bytes; \
	} \
	result = (int64_t)(result_temp * multiplier); \
} break

int64_t MotionScan::abs_diff(unsigned char *prev_ptr,
		unsigned char *current_ptr, int row_bytes, int w,
		int h, int color_model)
{
	int64_t result = 0;
	switch( color_model ) {
	ABS_DIFF(BC_RGB888, unsigned char, int64_t, 1, 3);
	ABS_DIFF(BC_RGBA8888, unsigned char, int64_t, 1, 4);
	ABS_DIFF(BC_RGB_FLOAT, float, double, 0x10000, 3);
	ABS_DIFF(BC_RGBA_FLOAT, float, double, 0x10000, 4);
	ABS_DIFF(BC_YUV888, unsigned char, int64_t, 1, 3);
	ABS_DIFF(BC_YUVA8888, unsigned char, int64_t, 1, 4);
	ABS_DIFF(BC_YUV161616, uint16_t, int64_t, 1, 3);
	ABS_DIFF(BC_YUVA16161616, uint16_t, int64_t, 1, 4);
	}
	return result;
}

#define ABS_DIFF_SUB(model, type, temp_type, multiplier, components) case model: { \
	temp_type result_temp = 0; \
	temp_type y2_fraction = sub_y * 0x100 / OVERSAMPLE; \
	temp_type y1_fraction = 0x100 - y2_fraction; \
	temp_type x2_fraction = sub_x * 0x100 / OVERSAMPLE; \
	temp_type x1_fraction = 0x100 - x2_fraction; \
	for( int i = 0; i < h_sub; i++ ) { \
		type *prev_row1 = (type*)prev_ptr; \
		type *prev_row2 = (type*)prev_ptr + components; \
		type *prev_row3 = (type*)(prev_ptr + row_bytes); \
		type *prev_row4 = (type*)(prev_ptr + row_bytes) + components; \
		type *current_row = (type*)current_ptr; \
		for( int j = 0; j < w_sub; j++ ) { \
/* Scan each component */ \
			for( int k = 0; k < 3; k++ ) { \
				temp_type difference; \
				temp_type prev_value = \
					(*prev_row1++ * x1_fraction * y1_fraction + \
					*prev_row2++ * x2_fraction * y1_fraction + \
					*prev_row3++ * x1_fraction * y2_fraction + \
					*prev_row4++ * x2_fraction * y2_fraction) / \
					0x100 / 0x100; \
				temp_type current_value = *current_row++; \
				difference = prev_value - current_value; \
				if( difference < 0 ) \
					result_temp -= difference; \
				else \
					result_temp += difference; \
			} \
 \
/* skip alpha */ \
			if( components == 4 ) { \
				prev_row1++; \
				prev_row2++; \
				prev_row3++; \
				prev_row4++; \
				current_row++; \
			} \
		} \
		prev_ptr += row_bytes; \
		current_ptr += row_bytes; \
	} \
	result = (int64_t)(result_temp * multiplier); \
} break

int64_t MotionScan::abs_diff_sub(unsigned char *prev_ptr,
		unsigned char *current_ptr, int row_bytes,
		int w, int h, int color_model, int sub_x,
		int sub_y)
{
	int h_sub = h - 1;
	int w_sub = w - 1;
	int64_t result = 0;

	switch( color_model ) {
	ABS_DIFF_SUB(BC_RGB888, unsigned char, int64_t, 1, 3);
	ABS_DIFF_SUB(BC_RGBA8888, unsigned char, int64_t, 1, 4);
	ABS_DIFF_SUB(BC_RGB_FLOAT, float, double, 0x10000, 3);
	ABS_DIFF_SUB(BC_RGBA_FLOAT, float, double, 0x10000, 4);
	ABS_DIFF_SUB(BC_YUV888, unsigned char, int64_t, 1, 3);
	ABS_DIFF_SUB(BC_YUVA8888, unsigned char, int64_t, 1, 4);
	ABS_DIFF_SUB(BC_YUV161616, uint16_t, int64_t, 1, 3);
	ABS_DIFF_SUB(BC_YUVA16161616, uint16_t, int64_t, 1, 4);
	}
	return result;
}

MotionScanCache::MotionScanCache(int x, int y, int64_t difference)
{
	this->x = x;
	this->y = y;
	this->difference = difference;
}

void MotionScan::clamp_scan(int w, int h,
		int *block_x1, int *block_y1, int *block_x2,
		int *block_y2, int *scan_x1, int *scan_y1,
		int *scan_x2, int *scan_y2, int use_absolute)
{
//printf("MotionMain::clamp_scan 1 w=%d h=%d block=%d %d %d %d scan=%d %d %d %d absolute=%d\n",
// w, h, *block_x1, *block_y1, *block_x2, *block_y2, *scan_x1, *scan_y1, *scan_x2, *scan_y2,
// use_absolute);

	if( use_absolute ) {
// Limit size of scan area
// Used for drawing vectors
// scan is always out of range before block.
		if( *scan_x1 < 0 ) {
//			int difference = -*scan_x1;
//			*block_x1 += difference;
			*scan_x1 = 0;
		}

		if( *scan_y1 < 0 ) {
//			int difference = -*scan_y1;
//			*block_y1 += difference;
			*scan_y1 = 0;
		}

		if( *scan_x2 > w ) {
			int difference = *scan_x2 - w;
//			*block_x2 -= difference;
			*scan_x2 -= difference;
		}

		if( *scan_y2 > h ) {
			int difference = *scan_y2 - h;
//			*block_y2 -= difference;
			*scan_y2 -= difference;
		}

		CLAMP(*scan_x1, 0, w);
		CLAMP(*scan_y1, 0, h);
		CLAMP(*scan_x2, 0, w);
		CLAMP(*scan_y2, 0, h);
	}
	else {
// Limit range of upper left block coordinates
// Used for motion tracking
		if( *scan_x1 < 0 ) {
			int difference = -*scan_x1;
//			*block_x1 += difference;
			*scan_x2 += difference;
			*scan_x1 = 0;
		}

		if( *scan_y1 < 0 ) {
			int difference = -*scan_y1;
//			*block_y1 += difference;
			*scan_y2 += difference;
			*scan_y1 = 0;
		}

		if( *scan_x2 - *block_x1 + *block_x2 > w ) {
			int difference = *scan_x2 - *block_x1 + *block_x2 - w;
			*scan_x2 -= difference;
//			*block_x2 -= difference;
		}

		if( *scan_y2 - *block_y1 + *block_y2 > h ) {
			int difference = *scan_y2 - *block_y1 + *block_y2 - h;
			*scan_y2 -= difference;
//			*block_y2 -= difference;
		}
//              CLAMP(*scan_x1, 0, w - (*block_x2 - *block_x1));
//              CLAMP(*scan_y1, 0, h - (*block_y2 - *block_y1));
//              CLAMP(*scan_x2, 0, w - (*block_x2 - *block_x1));
//              CLAMP(*scan_y2, 0, h - (*block_y2 - *block_y1));
	}

// Sanity checks which break the calculation but should never happen if the
// center of the block is inside the frame.
	CLAMP(*block_x1, 0, w);
	CLAMP(*block_x2, 0, w);
	CLAMP(*block_y1, 0, h);
	CLAMP(*block_y2, 0, h);

//printf("MotionMain::clamp_scan 2 w=%d h=%d block=%d %d %d %d scan=%d %d %d %d absolute=%d\n",
// w, h, *block_x1, *block_y1, *block_x2, *block_y2, *scan_x1, *scan_y1, *scan_x2, *scan_y2,
// use_absolute);
}
