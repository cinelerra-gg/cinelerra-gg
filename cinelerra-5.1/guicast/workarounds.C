
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

#include "clip.h"
#include "mutex.h"
#include <math.h>
#include "workarounds.h"

void Workarounds::copy_int(int &a, int &b)
{
	a = b;
}

double Workarounds::divide_double(double a, double b)
{
	return a / b;
}

void Workarounds::copy_double(double *a, double b)
{
	*a = b;
}


void Workarounds::clamp(int32_t &x, int32_t y, int32_t z)
{
	if(x < y) x = y;
	else
	if(x > z) x = z;
}

void Workarounds::clamp(int64_t &x, int64_t y, int64_t z)
{
	if(x < y) x = y;
	else
	if(x > z) x = z;
}

void Workarounds::clamp(float &x, float y, float z)
{
	if(x < y) x = y;
	else
	if(x > z) x = z;
}

void Workarounds::clamp(double &x, double y, double z)
{
	if(x < y) x = y;
	else
	if(x > z) x = z;
}

float Workarounds::pow(float x, float y)
{
	return powf(x, y);
}

#ifdef HAVE_XFT
// not thread safe
static Mutex xft_lock("xft_lock");

Bool xftInit(const char *config)
{
	xft_lock.lock("XftInit");
	Bool ret = XftInit(config);
	xft_lock.unlock();
	return ret;
}

FcBool xftInitFtLibrary(void)
{
	xft_lock.lock("xftInitFtLibrary");
	FcBool ret = XftInitFtLibrary();
	xft_lock.unlock();
	return ret;
}

Bool xftDefaultHasRender(Display *dpy)
{
	xft_lock.lock("xftDefaultHasRender");
	Bool ret = XftDefaultHasRender(dpy);
	xft_lock.unlock();
	return ret;
}

Bool xftDefaultSet(Display *dpy, FcPattern *defaults)
{
	xft_lock.lock("xftDefaultHasRender");
	Bool ret = XftDefaultSet(dpy, defaults);
	xft_lock.unlock();
	return ret;
}

FcBool xftCharExists(Display *dpy, XftFont *pub, FcChar32 ucs4)
{
	xft_lock.lock("xftCharExists");
	FcBool ret = XftCharExists(dpy, pub, ucs4);
	xft_lock.unlock();
	return ret;
}

void xftTextExtents8(Display *dpy, XftFont *pub,
		_Xconst FcChar8 *string, int len, XGlyphInfo *extents)
{
	xft_lock.lock("xftTextExtents8");
	XftTextExtents8(dpy, pub, string, len, extents);
	xft_lock.unlock();
}

void xftTextExtentsUtf8(Display *dpy, XftFont *pub,
		_Xconst FcChar8 *string, int len, XGlyphInfo *extents)
{
	xft_lock.lock("xftTextExtentsUtf8");
	XftTextExtentsUtf8(dpy, pub, string, len, extents);
	xft_lock.unlock();
}

void xftTextExtents32(Display *dpy, XftFont *pub,
		_Xconst FcChar32 *string, int len, XGlyphInfo *extents)
{
	xft_lock.lock("xftTextExtents32");
	XftTextExtents32(dpy, pub, string, len, extents);
	xft_lock.unlock();
}

XftDraw *xftDrawCreate(Display *dpy, Drawable drawable, Visual *visual, Colormap colormap)
{
	xft_lock.lock("xftDrawCreate");
	XftDraw *ret = XftDrawCreate(dpy, drawable, visual, colormap);
	xft_lock.unlock();
	return ret;
}

XftDraw *xftDrawCreateBitmap(Display  *dpy, Pixmap bitmap)
{
	xft_lock.lock("xftDrawCreateBitmap");
	XftDraw *ret = XftDrawCreateBitmap(dpy, bitmap);
	xft_lock.unlock();
	return ret;
}

void xftDrawDestroy(XftDraw *draw)
{
	xft_lock.lock("xftDrawDestroy");
	XftDrawDestroy(draw);
	xft_lock.unlock();
}

