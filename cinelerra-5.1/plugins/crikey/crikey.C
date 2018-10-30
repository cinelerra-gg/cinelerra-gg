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
#include "crikey.h"
#include "crikeywindow.h"
#include "language.h"
#include "vframe.h"

// chroma interpolated key, crikey

REGISTER_PLUGIN(CriKey)

#if 0
void crikey_pgm(const char *fn,VFrame *vfrm)
{
	FILE *fp = fopen(fn,"w");
	int w = vfrm->get_w(), h = vfrm->get_h();
	fprintf(fp,"P5\n%d %d\n255\n",w,h);
	fwrite(vfrm->get_data(),w,h,fp);
	fclose(fp);
}
#endif

CriKeyPoint::CriKeyPoint(int tag, int e, float x, float y, float t)
{
	this->tag = tag;  this->e = e;
	this->x = x;      this->y = y;
	this->t = t;
}
CriKeyPoint::~CriKeyPoint()
{
}

CriKeyConfig::CriKeyConfig()
{
	threshold = 0.5f;
	draw_mode = DRAW_ALPHA;
	drag = 0;
	selected = 0;
}
CriKeyConfig::~CriKeyConfig()
{
}

int CriKeyConfig::equivalent(CriKeyConfig &that)
{
	if( !EQUIV(this->threshold, that.threshold) ) return 0;
	if( this->draw_mode != that.draw_mode ) return 0;
	if( this->drag != that.drag ) return 0;
	if( this->points.size() != that.points.size() ) return 0;
	for( int i=0, n=points.size(); i<n; ++i ) {
		CriKeyPoint *ap = this->points[i], *bp = that.points[i];
		if( ap->tag != bp->tag ) return 0;
		if( ap->e != bp->e ) return 0;
		if( !EQUIV(ap->x, bp->x) ) return 0;
		if( !EQUIV(ap->y, bp->y) ) return 0;
		if( !EQUIV(ap->t, bp->t) ) return 0;
	}
	return 1;
}

void CriKeyConfig::copy_from(CriKeyConfig &that)
{
	this->threshold = that.threshold;
	this->draw_mode = that.draw_mode;
	this->drag = that.drag;
	this->selected = that.selected;

	points.remove_all_objects();
	for( int i=0,n=that.points.size(); i<n; ++i ) {
		CriKeyPoint *pt = that.points[i];
		add_point(pt->tag, pt->e, pt->x, pt->y, pt->t);
	}
}

void CriKeyConfig::interpolate(CriKeyConfig &prev, CriKeyConfig &next,
		long prev_frame, long next_frame, long current_frame)
{
	this->threshold = prev.threshold;
	this->draw_mode = prev.draw_mode;
	this->drag = prev.drag;
	this->selected = prev.selected;

	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	points.remove_all_objects();
	int prev_sz = prev.points.size(), next_sz = next.points.size();
	for( int i=0; i<prev_sz; ++i ) {
		CriKeyPoint *pt = prev.points[i], *nt = 0;
		float x = pt->x, y = pt->y, t = pt->t;
		int k = next_sz;  // associated by tag id in next
		while( --k >= 0 && pt->tag != (nt=next.points[k])->tag );
		if( k >= 0 ) {
			x = x * prev_scale + nt->x * next_scale;
			y = y * prev_scale + nt->y * next_scale;
			t = t * prev_scale + nt->t * next_scale;
		}
		add_point(pt->tag, pt->e, x, y, t);
	}
}

void CriKeyConfig::limits()
{
	bclamp(threshold, 0.0f, 1.0f);
	bclamp(draw_mode, 0, DRAW_MODES-1);
}

int CriKeyConfig::add_point(int tag, int e, float x, float y, float t)
{
	int k = points.size();
	if( tag < 0 ) {
		tag = 0;
		for( int i=k; --i>=0; ) {
			int n = points[i]->tag;
			if( n >= tag ) tag = n + 1;
		}
	}
	points.append(new CriKeyPoint(tag, e, x, y, t));
	return k;
}

