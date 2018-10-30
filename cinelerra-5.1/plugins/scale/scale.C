
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
#include "filexml.h"
#include "language.h"
#include "mwindow.h"
#include "pluginserver.h"
#include "scale.h"
#include "scalewin.h"

#include <string.h>


REGISTER_PLUGIN(ScaleMain)



ScaleConfig::ScaleConfig()
{
	type = FIXED_SCALE;
	x_factor = y_factor = 1;
	width = height = 0;
	constrain = 0;
}

void ScaleConfig::copy_from(ScaleConfig &src)
{
	type = src.type;
	x_factor = src.x_factor;
	y_factor = src.y_factor;
	constrain = src.constrain;
	width = src.width;
	height = src.height;
}
int ScaleConfig::equivalent(ScaleConfig &src)
{
	return type != src.type ? 0 : type == FIXED_SCALE ?
		EQUIV(x_factor, src.x_factor) && EQUIV(y_factor, src.y_factor) &&
			constrain == src.constrain :
		width == src.width && height == src.height;
}

void ScaleConfig::interpolate(ScaleConfig &prev, ScaleConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	double u = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	double v = 1. - u;

	this->type = prev.type;
	this->x_factor = u*prev.x_factor + v*next.x_factor;
	this->y_factor = u*prev.y_factor + v*next.y_factor;
	this->constrain = prev.constrain;
	this->width = u*prev.width + v*next.width;
	this->height = u*prev.height + v*next.height;
}



ScaleMain::ScaleMain(PluginServer *server)
 : PluginVClient(server)
{
	this->server = server;
	overlayer = 0;
}

ScaleMain::~ScaleMain()
{
	if(overlayer) delete overlayer;
}

const char* ScaleMain::plugin_title() { return N_("Scale"); }
int ScaleMain::is_realtime() { return 1; }



LOAD_CONFIGURATION_MACRO(ScaleMain, ScaleConfig)


void ScaleMain::set_type(int type)
{
	if( type != config.type ) {
		config.type = type;
		ScaleWin *swin = (ScaleWin *)thread->window;
		int fixed_scale = type == FIXED_SCALE ? 1 : 0;
		swin->use_scale->update(fixed_scale);
		swin->x_factor->enabled = fixed_scale;
		swin->y_factor->enabled = fixed_scale;
		int fixed_size = 1 - fixed_scale;
		swin->use_size->update(fixed_size);
		swin->width->enabled = fixed_size;
		swin->height->enabled = fixed_size;
		send_configure_change();
	}
}

void ScaleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);

// Store data
	output.tag.set_title("SCALE");
	output.tag.set_property("TYPE", config.type);
	output.tag.set_property("X_FACTOR", config.x_factor);
	output.tag.set_property("Y_FACTOR", config.y_factor);
	output.tag.set_property("WIDTH", config.width);
	output.tag.set_property("HEIGHT", config.height);
	output.tag.set_property("CONSTRAIN", config.constrain);
	output.append_tag();
	output.tag.set_title("/SCALE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
// data is now in *text
}

void ScaleMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	config.constrain = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SCALE"))
			{
				config.type = input.tag.get_property("TYPE", config.type);
				config.x_factor = input.tag.get_property("X_FACTOR", config.x_factor);
				config.y_factor = input.tag.get_property("Y_FACTOR", config.y_factor);
				config.constrain = input.tag.get_property("CONSTRAIN", config.constrain);
				config.width = input.tag.get_property("WIDTH", config.x_factor);
				config.height = input.tag.get_property("HEIGHT", config.y_factor);
			}
		}
	}
}








int ScaleMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	VFrame *input, *output;
	input = output = frame;

	load_configuration();
	read_frame(frame, 0, start_position, frame_rate, get_use_opengl());

// No scaling
	if( config.type == FIXED_SCALE ?
		config.x_factor == 1 && config.y_factor == 1 :
		config.width == frame->get_w() && config.height == frame->get_h() )
		return 0;

	if(get_use_opengl()) return run_opengl();

	VFrame *temp_frame = new_temp(frame->get_w(), frame->get_h(),
			frame->get_color_model());
	temp_frame->copy_from(frame);
	input = temp_frame;

	if(!overlayer)
	{
		overlayer = new OverlayFrame(PluginClient::get_project_smp() + 1);
	}