void xftDrawString32(XftDraw *draw, _Xconst XftColor *color, XftFont *pub,
		int x, int y, _Xconst FcChar32 *string, int len)
{
	xft_lock.lock("xftDrawString32");
	XftDrawString32(draw, color, pub, x, y, string, len);
	xft_lock.unlock();
}

Bool xftColorAllocValue(Display *dpy, Visual *visual,
		Colormap cmap, _Xconst XRenderColor *color, XftColor *result)
{
	xft_lock.lock("xftColorAllocValue");
	Bool ret = XftColorAllocValue(dpy, visual, cmap, color, result);
	xft_lock.unlock();
	return ret;
}

void xftColorFree(Display *dpy, Visual *visual, Colormap cmap, XftColor *color)
{
	xft_lock.lock("xftColorFree");
	XftColorFree(dpy, visual, cmap, color);
	xft_lock.unlock();
}

XftFont *xftFontOpenName(Display *dpy, int screen, _Xconst char *name)
{
	xft_lock.lock("xftFontOpenName");
	XftFont *ret = XftFontOpenName(dpy, screen, name);
	xft_lock.unlock();
	return ret;
}

XftFont *xftFontOpenXlfd(Display *dpy, int screen, _Xconst char *xlfd)
{
	xft_lock.lock("xftFontOpenXlfd");
	XftFont *ret = XftFontOpenXlfd(dpy, screen, xlfd);
	xft_lock.unlock();
	return ret;
}

XftFont *xftFontOpenPattern(Display *dpy, FcPattern *pattern)
{
	xft_lock.lock("xftFontOpenPattern");
	XftFont *ret = XftFontOpenPattern(dpy, pattern);
	xft_lock.unlock();
	return ret;
}

void xftFontClose(Display *dpy, XftFont *pub)
{
	xft_lock.lock("xftFontClose");
	XftFontClose(dpy, pub);
	xft_lock.unlock();
}


static Mutex &ft_lock = xft_lock;

FT_Error ft_Done_Face(FT_Face face)
{
	ft_lock.lock("ft_Done_Face");
	FT_Error ret = FT_Done_Face(face);
	ft_lock.unlock();
	return ret;
}

FT_Error ft_Done_FreeType(FT_Library library)
{
	ft_lock.lock("ft_Done_FreeType");
	FT_Error ret = FT_Done_FreeType(library);
	ft_lock.unlock();
	return ret;
}

void ft_Done_Glyph(FT_Glyph glyph)
{
	ft_lock.lock("ft_Done_Glyph");
	FT_Done_Glyph(glyph);
	ft_lock.unlock();
}

FT_UInt ft_Get_Char_Index(FT_Face face, FT_ULong charcode)
{
	ft_lock.lock("ft_Get_Char_Index");
	FT_UInt ret = FT_Get_Char_Index(face, charcode);
	ft_lock.unlock();
	return ret;
}

FT_Error ft_Get_Glyph(FT_GlyphSlot  slot, FT_Glyph *aglyph)
{
	ft_lock.lock("ft_Get_Glyph");
	FT_Error ret = FT_Get_Glyph(slot, aglyph);
	ft_lock.unlock();
	return ret;
}

FT_Error ft_Get_Kerning(FT_Face face, FT_UInt left_glyph, FT_UInt right_glyph, FT_UInt kern_mode, FT_Vector *akerning)
{
	ft_lock.lock("ft_Get_Kerning");
	FT_Error ret = FT_Get_Kerning(face, left_glyph, right_glyph, kern_mode, akerning);
	ft_lock.unlock();
	return ret;
}

FT_Error ft_Init_FreeType(FT_Library *alibrary)
{
	ft_lock.lock("ft_Init_FreeType");
	FT_Error ret = FT_Init_FreeType(alibrary);
	ft_lock.unlock();
	return ret;
}

