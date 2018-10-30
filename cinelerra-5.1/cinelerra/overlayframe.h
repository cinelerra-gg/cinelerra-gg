
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

#ifndef OVERLAYFRAME_H
#define OVERLAYFRAME_H

#include "loadbalance.h"
#include "overlayframe.inc"
#include "vframe.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define DIRECT_COPY 0
#define BILINEAR 1
#define BICUBIC 2
#define LANCZOS 3

#define STD_ALPHA(mx, Sa, Da) (Sa + Da - (Sa * Da) / mx)
#define STD_BLEND(mx, Sc, Sa, Dc, Da) ((Sc * (mx - Da) + Dc * (mx - Sa)) / mx)

#define ZERO 0
#define ONE 1
#define TWO 2

// NORMAL	[Sa + Da * (1 - Sa), Sc * Sa + Dc * (1 - Sa)])
#define ALPHA_NORMAL(mx, Sa, Da) (Sa + (Da * (mx - Sa)) / mx)
#define COLOR_NORMAL(mx, Sc, Sa, Dc, Da) ((Sc * Sa + Dc * (mx - Sa)) / mx)
#define CHROMA_NORMAL COLOR_NORMAL

// ADDITION	[(Sa + Da), (Sc + Dc)]
#define ALPHA_ADDITION(mx, Sa, Da) (Sa + Da)
#define COLOR_ADDITION(mx, Sc, Sa, Dc, Da) (Sc + Dc)
#define CHROMA_ADDITION COLOR_ADDITION

// SUBTRACT	[(Sa - Da), (Sc - Dc)]
#define ALPHA_SUBTRACT(mx, Sa, Da) (Sa - Da)
#define COLOR_SUBTRACT(mx, Sc, Sa, Dc, Da) (Sc - Dc)
#define CHROMA_SUBTRACT COLOR_SUBTRACT

// MULTIPLY	[Sa + Da - Sa*Da, Sc*(1 - Da) + Dc*(1 - Sa) +  Sc * Dc]
#define ALPHA_MULTIPLY STD_ALPHA
#define COLOR_MULTIPLY(mx, Sc, Sa, Dc, Da) (STD_BLEND(mx,Sc,Sa,Dc,Da) + \
  (Sc * Dc) / mx)
#define CHROMA_MULTIPLY COLOR_MULTIPLY

// DIVIDE	[Sa + Da - Sa*Da, Sc*(1 - Da) + Dc*(1 - Sa) +  Sc / Dc]
#define ALPHA_DIVIDE STD_ALPHA
#define COLOR_DIVIDE(mx, Sc, Sa, Dc, Da) (STD_BLEND(mx,Sc,Sa,Dc,Da) + \
  (Dc > ZERO ? (Sc * mx) / Dc : ZERO))
#define CHROMA_DIVIDE COLOR_DIVIDE

// REPLACE	[Sa, Sc] (fade = 1)
#define ALPHA_REPLACE(mx, Sa, Da) Sa
#define COLOR_REPLACE(mx, Sc, Sa, Dc, Da) Sc
#define CHROMA_REPLACE COLOR_REPLACE

// MAX		[max(Sa, Da), MAX(Sc, Dc)]
#define ALPHA_MAX(mx, Sa, Da) (Sa > Da ? Sa : Da)
#define COLOR_MAX(mx, Sc, Sa, Dc, Da) (Sc > Dc ? Sc : Dc)
#define CHROMA_MAX(mx, Sc, Sa, Dc, Da) (mabs(Sc) > mabs(Dc) ? Sc : Dc)

// MIN		[min(Sa, Da), MIN(Sc, Dc)]
#define ALPHA_MIN(mx, Sa, Da) (Sa < Da ? Sa : Da)
#define COLOR_MIN(mx, Sc, Sa, Dc, Da) (Sc < Dc ? Sc : Dc)
#define CHROMA_MIN(mx, Sc, Sa, Dc, Da) (mabs(Sc) < mabs(Dc) ? Sc : Dc)

// DARKEN  	[Sa + Da - Sa*Da, Sc*(1 - Da) + Dc*(1 - Sa) +  min(Sc*Da, Dc*Sa)]
#define ALPHA_DARKEN STD_ALPHA
#define COLOR_DARKEN(mx, Sc, Sa, Dc, Da) (STD_BLEND(mx,Sc,Sa,Dc,Da) + \
  mmin(Sc * Da, Dc * Sa) / mx)
