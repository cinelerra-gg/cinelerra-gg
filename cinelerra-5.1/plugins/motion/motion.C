
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

#include "affine.h"
#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "bcsignals.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "mainerror.h"
#include "motion.h"
#include "motionscan.h"
#include "motionwindow.h"
#include "mutex.h"
#include "overlayframe.h"
#include "rotateframe.h"
#include "transportque.h"


#include <errno.h>
#include <unistd.h>

REGISTER_PLUGIN(MotionMain)


//#define DEBUG

MotionConfig::MotionConfig()
{
	global_range_w = 25; //5;
	global_range_h = 25; //5;
	rotation_range = 8; //5;
	rotation_center = 0;
	block_count = 1;
	global_block_w = 33; //MIN_BLOCK;
	global_block_h = 33; //MIN_BLOCK;
	block_x = 50;
	block_y = 50;
	global_positions = 256;
	rotate_positions = 8; // 4;
	magnitude = 100;
	rotate_magnitude = 30;
	return_speed = 5; //0;
	rotate_return_speed = 5; //0;
	action_type = MotionScan::STABILIZE;
	global = 1;
	rotate = 1;
	addtrackedframeoffset = 0;
	strcpy(tracking_file, TRACKING_FILE);
	tracking_type = MotionScan::SAVE; //MotionScan::NO_CALCULATE;
	tracking_object = MotionScan::TRACK_PREVIOUS; //TRACK_SINGLE;
	draw_vectors = 1; //0;
	track_frame = 0;
	bottom_is_master = 1;
	horizontal_only = 0;
	vertical_only = 0;
}


void MotionConfig::boundaries()
{
	CLAMP(global_range_w, MIN_RADIUS, MAX_RADIUS);
	CLAMP(global_range_h, MIN_RADIUS, MAX_RADIUS);
	CLAMP(rotation_range, MIN_ROTATION, MAX_ROTATION);
	CLAMP(rotation_center, -MAX_ROTATION, MAX_ROTATION);
	CLAMP(block_count, MIN_BLOCKS, MAX_BLOCKS);
	CLAMP(global_block_w, MIN_BLOCK, MAX_BLOCK);
	CLAMP(global_block_h, MIN_BLOCK, MAX_BLOCK);
}

int MotionConfig::equivalent(MotionConfig &that)
{
	return global_range_w == that.global_range_w &&
		global_range_h == that.global_range_h &&
		rotation_range == that.rotation_range &&
		rotation_center == that.rotation_center &&
		action_type == that.action_type &&
		global == that.global && rotate == that.rotate &&
		addtrackedframeoffset == that.addtrackedframeoffset &&
		draw_vectors == that.draw_vectors &&
		block_count == that.block_count &&
		global_block_w == that.global_block_w &&
		global_block_h == that.global_block_h &&
		EQUIV(block_x, that.block_x) &&
		EQUIV(block_y, that.block_y) &&
		global_positions == that.global_positions &&
		rotate_positions == that.rotate_positions &&
		magnitude == that.magnitude &&
		return_speed == that.return_speed &&
		rotate_return_speed == that.rotate_return_speed &&
		rotate_magnitude == that.rotate_magnitude &&
		tracking_object == that.tracking_object &&
		track_frame == that.track_frame &&
		bottom_is_master == that.bottom_is_master &&
		horizontal_only == that.horizontal_only &&
		vertical_only == that.vertical_only;
}

void MotionConfig::copy_from(MotionConfig &that)
{
	global_range_w = that.global_range_w;
	global_range_h = that.global_range_h;
	rotation_range = that.rotation_range;
	rotation_center = that.rotation_center;
	action_type = that.action_type;
	global = that.global;
	rotate = that.rotate;
	addtrackedframeoffset = that.addtrackedframeoffset;
	tracking_type = that.tracking_type;
	draw_vectors = that.draw_vectors;
	block_count = that.block_count;
	block_x = that.block_x;
	block_y = that.block_y;
	global_positions = that.global_positions;
	rotate_positions = that.rotate_positions;
	global_block_w = that.global_block_w;
	global_block_h = that.global_block_h;
	magnitude = that.magnitude;
	return_speed = that.return_speed;
	rotate_magnitude = that.rotate_magnitude;
	rotate_return_speed = that.rotate_return_speed;
	tracking_object = that.tracking_object;
	track_frame = that.track_frame;
	bottom_is_master = that.bottom_is_master;
	horizontal_only = that.horizontal_only;
	vertical_only = that.vertical_only;
}

void MotionConfig::interpolate(MotionConfig &prev, MotionConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	copy_from(prev);
}


MotionMain::MotionMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	rotate_engine = 0;
	motion_rotate = 0;
	total_dx = 0;
	total_dy = 0;
	total_angle = 0;
	overlayer = 0;
	search_area = 0;
	search_size = 0;
	temp_frame = 0;
	previous_frame_number = -1;

	prev_global_ref = 0;
	current_global_ref = 0;
	global_target_src = 0;
	global_target_dst = 0;

	cache_file[0] = 0;
	cache_fp = active_fp = 0;
	cache_line[0] = 0;
	cache_key = active_key = -1;
	dx_offset = dy_offset = 0;
	load_ok = 0;
	save_dx = load_dx = 0;
	save_dy = load_dy = 0;
	save_dt = load_dt = 0;
	tracking_frame = -1;
	prev_rotate_ref = 0;
	current_rotate_ref = 0;
	rotate_target_src = 0;
	rotate_target_dst = 0;
}

MotionMain::~MotionMain()
{

	delete engine;
	delete overlayer;
	delete [] search_area;
	delete temp_frame;
	delete rotate_engine;
	delete motion_rotate;

	delete prev_global_ref;
	delete current_global_ref;
	delete global_target_src;
	delete global_target_dst;

	reset_cache_file();

	delete prev_rotate_ref;
	delete current_rotate_ref;
	delete rotate_target_src;
	delete rotate_target_dst;
}

const char* MotionMain::plugin_title() { return N_("Motion"); }
int MotionMain::is_realtime() { return 1; }
int MotionMain::is_multichannel() { return 1; }


NEW_WINDOW_MACRO(MotionMain, MotionWindow)

LOAD_CONFIGURATION_MACRO(MotionMain, MotionConfig)



