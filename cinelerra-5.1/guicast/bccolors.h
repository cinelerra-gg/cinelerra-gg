
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

#ifndef __BCCOLORS_H__
#define __BCCOLORS_H__

// Duplicate filename in guicast

#include "bccolors.inc"
#include "clip.h"
#include "colors.h"
#include "vframe.inc"

#include <stdint.h>


/*
Digital YCbCr is derived from analog RGB as follows:
RGB are 0..1 (gamma corrected supposedly),
normalized equations:
Y  Py =   Kr * R  +  Kg * G  +  Kb * B
U  Pb = - 0.5*Kr/(1-Kb)*R - 0.5*Kg/(1-Kb)*G + 0.5*B
V  Pr =   0.5*R - 0.5*Kg/(1-Kr)*G - 0.5*Kb/(1-Kr)*B
inverse:
   R = Py + Pr * 2*(1-Kr)
   G = Py - Pr * 2*Kr*(1-Kr)/Kg - Pb * 2*Kb*(1-Kb)/Kg
   B = Py                       + Pb * 2*(1-Kb)

bt601, white vector= Kr=0.299,Kg=0.587,Kb=0.114
   Py = + 0.299000*R + 0.587000*G + 0.114000*B
   Pb = - 0.168736*R - 0.331264*G + 0.500000*B
   Pr = + 0.500000*R - 0.418698*G - 0.081312*B
inverse:
   R  = Py + 1.402000*Pr
   G  = Py - 0.714136*Pr - 0.344136*Pb
   B  = Py               + 1.772000*Pb

equations zeroed at (0,128,128), range (255,255,255)
bt.601  0..255
   Y  = 0   + 0.299000*R + 0.587000*G + 0.114000*B
   Cb = 128 - 0.168736*R - 0.331264*G + 0.500000*B
   Cr = 128 + 0.500000*R - 0.418698*G - 0.081312*B
inverse:
   R  = Y + 1.402000*(Cr-128)
   G  = Y - 0.714136*(Cr-128) - 0.344136*(Cb-128)
   B  = Y                     + 1.772000*(Cb-128)

equations zeroed at (16,128,128), range (219,224,224)
Y,Cb,Cr = (16,128,128) + (219*Py,224*Pb,224*Pr)

bt.601  16..235
   Y  = 16  + 0.256788*R + 0.504129*G + 0.097906*B
   Cb = 128 - 0.148227*R - 0.290992*G + 0.439216*B
   Cr = 128 + 0.439216*R - 0.367789*G - 0.071426*B
inverse:
   R  = (Y-16)*1.164384 + 1.596027*(Cr-128)
   G  = (Y-16)*1.164384 - 0.812968*(Cr-128) - 0.391762*(Cb-128)
   B  = (Y-16)*1.164384                     + 2.017232*(Cb-128)

bt.709, white vector= Kr=0.2126,Kg=0.7152,Kb=0.0722
   Py = + 0.212600*R + 0.715200*G + 0.072200*B
   Pb = - 0.114572*R - 0.385428*G + 0.500000*B
   Pr = + 0.500000*R - 0.454153*G - 0.045847*B

equations zeroed at (0,128,128), range (255,255,255)
bt.709  0..255
   Y  = 0   + 0.212600*R + 0.715200*G + 0.072200*B 
   Cb = 128 - 0.114572*R - 0.385428*G + 0.500000*B
   Cr = 128 + 0.500000*R - 0.454153*G - 0.045847*B
inverse:
   R = Y + 1.574800*(Cr-128)
   G = Y - 0.468124*(Cr-128) - 0.187324*(Cb-128)
   B = Y                     + 1.855600*(Cb-128)

equations zeroed at (16,128,128), range (219,224,224)
Y,Cb,Cr = (16,128,128) + (219*Py,224*Pb,224*Pr)
   Y  = 16  + 0.182586*R + 0.614231*G + 0.062007*B
   Cb = 128 - 0.100644*R - 0.338572*G + 0.439216*B
   Cr = 128 + 0.439216*R - 0.398942*G - 0.040276*B
inverse:
   R = (Y-16)*1.164384 + 1.792741*(Cr-128)
   G = (Y-16)*1.164384 - 0.532909*(Cr-128) - 0.213249*(Cb-128)
   B = (Y-16)*1.164384                     + 2.112402*(Cb-128)

*/
// white vector normalized, so:
//  Kg = 1 - Kr - Kb

