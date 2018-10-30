
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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

#ifndef PLAYBACK3D_H
#define PLAYBACK3D_H

#include "arraylist.h"
#include "bccolors.h"
#include "bcpixmap.inc"
#include "bcsynchronous.h"
#include "bcwindowbase.inc"
#include "canvas.inc"
#include "condition.inc"
#include "maskauto.inc"
#include "maskautos.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "pluginclient.inc"
#include "thread.h"
#include "vframe.inc"




// use static presets YUV in bccolors.h
#define BC_GL_MATRIX(shader, mat) \
	glUniformMatrix3fv(glGetUniformLocation(shader, #mat), 1, 1, YUV::mat)
#define BC_GL_VECTOR(shader, vec) \
	glUniform3fv(glGetUniformLocation(shader, #vec), 1, YUV::vec)

#define BC_GL_YMINF(shader) \
	glUniform1f(glGetUniformLocation(shader, "yminf"), YUV::yuv.get_yminf())

#define BC_GL_RGB_TO_YUV(shader) do { \
	BC_GL_MATRIX(shader, rgb_to_yuv_matrix); \
	BC_GL_YMINF(shader); \
} while(0)

#define BC_GL_RGB_TO_Y(shader) do { \
	BC_GL_VECTOR(shader, rgb_to_y_vector); \
	BC_GL_YMINF(shader); \
} while(0)

#define BC_GL_YUV_TO_RGB(shader) do { \
	BC_GL_MATRIX(shader, yuv_to_rgb_matrix); \
	BC_GL_YMINF(shader); \
} while(0)

#define BC_GL_COLORS(shader) do { \
	BC_GL_MATRIX(shader, yuv_to_rgb_matrix); \
	BC_GL_MATRIX(shader, rgb_to_yuv_matrix); \
	BC_GL_YMINF(shader); \
} while(0)


#define bc_gl_yuv_to_rgb "uniform mat3 yuv_to_rgb_matrix;\n"
#define bc_gl_rgb_to_yuv "uniform mat3 rgb_to_yuv_matrix;\n"
#define bc_gl_rgb_to_y   "uniform vec3 rgb_to_y_vector;\n"
#define bc_gl_yminf      "uniform float yminf;\n"
#define bc_gl_colors bc_gl_yuv_to_rgb bc_gl_rgb_to_yuv bc_gl_yminf

// Macros for useful fragment shaders
#define YUV_TO_RGB_FRAG(PIXEL) \
	PIXEL ".rgb -= vec3(yminf, 0.5, 0.5);\n" \
	PIXEL ".rgb = yuv_to_rgb_matrix * " PIXEL ".rgb;\n"

#define RGB_TO_YUV_FRAG(PIXEL) \
	PIXEL ".rgb = rgb_to_yuv_matrix * " PIXEL ".rgb;\n" \
	PIXEL ".rgb += vec3(yminf, 0.5, 0.5);\n"

#define RGB_TO_HSV_FRAG(PIXEL) \
	"{\n" \
	"float r, g, b;\n" \
	"float h, s, v;\n" \
	"float min, max, delta;\n" \
	"float f, p, q, t;\n" \
	"r = " PIXEL ".r;\n" \
	"g = " PIXEL ".g;\n" \
	"b = " PIXEL ".b;\n" \
	"min = ((r < g) ? r : g) < b ? ((r < g) ? r : g) : b;\n" \
	"max = ((r > g) ? r : g) > b ? ((r > g) ? r : g) : b;\n" \
	"v = max;\n" \
	"delta = max - min;\n" \
	"if(max != 0.0 && delta != 0.0)\n" \
    "{\n" \
	"    s = delta / max;\n" \
	"	if(r == max)\n" \
    "    	h = (g - b) / delta;\n" \
	"	else \n" \
	"	if(g == max)\n" \
    "    	h = 2.0 + (b - r) / delta;\n" \
	"	else\n" \
    "    	h = 4.0 + (r - g) / delta;\n" \
	"\n" \
	"	h *= 60.0;\n" \
	"	if(h < 0.0)\n" \
    "    	h += 360.0;\n" \
	"}\n" \
	"else\n"  \
	"{\n" \
    "    s = 0.0;\n" \
    "    h = -1.0;\n" \
	"}\n" \
	"" PIXEL ".r = h;\n" \
	"" PIXEL ".g = s;\n" \
	"" PIXEL ".b = v;\n" \
	"}\n"

#define HSV_TO_RGB_FRAG(PIXEL) \
	"{\n" \
    "int i;\n" \
	"float r, g, b;\n" \
	"float h, s, v;\n" \
	"float min, max, delta;\n" \
	"float f, p, q, t;\n" \
	"h = " PIXEL ".r;\n" \
	"s = " PIXEL ".g;\n" \
	"v = " PIXEL ".b;\n" \
    "if(s == 0.0) \n" \
	"{\n" \
    "    r = g = b = v;\n" \
    "}\n" \
	"else\n" \
	"{\n" \
    "	h /= 60.0;\n" \
    "	i = int(h);\n" \
    "	f = h - float(i);\n" \
    "	p = v * (1.0 - s);\n" \
    "	q = v * (1.0 - s * f);\n" \
    "	t = v * (1.0 - s * (1.0 - f));\n" \
	"\n" \
    "	if(i == 0)\n" \
	"	{\n" \
    "        	r = v;\n" \
    "        	g = t;\n" \
    "        	b = p;\n" \
    "    }\n" \
	"	else\n" \
	"	if(i == 1)\n" \
	"	{\n" \
    "        	r = q;\n" \
    "        	g = v;\n" \
    "        	b = p;\n" \
    "    }\n" \
	"	else\n" \
	"	if(i == 2)\n" \
	"	{\n" \
    "        	r = p;\n" \
    "        	g = v;\n" \
    "        	b = t;\n" \
    "   }\n" \
	"	else\n" \
	"	if(i == 3)\n" \
	"	{\n" \
    "        	r = p;\n" \
    "        	g = q;\n" \
    "        	b = v;\n" \
    "   }\n" \
	"	else\n" \
	"	if(i == 4)\n" \
	"	{\n" \
    "        	r = t;\n" \
    "        	g = p;\n" \
    "        	b = v;\n" \
    "    }\n" \
	"	else\n" \
	"	if(i == 5)\n" \
	"	{\n" \
    "        	r = v;\n" \
    "        	g = p;\n" \
    "        	b = q;\n" \
    "	}\n" \
	"}\n" \
	"" PIXEL ".r = r;\n" \
	"" PIXEL ".g = g;\n" \
	"" PIXEL ".b = b;\n" \
	"}\n"

