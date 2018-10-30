
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "bcdisplayinfo.h"
#include "bccolors.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "loadbalance.h"
#include "bccolors.h"
#include "pluginvclient.h"
#include "fonts.h"
#include "scopewindow.h"
#include "theme.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>





class VideoScopeEffect;
class VideoScopeEngine;
class VideoScopeWindow;

class VideoScopeConfig
{
public:
	VideoScopeConfig();
};

class VideoScopeWindow : public ScopeGUI
{
public:
	VideoScopeWindow(VideoScopeEffect *plugin);
	~VideoScopeWindow();

	void create_objects();
	void toggle_event();
	int resize_event(int w, int h);
	void update();

	VideoScopeEffect *plugin;
};





class VideoScopePackage : public LoadPackage
{
public:
	VideoScopePackage();
	int row1, row2;
};


class VideoScopeUnit : public LoadClient
{
public:
	VideoScopeUnit(VideoScopeEffect *plugin, VideoScopeEngine *server);
	void process_package(LoadPackage *package);
	VideoScopeEffect *plugin;
};

class VideoScopeEngine : public LoadServer
{
public:
	VideoScopeEngine(VideoScopeEffect *plugin, int cpus);
	~VideoScopeEngine();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	VideoScopeEffect *plugin;
};

class VideoScopeEffect : public PluginVClient
{
public:
	VideoScopeEffect(PluginServer *server);
	~VideoScopeEffect();


	PLUGIN_CLASS_MEMBERS2(VideoScopeConfig)
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	void render_gui(void *input);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	int use_hist, use_wave, use_vector;
	int use_hist_parade, use_wave_parade;
	int w, h;
	VFrame *input;
};














VideoScopeConfig::VideoScopeConfig()
{
}












VideoScopeWindow::VideoScopeWindow(VideoScopeEffect *plugin)
 : ScopeGUI(plugin,
	plugin->w,
	plugin->h)
{
	this->plugin = plugin;
}

VideoScopeWindow::~VideoScopeWindow()
{
}

void VideoScopeWindow::create_objects()
{
	use_hist = plugin->use_hist;
	use_wave = plugin->use_wave;
	use_vector = plugin->use_vector;
	use_hist_parade = plugin->use_hist_parade;
	use_wave_parade = plugin->use_wave_parade;
	ScopeGUI::create_objects();
}

void VideoScopeWindow::toggle_event()
{
	plugin->use_hist = use_hist;
	plugin->use_wave = use_wave;
	plugin->use_vector = use_vector;
	plugin->use_hist_parade = use_hist_parade;
	plugin->use_wave_parade = use_wave_parade;
// Make it reprocess
	plugin->send_configure_change();
}


int VideoScopeWindow::resize_event(int w, int h)
{
	ScopeGUI::resize_event(w, h);
	plugin->w = w;
	plugin->h = h;
// Make it reprocess
	plugin->send_configure_change();
	return 1;
}




















REGISTER_PLUGIN(VideoScopeEffect)






VideoScopeEffect::VideoScopeEffect(PluginServer *server)
 : PluginVClient(server)
{
	w = MIN_SCOPE_W;
	h = MIN_SCOPE_H;
	use_hist = 0;
	use_wave = 0;
	use_vector = 1;
	use_hist_parade = 1;
	use_wave_parade = 1;
}

VideoScopeEffect::~VideoScopeEffect()
{


}



const char* VideoScopeEffect::plugin_title() { return N_("VideoScope"); }
int VideoScopeEffect::is_realtime() { return 1; }

int VideoScopeEffect::load_configuration()
{
	return 0;
}

void VideoScopeEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("VIDEOSCOPE");


	if(is_defaults())
	{
		output.tag.set_property("W", w);
		output.tag.set_property("H", h);
		output.tag.set_property("USE_HIST", use_hist);
		output.tag.set_property("USE_WAVE", use_wave);
		output.tag.set_property("USE_VECTOR", use_vector);
		output.tag.set_property("USE_HIST_PARADE", use_hist_parade);
		output.tag.set_property("USE_WAVE_PARADE", use_wave_parade);
	}

	output.append_tag();
	output.tag.set_title("/VIDEOSCOPE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void VideoScopeEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;


	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("VIDEOSCOPE"))
			{
				if(is_defaults())
				{
					w = input.tag.get_property("W", w);
					h = input.tag.get_property("H", h);
					use_hist = input.tag.get_property("USE_HIST", use_hist);
					use_wave = input.tag.get_property("USE_WAVE", use_wave);
					use_vector = input.tag.get_property("USE_VECTOR", use_vector);
					use_hist_parade = input.tag.get_property("USE_HIST_PARADE", use_hist_parade);
					use_wave_parade = input.tag.get_property("USE_WAVE_PARADE", use_wave_parade);
				}
			}
		}
	}
}




