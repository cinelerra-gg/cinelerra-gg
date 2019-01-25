
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
#include "translate.h"
#include "translatewin.h"

#include <string.h>




REGISTER_PLUGIN(TranslateMain)

TranslateConfig::TranslateConfig()
{
	reset();
}

void TranslateConfig::reset()
{
	in_x = 0;
	in_y = 0;
	in_w = 720;
	in_h = 480;
	out_x = 0;
	out_y = 0;
	out_w = 720;
	out_h = 480;
}


int TranslateConfig::equivalent(TranslateConfig &that)
{
	return EQUIV(in_x, that.in_x) &&
		EQUIV(in_y, that.in_y) &&
		EQUIV(in_w, that.in_w) &&
		EQUIV(in_h, that.in_h) &&
		EQUIV(out_x, that.out_x) &&
		EQUIV(out_y, that.out_y) &&
		EQUIV(out_w, that.out_w) &&
		EQUIV(out_h, that.out_h);
}

void TranslateConfig::copy_from(TranslateConfig &that)
{
	in_x = that.in_x;
	in_y = that.in_y;
	in_w = that.in_w;
	in_h = that.in_h;
	out_x = that.out_x;
	out_y = that.out_y;
	out_w = that.out_w;
	out_h = that.out_h;
}

void TranslateConfig::interpolate(TranslateConfig &prev,
	TranslateConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->in_x = prev.in_x * prev_scale + next.in_x * next_scale;
	this->in_y = prev.in_y * prev_scale + next.in_y * next_scale;
	this->in_w = prev.in_w * prev_scale + next.in_w * next_scale;
	this->in_h = prev.in_h * prev_scale + next.in_h * next_scale;
	this->out_x = prev.out_x * prev_scale + next.out_x * next_scale;
	this->out_y = prev.out_y * prev_scale + next.out_y * next_scale;
	this->out_w = prev.out_w * prev_scale + next.out_w * next_scale;
	this->out_h = prev.out_h * prev_scale + next.out_h * next_scale;
}








TranslateMain::TranslateMain(PluginServer *server)
 : PluginVClient(server)
{
	temp_frame = 0;
	overlayer = 0;

}

TranslateMain::~TranslateMain()
{


	if(temp_frame) delete temp_frame;
	temp_frame = 0;
	if(overlayer) delete overlayer;
	overlayer = 0;
}

const char* TranslateMain::plugin_title() { return N_("Translate"); }
int TranslateMain::is_realtime() { return 1; }



LOAD_CONFIGURATION_MACRO(TranslateMain, TranslateConfig)

void TranslateMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);

// Store data
	output.tag.set_title("TRANSLATE");
	output.tag.set_property("IN_X", config.in_x);
	output.tag.set_property("IN_Y", config.in_y);
	output.tag.set_property("IN_W", config.in_w);
	output.tag.set_property("IN_H", config.in_h);
	output.tag.set_property("OUT_X", config.out_x);
	output.tag.set_property("OUT_Y", config.out_y);
	output.tag.set_property("OUT_W", config.out_w);
	output.tag.set_property("OUT_H", config.out_h);
	output.append_tag();
	output.tag.set_title("/TRANSLATE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
// data is now in *text
}

void TranslateMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("TRANSLATE"))
			{
 				config.in_x = input.tag.get_property("IN_X", config.in_x);
				config.in_y = input.tag.get_property("IN_Y", config.in_y);
				config.in_w = input.tag.get_property("IN_W", config.in_w);
				config.in_h = input.tag.get_property("IN_H", config.in_h);
				config.out_x =	input.tag.get_property("OUT_X", config.out_x);
				config.out_y =	input.tag.get_property("OUT_Y", config.out_y);
				config.out_w =	input.tag.get_property("OUT_W", config.out_w);
				config.out_h =	input.tag.get_property("OUT_H", config.out_h);
			}
		}
	}
}


#define EPSILON 0.001

int TranslateMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	VFrame *input = input_ptr;
	VFrame *output = output_ptr;

	load_configuration();

//printf("TranslateMain::process_realtime 1 %p\n", input);
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
//printf("TranslateMain::process_realtime 2 %p\n", input);


	if(!overlayer)
	{
		overlayer = new OverlayFrame(smp + 1);
	}

	output->clear_frame();

	if( config.in_w < EPSILON ) return 1;
	if( config.in_h < EPSILON ) return 1;
	if( config.out_w < EPSILON ) return 1;
	if( config.out_h < EPSILON ) return 1;

	float ix1 = config.in_x, ox1 = config.out_x;
	float ix2 = ix1 + config.in_w;

	if( ix1 < 0 ) {
		ox1 -= ix1;
		ix2 = config.in_w;
		ix1 = 0;
	}

	if(ix2 > output->get_w())
		ix2 = output->get_w();

	float iy1 = config.in_y, oy1 = config.out_y;
	float iy2 = iy1 + config.in_h;

	if( iy1 < 0 ) {
		oy1 -= iy1;
		iy2 = config.in_h;
		iy1 = 0;
	}

	if( iy2 > output->get_h() )
		iy2 = output->get_h();

	float cx = config.out_w / config.in_w;
	float cy = config.out_h / config.in_h;

	float ox2 = ox1 + (ix2 - ix1) * cx;
	float oy2 = oy1 + (iy2 - iy1) * cy;

	if( ox1 < 0 ) {
		ix1 += -ox1 / cx;
		ox1 = 0;
	}
	if( oy1 < 0 ) {
		iy1 += -oy1 / cy;
		oy1 = 0;
	}
	if( ox2 > output->get_w() ) {
		ix2 -= (ox2 - output->get_w()) / cx;
		ox2 = output->get_w();
	}
	if( oy2 > output->get_h() ) {
		iy2 -= (oy2 - output->get_h()) / cy;
		oy2 = output->get_h();
	}

	if( ix1 >= ix2 ) return 1;
	if( iy1 >= iy2 ) return 1;
	if( ox1 >= ox2 ) return 1;
	if( oy1 >= oy2 ) return 1;

	overlayer->overlay(output, input,
		ix1, iy1, ix2, iy2,
		ox1, oy1, ox2, oy2,
		1, TRANSFER_REPLACE,
		get_interpolation_type());
	return 0;
}

NEW_WINDOW_MACRO(TranslateMain, TranslateWin)

void TranslateMain::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;

	TranslateWin *window = (TranslateWin*)thread->window;
	window->lock_window();
	window->in_x->update(config.in_x);
	window->in_y->update(config.in_y);
	window->in_w->update(config.in_w);
	window->in_h->update(config.in_h);
	window->out_x->update(config.out_x);
	window->out_y->update(config.out_y);
	window->out_w->update(config.out_w);
	window->out_h->update(config.out_h);
	window->unlock_window();
}