void CriKeyConfig::del_point(int i)
{
	points.remove_object_number(i);
}


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
	float *edg;
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
	this->edg = (float*) edg->get_data();
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
			if( !msk[ofs] ) continue;
			msk[ofs] = 0;
			if( edge_pixel(ofs) ) continue;
			int lt = x, rt = x;
			int lofs = ofs;
			for( int i=lt; --i>=0; ) {
				if( !msk[--lofs] ) break;
				msk[lofs] = 0;  lt = i;
				if( edge_pixel(lofs) ) break;
			}
			int rofs = ofs;
			for( int i=rt; ++i< w; rt=i,msk[rofs]=0 ) {
				if( !msk[++rofs] ) break;
				msk[rofs] = 0;  rt = i;
				if( edge_pixel(rofs) ) break;
			}
			if( y+1 <  h ) push(y+1, lt, rt);
			if( y-1 >= 0 ) push(y-1, lt, rt);
		}
	}
}


CriKey::CriKey(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	msk = 0;
	edg = 0;
}

CriKey::~CriKey()
{
	delete engine;
	delete msk;
	delete edg;
}

int CriKey::set_target(float *color, int x, int y)
{
	if( x < 0 || x >= w ) return 1;
	if( y < 0 || y >= h ) return 1;
	uint8_t **src_rows = src->get_rows();
	if( is_float ) {
		float *fp = (float*)(src_rows[y] + x*bpp);
		for( int i=0; i<comp; ++i,++fp ) color[i] = *fp;
	}
	else {
		uint8_t *sp = src_rows[y] + x*bpp;
		for( int i=0; i<comp; ++i,++sp ) color[i] = *sp;
	}
	return 0;
}

const char* CriKey::plugin_title() { return N_("CriKey"); }
int CriKey::is_realtime() { return 1; }

NEW_WINDOW_MACRO(CriKey, CriKeyWindow);
LOAD_CONFIGURATION_MACRO(CriKey, CriKeyConfig)

int CriKey::new_point()
{
	EDLSession *session = get_edlsession();
	float x = !session ? 0.f : session->output_w / 2.f;
	float y = !session ? 0.f : session->output_h / 2.f;
	return config.add_point(-1, 0, x, y, 0.5f);
}

void CriKey::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("CRIKEY");
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("DRAW_MODE", config.draw_mode);
	output.tag.set_property("DRAG", config.drag);
	output.tag.set_property("SELECTED", config.selected);
	output.append_tag();
	output.append_newline();
	output.tag.set_title("/CRIKEY");
	output.append_tag();
	output.append_newline();
	for( int i=0, n = config.points.size(); i<n; ++i ) {
		CriKeyPoint *pt = config.points[i];
		char point[BCSTRLEN];
		sprintf(point,"/POINT_%d",pt->tag);
		output.tag.set_title(point+1);
		output.tag.set_property("E", pt->e);
		output.tag.set_property("X", pt->x);
		output.tag.set_property("Y", pt->y);
		output.tag.set_property("T", pt->t);
		output.append_tag();
		output.tag.set_title(point+0);
		output.append_tag();
		output.append_newline();
	}
	output.terminate_string();
}

void CriKey::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	config.points.remove_all_objects();
	int result = 0;

	while( !(result=input.read_tag()) ) {
		if( input.tag.title_is("CRIKEY") ) {
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			config.draw_mode = input.tag.get_property("DRAW_MODE", config.draw_mode);
			config.drag = input.tag.get_property("DRAG", config.drag);
			config.selected = input.tag.get_property("SELECTED", 0);
			config.limits();
		}
		else if( !strncmp(input.tag.get_title(),"POINT_",6) ) {
			int tag = atoi(input.tag.get_title() + 6);
			int e = input.tag.get_property("E", 0);
			float x = input.tag.get_property("X", 0.f);
			float y = input.tag.get_property("Y", 0.f);
			float t = input.tag.get_property("T", .5f);
			config.add_point(tag, e, x, y, t);
		}
	}

	if( !config.points.size() ) new_point();
}

