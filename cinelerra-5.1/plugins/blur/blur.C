
/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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

#include "filexml.h"
#include "blur.h"
#include "blurwindow.h"
#include "bchash.h"
#include "keyframe.h"
#include "language.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>





#define MIN_RADIUS 2


BlurConfig::BlurConfig()
{
	vertical = 1;
	horizontal = 1;
	radius = 5;
	a_key = 0;
	a = r = g = b = 1;
}

int BlurConfig::equivalent(BlurConfig &that)
{
	return (vertical == that.vertical &&
		horizontal == that.horizontal &&
		radius == that.radius &&
		a_key == that.a_key &&
		a == that.a &&
		r == that.r &&
		g == that.g &&
		b == that.b);
}

void BlurConfig::copy_from(BlurConfig &that)
{
	vertical = that.vertical;
	horizontal = that.horizontal;
	radius = that.radius;
	a_key = that.a_key;
	a = that.a;
	r = that.r;
	g = that.g;
	b = that.b;
}

void BlurConfig::interpolate(BlurConfig &prev,
	BlurConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);


//printf("BlurConfig::interpolate %d %d %d\n", prev_frame, next_frame, current_frame);
	this->vertical = prev.vertical;
	this->horizontal = prev.horizontal;
	this->radius = round(prev.radius * prev_scale + next.radius * next_scale);
	a_key = prev.a_key;
	a = prev.a;
	r = prev.r;
	g = prev.g;
	b = prev.b;
}






REGISTER_PLUGIN(BlurMain)








BlurMain::BlurMain(PluginServer *server)
 : PluginVClient(server)
{
	need_reconfigure = 1;
	engine = 0;
	overlayer = 0;
}

BlurMain::~BlurMain()
{
//printf("BlurMain::~BlurMain 1\n");

	if(engine)
	{
		for(int i = 0; i < (get_project_smp() + 1); i++)
			delete engine[i];
		delete [] engine;
	}

	if(overlayer) delete overlayer;
}

const char* BlurMain::plugin_title() { return N_("Blur"); }
int BlurMain::is_realtime() { return 1; }


NEW_WINDOW_MACRO(BlurMain, BlurWindow)

LOAD_CONFIGURATION_MACRO(BlurMain, BlurConfig)




int BlurMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	int i;

	need_reconfigure |= load_configuration();

	read_frame(frame,
		0,
		start_position,
		frame_rate,
		0);

// Create temp based on alpha keying.
// Alpha keying needs 2x oversampling.

	if(config.a_key)
	{
		PluginVClient::new_temp(frame->get_w() * 2,
				frame->get_h() * 2,
				frame->get_color_model());
		if(!overlayer)
		{
			overlayer = new OverlayFrame(PluginClient::get_project_smp() + 1);
		}

		overlayer->overlay(PluginVClient::get_temp(),
			frame,
			0,
			0,
			frame->get_w(),
			frame->get_h(),
			0,
			0,
			PluginVClient::get_temp()->get_w(),
			PluginVClient::get_temp()->get_h(),
			1,        // 0 - 1
			TRANSFER_REPLACE,
			NEAREST_NEIGHBOR);
		input_frame = PluginVClient::get_temp();
	}
	else
	{
		PluginVClient::new_temp(frame->get_w(),
				frame->get_h(),
				frame->get_color_model());
		input_frame = frame;
	}


//printf("BlurMain::process_realtime 1 %d %d\n", need_reconfigure, config.radius);
	if(need_reconfigure)
	{
		if(!engine)
		{
			engine = new BlurEngine*[(get_project_smp() + 1)];
			for(i = 0; i < (get_project_smp() + 1); i++)
			{
				engine[i] = new BlurEngine(this);
				engine[i]->start();
			}
		}

		for(i = 0; i < (get_project_smp() + 1); i++)
		{
			engine[i]->reconfigure(&engine[i]->forward_constants, config.radius);
			engine[i]->reconfigure(&engine[i]->reverse_constants, config.radius);
		}
		need_reconfigure = 0;
	}


	if(config.radius < MIN_RADIUS ||
		(!config.vertical && !config.horizontal))
	{
// Data never processed
	}
	else
	{
// Process blur
// Need to blur vertically to a temp and
// horizontally to the output in 2 discrete passes.

		for(i = 0; i < get_project_smp() + 1; i++)
		{
			engine[i]->set_range(
				input_frame->get_h() * i / (get_project_smp() + 1),
				input_frame->get_h() * (i + 1) / (get_project_smp() + 1),
				input_frame->get_w() * i / (get_project_smp() + 1),
				input_frame->get_w() * (i + 1) / (get_project_smp() + 1));
		}

		for(i = 0; i < (get_project_smp() + 1); i++)
		{
			engine[i]->do_horizontal = 0;
			engine[i]->start_process_frame(input_frame);
		}

		for(i = 0; i < (get_project_smp() + 1); i++)
		{
			engine[i]->wait_process_frame();
		}

		for(i = 0; i < (get_project_smp() + 1); i++)
		{
			engine[i]->do_horizontal = 1;
			engine[i]->start_process_frame(input_frame);
		}

		for(i = 0; i < (get_project_smp() + 1); i++)
		{
			engine[i]->wait_process_frame();
		}
	}


