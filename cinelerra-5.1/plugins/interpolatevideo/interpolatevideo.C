/*
 * CINELERRA
 * Copyright (C) 1997-2016 Adam Williams <broadcast at earthling dot net>
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

#include "bcsignals.h"
#include "clip.h"
#include "filexml.h"
#include "interpolatevideo.h"
#include "interpolatewindow.h"
#include "language.h"
#include "motionscan-hv.h"
#include "opticflow.h"
#include "transportque.inc"
#include <unistd.h>









InterpolateVideoConfig::InterpolateVideoConfig()
{
	input_rate = (double)30000 / 1001;
	use_keyframes = 0;
	optic_flow = 1;
	draw_vectors = 1;
	search_radius = 16;
	macroblock_size = 16;
}

void InterpolateVideoConfig::copy_from(InterpolateVideoConfig *config)
{
	this->input_rate = config->input_rate;
	this->use_keyframes = config->use_keyframes;
	this->optic_flow = config->optic_flow;
	this->draw_vectors = config->draw_vectors;
	this->search_radius = config->search_radius;
	this->macroblock_size = config->macroblock_size;
}

int InterpolateVideoConfig::equivalent(InterpolateVideoConfig *config)
{
	return EQUIV(this->input_rate, config->input_rate) &&
		(this->use_keyframes == config->use_keyframes) &&
		this->optic_flow == config->optic_flow &&
		this->draw_vectors == config->draw_vectors &&
		this->search_radius == config->search_radius &&
		this->macroblock_size == config->macroblock_size;
}








REGISTER_PLUGIN(InterpolateVideo)







InterpolateVideo::InterpolateVideo(PluginServer *server)
 : PluginVClient(server)
{
	optic_flow_engine = 0;
	warp_engine = 0;
	blend_engine = 0;
	bzero(frames, sizeof(VFrame*) * 2);
	for(int i = 0; i < 2; i++)
		frame_number[i] = -1;
	last_position = -1;
	last_rate = -1;
	last_macroblock_size = 0;
	last_search_radius = 0;
	total_macroblocks = 0;
}


InterpolateVideo::~InterpolateVideo()
{
	delete optic_flow_engine;
	delete warp_engine;
	delete blend_engine;
	if(frames[0]) delete frames[0];
	if(frames[1]) delete frames[1];
	macroblocks.remove_all_objects();
}


void InterpolateVideo::fill_border(double frame_rate, int64_t start_position)
{
// A border frame changed or the start position is not identical to the last
// start position.

	int64_t frame_start = range_start + (get_direction() == PLAY_REVERSE ? 1 : 0);
	int64_t frame_end = range_end + (get_direction() == PLAY_REVERSE ? 1 : 0);

	if( last_position == start_position && EQUIV(last_rate, frame_rate) &&
		frame_number[0] >= 0 && frame_number[0] == frame_start &&
		frame_number[1] >= 0 && frame_number[1] == frame_end ) return;

	if( frame_start == frame_number[1] || frame_end == frame_number[0] )
	{
		int64_t n = frame_number[0];
		frame_number[0] = frame_number[1];
		frame_number[1] = n;
		VFrame *f = frames[0];
		frames[0] = frames[1];
		frames[1] = f;
	}

	if( frame_start != frame_number[0] )
	{
//printf("InterpolateVideo::fill_border 1 %lld\n", range_start);
		read_frame(frames[0], 0, frame_start, active_input_rate, 0);
		frame_number[0] = frame_start;
	}

	if( frame_end != frame_number[1] )
	{
//printf("InterpolateVideo::fill_border 2 %lld\n", range_start);
		read_frame(frames[1], 0, frame_end, active_input_rate, 0);
		frame_number[1] = frame_end;
	}

	last_position = start_position;
	last_rate = frame_rate;
}





void InterpolateVideo::draw_pixel(VFrame *frame, int x, int y)
{
	if(!(x >= 0 && y >= 0 && x < frame->get_w() && y < frame->get_h())) return;

#define DRAW_PIXEL(x, y, components, do_yuv, max, type) \
{ \
	type **rows = (type**)frame->get_rows(); \
	rows[y][x * components] = max - rows[y][x * components]; \
	if(!do_yuv) \
	{ \
		rows[y][x * components + 1] = max - rows[y][x * components + 1]; \
		rows[y][x * components + 2] = max - rows[y][x * components + 2]; \
	} \
	else \
	{ \
		rows[y][x * components + 1] = (max / 2 + 1) - rows[y][x * components + 1]; \
		rows[y][x * components + 2] = (max / 2 + 1) - rows[y][x * components + 2]; \
	} \
	if(components == 4) \
		rows[y][x * components + 3] = max; \
}


	switch(frame->get_color_model())
	{
		case BC_RGB888:
			DRAW_PIXEL(x, y, 3, 0, 0xff, unsigned char);
			break;
		case BC_RGBA8888:
			DRAW_PIXEL(x, y, 4, 0, 0xff, unsigned char);
			break;
		case BC_RGB_FLOAT:
			DRAW_PIXEL(x, y, 3, 0, 1.0, float);
			break;
		case BC_RGBA_FLOAT:
			DRAW_PIXEL(x, y, 4, 0, 1.0, float);
			break;
		case BC_YUV888:
			DRAW_PIXEL(x, y, 3, 1, 0xff, unsigned char);
			break;
		case BC_YUVA8888:
			DRAW_PIXEL(x, y, 4, 1, 0xff, unsigned char);
			break;
		case BC_RGB161616:
			DRAW_PIXEL(x, y, 3, 0, 0xffff, uint16_t);
			break;
		case BC_YUV161616:
			DRAW_PIXEL(x, y, 3, 1, 0xffff, uint16_t);
			break;
		case BC_RGBA16161616:
			DRAW_PIXEL(x, y, 4, 0, 0xffff, uint16_t);
			break;
		case BC_YUVA16161616:
			DRAW_PIXEL(x, y, 4, 1, 0xffff, uint16_t);
			break;
	}
}


void InterpolateVideo::draw_line(VFrame *frame, int x1, int y1, int x2, int y2)
{
	int w = labs(x2 - x1);
	int h = labs(y2 - y1);
//printf("InterpolateVideo::draw_line 1 %d %d %d %d\n", x1, y1, x2, y2);

	if(!w && !h)
	{
		draw_pixel(frame, x1, y1);
	}
	else
	if(w > h)
	{
// Flip coordinates so x1 < x2
		if(x2 < x1)
		{
			y2 ^= y1;
			y1 ^= y2;
			y2 ^= y1;
			x1 ^= x2;
			x2 ^= x1;
			x1 ^= x2;
		}
		int numerator = y2 - y1;
		int denominator = x2 - x1;
		for(int i = x1; i < x2; i++)
		{
			int y = y1 + (int64_t)(i - x1) * (int64_t)numerator / (int64_t)denominator;
			draw_pixel(frame, i, y);
		}
	}
	else
	{
// Flip coordinates so y1 < y2
		if(y2 < y1)
		{
			y2 ^= y1;
			y1 ^= y2;
			y2 ^= y1;
			x1 ^= x2;
			x2 ^= x1;
			x1 ^= x2;
		}
		int numerator = x2 - x1;
		int denominator = y2 - y1;
		for(int i = y1; i < y2; i++)
		{
			int x = x1 + (int64_t)(i - y1) * (int64_t)numerator / (int64_t)denominator;
			draw_pixel(frame, x, i);
		}
	}
//printf("InterpolateVideo::draw_line 2\n");
}

#define ARROW_SIZE 10
void InterpolateVideo::draw_arrow(VFrame *frame,
	int x1,
	int y1,
	int x2,
	int y2)
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

void InterpolateVideo::draw_rect(VFrame *frame,
	int x1,
	int y1,
	int x2,
	int y2)
{
	draw_line(frame, x1, y1, x2, y1);
	draw_line(frame, x2, y1, x2, y2);
	draw_line(frame, x2, y2, x1, y2);
	draw_line(frame, x1, y2, x1, y1);
}


void InterpolateVideo::create_macroblocks()
{
// Get macroblock size
	x_macroblocks = frames[0]->get_w() / config.macroblock_size;
	y_macroblocks = frames[0]->get_h() / config.macroblock_size;
// printf("InterpolateVideo::create_macroblocks %d %d %d %d\n", 
// __LINE__, 
// config.macroblock_size,
// x_macroblocks,
// y_macroblocks);

	if(config.macroblock_size * x_macroblocks < frames[0]->get_w())
	{
		x_macroblocks++;
	}

	if(config.macroblock_size * y_macroblocks < frames[0]->get_h())
	{
		y_macroblocks++;
	}

	total_macroblocks = x_macroblocks * y_macroblocks;

	if(total_macroblocks != macroblocks.size())
	{
		macroblocks.remove_all_objects();
	}

	for(int i = 0; i < total_macroblocks; i++)
	{
		OpticFlowMacroblock *mb = 0;
		if(macroblocks.size() > i)
		{
			mb = macroblocks.get(i);
		}
		else
		{
			mb = new OpticFlowMacroblock;
			macroblocks.append(mb);
		}

		mb->x = (i % x_macroblocks) * config.macroblock_size + config.macroblock_size / 2;
		mb->y = (i / x_macroblocks) * config.macroblock_size + config.macroblock_size / 2;
	}
}


void InterpolateVideo::draw_vectors(int processed)
{
// Draw arrows
	if(config.draw_vectors)
	{
		create_macroblocks();

		for(int i = 0; i < total_macroblocks; i++)
		{
			OpticFlowMacroblock *mb = macroblocks.get(i);
// 		    printf("InterpolateVideo::optic_flow %d x=%d y=%d dx=%d dy=%d\n",
// 		  	    __LINE__,
// 			    mb->x,
// 			    mb->y,
// 			    mb->dx / OVERSAMPLE,
// 			    mb->dy / OVERSAMPLE);

			if(processed)
			{
				draw_arrow(get_output(),
					mb->x,
					mb->y,
					mb->x - mb->dx / OVERSAMPLE,
					mb->y - mb->dy / OVERSAMPLE);

// debug
// 				if(mb->is_valid && mb->visible)
// 				{
// 					draw_arrow(get_output(),
// 						mb->x + 1,
// 						mb->y + 1,
// 						mb->x - mb->dx / OVERSAMPLE + 1,
// 						mb->y - mb->dy / OVERSAMPLE + 1);
// 				}
			}
			else
			{
				draw_pixel(get_output(),
					mb->x,
					mb->y);
			}
		}

// Draw center macroblock
		OpticFlowMacroblock *mb = macroblocks.get(
			x_macroblocks / 2 + y_macroblocks / 2 * x_macroblocks);
		draw_rect(get_output(),
			mb->x - config.macroblock_size / 2,
			mb->y - config.macroblock_size / 2,
			mb->x + config.macroblock_size / 2,
			mb->y + config.macroblock_size / 2);
		draw_rect(get_output(),
			mb->x - config.macroblock_size / 2 - config.search_radius,
			mb->y - config.macroblock_size / 2 - config.search_radius,
			mb->x + config.macroblock_size / 2 + config.search_radius,
			mb->y + config.macroblock_size / 2 + config.search_radius);
	}
}


int InterpolateVideo::angles_overlap(float dst2_angle1,
	float dst2_angle2,
	float dst1_angle1,
	float dst1_angle2)
{
	if(dst2_angle1 < 0 || dst2_angle2 < 0)
	{
		dst2_angle1 += 2 * M_PI;
		dst2_angle2 += 2 * M_PI;
	}

	if(dst1_angle1 < 0 || dst1_angle2 < 0)
	{
		dst1_angle1 += 2 * M_PI;
		dst1_angle2 += 2 * M_PI;
	}

	if(dst1_angle1 < dst2_angle2 &&
		dst1_angle2 > dst2_angle1) return 1;

	return 0;
}



void InterpolateVideo::blend_macroblock(int number)
{
	OpticFlowMacroblock *src = macroblocks.get(number);
	struct timeval start_time;
	gettimeofday(&start_time, 0);
// 	printf("InterpolateVideo::blend_macroblock %d %d\n",
// 	__LINE__,
// 	src->is_valid);


// Copy macroblock table to local thread
	ArrayList<OpticFlowMacroblock*> local_macroblocks;
	for(int i = 0; i < macroblocks.size(); i++)
	{
		OpticFlowMacroblock *mb = new OpticFlowMacroblock;
		mb->copy_from(macroblocks.get(i));
		local_macroblocks.append(mb);
	}

// Get nearest macroblocks
	for(int i = 0; i < local_macroblocks.size(); i++)
	{
		OpticFlowMacroblock *dst = local_macroblocks.get(i);
		if(i != number && dst->is_valid)
		{

// rough estimation of angle coverage
			float angle = atan2(dst->y - src->y, dst->x - src->x);
			float dist = sqrt(SQR(dst->y - src->y) + SQR(dst->x - src->x));
			float span = sin((float)config.macroblock_size / dist);
			dst->angle1 = angle - span / 2;
			dst->angle2 = angle + span / 2;
			dst->dist = dist;
// All macroblocks start as visible
			dst->visible = 1;

// printf("InterpolateVideo::blend_macroblock %d %d x=%d y=%d span=%f angle1=%f angle2=%f dist=%f\n",
// __LINE__,
// i,
// dst->x,
// dst->y,
// span * 360 / 2 / M_PI,
// dst->angle1 * 360 / 2 / M_PI,
// dst->angle2 * 360 / 2 / M_PI,
// dst->dist);
		}
	}

	for(int i = 0; i < local_macroblocks.size(); i++)
	{
// Conceil macroblocks which are hidden
		OpticFlowMacroblock *dst1 = local_macroblocks.get(i);
		if(i != number && dst1->is_valid && dst1->visible)
		{
// Find macroblock which is obstructing
			for(int j = 0; j < local_macroblocks.size(); j++)
			{
				OpticFlowMacroblock *dst2 = local_macroblocks.get(j);
				if(j != number &&
					dst2->is_valid &&
					dst2->dist < dst1->dist &&
					angles_overlap(dst2->angle1,
						dst2->angle2,
						dst1->angle1,
						dst1->angle2))
				{
					dst1->visible = 0;
					j = local_macroblocks.size();
				}
			}
		}
	}

// Blend all visible macroblocks
// Get distance metrics
	float total = 0;
	float min = 0;
	float max = 0;
	int first = 1;
	for(int i = 0; i < local_macroblocks.size(); i++)
	{
		OpticFlowMacroblock *dst = local_macroblocks.get(i);
		if(i != number && dst->is_valid && dst->visible)
		{
			total += dst->dist;
			if(first)
			{
				min = max = dst->dist;
				first = 0;
			}
			else
			{
				min = MIN(dst->dist, min);
				max = MAX(dst->dist, max);
			}
// printf("InterpolateVideo::blend_macroblock %d %d x=%d y=%d dist=%f\n",
// __LINE__,
// i,
// dst->x,
// dst->y,
// dst->dist);

		}
	}

// Invert distances to convert to weights
	total = 0;
	for(int i = 0; i < local_macroblocks.size(); i++)
	{
		OpticFlowMacroblock *dst = local_macroblocks.get(i);
		if(i != number && dst->is_valid && dst->visible)
		{
			dst->dist = max - dst->dist + min;
			total += dst->dist;
// printf("InterpolateVideo::blend_macroblock %d %d x=%d y=%d dist=%f\n",
// __LINE__,
// i,
// dst->x,
// dst->y,
// max - dst->dist + min);

		}
	}

// Add weighted vectors
	float dx = 0;
	float dy = 0;
	if(total > 0)
	{
		for(int i = 0; i < local_macroblocks.size(); i++)
		{
			OpticFlowMacroblock *dst = local_macroblocks.get(i);
			if(i != number && dst->is_valid && dst->visible)
			{
				dx += dst->dist * dst->dx / total;
				dy += dst->dist * dst->dy / total;
				src->dx = dx;
				src->dy = dy;
// printf("InterpolateVideo::blend_macroblock %d %d x=%d y=%d dist=%f\n",
// __LINE__,
// i,
// dst->x,
// dst->y,
// max - dst->dist + min);

			}
		}
	}

	local_macroblocks.remove_all_objects();

// printf("InterpolateVideo::blend_macroblock %d total=%f\n",
// __LINE__,
// total);
	struct timeval end_time;
	gettimeofday(&end_time, 0);
// 	printf("InterpolateVideo::blend_macroblock %d %d\n",
// 		__LINE__,
// 		end_time.tv_sec * 1000 + end_time.tv_usec / 1000 -
// 		start_time.tv_sec * 1000 - start_time.tv_usec / 1000);
}



void InterpolateVideo::optic_flow()
{

	create_macroblocks();
	int need_motion = 0;

// New engine
	if(!optic_flow_engine)
	{
		optic_flow_engine = new OpticFlow(this,
			PluginClient::get_project_smp() + 1,
			PluginClient::get_project_smp() + 1);
		need_motion = 1;
	}
	else
// Reuse old vectors
	if(motion_number[0] == frame_number[0] &&
		motion_number[1] == frame_number[1] &&
		last_macroblock_size == config.macroblock_size &&
		last_search_radius == config.search_radius)
	{
		;
	}
	else
// Calculate new vectors
	{
		need_motion = 1;
	}

	if(need_motion)
	{
		optic_flow_engine->set_package_count(MIN(MAX_PACKAGES, total_macroblocks));
		optic_flow_engine->process_packages();

// Fill in failed macroblocks
		invalid_blocks.remove_all();
		for(int i = 0; i < macroblocks.size(); i++)
		{
			if(!macroblocks.get(i)->is_valid
// debug
//				 && i >= 30 * x_macroblocks)
				)
			{
				invalid_blocks.append(i);
			}
		}

//printf("InterpolateVideo::optic_flow %d %d\n", __LINE__, invalid_blocks.size());
		if(invalid_blocks.size())
		{
			if(!blend_engine)
			{
				blend_engine = new BlendMacroblock(this,
					PluginClient::get_project_smp() + 1,
					PluginClient::get_project_smp() + 1);
			}

			blend_engine->set_package_count(MIN(PluginClient::get_project_smp() + 1,
				invalid_blocks.size()));
			blend_engine->process_packages();
		}
	}



// for(int i = 0; i < total_macroblocks; i++)
// {
// 	OpticFlowPackage *pkg = (OpticFlowPackage*)optic_flow_engine->get_package(
// 		i);
// 	if((i / x_macroblocks) % 2)
// 	{
// 		pkg->dx = 0;
// 		pkg->dy = 0;
// 	}
// 	else
// 	{
// 		pkg->dx = -32;
// 		pkg->dy = 0;
// 	}
// }


	if(!warp_engine)
	{
		warp_engine = new Warp(this,
			PluginClient::get_project_smp() + 1,
			PluginClient::get_project_smp() + 1);
	}

	warp_engine->process_packages();

	motion_number[0] = frame_number[0];
	motion_number[1] = frame_number[1];
	last_macroblock_size = config.macroblock_size;
	last_search_radius = config.search_radius;


// Debug
//	get_output()->copy_from(frames[1]);


	draw_vectors(1);
}


#define AVERAGE(type, temp_type,components, max) \
{ \
	temp_type fraction0 = (temp_type)(lowest_fraction * max); \
	temp_type fraction1 = (temp_type)(max - fraction0); \
 \
	for(int i = 0; i < h; i++) \
	{ \
		type *prev_row0 = (type*)frames[0]->get_rows()[i]; \
		type *next_row0 = (type*)frames[1]->get_rows()[i]; \
		type *out_row = (type*)frame->get_rows()[i]; \
		for(int j = 0; j < w * components; j++) \
		{ \
			*out_row++ = (*prev_row0++ * fraction0 + *next_row0++ * fraction1) / max; \
		} \
	} \
}


void InterpolateVideo::average()
{
	VFrame *frame = get_output();
	int w = frame->get_w();
	int h = frame->get_h();

	switch(frame->get_color_model())
	{
		case BC_RGB_FLOAT:
			AVERAGE(float, float, 3, 1);
			break;
		case BC_RGB888:
		case BC_YUV888:
			AVERAGE(unsigned char, int, 3, 0xff);
			break;
		case BC_RGBA_FLOAT:
			AVERAGE(float, float, 4, 1);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			AVERAGE(unsigned char, int, 4, 0xff);
			break;
	}
}


int InterpolateVideo::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	if(get_direction() == PLAY_REVERSE) start_position--;
	load_configuration();

	if(!frames[0])
	{
		for(int i = 0; i < 2; i++)
		{
			frames[i] = new VFrame(frame->get_w(), frame->get_h(),
				frame->get_color_model(), 0);
		}
	}
//printf("InterpolateVideo::process_buffer 1 %lld %lld\n", range_start, range_end);

// Fraction of lowest frame in output
	int64_t requested_range_start = (int64_t)((double)range_start *
		frame_rate / active_input_rate);
	int64_t requested_range_end = (int64_t)((double)range_end *
		frame_rate / active_input_rate);
	if(requested_range_start == requested_range_end)
	{
		read_frame(frame, 0, range_start, active_input_rate, 0);
        }
        else
        {

// Fill border frames
		fill_border(frame_rate, start_position);

		float highest_fraction = (float)(start_position - requested_range_start) /
			(requested_range_end - requested_range_start);

// Fraction of highest frame in output
		lowest_fraction = 1.0 - highest_fraction;

		CLAMP(highest_fraction, 0, 1);
		CLAMP(lowest_fraction, 0, 1);

// printf("InterpolateVideo::process_buffer %lld %lld %lld %f %f %lld %lld %f %f\n",
// range_start,
// range_end,
// requested_range_start,
// requested_range_end,
// start_position,
// config.input_rate,
// frame_rate,
// lowest_fraction,
// highest_fraction);

		if(start_position == (int64_t)(range_start * frame_rate / active_input_rate))
		{
//printf("InterpolateVideo::process_buffer %d\n", __LINE__);
			frame->copy_from(frames[0]);

			if(config.optic_flow)
			{
				draw_vectors(0);
			}
		}
		else
		if(config.optic_flow)
		{
//printf("InterpolateVideo::process_buffer %d\n", __LINE__);
			optic_flow();
		}
		else
		{
			average();
		}
	}
	return 0;
}




NEW_WINDOW_MACRO(InterpolateVideo, InterpolateVideoWindow)
const char* InterpolateVideo::plugin_title() { return N_("Interpolate Video"); }
int InterpolateVideo::is_realtime() { return 1; }

int InterpolateVideo::load_configuration()
{
	KeyFrame *prev_keyframe, *next_keyframe;
	InterpolateVideoConfig old_config;
	old_config.copy_from(&config);

	next_keyframe = get_next_keyframe(get_source_position());
	prev_keyframe = get_prev_keyframe(get_source_position());
// Previous keyframe stays in config object.
	read_data(prev_keyframe);


	int64_t prev_position = edl_to_local(prev_keyframe->position);
	int64_t next_position = edl_to_local(next_keyframe->position);
	if(prev_position == 0 && next_position == 0)
	{
		next_position = prev_position = get_source_start();
	}
// printf("InterpolateVideo::load_configuration 1 %lld %lld %lld %lld\n",
// prev_keyframe->position,
// next_keyframe->position,
// prev_position,
// next_position);

// Get range to average in requested rate
	range_start = prev_position;
	range_end = next_position;


// Use keyframes to determine range
	if(config.use_keyframes)
	{
		active_input_rate = get_framerate();
// Between keyframe and edge of range or no keyframes
		if(range_start == range_end)
		{
// Between first keyframe and start of effect
			if(get_source_position() >= get_source_start() &&
				get_source_position() < range_start)
			{
				range_start = get_source_start();
			}
			else
// Between last keyframe and end of effect
			if(get_source_position() >= range_start &&
				get_source_position() < get_source_start() + get_total_len())
			{
// Last frame should be inclusive of current effect
				range_end = get_source_start() + get_total_len() - 1;
			}
			else
			{
// Should never get here
				;
			}
		}


// Make requested rate equal to input rate for this mode.

// Convert requested rate to input rate
// printf("InterpolateVideo::load_configuration 2 %lld %lld %f %f\n",
// range_start,
// range_end,
// get_framerate(),
// config.input_rate);
//		range_start = (int64_t)((double)range_start / get_framerate() * active_input_rate + 0.5);
//		range_end = (int64_t)((double)range_end / get_framerate() * active_input_rate + 0.5);
	}
	else
// Use frame rate
	{
		active_input_rate = config.input_rate;
// Convert to input frame rate
		range_start = (int64_t)(get_source_position() /
			get_framerate() *
			active_input_rate);
		range_end = (int64_t)(get_source_position() /
			get_framerate() *
			active_input_rate) + 1;
	}

// printf("InterpolateVideo::load_configuration 1 %lld %lld %lld %lld %lld %lld\n",
// prev_keyframe->position,
// next_keyframe->position,
// prev_position,
// next_position,
// range_start,
// range_end);


	return !config.equivalent(&old_config);
}


void InterpolateVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("INTERPOLATEVIDEO");
	output.tag.set_property("INPUT_RATE", config.input_rate);
	output.tag.set_property("USE_KEYFRAMES", config.use_keyframes);
	output.tag.set_property("OPTIC_FLOW", config.optic_flow);
	output.tag.set_property("DRAW_VECTORS", config.draw_vectors);
	output.tag.set_property("SEARCH_RADIUS", config.search_radius);
	output.tag.set_property("MACROBLOCK_SIZE", config.macroblock_size);
	output.append_tag();
	output.tag.set_title("/INTERPOLATEVIDEO");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void InterpolateVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	while(!input.read_tag())
	{
		if(input.tag.title_is("INTERPOLATEVIDEO"))
		{
			config.input_rate = input.tag.get_property("INPUT_RATE", config.input_rate);
			config.input_rate = Units::fix_framerate(config.input_rate);
			config.use_keyframes = input.tag.get_property("USE_KEYFRAMES", config.use_keyframes);
			config.optic_flow = input.tag.get_property("OPTIC_FLOW", config.optic_flow);
			config.draw_vectors = input.tag.get_property("DRAW_VECTORS", config.draw_vectors);
			config.search_radius = input.tag.get_property("SEARCH_RADIUS", config.search_radius);
			config.macroblock_size = input.tag.get_property("MACROBLOCK_SIZE", config.macroblock_size);
		}
	}
}

void InterpolateVideo::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("InterpolateVideo::update_gui");
			((InterpolateVideoWindow*)thread->window)->rate->update((float)config.input_rate);
			((InterpolateVideoWindow*)thread->window)->keyframes->update(config.use_keyframes);
			((InterpolateVideoWindow*)thread->window)->flow->update(config.optic_flow);
			((InterpolateVideoWindow*)thread->window)->vectors->update(config.draw_vectors);
			((InterpolateVideoWindow*)thread->window)->radius->update(config.search_radius);
			((InterpolateVideoWindow*)thread->window)->size->update(config.macroblock_size);
			((InterpolateVideoWindow*)thread->window)->update_enabled();
			thread->window->unlock_window();
		}
	}
}