void CriKey::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	thread->window->lock_window("CriKey::update_gui");
	CriKeyWindow *window = (CriKeyWindow*)thread->window;
	window->update_gui();
	window->flush();
	thread->window->unlock_window();
}

void CriKey::draw_alpha(VFrame *msk)
{
	uint8_t **src_rows = src->get_rows();
	uint8_t **msk_rows = msk->get_rows();
	switch( color_model ) {
	case BC_RGB_FLOAT:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				if( *mp ) continue;
				float *px = (float *)sp;
				px[0] = px[1] = px[2] = 0;
			}
		}
		break;
	case BC_RGBA_FLOAT:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				if( *mp ) continue;
				float *px = (float *)sp;
				px[3] = 0;
			}
		}
		break;
	case BC_RGB888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				if( *mp ) continue;
				uint8_t *px = sp;
				px[0] = px[1] = px[2] = 0;
			}
		}
		break;
	case BC_YUV888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				if( *mp ) continue;
				uint8_t *px = sp;
				px[0] = 0;
				px[1] = px[2] = 0x80;
			}
		}
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				if( *mp ) continue;
				uint8_t *px = sp;
				px[3] = 0;
			}
		}
		break;
	}
}

void CriKey::draw_edge(VFrame *edg)
{
	uint8_t **src_rows = src->get_rows();
	float **edg_rows = (float**) edg->get_rows(), gain = 10;
	switch( color_model ) {
	case BC_RGB_FLOAT:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y];
			float *ap = edg_rows[y];
			for( int x=0; x<w; ++x,++ap,sp+=bpp ) {
				float *px = (float *)sp, v = *ap * gain;
				px[0] = px[1] = px[2] = v<1 ? v : 1;
			}
		}
		break;
	case BC_RGBA_FLOAT:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y];
			float *ap = edg_rows[y];
			for( int x=0; x<w; ++x,++ap,sp+=bpp ) {
				float *px = (float *)sp, v = *ap * gain;
				px[0] = px[1] = px[2] = v<1 ? v : 1;
				px[3] = 1.0f;
			}
		}
		break;
	case BC_RGB888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y];
			float *ap = edg_rows[y];
			for( int x=0; x<w; ++x,++ap,sp+=bpp ) {
				uint8_t *px = sp;  float v = *ap * gain;
				px[0] = px[1] = px[2] = v<1 ? v*256 : 255;
			}
		}
		break;
	case BC_RGBA8888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y];
			float *ap = edg_rows[y];
			for( int x=0; x<w; ++x,++ap,sp+=bpp ) {
				uint8_t *px = sp;  float v = *ap * gain;
				px[0] = px[1] = px[2] = v<1 ? v*256 : 255;
				px[3] = 0xff;
			}
		}
		break;
	case BC_YUV888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y];
			float *ap = edg_rows[y];
			for( int x=0; x<w; ++x,++ap,sp+=bpp ) {
				uint8_t *px = sp;  float v = *ap * gain;
				px[0] = *ap<1 ? v*256 : 255;
				px[1] = px[2] = 0x80;
			}
		}
		break;
	case BC_YUVA8888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y];
			float *ap = edg_rows[y];
			for( int x=0; x<w; ++x,++ap,sp+=bpp ) {
				uint8_t *px = sp;  float v = *ap * gain;
				px[0] = *ap<1 ? v*256 : 255;
				px[1] = px[2] = 0x80;
				px[3] = 0xff;
			}
		}
		break;
	}
}

