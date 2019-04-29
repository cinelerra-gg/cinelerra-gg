
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

#include "bcbitmap.h"
#include "bcpixmap.h"
#include "bcpopup.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bctimer.h"
#include "bcwindowbase.h"
#include "clip.h"
#include "bccolors.h"
#include "bctrace.h"
#include "cursors.h"
#include "fonts.h"
#include "vframe.h"
#include <string.h>
#include <wchar.h>
#include <ft2build.h>
#include "workarounds.h"

void BC_WindowBase::copy_area(int x1, int y1, int x2, int y2, int w, int h, BC_Pixmap *pixmap)
{ BT
	XCopyArea(top_level->display,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, x1, y1, w, h, x2, y2);
}


void BC_WindowBase::draw_box(int x, int y, int w, int h, BC_Pixmap *pixmap)
{ BT
//if(x == 0) printf("BC_WindowBase::draw_box %d %d %d %d\n", x, y, w, h);
	XFillRectangle(top_level->display,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, x, y, w, h);
}


void BC_WindowBase::draw_circle(int x, int y, int w, int h, BC_Pixmap *pixmap)
{ BT
	XDrawArc(top_level->display,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, x, y, (w - 1), (h - 2), 0 * 64, 360 * 64);
}

void BC_WindowBase::draw_arc(int x, int y, int w, int h,
	int start_angle, int angle_length, BC_Pixmap *pixmap)
{ BT
	XDrawArc(top_level->display,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, x, y, (w - 1), (h - 2), start_angle * 64,
		angle_length * 64);
}

void BC_WindowBase::draw_disc(int x, int y, int w, int h, BC_Pixmap *pixmap)
{ BT
	XFillArc(top_level->display,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, x, y, (w - 1), (h - 2), 0 * 64, 360 * 64);
}

void BC_WindowBase::clear_box(int x, int y, int w, int h, BC_Pixmap *pixmap)
{ BT
	set_color(bg_color);
	Pixmap xpixmap = pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap;
	XFillRectangle(top_level->display, xpixmap, top_level->gc, x, y, w, h);
}

void BC_WindowBase::draw_text_line(int x, int y, const char *text, int len,
	BC_Pixmap *pixmap)
{
#ifdef HAVE_XFT
	if( get_resources()->use_xft ) {
		draw_xft_text(x, y, text, len, pixmap);
		return;
	}
#endif
 BT
	Pixmap xpixmap = pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap;
	if( get_resources()->use_fontset ) {
		XFontSet fontset = top_level->get_curr_fontset();
		if( fontset ) {
	       		XmbDrawString(top_level->display, xpixmap, fontset,
				top_level->gc, x, y, text, len);
			return;
		}

	}
//printf("BC_WindowBase::draw_text 3\n");
	XDrawString(top_level->display, xpixmap, top_level->gc, x, y, text, len);
}

void BC_WindowBase::draw_text(int x, int y, const char *text, int length,
	BC_Pixmap *pixmap)
{
	if( length < 0 ) length = strlen(text);
	//int boldface = top_level->current_font & BOLDFACE;
	int font = top_level->current_font & 0xff;

	switch(top_level->current_font) {
	case MEDIUM_7SEGMENT:
		for(int i = 0; i < length; i++) {
			VFrame *image, **img7seg = get_resources()->medium_7segment;
			int ch = text[i];
			switch( ch ) {
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				  image = img7seg[ch-'0'];  break;
			case ':': image = img7seg[10];      break;
			case '.': image = img7seg[11];      break;
			case 'a': case 'b': case 'c':
			case 'd': case 'e': case 'f':  ch -= 'a'-'A';
			case 'A': case 'B': case 'C':  /* fallthru */
			case 'D': case 'E': case 'F':
				image = img7seg[12+ch-'A']; break;
				break;
			case '-': image = img7seg[19];      break;
			default:
			case ' ': image = img7seg[18];      break;
			}

			draw_vframe(image, x, y - image->get_h());
			x += image->get_w();
		}
		break;

	default: {
		if(top_level->get_xft_struct(top_level->current_font)) {
			draw_xft_text(x, y, text, length, pixmap);
			return;
		}
 BT
		for(int i = 0, j = 0; i <= length; i++) {
			if(text[i] == '\n' || text[i] == 0) {
				if(get_resources()->use_fontset && top_level->get_curr_fontset()) {
					XmbDrawString(top_level->display,
						pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
						top_level->get_curr_fontset(),
						top_level->gc, x, y, &text[j], i-j);
				}
				else {
					XDrawString(top_level->display,
						pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
						top_level->gc, x, y, &text[j], i-j);
				}
				j = i + 1;
				y += get_text_height(font);
			}
		}
		break; }
	}
}

void BC_WindowBase::draw_utf8_text(int x, int y,
	const char *text, int length, BC_Pixmap *pixmap)
{
	if(length < 0) length = strlen(text);

	if(top_level->get_xft_struct(top_level->current_font))
	{
		draw_xft_text(x,
			y,
			text,
			length,
			pixmap,
			1);
		return;
	}
 BT
	for(int i = 0, j = 0; i <= length; i++)
	{
		if(text[i] == '\n' || text[i] == 0)
		{
			if(get_resources()->use_fontset && top_level->get_curr_fontset())
			{
				XmbDrawString(top_level->display,
					pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
					top_level->get_curr_fontset(),
					top_level->gc,
					x,
					y,
					&text[j],
					i - j);
			}
			else
			{
				XDrawString(top_level->display,
					pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
					top_level->gc,
					x,
					y,
					&text[j],
					i - j);
			}

			j = i + 1;
			y += get_text_height(MEDIUMFONT);
		}
	}
}

void BC_WindowBase::draw_xft_text(int x, int y,
	const char *text, int length, BC_Pixmap *pixmap, int is_utf8)
{
	int l = length + 1;
	wchar_t wide_text[l];
	length = BC_Resources::encode(
		is_utf8 ? "UTF8" : BC_Resources::encoding, BC_Resources::wide_encoding,
		(char*)text, length, (char*)wide_text, l*sizeof(wchar_t)) / sizeof(wchar_t);
	draw_xft_text(x, y, wide_text, length, pixmap);
}

void BC_WindowBase::draw_xft_text(int x, int y,
	const wchar_t *text, int length, BC_Pixmap *pixmap)
{
	int dy = -1;
	const wchar_t *wsp = text, *wep = wsp + length;
	int font = top_level->current_font;
	while( wsp < wep ) {
		const wchar_t *wcp = wsp;
		while( wcp < wep && *wcp != '\n' ) ++wcp;
		int len = wcp - wsp;
		if( len > 0 )
			draw_single_text(1, font, x, y, wsp, len, pixmap);
		if( wcp >= wep ) break;
		if( dy < 0 )
			dy = get_text_height(font);
		y += dy;
		wsp = wcp + 1;
	}
}