FT_Error ft_Load_Char(FT_Face face, FT_ULong char_code, FT_Int32 load_flags)
{
	ft_lock.lock("ft_Load_Char");
	FT_Error ret = FT_Load_Char(face, char_code, load_flags);
	ft_lock.unlock();
	return ret;
}

FT_Error ft_Load_Glyph(FT_Face face, FT_UInt glyph_index, FT_Int32 load_flags)
{
	ft_lock.lock("ft_Load_Glyph");
	FT_Error ret = FT_Load_Glyph(face, glyph_index, load_flags);
	ft_lock.unlock();
	return ret;
}

FT_Error ft_New_Face(FT_Library library, const char *filepathname, FT_Long face_index, FT_Face *aface)
{
	ft_lock.lock("ft_New_Face");
	FT_Error ret = FT_New_Face(library, filepathname, face_index, aface);
	ft_lock.unlock();
	return ret;
}

FT_Error ft_Outline_Done(FT_Library library, FT_Outline *outline)
{
	ft_lock.lock("ft_Outline_Done");
	FT_Error ret = FT_Outline_Done(library, outline);
	ft_lock.unlock();
	return ret;
}

FT_Error ft_Outline_Get_BBox(FT_Outline *outline, FT_BBox *abbox)
{
	ft_lock.lock("ft_Outline_Get_BBox");
	FT_Error ret = FT_Outline_Get_BBox(outline, abbox);
	ft_lock.unlock();
	return ret;
}

FT_Error ft_Outline_Get_Bitmap(FT_Library library, FT_Outline *outline, const FT_Bitmap  *abitmap)
{
	ft_lock.lock("ft_Outline_Get_Bitmap");
	FT_Error ret = FT_Outline_Get_Bitmap(library, outline, abitmap);
	ft_lock.unlock();
	return ret;
}

FT_Error ft_Outline_New(FT_Library library, FT_UInt numPoints, FT_Int numContours, FT_Outline *anoutline)
{
	ft_lock.lock("ft_Outline_New");
	FT_Error ret = FT_Outline_New(library, numPoints, numContours, anoutline);
	ft_lock.unlock();
	return ret;
}

void ft_Outline_Translate(const FT_Outline *outline, FT_Pos xOffset, FT_Pos yOffset)
{
	ft_lock.lock("ft_Outline_Translate");
	FT_Outline_Translate(outline, xOffset, yOffset);
	ft_lock.unlock();
}

FT_Error ft_Select_Charmap(FT_Face face, FT_Encoding encoding)
{
	ft_lock.lock("ft_Select_Charmap");
	FT_Error ret = FT_Select_Charmap(face, encoding);
	ft_lock.unlock();
	return ret;
}
FT_Error ft_Set_Pixel_Sizes(FT_Face face, FT_UInt pixel_width, FT_UInt pixel_height)
{
	ft_lock.lock("ft_Set_Pixel_Sizes");
	FT_Error ret = FT_Set_Pixel_Sizes(face, pixel_width, pixel_height);
	ft_lock.unlock();
	return ret;
}

void ft_Stroker_Done(FT_Stroker stroker)
{
	ft_lock.lock("ft_Stroker_Done");
	FT_Stroker_Done(stroker);
	ft_lock.unlock();
}

void ft_Stroker_Export(FT_Stroker stroker, FT_Outline *outline)
{
	ft_lock.lock("ft_Stroker_Export");
	FT_Stroker_Export(stroker, outline);
	ft_lock.unlock();
}

FT_Error ft_Stroker_GetCounts(FT_Stroker stroker, FT_UInt *anum_points, FT_UInt *anum_contours)
{
	ft_lock.lock("ft_Stroker_GetCounts");
	FT_Error ret = FT_Stroker_GetCounts(stroker, anum_points, anum_contours);
	ft_lock.unlock();
	return ret;
}

FT_Error ft_Stroker_New(FT_Library library, FT_Stroker  *astroker)
{
	ft_lock.lock("ft_Stroker_New");
	FT_Error ret = FT_Stroker_New(library, astroker);
	ft_lock.unlock();
	return ret;
}

