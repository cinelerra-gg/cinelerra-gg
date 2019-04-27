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
#include<float.h>

#include "arraylist.h"
#include "bccmodels.h"
#include "bccolors.h"
#include "clip.h"
#include "edlsession.h"
#include "filexml.h"
#include "tracer.h"
#include "tracerwindow.h"
#include "language.h"
#include "vframe.h"

REGISTER_PLUGIN(Tracer)

void tracer_pgm(const char *fn,VFrame *vfrm)
{
	FILE *fp = fopen(fn,"w");
	int w = vfrm->get_w(), h = vfrm->get_h();
	fprintf(fp,"P5\n%d %d\n255\n",w,h);
	fwrite(vfrm->get_data(),w,h,fp);
	fclose(fp);
}

TracerPoint::TracerPoint(float x, float y)
{
	this->x = x;      this->y = y;
}
TracerPoint::~TracerPoint()
{
}

TracerConfig::TracerConfig()
{
	drag = draw = 1;  fill = 0;
	feather = 0;  radius = 1;
	invert = 0;  selected = -1;
}
TracerConfig::~TracerConfig()
{
}

int TracerConfig::equivalent(TracerConfig &that)
{
	if( this->drag != that.drag ) return 0;
	if( this->draw != that.draw ) return 0;
	if( this->fill != that.fill ) return 0;
	if( this->feather != that.feather ) return 0;
	if( this->invert != that.invert ) return 0;
	if( this->radius != that.radius ) return 0;
	if( this->points.size() != that.points.size() ) return 0;
	for( int i=0, n=points.size(); i<n; ++i ) {
		TracerPoint *ap = this->points[i], *bp = that.points[i];
		if( !EQUIV(ap->x, bp->x) ) return 0;
		if( !EQUIV(ap->y, bp->y) ) return 0;
	}
	return 1;
}

void TracerConfig::copy_from(TracerConfig &that)
{
	this->drag = that.drag;
	this->draw = that.draw;
	this->fill = that.fill;
	this->selected = that.selected;
	this->feather = that.feather;
	this->invert = that.invert;
	this->radius = that.radius;
	points.remove_all_objects();
	for( int i=0,n=that.points.size(); i<n; ++i ) {
		TracerPoint *pt = that.points[i];
		add_point(pt->x, pt->y);
	}
}

void TracerConfig::interpolate(TracerConfig &prev, TracerConfig &next,
		long prev_frame, long next_frame, long current_frame)
{
	copy_from(prev);
}

void TracerConfig::limits()
{
}

int TracerConfig::add_point(float x, float y)
{
	int i = points.size();
	points.append(new TracerPoint(x, y));
	return i;
}

void TracerConfig::del_point(int i)
{
	points.remove_object_number(i);
}


Tracer::Tracer(PluginServer *server)
 : PluginVClient(server)
{
	frm = 0; frm_rows = 0;
	msk = 0; msk_rows = 0;
	edg = 0; edg_rows = 0;
	w = 0;   w1 = w-1;
	h = 0;   h1 = h-1;
	color_model = bpp = 0;
	is_float = is_yuv = 0;
	comps = comp = 0;
	ax = 0;  ay = 0;
	bx = 0;  by = 0;
	cx = 0;  cy = 0;
	ex = 0;  ey = 0;
}

Tracer::~Tracer()
{
	delete edg;
	delete msk;
}

const char* Tracer::plugin_title() { return N_("Tracer"); }
int Tracer::is_realtime() { return 1; }

NEW_WINDOW_MACRO(Tracer, TracerWindow);
int Tracer::load_configuration1()
{
	KeyFrame *prev_keyframe = get_prev_keyframe(get_source_position());
	if( prev_keyframe->position == get_source_position() ) {
		read_data(prev_keyframe);
		return 1;
	}
	return load_configuration();
}
LOAD_CONFIGURATION_MACRO(Tracer, TracerConfig);

int Tracer::new_point()
{
	EDLSession *session = get_edlsession();
	float x = !session ? 0.f : session->output_w / 2.f;
	float y = !session ? 0.f : session->output_h / 2.f;
	return config.add_point(x, y);
}

