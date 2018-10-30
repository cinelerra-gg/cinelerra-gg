
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

#ifndef TITLE_H
#define TITLE_H
#define USE_STROKER

#define KW_NUDGE  N_("nudge")
#define KW_COLOR  N_("color")
#define KW_ALPHA  N_("alpha")
#define KW_FONT   N_("font")
#define KW_SIZE   N_("size")
#define KW_BOLD   N_("bold")
#define KW_ITALIC N_("italic")
#define KW_CAPS   N_("caps")
#define KW_UL     N_("ul")
#define KW_BLINK  N_("blink")
#define KW_FIXED  N_("fixed")
#define KW_ALIAS  N_("smooth")
#define KW_SUP    N_("sup")
#define KW_PNG    N_("png")

class TitleConfig;
class TitleGlyph;
class TitleGlyphs;
class TitleImage;
class TitleImages;
class TitleChar;
class TitleChars;
class TitleRow;
class TitleRows;
class GlyphPackage;
class GlyphUnit;
class GlyphEngine;
class TitlePackage;
class TitleUnit;
class TitleEngine;
class TitleOutlinePackage;
class TitleOutlineUnit;
class TitleOutlineEngine;
class TitleTranslatePackage;
class TitleTranslateUnit;
class TitleTranslate;
class TitleCurFont;
class TitleCurSize;
class TitleCurColor;
class TitleCurAlpha;
class TitleCurBold;
class TitleCurItalic;
class TitleCurCaps;
class TitleCurUnder;
class TitleCurBlink;
class TitleCurFixed;
class TitleCurAlias;
class TitleCurSuper;
class TitleCurNudge;
class TitleParser;
class TitleMain;

#include "bchash.h"
#include "bcfontentry.h"
#include "file.inc"
#include "indexable.inc"
#include "loadbalance.h"
#include "mutex.h"
#include "overlayframe.h"
#include "pluginvclient.h"
#include "renderengine.inc"
#include "titlerwindow.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <sys/types.h>
#include <wchar.h>
#include <wctype.h>

// Motion strategy
#define TOTAL_PATHS 5
#define NO_MOTION     0x0
#define BOTTOM_TO_TOP 0x1
#define TOP_TO_BOTTOM 0x2
#define RIGHT_TO_LEFT 0x3
#define LEFT_TO_RIGHT 0x4

// Horizontal justification
#define JUSTIFY_LEFT   0x0
#define JUSTIFY_CENTER 0x1
#define JUSTIFY_RIGHT  0x2

// Vertical justification
#define JUSTIFY_TOP     0x0
#define JUSTIFY_MID     0x1
#define JUSTIFY_BOTTOM  0x2

// char types
#define CHAR_GLYPH  0
#define CHAR_IMAGE  1

// flags
#define FLAG_UNDER  0x0001
#define FLAG_FIXED  0x0002
#define FLAG_SUPER  0x0004
#define FLAG_SUBER  0x0008
#define FLAG_BLINK  0x0010

#define FONT_ALIAS 0x08

class TitleConfig
{
public:
	TitleConfig();
	~TitleConfig();

	void to_wtext(const char *from_enc, const char *text, int tlen);
// Only used to clear glyphs
	int equivalent(TitleConfig &that);
	void copy_from(TitleConfig &that);
	void interpolate(TitleConfig &prev,
		TitleConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);

// Font information
	char font[BCTEXTLEN];
// Encoding to convert from
	char encoding[BCTEXTLEN];
	int style;
	float size;
	int color;
	int alpha;
	float outline_size;
	int outline_color;
	int outline_alpha;
	int color_stroke;
	float stroke_width;
	int motion_strategy;     // Motion of title across frame
	int line_pitch;
	int loop;                // Loop motion path
	int hjustification;
	int vjustification;
// Number of seconds the fade in and fade out of the title take
	double fade_in, fade_out;
	float pixels_per_second; // Speed of motion
// Text to display
	wchar_t *wtext;
	long wsize, wlen;
// Position in frame relative to top left
	float title_x, title_y;
	int title_w, title_h;
// Size of window
	int window_w, window_h;
// Calculated during every frame for motion strategy
	int64_t prev_keyframe_position;
	int64_t next_keyframe_position;
// Stamp timecode
	int timecode;
	int dropshadow;
	int background;
	char background_path[BCTEXTLEN];

	void convert_text();
	int demand(long sz);

// Time Code Format
	int timecode_format;
// drag enable
	int drag;
// loop background playback
	int loop_playback;
};

