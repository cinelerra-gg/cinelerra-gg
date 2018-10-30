
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

#include "dissolve.h"
#include "edl.inc"
#include "language.h"
#include "overlayframe.h"
#include "vframe.h"

#include <string.h>

PluginClient* new_plugin(PluginServer *server)
{
	return new DissolveMain(server);
}





DissolveMain::DissolveMain(PluginServer *server)
 : PluginVClient(server)
{
	overlayer = 0;
}

DissolveMain::~DissolveMain()
{
	delete overlayer;
}

const char* DissolveMain::plugin_title() { return N_("Dissolve"); }
int DissolveMain::is_video() { return 1; }
int DissolveMain::is_transition() { return 1; }
int DissolveMain::uses_gui() { return 0; }



int DissolveMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	fade = (float)PluginClient::get_source_position() /
			PluginClient::get_total_len();

// Use hardware
	if(get_use_opengl())
	{
		run_opengl();
		return 0;
	}

// Use software
	if(!overlayer) overlayer = new OverlayFrame(get_project_smp() + 1);

	overlayer->overlay(outgoing, incoming,
		0, 0, incoming->get_w(), incoming->get_h(),
		0, 0, incoming->get_w(), incoming->get_h(),
		fade, TRANSFER_SRC, NEAREST_NEIGHBOR);

	return 0;
}

int DissolveMain::handle_opengl()
{
#ifdef HAVE_GL
	static const char *blend_dissolve =
		"uniform float fade;\n"
		"uniform sampler2D src_tex;\n"
		"uniform sampler2D dst_tex;\n"
		"uniform vec2 dst_tex_dimensions;\n"
		"uniform vec3 chroma_offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 result_color;\n"
		"	vec4 dst_color = texture2D(dst_tex, gl_FragCoord.xy / dst_tex_dimensions);\n"
		"	vec4 src_color = texture2D(src_tex, gl_TexCoord[0].st);\n"
		"	src_color.rgb -= chroma_offset;\n"
		"	dst_color.rgb -= chroma_offset;\n"
		"	result_color = mix(dst_color, src_color, fade);\n"
		"	result_color.rgb += chroma_offset;\n"
		"	gl_FragColor = result_color;\n"
		"}\n";

// Read images from RAM
	VFrame *src = get_input();
	src->to_texture();
	VFrame *dst = get_output();
	dst->to_texture();
	dst->enable_opengl();
	dst->init_screen();
	src->bind_texture(0);
	dst->bind_texture(1);

	unsigned int shader = VFrame::make_shader(0, blend_dissolve, 0);
	if( shader > 0 ) {
		glUseProgram(shader);
		glUniform1f(glGetUniformLocation(shader, "fade"), fade);
		glUniform1i(glGetUniformLocation(shader, "src_tex"), 0);
		glUniform1i(glGetUniformLocation(shader, "dst_tex"), 1);
		if(BC_CModels::is_yuv(dst->get_color_model()))
			glUniform3f(glGetUniformLocation(shader, "chroma_offset"), 0.0, 0.5, 0.5);
		else
			glUniform3f(glGetUniformLocation(shader, "chroma_offset"), 0.0, 0.0, 0.0);
		glUniform2f(glGetUniformLocation(shader, "dst_tex_dimensions"),
			(float)dst->get_texture_w(),
			(float)dst->get_texture_h());
	}
	glDisable(GL_BLEND);
	src->draw_texture();
	glUseProgram(0);

	glDisable(GL_BLEND);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);

	dst->set_opengl_state(VFrame::SCREEN);
#endif
	return 0;
}