NEW_WINDOW_MACRO(VideoScopeEffect, VideoScopeWindow)


int VideoScopeEffect::process_realtime(VFrame *input, VFrame *output)
{

	send_render_gui(input);
//printf("VideoScopeEffect::process_realtime 1\n");
	if(input->get_rows()[0] != output->get_rows()[0])
		output->copy_from(input);
	return 1;
}

void VideoScopeEffect::render_gui(void *input)
{
	if(thread)
	{
		VideoScopeWindow *window = ((VideoScopeWindow*)thread->window);
		window->lock_window();

//printf("VideoScopeEffect::process_realtime 1\n");
		this->input = (VFrame*)input;
//printf("VideoScopeEffect::process_realtime 1\n");
		window->process(this->input);


		window->unlock_window();
	}
}





VideoScopePackage::VideoScopePackage()
 : LoadPackage()
{
}






VideoScopeUnit::VideoScopeUnit(VideoScopeEffect *plugin,
	VideoScopeEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}


#define INTENSITY(p) ((unsigned int)(((p)[0]) * 77+ \
									((p)[1] * 150) + \
									((p)[2] * 29)) >> 8)


static void draw_point(unsigned char **rows,
	int color_model,
	int x,
	int y,
	int r,
	int g,
	int b)
{
	switch(color_model)
	{
		case BC_BGR8888:
		{
			unsigned char *pixel = rows[y] + x * 4;
			pixel[0] = b;
			pixel[1] = g;
			pixel[2] = r;
			break;
		}
		case BC_BGR888:
			break;
		case BC_RGB565:
		{
			unsigned char *pixel = rows[y] + x * 2;
			pixel[0] = (r & 0xf8) | (g >> 5);
			pixel[1] = ((g & 0xfc) << 5) | (b >> 3);
			break;
		}
		case BC_BGR565:
			break;
		case BC_RGB8:
			break;
	}
}



#define VIDEOSCOPE(type, temp_type, max, components, use_yuv) \
{ \
	for(int i = pkg->row1; i < pkg->row2; i++) \
	{ \
		type *in_row = (type*)plugin->input->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			type *in_pixel = in_row + j * components; \
			float intensity; \
 \
/* Analyze pixel */ \
			if(use_yuv) intensity = (float)*in_pixel / max; \
 \
			float h, s, v; \
			temp_type r, g, b; \
			if(use_yuv) \
			{ \
				if(sizeof(type) == 2) \
				{ \
					YUV::yuv.yuv_to_rgb_16(r, g, b, \
						in_pixel[0], in_pixel[1], in_pixel[2]); \
				} \
				else \
				{ \
					YUV::yuv.yuv_to_rgb_8(r, g, b, \
						in_pixel[0], in_pixel[1], in_pixel[2]); \
				} \
			} \
			else \
			{ \
				r = in_pixel[0]; \
				g = in_pixel[1]; \
				b = in_pixel[2]; \
			} \
 \
			HSV::rgb_to_hsv((float)r / max, \
					(float)g / max, \
					(float)b / max, \
					h, \
					s, \
					v); \
 \
/* Calculate waveform */ \
			if(parade) \
			{ \
/* red */ \
				int x = j * waveform_w / w / 3; \
				int y = waveform_h - (int)(((float)r / max - FLOAT_MIN) / \
					(FLOAT_MAX - FLOAT_MIN) * \
					waveform_h); \
				if(x >= 0 && x < waveform_w / 3 && y >= 0 && y < waveform_h) \
					draw_point(waveform_rows, \
						waveform_cmodel, \
						x, \
						y, \
						0xff, \
						0x0, \
						0x0); \
 \
/* green */ \
				x = waveform_w / 3 + j * waveform_w / w / 3; \
				y = waveform_h - (int)(((float)g / max - FLOAT_MIN) / \
					(FLOAT_MAX - FLOAT_MIN) * \
					waveform_h); \
				if(x >= waveform_w / 3 && x < waveform_w * 2 / 3 && \
					y >= 0 && y < waveform_h) \
					draw_point(waveform_rows, \
						waveform_cmodel, \
						x, \
						y, \
						0x0, \
						0xff, \
						0x0); \
 \
/* blue */ \
				x = waveform_w * 2 / 3 + j * waveform_w / w / 3; \
				y = waveform_h - (int)(((float)b / max - FLOAT_MIN) / \
					(FLOAT_MAX - FLOAT_MIN) * \
					waveform_h); \
				if(x >= waveform_w * 2 / 3 && x < waveform_w && \
					y >= 0 && y < waveform_h) \
					draw_point(waveform_rows, \
						waveform_cmodel, \
						x, \
						y, \
						0x0, \
						0x0, \
						0xff); \
			} \
			else \
			{ \
				if(!use_yuv) intensity = v; \
				intensity = (intensity - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN) * \
					waveform_h; \
				int y = waveform_h - (int)intensity; \
				int x = j * waveform_w / w; \
				if(x >= 0 && x < waveform_w && y >= 0 && y < waveform_h) \
					draw_point(waveform_rows, \
						waveform_cmodel, \
						x, \
						y, \
						0xff, \
						0xff, \
						0xff); \
			} \
 \
/* Calculate vectorscope */ \
			float adjacent = cos((h + 90) / 360 * 2 * M_PI); \
			float opposite = sin((h + 90) / 360 * 2 * M_PI); \
			int x = (int)(vector_w / 2 +  \
				adjacent * (s - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN) * radius); \
 \
			int y = (int)(vector_h / 2 -  \
				opposite * (s - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN) * radius); \
 \
 \
			CLAMP(x, 0, vector_w - 1); \
			CLAMP(y, 0, vector_h - 1); \
/* Get color with full saturation & value */ \
			float r_f, g_f, b_f; \
			HSV::hsv_to_rgb(r_f, \
					g_f, \
					b_f, \
					h, \
					s, \
					1); \
			r = (int)(r_f * 255); \
			g = (int)(g_f * 255); \
			b = (int)(b_f * 255); \
 \
 /* float */ \
			if(sizeof(type) == 4) \
			{ \
				r = CLIP(r, 0, 0xff); \
				g = CLIP(g, 0, 0xff); \
				b = CLIP(b, 0, 0xff); \
			} \
 \
			draw_point(vector_rows, \
				vector_cmodel, \
				x, \
				y, \
				(int)r, \
				(int)g, \
				(int)b); \
 \
		} \
	} \
}