#define BT601_Kr 0.299
#define BT601_Kb 0.114

#define BT709_Kr 0.2126
#define BT709_Kb 0.0722

#define BT2020_Kr 0.2627
#define BT2020_Kb 0.0593

class YUV
{
	int mpeg, yzero, uvzero;
	int ymin8, ymax8, ymin16, ymax16;
	int uvmin8, uvmax8, uvmin16, uvmax16;
	double Kr, Kg, Kb;
	float yminf, ymaxf, yrangef;
	float uvminf, uvmaxf, uvrangef;
	float r_to_y, g_to_y, b_to_y;
	float r_to_u, g_to_u, b_to_u;
	float r_to_v, g_to_v, b_to_v;
	float v_to_r, v_to_g;
	float u_to_g, u_to_b;
	int *tab;
	float *tabf;

	void init(double Kr, double Kb, int mpeg);
	void init_tables(int len,
		int *rtoy, int *rtou, int *rtov,
		int *gtoy, int *gtou, int *gtov,
		int *btoy, int *btou, int *btov,
		int *ytab, int *vtor, int *vtog, int *utog, int *utob);
	void init_tables(int len,
		float *vtorf, float *vtogf, float *utogf, float *utobf);

// dont use pointers,
//  offsets do not require indirect access
#define rtoy16 (tab+0x00000)
#define gtoy16 (tab+0x10000)
#define btoy16 (tab+0x20000)
#define rtou16 (tab+0x30000)
#define gtou16 (tab+0x40000)
#define btou16 (tab+0x50000)
#define rtov16 (tab+0x60000)
#define gtov16 (tab+0x70000)
#define btov16 (tab+0x80000)
#define ytab16 (tab+0x90000)
#define vtor16 (tab+0xa0000)
#define vtog16 (tab+0xb0000)
#define utog16 (tab+0xc0000)
#define utob16 (tab+0xd0000)

#define rtoy8 (tab+0xe0000)
#define gtoy8 (tab+0xe0100)
#define btoy8 (tab+0xe0200)
#define rtou8 (tab+0xe0300)
#define gtou8 (tab+0xe0400)
#define btou8 (tab+0xe0500)
#define rtov8 (tab+0xe0600)
#define gtov8 (tab+0xe0700)
#define btov8 (tab+0xe0800)
#define ytab8 (tab+0xe0900)
#define vtor8 (tab+0xe0a00)
#define vtog8 (tab+0xe0b00)
#define utog8 (tab+0xe0c00)
#define utob8 (tab+0xe0d00)

#define vtor16f (tabf+0x00000)
#define vtog16f (tabf+0x10000)
#define utog16f (tabf+0x20000)
#define utob16f (tabf+0x30000)

#define vtor8f (tabf+0x40000)
#define vtog8f (tabf+0x40100)
#define utog8f (tabf+0x40200)
#define utob8f (tabf+0x40300)

public:
	YUV();
	~YUV();
	void yuv_set_colors(int color_space, int color_range);
	inline int is_mpeg() { return mpeg; }

	static YUV yuv;
	static float yuv_to_rgb_matrix[9];
	static float rgb_to_yuv_matrix[9];
	static float *const rgb_to_y_vector;
	inline float get_yminf() { return yminf; }

#define YUV_rgb_to_yuv_8(r,g,b, y,u,v) \
	y = iclip((rtoy8[r] + gtoy8[g] + btoy8[b] +  yzero) >> 16,  ymin8,  ymax8); \
	u = iclip((rtou8[r] + gtou8[g] + btou8[b] + uvzero) >> 16, uvmin8, uvmax8); \
	v = iclip((rtov8[r] + gtov8[g] + btov8[b] + uvzero) >> 16, uvmin8, uvmax8)