FT_Error ft_Stroker_ParseOutline(FT_Stroker stroker, FT_Outline *outline, FT_Bool opened)
{
	ft_lock.lock("ft_Stroker_ParseOutline");
	FT_Error ret = FT_Stroker_ParseOutline(stroker, outline, opened);
	ft_lock.unlock();
	return ret;
}

void ft_Stroker_Set(FT_Stroker stroker, FT_Fixed radius, FT_Stroker_LineCap line_cap, FT_Stroker_LineJoin line_join, FT_Fixed miter_limit)
{
	ft_lock.lock("ft_Stroker_Set");
	FT_Stroker_Set(stroker, radius, line_cap, line_join, miter_limit);
	ft_lock.unlock();
}


static Mutex &fc_lock = xft_lock;

FcBool fcCharSetAddChar(FcCharSet *fcs, FcChar32 ucs4)
{
	fc_lock.lock("fcCharSetAddChar");
	FcBool ret = FcCharSetAddChar(fcs, ucs4);
	fc_lock.unlock();
	return ret;
}

FcCharSet *fcCharSetCreate(void)
{
	fc_lock.lock("fcCharSetCreate");
	FcCharSet *ret = FcCharSetCreate();
	fc_lock.unlock();
	return ret;
}
void fcCharSetDestroy(FcCharSet *fcs)
{
	fc_lock.lock("fcCharSetDestroy");
	FcCharSetDestroy(fcs);
	fc_lock.unlock();
}

FcBool fcCharSetHasChar(const FcCharSet *fcs, FcChar32 ucs4)
{
	fc_lock.lock("fcCharSetHasChar");
	FcBool ret = FcCharSetHasChar(fcs, ucs4);
	fc_lock.unlock();
	return ret;
}

FcBool fcConfigAppFontAddDir(FcConfig *config, const FcChar8 *dir)
{
	fc_lock.lock("fcConfigAppFontAddDir");
	FcBool ret = FcConfigAppFontAddDir(config, dir);
	fc_lock.unlock();
	return ret;
}

FcConfig *fcConfigGetCurrent()
{
	fc_lock.lock("fcConfigGetCurrent");
	FcConfig *ret = FcConfigGetCurrent();
	fc_lock.unlock();
	return ret;
}

FcBool fcConfigSetRescanInterval(FcConfig *config, int rescanInterval)
{
	fc_lock.lock("fcConfigSetRescanInterval");
	FcBool ret = FcConfigSetRescanInterval(config, rescanInterval);
	fc_lock.unlock();
	return ret;
}

FcFontSet *fcFontList(FcConfig *config, FcPattern *p, FcObjectSet *os)
{
	fc_lock.lock("fcFontList");
	FcFontSet *ret = FcFontList(config, p, os);
	fc_lock.unlock();
	return ret;
}

void fcFontSetDestroy(FcFontSet *s)
{
	fc_lock.lock("fcFontSetDestroy");
	FcFontSetDestroy(s);
	fc_lock.unlock();
}

FcPattern *fcFreeTypeQueryFace(const FT_Face face, const FcChar8 *file, unsigned int id, FcBlanks *blanks)
{
	fc_lock.lock("fcFreeTypeQueryFace");
	FcPattern *ret = FcFreeTypeQueryFace(face, file, id, blanks);
	fc_lock.unlock();
	return ret;
}

FcBool fcInit(void)
{
	fc_lock.lock("fcInit");
	FcBool ret = FcInit();
	fc_lock.unlock();
	return ret;
}

FcBool fcLangSetAdd(FcLangSet *ls, const FcChar8 *lang)
{
	fc_lock.lock("fcLangSetAdd");
	FcBool ret = FcLangSetAdd(ls, lang);
	fc_lock.unlock();
	return ret;
}

FcLangSet *fcLangSetCreate(void)
{
	fc_lock.lock("fcLangSetCreate");
	FcLangSet *ret = FcLangSetCreate();
	fc_lock.unlock();
	return ret;
}

