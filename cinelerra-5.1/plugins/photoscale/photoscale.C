
/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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

#include "bcdisplayinfo.h"
#include "filexml.h"
#include "photoscale.h"
#include "bchash.h"
#include "keyframe.h"
#include "language.h"
#include "new.h"
#include "theme.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>

#define MAXBORDER 50




PhotoScaleWindow::PhotoScaleWindow(PhotoScaleMain *plugin)
 : PluginClientWindow(plugin,
	250,
	200,
	250,
	200,
	0)
{
	this->plugin = plugin;
}

PhotoScaleWindow::~PhotoScaleWindow()
{
}

void PhotoScaleWindow::create_objects()
{
	int x = 10, y = 10;
	BC_Title *title;

	int x2 = x + BC_Title::calculate_w(this, _("Height:")) + plugin->get_theme()->widget_border;
	add_subwindow(title = new BC_Title(x,
		y,
		_("Output size:")));

	y += title->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(title = new BC_Title(x, y, _("Width:")));
	add_subwindow(output_size[0] = new PhotoScaleSizeText(
		plugin,
		this,
		x2,
		y,
		100,
		&(plugin->config.width)));

	y += output_size[0]->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(title = new BC_Title(x, y, _("Height:")));
	add_subwindow(output_size[1] = new PhotoScaleSizeText(
		plugin,
		this,
		x2,
		y,
		100,
		&(plugin->config.height)));

	FrameSizePulldown *pulldown;
	add_subwindow(pulldown = new FrameSizePulldown(plugin->get_theme(),
		output_size[0],
		output_size[1],
		x + title->get_w() + output_size[1]->get_w() + plugin->get_theme()->widget_border,
		y));

	add_subwindow(new PhotoScaleSwapExtents(
		plugin,
		this,
		x + title->get_w() + output_size[1]->get_w() + plugin->get_theme()->widget_border + pulldown->get_w(),
		y));

	y += pulldown->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(file = new PhotoScaleFile(plugin,
		this,
		x,
		y));

	y += file->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(scan = new PhotoScaleScan(plugin,
		this,
		x,
		y));

	show_window();
	flush();
}



PhotoScaleSizeText::PhotoScaleSizeText(PhotoScaleMain *plugin,
	PhotoScaleWindow *gui,
	int x,
	int y,
	int w,
	int *output)
 : BC_TextBox(x, y, w, 1, *output)
{
	this->plugin = plugin;
	this->gui = gui;
	this->output = output;
}

PhotoScaleSizeText::~PhotoScaleSizeText()
{
}

int PhotoScaleSizeText::handle_event()
{
	*output = atol(get_text());
	*output /= 2;
	*output *= 2;
	if(*output <= 0) *output = 2;
	if(*output > 10000) *output = 10000;
	plugin->send_configure_change();
	return 1;
}