	bc_always_inline void rgb_to_yuv_8(int r, int g, int b, int &y, int &u, int &v) {
		YUV_rgb_to_yuv_8(r,g,b, y,u,v);
	}
	bc_always_inline void rgb_to_yuv_8(int r, int g, int b, uint8_t &y, uint8_t &u, uint8_t &v) {
		YUV_rgb_to_yuv_8(r,g,b, y,u,v);
	}
	bc_always_inline void rgb_to_yuv_8(int &y, int &u, int &v) {
		int r = y, g = u, b = v;  YUV_rgb_to_yuv_8(r, g, b, y, u, v);
	}

#define YUV_rgb_to_yuv_16(r,g,b, y,u,v) \
	y = iclip((rtoy16[r] + gtoy16[g] + btoy16[b] +  yzero) >> 8,  ymin16,  ymax16); \
	u = iclip((rtou16[r] + gtou16[g] + btou16[b] + uvzero) >> 8, uvmin16, uvmax16); \
	v = iclip((rtov16[r] + gtov16[g] + btov16[b] + uvzero) >> 8, uvmin16, uvmax16)

	bc_always_inline void rgb_to_yuv_16(int r, int g, int b, int &y, int &u, int &v) {
		YUV_rgb_to_yuv_16(r,g,b, y,u,v);
	}
	bc_always_inline void rgb_to_yuv_16(int r, int g, int b, uint16_t &y, uint16_t &u, uint16_t &v) {
		YUV_rgb_to_yuv_16(r,g,b, y,u,v);
	}
	bc_always_inline void rgb_to_yuv_16(int &y, int &u, int &v) {
		int r = y, g = u, b = v;
		YUV_rgb_to_yuv_16(r, g, b, y, u, v);
	}

	bc_always_inline void rgb_to_yuv_f(float r, float g, float b, float &y, float &u, float &v) {
		y = r * r_to_y + g * g_to_y + b * b_to_y + yminf;
		u = r * r_to_u + g * g_to_u + b * b_to_u;
		v = r * r_to_v + g * g_to_v + b * b_to_v;
	}
	bc_always_inline void rgb_to_yuv_f(float r, float g, float b, uint8_t &y, uint8_t &u, uint8_t &v) {
		int ir = iclip(r*0x100, 0, 0xff);
		int ig = iclip(g*0x100, 0, 0xff);
		int ib = iclip(b*0x100, 0, 0xff);
		rgb_to_yuv_8(ir,ig,ib, y,u,v);
	}
	bc_always_inline void rgb_to_yuv_f(float r, float g, float b, uint16_t &y, uint16_t &u, uint16_t &v) {
		int ir = iclip(r*0x10000, 0, 0xffff);
		int ig = iclip(g*0x10000, 0, 0xffff);
		int ib = iclip(b*0x10000, 0, 0xffff);
		rgb_to_yuv_16(ir,ig,ib, y,u,v);
	}

#define YUV_yuv_to_rgb_8(r,g,b, y,u,v) \
	r = iclip((ytab8[y] + vtor8[v]) >> 16, 0, 0xff); \
	g = iclip((ytab8[y] + utog8[u] + vtog8[v]) >> 16, 0, 0xff); \
	b = iclip((ytab8[y] + utob8[u]) >> 16, 0, 0xff)

	bc_always_inline void yuv_to_rgb_8(int &r, int &g, int &b, int y, int u, int v) {
		YUV_yuv_to_rgb_8(r,g,b, y,u,v);
	}
	bc_always_inline void yuv_to_rgb_8(uint8_t &r, uint8_t &g, uint8_t &b, int y, int u, int v) {
		YUV_yuv_to_rgb_8(r,g,b, y,u,v);
	}
	bc_always_inline void yuv_to_rgb_8(int &r, int &g, int &b) {
		int y = r, u = g, v = b;  YUV_yuv_to_rgb_8(r,g,b, y,u,v);
	}
	bc_always_inline void yuv_to_rgb_8(float &r, float &g, float &b, int y, int u, int v) {
		int ir, ig, ib;  yuv_to_rgb_8(ir,ig,ib, y,u,v);
		float s = 1/255.f;  r = s*ir;  g = s*ig;  b = s*ib;
	}

#define YUV_yuv_to_rgb_16(r,g,b, y,u,v) \
	r = iclip((ytab16[y] + vtor16[v]) >> 8, 0, 0xffff); \
	g = iclip((ytab16[y] + utog16[u] + vtog16[v]) >> 8, 0, 0xffff); \
	b = iclip((ytab16[y] + utob16[u]) >> 8, 0, 0xffff)