void fcLangSetDestroy(FcLangSet *ls)
{
	fc_lock.lock("fcLangSetDestroy");
	FcLangSetDestroy(ls);
	fc_lock.unlock();
}

FcObjectSet *fcObjectSetBuild(const char *first, ...)
{
	fc_lock.lock("fcObjectSetBuild");
	va_list va;  va_start(va, first);
	FcObjectSet *ret = FcObjectSetVaBuild(first, va);
	va_end(va);
	fc_lock.unlock();
	return ret;
}

void fcObjectSetDestroy(FcObjectSet *os)
{
	fc_lock.lock("fcObjectSetDestroy");
	FcObjectSetDestroy(os);
	fc_lock.unlock();
}

FcBool fcPatternAddBool(FcPattern *p, const char *object, FcBool b)
{
	fc_lock.lock("fcPatternAddBool");
	FcBool ret = FcPatternAddBool(p, object, b);
	fc_lock.unlock();
	return ret;
}

FcBool fcPatternAddCharSet(FcPattern *p, const char *object, const FcCharSet *c)
{
	fc_lock.lock("fcPatternAddCharSet");
	FcBool ret = FcPatternAddCharSet(p, object, c);
	fc_lock.unlock();
	return ret;
}

FcBool fcPatternAddDouble(FcPattern *p, const char *object, double d)
{
	fc_lock.lock("fcPatternAddDouble");
	FcBool ret = FcPatternAddDouble(p, object, d);
	fc_lock.unlock();
	return ret;
}

FcBool fcPatternAddInteger(FcPattern *p, const char *object, int i)
{
	fc_lock.lock("fcPatternAddInteger");
	FcBool ret = FcPatternAddInteger(p, object, i);
	fc_lock.unlock();
	return ret;
}

FcBool fcPatternAddLangSet(FcPattern *p, const char *object, const FcLangSet *ls)
{
	fc_lock.lock("fcPatternAddLangSet");
	FcBool ret = FcPatternAddLangSet(p, object, ls);
	fc_lock.unlock();
	return ret;
}

FcResult fcPatternGetInteger(const FcPattern *p, const char *object, int n, int *i)
{
	fc_lock.lock("fcPatternGetInteger");
	FcResult ret = FcPatternGetInteger(p, object, n, i);
	fc_lock.unlock();
	return ret;
}

FcResult fcPatternGetDouble (const FcPattern *p, const char *object, int n, double *d)
{
	fc_lock.lock("fcPatternGetDouble");
	FcResult ret = FcPatternGetDouble(p, object, n, d);
	fc_lock.unlock();
	return ret;
}

FcResult fcPatternGetString(const FcPattern *p, const char *object, int n, FcChar8 **s)
{
	fc_lock.lock("fcPatternGetString");
	FcResult ret = FcPatternGetString(p, object, n, s);
	fc_lock.unlock();
	return ret;
}

FcPattern *fcPatternCreate(void)
{
	fc_lock.lock("fcPatternCreate");
	FcPattern *ret = FcPatternCreate();
	fc_lock.unlock();
	return ret;
}

FcBool fcPatternDel(FcPattern *p, const char *object)
{
	fc_lock.lock("fcPatternDel");
	FcBool ret = FcPatternDel(p, object);
	fc_lock.unlock();
	return ret;
}

void fcPatternDestroy(FcPattern *p)
{
	fc_lock.lock("fcPatternDestroy ");
	FcPatternDestroy(p);
	fc_lock.unlock();
}

FcPattern *fcPatternDuplicate(const FcPattern *p)
{
	fc_lock.lock("fcPatternDuplicate");
	FcPattern *ret = FcPatternDuplicate(p);
	fc_lock.unlock();
	return ret;
}

FcResult fcPatternGetCharSet(const FcPattern *p, const char *object, int n, FcCharSet **c)
{
	fc_lock.lock("fcPatternGetCharSet");
	FcResult ret = FcPatternGetCharSet(p, object, n, c);
	fc_lock.unlock();
	return ret;
}

#endif
