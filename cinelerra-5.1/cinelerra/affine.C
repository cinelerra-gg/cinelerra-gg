
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

#ifdef HAVE_GL
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#endif

#include "affine.h"
#include "interp.h"
#include "clip.h"
#include "vframe.h"


#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

AffineMatrix::AffineMatrix()
{
	bzero(values, sizeof(values));
}

void AffineMatrix::identity()
{
	bzero(values, sizeof(values));
	values[0][0] = 1;
	values[1][1] = 1;
	values[2][2] = 1;
}

void AffineMatrix::translate(double x, double y)
{
	double g = values[2][0];
	double h = values[2][1];
	double i = values[2][2];
	values[0][0] += x * g;
	values[0][1] += x * h;
	values[0][2] += x * i;
	values[1][0] += y * g;
	values[1][1] += y * h;
	values[1][2] += y * i;
}

void AffineMatrix::scale(double x, double y)
{
	values[0][0] *= x;
	values[0][1] *= x;
	values[0][2] *= x;

	values[1][0] *= y;
	values[1][1] *= y;
	values[1][2] *= y;
}

void AffineMatrix::multiply(AffineMatrix *dst)
{
	AffineMatrix tmp;

	for( int i=0; i<3; ++i ) {
		double t1 = values[i][0], t2 = values[i][1], t3 = values[i][2];
		for( int j=0; j<3; ++j ) {
			tmp.values[i][j]  = t1 * dst->values[0][j];
			tmp.values[i][j] += t2 * dst->values[1][j];
			tmp.values[i][j] += t3 * dst->values[2][j];
		}
	}
	dst->copy_from(&tmp);
}

double AffineMatrix::determinant()
{
	double determinant;

	determinant  =
		values[0][0] * (values[1][1] * values[2][2] - values[1][2] * values[2][1]);
	determinant -=
		values[1][0] * (values[0][1] * values[2][2] - values[0][2] * values[2][1]);
	determinant +=
		values[2][0] * (values[0][1] * values[1][2] - values[0][2] * values[1][1]);

	return determinant;
}

void AffineMatrix::invert(AffineMatrix *dst)
{
	double det_1;

	det_1 = determinant();

	if(det_1 == 0.0)
	return;

	det_1 = 1.0 / det_1;

	dst->values[0][0] =
		(values[1][1] * values[2][2] - values[1][2] * values[2][1]) * det_1;

	dst->values[1][0] =
		- (values[1][0] * values[2][2] - values[1][2] * values[2][0]) * det_1;

	dst->values[2][0] =
		(values[1][0] * values[2][1] - values[1][1] * values[2][0]) * det_1;

	dst->values[0][1] =
		- (values[0][1] * values[2][2] - values[0][2] * values[2][1] ) * det_1;

	dst->values[1][1] =
		(values[0][0] * values[2][2] - values[0][2] * values[2][0]) * det_1;

	dst->values[2][1] =
		- (values[0][0] * values[2][1] - values[0][1] * values[2][0]) * det_1;

	dst->values[0][2] =
		(values[0][1] * values[1][2] - values[0][2] * values[1][1]) * det_1;

	dst->values[1][2] =
		- (values[0][0] * values[1][2] - values[0][2] * values[1][0]) * det_1;

	dst->values[2][2] =
		(values[0][0] * values[1][1] - values[0][1] * values[1][0]) * det_1;
}

void AffineMatrix::copy_from(AffineMatrix *src)
{
	memcpy(&values[0][0], &src->values[0][0], sizeof(values));
}

void AffineMatrix::set_matrix(
	double in_x1, double in_y1, double in_x2, double in_y2,
	double out_x1, double out_y1, double out_x2, double out_y2,
	double out_x3, double out_y3, double out_x4, double out_y4)
{
	double scalex = in_x2 > in_x1 ? 1./(in_x2 - in_x1) : 1.0;
	double scaley = in_y2 > in_y1 ? 1./(in_y2 - in_y1) : 1.0;
	double dx1 = out_x2 - out_x4, dx2 = out_x3 - out_x4;
	double dx3 = out_x1 - out_x2 + out_x4 - out_x3;

	double dy1 = out_y2 - out_y4, dy2 = out_y3 - out_y4;
	double dy3 = out_y1 - out_y2 + out_y4 - out_y3;
	double det = dx1 * dy2 - dy1 * dx2;
	if( !det ) { identity();  return; }

	AffineMatrix m;
	m.values[2][0] = (dx3 * dy2 - dy3 * dx2) / det;
	m.values[2][1] = (dx1 * dy3 - dy1 * dx3) / det;
	m.values[0][0] = out_x2 - out_x1 + m.values[2][0] * out_x2;
	m.values[0][1] = out_x3 - out_x1 + m.values[2][1] * out_x3;
	m.values[0][2] = out_x1;
	m.values[1][0] = out_y2 - out_y1 + m.values[2][0] * out_y2;
	m.values[1][1] = out_y3 - out_y1 + m.values[2][1] * out_y3;
	m.values[1][2] = out_y1;
	m.values[2][2] = 1.0;

	identity();
	translate(-in_x1, -in_y1);
	scale(scalex, scaley);
	m.multiply(this);
}

