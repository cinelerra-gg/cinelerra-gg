
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

#include "bccolors.h"

#include <stdio.h>
#include <stdlib.h>

HSV::HSV()
{
}


HSV::~HSV()
{
}

int HSV::rgb_to_hsv(float r, float g, float b, float &h, float &s, float &v)
{
	float min = ((r < g) ? r : g) < b ? ((r < g) ? r : g) : b;
	float max = ((r > g) ? r : g) > b ? ((r > g) ? r : g) : b;
	float delta = max - min;
	if( max != 0 && delta != 0 ) {
		v = max;
		s = delta / max;
		h = r == max ? (g - b) / delta :     // between yellow & magenta
		    g == max ? 2 + (b - r) / delta : // between cyan & yellow
		               4 + (r - g) / delta;  // between magenta & cyan
		if( (h*=60) < 0 ) h += 360;          // degrees
	}
	else { // r = g = b
	        h = 0;  s = 0;  v = max;
	}

	return 0;
}

int HSV::hsv_to_rgb(float &r, float &g, float &b, float h, float s, float v)
{
	if( s == 0 ) { // achromatic (grey)
		r = g = b = v;
		return 0;
	}

	h /= 60;			// sector 0 to 5
	int i = (int)h;
	float f = h - i;		// factorial part of h
	float p = v * (1 - s);
	float q = v * (1 - s * f);
	float t = v * (1 - s * (1 - f));

	switch(i) {
        case 0:  r = v; g = t; b = p; break;
        case 1:  r = q; g = v; b = p; break;
        case 2:  r = p; g = v; b = t; break;
        case 3:  r = p; g = q; b = v; break;
        case 4:  r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
	}
	return 0;
}

int HSV::yuv_to_hsv(int y, int u, int v, float &h, float &s, float &va, int max)
{
	float r, g, b;
	int r_i, g_i, b_i;

// 	if(max == 0xffff)
// 	{
// 		YUV::yuv.yuv_to_rgb_16(r_i, g_i, b_i, y, u, v);
// 	}
// 	else
	{
		YUV::yuv.yuv_to_rgb_8(r_i, g_i, b_i, y, u, v);
	}
	r = (float)r_i / max;
	g = (float)g_i / max;
	b = (float)b_i / max;

	float h2, s2, v2;
	HSV::rgb_to_hsv(r, g, b, h2, s2, v2);

	h = h2;  s = s2;  va = v2;
	return 0;
}

int HSV::hsv_to_yuv(int &y, int &u, int &v, float h, float s, float va, int max)
{
	float r, g, b;
	int r_i, g_i, b_i;
	HSV::hsv_to_rgb(r, g, b, h, s, va);
	r = r * max + 0.5;
	g = g * max + 0.5;
	b = b * max + 0.5;
	r_i = (int)CLIP(r, 0, max);
	g_i = (int)CLIP(g, 0, max);
	b_i = (int)CLIP(b, 0, max);

	int y2, u2, v2;
// 	if(max == 0xffff)
// 		YUV::yuv.rgb_to_yuv_16(r_i, g_i, b_i, y2, u2, v2);
// 	else
		YUV::yuv.rgb_to_yuv_8(r_i, g_i, b_i, y2, u2, v2);

	y = y2;  u = u2;  v = v2;
	return 0;
}


YUV YUV::yuv;
float YUV::yuv_to_rgb_matrix[9];
float YUV::rgb_to_yuv_matrix[9];
float *const YUV::rgb_to_y_vector = YUV::rgb_to_yuv_matrix;

YUV::YUV()
{
	this->tab = new int[0xe0e00];
	this->tabf = new float[0x40400];
	yuv_set_colors(0, 0);
}

void YUV::yuv_set_colors(int color_space, int color_range)
{
	double kr, kb;
	int mpeg;
	switch( color_space ) {
	default:
	case BC_COLORS_BT601:  kr = BT601_Kr;   kb = BT601_Kb;   break;
	case BC_COLORS_BT709:  kr = BT709_Kr;   kb = BT709_Kb;   break;
	case BC_COLORS_BT2020: kr = BT2020_Kr;  kb = BT2020_Kb;  break;
	}
	switch( color_range ) {
	default:
	case BC_COLORS_JPEG: mpeg = 0;  break;
	case BC_COLORS_MPEG: mpeg = 1;  break;
	}
	init(kr, kb, mpeg);
}

