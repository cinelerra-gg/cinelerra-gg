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
#include "overlayframe.inc"
#include "vframe.h"

class Sketcher;

#define pt_type SketcherPoint::types
#define cv_pen SketcherCurve::pens
#define CV_COLOR WHITE

enum { PT_ID, PT_TY, PT_X, PT_Y, PT_SZ };
enum { CV_ID, CV_RAD, CV_PEN, CV_CLR, CV_ALP, CV_SZ };
enum { ARC_OFF, ARC_LINE, ARC_CURVE, ARC_FILL, ARC_SZ };
enum { PEN_OFF, PEN_SQUARE, PEN_PLUS, PEN_SLANT, PEN_XLANT, PEN_SZ };
typedef float coord;

class SketcherVPen : public VFrame
{
public:
	VFrame *vfrm;
	int n;
	uint8_t *msk;

	SketcherVPen(VFrame *vfrm, int n)
	 : VFrame(vfrm->get_data(), -1, vfrm->get_y()-vfrm->get_data(),
	    vfrm->get_u()-vfrm->get_data(), vfrm->get_v()-vfrm->get_data(),
	    vfrm->get_w(), vfrm->get_h(), vfrm->get_color_model(),
	    vfrm->get_bytes_per_line()) {
		this->vfrm = vfrm;  this->n = n;
		int sz = vfrm->get_w()*vfrm->get_h();
		this->msk = (uint8_t*)memset(new uint8_t[sz],0,sz);
	}
	~SketcherVPen() { delete [] msk; }

	void draw_line(float x1, float y1, float x2, float y2) {
		VFrame::draw_line(int(x1+.5f),int(y1+.5f), int(x2+.5f),int(y2+.5f));
	}
	void draw_smooth(float x1, float y1, float x2, float y2, float x3, float y3) {
		VFrame::draw_smooth(int(x1+.5f),int(y1+.5f),
			int(x2+.5f),int(y2+.5f), int(x3+.5f),int(y3+.5f));
	}

	virtual int draw_pixel(int x, int y) = 0;
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
	int id, arc;
	coord x, y;

	void init(int id, int arc, coord x, coord y);
	SketcherPoint(int id, int arc, coord x, coord y);
	SketcherPoint(int id=-1);
	SketcherPoint(SketcherPoint &pt);
	~SketcherPoint();
	int equivalent(SketcherPoint &that);
	void copy_from(SketcherPoint &that);
	void save_data(FileXML &output);
	void read_data(FileXML &input);
	static const char *types[ARC_SZ];
};
class SketcherPoints : public ArrayList<SketcherPoint *>
{
public:
	SketcherPoints() {}
	~SketcherPoints() { remove_all_objects(); }
	void dump();
};


class SketcherCurve
{
public:
	int id, pen, width, color;
	static const char *pens[PEN_SZ];

	SketcherPoints points;

	void init(int id, int pen, int width, int color);
	SketcherCurve(int id, int pen, int width, int color);
	SketcherCurve(int id=-1);
	~SketcherCurve();
	SketcherCurve(SketcherCurve &cv);
	int equivalent(SketcherCurve &that);
	void copy_from(SketcherCurve &that);
	void save_data(FileXML &output);
	void read_data(FileXML &input);
	double nearest_point(int &pi, coord x, coord y);

	SketcherVPen *new_vpen(VFrame *out);
	void draw(VFrame *img);
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
	double nearest_point(int &ci, int &pi, coord x, coord y);
	void limits();
	void dump();

	int drag;
	int cv_selected, pt_selected;
};

class Sketcher : public PluginVClient
{
public:
	Sketcher(PluginServer *server);
	~Sketcher();
	PLUGIN_CLASS_MEMBERS2(SketcherConfig)
	int is_realtime();
	int is_synthesis();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int new_curve(int pen, int width, int color);
	int new_curve();
	int new_point(SketcherCurve *cv, int arc, coord x, coord y, int idx=-1);
	int new_point(int idx, int arc);
	int process_realtime(VFrame *input, VFrame *output);
	static void draw_point(VFrame *vfrm, SketcherPoint *pt, int color, int d);
	void draw_point(VFrame *vfrm, SketcherPoint *pt, int color);

	VFrame *input, *output;
	VFrame *img, *out;
	OverlayFrame *overlay_frame;
	int w, h, color_model, bpp, comp;
	int is_yuv, is_float;
};

#endif