void VideoScopeUnit::process_package(LoadPackage *package)
{
	VideoScopeWindow *window = (VideoScopeWindow*)plugin->thread->window;
	VideoScopePackage *pkg = (VideoScopePackage*)package;
	int w = plugin->input->get_w();
//	int h = plugin->input->get_h();
	int waveform_h = window->wave_h;
	int waveform_w = window->wave_w;
	int waveform_cmodel = window->waveform_bitmap->get_color_model();
	unsigned char **waveform_rows = window->waveform_bitmap->get_row_pointers();
	int vector_h = window->vector_bitmap->get_h();
	int vector_w = window->vector_bitmap->get_w();
	int vector_cmodel = window->vector_bitmap->get_color_model();
	unsigned char **vector_rows = window->vector_bitmap->get_row_pointers();
	float radius = MIN(vector_w / 2, vector_h / 2);
	int parade = 1;

	switch(plugin->input->get_color_model())
	{
		case BC_RGB888:
			VIDEOSCOPE(unsigned char, int, 0xff, 3, 0)
			break;

		case BC_RGB_FLOAT:
			VIDEOSCOPE(float, float, 1, 3, 0)
			break;

		case BC_YUV888:
			VIDEOSCOPE(unsigned char, int, 0xff, 3, 1)
			break;

		case BC_RGB161616:
			VIDEOSCOPE(uint16_t, int, 0xffff, 3, 0)
			break;

		case BC_YUV161616:
			VIDEOSCOPE(uint16_t, int, 0xffff, 3, 1)
			break;

		case BC_RGBA8888:
			VIDEOSCOPE(unsigned char, int, 0xff, 4, 0)
			break;

		case BC_RGBA_FLOAT:
			VIDEOSCOPE(float, float, 1, 4, 0)
			break;

		case BC_YUVA8888:
			VIDEOSCOPE(unsigned char, int, 0xff, 4, 1)
			break;

		case BC_RGBA16161616:
			VIDEOSCOPE(uint16_t, int, 0xffff, 4, 0)
			break;

		case BC_YUVA16161616:
			VIDEOSCOPE(uint16_t, int, 0xffff, 4, 1)
			break;
	}
}






VideoScopeEngine::VideoScopeEngine(VideoScopeEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

VideoScopeEngine::~VideoScopeEngine()
{
}

void VideoScopeEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		VideoScopePackage *pkg = (VideoScopePackage*)get_package(i);
		pkg->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}


LoadClient* VideoScopeEngine::new_client()
{
	return new VideoScopeUnit(plugin, this);
}

LoadPackage* VideoScopeEngine::new_package()
{
	return new VideoScopePackage;
}