void YUV::init(double Kr, double Kb, int mpeg)
{
	this->mpeg   = mpeg;
	int ymin = !mpeg? 0 : 16;
	int ymax = !mpeg? 255 : 235;
	int uvmin = !mpeg? 0 : 16;
	int uvmax = !mpeg? 255 : 240;
	this->ymin8  = ymin;
	this->ymax8  = ymax;
	this->ymin16 = ymin*0x100;
	this->ymax16 = (ymax+1)*0x100 - 1;
	this->yzero  = 0x10000 * ymin;
	this->uvmin8  = uvmin;
	this->uvmax8  = uvmax;
	this->uvmin16 = uvmin*0x100;
	this->uvmax16 = (uvmax+1)*0x100 - 1;
	this->uvzero = 0x800000;
	this->yminf = ymin / 256.;
	this->ymaxf = (ymax+1) / 256.;
	this->yrangef = ymaxf - yminf;
	this->uvminf = uvmin / 256.;
	this->uvmaxf = (uvmax+1) / 256.;
	this->uvrangef = uvmaxf - uvminf;
	this->Kr = Kr;
	this->Kg = 1. - Kr - Kb;
	this->Kb = Kb;
	double R_to_Y = Kr;
	double G_to_Y = Kg;
	double B_to_Y = Kb;
	double R_to_U = -0.5*Kr/(1-Kb);
	double G_to_U = -0.5*Kg/(1-Kb);
	double B_to_U =  0.5;
	double R_to_V =  0.5;
	double G_to_V = -0.5*Kg/(1-Kr);
	double B_to_V = -0.5*Kb/(1-Kr);
	double V_to_R =  2.0*(1-Kr);
	double V_to_G = -2.0*Kr*(1-Kr)/Kg;
	double U_to_G = -2.0*Kb*(1-Kb)/Kg;
	double U_to_B =  2.0*(1-Kb);

	this->r_to_y = yrangef * R_to_Y;
	this->g_to_y = yrangef * G_to_Y;
	this->b_to_y = yrangef * B_to_Y;
	this->r_to_u = uvrangef * R_to_U;
	this->g_to_u = uvrangef * G_to_U;
	this->b_to_u = uvrangef * B_to_U;
	this->r_to_v = uvrangef * R_to_V;
	this->g_to_v = uvrangef * G_to_V;
	this->b_to_v = uvrangef * B_to_V;
	this->v_to_r = V_to_R / uvrangef;
	this->v_to_g = V_to_G / uvrangef;
	this->u_to_g = U_to_G / uvrangef;
	this->u_to_b = U_to_B / uvrangef;
	
	init_tables(0x100,
		rtoy8, rtou8, rtov8,
		gtoy8, gtou8, gtov8,
		btoy8, btou8, btov8,
		ytab8, vtor8, vtog8, utog8, utob8);
	init_tables(0x10000,
		rtoy16, rtou16, rtov16,
		gtoy16, gtou16, gtov16,
		btoy16, btou16, btov16,
		ytab16, vtor16, vtog16, utog16, utob16); 
	init_tables(0x100,
		vtor8f, vtog8f, utog8f, utob8f);
	init_tables(0x10000,
		vtor16f, vtog16f, utog16f, utob16f);

	rgb_to_yuv_matrix[0] = r_to_y;
	rgb_to_yuv_matrix[1] = g_to_y;
	rgb_to_yuv_matrix[2] = b_to_y;
	rgb_to_yuv_matrix[3] = r_to_u;
	rgb_to_yuv_matrix[4] = g_to_u;
	rgb_to_yuv_matrix[5] = b_to_u;
	rgb_to_yuv_matrix[6] = r_to_v;
	rgb_to_yuv_matrix[7] = g_to_v;
	rgb_to_yuv_matrix[8] = b_to_v;

	float yscale = 1.f / yrangef;
	yuv_to_rgb_matrix[0] = yscale;
	yuv_to_rgb_matrix[1] = 0;
	yuv_to_rgb_matrix[2] = v_to_r;
	yuv_to_rgb_matrix[3] = yscale;
	yuv_to_rgb_matrix[4] = u_to_g;
	yuv_to_rgb_matrix[5] = v_to_g;
	yuv_to_rgb_matrix[6] = yscale;
	yuv_to_rgb_matrix[7] = u_to_b;
	yuv_to_rgb_matrix[8] = 0;
}

