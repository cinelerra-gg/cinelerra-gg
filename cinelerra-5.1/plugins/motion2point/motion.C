
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

#include "affine.h"
#include "../motion-hv/motionscan-hv.h"
#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "motion.h"
#include "motionwindow.h"
#include "mutex.h"
#include "overlayframe.h"
#include "rotateframe.h"
#include "transportque.h"


#include <errno.h>
#include <unistd.h>

REGISTER_PLUGIN(MotionMain2)

//#undef DEBUG

#ifndef DEBUG
#define DEBUG
#endif


MotionConfig::MotionConfig()
{
	for(int i = 0; i < TOTAL_POINTS; i++)
	{
		global[i] = 1;
		global_range_w[i] = 5;
		global_range_h[i] = 5;
		global_origin_x[i] = 0;
		global_origin_y[i] = 0;
		global_block_w[i] = MIN_BLOCK;
		global_block_h[i] = MIN_BLOCK;
		block_x[i] = 50;
		block_y[i] = 50;
		draw_vectors[i] = 1;
	}

	global_positions = 256;
	magnitude = 100;
	return_speed = 0;
	action = MotionScan::STABILIZE;
	calculation = MotionScan::NO_CALCULATE;
	tracking_object = MotionScan::TRACK_SINGLE;
	track_frame = 0;
	bottom_is_master = 1;
	horizontal_only = 0;
	vertical_only = 0;
}

void MotionConfig::boundaries()
{
	for(int i = 0; i < TOTAL_POINTS; i++)
	{
		CLAMP(global_range_w[i], MIN_RADIUS, MAX_RADIUS);
		CLAMP(global_range_h[i], MIN_RADIUS, MAX_RADIUS);
		CLAMP(global_origin_x[i], MIN_ORIGIN, MAX_ORIGIN);
		CLAMP(global_origin_y[i], MIN_ORIGIN, MAX_ORIGIN);
		CLAMP(global_block_w[i], MIN_BLOCK, MAX_BLOCK);
		CLAMP(global_block_h[i], MIN_BLOCK, MAX_BLOCK);
	}

}

int MotionConfig::equivalent(MotionConfig &that)
{
	int result = 1;
	for(int i = 0; i < TOTAL_POINTS; i++)
	{
		if(global[i] != that.global[i] ||
		   global_range_w[i] != that.global_range_w[i] ||
		   global_range_h[i] != that.global_range_h[i] ||
		   draw_vectors[i] != that.draw_vectors[i] ||
		   global_block_w[i] != that.global_block_w[i] ||
		   global_block_h[i] != that.global_block_h[i] ||
		   global_origin_x[i] != that.global_origin_x[i] ||
		   global_origin_y[i] != that.global_origin_y[i] ||
		   !EQUIV(block_x[i], that.block_x[i]) ||
		   !EQUIV(block_y[i], that.block_y[i]))
		   result = 0;
	}

	if(magnitude != that.magnitude ||
		return_speed != that.return_speed ||
		action != that.action ||
		calculation != that.calculation ||
		tracking_object != that.tracking_object ||
		track_frame != that.track_frame ||
		bottom_is_master != that.bottom_is_master ||
		horizontal_only != that.horizontal_only ||
		vertical_only != that.vertical_only ||
 	    global_positions != that.global_positions) result = 0;

	return result;
}

void MotionConfig::copy_from(MotionConfig &that)
{
	for(int i = 0; i < TOTAL_POINTS; i++)
	{
		global[i] = that.global[i];
		global_range_w[i] = that.global_range_w[i];
		global_range_h[i] = that.global_range_h[i];
		global_origin_x[i] = that.global_origin_x[i];
		global_origin_y[i] = that.global_origin_y[i];
		draw_vectors[i] = that.draw_vectors[i];
		block_x[i] = that.block_x[i];
		block_y[i] = that.block_y[i];
		global_block_w[i] = that.global_block_w[i];
		global_block_h[i] = that.global_block_h[i];
	}

	global_positions = that.global_positions;
	magnitude = that.magnitude;
	return_speed = that.return_speed;
	action = that.action;
	calculation = that.calculation;
	tracking_object = that.tracking_object;
	track_frame = that.track_frame;
	bottom_is_master = that.bottom_is_master;
	horizontal_only = that.horizontal_only;
	vertical_only = that.vertical_only;
}

void MotionConfig::interpolate(MotionConfig &prev,
	MotionConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	//double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	//double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	for(int i = 0; i < TOTAL_POINTS; i++)
	{
		this->global[i] = prev.global[i];
		this->block_x[i] = prev.block_x[i];
		this->block_y[i] = prev.block_y[i];
		this->global_range_w[i] = prev.global_range_w[i];
		this->global_range_h[i] = prev.global_range_h[i];
		this->global_origin_x[i] = prev.global_origin_x[i];
		this->global_origin_y[i] = prev.global_origin_y[i];
		this->draw_vectors[i] = prev.draw_vectors[i];
		this->global_block_w[i] = prev.global_block_w[i];
		this->global_block_h[i] = prev.global_block_h[i];
	}

	this->global_positions = prev.global_positions;
	magnitude = prev.magnitude;
	return_speed = prev.return_speed;
	action = prev.action;
	calculation = prev.calculation;
	tracking_object = prev.tracking_object;
	track_frame = prev.track_frame;
	bottom_is_master = prev.bottom_is_master;
	horizontal_only = prev.horizontal_only;
	vertical_only = prev.vertical_only;
}



















MotionMain2::MotionMain2(PluginServer *server)
 : PluginVClient(server)
{

	engine = 0;
	affine = 0;
	for(int i = 0; i < TOTAL_POINTS; i++)
	{
		total_dx[i] = 0;
		total_dy[i] = 0;
	}
	overlayer = 0;
	search_area = 0;
	search_size = 0;
	temp_frame = 0;
	previous_frame_number = -1;

	prev_global_ref = 0;
	current_global_ref = 0;
	global_target_src = 0;
	global_target_dst = 0;
}

MotionMain2::~MotionMain2()
{

	delete engine;
	delete affine;
	delete overlayer;
	delete [] search_area;
	delete temp_frame;


	delete prev_global_ref;
	delete current_global_ref;
	delete global_target_src;
	delete global_target_dst;
}

const char* MotionMain2::plugin_title() { return N_("Motion 2 Point"); }
int MotionMain2::is_realtime() { return 1; }
int MotionMain2::is_multichannel() { return 1; }


NEW_WINDOW_MACRO(MotionMain2, MotionWindow)

LOAD_CONFIGURATION_MACRO(MotionMain2, MotionConfig)