#define CHROMA_DARKEN(mx, Sc, Sa, Dc, Da) (CHROMA_XOR(mx,Sc,Sa,Dc,Da) + \
  (mabs(Sc * Da) < mabs(Dc * Sa) ? Sc * Da : Dc * Sa) / mx)

// LIGHTEN  	[Sa + Da - Sa*Da, Sc*(1 - Da) + Dc*(1 - Sa) +  max(Sc*Da, Dc*Sa)]
#define ALPHA_LIGHTEN STD_ALPHA
#define COLOR_LIGHTEN(mx, Sc, Sa, Dc, Da) (STD_BLEND(mx,Sc,Sa,Dc,Da) + \
  mmax(Sc * Da, Dc * Sa) / mx)
#define CHROMA_LIGHTEN(mx, Sc, Sa, Dc, Da) (CHROMA_XOR(mx,Sc,Sa,Dc,Da) + \
  (mabs(Sc * Da) > mabs(Dc * Sa) ? Sc * Da : Dc * Sa) / mx)

// DST  	[Da, Dc]
#define ALPHA_DST(mx, Sa, Da) Da
#define COLOR_DST(mx, Sc, Sa, Dc, Da) Dc
#define CHROMA_DST COLOR_DST

// DST_ATOP  	[Sa, Sc * (1 - Da) + Dc * Sa]
#define ALPHA_DST_ATOP(mx, Sa, Da) Sa
#define COLOR_DST_ATOP(mx, Sc, Sa, Dc, Da) ((Sc * (mx - Da) + Dc * Sa) / mx)
#define CHROMA_DST_ATOP COLOR_DST_ATOP

// DST_IN  	[Da * Sa, Dc * Sa]
#define ALPHA_DST_IN(mx, Sa, Da) ((Da * Sa) / mx)
#define COLOR_DST_IN(mx, Sc, Sa, Dc, Da) ((Dc * Sa) / mx)
#define CHROMA_DST_IN COLOR_DST_IN

// DST_OUT  	[Da * (1 - Sa), Dc * (1 - Sa)]
#define ALPHA_DST_OUT(mx, Sa, Da) (Da * (mx - Sa) / mx)
#define COLOR_DST_OUT(mx, Sc, Sa, Dc, Da) (Dc * (mx - Sa) / mx)
#define CHROMA_DST_OUT COLOR_DST_OUT

// DST_OVER  	[Sa + Da - Sa*Da, Sc * (1 - Da) + Dc]
#define ALPHA_DST_OVER STD_ALPHA
#define COLOR_DST_OVER(mx, Sc, Sa, Dc, Da) (Sc * (mx - Da)/ mx + Dc)
#define CHROMA_DST_OVER COLOR_DST_OVER

// SRC  		[Sa, Sc]
#define ALPHA_SRC(mx, Sa, Da) Sa
#define COLOR_SRC(mx, Sc, Sa, Dc, Da) Sc
#define CHROMA_SRC COLOR_SRC

// SRC_ATOP  	[Da, Sc * Da + Dc * (1 - Sa)]
#define ALPHA_SRC_ATOP(mx, Sa, Da) Da
#define COLOR_SRC_ATOP(mx, Sc, Sa, Dc, Da) ((Sc * Da + Dc * (mx - Sa)) / mx)
#define CHROMA_SRC_ATOP COLOR_SRC_ATOP

// SRC_IN  	[Sa * Da, Sc * Da]
#define ALPHA_SRC_IN(mx, Sa, Da) ((Sa * Da) / mx)
#define COLOR_SRC_IN(mx, Sc, Sa, Dc, Da) (Sc * Da / mx)
#define CHROMA_SRC_IN COLOR_SRC_IN

// SRC_OUT  	[Sa * (1 - Da), Sc * (1 - Da)]
#define ALPHA_SRC_OUT(mx, Sa, Da) (Sa * (mx - Da) / mx)
#define COLOR_SRC_OUT(mx, Sc, Sa, Dc, Da) (Sc * (mx - Da) / mx)
#define CHROMA_SRC_OUT COLOR_SRC_OUT

