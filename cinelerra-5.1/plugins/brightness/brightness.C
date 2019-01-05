
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

#include "brightness.h"
#include "bccolors.h"
#include "bchash.h"
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "playback3d.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>


REGISTER_PLUGIN(BrightnessMain)


BrightnessConfig::BrightnessConfig()
{
	reset();
}

void BrightnessConfig::reset()

{
	brightness = 0;
	contrast = 0;
	luma = 1;
}

int BrightnessConfig::equivalent(BrightnessConfig &that)
{
	return (brightness == that.brightness &&
		contrast == that.contrast &&
		luma == that.luma);
}

void BrightnessConfig::copy_from(BrightnessConfig &that)
{
	brightness = that.brightness;
	contrast = that.contrast;
	luma = that.luma;
}

void BrightnessConfig::interpolate(BrightnessConfig &prev,
	BrightnessConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->brightness = prev.brightness * prev_scale + next.brightness * next_scale;
	this->contrast = prev.contrast * prev_scale + next.contrast * next_scale;
	this->luma = (int)(prev.luma * prev_scale + next.luma * next_scale);
}









BrightnessMain::BrightnessMain(PluginServer *server)
 : PluginVClient(server)
{
    redo_buffers = 1;
	engine = 0;

}

BrightnessMain::~BrightnessMain()
{

	if(engine) delete engine;
}

const char* BrightnessMain::plugin_title() { return N_("Brightness/Contrast"); }
int BrightnessMain::is_realtime() { return 1; }

NEW_WINDOW_MACRO(BrightnessMain, BrightnessWindow)
LOAD_CONFIGURATION_MACRO(BrightnessMain, BrightnessConfig)

int BrightnessMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	read_frame(frame,
		0,
		start_position,
		frame_rate,
		get_use_opengl());


// Use hardware
	if(get_use_opengl())
	{
		run_opengl();
		return 0;
	}




	if(!engine) engine = new BrightnessEngine(this, PluginClient::smp + 1);

	this->input = frame;
	this->output = frame;

	if(!EQUIV(config.brightness, 0) || !EQUIV(config.contrast, 0))
	{
		engine->process_packages();
	}

	return 0;
}

int BrightnessMain::handle_opengl()
{
#ifdef HAVE_GL
	static const char *brightness_yuvluma_frag =
		"uniform sampler2D tex;\n"
		"uniform float brightness;\n"
		"uniform float contrast;\n"
		"uniform float offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 yuva = texture2D(tex, gl_TexCoord[0].st);\n"
		"	yuva.r += brightness;\n"
		"	yuva.r = yuva.r * contrast + offset;\n"
		"	gl_FragColor = yuva;\n"
		"}\n";

	static const char *brightness_yuv_frag =
		"uniform sampler2D tex;\n"
		"uniform float brightness;\n"
		"uniform float contrast;\n"
		"uniform float offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 yuva = texture2D(tex, gl_TexCoord[0].st);\n"
		"	yuva.r += brightness;\n"
		"	yuva.rgb *= vec3(contrast, contrast, contrast);\n"
		"	yuva.rgb += vec3(offset, offset, offset);\n"
		"	gl_FragColor = yuva;\n"
		"}\n";

	static const char *brightness_rgb_frag =
		"uniform sampler2D tex;\n"
		"uniform float brightness;\n"
		"uniform float contrast;\n"
		"uniform float offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 rgba = texture2D(tex, gl_TexCoord[0].st);\n"
		"	rgba.rgb += vec3(brightness, brightness, brightness);\n"
		"	rgba.rgb *= vec3(contrast, contrast, contrast);\n"
		"	rgba.rgb += vec3(offset, offset, offset);\n"
		"	gl_FragColor = rgba;\n"
		"}\n";

	static const char *brightness_rgbluma_frag =
		"uniform sampler2D tex;\n"
		"uniform float brightness;\n"
		"uniform float contrast;\n"
		"uniform float offset;\n"
		"uniform mat3 yuv_to_rgb_matrix;\n"
		"uniform mat3 rgb_to_yuv_matrix;\n"

		"void main()\n"
		"{\n"
		"	vec4 rgba = texture2D(tex, gl_TexCoord[0].st);\n"
		"	rgba.rgb = rgb_to_yuv_matrix * rgba.rgb;\n"
		"	rgba.r += brightness;\n"
		"	rgba.r = rgba.r * contrast + offset;\n"
		"	rgba.rgb = yuv_to_rgb_matrix * rgba.rgb;\n"
		"	gl_FragColor = rgba;\n"
		"}\n";

	get_output()->to_texture();
	get_output()->enable_opengl();
	int need_matrix = 0;
	const char *brightness_frag = BC_CModels::is_yuv(get_output()->get_color_model()) ?
		(config.luma ? (need_matrix = 0, brightness_yuvluma_frag) : brightness_yuv_frag) :
		(config.luma ? (need_matrix = 1, brightness_rgbluma_frag) : brightness_rgb_frag) ;

	unsigned int shader_id = VFrame::make_shader(0, brightness_frag, 0);
	if( shader_id > 0 ) {
		glUseProgram(shader_id);
		glUniform1i(glGetUniformLocation(shader_id, "tex"), 0);
		glUniform1f(glGetUniformLocation(shader_id, "brightness"), config.brightness / 100);
		float contrast = (config.contrast < 0) ?
			(config.contrast + 100) / 100 :
			(config.contrast + 25) / 25;
		glUniform1f(glGetUniformLocation(shader_id, "contrast"), contrast);
		float offset = 0.5 - contrast / 2;
		glUniform1f(glGetUniformLocation(shader_id, "offset"), offset);
		if( need_matrix ) {
			BC_GL_MATRIX(shader_id, yuv_to_rgb_matrix);
			BC_GL_MATRIX(shader_id, rgb_to_yuv_matrix);
		}
	}

	get_output()->init_screen();
	get_output()->bind_texture(0);



	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
//printf("BrightnessMain::handle_opengl 100 %x\n", glGetError());
#endif
	return 0;
}


void BrightnessMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			((BrightnessWindow*)thread->window)->lock_window("BrightnessMain::update_gui");
			((BrightnessWindow*)thread->window)->brightness->update(config.brightness);
			((BrightnessWindow*)thread->window)->contrast->update(config.contrast);
			((BrightnessWindow*)thread->window)->luma->update(config.luma);
			((BrightnessWindow*)thread->window)->unlock_window();
		}
	}
}


void BrightnessMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("BRIGHTNESS");
	output.tag.set_property("BRIGHTNESS", config.brightness);
	output.tag.set_property("CONTRAST",  config.contrast);
	output.tag.set_property("LUMA",  config.luma);
	output.append_tag();
	output.tag.set_title("/BRIGHTNESS");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void BrightnessMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("BRIGHTNESS"))
			{
				config.brightness = input.tag.get_property("BRIGHTNESS", config.brightness);
				config.contrast = input.tag.get_property("CONTRAST", config.contrast);
				config.luma = input.tag.get_property("LUMA", config.luma);
			}
		}
	}
}













BrightnessPackage::BrightnessPackage()
 : LoadPackage()
{
}




BrightnessUnit::BrightnessUnit(BrightnessEngine *server, BrightnessMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
}

BrightnessUnit::~BrightnessUnit()
{
}