void MotionMain2::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("MotionMain2::update_gui");

			char string[BCTEXTLEN];

			for(int i = 0; i < TOTAL_POINTS; i++)
			{
				((MotionWindow*)thread->window)->global[i]->update(config.global[i]);

				((MotionWindow*)thread->window)->global_block_w[i]->update(config.global_block_w[i]);
				((MotionWindow*)thread->window)->global_block_h[i]->update(config.global_block_h[i]);
				((MotionWindow*)thread->window)->global_origin_x[i]->update(config.global_origin_x[i]);
				((MotionWindow*)thread->window)->global_origin_y[i]->update(config.global_origin_y[i]);
				((MotionWindow*)thread->window)->block_x[i]->update(config.block_x[i]);
				((MotionWindow*)thread->window)->block_y[i]->update(config.block_y[i]);
				((MotionWindow*)thread->window)->block_x_text[i]->update((float)config.block_x[i]);
				((MotionWindow*)thread->window)->block_y_text[i]->update((float)config.block_y[i]);
				((MotionWindow*)thread->window)->vectors[i]->update(config.draw_vectors[i]);
			}

			sprintf(string, "%d", config.global_positions);
			((MotionWindow*)thread->window)->global_search_positions->set_text(string);
			((MotionWindow*)thread->window)->magnitude->update(config.magnitude);
			((MotionWindow*)thread->window)->return_speed->update(config.return_speed);


			((MotionWindow*)thread->window)->track_single->update(config.tracking_object == MotionScan::TRACK_SINGLE);
			((MotionWindow*)thread->window)->track_frame_number->update(config.track_frame);
			((MotionWindow*)thread->window)->track_previous->update(config.tracking_object == MotionScan::TRACK_PREVIOUS);
			((MotionWindow*)thread->window)->previous_same->update(config.tracking_object == MotionScan::PREVIOUS_SAME_BLOCK);
			if(config.tracking_object != MotionScan::TRACK_SINGLE)
				((MotionWindow*)thread->window)->track_frame_number->disable();
			else
				((MotionWindow*)thread->window)->track_frame_number->enable();

			((MotionWindow*)thread->window)->action->set_text(
				Action::to_text(config.action));
			((MotionWindow*)thread->window)->calculation->set_text(
				Calculation::to_text(config.calculation));
			((MotionWindow*)thread->window)->tracking_direction->set_text(
				TrackingDirection::to_text(config.horizontal_only, config.vertical_only));
			((MotionWindow*)thread->window)->master_layer->set_text(
				MasterLayer::to_text(config.bottom_is_master));


			((MotionWindow*)thread->window)->update_mode();
			thread->window->unlock_window();
		}
	}
}





void MotionMain2::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("MOTION2");

	char string[BCTEXTLEN];
	for(int i = 0; i < TOTAL_POINTS; i++)
	{
		sprintf(string, "GLOBAL%d", i);
		output.tag.set_property(string, config.global[i]);
		sprintf(string, "GLOBAL_BLOCK_W%d", i);
		output.tag.set_property(string, config.global_block_w[i]);
		sprintf(string, "GLOBAL_BLOCK_H%d", i);
		output.tag.set_property(string, config.global_block_h[i]);
		sprintf(string, "BLOCK_X%d", i);
		output.tag.set_property(string, config.block_x[i]);
		sprintf(string, "BLOCK_Y%d", i);
		output.tag.set_property(string, config.block_y[i]);
		sprintf(string, "GLOBAL_RANGE_W%d", i);
		output.tag.set_property(string, config.global_range_w[i]);
		sprintf(string, "GLOBAL_RANGE_H%d", i);
		output.tag.set_property(string, config.global_range_h[i]);
		sprintf(string, "GLOBAL_ORIGIN_X%d", i);
		output.tag.set_property(string, config.global_origin_x[i]);
		sprintf(string, "GLOBAL_ORIGIN_Y%d", i);
		output.tag.set_property(string, config.global_origin_y[i]);
		sprintf(string, "DRAW_VECTORS%d", i);
		output.tag.set_property(string, config.draw_vectors[i]);
	}

	output.tag.set_property("GLOBAL_POSITIONS", config.global_positions);
	output.tag.set_property("MAGNITUDE", config.magnitude);
	output.tag.set_property("RETURN_SPEED", config.return_speed);
	output.tag.set_property("ACTION_TYPE", config.action);
	output.tag.set_property("TRACKING_TYPE", config.calculation);
	output.tag.set_property("TRACKING_OBJECT", config.tracking_object);
	output.tag.set_property("TRACK_FRAME", config.track_frame);
	output.tag.set_property("BOTTOM_IS_MASTER", config.bottom_is_master);
	output.tag.set_property("HORIZONTAL_ONLY", config.horizontal_only);
	output.tag.set_property("VERTICAL_ONLY", config.vertical_only);
	output.append_tag();
	output.tag.set_title("/MOTION2");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void MotionMain2::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("MOTION2"))
			{
				char string[BCTEXTLEN];
				for(int i = 0; i < TOTAL_POINTS; i++)
				{
					sprintf(string, "GLOBAL%d", i);
					config.global[i] = input.tag.get_property(string, config.global[i]);
					sprintf(string, "GLOBAL_BLOCK_W%d", i);
					config.global_block_w[i] = input.tag.get_property(string, config.global_block_w[i]);
					sprintf(string, "GLOBAL_BLOCK_H%d", i);
					config.global_block_h[i] = input.tag.get_property(string, config.global_block_h[i]);
					sprintf(string, "BLOCK_X%d", i);
					config.block_x[i] = input.tag.get_property(string, config.block_x[i]);
					sprintf(string, "BLOCK_Y%d", i);
					config.block_y[i] = input.tag.get_property(string, config.block_y[i]);
					sprintf(string, "GLOBAL_RANGE_W%d", i);
					config.global_range_w[i] = input.tag.get_property(string, config.global_range_w[i]);
					sprintf(string, "GLOBAL_RANGE_H%d", i);
					config.global_range_h[i] = input.tag.get_property(string, config.global_range_h[i]);
					sprintf(string, "GLOBAL_ORIGIN_X%d", i);
					config.global_origin_x[i] = input.tag.get_property(string, config.global_origin_x[i]);
					sprintf(string, "GLOBAL_ORIGIN_Y%d", i);
					config.global_origin_y[i] = input.tag.get_property(string, config.global_origin_y[i]);
					sprintf(string, "DRAW_VECTORS%d", i);
					config.draw_vectors[i] = input.tag.get_property(string, config.draw_vectors[i]);
				}

				config.global_positions = input.tag.get_property("GLOBAL_POSITIONS", config.global_positions);
				config.magnitude = input.tag.get_property("MAGNITUDE", config.magnitude);
				config.return_speed = input.tag.get_property("RETURN_SPEED", config.return_speed);
				config.action = input.tag.get_property("ACTION_TYPE", config.action);
				config.calculation = input.tag.get_property("TRACKING_TYPE", config.calculation);
				config.tracking_object = input.tag.get_property("TRACKING_OBJECT", config.tracking_object);
				config.track_frame = input.tag.get_property("TRACK_FRAME", config.track_frame);
				config.bottom_is_master = input.tag.get_property("BOTTOM_IS_MASTER", config.bottom_is_master);
				config.horizontal_only = input.tag.get_property("HORIZONTAL_ONLY", config.horizontal_only);
				config.vertical_only = input.tag.get_property("VERTICAL_ONLY", config.vertical_only);
			}
		}
	}
	config.boundaries();
}









void MotionMain2::allocate_temp(int w, int h, int color_model)
{
	if(temp_frame &&
		(temp_frame->get_w() != w ||
		temp_frame->get_h() != h))
	{
		delete temp_frame;
		temp_frame = 0;
	}
	if(!temp_frame)
		temp_frame = new VFrame(w, h, color_model, 0);
}