// SRC_OVER  	[Sa + Da - Sa*Da, Sc + (1 - Sa) * Dc]
#define ALPHA_SRC_OVER STD_ALPHA
#define COLOR_SRC_OVER(mx, Sc, Sa, Dc, Da) (Sc + Dc * (mx - Sa) / mx)
#define CHROMA_SRC_OVER COLOR_SRC_OVER

// AND 	[Sa * Da, Sc * Dc]
#define ALPHA_AND(mx, Sa, Da) ((Sa * Da) / mx)
#define COLOR_AND(mx, Sc, Sa, Dc, Da) ((Sc * Dc) / mx)
#define CHROMA_AND COLOR_AND

// OR  	[Sa + Da - Sa * Da, Sc + Dc - Sc * Dc]
#define ALPHA_OR(mx, Sa, Da) (Sa + Da - (Sa * Da) / mx)
#define COLOR_OR(mx, Sc, Sa, Dc, Da) (Sc + Dc - (Sc * Dc) / mx)
#define CHROMA_OR COLOR_OR

// XOR 	[Sa + Da - 2 * Sa * Da, Sc * (1 - Da) + Dc * (1 - Sa)]
#define ALPHA_XOR(mx, Sa, Da) (Sa + Da - (TWO * Sa * Da / mx))
#define COLOR_XOR(mx, Sc, Sa, Dc, Da) ((Sc * (mx - Da) + Dc * (mx - Sa)) / mx)
#define CHROMA_XOR COLOR_XOR

//SVG 1.2
//https://www.w3.org/TR/2004/WD-SVG12-20041027/rendering.html
// OVERLAY [Sa + Da - Sa * Da,  Sc*(1 - Da) + Dc*(1 - Sa) +
//   2*Dc < Da ? 2*Sc*Dc : Sa*Da - 2*(Da-Dc)*(Sa-Sc) ]
#define ALPHA_OVERLAY STD_ALPHA
#define COLOR_OVERLAY(mx, Sc, Sa, Dc, Da) (STD_BLEND(mx,Sc,Sa,Dc,Da) + \
  ((TWO * Dc < Da) ? \
    (TWO * Sc * Dc) : (Sa * Da - TWO * (Da - Dc) * (Sa - Sc))) / mx)
#define CHROMA_OVERLAY COLOR_OVERLAY

// SCREEN [Sa + Da - Sa * Da, Sc + Dc - (Sc * Dc)] (same as OR)
#define ALPHA_SCREEN STD_ALPHA
#define COLOR_SCREEN(mx, Sc, Sa, Dc, Da) (Sc + Dc - (Sc * Dc) / mx)
#define CHROMA_SCREEN COLOR_SCREEN

// BURN	[Sa + Da - Sa * Da,  Sc*(1 - Da) + Dc*(1 - Sa) +
//  Sc <= 0 || Sc*Da + Dc*Sa <= Sa*Da ? 0 : (Sc*Da + Dc*Sa - Sa*Da)*Sa/Sc]
#define ALPHA_BURN STD_ALPHA
#define COLOR_BURN(mx, Sc, Sa, Dc, Da) (STD_BLEND(mx,Sc,Sa,Dc,Da) + \
  ((Sc <= ZERO || Sc * Da + Dc * Sa <= Sa * Da) ? ZERO : \
    (Sa * ((Sc * Da + Dc * Sa - Sa * Da) / Sc) / mx)))
#define CHROMA_BURN COLOR_BURN

// DODGE [Sa + Da - Sa * Da,  Sc*(1 - Da) + Dc*(1 - Sa) +
//  Sa <= Sc || Sc*Da + Dc*Sa >= Sa*Da) ? Sa*Da : Dc*Sa / (1 - Sc/Sa)]
#define ALPHA_DODGE STD_ALPHA
#define COLOR_DODGE(mx, Sc, Sa, Dc, Da) (STD_BLEND(mx,Sc,Sa,Dc,Da) + \
  ((Sa <= Sc || Sc * Da + Dc * Sa >= Sa * Da) ? (Sa * Da) : \
    (Sa * ((Dc * Sa) / (Sa - Sc))) / mx))
#define CHROMA_DODGE COLOR_DODGE