void YUV::init_tables(int len,
		int *rtoy, int *rtou, int *rtov,
		int *gtoy, int *gtou, int *gtov,
		int *btoy, int *btou, int *btov,
		int *ytab,
		int *vtor, int *vtog,
		int *utog, int *utob)
{
// rgb->yuv
	double s = (double)0xffffff / len;
	double r2y = s*r_to_y, g2y = s*g_to_y, b2y = s*b_to_y;
	double r2u = s*r_to_u, g2u = s*g_to_u, b2u = s*b_to_u;
	double r2v = s*r_to_v, g2v = s*g_to_v, b2v = s*b_to_v;
	for( int rgb=0; rgb<len; ++rgb ) {
		rtoy[rgb] = r2y*rgb;  rtou[rgb] = r2u*rgb;  rtov[rgb] = r2v*rgb;
		gtoy[rgb] = g2y*rgb;  gtou[rgb] = g2u*rgb;  gtov[rgb] = g2v*rgb;
		btoy[rgb] = b2y*rgb;  btou[rgb] = b2u*rgb;  btov[rgb] = b2v*rgb;
	}

// yuv->rgb
	int y0  = ymin8 * len/0x100,  y1 = (ymax8+1) * len/0x100 - 1;
	s = (double)0xffffff / (y1 - y0);
	int y = 0, iy = 0;
	while( y < y0 ) ytab[y++] = 0;
	while( y < y1 ) ytab[y++] = s*iy++;
	while( y < len ) ytab[y++] = 0xffffff;

	int uv0 = uvmin8 * len/0x100, uv1 = (uvmax8+1) * len/0x100 - 1;
	s = (double)0xffffff / (uv1 - uv0);
	double v2r = s*v_to_r, v2g = s*v_to_g;
	double u2g = s*u_to_g, u2b = s*u_to_b;
	int uv = 0, iuv = uv0 - len/2;
	int vr0 = v2r * iuv, vg0 = v2g * iuv;
	int ug0 = u2g * iuv, ub0 = u2b * iuv;
	while( uv < uv0 ) {
		vtor[uv] = vr0;  vtog[uv] = vg0;
		utog[uv] = ug0;  utob[uv] = ub0;
		++uv;
	}
	while( uv < uv1 ) {
		vtor[uv] = iuv * v2r;  vtog[uv] = iuv * v2g;
		utog[uv] = iuv * u2g;  utob[uv] = iuv * u2b;
		++uv;  ++iuv;
	}
	int vr1 = v2r * iuv, vg1 = v2g * iuv;
	int ug1 = u2g * iuv, ub1 = u2b * iuv;
	while( uv < len ) {
		vtor[uv] = vr1;  vtog[uv] = vg1;
		utog[uv] = ug1;  utob[uv] = ub1;
 		++uv;
	}
}

void YUV::init_tables(int len,
		float *vtorf, float *vtogf, float *utogf, float *utobf)
{
	int len1 = len-1, len2 = len/2;
	for( int i=0,k=-len2; i<len; ++i,++k ) {
		vtorf[i] = (v_to_r * k)/len1;
		vtogf[i] = (v_to_g * k)/len1;
		utogf[i] = (u_to_g * k)/len1;
		utobf[i] = (u_to_b * k)/len1;
	}
}

YUV::~YUV()
{
	delete [] tab;
	delete [] tabf;
}