PhotoScaleFile::PhotoScaleFile(PhotoScaleMain *plugin,
	PhotoScaleWindow *gui,
	int x,
	int y)
 : BC_Radial(x, y, plugin->config.use_file, _("Override camera"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int PhotoScaleFile::handle_event()
{
	plugin->config.use_file = 1;
	gui->scan->update(0);
	plugin->send_configure_change();
	return 1;
}


PhotoScaleScan::PhotoScaleScan(PhotoScaleMain *plugin,
	PhotoScaleWindow *gui,
	int x,
	int y)
 : BC_Radial(x, y, !plugin->config.use_file, _("Use alpha/black level"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int PhotoScaleScan::handle_event()
{
	plugin->config.use_file = 0;
	gui->file->update(0);
	plugin->send_configure_change();
	return 1;
}








PhotoScaleSwapExtents::PhotoScaleSwapExtents(PhotoScaleMain *plugin,
	PhotoScaleWindow *gui,
	int x,
	int y)
 : BC_Button(x, y, plugin->get_theme()->get_image_set("swap_extents"))
{
	this->plugin = plugin;
	this->gui = gui;
	set_tooltip(_("Swap dimensions"));
}

int PhotoScaleSwapExtents::handle_event()
{
	int w = plugin->config.width;
	int h = plugin->config.height;
	gui->output_size[0]->update((int64_t)h);
	gui->output_size[1]->update((int64_t)w);
	plugin->config.width = h;
	plugin->config.height = w;
	plugin->send_configure_change();
	return 1;
}




















PhotoScaleConfig::PhotoScaleConfig()
{
	width = 640;
	height = 480;
	use_file = 1;
}

int PhotoScaleConfig::equivalent(PhotoScaleConfig &that)
{
	return (width == that.width &&
		height == that.height &&
		use_file == that.use_file);
}

void PhotoScaleConfig::copy_from(PhotoScaleConfig &that)
{
	width = that.width;
	height = that.height;
	use_file = that.use_file;
}

void PhotoScaleConfig::interpolate(PhotoScaleConfig &prev,
	PhotoScaleConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	copy_from(next);
}






REGISTER_PLUGIN(PhotoScaleMain)








PhotoScaleMain::PhotoScaleMain(PluginServer *server)
 : PluginVClient(server)
{
	need_reconfigure = 1;
	overlayer = 0;
	engine = 0;
}

PhotoScaleMain::~PhotoScaleMain()
{
//printf("PhotoScaleMain::~PhotoScaleMain 1\n");
	if(overlayer) delete overlayer;
	if(engine) delete engine;
}

const char* PhotoScaleMain::plugin_title() { return N_("Auto Scale"); }
int PhotoScaleMain::is_realtime() { return 1; }


NEW_WINDOW_MACRO(PhotoScaleMain, PhotoScaleWindow)

LOAD_CONFIGURATION_MACRO(PhotoScaleMain, PhotoScaleConfig)




int PhotoScaleMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	if(config.use_file)
	{
		frame->get_params()->update("AUTOSCALE", 1);
		frame->get_params()->update("AUTOSCALE_W", config.width);
		frame->get_params()->update("AUTOSCALE_H", config.height);
		read_frame(frame,
			0,
			start_position,
			frame_rate,
			get_use_opengl());
		return 0;
	}
	else
	{
		frame->get_params()->update("AUTOSCALE", 0);
	}


	read_frame(frame,
		0,
		start_position,
		frame_rate,
		0);

	if(!engine) engine = new PhotoScaleEngine(this,
		PluginClient::get_project_smp() + 1);
	engine->process_packages();

// 	printf("PhotoScaleMain::process_buffer %d %d %d %d %d\n",
// 		__LINE__,
// 		engine->top_border,
// 		engine->bottom_border,
// 		engine->left_border,
// 		engine->right_border);

	int in_width = engine->right_border - engine->left_border;
	int in_height = engine->bottom_border - engine->top_border;
	if(in_width > 0 &&
		in_height > 0 &&
		(engine->top_border > 0 ||
		engine->left_border > 0 ||
		engine->bottom_border < frame->get_h() ||
		engine->right_border < frame->get_w()))
	{
		VFrame *temp_frame = new_temp(frame->get_w(),
			frame->get_h(),
			frame->get_color_model());
		temp_frame->copy_from(frame);
		if(!overlayer)
		{
			overlayer = new OverlayFrame(PluginClient::get_project_smp() + 1);
		}

		float scale1 = (float)config.width / in_width;
		float scale2 = (float)config.height / in_height;
		float out_x1 = 0;
		float out_y1 = 0;
		float out_x2 = 0;
		float out_y2 = 0;

// printf("PhotoScaleMain::process_buffer %d %d %d %d %d\n",
// __LINE__,
// engine->left_border,
// engine->top_border,
// engine->right_border,
// engine->bottom_border);
// printf("PhotoScaleMain::process_buffer %d %f %f\n", __LINE__, scale1, scale2);
		if(scale1 < scale2)
		{
			out_x1 = (float)frame->get_w() / 2 - config.width / 2;
			out_y1 = (float)frame->get_h() / 2 - in_height * scale1 / 2;
			out_x2 = (float)frame->get_w() / 2 + config.width / 2;
			out_y2 = (float)frame->get_h() / 2 + in_height * scale1 / 2;
		}
		else
		{
			out_x1 = (float)frame->get_w() / 2 - in_width * scale2 / 2;
			out_y1 = (float)frame->get_h() / 2 - config.height / 2;
			out_x2 = (float)frame->get_w() / 2 + in_width * scale2 / 2;
			out_y2 = (float)frame->get_h() / 2 + config.height / 2;
		}

// printf("PhotoScaleMain::process_buffer %d %d %d %d %d\n",
// __LINE__,
// (int)out_x1,
// (int)out_y1,
// (int)out_x2,
// (int)out_y2);
		frame->clear_frame();
		overlayer->overlay(frame,
			temp_frame,
			(float)engine->left_border,
			(float)engine->top_border,
			(float)engine->right_border,
			(float)engine->bottom_border,
			(float)out_x1,
			(float)out_y1,
			(float)out_x2,
			(float)out_y2,
			1,
			TRANSFER_REPLACE,
			get_interpolation_type());

	}

	return 0;
}


void PhotoScaleMain::update_gui()
{
	if(thread)
	{
		int reconfigure = load_configuration();
		if(reconfigure)
		{
			PhotoScaleWindow *window = (PhotoScaleWindow*)thread->window;
			window->lock_window("PhotoScaleMain::update_gui");
			window->output_size[0]->update((int64_t)config.width);
			window->output_size[1]->update((int64_t)config.height);
			window->file->update(config.use_file);
			window->scan->update(!config.use_file);
			window->unlock_window();
		}
	}
}



void PhotoScaleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("PHOTOSCALE");
	output.tag.set_property("WIDTH", config.width);
	output.tag.set_property("HEIGHT", config.height);
	output.tag.set_property("USE_FILE", config.use_file);
	output.append_tag();
	output.tag.set_title("/PHOTOSCALE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void PhotoScaleMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("PHOTOSCALE"))
			{
				config.width = input.tag.get_property("WIDTH", config.width);
				config.height = input.tag.get_property("HEIGHT", config.height);
				config.use_file = input.tag.get_property("USE_FILE", config.use_file);
			}
		}
	}
}





PhotoScaleEngine::PhotoScaleEngine(PhotoScaleMain *plugin, int cpus)
 : LoadServer(cpus, 4)
{
	this->plugin = plugin;
}

PhotoScaleEngine::~PhotoScaleEngine()
{
}

void PhotoScaleEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		PhotoScalePackage *package = (PhotoScalePackage*)get_package(i);
		package->side = i;
	}

	top_border = 0;
	bottom_border = 0;
	left_border = 0;
	right_border = 0;
}