void BC_WindowBase::xft_draw_string(XftColor *xft_color, XftFont *xft_font,
		int x, int y, const FcChar32 *fc, int len, BC_Pixmap *pixmap)
{ BT
	Pixmap draw_pixmap = 0;
	XftDraw *xft_draw = (XftDraw *)
		(pixmap ? pixmap->opaque_xft_draw : this->pixmap->opaque_xft_draw);
	int src_x = x, src_y = y, src_w = 0, src_h = 0;
	XGCValues values;
	XGetGCValues(top_level->display, top_level->gc, GCFunction, &values);
	if( values.function != GXcopy ) {
		XSetFunction(top_level->display, top_level->gc, GXcopy);
		XGlyphInfo info;
		xftTextExtents32(top_level->display, xft_font, fc, len, &info);
		src_w = info.width;  src_h = info.height;
		draw_pixmap = XCreatePixmap(top_level->display, top_level->win,
                        src_w, src_h, top_level->default_depth);
		int color = get_color(); set_color(0);
		XFillRectangle(top_level->display, draw_pixmap, top_level->gc, 0, 0, src_w, src_h);
		set_color(color);
		xft_draw = xftDrawCreate(top_level->display, draw_pixmap,
                           top_level->vis, top_level->cmap);
		src_x = info.x;  src_y = info.y;
	}
	xftDrawString32(xft_draw, xft_color, xft_font, src_x, src_y, fc, len);
	if( values.function != GXcopy ) {
		XSetFunction(top_level->display, top_level->gc, values.function);
		Pixmap xpixmap = pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap;
		XCopyArea(top_level->display, draw_pixmap, xpixmap,
                        top_level->gc, 0, 0, src_w, src_h, x, y);
		XFreePixmap(top_level->display, draw_pixmap);
		xftDrawDestroy(xft_draw);
	}
}

int BC_WindowBase::get_single_text_width(int font, const wchar_t *text, int length)
{
	return draw_single_text(0, font, 0,0, text, length);
}

int BC_WindowBase::draw_single_text(int draw, int font,
	int x, int y, const wchar_t *text, int length, BC_Pixmap *pixmap)
{
	if( length < 0 )
		length = wcslen(text);
	if( !length ) return 0;

	if( !get_resources()->use_xft ) {
 BT
		if( !get_font_struct(font) ) return 0;
		XChar2b xtext[length], *xp = xtext;
		for( int i=0; i<length; ++i,++xp ) {
			xp->byte1 = (unsigned char) (text[i] >> 8);
			xp->byte2 = (unsigned char) (text[i] & 0xff);
		}
		if( draw ) {
			XDrawString16(top_level->display,
				pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
				top_level->gc, x, y, xtext, length);
		}
		return XTextWidth16(get_font_struct(font), xtext, length);
	}

#ifdef HAVE_XFT
	XftColor xft_color;
	if( draw ) {
		XRenderColor color;
		color.red = (top_level->current_color & 0xff0000) >> 16;
		color.red |= color.red << 8;
		color.green = (top_level->current_color & 0xff00) >> 8;
		color.green |= color.green << 8;
		color.blue = (top_level->current_color & 0xff);
		color.blue |= color.blue << 8;
		color.alpha = 0xffff;

		xftColorAllocValue(top_level->display, top_level->vis, top_level->cmap,
			&color, &xft_color);
	}

	int x0 = x;
	XftFont *basefont = top_level->get_xft_struct(font);
	XftFont *curfont = 0, *altfont = 0;
	const wchar_t *up = text, *ubp = up, *uep = ubp + length;

	while( up < uep ) {
		XftFont *xft_font = 0;
		if( xftCharExists(top_level->display, basefont, *up) )
			xft_font = basefont;
		else if( altfont ) {
			if( xftCharExists(top_level->display, altfont, *up))
				xft_font = altfont;
			else {
				xftFontClose(top_level->display, altfont);
				altfont = 0;
			}
		}
		if( !xft_font ) {
			FcPattern *pattern = BC_Resources::find_similar_font(*up, basefont->pattern);
			if( pattern != 0 ) {
				double psize = 0;
				fcPatternGetDouble(basefont->pattern, FC_PIXEL_SIZE, 0, &psize);
				fcPatternAddDouble(pattern, FC_PIXEL_SIZE, psize);
				fcPatternDel(pattern, FC_SCALABLE);
				xft_font = altfont = xftFontOpenPattern(top_level->display, pattern);
			}
		}
		if( !xft_font )
			xft_font = basefont;
		if( xft_font != curfont ) {
			if( curfont && up > ubp ) {
				if( draw ) {
					xft_draw_string(&xft_color, curfont, x, y,
						(const FcChar32*)ubp, up-ubp, pixmap);
				}
				XGlyphInfo extents;
				xftTextExtents32(top_level->display, curfont,
					(const FcChar32*)ubp, up-ubp, &extents);
				x += extents.xOff;
			}
			ubp = up;  curfont = xft_font;
		}
		++up;
	}

	if( curfont && up > ubp ) {
		if( draw ) {
			xft_draw_string(&xft_color, curfont, x, y,
				(const FcChar32*)ubp, up-ubp, pixmap);
		}
		XGlyphInfo extents;
		xftTextExtents32(top_level->display, curfont,
			(const FcChar32*)ubp, up-ubp, &extents);
		x += extents.xOff;
	}

	if( altfont )
		xftFontClose(top_level->display, altfont);

	xftColorFree(top_level->display, top_level->vis, top_level->cmap, &xft_color);
#endif
	return x - x0;
}

void BC_WindowBase::truncate_text(char *result, const char *text, int w)
{
	int new_w = get_text_width(current_font, text);

	if( new_w > w ) {
		const char* separator = "...";
		int separator_w = get_text_width(current_font, separator);
// can't fit
		if( separator_w >= w ) {
			strcpy(result, separator);
			return;
		}

		int text_len = strlen(text);
// widen middle gap until it fits
		for( int i=text_len/2; i>0; --i ) {
			strncpy(result, text, i);
			result[i] = 0;
			strcat(result, separator);
			strncat(result, text + text_len - i, i);
			result[i + strlen(separator) + i] = 0;
			new_w = get_text_width(current_font, result);
//printf("BC_WindowBase::truncate_text %d %d %d %s\n", __LINE__, new_w, w, result);
			if(new_w < w) return;
		}

// Didn't fit
		strcpy(result, separator);
		return;
	}

	strcpy(result, text);
}

void BC_WindowBase::draw_center_text(int x, int y, const char *text, int length)
{
	if(length < 0) length = strlen(text);
	int w = get_text_width(current_font, text, length);
	x -= w / 2;
	draw_text(x, y, text, length);
}

void BC_WindowBase::draw_line(int x1, int y1, int x2, int y2, BC_Pixmap *pixmap)
{ BT
// Some X drivers can't draw 0 length lines
	if( x1 == x2 && y1 == y2 ) {
		draw_pixel(x1, y1, pixmap);
	}
	else {
		XDrawLine(top_level->display,
			pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
			top_level->gc, x1, y1, x2, y2);
	}
}

void BC_WindowBase::draw_polygon(ArrayList<int> *x, ArrayList<int> *y, BC_Pixmap *pixmap)
{ BT
	int npoints = MIN(x->total, y->total);
	XPoint *points = new XPoint[npoints];

	for( int i=0; i<npoints; ++i ) {
		points[i].x = x->values[i];
		points[i].y = y->values[i];
	}

	XDrawLines(top_level->display,
    		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
	    	top_level->gc, points, npoints, CoordModeOrigin);

	delete [] points;
}

void BC_WindowBase::fill_polygon(ArrayList<int> *x, ArrayList<int> *y, BC_Pixmap *pixmap)
{ BT
	int npoints = MIN(x->total, y->total);
	XPoint *points = new XPoint[npoints];

	for( int i=0; i<npoints; ++i ) {
		points[i].x = x->values[i];
		points[i].y = y->values[i];
	}

	XFillPolygon(top_level->display,
	    	pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
    		top_level->gc, points, npoints, Nonconvex, CoordModeOrigin);

	delete [] points;
}