	bc_always_inline void yuv_to_rgb_16(int &r, int &g, int &b, int y, int u, int v) {
		YUV_yuv_to_rgb_16(r,g,b, y,u,v);
	}
	bc_always_inline void yuv_to_rgb_16(uint16_t &r, uint16_t &g, uint16_t &b, int y, int u, int v) {
		YUV_yuv_to_rgb_16(r,g,b, y,u,v);
	}
	bc_always_inline void yuv_to_rgb_16(int &r, int &g, int &b) {
		int y = r, u = g, v = b;  YUV_yuv_to_rgb_16(r,g,b, y,u,v);
	}
	bc_always_inline void yuv_to_rgb_16(float &r, float &g, float &b, int y, int u, int v) {
		int ir, ig, ib;  YUV_yuv_to_rgb_16(ir,ig,ib, y,u,v);
		float s = 1/65535.f;  r = s*ir;  g = s*ig;  b = s*ib;
	}

	bc_always_inline void yuv_to_rgb_f(float &r, float &g, float &b, float y, float u, float v) {
		y = (y-yminf) / yrangef;
		r = y + v_to_r * v;
		g = y + u_to_g * u + v_to_g * v;
		b = y + u_to_b * u;
	}
	bc_always_inline void yuv_to_rgb_f(float &r, float &g, float &b, uint8_t &y, uint8_t &u, uint8_t &v) {
		yuv_to_rgb_8(r,g,b, y,u,v);
	}
	bc_always_inline void yuv_to_rgb_f(float &r, float &g, float &b, uint16_t &y, uint16_t &u, uint16_t &v) {
		yuv_to_rgb_16(r,g,b, y,u,v);
	}

	bc_always_inline int rgb_to_y_8(int r, int g, int b) {
		return (rtoy8[r] + gtoy8[g] + btoy8[b] + yzero) >> 16;
	}
	bc_always_inline int rgb_to_y_16(int r, int g, int b) {
		return (rtoy16[r] + gtoy16[g] + btoy16[b] + yzero) >> 8;
	}
	bc_always_inline float rgb_to_y_f(float r, float g, float b) {
		return r * r_to_y + g * g_to_y + b * b_to_y + yminf;
	}

// For easier programming.  Doesn't do anything.
// unused cases in macro expansions, mismatched argument types
	inline void yuv_to_rgb_8(float &r, float &g, float &b, float y, float u, float v) {}
	inline void yuv_to_rgb_16(float &r, float &g, float &b, float y, float u, float v) {}
	inline void yuv_to_rgb_f(int &r, int &g, int &b, int y, int u, int v) {}
	inline void yuv_to_rgb_f(uint8_t &r, uint8_t &g, uint8_t &b, int&y, int u, int v) {}
	inline void yuv_to_rgb_f(uint16_t &r, uint16_t &g, uint16_t &b, int y, int u, int v) {}

	inline void rgb_to_yuv_8(float r, float g, float b, float &y, float &u, float &v) {}
	inline void rgb_to_yuv_16(float r, float g, float b, float &y, float &u, float &v) {}
	inline void rgb_to_yuv_f(int r, int g, int b, int &y, int &u, int &v) {}
	inline void rgb_to_yuv_f(uint8_t r, uint8_t g, uint8_t b, int &y, int &u, int &v) {}
	inline void rgb_to_yuv_f(uint16_t r, uint16_t g, uint16_t b, int &y, int &u, int &v) {}
};


class HSV
{
public:
	HSV();
	~HSV();

// All units are 0 - 1
	static int rgb_to_hsv(float r, float g, float b, float &h, float &s, float &v);
	static int hsv_to_rgb(float &r, float &g, float &b, float h, float s, float v);

// YUV units are 0 - max.  HSV units are 0 - 1
	static int yuv_to_hsv(int y, int u, int v, float &h, float &s, float &va, int max);
	static int hsv_to_yuv(int &y, int &u, int &v, float h, float s, float va, int max);
// Dummies for macros
	static int yuv_to_hsv(float y, float u, float v, float &h, float &s, float &va, float max) { return 0; };
	static int hsv_to_yuv(float &y, float &u, float &v, float h, float s, float va, float max) { return 0; };
};

#endif
