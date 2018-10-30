
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
#include "bchash.h"
#include "deinterlace.h"
#include "deinterwindow.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>


REGISTER_PLUGIN(DeInterlaceMain)

DeInterlaceConfig::DeInterlaceConfig()
{
	mode = DEINTERLACE_EVEN;
//	adaptive = 0;
//	threshold = 40;
}

int DeInterlaceConfig::equivalent(DeInterlaceConfig &that)
{
	return mode == that.mode;
//		&&
//		adaptive == that.adaptive &&
//		threshold == that.threshold;
}

void DeInterlaceConfig::copy_from(DeInterlaceConfig &that)
{
	mode = that.mode;
//	adaptive = that.adaptive;
//	threshold = that.threshold;
}

void DeInterlaceConfig::interpolate(DeInterlaceConfig &prev,
	DeInterlaceConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	copy_from(prev);
}




DeInterlaceMain::DeInterlaceMain(PluginServer *server)
 : PluginVClient(server)
{

//	temp = 0;
}

DeInterlaceMain::~DeInterlaceMain()
{

//	if(temp) delete temp;
}

const char* DeInterlaceMain::plugin_title() { return N_("Deinterlace"); }
int DeInterlaceMain::is_realtime() { return 1; }



#define DEINTERLACE_EVEN_MACRO(type, components, dominance) \
{ \
	int w = input->get_w(); \
	int h = input->get_h(); \
 \
	for(int i = 0; i < h - 1; i += 2) \
	{ \
		type *input_row = (type*)input->get_rows()[dominance ? i + 1 : i]; \
		type *output_row1 = (type*)output->get_rows()[i]; \
		type *output_row2 = (type*)output->get_rows()[i + 1]; \
		memcpy(output_row1, input_row, w * components * sizeof(type)); \
		memcpy(output_row2, input_row, w * components * sizeof(type)); \
	} \
}

#define DEINTERLACE_AVG_EVEN_MACRO(type, temp_type, components, dominance) \
{ \
	int w = input->get_w(); \
	int h = input->get_h(); \
	changed_rows = 0; \
 \
 	type **in_rows = (type**)input->get_rows(); \
	type **out_rows = (type**)temp->get_rows(); \
	int max_h = h - 1; \
/*	temp_type abs_diff = 0, total = 0; */ \
 \
	for(int i = 0; i < max_h; i += 2) \
	{ \
		int in_number1 = dominance ? i - 1 : i + 0; \
		int in_number2 = dominance ? i + 1 : i + 2; \
		int out_number1 = dominance ? i - 1 : i; \
		int out_number2 = dominance ? i : i + 1; \
		in_number1 = MAX(in_number1, 0); \
		in_number2 = MIN(in_number2, max_h); \
		out_number1 = MAX(out_number1, 0); \
		out_number2 = MIN(out_number2, max_h); \
 \
		type *input_row1 = in_rows[in_number1]; \
		type *input_row2 = in_rows[in_number2]; \
/*		type *input_row3 = in_rows[out_number2]; */\
		type *temp_row1 = out_rows[out_number1]; \
		type *temp_row2 = out_rows[out_number2]; \
/*		temp_type sum = 0; */\
		temp_type accum_r, accum_b, accum_g, accum_a; \
 \
		memcpy(temp_row1, input_row1, w * components * sizeof(type)); \
		for(int j = 0; j < w; j++) \
		{ \
			accum_r = (*input_row1++) + (*input_row2++); \
			accum_g = (*input_row1++) + (*input_row2++); \
			accum_b = (*input_row1++) + (*input_row2++); \
			if(components == 4) \
				accum_a = (*input_row1++) + (*input_row2++); \
			accum_r /= 2; \
			accum_g /= 2; \
			accum_b /= 2; \
			accum_a /= 2; \
 \
/* 			total += *input_row3; */\
/*			sum = ((temp_type)*input_row3++) - accum_r; */\
/*			abs_diff += (sum < 0 ? -sum : sum); */\
			*temp_row2++ = accum_r; \
 \
/* 			total += *input_row3; */\
/*			sum = ((temp_type)*input_row3++) - accum_g; */\
/*			abs_diff += (sum < 0 ? -sum : sum); */\
			*temp_row2++ = accum_g; \
 \
/* 			total += *input_row3; */\
/*			sum = ((temp_type)*input_row3++) - accum_b; */\
/*			abs_diff += (sum < 0 ? -sum : sum); */\
			*temp_row2++ = accum_b; \
 \
			if(components == 4) \
			{ \
/*	 			total += *input_row3; */\
/*				sum = ((temp_type)*input_row3++) - accum_a; */\
/*				abs_diff += (sum < 0 ? -sum : sum); */\
				*temp_row2++ = accum_a; \
			} \
		} \
	} \
 \
/*	temp_type threshold = (temp_type)total * config.threshold / THRESHOLD_SCALAR; */\
/* printf("total=%lld threshold=%lld abs_diff=%lld\n", total, threshold, abs_diff); */ \
/* 	if(abs_diff > threshold || !config.adaptive) */\
/*	{ */\
/*		output->copy_from(temp); */ \
/*		changed_rows = 240; */ \
/*	} */\
/*	else */\
/*	{ */\
/*		output->copy_from(input); */\
/*		changed_rows = 0; */\
/*	} */\
 \
}

