
/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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

// Originally developed by Heroine Virtual Ltd.
// Support for multiple encodings, outline (stroke) by
// Andraz Tori <Andraz.tori1@guest.arnes.si>
// Additional support for UTF-8 by
// Paolo Rampino aka Akirad <info at tuttoainternet.it>

#include "asset.h"
#include "bccmodels.h"
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "cstrdup.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "filesystem.h"
#include "indexable.h"
#include "ft2build.h"
#include FT_GLYPH_H
#include FT_BBOX_H
#include FT_OUTLINE_H
#include FT_STROKER_H
#include "language.h"
#include "mwindow.inc"
#include "overlayframe.h"
#include "plugin.h"
#include "renderengine.h"
#include "titler.h"
#include "titlerwindow.h"
#include "transportque.h"
#include "vrender.h"
#include "workarounds.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <byteswap.h>
#include <iconv.h>
#include <sys/stat.h>
#include <fontconfig/fontconfig.h>

#define FIXED_FONT "bitstream vera sans mono (bitstream)"
#define SMALL (1.0 / 64.0)

#define MAX_FLT  3.40282347e+38
#define MIN_FLT -3.40282347e+38

REGISTER_PLUGIN(TitleMain)

#ifdef X_HAVE_UTF8_STRING
#define DEFAULT_ENCODING "UTF-8"
#else
#define DEFAULT_ENCODING "ISO8859-1"
#endif
#define DEFAULT_TIMECODEFORMAT TIME_HMS

static inline int kw_strcmp(const char *ap, const char *bp) {
	return !strcmp(ap, bp) ? 0 : strcmp(ap,_(bp));
}

TitleConfig::TitleConfig()
{
	strcpy(font, "fixed");
	strcpy(encoding, DEFAULT_ENCODING);
	style = FONT_ALIAS;
	size = 24;
	color = BLACK;
	alpha = 0xff;
	outline_size = 0.;
	outline_color = WHITE;
	outline_alpha = 0xff;
	color_stroke = 0xff0000;
	stroke_width = 0.0;
	motion_strategy = NO_MOTION;
	line_pitch = 0;
	loop = 0;
	hjustification = JUSTIFY_CENTER;
	vjustification = JUSTIFY_MID;
	fade_in = 0.0;  fade_out = 0.0;
	pixels_per_second = 100.0;
	wtext = 0;  wsize = 0;  wlen = 0;
	title_x = title_y = 0.0;
	title_w = title_h = 0;
	window_w = 860;
	window_h = 460;
	next_keyframe_position = 0;
	prev_keyframe_position = 0;
	timecode = 0;
	dropshadow = 2;
	background = 0;
	strcpy(background_path, "");
	timecode_format = DEFAULT_TIMECODEFORMAT;
	drag = 0;
	loop_playback = 0;
}

TitleConfig::~TitleConfig()
{
	delete [] wtext;
}

int TitleConfig::equivalent(TitleConfig &that)
{
	return !strcasecmp(font, that.font) &&
		!strcasecmp(encoding, that.encoding) &&
		style == that.style &&
		size == that.size &&
		color == that.color &&
		alpha == that.alpha &&
		outline_size == that.outline_size &&
		outline_color == that.outline_color &&
		outline_alpha == that.outline_alpha &&
		color_stroke == that.color_stroke &&
		stroke_width == that.stroke_width &&
// dont require redraw for these
//		motion_strategy == that.motion_strategy &&
		line_pitch == that.line_pitch &&
//		loop == that.loop &&
		hjustification == that.hjustification &&
		vjustification == that.vjustification &&
//		fade_in == that.fade_in && fade_out == that.fade_out &&
//		EQUIV(pixels_per_second, that.pixels_per_second) &&
		wlen == that.wlen &&
		!memcmp(wtext, that.wtext, wlen * sizeof(wchar_t)) &&
//		title_x == that.title_x && title_y == that.title_y &&
		title_w == that.title_w && title_h == that.title_h &&
//		window_w == that.window_w && window_h == that.window_h &&
		timecode == that.timecode &&
		dropshadow == that.dropshadow &&
		background == that.background &&
		!strcmp(background_path, that.background_path) &&
		timecode_format == that.timecode_format &&
//		drag == that.drag &&
		loop_playback == that.loop_playback;
}

void TitleConfig::copy_from(TitleConfig &that)
{
	strcpy(font, that.font);
	strcpy(encoding, that.encoding);
	style = that.style;
	size = that.size;
	color = that.color;
	alpha = that.alpha;
	outline_size = that.outline_size;
	outline_color = that.outline_color;
	outline_alpha = that.outline_alpha;
	color_stroke = that.color_stroke;
	stroke_width = that.stroke_width;
	motion_strategy = that.motion_strategy;
	line_pitch = that.line_pitch;
	loop = that.loop;
	hjustification = that.hjustification;
	vjustification = that.vjustification;
	fade_in = that.fade_in;
	fade_out = that.fade_out;
	pixels_per_second = that.pixels_per_second;
	demand(wlen = that.wlen);
	memcpy(wtext, that.wtext, that.wlen * sizeof(wchar_t));
	title_x = that.title_x;  title_y = that.title_y;
	title_w = that.title_w;  title_h = that.title_h;
	window_w = that.window_w;  window_h = that.window_h;
	timecode = that.timecode;
	dropshadow = that.dropshadow;
	background = that.background;
	strcpy(background_path, that.background_path);
	timecode_format = that.timecode_format;
	drag = that.drag;
	loop_playback = that.loop_playback;
}

void TitleConfig::interpolate(TitleConfig &prev, TitleConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	strcpy(font, prev.font);
	strcpy(encoding, prev.encoding);
	style = prev.style;
	size = prev.size;
	color = prev.color;
	alpha = prev.alpha;
	outline_size = prev.outline_size;
	outline_color = prev.outline_color;
	outline_alpha = prev.outline_alpha;
	color_stroke = prev.color_stroke;
	stroke_width = prev.stroke_width;
	motion_strategy = prev.motion_strategy;
	line_pitch = prev.line_pitch;
	loop = prev.loop;
	hjustification = prev.hjustification;
	vjustification = prev.vjustification;
	fade_in = prev.fade_in;
	fade_out = prev.fade_out;
	pixels_per_second = prev.pixels_per_second;
	demand(wlen = prev.wlen);
	memcpy(wtext, prev.wtext, prev.wlen * sizeof(wchar_t));
	wtext[wlen] = 0;
	this->title_x = prev.title_x == next.title_x ? prev.title_x :
		prev.title_x * prev_scale + next.title_x * next_scale;
	this->title_y = prev.title_y == next.title_y ? prev.title_y :
		prev.title_y * prev_scale + next.title_y * next_scale;
	this->title_w = prev.title_w == next.title_w ? prev.title_w :
		prev.title_w * prev_scale + next.title_w * next_scale;
	this->title_h = prev.title_h == next.title_h ? prev.title_h :
		prev.title_h * prev_scale + next.title_h * next_scale;
	window_w = prev.window_w;
	window_h = prev.window_h;
	timecode = prev.timecode;
	this->dropshadow = prev.dropshadow == next.dropshadow ? prev.dropshadow :
		prev.dropshadow * prev_scale + next.dropshadow * next_scale;
	background = prev.background;
	strcpy(background_path, prev.background_path);
	timecode_format = prev.timecode_format;
	drag = prev.drag;
	loop_playback = prev.loop_playback;
}

int TitleConfig::demand(long sz)
{
	if( wtext && wsize >= sz ) return 0;
	delete [] wtext;
	wsize = sz + wlen/2 + 0x1000;
	wtext = new wchar_t[wsize+1];
	wtext[wsize] = 0;
	return 1;
}

void TitleConfig::to_wtext(const char *from_enc, const char *text, int tlen)
{
	demand(tlen);
	wlen = BC_Resources::encode(from_enc, BC_Resources::wide_encoding,
		(char*)text,tlen, (char *)wtext,sizeof(*wtext)*wsize) / sizeof(wchar_t);
	while( wlen > 0 && !wtext[wlen-1] ) --wlen;
}


TitleGlyph::TitleGlyph()
{
	char_code = 0;
	size = 0;
	data = 0;
	data_stroke = 0;
	freetype_index = 0;
	width = 0;
	height = 0;
	style = 0;
	pitch = 0;
	left = 0;
	top = 0;
	bottom = 0;
	right = 0;
	advance_x = 0;
}


TitleGlyph::~TitleGlyph()
{
//printf("TitleGlyph::~TitleGlyph 1\n");
	if( data ) delete data;
	if( data_stroke ) delete data_stroke;
}


GlyphPackage::GlyphPackage() : LoadPackage()
{
	glyph = 0;
}

GlyphUnit::GlyphUnit(TitleMain *plugin, GlyphEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
	freetype_library = 0;
	freetype_face = 0;
}

GlyphUnit::~GlyphUnit()
{
	if( freetype_library )
		ft_Done_FreeType(freetype_library);
}

static inline void to_mono(VFrame *data)
{
	if( !data ) return;
	int w = data->get_w(), h = data->get_h();
	uint8_t **rows = data->get_rows();
	for( int y=0; y<h; ++y ) {
		uint8_t *dp = rows[y];
		for( int x=0; x<w; ++x,++dp ) *dp = *dp >= 0x80 ? 0xff : 0;
	}
}

