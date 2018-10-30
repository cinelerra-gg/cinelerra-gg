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



#include "bccmodels.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int BC_CModels::is_planar(int colormodel)
{
	switch(colormodel) {
	case BC_YUV420P:
	case BC_YUV420PI:
	case BC_YUV422P:
	case BC_YUV444P:
	case BC_GBRP:
	case BC_YUV411P:
	case BC_YUV410P:
	case BC_RGB_FLOATP:
	case BC_RGBA_FLOATP:
		return 1;
	}
	return 0;
}

int BC_CModels::components(int colormodel)
{
	switch(colormodel) {
	case BC_RGB8:
	case BC_RGB565:
	case BC_BGR565:
	case BC_BGR888:
	case BC_RGB888:
	case BC_RGB161616:
	case BC_RGB_FLOAT:
	case BC_BGR8888:
	case BC_YUV888:
	case BC_YUV161616:
	case BC_UVY422:
	case BC_YUV422:
	case BC_YUV101010:
	case BC_VYU888:
		return 3;
	case BC_RGBA8888:
	case BC_RGBX8888:
	case BC_ARGB8888:
	case BC_ABGR8888:
	case BC_RGBA16161616:
	case BC_RGBX16161616:
	case BC_RGBA_FLOAT:
	case BC_RGBX_FLOAT:
	case BC_YUVA8888:
	case BC_YUVX8888:
	case BC_YUVA16161616:
	case BC_YUVX16161616:
	case BC_UYVA8888:
	case BC_AYUV16161616:
		return 4;
	case BC_A8:
	case BC_A16:
	case BC_A_FLOAT:
	case BC_GREY8:
	case BC_GREY16:
		return 1;
	}
// planar, compressed, transparent
	return 0;
}

int BC_CModels::calculate_pixelsize(int colormodel)
{
	switch(colormodel) {
	case BC_A8:           return 1;
	case BC_A16:          return 2;
	case BC_A_FLOAT:      return 4;
	case BC_TRANSPARENCY: return 1;
	case BC_COMPRESSED:   return 1;
	case BC_RGB8:         return 1;
	case BC_RGB565:       return 2;
	case BC_BGR565:       return 2;
	case BC_BGR888:       return 3;
	case BC_BGR8888:      return 4;
// Working bitmaps are packed to simplify processing
	case BC_RGB888:       return 3;
	case BC_ARGB8888:     return 4;
	case BC_ABGR8888:     return 4;
	case BC_RGBA8888:     return 4;
	case BC_RGBX8888:     return 4;
	case BC_RGB161616:    return 6;
	case BC_RGBA16161616: return 8;
	case BC_RGBX16161616: return 8;
	case BC_YUV888:       return 3;
	case BC_YUVA8888:     return 4;
	case BC_YUVX8888:     return 4;
	case BC_YUV161616:    return 6;
	case BC_YUVA16161616: return 8;
	case BC_YUVX16161616: return 8;
	case BC_AYUV16161616: return 8;
	case BC_YUV101010:    return 4;
	case BC_VYU888:       return 3;
	case BC_UYVA8888:     return 4;
	case BC_RGB_FLOAT:    return 12;
	case BC_RGBA_FLOAT:   return 16;
	case BC_RGBX_FLOAT:   return 16;
	case BC_GREY8:        return 1;
	case BC_GREY16:       return 2;
// Planar
	case BC_YUV420P:      return 1;
	case BC_YUV420PI:     return 1;
	case BC_YUV422P:      return 1;
	case BC_YUV444P:      return 1;
	case BC_GBRP:         return 1;
	case BC_YUV422:       return 2;
	case BC_UVY422:       return 2;
	case BC_YUV411P:      return 1;
	case BC_YUV410P:      return 1;
	case BC_RGB_FLOATP:   return 4;
	case BC_RGBA_FLOATP:  return 4;
	}
	return 0;
}

int BC_CModels::calculate_max(int colormodel)
{
	switch(colormodel) {
// Working bitmaps are packed to simplify processing
	case BC_A8:           return 0xff;
	case BC_A16:          return 0xffff;
	case BC_A_FLOAT:      return 1;
	case BC_RGB888:       return 0xff;
	case BC_RGBA8888:     return 0xff;
	case BC_RGBX8888:     return 0xff;
	case BC_RGB161616:    return 0xffff;
	case BC_RGBA16161616: return 0xffff;
	case BC_RGBX16161616: return 0xffff;
	case BC_YUV888:       return 0xff;
	case BC_YUVA8888:     return 0xff;
	case BC_YUVX8888:     return 0xff;
	case BC_YUV161616:    return 0xffff;
	case BC_YUVA16161616: return 0xffff;
	case BC_YUVX16161616: return 0xffff;
	case BC_AYUV16161616: return 0xffff;
	case BC_RGB_FLOAT:    return 1;
	case BC_RGBA_FLOAT:   return 1;
	case BC_RGBX_FLOAT:   return 1;
	case BC_RGB_FLOATP:   return 1;
	case BC_RGBA_FLOATP:  return 1;
	case BC_GREY8:        return 0xff;
	case BC_GREY16:       return 0xffff;
	case BC_GBRP:         return 0xff;
	}
	return 0;
}