LoadClient* PhotoScaleEngine::new_client()
{
	return new PhotoScaleUnit(this);
}

LoadPackage* PhotoScaleEngine::new_package()
{
	return new PhotoScalePackage;
}







PhotoScalePackage::PhotoScalePackage()
 : LoadPackage()
{
	side = 0;
}








PhotoScaleUnit::PhotoScaleUnit(PhotoScaleEngine *server)
 : LoadClient(server)
{
	this->server = server;
}

PhotoScaleUnit::~PhotoScaleUnit()
{
}

#define TEST_PIXEL(components, is_yuv) \
	(components == 4 && pixel[3] > 0) || \
	(components == 3 && is_yuv && pixel[0] > 0) || \
	(components == 3 && !is_yuv && (pixel[0] > 0 || pixel[1] > 0 || pixel[2] > 0))

#define SCAN_BORDER(type, components, is_yuv) \
{ \
	type **rows = (type**)input->get_rows(); \
	switch(pkg->side) \
	{ \
/* top */ \
		case 0: \
			for(int i = 0; i < h / 2; i++) \
			{ \
				type *row = rows[i]; \
				for(int j = 0; j < w; j++) \
				{ \
					type *pixel = row + j * components; \
					if(TEST_PIXEL(components, is_yuv)) \
					{ \
						server->top_border = i; \
						j = w; \
						i = h; \
					} \
				} \
			} \
			break; \
 \
/* bottom */ \
		case 1: \
			for(int i = h - 1; i >= h / 2; i--) \
			{ \
				type *row = rows[i]; \
				for(int j = 0; j < w; j++) \
				{ \
					type *pixel = row + j * components; \
					if(TEST_PIXEL(components, is_yuv)) \
					{ \
						server->bottom_border = i + 1; \
						j = w; \
						i = 0; \
					} \
				} \
			} \
			break; \
 \
/* left */ \
		case 2: \
			for(int i = 0; i < w / 2; i++) \
			{ \
				for(int j = 0; j < h; j++) \
				{ \
					type *row = rows[j]; \
					type *pixel = row + i * components; \
					if(TEST_PIXEL(components, is_yuv)) \
					{ \
						server->left_border = i; \
						j = h; \
						i = w; \
					} \
				} \
			} \
 \
/* right */ \
		case 3: \
			for(int i = w - 1; i >= w / 2; i--) \
			{ \
				for(int j = 0; j < h; j++) \
				{ \
					type *row = rows[j]; \
					type *pixel = row + i * components; \
					if(TEST_PIXEL(components, is_yuv)) \
					{ \
						server->right_border = i + 1; \
						j = h; \
						i = 0; \
					} \
				} \
			} \
	} \
}

void PhotoScaleUnit::process_package(LoadPackage *package)
{
	PhotoScalePackage *pkg = (PhotoScalePackage*)package;
	VFrame *input = server->plugin->get_input();
	int w = input->get_w();
	int h = input->get_h();


	switch(input->get_color_model())
	{
		case BC_RGB_FLOAT:
			SCAN_BORDER(float, 3, 0);
			break;
		case BC_RGBA_FLOAT:
			SCAN_BORDER(float, 4, 0);
			break;
		case BC_RGB888:
			SCAN_BORDER(unsigned char, 3, 0);
			break;
		case BC_YUV888:
			SCAN_BORDER(unsigned char, 3, 1);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			SCAN_BORDER(unsigned char, 4, 0);
			break;
	}
}