void MotionMain2::scan_motion(int point)
{
	int w = current_global_ref->get_w();
	int h = current_global_ref->get_h();


	if(!engine) engine = new MotionScan(PluginClient::get_project_smp() + 1,
		PluginClient::get_project_smp() + 1);

// Get the current motion vector between the previous and current frame
	engine->scan_frame(current_global_ref,
		prev_global_ref,
		config.global_range_w[point] * w / 100,
		config.global_range_h[point] * h / 100,
		config.global_block_w[point] * w / 100,
		config.global_block_h[point] * h / 100,
		config.block_x[point] * w / 100,
		config.block_y[point] * h / 100,
		config.tracking_object,
		config.calculation,
		config.action,
		config.horizontal_only,
		config.vertical_only,
		get_source_position(),
		total_dx[point],
		total_dy[point],
		config.global_origin_x[point] * w / 100,
		config.global_origin_y[point] * h / 100,
		1,
		0,
		0,
		0);

	current_dx[point] = engine->dx_result;
	current_dy[point] = engine->dy_result;

// Add current motion vector to accumulation vector.
	if(config.tracking_object != MotionScan::TRACK_SINGLE)
	{
// Retract over time
		total_dx[point] = (int64_t)total_dx[point] * (100 - config.return_speed) / 100;
		total_dy[point] = (int64_t)total_dy[point] * (100 - config.return_speed) / 100;
		total_dx[point] += engine->dx_result;
		total_dy[point] += engine->dy_result;
	}
	else
// Make accumulation vector current
	{
		total_dx[point] = engine->dx_result;
		total_dy[point] = engine->dy_result;
// printf("MotionMain2::scan_motion %d %d %d %d\n",
// __LINE__,
// point,
// total_dx[point],
// total_dy[point]);
	}

// Clamp accumulation vector
	if(config.magnitude < 100)
	{
		//int block_w = (int64_t)config.global_block_w[point] *
		//		current_global_ref->get_w() / 100;
		//int block_h = (int64_t)config.global_block_h[point] *
		//		current_global_ref->get_h() / 100;
		int block_x_orig = (int64_t)(config.block_x[point] *
			current_global_ref->get_w() /
			100);
		int block_y_orig = (int64_t)(config.block_y[point] *
			current_global_ref->get_h() /
			100);

		int max_block_x = (int64_t)(current_global_ref->get_w() - block_x_orig) *
			OVERSAMPLE *
			config.magnitude /
			100;
		int max_block_y = (int64_t)(current_global_ref->get_h() - block_y_orig) *
			OVERSAMPLE *
			config.magnitude /
			100;
		int min_block_x = (int64_t)-block_x_orig *
			OVERSAMPLE *
			config.magnitude /
			100;
		int min_block_y = (int64_t)-block_y_orig *
			OVERSAMPLE *
			config.magnitude /
			100;

		CLAMP(total_dx[point], min_block_x, max_block_x);
		CLAMP(total_dy[point], min_block_y, max_block_y);
	}

// printf("MotionMain2::scan_motion %d %d %d %d\n",
// __LINE__,
// point,
// total_dx[point],
// total_dy[point]);


}




void MotionMain2::apply_motion()
{
	if(config.tracking_object != MotionScan::TRACK_SINGLE)
	{
// Transfer current reference frame to previous reference frame and update
// counter.
		prev_global_ref->copy_from(current_global_ref);
		previous_frame_number = get_source_position();
	}

// Decide what to do with target based on requested operation
	//int interpolation; //   variable set but not used
	float origin_x[TOTAL_POINTS];
	float origin_y[TOTAL_POINTS];
	float end_x[TOTAL_POINTS];
	float end_y[TOTAL_POINTS];
	for(int i = 0; i < TOTAL_POINTS; i++)
	{
		get_current_vector(&origin_x[i],
			&origin_y[i],
			0,
			0,
			&end_x[i],
			&end_y[i],
			i);
	}

// Calculate rotation if 2 points
	double angle = 0.0;
	double angle0 = 0.0;
	double zoom = 1.0;
	if(config.global[ROTATION_POINT])
	{
			if(origin_x[1] - origin_x[0])
				angle0 = atan((double)(origin_y[0] - origin_y[1]) /
					(double)(origin_x[0] - origin_x[1]));
			if(end_x[1] - end_x[0])
				angle = atan((double)(end_y[0] - end_y[1]) /
					(double)(end_x[0] - end_x[1]));
// printf("MotionMain2::apply_motion %d angle0=%f angle=%f\n",
// __LINE__,
// angle0 * 360 / 2 / M_PI,
// angle * 360 / 2 / M_PI);

			angle -= angle0;

// Calculate zoom
//			zoom = DISTANCE(origin_x[1], origin_y[1], origin_x[0], origin_y[0]) /
//				DISTANCE(end_x[1], end_y[1], end_x[0], end_y[0]);

	}

printf("MotionMain2::apply_motion %d total_dx=%.02f total_dy=%.02f angle=%f zoom=%f\n",
__LINE__,
(float)total_dx[TRANSLATION_POINT] / OVERSAMPLE,
(float)total_dy[TRANSLATION_POINT] / OVERSAMPLE,
angle * 360 / 2 / M_PI,
zoom);

// Calculate translation
	float dx = 0.0;
	float dy = 0.0;
	switch(config.action)
	{
		case MotionScan::NOTHING:
			global_target_dst->copy_from(global_target_src);
			break;
		case MotionScan::TRACK:
			//interpolation = CUBIC_LINEAR;
			dx = (float)total_dx[0] / OVERSAMPLE;
			dy = (float)total_dy[0] / OVERSAMPLE;
			break;
		case MotionScan::TRACK_PIXEL:
			//interpolation = NEAREST_NEIGHBOR;
			dx = (int)(total_dx[0] / OVERSAMPLE);
			dy = (int)(total_dy[0] / OVERSAMPLE);
			break;
			//interpolation = NEAREST_NEIGHBOR;
			dx = -(int)(total_dx[0] / OVERSAMPLE);
			dy = -(int)(total_dy[0] / OVERSAMPLE);
			angle *= -1;
			break;
		case MotionScan::STABILIZE:
			//interpolation = CUBIC_LINEAR;
			dx = -(float)total_dx[0] / OVERSAMPLE;
			dy = -(float)total_dy[0] / OVERSAMPLE;
			angle *= -1;
			break;
	}





	if(config.action != MotionScan::NOTHING)
	{
		double w = get_output()->get_w();
		double h = get_output()->get_h();
		double pivot_x = end_x[0];
		double pivot_y = end_y[0];
		double angle1 = atan((double)pivot_y / (double)pivot_x) + angle;
		double angle2 = atan((double)(w - pivot_x) / (double)pivot_y) + angle;
		double angle3 = atan((double)(h - pivot_y) / (double)(w - pivot_x)) + angle;
		double angle4 = atan((double)pivot_x / (double)(h - pivot_y)) + angle;
		double radius1 = DISTANCE(0, 0, pivot_x, pivot_y) * zoom;
		double radius2 = DISTANCE(w, 0, pivot_x, pivot_y) * zoom;
		double radius3 = DISTANCE(w, h, pivot_x, pivot_y) * zoom;
		double radius4 = DISTANCE(0, h, pivot_x, pivot_y) * zoom;


		float x1 = (dx + pivot_x - cos(angle1) * radius1) * 100 / w;
		float y1 = (dy + pivot_y - sin(angle1) * radius1) * 100 / h;
		float x2 = (dx + pivot_x + sin(angle2) * radius2) * 100 / w;
		float y2 = (dy + pivot_y - cos(angle2) * radius2) * 100 / h;
		float x3 = (dx + pivot_x + cos(angle3) * radius3) * 100 / w;
		float y3 = (dy + pivot_y + sin(angle3) * radius3) * 100 / h;
		float x4 = (dx + pivot_x - sin(angle4) * radius4) * 100 / w;
		float y4 = (dy + pivot_y + cos(angle4) * radius4) * 100 / h;


		if(!affine)
			affine = new AffineEngine(PluginClient::get_project_smp() + 1,
				PluginClient::get_project_smp() + 1);
		global_target_dst->clear_frame();


// printf("MotionMain2::apply_motion %d %.02f %.02f %.02f %.02f %.02f %.02f %.02f %.02f\n",
// __LINE__,
// x1,
// y1,
// x2,
// y2,
// x3,
// y3,
// x4,
// y4);

		affine->process(global_target_dst,
			global_target_src,
			0,
			AffineEngine::PERSPECTIVE,
			x1,
			y1,
			x2,
			y2,
			x3,
			y3,
			x4,
			y4,
			1);
	}
}






