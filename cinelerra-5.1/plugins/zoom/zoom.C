
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

#include "bchash.h"
#include "clip.h"
#include "filexml.h"
#include "zoom.h"
#include "edl.inc"
#include "overlayframe.h"
#include "language.h"
#include "theme.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN(ZoomMain)



#define MIN_MAGNIFICATION 1.0
#define MAX_MAGNIFICATION 100.0


ZoomLimit::ZoomLimit(ZoomMain *plugin,
	ZoomWindow *window,
	float *output,
	int x,
	int y,
	int w)
 : BC_TumbleTextBox(window,
 	*output,
	(float)MIN_MAGNIFICATION,
	(float)MAX_MAGNIFICATION,
	x,
	y,
	w)
{
	this->output = output;
	this->plugin = plugin;
}

int ZoomLimit::handle_event()
{
	*output = atof(get_text());
	plugin->send_configure_change();
	return 1;
}








ZoomWindow::ZoomWindow(ZoomMain *plugin)
 : PluginClientWindow(plugin,
	250,
	125,
	250,
	125,
	0)
{
	this->plugin = plugin;
}

ZoomWindow::~ZoomWindow()
{
}



void ZoomWindow::create_objects()
{
	BC_Title *title = 0;
	lock_window("ZoomWindow::create_objects");
	int widget_border = plugin->get_theme()->widget_border;
	int window_border = plugin->get_theme()->window_border;
	int x = window_border, y = window_border;

	add_subwindow(title = new BC_Title(x, y, _("X Magnification:")));
	y += title->get_h() + widget_border;
	limit_x = new ZoomLimit(plugin,
		this,
		&plugin->max_magnification_x,
		x,
		y,
		get_w() - window_border * 2 - widget_border - BC_Tumbler::calculate_w());
	limit_x->create_objects();
	y += limit_x->get_h() + widget_border;
	add_subwindow(title = new BC_Title(x, y, _("Y Magnification:")));
	y += title->get_h() + widget_border;
	limit_y = new ZoomLimit(plugin,
		this,
		&plugin->max_magnification_y,
		x,
		y,
		get_w() - window_border * 2 - widget_border - BC_Tumbler::calculate_w());
	limit_y->create_objects();

	show_window();
	unlock_window();
}









ZoomMain::ZoomMain(PluginServer *server)
 : PluginVClient(server)
{
	overlayer = 0;
	temp = 0;
	max_magnification_x = 10;
	max_magnification_y = 10;
}

ZoomMain::~ZoomMain()
{
	delete overlayer;
	delete temp;
}

const char* ZoomMain::plugin_title() { return N_("Zoom"); }
int ZoomMain::is_video() { return 1; }
int ZoomMain::is_transition() { return 1; }
int ZoomMain::uses_gui() { return 1; }





void ZoomMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("ZOOMTRANSITION");
	output.tag.set_property("MAGNIFICATION_X", max_magnification_x);
	output.tag.set_property("MAGNIFICATION_Y", max_magnification_y);
	output.append_tag();
	output.tag.set_title("/ZOOMTRANSITION");
	output.append_tag();
	output.terminate_string();
}

void ZoomMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	while(!input.read_tag())
	{
		if(input.tag.title_is("ZOOMTRANSITION"))
		{
			max_magnification_x = input.tag.get_property("MAGNIFICATION_X", max_magnification_x);
			max_magnification_y = input.tag.get_property("MAGNIFICATION_Y", max_magnification_y);
		}
	}
}

int ZoomMain::load_configuration()
{
	read_data(get_prev_keyframe(get_source_position()));
	return 1;
}


NEW_WINDOW_MACRO(ZoomMain, ZoomWindow);

int ZoomMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	int half = PluginClient::get_total_len() / 2;
	int position = half - labs(PluginClient::get_source_position() - half);
	float fraction = (float)position / half;
	is_before = PluginClient::get_source_position() < half;
	int w = incoming->get_w();
	int h = incoming->get_h();

	load_configuration();
	this->max_magnification_x = MAX(MIN_MAGNIFICATION, this->max_magnification_x);
	this->max_magnification_y = MAX(MIN_MAGNIFICATION, this->max_magnification_y);
	CLAMP(this->max_magnification_x, MIN_MAGNIFICATION, MAX_MAGNIFICATION);
	CLAMP(this->max_magnification_y, MIN_MAGNIFICATION, MAX_MAGNIFICATION);

	float min_magnification_x = MIN(1.0, this->max_magnification_x);
	float max_magnification_x = MAX(1.0, this->max_magnification_x);
	float min_magnification_y = MIN(1.0, this->max_magnification_y);
	float max_magnification_y = MAX(1.0, this->max_magnification_y);

	if(fraction > 0)
	{
		in_w = (float)w / ((float)fraction *
			(max_magnification_x - min_magnification_x) +
			min_magnification_x);
		in_x = (float)w / 2 - in_w / 2;
		in_h = (float)h / ((float)fraction *
			(max_magnification_y - min_magnification_y) +
			min_magnification_y);
		in_y = (float)h / 2 - in_h / 2;
	}
	else
	{
		in_w = w;
		in_h = h;
		in_x = 0;
		in_y = 0;
	}

// printf("ZoomMain::process_realtime %f %f %f %f\n",
// fraction,
// ((float)fraction *
// (max_magnification_x - min_magnification_x) +
// min_magnification_x),
// in_x,
// in_y,
// in_x + in_w,
// in_y + in_h);


// Use hardware
	if(get_use_opengl())
	{
		run_opengl();
		return 0;
	}

// Use software
	if(!overlayer) overlayer = new OverlayFrame(get_project_smp() + 1);

	if(is_before)
	{
		if(!temp) temp = new VFrame(outgoing->get_w(), outgoing->get_h(),
			outgoing->get_color_model(), 0);
		temp->clear_frame();
		overlayer->overlay(temp, outgoing,
			in_x, in_y, in_x + in_w, in_y + in_h,
			0, 0, temp->get_w(), temp->get_h(),
			1.0, TRANSFER_REPLACE, CUBIC_LINEAR);
		outgoing->copy_from(temp);
	}
	else
	{
		outgoing->clear_frame();
		overlayer->overlay(outgoing, incoming,
			in_x, in_y, in_x + in_w, in_y + in_h,
			0, 0, outgoing->get_w(), outgoing->get_h(),
			1.0, TRANSFER_REPLACE, CUBIC_LINEAR);
	}

	return 0;
}


int ZoomMain::handle_opengl()
{
#ifdef HAVE_GL

	if(is_before)
	{
// Read images from RAM
		get_output()->to_texture();

// Create output pbuffer
		get_output()->enable_opengl();
		VFrame::init_screen(get_output()->get_w(), get_output()->get_h());

// Enable output texture
		get_output()->bind_texture(0);
// Draw output texture
// printf("ZoomMain::handle_opengl %f %f %f %f\n",
// in_x,
// in_y,
// in_x + in_w,
// in_y + in_h);
		get_output()->draw_texture(in_x,
			in_y,
			in_x + in_w,
			in_y + in_h,
			0,
			0,
			get_output()->get_w(),
			get_output()->get_h());
	}
	else
	{
// Read images from RAM
		get_input()->to_texture();

// Create output pbuffer
		get_output()->enable_opengl();
		VFrame::init_screen(get_output()->get_w(), get_output()->get_h());

// Enable output texture
		get_input()->bind_texture(0);
// Draw input texture on output pbuffer
		get_output()->draw_texture(in_x,
			in_y,
			in_x + in_w,
			in_y + in_h,
			0,
			0,
			get_output()->get_w(),
			get_output()->get_h());
	}

	get_output()->set_opengl_state(VFrame::SCREEN);

	return 1;

#endif

	return 0;
}


