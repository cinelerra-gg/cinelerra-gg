/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */


#ifndef BCCMODELS_H
#define BCCMODELS_H

// Colormodels
// Must match colormodels.h in quicktime
#ifndef BC_TRANSPARENCY

enum BC_CModel {
	BC_TRANSPARENCY = 0,
	BC_COMPRESSED   = 1,
	BC_RGB8         = 2,
	BC_RGB565       = 3,
	BC_BGR565       = 4,
	BC_BGR888       = 5,
	BC_BGR8888      = 6,
// Working bitmaps are packed to simplify processing
	BC_RGB888       = 9,
	BC_RGBA8888     = 10,
	BC_ARGB8888     = 20,
	BC_ABGR8888     = 21,
	BC_RGB161616    = 11,
	BC_RGBA16161616 = 12,
	BC_YUV888       = 13,
	BC_YUVA8888     = 14,
	BC_YUV161616    = 15,
	BC_YUVA16161616 = 16,
	BC_UVY422       = 18,
	BC_YUV422       = 19,
	BC_A8           = 22,
	BC_A16          = 23,
	BC_A_FLOAT      = 31,
	BC_YUV101010    = 24,
	BC_VYU888       = 25,
	BC_UYVA8888     = 26,
	BC_RGB_FLOAT    = 29,
	BC_RGBA_FLOAT   = 30,
// Planar
	BC_YUV420P      = 7,
	BC_YUV422P      = 8,
	BC_YUV444P      = 27,
	BC_YUV411P      = 17,
	BC_YUV410P      = 28,
	BC_RGB_FLOATP   = 32,
	BC_RGBA_FLOATP  = 33,
	BC_YUV420PI     = 34,
	BC_GBRP         = 38,
// only used in intermediate ffmpeg xfers
	BC_AYUV16161616 = 35,
	BC_GREY8        = 36,
	BC_GREY16       = 37,
//only used for chroma only xfers (transfer with bg_color < 0)
	BC_RGBX8888	= 39,
	BC_RGBX16161616	= 40,
	BC_YUVX8888	= 41,
	BC_YUVX16161616	= 42,
	BC_RGBX_FLOAT   = 43,
};

// Colormodels purely used by Quicktime are done in Quicktime.

// For communication with the X Server
#define FOURCC_YV12 0x32315659  /* YV12   YUV420P */
#define FOURCC_YUV2 0x32595559  /* YUV2   YUV422 */
#define FOURCC_UYVY 0x59565955  /* UYVY   UVY422 */
#define FOURCC_I420 0x30323449  /* I420   Intel Indeo 4 */


#endif // !BC_TRANSPARENCY


// Access with BC_WindowBase::cmodels
class BC_CModels
{
public:
	BC_CModels();

	static int calculate_pixelsize(int colormodel);
	static int calculate_datasize(int w, int h, int bytes_per_line, int color_model);
	static int calculate_max(int colormodel);
	static int components(int colormodel);
	static int is_yuv(int colormodel);
	static int has_alpha(int colormodel);
	static int is_float(int colormodel);

	// Tell when to use plane arguments or row pointer arguments to functions
	static int is_planar(int color_model);
	static void to_text(char *string, int cmodel);
	static int from_text(const char *text);

	static void transfer(unsigned char **output_rows, /* Leave NULL if non existent */
		unsigned char **input_rows,
		unsigned char *out_y_plane, /* Leave NULL if non existent */
		unsigned char *out_u_plane,
		unsigned char *out_v_plane,
		unsigned char *in_y_plane, /* Leave NULL if non existent */
		unsigned char *in_u_plane,
		unsigned char *in_v_plane,
		int in_x,        /* Dimensions to capture from input frame */
		int in_y,
		int in_w,
		int in_h,
		int out_x,       /* Dimensions to project on output frame */
		int out_y,
		int out_w,
		int out_h,
		int in_colormodel,
		int out_colormodel,
		int bg_color,         /* -1 for chroma only xfer, else bg color for BC_RGBA8888 to non-alpha */
		int in_rowspan,       /* For planar use the luma rowspan */
		int out_rowspan);     /* For planar use the luma rowspan */

// ptrs are either row pointers (colormodel flat) or plane pointers (colormodel planar)
	static void transfer(
		unsigned char **output_ptrs, int out_colormodel,
			int out_x, int out_y, int out_w, int out_h, int out_rowspan,
		unsigned char **input_ptrs, int in_colormodel,
			int in_x, int in_y, int in_w, int in_h, int in_rowspan,
		int bg_color);

	static int bc_to_x(int color_model);
	static void bcxfer_stop_slicers();
};


#endif