void MotionMain::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	thread->window->lock_window("MotionMain::update_gui");
	MotionWindow *window = (MotionWindow*)thread->window;

	char string[BCTEXTLEN];
	sprintf(string, "%d", config.global_positions);
	window->global_search_positions->set_text(string);
	sprintf(string, "%d", config.rotate_positions);
	window->rotation_search_positions->set_text(string);

	window->global_block_w->update(config.global_block_w);
	window->global_block_h->update(config.global_block_h);
	window->block_x->update(config.block_x);
	window->block_y->update(config.block_y);
	window->block_x_text->update((float)config.block_x);
	window->block_y_text->update((float)config.block_y);
	window->magnitude->update(config.magnitude);
	window->return_speed->update(config.return_speed);
	window->rotate_magnitude->update(config.rotate_magnitude);
	window->rotate_return_speed->update(config.rotate_return_speed);
	window->rotation_range->update(config.rotation_range);
	window->rotation_center->update(config.rotation_center);


	window->track_single->update(config.tracking_object == MotionScan::TRACK_SINGLE);
	window->track_frame_number->update(config.track_frame);
	window->track_previous->update(config.tracking_object == MotionScan::TRACK_PREVIOUS);
	window->previous_same->update(config.tracking_object == MotionScan::PREVIOUS_SAME_BLOCK);
	if( config.tracking_object != MotionScan::TRACK_SINGLE )
		window->track_frame_number->disable();
	else
		window->track_frame_number->enable();

	window->action_type->set_text(
		ActionType::to_text(config.action_type));
	window->tracking_type->set_text(
		TrackingType::to_text(config.tracking_type));
	window->track_direction->set_text(
		TrackDirection::to_text(config.horizontal_only, config.vertical_only));
	window->master_layer->set_text(
		MasterLayer::to_text(config.bottom_is_master));

	window->update_mode();
	thread->window->unlock_window();
}




void MotionMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("MOTION");

	output.tag.set_property("BLOCK_COUNT", config.block_count);
	output.tag.set_property("GLOBAL_POSITIONS", config.global_positions);
	output.tag.set_property("ROTATE_POSITIONS", config.rotate_positions);
	output.tag.set_property("GLOBAL_BLOCK_W", config.global_block_w);
	output.tag.set_property("GLOBAL_BLOCK_H", config.global_block_h);
	output.tag.set_property("BLOCK_X", config.block_x);
	output.tag.set_property("BLOCK_Y", config.block_y);
	output.tag.set_property("GLOBAL_RANGE_W", config.global_range_w);
	output.tag.set_property("GLOBAL_RANGE_H", config.global_range_h);
	output.tag.set_property("ROTATION_RANGE", config.rotation_range);
	output.tag.set_property("ROTATION_CENTER", config.rotation_center);
	output.tag.set_property("MAGNITUDE", config.magnitude);
	output.tag.set_property("RETURN_SPEED", config.return_speed);
	output.tag.set_property("ROTATE_MAGNITUDE", config.rotate_magnitude);
	output.tag.set_property("ROTATE_RETURN_SPEED", config.rotate_return_speed);
	output.tag.set_property("ACTION_TYPE", config.action_type);
	output.tag.set_property("GLOBAL", config.global);
	output.tag.set_property("ROTATE", config.rotate);
	output.tag.set_property("ADDTRACKEDFRAMEOFFSET", config.addtrackedframeoffset);
	output.tag.set_property("TRACKING_FILE", config.tracking_file);
	output.tag.set_property("TRACKING_TYPE", config.tracking_type);
	output.tag.set_property("DRAW_VECTORS", config.draw_vectors);
	output.tag.set_property("TRACKING_OBJECT", config.tracking_object);
	output.tag.set_property("TRACK_FRAME", config.track_frame);
	output.tag.set_property("BOTTOM_IS_MASTER", config.bottom_is_master);
	output.tag.set_property("HORIZONTAL_ONLY", config.horizontal_only);
	output.tag.set_property("VERTICAL_ONLY", config.vertical_only);
	output.append_tag();
	output.tag.set_title("/MOTION");
	output.append_tag();
	output.terminate_string();
}

void MotionMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	int result = 0;

	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("MOTION") ) {
			config.block_count = input.tag.get_property("BLOCK_COUNT", config.block_count);
			config.global_positions = input.tag.get_property("GLOBAL_POSITIONS", config.global_positions);
			config.rotate_positions = input.tag.get_property("ROTATE_POSITIONS", config.rotate_positions);
			config.global_block_w = input.tag.get_property("GLOBAL_BLOCK_W", config.global_block_w);
			config.global_block_h = input.tag.get_property("GLOBAL_BLOCK_H", config.global_block_h);
			config.block_x = input.tag.get_property("BLOCK_X", config.block_x);
			config.block_y = input.tag.get_property("BLOCK_Y", config.block_y);
			config.global_range_w = input.tag.get_property("GLOBAL_RANGE_W", config.global_range_w);
			config.global_range_h = input.tag.get_property("GLOBAL_RANGE_H", config.global_range_h);
			config.rotation_range = input.tag.get_property("ROTATION_RANGE", config.rotation_range);
			config.rotation_center = input.tag.get_property("ROTATION_CENTER", config.rotation_center);
			config.magnitude = input.tag.get_property("MAGNITUDE", config.magnitude);
			config.return_speed = input.tag.get_property("RETURN_SPEED", config.return_speed);
			config.rotate_magnitude = input.tag.get_property("ROTATE_MAGNITUDE", config.rotate_magnitude);
			config.rotate_return_speed = input.tag.get_property("ROTATE_RETURN_SPEED", config.rotate_return_speed);
			config.action_type = input.tag.get_property("ACTION_TYPE", config.action_type);
			config.global = input.tag.get_property("GLOBAL", config.global);
			config.rotate = input.tag.get_property("ROTATE", config.rotate);
			config.addtrackedframeoffset = input.tag.get_property("ADDTRACKEDFRAMEOFFSET", config.addtrackedframeoffset);
			input.tag.get_property("TRACKING_FILE", config.tracking_file);
			config.tracking_type = input.tag.get_property("TRACKING_TYPE", config.tracking_type);
			config.draw_vectors = input.tag.get_property("DRAW_VECTORS", config.draw_vectors);
			config.tracking_object = input.tag.get_property("TRACKING_OBJECT", config.tracking_object);
			config.track_frame = input.tag.get_property("TRACK_FRAME", config.track_frame);
			config.bottom_is_master = input.tag.get_property("BOTTOM_IS_MASTER", config.bottom_is_master);
			config.horizontal_only = input.tag.get_property("HORIZONTAL_ONLY", config.horizontal_only);
			config.vertical_only = input.tag.get_property("VERTICAL_ONLY", config.vertical_only);
		}
	}
	config.boundaries();
}

void MotionMain::allocate_temp(int w, int h, int color_model)
{
	if( temp_frame &&
	    ( temp_frame->get_w() != w || temp_frame->get_h() != h ) ) {
		delete temp_frame;
		temp_frame = 0;
	}
	if( !temp_frame )
		temp_frame = new VFrame(w, h, color_model, 0);
}

