
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

#define GL_GLEXT_PROTOTYPES

#include "bcpbuffer.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bctexture.h"
#include "bcwindowbase.h"
#include "vframe.h"

#ifdef HAVE_GL
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int VFrame::get_opengl_state()
{
	return opengl_state;
}

void VFrame::set_opengl_state(int value)
{
	opengl_state = value;
}

int VFrame::get_window_id()
{
	return texture ? texture->window_id : -1;
}

int VFrame::get_texture_id()
{
	return texture ? texture->texture_id : -1;
}

int VFrame::get_texture_w()
{
	return texture ? texture->texture_w : 0;
}

int VFrame::get_texture_h()
{
	return texture ? texture->texture_h : 0;
}


int VFrame::get_texture_components()
{
	return texture ? texture->texture_components : 0;
}











void VFrame::to_texture()
{
#ifdef HAVE_GL

// Must be here so user can create textures without copying data by setting
// opengl_state to TEXTURE.
	BC_Texture::new_texture(&texture, get_w(), get_h(), get_color_model());

// Determine what to do based on state
	switch(opengl_state) {
	case VFrame::TEXTURE:
		return;

	case VFrame::SCREEN:
		if(pbuffer) {
			enable_opengl();
			screen_to_texture();
		}
		opengl_state = VFrame::TEXTURE;
		return;
	}

//printf("VFrame::to_texture %d\n", texture_id);
	GLenum type, format;
	switch(color_model) {
	case BC_RGB888:
	case BC_YUV888:
		type = GL_UNSIGNED_BYTE;
		format = GL_RGB;
		break;

	case BC_RGBA8888:
	case BC_YUVA8888:
		type = GL_UNSIGNED_BYTE;
		format = GL_RGBA;
		break;

	case BC_RGB_FLOAT:
		type = GL_FLOAT;
		format = GL_RGB;
		break;

	case BC_RGBA_FLOAT:
		format = GL_RGBA;
		type = GL_FLOAT;
		break;

	default:
		fprintf(stderr,
			"VFrame::to_texture: unsupported color model %d.\n",
			color_model);
		return;
	}

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, get_w(), get_h(),
		format, type, get_rows()[0]);
	opengl_state = VFrame::TEXTURE;
#endif
}

void VFrame::create_pbuffer()
{
	int ww = (get_w()+3) & ~3, hh = (get_h()+3) & ~3;
	if( pbuffer && (pbuffer->w != ww || pbuffer->h != hh ||
	    pbuffer->window_id != BC_WindowBase::get_synchronous()->current_window->get_id() ) ) {
		delete pbuffer;
		pbuffer = 0;
	}

	if( !pbuffer ) {
		pbuffer = new BC_PBuffer(ww, hh);
	}
}

void VFrame::enable_opengl()
{
	create_pbuffer();
	if( pbuffer ) {
		pbuffer->enable_opengl();
	}
}

BC_PBuffer* VFrame::get_pbuffer()
{
	return pbuffer;
}


void VFrame::screen_to_texture(int x, int y, int w, int h)
{
#ifdef HAVE_GL
// Create texture
	BC_Texture::new_texture(&texture,
		get_w(), get_h(), get_color_model());

	if(pbuffer) {
		glEnable(GL_TEXTURE_2D);

// Read canvas into texture, use back texture for DOUBLE_BUFFER
#if 1
// According to the man page, it must be GL_BACK for the onscreen buffer
// and GL_FRONT for a single buffered PBuffer.  In reality it must be
// GL_BACK for a single buffered PBuffer if the PBuffer has alpha and using
// GL_FRONT captures the onscreen front buffer.
// 10/11/2010 is now generating "illegal operation"
		glReadBuffer(GL_BACK);
#else
		glReadBuffer(BC_WindowBase::get_synchronous()->is_pbuffer ?
			GL_FRONT : GL_BACK);
#endif
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
			x >= 0 ? x : 0, y >= 0 ? y : 0,
			w >= 0 ? w : get_w(), h >= 0 ? h : get_h());
	}
#endif
}

void VFrame::screen_to_ram()
{
#ifdef HAVE_GL
	enable_opengl();
	glReadBuffer(GL_BACK);
	int type = BC_CModels::is_float(color_model) ? GL_FLOAT : GL_UNSIGNED_BYTE;
	int format = BC_CModels::has_alpha(color_model) ? GL_RGBA : GL_RGB;
	glReadPixels(0, 0, get_w(), get_h(), format, type, get_rows()[0]);
	opengl_state = VFrame::RAM;
#endif
}

void VFrame::draw_texture(
	float in_x1, float in_y1, float in_x2, float in_y2,
	float out_x1, float out_y1, float out_x2, float out_y2,
	int flip_y)
{
#ifdef HAVE_GL
	in_x1 /= get_texture_w();  in_y1 /= get_texture_h();
	in_x2 /= get_texture_w();  in_y2 /= get_texture_h();
	float ot_y1 = flip_y ? -out_y1 : -out_y2;
	float ot_y2 = flip_y ? -out_y2 : -out_y1;
	texture->draw_texture(
		in_x1,in_y1,  in_x2,in_y2,
		out_x1,ot_y1, out_x2, ot_y2);
#endif
}

void VFrame::draw_texture(int flip_y)
{
	draw_texture(0,0,  get_w(),get_h(),
		0,0, get_w(),get_h(), flip_y);
}


void VFrame::bind_texture(int texture_unit)
{
// Bind the texture
	if(texture)
	{
		texture->bind(texture_unit);
	}
}