#define DEINTERLACE_AVG_MACRO(type, temp_type, components) \
{ \
	int w = input->get_w(); \
	int h = input->get_h(); \
 \
	for(int i = 0; i < h - 1; i += 2) \
	{ \
		type *input_row1 = (type*)input->get_rows()[i]; \
		type *input_row2 = (type*)input->get_rows()[i + 1]; \
		type *output_row1 = (type*)output->get_rows()[i]; \
		type *output_row2 = (type*)output->get_rows()[i + 1]; \
		type result; \
 \
		for(int j = 0; j < w * components; j++) \
		{ \
			result = ((temp_type)input_row1[j] + input_row2[j]) / 2; \
			output_row1[j] = result; \
			output_row2[j] = result; \
		} \
	} \
}

#define DEINTERLACE_SWAP_MACRO(type, components, dominance) \
{ \
	int w = input->get_w(); \
	int h = input->get_h(); \
 \
	for(int i = dominance; i < h - 1; i += 2) \
	{ \
		type *input_row1 = (type*)input->get_rows()[i]; \
		type *input_row2 = (type*)input->get_rows()[i + 1]; \
		type *output_row1 = (type*)output->get_rows()[i]; \
		type *output_row2 = (type*)output->get_rows()[i + 1]; \
		type temp1, temp2; \
 \
		for(int j = 0; j < w * components; j++) \
		{ \
			temp1 = input_row1[j]; \
			temp2 = input_row2[j]; \
			output_row1[j] = temp2; \
			output_row2[j] = temp1; \
		} \
	} \
}


void DeInterlaceMain::deinterlace_even(VFrame *input, VFrame *output, int dominance)
{
	switch(input->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			DEINTERLACE_EVEN_MACRO(unsigned char, 3, dominance);
			break;
		case BC_RGB_FLOAT:
			DEINTERLACE_EVEN_MACRO(float, 3, dominance);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			DEINTERLACE_EVEN_MACRO(unsigned char, 4, dominance);
			break;
		case BC_RGBA_FLOAT:
			DEINTERLACE_EVEN_MACRO(float, 4, dominance);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			DEINTERLACE_EVEN_MACRO(uint16_t, 3, dominance);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			DEINTERLACE_EVEN_MACRO(uint16_t, 4, dominance);
			break;
	}
}

void DeInterlaceMain::deinterlace_avg_even(VFrame *input, VFrame *output, int dominance)
{
	switch(input->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			DEINTERLACE_AVG_EVEN_MACRO(unsigned char, int64_t, 3, dominance);
			break;
		case BC_RGB_FLOAT:
			DEINTERLACE_AVG_EVEN_MACRO(float, double, 3, dominance);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			DEINTERLACE_AVG_EVEN_MACRO(unsigned char, int64_t, 4, dominance);
			break;
		case BC_RGBA_FLOAT:
			DEINTERLACE_AVG_EVEN_MACRO(float, double, 4, dominance);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			DEINTERLACE_AVG_EVEN_MACRO(uint16_t, int64_t, 3, dominance);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			DEINTERLACE_AVG_EVEN_MACRO(uint16_t, int64_t, 4, dominance);
			break;
	}
}