void MotionMain::process_global()
{

	if( !engine ) engine = new MotionScan(PluginClient::get_project_smp() + 1,
		PluginClient::get_project_smp() + 1);

// Determine if frames changed
	engine->scan_frame(current_global_ref, prev_global_ref,
		config.global_range_w, config.global_range_h,
		config.global_block_w, config.global_block_h,
		config.block_x, config.block_y,
		config.tracking_object, config.tracking_type,
		config.action_type, config.horizontal_only,
		config.vertical_only, get_source_position(),
		config.global_positions, total_dx, total_dy,
		0, 0, load_ok, load_dx, load_dy);
	current_dx = (engine->dx_result += dx_offset);
	current_dy = (engine->dy_result += dy_offset);

// Write results
	if( config.tracking_type == MotionScan::SAVE ) {
		save_dx = engine->dx_result;
		save_dy = engine->dy_result;
	}

// Add current motion vector to accumulation vector.
	if( config.tracking_object != MotionScan::TRACK_SINGLE ) {
// Retract over time
		total_dx = (int64_t)total_dx * (100 - config.return_speed) / 100;
		total_dy = (int64_t)total_dy * (100 - config.return_speed) / 100;
		total_dx += engine->dx_result;
		total_dy += engine->dy_result;
// printf("MotionMain::process_global total_dx=%d engine->dx_result=%d\n",
// total_dx, engine->dx_result);
	}
	else {
// Make accumulation vector current
		total_dx = engine->dx_result;
		total_dy = engine->dy_result;
	}

// Clamp accumulation vector
	if( config.magnitude < 100 ) {
		int block_x_orig = (int64_t)(config.block_x * current_global_ref->get_w() / 100);
		int block_y_orig = (int64_t)(config.block_y * current_global_ref->get_h() / 100);
		int max_block_x = (int64_t)(current_global_ref->get_w() - block_x_orig)
			* OVERSAMPLE * config.magnitude / 100;
		int max_block_y = (int64_t)(current_global_ref->get_h() - block_y_orig)
			* OVERSAMPLE * config.magnitude / 100;
		int min_block_x = (int64_t)-block_x_orig
			* OVERSAMPLE * config.magnitude / 100;
		int min_block_y = (int64_t)-block_y_orig
			* OVERSAMPLE * config.magnitude / 100;

		CLAMP(total_dx, min_block_x, max_block_x);
		CLAMP(total_dy, min_block_y, max_block_y);
	}

#ifdef DEBUG
printf("MotionMain::process_global 2 total_dx=%.02f total_dy=%.02f\n",
  (float)total_dx / OVERSAMPLE, (float)total_dy / OVERSAMPLE);
#endif

	if( config.tracking_object != MotionScan::TRACK_SINGLE && !config.rotate ) {
// Transfer current reference frame to previous reference frame and update
// counter.  Must wait for rotate to compare.
		prev_global_ref->copy_from(current_global_ref);
		previous_frame_number = get_source_position();
	}

// Decide what to do with target based on requested operation
	int interpolation = NEAREST_NEIGHBOR;
	float dx = 0., dy = 0.;
	switch(config.action_type) {
	case MotionScan::NOTHING:
		global_target_dst->copy_from(global_target_src);
		break;
	case MotionScan::TRACK_PIXEL:
		interpolation = NEAREST_NEIGHBOR;
		dx = (int)(total_dx / OVERSAMPLE);
		dy = (int)(total_dy / OVERSAMPLE);
		break;
	case MotionScan::STABILIZE_PIXEL:
		interpolation = NEAREST_NEIGHBOR;
		dx = -(int)(total_dx / OVERSAMPLE);
		dy = -(int)(total_dy / OVERSAMPLE);
		break;
	case MotionScan::TRACK:
		interpolation = CUBIC_LINEAR;
		dx = (float)total_dx / OVERSAMPLE;
		dy = (float)total_dy / OVERSAMPLE;
		break;
	case MotionScan::STABILIZE:
		interpolation = CUBIC_LINEAR;
		dx = -(float)total_dx / OVERSAMPLE;
		dy = -(float)total_dy / OVERSAMPLE;
		break;
	}


	if( config.action_type != MotionScan::NOTHING ) {
		if( !overlayer )
			overlayer = new OverlayFrame(PluginClient::get_project_smp() + 1);
		global_target_dst->clear_frame();
		overlayer->overlay(global_target_dst, global_target_src,
			0, 0, global_target_src->get_w(), global_target_src->get_h(),
			dx, dy,
			(float)global_target_src->get_w() + dx,
			(float)global_target_src->get_h() + dy,
			1, TRANSFER_REPLACE, interpolation);
	}
}



void MotionMain::process_rotation()
{
	int block_x, block_y;

// Convert the previous global reference into the previous rotation reference.
// Convert global target destination into rotation target source.
	if( config.global ) {
		if( !overlayer )
			overlayer = new OverlayFrame(PluginClient::get_project_smp() + 1);
		float dx, dy;
		if( config.tracking_object == MotionScan::TRACK_SINGLE ) {
			dx = (float)total_dx / OVERSAMPLE;
			dy = (float)total_dy / OVERSAMPLE;
		}
		else {
			dx = (float)current_dx / OVERSAMPLE;
			dy = (float)current_dy / OVERSAMPLE;
		}

		prev_rotate_ref->clear_frame();
		overlayer->overlay(prev_rotate_ref, prev_global_ref,
			0, 0, prev_global_ref->get_w(), prev_global_ref->get_h(),
			dx, dy,
			(float)prev_global_ref->get_w() + dx,
			(float)prev_global_ref->get_h() + dy,
			1, TRANSFER_REPLACE, CUBIC_LINEAR);
// Pivot is destination global position
		block_x = (int)(prev_rotate_ref->get_w() *
			config.block_x / 100 + (float)total_dx / OVERSAMPLE);
		block_y = (int)(prev_rotate_ref->get_h() *
			config.block_y / 100 + (float)total_dy / OVERSAMPLE);
// Use the global target output as the rotation target input
		rotate_target_src->copy_from(global_target_dst);
// Transfer current reference frame to previous reference frame for global.
		if( config.tracking_object != MotionScan::TRACK_SINGLE ) {
			prev_global_ref->copy_from(current_global_ref);
			previous_frame_number = get_source_position();
		}
	}
	else {
// Pivot is fixed
		block_x = (int)(prev_rotate_ref->get_w() * config.block_x / 100);
		block_y = (int)(prev_rotate_ref->get_h() * config.block_y / 100);
	}

// Get rotation
	if( !motion_rotate )
		motion_rotate = new RotateScan(this,
			get_project_smp() + 1, get_project_smp() + 1);

	current_angle = motion_rotate->
		scan_frame(prev_rotate_ref, current_rotate_ref, block_x, block_y);

// Write results
	if( config.tracking_type == MotionScan::SAVE ) {
		save_dt = current_angle;
	}

// Add current rotation to accumulation
	if( config.tracking_object != MotionScan::TRACK_SINGLE ) {
// Retract over time
		total_angle = total_angle * (100 - config.rotate_return_speed) / 100;
// Accumulate current rotation
		total_angle += current_angle;

// Clamp rotation accumulation
		if( config.rotate_magnitude < 90 ) {
			CLAMP(total_angle, -config.rotate_magnitude, config.rotate_magnitude);
		}

		if( !config.global ) {
// Transfer current reference frame to previous reference frame and update counter.
			prev_rotate_ref->copy_from(current_rotate_ref);
			previous_frame_number = get_source_position();
		}
	}
	else {
		total_angle = current_angle;
	}

#ifdef DEBUG
printf("MotionMain::process_rotation total_angle=%f\n", total_angle);
#endif


// Calculate rotation parameters based on requested operation
	float angle = 0.;
	switch(config.action_type) {
	case MotionScan::NOTHING:
		rotate_target_dst->copy_from(rotate_target_src);
		break;
	case MotionScan::TRACK:
	case MotionScan::TRACK_PIXEL:
		angle = total_angle;
		break;
	case MotionScan::STABILIZE:
	case MotionScan::STABILIZE_PIXEL:
		angle = -total_angle;
		break;
	}

	if( config.action_type != MotionScan::NOTHING ) {
		if( !rotate_engine )
			rotate_engine = new AffineEngine(
				PluginClient::get_project_smp() + 1,
				PluginClient::get_project_smp() + 1);

		rotate_target_dst->clear_frame();

// Determine pivot based on a number of factors.
		switch(config.action_type) {
		case MotionScan::TRACK:
		case MotionScan::TRACK_PIXEL:
// Use destination of global tracking.
			rotate_engine->set_in_pivot(block_x, block_y);
			rotate_engine->set_out_pivot(block_x, block_y);
			break;

		case MotionScan::STABILIZE:
		case MotionScan::STABILIZE_PIXEL:
			if( config.global ) {
// Use origin of global stabilize operation
				rotate_engine->set_in_pivot(
					(int)(rotate_target_dst->get_w() * config.block_x / 100),
					(int)(rotate_target_dst->get_h() * config.block_y / 100));
				rotate_engine->set_out_pivot(
					(int)(rotate_target_dst->get_w() * config.block_x / 100),
					(int)(rotate_target_dst->get_h() * config.block_y / 100));
				}
				else {
// Use origin
					rotate_engine->set_in_pivot(block_x, block_y);
					rotate_engine->set_out_pivot(block_x, block_y);
				}
				break;
		}

		rotate_engine->rotate(rotate_target_dst, rotate_target_src, angle);
// overlayer->overlay(rotate_target_dst, prev_rotate_ref,
// 	0, 0, prev_rotate_ref->get_w(), prev_rotate_ref->get_h(),
// 	0, 0, prev_rotate_ref->get_w(), prev_rotate_ref->get_h(),
// 	1, TRANSFER_NORMAL, CUBIC_LINEAR);
// overlayer->overlay(rotate_target_dst, current_rotate_ref,
// 	0, 0, prev_rotate_ref->get_w(), prev_rotate_ref->get_h(),
// 	0, 0, prev_rotate_ref->get_w(), prev_rotate_ref->get_h(),
// 	1, TRANSFER_NORMAL, // 	CUBIC_LINEAR);
	}
}