// Downsample
	if(config.a_key)
	{
		overlayer->overlay(frame,
			PluginVClient::get_temp(),
			0,
			0,
			PluginVClient::get_temp()->get_w(),
			PluginVClient::get_temp()->get_h(),
			0,
			0,
			frame->get_w(),
			frame->get_h(),
			1,        // 0 - 1
			TRANSFER_REPLACE,
			NEAREST_NEIGHBOR);
	}

	return 0;
}


void BlurMain::update_gui()
{
	if(thread)
	{
		int reconfigure = load_configuration();
		if(reconfigure)
		{
			((BlurWindow*)thread->window)->lock_window("BlurMain::update_gui");
			((BlurWindow*)thread->window)->horizontal->update(config.horizontal);
			((BlurWindow*)thread->window)->vertical->update(config.vertical);
			((BlurWindow*)thread->window)->radius->update(config.radius);
			((BlurWindow*)thread->window)->radius_text->update((int64_t)config.radius);
			((BlurWindow*)thread->window)->a_key->update(config.a_key);
			((BlurWindow*)thread->window)->a->update(config.a);
			((BlurWindow*)thread->window)->r->update(config.r);
			((BlurWindow*)thread->window)->g->update(config.g);
			((BlurWindow*)thread->window)->b->update(config.b);
			((BlurWindow*)thread->window)->unlock_window();
		}
	}
}





void BlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("BLUR");
	output.tag.set_property("VERTICAL", config.vertical);
	output.tag.set_property("HORIZONTAL", config.horizontal);
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.tag.set_property("A_KEY", config.a_key);
	output.append_tag();
	output.tag.set_title("/BLUR");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void BlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("BLUR"))
			{
				config.vertical = input.tag.get_property("VERTICAL", config.vertical);
				config.horizontal = input.tag.get_property("HORIZONTAL", config.horizontal);
				config.radius = input.tag.get_property("RADIUS", config.radius);
//printf("BlurMain::read_data 1 %d %d %s\n", get_source_position(), keyframe->position, keyframe->get_data());
				config.r = input.tag.get_property("R", config.r);
				config.g = input.tag.get_property("G", config.g);
				config.b = input.tag.get_property("B", config.b);
				config.a = input.tag.get_property("A", config.a);
				config.a_key = input.tag.get_property("A_KEY", config.a_key);
			}
		}
	}
}









BlurEngine::BlurEngine(BlurMain *plugin)
 : Thread(1, 0, 0)
{
	this->plugin = plugin;
	last_frame = 0;

// Strip size
	int size = plugin->get_input()->get_w() > plugin->get_input()->get_h() ?
		plugin->get_input()->get_w() : plugin->get_input()->get_h();
// Prepare for oversampling
	size *= 2;
	val_p = new pixel_f[size];
	val_m = new pixel_f[size];
	radius = new double[size];
	src = new pixel_f[size];
	dst = new pixel_f[size];

	set_synchronous(1);
	input_lock.lock();
	output_lock.lock();
}

BlurEngine::~BlurEngine()
{
	last_frame = 1;
	input_lock.unlock();
	join();
	delete [] val_p;
	delete [] val_m;
	delete [] src;
	delete [] dst;
	delete [] radius;
}

void BlurEngine::set_range(int start_y,
	int end_y,
	int start_x,
	int end_x)
{
	this->start_y = start_y;
	this->end_y = end_y;
	this->start_x = start_x;
	this->end_x = end_x;
}

int BlurEngine::start_process_frame(VFrame *frame)
{
	this->frame = frame;
	input_lock.unlock();
	return 0;
}

int BlurEngine::wait_process_frame()
{
	output_lock.lock();
	return 0;
}