int MotionMain2::process_buffer(VFrame **frame,
	int64_t start_position,
	double frame_rate)
{
	int need_reconfigure = load_configuration();
	int color_model = frame[0]->get_color_model();
	w = frame[0]->get_w();
	h = frame[0]->get_h();


#ifdef DEBUG
printf("MotionMain2::process_buffer 1 start_position=%jd\n", start_position);
#endif


// Calculate the source and destination pointers for each of the operations.
// Get the layer to track motion in.
	reference_layer = config.bottom_is_master ?
		PluginClient::total_in_buffers - 1 :
		0;
// Get the layer to apply motion in.
	target_layer = config.bottom_is_master ?
		0 :
		PluginClient::total_in_buffers - 1;


	output_frame = frame[target_layer];


// Get the position of previous reference frame.
	int64_t actual_previous_number;
// Skip if match frame not available
	int skip_current = 0;


	if(config.tracking_object == MotionScan::TRACK_SINGLE)
	{
		actual_previous_number = config.track_frame;
		if(get_direction() == PLAY_REVERSE)
			actual_previous_number++;
		if(actual_previous_number == start_position)
			skip_current = 1;
	}
	else
	{
		actual_previous_number = start_position;
		if(get_direction() == PLAY_FORWARD)
		{
			actual_previous_number--;
			if(actual_previous_number < get_source_start())
				skip_current = 1;
			else
			{
				KeyFrame *keyframe = get_prev_keyframe(start_position, 1);
				if(keyframe->position > 0 &&
					actual_previous_number < keyframe->position)
					skip_current = 1;
			}
		}
		else
		{
			actual_previous_number++;
			if(actual_previous_number >= get_source_start() + get_total_len())
				skip_current = 1;
			else
			{
				KeyFrame *keyframe = get_next_keyframe(start_position, 1);
				if(keyframe->position > 0 &&
					actual_previous_number >= keyframe->position)
					skip_current = 1;
			}
		}

// Only count motion since last keyframe


	}


// Point 0 must be tracked for any other points to be tracked
// Action and Calculation must be something
	if( !config.global[0] || ( config.action == MotionScan::NOTHING &&
			config.calculation == MotionScan::NO_CALCULATE) )
		skip_current = 1;


// printf("process_buffer %d %lld %lld %d\n",
// skip_current,
// previous_frame_number,
// actual_previous_number,
// need_reconfigure);
// Load match frame and reset vectors
	int need_reload = !skip_current &&
		(previous_frame_number != actual_previous_number ||
		need_reconfigure);
	if(need_reload)
	{
		for(int i = 0; i < TOTAL_POINTS; i++)
		{
			total_dx[i] = 0;
			total_dy[i] = 0;
		}
		previous_frame_number = actual_previous_number;
	}


	if(skip_current)
	{
		for(int i = 0; i < TOTAL_POINTS; i++)
		{
			total_dx[i] = 0;
			total_dy[i] = 0;
			current_dx[i] = 0;
			current_dy[i] = 0;
		}
	}




// Get the global pointers.  Here we walk through the sequence of events.
	if(config.global[0])
	{
// Global reads previous frame and compares
// with current frame to get the current translation.
// The center of the search area is fixed in compensate mode or
// the user value + the accumulation vector in track mode.
		if(!prev_global_ref)
			prev_global_ref = new VFrame(w, h, color_model, 0);
		if(!current_global_ref)
			current_global_ref = new VFrame(w, h, color_model, 0);

// Global loads the current target frame into the src and
// writes it to the dst frame with desired translation.
		if(!global_target_src)
			global_target_src = new VFrame(w, h, color_model, 0);
		if(!global_target_dst)
			global_target_dst = new VFrame(w, h, color_model, 0);


// Load the global frames
		if(need_reload)
		{
			read_frame(prev_global_ref,
				reference_layer,
				previous_frame_number,
				frame_rate,
				0);
		}

		read_frame(current_global_ref,
			reference_layer,
			start_position,
			frame_rate,
			0);
		read_frame(global_target_src,
			target_layer,
			start_position,
			frame_rate,
			0);
	}








	if(!skip_current)
	{
// Get position change from previous frame to current frame
		if(config.global[0])
		{
			for(int i = 0; i < TOTAL_POINTS; i++)
				if(config.global[i]) scan_motion(i);
		}

		apply_motion();
	}




//printf("MotionMain2::process_buffer 90 %d\n", skip_current);

// Transfer the relevant target frame to the output
	if(!skip_current)
	{
		frame[target_layer]->copy_from(global_target_dst);
	}
	else
// Read the target destination directly
	{
		read_frame(frame[target_layer],
			target_layer,
			start_position,
			frame_rate,
			0);
	}

	if(config.draw_vectors)
	{
		for(int i = 0; i < TOTAL_POINTS; i++)
			draw_vectors(frame[target_layer], i);
	}

#ifdef DEBUG
printf("MotionMain2::process_buffer 100 skip_current=%d\n", skip_current);
#endif
	return 0;
}



#if 0

void MotionMain2::clamp_scan(int w,
	int h,
	int *block_x1,
	int *block_y1,
	int *block_x2,
	int *block_y2,
	int *scan_x1,
	int *scan_y1,
	int *scan_x2,
	int *scan_y2,
	int use_absolute)
{
// printf("MotionMain2::clamp_scan 1 w=%d h=%d block=%d %d %d %d scan=%d %d %d %d absolute=%d\n",
// w,
// h,
// *block_x1,
// *block_y1,
// *block_x2,
// *block_y2,
// *scan_x1,
// *scan_y1,
// *scan_x2,
// *scan_y2,
// use_absolute);

	if(use_absolute)
	{
// scan is always out of range before block.
		if(*scan_x1 < 0)
		{
			int difference = -*scan_x1;
			*block_x1 += difference;
			*scan_x1 = 0;
		}

		if(*scan_y1 < 0)
		{
			int difference = -*scan_y1;
			*block_y1 += difference;
			*scan_y1 = 0;
		}

		if(*scan_x2 > w)
		{
			int difference = *scan_x2 - w;
			*block_x2 -= difference;
			*scan_x2 -= difference;
		}

		if(*scan_y2 > h)
		{
			int difference = *scan_y2 - h;
			*block_y2 -= difference;
			*scan_y2 -= difference;
		}

		CLAMP(*scan_x1, 0, w);
		CLAMP(*scan_y1, 0, h);
		CLAMP(*scan_x2, 0, w);
		CLAMP(*scan_y2, 0, h);
	}
	else
	{
		if(*scan_x1 < 0)
		{
			int difference = -*scan_x1;
			*block_x1 += difference;
			*scan_x2 += difference;
			*scan_x1 = 0;
		}

		if(*scan_y1 < 0)
		{
			int difference = -*scan_y1;
			*block_y1 += difference;
			*scan_y2 += difference;
			*scan_y1 = 0;
		}

		if(*scan_x2 - *block_x1 + *block_x2 > w)
		{
			int difference = *scan_x2 - *block_x1 + *block_x2 - w;
			*block_x2 -= difference;
		}

		if(*scan_y2 - *block_y1 + *block_y2 > h)
		{
			int difference = *scan_y2 - *block_y1 + *block_y2 - h;
			*block_y2 -= difference;
		}

// 		CLAMP(*scan_x1, 0, w - (*block_x2 - *block_x1));
// 		CLAMP(*scan_y1, 0, h - (*block_y2 - *block_y1));
// 		CLAMP(*scan_x2, 0, w - (*block_x2 - *block_x1));
// 		CLAMP(*scan_y2, 0, h - (*block_y2 - *block_y1));
	}

// Sanity checks which break the calculation but should never happen if the
// center of the block is inside the frame.
	CLAMP(*block_x1, 0, w);
	CLAMP(*block_x2, 0, w);
	CLAMP(*block_y1, 0, h);
	CLAMP(*block_y2, 0, h);

// printf("MotionMain2::clamp_scan 2 w=%d h=%d block=%d %d %d %d scan=%d %d %d %d absolute=%d\n",
// w,
// h,
// *block_x1,
// *block_y1,
// *block_x2,
// *block_y2,
// *scan_x1,
// *scan_y1,
// *scan_x2,
// *scan_y2,
// use_absolute);
}


