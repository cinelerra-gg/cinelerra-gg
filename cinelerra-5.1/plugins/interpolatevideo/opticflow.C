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





#include "clip.h"
#include "interpolatevideo.h"
#include "motioncache-hv.h"
#include "motionscan-hv.h"
#include "opticflow.h"

#include <sys/time.h>
#include <unistd.h>




OpticFlowMacroblock::OpticFlowMacroblock()
{
	dx = 0;
	dy = 0;
	visible = 0;
}

void OpticFlowMacroblock::copy_from(OpticFlowMacroblock *src)
{
	this->x = src->x;
	this->y = src->y;
	this->dx = src->dx;
	this->dy = src->dy;
	this->is_valid = src->is_valid;
// Temporaries for blending macroblocks
	this->angle1 = src->angle1;
	this->angle2 = src->angle2;
	this->dist = src->dist;
	this->visible = src->visible;
}







OpticFlowPackage::OpticFlowPackage()
 : LoadPackage()
{
}







OpticFlowUnit::OpticFlowUnit(OpticFlow *server)
 : LoadClient(server)
{
	this->server = server;
	motion = 0;
}

OpticFlowUnit::~OpticFlowUnit()
{
	delete motion;
}

void OpticFlowUnit::process_package(LoadPackage *package)
{
	OpticFlowPackage *pkg = (OpticFlowPackage*)package;
	InterpolateVideo *plugin = server->plugin;
	//int w = plugin->frames[0]->get_w();
	//int h = plugin->frames[0]->get_h();
	struct timeval start_time;
	gettimeofday(&start_time, 0);

	if(!motion) motion = new MotionHVScan(1, 1);

	motion->set_test_match(0);
	motion->set_cache(server->downsample_cache);
	
// printf("OpticFlowUnit::process_package %d %d %d\n",
// __LINE__,
// pkg->macroblock0,
// pkg->macroblock1);

	for(int i = pkg->macroblock0; i < pkg->macroblock1; i++)
	{
		OpticFlowMacroblock *mb = plugin->macroblocks.get(i);
//printf("OpticFlowUnit::process_package %d i=%d x=%d y=%d\n", __LINE__, i, mb->x, mb->y);
		motion->scan_frame(plugin->frames[0],
// Frame after motion
			plugin->frames[1],
			plugin->config.search_radius,
			plugin->config.search_radius,
			plugin->config.macroblock_size,
			plugin->config.macroblock_size,
			mb->x,
			mb->y,
			MotionHVScan::TRACK_PREVIOUS,
			MotionHVScan::CALCULATE,
// Get it to do the subpixel step
			MotionHVScan::STABILIZE,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			1,
			0,
			0,
			0);
//printf("OpticFlowUnit::process_package 2\n", __LINE__);


		mb->dx = motion->dx_result;
		mb->dy = motion->dy_result;
//		mb->is_valid = motion->result_valid;
		mb->is_valid = 1;
	}

	struct timeval end_time;
	gettimeofday(&end_time, 0);
// 	printf("OpticFlowUnit::process_package %d %d\n",
// 		__LINE__,
// 		end_time.tv_sec * 1000 + end_time.tv_usec / 1000 -
// 		start_time.tv_sec * 1000 - start_time.tv_usec / 1000);
}






OpticFlow::OpticFlow(InterpolateVideo *plugin,
	int total_clients,
	int total_packages)
// : LoadServer(1,
//	total_packages)
 : LoadServer(total_clients,
	total_packages)
{
	this->plugin = plugin;
	downsample_cache = 0;
}


OpticFlow::~OpticFlow()
{
	if(downsample_cache)
	{
//printf("OpticFlow::~OpticFlow %d %p\n", __LINE__, downsample_cache);
		delete downsample_cache;
	}
}

void OpticFlow::init_packages()
{
	if(!downsample_cache)
	{
		downsample_cache = new MotionHVCache();
	}
	
	downsample_cache->clear();

	for(int i = 0; i < get_total_packages(); i++)
	{
		OpticFlowPackage *pkg = (OpticFlowPackage*)get_package(i);
		pkg->macroblock0 = plugin->total_macroblocks * i / get_total_packages();
		pkg->macroblock1 = plugin->total_macroblocks * (i + 1) / get_total_packages();
// printf("OpticFlow::init_packages %d %d %d %d %d\n", 
// __LINE__, 
// plugin->total_macroblocks,
// get_total_packages(),
// pkg->macroblock0,
// pkg->macroblock1);
	}
}

LoadClient* OpticFlow::new_client()
{
	return new OpticFlowUnit(this);
}

LoadPackage* OpticFlow::new_package()
{
	return new OpticFlowPackage;
}







WarpPackage::WarpPackage()
 : LoadPackage()
{
}