void BrightnessUnit::process_package(LoadPackage *package)
{
	BrightnessPackage *pkg = (BrightnessPackage*)package;


	VFrame *output = plugin->output;
	VFrame *input = plugin->input;





#define DO_BRIGHTNESS(max, type, components, is_yuv) \
{ \
	type **input_rows = (type**)input->get_rows(); \
	type **output_rows = (type**)output->get_rows(); \
	int row1 = pkg->row1; \
	int row2 = pkg->row2; \
	int width = output->get_w(); \
	int r, g, b; \
 \
	if(!EQUIV(plugin->config.brightness, 0)) \
	{ \
		int offset = (int)(plugin->config.brightness / 100 * max); \
/*printf("DO_BRIGHTNESS offset=%d\n", offset);*/ \
 \
		for(int i = row1; i < row2; i++) \
		{ \
			type *input_row = input_rows[i]; \
			type *output_row = output_rows[i]; \
 \
			for(int j = 0; j < width; j++) \
			{ \
				r = input_row[j * components] + offset; \
 \
 				if(!is_yuv) \
				{ \
					g = input_row[j * components + 1] + offset; \
					b = input_row[j * components + 2] + offset; \
				} \
 \
				CLAMP(r, 0, max); \
				if(!is_yuv) \
				{ \
					CLAMP(g, 0, max); \
					CLAMP(b, 0, max); \
				} \
 \
				output_row[j * components] = r; \
 \
 				if(!is_yuv) \
				{ \
					output_row[j * components + 1] = g; \
					output_row[j * components + 2] = b; \
				} \
				else \
				{ \
					output_row[j * components + 1] = input_row[j * components + 1]; \
					output_row[j * components + 2] = input_row[j * components + 2]; \
				} \
 \
 				if(components == 4)  \
					output_row[j * components + 3] = input_row[j * components + 3]; \
			} \
		} \
 \
/* Data to be processed is now in the output buffer */ \
		input_rows = output_rows; \
	} \
 \
	if(!EQUIV(plugin->config.contrast, 0)) \
	{ \
		float contrast = (plugin->config.contrast < 0) ?  \
			(plugin->config.contrast + 100) / 100 :  \
			(plugin->config.contrast + 25) / 25; \
/*printf("DO_BRIGHTNESS contrast=%f\n", contrast);*/ \
 \
		int scalar = (int)(contrast * 0x100); \
		int offset = (max << 8) / 2 - max * scalar / 2; \
		int y, u, v; \
 \
		for(int i = row1; i < row2; i++) \
		{ \
			type *input_row = input_rows[i]; \
			type *output_row = output_rows[i]; \
 \
 			if(plugin->config.luma) \
			{ \
				for(int j = 0; j < width; j++) \
				{ \
					if(is_yuv) \
					{ \
						y = input_row[j * components]; \
					} \
					else \
					{ \
						r = input_row[j * components]; \
						g = input_row[j * components + 1]; \
						b = input_row[j * components + 2]; \
						if(max == 0xff) \
						{ \
							YUV::yuv.rgb_to_yuv_8( \
								r,  \
								g,  \
								b,  \
								y,  \
								u,  \
								v); \
						} \
						else \
						{ \
							YUV::yuv.rgb_to_yuv_16( \
								r,  \
								g,  \
								b,  \
								y,  \
								u,  \
								v); \
						} \
	 \
					} \
	 \
					y = (y * scalar + offset) >> 8; \
					CLAMP(y, 0, max); \
	 \
	 \
 					if(is_yuv) \
					{ \
						output_row[j * components] = y; \
						output_row[j * components + 1] = input_row[j * components + 1]; \
						output_row[j * components + 2] = input_row[j * components + 2]; \
					} \
					else \
					{ \
						if(max == 0xff) \
						{ \
							YUV::yuv.yuv_to_rgb_8( \
								r,  \
								g,  \
								b,  \
								y,  \
								u,  \
								v); \
						} \
						else \
						{ \
							YUV::yuv.yuv_to_rgb_16( \
								r,  \
								g,  \
								b,  \
								y,  \
								u,  \
								v); \
						} \
						input_row[j * components] = r; \
						input_row[j * components + 1] = g; \
						input_row[j * components + 2] = b; \
					} \
	 \
 					if(components == 4)  \
						output_row[j * components + 3] = input_row[j * components + 3]; \
				} \
			} \
			else \
			{ \
				for(int j = 0; j < width; j++) \
				{ \
					r = input_row[j * components]; \
					g = input_row[j * components + 1]; \
					b = input_row[j * components + 2]; \
 \
					r = (r * scalar + offset) >> 8; \
					g = (g * scalar + offset) >> 8; \
					b = (b * scalar + offset) >> 8; \
 \
					CLAMP(r, 0, max); \
					CLAMP(g, 0, max); \
					CLAMP(b, 0, max); \
 \
					output_row[j * components] = r; \
					output_row[j * components + 1] = g; \
					output_row[j * components + 2] = b; \
 \
 					if(components == 4)  \
						output_row[j * components + 3] = input_row[j * components + 3]; \
				} \
			} \
		} \
	} \
}



#define DO_BRIGHTNESS_F(components) \
{ \
	float **input_rows = (float**)input->get_rows(); \
	float **output_rows = (float**)output->get_rows(); \
	int row1 = pkg->row1; \
	int row2 = pkg->row2; \
	int width = output->get_w(); \
	float r, g, b; \
 \
	if(!EQUIV(plugin->config.brightness, 0)) \
	{ \
		float offset = plugin->config.brightness / 100; \
 \
		for(int i = row1; i < row2; i++) \
		{ \
			float *input_row = input_rows[i]; \
			float *output_row = output_rows[i]; \
 \
			for(int j = 0; j < width; j++) \
			{ \
				r = input_row[j * components] + offset; \
				g = input_row[j * components + 1] + offset; \
				b = input_row[j * components + 2] + offset; \
 \
				output_row[j * components] = r; \
				output_row[j * components + 1] = g; \
				output_row[j * components + 2] = b; \
 				if(components == 4)  \
					output_row[j * components + 3] = input_row[j * components + 3]; \
			} \
		} \
 \
/* Data to be processed is now in the output buffer */ \
		input_rows = output_rows; \
	} \
 \
	if(!EQUIV(plugin->config.contrast, 0)) \
	{ \
		float contrast = (plugin->config.contrast < 0) ?  \
			(plugin->config.contrast + 100) / 100 :  \
			(plugin->config.contrast + 25) / 25; \
 \
/* Shift black level down so shadows get darker instead of lighter */ \
		float offset = 0.5 - contrast / 2; \
		float y, u, v; \
 \
		for(int i = row1; i < row2; i++) \
		{ \
			float *input_row = input_rows[i]; \
			float *output_row = output_rows[i]; \
 \
 			if(plugin->config.luma) \
			{ \
				for(int j = 0; j < width; j++) \
				{ \
					r = input_row[j * components]; \
					g = input_row[j * components + 1]; \
					b = input_row[j * components + 2]; \
					YUV::yuv.rgb_to_yuv_f(r, g, b, y, u, v); \
					y = y * contrast + offset; \
					YUV::yuv.yuv_to_rgb_f(r, g, b, y, u, v); \
					input_row[j * components] = r; \
					input_row[j * components + 1] = g; \
					input_row[j * components + 2] = b; \
 \
 					if(components == 4)  \
						output_row[j * components + 3] = input_row[j * components + 3]; \
				} \
			} \
			else \
			{ \
				for(int j = 0; j < width; j++) \
				{ \
					r = input_row[j * components]; \
					g = input_row[j * components + 1]; \
					b = input_row[j * components + 2]; \
 \
					r = r * contrast + offset; \
					g = g * contrast + offset; \
					b = b * contrast + offset; \
 \
					output_row[j * components] = r; \
					output_row[j * components + 1] = g; \
					output_row[j * components + 2] = b; \
 \
 					if(components == 4)  \
						output_row[j * components + 3] = input_row[j * components + 3]; \
				} \
			} \
		} \
	} \
}


	switch(input->get_color_model())
	{
		case BC_RGB888:
			DO_BRIGHTNESS(0xff, unsigned char, 3, 0)
			break;

		case BC_RGB_FLOAT:
			DO_BRIGHTNESS_F(3)
			break;

		case BC_YUV888:
			DO_BRIGHTNESS(0xff, unsigned char, 3, 1)
			break;

		case BC_RGBA8888:
			DO_BRIGHTNESS(0xff, unsigned char, 4, 0)
			break;

		case BC_RGBA_FLOAT:
			DO_BRIGHTNESS_F(4)
			break;

		case BC_YUVA8888:
			DO_BRIGHTNESS(0xff, unsigned char, 4, 1)
			break;

		case BC_RGB161616:
			DO_BRIGHTNESS(0xffff, uint16_t, 3, 0)
			break;

		case BC_YUV161616:
			DO_BRIGHTNESS(0xffff, uint16_t, 3, 1)
			break;

		case BC_RGBA16161616:
			DO_BRIGHTNESS(0xffff, uint16_t, 4, 0)
			break;

		case BC_YUVA16161616:
			DO_BRIGHTNESS(0xffff, uint16_t, 4, 1)
			break;
	}









}






BrightnessEngine::BrightnessEngine(BrightnessMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

BrightnessEngine::~BrightnessEngine()
{
}


void BrightnessEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		BrightnessPackage *package = (BrightnessPackage*)LoadServer::get_package(i);
		package->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		package->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}

LoadClient* BrightnessEngine::new_client()
{
	return new BrightnessUnit(this, plugin);
}

LoadPackage* BrightnessEngine::new_package()
{
	return new BrightnessPackage;
}