void BC_WindowBase::draw_rectangle(int x, int y, int w, int h)
{ BT
	XDrawRectangle(top_level->display,
		pixmap->opaque_pixmap, top_level->gc,
		x, y, w - 1, h - 1);
}

void BC_WindowBase::draw_3d_border(int x, int y, int w, int h, int is_down)
{
	draw_3d_border(x, y, w, h,
		top_level->get_resources()->border_shadow2,
		top_level->get_resources()->border_shadow1,
		top_level->get_resources()->border_light1,
		top_level->get_resources()->border_light2);
}


void BC_WindowBase::draw_3d_border(int x, int y, int w, int h,
	int light1, int light2, int shadow1, int shadow2)
{
	int lx, ly, ux, uy;

	h--; w--;

	lx = x+1;  ly = y+1;
	ux = x+w-1;  uy = y+h-1;

	set_color(light1);
	draw_line(x, y, ux, y);
	draw_line(x, y, x, uy);
	set_color(light2);
	draw_line(lx, ly, ux - 1, ly);
	draw_line(lx, ly, lx, uy - 1);

	set_color(shadow1);
	draw_line(ux, ly, ux, uy);
	draw_line(lx, uy, ux, uy);
	set_color(shadow2);
	draw_line(x + w, y, x + w, y + h);
	draw_line(x, y + h, x + w, y + h);
}

void BC_WindowBase::draw_3d_box(int x, int y, int w, int h,
	int light1, int light2, int middle, int shadow1, int shadow2,
	BC_Pixmap *pixmap)
{
	int lx, ly, ux, uy;

	h--; w--;

	lx = x+1;  ly = y+1;
	ux = x+w-1;  uy = y+h-1;

	set_color(middle);
	draw_box(x, y, w, h, pixmap);

	set_color(light1);
	draw_line(x, y, ux, y, pixmap);
	draw_line(x, y, x, uy, pixmap);
	set_color(light2);
	draw_line(lx, ly, ux - 1, ly, pixmap);
	draw_line(lx, ly, lx, uy - 1, pixmap);

	set_color(shadow1);
	draw_line(ux, ly, ux, uy, pixmap);
	draw_line(lx, uy, ux, uy, pixmap);
	set_color(shadow2);
	draw_line(x + w, y, x + w, y + h, pixmap);
	draw_line(x, y + h, x + w, y + h, pixmap);
}

void BC_WindowBase::draw_colored_box(int x, int y, int w, int h, int down, int highlighted)
{
	if(!down)
	{
		if(highlighted)
			draw_3d_box(x, y, w, h,
				top_level->get_resources()->button_light,
				top_level->get_resources()->button_highlighted,
				top_level->get_resources()->button_highlighted,
				top_level->get_resources()->button_shadow,
				BLACK);
		else
			draw_3d_box(x, y, w, h,
				top_level->get_resources()->button_light,
				top_level->get_resources()->button_up,
				top_level->get_resources()->button_up,
				top_level->get_resources()->button_shadow,
				BLACK);
	}
	else
	{
// need highlighting for toggles
		if(highlighted)
			draw_3d_box(x, y, w, h,
				top_level->get_resources()->button_shadow,
				BLACK,
				top_level->get_resources()->button_up,
				top_level->get_resources()->button_up,
				top_level->get_resources()->button_light);
		else
			draw_3d_box(x, y, w, h,
				top_level->get_resources()->button_shadow,
				BLACK,
				top_level->get_resources()->button_down,
				top_level->get_resources()->button_down,
				top_level->get_resources()->button_light);
	}
}


void BC_WindowBase::draw_border(char *text, int x, int y, int w, int h)
{
	int left_indent = 20;
	int lx, ly, ux, uy;

	h--; w--;
	lx = x + 1;  ly = y + 1;
	ux = x + w - 1;  uy = y + h - 1;

	set_opaque();
	if(text && text[0] != 0)
	{
		set_color(BLACK);
		set_font(MEDIUMFONT);
		draw_text(x + left_indent, y + get_text_height(MEDIUMFONT) / 2, text);
	}

	set_color(top_level->get_resources()->button_shadow);
	draw_line(x, y, x + left_indent - 5, y);
	draw_line(x, y, x, uy);
	draw_line(x + left_indent + 5 + get_text_width(MEDIUMFONT, text), y, ux, y);
	draw_line(x, y, x, uy);
	draw_line(ux, ly, ux, uy);
	draw_line(lx, uy, ux, uy);
	set_color(top_level->get_resources()->button_light);
	draw_line(lx, ly, x + left_indent - 5 - 1, ly);
	draw_line(lx, ly, lx, uy - 1);
	draw_line(x + left_indent + 5 + get_text_width(MEDIUMFONT, text), ly, ux - 1, ly);
	draw_line(lx, ly, lx, uy - 1);
	draw_line(x + w, y, x + w, y + h);
	draw_line(x, y + h, x + w, y + h);
}

void BC_WindowBase::draw_triangle_down_flat(int x, int y, int w, int h)
{ BT
	int x1, y1, x2, y2, x3;
	XPoint point[3];

	x1 = x+1; x2 = x + w/2; x3 = x+w-1;
	y1 = y; y2 = y+h-1;

	point[0].x = x2; point[0].y = y2;
	point[1].x = x3; point[1].y = y1;
	point[2].x = x1; point[2].y = y1;

	XFillPolygon(top_level->display,
		pixmap->opaque_pixmap,
		top_level->gc,
		(XPoint *)point,
		3,
		Nonconvex,
		CoordModeOrigin);
	draw_line(x1,y1, x3,y1);
}

void BC_WindowBase::draw_triangle_up(int x, int y, int w, int h,
	int light1, int light2, int middle, int shadow1, int shadow2)
{ BT
	int x1, y1, x2, y2, x3;
	XPoint point[3];

	x1 = x; y1 = y; x2 = x + w / 2;
	y2 = y + h - 1; x3 = x + w - 1;

// middle
	point[0].x = x2; point[0].y = y1; point[1].x = x3;
	point[1].y = y2; point[2].x = x1; point[2].y = y2;

	set_color(middle);
	XFillPolygon(top_level->display,
		pixmap->opaque_pixmap,
		top_level->gc,
		(XPoint *)point,
		3,
		Nonconvex,
		CoordModeOrigin);

// bottom and top right
	set_color(shadow1);
	draw_line(x3, y2-1, x1, y2-1);
	draw_line(x2-1, y1, x3-1, y2);
	set_color(shadow2);
	draw_line(x3, y2, x1, y2);
	draw_line(x2, y1, x3, y2);

// top left
	set_color(light2);
	draw_line(x2+1, y1, x1+1, y2);
	set_color(light1);
	draw_line(x2, y1, x1, y2);
}

void BC_WindowBase::draw_triangle_down(int x, int y, int w, int h,
	int light1, int light2, int middle, int shadow1, int shadow2)
{
	int x1, y1, x2, y2, x3;
	XPoint point[3];

	x1 = x; x2 = x + w / 2; x3 = x + w - 1;
	y1 = y; y2 = y + h - 1;

	point[0].x = x2; point[0].y = y2; point[1].x = x3;
	point[1].y = y1; point[2].x = x1; point[2].y = y1;

	set_color(middle);
	XFillPolygon(top_level->display,
		pixmap->opaque_pixmap,
		top_level->gc,
		(XPoint *)point,
		3,
		Nonconvex,
		CoordModeOrigin);

// top and bottom left
	set_color(light2);
	draw_line(x3-1, y1+1, x1+1, y1+1);
	draw_line(x1+1, y1, x2+1, y2);
	set_color(light1);
	draw_line(x3, y1, x1, y1);
	draw_line(x1, y1, x2, y2);

// bottom right
	set_color(shadow1);
  	draw_line(x3-1, y1, x2-1, y2);
	set_color(shadow2);
	draw_line(x3, y1, x2, y2);
}