WarpUnit::WarpUnit(Warp *server)
 : LoadClient(server)
{
	this->server = server;
}

WarpUnit::~WarpUnit()
{
}


#define AVERAGE2(type, components, max) \
{ \
	type *prev0 = (type*)prev_rows[(int)prev_y0] + ((int)prev_x0) * components; \
	type *prev1 = (type*)prev_rows[(int)prev_y0] + ((int)prev_x1) * components; \
	type *prev2 = (type*)prev_rows[(int)prev_y1] + ((int)prev_x0) * components; \
	type *prev3 = (type*)prev_rows[(int)prev_y1] + ((int)prev_x1) * components; \
 \
 \
 \
	type *next0 = (type*)next_rows[(int)next_y0] + ((int)next_x0) * components; \
	type *next1 = (type*)next_rows[(int)next_y0] + ((int)next_x1) * components; \
	type *next2 = (type*)next_rows[(int)next_y1] + ((int)next_x0) * components; \
	type *next3 = (type*)next_rows[(int)next_y1] + ((int)next_x1) * components; \
 \
 \
 \
	type *out_row = (type*)out_rows[i] + j * components; \
 \
	for(int k = 0; k < components; k++) \
	{ \
		float value = \
			prev_alpha * (*prev0 * prev_fraction_x0 * prev_fraction_y0 + \
				*prev1 * prev_fraction_x1 * prev_fraction_y0 + \
				*prev2 * prev_fraction_x0 * prev_fraction_y1 + \
				*prev3 * prev_fraction_x1 * prev_fraction_y1) + \
			next_alpha * (*next0 * next_fraction_x0 * next_fraction_y0 + \
				*next1 * next_fraction_x1 * next_fraction_y0 + \
				*next2 * next_fraction_x0 * next_fraction_y1 + \
				*next3 * next_fraction_x1 * next_fraction_y1); \
		CLAMP(value, 0, max); \
		*out_row++ = (type)value; \
		prev0++; \
		prev1++; \
		prev2++; \
		prev3++; \
		next0++; \
		next1++; \
		next2++; \
		next3++; \
	} \
}


void WarpUnit::process_package(LoadPackage *package)
{
	WarpPackage *pkg = (WarpPackage*)package;
	InterpolateVideo *plugin = server->plugin;
	int w = plugin->frames[0]->get_w();
	int h = plugin->frames[0]->get_h();
	unsigned char **prev_rows = plugin->frames[0]->get_rows();
	unsigned char **next_rows = plugin->frames[1]->get_rows();
	unsigned char **out_rows = plugin->get_output()->get_rows();
	int color_model = plugin->get_output()->get_color_model();
	int macroblock_size = plugin->config.macroblock_size;

	float lowest_fraction = plugin->lowest_fraction;
	//float highest_fraction = 1.0 - lowest_fraction;

	float prev_alpha = lowest_fraction;
	float next_alpha = 1.0 - prev_alpha;
//printf("WarpUnit::process_package %d %p %d %d\n", __LINE__, this, pkg->y1, pkg->y2);

// Count all macroblocks as valid
	for(int i = pkg->y1; i < pkg->y2; i++)
	{
		for(int j = 0; j < w; j++)
		{
// Get the motion vector for each pixel, based on the nearest motion vectors
			int x_macroblock = j / macroblock_size;
			int y_macroblock = i / macroblock_size;

			int x_macroblock2 = x_macroblock + 1;
			int y_macroblock2 = y_macroblock + 1;

			x_macroblock2 = MIN(x_macroblock2, plugin->x_macroblocks - 1);
			y_macroblock2 = MIN(y_macroblock2, plugin->y_macroblocks - 1);

			float x_fraction = (float)(j - x_macroblock * macroblock_size) / macroblock_size;
			float y_fraction = (float)(i - y_macroblock * macroblock_size) / macroblock_size;

			OpticFlowMacroblock *mb;
			mb = plugin->macroblocks.get(
				x_macroblock + y_macroblock * plugin->x_macroblocks);

			float dx = (float)mb->dx * (1.0 - x_fraction) * (1.0 - y_fraction);
			float dy = (float)mb->dy * (1.0 - x_fraction) * (1.0 - y_fraction);

			mb = plugin->macroblocks.get(
				x_macroblock2 + y_macroblock * plugin->x_macroblocks);
			dx += (float)mb->dx * (x_fraction) * (1.0 - y_fraction);
			dy += (float)mb->dy * (x_fraction) * (1.0 - y_fraction);

			mb = plugin->macroblocks.get(
				x_macroblock + y_macroblock2 * plugin->x_macroblocks);
			dx += (float)mb->dx * (1.0 - x_fraction) * (y_fraction);
			dy += (float)mb->dy * (1.0 - x_fraction) * (y_fraction);

			mb = plugin->macroblocks.get(
				x_macroblock2 + y_macroblock2 * plugin->x_macroblocks);
			dx += (float)mb->dx * (x_fraction) * (y_fraction);
			dy += (float)mb->dy * (x_fraction) * (y_fraction);

			dx /= (float)OVERSAMPLE;
			dy /= (float)OVERSAMPLE;


// Input pixels

// 4 pixels from prev frame
			float prev_x0 = (float)j + dx * (1.0 - lowest_fraction);
			float prev_y0 = (float)i + dy * (1.0 - lowest_fraction);
			float prev_x1 = prev_x0 + 1;
			float prev_y1 = prev_y0 + 1;
			float prev_fraction_x1 = prev_x0 - floor(prev_x0);
			float prev_fraction_x0 = 1.0 - prev_fraction_x1;
			float prev_fraction_y1 = prev_y0 - floor(prev_y0);
			float prev_fraction_y0 = 1.0 - prev_fraction_y1;


// 4 pixels from next frame
			float next_x0 = (float)j - dx * plugin->lowest_fraction;
			float next_y0 = (float)i - dy * plugin->lowest_fraction;
			float next_x1 = next_x0 + 1;
			float next_y1 = next_y0 + 1;
			float next_fraction_x1 = next_x0 - floor(next_x0);
			float next_fraction_x0 = 1.0 - next_fraction_x1;
			float next_fraction_y1 = next_y0 - floor(next_y0);
			float next_fraction_y0 = 1.0 - next_fraction_y1;


			CLAMP(prev_x0, 0, w - 1);
			CLAMP(prev_y0, 0, h - 1);
			CLAMP(prev_x1, 0, w - 1);
			CLAMP(prev_y1, 0, h - 1);

			CLAMP(next_x0, 0, w - 1);
			CLAMP(next_y0, 0, h - 1);
			CLAMP(next_x1, 0, w - 1);
			CLAMP(next_y1, 0, h - 1);

//printf("WarpUnit::process_package %d\n", __LINE__);

			switch(color_model)
			{
				case BC_RGB_FLOAT:
					AVERAGE2(float, 3, 1.0);
					break;
				case BC_RGB888:
				case BC_YUV888:
					AVERAGE2(unsigned char, 3, 0xff);
					break;
				case BC_RGBA_FLOAT:
					AVERAGE2(float, 4, 1.0);
					break;
				case BC_RGBA8888:
				case BC_YUVA8888:
					AVERAGE2(unsigned char, 4, 0xff);
					break;
			}
//printf("WarpUnit::process_package %d\n", __LINE__);
		}
	}
}