void DeInterlaceMain::deinterlace_avg(VFrame *input, VFrame *output)
{
	switch(input->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			DEINTERLACE_AVG_MACRO(unsigned char, uint64_t, 3);
			break;
		case BC_RGB_FLOAT:
			DEINTERLACE_AVG_MACRO(float, double, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			DEINTERLACE_AVG_MACRO(unsigned char, uint64_t, 4);
			break;
		case BC_RGBA_FLOAT:
			DEINTERLACE_AVG_MACRO(float, double, 4);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			DEINTERLACE_AVG_MACRO(uint16_t, uint64_t, 3);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			DEINTERLACE_AVG_MACRO(uint16_t, uint64_t, 4);
			break;
	}
}

void DeInterlaceMain::deinterlace_swap(VFrame *input, VFrame *output, int dominance)
{
	switch(input->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			DEINTERLACE_SWAP_MACRO(unsigned char, 3, dominance);
			break;
		case BC_RGB_FLOAT:
			DEINTERLACE_SWAP_MACRO(float, 3, dominance);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			DEINTERLACE_SWAP_MACRO(unsigned char, 4, dominance);
			break;
		case BC_RGBA_FLOAT:
			DEINTERLACE_SWAP_MACRO(float, 4, dominance);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			DEINTERLACE_SWAP_MACRO(uint16_t, 3, dominance);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			DEINTERLACE_SWAP_MACRO(uint16_t, 4, dominance);
			break;
	}
}


int DeInterlaceMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	changed_rows = frame->get_h();
	load_configuration();


	read_frame(frame,
		0,
		start_position,
		frame_rate,
		get_use_opengl());
	if(get_use_opengl()) return run_opengl();

// Temp was used for adaptive deinterlacing where it took deinterlacing
// an entire frame to decide if the deinterlaced output should be used.
	temp = frame;

// 	if(!temp)
// 		temp = new VFrame(frame->get_w(), frame->get_h(),
// 			frame->get_color_model(), 0);

	switch(config.mode)
	{
		case DEINTERLACE_NONE:
//			output->copy_from(input);
			break;
		case DEINTERLACE_EVEN:
			deinterlace_even(frame, frame, 0);
			break;
		case DEINTERLACE_ODD:
			deinterlace_even(frame, frame, 1);
			break;
		case DEINTERLACE_AVG:
			deinterlace_avg(frame, frame);
			break;
		case DEINTERLACE_AVG_EVEN:
			deinterlace_avg_even(frame, frame, 0);
			break;
		case DEINTERLACE_AVG_ODD:
			deinterlace_avg_even(frame, frame, 1);
			break;
		case DEINTERLACE_SWAP_ODD:
			deinterlace_swap(frame, frame, 1);
			break;
		case DEINTERLACE_SWAP_EVEN:
			deinterlace_swap(frame, frame, 0);
			break;
	}
	send_render_gui(&changed_rows);
	return 0;
}