int MotionMain::process_buffer(VFrame **frame, int64_t start_position, double frame_rate)
{
	int prev_config_tracking_type = config.tracking_type;
	int need_reconfigure = load_configuration();
	int color_model = frame[0]->get_color_model();
	w = frame[0]->get_w();
	h = frame[0]->get_h();

#ifdef DEBUG
printf("MotionMain::process_buffer %d start_position=%jd\n", __LINE__, start_position);
#endif

// Calculate the source and destination pointers for each of the operations.
// Get the layer to track motion in.
// Get the layer to apply motion in.
	reference_layer = config.bottom_is_master ?
		PluginClient::total_in_buffers - 1 : 0;
	target_layer = config.bottom_is_master ?
		0 : PluginClient::total_in_buffers - 1;

	output_frame = frame[target_layer];
// Get the position of previous reference frame.
	int64_t actual_previous_number;
// Skip if match frame not available
	int skip_current = 0;

	if( config.tracking_object == MotionScan::TRACK_SINGLE ) {
		actual_previous_number = config.track_frame;
		if( get_direction() == PLAY_REVERSE )
			actual_previous_number++;
		if( actual_previous_number == start_position )
			skip_current = 1;
	}
	else {
		actual_previous_number = start_position;
		if( get_direction() == PLAY_FORWARD ) {
			actual_previous_number--;
			if( actual_previous_number < get_source_start() )
				skip_current = 1;
			else {
				KeyFrame *keyframe = get_prev_keyframe(start_position, 1);
				if( keyframe->position > 0 &&
				    actual_previous_number < keyframe->position )
					skip_current = 1;
			}
		}
		else {
			actual_previous_number++;
			if( actual_previous_number >= get_source_start() + get_total_len() )
				skip_current = 1;
			else {
				KeyFrame *keyframe = get_next_keyframe(start_position, 1);
				if( keyframe->position > 0 &&
				    actual_previous_number >= keyframe->position )
					skip_current = 1;
			}
		}
// Only count motion since last keyframe
	}

	if( !config.global && !config.rotate )
		skip_current = 1;

//printf("process_realtime: %jd %d %jd %jd\n", start_position,
// skip_current, previous_frame_number, actual_previous_number);
	if( prev_config_tracking_type != MotionScan::SAVE &&
	    config.tracking_type == MotionScan::SAVE ) {
		reset_cache_file();
		char save_file[BCTEXTLEN];
		snprintf(save_file, sizeof(save_file), "%s.bak", config.tracking_file);
#ifdef DEBUG
printf("MotionMain::process_buffer 2 rename tracking file: %s to %s\n",
 config.tracking_file, save_file);
#endif
		::rename(config.tracking_file, save_file);
	}
	else if( !cache_file[0] || active_key > start_position )
		reset_cache_file();

// Load match frame and reset vectors
	int need_reload = !skip_current &&
		(previous_frame_number != actual_previous_number ||
		need_reconfigure);
	if( need_reload ) {
		total_dx = total_dy = 0; total_angle = 0;
		previous_frame_number = actual_previous_number;
	}

	if( skip_current ) {
		total_dx = total_dy = 0;
		current_dx = current_dy = 0;
		total_angle = current_angle = 0;
	}

// Get the global pointers.  Here we walk through the sequence of events.
	if( config.global ) {
// Assume global only.  Global reads previous frame and compares
// with current frame to get the current translation.
// The center of the search area is fixed in compensate mode or
// the user value + the accumulation vector in track mode.
		if( !prev_global_ref )
			prev_global_ref = new VFrame(w, h, color_model, 0);
		if( !current_global_ref )
			current_global_ref = new VFrame(w, h, color_model, 0);

// Global loads the current target frame into the src and
// writes it to the dst frame with desired translation.
		if( !global_target_src )
			global_target_src = new VFrame(w, h, color_model, 0);
		if( !global_target_dst )
			global_target_dst = new VFrame(w, h, color_model, 0);

// Load the global frames
		if( need_reload ) {
			read_frame(prev_global_ref, reference_layer,
				previous_frame_number, frame_rate, 0);
		}

		read_frame(current_global_ref, reference_layer,
			start_position, frame_rate, 0);
		read_frame(global_target_src, target_layer,
			start_position, frame_rate, 0);

// Global followed by rotate
		if( config.rotate ) {
// Must translate the previous global reference by the current global
// accumulation vector to match the current global reference.
// The center of the search area is always the user value + the accumulation
// vector.
			if( !prev_rotate_ref )
				prev_rotate_ref = new VFrame(w, h, color_model, 0);
// The current global reference is the current rotation reference.
			if( !current_rotate_ref )
				current_rotate_ref = new VFrame(w, h, color_model, 0);
			current_rotate_ref->copy_from(current_global_ref);

// The global target destination is copied to the rotation target source
// then written to the rotation output with rotation.
// The pivot for the rotation is the center of the search area
// if we're tracking.
// The pivot is fixed to the user position if we're compensating.
			if( !rotate_target_src )
				rotate_target_src = new VFrame(w, h, color_model, 0);
			if( !rotate_target_dst )
				rotate_target_dst = new VFrame(w, h, color_model, 0);
		}
	}
// Rotation only
	else if( config.rotate ) {
// Rotation reads the previous reference frame and compares it with current
// reference frame.
		if( !prev_rotate_ref )
			prev_rotate_ref = new VFrame(w, h, color_model, 0);
		if( !current_rotate_ref )
			current_rotate_ref = new VFrame(w, h, color_model, 0);

// Rotation loads target frame to temporary, rotates it, and writes it to the
// target frame.  The pivot is always fixed.
		if( !rotate_target_src )
			rotate_target_src = new VFrame(w, h, color_model, 0);
		if( !rotate_target_dst )
			rotate_target_dst = new VFrame(w, h, color_model, 0);


// Load the rotate frames
		if( need_reload ) {
			read_frame(prev_rotate_ref, reference_layer,
				previous_frame_number, frame_rate, 0);
		}
		read_frame(current_rotate_ref, reference_layer,
			start_position, frame_rate, 0);
		read_frame(rotate_target_src, target_layer,
			start_position, frame_rate, 0);
	}

	dx_offset = 0; dy_offset = 0;
	if( config.tracking_type == MotionScan::LOAD ) {
		if( config.addtrackedframeoffset ) {
			if( config.track_frame != tracking_frame ) {
				tracking_frame = config.track_frame;
				int64_t no;  int dx, dy;  float dt;
				if( !get_cache_line(tracking_frame) &&
				    sscanf(cache_line, "%jd %d %d %f", &no, &dx, &dy, &dt) == 4 ) {
					dx_offset = dx; dy_offset = dy;
				}
				else {
					eprintf("no offset data frame %jd\n", tracking_frame);
				}
			}
		}
		else
			tracking_frame = -1;
	}

	if( !skip_current ) {
		load_ok = 0;
		if( config.tracking_type == MotionScan::LOAD ||
		    config.tracking_type == MotionScan::SAVE ) {
			int64_t no;  int dx, dy;  float dt;
			int64_t frame_no = get_source_position();
// Load result from disk
			if( !get_cache_line(frame_no) &&
			    sscanf(cache_line, "%jd %d %d %f", &no, &dx, &dy, &dt) == 4 ) {
				load_ok = 1;  load_dx = dx;  load_dy = dy;  load_dt = dt;
			}
			else {
#ifdef DEBUG
printf("MotionMain::process_buffer: no tracking data frame %jd\n", frame_no);
#endif
			}
		}

// Get position change from previous frame to current frame
		if( config.global )
			process_global();
// Get rotation change from previous frame to current frame
		if( config.rotate )
			process_rotation();
//frame[target_layer]->copy_from(prev_rotate_ref);
//frame[target_layer]->copy_from(current_rotate_ref);

// write results to disk
		if( config.tracking_type == MotionScan::SAVE ) {
			char line[BCSTRLEN];
			int64_t frame_no = get_source_position();
			snprintf(line, sizeof(line), "%jd %d %d %f\n",
				frame_no, save_dx, save_dy, save_dt);
			put_cache_line(line);
		}
// Transfer the relevant target frame to the output
		if( config.rotate ) {
			frame[target_layer]->copy_from(rotate_target_dst);
		}
		else {
			frame[target_layer]->copy_from(global_target_dst);
		}
	}
// Read the target destination directly
	else {
		read_frame(frame[target_layer],
			target_layer, start_position, frame_rate, 0);
	}

	if( config.draw_vectors ) {
		draw_vectors(frame[target_layer]);
	}

#ifdef DEBUG
printf("MotionMain::process_buffer %d\n", __LINE__);
#endif
	return 0;
}