void BlurEngine::run()
{
	int j, k;


	while(1)
	{
		input_lock.lock();
		if(last_frame)
		{
			output_lock.unlock();
			return;
		}





		color_model = frame->get_color_model();
		int w = frame->get_w();
		int h = frame->get_h();
// Force recalculation of filter
		prev_forward_radius = -65536;
		prev_reverse_radius = -65536;




#define BLUR(type, max, components) \
{ \
	type **input_rows = (type **)frame->get_rows(); \
	type **output_rows = (type **)frame->get_rows(); \
	type **current_input = input_rows; \
	type **current_output = output_rows; \
	vmax = max; \
 \
	if(!do_horizontal && plugin->config.vertical) \
	{ \
/* Vertical pass */ \
/* Render to temp if a horizontal pass comes next */ \
/*		if(plugin->config.horizontal) */ \
/*		{ */ \
/*			current_output = (type **)plugin->get_temp()->get_rows(); */ \
/*		} */ \
 \
		for(j = start_x; j < end_x; j++) \
		{ \
			bzero(val_p, sizeof(pixel_f) * h); \
			bzero(val_m, sizeof(pixel_f) * h); \
 \
			for(k = 0; k < h; k++) \
			{ \
				if(plugin->config.r) src[k].r = (double)current_input[k][j * components]; \
				if(plugin->config.g) src[k].g = (double)current_input[k][j * components + 1]; \
				if(plugin->config.b) src[k].b = (double)current_input[k][j * components + 2]; \
				if(components == 4) \
				{ \
					if(plugin->config.a || plugin->config.a_key) src[k].a = (double)current_input[k][j * components + 3]; \
				} \
			} \
 \
			if(components == 4) \
				blur_strip4(h); \
			else \
				blur_strip3(h); \
 \
			for(k = 0; k < h; k++) \
			{ \
				if(plugin->config.r) current_output[k][j * components] = (type)dst[k].r; \
				if(plugin->config.g) current_output[k][j * components + 1] = (type)dst[k].g; \
				if(plugin->config.b) current_output[k][j * components + 2] = (type)dst[k].b; \
				if(components == 4) \
					if(plugin->config.a || plugin->config.a_key) current_output[k][j * components + 3] = (type)dst[k].a; \
			} \
		} \
 \
/*		current_input = current_output; */ \
/*		current_output = output_rows; */ \
	} \
 \
 \
	if(do_horizontal && plugin->config.horizontal) \
	{ \
/* Vertical pass */ \
/*		if(plugin->config.vertical) */ \
/*		{ */ \
/*			current_input = (type **)plugin->get_temp()->get_rows(); */ \
/*		} */ \
 \
/* Horizontal pass */ \
		for(j = start_y; j < end_y; j++) \
		{ \
			bzero(val_p, sizeof(pixel_f) * w); \
			bzero(val_m, sizeof(pixel_f) * w); \
 \
			for(k = 0; k < w; k++) \
			{ \
				if(plugin->config.r) src[k].r = (double)current_input[j][k * components]; \
				if(plugin->config.g) src[k].g = (double)current_input[j][k * components + 1]; \
				if(plugin->config.b) src[k].b = (double)current_input[j][k * components + 2]; \
				if(components == 4) \
					if(plugin->config.a || plugin->config.a_key) src[k].a = (double)current_input[j][k * components + 3]; \
			} \
 \
 			if(components == 4) \
				blur_strip4(w); \
			else \
				blur_strip3(w); \
 \
			for(k = 0; k < w; k++) \
			{ \
				if(plugin->config.r) current_output[j][k * components] = (type)dst[k].r; \
				if(plugin->config.g) current_output[j][k * components + 1] = (type)dst[k].g; \
				if(plugin->config.b) current_output[j][k * components + 2] = (type)dst[k].b; \
				if(components == 4) \
				{ \
					if(plugin->config.a && !plugin->config.a_key) \
						current_output[j][k * components + 3] = (type)dst[k].a; \
					else if(plugin->config.a_key) \
						current_output[j][k * components + 3] = max; \
				} \
			} \
		} \
	} \
}



		switch(color_model)
		{
			case BC_RGB888:
			case BC_YUV888:
				BLUR(unsigned char, 0xff, 3);
				break;

			case BC_RGB_FLOAT:
				BLUR(float, 1.0, 3);
				break;

			case BC_RGBA8888:
			case BC_YUVA8888:
				BLUR(unsigned char, 0xff, 4);
				break;

			case BC_RGBA_FLOAT:
				BLUR(float, 1.0, 4);
				break;

			case BC_RGB161616:
			case BC_YUV161616:
				BLUR(uint16_t, 0xffff, 3);
				break;

			case BC_RGBA16161616:
			case BC_YUVA16161616:
				BLUR(uint16_t, 0xffff, 4);
				break;
		}

		output_lock.unlock();
	}
}