class Playback3DCommand : public BC_SynchronousCommand
{
public:
	Playback3DCommand();
	void copy_from(BC_SynchronousCommand *command);

// Extra commands
	enum
	{
// 5
		WRITE_BUFFER = LAST_COMMAND,
		CLEAR_OUTPUT,
		OVERLAY,
		DO_FADE,
		DO_MASK,
		PLUGIN,
		CLEAR_INPUT,
		DO_CAMERA,
		COPY_FROM,
		CONVERT_CMODEL
	};

	Canvas *canvas;
	int is_cleared;

// Parameters for overlay command
	float in_x1;
	float in_y1;
	float in_x2;
	float in_y2;
	float out_x1;
	float out_y1;
	float out_x2;
	float out_y2;
// 0 - 1
	float alpha;
	int mode;
	int interpolation_type;
	VFrame *input;
	int want_texture;
	int is_nested;

	int dst_cmodel;
	int64_t start_position_project;
	MaskAutos *keyframe_set;
	MaskAuto *keyframe;
	MaskAuto *default_auto;
	PluginClient *plugin_client;
};


class Playback3D : public BC_Synchronous
{
public:
	Playback3D(MWindow *mwindow);
	~Playback3D();

	BC_SynchronousCommand* new_command();
	void handle_command(BC_SynchronousCommand *command);

// Called by VDeviceX11::write_buffer during video playback
	void write_buffer(Canvas *canvas,
		VFrame *frame,
		float in_x1,
		float in_y1,
		float in_x2,
		float in_y2,
		float out_x1,
		float out_y1,
		float out_x2,
		float out_y2,
		int is_cleared);

// Reads from pbuffer to either RAM or texture and updates the dst state
// want_texture - causes read into texture if 1
	void copy_from(Canvas *canvas,
		VFrame *dst,
		VFrame *src,
		int want_texture = 0);

// Clear framebuffer before composing virtual console
// output - passed when rendering refresh frame.  If 0, the canvas is cleared.
	void clear_output(Canvas *canvas, VFrame *output);

	void do_fade(Canvas *canvas, VFrame *frame, float fade);
	void convert_cmodel(Canvas *canvas, VFrame *output, int dst_cmodel);

	void do_mask(Canvas *canvas,
		VFrame *output,
		int64_t start_position_project,
		MaskAutos *keyframe_set,
		MaskAuto *keyframe,
		MaskAuto *default_auto);


// Overlay a virtual node on the framebuffer
	void overlay(Canvas *canvas,
		VFrame *input,
		float in_x1,
		float in_y1,
		float in_x2,
		float in_y2,
		float out_x1,
		float out_y1,
		float out_x2,
		float out_y2,
		float alpha,        // 0 - 1
		int mode,
		int interpolation_type,
// supplied if rendering single frame to PBuffer.
		VFrame *output = 0,
		int is_nested = 0);


	int run_plugin(Canvas *canvas, PluginClient *client);

	void clear_input(Canvas *canvas, VFrame *frame);
	void do_camera(Canvas *canvas,
		VFrame *output,
		VFrame *input,
		float in_x1,
		float in_y1,
		float in_x2,
		float in_y2,
		float out_x1,
		float out_y1,
		float out_x2,
		float out_y2);

private:
// Called by write_buffer and clear_frame to initialize OpenGL flags
	void init_frame(Playback3DCommand *command, int is_yuv=0);
	void write_buffer_sync(Playback3DCommand *command);
	void draw_output(Playback3DCommand *command, int flip_y);
	void clear_output_sync(Playback3DCommand *command);
	void clear_input_sync(Playback3DCommand *command);
	void overlay_sync(Playback3DCommand *command);
// Read frame buffer back into texture for overlay operation
	void enable_overlay_texture(Playback3DCommand *command);
	void do_fade_sync(Playback3DCommand *command);
	void do_mask_sync(Playback3DCommand *command);
	void run_plugin_sync(Playback3DCommand *command);
	void do_camera_sync(Playback3DCommand *command);
//	void draw_refresh_sync(Playback3DCommand *command);
	void copy_from_sync(Playback3DCommand *command);
	void convert_cmodel_sync(Playback3DCommand *command);

// Print errors from shader compilation
	void print_error(unsigned int object, int is_program);

// This quits the program when it's 1.
	MWindow *mwindow;
// Temporaries for render to texture
	BC_Texture *temp_texture;
// This is set by clear_output and used in compositing directly
// to the output framebuffer.
	int canvas_w;
	int canvas_h;
};




#endif