void GlyphUnit::process_package(LoadPackage *package)
{
	GlyphPackage *pkg = (GlyphPackage*)package;
	TitleGlyph *glyph = pkg->glyph;
	BC_Resources *resources =  BC_WindowBase::get_resources();
	if( resources->font_debug )
		printf("GlyphUnit load glyph (%s) %04x, '%c'\n", glyph->font->displayname,
			(unsigned)glyph->char_code, (unsigned)glyph->char_code);
	int result = 0;
	char new_path[BCTEXTLEN];
	if( plugin->load_freetype_face(freetype_library, freetype_face, glyph->font->path) ) {
		printf(_("GlyphUnit::process_package FT_New_Face failed, path=%s\n"),
			glyph->font->path);
		result = 1;
	}
	if( !result ) {
		int gindex = ft_Get_Char_Index(freetype_face, glyph->char_code);
		if( !gindex && !freetype_face->charmap && // if no default charmap
		    freetype_face->charmaps && freetype_face->charmaps[0] &&
		    !ft_Select_Charmap(freetype_face, freetype_face->charmaps[0]->encoding) ) {
			gindex = ft_Get_Char_Index(freetype_face, glyph->char_code);
		}
		if( gindex == 0 ) {
printf("GlyphUnit::process_package 1 glyph not found (%s) %04x, '%c'\n",
 glyph->font->displayname, (unsigned)glyph->char_code, (unsigned)glyph->char_code);
			// Search replacement font
			if( resources->find_font_by_char(glyph->char_code, new_path, freetype_face) ) {
				plugin->load_freetype_face(freetype_library,
					freetype_face, new_path);
				gindex = ft_Get_Char_Index(freetype_face, glyph->char_code);
			}
		}
		ft_Set_Pixel_Sizes(freetype_face, glyph->size, 0);

		if( gindex == 0 ) {
// carrige return
			if( glyph->char_code != 10 )
				printf(_("GlyphUnit::process_package FT_Load_Char failed - char: %li.\n"),
					glyph->char_code);
// Prevent a crash here
			glyph->width = 8;  glyph->height = 8;
			glyph->pitch = 8;  glyph->advance_x = 8;
			glyph->left = 9;   glyph->top = 9;
			glyph->right = 0;  glyph->bottom = 0;
			glyph->freetype_index = 0;
			glyph->data = new VFrame(glyph->width, glyph->height, BC_A8, glyph->pitch);
			glyph->data->clear_frame();
			glyph->data_stroke = 0;

// create outline glyph
			if( plugin->config.stroke_width >= SMALL &&
				(plugin->config.style & BC_FONT_OUTLINE) ) {
				glyph->data_stroke = new VFrame(glyph->width, glyph->height, BC_A8, glyph->pitch);
				glyph->data_stroke->clear_frame();
			}
		}
// char found and no outline desired
		else if( plugin->config.stroke_width < SMALL ||
			!(plugin->config.style & BC_FONT_OUTLINE) ) {
			FT_Glyph glyph_image;
			FT_BBox bbox;
			FT_Bitmap bm;
			ft_Load_Glyph(freetype_face, gindex, FT_LOAD_DEFAULT);
			ft_Get_Glyph(freetype_face->glyph, &glyph_image);
			ft_Outline_Get_BBox(&((FT_OutlineGlyph) glyph_image)->outline, &bbox);
//			printf("Stroke: Xmin: %ld, Xmax: %ld, Ymin: %ld, yMax: %ld\n",
//					bbox.xMin,bbox.xMax, bbox.yMin, bbox.yMax);

			glyph->width = bm.width = ((bbox.xMax - bbox.xMin + 63) >> 6);
			glyph->height = bm.rows = ((bbox.yMax - bbox.yMin + 63) >> 6);
			glyph->pitch = bm.pitch = bm.width;
			bm.pixel_mode = FT_PIXEL_MODE_GRAY;
			bm.num_grays = 256;
			glyph->left = (bbox.xMin + 31) >> 6;
			glyph->top = (bbox.yMax + 31) >> 6;
			glyph->right = (bbox.xMax + 31) >> 6;
			glyph->bottom = (bbox.yMin + 31) >> 6;
			glyph->freetype_index = gindex;
			glyph->advance_x = ((freetype_face->glyph->advance.x + 31) >> 6);
//printf("GlyphUnit::process_package 1 width=%d height=%d pitch=%d left=%d top=%d advance_x=%d freetype_index=%d\n",
//glyph->width, glyph->height, glyph->pitch, glyph->left, glyph->top, glyph->advance_x, glyph->freetype_index);

			glyph->data = new VFrame(glyph->width, glyph->height, BC_A8, glyph->pitch);
			glyph->data->clear_frame();
			bm.buffer = glyph->data->get_data();
			ft_Outline_Translate(&((FT_OutlineGlyph) glyph_image)->outline,
				- bbox.xMin, - bbox.yMin);
			ft_Outline_Get_Bitmap( freetype_library,
				&((FT_OutlineGlyph) glyph_image)->outline,
				&bm);
			ft_Done_Glyph(glyph_image);
		}
		else {
// Outline desired and glyph found
			FT_Glyph glyph_image;
			FT_Stroker stroker;
			FT_Outline outline;
			FT_Bitmap bm;
			FT_BBox bbox;
			FT_UInt npoints, ncontours;

			ft_Load_Glyph(freetype_face, gindex, FT_LOAD_DEFAULT);
			ft_Get_Glyph(freetype_face->glyph, &glyph_image);

// check if the outline is ok (non-empty);
			ft_Outline_Get_BBox(&((FT_OutlineGlyph) glyph_image)->outline, &bbox);
			if( bbox.xMin == 0 && bbox.xMax == 0 &&
			    bbox.yMin == 0 && bbox.yMax == 0 ) {
				ft_Done_Glyph(glyph_image);
				glyph->width = 0;   glyph->height = 0;
				glyph->left = 0;    glyph->top = 0;
				glyph->right = 0;   glyph->bottom = 0;
				glyph->pitch = 0;
				glyph->data =
					new VFrame(glyph->width, glyph->height, BC_A8, glyph->pitch);
				glyph->data_stroke =
					new VFrame(glyph->width, glyph->height, BC_A8, glyph->pitch);
				glyph->advance_x =((int)(freetype_face->glyph->advance.x +
					plugin->config.stroke_width * 64)) >> 6;
				return;
			}
#if FREETYPE_MAJOR > 2 || (FREETYPE_MAJOR == 2 && FREETYPE_MINOR >= 2)
			ft_Stroker_New(freetype_library, &stroker);
#else
			ft_Stroker_New(((FT_LibraryRec *)freetype_library)->memory, &stroker);
#endif
			ft_Stroker_Set(stroker, (int)(plugin->config.stroke_width * 64),
				FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
			ft_Stroker_ParseOutline(stroker, &((FT_OutlineGlyph) glyph_image)->outline,1);
			ft_Stroker_GetCounts(stroker,&npoints, &ncontours);
			if( npoints == 0 && ncontours == 0 ) {
// this never happens, but FreeType has a bug regarding Linotype's Palatino font
				ft_Stroker_Done(stroker);
				ft_Done_Glyph(glyph_image);
				glyph->width = 0;   glyph->height = 0;
				glyph->left = 0;    glyph->top = 0;
				glyph->right = 0;   glyph->bottom = 0;
				glyph->pitch = 0;
				glyph->data =
					new VFrame(glyph->width, glyph->height, BC_A8, glyph->pitch);
				glyph->data_stroke =
					new VFrame(glyph->width, glyph->height, BC_A8, glyph->pitch);
				glyph->advance_x =((int)(freetype_face->glyph->advance.x +
					plugin->config.stroke_width * 64)) >> 6;
				return;
			};

			ft_Outline_New(freetype_library, npoints, ncontours, &outline);
			outline.n_points=0;
			outline.n_contours=0;
			ft_Stroker_Export(stroker, &outline);
			ft_Outline_Get_BBox(&outline, &bbox);
			ft_Outline_Translate(&outline, - bbox.xMin, - bbox.yMin);
			ft_Outline_Translate(&((FT_OutlineGlyph) glyph_image)->outline,
					- bbox.xMin,
					- bbox.yMin + (int)(plugin->config.stroke_width*32));
//			printf("Stroke: Xmin: %ld, Xmax: %ld, Ymin: %ld, yMax: %ld\n"
//					"Fill	Xmin: %ld, Xmax: %ld, Ymin: %ld, yMax: %ld\n",
//					bbox.xMin,bbox.xMax, bbox.yMin, bbox.yMax,
//					bbox_fill.xMin,bbox_fill.xMax, bbox_fill.yMin, bbox_fill.yMax);

			glyph->width = bm.width = ((bbox.xMax - bbox.xMin) >> 6)+1;
			glyph->height = bm.rows = ((bbox.yMax - bbox.yMin) >> 6) +1;
			glyph->pitch = bm.pitch = bm.width;
			bm.pixel_mode = FT_PIXEL_MODE_GRAY;
			bm.num_grays = 256;
			glyph->left = (bbox.xMin + 31) >> 6;
			glyph->top = (bbox.yMax + 31) >> 6;
			glyph->right = (bbox.xMax + 31) >> 6;
			glyph->bottom = (bbox.yMin + 31) >> 6;
			glyph->freetype_index = gindex;
			int real_advance = ((int)ceil((float)freetype_face->glyph->advance.x +
				plugin->config.stroke_width * 64) >> 6);
			glyph->advance_x = glyph->width + glyph->left;
			if( real_advance > glyph->advance_x )
				glyph->advance_x = real_advance;
//printf("GlyphUnit::process_package 1 width=%d height=%d "
// "pitch=%d left=%d top=%d advance_x=%d freetype_index=%d\n",
// glyph->width, glyph->height, glyph->pitch, glyph->left,
// glyph->top, glyph->advance_x, glyph->freetype_index);


//printf("GlyphUnit::process_package 1\n");
			glyph->data = new VFrame(glyph->width, glyph->height, BC_A8, glyph->pitch);
			glyph->data->clear_frame();
			glyph->data_stroke = new VFrame(glyph->width, glyph->height, BC_A8, glyph->pitch);
			glyph->data_stroke->clear_frame();
// for debugging	memset(	glyph->data_stroke->get_data(), 60, glyph->pitch * glyph->height);
			bm.buffer=glyph->data->get_data();
			ft_Outline_Get_Bitmap( freetype_library,
				&((FT_OutlineGlyph) glyph_image)->outline,
				&bm);
			bm.buffer=glyph->data_stroke->get_data();
			ft_Outline_Get_Bitmap( freetype_library,
           		&outline,
				&bm);
			ft_Outline_Done(freetype_library,&outline);
			ft_Stroker_Done(stroker);
			ft_Done_Glyph(glyph_image);

//printf("GlyphUnit::process_package 2\n");
		}

		if( !(glyph->style & FONT_ALIAS) ) {
			to_mono(glyph->data);
			to_mono(glyph->data_stroke);
		}
	}
}


GlyphEngine::GlyphEngine(TitleMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void GlyphEngine::init_packages()
{
	int current_package = 0;
	for( int i=0; i<plugin->title_glyphs.count(); ++i ) {
		if( !plugin->title_glyphs[i]->data ) {
			GlyphPackage *pkg = (GlyphPackage*)get_package(current_package++);
			pkg->glyph = plugin->title_glyphs[i];
		}
	}
}

LoadClient* GlyphEngine::new_client()
{
	return new GlyphUnit(plugin, this);
}

LoadPackage* GlyphEngine::new_package()
{
	return new GlyphPackage;
}


// Copy a single character to the text mask
TitlePackage::TitlePackage()
 : LoadPackage()
{
	x = y = 0;
	chr = 0;
}


TitleUnit::TitleUnit(TitleMain *plugin, TitleEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->engine = server;
	this->chr = 0;
}

static void get_mask_colors(int rgb, int color_model, int &rr, int &gg, int &bb)
{
	int r = 0xff & (rgb>>16), g = 0xff & (rgb>>8), b = 0xff & (rgb>>0);
	if( BC_CModels::is_yuv(color_model) ) YUV::yuv.rgb_to_yuv_8(r,g,b);
	rr = r;  gg = g; bb = b;
}

void TitleUnit::draw_frame(int mode, VFrame *dst, VFrame *src, int x, int y)
{
	int inp_w = src->get_w(), inp_h = src->get_h();
	int out_w = dst->get_w(), out_h = dst->get_h();
	unsigned char **inp_rows = src->get_rows();
	unsigned char **out_rows = dst->get_rows();

	int x_inp = 0, y_inp = 0;
	int x_out = x;
	if( x_out < 0 ) { x_inp = -x_out;  x_out = 0; }
	if( x_out+inp_w > out_w ) inp_w = out_w-x_out;
	if( x_inp >= inp_w || x_out >= out_w ) return;
	int y_out = y;
	if( y_out < 0 ) { y_inp = -y_out;  y_out = 0; }
	if( y_out+inp_h > out_h ) inp_h = out_h-y_out;
	if( y_inp >= inp_h || y_out >= out_h ) return;
	int color = chr->color, max = 0xff;
	int alpha = chr->alpha * chr->fade;
	int ofs = BC_CModels::is_yuv(dst->get_color_model()) ? 0x80 : 0x00;

	switch( mode ) {
	case DRAW_ALPHA: {
		while( y_inp < inp_h && y_out < out_h ) {
			uint8_t *inp = inp_rows[y_inp], *out = out_rows[y_out];
			for( int xin=x_inp,xout=x_out*4+3; xin<inp_w; ++xin,xout+=4 ) {
				out[xout] = STD_ALPHA(max, inp[xin], out[xout]);
			}
			++y_inp;  ++y_out;
		}
		break; }
	case DRAW_COLOR: {
		int r = 0, g = 0, b = 0;
		get_mask_colors(color, plugin->text_model, r, g, b);
		while( y_inp < inp_h && y_out < out_h ) {
			uint8_t *inp = inp_rows[y_inp], *out = out_rows[y_out];
			for( int xin=x_inp,xout=x_out*4+0; xin<inp_w; ++xin,xout+=4 ) {
				int in_a = inp[xin], out_a = out[xout+3];
				if( in_a + out_a == 0 ) continue;
				if( in_a >= out_a ) { // crayola for top draw
					out[xout+0] = r;  out[xout+1] = g;  out[xout+2] = b;
					out[xout+3] = alpha * in_a / max;
				}
				else {
					int i_r = r, i_g = g-ofs, i_b = b-ofs;
					int o_r = out[xout+0], o_g = out[xout+1]-ofs, o_b = out[xout+2]-ofs;
					out[xout+0] = COLOR_NORMAL(max, i_r, in_a, o_r, out_a);
					out[xout+1] = COLOR_NORMAL(max, i_g, in_a, o_g, out_a) + ofs;
					out[xout+2] = COLOR_NORMAL(max, i_b, in_a, o_b, out_a) + ofs;
					out[xout+3] = alpha * STD_ALPHA(max, in_a, out_a) / max;
				}
			}
			++y_inp;  ++y_out;
		}
		break; }
	case DRAW_IMAGE: {
		while( y_inp < inp_h && y_out < out_h ) {
			uint8_t *inp = inp_rows[y_inp], *out = out_rows[y_out];
			for( int xin=x_inp,xout=x_out*4+0; xin<inp_w; ++xin,xout+=4 ) {
				int xinp = xin*4;
				int in_a = inp[xinp+3], out_a = out[xout+3];
				if( in_a + out_a == 0 ) continue;
				if( in_a >= out_a ) {
					int i_r = inp[xinp+0], i_g = inp[xinp+1], i_b = inp[xinp+2];
					out[xout+0] = i_r;  out[xout+1] = i_g;  out[xout+2] = i_b;
					out[xout+3] = alpha * in_a / max;
				}
				else {
					int i_r = inp[xinp+0], i_g = inp[xinp+1]-ofs, i_b = inp[xinp+2]-ofs;
					int o_r = out[xout+0], o_g = out[xout+1]-ofs, o_b = out[xout+2]-ofs;
					out[xout+0] = COLOR_NORMAL(max, i_r, in_a, o_r, out_a);
					out[xout+1] = COLOR_NORMAL(max, i_g, in_a, o_g, out_a) + ofs;
					out[xout+2] = COLOR_NORMAL(max, i_b, in_a, o_b, out_a) + ofs;
					out[xout+3] = alpha * STD_ALPHA(max, in_a, out_a) / max;
				}
			}
			++y_inp;  ++y_out;
		}
		break; }
	}
}


void TitleUnit::process_package(LoadPackage *package)
{
	TitlePackage *pkg = (TitlePackage*)package;
	int x = pkg->x, y = pkg->y;
	chr = pkg->chr;
	switch( chr->typ ) {
	case CHAR_IMAGE: {
		VFrame *vframe = (VFrame *)chr->vp;
		if( !vframe ) return;
		draw_frame(DRAW_IMAGE, plugin->text_mask, vframe, x, y);
		break; }
	case CHAR_GLYPH: {
		if( chr->wch == 0 || chr->wch == '\n') return;
		TitleGlyph *glyph = (TitleGlyph *)chr->vp;
		if( !glyph ) return;
		if( engine->do_dropshadow ) {
			x += plugin->config.dropshadow;
			y += plugin->config.dropshadow;
		}
		else if( plugin->config.dropshadow < 0 ) {
			x -= plugin->config.dropshadow;
			y -= plugin->config.dropshadow;
		}
		int mode = engine->do_dropshadow ? DRAW_ALPHA : DRAW_COLOR;
		draw_frame(mode, plugin->text_mask, glyph->data, x, y);
		if( plugin->config.stroke_width >= SMALL && (plugin->config.style & BC_FONT_OUTLINE) )
			draw_frame(mode, plugin->stroke_mask, glyph->data_stroke, x, y);
		break; }
	}
}

TitleEngine::TitleEngine(TitleMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void TitleEngine::init_packages()
{
	int current_package = 0;
	for( int i=plugin->visible_char1; i<plugin->visible_char2; ++i ) {
		TitlePackage *pkg = (TitlePackage*)get_package(current_package++);
		TitleChar *chr = plugin->title_chars[i];
		TitleRow *row = plugin->title_rows[chr->row];
		pkg->chr = chr;
		pkg->x = row->x0 + chr->x - plugin->mask_x1;
		pkg->y = row->y0 - chr->y - plugin->mask_y1;
//printf("draw '%c' at %d,%d\n",(int)pkg->chr->wch, pkg->x, pkg->y);
	}
}

LoadClient* TitleEngine::new_client()
{
	return new TitleUnit(plugin, this);
}

LoadPackage* TitleEngine::new_package()
{
	return new TitlePackage;
}


// Copy a single character to the text mask
TitleOutlinePackage::TitleOutlinePackage()
 : LoadPackage()
{
}


TitleOutlineUnit::TitleOutlineUnit(TitleMain *plugin, TitleOutlineEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->engine = server;
}

void TitleOutlineUnit::process_package(LoadPackage *package)
{
	TitleOutlinePackage *pkg = (TitleOutlinePackage*)package;
	int r = 0, g = 0, b = 0, outline_a = plugin->config.outline_alpha;
	get_mask_colors(plugin->config.outline_color, plugin->mask_model, r, g, b);

	unsigned char **outline_rows = plugin->outline_mask->get_rows();
	unsigned char **text_rows = plugin->text_mask->get_rows();
	int mask_w1 = plugin->text_mask->get_w()-1;
	int mask_h1 = plugin->text_mask->get_h()-1;
	int oln_sz = plugin->config.outline_size;

	if( engine->pass == 0 ) {
// get max alpha under outline size macropixel
		for( int y=pkg->y1, my=pkg->y2; y<my; ++y ) {
			unsigned char *out_row = outline_rows[y];
			int y1 = y - oln_sz, y2 = y + oln_sz;
			CLAMP(y1, 0, mask_h1);  CLAMP(y2, 0, mask_h1);
			for( int x=0, mx=plugin->text_mask->get_w(); x<mx; ++x ) {
				int x1 = x - oln_sz, x2 = x + oln_sz;
				CLAMP(x1, 0, mask_w1);  CLAMP(x2, 0, mask_w1);
				int max_a = 0;
				for( int yy=y1; yy<=y2; ++yy ) {
					unsigned char *text_row = text_rows[yy];
					for( int xx = x1; xx <= x2; ++xx ) {
						unsigned char *pixel = text_row + xx*4;
						if( pixel[3] > max_a ) max_a = pixel[3];
					}
				}
				unsigned char *out = out_row + x*4;
				out[0] = r;  out[1] = g;  out[2] = b;
				out[3] = (max_a * outline_a) / 0xff;
			}
		}
		return;
	}
	else {
// Overlay text mask on top of outline mask
		int ofs = BC_CModels::is_yuv(plugin->output->get_color_model()) ? 0x80 : 0;
		for( int y=pkg->y1, my=pkg->y2; y<my; ++y ) {
			unsigned char *outline_row = outline_rows[y];
			unsigned char *text_row = text_rows[y];
			for( int x=0, mx=plugin->text_mask->get_w(); x<mx; ++x ) {
				unsigned char *out = text_row + x * 4;
				unsigned char *inp = outline_row + x * 4;
				int out_a = out[3], in_a = inp[3];
				int transparency = in_a * (0xff - out_a) / 0xff;
				out[0] = (out[0] * out_a + inp[0] * transparency) / 0xff;
				out[1] = ((out[1]-ofs) * out_a + (inp[1]-ofs) * transparency) / 0xff + ofs;
				out[2] = ((out[2]-ofs) * out_a + (inp[2]-ofs) * transparency) / 0xff + ofs;
				out[3] = in_a + out_a - in_a*out_a / 0xff;
			}
		}
	}
}

TitleOutlineEngine::TitleOutlineEngine(TitleMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void TitleOutlineEngine::init_packages()
{
	int mask_h = plugin->text_mask->get_h();
	if( !mask_h ) return;
	int py1 = 0, py2 = 0;
	int pkgs = get_total_packages();
	for( int i=0; i<pkgs; py1=py2 ) {
		TitleOutlinePackage *pkg = (TitleOutlinePackage*)get_package(i);
		py2 = (++i * mask_h)/ pkgs;
		pkg->y1 = py1;  pkg->y2 = py2;
	}
}

void TitleOutlineEngine::do_outline()
{
	pass = 0;  process_packages();
	pass = 1;  process_packages();
}

LoadClient* TitleOutlineEngine::new_client()
{
	return new TitleOutlineUnit(plugin, this);
}

LoadPackage* TitleOutlineEngine::new_package()
{
	return new TitleOutlinePackage;
}


TitleCurNudge::TitleCurNudge(TitleParser *parser, TitleMain *plugin)
 : TitleStack<int>(parser, 0)
{
}
TitleCurColor::TitleCurColor(TitleParser *parser, TitleMain *plugin)
 : TitleStack<int>(parser, plugin->config.color)
{
}
TitleCurAlpha::TitleCurAlpha(TitleParser *parser, TitleMain *plugin)
 : TitleStack<int>(parser, plugin->config.alpha)
{
}
TitleCurSize::TitleCurSize(TitleParser *parser, TitleMain *plugin)
 : TitleStack<float>(parser, plugin->config.size)
{
}
TitleCurBold::TitleCurBold(TitleParser *parser, TitleMain *plugin)
 : TitleStack<int>(parser, (plugin->config.style & BC_FONT_BOLD) ? 1 : 0)
{
}
TitleCurItalic::TitleCurItalic(TitleParser *parser, TitleMain *plugin)
 : TitleStack<int>(parser, (plugin->config.style & BC_FONT_ITALIC) ? 1 : 0)
{
}
TitleCurFont::TitleCurFont(TitleParser *parser, TitleMain *plugin)
 : TitleStack<BC_FontEntry*>(parser, plugin->config_font())
{
}
TitleCurCaps::TitleCurCaps(TitleParser *parser, TitleMain *plugin)
 : TitleStack<int>(parser, 0)
{
}
TitleCurUnder::TitleCurUnder(TitleParser *parser, TitleMain *plugin)
 : TitleStack<int>(parser, 0)
{
}
TitleCurBlink::TitleCurBlink(TitleParser *parser, TitleMain *plugin)
 : TitleStack<float>(parser, 0)
{
}
TitleCurFixed::TitleCurFixed(TitleParser *parser, TitleMain *plugin)
 : TitleStack<int>(parser, 0)
{
}
TitleCurAlias::TitleCurAlias(TitleParser *parser, TitleMain *plugin)
 : TitleStack<int>(parser, (plugin->config.style & FONT_ALIAS) ? 1 : 0)
{
}
TitleCurSuper::TitleCurSuper(TitleParser *parser, TitleMain *plugin)
 : TitleStack<int>(parser, 0)
{
}

TitleParser::TitleParser(TitleMain *plugin)
 : plugin(plugin),
   cur_nudge(this, plugin),
   cur_color(this, plugin),
   cur_alpha(this, plugin),
   cur_size(this, plugin),
   cur_bold(this, plugin),
   cur_italic(this, plugin),
   cur_font(this, plugin),
   cur_caps(this, plugin),
   cur_under(this, plugin),
   cur_blink(this, plugin),
   cur_fixed(this, plugin),
   cur_alias(this, plugin),
   cur_super(this, plugin)
{
	bfr = out = plugin->config.wtext;
	lmt = bfr + plugin->config.wlen;
}

TitleMain::TitleMain(PluginServer *server)
 : PluginVClient(server)
{
	text_mask = 0;
	stroke_mask = 0;
	outline_mask = 0;
	glyph_engine = 0;
	title_engine = 0;
	translate = 0;
	outline_engine = 0;
	freetype_library = 0;
	freetype_face = 0;
	text_font[0] = 0;
	window_w = window_h = 0;
	title_x = title_y = 0;
	title_w = title_h = 0;
	text_w = text_h = 0;
	text_x = text_y = 0;  mask_w = mask_h = 0;
	mask_x1 = mask_y1 = mask_x2 = mask_y2 = 0;
	text_rows = 0;
	visible_row1 = visible_char1 = 0;
	visible_row2 = visible_char2 = 0;
	fade = 1;
	input = 0; output = 0;
	output_model = BC_RGBA8888;
	mask_model = BC_RGBA8888;
	background = 0; bg_file = 0; bg_frame = 0;
	render_engine = 0;  video_cache = 0;
	overlay_frame = 0;
	cpus = PluginClient::smp + 1;
	if( cpus > 8 ) cpus = 8;
	last_position = -1;
	need_reconfigure = 1;
}

TitleMain::~TitleMain()
{
	if( background ) {
		background->Garbage::remove_user();
		background = 0;
	}
	delete render_engine;
	delete video_cache;
	delete overlay_frame;
	delete bg_file;
	delete bg_frame;
	delete text_mask;
	delete outline_mask;
	delete stroke_mask;
	delete glyph_engine;
	delete title_engine;
	if( freetype_face )
		ft_Done_Face(freetype_face);
	if( freetype_library )
		ft_Done_FreeType(freetype_library);
	delete translate;
	delete outline_engine;
}

const char* TitleMain::plugin_title() { return N_("Title"); }
int TitleMain::is_realtime() { return 1; }
int TitleMain::is_synthesis() { return 1; }

NEW_WINDOW_MACRO(TitleMain, TitleWindow);


void TitleMain::build_previews(TitleWindow *gui)
{
	BC_Resources *resources =  BC_WindowBase::get_resources();
	ArrayList<BC_FontEntry*>&fonts = *resources->fontlist;

	for( int font_number=0; font_number<fonts.size(); ++font_number ) {
		BC_FontEntry *font = fonts.get(font_number);
// already have examples
		if( font->image ) return;
	}

// create example bitmaps
	FT_Library freetype_library = 0;      	// Freetype library
	FT_Face freetype_face = 0;
	const char *test_string = "Aa";
	char new_path[BCTEXTLEN];
	int text_height = gui->get_text_height(LARGEFONT);
	int max_height = 3*text_height/2, max_width = 2 * max_height;
	int text_color = resources->default_text_color;
	int r = (text_color >> 16) & 0xff;
	int g = (text_color >> 8) & 0xff;
	int b = text_color & 0xff;
// dimensions for each line
	int height[fonts.size()];
	int ascent[fonts.size()];

// pass 1 gets the extents for all the fonts
// pass 2 draws the image
	int total_w = 0;
	int total_h = 0;
	for( int pass=0; pass<2; ++pass ) {
		if( resources->font_debug )
			printf("Titler: build previews pass %d\n",pass);
//printf("TitleMain::build_previews %d %d %d\n",
//__LINE__, text_height, total_h);
		for( int font_number=0; font_number<fonts.size(); ++font_number ) {
			BC_FontEntry *font = fonts[font_number];
// test if font of same name has been processed
			int skip = 0;
			for( int i=0; i<font_number; ++i ) {
				if( !strcasecmp(fonts[i]->displayname, font->displayname) ) {
					if( pass == 1 ) {
						font->image = new VFrame(*fonts[i]->image);
					}
					skip = 1;
					break;
				}
			}

			if( skip ) continue;
			if( resources->font_debug )
				printf("Titler: preview %s = %s\n",font->displayname, font->path);
			if( pass > 0 ) {
				font->image = new VFrame;
				font->image->set_use_shm(0);
				font->image->reallocate(0, -1, 0, 0, 0,
					total_w, total_h, BC_RGBA8888, -1);
				font->image->clear_frame();
			}

			int current_w = 1, current_h = 1;
			int current_x = 0, current_ascent = 0;
			int len = strlen(test_string);

			for( int j=0; j<len; ++j ) {
				FT_ULong c = test_string[j];
// memory leaks here are fatal
//				check_char_code_path(freetype_library,
//					font->path,
//					c,
//					(char *)new_path);
				strcpy(new_path, font->path);
				if( load_freetype_face(freetype_library, freetype_face, new_path)) continue;
				ft_Set_Pixel_Sizes(freetype_face, text_height, 0);
				if( ft_Load_Char(freetype_face, c, FT_LOAD_RENDER) ) continue;
				int glyph_w = freetype_face->glyph->bitmap.width;
				int glyph_h = freetype_face->glyph->bitmap.rows;
				if( glyph_h > max_height ) glyph_h = max_height;
				int glyph_a = freetype_face->glyph->advance.x >> 6;
				int glyph_t = freetype_face->glyph->bitmap_top;
				if( pass == 0 ) {
					current_w = current_x + glyph_w;
					if( current_w > max_width ) current_w = max_width;
					if( total_w < current_w ) total_w = current_w;
					if( current_ascent < glyph_t ) current_ascent = glyph_t;
					if( current_h < glyph_h ) current_h = glyph_h;
					if( total_h < glyph_h ) total_h = glyph_h;
				}
				else {
// copy 1 row at a time, center vertically
					int out_y = (total_h-height[font_number])/2 + ascent[font_number]-glyph_t;
					if( out_y < 0 ) out_y = 0;
					for( int in_y = 0; in_y < glyph_h && out_y < total_h; ++in_y, ++out_y ) {
						unsigned char *in_row = freetype_face->glyph->bitmap.buffer +
							freetype_face->glyph->bitmap.pitch * in_y;
						int out_x = current_x;
						unsigned char *out_row = font->image->get_rows()[out_y] +
							out_x * 4;

						for( int in_x = 0; in_x < glyph_w && out_x < total_w; ++in_x, ++out_x ) {
							*out_row = (*in_row * r +
								(0xff - *in_row) * *out_row) / 0xff; ++out_row;
							*out_row = (*in_row * g +
								(0xff - *in_row) * *out_row) / 0xff; ++out_row;
							*out_row = (*in_row * b +
								(0xff - *in_row) * *out_row) / 0xff; ++out_row;
							*out_row = MAX(*in_row, *out_row);  ++out_row;
							in_row++;
						}
					}
				}
				current_x += glyph_a;
			}
			height[font_number] = current_h;
			ascent[font_number] = current_ascent;
		}
	}

	if( freetype_library ) ft_Done_FreeType(freetype_library);
}


//This checks if char_code is on the selected font, else it changes font to the first compatible //Akirad
int TitleMain::check_char_code_path(FT_Library &freetype_library,
	char *path_old, FT_ULong &char_code, char *path_new)
{
	FT_Face temp_freetype_face;
	FcPattern *pat;
	FcFontSet *fs;
	FcObjectSet *os;
	FcChar8 *file, *format;
	FcConfig *config;
	FcPattern *font;

	fcInit();
	config = fcConfigGetCurrent();
	fcConfigSetRescanInterval(config, 0);

	pat = fcPatternCreate();
	os = fcObjectSetBuild( FC_FILE, FC_FONTFORMAT, (char *) 0);
	fs = fcFontList(config, pat, os);
	int found = 0;
	char tmpstring[BCTEXTLEN];
	int limit_to_truetype = 0; //if you want to limit search to truetype put 1
	if( !freetype_library ) ft_Init_FreeType(&freetype_library);
	if( !ft_New_Face(freetype_library, path_old, 0, &temp_freetype_face) ) {
		ft_Set_Pixel_Sizes(temp_freetype_face, 128, 0);
		int gindex = ft_Get_Char_Index(temp_freetype_face, char_code);
		if( gindex != 0 && char_code == 10 ) {
			strcpy(path_new, path_old);
			found = 1;
		}
	}

	if( !found ) {
		for( int i=0; fs && i<fs->nfont; ++i ) {
			font = fs->fonts[i];
			fcPatternGetString(font, FC_FONTFORMAT, 0, &format);
			if( strcmp((char *)format, "TrueType") && !limit_to_truetype ) continue;
			if( fcPatternGetString(font, FC_FILE, 0, &file) != FcResultMatch ) continue;
			sprintf(tmpstring, "%s", file);
			if( !ft_New_Face(freetype_library, tmpstring, 0, &temp_freetype_face) ) continue;
			ft_Set_Pixel_Sizes(temp_freetype_face, 128, 0);
			int gindex = ft_Get_Char_Index(temp_freetype_face, char_code);
			if( gindex != 0 && char_code == 10 ) {
				sprintf(path_new, "%s", tmpstring);
				found = 1;
				goto done;
			}
		}
	}

done:
	if( fs ) fcFontSetDestroy(fs);
	if( temp_freetype_face ) ft_Done_Face(temp_freetype_face);
	temp_freetype_face = 0;

	if( !found ) {
		strcpy(path_new, path_old);
		return 1;
	}

	return 0;
}


int TitleMain::load_freetype_face(FT_Library &freetype_library,
	FT_Face &freetype_face, const char *path)
{
//printf("TitleMain::load_freetype_face 1\n");
	if( !freetype_library )
		ft_Init_FreeType(&freetype_library);
	if( freetype_face )
		ft_Done_Face(freetype_face);
	freetype_face = 0;
//printf("TitleMain::load_freetype_face 2\n");

// Use freetype's internal function for loading font
	if( ft_New_Face(freetype_library, path, 0, &freetype_face) ) {
		fprintf(stderr, _("TitleMain::load_freetype_face %s failed.\n"), path);
		freetype_face = 0;
		freetype_library = 0;
		return 1;
	}
	return 0;
}

int TitleMain::load_font(BC_FontEntry *font)
{
	if( !font || load_freetype_face(freetype_library,freetype_face, font->path) ) return 1;
	strcpy(text_font, font->displayname);
	return 0;
}


Indexable *TitleMain::open_background(const char *filename)
{
	delete render_engine;  render_engine = 0;
	delete video_cache;    video_cache = 0;
	delete bg_file;        bg_file = new File;

	Asset *asset = new Asset(filename);
	int result = bg_file->open_file(server->preferences, asset, 1, 0);
	if( result == FILE_OK ) return (Indexable *)asset;
// not asset
	asset->Garbage::remove_user();
	delete bg_file;   bg_file = 0;
	if( result != FILE_IS_XML ) return 0;
// nested edl
	FileXML xml_file;
	if( xml_file.read_from_file(filename) ) return 0;
	EDL *nested_edl = new EDL;
	nested_edl->create_objects();
	nested_edl->set_path(filename);
	nested_edl->load_xml(&xml_file, LOAD_ALL);
	TransportCommand command;
	//command.command = audio_tracks ? NORMAL_FWD : CURRENT_FRAME;
	command.command = CURRENT_FRAME;
	command.get_edl()->copy_all(nested_edl);
	command.change_type = CHANGE_ALL;
	command.realtime = 0;
	render_engine = new RenderEngine(0, server->preferences, 0, 0);
	render_engine->set_vcache(video_cache = new CICache(server->preferences));
	render_engine->arm_command(&command);
	return (Indexable *)nested_edl;
}

int TitleMain::read_background(VFrame *frame, int64_t position, int color_model)
{
	int result = 1;
	VFrame *iframe = frame;
	int bw = background->get_w(), bh = background->get_h();
	if( background->is_asset ) {
		Asset *asset = (Asset *)background;
		if( bw != asset->width || bh != asset->height )
			iframe = new_temp(asset->width, asset->height, color_model);
		int64_t source_position = (int64_t)(position *
			asset->frame_rate / project_frame_rate);
		if( config.loop_playback ) {
			int64_t loop_size = asset->get_video_frames();
			source_position -= (int64_t)(source_position / loop_size) * loop_size;
		}
		if( bg_file ) {
			bg_file->set_video_position(source_position, 0);
			result = bg_file->read_frame(iframe);
		}
	}
	else {
		EDL *nested_edl = (EDL *)background;
		if( color_model != nested_edl->session->color_model ||
		    bw != nested_edl->session->output_w ||
		    bh != nested_edl->session->output_h )
			iframe = new_temp(
				nested_edl->session->output_w,
				nested_edl->session->output_h,
				nested_edl->session->color_model);
		int64_t source_position = (int64_t)(position *
			nested_edl->session->frame_rate / project_frame_rate);
		if( config.loop_playback ) {
			int64_t loop_size = nested_edl->get_video_frames();
			source_position -= (int64_t)(source_position / loop_size) * loop_size;
		}
                if( render_engine->vrender )
                        result = render_engine->vrender->process_buffer(iframe, source_position, 0);
	}
	if( !result && iframe != frame )
		frame->transfer_from(iframe);
	return result;
}

void TitleMain::draw_background()
{
	if( background ) {
		if( strcmp(background->path, config.background_path) ) {
			if( background ) {
				background->Garbage::remove_user();
				background = 0;
			}
			if( bg_file ) {
				delete bg_file;
				bg_file = 0;
			}
		}
	}
	if( !background && config.background_path[0] && !access(config.background_path,R_OK) )
		background = open_background(config.background_path);
	if( background ) {
		int bw = background->get_w(), bh = background->get_h();
		if( bg_frame && (bg_frame->get_w() != bw || bg_frame->get_h() != bh ||
		    bg_frame->get_color_model() != output_model) ) {
			delete bg_frame;  bg_frame = 0;
		}
		if( !bg_frame )
			bg_frame = new VFrame(bw, bh, output_model);
		int64_t position = get_source_position() - get_source_start();
		if( !read_background(bg_frame, position, output_model) ) {
			if( !overlay_frame )
				overlay_frame = new OverlayFrame(cpus);
			float in_x1 = 0, in_x2 = bg_frame->get_w();
			float in_y1 = 0, in_y2 = bg_frame->get_h();
			float out_x1 = config.title_x, out_x2 = out_x1 + title_w;
			float out_y1 = config.title_y, out_y2 = out_y1 + title_h;
			overlay_frame->overlay(output, bg_frame,
				in_x1,in_y1, in_x2,in_y2,
				out_x1,out_y1, out_x2,out_y2,
				1, TRANSFER_NORMAL, CUBIC_LINEAR);
		}
	}
}

BC_FontEntry* TitleMain::get_font(const char *font_name, int style)
{
	if( !strcmp("fixed", font_name) )
		font_name = FIXED_FONT;
	int flavor = FL_WIDTH_MASK |
	    ((style & BC_FONT_ITALIC) != 0 ? FL_SLANT_ITALIC | FL_SLANT_OBLIQUE : FL_SLANT_ROMAN) |
	    ((style & BC_FONT_BOLD) != 0 ? FL_WEIGHT_BOLD | FL_WEIGHT_DEMIBOLD |
			FL_WEIGHT_EXTRABOLD| FL_WEIGHT_BLACK | FL_WEIGHT_EXTRABLACK :
			FL_WEIGHT_BOOK | FL_WEIGHT_NORMAL | FL_WEIGHT_MEDIUM |
			FL_WEIGHT_LIGHT | FL_WEIGHT_EXTRALIGHT | FL_WEIGHT_THIN);

	int mask = FL_WEIGHT_MASK | FL_SLANT_MASK | FL_WIDTH_MASK;

	BC_Resources *resources =  BC_WindowBase::get_resources();
	BC_FontEntry *font = resources->find_fontentry(font_name, flavor, mask, style);
	if( font && strcmp(font_name, font->displayname) ) font = 0;
	return font;
}
BC_FontEntry* TitleMain::config_font()
{
	BC_FontEntry *font = get_font(config.font, config.style);
	if( !font || load_font(font) )
		load_font(font = get_font(FIXED_FONT,0));
	return font;
}


static inline bool is_ltr(wchar_t wch) { return iswalpha(wch); }
static inline bool is_nbr(wchar_t wch) { return iswdigit(wch); }
static inline bool is_ws(wchar_t wch) { return wch==' ' || wch=='\t'; }
static inline bool is_idch(wchar_t wch) { return is_ltr(wch) || is_nbr(wch) || wch=='_'; }

// return eof=-1, chr=0, opener=1, closer=2
int TitleParser::wget(wchar_t &wch)
{
	wchar_t *wip = wid, *wtp = wtxt;  *wip = 0;  *wtp = 0;
	int ilen = sizeof(wid)/sizeof(wid[0]);
	int tlen = sizeof(wtxt)/sizeof(wtxt[0]);
	int ich;
	while( (ich=wnext()) >= 0 ) {
		if( ich == '\\' ) {
			if( (ich=wnext()) < 0 ) break;
			if( !ich || ich == '\n' ) continue;
			wch = ich;
			return 0;
		}
		if( ich == '<' ) break;
		if( ich != '#' ) { wch = ich;  return 0; }
		while( (ich=wnext()) >= 0 && ich != '\n' );
		if( ich < 0 ) break;
	}
	if( ich < 0 ) return -1;
 	int ret = 1;  long pos = tell();
	if( (ich=wnext()) == '/' ) { ret = 2; ich = wnext(); }
	if( is_ltr(ich) ) {
		*wip++ = ich;
		while( is_idch(ich=wnext()) )
			if( --ilen > 0 ) *wip++ = ich;
	}
	*wip = 0;
	while( is_ws(ich) ) ich = wnext();
	while( ich >= 0 && ich != '>' ) {
		if( ich == '\n' || ich == '<' ) { ich = -1;  break; }
		if( ich == '\\' && (ich=wnext()) < 0 ) break;
		if( --tlen > 0 ) *wtp++ = ich;
		ich = wnext();
	}
	*wtp = 0;
	if( ich < 0 ) { ich = '<';  seek(pos);  ret = 0; }
	wch = ich;
	return ret;
}
int TitleParser::tget(wchar_t &wch)
{
	int ret = wget(wch);
	if( ret > 0 ) {
		int wid_len = wcslen(wid)+1;
		BC_Resources::encode(
			BC_Resources::wide_encoding, plugin->config.encoding,
			(char*)wid,wid_len*sizeof(wid[0]), (char *)id,sizeof(id));
		int wtxt_len = wcslen(wtxt)+1;
		BC_Resources::encode(
			BC_Resources::wide_encoding, plugin->config.encoding,
			(char*)wtxt,wtxt_len*sizeof(wtxt[0]), (char *)text,sizeof(text));
	}
	return ret;
}

TitleGlyph *TitleMain::get_glyph(FT_ULong char_code, BC_FontEntry *font, int size, int style)
{
	for( int i=0, n=title_glyphs.count(); i<n; ++i ) {
		TitleGlyph *glyph = title_glyphs[i];
		if( glyph->char_code == char_code && glyph->font == font &&
		    glyph->size == size && glyph->style == style )
			return glyph;
	}
	return 0;
}

int TitleMain::get_width(TitleGlyph *cur, TitleGlyph *nxt)
{
	if( !cur || cur->char_code == '\n' ) return 0;
	int result = cur->advance_x;
	if( !nxt ) return result;
	FT_Vector kerning;
	if( !ft_Get_Kerning(freetype_face,
	    cur->freetype_index, nxt->freetype_index,
	    ft_kerning_default, &kerning) )
		result += (kerning.x >> 6);
	return result;
}


VFrame *TitleMain::get_image(const char *path)
{
	for( int i=0; i<title_images.count(); ++i ) {
		if( !strcmp(path, title_images[i]->path) )
			return title_images[i]->vframe;
	}
	return 0;
}

VFrame *TitleMain::add_image(const char *path)
{
	VFrame *vframe = get_image(path);
	if( !vframe && (vframe=VFramePng::vframe_png(path)) != 0 ) {
		if( vframe->get_color_model() != text_model ) {
			VFrame *frame = new VFrame(vframe->get_w(), vframe->get_h(),
				text_model, 0);
			frame->transfer_from(vframe);  delete vframe;
			vframe = frame;
		}
		title_images.append(new TitleImage(path, vframe));
	}
	return vframe;
}

int TitleCurColor::set(const char *txt)
{
#define BCOLOR(NM) { #NM, NM }
	static const struct { const char *name; int color; } colors[] = {
		BCOLOR(MNBLUE),  BCOLOR(ORANGE),  BCOLOR(BLOND), 
		BCOLOR(MNGREY),  BCOLOR(FGGREY),  BCOLOR(FTGREY),   BCOLOR(DKGREY),
		BCOLOR(LTGREY),  BCOLOR(MEGREY),  BCOLOR(DMGREY),   BCOLOR(MDGREY),
		BCOLOR(LTPURPLE),BCOLOR(MEPURPLE),BCOLOR(MDPURPLE), BCOLOR(DKPURPLE),
		BCOLOR(LTCYAN),  BCOLOR(MECYAN),  BCOLOR(MDCYAN),   BCOLOR(DKCYAN),
		BCOLOR(YELLOW),  BCOLOR(LTYELLOW),BCOLOR(MEYELLOW), BCOLOR(MDYELLOW),
		BCOLOR(LTGREEN), BCOLOR(DKGREEN), BCOLOR(DKYELLOW), BCOLOR(LTPINK),
		BCOLOR(PINK),    BCOLOR(LTBLUE),  BCOLOR(DKBLUE),
		BCOLOR(RED),     BCOLOR(GREEN),   BCOLOR(BLUE),
		BCOLOR(BLACK),   BCOLOR(WHITE),
	};
	int color;
	if( *txt ) {
		if( txt[0] == '#' ) {
			if( sscanf(&txt[1], "%x", &color) != 1 ) return 1;
		}
		else {
			int i = sizeof(colors)/sizeof(colors[0]);
			while( --i >= 0 && strcasecmp(txt, colors[i].name) );
			if( i < 0 ) return 1;
			color = colors[i].color;
		}
	}
	else
		color = parser->plugin->config.color;
	push(color);
	return 0;
}

int TitleCurAlpha::set(const char *txt)
{
	float a = !*txt ?
		parser->plugin->config.alpha :
		strtof(txt,(char**)&txt) * 255;
	int alpha = a;
	if( *txt || alpha < 0 || alpha > 255 ) return 1;
	push(alpha);
	return 0;
}

int TitleCurSize::set(const char *txt)
{
	float size = 0;
	if( !*txt ) {
		size = parser->plugin->config.size;
	}
	else if( *txt == '+' || *txt == '-' ) {
		size = *this;
		for( int ch; (ch=*txt)!=0; ++txt ) {
			if( ch == '+' ) { size *= 5./4.; continue; }
			if( ch == '-' ) { size *= 4./5.; continue; }
			return 1;
		}
	}
	else {
		size = strtof(txt,(char**)&txt);
	}
	if( *txt || size <= 0 || size > 2048 ) return 1;
	int style = parser->cur_font.style();
	if( !parser->cur_font.set(0,style) ) return 1;
	push(size);
	return 0;
}
int TitleCurSize::unset(const char *txt)
{
	if( pop() ) return 1;
	int style = parser->cur_font.style();
	parser->cur_font.set(0,style);
	return 0;
}


int TitleCurBold::set(const char *txt)
{
	int bold = !*txt ? 1 :
		strtol(txt,(char**)&txt,0);
	if( *txt || bold < 0 || bold > 1 ) return 1;
	int style = parser->cur_font.style();
	if( bold ) style |= BC_FONT_BOLD;
	else style &= ~BC_FONT_BOLD;
	if( !parser->cur_font.set(0,style) ) return 1;
	push(bold);
	return 0;
}
int TitleCurBold::unset(const char *txt)
{
	if( pop() ) return 1;
	int style = parser->cur_font.style();
	parser->cur_font.set(0,style);
	return 0;
}

int TitleCurItalic::set(const char *txt)
{
	int italic = !*txt ? 1 :
		strtol(txt,(char**)&txt,0);
	if( *txt || italic < 0 || italic > 1 ) return 1;
	int style = parser->cur_font.style();
	if( italic ) style |= BC_FONT_ITALIC;
	else style &= ~BC_FONT_ITALIC;
	if( !parser->cur_font.set(0,style) ) return 1;
	push(italic);
	return 0;
}
int TitleCurItalic::unset(const char *txt)
{
	if( pop() ) return 1;
	int style = parser->cur_font.style();
	parser->cur_font.set(0,style);
	return 0;
}


int TitleCurFont::style()
{
	int style = 0;
	if( parser->cur_bold ) style |= BC_FONT_BOLD;
	if( parser->cur_italic ) style |= BC_FONT_ITALIC;
	return style;
}
BC_FontEntry* TitleCurFont::get(const char *txt, int style)
{
	if( !txt ) txt = parser->plugin->text_font;
	else if( !*txt ) txt = parser->plugin->config.font;
	return parser->plugin->get_font(txt, style);
}
BC_FontEntry *TitleCurFont::set(const char *txt, int style)
{
	BC_FontEntry *font = get(txt, style);
	if( !font || parser->plugin->load_font(font) ) return 0;
	if( !txt ) (BC_FontEntry*&)*this = font;
	return font;
}
int TitleCurFont::set(const char *txt)
{
	BC_FontEntry *font = set(txt, style());
	if( !font ) return 1;
	push(font);
	return 0;
}
int TitleCurFont::unset(const char *txt)
{
	if( *txt || pop() ) return 1;
	BC_FontEntry *font = *this;
	if( !font ) return 1;
	font = get(font->displayname, style());
	if( !font ) return 1;
	(BC_FontEntry*&)*this = font;
	return 0;
}

int TitleCurCaps::set(const char *txt)
{
	int caps = !*txt ? 1 : strtol(txt,(char **)&txt,0);
	if( *txt || caps < -1 || caps > 1 ) return 1;
	push(caps);
	return 0;
}

int TitleCurBlink::set(const char *txt)
{
	float blink = !*txt ? 1 : strtof(txt,(char **)&txt);
	if( *txt ) return 1;
	push(blink);
	return 0;
}

int TitleCurFixed::set(const char *txt)
{
	int fixed = !*txt ?
		parser->cur_size*3/4 :
		strtol(txt,(char **)&txt,0);
	if( *txt || fixed < 0 ) return 1;
	push(fixed);
	return 0;
}

int TitleCurAlias::set(const char *txt)
{
	int alias = !*txt ? 1 : strtol(txt,(char **)&txt,0);
	if( *txt ) return 1;
	push(alias);
	return 0;
}

int TitleCurSuper::set(const char *txt)
{
	int super = !*txt ? 1 : strtol(txt,(char **)&txt,0);
	if( *txt || super < -1 || super > 1 ) return 1;
	push(super);
	return 0;
}

int TitleCurNudge::set(const char *txt)
{
	if( !*txt ) return 1;
	short nx = strtol(txt,(char **)&txt,0);
	if( *txt++ != ',' ) return 1;
	short ny = strtol(txt,(char **)&txt,0);
	if( *txt ) return 1;
	int nudge = (nx << 16) | (ny & 0xffff);
	push(nudge);
	return 0;
}

int TitleParser::set_attributes(int ret)
{
        if( !kw_strcmp(id,KW_NUDGE) )  return ret>1 ? cur_nudge.unset(text)  : cur_nudge.set(text);
        if( !kw_strcmp(id,KW_COLOR) )  return ret>1 ? cur_color.unset(text)  : cur_color.set(text);
        if( !kw_strcmp(id,KW_ALPHA) )  return ret>1 ? cur_alpha.unset(text)  : cur_alpha.set(text);
        if( !kw_strcmp(id,KW_FONT) )   return ret>1 ? cur_font.unset(text)   : cur_font.set(text);
        if( !kw_strcmp(id,KW_SIZE) )   return ret>1 ? cur_size.unset(text)   : cur_size.set(text);
        if( !kw_strcmp(id,KW_BOLD) )   return ret>1 ? cur_bold.unset(text)   : cur_bold.set(text);
        if( !kw_strcmp(id,KW_ITALIC) ) return ret>1 ? cur_italic.unset(text) : cur_italic.set(text);
        if( !kw_strcmp(id,KW_CAPS) )   return ret>1 ? cur_caps.unset(text)   : cur_caps.set(text);
        if( !kw_strcmp(id,KW_UL) )     return ret>1 ? cur_under.unset(text)  : cur_under.set(text);
        if( !kw_strcmp(id,KW_BLINK) )  return ret>1 ? cur_blink.unset(text)  : cur_blink.set(text);
        if( !kw_strcmp(id,KW_FIXED) )  return ret>1 ? cur_fixed.unset(text)  : cur_fixed.set(text);
        if( !kw_strcmp(id,KW_ALIAS) )  return ret>1 ? cur_alias.unset(text)  : cur_alias.set(text);
        if( !kw_strcmp(id,KW_SUP) )    return ret>1 ? cur_super.unset(text)  : cur_super.set(text);
	return 1;
}


void TitleMain::load_glyphs()
{
// Build table of all glyphs needed
	TitleParser wchrs(this);
	int total_packages = 0;

	while( !wchrs.eof() ) {
		wchar_t wch1 = wchrs.wcur(), wch;
		long ipos = wchrs.tell();
		int ret = wchrs.tget(wch);
		if( ret > 0 ) {
			if( !wchrs.set_attributes(ret) ) continue;
			if( !kw_strcmp(wchrs.id,KW_PNG) && add_image(wchrs.text) ) continue;
			wch = wch1;  wchrs.seek(ipos+1);
			ret = 0;
		}
		if( ret || wch == '\n' ) continue;

		int cur_caps = wchrs.cur_caps;
		if( cur_caps > 0 ) wch = towupper(wch);
		else if( cur_caps < 0 ) wch = towlower(wch);
		BC_FontEntry *cur_font = wchrs.cur_font;
		int cur_size = wchrs.cur_size;
		int cur_style = 0;
		int cur_bold  = wchrs.cur_bold;
		if( cur_bold ) cur_style |= BC_FONT_BOLD;
		int cur_italic  = wchrs.cur_italic;
		if( cur_italic ) cur_style |= BC_FONT_ITALIC;
		int cur_alias  = wchrs.cur_alias;
		if( cur_alias ) cur_style |= FONT_ALIAS;
		int cur_super = wchrs.cur_super;
		if( cur_super ) cur_size /= 2;
		int exists = 0;
		for( int j=0; j<title_glyphs.count(); ++j ) {
			TitleGlyph *glyph = title_glyphs[j];
			if( glyph->char_code == (FT_ULong)wch && glyph->font == cur_font &&
			    glyph->size == cur_size && glyph->style == cur_style ) {
				exists = 1;   break;
			}
		}

		if( !exists && cur_font ) {
			total_packages++;
			TitleGlyph *glyph = new TitleGlyph;
			glyph->char_code = (FT_ULong)wch;
			glyph->font = cur_font;
			glyph->size = cur_size;
			glyph->style = cur_style;
			title_glyphs.append(glyph);
		}
	}

	if( !glyph_engine )
		glyph_engine = new GlyphEngine(this, cpus);

	glyph_engine->set_package_count(total_packages);
	glyph_engine->process_packages();
}


TitleImage::TitleImage(const char *path, VFrame *vfrm)
{
	this->path = cstrdup(path);
	this->vframe = vfrm;
}
TitleImage::~TitleImage()
{
	delete [] path;
	delete vframe;
}

TitleChar *TitleChar::init(int typ, void *vp)
{
	wch = 0;
	flags = 0;
	this->typ = typ;
	this->vp = vp;
	x = y = 0;
	row = dx = 0;
	blink = 0.;
	size = 0.;
	color = BLACK;
	alpha = 0xff;
	fade = 1;
	return this;
}

TitleRow *TitleRow::init()
{
	x0 = y0 = 0;
	x1 = y2 = 0; //MAX_FLT;
	y1 = x2 = 0; //MIN_FLT;
	return this;
}

int TitleMain::get_text()
{
// Determine extents of total text
	title_chars.reset();
	title_rows.reset();
	int pitch = config.line_pitch;
	float font_h = config.size;
	int descent = 0;

	TitleParser wchrs(this);

	TitleRow *row = 0;
	float row_w = 0, row_h = 0;
	float row_x = 0, row_y = 0;
	text_rows = 0;

	for(;;) {
		if( !row ) row = title_rows.add();
		TitleChar *chr = 0;
		long ipos = wchrs.tell();
		wchar_t wch1 = wchrs.wcur(), wch;
		int ret = wchrs.tget(wch);
		if( ret < 0 || wch == '\n' ) {
			if( row->x1 > row->x2 ) row->x1 = row->x2 = 0;
			if( row->y2 > row->y1 ) row->y1 = row->y2 = 0;
			int dy = row->y1 - descent;
			row_y += pitch ? pitch : dy > font_h ? dy : font_h;
			row->y0 = row_y;
			descent = row->y2;
			if( row_x > row_w ) row_w = row_x;
			if( row_y > row_h ) row_h = row_y;
			text_rows = title_rows.count();
			row_x = 0;  row = 0;
			if( ret < 0 ) break;
			continue;
		}
		BC_FontEntry *cur_font = wchrs.cur_font;
		int cur_color   = wchrs.cur_color;
		int cur_alpha   = wchrs.cur_alpha;
		float cur_size  = wchrs.cur_size;
		int cur_caps    = wchrs.cur_caps;
		int cur_under   = wchrs.cur_under;
		float cur_blink = wchrs.cur_blink;
		int cur_fixed   = wchrs.cur_fixed;
		int cur_super   = wchrs.cur_super;
		int cur_nudge   = wchrs.cur_nudge;
		int cur_style = 0;
		int cur_bold  = wchrs.cur_bold;
		if( cur_bold ) cur_style |= BC_FONT_BOLD;
		int cur_alias   = wchrs.cur_alias;
		if( cur_alias ) cur_style |= FONT_ALIAS;
		int cur_italic  = wchrs.cur_italic;
		if( cur_italic ) cur_style |= BC_FONT_ITALIC;
		short nx = cur_nudge >> 16, ny = cur_nudge;
		int cx = nx, cy = ny, cw = 0, ch = 0, dx = 0;
		if( ret > 0 ) {
			if( !wchrs.set_attributes(ret) ) continue;
			ret = -1;
			if( !kw_strcmp(wchrs.id,KW_PNG) ) {
				VFrame *png_image = get_image(wchrs.text);
				if( png_image ) {
					chr = title_chars.add(CHAR_IMAGE, png_image);
					cy += ch = png_image->get_h();
					cw = dx = png_image->get_w();
					wch = 0;  ret = 0;
				}
			}
			if( ret < 0 ) {
				wch = wch1;  wchrs.seek(ipos+1);
			}
		}
		if( wch ) {
			if( cur_caps > 0 ) wch = towupper(wch);
			else if( cur_caps < 0 ) wch = towlower(wch);
			int size = !cur_super ? cur_size : cur_size/2;
			TitleGlyph *gp = get_glyph(wch, cur_font, size, cur_style);
			if( !gp ) continue;

			if( cur_super > 0 ) cy += cur_size-4*size/3;
			else if( cur_super < 0 ) cy -= size/3;

			dx =  gp->advance_x;

			if( cur_fixed )
				dx = cur_fixed;
			else if( !wchrs.eof() ) {
				TitleGlyph *np = get_glyph(wchrs.wcur(), cur_font, cur_size, cur_style);
				dx = get_width(gp, np);
			}

			chr = title_chars.add(CHAR_GLYPH, gp);
			cx += gp->left;
			cy += gp->top;
			cw = gp->right - gp->left;
			ch = gp->top - gp->bottom;
		}

		if( !chr ) continue;
		chr->wch = wch;
		chr->size  = cur_size;
		chr->blink = cur_blink;
		chr->color = cur_color;
		chr->alpha = cur_alpha;
		chr->fade = 1;
		if( cur_fixed ) chr->flags |= FLAG_FIXED;
		if( cur_super > 0 ) chr->flags |= FLAG_SUPER;
		if( cur_super < 0 ) chr->flags |= FLAG_SUBER;
		if( cur_under ) chr->flags |= FLAG_UNDER;
		if( cur_blink ) chr->flags |= FLAG_BLINK;
		chr->x = (cx += row_x);
		chr->y = cy;
		chr->dx = dx;
		row->bound(cx,cy, cx+cw,cy-ch);
		chr->row = text_rows;
		row_x += chr->dx;
	}

	if( !row_w || !row_h ) return 1;

// rows boundary, note: row->xy is y up (FT), row_xy is y down (X)
	text_x1 = text_y1 = MAX_FLT;
	text_x2 = text_y2 = MIN_FLT;
	for( int i=0; i<text_rows; ++i ) {
		row = title_rows[i];
		switch( config.hjustification ) {
		case JUSTIFY_LEFT:   row->x0 = 0;  break;
		case JUSTIFY_CENTER: row->x0 = (row_w - (row->x2-row->x1)) / 2;  break;
		case JUSTIFY_RIGHT:  row->x0 = row_w - (row->x2-row->x1);  break;
		}
		float v;
		if( text_x1 > (v=row->x0+row->x1) ) text_x1 = v;
		if( text_y1 > (v=row->y0-row->y1) ) text_y1 = v;
		if( text_x2 < (v=row->x0+row->x2) ) text_x2 = v;
		if( text_y2 < (v=row->y0-row->y2) ) text_y2 = v;
	}
	if( text_x1 > text_x2 || text_y1 > text_y2 ) return 1;
	text_x1 += fuzz1;  text_y1 += fuzz1;
	text_x2 += fuzz2;  text_y2 += fuzz2;
	text_w = text_x2 - text_x1;
	text_h = text_y2 - text_y1;
	return 0;
}

int TitleMain::get_visible_text()
{
// Determine y of visible text
	switch( config.motion_strategy ) {
	case BOTTOM_TO_TOP:
	case TOP_TO_BOTTOM: {
		float magnitude = config.pixels_per_second *
			(get_source_position() - config.prev_keyframe_position) /
			PluginVClient::project_frame_rate;
		if( config.loop ) {
			int loop_size = text_h + title_h;
			magnitude -= (int)(magnitude / loop_size) * loop_size;
		}
		text_y = config.motion_strategy == BOTTOM_TO_TOP ?
			title_h - magnitude :
			magnitude - text_h;
		break; }
	default: switch( config.vjustification ) {
	case JUSTIFY_TOP:    text_y =  0;  break;
	case JUSTIFY_MID:    text_y = (title_h - text_h) / 2;  break;
	case JUSTIFY_BOTTOM: text_y =  title_h - text_h;  break;
	default: break; }
	}

// Determine x of visible text
	switch( config.motion_strategy ) {
	case RIGHT_TO_LEFT:
	case LEFT_TO_RIGHT: {
		float magnitude = config.pixels_per_second *
			(get_source_position() - get_source_start()) /
			PluginVClient::project_frame_rate;
		if( config.loop ) {
			int loop_size = text_w + title_w;
			magnitude -= (int)(magnitude / loop_size) * loop_size;
		}
		text_x = config.motion_strategy == RIGHT_TO_LEFT ?
			title_w - magnitude :
			magnitude - text_w;
		break; }
	default: switch ( config.hjustification ) {
	case JUSTIFY_LEFT:   text_x =  0;  break;
	case JUSTIFY_CENTER: text_x = (title_w - text_w) / 2;  break;
	case JUSTIFY_RIGHT:  text_x =  title_w - text_w;  break;
	default: break; }
	}


	// until bottom of this row is visible
	int row1 = 0;
	int y0 = text_y - text_y1;
	while( row1 < text_rows ) {
		TitleRow *row = title_rows[row1];
		if( y0 + row->y0-row->y2 > 0 ) break;
		++row1;
	}
	if( row1 >= text_rows ) return 0;

	// until top of next row is not visible
	y0 -= title_h;
	int row2 = row1;
	while( row2 < text_rows ) {
		TitleRow *row = title_rows[row2];
		if( y0 + row->y0-row->y1 >= 0 ) break;
		++row2;
	}
	if( row1 >= row2 ) return 0;

//printf("get_visible_rows: visible_row1/2 = %d to %d\n",visible_row1,visible_row2);
	// Only draw visible chars
	double frame_rate = PluginVClient::get_project_framerate();
	int64_t units = get_source_position() - get_source_start();
	double position = units / frame_rate;
	int blinking = 0;
	int char1 = -1, char2 = -1, nchars = title_chars.count();
	for( int i=0; i<nchars; ++i ) {
		TitleChar *chr = title_chars[i];
		if( chr->row < row1 ) continue;
		if( chr->row >= row2 ) break;
		if( char1 < 0 ) char1 = i;
		char2 = i;
		if( (chr->flags & FLAG_BLINK) != 0 ) {
			blinking = 1;
			double rate = 1 / fabs(chr->blink), rate2 = 2 * rate;
			double pos = position - ((int64_t)(position / rate2)) * rate2;
			chr->fade = chr->blink > 0 ?
				(pos > rate ? 0 : 1) : fabs(rate+pos) / rate;
		}
	}
	if( char1 < 0 ) char1 = 0;
	++char2;
	if( !blinking && visible_row1 == row1 && visible_row2 == row2 ) return 0;
// need redraw
	visible_row1  = row1;   visible_row2  = row2;
	visible_char1 = char1;  visible_char2 = char2;
	return 1;
}


int TitleMain::draw_text(int need_redraw)
{
	// until top of next row is not visible
	mask_x1 = mask_y1 = INT_MAX;
	mask_x2 = mask_y2 = INT_MIN;
	for( int i=visible_row1; i<visible_row2; ++i ) {
		int v;  TitleRow *row = title_rows[i];
		if( mask_x1 > (v=row->x0+row->x1) ) mask_x1 = v;
		if( mask_y1 > (v=row->y0-row->y1) ) mask_y1 = v;
		if( mask_x2 < (v=row->x0+row->x2) ) mask_x2 = v;
		if( mask_y2 < (v=row->y0-row->y2) ) mask_y2 = v;
	}
	if( mask_x1 >= mask_x2 || mask_y1 >= mask_y2 ) return 1;

	mask_x1 += fuzz1;  mask_y1 += fuzz1;
	mask_x2 += fuzz2;  mask_y2 += fuzz2;
	mask_w = mask_x2 - mask_x1;
	mask_h = mask_y2 - mask_y1;

//printf("TitleMain::draw_text %d-%d frame %dx%d\n",
//  visible_row1, visible_row2, mask_w,mask_h);
	if( text_mask && (text_mask->get_color_model() != text_model ||
	    text_mask->get_w() != mask_w || text_mask->get_h() != mask_h) ) {
		delete text_mask;    text_mask = 0;
		delete stroke_mask;  stroke_mask = 0;
	}

	if( !text_mask ) {
// Always use 8 bit because the glyphs are 8 bit
// Need to set YUV to get clear_frame to set the right chroma.
		mask_model = text_model;
		text_mask = new VFrame;
		text_mask->set_use_shm(0);
		text_mask->reallocate(0, -1, 0, 0, 0, mask_w, mask_h, mask_model, -1);
		int drop = abs(config.dropshadow);
		int drop_w = mask_w + drop;
		int drop_h = mask_h + drop;
                stroke_mask = new VFrame;
		stroke_mask->set_use_shm(0);
		stroke_mask->reallocate(0, -1, 0, 0, 0, drop_w, drop_h, mask_model, -1);
		need_redraw = 1;
	}

// Draw on text mask if it has changed
	if( need_redraw ) {
//printf("redraw %d to %d   %d,%d  %d,%d - %d,%d\n", visible_char1,visible_char2,
//  ext_x0, ext_y0, ext_x1,ext_y1, ext_x2,ext_y2);

		text_mask->clear_frame();
		stroke_mask->clear_frame();
#if 0
		unsigned char *data = text_mask->get_data(); // draw bbox on text
		for( int x=0; x<mask_w; ++x ) data[4*x+3] = 0xff;
		for( int x=0; x<mask_w; ++x ) data[4*((mask_h-1)*mask_w+x)+3] = 0xff;
		for( int y=0; y<mask_h; ++y ) data[4*mask_w*y+3] = 0xff;
		for( int y=0; y<mask_h; ++y ) data[4*(mask_w*y + mask_w-1)+3] = 0xff;
#endif
		if( !title_engine )
			title_engine = new TitleEngine(this, cpus);

// Draw dropshadow first
		if( config.dropshadow ) {
			title_engine->do_dropshadow = 1;
			title_engine->set_package_count(visible_char2 - visible_char1);
			title_engine->process_packages();
		}

// Then draw foreground
		title_engine->do_dropshadow = 0;
		title_engine->set_package_count(visible_char2 - visible_char1);
		title_engine->process_packages();

		draw_underline(text_mask, config.alpha);

// Convert to text outlines
		if( config.outline_size > 0 ) {
			if( outline_mask &&
			    (text_mask->get_w() != outline_mask->get_w() ||
			     text_mask->get_h() != outline_mask->get_h()) ) {
				delete outline_mask;  outline_mask = 0;
			}

			if( !outline_mask ) {
				outline_mask = new VFrame;
				outline_mask->set_use_shm(0);
				outline_mask->reallocate(0, -1, 0, 0, 0,
					text_mask->get_w(), text_mask->get_h(),
					text_mask->get_color_model(), -1);
			}
			if( !outline_engine ) outline_engine =
				new TitleOutlineEngine(this, cpus);
			outline_engine->do_outline();
		}

	}

	return 0;
}

int TitleMain::draw_underline(VFrame *mask, int alpha)
{
	int row = -1, sz = -1, color = -1, rgb = -1;
	int bpp = mask->get_bytes_per_pixel(), bpp1 = bpp-1;
	int x1 = 0, x2 = 0, y0 = 0;
	for( int i=visible_char1; i<visible_char2; ) {
		TitleChar *chr = title_chars[i];
		if( (chr->flags & FLAG_UNDER) && row < 0 ) {
			sz = chr->size;
			rgb = chr->color;  int rr, gg, bb;
			get_mask_colors(rgb, mask_model, rr, gg, bb);
			color = (rr<<16) | (gg<<8) | (bb<<0);
			row = chr->row;
			TitleRow *rp = title_rows[row];
			x1 = rp->x0 + chr->x;
			y0 = rp->y0;
		}
		if( (chr->flags & FLAG_UNDER) && row == chr->row && chr->color == rgb ) {
			TitleRow *rp = title_rows[row];
			x2 = rp->x0 + chr->x + chr->dx;
			if( sz < chr->size ) sz = chr->size;
			if( ++i < visible_char2 ) continue;
		}
		if( row < 0 ) { ++i;  continue; }
		x1 -= mask_x1;  x2 -= mask_x1;  y0 -= mask_y1;
		if( x1 < 0 ) x1 = 0;
		if( x2 > mask_w ) x2 = mask_w;
		int sz1 = sz / 32 + 1, sz2 = sz1/2;
		int y1 = y0 - sz2, y2 = y1 + sz1;
		if( y1 < 0 ) y1 = 0;
		if( y2 > mask_h ) y2 = mask_h;
		unsigned char **rows = mask->get_rows();
		for( int y=y1; y<y2; ++y ) {
			unsigned char *rp = rows[y];
			for( int x=x1; x<x2; ++x ) {
				unsigned char *bp = rp + bpp * x;
				for( int i=bpp1; --i>=0; ) *bp++ = color>>(8*i);
				*bp = alpha;
			}
		}
		row = -1;
	}
	return 0;
}

void TitleMain::draw_overlay()
{
//printf("TitleMain::draw_overlay 1\n");
        fade = 1;
        if( !EQUIV(config.fade_in, 0) ) {
		int64_t plugin_start = server->plugin->startproject;
		int64_t fade_len = lroundf(config.fade_in * PluginVClient::project_frame_rate);
		int64_t fade_position = get_source_position() - plugin_start;

		if( fade_position >= 0 && fade_position < fade_len ) {
			fade = (float)fade_position / fade_len;
		}
	}
        if( !EQUIV(config.fade_out, 0) ) {
		int64_t plugin_end = server->plugin->startproject + server->plugin->length;
		int64_t fade_len = lroundf(config.fade_out * PluginVClient::project_frame_rate);
		int64_t fade_position = plugin_end - get_source_position();

		if( fade_position >= 0 && fade_position < fade_len ) {
			fade = (float)fade_position / fade_len;
		}
	}

	if( !translate )
		translate = new TitleTranslate(this, cpus);

	int tx = text_x - text_x1 + mask_x1;
	if( tx < title_w && tx+mask_w > 0 ) {
		translate->copy(text_mask);
		if( config.stroke_width >= SMALL && (config.style & BC_FONT_OUTLINE) ) {
			translate->copy(stroke_mask);
		}
	}
}


TitleTranslate::TitleTranslate(TitleMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

TitleTranslate::~TitleTranslate()
{
}

void TitleTranslate::copy(VFrame *input)
{
	this->input = input;
	in_w = input->get_w();
	in_h = input->get_h();
	ix1 = 0, ix2 = ix1 + in_w;
	iy1 = 0, iy2 = iy1 + in_h;

	out_w = plugin->output->get_w();
	out_h = plugin->output->get_h();
	float x1 = plugin->title_x, x2 = x1 + plugin->title_w;
	float y1 = plugin->title_y, y2 = y1 + plugin->title_h;
	bclamp(x1, 0, out_w);  bclamp(y1, 0, out_h);
	bclamp(x2, 0, out_w);  bclamp(y2, 0, out_h);

	ox1 = plugin->title_x + plugin->text_x - plugin->text_x1 + plugin->mask_x1;
	ox2 = ox1 + in_w;
	oy1 = plugin->title_y + plugin->text_y - plugin->text_y1 + plugin->mask_y1;
	oy2 = oy1 + in_h;
	if( ox1 < x1 ) { ix1 -= (ox1-x1);  ox1 = x1; }
	if( oy1 < y1 ) { iy1 -= (oy1-y1);  oy1 = y1; }
	if( ox2 > x2 ) { ix2 -= (ox2-x2);  ox2 = x2; }
	if( oy2 > y2 ) { iy2 -= (oy2-x2);  oy2 = y2; }
#if 0
printf("TitleTranslate text  txy=%7.2f,%-7.2f\n"
  "  mxy1=%7d,%-7d mxy2=%7d,%-7d\n"
  "   xy1=%7.2f,%-7.2f  xy2=%7.2f,%-7.2f\n"
  "  ixy1=%7.2f,%-7.2f ixy2=%7.2f,%-7.2f\n"
  "  oxy1=%7.2f,%-7.2f oxy2=%7.2f,%-7.2f\n",
   plugin->text_x, plugin->text_y,
   plugin->mask_x1, plugin->mask_y1, plugin->mask_x2, plugin->mask_y2,
   x1,y1, x2,y2, ix1,iy1, ix2,iy2, ox1,oy1, ox2,oy2);
#endif
	process_packages();
}


TitleTranslatePackage::TitleTranslatePackage()
 : LoadPackage()
{
	y1 = y2 = 0;
}

TitleTranslateUnit::TitleTranslateUnit(TitleMain *plugin, TitleTranslate *server)
 : LoadClient(server)
{
}

#define TRANSLATE(type, max, comps, ofs) { \
	type **out_rows = (type**)output->get_rows(); \
	float fr = 1./(256.-max), fs = max/255.; \
	float r = max > 1 ? 0.5 : 0; \
	int ix1= x1, iy1 = y1, ix2= x2, iy2 = y2; \
	float fy = y1 + yofs; \
	for( int y=iy1; y<iy2; ++y,++fy ) { \
		int iy = fy;  float yf1 = fy - iy; \
		if( yf1 < 0 ) ++yf1; \
		float yf0 = 1. - yf1; \
		unsigned char *in_row0 = in_rows[iy<0 ? 0 : iy]; \
		unsigned char *in_row1 = in_rows[iy<ih1 ? iy+1 : ih1]; \
		float fx = x1 + xofs; \
		for( int x=ix1; x<ix2; ++x,++fx ) { \
			type *op = out_rows[y] + x*comps; \
			int ix = fx;  float xf1 = fx - ix; \
			if( xf1 < 0 ) ++xf1; \
			float xf0 = 1. - xf1; \
			int i0 = (ix<0 ? 0 : ix)*4, i1 = (ix<iw1 ? ix+1 : iw1)*4; \
			uint8_t *cp00 = in_row0 + i0, *cp01 = in_row0 + i1; \
			uint8_t *cp10 = in_row1 + i0, *cp11 = in_row1 + i1; \
			float a00 = yf0 * xf0 * cp00[3], a01 = yf0 * xf1 * cp01[3]; \
			float a10 = yf1 * xf0 * cp10[3], a11 = yf1 * xf1 * cp11[3]; \
			float fa = a00 + a01 + a10 + a11;  if( !fa ) continue;  \
			type in_a = fa*fr + r;  float s = fs/fa; \
			type in_r = (cp00[0]*a00 + cp01[0]*a01 + cp10[0]*a10 + cp11[0]*a11)*s + r; \
			type in_g = (cp00[1]*a00 + cp01[1]*a01 + cp10[1]*a10 + cp11[1]*a11)*s + r; \
			type in_b = (cp00[2]*a00 + cp01[2]*a01 + cp10[2]*a10 + cp11[2]*a11)*s + r; \
			type a = in_a*plugin->fade, b = max - a, px; \
			/*if( comps == 4 ) { b = (b * op[3]) / max; }*/ \
			px = *op;  *op++ = (a*in_r + b*px) / max; \
			px = *op;  *op++ = (a*(in_g-ofs) + b*(px-ofs)) / max + ofs; \
			px = *op;  *op++ = (a*(in_b-ofs) + b*(px-ofs)) / max + ofs; \
			if( comps == 4 ) { b = *op;  *op++ = a + b - a*b / max; } \
		} \
	} \
}

void TitleTranslateUnit::process_package(LoadPackage *package)
{
	TitleTranslatePackage *pkg = (TitleTranslatePackage*)package;
	TitleTranslate *server = (TitleTranslate*)this->server;
	TitleMain *plugin = server->plugin;
	VFrame *input = server->input, *output = plugin->output;
	int iw = input->get_w(),  ih = input->get_h();
	int iw1 = iw-1, ih1 = ih-1;
	float x1 = server->ox1, x2 = server->ox2;
	float y1 = pkg->y1, y2 = pkg->y2;
	float xofs = server->ix1 - server->ox1;
	float yofs = server->iy1 - server->oy1;
	unsigned char **in_rows = input->get_rows();

	switch( output->get_color_model() ) {
	case BC_RGB888:     TRANSLATE(unsigned char, 0xff, 3, 0);    break;
	case BC_RGB_FLOAT:  TRANSLATE(float, 1.0, 3, 0);             break;
	case BC_YUV888:     TRANSLATE(unsigned char, 0xff, 3, 0x80); break;
	case BC_RGBA_FLOAT: TRANSLATE(float, 1.0, 4, 0);             break;
	case BC_RGBA8888:   TRANSLATE(unsigned char, 0xff, 4, 0);    break;
	case BC_YUVA8888:   TRANSLATE(unsigned char, 0xff, 4, 0x80); break;
	}
}

void TitleTranslate::init_packages()
{
	int oh = oy2 - oy1;
	int py = oy1;
	int i = 0, pkgs = get_total_packages();
	while( i < pkgs ) {
		TitleTranslatePackage *pkg = (TitleTranslatePackage*)get_package(i++);
		pkg->y1 = py;
		py = oy1 + i*oh / pkgs;
		pkg->y2 = py;
	}
}

LoadClient* TitleTranslate::new_client()
{
	return new TitleTranslateUnit(plugin, this);
}

LoadPackage* TitleTranslate::new_package()
{
	return new TitleTranslatePackage;
}



const char* TitleMain::motion_to_text(int motion)
{
	switch( motion ) {
	case NO_MOTION:     return _("No motion");
	case BOTTOM_TO_TOP: return _("Bottom to top");
	case TOP_TO_BOTTOM: return _("Top to bottom");
	case RIGHT_TO_LEFT: return _("Right to left");
	case LEFT_TO_RIGHT: return _("Left to right");
	}
	return _("Unknown");
}

int TitleMain::text_to_motion(const char *text)
{
	for( int i=0; i<TOTAL_PATHS; ++i ) {
		if( !strcasecmp(motion_to_text(i), text) ) return i;
	}
	return 0;
}

void TitleMain::reset_render()
{
	delete text_mask;     text_mask = 0;
	delete stroke_mask;   stroke_mask = 0;
	delete glyph_engine;  glyph_engine = 0;
	visible_row1 = 0;     visible_row2 = 0;
	visible_char1 = 0;    visible_char2 = 0;
	title_rows.clear();
	title_glyphs.clear();
	title_images.clear();
	if( freetype_face ) {
		ft_Done_Face(freetype_face);
		freetype_face = 0;
	}
}

int TitleMain::init_freetype()
{
	if( !freetype_library )
		ft_Init_FreeType(&freetype_library);
	return 0;
}

void TitleMain::draw_boundry()
{
	if( !gui_open() ) return;
	DragCheckBox::draw_boundary(output,
		title_x, title_y, title_w, title_h);
}


int TitleMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	int result = 0;
	input = input_ptr;
	output = output_ptr;
	output_model = output->get_color_model();
	text_model = BC_CModels::is_yuv(output_model) ? BC_YUVA8888 : BC_RGBA8888;

	if( text_model != mask_model ) need_reconfigure = 1;
	need_reconfigure |= load_configuration();
	int64_t cur_position =  get_source_position();
	if( last_position < 0 || last_position+1 != cur_position )
		need_reconfigure = 1;
	last_position = cur_position;

	title_x = config.title_x;  title_y = config.title_y;
	title_w = config.title_w ? config.title_w : input->get_w();
	title_h = config.title_h ? config.title_h : input->get_h();

	fuzz1 = -config.outline_size;
	fuzz2 = config.outline_size;
	if( config.dropshadow < 0 )
		fuzz1 += config.dropshadow;
	else
		fuzz2 += config.dropshadow;
	fuzz = fuzz2 - fuzz1;

// Check boundaries
	if( config.size <= 0 || config.size >= 2048 )
		config.size = 72;
	if( config.stroke_width < 0 || config.stroke_width >= 512 )
		config.stroke_width = 0.0;
	if( !config.wlen && !config.timecode )
		return 0;
	if( !strlen(config.encoding) )
		strcpy(config.encoding, DEFAULT_ENCODING);

// Always synthesize text and redraw it for timecode
	if( config.timecode ) {
		int64_t rendered_frame = get_source_position();
		if( get_direction() == PLAY_REVERSE )
			rendered_frame -= 1;

		char text[BCTEXTLEN];
		Units::totext(text,
				(double)rendered_frame / PluginVClient::project_frame_rate,
				config.timecode_format,
				PluginVClient::get_project_samplerate(),
				PluginVClient::get_project_framerate(),
				16);
		config.to_wtext(config.encoding, text, strlen(text)+1);
		need_reconfigure = 1;
	}

	if( config.background )
		draw_background();

// Handle reconfiguration
	if( need_reconfigure ) {
		need_reconfigure = 0;
		reset_render();
		result = init_freetype();
		if( !result ) {
			load_glyphs();
			result = get_text();
		}
	}

	if( !result )
		result = draw_text(get_visible_text());


// Overlay mask on output
	if( !result )
		draw_overlay();

	if( config.drag )
		draw_boundry();

	return 0;
}

void TitleMain::update_gui()
{
	if( thread ) {
		int reconfigure = load_configuration();
		if( reconfigure ) {
			TitleWindow *window = (TitleWindow*)thread->window;
			window->lock_window("TitleMain::update_gui");
			window->update();
			window->unlock_window();
		}
	}
}

int TitleMain::load_configuration()
{
	KeyFrame *prev_keyframe, *next_keyframe;
	prev_keyframe = get_prev_keyframe(get_source_position());
	next_keyframe = get_next_keyframe(get_source_position());

	TitleConfig old_config, prev_config, next_config;
	old_config.copy_from(config);
	read_data(prev_keyframe);
	prev_config.copy_from(config);
	read_data(next_keyframe);
	next_config.copy_from(config);

	config.prev_keyframe_position = prev_keyframe->position;
	config.next_keyframe_position = next_keyframe->position;

	// if no previous keyframe exists, it should be start of the plugin, not start of the track
	if( config.next_keyframe_position == config.prev_keyframe_position )
		config.next_keyframe_position = get_source_start() + get_total_len();
	if( config.prev_keyframe_position == 0 )
		config.prev_keyframe_position = get_source_start();
// printf("TitleMain::load_configuration 10 %d %d\n",
// config.prev_keyframe_position,
// config.next_keyframe_position);

	config.interpolate(prev_config,
		next_config,
		(next_keyframe->position == prev_keyframe->position) ?
			get_source_position() :
			prev_keyframe->position,
		(next_keyframe->position == prev_keyframe->position) ?
			get_source_position() + 1 :
			next_keyframe->position,
		get_source_position());

	if( !config.equivalent(old_config) )
		return 1;
	return 0;
}


void TitleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("TITLE");
	output.tag.set_property("FONT", config.font);
	output.tag.set_property("ENCODING", config.encoding);
	output.tag.set_property("STYLE", (int64_t)config.style);
	output.tag.set_property("SIZE", config.size);
	output.tag.set_property("COLOR", config.color);
        output.tag.set_property("ALPHA", config.alpha);
	output.tag.set_property("OUTLINE_SIZE", config.outline_size);
        output.tag.set_property("OUTLINE_COLOR", config.outline_color);
        output.tag.set_property("OUTLINE_ALPHA", config.outline_alpha);
	output.tag.set_property("COLOR_STROKE", config.color_stroke);
	output.tag.set_property("STROKE_WIDTH", config.stroke_width);
	output.tag.set_property("MOTION_STRATEGY", config.motion_strategy);
	output.tag.set_property("LOOP", config.loop);
	output.tag.set_property("LINE_PITCH", config.line_pitch);
	output.tag.set_property("PIXELS_PER_SECOND", config.pixels_per_second);
	output.tag.set_property("HJUSTIFICATION", config.hjustification);
	output.tag.set_property("VJUSTIFICATION", config.vjustification);
	output.tag.set_property("FADE_IN", config.fade_in);
	output.tag.set_property("FADE_OUT", config.fade_out);
	output.tag.set_property("TITLE_X", config.title_x);
	output.tag.set_property("TITLE_Y", config.title_y);
	output.tag.set_property("TITLE_W", config.title_w);
	output.tag.set_property("TITLE_H", config.title_h);
	output.tag.set_property("DROPSHADOW", config.dropshadow);
	output.tag.set_property("TIMECODE", config.timecode);
	output.tag.set_property("TIMECODEFORMAT", config.timecode_format);
	output.tag.set_property("WINDOW_W", config.window_w);
	output.tag.set_property("WINDOW_H", config.window_h);
	output.tag.set_property("DRAG", config.drag);
	output.tag.set_property("BACKGROUND", config.background);
	output.tag.set_property("BACKGROUND_PATH", config.background_path);
	output.tag.set_property("LOOP_PLAYBACK", config.loop_playback);
	output.append_tag();
	output.append_newline();
	long tsz = 2*config.wsize + 0x1000;
	char text[tsz];
	int text_len = BC_Resources::encode(
		BC_Resources::wide_encoding, DEFAULT_ENCODING,
		(char*)config.wtext, config.wlen*sizeof(wchar_t),
		text, tsz);
	output.append_text(text, text_len);
	output.tag.set_title("/TITLE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
//printf("TitleMain::save_data 1\n%s\n", output.string);
//printf("TitleMain::save_data 2\n%s\n", config.text);
}

void TitleMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	config.prev_keyframe_position = keyframe->position;
	while(!result)
	{
		result = input.read_tag();
		if( result ) break;

		if( input.tag.title_is("TITLE") ) {
			input.tag.get_property("FONT", config.font);
			input.tag.get_property("ENCODING", config.encoding);
			config.style = input.tag.get_property("STYLE", (int64_t)config.style);
			config.size = input.tag.get_property("SIZE", config.size);
			config.color = input.tag.get_property("COLOR", config.color);
			config.alpha = input.tag.get_property("ALPHA", config.alpha);
			config.outline_size = input.tag.get_property("OUTLINE_SIZE", config.outline_size);
			config.outline_color = input.tag.get_property("OUTLINE_COLOR", config.outline_color);
			config.outline_alpha = input.tag.get_property("OUTLINE_ALPHA", config.outline_alpha);
			config.color_stroke = input.tag.get_property("COLOR_STROKE", config.color_stroke);
			config.stroke_width = input.tag.get_property("STROKE_WIDTH", config.stroke_width);
			config.motion_strategy = input.tag.get_property("MOTION_STRATEGY", config.motion_strategy);
			config.loop = input.tag.get_property("LOOP", config.loop);
			config.line_pitch = input.tag.get_property("LINE_PITCH", config.line_pitch);
			config.pixels_per_second = input.tag.get_property("PIXELS_PER_SECOND", config.pixels_per_second);
			config.hjustification = input.tag.get_property("HJUSTIFICATION", config.hjustification);
			config.vjustification = input.tag.get_property("VJUSTIFICATION", config.vjustification);
			config.fade_in = input.tag.get_property("FADE_IN", config.fade_in);
			config.fade_out = input.tag.get_property("FADE_OUT", config.fade_out);
			config.title_x = input.tag.get_property("TITLE_X", config.title_x);
			config.title_y = input.tag.get_property("TITLE_Y", config.title_y);
			config.title_w = input.tag.get_property("TITLE_W", config.title_w);
			config.title_h = input.tag.get_property("TITLE_H", config.title_h);
			config.dropshadow = input.tag.get_property("DROPSHADOW", config.dropshadow);
			config.timecode = input.tag.get_property("TIMECODE", config.timecode);
			config.timecode_format = input.tag.get_property("TIMECODEFORMAT", config.timecode_format);
			config.window_w = input.tag.get_property("WINDOW_W", config.window_w);
			config.window_h = input.tag.get_property("WINDOW_H", config.window_h);
			config.drag = input.tag.get_property("DRAG", config.drag);
			config.background = input.tag.get_property("BACKGROUND", config.background);
			input.tag.get_property("BACKGROUND_PATH", config.background_path);
			config.loop_playback = input.tag.get_property("LOOP_PLAYBACK", config.loop_playback);
			const char *text = input.read_text();
			config.to_wtext(config.encoding, text, strlen(text)+1);
		}
		else if( input.tag.title_is("/TITLE") ) {
			result = 1;
		}
	}
}

void TitleMain::insert_text(const wchar_t *wtxt, int pos)
{
	int len = wcslen(wtxt);
	int wlen = config.wlen;
	if( pos < 0 ) pos = 0;
	if( pos > wlen ) pos = wlen;
	config.demand(wlen + len);
	int wsize1 = config.wsize-1;
	wchar_t *wtext = config.wtext;
	for( int i=wlen, j=wlen+len; --i>=pos; ) {
		if( --j >= wsize1 ) continue;
		wtext[j] = wtext[i];
	}
	for( int i=pos, j=0; j<len; ++i,++j ) {
		if( i >= wsize1 ) break;
		wtext[i] = wtxt[j];
	}

	if( (wlen+=len) > wsize1 ) wlen = wsize1;
	wtext[wlen] = 0;
	config.wlen = wlen;
}