void AffineMatrix::transform_point(float x,
	float y,
	float *newx,
	float *newy)
{
	double w;

	w = values[2][0] * x + values[2][1] * y + values[2][2];
	w = !w ? 1 : 1/w;

	*newx = (values[0][0] * x + values[0][1] * y + values[0][2]) * w;
	*newy = (values[1][0] * x + values[1][1] * y + values[1][2]) * w;
}

void AffineMatrix::dump()
{
	printf("AffineMatrix::dump\n");
	printf("%f %f %f\n", values[0][0], values[0][1], values[0][2]);
	printf("%f %f %f\n", values[1][0], values[1][1], values[1][2]);
	printf("%f %f %f\n", values[2][0], values[2][1], values[2][2]);
}





AffinePackage::AffinePackage()
 : LoadPackage()
{
}




AffineUnit::AffineUnit(AffineEngine *server)
 : LoadClient(server)
{
	this->server = server;
}


static inline float transform_cubic(float dx,
		float p0, float p1, float p2, float p3)
{
/* Catmull-Rom - not bad */
	float result = ((( (- p0 + 3*p1 - 3*p2 + p3) * dx +
			 ( 2*p0 - 5*p1 + 4*p2 - p3 ) ) * dx +
			 ( - p0 + p2 ) ) * dx + (p1 + p1) ) / 2;
// printf("%f %f %f %f %f\n", result, p0, p1, p2, p3);
	return result;
}

static inline float transform_linear(float dx,
		float p1, float p2)
{
	float result = p1 * (1-dx) + p2 * dx;
	return result;
}