// HARDLIGHT [Sa + Da - Sa * Da, Sc*(1 - Da) + Dc*(1 - Sa) +
//  2*Sc < Sa ? 2*Sc*Dc : Sa*Da - 2*(Da - Dc)*(Sa - Sc)]
#define ALPHA_HARDLIGHT STD_ALPHA
#define COLOR_HARDLIGHT(mx, Sc, Sa, Dc, Da) (STD_BLEND(mx,Sc,Sa,Dc,Da) + \
  ((TWO * Sc < Sa) ? \
    (TWO * Sc * Dc) : (Sa * Da - TWO * (Da - Dc) * (Sa - Sc))) / mx)
#define CHROMA_HARDLIGHT COLOR_HARDLIGHT

// SOFTLIGHT [Sa + Da - Sa * Da,  Sc*(1 - Da) + Dc*(1 - Sa) +
//  Da > 0 ? (Dc*Sa + 2*Sc*(Da - Dc))/Da : 0]
#define ALPHA_SOFTLIGHT STD_ALPHA
#define COLOR_SOFTLIGHT(mx, Sc, Sa, Dc, Da) (STD_BLEND(mx,Sc,Sa,Dc,Da) + \
  ((Da > ZERO) ? \
    (Dc * ((Dc*Sa + TWO * Sc * (Da - Dc)) / Da) / mx) : ZERO))
#define CHROMA_SOFTLIGHT COLOR_SOFTLIGHT

// DIFFERENCE [Sa + Da - Sa * Da,  Sc*(1 - Da) + Dc*(1 - Sa) +
//  abs(Sc * Da - Dc * Sa)]
#define ALPHA_DIFFERENCE STD_ALPHA
#define COLOR_DIFFERENCE(mx, Sc, Sa, Dc, Da) (STD_BLEND(mx,Sc,Sa,Dc,Da) + \
  (mabs(Sc * Da - Dc * Sa) / mx))
#define CHROMA_DIFFERENCE COLOR_DIFFERENCE

static inline int   mabs(int32_t v) { return abs(v); }
static inline int   mabs(int64_t v) { return llabs(v); }
static inline float mabs(float v)   { return fabsf(v); }
static inline int   mmin(int32_t a, int32_t b) { return a < b ? a : b; }
static inline int   mmin(int64_t a, int64_t b) { return a < b ? a : b; }
static inline float mmin(float a, float b)   { return a < b ? a : b; }
static inline int   mmax(int32_t a, int32_t b) { return a > b ? a : b; }
static inline int   mmax(int64_t a, int64_t b) { return a > b ? a : b; }
static inline float mmax(float a, float b)   { return a > b ? a : b; }

static inline int32_t aclip(int32_t v, int mx) {
	return v < 0 ? 0 : v > mx ? mx : v;
}
static inline int64_t aclip(int64_t v, int mx) {
	return v < 0 ? 0 : v > mx ? mx : v;
}
static inline float   aclip(float v, float mx) {
	return v < 0 ? 0 : v > mx ? mx : v;
}
static inline float   aclip(float v, int mx) {
	return v < 0 ? 0 : v > mx ? mx : v;
}
static inline int   aclip(int v, float mx) {
	return v < 0 ? 0 : v > mx ? mx : v;
}
static inline int32_t cclip(int32_t v, int mx) {
	return v > (mx/=2) ? mx : v < (mx=(-mx-1)) ? mx : v;
}
static inline int64_t cclip(int64_t v, int mx) {
	return v > (mx/=2) ? mx : v < (mx=(-mx-1)) ? mx : v;
}
static inline float   cclip(float v, float mx) {
	return v > (mx/=2) ? mx : v < (mx=(-mx)) ? mx : v;
}
static inline float   cclip(float v, int mx) {
	return v > (mx/=2) ? mx : v < (mx=(-mx-1)) ? mx : v;
}
static inline int   cclip(int v, float mx) {
	return v > (mx/=2) ? mx : v < (mx=(-mx-1)) ? mx : v;
}

/* number of data pts per unit x in lookup table */
#define TRANSFORM_SPP    (4096)
/* bits of fraction past TRANSFORM_SPP on kernel index accumulation */
#define INDEX_FRACTION   (8)
#define TRANSFORM_MIN    (.5 / TRANSFORM_SPP)