// Perform scaling
	float in_x1, in_x2, in_y1, in_y2, out_x1, out_x2, out_y1, out_y2;
	calculate_transfer(output,
		in_x1, in_x2, in_y1, in_y2,
		out_x1, out_x2, out_y1, out_y2);
	output->clear_frame();

// printf("ScaleMain::process_realtime 3 output=%p input=%p config.x_factor=%f config.y_factor=%f"
// 	" config.width=%d config.height=%d config.type=%d   %f %f %f %f -> %f %f %f %f\n",
// 	output, input, config.x_factor, config.y_factor, config.width, config.height, config.type,
// 	in_x1, in_y1, in_x2, in_y2, out_x1, out_y1, out_x2, out_y2);
	overlayer->overlay(output, input,
		in_x1, in_y1, in_x2, in_y2,
		out_x1, out_y1, out_x2, out_y2,
		1, TRANSFER_REPLACE, get_interpolation_type());

	return 0;
}

void ScaleMain::calculate_transfer(VFrame *frame,
	float &in_x1, float &in_x2, float &in_y1, float &in_y2,
	float &out_x1, float &out_x2, float &out_y1, float &out_y2)
{
	float fw = (float)frame->get_w();
	float fh = (float)frame->get_h();
	in_x1 = in_y1 = 0;
	in_x2 = fw;
	in_y2 = fh;
	float x_factor = config.type == FIXED_SCALE ? config.x_factor :
		in_x2 > 0. ? (float)config.width / in_x2 : 0;
	float y_factor = config.type == FIXED_SCALE ? config.y_factor :
		in_y2 > 0. ? (float)config.height / in_y2 : 0;

	float fw2 = fw/2, fw2s = fw2*x_factor;
	float fh2 = fh/2, fh2s = fh2*y_factor;
	out_x1 = fw2 - fw2s;
	out_x2 = fw2 + fw2s;
	out_y1 = fh2 - fh2s;
	out_y2 = fh2 + fh2s;

	if(out_x1 < 0) {
		in_x1 = x_factor>0 ? in_x1 - out_x1/x_factor : 0;
		out_x1 = 0;
	}

	if(out_x2 > frame->get_w()) {
		in_x2 = x_factor>0 ? in_x2 - (out_x2-frame->get_w())/x_factor : 0;
		out_x2 = frame->get_w();
	}

	if(out_y1 < 0) {
		in_y1 = y_factor>0 ? in_y1 - out_y1/y_factor : 0;
		out_y1 = 0;
	}

	if(out_y2 > frame->get_h()) {
		in_y2 = y_factor>0 ? in_y2 - (out_y2-frame->get_h())/y_factor : 0;
		out_y2 = frame->get_h();
	}
}

int ScaleMain::handle_opengl()
{
#ifdef HAVE_GL
	float in_x1, in_x2, in_y1, in_y2, out_x1, out_x2, out_y1, out_y2;
	calculate_transfer(get_output(),
		in_x1, in_x2, in_y1, in_y2,
		out_x1, out_x2, out_y1, out_y2);

	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();
	get_output()->clear_pbuffer();
	get_output()->bind_texture(0);
	get_output()->draw_texture(
		in_x1, in_y1, in_x2, in_y2,
		out_x1, out_y1, out_x2, out_y2);
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
	return 0;
}



NEW_WINDOW_MACRO(ScaleMain, ScaleWin)

void ScaleMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		set_type(config.type);
		ScaleWin *swin = (ScaleWin *)thread->window;
		swin->x_factor->update(config.x_factor);
		swin->y_factor->update(config.y_factor);
		swin->width->update((int64_t)config.width);
		swin->height->update((int64_t)config.height);
		swin->constrain->update(config.constrain);
		thread->window->unlock_window();
	}
}