#endif // 0

void MotionMain2::get_current_vector(float *origin_x,
	float *origin_y,
	float *current_x1,
	float *current_y1,
	float *current_x2,
	float *current_y2,
	int point)
{
	int w = get_output()->get_w();
	int h = get_output()->get_h();
	float temp1;
	float temp2;
	float temp3;
	float temp4;
	float temp5;
	float temp6;
	if(!origin_x) origin_x = &temp1;
	if(!origin_y) origin_y = &temp2;
	if(!current_x1) current_x1 = &temp3;
	if(!current_y1) current_y1 = &temp4;
	if(!current_x2) current_x2 = &temp5;
	if(!current_y2) current_y2 = &temp6;

	*origin_x = 0;
	*origin_y = 0;
	*current_x1 = 0.0;
	*current_y1 = 0.0;
	*current_x2 = 0.0;
	*current_y2 = 0.0;


	if(config.global[point])
	{
// Get vector
// Start of vector is center of previous block.
// End of vector is total accumulation.
		if(config.tracking_object == MotionScan::TRACK_SINGLE)
		{
			(*origin_x) = (*current_x1) = ((float)config.block_x[point] *
				w /
				100);
			(*origin_y) = (*current_y1) = ((float)config.block_y[point] *
				h /
				100);
			(*current_x2) = (*current_x1) + (float)total_dx[point] / OVERSAMPLE;
			(*current_y2) = (*current_y1) + (float)total_dy[point] / OVERSAMPLE;
		}
		else
// Start of vector is center of previous block.
// End of vector is current change.
		if(config.tracking_object == MotionScan::PREVIOUS_SAME_BLOCK)
		{
			(*origin_x) = (*current_x1) = ((float)config.block_x[point] *
				w /
				100);
			(*origin_y) = (*current_y1) = ((float)config.block_y[point] *
				h /
				100);
			(*current_x2) = (*origin_x) + (float)current_dx[point] / OVERSAMPLE;
			(*current_y2) = (*origin_y) + (float)current_dy[point] / OVERSAMPLE;
		}
		else
		{
			(*origin_x) = (float)config.block_x[point] *
				w /
				100;
			(*origin_y) = (float)config.block_y[point] *
				h /
				100;
			(*current_x1) = ((*origin_x) +
				(float)(total_dx[point] - current_dx[point]) /
				OVERSAMPLE);
			(*current_y1) = ((*origin_y) +
				(float)(total_dy[point] - current_dy[point]) /
				OVERSAMPLE);
			(*current_x2) = ((*origin_x) +
				(float)total_dx[point] /
				OVERSAMPLE);
			(*current_y2) = ((*origin_y) +
				(float)total_dy[point] /
				OVERSAMPLE);
		}
	}
}


void MotionMain2::draw_vectors(VFrame *frame, int point)
{
	int w = frame->get_w();
	int h = frame->get_h();
	float global_x1, global_y1;
	float global_x2, global_y2;
	int block_x, block_y;
	int block_w, block_h;
	int block_x1, block_y1;
	int block_x2, block_y2;
	int search_w, search_h;
	int search_x1, search_y1;
	int search_x2, search_y2;
	int origin_offset_x;
	int origin_offset_y;

	if(!config.draw_vectors[point]) return;


	if(config.global[point])
	{
// Get vector
		get_current_vector(0,
			0,
			&global_x1,
			&global_y1,
			&global_x2,
			&global_y2,
			point);


// Draw destination rectangle
		if(config.action == MotionScan::NOTHING ||
			config.action == MotionScan::TRACK)
		{
			block_x = (int)global_x2;
			block_y = (int)global_y2;
		}
		else
// Draw source rectangle
		{
			block_x = (int)global_x1;
			block_y = (int)global_y1;
		}
		block_w = config.global_block_w[point] * w / 100;
		block_h = config.global_block_h[point] * h / 100;
		origin_offset_x = config.global_origin_x[point] * w / 100;
		origin_offset_y = config.global_origin_y[point] * h / 100;
		block_x1 = block_x - block_w / 2;
		block_y1 = block_y - block_h / 2;
		block_x2 = block_x + block_w / 2;
		block_y2 = block_y + block_h / 2;
		search_w = config.global_range_w[point] * w / 100;
		search_h = config.global_range_h[point] * h / 100;
		search_x1 = block_x1 + origin_offset_x - search_w / 2;
		search_y1 = block_y1 + origin_offset_y - search_h / 2;
		search_x2 = block_x2 + origin_offset_x + search_w / 2;
		search_y2 = block_y2 + origin_offset_y + search_h / 2;

// printf("MotionMain2::draw_vectors %d %d %d %d %d %d %d %d %d %d %d %d\n",
// global_x1,
// global_y1,
// block_w,
// block_h,
// block_x1,
// block_y1,
// block_x2,
// block_y2,
// search_x1,
// search_y1,
// search_x2,
// search_y2);

		MotionScan::clamp_scan(w,
			h,
			&block_x1,
			&block_y1,
			&block_x2,
			&block_y2,
			&search_x1,
			&search_y1,
			&search_x2,
			&search_y2,
			1);

// Vector
		draw_arrow(frame, (int)global_x1, (int)global_y1, (int)global_x2, (int)global_y2);

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
	}
}


Motion2VVFrame::Motion2VVFrame(VFrame *vfrm, int n)
 : VFrame(vfrm->get_data(), -1, vfrm->get_y()-vfrm->get_data(),
	vfrm->get_u()-vfrm->get_data(), vfrm->get_v()-vfrm->get_data(),
	vfrm->get_w(), vfrm->get_h(), vfrm->get_color_model(),
	vfrm->get_bytes_per_line())
{
	this->n = n;
}

int Motion2VVFrame::draw_pixel(int x, int y)
{
	VFrame::draw_pixel(x+0, y+0);
	for( int i=1; i<n; ++i ) {
		VFrame::draw_pixel(x-i, y-i);
		VFrame::draw_pixel(x+i, y+i);
	}
	return 0;
}


void MotionMain2::draw_line(VFrame *frame, int x1, int y1, int x2, int y2)
{
	int iw = frame->get_w(), ih = frame->get_h();
	int mx = iw > ih ? iw : ih;
	int n = mx/800 + 1;
	Motion2VVFrame vfrm(frame, n);
	vfrm.set_pixel_color(WHITE);
	int m = 2;  while( m < n ) m <<= 1;
	vfrm.set_stiple(2*m);
	vfrm.draw_line(x1,y1, x2,y2);
}