void BC_WindowBase::draw_triangle_left(int x, int y, int w, int h,
	int light1, int light2, int middle, int shadow1, int shadow2)
{ BT
  	int x1, y1, x2, y2, y3;
	XPoint point[3];

	// draw back arrow
  	y1 = y; x1 = x; y2 = y + h / 2;
  	x2 = x + w - 1; y3 = y + h - 1;

	point[0].x = x1; point[0].y = y2; point[1].x = x2;
	point[1].y = y1; point[2].x = x2; point[2].y = y3;

	set_color(middle);
  	XFillPolygon(top_level->display,
		pixmap->opaque_pixmap,
		top_level->gc,
		(XPoint *)point,
		3,
		Nonconvex,
		CoordModeOrigin);

// right and bottom right
	set_color(shadow1);
  	draw_line(x2-1, y1, x2-1, y3-1);
  	draw_line(x2, y3-1, x1, y2-1);
	set_color(shadow2);
  	draw_line(x2, y1, x2, y3);
  	draw_line(x2, y3, x1, y2);

// top left
	set_color(light1);
	draw_line(x1, y2, x2, y1);
	set_color(light2);
	draw_line(x1, y2+1, x2, y1+1);
}

void BC_WindowBase::draw_triangle_right(int x, int y, int w, int h,
	int light1, int light2, int middle, int shadow1, int shadow2)
{ BT
  	int x1, y1, x2, y2, y3;
	XPoint point[3];

	y1 = y; y2 = y + h / 2; y3 = y + h - 1;
	x1 = x; x2 = x + w - 1;

	point[0].x = x1; point[0].y = y1; point[1].x = x2;
	point[1].y = y2; point[2].x = x1; point[2].y = y3;

	set_color(middle);
  	XFillPolygon(top_level->display,
		pixmap->opaque_pixmap,
		top_level->gc,
		(XPoint *)point,
		3,
		Nonconvex,
		CoordModeOrigin);

// left and top right
	set_color(light2);
	draw_line(x1+1, y3, x1+1, y1);
	draw_line(x1, y1+1, x2, y2+1);
	set_color(light1);
	draw_line(x1, y3, x1, y1);
	draw_line(x1, y1, x2, y2);

// bottom right
	set_color(shadow1);
  	draw_line(x2, y2-1, x1, y3-1);
	set_color(shadow2);
  	draw_line(x2, y2, x1, y3);
}


void BC_WindowBase::draw_check(int x, int y)
{
	const int w = 15, h = 15;
	draw_line(x + 3, y + h / 2 + 0, x + 6, y + h / 2 + 2);
	draw_line(x + 3, y + h / 2 + 1, x + 6, y + h / 2 + 3);
	draw_line(x + 6, y + h / 2 + 2, x + w - 4, y + h / 2 - 3);
	draw_line(x + 3, y + h / 2 + 2, x + 6, y + h / 2 + 4);
	draw_line(x + 6, y + h / 2 + 2, x + w - 4, y + h / 2 - 3);
	draw_line(x + 6, y + h / 2 + 3, x + w - 4, y + h / 2 - 2);
	draw_line(x + 6, y + h / 2 + 4, x + w - 4, y + h / 2 - 1);
}

void BC_WindowBase::draw_tiles(BC_Pixmap *tile, int origin_x, int origin_y, int x, int y, int w, int h)
{ BT
	if( !tile ) {
		set_color(bg_color);
		draw_box(x, y, w, h);
	}
	else {
		XSetFillStyle(top_level->display, top_level->gc, FillTiled);
// Don't know how slow this is
		XSetTile(top_level->display, top_level->gc, tile->get_pixmap());
		XSetTSOrigin(top_level->display, top_level->gc, origin_x, origin_y);
		draw_box(x, y, w, h);
		XSetFillStyle(top_level->display, top_level->gc, FillSolid);
	}
}

void BC_WindowBase::draw_top_tiles(BC_WindowBase *parent_window, int x, int y, int w, int h)
{ BT
	Window tempwin;
	int origin_x, origin_y;
	XTranslateCoordinates(top_level->display,
		parent_window->win, win,
		0, 0, &origin_x, &origin_y, &tempwin);
	draw_tiles(parent_window->bg_pixmap,
		origin_x, origin_y,
		x, y, w, h);
}

void BC_WindowBase::draw_top_background(BC_WindowBase *parent_window,
	int x, int y, int w, int h, BC_Pixmap *pixmap)
{ BT
	Window tempwin;
	int top_x, top_y;
	XLockDisplay(top_level->display);

	XTranslateCoordinates(top_level->display,
		win, parent_window->win,
		x, y, &top_x, &top_y, &tempwin);

	XCopyArea(top_level->display,
		parent_window->pixmap->opaque_pixmap,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, top_x, top_y, w, h, x, y);

	XUnlockDisplay(top_level->display);
}

void BC_WindowBase::draw_background(int x, int y, int w, int h)
{
	if( bg_pixmap ) {
		draw_tiles(bg_pixmap, 0, 0, x, y, w, h);
	}
	else {
		clear_box(x, y, w, h);
	}
}

void BC_WindowBase::draw_bitmap(BC_Bitmap *bitmap, int dont_wait,
	int dest_x, int dest_y, int dest_w, int dest_h,
	int src_x, int src_y, int src_w, int src_h,
	BC_Pixmap *pixmap)
{ BT
// Hide cursor if video enabled
	update_video_cursor();

//printf("BC_WindowBase::draw_bitmap %d dest_y=%d\n", __LINE__, dest_y);
	if( dest_w <= 0 || dest_h <= 0 ) {
// Use hardware scaling to canvas dimensions if proper color model.
		if( bitmap->get_color_model() == BC_YUV420P ) {
			dest_w = w;
			dest_h = h;
		}
		else {
			dest_w = bitmap->get_w();
			dest_h = bitmap->get_h();
		}
	}

	if( src_w <= 0 || src_h <= 0 ) {
		src_w = bitmap->get_w();
		src_h = bitmap->get_h();
	}

	if( video_on ) {
		bitmap->write_drawable(win,
			top_level->gc, src_x, src_y, src_w, src_h,
			dest_x, dest_y, dest_w, dest_h, dont_wait);
		top_level->flush();
	}
	else {
		bitmap->write_drawable(pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
			top_level->gc, dest_x, dest_y, src_x, src_y, dest_w, dest_h, dont_wait);
	}
//printf("BC_WindowBase::draw_bitmap 2\n");
}


void BC_WindowBase::draw_pixel(int x, int y, BC_Pixmap *pixmap)
{ BT
	XDrawPoint(top_level->display,
		pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
		top_level->gc, x, y);
}


void BC_WindowBase::draw_pixmap(BC_Pixmap *pixmap,
	int dest_x, int dest_y, int dest_w, int dest_h,
	int src_x, int src_y, BC_Pixmap *dst)
{ BT
	pixmap->write_drawable(dst ? dst->opaque_pixmap : this->pixmap->opaque_pixmap,
			dest_x, dest_y, dest_w, dest_h, src_x, src_y);
}

