/*
 * CINELERRA
 * Copyright (C) 1997-2015 Adam Williams <broadcast at earthling dot net>
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

#include<stdio.h>
#include<stdint.h>
#include<math.h>
#include<string.h>

#include "arraylist.h"
#include "bccmodels.h"
#include "bccolors.h"
#include "clip.h"
#include "edlsession.h"
#include "filexml.h"
#include "overlayframe.h"
#include "pluginserver.h"
#include "preferences.h"
#include "sketcher.h"
#include "sketcherwindow.h"
#include "language.h"
#include "vframe.h"

void SketcherPoint::init(int id, int arc, coord x, coord y)
{
	this->id = id;  this->arc = arc;
	this->x = x;    this->y = y;
}
SketcherPoint::SketcherPoint(int id)
{
	init(id, ARC_LINE, 0, 0);
}
SketcherPoint::SketcherPoint(int id, int arc, coord x, coord y)
{
	init(id, arc, x, y);
}
SketcherPoint::~SketcherPoint()
{
}
SketcherPoint::SketcherPoint(SketcherPoint &pt)
{
	copy_from(pt);
}
int SketcherPoint::equivalent(SketcherPoint &that)
{
	return this->id == that.id &&
		this->arc == that.arc &&
		EQUIV(this->x, that.x) &&
		EQUIV(this->y, that.y) ? 1 : 0;
}
void SketcherPoint::copy_from(SketcherPoint &that)
{
	this->id = that.id;  this->arc = that.arc;
	this->x = that.x;    this->y = that.y;
}
void SketcherPoint::save_data(FileXML &output)
{
	char point[BCSTRLEN];
	sprintf(point,"/POINT_%d",id);
	output.tag.set_title(point+1);
	output.tag.set_property("TYPE", arc);
	output.tag.set_property("X", x);
	output.tag.set_property("Y", y);
	output.append_tag();
	output.tag.set_title(point+0);
	output.append_tag();
	output.append_newline();
}
void SketcherPoint::read_data(FileXML &input)
{
	id = atoi(input.tag.get_title() + 6);
	arc = input.tag.get_property("TYPE", ARC_OFF);
	x = input.tag.get_property("X", (coord)0);
	y = input.tag.get_property("Y", (coord)0);
	bclamp(arc, 0, ARC_SZ-1);
}

void SketcherCurve::init(int id, int pen, int width, int color)
{
	this->id = id;
	this->width = width;
	this->pen = pen;
	this->color = color;
}
SketcherCurve::SketcherCurve(int id)
{
	init(id, 1, ARC_LINE, CV_COLOR);
}
SketcherCurve::SketcherCurve(int id, int pen, int width, int color)
{
	init(id, pen, width, color);
}
SketcherCurve::~SketcherCurve()
{
}
SketcherCurve::SketcherCurve(SketcherCurve &cv)
{
	copy_from(cv);
}
int SketcherCurve::equivalent(SketcherCurve &that)
{
	if( this->id != that.id ) return 0;
	if( this->pen != that.pen ) return 0;
	if( this->width != that.width ) return 0;
	if( this->color != that.color ) return 0;
	int n = this->points.size();
	if( n != that.points.size() ) return 0;
	for( int i=0; i<n; ++i ) {
		if( !points[i]->equivalent(*that.points[i]) ) return 0;
	}
	return 1;
}
void SketcherCurve::copy_from(SketcherCurve &that)
{
	this->id = that.id;
	this->pen = that.pen;
	this->width = that.width;
	this->color = that.color;
	int m = points.size(), n = that.points.size();
	while( m > n ) points.remove_object_number(--m);
	while( m < n ) { points.append(new SketcherPoint());  ++m; }
	for( int i=0; i<n; ++i ) points[i]->copy_from(*that.points[i]);
}
void SketcherCurve::save_data(FileXML &output)
{
	char curve[BCSTRLEN];
	sprintf(curve,"/CURVE_%d",id);
	output.tag.set_title(curve+1);
	output.tag.set_property("PEN", pen);
	output.tag.set_property("RADIUS", width);
	output.tag.set_property("COLOR", color);
	output.append_tag();
	output.append_newline();
	for( int i=0,n=points.size(); i<n; ++i )
		points[i]->save_data(output);
	output.tag.set_title(curve+0);
	output.append_tag();
	output.append_newline();
}
void SketcherCurve::read_data(FileXML &input)
{
	id = atoi(input.tag.get_title() + 6);
	pen = input.tag.get_property("PEN", PEN_OFF);
	width = input.tag.get_property("RADIUS", 1.);
	color = input.tag.get_property("COLOR", CV_COLOR);
	bclamp(pen, 0, PEN_SZ-1);
}

int Sketcher::new_curve(int pen, int width, int color)
{
	SketcherCurves &curves = config.curves;
	int k = curves.size(), id = 1;
	for( int i=k; --i>=0; ) {
		int n = config.curves[i]->id;
		if( n >= id ) id = n + 1;
	}
	SketcherCurve *cv = new SketcherCurve(id, pen, width, color);
	curves.append(cv);
	config.cv_selected = k;
	return k;
}

int Sketcher::new_curve()
{
	return new_curve(PEN_XLANT, 1, CV_COLOR);
}

int Sketcher::new_point(SketcherCurve *cv, int arc, coord x, coord y, int idx)
{
	int id = 1;
	for( int i=cv->points.size(); --i>=0; ) {
		int n = cv->points[i]->id;
		if( n >= id ) id = n + 1;
	}
	SketcherPoint *pt = new SketcherPoint(id, arc, x, y);
	int n = cv->points.size();
	if( idx < 0 || idx > n ) idx = n;
	cv->points.insert(pt, idx);
	return idx;
}

int Sketcher::new_point(int idx, int arc)
{
	int ci = config.cv_selected;
	if( ci < 0 || ci >= config.curves.size() )
		return -1;
	SketcherCurve *cv = config.curves[ci];
	EDLSession *session = get_edlsession();
	coord x = !session ? 0.f : session->output_w / 2.f;
	coord y = !session ? 0.f : session->output_h / 2.f;
	return new_point(cv, arc, x, y, idx);
}

double SketcherCurve::nearest_point(int &pi, coord x, coord y)
{
	pi = -1;
	double dist = DBL_MAX;
	for( int i=0; i<points.size(); ++i ) {
		SketcherPoint *p = points[i];
		double d = DISTANCE(x,y, p->x,p->y);
		if( d < dist ) { dist = d;  pi = i;  }
	}
	return pi >= 0 ? dist : -1.;
}

double SketcherConfig::nearest_point(int &ci, int &pi, coord x, coord y)
{
	double dist = DBL_MAX;
	ci = -1;  pi = -1;
	for( int i=0; i<curves.size(); ++i ) {
		SketcherCurve *crv = curves[i];
		SketcherPoints &points = crv->points;
		for( int k=0; k<points.size(); ++k ) {
			SketcherPoint *p = points[k];
			double d = DISTANCE(x,y, p->x,p->y);
			if( d < dist ) {  dist = d; ci = i;  pi = k; }
		}
	}
	return pi >= 0 ? dist : -1.;
}


REGISTER_PLUGIN(Sketcher)

SketcherConfig::SketcherConfig()
{
	drag = 1;
	cv_selected = 0;
	pt_selected = 0;
}
SketcherConfig::~SketcherConfig()
{
}

int SketcherConfig::equivalent(SketcherConfig &that)
{
	if( this->drag != that.drag ) return 0;
	if( this->cv_selected != that.cv_selected ) return 0;
	if( this->pt_selected != that.pt_selected ) return 0;
	if( this->curves.size() != that.curves.size() ) return 0;
	for( int i=0, n=curves.size(); i<n; ++i ) {
		if( !curves[i]->equivalent(*that.curves[i]) ) return 0;
	}
	return 1;
}

void SketcherConfig::copy_from(SketcherConfig &that)
{
	this->drag = that.drag;
	this->cv_selected = that.cv_selected;
	this->pt_selected = that.pt_selected;
	int m = curves.size(), n = that.curves.size();
	while( m > n ) curves.remove_object_number(--m);
	while( m < n ) { curves.append(new SketcherCurve());  ++m; }
	for( int i=0; i<n; ++i ) curves[i]->copy_from(*that.curves[i]);
}

void SketcherConfig::interpolate(SketcherConfig &prev, SketcherConfig &next,
		long prev_frame, long next_frame, long current_frame)
{
	this->cv_selected = prev.cv_selected;
	this->pt_selected = prev.pt_selected;
	this->drag = prev.drag;

	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	curves.remove_all_objects();
	int prev_cv_sz = prev.curves.size();
	int next_cv_sz = next.curves.size();
	for( int i=0; i<prev_cv_sz; ++i ) {
		SketcherCurve *pcv = prev.curves[i], *ncv = 0;
		SketcherCurve *cv = curves.append(new SketcherCurve());
		int k = next_cv_sz;  // associated by id in next
		while( --k >= 0 && pcv->id != (ncv=next.curves[k])->id );
		if( k >= 0 ) {
			cv->id = pcv->id;
			cv->pen = pcv->pen;
			cv->width = pcv->width == ncv->width ? pcv->width :
				pcv->width*prev_scale + ncv->width*next_scale + 0.5;
			int pr =  (pcv->color>>16)&0xff, nr =  (ncv->color>>16)&0xff;
			int pg =  (pcv->color>> 8)&0xff, ng =  (ncv->color>> 8)&0xff;
			int pb =  (pcv->color>> 0)&0xff, nb =  (ncv->color>> 0)&0xff;
			int pa = (~pcv->color>>24)&0xff, na = (~ncv->color>>24)&0xff;
			int r = pr == nr ? pr : pr*prev_scale + nr*next_scale + 0.5;
			int g = pg == ng ? pg : pg*prev_scale + ng*next_scale + 0.5;
			int b = pb == nb ? pb : pb*prev_scale + nb*next_scale + 0.5;
			int a = pa == na ? pa : pa*prev_scale + na*next_scale + 0.5;
			bclamp(r,0,255); bclamp(g,0,255); bclamp(b,0,255); bclamp(a,0,255);
			cv->color = (~a<<24) | (r<<16) | (g<<8) | (b<<0);
			int prev_pt_sz = pcv->points.size(), next_pt_sz = ncv->points.size();
			for( int j=0; j<prev_pt_sz; ++j ) {
				SketcherPoint &pt = *pcv->points[j], *nt = 0;
				k = next_pt_sz;  // associated by id in next
				while( --k >= 0 && pt.id != (nt=ncv->points[k])->id );
				coord x = pt.x, y = pt.y;
				if( k >= 0 ) {
					if( x != nt->x )
						x = x * prev_scale + nt->x * next_scale;
					if( y != nt->y )
						y = y * prev_scale + nt->y * next_scale;
				}
				cv->points.append(new SketcherPoint(pt.id, pt.arc, x, y));
			}
		}
		else
			cv->copy_from(*pcv);
	}
}

void SketcherConfig::limits()
{
}


Sketcher::Sketcher(PluginServer *server)
 : PluginVClient(server)
{
	img = 0;
	out = 0;
	overlay_frame = 0;
}

Sketcher::~Sketcher()
{
	delete img;
	delete out;
	delete overlay_frame;
}

const char* Sketcher::plugin_title() { return N_("Sketcher"); }
int Sketcher::is_realtime() { return 1; }

NEW_WINDOW_MACRO(Sketcher, SketcherWindow);
LOAD_CONFIGURATION_MACRO(Sketcher, SketcherConfig)

void Sketcher::save_data(KeyFrame *keyframe)
{
	FileXML output;
// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("SKETCHER");
	output.tag.set_property("DRAG", config.drag);
	output.tag.set_property("CV_SELECTED", config.cv_selected);
	output.tag.set_property("PT_SELECTED", config.pt_selected);
	output.append_tag();
	output.append_newline();
	for( int i=0,n=config.curves.size(); i<n; ++i ) {
		config.curves[i]->save_data(output);
	}
	output.tag.set_title("/SKETCHER");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void Sketcher::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	config.curves.remove_all_objects();
	int result = 0;
	SketcherCurve *cv = 0;

	while( !(result=input.read_tag()) ) {
		if( input.tag.title_is("SKETCHER") ) {
			config.drag = input.tag.get_property("DRAG", config.drag);
			config.cv_selected = input.tag.get_property("CV_SELECTED", 0);
			config.pt_selected = input.tag.get_property("PT_SELECTED", 0);
		}
		else if( !strncmp(input.tag.get_title(),"CURVE_",6) ) {
			cv = new SketcherCurve();
			cv->read_data(input);
			config.curves.append(cv);
		}
		else if( !strncmp(input.tag.get_title(),"/CURVE_",7) )
			cv = 0;
		else if( !strncmp(input.tag.get_title(),"POINT_",6) ) {
			if( cv ) {
				SketcherPoint *pt = new SketcherPoint();
				pt->read_data(input);
				cv->points.append(pt);
			}
			else
				printf("Sketcher::read_data: no curve for point\n");
		}
	}

	if( !config.curves.size() )
		new_curve();
	config.limits();
}

void Sketcher::update_gui()
{
	if( !thread ) return;
	thread->window->lock_window("Sketcher::update_gui");
	if( load_configuration() ) {
		SketcherWindow *window = (SketcherWindow*)thread->window;
		window->update_gui();
		window->flush();
	}
	thread->window->unlock_window();
}

void Sketcher::draw_point(VFrame *vfrm, SketcherPoint *pt, int color, int d)
{
	int r = d/2+1, x = pt->x, y = pt->y;
	vfrm->set_pixel_color(color);
	vfrm->draw_smooth(x-r,y+0, x-r, y-r, x+0,y-r);
	vfrm->draw_smooth(x+0,y-r, x+r, y-r, x+r,y+0);
	vfrm->draw_smooth(x+r,y+0, x+r, y+r, x+0,y+r);
	vfrm->draw_smooth(x+0,y+r, x-r, y+r, x-r,y+0);
	vfrm->draw_x(pt->x, pt->y, d);
}
void Sketcher::draw_point(VFrame *vfrm, SketcherPoint *pt, int color)
{
	draw_point(vfrm, pt, color, bmax(w,h)/200 + 2);
}


int SketcherVPen::draw_pixel(int x, int y)
{
	if( x >= 0 && x < vfrm->get_w() &&
	    y >= 0 && y < vfrm->get_h() )
		msk[vfrm->get_w()*y + x] = 0xff;
	return 0;
}

int SketcherPenSquare::draw_pixel(int x, int y)
{
	vfrm->draw_line(x-n, y, x+n, y);
	for( int i=-n; i<n; ++i )
		vfrm->draw_line(x-n, y+i, x+n, y+i);
	return SketcherVPen::draw_pixel(x, y);
}
int SketcherPenPlus::draw_pixel(int x, int y)
{
	if( n > 1 ) {
		vfrm->draw_line(x-n, y, x+n, y);
		vfrm->draw_line(x, y-n, x, y+n);
	}
	else
		vfrm->draw_pixel(x, y);
	return SketcherVPen::draw_pixel(x, y);
}
int SketcherPenSlant::draw_pixel(int x, int y)
{
	vfrm->draw_line(x-n,   y+n,   x+n,   y-n);
	vfrm->draw_line(x-n+1, y+n,   x+n+1, y-n);
	vfrm->draw_line(x-n,   y+n+1, x+n,   y-n+1);
	return SketcherVPen::draw_pixel(x, y);
}
int SketcherPenXlant::draw_pixel(int x, int y)
{
	vfrm->draw_line(x-n,   y+n,   x+n,   y-n);
	vfrm->draw_line(x-n+1, y+n,   x+n+1, y-n);
	vfrm->draw_line(x-n,   y+n+1, x+n,   y-n+1);
	vfrm->draw_line(x-n,   y-n,   x+n,   y+n);
	vfrm->draw_line(x-n+1, y-n,   x+n+1, y+n);
	vfrm->draw_line(x-n,   y-n+1, x+n,   y-n+1);
	return SketcherVPen::draw_pixel(x, y);
}


SketcherVPen *SketcherCurve::new_vpen(VFrame *out)
{
	switch( pen ) {
	case PEN_SQUARE: return new SketcherPenSquare(out, width);
	case PEN_PLUS:   return new SketcherPenPlus(out, width);
	case PEN_SLANT:  return new SketcherPenSlant(out, width);
	case PEN_XLANT:  return new SketcherPenXlant(out, width);
	}
	return 0;
}

static int intersects_at(float &x, float &y,
		float ax,float ay, float bx, float by, float cx,float cy,  // line slope ab thru c
		float dx,float dy, float ex, float ey, float fx,float fy, // line slope de thru f
		float mx=0)
{
	float badx = bx - ax, bady = by - ay;
	float eddx = ex - dx, eddy = ey - dy;
	float d = badx*eddy - bady*eddx;
	int ret = 0;
	if( fabsf(d) < 1 ) { ret = 1;  d = signbit(d) ? -1 : 1; }
	x = (badx*cy*eddx - badx*eddx*fy + badx*eddy*fx - bady*cx*eddx) / d;
	y = (badx*cy*eddy - bady*cx*eddy - bady*eddx*fy + bady*eddy*fx) / d;
	if( mx > 0 ) { bclamp(x, -mx,mx);  bclamp(y, -mx,mx); }
	return ret;
}

static void smooth_axy(float &ax, float &ay,
	float bx, float by, float cx, float cy, float dx, float dy)
{
//middle of bd reflected around ctr
// point ctr = (b+d)/2, dv=c-ctr, a=ctr-dv;
	float xc = (bx+dx)*.5f, yc = (by+dy)*.5f;
	float xd = cx - xc, yd = cy - yc;
	ax = xc - xd;  ay = yc - yd;
}
static void smooth_dxy(float &dx, float &dy,
	float ax, float ay, float bx, float by, float cx, float cy)
{
//middle of ac reflected around ctr
// point ctr = (a+c)/2, dv=c-ctr, d=ctr-dv;
	float xc = (ax+cx)*.5f, yc = (ay+cy)*.5f;
	float xd = bx - xc, yd = by - yc;
	dx = xc - xd;  dy = yc - yd;
}

#if 0
static int convex(float ax,float ay, float bx,float by,
		  float cx,float cy, float dx,float dy)
{
	float abdx = bx-ax, abdy = by-ay;
	float acdx = cx-ax, acdy = cy-ay;
	float bcdx = cx-bx, bcdy = cy-by;
	float bddx = dx-bx, bddy = dy-by;
	float abc = abdx*acdy - abdy*acdx;
	float bcd = bcdx*bddy - bcdy*bddx;
	float v = abc * bcd;
	return !v ? 0 : v>0 ? 1 : -1;
}
#endif


class FillRegion
{
	class segment { public: int y, lt, rt; };
	ArrayList<segment> stack;

	void push(int y, int lt, int rt) {
		segment &seg = stack.append();
		seg.y = y;  seg.lt = lt;  seg.rt = rt;
	}
	void pop(int &y, int &lt, int &rt) {
		segment &seg = stack.last();
		y = seg.y;  lt = seg.lt;  rt = seg.rt;
		stack.remove();
	}
 
	VFrame *img;
	uint8_t *msk;
	int w, h, nxt;
	SketcherPoints &points;
public:
	SketcherPoint *next();
	bool exists() { return stack.size() > 0; }
	void start_at(int x, int y);
	void run();
	FillRegion(SketcherPoints &pts, SketcherVPen *vpen);
	~FillRegion();
};

FillRegion::FillRegion(SketcherPoints &pts, SketcherVPen *vpen)
 : points(pts)
{
	this->img = vpen->vfrm;
	this->msk = vpen->msk;
	this->w = img->get_w();
	this->h = img->get_h();
	nxt = 0;
}
FillRegion::~FillRegion()
{
}

void FillRegion::start_at(int x, int y)
{
	bclamp(x, 0, w-1);
	bclamp(y, 0, h-1);
	push(y, x, x);
}

void FillRegion::run()
{
	while( stack.size() > 0 ) {
		int y, ilt, irt;
		pop(y, ilt, irt);
		int ofs = y*w + ilt;
		for( int x=ilt; x<=irt; ++x,++ofs ) {
			if( msk[ofs] ) continue;
			msk[ofs] = 0xff;
			img->draw_pixel(x, y);
			int lt = x, rt = x;
			int lofs = ofs;
			for( int i=lt; --i>=0; ) {
				if( msk[--lofs] ) break;
				img->draw_pixel(i, y);
				msk[lofs] = 0xff;  lt = i;
			}
			int rofs = ofs;
			for( int i=rt; ++i< w; ) {
				if( msk[++rofs] ) break;
				img->draw_pixel(i, y);
				msk[rofs] = 0xff;  rt = i;
			}
			if( y+1 <  h ) push(y+1, lt, rt);
			if( y-1 >= 0 ) push(y-1, lt, rt);
		}
	}
}

SketcherPoint *FillRegion::next()
{
	while( nxt < points.size() ) {
		SketcherPoint *pt = points[nxt];
		if( pt->arc == ARC_OFF ) continue;
		if( pt->arc != ARC_FILL ) break;
		start_at(pt->x, pt->y);
		++nxt;
	}
	return nxt < points.size() ? points[nxt++] : 0;
}


void SketcherCurve::draw(VFrame *img)
{
	if( !points.size() ) return;
	const float fmx = 16383;
	SketcherVPen *vpen = new_vpen(img);
	FillRegion fill(points, vpen);
	SketcherPoint *pnt0 = fill.next();
	SketcherPoint *pnt1 = pnt0 ? fill.next() : 0;
	SketcherPoint *pnt2 = pnt1 ? fill.next() : 0;
	if( pnt0 && pnt1 && pnt2 ) {
		SketcherPoint *pt0 = pnt0, *pt1 = pnt1, *pt2 = pnt2;
		float ax,ay, bx,by, cx,cy, dx,dy, sx,sy;
		bx = pt0->x;  by = pt0->y;
		cx = pt1->x;  cy = pt1->y;
		dx = pt2->x;  dy = pt2->y;
		smooth_axy(ax,ay, bx,by, cx,cy, dx,dy);
		while( pt2 ) {
			dx = pt2->x;  dy = pt2->y;
			switch( pt0->arc ) {
			case ARC_LINE:
				vpen->draw_line(bx, by, cx, cy);
				break;
			case ARC_CURVE: {
				// s = ac thru b x bd thru c
				intersects_at(sx,sy, ax,ay,cx,cy,bx,by, bx,by,dx,dy,cx,cy,fmx);
				vpen->draw_smooth(bx,by, sx,sy, cx,cy);
				break; }
			}
			ax = bx;  ay = by;  pt0 = pt1;
			bx = cx;  by = cy;  pt1 = pt2;
			cx = dx;  cy = dy;  pt2 = fill.next();
		}
		switch( pt1->arc ) {
		case ARC_LINE:
			vpen->draw_line(bx, by, cx, cy);
			if( fill.exists() ) {
				dx = pnt0->x;  dy = pnt0->y;
				vpen->draw_line(cx,cy, dx,dy);
			}
			break;
		case ARC_CURVE: {
			if( fill.exists() ) {
				dx = pnt0->x;  dy = pnt0->y;
				intersects_at(sx,sy, ax,ay,cx,cy,bx,by, bx,by,dx,dy,cx,cy,fmx);
				vpen->draw_smooth(bx,by, sx,sy, cx,cy);
				ax = bx;  ay = by;
				bx = cx;  by = cy;
				cx = dx;  cy = dy;
				dx = pnt1->x;  dy = pnt1->y;
			}
			else
				smooth_dxy(dx,dy, ax,ay, bx,by, cx,cy);
			intersects_at(sx,sy, ax,ay,cx,cy,bx,by, bx,by,dx,dy,cx,cy,fmx);
			vpen->draw_smooth(bx,by, sx,sy, cx,cy);
			break; }
		}
		fill.run();
	}
	else if( pnt0 && pnt1 ) {
		vpen->draw_line(pnt0->x, pnt0->y, pnt1->x, pnt1->y);
	}
	else if( pnt0 ) {
		vpen->draw_pixel(pnt0->x, pnt0->y);
	}
	delete vpen;
}

int Sketcher::process_realtime(VFrame *input, VFrame *output)
{
	this->input = input;  this->output = output;
	w = output->get_w();  h = output->get_h();

	load_configuration();

	int out_color_model = output->get_color_model();
	int color_model = out_color_model;
	switch( color_model ) { // add alpha if needed
	case BC_RGB888:		color_model = BC_RGBA8888;	break;
	case BC_YUV888:		color_model = BC_YUVA8888;	break;
	case BC_RGB161616:	color_model = BC_RGBA16161616;	break;
	case BC_YUV161616:	color_model = BC_YUVA16161616;	break;
	case BC_RGB_FLOAT:	color_model = BC_RGBA_FLOAT;	break;
	case BC_RGB_FLOATP:	color_model = BC_RGBA_FLOATP;	break;
	}
	if( color_model == out_color_model ) {
		delete out;  out = output;
		if( output != input )
			output->transfer_from(input);
	}
	else {
		VFrame::get_temp(out, w, h, color_model);
		out->transfer_from(input);
	}
	VFrame::get_temp(img, w, h, color_model);
	
	if( !overlay_frame ) {
		int cpus = server->preferences->project_smp;
		int max = (w*h)/0x80000 + 2;
		if( cpus > max ) cpus = max;
		overlay_frame = new OverlayFrame(cpus);
	}

	for( int ci=0, n=config.curves.size(); ci<n; ++ci ) {
		SketcherCurve *cv = config.curves[ci];
		if( cv->pen == PEN_OFF ) continue;
		int m = cv->points.size();
		if( !m ) continue;
		img->clear_frame();
		img->set_pixel_color(cv->color, (~cv->color>>24)&0xff);
		cv->draw(img);
		overlay_frame->overlay(out, img, 0,0,w,h, 0,0,w,h,
				1.f, TRANSFER_NORMAL, NEAREST_NEIGHBOR);
	}

	if( config.drag ) {
		for( int ci=0, n=config.curves.size(); ci<n; ++ci ) {
			SketcherCurve *cv = config.curves[ci];
			for( int pi=0,m=cv->points.size(); pi<m; ++pi ) {
				int color = pi==config.pt_selected && ci==config.cv_selected ?
					RED : cv->color ; 
				draw_point(out, cv->points[pi], color);
			}
		}
	}

	if( output != out )
		output->transfer_from(out);
	else
		out = 0;

	return 0;
}

void SketcherPoints::dump()
{
	for( int i=0; i<size(); ++i ) {
		SketcherPoint *pt = get(i);
		printf("  Pt %d, id=%d, arc=%s, x=%0.1f, y=%0.1f\n",
			i, pt->id, pt_type[pt->arc], pt->x, pt->y);
	}
}
void SketcherCurves::dump()
{
	for( int i=0; i<size(); ++i ) {
		SketcherCurve *cv = get(i);
		printf("Curve %d, id=%d, pen=%s, r=%d, color=%02x%02x%02x%02x, %d points\n",
			i, cv->id, cv_pen[cv->pen], cv->width, (~cv->color>>24)&0xff,
			(cv->color>>16)&0xff, (cv->color>>8)&0xff, (cv->color>>0)&0xff,
			cv->points.size());
		cv->points.dump();
	}
}
void SketcherConfig::dump()
{
	printf("Config drag=%d, cv_selected=%d, pt_selected=%d %d curves\n",
			drag, cv_selected, pt_selected, curves.size());
	curves.dump();
}