#define ARROW_SIZE 10
void MotionMain2::draw_arrow(VFrame *frame, int x1, int y1, int x2, int y2)
{
	double angle = atan((float)(y2 - y1) / (float)(x2 - x1));
	double angle1 = angle + (float)145 / 360 * 2 * 3.14159265;
	double angle2 = angle - (float)145 / 360 * 2 * 3.14159265;
	int x3;
	int y3;
	int x4;
	int y4;
	if(x2 < x1)
	{
		x3 = x2 - (int)(ARROW_SIZE * cos(angle1));
		y3 = y2 - (int)(ARROW_SIZE * sin(angle1));
		x4 = x2 - (int)(ARROW_SIZE * cos(angle2));
		y4 = y2 - (int)(ARROW_SIZE * sin(angle2));
	}
	else
	{
		x3 = x2 + (int)(ARROW_SIZE * cos(angle1));
		y3 = y2 + (int)(ARROW_SIZE * sin(angle1));
		x4 = x2 + (int)(ARROW_SIZE * cos(angle2));
		y4 = y2 + (int)(ARROW_SIZE * sin(angle2));
	}

// Main vector
	draw_line(frame, x1, y1, x2, y2);
//	draw_line(frame, x1, y1 + 1, x2, y2 + 1);

// Arrow line
	if(abs(y2 - y1) || abs(x2 - x1)) draw_line(frame, x2, y2, x3, y3);
//	draw_line(frame, x2, y2 + 1, x3, y3 + 1);
// Arrow line
	if(abs(y2 - y1) || abs(x2 - x1)) draw_line(frame, x2, y2, x4, y4);
//	draw_line(frame, x2, y2 + 1, x4, y4 + 1);
}










#if 0



#define ABS_DIFF(type, temp_type, multiplier, components) \
{ \
	temp_type result_temp = 0; \
	for(int i = 0; i < h; i++) \
	{ \
		type *prev_row = (type*)prev_ptr; \
		type *current_row = (type*)current_ptr; \
		for(int j = 0; j < w; j++) \
		{ \
			for(int k = 0; k < 3; k++) \
			{ \
				temp_type difference; \
				difference = *prev_row++ - *current_row++; \
				if(difference < 0) \
					result_temp -= difference; \
				else \
					result_temp += difference; \
			} \
			if(components == 4) \
			{ \
				prev_row++; \
				current_row++; \
			} \
		} \
		prev_ptr += row_bytes; \
		current_ptr += row_bytes; \
	} \
	result = (int64_t)(result_temp * multiplier); \
}

int64_t MotionMain2::abs_diff(unsigned char *prev_ptr,
	unsigned char *current_ptr,
	int row_bytes,
	int w,
	int h,
	int color_model)
{
	int64_t result = 0;
	switch(color_model)
	{
		case BC_RGB888:
			ABS_DIFF(unsigned char, int64_t, 1, 3)
			break;
		case BC_RGBA8888:
			ABS_DIFF(unsigned char, int64_t, 1, 4)
			break;
		case BC_RGB_FLOAT:
			ABS_DIFF(float, double, 0x10000, 3)
			break;
		case BC_RGBA_FLOAT:
			ABS_DIFF(float, double, 0x10000, 4)
			break;
		case BC_YUV888:
			ABS_DIFF(unsigned char, int64_t, 1, 3)
			break;
		case BC_YUVA8888:
			ABS_DIFF(unsigned char, int64_t, 1, 4)
			break;
		case BC_YUV161616:
			ABS_DIFF(uint16_t, int64_t, 1, 3)
			break;
		case BC_YUVA16161616:
			ABS_DIFF(uint16_t, int64_t, 1, 4)
			break;
	}
	return result;
}