void Tracer::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("TRACER");
	output.tag.set_property("DRAG", config.drag);
	output.tag.set_property("DRAW", config.draw);
	output.tag.set_property("FILL", config.fill);
	output.tag.set_property("FEATHER", config.feather);
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("INVERT", config.invert);
	output.tag.set_property("SELECTED", config.selected);
	output.append_tag();
	output.append_newline();
	output.tag.set_title("/TRACER");
	output.append_tag();
	output.append_newline();
	for( int i=0, n=config.points.size(); i<n; ++i ) {
		TracerPoint *pt = config.points[i];
		char point[BCSTRLEN];
		sprintf(point,"/POINT_%d",i+1);
		output.tag.set_title(point+1);
		output.tag.set_property("X", pt->x);
		output.tag.set_property("Y", pt->y);
		output.append_tag();
		output.tag.set_title(point+0);
		output.append_tag();
		output.append_newline();
	}
	output.terminate_string();
}

void Tracer::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	config.points.remove_all_objects();
	int result = 0;

	while( !(result=input.read_tag()) ) {
		if( input.tag.title_is("TRACER") ) {
			config.drag = input.tag.get_property("DRAG", config.drag);
			config.draw = input.tag.get_property("DRAW", config.draw);
			config.fill = input.tag.get_property("FILL", config.fill);
			config.feather = input.tag.get_property("FEATHER", config.feather);
			config.radius = input.tag.get_property("RADIUS", config.radius);
			config.invert = input.tag.get_property("INVERT", config.invert);
			config.selected = input.tag.get_property("SELECTED", 0);
			config.limits();
		}
		else if( !strncmp(input.tag.get_title(),"POINT_",6) ) {
			float x = input.tag.get_property("X", 0.f);
			float y = input.tag.get_property("Y", 0.f);
			config.add_point(x, y);
		}
	}
}

void Tracer::update_gui()
{
	if( !thread ) return;
	thread->window->lock_window("Tracer::update_gui");
	TracerWindow *window = (TracerWindow*)thread->window;
	if( load_configuration1() ) {
		window->update_gui();
		window->flush();
	}
	thread->window->unlock_window();
}

void Tracer::draw_point(TracerPoint *pt)
{
	int d = bmax(w,h) / 200 + 2;
	int r = d/2+1, x = pt->x, y = pt->y;
	frm->draw_smooth(x-r,y+0, x-r, y-r, x+0,y-r);
	frm->draw_smooth(x+0,y-r, x+r, y-r, x+r,y+0);
	frm->draw_smooth(x+r,y+0, x+r, y+r, x+0,y+r);
	frm->draw_smooth(x+0,y+r, x-r, y+r, x-r,y+0);
}

void Tracer::draw_points()
{
	for( int i=0, n=config.points.size(); i<n; ++i ) {
		TracerPoint *pt = config.points[i];
		frm->set_pixel_color(config.selected == i ? GREEN : WHITE);
		draw_point(pt);
	}
}
void Tracer::draw_edge()
{
	float scale = 1 / 255.0f;
	int color_model = frm->get_color_model();
	int bpp = BC_CModels::calculate_pixelsize(color_model);
	switch( color_model ) {
	case BC_RGB_FLOAT:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = frm_rows[y], *ep = edg_rows[y];
			for( int x=0; x<w; ++x,++ep,sp+=bpp ) {
				if( !*ep ) continue;
				float a = *ep * scale;
				float *px = (float *)sp;
				px[0] = px[1] = px[2] = a;
			}
		}
		break;
	case BC_RGBA_FLOAT:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = frm_rows[y], *ep = edg_rows[y];
			for( int x=0; x<w; ++x,++ep,sp+=bpp ) {
				if( !*ep ) continue;
				float a = *ep * scale;
				float *px = (float *)sp;
				px[0] = px[1] = px[2] = a;
				px[3] = 1.0f;
			}
		}
		break;
	case BC_RGB888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = frm_rows[y], *ep = edg_rows[y];
			for( int x=0; x<w; ++x,++ep,sp+=bpp ) {
				if( !*ep ) continue;
				float a = *ep * scale;
				uint8_t *px = sp;
				px[0] = px[1] = px[2] = a * 255;
			}
		}
		break;
	case BC_RGBA8888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = frm_rows[y], *ep = edg_rows[y];
			for( int x=0; x<w; ++x,++ep,sp+=bpp ) {
				if( !*ep ) continue;
				float a = *ep * scale;
				uint8_t *px = sp;
				px[0] = px[1] = px[2] = a * 255;
				px[3] = 0xff;
			}
		}
		break;
	case BC_YUV888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = frm_rows[y], *ep = edg_rows[y];
			for( int x=0; x<w; ++x,++ep,sp+=bpp ) {
				if( !*ep ) continue;
				float a = *ep * scale;
				uint8_t *px = sp;
				px[0] = a * 255;
				px[1] = px[2] = 0x80;
			}
		}
		break;
	case BC_YUVA8888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = frm_rows[y], *ep = edg_rows[y];
			for( int x=0; x<w; ++x,++ep,sp+=bpp ) {
				if( !*ep ) continue;
				float a = *ep * scale;
				uint8_t *px = sp;
				px[0] = a * 255;
				px[1] = px[2] = 0x80;
				px[3] = 0xff;
			}
		}
		break;
	}
}