class TitleGlyph
{
public:
	TitleGlyph();
	~TitleGlyph();

	FT_ULong char_code;
	int freetype_index;
	BC_FontEntry *font;
	int width, height, style;
	int size, pitch;
	int advance_x;
	int left, top, right, bottom;
	VFrame *data, *data_stroke;
};
class TitleGlyphs : public ArrayList<TitleGlyph *> {
public:
	void clear() { remove_all_objects(); }
	int count() { return size(); }

	TitleGlyphs() {}
	~TitleGlyphs() { clear(); }
};

class TitleImage {
public:
	char *path;
	VFrame *vframe;

	TitleImage(const char *path, VFrame *vfrm);
	~TitleImage();
};
class TitleImages : public ArrayList<TitleImage *> {
public:
	void clear() { remove_all_objects(); }
	int count() { return size(); }

	TitleImages() {}
	~TitleImages() { clear(); }
};


// Position of each image box in a row
class TitleChar {
public:
	wchar_t wch;
	int typ, flags;
	void *vp;
	int x, y;
	int row, dx;
	int color, alpha;
	float fade;
	float blink, size;

	TitleChar *init(int typ, void *vp);
};
class TitleChars : public ArrayList<TitleChar *> {
	int next;
public:
	void reset() { next = 0; }
	void clear() { remove_all_objects(); reset(); }
	int count() { return next; }
	TitleChar *add(int typ, void *vp) {
		TitleChar *ret = next < size() ? get(next++) : 0;
		if( !ret ) { append(ret = new TitleChar());  next = size(); }
		return ret->init(typ, vp);
	}
	TitleChars() { reset(); }
	~TitleChars() { clear(); }
};

class TitleRow {
public:
	float x0, y0, x1, y1, x2, y2;
	TitleRow *init();
	void bound(float lt, float tp, float rt, float bt) {
		if( x1 > lt ) x1 = lt;
		if( y1 < tp ) y1 = tp;
		if( x2 < rt ) x2 = rt;
		if( y2 > bt ) y2 = bt;
	}
};
class TitleRows : public ArrayList<TitleRow *> {
	int next;
public:
	void reset() { next = 0; }
	void clear() { remove_all_objects(); reset(); }
	int count() { return next; }
	TitleRow *add() {
		TitleRow *ret = next < size() ? get(next++) : 0;
		if( !ret ) { append(ret = new TitleRow());  next = size(); }
		return ret->init();
	}
	TitleRows() { reset(); }
	~TitleRows() { clear(); }
};

// Draw a single character into the glyph cache
//
class GlyphPackage : public LoadPackage
{
public:
	GlyphPackage();
	TitleGlyph *glyph;
};


class GlyphUnit : public LoadClient
{
public:
	GlyphUnit(TitleMain *plugin, GlyphEngine *server);
	~GlyphUnit();
	void process_package(LoadPackage *package);

	TitleMain *plugin;
	FT_Library freetype_library;      	// Freetype library
	FT_Face freetype_face;
};

class GlyphEngine : public LoadServer
{
public:
	GlyphEngine(TitleMain *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	TitleMain *plugin;
};


// Copy a single character to the text mask
class TitlePackage : public LoadPackage
{
public:
	TitlePackage();
	int x, y;
	TitleChar *chr;
};

// overlay modes
#define DRAW_ALPHA 1
#define DRAW_COLOR 2
#define DRAW_IMAGE 3

class TitleUnit : public LoadClient
{
public:
	TitleUnit(TitleMain *plugin, TitleEngine *server);
	void process_package(LoadPackage *package);
	void draw_frame(int mode, VFrame *dst, VFrame *src, int x, int y);