int BlurEngine::reconfigure(BlurConstants *constants, double radius)
{
// Blurring an oversampled temp
	if(plugin->config.a_key) radius *= 2;
	double std_dev = sqrt(-(double)(radius * radius) /
		(2 * log (1.0 / 255.0)));
	get_constants(constants, std_dev);
	return 0;
}

int BlurEngine::get_constants(BlurConstants *ptr, double std_dev)
{
	int i;
	double constants[8];
	double div;

	div = sqrt(2 * M_PI) * std_dev;
	constants[0] = -1.783 / std_dev;
	constants[1] = -1.723 / std_dev;
	constants[2] = 0.6318 / std_dev;
	constants[3] = 1.997  / std_dev;
	constants[4] = 1.6803 / div;
	constants[5] = 3.735 / div;
	constants[6] = -0.6803 / div;
	constants[7] = -0.2598 / div;

	ptr->n_p[0] = constants[4] + constants[6];
	ptr->n_p[1] = exp(constants[1]) *
				(constants[7] * sin(constants[3]) -
				(constants[6] + 2 * constants[4]) * cos(constants[3])) +
				exp(constants[0]) *
				(constants[5] * sin(constants[2]) -
				(2 * constants[6] + constants[4]) * cos(constants[2]));

	ptr->n_p[2] = 2 * exp(constants[0] + constants[1]) *
				((constants[4] + constants[6]) * cos(constants[3]) *
				cos(constants[2]) - constants[5] *
				cos(constants[3]) * sin(constants[2]) -
				constants[7] * cos(constants[2]) * sin(constants[3])) +
				constants[6] * exp(2 * constants[0]) +
				constants[4] * exp(2 * constants[1]);

	ptr->n_p[3] = exp(constants[1] + 2 * constants[0]) *
				(constants[7] * sin(constants[3]) -
				constants[6] * cos(constants[3])) +
				exp(constants[0] + 2 * constants[1]) *
				(constants[5] * sin(constants[2]) - constants[4] *
				cos(constants[2]));
	ptr->n_p[4] = 0.0;

	ptr->d_p[0] = 0.0;
	ptr->d_p[1] = -2 * exp(constants[1]) * cos(constants[3]) -
				2 * exp(constants[0]) * cos(constants[2]);

	ptr->d_p[2] = 4 * cos(constants[3]) * cos(constants[2]) *
				exp(constants[0] + constants[1]) +
				exp(2 * constants[1]) + exp (2 * constants[0]);

	ptr->d_p[3] = -2 * cos(constants[2]) * exp(constants[0] + 2 * constants[1]) -
				2 * cos(constants[3]) * exp(constants[1] + 2 * constants[0]);

	ptr->d_p[4] = exp(2 * constants[0] + 2 * constants[1]);

	for(i = 0; i < 5; i++) ptr->d_m[i] = ptr->d_p[i];

	ptr->n_m[0] = 0.0;
	for(i = 1; i <= 4; i++)
		ptr->n_m[i] = ptr->n_p[i] - ptr->d_p[i] * ptr->n_p[0];

	double sum_n_p, sum_n_m, sum_d;
	double a, b;

	sum_n_p = 0.0;
	sum_n_m = 0.0;
	sum_d = 0.0;
	for(i = 0; i < 5; i++)
	{
		sum_n_p += ptr->n_p[i];
		sum_n_m += ptr->n_m[i];
		sum_d += ptr->d_p[i];
	}

	a = sum_n_p / (1 + sum_d);
	b = sum_n_m / (1 + sum_d);

	for (i = 0; i < 5; i++)
	{
		ptr->bd_p[i] = ptr->d_p[i] * a;
		ptr->bd_m[i] = ptr->d_m[i] * b;
	}

	return 0;
}

#define BOUNDARY(x) if((x) > vmax) (x) = vmax; else if((x) < 0) (x) = 0;