#define PIX_GRADIENT(type, ix, iy) do { \
	int xi = vx+ix, yi = vy+iy; \
	if( edg_rows[yi][xi] ) break; \
	type *px = (type *)(frm_rows[yi] + xi*bpp); \
	float dv = px[0]-xp[0], v = dv*dv; \
	for( int c=1; c<comp; ++c ) { \
		dv = px[c]-xp[c]; v += dv*dv; \
	} \
	v = sqrt(v); \
	if( vmax < v ) vmax = v; \
} while(0)
#define ROW_GRADIENT(type, iy) do { \
	if( vx > 0 ) PIX_GRADIENT(type,-1, iy); \
	if( iy != 0) PIX_GRADIENT(type, 0, iy); \
	if( vx < w1) PIX_GRADIENT(type, 1, iy); \
} while(0)
#define MAX_GRADIENT(type) do { \
	type *xp = (type *)(frm_rows[vy] + vx*bpp); \
	if( vy > 0 ) ROW_GRADIENT(type,-1); \
	ROW_GRADIENT(type, 0); \
	if( vy < h1 ) ROW_GRADIENT(type, 1); \
} while(0)

#define MAX_PIXEL(type, ix, iy) do { \
	int vx = cx + ix, vy = cy + iy; \
	if( edg_rows[vy][vx] ) break; \
	float vv = FLT_MAX; \
	int dx = ex-vx, dy = ey-vy; \
	int rr = dx*dx + dy*dy; \
	if( rr > dd ) break; \
	if( rr > 0 ) { \
		float r = (float)(ix*dx + iy*dy) / rr; \
		float vmax = 0; \
		MAX_GRADIENT(type); \
		vv = r + vmax; \
	} \
	if( maxv < vv ) { \
		maxv = vv; \
		nx = vx;  ny = vy; \
	} \
} while(0)
#define ROW_MAX(type, iy) do { \
	if( cx >  0 ) MAX_PIXEL(type,-1, iy); \
	if( iy != 0 ) MAX_PIXEL(type, 0, iy); \
	if( cx < w1 ) MAX_PIXEL(type, 1, iy); \
} while(0)

int Tracer::step()
{
	int ret = 0;
	if( !edg_rows[cy][cx] ) {
		points.add(cx,cy);
		edg_rows[cy][cx] = 0xff;
	}
	int dx = ex-cx, dy = ey-cy;
	int dd = dx*dx + dy*dy;
	if( !dd ) return ret;
	int nx = cx, ny = cy;
	double maxv = -FLT_MAX;
	if( cy > 0 )  ROW_MAX(uint8_t,-1);
	ROW_MAX(uint8_t, 0);
	if( cy < h1 ) ROW_MAX(uint8_t, 1);
	cx = nx;  cy = ny;
	return maxv > 0 ? 1 : 0;
}

void Tracer::trace(int i0, int i1)
{
	TracerPoint *pt0 = config.points[i0];
	TracerPoint *pt1 = config.points[i1];
	cx = pt0->x;  bclamp(cx, 0, w1);
	cy = pt0->y;  bclamp(cy, 0, h1);
	ex = pt1->x;  bclamp(ex, 0, w1);
	ey = pt1->y;  bclamp(ey, 0, h1);
	while( step() );
}