	TitleMain *plugin;
	TitleEngine *engine;
	TitleChar *chr;
};

class TitleEngine : public LoadServer
{
public:
	TitleEngine(TitleMain *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	TitleMain *plugin;
	int do_dropshadow;
};


// Create outline
class TitleOutlinePackage : public LoadPackage
{
public:
	TitleOutlinePackage();
	int y1, y2;
};


class TitleOutlineUnit : public LoadClient
{
public:
	TitleOutlineUnit(TitleMain *plugin, TitleOutlineEngine *server);
	void process_package(LoadPackage *package);
	TitleMain *plugin;
	TitleOutlineEngine *engine;
};

class TitleOutlineEngine : public LoadServer
{
public:
	TitleOutlineEngine(TitleMain *plugin, int cpus);
	void init_packages();
	void do_outline();
	LoadClient* new_client();
	LoadPackage* new_package();
	TitleMain *plugin;
	int pass;
};

template<class typ> class TitleStack : public ArrayList<typ>
{
	typ &last() { return ArrayList<typ>::last(); }
	int size() { return ArrayList<typ>::size(); }
	typ &append(typ &v) { return ArrayList<typ>::append(v); }
	void remove() { return ArrayList<typ>::remove(); }
public:
	TitleParser *parser;
	TitleStack(TitleParser *p, typ v) : parser(p) { append(v); }
	operator typ&() { return last(); }
	typ &push(typ &v) { return append(v); }
	int pop() { return size()>1 ? (remove(),0) : 1; }
	int set(const char *txt);
	int unset(const char *txt);
};

template<class typ> int TitleStack<typ>::set(const char *txt)
{
	typ v = !*txt ? 1 : strtol(txt,(char **)&txt,0);
        if( *txt || v < 0 || v > 1 ) return 1;
        push(v);
        return 0;
}
template<class typ> int TitleStack<typ>::unset(const char *txt)
{
        return pop();
}

class TitleCurNudge : public TitleStack<int> {
public:
	TitleCurNudge(TitleParser *parser, TitleMain *plugin);
	int set(const char *txt);
};

class TitleCurColor : public TitleStack<int> {
public:
	TitleCurColor(TitleParser *parser, TitleMain *plugin);
	int set(const char *txt);
};

class TitleCurAlpha : public TitleStack<int> {
public:
	TitleCurAlpha(TitleParser *parser, TitleMain *plugin);
	int set(const char *txt);
};

class TitleCurSize : public TitleStack<float> {
public:
	TitleCurSize(TitleParser *parser, TitleMain *plugin);
	int set(const char *txt);
	int unset(const char *txt);
};

class TitleCurBold : public TitleStack<int> {
public:
	TitleCurBold(TitleParser *parser, TitleMain *plugin);
	int set(const char *txt);
	int unset(const char *txt);
};

class TitleCurItalic : public TitleStack<int> {
public:
	TitleCurItalic(TitleParser *parser, TitleMain *plugin);
	int set(const char *txt);
	int unset(const char *txt);
};

class TitleCurFont : public TitleStack<BC_FontEntry*>
{
public:
	BC_FontEntry *get(const char *txt, int style);
	BC_FontEntry *set(const char *txt, int style);
	int style();
	virtual int set(const char *txt=0);
	virtual int unset(const char *txt);
	TitleCurFont(TitleParser *parser, TitleMain *plugin);
};

class TitleCurCaps : public TitleStack<int> {
public:
	TitleCurCaps(TitleParser *parser, TitleMain *plugin);
	int set(const char *txt);
};

class TitleCurUnder : public TitleStack<int> {
public:
	TitleCurUnder(TitleParser *parser, TitleMain *plugin);
};

class TitleCurBlink : public TitleStack<float> {
public:
	TitleCurBlink(TitleParser *parser, TitleMain *plugin);
	int set(const char *txt);
};

class TitleCurFixed : public TitleStack<int> {
public:
	TitleCurFixed(TitleParser *parser, TitleMain *plugin);
	int set(const char *txt);
};

class TitleCurAlias : public TitleStack<int> {
public:
	TitleCurAlias(TitleParser *parser, TitleMain *plugin);
	int set(const char *txt);
};

class TitleCurSuper : public TitleStack<int> {
public:
	TitleCurSuper(TitleParser *parser, TitleMain *plugin);
	int set(const char *txt);
};


class TitleParser
{
	const wchar_t *bfr, *out, *lmt;
public:
	TitleMain *plugin;

	long tell() { return out - bfr; }
	void seek(long pos) { out = bfr + pos; }
	bool eof() { return out >= lmt; }
	int wcur() { return eof() ? -1 : *out; }
	int wnext() { return eof() ? -1 : *out++; }
	int wget(wchar_t &wch);
	int tget(wchar_t &wch);
	wchar_t wid[BCSTRLEN], wtxt[BCTEXTLEN];
	char id[BCSTRLEN], text[BCTEXTLEN];
	int set_attributes(int ret);