void MotionMain::draw_vectors(VFrame *frame)
{
	int w = frame->get_w(), h = frame->get_h();
	int global_x1, global_y1, global_x2, global_y2;
	int block_x, block_y, block_w, block_h;
	int block_x1, block_y1, block_x2, block_y2;
	int block_x3, block_y3, block_x4, block_y4;
	int search_x1, search_y1, search_x2, search_y2;
	int search_w, search_h;


	if( config.global ) {
// Get vector
// Start of vector is center of previous block.
// End of vector is total accumulation.
		if( config.tracking_object == MotionScan::TRACK_SINGLE ) {
			global_x1 = (int64_t)(config.block_x * w / 100);
			global_y1 = (int64_t)(config.block_y * h / 100);
			global_x2 = global_x1 + total_dx / OVERSAMPLE;
			global_y2 = global_y1 + total_dy / OVERSAMPLE;
//printf("MotionMain::draw_vectors %d %d %d %d %d %d\n", total_dx, total_dy, global_x1, global_y1, global_x2, global_y2);
		}
// Start of vector is center of previous block.
// End of vector is current change.
		else if( config.tracking_object == MotionScan::PREVIOUS_SAME_BLOCK ) {
			global_x1 = (int64_t)(config.block_x * w / 100);
			global_y1 = (int64_t)(config.block_y * h / 100);
			global_x2 = global_x1 + current_dx / OVERSAMPLE;
			global_y2 = global_y1 + current_dy / OVERSAMPLE;
		}
		else {
			global_x1 = (int64_t)(config.block_x * w / 100
				+ (total_dx - current_dx) / OVERSAMPLE);
			global_y1 = (int64_t)(config.block_y * h / 100
				+ (total_dy - current_dy) / OVERSAMPLE);
			global_x2 = (int64_t)(config.block_x * w / 100
				+ total_dx / OVERSAMPLE);
			global_y2 = (int64_t)(config.block_y * h / 100
				+ total_dy / OVERSAMPLE);
		}

		block_x = global_x1;
		block_y = global_y1;
		block_w = config.global_block_w * w / 100;
		block_h = config.global_block_h * h / 100;
		block_x1 = block_x - block_w / 2;
		block_y1 = block_y - block_h / 2;
		block_x2 = block_x + block_w / 2;
		block_y2 = block_y + block_h / 2;
		search_w = config.global_range_w * w / 100;
		search_h = config.global_range_h * h / 100;
		search_x1 = block_x1 - search_w / 2;
		search_y1 = block_y1 - search_h / 2;
		search_x2 = block_x2 + search_w / 2;
		search_y2 = block_y2 + search_h / 2;

//printf("MotionMain::draw_vectors %d %d %d %d %d %d %d %d %d %d %d %d\n",
// global_x1, global_y1, block_w, block_h, block_x1, block_y1,
// block_x2, block_y2, search_x1, search_y1, search_x2, search_y2);

		MotionScan::clamp_scan(w, h,
			&block_x1, &block_y1, &block_x2, &block_y2,
			&search_x1, &search_y1, &search_x2, &search_y2, 1);

// Vector
		draw_arrow(frame, global_x1, global_y1, global_x2, global_y2);

// Macroblock
		draw_line(frame, block_x1, block_y1, block_x2, block_y1);
		draw_line(frame, block_x2, block_y1, block_x2, block_y2);
		draw_line(frame, block_x2, block_y2, block_x1, block_y2);
		draw_line(frame, block_x1, block_y2, block_x1, block_y1);

// Search area
		draw_line(frame, search_x1, search_y1, search_x2, search_y1);
		draw_line(frame, search_x2, search_y1, search_x2, search_y2);
		draw_line(frame, search_x2, search_y2, search_x1, search_y2);
		draw_line(frame, search_x1, search_y2, search_x1, search_y1);

// Block should be endpoint of motion
		if( config.rotate ) {
			block_x = global_x2;
			block_y = global_y2;
		}
	}
	else {
		block_x = (int64_t)(config.block_x * w / 100);
		block_y = (int64_t)(config.block_y * h / 100);
	}

	block_w = config.global_block_w * w / 100;
	block_h = config.global_block_h * h / 100;
	if( config.rotate ) {
		float angle = total_angle * 2 * M_PI / 360;
		double base_angle1 = atan((float)block_h / block_w);
		double base_angle2 = atan((float)block_w / block_h);
		double target_angle1 = base_angle1 + angle;
		double target_angle2 = base_angle2 + angle;
		double radius = sqrt(block_w * block_w + block_h * block_h) / 2;
		block_x1 = (int)(block_x - cos(target_angle1) * radius);
		block_y1 = (int)(block_y - sin(target_angle1) * radius);
		block_x2 = (int)(block_x + sin(target_angle2) * radius);
		block_y2 = (int)(block_y - cos(target_angle2) * radius);
		block_x3 = (int)(block_x - sin(target_angle2) * radius);
		block_y3 = (int)(block_y + cos(target_angle2) * radius);
		block_x4 = (int)(block_x + cos(target_angle1) * radius);
		block_y4 = (int)(block_y + sin(target_angle1) * radius);

		draw_line(frame, block_x1, block_y1, block_x2, block_y2);
		draw_line(frame, block_x2, block_y2, block_x4, block_y4);
		draw_line(frame, block_x4, block_y4, block_x3, block_y3);
		draw_line(frame, block_x3, block_y3, block_x1, block_y1);


// Center
		if( !config.global ) {
			draw_line(frame, block_x, block_y - 5, block_x, block_y + 6);
			draw_line(frame, block_x - 5, block_y, block_x + 6, block_y);
		}
	}
}