int Tracer::smooth()
{
	int &n = points.total, m = 0;
	if( n < 3 ) return m;
	TracePoint *ap;
	TracePoint *bp = &points[0];
	TracePoint *cp = &points[1];
	for( ; n>=3; --n ) {
		ap = &points[n-1];
		if( abs(ap->x-cp->x)<2 && abs(ap->y-cp->y)<2 ) {
			edg_rows[bp->y][bp->x] = 0;
			++m;
		}
		else
			break;
	}
	if( n < 3 ) return m;
	ap = &points[0];
	bp = &points[1];
	for( int i=2; i<n; ++i ) {
		cp = &points[i];
		if( abs(ap->x-cp->x)<2 && abs(ap->y-cp->y)<2 &&
		    ( (bp->x==ap->x || bp->x==cp->x) &&
		      (bp->y==ap->y || bp->y==cp->y) ) ) {
			edg_rows[bp->y][bp->x] = 0;
			++m;
		}
		else {
			++ap;  ++bp;
		}
		bp->x = cp->x;  bp->y = cp->y;
	}
	n -= m;
	return m;
}

#if 0
int winding2(int x, int y, TracePoints &pts, int n)
{
	int w = 0;
	int x0 = pts[0].x-x, y0 = pts[0].y-y;
	for( int x1,y1,i=1; i<n; x0=x1,y0=y1,++i ) { 
		x1 = pts[i].x-x;  y1 = pts[i].y-y;
		if( y0*y1 < 0 ) {			   // crosses x axis
			int xi = x0 - y0*(x1-x0)/(y1-y0);  // x-intercept
			if( xi > 0 ) w += y0<0 ? 2 : -2;   // crosses x on plus side
		}
		else if( !y0 && x0 > 0 ) w += y1>0 ? 1 : -1;
		else if( !y1 && x1 > 0 ) w += y0>0 ? 1 : -1;
	}
	return w;
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
 
	int w, h;
	uint8_t *edg;
	uint8_t *msk;
	bool edge_pixel(int i) { return edg[i] > 0; }

public:
	void fill(int x, int y);
	void run();

	FillRegion(VFrame *edg, VFrame *msk);
};

FillRegion::FillRegion(VFrame *edg, VFrame *msk)
{
	this->w = msk->get_w();
	this->h = msk->get_h();
	this->msk = (uint8_t*) msk->get_data();
	this->edg = (uint8_t*) edg->get_data();
}

void FillRegion::fill(int x, int y)
{
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
			if( edge_pixel(ofs) ) continue;
			int lt = x, rt = x;
			int lofs = ofs;
			for( int i=lt; --i>=0; lt=i ) {
				if( msk[--lofs] ) break;
				msk[lofs] = 0xff;
				if( edge_pixel(lofs) ) break;
			}
			int rofs = ofs;
			for( int i=rt; ++i< w; rt=i ) {
				if( msk[++rofs] ) break;
				msk[rofs] = 0xff;
				if( edge_pixel(rofs) ) break;
			}
			if( y+1 <  h ) push(y+1, lt, rt);
			if( y-1 >= 0 ) push(y-1, lt, rt);
		}
	}
}

void Tracer::feather(int r, double s)
{
	if( !r ) return;
	int dir = r < 0 ? (r=-r, -1) : 1;
	int rr = r * r;
	int psf[rr];  // pt spot fn
	float p = powf(10.f, s/2);
	for( int i=0; i<rr; ++i ) {
		float v = powf((float)i/rr,p);
		if( dir < 0 ) v = 1-v;
		int vv = v*256;
		if( vv > 255 ) vv = 255;
		psf[i] = vv;
	}
	for( int i=0,n=points.size(); i<n; ++i ) {
		TracePoint *pt = &points[i];
		int xs = pt->x-r, xn=pt->x+r;
		bclamp(xs, 0, w);
		bclamp(xn, 0, w);
		int ys = pt->y-r, yn=pt->y+r;
		bclamp(ys, 0, h);
		bclamp(yn, 0, h);
		for( int y=ys ; y<yn; ++y ) {
			for( int x=xs; x<xn; ++x ) {
				int dx = x-pt->x, dy = y-pt->y;
				int dd = dx*dx + dy*dy;
				if( dd >= rr ) continue;
				int v = psf[dd], px = msk_rows[y][x];
				if( dir < 0 ? px<v : px>v ) msk_rows[y][x] = v;
			}
		}
	}
}