#define ZTYP(ty) typedef ty z_##ty __attribute__ ((__unused__))
ZTYP(int8_t);	ZTYP(uint8_t);
ZTYP(int16_t);	ZTYP(uint16_t);
ZTYP(int32_t);	ZTYP(uint32_t);
ZTYP(int64_t);	ZTYP(uint64_t);
ZTYP(float);	ZTYP(double);

#define ALPHA3_BLEND(FN, typ, inp, out, mx, iofs, oofs, rnd) \
  typ inp0 = (typ)inp[0], inp1 = (typ)inp[1] - iofs; \
  typ inp2 = (typ)inp[2] - iofs, inp3 = mx; \
  typ out0 = (typ)out[0], out1 = (typ)out[1] - oofs; \
  typ out2 = (typ)out[2] - oofs, out3 = mx; \
  r = COLOR_##FN(mx, inp0, inp3, out0, out3); \
  if( oofs ) { \
    g = CHROMA_##FN(mx, inp1, inp3, out1, out3); \
    b = CHROMA_##FN(mx, inp2, inp3, out2, out3); \
  } \
  else { \
    g = COLOR_##FN(mx, inp1, inp3, out1, out3); \
    b = COLOR_##FN(mx, inp2, inp3, out2, out3); \
  }

#define ALPHA4_BLEND(FN, typ, inp, out, mx, iofs, oofs, rnd) \
  typ inp0 = (typ)inp[0], inp1 = (typ)inp[1] - iofs; \
  typ inp2 = (typ)inp[2] - iofs, inp3 = inp[3]; \
  typ out0 = (typ)out[0], out1 = (typ)out[1] - oofs; \
  typ out2 = (typ)out[2] - oofs, out3 = out[3]; \
  r = COLOR_##FN(mx, inp0, inp3, out0, out3); \
  if( oofs ) { \
    g = CHROMA_##FN(mx, inp1, inp3, out1, out3); \
    b = CHROMA_##FN(mx, inp2, inp3, out2, out3); \
  } \
  else { \
    g = COLOR_##FN(mx, inp1, inp3, out1, out3); \
    b = COLOR_##FN(mx, inp2, inp3, out2, out3); \
  } \
  a = ALPHA_##FN(mx, inp3, out3)

#define ALPHA_STORE(out, ofs, mx) \
  out[0] = r; \
  out[1] = g + ofs; \
  out[2] = b + ofs

#define ALPHA3_STORE(out, ofs, mx) \
  r = aclip(r, mx); \
  g = ofs ? cclip(g, mx) : aclip(g, mx); \
  b = ofs ? cclip(b, mx) : aclip(b, mx); \
  if( trnsp ) { \
    r = (r * opcty + out0 * trnsp) / mx; \
    g = (g * opcty + out1 * trnsp) / mx; \
    b = (b * opcty + out2 * trnsp) / mx; \
  } \
  ALPHA_STORE(out, ofs, mx)

#define ALPHA4_STORE(out, ofs, mx) \
  r = aclip(r, mx); \
  g = ofs ? cclip(g, mx) : aclip(g, mx); \
  b = ofs ? cclip(b, mx) : aclip(b, mx); \
  if( trnsp ) { \
    r = (r * opcty + out0 * trnsp) / mx; \
    g = (g * opcty + out1 * trnsp) / mx; \
    b = (b * opcty + out2 * trnsp) / mx; \
    a = (a * opcty + out3 * trnsp) / mx; \
  } \
  ALPHA_STORE(out, ofs, mx); \
  out[3] = aclip(a, mx)