void AffineUnit::process_package(LoadPackage *package)
{
	AffinePackage *pkg = (AffinePackage*)package;
	int min_in_x = server->in_x;
	int min_in_y = server->in_y;
	int max_in_x = server->in_x + server->in_w - 1;
	int max_in_y = server->in_y + server->in_h - 1;


// printf("AffineUnit::process_package %d %d %d %d %d\n",
// __LINE__,
// min_in_x,
// min_in_y,
// max_in_x,
// max_in_y);
	int min_out_x = server->out_x;
	//int min_out_y = server->out_y;
	int max_out_x = server->out_x + server->out_w;
	//int max_out_y = server->out_y + server->out_h;

// Amount to shift the input coordinates relative to the output coordinates
// To get the pivots to line up
	int pivot_offset_x = server->in_pivot_x - server->out_pivot_x;
	int pivot_offset_y = server->in_pivot_y - server->out_pivot_y;

// Calculate real coords
	float out_x1, out_y1, out_x2, out_y2, out_x3, out_y3, out_x4, out_y4;
	if( server->mode == AffineEngine::STRETCH ||
	    server->mode == AffineEngine::PERSPECTIVE ||
	    server->mode == AffineEngine::ROTATE ||
	    server->mode == AffineEngine::TRANSFORM ) {
		out_x1 = (float)server->in_x + (float)server->x1 * server->in_w / 100;
		out_y1 = (float)server->in_y + (float)server->y1 * server->in_h / 100;
		out_x2 = (float)server->in_x + (float)server->x2 * server->in_w / 100;
		out_y2 = (float)server->in_y + (float)server->y2 * server->in_h / 100;
		out_x3 = (float)server->in_x + (float)server->x3 * server->in_w / 100;
		out_y3 = (float)server->in_y + (float)server->y3 * server->in_h / 100;
		out_x4 = (float)server->in_x + (float)server->x4 * server->in_w / 100;
		out_y4 = (float)server->in_y + (float)server->y4 * server->in_h / 100;
	}
	else {
		out_x1 = (float)server->in_x + (float)server->x1 * server->in_w / 100;
		out_y1 = server->in_y;
		out_x2 = out_x1 + server->in_w;
		out_y2 = server->in_y;
		out_x4 = (float)server->in_x + (float)server->x4 * server->in_w / 100;
		out_y4 = server->in_y + server->in_h;
		out_x3 = out_x4 + server->in_w;
		out_y3 = server->in_y + server->in_h;
	}



// Rotation with OpenGL uses a simple quad.
	if( server->mode == AffineEngine::ROTATE &&
	    server->use_opengl ) {
#ifdef HAVE_GL
		out_x1 -= pivot_offset_x;  out_y1 -= pivot_offset_y;
		out_x2 -= pivot_offset_x;  out_y2 -= pivot_offset_y;
		out_x3 -= pivot_offset_x;  out_y3 -= pivot_offset_y;
		out_x4 -= pivot_offset_x;  out_y4 -= pivot_offset_y;

		server->output->to_texture();
		server->output->enable_opengl();
		server->output->init_screen();
		server->output->bind_texture(0);
		server->output->clear_pbuffer();

		int texture_w = server->output->get_texture_w();
		int texture_h = server->output->get_texture_h();
		float output_h = server->output->get_h();
		float in_x1 = (float)server->in_x / texture_w;
		float in_x2 = (float)(server->in_x + server->in_w) / texture_w;
		float in_y1 = (float)server->in_y / texture_h;
		float in_y2 = (float)(server->in_y + server->in_h) / texture_h;

// printf("%f %f %f %f\n%f,%f %f,%f %f,%f %f,%f\n", in_x1, in_y1, in_x2, in_y2,
// out_x1, out_y1, out_x2, out_y2, out_x3, out_y3, out_x4, out_y4);

		glBegin(GL_QUADS);
		glNormal3f(0, 0, 1.0);

		glTexCoord2f(in_x1, in_y1);
		glVertex3f(out_x1, -output_h+out_y1, 0);

		glTexCoord2f(in_x2, in_y1);
		glVertex3f(out_x2, -output_h+out_y2, 0);

		glTexCoord2f(in_x2, in_y2);
		glVertex3f(out_x3, -output_h+out_y3, 0);

		glTexCoord2f(in_x1, in_y2);
		glVertex3f(out_x4, -output_h+out_y4, 0);


		glEnd();

		server->output->set_opengl_state(VFrame::SCREEN);
#endif
	}
	else
	if( server->mode == AffineEngine::PERSPECTIVE ||
	    server->mode == AffineEngine::SHEER ||
	    server->mode == AffineEngine::ROTATE ||
	    server->mode == AffineEngine::TRANSFORM ) {
		AffineMatrix matrix;
		float temp;
// swap points 3 & 4
		temp = out_x4;
		out_x4 = out_x3;
		out_x3 = temp;
		temp = out_y4;
		out_y4 = out_y3;
		out_y3 = temp;





		if( server->mode != AffineEngine::TRANSFORM ) {
			matrix.set_matrix(server->in_x, server->in_y,
				server->in_x + server->in_w,
				server->in_y + server->in_h,
				out_x1, out_y1, out_x2, out_y2,
				out_x3, out_y3, out_x4, out_y4);
		}
		else {
			matrix.copy_from(&server->matrix);
		}

//printf("AffineUnit::process_package %d\n%f %f %f\n%f %f %f\n%f %f %f\n", __LINE__,
// matrix.values[0][0], matrix.values[0][1], matrix.values[0][2],
// matrix.values[1][0], matrix.values[1][1], matrix.values[1][2],
// matrix.values[2][0], matrix.values[2][1], matrix.values[2][2]);
		int reverse = !server->forward;
		float tx, ty, tw;
		float xinc, yinc, winc;
		AffineMatrix m, im;
		float ttx = 0, tty = 0;
		int tx1 = 0, ty1 = 0, tx2 = 0, ty2 = 0;

		if(reverse) {
			m.copy_from(&matrix);
			m.invert(&im);
			matrix.copy_from(&im);
		}
		else {
			matrix.invert(&m);
		}

		float dx1 = 0, dy1 = 0;
		float dx2 = 0, dy2 = 0;
		float dx3 = 0, dy3 = 0;
		float dx4 = 0, dy4 = 0;
		matrix.transform_point(server->in_x, server->in_y, &dx1, &dy1);
		matrix.transform_point(server->in_x + server->in_w, server->in_y, &dx2, &dy2);
		matrix.transform_point(server->in_x, server->in_y + server->in_h, &dx3, &dy3);
		matrix.transform_point(server->in_x + server->in_w, server->in_y + server->in_h, &dx4, &dy4);

//printf("AffineUnit::process_package 1 y1=%d y2=%d\n", pkg->y1, pkg->y2);
//printf("AffineUnit::process_package 1 %f %f %f %f\n", dy1, dy2, dy3, dy4);
// printf("AffineUnit::process_package %d use_opengl=%d\n",
// __LINE__, server->use_opengl);

		if( server->use_opengl &&
		    server->interpolation == AffineEngine::AF_DEFAULT ) {
#ifdef HAVE_GL
			static const char *affine_frag =
				"uniform sampler2D tex;\n"
				"uniform mat3 affine_matrix;\n"
				"uniform vec2 texture_extents;\n"
				"uniform vec2 image_extents;\n"
				"uniform vec4 border_color;\n"
				"void main()\n"
				"{\n"
				"	vec2 outcoord = gl_TexCoord[0].st;\n"
				"	outcoord *= texture_extents;\n"
				"	mat3 coord_matrix = mat3(\n"
				"		outcoord.x, outcoord.y, 1.0, \n"
				"		outcoord.x, outcoord.y, 1.0, \n"
				"		outcoord.x, outcoord.y, 1.0);\n"
				"	mat3 incoord_matrix = affine_matrix * coord_matrix;\n"
				"	vec2 incoord = vec2(incoord_matrix[0][0], incoord_matrix[0][1]);\n"
				"	incoord /= incoord_matrix[0][2];\n"
			 	"	incoord /= texture_extents;\n"
				"	if(incoord.x > image_extents.x || incoord.y > image_extents.y)\n"
				"		gl_FragColor = border_color;\n"
				"	else\n"
			 	"		gl_FragColor = texture2D(tex, incoord);\n"
				"}\n";

			float affine_matrix[9] = {
				(float)m.values[0][0], (float)m.values[1][0], (float)m.values[2][0],
				(float)m.values[0][1], (float)m.values[1][1], (float)m.values[2][1],
				(float)m.values[0][2], (float)m.values[1][2], (float)m.values[2][2]
			};


			server->output->to_texture();
			server->output->enable_opengl();
			unsigned int frag_shader = VFrame::make_shader(0, affine_frag, 0);
			if( frag_shader > 0 ) {
				glUseProgram(frag_shader);
				glUniform1i(glGetUniformLocation(frag_shader, "tex"), 0);
				glUniformMatrix3fv(glGetUniformLocation(frag_shader, "affine_matrix"),
					1, 0, affine_matrix);
				glUniform2f(glGetUniformLocation(frag_shader, "texture_extents"),
					(GLfloat)server->output->get_texture_w(),
					(GLfloat)server->output->get_texture_h());
				glUniform2f(glGetUniformLocation(frag_shader, "image_extents"),
					(GLfloat)server->output->get_w() / server->output->get_texture_w(),
					(GLfloat)server->output->get_h() / server->output->get_texture_h());
				float border_color[] = { 0, 0, 0, 0 };
				if(BC_CModels::is_yuv(server->output->get_color_model())) {
					border_color[1] = 0.5;
					border_color[2] = 0.5;
				}
				if(!BC_CModels::has_alpha(server->output->get_color_model())) {
					border_color[3] = 1.0;
				}

				glUniform4fv(glGetUniformLocation(frag_shader, "border_color"),
					1, (GLfloat*)border_color);
				server->output->init_screen();
				server->output->bind_texture(0);
				glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
				glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				server->output->draw_texture();
				glUseProgram(0);
				server->output->set_opengl_state(VFrame::SCREEN);
			}
			return;
#endif // HAVE_GL
		}

#define ROUND(x) ((int)((x > 0) ? (x) + 0.5 : (x) - 0.5))
#define MIN4(a,b,c,d) MIN(MIN(MIN(a,b),c),d)
#define MAX4(a,b,c,d) MAX(MAX(MAX(a,b),c),d)

	tx1 = ROUND(MIN4(dx1 - pivot_offset_x, dx2 - pivot_offset_x, dx3 - pivot_offset_x, dx4 - pivot_offset_x));
	ty1 = ROUND(MIN4(dy1 - pivot_offset_y, dy2 - pivot_offset_y, dy3 - pivot_offset_y, dy4 - pivot_offset_y));

	tx2 = ROUND(MAX4(dx1 - pivot_offset_x, dx2 - pivot_offset_x, dx3 - pivot_offset_x, dx4 - pivot_offset_x));
	ty2 = ROUND(MAX4(dy1 - pivot_offset_y, dy2 - pivot_offset_y, dy3 - pivot_offset_y, dy4 - pivot_offset_y));

	CLAMP(ty1, pkg->y1, pkg->y2);
	CLAMP(ty2, pkg->y1, pkg->y2);
	CLAMP(tx1, server->out_x, server->out_x + server->out_w);
	CLAMP(tx2, server->out_x, server->out_x + server->out_w);


	xinc = m.values[0][0];
	yinc = m.values[1][0];
	winc = m.values[2][0];

//printf("AffineUnit::process_package 2 tx1=%d ty1=%d tx2=%d ty2=%d %f %f\n",
// tx1, ty1, tx2, ty2, out_x4, out_y4);
//printf("AffineUnit::process_package %d %d %d %d %d\n",
// __LINE__, min_in_x, max_in_x, min_in_y, max_in_y);

#define DO_INTERP(tag, interp, components, type, temp_type, chroma, max) \
case tag: { \
    type **inp_rows = (type**)server->input->get_rows(); \
    type **out_rows = (type**)server->output->get_rows(); \
    float round_factor = sizeof(type) < 4 ? 0.5 : 0; \
    INTERP_SETUP(inp_rows, max, min_in_x,min_in_y, max_in_x,max_in_y); \
 \
    for( int y=ty1; y<ty2; ++y ) { \
        type *out_row = (type*)out_rows[y]; \
        int x1 = tx1, x2 = tx2; \
        if( x1 < min_out_x ) x1 = min_out_x; \
        if( x2 > max_out_x ) x2 = max_out_x; \
        tx = xinc * x1 + m.values[0][1] * (y + pivot_offset_y) + m.values[0][2] \
            + pivot_offset_x * xinc; \
        ty = yinc * x1 + m.values[1][1] * (y + pivot_offset_y) + m.values[1][2] \
            + pivot_offset_x * yinc; \
        tw = winc * x1 + m.values[2][1] * (y + pivot_offset_y) + m.values[2][2] \
            + pivot_offset_x * winc; \
        type *out = out_row + x1 * components; \
 \
        for( int x=x1; x<x2; ++x ) { \
/* Normalize homogeneous coords */ \
            if( tw == 0.0 ) { ttx = 0.0; tty = 0.0; } \
            else { ttx = tx / tw; tty = ty / tw; } \
            interp##_SETUP(type, components, ttx, tty); \
            *out++ = ((temp_type)interp##_interp(0, 0) + round_factor); \
            interp##_next(); \
            *out++ = ((temp_type)interp##_interp(chroma, chroma) + round_factor); \
            interp##_next(); \
            *out++ = ((temp_type)interp##_interp(chroma, chroma) + round_factor); \
            if( components == 4 ) { \
                interp##_next(); \
                *out++ = ((temp_type)interp##_interp(0, 0) + round_factor); \
            } \
 \
/*  increment the transformed coordinates  */ \
            tx += xinc;  ty += yinc;  tw += winc; \
        } \
    } \
} break

// printf("AffineUnit::process_package %d tx1=%d ty1=%d tx2=%d ty2=%d\n",
// __LINE__, tx1, ty1, tx2, ty2);

		switch( server->interpolation ) {
		case AffineEngine::AF_NEAREST:
			switch( server->input->get_color_model() ) {
			DO_INTERP( BC_RGB_FLOAT, nearest, 3, float, float, 0x0, 1.0);
			DO_INTERP( BC_RGB888, nearest, 3, unsigned char, int, 0x0, 0xff);
			DO_INTERP( BC_RGBA_FLOAT, nearest, 4, float, float, 0x0, 1.0);
			DO_INTERP( BC_RGBA8888, nearest, 4, unsigned char, int, 0x0, 0xff);
			DO_INTERP( BC_YUV888, nearest, 3, unsigned char, int, 0x80, 0xff);
			DO_INTERP( BC_YUVA8888, nearest, 4, unsigned char, int, 0x80, 0xff);
			DO_INTERP( BC_RGB161616, nearest, 3, uint16_t, int, 0x0, 0xffff);
			DO_INTERP( BC_RGBA16161616, nearest, 4, uint16_t, int, 0x0, 0xffff);
			DO_INTERP( BC_YUV161616, nearest, 3, uint16_t, int, 0x8000, 0xffff);
			DO_INTERP( BC_YUVA16161616, nearest, 4, uint16_t, int, 0x8000, 0xffff);
			}
			break;
		case AffineEngine::AF_LINEAR:
			switch( server->input->get_color_model() ) {
			DO_INTERP( BC_RGB_FLOAT, bi_linear, 3, float, float, 0x0, 1.0);
			DO_INTERP( BC_RGB888, bi_linear, 3, unsigned char, int, 0x0, 0xff);
			DO_INTERP( BC_RGBA_FLOAT, bi_linear, 4, float, float, 0x0, 1.0);
			DO_INTERP( BC_RGBA8888, bi_linear, 4, unsigned char, int, 0x0, 0xff);
			DO_INTERP( BC_YUV888, bi_linear, 3, unsigned char, int, 0x80, 0xff);
			DO_INTERP( BC_YUVA8888, bi_linear, 4, unsigned char, int, 0x80, 0xff);
			DO_INTERP( BC_RGB161616, bi_linear, 3, uint16_t, int, 0x0, 0xffff);
			DO_INTERP( BC_RGBA16161616, bi_linear, 4, uint16_t, int, 0x0, 0xffff);
			DO_INTERP( BC_YUV161616, bi_linear, 3, uint16_t, int, 0x8000, 0xffff);
			DO_INTERP( BC_YUVA16161616, bi_linear, 4, uint16_t, int, 0x8000, 0xffff);
			}
			break;
		default:
		case AffineEngine::AF_CUBIC:
			switch( server->input->get_color_model() ) {
			DO_INTERP( BC_RGB_FLOAT, bi_cubic, 3, float, float, 0x0, 1.0);
			DO_INTERP( BC_RGB888, bi_cubic, 3, unsigned char, int, 0x0, 0xff);
			DO_INTERP( BC_RGBA_FLOAT, bi_cubic, 4, float, float, 0x0, 1.0);
			DO_INTERP( BC_RGBA8888, bi_cubic, 4, unsigned char, int, 0x0, 0xff);
			DO_INTERP( BC_YUV888, bi_cubic, 3, unsigned char, int, 0x80, 0xff);
			DO_INTERP( BC_YUVA8888, bi_cubic, 4, unsigned char, int, 0x80, 0xff);
			DO_INTERP( BC_RGB161616, bi_cubic, 3, uint16_t, int, 0x0, 0xffff);
			DO_INTERP( BC_RGBA16161616, bi_cubic, 4, uint16_t, int, 0x0, 0xffff);
			DO_INTERP( BC_YUV161616, bi_cubic, 3, uint16_t, int, 0x8000, 0xffff);
			DO_INTERP( BC_YUVA16161616, bi_cubic, 4, uint16_t, int, 0x8000, 0xffff);
			}
			break;
		}
	}
	else
	{
		int min_x = server->in_x * AFFINE_OVERSAMPLE;
		int min_y = server->in_y * AFFINE_OVERSAMPLE;
		int max_x = server->in_x * AFFINE_OVERSAMPLE + server->in_w * AFFINE_OVERSAMPLE - 1;
		int max_y = server->in_y * AFFINE_OVERSAMPLE + server->in_h * AFFINE_OVERSAMPLE - 1;
		float top_w = out_x2 - out_x1;
		float bottom_w = out_x3 - out_x4;
		float left_h = out_y4 - out_y1;
		float right_h = out_y3 - out_y2;
		float out_w_diff = bottom_w - top_w;
		float out_left_diff = out_x4 - out_x1;
		float out_h_diff = right_h - left_h;
		float out_top_diff = out_y2 - out_y1;
		float distance1 = DISTANCE(out_x1, out_y1, out_x2, out_y2);
		float distance2 = DISTANCE(out_x2, out_y2, out_x3, out_y3);
		float distance3 = DISTANCE(out_x3, out_y3, out_x4, out_y4);
		float distance4 = DISTANCE(out_x4, out_y4, out_x1, out_y1);
		float max_v = MAX(distance1, distance3);
		float max_h = MAX(distance2, distance4);
		float max_dimension = MAX(max_v, max_h);
		float min_dimension = MIN(server->in_h, server->in_w);
		float step = min_dimension / max_dimension / AFFINE_OVERSAMPLE;
		float x_f = server->in_x;
		float y_f = server->in_y;
		float h_f = server->in_h;
		float w_f = server->in_w;

		if(server->use_opengl) {
			return;
		}

// Projection
#define DO_STRETCH(tag, type, components) \
case tag: { \
	type **in_rows = (type**)server->input->get_rows(); \
	type **out_rows = (type**)server->temp->get_rows(); \
 \
	for(float in_y = pkg->y1; in_y < pkg->y2; in_y += step) \
	{ \
		int i = (int)in_y; \
		type *in_row = in_rows[i]; \
		for(float in_x = x_f; in_x < w_f; in_x += step) \
		{ \
			int j = (int)in_x; \
			float in_x_fraction = (in_x - x_f) / w_f; \
			float in_y_fraction = (in_y - y_f) / h_f; \
			int out_x = (int)((out_x1 + \
				out_left_diff * in_y_fraction + \
				(top_w + out_w_diff * in_y_fraction) * in_x_fraction) *  \
				AFFINE_OVERSAMPLE); \
			int out_y = (int)((out_y1 +  \
				out_top_diff * in_x_fraction + \
				(left_h + out_h_diff * in_x_fraction) * in_y_fraction) * \
				AFFINE_OVERSAMPLE); \
			CLAMP(out_x, min_x, max_x); \
			CLAMP(out_y, min_y, max_y); \
			type *dst = out_rows[out_y] + out_x * components; \
			type *src = in_row + j * components; \
			dst[0] = src[0]; \
			dst[1] = src[1]; \
			dst[2] = src[2]; \
			if(components == 4) dst[3] = src[3]; \
		} \
	} \
} break

		switch( server->input->get_color_model() ) {
		DO_STRETCH( BC_RGB_FLOAT, float, 3 );
		DO_STRETCH( BC_RGB888, unsigned char, 3 );
		DO_STRETCH( BC_RGBA_FLOAT, float, 4 );
		DO_STRETCH( BC_RGBA8888, unsigned char, 4 );
		DO_STRETCH( BC_YUV888, unsigned char, 3 );
		DO_STRETCH( BC_YUVA8888, unsigned char, 4 );
		DO_STRETCH( BC_RGB161616, uint16_t, 3 );
		DO_STRETCH( BC_RGBA16161616, uint16_t, 4 );
		DO_STRETCH( BC_YUV161616, uint16_t, 3 );
		DO_STRETCH( BC_YUVA16161616, uint16_t, 4 );
		}
	}
}


AffineEngine::AffineEngine(int total_clients, int total_packages)
 : LoadServer(total_clients, total_packages) //(1, 1)
{
	user_in_viewport = 0;
	user_in_pivot = 0;
	user_out_viewport = 0;
	user_out_pivot = 0;
	use_opengl = 0;
	in_x = in_y = in_w = in_h = 0;
	out_x = out_y = out_w = out_h = 0;
	in_pivot_x = in_pivot_y = 0;
	out_pivot_x = out_pivot_y = 0;
	interpolation = AF_DEFAULT;
	this->total_packages = total_packages;
}

void AffineEngine::init_packages()
{
	int y1 = 0, npkgs = get_total_packages();
	for( int i=0; i<npkgs; ) {
		AffinePackage *package = (AffinePackage*)get_package(i);
		int y2 = out_y + (out_h * ++i / npkgs);
		package->y1 = y1;  package->y2 = y2;  y1 = y2;
	}
}

LoadClient* AffineEngine::new_client()
{
	return new AffineUnit(this);
}

LoadPackage* AffineEngine::new_package()
{
	return new AffinePackage;
}

void AffineEngine::process(VFrame *output, VFrame *input, VFrame *temp, int mode,
	float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4,
	int forward)
{


// printf("AffineEngine::process %d %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n",
// __LINE__, x1, y1, x2, y2, x3, y3, x4, y4);
//
// printf("AffineEngine::process %d %d %d %d %d\n",
// __LINE__, in_x, in_y, in_w, in_h);
//
// printf("AffineEngine::process %d %d %d %d %d\n",
// __LINE__, out_x, out_y, out_w, out_h);
//
// printf("AffineEngine::process %d %d %d %d %d\n",
// __LINE__, in_pivot_x, in_pivot_y, out_pivot_x, out_pivot_y);
//
// printf("AffineEngine::process %d %d %d %d %d\n",
// __LINE__, user_in_pivot, user_out_pivot, user_in_viewport, user_out_viewport);

	this->output = output;
	this->input = input;
	this->temp = temp;
	this->mode = mode;
	this->x1 = x1;  this->y1 = y1;
	this->x2 = x2;  this->y2 = y2;
	this->x3 = x3;  this->y3 = y3;
	this->x4 = x4;  this->y4 = y4;
	this->forward = forward;

	if(!user_in_viewport) {
		in_x = 0;  in_y = 0;
		in_w = input->get_w();
		in_h = input->get_h();
	}

	if(!user_out_viewport) {
		out_x = 0;  out_y = 0;
		out_w = output->get_w();
		out_h = output->get_h();
	}

	if(use_opengl) {
		set_package_count(1);
		process_single();
	}
	else {
		set_package_count(total_packages);
		process_packages();
	}
}

void AffineEngine::set_matrix(
	double in_x1, double in_y1, double in_x2, double in_y2,
	double out_x1, double out_y1, double out_x2, double out_y2,
	double out_x3, double out_y3, double out_x4, double out_y4)
{
	matrix.set_matrix(in_x1, in_y1, in_x2, in_y2,
		out_x1, out_y1, out_x2, out_y2, out_x3, out_y3, out_x4, out_y4);
}


void AffineEngine::rotate(VFrame *output,
	VFrame *input,
	float angle)
{
	this->output = output;
	this->input = input;
	this->temp = 0;
	this->mode = ROTATE;
	this->forward = 1;

	if( !user_in_viewport ) {
		in_x = 0;  in_y = 0;
		in_w = input->get_w();
		in_h = input->get_h();
// DEBUG
// in_x = 4;
// in_w = 2992;
// in_y = 4;
// in_h = 2992;
// printf("AffineEngine::rotate %d %d %d %d %d\n", __LINE__, in_x, in_w, in_y, in_h);
	}

	if( !user_in_pivot ) {
		in_pivot_x = in_x + in_w / 2;
		in_pivot_y = in_y + in_h / 2;
	}

	if( !user_out_viewport ) {
		out_x = 0;  out_y = 0;
		out_w = output->get_w();
		out_h = output->get_h();
	}

	if( !user_out_pivot ) {
		out_pivot_x = out_x + out_w / 2;
		out_pivot_y = out_y + out_h / 2;
	}

// All subscripts are clockwise around the quadrangle
	angle = angle * 2 * M_PI / 360;
	double angle1 = atan2((double)(in_pivot_y - in_y), (double)(in_pivot_x - in_x)) + angle;
	double angle2 = atan2((double)(in_x + in_w - in_pivot_x), (double)(in_pivot_y - in_y)) + angle;
	double angle3 = atan2((double)(in_y + in_h - in_pivot_y), (double)(in_x + in_w - in_pivot_x)) + angle;
	double angle4 = atan2((double)(in_pivot_x - in_x), (double)(in_y + in_h - in_pivot_y)) + angle;
	double radius1 = DISTANCE(in_x, in_y, in_pivot_x, in_pivot_y);
	double radius2 = DISTANCE(in_x + in_w, in_y, in_pivot_x, in_pivot_y);
	double radius3 = DISTANCE(in_x + in_w, in_y + in_h, in_pivot_x, in_pivot_y);
	double radius4 = DISTANCE(in_x, in_y + in_h, in_pivot_x, in_pivot_y);

	x1 = ((in_pivot_x - in_x) - cos(angle1) * radius1) * 100 / in_w;
	y1 = ((in_pivot_y - in_y) - sin(angle1) * radius1) * 100 / in_h;
	x2 = ((in_pivot_x - in_x) + sin(angle2) * radius2) * 100 / in_w;
	y2 = ((in_pivot_y - in_y) - cos(angle2) * radius2) * 100 / in_h;
	x3 = ((in_pivot_x - in_x) + cos(angle3) * radius3) * 100 / in_w;
	y3 = ((in_pivot_y - in_y) + sin(angle3) * radius3) * 100 / in_h;
	x4 = ((in_pivot_x - in_x) - sin(angle4) * radius4) * 100 / in_w;
	y4 = ((in_pivot_y - in_y) + cos(angle4) * radius4) * 100 / in_h;

// printf("AffineEngine::rotate angle=%f\n",
// angle);

//
// printf("	angle1=%f angle2=%f angle3=%f angle4=%f\n",
// angle1 * 360 / 2 / M_PI,  angle2 * 360 / 2 / M_PI,
// angle3 * 360 / 2 / M_PI,  angle4 * 360 / 2 / M_PI);
//
// printf("	radius1=%f radius2=%f radius3=%f radius4=%f\n",
// radius1, radius2, radius3, radius4);
//
// printf("	x1=%f y1=%f x2=%f y2=%f x3=%f y3=%f x4=%f y4=%f\n",
// x1 * w / 100, y1 * h / 100,
// x2 * w / 100, y2 * h / 100,
// x3 * w / 100, y3 * h / 100,
// x4 * w / 100, y4 * h / 100);

	if(use_opengl) {
		set_package_count(1);
		process_single();
	}
	else {
		set_package_count(total_packages);
		process_packages();
	}
}

void AffineEngine::set_in_viewport(int x, int y, int w, int h)
{
	this->in_x = x;  this->in_y = y;
	this->in_w = w;  this->in_h = h;
	this->user_in_viewport = 1;
}

void AffineEngine::set_out_viewport(int x, int y, int w, int h)
{
	this->out_x = x;  this->out_y = y;
	this->out_w = w;  this->out_h = h;
	this->user_out_viewport = 1;
}

void AffineEngine::set_viewport(int x, int y, int w, int h)
{
	set_in_viewport(x, y, w, h);
	set_out_viewport(x, y, w, h);
}

void AffineEngine::set_opengl(int value)
{
	this->use_opengl = value;
}

void AffineEngine::set_in_pivot(int x, int y)
{
	this->in_pivot_x = x;
	this->in_pivot_y = y;
	this->user_in_pivot = 1;
}

void AffineEngine::set_out_pivot(int x, int y)
{
	this->out_pivot_x = x;
	this->out_pivot_y = y;
	this->user_out_pivot = 1;
}

void AffineEngine::set_pivot(int x, int y)
{
	set_in_pivot(x, y);
	set_out_pivot(x, y);
}

void AffineEngine::unset_pivot()
{
	user_in_pivot = 0;
	user_out_pivot = 0;
}

void AffineEngine::unset_viewport()
{
	user_in_viewport = 0;
	user_out_viewport = 0;
}


void AffineEngine::set_interpolation(int type)
{
	interpolation = type;
}