#define ABS_DIFF_SUB(type, temp_type, multiplier, components) \
{ \
	temp_type result_temp = 0; \
	temp_type y2_fraction = sub_y * 0x100 / OVERSAMPLE; \
	temp_type y1_fraction = 0x100 - y2_fraction; \
	temp_type x2_fraction = sub_x * 0x100 / OVERSAMPLE; \
	temp_type x1_fraction = 0x100 - x2_fraction; \
	for(int i = 0; i < h_sub; i++) \
	{ \
		type *prev_row1 = (type*)prev_ptr; \
		type *prev_row2 = (type*)prev_ptr + components; \
		type *prev_row3 = (type*)(prev_ptr + row_bytes); \
		type *prev_row4 = (type*)(prev_ptr + row_bytes) + components; \
		type *current_row = (type*)current_ptr; \
		for(int j = 0; j < w_sub; j++) \
		{ \
			for(int k = 0; k < 3; k++) \
			{ \
				temp_type difference; \
				temp_type prev_value = \
					(*prev_row1++ * x1_fraction * y1_fraction + \
					*prev_row2++ * x2_fraction * y1_fraction + \
					*prev_row3++ * x1_fraction * y2_fraction + \
					*prev_row4++ * x2_fraction * y2_fraction) / \
					0x100 / 0x100; \
				temp_type current_value = *current_row++; \
				difference = prev_value - current_value; \
				if(difference < 0) \
					result_temp -= difference; \
				else \
					result_temp += difference; \
			} \
 \
			if(components == 4) \
			{ \
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
}




int64_t MotionMain2::abs_diff_sub(unsigned char *prev_ptr,
	unsigned char *current_ptr,
	int row_bytes,
	int w,
	int h,
	int color_model,
	int sub_x,
	int sub_y)
{
	int h_sub = h - 1;
	int w_sub = w - 1;
	int64_t result = 0;

	switch(color_model)
	{
		case BC_RGB888:
			ABS_DIFF_SUB(unsigned char, int64_t, 1, 3)
			break;
		case BC_RGBA8888:
			ABS_DIFF_SUB(unsigned char, int64_t, 1, 4)
			break;
		case BC_RGB_FLOAT:
			ABS_DIFF_SUB(float, double, 0x10000, 3)
			break;
		case BC_RGBA_FLOAT:
			ABS_DIFF_SUB(float, double, 0x10000, 4)
			break;
		case BC_YUV888:
			ABS_DIFF_SUB(unsigned char, int64_t, 1, 3)
			break;
		case BC_YUVA8888:
			ABS_DIFF_SUB(unsigned char, int64_t, 1, 4)
			break;
		case BC_YUV161616:
			ABS_DIFF_SUB(uint16_t, int64_t, 1, 3)
			break;
		case BC_YUVA16161616:
			ABS_DIFF_SUB(uint16_t, int64_t, 1, 4)
			break;
	}
	return result;
}





MotionScanPackage::MotionScanPackage()
 : LoadPackage()
{
	valid = 1;
}






MotionScanUnit::MotionScanUnit(MotionScan *server,
	MotionMain2 *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
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
	int w = server->current_frame->get_w();
	int h = server->current_frame->get_h();
	int color_model = server->current_frame->get_color_model();
	int pixel_size = cmodel_calculate_pixelsize(color_model);
	int row_bytes = server->current_frame->get_bytes_per_line();













// Single pixel
	if(!server->subpixel)
	{
		int search_x = pkg->scan_x1 + (pkg->pixel % (pkg->scan_x2 - pkg->scan_x1));
		int search_y = pkg->scan_y1 + (pkg->pixel / (pkg->scan_x2 - pkg->scan_x1));

// Try cache
		pkg->difference1 = server->get_cache(search_x, search_y);
		if(pkg->difference1 < 0)
		{
//printf("MotionScanUnit::process_package 1 %d %d\n",
//search_x, search_y, pkg->block_x2 - pkg->block_x1, pkg->block_y2 - pkg->block_y1);
// Pointers to first pixel in each block
			unsigned char *prev_ptr = server->previous_frame->get_rows()[
				search_y] +
				search_x * pixel_size;
			unsigned char *current_ptr = server->current_frame->get_rows()[
				pkg->block_y1] +
				pkg->block_x1 * pixel_size;
// Scan block
			pkg->difference1 = plugin->abs_diff(prev_ptr,
				current_ptr,
				row_bytes,
				pkg->block_x2 - pkg->block_x1,
				pkg->block_y2 - pkg->block_y1,
				color_model);
//printf("MotionScanUnit::process_package 2\n");
			server->put_cache(search_x, search_y, pkg->difference1);
		}
	}







	else








// Sub pixel
	{
		int sub_x = pkg->pixel % (OVERSAMPLE * 2 - 1) + 1;
		int sub_y = pkg->pixel / (OVERSAMPLE * 2 - 1) + 1;

		if(plugin->config.horizontal_only)
		{
			sub_y = 0;
		}

		if(plugin->config.vertical_only)
		{
			sub_x = 0;
		}

		int search_x = pkg->scan_x1 + sub_x / OVERSAMPLE;
		int search_y = pkg->scan_y1 + sub_y / OVERSAMPLE;
		sub_x %= OVERSAMPLE;
		sub_y %= OVERSAMPLE;


		unsigned char *prev_ptr = server->previous_frame->get_rows()[
			search_y] +
			search_x * pixel_size;
		unsigned char *current_ptr = server->current_frame->get_rows()[
			pkg->block_y1] +
			pkg->block_x1 * pixel_size;

// With subpixel, there are two ways to compare each position, one by shifting
// the previous frame and two by shifting the current frame.
		pkg->difference1 = plugin->abs_diff_sub(prev_ptr,
			current_ptr,
			row_bytes,
			pkg->block_x2 - pkg->block_x1,
			pkg->block_y2 - pkg->block_y1,
			color_model,
			sub_x,
			sub_y);
		pkg->difference2 = plugin->abs_diff_sub(current_ptr,
			prev_ptr,
			row_bytes,
			pkg->block_x2 - pkg->block_x1,
			pkg->block_y2 - pkg->block_y1,
			color_model,
			sub_x,
			sub_y);
// printf("MotionScanUnit::process_package sub_x=%d sub_y=%d search_x=%d search_y=%d diff1=%lld diff2=%lld\n",
// sub_x,
// sub_y,
// search_x,
// search_y,
// pkg->difference1,
// pkg->difference2);
	}





}










int64_t MotionScanUnit::get_cache(int x, int y)
{
	int64_t result = -1;
	cache_lock->lock("MotionScanUnit::get_cache");
	for(int i = 0; i < cache.total; i++)
	{
		MotionScanCache *ptr = cache.values[i];
		if(ptr->x == x && ptr->y == y)
		{
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











MotionScan::MotionScan(MotionMain2 *plugin,
	int total_clients,
	int total_packages)
 : LoadServer(
//1, 1
total_clients, total_packages
)
{
	this->plugin = plugin;
	cache_lock = new Mutex("MotionScan::cache_lock");
}

MotionScan::~MotionScan()
{
	delete cache_lock;
}


void MotionScan::init_packages()
{
// Set package coords
	for(int i = 0; i < get_total_packages(); i++)
	{
		MotionScanPackage *pkg = (MotionScanPackage*)get_package(i);

		pkg->block_x1 = block_x1;
		pkg->block_x2 = block_x2;
		pkg->block_y1 = block_y1;
		pkg->block_y2 = block_y2;
		pkg->scan_x1 = scan_x1;
		pkg->scan_x2 = scan_x2;
		pkg->scan_y1 = scan_y1;
		pkg->scan_y2 = scan_y2;
		pkg->pixel = (int64_t)i * (int64_t)total_pixels / (int64_t)total_steps;
		pkg->difference1 = 0;
		pkg->difference2 = 0;
		pkg->dx = 0;
		pkg->dy = 0;
		pkg->valid = 1;
	}
}

LoadClient* MotionScan::new_client()
{
	return new MotionScanUnit(this, plugin);
}

LoadPackage* MotionScan::new_package()
{
	return new MotionScanPackage;
}


void MotionScan::scan_frame(VFrame *previous_frame,
	VFrame *current_frame,
	int point)
{
	this->previous_frame = previous_frame;
	this->current_frame = current_frame;
	this->point = point;
	subpixel = 0;

	cache.remove_all_objects();


// Single macroblock
	int w = current_frame->get_w();
	int h = current_frame->get_h();

// Initial search parameters
	int scan_w = w * plugin->config.global_range_w[point] / 100;
	int scan_h = h * plugin->config.global_range_h[point] / 100;
	int block_w = w * plugin->config.global_block_w[point] / 100;
	int block_h = h * plugin->config.global_block_h[point] / 100;

// Location of block in previous frame
	block_x1 = (int)(w * plugin->config.block_x[point] / 100 - block_w / 2);
	block_y1 = (int)(h * plugin->config.block_y[point] / 100 - block_h / 2);
	block_x2 = (int)(w * plugin->config.block_x[point] / 100 + block_w / 2);
	block_y2 = (int)(h * plugin->config.block_y[point] / 100 + block_h / 2);

// Offset to location of previous block.  This offset needn't be very accurate
// since it's the offset of the previous image and current image we want.
	if(plugin->config.tracking_object == MotionScan::TRACK_PREVIOUS)
	{
		block_x1 += plugin->total_dx[point] / OVERSAMPLE;
		block_y1 += plugin->total_dy[point] / OVERSAMPLE;
		block_x2 += plugin->total_dx[point] / OVERSAMPLE;
		block_y2 += plugin->total_dy[point] / OVERSAMPLE;
	}

	skip = 0;

	switch(plugin->config.calculation)
	{
// Don't calculate
		case MotionScan::NO_CALCULATE:
			dx_result = 0;
			dy_result = 0;
			skip = 1;
			break;

		case MotionScan::LOAD:
		{
printf("MotionScan::scan_frame %d\n", __LINE__);
// Load result from disk
			char string[BCTEXTLEN];
			sprintf(string, "%s%06d",
				MOTION_FILE,
				plugin->get_source_position());
			FILE *input;
			input = fopen(string, "r");
			if(input)
			{
				for(int i = 0; i <= point; i++)
				{
					fscanf(input,
						"%d %d",
						&dx_result,
						&dy_result);
				}
				fclose(input);
				skip = 1;
			}
			break;
		}

// Scan from scratch
		default:
			skip = 0;
			break;
	}

// Perform scan
	if(!skip)
	{
printf("MotionScan::scan_frame %d\n", __LINE__);
// Calculate center of search area in current frame
		int origin_offset_x = plugin->config.global_origin_x[point] * w / 100;
		int origin_offset_y = plugin->config.global_origin_y[point] * h / 100;
		int x_result = block_x1 + origin_offset_x;
		int y_result = block_y1 + origin_offset_y;

// printf("MotionScan::scan_frame 1 %d %d %d %d %d %d %d %d\n",
// block_x1 + block_w / 2,
// block_y1 + block_h / 2,
// block_w,
// block_h,
// block_x1,
// block_y1,
// block_x2,
// block_y2);

		while(1)
		{
			scan_x1 = x_result - scan_w / 2;
			scan_y1 = y_result - scan_h / 2;
			scan_x2 = x_result + scan_w / 2;
			scan_y2 = y_result + scan_h / 2;



// Zero out requested values
			if(plugin->config.horizontal_only)
			{
				scan_y1 = block_y1;
				scan_y2 = block_y1 + 1;
			}
			if(plugin->config.vertical_only)
			{
				scan_x1 = block_x1;
				scan_x2 = block_x1 + 1;
			}

// printf("MotionScan::scan_frame 1 %d %d %d %d %d %d %d %d\n",
// block_x1,
// block_y1,
// block_x2,
// block_y2,
// scan_x1,
// scan_y1,
// scan_x2,
// scan_y2);
// Clamp the block coords before the scan so we get useful scan coords.
			MotionScan::clamp_scan(w,
				h,
				&block_x1,
				&block_y1,
				&block_x2,
				&block_y2,
				&scan_x1,
				&scan_y1,
				&scan_x2,
				&scan_y2,
				0);
// printf("MotionScan::scan_frame 1\n    block_x1=%d block_y1=%d block_x2=%d block_y2=%d\n    scan_x1=%d scan_y1=%d scan_x2=%d scan_y2=%d\n    x_result=%d y_result=%d\n",
// block_x1,
// block_y1,
// block_x2,
// block_y2,
// scan_x1,
// scan_y1,
// scan_x2,
// scan_y2,
// x_result,
// y_result);


// Give up if invalid coords.
			if(scan_y2 <= scan_y1 ||
				scan_x2 <= scan_x1 ||
				block_x2 <= block_x1 ||
				block_y2 <= block_y1)
				break;


// For subpixel, the top row and left column are skipped
			if(subpixel)
			{

				if(plugin->config.horizontal_only ||
					plugin->config.vertical_only)
				{
					total_pixels = 4 * OVERSAMPLE * OVERSAMPLE - 4 * OVERSAMPLE;
				}
				else
				{
					total_pixels = 4 * OVERSAMPLE;
				}

				total_steps = total_pixels;

				set_package_count(total_steps);
				process_packages();

// Get least difference
				int64_t min_difference = -1;
				for(int i = 0; i < get_total_packages(); i++)
				{
					MotionScanPackage *pkg = (MotionScanPackage*)get_package(i);
					if(pkg->difference1 < min_difference || min_difference == -1)
					{
						min_difference = pkg->difference1;

						if(plugin->config.vertical_only)
							x_result = scan_x1 * OVERSAMPLE;
						else
							x_result = scan_x1 * OVERSAMPLE +
								(pkg->pixel % (OVERSAMPLE * 2 - 1)) + 1;

						if(plugin->config.horizontal_only)
							y_result = scan_y1 * OVERSAMPLE;
						else
							y_result = scan_y1 * OVERSAMPLE +
								(pkg->pixel / (OVERSAMPLE * 2 - 1)) + 1;


// Fill in results
						dx_result = block_x1 * OVERSAMPLE - x_result;
						dy_result = block_y1 * OVERSAMPLE - y_result;
					}

					if(pkg->difference2 < min_difference)
					{
						min_difference = pkg->difference2;

						if(plugin->config.vertical_only)
							x_result = scan_x1 * OVERSAMPLE;
						else
							x_result = scan_x2 * OVERSAMPLE -
								((pkg->pixel % (OVERSAMPLE * 2 - 1)) + 1);

						if(plugin->config.horizontal_only)
							y_result = scan_y1 * OVERSAMPLE;
						else
							y_result = scan_y2 * OVERSAMPLE -
								((pkg->pixel / (OVERSAMPLE * 2 - 1)) + 1);

						dx_result = block_x1 * OVERSAMPLE - x_result;
						dy_result = block_y1 * OVERSAMPLE - y_result;
					}
				}


//printf("MotionScan::scan_frame 1 %d %d %d %d\n", block_x1, block_y1, x_result, y_result);
				break;
			}
			else
			{

				total_pixels = (scan_x2 - scan_x1) * (scan_y2 - scan_y1);
				total_steps = MIN(plugin->config.global_positions, total_pixels);

				set_package_count(total_steps);
				process_packages();

// Get least difference
				int64_t min_difference = -1;
				for(int i = 0; i < get_total_packages(); i++)
				{
					MotionScanPackage *pkg = (MotionScanPackage*)get_package(i);
					if(pkg->difference1 < min_difference || min_difference == -1)
					{
						min_difference = pkg->difference1;
						x_result = scan_x1 + (pkg->pixel % (scan_x2 - scan_x1));
						y_result = scan_y1 + (pkg->pixel / (scan_x2 - scan_x1));
						x_result *= OVERSAMPLE;
						y_result *= OVERSAMPLE;
					}
				}



// printf("MotionScan::scan_frame 10 total_steps=%d total_pixels=%d subpixel=%d\n",
// total_steps,
// total_pixels,
// subpixel);
//
// printf("	scan w=%d h=%d scan x1=%d y1=%d x2=%d y2=%d\n",
// scan_w,
// scan_h,
// scan_x1,
// scan_y1,
// scan_x2,
// scan_y2);
//
// printf("MotionScan::scan_frame 2 block x1=%d y1=%d x2=%d y2=%d result x=%.2f y=%.2f\n",
// block_x1,
// block_y1,
// block_x2,
// block_y2,
// (float)x_result / 4,
// (float)y_result / 4);


// If a new search is required, rescale results back to pixels.
				if(total_steps >= total_pixels)
				{
// Single pixel accuracy reached.  Now do exhaustive subpixel search.
					if(plugin->config.action == MotionScan::STABILIZE ||
						plugin->config.action == MotionScan::TRACK ||
						plugin->config.action == MotionScan::NOTHING)
					{
						x_result /= OVERSAMPLE;
						y_result /= OVERSAMPLE;
						scan_w = 2;
						scan_h = 2;
						subpixel = 1;
					}
					else
					{
// Fill in results and quit
						dx_result = block_x1 * OVERSAMPLE - x_result;
						dy_result = block_y1 * OVERSAMPLE - y_result;
						break;
					}
				}
				else
// Reduce scan area and try again
				{
					scan_w = (scan_x2 - scan_x1) / 2;
					scan_h = (scan_y2 - scan_y1) / 2;
					x_result /= OVERSAMPLE;
					y_result /= OVERSAMPLE;
				}
			}
		}

		dx_result *= -1;
		dy_result *= -1;
	}






// Write results
	if(plugin->config.calculation == MotionScan::SAVE)
	{
		char string[BCTEXTLEN];
		sprintf(string,
			"%s%06d",
			MOTION_FILE,
			plugin->get_source_position());
		FILE *output;
		if(point == 0)
			output = fopen(string, "w");
		else
			output = fopen(string, "a");
		if(output)
		{
			fprintf(output,
				"%d %d\n",
				dx_result,
				dy_result);
			fclose(output);
		}
		else
		{
			perror("MotionScan::scan_frame SAVE 1");
		}
	}

#ifdef DEBUG
printf("MotionScan::scan_frame 10 point=%d dx=%.2f dy=%.2f\n",
point,
(float)this->dx_result / OVERSAMPLE,
(float)this->dy_result / OVERSAMPLE);
#endif
}

















int64_t MotionScan::get_cache(int x, int y)
{
	int64_t result = -1;
	cache_lock->lock("MotionScan::get_cache");
	for(int i = 0; i < cache.total; i++)
	{
		MotionScanCache *ptr = cache.values[i];
		if(ptr->x == x && ptr->y == y)
		{
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





MotionScanCache::MotionScanCache(int x, int y, int64_t difference)
{
	this->x = x;
	this->y = y;
	this->difference = difference;
}










#endif