Warp::Warp(InterpolateVideo *plugin,
	int total_clients,
	int total_packages)
 : LoadServer(total_clients,
	total_packages)
{
	this->plugin = plugin;
}


Warp::~Warp()
{
}

void Warp::init_packages()
{
	int out_h = plugin->frames[0]->get_h();
	for(int i = 0; i < get_total_packages(); i++)
	{
		WarpPackage *pkg = (WarpPackage*)get_package(i);
		pkg->y1 = out_h * i / get_total_packages();
		pkg->y2 = out_h * (i + 1) / get_total_packages();
	}
}

LoadClient* Warp::new_client()
{
	return new WarpUnit(this);
}

LoadPackage* Warp::new_package()
{
	return new WarpPackage;
}








BlendPackage::BlendPackage()
 : LoadPackage()
{
}








BlendMacroblockUnit::BlendMacroblockUnit(BlendMacroblock *server)
 : LoadClient(server)
{
	this->server = server;
}

BlendMacroblockUnit::~BlendMacroblockUnit()
{
}

void BlendMacroblockUnit::process_package(LoadPackage *package)
{
	BlendPackage *pkg = (BlendPackage*)package;
	InterpolateVideo *plugin = server->plugin;

	for(int i = pkg->number0; i < pkg->number1; i++)
	{
		plugin->blend_macroblock(plugin->invalid_blocks.get(i));
	}
}










BlendMacroblock::BlendMacroblock(InterpolateVideo *plugin,
	int total_clients,
	int total_packages)
 : LoadServer(total_clients,
	total_packages)
{
	this->plugin = plugin;
}


BlendMacroblock::~BlendMacroblock()
{
}

void BlendMacroblock::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		BlendPackage *pkg = (BlendPackage*)get_package(i);
		pkg->number0 = plugin->invalid_blocks.size() * i / get_total_packages();
		pkg->number1 = plugin->invalid_blocks.size() * (i + 1) / get_total_packages();
	}
}



LoadClient* BlendMacroblock::new_client()
{
	return new BlendMacroblockUnit(this);
}

LoadPackage* BlendMacroblock::new_package()
{
	return new BlendPackage;
}




