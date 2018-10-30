
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "edlsession.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "overlayframe.h"
#include "pluginvclient.h"
#include "vpatchgui.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>


class Overlay;
class OverlayWindow;


class OverlayConfig
{
public:
	OverlayConfig();

	static const char* mode_to_text(int mode);
	int mode;

	static const char* direction_to_text(int direction);
	int direction;
	enum
	{
		BOTTOM_FIRST,
		TOP_FIRST
	};

	static const char* output_to_text(int output_layer);
	int output_layer;
	enum
	{
		TOP,
		BOTTOM
	};
};

class OverlayMode : public BC_PopupMenu
{
public:
	OverlayMode(Overlay *plugin,
		int x,
		int y);
	void create_objects();
	int handle_event();
	Overlay *plugin;
};

class OverlayDirection : public BC_PopupMenu
{
public:
	OverlayDirection(Overlay *plugin,
		int x,
		int y);
	void create_objects();
	int handle_event();
	Overlay *plugin;
};

class OverlayOutput : public BC_PopupMenu
{
public:
	OverlayOutput(Overlay *plugin,
		int x,
		int y);
	void create_objects();
	int handle_event();
	Overlay *plugin;
};


class OverlayWindow : public PluginClientWindow
{
public:
	OverlayWindow(Overlay *plugin);
	~OverlayWindow();

	void create_objects();


	Overlay *plugin;
	OverlayMode *mode;
	OverlayDirection *direction;
	OverlayOutput *output;
};

class Overlay : public PluginVClient
{
public:
	Overlay(PluginServer *server);
	~Overlay();


	PLUGIN_CLASS_MEMBERS(OverlayConfig);