int BlurEngine::transfer_pixels(pixel_f *src1,
	pixel_f *src2,
	pixel_f *src,
	double *radius,
	pixel_f *dest,
	int size)
{
	int i;
	double sum;

// printf("BlurEngine::transfer_pixels %d %d %d %d\n",
// plugin->config.r,
// plugin->config.g,
// plugin->config.b,
// plugin->config.a);

	for(i = 0; i < size; i++)
    {
		sum = src1[i].r + src2[i].r;
		BOUNDARY(sum);
		dest[i].r = sum;

		sum = src1[i].g + src2[i].g;
		BOUNDARY(sum);
		dest[i].g = sum;

		sum = src1[i].b + src2[i].b;
		BOUNDARY(sum);
		dest[i].b = sum;

		sum = src1[i].a + src2[i].a;
		BOUNDARY(sum);
		dest[i].a = sum;

// 		if(radius[i] < 2)
// 		{
// 			double scale = 2.0 - (radius[i] * radius[i] - 2.0);
// 			dest[i].r /= scale;
// 			dest[i].g /= scale;
// 			dest[i].b /= scale;
// 			dest[i].a /= scale;
// 		}
    }
	return 0;
}


int BlurEngine::multiply_alpha(pixel_f *row, int size)
{
//	int i;
//	double alpha;

// 	for(i = 0; i < size; i++)
// 	{
// 		alpha = (double)row[i].a / vmax;
// 		row[i].r *= alpha;
// 		row[i].g *= alpha;
// 		row[i].b *= alpha;
// 	}
	return 0;
}

int BlurEngine::separate_alpha(pixel_f *row, int size)
{
//	int i;
//	double alpha;
//	double result;

// 	for(i = 0; i < size; i++)
// 	{
// 		if(row[i].a > 0 && row[i].a < vmax)
// 		{
// 			alpha = (double)row[i].a / vmax;
// 			result = (double)row[i].r / alpha;
// 			row[i].r = (result > vmax ? vmax : result);
// 			result = (double)row[i].g / alpha;
// 			row[i].g = (result > vmax ? vmax : result);
// 			result = (double)row[i].b / alpha;
// 			row[i].b = (result > vmax ? vmax : result);
// 		}
// 	}
	return 0;
}

int BlurEngine::blur_strip3(int &size)
{
//	multiply_alpha(src, size);

	pixel_f *sp_p = src;
	pixel_f *sp_m = src + size - 1;
	pixel_f *vp = val_p;
	pixel_f *vm = val_m + size - 1;

	initial_p = sp_p[0];
	initial_m = sp_m[0];

	int l;
	for(int k = 0; k < size; k++)
	{
		terms = (k < 4) ? k : 4;

		radius[k] = plugin->config.radius;

		for(l = 0; l <= terms; l++)
		{
			if(plugin->config.r)
			{
				vp->r += forward_constants.n_p[l] * sp_p[-l].r - forward_constants.d_p[l] * vp[-l].r;
				vm->r += reverse_constants.n_m[l] * sp_m[l].r - reverse_constants.d_m[l] * vm[l].r;
			}
			if(plugin->config.g)
			{
				vp->g += forward_constants.n_p[l] * sp_p[-l].g - forward_constants.d_p[l] * vp[-l].g;
				vm->g += reverse_constants.n_m[l] * sp_m[l].g - reverse_constants.d_m[l] * vm[l].g;
			}
			if(plugin->config.b)
			{
				vp->b += forward_constants.n_p[l] * sp_p[-l].b - forward_constants.d_p[l] * vp[-l].b;
				vm->b += reverse_constants.n_m[l] * sp_m[l].b - reverse_constants.d_m[l] * vm[l].b;
			}
		}

		for( ; l <= 4; l++)
		{
			if(plugin->config.r)
			{
				vp->r += (forward_constants.n_p[l] - forward_constants.bd_p[l]) * initial_p.r;
				vm->r += (reverse_constants.n_m[l] - reverse_constants.bd_m[l]) * initial_m.r;
			}
			if(plugin->config.g)
			{
				vp->g += (forward_constants.n_p[l] - forward_constants.bd_p[l]) * initial_p.g;
				vm->g += (reverse_constants.n_m[l] - reverse_constants.bd_m[l]) * initial_m.g;
			}
			if(plugin->config.b)
			{
				vp->b += (forward_constants.n_p[l] - forward_constants.bd_p[l]) * initial_p.b;
				vm->b += (reverse_constants.n_m[l] - reverse_constants.bd_m[l]) * initial_m.b;
			}
		}
		sp_p++;
		sp_m--;
		vp++;
		vm--;
	}

	transfer_pixels(val_p, val_m, src, radius, dst, size);
//	separate_alpha(dst, size);
	return 0;
}