MotionVVFrame::MotionVVFrame(VFrame *vfrm, int n)
 : VFrame(vfrm->get_data(), -1, vfrm->get_y()-vfrm->get_data(),
	vfrm->get_u()-vfrm->get_data(), vfrm->get_v()-vfrm->get_data(),
	vfrm->get_w(), vfrm->get_h(), vfrm->get_color_model(),
	vfrm->get_bytes_per_line())
{
	this->n = n;
}

int MotionVVFrame::draw_pixel(int x, int y)
{
	VFrame::draw_pixel(x+0, y+0);
	for( int i=1; i<n; ++i ) {
		VFrame::draw_pixel(x-i, y-i);
		VFrame::draw_pixel(x+i, y+i);
	}
	return 0;
}

void MotionMain::draw_line(VFrame *frame, int x1, int y1, int x2, int y2)
{
	int iw = frame->get_w(), ih = frame->get_h();
	int mx = iw > ih ? iw : ih;
	int n = mx/800 + 1;
	MotionVVFrame vfrm(frame, n);
	vfrm.set_pixel_color(WHITE);
	int m = 2;  while( m < n ) m <<= 1;
	vfrm.set_stiple(2*m);
	vfrm.draw_line(x1,y1, x2,y2);
}

#define ARROW_SIZE 10
void MotionMain::draw_arrow(VFrame *frame, int x1, int y1, int x2, int y2)
{
	double angle = atan((float)(y2 - y1) / (float)(x2 - x1));
	double angle1 = angle + (float)145 / 360 * 2 * 3.14159265;
	double angle2 = angle - (float)145 / 360 * 2 * 3.14159265;
	int x3, y3, x4, y4;
	if( x2 < x1 ) {
		x3 = x2 - (int)(ARROW_SIZE * cos(angle1));
		y3 = y2 - (int)(ARROW_SIZE * sin(angle1));
		x4 = x2 - (int)(ARROW_SIZE * cos(angle2));
		y4 = y2 - (int)(ARROW_SIZE * sin(angle2));
	}
	else {
		x3 = x2 + (int)(ARROW_SIZE * cos(angle1));
		y3 = y2 + (int)(ARROW_SIZE * sin(angle1));
		x4 = x2 + (int)(ARROW_SIZE * cos(angle2));
		y4 = y2 + (int)(ARROW_SIZE * sin(angle2));
	}

// Main vector
	draw_line(frame, x1, y1, x2, y2);
//	draw_line(frame, x1, y1 + 1, x2, y2 + 1);

// Arrow line
	if( abs(y2 - y1) || abs(x2 - x1) ) draw_line(frame, x2, y2, x3, y3);
//	draw_line(frame, x2, y2 + 1, x3, y3 + 1);
// Arrow line
	if( abs(y2 - y1) || abs(x2 - x1) ) draw_line(frame, x2, y2, x4, y4);
//	draw_line(frame, x2, y2 + 1, x4, y4 + 1);
}

int MotionMain::open_cache_file()
{
	if( cache_fp ) return 0;
	if( !cache_file[0] ) return 1;
	if( !(cache_fp = fopen(cache_file, "r")) ) return 1;
	return 0;
}

void MotionMain::close_cache_file()
{
	if( !cache_fp ) return;
	fclose(cache_fp);
	cache_fp = 0; cache_key = -1; tracking_frame = -1;
}

int MotionMain::load_cache_line()
{
	cache_key = -1;
	if( open_cache_file() ) return 1;
	if( !fgets(cache_line, sizeof(cache_line), cache_fp) ) return 1;
	cache_key = strtol(cache_line, 0, 0);
	return 0;
}