	TitleCurNudge  cur_nudge;
	TitleCurColor  cur_color;
	TitleCurAlpha  cur_alpha;
	TitleCurSize   cur_size;
	TitleCurBold   cur_bold;
	TitleCurItalic cur_italic;
	TitleCurFont   cur_font;
	TitleCurCaps   cur_caps;
	TitleCurUnder  cur_under;
	TitleCurBlink  cur_blink;
	TitleCurFixed  cur_fixed;
	TitleCurAlias  cur_alias;
	TitleCurSuper  cur_super;

	TitleParser(TitleMain *main);
};


// Overlay text mask with fractional translation
// We don't use OverlayFrame to enable alpha blending on non alpha
// output.
class TitleTranslatePackage : public LoadPackage
{
public:
	TitleTranslatePackage();
	int y1, y2;
};

class TitleTranslateUnit : public LoadClient
{
public:
	TitleTranslateUnit(TitleMain *plugin, TitleTranslate *server);

	void process_package(LoadPackage *package);
	TitleMain *plugin;
};

class TitleTranslate : public LoadServer
{
public:
	TitleTranslate(TitleMain *plugin, int cpus);
	~TitleTranslate();

	TitleMain *plugin;
	VFrame *input;
	float in_w, in_h, out_w, out_h;
	float ix1, iy1, ix2, iy2;
	float ox1, oy1, ox2, oy2;

	void copy(VFrame *input);
	LoadClient* new_client();
	LoadPackage* new_package();
	void init_packages();
};


class TitleMain : public PluginVClient
{
public:
	TitleMain(PluginServer *server);
	~TitleMain();

// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS(TitleConfig)
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	int is_synthesis();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void insert_text(const wchar_t *wtxt, int pos);

	void build_previews(TitleWindow *gui);
	void reset_render();
	int init_freetype();
	int set_font(BC_FontEntry*&font, const char *txt);
	int set_font(BC_FontEntry*&font, const char *txt, int style);
	int set_size(int &size, const char *txt);
	int set_color(int &color, const char *txt);
	int set_bold(int &bold, const char *txt);
	int set_italic(int &italic, const char *txt);
	int set_caps(int &caps, const char *txt);
	int set_under(int &under, const char *txt);
	void load_glyphs();
	int draw_text(int need_redraw);
	int draw_underline(VFrame *mask, int alpha);
	void draw_overlay();
	void draw_boundry();
	int get_text();
	int get_visible_text();
	int check_char_code_path(FT_Library &freetype_library,
		char *path_old, FT_ULong &char_code, char *path_new);
	int load_freetype_face(FT_Library &freetype_library,
		FT_Face &freetype_face, const char *path);
	int load_font(BC_FontEntry *font);
	Indexable *open_background(const char *filename);
	int read_background(VFrame *frame, int64_t position, int color_model);
	void draw_background();
	static BC_FontEntry* get_font(const char *font_name, int style);
	BC_FontEntry* config_font();
	TitleGlyph *get_glyph(FT_ULong char_code, BC_FontEntry *font, int size, int style);
	int get_width(TitleGlyph *cur, TitleGlyph *nxt);

	VFrame *add_image(const char *path);
	VFrame *get_image(const char *path);

// backward compatibility
	void convert_encoding();
	static const char* motion_to_text(int motion);
	static int text_to_motion(const char *text);

	VFrame *text_mask;
	VFrame *stroke_mask;
	GlyphEngine *glyph_engine;
	TitleEngine *title_engine;
	VFrame *outline_mask;
	TitleOutlineEngine *outline_engine;
	TitleTranslate *translate;

	TitleChars title_chars;
	TitleRows title_rows;
	TitleGlyphs title_glyphs;
	TitleImages title_images;

	FT_Library freetype_library;
	FT_Face freetype_face;
	char text_font[BCTEXTLEN];

	int window_w, window_h;
	int fuzz, fuzz1, fuzz2;
	float title_x, title_y;
	int title_w, title_h;

	float text_x, text_y, text_w, text_h;
	float text_x1, text_y1, text_x2, text_y2;

	int mask_x, mask_y; int mask_w, mask_h;
	int mask_x1, mask_y1, mask_x2, mask_y2;

	int text_rows;
	int visible_row1, visible_char1;
	int visible_row2, visible_char2;
	float fade;

	VFrame *input, *output;
	int output_model, text_model, mask_model;

	Indexable *background;
	File *bg_file;
	VFrame *bg_frame;
	RenderEngine *render_engine;
	CICache *video_cache;
	OverlayFrame *overlay_frame;

	int64_t last_position;
	int need_reconfigure;
	int cpus;
};


#endif
