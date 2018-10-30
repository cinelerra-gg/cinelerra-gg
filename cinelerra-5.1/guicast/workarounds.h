
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

#ifndef WORKAROUNDS_H
#define WORKAROUNDS_H

#include <stdint.h>

class Workarounds
{
public:
	Workarounds() {};
	~Workarounds() {};

	static void copy_int(int &a, int &b);
	static void copy_double(double *a, double b);
	static double divide_double(double a, double b);
	static void clamp(int32_t &x, int32_t y, int32_t z);
	static void clamp(int64_t &x, int64_t y, int64_t z);
	static void clamp(float &x, float y, float z);
	static void clamp(double &x, double y, double z);
	static float pow(float x, float y);
};

#ifdef HAVE_XFT
// not thread safe

#include <X11/Xft/Xft.h>
#include "ft2build.h"
#include FT_GLYPH_H
#include FT_BBOX_H
#include FT_OUTLINE_H
#include FT_STROKER_H
#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

Bool xftInit(const char *config);
FcBool xftInitFtLibrary(void);
Bool xftDefaultHasRender(Display *dpy);
Bool xftDefaultSet(Display *dpy, FcPattern *defaults);
FcBool xftCharExists(Display *dpy, XftFont *pub, FcChar32 ucs4);
void xftTextExtents8(Display *dpy, XftFont *pub,
		_Xconst FcChar8 *string, int len, XGlyphInfo *extents);
void xftTextExtentsUtf8(Display *dpy, XftFont *pub,
		_Xconst FcChar8 *string, int len, XGlyphInfo *extents);
void xftTextExtents32(Display *dpy, XftFont *pub,
		_Xconst FcChar32 *string, int len, XGlyphInfo *extents);
XftDraw *xftDrawCreate(Display *dpy, Drawable drawable, Visual *visual,
		Colormap colormap);
XftDraw *xftDrawCreateBitmap(Display *dpy, Pixmap bitmap);
void xftDrawDestroy(XftDraw *draw);
void xftDrawString32(XftDraw *draw, _Xconst XftColor *color, XftFont *pub,
		int x, int y, _Xconst FcChar32 *string, int len);
Bool xftColorAllocValue(Display *dpy, Visual *visual,
		Colormap cmap, _Xconst XRenderColor *color, XftColor *result);
void xftColorFree(Display *dpy, Visual *visual, Colormap cmap, XftColor *color);
XftFont *xftFontOpenName(Display *dpy, int screen, _Xconst char *name);
XftFont *xftFontOpenXlfd(Display *dpy, int screen, _Xconst char *xlfd);
XftFont *xftFontOpenPattern(Display *dpy, FcPattern *pattern);
void xftFontClose(Display *dpy, XftFont *pub);

FT_Error ft_Done_Face(FT_Face face);
FT_Error ft_Done_FreeType(FT_Library library);
void ft_Done_Glyph(FT_Glyph glyph);
FT_UInt ft_Get_Char_Index(FT_Face face, FT_ULong charcode);
FT_Error ft_Get_Glyph(FT_GlyphSlot slot, FT_Glyph *aglyph);
FT_Error ft_Get_Kerning(FT_Face face, FT_UInt left_glyph, FT_UInt right_glyph,
		FT_UInt kern_mode, FT_Vector *akerning);
FT_Error ft_Init_FreeType(FT_Library *alibrary);
FT_Error ft_Load_Char(FT_Face face, FT_ULong char_code, FT_Int32 load_flags);
FT_Error ft_Load_Glyph(FT_Face face, FT_UInt glyph_index, FT_Int32 load_flags);
FT_Error ft_New_Face(FT_Library library, const char *filepathname, FT_Long face_index,
		FT_Face *aface);
FT_Error ft_Outline_Done(FT_Library library, FT_Outline *outline);
FT_Error ft_Outline_Get_BBox(FT_Outline *outline, FT_BBox *abbox);
FT_Error ft_Outline_Get_Bitmap(FT_Library library, FT_Outline *outline,
		const FT_Bitmap *abitmap);
FT_Error ft_Outline_New(FT_Library library, FT_UInt numPoints, FT_Int numContours,
		FT_Outline *anoutline);
void ft_Outline_Translate(const FT_Outline *outline, FT_Pos xOffset, FT_Pos yOffset);
FT_Error ft_Select_Charmap(FT_Face face, FT_Encoding encoding);
FT_Error ft_Set_Pixel_Sizes(FT_Face face, FT_UInt pixel_width, FT_UInt pixel_height);
void ft_Stroker_Done(FT_Stroker stroker);
void ft_Stroker_Export(FT_Stroker stroker, FT_Outline *outline);
FT_Error ft_Stroker_GetCounts(FT_Stroker stroker, FT_UInt *anum_points,
		FT_UInt *anum_contours);
FT_Error ft_Stroker_New(FT_Library library, FT_Stroker *astroker);
FT_Error ft_Stroker_ParseOutline(FT_Stroker stroker, FT_Outline *outline,
		FT_Bool opened);
void ft_Stroker_Set(FT_Stroker stroker, FT_Fixed radius, FT_Stroker_LineCap line_cap,
		FT_Stroker_LineJoin line_join, FT_Fixed miter_limit);

FcBool fcCharSetAddChar(FcCharSet *fcs, FcChar32 ucs4);
FcCharSet *fcCharSetCreate(void);
void fcCharSetDestroy(FcCharSet *fcs);
FcBool fcCharSetHasChar(const FcCharSet *fcs, FcChar32 ucs4);
FcBool fcConfigAppFontAddDir(FcConfig *config, const FcChar8 *dir);
FcConfig *fcConfigGetCurrent();
FcBool fcConfigSetRescanInterval(FcConfig *config, int rescanInterval);
FcFontSet *fcFontList(FcConfig *config, FcPattern *p, FcObjectSet *os);
void fcFontSetDestroy(FcFontSet *s);
FcPattern *fcFreeTypeQueryFace(const FT_Face face, const FcChar8 *file, unsigned int id,
		FcBlanks *blanks);
FcBool fcInit(void);
FcBool fcLangSetAdd(FcLangSet *ls, const FcChar8 *lang);
FcLangSet *fcLangSetCreate(void);
void fcLangSetDestroy(FcLangSet *ls);
FcObjectSet *fcObjectSetBuild(const char *first, ...);
void fcObjectSetDestroy(FcObjectSet *os);
FcBool fcPatternAddBool(FcPattern *p, const char *object, FcBool b);
FcBool fcPatternAddCharSet(FcPattern *p, const char *object, const FcCharSet *c);
FcBool fcPatternAddDouble(FcPattern *p, const char *object, double d);
FcBool fcPatternAddInteger(FcPattern *p, const char *object, int i);
FcBool fcPatternAddLangSet(FcPattern *p, const char *object, const FcLangSet *ls);
FcResult fcPatternGetInteger(const FcPattern *p, const char *object, int n, int *i);
FcResult fcPatternGetDouble(const FcPattern *p, const char *object, int n, double *d);
FcResult fcPatternGetString(const FcPattern *p, const char *object, int n, FcChar8 **s);
FcPattern *fcPatternCreate(void);
FcBool fcPatternDel(FcPattern *p, const char *object);
void fcPatternDestroy(FcPattern *p);
FcPattern *fcPatternDuplicate(const FcPattern *p);
FcResult fcPatternGetCharSet(const FcPattern *p, const char *object, int n,
		FcCharSet **c);
#endif
#endif