int MotionMain::get_cache_line(int64_t key)
{
	if( cache_key == key ) return 0;
	if( open_cache_file() ) return 1;
	if( cache_key >= 0 && key > cache_key ) {
		if( load_cache_line() ) return 1;
		if( cache_key == key ) return 0;
		if( cache_key > key ) return 1;
	}
// binary search file
	fseek(cache_fp, 0, SEEK_END);
	int64_t l = -1, r = ftell(cache_fp);
	while( (r - l) > 1 ) {
		int64_t m = (l + r) / 2;
		fseek(cache_fp, m, SEEK_SET);
		if( m > 0 && !fgets(cache_line, sizeof(cache_line), cache_fp) )
			return -1;
		if( !load_cache_line() ) {
			if( cache_key == key )
				return 0;
			if( cache_key < key ) { l = m; continue; }
		}
		r = m;
	}
	return 1;
}

int MotionMain::locate_cache_line(int64_t key)
{
	int ret = 1;
	if( key < 0 || !(ret=get_cache_line(key)) ||
	    ( cache_key >= 0 && cache_key < key ) )
		ret = load_cache_line();
	return ret;
}

int MotionMain::put_cache_line(const char *line)
{
	int64_t key = strtol(line, 0, 0);
	if( key == active_key ) return 1;
	if( !active_fp ) {
		close_cache_file();
		snprintf(cache_file, sizeof(cache_file), "%s.bak", config.tracking_file);
		::rename(config.tracking_file, cache_file);
		if( !(active_fp = fopen(config.tracking_file, "w")) ) {
			perror(config.tracking_file);
			fprintf(stderr, "err writing key %jd\n", key);
			return -1;
		}
		active_key = -1;
	}

	if( active_key < key ) {
		locate_cache_line(active_key);
		while( cache_key >= 0 && key >= cache_key ) {
			if( key > cache_key )
				fputs(cache_line, active_fp);
			load_cache_line();
		}
	}

	active_key = key;
	fputs(line, active_fp);
	fflush(active_fp);
	return 0;
}

void MotionMain::reset_cache_file()
{
	if( active_fp ) {
		locate_cache_line(active_key);
		while( cache_key >= 0 ) {
			fputs(cache_line, active_fp);
			load_cache_line();
		}
		close_cache_file();  ::remove(cache_file);
		fclose(active_fp); active_fp = 0; active_key = -1;
	}
	else
		close_cache_file();
	strcpy(cache_file, config.tracking_file);
}


RotateScanPackage::RotateScanPackage()
{
}

RotateScanUnit::RotateScanUnit(RotateScan *server, MotionMain *plugin)
 : LoadClient(server)
{
	this->server = server;
	this->plugin = plugin;
	rotater = 0;
	temp = 0;
}

RotateScanUnit::~RotateScanUnit()
{
	delete rotater;
	delete temp;
}

void RotateScanUnit::process_package(LoadPackage *package)
{
	if( server->skip ) return;
	RotateScanPackage *pkg = (RotateScanPackage*)package;

	if( (pkg->difference = server->get_cache(pkg->angle)) < 0 ) {
//printf("RotateScanUnit::process_package %d\n", __LINE__);
		int color_model = server->previous_frame->get_color_model();
		int pixel_size = BC_CModels::calculate_pixelsize(color_model);
		int row_bytes = server->previous_frame->get_bytes_per_line();

		if( !rotater )
			rotater = new AffineEngine(1, 1);
		if( !temp )
			temp = new VFrame(
				server->previous_frame->get_w(),
				server->previous_frame->get_h(),
				color_model, 0);
//printf("RotateScanUnit::process_package %d\n", __LINE__);


// Rotate original block size
// 		rotater->set_viewport(server->block_x1, server->block_y1,
// 			server->block_x2 - server->block_x1, server->block_y2 - server->block_y1);
		rotater->set_in_viewport(server->block_x1, server->block_y1,
			server->block_x2 - server->block_x1, server->block_y2 - server->block_y1);
		rotater->set_out_viewport(server->block_x1, server->block_y1,
			server->block_x2 - server->block_x1, server->block_y2 - server->block_y1);
//		rotater->set_pivot(server->block_x, server->block_y);
		rotater->set_in_pivot(server->block_x, server->block_y);
		rotater->set_out_pivot(server->block_x, server->block_y);
//printf("RotateScanUnit::process_package %d\n", __LINE__);
		rotater->rotate(temp, server->previous_frame, pkg->angle);

// Scan reduced block size
//plugin->output_frame->copy_from(server->current_frame);
//plugin->output_frame->copy_from(temp);
//printf("RotateScanUnit::process_package %d %d %d %d %d\n",
// __LINE__, server->scan_x, server->scan_y, server->scan_w, server->scan_h);
// Clamp coordinates
		int x1 = server->scan_x;
		int y1 = server->scan_y;
		int x2 = x1 + server->scan_w;
		int y2 = y1 + server->scan_h;
		x2 = MIN(temp->get_w(), x2);
		y2 = MIN(temp->get_h(), y2);
		x2 = MIN(server->current_frame->get_w(), x2);
		y2 = MIN(server->current_frame->get_h(), y2);
		x1 = MAX(0, x1);  y1 = MAX(0, y1);

		if( x2 > x1 && y2 > y1 ) {
			pkg->difference = MotionScan::abs_diff(
				temp->get_rows()[y1] + x1 * pixel_size,
				server->current_frame->get_rows()[y1] + x1 * pixel_size,
				row_bytes, x2 - x1, y2 - y1, color_model);
//printf("RotateScanUnit::process_package %d\n", __LINE__);
			server->put_cache(pkg->angle, pkg->difference);
		}
#if 0
	VFrame png(x2-x1, y2-y1, BC_RGB888, -1);
	png.transfer_from(temp, 0, x1, y1, x2-x1, y2-y1);
	char fn[64];
	sprintf(fn,"%s%f.png","/tmp/temp",pkg->angle); png.write_png(fn);
	png.transfer_from(server->current_frame, 0, x1, y1, x2-x1, y2-y1);
	sprintf(fn,"%s%f.png","/tmp/curr",pkg->angle); png.write_png(fn);
printf("RotateScanUnit::process_package 10 x=%d y=%d w=%d h=%d block_x=%d block_y=%d angle=%f scan_w=%d scan_h=%d diff=%jd\n",
 server->block_x1, server->block_y1, server->block_x2 - server->block_x1, server->block_y2 - server->block_y1,
 server->block_x,  server->block_y,  pkg->angle,  server->scan_w, server->scan_h, pkg->difference);
#endif
	}
}


RotateScan::RotateScan(MotionMain *plugin,
	int total_clients,
	int total_packages)
 : LoadServer( //1, 1)
		total_clients, total_packages)
{
	this->plugin = plugin;
	cache_lock = new Mutex("RotateScan::cache_lock");
}


RotateScan::~RotateScan()
{
	delete cache_lock;
}

void RotateScan::init_packages()
{
	for( int i = 0; i < get_total_packages(); i++ ) {
		RotateScanPackage *pkg = (RotateScanPackage*)get_package(i);
		pkg->angle = scan_angle1 +
			i * (scan_angle2 - scan_angle1) / (total_steps - 1);
	}
}