	int process_buffer(VFrame **frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int is_multichannel();
	int is_synthesis();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int handle_opengl();

	OverlayFrame *overlayer;
	VFrame *temp;
	int current_layer;
	int output_layer;
	int input_layer;
};

OverlayConfig::OverlayConfig()
{
	mode = TRANSFER_NORMAL;
	direction = OverlayConfig::BOTTOM_FIRST;
	output_layer = OverlayConfig::TOP;
}

const char* OverlayConfig::mode_to_text(int mode)
{
	return VModePatch::mode_to_text(mode);
}

const char* OverlayConfig::direction_to_text(int direction)
{
	switch(direction)
	{
		case OverlayConfig::BOTTOM_FIRST: return _("Bottom first");
		case OverlayConfig::TOP_FIRST:    return _("Top first");
	}
	return "";
}

const char* OverlayConfig::output_to_text(int output_layer)
{
	switch(output_layer)
	{
		case OverlayConfig::TOP:    return _("Top");
		case OverlayConfig::BOTTOM: return _("Bottom");
	}
	return "";
}









OverlayWindow::OverlayWindow(Overlay *plugin)
 : PluginClientWindow(plugin,
	300,
	160,
	300,
	160,
	0)
{
	this->plugin = plugin;
}

OverlayWindow::~OverlayWindow()
{
}

void OverlayWindow::create_objects()
{
	int x = 10, y = 10;

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Mode:")));
	add_subwindow(mode = new OverlayMode(plugin,
		x + title->get_w() + 5,
		y));
	mode->create_objects();

	y += 30;
	add_subwindow(title = new BC_Title(x, y, _("Layer order:")));
	add_subwindow(direction = new OverlayDirection(plugin,
		x + title->get_w() + 5,
		y));
	direction->create_objects();

	y += 30;
	add_subwindow(title = new BC_Title(x, y, _("Output layer:")));
	add_subwindow(output = new OverlayOutput(plugin,
		x + title->get_w() + 5,
		y));
	output->create_objects();

	show_window();
	flush();
}







OverlayMode::OverlayMode(Overlay *plugin, int x, int y)
 : BC_PopupMenu(x, y, 150,
	OverlayConfig::mode_to_text(plugin->config.mode), 1)
{
	this->plugin = plugin;
}

void OverlayMode::create_objects()
{
	for(int i = 0; i < TRANSFER_TYPES; i++)
		add_item(new BC_MenuItem(OverlayConfig::mode_to_text(i)));
}

int OverlayMode::handle_event()
{
	char *text = get_text();

	for(int i = 0; i < TRANSFER_TYPES; i++)
	{
		if(!strcmp(text, OverlayConfig::mode_to_text(i)))
		{
			plugin->config.mode = i;
			break;
		}
	}

	plugin->send_configure_change();
	return 1;
}


OverlayDirection::OverlayDirection(Overlay *plugin,
	int x,
	int y)
 : BC_PopupMenu(x,
 	y,
	150,
	OverlayConfig::direction_to_text(plugin->config.direction),
	1)
{
	this->plugin = plugin;
}

void OverlayDirection::create_objects()
{
	add_item(new BC_MenuItem(
		OverlayConfig::direction_to_text(
			OverlayConfig::TOP_FIRST)));
	add_item(new BC_MenuItem(
		OverlayConfig::direction_to_text(
			OverlayConfig::BOTTOM_FIRST)));
}

int OverlayDirection::handle_event()
{
	char *text = get_text();

	if(!strcmp(text,
		OverlayConfig::direction_to_text(
			OverlayConfig::TOP_FIRST)))
		plugin->config.direction = OverlayConfig::TOP_FIRST;
	else
	if(!strcmp(text,
		OverlayConfig::direction_to_text(
			OverlayConfig::BOTTOM_FIRST)))
		plugin->config.direction = OverlayConfig::BOTTOM_FIRST;

	plugin->send_configure_change();
	return 1;
}


OverlayOutput::OverlayOutput(Overlay *plugin,
	int x,
	int y)
 : BC_PopupMenu(x,
 	y,
	100,
	OverlayConfig::output_to_text(plugin->config.output_layer),
	1)
{
	this->plugin = plugin;
}

void OverlayOutput::create_objects()
{
	add_item(new BC_MenuItem(
		OverlayConfig::output_to_text(
			OverlayConfig::TOP)));
	add_item(new BC_MenuItem(
		OverlayConfig::output_to_text(
			OverlayConfig::BOTTOM)));
}

int OverlayOutput::handle_event()
{
	char *text = get_text();

	if(!strcmp(text,
		OverlayConfig::output_to_text(
			OverlayConfig::TOP)))
		plugin->config.output_layer = OverlayConfig::TOP;
	else
	if(!strcmp(text,
		OverlayConfig::output_to_text(
			OverlayConfig::BOTTOM)))
		plugin->config.output_layer = OverlayConfig::BOTTOM;

	plugin->send_configure_change();
	return 1;
}




















REGISTER_PLUGIN(Overlay)






Overlay::Overlay(PluginServer *server)
 : PluginVClient(server)
{

	overlayer = 0;
	temp = 0;
}


Overlay::~Overlay()
{

	if(overlayer) delete overlayer;
	if(temp) delete temp;
}



int Overlay::process_buffer(VFrame **frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	EDLSession* session = get_edlsession();
	int interpolation_type = session ? session->interpolation_type : NEAREST_NEIGHBOR;

	int step = config.direction == OverlayConfig::BOTTOM_FIRST ?  -1 : 1;
	int layers = get_total_buffers();
	input_layer = config.direction == OverlayConfig::BOTTOM_FIRST ? layers-1 : 0;
	output_layer = config.output_layer == OverlayConfig::TOP ?  0 : layers-1;
	VFrame *output = frame[output_layer];

	current_layer = input_layer;
	read_frame(output, current_layer,  // Direct copy the first layer
			 start_position, frame_rate, get_use_opengl());

	if( --layers > 0 ) {	// need 2 layers to do overlay
		if( !temp )
			temp = new VFrame(frame[0]->get_w(), frame[0]->get_h(),
					frame[0]->get_color_model(), 0);

		while( --layers >= 0 ) {
			current_layer += step;
			read_frame(temp, current_layer,
				 start_position, frame_rate, get_use_opengl());

			if(get_use_opengl()) {
				run_opengl();
				continue;
			}

			if(!overlayer)
				overlayer = new OverlayFrame(get_project_smp() + 1);

			overlayer->overlay(output, temp,
				0, 0, output->get_w(), output->get_h(),
				0, 0, output->get_w(), output->get_h(),
				1, config.mode, interpolation_type);
		}
	}

	return 0;
}

int Overlay::handle_opengl()
{
#ifdef HAVE_GL
	static const char *get_pixels_frag =
		"uniform sampler2D src_tex;\n"
		"uniform sampler2D dst_tex;\n"
		"uniform vec2 dst_tex_dimensions;\n"
		"uniform vec3 chroma_offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 dst_color = texture2D(dst_tex, gl_FragCoord.xy / dst_tex_dimensions);\n"
		"	vec4 src_color = texture2D(src_tex, gl_TexCoord[0].st);\n"
		"	src_color.rgb -= chroma_offset;\n"
		"	dst_color.rgb -= chroma_offset;\n";

	static const char *put_pixels_frag =
		"	result.rgb += chroma_offset;\n"
		"	gl_FragColor = result;\n"
		"}\n";

#define QQ(q)#q
#define SS(s)QQ(s)

#define GL_STD_FRAG(FN) static const char *blend_##FN##_frag = \
	"	vec4 result;\n" \
	"	result.rgb = " SS(COLOR_##FN(1.0, src_color.rgb, src_color.a, dst_color.rgb, dst_color.a)) ";\n" \
	"	result.a = " SS(ALPHA_##FN(1.0, src_color.a, dst_color.a))";\n" \

#define GL_VEC_FRAG(FN) static const char *blend_##FN##_frag = \
	"	vec4 result;\n" \
	"	result.r = " SS(COLOR_##FN(1.0, src_color.r, src_color.a, dst_color.r, dst_color.a)) ";\n" \
	"	result.g = " SS(COLOR_##FN(1.0, src_color.g, src_color.a, dst_color.g, dst_color.a)) ";\n" \
	"	result.b = " SS(COLOR_##FN(1.0, src_color.b, src_color.a, dst_color.b, dst_color.a)) ";\n" \
	"	result.a = " SS(ALPHA_##FN(1.0, src_color.a, dst_color.a)) ";\n" \
	"	result = clamp(result, 0.0, 1.0);\n" \

#undef mabs
#define mabs abs
#undef mmin
#define mmin min
#undef mmax
#define mmax max

#undef ZERO
#define ZERO 0.0
#undef ONE
#define ONE 1.0
#undef TWO
#define TWO 2.0

static const char *blend_NORMAL_frag =
	"	vec4 result = mix(src_color, src_color, src_color.a);\n";

static const char *blend_ADDITION_frag =
	"	vec4 result = dst_color + src_color;\n"
	"	result = clamp(result, 0.0, 1.0);\n";

static const char *blend_SUBTRACT_frag =
	"	vec4 result = dst_color - src_color;\n"
	"	result = clamp(result, 0.0, 1.0);\n";

static const char *blend_REPLACE_frag =
	"	vec4 result = src_color;\n";

GL_STD_FRAG(MULTIPLY);
GL_VEC_FRAG(DIVIDE);
GL_VEC_FRAG(MAX);
GL_VEC_FRAG(MIN);
GL_VEC_FRAG(DARKEN);
GL_VEC_FRAG(LIGHTEN);
GL_STD_FRAG(DST);
GL_STD_FRAG(DST_ATOP);
GL_STD_FRAG(DST_IN);
GL_STD_FRAG(DST_OUT);
GL_STD_FRAG(DST_OVER);
GL_STD_FRAG(SRC);
GL_STD_FRAG(SRC_ATOP);
GL_STD_FRAG(SRC_IN);
GL_STD_FRAG(SRC_OUT);
GL_STD_FRAG(SRC_OVER);
GL_STD_FRAG(AND);
GL_STD_FRAG(OR);
GL_STD_FRAG(XOR);
GL_VEC_FRAG(OVERLAY);
GL_STD_FRAG(SCREEN);
GL_VEC_FRAG(BURN);
GL_VEC_FRAG(DODGE);
GL_VEC_FRAG(HARDLIGHT);
GL_VEC_FRAG(SOFTLIGHT);
GL_VEC_FRAG(DIFFERENCE);

static const char * const overlay_shaders[TRANSFER_TYPES] = {
	blend_NORMAL_frag,	// TRANSFER_NORMAL
	blend_ADDITION_frag,	// TRANSFER_ADDITION
	blend_SUBTRACT_frag,	// TRANSFER_SUBTRACT
	blend_MULTIPLY_frag,	// TRANSFER_MULTIPLY
	blend_DIVIDE_frag,	// TRANSFER_DIVIDE
	blend_REPLACE_frag,	// TRANSFER_REPLACE
	blend_MAX_frag,		// TRANSFER_MAX
	blend_MIN_frag,		// TRANSFER_MIN
	blend_DARKEN_frag,	// TRANSFER_DARKEN
	blend_LIGHTEN_frag,	// TRANSFER_LIGHTEN
	blend_DST_frag,		// TRANSFER_DST
	blend_DST_ATOP_frag,	// TRANSFER_DST_ATOP
	blend_DST_IN_frag,	// TRANSFER_DST_IN
	blend_DST_OUT_frag,	// TRANSFER_DST_OUT
	blend_DST_OVER_frag,	// TRANSFER_DST_OVER
	blend_SRC_frag,		// TRANSFER_SRC
	blend_SRC_ATOP_frag,	// TRANSFER_SRC_ATOP
	blend_SRC_IN_frag,	// TRANSFER_SRC_IN
	blend_SRC_OUT_frag,	// TRANSFER_SRC_OUT
	blend_SRC_OVER_frag,	// TRANSFER_SRC_OVER
	blend_AND_frag,		// TRANSFER_AND
	blend_OR_frag,		// TRANSFER_OR
	blend_XOR_frag,		// TRANSFER_XOR
	blend_OVERLAY_frag,	// TRANSFER_OVERLAY
	blend_SCREEN_frag,	// TRANSFER_SCREEN
	blend_BURN_frag,	// TRANSFER_BURN
	blend_DODGE_frag,	// TRANSFER_DODGE
	blend_HARDLIGHT_frag,	// TRANSFER_HARDLIGHT
	blend_SOFTLIGHT_frag,	// TRANSFER_SOFTLIGHT
	blend_DIFFERENCE_frag,	// TRANSFER_DIFFERENCE
};

	glDisable(GL_BLEND);
	VFrame *dst = get_output(output_layer);
        VFrame *src = temp;

	switch( config.mode ) {
	case TRANSFER_REPLACE:
	case TRANSFER_SRC:
// Direct copy layer
        	src->to_texture();
	       	dst->enable_opengl();
		dst->init_screen();
		src->draw_texture();
		break;
	case TRANSFER_NORMAL:
// Move destination to screen
		if( dst->get_opengl_state() != VFrame::SCREEN ) {
			dst->to_texture();
        		dst->enable_opengl();
		        dst->init_screen();
			dst->draw_texture();
		}
        	src->to_texture();
	       	dst->enable_opengl();
		dst->init_screen();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		src->draw_texture();
		break;
	default:
        	src->to_texture();
		dst->to_texture();
	       	dst->enable_opengl();
		dst->init_screen();
        	src->bind_texture(0);
	        dst->bind_texture(1);

		const char *shader_stack[16];
		memset(shader_stack,0, sizeof(shader_stack));
		int current_shader = 0;

		shader_stack[current_shader++] = get_pixels_frag;
		shader_stack[current_shader++] = overlay_shaders[config.mode];
		shader_stack[current_shader++] = put_pixels_frag;
		shader_stack[current_shader] = 0;
	        unsigned int shader = VFrame::make_shader(shader_stack);
		if( shader > 0 ) {
			glUseProgram(shader);
			glUniform1i(glGetUniformLocation(shader, "src_tex"), 0);
			glUniform1i(glGetUniformLocation(shader, "dst_tex"), 1);
			glUniform2f(glGetUniformLocation(shader, "dst_tex_dimensions"),
					(float)dst->get_texture_w(), (float)dst->get_texture_h());
			float chroma_offset = BC_CModels::is_yuv(dst->get_color_model()) ? 0.5 : 0.0;
			glUniform3f(glGetUniformLocation(shader, "chroma_offset"),
					0.0, chroma_offset, chroma_offset);
		}
		src->draw_texture();
		glUseProgram(0);

		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		break;
	}

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

// get the data before something else uses the screen
	dst->screen_to_ram();
#endif
	return 0;
}


const char* Overlay::plugin_title() { return N_("Overlay"); }
int Overlay::is_realtime() { return 1; }
int Overlay::is_multichannel() { return 1; }
int Overlay::is_synthesis() { return 1; }



NEW_WINDOW_MACRO(Overlay, OverlayWindow)



int Overlay::load_configuration()
{
	KeyFrame *prev_keyframe;
	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);
	return 0;
}


void Overlay::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("OVERLAY");
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("DIRECTION", config.direction);
	output.tag.set_property("OUTPUT_LAYER", config.output_layer);
	output.append_tag();
	output.tag.set_title("/OVERLAY");
	output.append_tag();
	output.terminate_string();
}

void Overlay::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	while(!input.read_tag())
	{
		if(input.tag.title_is("OVERLAY"))
		{
			config.mode = input.tag.get_property("MODE", config.mode);
			config.direction = input.tag.get_property("DIRECTION", config.direction);
			config.output_layer = input.tag.get_property("OUTPUT_LAYER", config.output_layer);
		}
	}
}

void Overlay::update_gui()
{
	if(thread)
	{
		thread->window->lock_window("Overlay::update_gui");
		((OverlayWindow*)thread->window)->mode->set_text(OverlayConfig::mode_to_text(config.mode));
		((OverlayWindow*)thread->window)->direction->set_text(OverlayConfig::direction_to_text(config.direction));
		((OverlayWindow*)thread->window)->output->set_text(OverlayConfig::output_to_text(config.output_layer));
		thread->window->unlock_window();
	}
}