#define BLEND_SWITCH(FN) \
	switch( mode ) { \
        case TRANSFER_NORMAL: 		FN(NORMAL); \
        case TRANSFER_ADDITION:		FN(ADDITION); \
        case TRANSFER_SUBTRACT:		FN(SUBTRACT); \
        case TRANSFER_MULTIPLY:		FN(MULTIPLY); \
        case TRANSFER_DIVIDE: 		FN(DIVIDE); \
        case TRANSFER_REPLACE: 		FN(REPLACE); \
        case TRANSFER_MAX: 		FN(MAX); \
        case TRANSFER_MIN: 		FN(MIN); \
	case TRANSFER_DARKEN:		FN(DARKEN); \
	case TRANSFER_LIGHTEN:		FN(LIGHTEN); \
	case TRANSFER_DST:		FN(DST); \
	case TRANSFER_DST_ATOP:		FN(DST_ATOP); \
	case TRANSFER_DST_IN:		FN(DST_IN); \
	case TRANSFER_DST_OUT:		FN(DST_OUT); \
	case TRANSFER_DST_OVER:		FN(DST_OVER); \
	case TRANSFER_SRC:		FN(SRC); \
	case TRANSFER_SRC_ATOP:		FN(SRC_ATOP); \
	case TRANSFER_SRC_IN:		FN(SRC_IN); \
	case TRANSFER_SRC_OUT:		FN(SRC_OUT); \
	case TRANSFER_SRC_OVER:		FN(SRC_OVER); \
	case TRANSFER_AND:		FN(AND); \
	case TRANSFER_OR:		FN(OR); \
	case TRANSFER_XOR:		FN(XOR); \
	case TRANSFER_OVERLAY:		FN(OVERLAY); \
	case TRANSFER_SCREEN:		FN(SCREEN); \
	case TRANSFER_BURN:		FN(BURN); \
	case TRANSFER_DODGE:		FN(DODGE); \
	case TRANSFER_HARDLIGHT:	FN(HARDLIGHT); \
	case TRANSFER_SOFTLIGHT:	FN(SOFTLIGHT); \
	case TRANSFER_DIFFERENCE:	FN(DIFFERENCE); \
	}

class OverlayKernel
{
public:
	OverlayKernel(int interpolation_type);
	~OverlayKernel();

	float *lookup;
	float width;
	int n;
	int type;
};

class DirectEngine;

class DirectPackage : public LoadPackage
{
public:
	DirectPackage();

	int out_row1, out_row2;
};

class NNEngine;

class NNPackage : public LoadPackage
{
public:
	NNPackage();

	int out_row1, out_row2;
};

class SampleEngine;

class SamplePackage : public LoadPackage
{
public:
	SamplePackage();

	int out_col1, out_col2;
};


class DirectUnit : public LoadClient
{
public:
	DirectUnit(DirectEngine *server);
	~DirectUnit();

	void process_package(LoadPackage *package);
	DirectEngine *engine;
};

class NNUnit : public LoadClient
{
public:
	NNUnit(NNEngine *server);
	~NNUnit();

	void process_package(LoadPackage *package);

	NNEngine *engine;
};

class SampleUnit : public LoadClient
{
public:
	SampleUnit(SampleEngine *server);
	~SampleUnit();

	void process_package(LoadPackage *package);

	SampleEngine *engine;
};


class DirectEngine : public LoadServer
{
public:
	DirectEngine(int cpus);
	~DirectEngine();

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	VFrame *output;
	VFrame *input;
	int in_x1;
	int in_y1;
	int out_x1;
	int out_x2;
	int out_y1;
	int out_y2;
	float alpha;
	int mode;
};

class NNEngine : public LoadServer
{
public:
	NNEngine(int cpus);
	~NNEngine();

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	VFrame *output;
	VFrame *input;
	float in_x1;
	float in_x2;
	float in_y1;
	float in_y2;
	float out_x1;
	float out_x2;
	int out_x1i;
	int out_x2i;
	float out_y1;
	float out_y2;
	float alpha;
	int mode;

	int *in_lookup_x;
	int *in_lookup_y;
};

class SampleEngine : public LoadServer
{
public:
	SampleEngine(int cpus);
	~SampleEngine();

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	VFrame *output;
	VFrame *input;
	OverlayKernel *kernel;

	int col_out1;
	int col_out2;
	int row_in;

	float in1;
	float in2;
	float out1;
	float out2;

	float alpha;
	int mode;

	int *lookup_sx0;
	int *lookup_sx1;
	int *lookup_sk;
	float *lookup_wacc;
	int kd;
};

class OverlayFrame
{
public:
	OverlayFrame(int cpus = 1);
	virtual ~OverlayFrame();

	int overlay(VFrame *output,
		VFrame *input,
		float in_x1,
		float in_y1,
		float in_x2,
		float in_y2,
		float out_x1,
		float out_y1,
		float out_x2,
		float out_y2,
		float alpha,
		int mode,
		int interpolation_type);

	DirectEngine *direct_engine;
	NNEngine *nn_engine;
	SampleEngine *sample_engine;

	VFrame *temp_frame;
	int cpus;
	OverlayKernel *kernel[4];
};

#endif