LoadClient* RotateScan::new_client()
{
	return new RotateScanUnit(this, plugin);
}

LoadPackage* RotateScan::new_package()
{
	return new RotateScanPackage;
}


float RotateScan::scan_frame(VFrame *previous_frame, VFrame *current_frame,
	int block_x, int block_y)
{
	skip = 0;
	this->block_x = block_x;
	this->block_y = block_y;

//printf("RotateScan::scan_frame %d\n", __LINE__);
	switch(plugin->config.tracking_type) {
	case MotionScan::NO_CALCULATE:
		result = plugin->config.rotation_center;
		skip = 1;
		break;

	case MotionScan::LOAD:
	case MotionScan::SAVE:
		if( plugin->load_ok ) {
			result = plugin->load_dt;
			skip = 1;
		}
		break;
	}

	this->previous_frame = previous_frame;
	this->current_frame = current_frame;
	int w = current_frame->get_w();
	int h = current_frame->get_h();
	int block_w = w * plugin->config.global_block_w / 100;
	int block_h = h * plugin->config.global_block_h / 100;

	if( this->block_x - block_w / 2 < 0 ) block_w = this->block_x * 2;
	if( this->block_y - block_h / 2 < 0 ) block_h = this->block_y * 2;
	if( this->block_x + block_w / 2 > w ) block_w = (w - this->block_x) * 2;
	if( this->block_y + block_h / 2 > h ) block_h = (h - this->block_y) * 2;

	block_x1 = this->block_x - block_w / 2;
	block_x2 = this->block_x + block_w / 2;
	block_y1 = this->block_y - block_h / 2;
	block_y2 = this->block_y + block_h / 2;

// Calculate the maximum area available to scan after rotation.
// Must be calculated from the starting range because of cache.
// Get coords of rectangle after rotation.
	double center_x = this->block_x;
	double center_y = this->block_y;
	double max_angle = plugin->config.rotation_range;
	double base_angle1 = atan((float)block_h / block_w);
	double base_angle2 = atan((float)block_w / block_h);
	double target_angle1 = base_angle1 + max_angle * 2 * M_PI / 360;
	double target_angle2 = base_angle2 + max_angle * 2 * M_PI / 360;
	double radius = sqrt(block_w * block_w + block_h * block_h) / 2;
	double x1 = center_x - cos(target_angle1) * radius;
	double y1 = center_y - sin(target_angle1) * radius;
	double x2 = center_x + sin(target_angle2) * radius;
	double y2 = center_y - cos(target_angle2) * radius;
	double x3 = center_x - sin(target_angle2) * radius;
	double y3 = center_y + cos(target_angle2) * radius;

// Track top edge to find greatest area.
	double max_area1 = 0;
	//double max_x1 = 0;
	double max_y1 = 0;
	for( double x = x1; x < x2; x++ ) {
		double y = y1 + (y2 - y1) * (x - x1) / (x2 - x1);
		if( x >= center_x && x < block_x2 && y >= block_y1 && y < center_y ) {
			double area = fabs(x - center_x) * fabs(y - center_y);
			if( area > max_area1 ) {
				max_area1 = area;
				//max_x1 = x;
				max_y1 = y;
			}
		}
	}

// Track left edge to find greatest area.
	double max_area2 = 0;
	double max_x2 = 0;
	//double max_y2 = 0;
	for( double y = y1; y < y3; y++ ) {
		double x = x1 + (x3 - x1) * (y - y1) / (y3 - y1);
		if( x >= block_x1 && x < center_x && y >= block_y1 && y < center_y ) {
			double area = fabs(x - center_x) * fabs(y - center_y);
			if( area > max_area2 ) {
				max_area2 = area;
				max_x2 = x;
				//max_y2 = y;
			}
		}
	}

	double max_x, max_y;
	max_x = max_x2;
	max_y = max_y1;

// Get reduced scan coords
	scan_w = (int)(fabs(max_x - center_x) * 2);
	scan_h = (int)(fabs(max_y - center_y) * 2);
	scan_x = (int)(center_x - scan_w / 2);
	scan_y = (int)(center_y - scan_h / 2);
// printf("RotateScan::scan_frame center=%d,%d scan=%d,%d %dx%d\n",
// this->block_x, this->block_y, scan_x, scan_y, scan_w, scan_h);
// printf("    angle_range=%f block= %d,%d,%d,%d\n", max_angle, block_x1, block_y1, block_x2, block_y2);

// Determine min angle from size of block
	double angle1 = atan((double)block_h / block_w);
	double angle2 = atan((double)(block_h - 1) / (block_w + 1));
	double min_angle = fabs(angle2 - angle1) / OVERSAMPLE;
	min_angle = MAX(min_angle, MIN_ANGLE);

//printf("RotateScan::scan_frame %d min_angle=%f\n", __LINE__, min_angle * 360 / 2 / M_PI);

	cache.remove_all_objects();


	if( !skip ) {
		if( previous_frame->data_matches(current_frame) ) {
//printf("RotateScan::scan_frame: frames match.  Skipping.\n");
			result = plugin->config.rotation_center;
			skip = 1;
		}
	}

	if( !skip ) {
// Initial search range
		float angle_range = max_angle;
		result = plugin->config.rotation_center;
		total_steps = plugin->config.rotate_positions;


		while( angle_range >= min_angle * total_steps ) {
			scan_angle1 = result - angle_range;
			scan_angle2 = result + angle_range;

			set_package_count(total_steps);
//set_package_count(1);
			process_packages();

			int64_t min_difference = -1;
			for( int i = 0; i < get_total_packages(); i++ ) {
				RotateScanPackage *pkg = (RotateScanPackage*)get_package(i);
				if( pkg->difference < min_difference || min_difference == -1 ) {
					min_difference = pkg->difference;
					result = pkg->angle;
				}
//break;
			}

			angle_range /= 2;

//break;
		}
	}

	if( plugin->config.tracking_type == MotionScan::SAVE ) {
		plugin->save_dt = result;
	}
//printf("RotateScan::scan_frame %d angle=%f\n", __LINE__, result);
	return result;
}

int64_t RotateScan::get_cache(float angle)
{
	int64_t result = -1;
	cache_lock->lock("RotateScan::get_cache");
	for( int i = 0; i < cache.total; i++ ) {
		RotateScanCache *ptr = cache.values[i];
		if( fabs(ptr->angle - angle) <= MIN_ANGLE ) {
			result = ptr->difference;
			break;
		}
	}
	cache_lock->unlock();
	return result;
}

void RotateScan::put_cache(float angle, int64_t difference)
{
	RotateScanCache *ptr = new RotateScanCache(angle, difference);
	cache_lock->lock("RotateScan::put_cache");
	cache.append(ptr);
	cache_lock->unlock();
}


RotateScanCache::RotateScanCache(float angle, int64_t difference)
{
	this->angle = angle;
	this->difference = difference;
}