void BC_WindowBase::draw_vframe(VFrame *frame,
	int dest_x, int dest_y, int dest_w, int dest_h,
	int src_x, int src_y, int src_w, int src_h,
	BC_Pixmap *pixmap)
{
	if(dest_w <= 0) dest_w = frame->get_w() - src_x;
	if(dest_h <= 0) dest_h = frame->get_h() - src_y;
	if(src_w <= 0) src_w = frame->get_w() - src_x;
	if(src_h <= 0) src_h = frame->get_h() - src_y;
	CLAMP(src_x, 0, frame->get_w() - 1);
	CLAMP(src_y, 0, frame->get_h() - 1);
	if(src_x + src_w > frame->get_w()) src_w = frame->get_w() - src_x;
	if(src_y + src_h > frame->get_h()) src_h = frame->get_h() - src_y;

	if( !temp_bitmap )
		temp_bitmap = new BC_Bitmap(this, dest_w, dest_h, get_color_model(), 0);
	temp_bitmap->match_params(dest_w, dest_h, get_color_model(), 0);

	temp_bitmap->read_frame(frame,
		src_x, src_y, src_w, src_h,
		0, 0, dest_w, dest_h, bg_color);

	draw_bitmap(temp_bitmap, 0,
		dest_x, dest_y, dest_w, dest_h,
		0, 0, -1, -1, pixmap);
}

void BC_WindowBase::draw_tooltip(const char *text)
{
	if( !text )
		text = tooltip_text;
	if(tooltip_popup && text)
	{
		int w = tooltip_popup->get_w(), h = tooltip_popup->get_h();
		tooltip_popup->set_color(get_resources()->tooltip_bg_color);
		tooltip_popup->draw_box(0, 0, w, h);
		tooltip_popup->set_color(BLACK);
		tooltip_popup->draw_rectangle(0, 0, w, h);
		tooltip_popup->set_font(MEDIUMFONT);
		tooltip_popup->draw_text(TOOLTIP_MARGIN,
			get_text_ascent(MEDIUMFONT) + TOOLTIP_MARGIN,
			text);
	}
}

void BC_WindowBase::slide_left(int distance)
{ BT
	if(distance < w)
	{
		XCopyArea(top_level->display,
			pixmap->opaque_pixmap,
			pixmap->opaque_pixmap,
			top_level->gc,
			distance,
			0,
			w - distance,
			h,
			0,
			0);
	}
}

void BC_WindowBase::slide_right(int distance)
{ BT
	if(distance < w)
	{
		XCopyArea(top_level->display,
			pixmap->opaque_pixmap,
			pixmap->opaque_pixmap,
			top_level->gc,
			0,
			0,
			w - distance,
			h,
			distance,
			0);
	}
}

void BC_WindowBase::slide_up(int distance)
{ BT
	if(distance < h)
	{
		XCopyArea(top_level->display,
			pixmap->opaque_pixmap,
			pixmap->opaque_pixmap,
			top_level->gc,
			0,
			distance,
			w,
			h - distance,
			0,
			0);
		set_color(bg_color);
		XFillRectangle(top_level->display,
			pixmap->opaque_pixmap,
			top_level->gc,
			0,
			h - distance,
			w,
			distance);
	}
}

void BC_WindowBase::slide_down(int distance)
{ BT
	if(distance < h)
	{
		XCopyArea(top_level->display,
			pixmap->opaque_pixmap,
			pixmap->opaque_pixmap,
			top_level->gc,
			0,
			0,
			w,
			h - distance,
			0,
			distance);
		set_color(bg_color);
		XFillRectangle(top_level->display,
			pixmap->opaque_pixmap,
			top_level->gc,
			0,
			0,
			w,
			distance);
	}
}

// 3 segments in separate pixmaps.  Obsolete.
void BC_WindowBase::draw_3segment(int x,
	int y,
	int w,
	int h,
	BC_Pixmap *left_image,
	BC_Pixmap *mid_image,
	BC_Pixmap *right_image,
	BC_Pixmap *pixmap)
{
	if(w <= 0 || h <= 0) return;
	int left_boundary = left_image->get_w_fixed();
	int right_boundary = w - right_image->get_w_fixed();
	for(int i = 0; i < w; )
	{
		BC_Pixmap *image;

		if(i < left_boundary)
			image = left_image;
		else
		if(i < right_boundary)
			image = mid_image;
		else
			image = right_image;

		int output_w = image->get_w_fixed();

		if(i < left_boundary)
		{
			if(i + output_w > left_boundary) output_w = left_boundary - i;
		}
		else
		if(i < right_boundary)
		{
			if(i + output_w > right_boundary) output_w = right_boundary - i;
		}
		else
			if(i + output_w > w) output_w = w - i;

		image->write_drawable(pixmap ? pixmap->opaque_pixmap : this->pixmap->opaque_pixmap,
				x + i,
				y,
				output_w,
				h,
				0,
				0);

		i += output_w;
	}
}
// 3 segments in separate vframes.  Obsolete.
void BC_WindowBase::draw_3segment(int x,
	int y,
	int w,
	int h,
	VFrame *left_image,
	VFrame *mid_image,
	VFrame *right_image,
	BC_Pixmap *pixmap)
{
	if(w <= 0 || h <= 0) return;
	int left_boundary = left_image->get_w_fixed();
	int right_boundary = w - right_image->get_w_fixed();


	for(int i = 0; i < w; )
	{
		VFrame *image;

		if(i < left_boundary)
			image = left_image;
		else
		if(i < right_boundary)
			image = mid_image;
		else
			image = right_image;

		int output_w = image->get_w_fixed();

		if(i < left_boundary)
		{
			if(i + output_w > left_boundary) output_w = left_boundary - i;
		}
		else
		if(i < right_boundary)
		{
			if(i + output_w > right_boundary) output_w = right_boundary - i;
		}
		else
			if(i + output_w > w) output_w = w - i;

		if(image)
			draw_vframe(image,
					x + i,
					y,
					output_w,
					h,
					0,
					0,
					0,
					0,
					pixmap);

		if(output_w == 0) break;
		i += output_w;
	}
}

// Draw all 3 segments in a single vframe for a changing level

// total_x
// <------>
// total_w
//         <------------------------------------------------------------>
// x
// |
// w
// <-------------------------------------------------------------------->
// output
//         |-------------------|----------------------|------------------|


void BC_WindowBase::draw_3segmenth(int x,
		int y,
		int w,
		VFrame *image,
		BC_Pixmap *pixmap)
{
	draw_3segmenth(x,
		y,
		w,
		x,
		w,
		image,
		pixmap);
}