void VFrame::init_screen(int w, int h)
{
#ifdef HAVE_GL
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float near = 1;
	float far = 100;
	float frustum_ratio = near / ((near + far)/2);
 	float near_h = (float)h * frustum_ratio;
	float near_w = (float)w * frustum_ratio;
	glFrustum(-near_w/2, near_w/2, -near_h/2, near_h/2, near, far);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
// Shift down and right so 0,0 is the top left corner
	glTranslatef(-(w-1)/2.f, (h-1)/2.f, 0.0);
	glTranslatef(0.0, 0.0, -(far + near) / 2);

	glDisable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
// Default for direct copy playback
	glDisable(GL_BLEND);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_CULL_FACE);
	glEnable(GL_NORMALIZE);
	glAlphaFunc(GL_ALWAYS, 0);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_LIGHTING);

	const GLfloat zero[] = { 0, 0, 0, 0 };
//	const GLfloat one[] = { 1, 1, 1, 1 };
//	const GLfloat light_position[] = { 0, 0, -1, 0 };
//	const GLfloat light_direction[] = { 0, 0, 1, 0 };

// 	glEnable(GL_LIGHT0);
// 	glLightfv(GL_LIGHT0, GL_AMBIENT, zero);
// 	glLightfv(GL_LIGHT0, GL_DIFFUSE, one);
// 	glLightfv(GL_LIGHT0, GL_SPECULAR, one);
// 	glLighti(GL_LIGHT0, GL_SPOT_CUTOFF, 180);
// 	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
// 	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_direction);
// 	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
// 	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, zero);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0);
#endif
}

void VFrame::init_screen()
{
	init_screen(get_w(), get_h());
}

static int print_error(char *source, unsigned int object, int is_program)
{
#ifdef HAVE_GL
    char string[BCTEXTLEN];
	int len = 0;
    if(is_program)
		glGetProgramInfoLog(object, BCTEXTLEN, &len, string);
	else
		glGetShaderInfoLog(object, BCTEXTLEN, &len, string);
	if(len > 0) printf("Playback3D::print_error:\n%s\n%s\n", source, string);
	if(len > 0) return 1;
#endif
	return 0;
}



// call as:
//    make_shader(0, frag1, .., fragn, 0);
// or make_shader(fragments);

unsigned int VFrame::make_shader(const char **fragments, ...)
{
	unsigned int result = 0;
#ifdef HAVE_GL
// Construct single source file out of arguments
	char *program = 0;
	int nb_mains = 0;

	int nb_frags = 1;
	if( !fragments ) {
		va_list list;  va_start(list, fragments);
		while( va_arg(list, char*) != 0 ) ++nb_frags;
		va_end(list);
	}
	const char *frags[nb_frags], *text = 0;
	if( !fragments ) {
		va_list list;  va_start(list, fragments);
		for( int i=0; i<nb_frags; ++i ) frags[i] = va_arg(list, char*);
		va_end(list);
		fragments = frags;
	}

	while( (text = *fragments++) ) {
		char src[strlen(text) + BCSTRLEN + 1];
		const char *tp = strstr(text, "main()");
		if( tp ) {
// Replace main() with a mainxxx()
			char mainxxx[BCSTRLEN], *sp = src;
			sprintf(mainxxx, "main%03d()", nb_mains++);
			int n = tp - text;
			memcpy(sp, text, n);  sp += n;
			n = strlen(mainxxx);
			memcpy(sp, mainxxx, n);  sp += n;
			tp += strlen("main()");
			strcpy(sp, tp);
			text = src;
		}

		char *new_program = !program ? cstrdup(text) :
			cstrcat(2, program, text);
		delete [] program;  program = new_program;
	}

// Add main() which calls mainxxx() in order
	char main_program[BCTEXTLEN], *cp = main_program;
	cp += sprintf(cp, "\nvoid main() {\n");
	for( int i=0; i < nb_mains; ++i )
		cp += sprintf(cp, "\tmain%03d();\n", i);
	cp += sprintf(cp, "}\n");
	cp = !program ? cstrdup(main_program) :
		cstrcat(2, program, main_program);
	delete [] program;  program = cp;

	int got_it = 0;
	result = BC_WindowBase::get_synchronous()->get_shader(program, &got_it);
	if( !got_it ) {
		result = glCreateProgram();
		unsigned int shader = glCreateShader(GL_FRAGMENT_SHADER);
		const GLchar *text_ptr = program;
		glShaderSource(shader, 1, &text_ptr, NULL);
		glCompileShader(shader);
		int error = print_error(program, shader, 0);
		glAttachShader(result, shader);
		glDeleteShader(shader);
		glLinkProgram(result);
		if( !error )
			error = print_error(program, result, 1);
//printf("BC_WindowBase::make_shader: shader=%d window_id=%d\n", result,
// BC_WindowBase::get_synchronous()->current_window->get_id());
		BC_WindowBase::get_synchronous()->put_shader(result, program);
	}

//printf("VFrame::make_shader\n%s\n", program);
	delete [] program;
#endif
	return result;
}

void VFrame::dump_shader(int shader_id)
{
	BC_WindowBase::get_synchronous()->dump_shader(shader_id);
}


void VFrame::clear_pbuffer()
{
#ifdef HAVE_GL
	float gbuv = BC_CModels::is_yuv(get_color_model()) ? 0.5 : 0;
	glClearColor(0.0, gbuv, gbuv, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