void Tracer::draw_mask()
{
	switch( color_model ) {
	case BC_RGB888:
		for( int y=0; y<h; ++y ) {
			uint8_t *mp = msk_rows[y];
			uint8_t *rp = frm_rows[y];
			for( int x=0; x<w; rp+=bpp,++x ) {
				int a = !config.invert ? 0xff-mp[x] : mp[x];
				rp[0] = a*rp[0] / 0xff;
				rp[1] = a*rp[1] / 0xff;
				rp[2] = a*rp[2] / 0xff;
			}
		}
		break;
	case BC_YUV888:
		for( int y=0; y<h; ++y ) {
			uint8_t *mp = msk_rows[y];
			uint8_t *rp = frm_rows[y];
			for( int x=0; x<w; rp+=bpp,++x ) {
				int a = !config.invert ? 0xff-mp[x] : mp[x];
				rp[0] = a*rp[0] / 0xff;
				rp[1] = a*(rp[1]-0x80)/0xff + 0x80;
				rp[2] = a*(rp[2]-0x80)/0xff + 0x80;
			}
		}
		break;
	case BC_RGB_FLOAT:
		for( int y=0; y<h; ++y ) {
			uint8_t *mp = msk_rows[y];
			uint8_t *rp = frm_rows[y];
			for( int x=0; x<w; rp+=bpp,++x ) {
				float a = !config.invert ? 1-mp[x]/255.f : mp[x]/255.f;
				float *fp = (float*)rp;
				fp[0] *= a;  fp[1] *= a;  fp[2] *= a;
			}
		}
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		for( int y=0; y<h; ++y ) {
			uint8_t *mp = msk_rows[y];
			uint8_t *rp = frm_rows[y];
			for( int x=0; x<w; rp+=bpp,++x ) {
				rp[3] = !config.invert ? 0xff-mp[x] : mp[x];
			}
		}
		break;
	case BC_RGBA_FLOAT:
		for( int y=0; y<h; ++y ) {
			uint8_t *mp = msk_rows[y];
			uint8_t *rp = frm_rows[y];
			for( int x=0; x<w; rp+=bpp,++x ) {
				float *fp = (float*)rp;
				fp[3] = !config.invert ? 1-mp[x]/255.f : mp[x]/255.f;
			}
		}
		break;
	}
}

int Tracer::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
{
	int redraw = load_configuration1();
	frm = frame;
	frm_rows = frm->get_rows();
	w = frm->get_w();  w1 = w-1;
	h = frm->get_h();  h1 = h-1;
	color_model = frm->get_color_model();
	bpp = BC_CModels::calculate_pixelsize(color_model);
	is_float = BC_CModels::is_float(color_model);
	is_yuv = BC_CModels::is_yuv(color_model);
	has_alpha = BC_CModels::has_alpha(color_model);
	comps = BC_CModels::components(color_model);
	comp = bmin(comps, 3);
	read_frame(frm, 0, start_position, frame_rate, 0);
	if( !edg ) redraw = 1;
	VFrame::get_temp(edg, w, h, BC_GREY8);
	if( redraw ) {
		edg->clear_frame();
		edg_rows = edg->get_rows();

		int n = config.points.size()-1;
		points.clear();
		for( int i=0; i<n; ++i )
			trace(i,i+1);
		if( n > 0 )
			trace(n,0);
		while( smooth() > 0 );

		if( config.fill && points.size() > 2 ) {
			int l = points.size(), l2 = l/2;
			TracePoint *pt0 = &points[0], *pt1 = &points[l2];
			int cx = (pt0->x+pt1->x)/2, cy = (pt0->y+pt1->y)/2;
			VFrame::get_temp(msk, w, h, BC_GREY8);
			msk->clear_frame();
			msk_rows = msk->get_rows();

			FillRegion fill_region(edg, msk);
			fill_region.fill(cx, cy);
			fill_region.run();

			feather(config.feather, config.radius);
		}
	}
	if( config.fill && msk )
		draw_mask();
	if( config.draw && edg )
		draw_edge();
	if( config.drag )
		draw_points();
	return 0;
}