void CriKey::draw_mask(VFrame *msk)
{
	uint8_t **src_rows = src->get_rows();
	uint8_t **msk_rows = msk->get_rows();
	float scale = 1 / 255.0f;
	switch( color_model ) {
	case BC_RGB_FLOAT:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				float a = *mp * scale;
				float *px = (float *)sp;
				px[0] = px[1] = px[2] = a;
			}
		}
		break;
	case BC_RGBA_FLOAT:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				float a = *mp * scale;
				float *px = (float *)sp;
				px[0] = px[1] = px[2] = a;
				px[3] = 1.0f;
			}
		}
		break;
	case BC_RGB888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				float a = *mp * scale;
				uint8_t *px = sp;
				px[0] = px[1] = px[2] = a * 255;
			}
		}
		break;
	case BC_RGBA8888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				float a = *mp * scale;
				uint8_t *px = sp;
				px[0] = px[1] = px[2] = a * 255;
				px[3] = 0xff;
			}
		}
		break;
	case BC_YUV888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				float a = *mp * scale;
				uint8_t *px = sp;
				px[0] = a * 255;
				px[1] = px[2] = 0x80;
			}
		}
		break;
	case BC_YUVA8888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				float a = *mp * scale;
				uint8_t *px = sp;
				px[0] = a * 255;
				px[1] = px[2] = 0x80;
				px[3] = 0xff;
			}
		}
		break;
	}
}


void CriKey::draw_point(VFrame *src, CriKeyPoint *pt)
{
	int d = bmax(w,h) / 200 + 2;
	int r = d/2+1, x = pt->x, y = pt->y;
	src->draw_smooth(x-r,y+0, x-r, y-r, x+0,y-r);
	src->draw_smooth(x+0,y-r, x+r, y-r, x+r,y+0);
	src->draw_smooth(x+r,y+0, x+r, y+r, x+0,y+r);
	src->draw_smooth(x+0,y+r, x-r, y+r, x-r,y+0);
	if( pt->e ) {
		src->set_pixel_color(RED);
		src->draw_x(pt->x, pt->y, d);
	}
	else {
		src->set_pixel_color(BLUE);
		src->draw_t(pt->x, pt->y, d);
	}
}


static void get_vframe(VFrame *&vfrm, int w, int h, int color_model)
{
	if( vfrm && ( vfrm->get_w() != w || vfrm->get_h() != h ) ) {
		delete vfrm;  vfrm = 0;
	}
	if( !vfrm ) vfrm = new VFrame(w, h, color_model, 0);
}

static void fill_edge(VFrame *vfrm, int w, int h)
{
	int w1 = w-1, h1 = h-1;
	float *dp = (float*) vfrm->get_data();
	if( w1 > 0 ) for( int y=0; y<h1; ++y,dp+=w ) dp[w1] = dp[w1-1];
	if( h1 > 0 ) for( int x=0; x<w; ++x ) dp[x] = dp[x-w];
}

int CriKey::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
{
	load_configuration();
	src = frame;
	w = src->get_w(), h = src->get_h();
	color_model = src->get_color_model();
	bpp = BC_CModels::calculate_pixelsize(color_model);
	is_float = BC_CModels::is_float(color_model);
	is_yuv = BC_CModels::is_yuv(color_model);
	comp = BC_CModels::components(color_model);
	if( comp > 3 ) comp = 3;

	read_frame(src, 0, start_position, frame_rate, 0);
	get_vframe(edg, w, h, BC_A_FLOAT);

	if( !engine )
		engine = new CriKeyEngine(this,
			PluginClient::get_project_smp() + 1,
			PluginClient::get_project_smp() + 1);

	get_vframe(msk, w, h, BC_A8);
	memset(msk->get_data(), 0xff, msk->get_data_size());

	for( int i=0, n=config.points.size(); i<n; ++i ) {
		CriKeyPoint *pt = config.points[i];
		if( !pt->e ) continue;
		if( set_target(engine->color, pt->x, pt->y) ) continue;
		engine->threshold = pt->t;
		edg->clear_frame();
		engine->process_packages();
		fill_edge(edg, w, h);
		FillRegion fill_region(edg, msk);
		fill_region.fill(pt->x, pt->y);
		fill_region.run();
	}

//crikey_pgm("/tmp/msk.pgm",msk);
	switch( config.draw_mode ) {
	case DRAW_ALPHA: draw_alpha(msk);  break;
	case DRAW_EDGE:  draw_edge(edg);   break;
	case DRAW_MASK:  draw_mask(msk);   break;
	}

	if( config.drag ) {
		for( int i=0, n=config.points.size(); i<n; ++i ) {
			CriKeyPoint *pt = config.points[i];
			src->set_pixel_color(config.selected == i ? GREEN : WHITE);
			draw_point(src, pt);
		}
	}
	return 0;
}