void BC_WindowBase::draw_3segmenth(int x, int y, int w,
		int total_x, int total_w, VFrame *image,
		BC_Pixmap *pixmap)
{
	if(total_w <= 0 || w <= 0 || h <= 0) return;
	int third_image = image->get_w() / 3;
	int half_image = image->get_w() / 2;
	//int left_boundary = third_image;
	//int right_boundary = total_w - third_image;
	int left_in_x = 0;
	int left_in_w = third_image;
	int left_out_x = total_x;
	int left_out_w = third_image;
	int right_in_x = image->get_w() - third_image;
	int right_in_w = third_image;
	int right_out_x = total_x + total_w - third_image;
	int right_out_w = third_image;
	int center_out_x = total_x + third_image;
	int center_out_w = total_w - third_image * 2;
	//int image_x, image_w;

//printf("BC_WindowBase::draw_3segment 1 left_out_x=%d left_out_w=%d center_out_x=%d center_out_w=%d right_out_x=%d right_out_w=%d\n",
//	left_out_x, left_out_w, center_out_x, center_out_w, right_out_x, right_out_w);

	if(left_out_x < x)
	{
		left_in_w -= x - left_out_x;
		left_out_w -= x - left_out_x;
		left_in_x += x - left_out_x;
		left_out_x += x - left_out_x;
	}

	if(left_out_x + left_out_w > x + w)
	{
		left_in_w -= (left_out_x + left_out_w) - (x + w);
		left_out_w -= (left_out_x + left_out_w) - (x + w);
	}

	if(right_out_x < x)
	{
		right_in_w -= x - right_out_x;
		right_out_w -= x - right_out_x;
		right_in_x += x - right_out_x;
		right_out_x += x - right_out_x;
	}

	if(right_out_x + right_out_w > x + w)
	{
		right_in_w -= (right_out_x + right_out_w) - (x + w);
		right_out_w -= (right_out_x + right_out_w) - (x + w);
	}

	if(center_out_x < x)
	{
		center_out_w -= x - center_out_x;
		center_out_x += x - center_out_x;
	}

	if(center_out_x + center_out_w > x + w)
	{
		center_out_w -= (center_out_x + center_out_w) - (x + w);
	}

	if(!temp_bitmap) temp_bitmap = new BC_Bitmap(top_level,
		image->get_w(), image->get_h(),
		get_color_model(), 0);
	temp_bitmap->match_params(image->get_w(), image->get_h(),
		get_color_model(), 0);
	temp_bitmap->read_frame(image,
		0, 0, image->get_w(), image->get_h(), bg_color);
// src width and height are meaningless in video_off mode
//printf("BC_WindowBase::draw_3segment 2 left_out_x=%d left_out_w=%d center_out_x=%d center_out_w=%d right_out_x=%d right_out_w=%d\n",
//	left_out_x, left_out_w, center_out_x, center_out_w, right_out_x, right_out_w);
	if(left_out_w > 0) {
		draw_bitmap(temp_bitmap, 0,
			left_out_x, y, left_out_w, image->get_h(),
			left_in_x, 0, -1, -1, pixmap);
	}

	if(right_out_w > 0) {
		draw_bitmap(temp_bitmap, 0,
			right_out_x, y, right_out_w, image->get_h(),
			right_in_x, 0, -1, -1, pixmap);
	}

	for( int pixel = center_out_x;
		 pixel < center_out_x + center_out_w;
		 pixel += half_image ) {
		int fragment_w = half_image;
		if(fragment_w + pixel > center_out_x + center_out_w)
			fragment_w = (center_out_x + center_out_w) - pixel;

//printf("BC_WindowBase::draw_3segment 2 pixel=%d fragment_w=%d\n", pixel, fragment_w);
		draw_bitmap(temp_bitmap, 0,
			pixel, y, fragment_w, image->get_h(),
			third_image, 0, -1, -1, pixmap);
	}

}

void BC_WindowBase::draw_3segmenth(int x, int y, int w, int total_x, int total_w,
		BC_Pixmap *src, BC_Pixmap *dst)
{
	if(w <= 0 || total_w <= 0) return;
	if(!src) printf("BC_WindowBase::draw_3segmenth src=0\n");
	int quarter_src = src->get_w() / 4;
	int half_src = src->get_w() / 2;
	//int left_boundary = quarter_src;
	//int right_boundary = total_w - quarter_src;
	int left_in_x = 0;
	int left_in_w = quarter_src;
	int left_out_x = total_x;
	int left_out_w = quarter_src;
	int right_in_x = src->get_w() - quarter_src;
	int right_in_w = quarter_src;
	int right_out_x = total_x + total_w - quarter_src;
	int right_out_w = quarter_src;
	int center_out_x = total_x + quarter_src;
	int center_out_w = total_w - quarter_src * 2;
	//int src_x, src_w;

//printf("BC_WindowBase::draw_3segment 1 left_out_x=%d left_out_w=%d center_out_x=%d center_out_w=%d right_out_x=%d right_out_w=%d\n",
//	left_out_x, left_out_w, center_out_x, center_out_w, right_out_x, right_out_w);

	if(left_out_x < x)
	{
		left_in_w -= x - left_out_x;
		left_out_w -= x - left_out_x;
		left_in_x += x - left_out_x;
		left_out_x += x - left_out_x;
	}

	if(left_out_x + left_out_w > x + w)
	{
		left_in_w -= (left_out_x + left_out_w) - (x + w);
		left_out_w -= (left_out_x + left_out_w) - (x + w);
	}

	if(right_out_x < x)
	{
		right_in_w -= x - right_out_x;
		right_out_w -= x - right_out_x;
		right_in_x += x - right_out_x;
		right_out_x += x - right_out_x;
	}

	if(right_out_x + right_out_w > x + w)
	{
		right_in_w -= (right_out_x + right_out_w) - (x + w);
		right_out_w -= (right_out_x + right_out_w) - (x + w);
	}

	if(center_out_x < x)
	{
		center_out_w -= x - center_out_x;
		center_out_x += x - center_out_x;
	}

	if(center_out_x + center_out_w > x + w)
	{
		center_out_w -= (center_out_x + center_out_w) - (x + w);
	}


//printf("BC_WindowBase::draw_3segment 2 left_out_x=%d left_out_w=%d center_out_x=%d center_out_w=%d right_out_x=%d right_out_w=%d\n",
//	left_out_x, left_out_w, center_out_x, center_out_w, right_out_x, right_out_w);
	if(left_out_w > 0)
	{
		draw_pixmap(src,
			left_out_x,
			y,
			left_out_w,
			src->get_h(),
			left_in_x,
			0,
			dst);
	}

	if(right_out_w > 0)
	{
		draw_pixmap(src,
			right_out_x,
			y,
			right_out_w,
			src->get_h(),
			right_in_x,
			0,
			dst);
	}

	for(int pixel = center_out_x;
		pixel < center_out_x + center_out_w;
		pixel += half_src)
	{
		int fragment_w = half_src;
		if(fragment_w + pixel > center_out_x + center_out_w)
			fragment_w = (center_out_x + center_out_w) - pixel;

//printf("BC_WindowBase::draw_3segment 2 pixel=%d fragment_w=%d\n", pixel, fragment_w);
		draw_pixmap(src,
			pixel,
			y,
			fragment_w,
			src->get_h(),
			quarter_src,
			0,
			dst);
	}

}