int DeInterlaceMain::handle_opengl()
{
#ifdef HAVE_GL
	static const char *head_frag =
		"uniform sampler2D tex;\n"
		"uniform float double_line_h;\n"
		"uniform float line_h;\n"
		"uniform float y_offset;\n"
		"void main()\n"
		"{\n"
		"	vec2 coord = gl_TexCoord[0].st;\n";

	static const char *line_double_frag =
		"	float line1 = floor((coord.y - y_offset) / double_line_h) * double_line_h + y_offset;\n"
		"	gl_FragColor =  texture2D(tex, vec2(coord.x, line1));\n";

	static const char *line_avg_frag =
		"	float line1 = floor((coord.y - 0.0) / double_line_h) * double_line_h + 0.0;\n"
		"	float line2 = line1 + line_h;\n"
		"	gl_FragColor = (texture2D(tex, vec2(coord.x, line1)) + \n"
		"		texture2D(tex, vec2(coord.x, line2))) / 2.0;\n";

	static const char *field_avg_frag =
		"	float line1 = floor((coord.y - y_offset) / double_line_h) * double_line_h + y_offset;\n"
		"	float line2 = line1 + double_line_h;\n"
		"	float frac = (line2 - coord.y) / double_line_h;\n"
		"	gl_FragColor = mix(texture2D(tex, vec2(coord.x, line2)),\n"
		"		texture2D(tex, vec2(coord.x, line1)),frac);\n";

	static const char *line_swap_frag =
		"	float line1 = floor((coord.y - y_offset) / double_line_h) * double_line_h + y_offset;\n"
// This is the input line for line2, not the denominator of the fraction
		"	float line2 = line1 + line_h;\n"
		"	float frac = (coord.y - line1) / double_line_h;\n"
		"	gl_FragColor = mix(texture2D(tex, vec2(coord.x, line2)),\n"
		"		texture2D(tex, vec2(coord.x, line1)), frac);\n";

	static const char *tail_frag =
		"}\n";

	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();

	if( config.mode != DEINTERLACE_NONE ) {
		const char *shader_stack[16];
		memset(shader_stack,0, sizeof(shader_stack));
		int current_shader = 0;

		shader_stack[current_shader++] = head_frag;

		float double_line_h = 2.0 / get_output()->get_texture_h();
		float line_h = 1.0 / get_output()->get_texture_h();
		float y_offset = 0.0;
		const char *shader_frag = 0;

		switch(config.mode) {
		case DEINTERLACE_EVEN:
			shader_frag = line_double_frag;
			break;
		case DEINTERLACE_ODD:
			shader_frag = line_double_frag;
			y_offset += 1.0;
			break;

		case DEINTERLACE_AVG:
			shader_frag = line_avg_frag;
			break;

		case DEINTERLACE_AVG_EVEN:
			shader_frag = field_avg_frag;
			break;

		case DEINTERLACE_AVG_ODD:
			shader_frag = field_avg_frag;
			y_offset += 1.0;
			break;

		case DEINTERLACE_SWAP_EVEN:
			shader_frag = line_swap_frag;
			break;

		case DEINTERLACE_SWAP_ODD:
			shader_frag = line_swap_frag;
			y_offset += 1.0;
			break;
		}
		if( shader_frag )
			shader_stack[current_shader++] = shader_frag;

		shader_stack[current_shader++] = tail_frag;
		shader_stack[current_shader] = 0;
		unsigned int shader = VFrame::make_shader(shader_stack);
		if( shader > 0 ) {
			y_offset /= get_output()->get_texture_h();
			glUseProgram(shader);
			glUniform1i(glGetUniformLocation(shader, "tex"), 0);
			glUniform1f(glGetUniformLocation(shader, "line_h"), line_h);
			glUniform1f(glGetUniformLocation(shader, "double_line_h"), double_line_h);
			glUniform1f(glGetUniformLocation(shader, "y_offset"), y_offset);
		}
	}

	get_output()->bind_texture(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	get_output()->draw_texture();

	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

#endif
	return 0;
}

void DeInterlaceMain::render_gui(void *data)
{
	if(thread)
	{
		((DeInterlaceWindow*)thread->window)->lock_window();
		char string[BCTEXTLEN];
		((DeInterlaceWindow*)thread->window)->get_status_string(string, *(int*)data);
//		((DeInterlaceWindow*)thread->window)->status->update(string);
		((DeInterlaceWindow*)thread->window)->flush();
		((DeInterlaceWindow*)thread->window)->unlock_window();
	}
}

NEW_WINDOW_MACRO(DeInterlaceMain, DeInterlaceWindow)
LOAD_CONFIGURATION_MACRO(DeInterlaceMain, DeInterlaceConfig)



void DeInterlaceMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("DEINTERLACE");
	output.tag.set_property("MODE", config.mode);
//	output.tag.set_property("ADAPTIVE", config.adaptive);
//	output.tag.set_property("THRESHOLD", config.threshold);
	output.append_tag();
	output.tag.set_title("/DEINTERLACE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void DeInterlaceMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	while(!input.read_tag())
	{
		if(input.tag.title_is("DEINTERLACE"))
		{
			config.mode = input.tag.get_property("MODE", config.mode);
//			config.adaptive = input.tag.get_property("ADAPTIVE", config.adaptive);
//			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
		}
	}

}

void DeInterlaceMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		((DeInterlaceWindow*)thread->window)->lock_window();
		((DeInterlaceWindow*)thread->window)->set_mode(config.mode, 1);
//		((DeInterlaceWindow*)thread->window)->adaptive->update(config.adaptive);
//		((DeInterlaceWindow*)thread->window)->threshold->update(config.threshold);
		((DeInterlaceWindow*)thread->window)->unlock_window();
	}
}