void CriKeyEngine::init_packages()
{
	int y = 0, h1 = plugin->h-1;
	for(int i = 0; i < get_total_packages(); ) {
		CriKeyPackage *pkg = (CriKeyPackage*)get_package(i++);
		pkg->y1 = y;
		y = h1 * i / LoadServer::get_total_packages();
		pkg->y2 = y;
	}
}

LoadPackage* CriKeyEngine::new_package()
{
	return new CriKeyPackage();
}

LoadClient* CriKeyEngine::new_client()
{
	return new CriKeyUnit(this);
}

#define EDGE_MACRO(type, components, is_yuv) \
{ \
	uint8_t **src_rows = src->get_rows(); \
	int comps = MIN(components, 3); \
	for( int y=y1; y<y2; ++y ) { \
		uint8_t *row0 = src_rows[y], *row1 = src_rows[y+1]; \
		float *edgp = edg_rows[y]; \
		for( int x=x1; x<x2; ++edgp,++x,row0+=bpp,row1+=bpp ) { \
			type *r0 = (type*)row0, *r1 = (type*)row1; \
			float a00 = 0, a01 = 0, a10 = 0, a11 = 0; \
			for( int c=0; c<comps; ++c,++r0,++r1 ) { \
				float t = target_color[c]; \
                                a00 += fabs(t - r0[0]); \
				a01 += fabs(t - r0[components]); \
				a10 += fabs(t - r1[0]); \
				a11 += fabs(t - r1[components]); \
			} \
			float mx = scale * bmax(bmax(a00, a01), bmax(a10, a11)); \
			if( mx < threshold ) continue; \
                        float mn = scale * bmin(bmin(a00, a01), bmin(a10, a11)); \
			if( mn >= threshold ) continue; \
			*edgp += (mx - mn); \
		} \
	} \
} break


void CriKeyUnit::process_package(LoadPackage *package)
{
	int color_model = server->plugin->color_model;
	int bpp = server->plugin->bpp;
	VFrame *src = server->plugin->src;
	VFrame *edg = server->plugin->edg;
	float **edg_rows = (float**)edg->get_rows();
	float *target_color = server->color;
	float threshold = 2.f * server->threshold*server->threshold;
	float scale = 1./BC_CModels::calculate_max(color_model);
	CriKeyPackage *pkg = (CriKeyPackage*)package;
	int x1 = 0, x2 = server->plugin->w-1;
	int y1 = pkg->y1, y2 = pkg->y2;

	switch( color_model ) {
	case BC_RGB_FLOAT:  EDGE_MACRO(float, 3, 0);
	case BC_RGBA_FLOAT: EDGE_MACRO(float, 4, 0);
	case BC_RGB888:	    EDGE_MACRO(unsigned char, 3, 0);
	case BC_YUV888:	    EDGE_MACRO(unsigned char, 3, 1);
	case BC_RGBA8888:   EDGE_MACRO(unsigned char, 4, 0);
	case BC_YUVA8888:   EDGE_MACRO(unsigned char, 4, 1);
	}
}