void BC_WindowBase::draw_3segmenth(int x,
		int y,
		int w,
		BC_Pixmap *src,
		BC_Pixmap *dst)
{
	if(w <= 0) return;
	int third_image = src->get_w() / 3;
	int half_output = w / 2;
	//int left_boundary = third_image;
	//int right_boundary = w - third_image;
	int left_in_x = 0;
	int left_in_w = third_image;
	int left_out_x = x;
	int left_out_w = third_image;
	int right_in_x = src->get_w() - third_image;
	int right_in_w = third_image;
	int right_out_x = x + w - third_image;
	int right_out_w = third_image;
	//int image_x, image_w;

//printf("BC_WindowBase::draw_3segment 1 left_out_x=%d left_out_w=%d center_out_x=%d center_out_w=%d right_out_x=%d right_out_w=%d\n",
//	left_out_x, left_out_w, center_out_x, center_out_w, right_out_x, right_out_w);

	if(left_out_w > half_output)
	{
		left_in_w -= left_out_w - half_output;
		left_out_w -= left_out_w - half_output;
	}

	if(right_out_x < x + half_output)
	{
		right_in_w -= x + half_output - right_out_x;
		right_out_w -= x + half_output - right_out_x;
		right_in_x += x + half_output - right_out_x;
		right_out_x += x + half_output - right_out_x;
	}

//printf("BC_WindowBase::draw_3segment 2 left_out_x=%d left_out_w=%d center_out_x=%d center_out_w=%d right_out_x=%d right_out_w=%d\n",
//	left_out_x, left_out_w, center_out_x, center_out_w, right_out_x, right_out_w);
	if(left_out_w > 0)
	{
		draw_pixmap(src,
			left_out_x,
			y,
			left_out_w,
			src->get_h(),
			left_in_x,
			0,
			dst);
	}

	if(right_out_w > 0)
	{
		draw_pixmap(src,
			right_out_x,
			y,
			right_out_w,
			src->get_h(),
			right_in_x,
			0,
			dst);
	}

	for(int pixel = left_out_x + left_out_w;
		pixel < right_out_x;
		pixel += third_image)
	{
		int fragment_w = third_image;
		if(fragment_w + pixel > right_out_x)
			fragment_w = right_out_x - pixel;

//printf("BC_WindowBase::draw_3segment 2 pixel=%d fragment_w=%d\n", pixel, fragment_w);
		draw_pixmap(src,
			pixel,
			y,
			fragment_w,
			src->get_h(),
			third_image,
			0,
			dst);
	}

}







void BC_WindowBase::draw_3segmentv(int x,
		int y,
		int h,
		VFrame *src,
		BC_Pixmap *dst)
{
	if(h <= 0) return;
	int third_image = src->get_h() / 3;
	int half_output = h / 2;
	//int left_boundary = third_image;
	//int right_boundary = h - third_image;
	int left_in_y = 0;
	int left_in_h = third_image;
	int left_out_y = y;
	int left_out_h = third_image;
	int right_in_y = src->get_h() - third_image;
	int right_in_h = third_image;
	int right_out_y = y + h - third_image;
	int right_out_h = third_image;
	//int image_y, image_h;


	if(left_out_h > half_output)
	{
		left_in_h -= left_out_h - half_output;
		left_out_h -= left_out_h - half_output;
	}

	if(right_out_y < y + half_output)
	{
		right_in_h -= y + half_output - right_out_y;
		right_out_h -= y + half_output - right_out_y;
		right_in_y += y + half_output - right_out_y;
		right_out_y += y + half_output - right_out_y;
	}


	if(!temp_bitmap) temp_bitmap = new BC_Bitmap(top_level,
		src->get_w(),
		src->get_h(),
		get_color_model(),
		0);
	temp_bitmap->match_params(src->get_w(),
		src->get_h(),
		get_color_model(),
		0);
	temp_bitmap->read_frame(src,
		0, 0, src->get_w(), src->get_h(), bg_color);


	if(left_out_h > 0)
	{
		draw_bitmap(temp_bitmap,
			0,
			x,
			left_out_y,
			src->get_w(),
			left_out_h,
			0,
			left_in_y,
			-1,
			-1,
			dst);
	}

	if(right_out_h > 0)
	{
		draw_bitmap(temp_bitmap,
			0,
			x,
			right_out_y,
			src->get_w(),
			right_out_h,
			0,
			right_in_y,
			-1,
			-1,
			dst);
	}

	for(int pixel = left_out_y + left_out_h;
		pixel < right_out_y;
		pixel += third_image)
	{
		int fragment_h = third_image;
		if(fragment_h + pixel > right_out_y)
			fragment_h = right_out_y - pixel;

//printf("BC_WindowBase::draw_3segment 2 pixel=%d fragment_w=%d\n", pixel, fragment_w);
		draw_bitmap(temp_bitmap,
			0,
			x,
			pixel,
			src->get_w(),
			fragment_h,
			0,
			third_image,
			-1,
			-1,
			dst);
	}
}

void BC_WindowBase::draw_3segmentv(int x,
		int y,
		int h,
		BC_Pixmap *src,
		BC_Pixmap *dst)
{
	if(h <= 0) return;
	int third_image = src->get_h() / 3;
	int half_output = h / 2;
	//int left_boundary = third_image;
	//int right_boundary = h - third_image;
	int left_in_y = 0;
	int left_in_h = third_image;
	int left_out_y = y;
	int left_out_h = third_image;
	int right_in_y = src->get_h() - third_image;
	int right_in_h = third_image;
	int right_out_y = y + h - third_image;
	int right_out_h = third_image;
	//int image_y, image_h;


	if(left_out_h > half_output)
	{
		left_in_h -= left_out_h - half_output;
		left_out_h -= left_out_h - half_output;
	}

	if(right_out_y < y + half_output)
	{
		right_in_h -= y + half_output - right_out_y;
		right_out_h -= y + half_output - right_out_y;
		right_in_y += y + half_output - right_out_y;
		right_out_y += y + half_output - right_out_y;
	}

	if(left_out_h > 0)
	{
		draw_pixmap(src,
			x,
			left_out_y,
			src->get_w(),
			left_out_h,
			0,
			left_in_y,
			dst);
	}

	if(right_out_h > 0)
	{
		draw_pixmap(src,
			x,
			right_out_y,
			src->get_w(),
			right_out_h,
			0,
			right_in_y,
			dst);
	}

	for(int pixel = left_out_y + left_out_h;
		pixel < right_out_y;
		pixel += third_image)
	{
		int fragment_h = third_image;
		if(fragment_h + pixel > right_out_y)
			fragment_h = right_out_y - pixel;

//printf("BC_WindowBase::draw_3segment 2 pixel=%d fragment_w=%d\n", pixel, fragment_w);
		draw_pixmap(src,
			x,
			pixel,
			src->get_w(),
			fragment_h,
			0,
			third_image,
			dst);
	}
}