int BC_CModels::calculate_datasize(int w, int h, int bytes_per_line, int color_model)
{
	switch(color_model) {
	case BC_YUV410P: return w * h + w * h / 8 + 4;
	case BC_YUV420P:
	case BC_YUV420PI:
	case BC_YUV411P: return w * h + w * h / 2 + 4;
	case BC_YUV422P: return w * h * 2 + 4;
	case BC_YUV444P: return w * h * 3 + 4;
	case BC_GBRP:    return w * h * 3 + 4;
	case BC_RGB_FLOATP: return w * h * 3 * sizeof(float) + 4;
	case BC_RGBA_FLOATP: return w * h * 4 * sizeof(float) + 4;
	}
	if( bytes_per_line < 0 )
		bytes_per_line = w * calculate_pixelsize(color_model);
	return h * bytes_per_line + 4;
}

int BC_CModels::bc_to_x(int color_model)
{
	switch(color_model) {
	case BC_YUV420P: return FOURCC_YV12;
	case BC_YUV422:  return FOURCC_YUV2;
	case BC_UVY422:  return FOURCC_UYVY;
	}
	return -1;
}

void BC_CModels::to_text(char *string, int cmodel)
{
	switch(cmodel) {
	case BC_RGB888:       strcpy(string, "RGB-8 Bit");   break;
	case BC_RGBA8888:     strcpy(string, "RGBA-8 Bit");  break;
	case BC_RGB161616:    strcpy(string, "RGB-16 Bit");  break;
	case BC_RGBA16161616: strcpy(string, "RGBA-16 Bit"); break;
	case BC_YUV888:       strcpy(string, "YUV-8 Bit");   break;
	case BC_YUVA8888:     strcpy(string, "YUVA-8 Bit");  break;
	case BC_YUV161616:    strcpy(string, "YUV-16 Bit");  break;
	case BC_YUVA16161616: strcpy(string, "YUVA-16 Bit"); break;
	case BC_AYUV16161616: strcpy(string, "AYUV-16 Bit"); break;
	case BC_RGB_FLOAT:    strcpy(string, "RGB-FLOAT");   break;
	case BC_RGBA_FLOAT:   strcpy(string, "RGBA-FLOAT");  break;
	case BC_RGB_FLOATP:   strcpy(string, "RGB-FLOATP");  break;
	case BC_RGBA_FLOATP:  strcpy(string, "RGBA-FLOATP"); break;
	default: strcpy(string, "RGB-8 Bit"); break;
	}
}

int BC_CModels::from_text(const char *text)
{
	if(!strcasecmp(text, "RGB-8 Bit"))   return BC_RGB888;
	if(!strcasecmp(text, "RGBA-8 Bit"))  return BC_RGBA8888;
	if(!strcasecmp(text, "RGB-16 Bit"))  return BC_RGB161616;
	if(!strcasecmp(text, "RGBA-16 Bit")) return BC_RGBA16161616;
	if(!strcasecmp(text, "RGB-FLOAT"))   return BC_RGB_FLOAT;
	if(!strcasecmp(text, "RGBA-FLOAT"))  return BC_RGBA_FLOAT;
	if(!strcasecmp(text, "RGB-FLOATP"))  return BC_RGB_FLOATP;
	if(!strcasecmp(text, "RGBA-FLOATP")) return BC_RGBA_FLOATP;
	if(!strcasecmp(text, "YUV-8 Bit"))   return BC_YUV888;
	if(!strcasecmp(text, "YUVA-8 Bit"))  return BC_YUVA8888;
	if(!strcasecmp(text, "YUV-16 Bit"))  return BC_YUV161616;
	if(!strcasecmp(text, "YUVA-16 Bit")) return BC_YUVA16161616;
	if(!strcasecmp(text, "AYUV-16 Bit")) return BC_AYUV16161616;
	return BC_RGB888;
}

int BC_CModels::has_alpha(int colormodel)
{

	switch(colormodel) {
	case BC_RGBA8888:
	case BC_ARGB8888:
	case BC_ABGR8888:
	case BC_RGBA16161616:
	case BC_YUVA8888:
	case BC_YUVA16161616:
	case BC_UYVA8888:
	case BC_RGBA_FLOAT:
	case BC_RGBA_FLOATP:
		return 1;
	}
	return 0;
}

int BC_CModels::is_float(int colormodel)
{
	switch(colormodel) {
	case BC_RGB_FLOAT:
	case BC_RGBA_FLOAT:
	case BC_RGBX_FLOAT:
	case BC_RGB_FLOATP:
	case BC_RGBA_FLOATP:
		return 1;
	}
	return 0;
}

int BC_CModels::is_yuv(int colormodel)
{

	switch(colormodel) {
	case BC_YUV888:
	case BC_YUVA8888:
	case BC_YUVX8888:
	case BC_YUV161616:
	case BC_YUVA16161616:
	case BC_YUVX16161616:
	case BC_AYUV16161616:
	case BC_YUV422:
	case BC_UVY422:
	case BC_YUV101010:
	case BC_VYU888:
	case BC_UYVA8888:
	case BC_YUV420P:
	case BC_YUV420PI:
	case BC_YUV422P:
	case BC_YUV444P:
	case BC_YUV411P:
	case BC_YUV410P:
	case BC_GREY8:
	case BC_GREY16:
		return 1;
	}
	return 0;
}