int BlurEngine::blur_strip4(int &size)
{

//	multiply_alpha(src, size);

	pixel_f *sp_p = src;
	pixel_f *sp_m = src + size - 1;
	pixel_f *vp = val_p;
	pixel_f *vm = val_m + size - 1;

	initial_p = sp_p[0];
	initial_m = sp_m[0];

	int l;
	for(int k = 0; k < size; k++)
	{
		if(plugin->config.a_key)
			radius[k] = (double)plugin->config.radius * src[k].a / vmax;
		else
			radius[k] = plugin->config.radius;
	}

	for(int k = 0; k < size; k++)
	{
		terms = (k < 4) ? k : 4;

		if(plugin->config.a_key)
		{
			if(radius[k] != prev_forward_radius)
			{
				prev_forward_radius = radius[k];
				if(radius[k] >= MIN_RADIUS / 2)
				{
					reconfigure(&forward_constants, radius[k]);
				}
				else
				{
					reconfigure(&forward_constants, (double)MIN_RADIUS / 2);
				}
			}

			if(radius[size - 1 - k] != prev_reverse_radius)
			{
//printf("BlurEngine::blur_strip4 %f\n", sp_m->a);
				prev_reverse_radius = radius[size - 1 - k];
				if(radius[size - 1 - k] >= MIN_RADIUS / 2)
				{
					reconfigure(&reverse_constants, radius[size - 1 - k]);
				}
				else
				{
					reconfigure(&reverse_constants, (double)MIN_RADIUS / 2);
				}
			}

// Force alpha to be copied regardless of alpha blur enabled
			vp->a = sp_p->a;
			vm->a = sp_m->a;
		}


		for(l = 0; l <= terms; l++)
		{
			if(plugin->config.r)
			{
				vp->r += forward_constants.n_p[l] * sp_p[-l].r - forward_constants.d_p[l] * vp[-l].r;
				vm->r += reverse_constants.n_m[l] * sp_m[l].r - reverse_constants.d_m[l] * vm[l].r;
			}
			if(plugin->config.g)
			{
				vp->g += forward_constants.n_p[l] * sp_p[-l].g - forward_constants.d_p[l] * vp[-l].g;
				vm->g += reverse_constants.n_m[l] * sp_m[l].g - reverse_constants.d_m[l] * vm[l].g;
			}
			if(plugin->config.b)
			{
				vp->b += forward_constants.n_p[l] * sp_p[-l].b - forward_constants.d_p[l] * vp[-l].b;
				vm->b += reverse_constants.n_m[l] * sp_m[l].b - reverse_constants.d_m[l] * vm[l].b;
			}
			if(plugin->config.a && !plugin->config.a_key)
			{
				vp->a += forward_constants.n_p[l] * sp_p[-l].a - forward_constants.d_p[l] * vp[-l].a;
				vm->a += reverse_constants.n_m[l] * sp_m[l].a - reverse_constants.d_m[l] * vm[l].a;
			}
		}

		for( ; l <= 4; l++)
		{
			if(plugin->config.r)
			{
				vp->r += (forward_constants.n_p[l] - forward_constants.bd_p[l]) * initial_p.r;
				vm->r += (reverse_constants.n_m[l] - reverse_constants.bd_m[l]) * initial_m.r;
			}
			if(plugin->config.g)
			{
				vp->g += (forward_constants.n_p[l] - forward_constants.bd_p[l]) * initial_p.g;
				vm->g += (reverse_constants.n_m[l] - reverse_constants.bd_m[l]) * initial_m.g;
			}
			if(plugin->config.b)
			{
				vp->b += (forward_constants.n_p[l] - forward_constants.bd_p[l]) * initial_p.b;
				vm->b += (reverse_constants.n_m[l] - reverse_constants.bd_m[l]) * initial_m.b;
			}
			if(plugin->config.a && !plugin->config.a_key)
			{
				vp->a += (forward_constants.n_p[l] - forward_constants.bd_p[l]) * initial_p.a;
				vm->a += (reverse_constants.n_m[l] - reverse_constants.bd_m[l]) * initial_m.a;
			}
		}

		sp_p++;
		sp_m--;
		vp++;
		vm--;
	}

	transfer_pixels(val_p, val_m, src, radius, dst, size);
//	separate_alpha(dst, size);
	return 0;
}






