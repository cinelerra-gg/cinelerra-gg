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



#ifndef __SKETCHERS_H__
#define __SKETCHERS_H__

#include "pluginvclient.h"

class Sketcher;

enum { PT_ID, PT_X, PT_Y, PT_SZ };
enum { CV_ID, CV_TY, CV_RAD, CV_PEN, CV_CLR, CV_SZ };
enum { TYP_OFF, TYP_SKETCHER, TYP_SMOOTH, TYP_SZ };
enum { PEN_SQUARE, PEN_PLUS, PEN_SLANT, PEN_XLANT, PEN_SZ };

class SketcherVPen : public VFrame
{
public:
	SketcherVPen(VFrame *vfrm, int n)
	 : VFrame(vfrm->get_data(), -1, vfrm->get_y()-vfrm->get_data(),
	    vfrm->get_u()-vfrm->get_data(), vfrm->get_v()-vfrm->get_data(),
	    vfrm->get_w(), vfrm->get_h(), vfrm->get_color_model(),
	    vfrm->get_bytes_per_line()) {
		this->vfrm = vfrm;  this->n = n;
	}
	virtual int draw_pixel(int x, int y) = 0;
	VFrame *vfrm;
	int n;
};

class SketcherPenSquare : public SketcherVPen
{
public:
	SketcherPenSquare(VFrame *vfrm, int n) : SketcherVPen(vfrm, n) {}
	int draw_pixel(int x, int y);
};
class SketcherPenPlus : public SketcherVPen
{
public:
	SketcherPenPlus(VFrame *vfrm, int n) : SketcherVPen(vfrm, n) {}
	int draw_pixel(int x, int y);
};
class SketcherPenSlant : public SketcherVPen
{
public:
	SketcherPenSlant(VFrame *vfrm, int n) : SketcherVPen(vfrm, n) {}
	int draw_pixel(int x, int y);
};
class SketcherPenXlant : public SketcherVPen
{
public:
	SketcherPenXlant(VFrame *vfrm, int n) : SketcherVPen(vfrm, n) {}
	int draw_pixel(int x, int y);
};


class SketcherPoint
{
public:
	int id;
	int x, y;

	void init(int id, int x, int y);
	SketcherPoint(int id, int x, int y);
	SketcherPoint(int id=-1);
	SketcherPoint(SketcherPoint &pt);
	~SketcherPoint();
	int equivalent(SketcherPoint &that);
	void copy_from(SketcherPoint &that);
	void save_data(FileXML &output);
	void read_data(FileXML &input);
};
class SketcherPoints : public ArrayList<SketcherPoint *>
{
public:
	SketcherPoints() {}
	~SketcherPoints() { remove_all_objects(); }
	void dump();
};

#define cv_type SketcherCurve::types
#define cv_pen SketcherCurve::pens

class SketcherCurve
{
public:
	int id, ty, radius, pen, color;
	static const char *types[TYP_SZ];
	static const char *pens[PEN_SZ];

	SketcherPoints points;

	void init(int id, int ty, int radius, int pen, int color);
	SketcherCurve(int id, int ty, int radius, int pen, int color);
	SketcherCurve(int id=-1);
	~SketcherCurve();
	SketcherCurve(SketcherCurve &cv);
	int equivalent(SketcherCurve &that);
	void copy_from(SketcherCurve &that);
	void save_data(FileXML &output);
	void read_data(FileXML &input);
	VFrame *new_vpen(VFrame *out);
	void draw_line(VFrame *out);
	void draw_smooth(VFrame *out);
};
class SketcherCurves : public ArrayList<SketcherCurve *>
{
public:
	SketcherCurves() {}
	~SketcherCurves() { remove_all_objects(); }
	void dump();
};

class SketcherConfig
{
public:
	SketcherConfig();
	~SketcherConfig();

	SketcherCurves curves;
	int equivalent(SketcherConfig &that);
	void copy_from(SketcherConfig &that);
	void interpolate(SketcherConfig &prev, SketcherConfig &next,
		long prev_frame, long next_frame, long current_frame);
	void limits();

	int drag;
	int cv_selected, pt_selected;
};

class Sketcher : public PluginVClient
{
public:
	Sketcher(PluginServer *server);
	~Sketcher();
// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS2(SketcherConfig)
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int new_curve(int ty, int radius, int pen, int color);
	int new_curve();
	int new_point(SketcherCurve *cv, int x, int y);
	int new_point();
	int process_realtime(VFrame *input, VFrame *output);
	static void draw_point(VFrame *vfrm, SketcherPoint *pt, int color, int d);
	void draw_point(VFrame *vfrm, SketcherPoint *pt, int color);

	VFrame *input, *output;
	int w, h, color_model, bpp, comp;
	int is_yuv, is_float;
};

#endif
