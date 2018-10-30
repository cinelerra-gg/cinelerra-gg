
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



#include "clip.h"
#include "downsampleengine.h"
#include "vframe.h"


#include <stdint.h>

DownSamplePackage::DownSamplePackage()
 : LoadPackage()
{
}




DownSampleUnit::DownSampleUnit(DownSampleServer *server)
 : LoadClient(server)
{
	this->server = server;
}



#define DOWNSAMPLE(type, temp_type, components, max) \
{ \
	temp_type r; \
	temp_type g; \
	temp_type b; \
	temp_type a; \
	int do_r = server->r; \
	int do_g = server->g; \
	int do_b = server->b; \
	int do_a = server->a; \
	type **in_rows = (type**)server->input->get_rows(); \
	type **out_rows = (type**)server->output->get_rows(); \
 \
	for(int i = pkg->y1; i < pkg->y2; i += server->vertical) \
	{ \
		int y1 = MAX(i, 0); \
		int y2 = MIN(i + server->vertical, h); \
 \
 \
		for(int j = server->horizontal_x - server->horizontal; \
			j < w; \
			j += server->horizontal) \
		{ \
			int x1 = MAX(j, 0); \
			int x2 = MIN(j + server->horizontal, w); \
 \
			temp_type scale = (x2 - x1) * (y2 - y1); \
			if(x2 > x1 && y2 > y1) \
			{ \
 \
/* Read in values */ \
				r = 0; \
				g = 0; \
				b = 0; \
				if(components == 4) a = 0; \
 \
				for(int k = y1; k < y2; k++) \
				{ \
					type *row = in_rows[k] + x1 * components; \
					for(int l = x1; l < x2; l++) \
					{ \
						if(do_r) r += *row++; else row++; \
						if(do_g) g += *row++; else row++;  \
						if(do_b) b += *row++; else row++;  \
						if(components == 4) \
						{ \
							if(do_a) *row = a; \
							 row++; \
						} \
					} \
				} \
 \
/* Write average */ \
				r /= scale; \
				g /= scale; \
				b /= scale; \
				if(components == 4) a /= scale; \
				for(int k = y1; k < y2; k++) \
				{ \
					type *row = out_rows[k] + x1 * components; \
					for(int l = x1; l < x2; l++) \
					{ \
						if(do_r) *row++ = r; else row++; \
						if(do_g) *row++ = g; else row++; \
						if(do_b) *row++ = b; else row++; \
						if(components == 4) \
						{ \
							if(do_a) *row = a; \
							 row++; \
						} \
					} \
				} \
			} \
		} \
/*printf("DOWNSAMPLE 3 %d\n", i);*/ \
	} \
}

void DownSampleUnit::process_package(LoadPackage *package)
{
	DownSamplePackage *pkg = (DownSamplePackage*)package;
	int h = server->output->get_h();
	int w = server->output->get_w();


	switch(server->input->get_color_model())
	{
		case BC_RGB888:
			DOWNSAMPLE(uint8_t, int64_t, 3, 0xff)
			break;
		case BC_RGB_FLOAT:
			DOWNSAMPLE(float, float, 3, 1.0)
			break;
		case BC_RGBA8888:
			DOWNSAMPLE(uint8_t, int64_t, 4, 0xff)
			break;
		case BC_RGBA_FLOAT:
			DOWNSAMPLE(float, float, 4, 1.0)
			break;
		case BC_RGB161616:
			DOWNSAMPLE(uint16_t, int64_t, 3, 0xffff)
			break;
		case BC_RGBA16161616:
			DOWNSAMPLE(uint16_t, int64_t, 4, 0xffff)
			break;
		case BC_YUV888:
			DOWNSAMPLE(uint8_t, int64_t, 3, 0xff)
			break;
		case BC_YUVA8888:
			DOWNSAMPLE(uint8_t, int64_t, 4, 0xff)
			break;
		case BC_YUV161616:
			DOWNSAMPLE(uint16_t, int64_t, 3, 0xffff)
			break;
		case BC_YUVA16161616:
			DOWNSAMPLE(uint16_t, int64_t, 4, 0xffff)
			break;
	}
}






DownSampleServer::DownSampleServer(int total_clients,
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
}

void DownSampleServer::init_packages()
{
	int y1 = vertical_y - vertical;
	int total_strips = (int)((float)output->get_h() / vertical + 1);
	int strips_per_package = (int)((float)total_strips / get_total_packages() + 1);

	for(int i = 0; i < get_total_packages(); i++)
	{
		DownSamplePackage *package = (DownSamplePackage*)get_package(i);
		package->y1 = y1;
		package->y2 = y1 + strips_per_package * vertical;
		package->y1 = MIN(output->get_h(), package->y1);
		package->y2 = MIN(output->get_h(), package->y2);
		y1 = package->y2;
	}
}

LoadClient* DownSampleServer::new_client()
{
	return new DownSampleUnit(this);
}

LoadPackage* DownSampleServer::new_package()
{
	return new DownSamplePackage;
}

void DownSampleServer::process_frame(VFrame *output,
	VFrame *input,
	int r,
	int g,
	int b,
	int a,
	int vertical,
	int horizontal,
	int vertical_y,
	int horizontal_x)
{
	this->input = input;
	this->output = output;
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
	this->vertical = vertical;
	this->vertical_y = vertical_y;
	this->horizontal = horizontal;
	this->horizontal_x = horizontal_x;

	process_packages();
}