void BC_WindowBase::draw_9segment(int x,
		int y,
		int w,
		int h,
		BC_Pixmap *src,
		BC_Pixmap *dst)
{
	if(w <= 0 || h <= 0) return;

	int in_x_third = src->get_w() / 3;
	int in_y_third = src->get_h() / 3;
	int out_x_half = w / 2;
	int out_y_half = h / 2;

	int in_x1 = 0;
	int in_y1 = 0;
	int out_x1 = 0;
	int out_y1 = 0;
	int in_x2 = MIN(in_x_third, out_x_half);
	int in_y2 = MIN(in_y_third, out_y_half);
	int out_x2 = in_x2;
	int out_y2 = in_y2;

	int out_x3 = MAX(w - out_x_half, w - in_x_third);
	int out_x4 = w;
	int in_x3 = src->get_w() - (out_x4 - out_x3);
	//int in_x4 = src->get_w();

	int out_y3 = MAX(h - out_y_half, h - in_y_third);
	int out_y4 = h;
	int in_y3 = src->get_h() - (out_y4 - out_y3);
	//int in_y4 = src->get_h();

// Segment 1
	draw_pixmap(src,
		x + out_x1,
		y + out_y1,
		out_x2 - out_x1,
		out_y2 - out_y1,
		in_x1,
		in_y1,
		dst);


// Segment 2 * n
	for(int i = out_x2; i < out_x3; i += in_x3 - in_x2)
	{
		if(out_x3 - i > 0)
		{
			int w = MIN(in_x3 - in_x2, out_x3 - i);
			draw_pixmap(src,
				x + i,
				y + out_y1,
				w,
				out_y2 - out_y1,
				in_x2,
				in_y1,
				dst);
		}
	}





// Segment 3
	draw_pixmap(src,
		x + out_x3,
		y + out_y1,
		out_x4 - out_x3,
		out_y2 - out_y1,
		in_x3,
		in_y1,
		dst);



// Segment 4 * n
	for(int i = out_y2; i < out_y3; i += in_y3 - in_y2)
	{
		if(out_y3 - i > 0)
		{
			int h = MIN(in_y3 - in_y2, out_y3 - i);
			draw_pixmap(src,
				x + out_x1,
				y + i,
				out_x2 - out_x1,
				h,
				in_x1,
				in_y2,
				dst);
		}
	}


// Segment 5 * n * n
	for(int i = out_y2; i < out_y3; i += in_y3 - in_y2 /* in_y_third */)
	{
		if(out_y3 - i > 0)
		{
			int h = MIN(in_y3 - in_y2 /* in_y_third */, out_y3 - i);


			for(int j = out_x2; j < out_x3; j += in_x3 - in_x2 /* in_x_third */)
			{
				int w = MIN(in_x3 - in_x2 /* in_x_third */, out_x3 - j);
				if(out_x3 - j > 0)
					draw_pixmap(src,
						x + j,
						y + i,
						w,
						h,
						in_x2,
						in_y2,
						dst);
			}
		}
	}

// Segment 6 * n
	for(int i = out_y2; i < out_y3; i += in_y3 - in_y2)
	{
		if(out_y3 - i > 0)
		{
			int h = MIN(in_y3 - in_y2, out_y3 - i);
			draw_pixmap(src,
				x + out_x3,
				y + i,
				out_x4 - out_x3,
				h,
				in_x3,
				in_y2,
				dst);
		}
	}




// Segment 7
	draw_pixmap(src,
		x + out_x1,
		y + out_y3,
		out_x2 - out_x1,
		out_y4 - out_y3,
		in_x1,
		in_y3,
		dst);


// Segment 8 * n
	for(int i = out_x2; i < out_x3; i += in_x3 - in_x2)
	{
		if(out_x3 - i > 0)
		{
			int w = MIN(in_x3 - in_y2, out_x3 - i);
			draw_pixmap(src,
				x + i,
				y + out_y3,
				w,
				out_y4 - out_y3,
				in_x2,
				in_y3,
				dst);
		}
	}



// Segment 9
	draw_pixmap(src,
		x + out_x3,
		y + out_y3,
		out_x4 - out_x3,
		out_y4 - out_y3,
		in_x3,
		in_y3,
		dst);
}


void BC_WindowBase::draw_9segment(int x, int y, int w, int h,
		VFrame *src, BC_Pixmap *dst)
{
	if(w <= 0 || h <= 0) return;

	int in_x_third = src->get_w() / 3;
	int in_y_third = src->get_h() / 3;
	int out_x_half = w / 2;
	int out_y_half = h / 2;

	int in_x1 = 0;
	int in_y1 = 0;
	int out_x1 = 0;
	int out_y1 = 0;
	int in_x2 = MIN(in_x_third, out_x_half);
	int in_y2 = MIN(in_y_third, out_y_half);
	int out_x2 = in_x2;
	int out_y2 = in_y2;

	int out_x3 = MAX(w - out_x_half, w - in_x_third);
	int out_x4 = w;
	int in_x3 = src->get_w() - (out_x4 - out_x3);
	int in_x4 = src->get_w();

	int out_y3 = MAX(h - out_y_half, h - in_y_third);
	int out_y4 = h;
	int in_y3 = src->get_h() - (out_y4 - out_y3);
	int in_y4 = src->get_h();

//printf("PFCFrame::draw_9segment 1 %d %d %d %d\n", out_x1, out_x2, out_x3, out_x4);
//printf("PFCFrame::draw_9segment 2 %d %d %d %d\n", in_x1, in_x2, in_x3, in_x4);
//printf("PFCFrame::draw_9segment 2 %d %d %d %d\n", in_y1, in_y2, in_y3, in_y4);

	if(!temp_bitmap) temp_bitmap = new BC_Bitmap(top_level,
		src->get_w(),
		src->get_h(),
		get_color_model(),
		0);
	temp_bitmap->match_params(src->get_w(),
		src->get_h(),
		get_color_model(),
		0);
	temp_bitmap->read_frame(src,
		0, 0, src->get_w(), src->get_h(), bg_color);

// Segment 1
	draw_bitmap(temp_bitmap, 0,
		x + out_x1, y + out_y1, out_x2 - out_x1, out_y2 - out_y1,
		in_x1, in_y1, in_x2 - in_x1, in_y2 - in_y1,
		dst);

// Segment 2 * n
	for( int i = out_x2; i < out_x3; i += in_x3 - in_x2 ) {
		if( out_x3 - i > 0 ) {
			int w = MIN(in_x3 - in_x2, out_x3 - i);
			draw_bitmap(temp_bitmap, 0,
				x + i, y + out_y1, w, out_y2 - out_y1,
				in_x2, in_y1, w, in_y2 - in_y1,
				dst);
		}
	}

// Segment 3
	draw_bitmap(temp_bitmap, 0,
		x + out_x3, y + out_y1, out_x4 - out_x3, out_y2 - out_y1,
		in_x3, in_y1, in_x4 - in_x3, in_y2 - in_y1,
		dst);

// Segment 4 * n
	for( int i = out_y2; i < out_y3; i += in_y3 - in_y2 ) {
		if( out_y3 - i > 0 ) {
			int h = MIN(in_y3 - in_y2, out_y3 - i);
			draw_bitmap(temp_bitmap, 0,
				x + out_x1, y + i, out_x2 - out_x1, h,
				in_x1, in_y2, in_x2 - in_x1, h,
				dst);
		}
	}

// Segment 5 * n * n
	for( int i = out_y2; i < out_y3; i += in_y3 - in_y2 ) {
		if( out_y3 - i > 0 ) {
			int h = MIN(in_y3 - in_y2, out_y3 - i);

			for( int j = out_x2; j < out_x3; j += in_x3 - in_x2 ) {
				int w = MIN(in_x3 - in_x2, out_x3 - j);
				if(out_x3 - j > 0)
					draw_bitmap(temp_bitmap, 0,
						x + j, y + i, w, h,
						in_x2, in_y2, w, h,
						dst);
			}
		}
	}

// Segment 6 * n
	for( int i = out_y2; i < out_y3; i += in_y_third ) {
		if( out_y3 - i > 0 ) {
			int h = MIN(in_y_third, out_y3 - i);
			draw_bitmap(temp_bitmap, 0,
				x + out_x3, y + i, out_x4 - out_x3, h,
				in_x3, in_y2, in_x4 - in_x3, h,
				dst);
		}
	}

// Segment 7
	draw_bitmap(temp_bitmap, 0,
		x + out_x1, y + out_y3, out_x2 - out_x1, out_y4 - out_y3,
		in_x1, in_y3, in_x2 - in_x1, in_y4 - in_y3,
		dst);

// Segment 8 * n
	for( int i = out_x2; i < out_x3; i += in_x_third ) {
		if( out_x3 - i > 0 ) {
			int w = MIN(in_x_third, out_x3 - i);
			draw_bitmap(temp_bitmap, 0,
				x + i, y + out_y3, w, out_y4 - out_y3,
				in_x2, in_y3, w, in_y4 - in_y3,
				dst);
		}
	}

// Segment 9
	draw_bitmap(temp_bitmap, 0,
		x + out_x3, y + out_y3, out_x4 - out_x3, out_y4 - out_y3,
		in_x3, in_y3, in_x4 - in_x3, in_y4 - in_y3,
		dst);
}

