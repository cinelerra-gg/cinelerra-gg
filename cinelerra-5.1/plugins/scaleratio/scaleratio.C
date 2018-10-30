
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
#include "scaleratio.h"
#include "scaleratiowin.h"

#include <string.h>




REGISTER_PLUGIN(ScaleRatioMain)

ScaleRatioConfig::ScaleRatioConfig()
{
	type = 0;
	in_w = 720;   in_h = 480;   in_r = 1;
	out_w = 720;  out_h = 480;  out_r = 1;
	src_x = src_y = 0;  src_w = 720;  src_h = 480;
	dst_x = dst_y = 0;  dst_w = 720;  dst_h = 480;
}

int ScaleRatioConfig::equivalent(ScaleRatioConfig &that)
{
	return EQUIV(src_x, that.src_x) && EQUIV(src_y, that.src_y) &&
		EQUIV(src_w, that.src_w) && EQUIV(src_h, that.src_h) &&
		EQUIV(dst_x, that.dst_x) && EQUIV(dst_y, that.dst_y) &&
		EQUIV(dst_w, that.dst_w) && EQUIV(dst_h, that.dst_h);
}

void ScaleRatioConfig::copy_from(ScaleRatioConfig &that)
{
	in_w  = that.in_w;   in_h  = that.in_h;   in_r  = that.in_r;
	out_w = that.out_w;  out_h = that.out_h;  out_r = that.out_r;
	src_x = that.src_x;  src_y = that.src_y;
	src_w = that.src_w;  src_h = that.src_h;
	dst_x = that.dst_x;  dst_y = that.dst_y;
	dst_w = that.dst_w;  dst_h = that.dst_h;
}

void ScaleRatioConfig::interpolate(ScaleRatioConfig &prev, ScaleRatioConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->src_x = prev.src_x * prev_scale + next.src_x * next_scale;
	this->src_y = prev.src_y * prev_scale + next.src_y * next_scale;
	this->src_w = prev.src_w * prev_scale + next.src_w * next_scale;
	this->src_h = prev.src_h * prev_scale + next.src_h * next_scale;
	this->dst_x = prev.dst_x * prev_scale + next.dst_x * next_scale;
	this->dst_y = prev.dst_y * prev_scale + next.dst_y * next_scale;
	this->dst_w = prev.dst_w * prev_scale + next.dst_w * next_scale;
	this->dst_h = prev.dst_h * prev_scale + next.dst_h * next_scale;
}


ScaleRatioMain::ScaleRatioMain(PluginServer *server)
 : PluginVClient(server)
{
	overlayer = 0;
	temp_frame = 0;
}

ScaleRatioMain::~ScaleRatioMain()
{


	if(temp_frame) delete temp_frame;
	temp_frame = 0;
	if(overlayer) delete overlayer;
	overlayer = 0;
}

const char* ScaleRatioMain::plugin_title() { return N_("Scale Ratio"); }
int ScaleRatioMain::is_realtime() { return 1; }



LOAD_CONFIGURATION_MACRO(ScaleRatioMain, ScaleRatioConfig)

void ScaleRatioMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);

// Store data
	output.tag.set_title("SCALERATIO");
	output.tag.set_property("TYPE", config.type);
	output.tag.set_property("IN_W", config.in_w);
	output.tag.set_property("IN_H", config.in_h);
	output.tag.set_property("IN_ASPECT_RATIO", config.in_r);
	output.tag.set_property("OUT_W", config.out_w);
	output.tag.set_property("OUT_H", config.out_h);
	output.tag.set_property("OUT_ASPECT_RATIO", config.out_r);
	output.tag.set_property("SRC_X", config.src_x);
	output.tag.set_property("SRC_Y", config.src_y);
	output.tag.set_property("SRC_W", config.src_w);
	output.tag.set_property("SRC_H", config.src_h);
	output.tag.set_property("DST_X", config.dst_x);
	output.tag.set_property("DST_Y", config.dst_y);
	output.tag.set_property("DST_W", config.dst_w);
	output.tag.set_property("DST_H", config.dst_h);
	output.append_tag();
	output.tag.set_title("/SCALERATIO");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
// data is now in *text
}

void ScaleRatioMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while( !(result = input.read_tag()) ) {
		if(input.tag.title_is("SCALERATIO")) {
 			config.type  = input.tag.get_property("TYPE", config.type);
			config.in_w  = input.tag.get_property("IN_W", config.in_w);
			config.in_h  = input.tag.get_property("IN_H", config.in_h);
			config.in_r  = input.tag.get_property("IN_ASPECT_RATIO", config.in_r);
			config.out_w = input.tag.get_property("OUT_W", config.out_w);
			config.out_h = input.tag.get_property("OUT_H", config.out_h);
			config.out_r = input.tag.get_property("OUT_ASPECT_RATIO", config.out_r);
 			config.src_x = input.tag.get_property("SRC_X", config.src_x);
			config.src_y = input.tag.get_property("SRC_Y", config.src_y);
			config.src_w = input.tag.get_property("SRC_W", config.src_w);
			config.src_h = input.tag.get_property("SRC_H", config.src_h);
			config.dst_x = input.tag.get_property("DST_X", config.dst_x);
			config.dst_y = input.tag.get_property("DST_Y", config.dst_y);
			config.dst_w = input.tag.get_property("DST_W", config.dst_w);
			config.dst_h = input.tag.get_property("DST_H", config.dst_h);
		}
	}
}


#define EPSILON 0.001

int ScaleRatioMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	VFrame *input = input_ptr;
	VFrame *output = output_ptr;

	load_configuration();

	if(get_use_opengl()) return run_opengl();

//printf("ScaleRatioMain::process_realtime 1 %p\n", input);
	if( input->get_rows()[0] == output->get_rows()[0] ) {
		if( temp_frame && (
		    temp_frame->get_w() != input_ptr->get_w() ||
		    temp_frame->get_h() != input_ptr->get_h() ||
		    temp_frame->get_color_model() != input_ptr->get_color_model() ) ) {
			delete temp_frame;
			temp_frame = 0;
		}
		if(!temp_frame)
			temp_frame = new VFrame(input_ptr->get_w(), input_ptr->get_h(),
				input->get_color_model(), 0);
		temp_frame->copy_from(input);
		input = temp_frame;
	}
//printf("ScaleRatioMain::process_realtime 2 %p\n", input);


	if(!overlayer) {
		overlayer = new OverlayFrame(smp + 1);
	}

	output->clear_frame();

	if( config.src_w < EPSILON ) return 1;
	if( config.src_h < EPSILON ) return 1;
	if( config.dst_w < EPSILON ) return 1;
	if( config.dst_h < EPSILON ) return 1;
	float ix0 = (input->get_w() - config.src_w)/2 + config.src_x;
	float iy0 = (input->get_h() - config.src_h)/2 + config.src_y;
	float ix1 = ix0, ix2 = ix1 + config.src_w;
	float iy1 = iy0, iy2 = iy1 + config.src_h;
	float ox0 = (output->get_w() - config.dst_w)/2 + config.dst_x;
	float oy0 = (output->get_h() - config.dst_h)/2 + config.dst_y;
	float ox1 = ox0, ox2 = ox1 + config.dst_w;
	float oy1 = oy0, oy2 = oy1 + config.dst_h;

	overlayer->overlay(output, input,
		ix1, iy1, ix2, iy2,
		ox1, oy1, ox2, oy2,
		1, TRANSFER_REPLACE,
		get_interpolation_type());

	return 0;
}

NEW_WINDOW_MACRO(ScaleRatioMain, ScaleRatioWin)

void ScaleRatioMain::update_gui()
{
        if( !thread ) return;
	ScaleRatioWin *window = (ScaleRatioWin*)thread->get_window();
        window->lock_window("ScaleRatio::update_gui");
        if( load_configuration() )
                window->update_gui();
        window->unlock_window();
}


int ScaleRatioMain::handle_opengl()
{
#ifdef HAVE_GL
	VFrame *input = get_input(), *output = get_output();
	float ix1 = (input->get_w() - config.src_w)/2 + config.src_x;
	float iy1 = (input->get_h() - config.src_h)/2 + config.src_y;
	float ix2 = ix1 + config.src_w;
	float iy2 = iy1 + config.src_h;
	float ox1 = (output->get_w() - config.dst_w)/2 + config.dst_x;
	float oy1 = (output->get_h() - config.dst_h)/2 + config.dst_y;
	float ox2 = ox1 + config.dst_w;
	float oy2 = oy1 + config.dst_h;

	output->to_texture();
	output->enable_opengl();
	output->init_screen();
	output->clear_pbuffer();
	output->bind_texture(0);
	output->draw_texture(ix1,iy1, ix2,iy2, ox1,oy1, ox2,oy2);
	output->set_opengl_state(VFrame::SCREEN);
#endif
	return 0;
}


