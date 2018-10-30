
/*
 * CINELERRA
 * Copyright (C) 2008-2012 Adam Williams <broadcast at earthling dot net>
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

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "histeq.h"
#include "filexml.h"
#include "language.h"
#include "loadbalance.h"
#include "bccolors.h"
#include "vframe.h"

class HistEqMain;
class HistEqEngine;
class HistEqWindow;

REGISTER_PLUGIN(HistEqMain)

HistEqConfig::HistEqConfig()
{
	split = 0;
	plot = 0;
	blend = 0.5;
	gain = 1.0;
}

HistEqConfig::~HistEqConfig()
{
}

void HistEqConfig::copy_from(HistEqConfig &that)
{
	split = that.split;
	plot = that.plot;
	blend = that.blend;
	gain = that.gain;
}

int HistEqConfig::equivalent(HistEqConfig &that)
{
	return 1;
}

void HistEqConfig::interpolate(HistEqConfig &prev, HistEqConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	copy_from(prev);
}


HistEqWindow::HistEqWindow(HistEqMain *plugin)
 : PluginClientWindow(plugin, plugin->w, plugin->h, plugin->w, plugin->h, 0)
{
	this->plugin = plugin;
}

HistEqWindow::~HistEqWindow()
{
}


void HistEqWindow::create_objects()
{
	int x = 10, y = 10;
	int cw = get_w()-2*x, ch = cw*3/4;
	add_subwindow(canvas = new HistEqCanvas(this, plugin, x, y, cw, ch));
	y += canvas->get_h() + 10;
	
	add_subwindow(split = new HistEqSplit(this, plugin, x, y));
	y += split->get_h() + 10;

	add_subwindow(plot = new HistEqPlot(this, plugin, x, y));
	y += plot->get_h() + 10;

	int x1 = x + 60;
	add_subwindow(new BC_Title(x, y, _("Blend:")));
	add_subwindow(blend = new HistEqBlend(this, plugin, x1, y));
	y += blend->get_h() + 10;

	add_subwindow(new BC_Title(x, y, _("Gain:")));
	add_subwindow(gain = new HistEqGain(this, plugin, x1, y));
	y += gain->get_h() + 10;

	show_window();
}

void HistEqWindow::update()
{
}

HistEqCanvas::HistEqCanvas(HistEqWindow *gui, HistEqMain *plugin,
                int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
        this->gui = gui;
        this->plugin = plugin;
}
void HistEqCanvas::clear()
{
	clear_box(0,0, get_w(),get_h());
}

void HistEqCanvas::draw_bins(HistEqMain *plugin)
{
	set_color(GREEN);
	int *data = plugin->bins;
	int n = plugin->binsz, max = 0;
	for( int i=0; i<n; ++i )
		if( max < data[i] ) max = data[i];
	double lmax = log(max);
	int cw = get_w(), ch = get_h();
	int x0 = 0, x = 0;
	while( x < cw ) {
		int mx = data[x0];
		int x1 = (n * ++x) / cw;
		for( int i=x0; ++i<x1; ) {
			int m = data[i];
			if( m > mx ) mx = m;
		}
		double lmx = mx>0 ? log(mx) : 0;
		int y1 = ch * (1 - lmx/lmax);
		draw_line(x,0, x,y1);
		x0 = x1;
	}
}

void HistEqCanvas::draw_wts(HistEqMain *plugin)
{
	float *wts = plugin->wts;
	int n = plugin->binsz;
	set_color(BLUE);
	int cw1 = get_w()-1, ch1 = get_h()-1;
	float g0 = plugin->config.gain, g1 = 1-g0;
	int x1 = 0, y1 = ch1;
	while( x1 < cw1 ) {
		int x0 = x1++, y0 = y1;
		int is = (n * x1) / cw1;
		float fy = wts[is]*g0 + ((float)x1/cw1)*g1;
		y1 = (1-fy) * ch1;
		draw_line(x0,y0, x1,y1);
	}
}

void HistEqCanvas::draw_reg(HistEqMain *plugin)
{
	set_color(WHITE);
	int cw1 = get_w()-1, ch1 = get_h()-1;
	float g0 = plugin->config.gain, g1 = 1-g0;
	int x0 = 0, x1 = plugin->binsz-1;
	double a = plugin->a, b = plugin->b;
	float fy0 = (a*x0 + b)*g0 + 0*g1;
	float fy1 = (a*x1 + b)*g0 + 1*g1;
	int y0 = (1 - fy0) * ch1;
	int y1 = (1 - fy1) * ch1;
	draw_line(0,y0, cw1,y1);
}

void HistEqCanvas::draw_lut(HistEqMain *plugin)
{
	int *lut = plugin->lut;
	int n = plugin->lutsz-1;
	double s = 1. / n;
	set_color(RED);
	int cw1 = get_w()-1, ch1 = get_h()-1;
	int x1 = 0, y1 = ch1;
	while( x1 < cw1 ) {
		int x0 = x1++, y0 = y1;
		int is = (n * x1) / cw1;
		y1 = (1-s*lut[is]) * ch1;
		draw_line(x0,y0, x1,y1);
	}
}

void HistEqCanvas::update(HistEqMain *plugin)
{
	clear();
	draw_bins(plugin);
	draw_wts(plugin);
	draw_reg(plugin);
	draw_lut(plugin);
	flash();
}

HistEqSplit::HistEqSplit(HistEqWindow *gui, HistEqMain *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.split, _("Split output"))
{
        this->gui = gui;
        this->plugin = plugin;
}
HistEqSplit::~HistEqSplit()
{
}

int HistEqSplit::handle_event()
{
        plugin->config.split = get_value();
        plugin->send_configure_change();
        return 1;
}

HistEqPlot::HistEqPlot(HistEqWindow *gui, HistEqMain *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.plot, _("Plot bins/lut"))
{
        this->gui = gui;
        this->plugin = plugin;
}
HistEqPlot::~HistEqPlot()
{
}

int HistEqPlot::handle_event()
{
        plugin->config.plot = get_value();
        plugin->send_configure_change();
        return 1;
}

HistEqBlend::HistEqBlend(HistEqWindow *gui, HistEqMain *plugin, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1.0, plugin->config.blend, 0)
{
        this->gui = gui;
        this->plugin = plugin;
        set_precision(0.01);
}
HistEqBlend::~HistEqBlend()
{
}

int HistEqBlend::handle_event()
{
        plugin->config.blend = get_value();
        plugin->send_configure_change();
        return 1;
}


HistEqGain::HistEqGain(HistEqWindow *gui, HistEqMain *plugin, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1.0, plugin->config.gain, 0)
{
        this->gui = gui;
        this->plugin = plugin;
        set_precision(0.01);
}
HistEqGain::~HistEqGain()
{
}

int HistEqGain::handle_event()
{
        plugin->config.gain = get_value();
        plugin->send_configure_change();
        return 1;
}


HistEqMain::HistEqMain(PluginServer *server)
 : PluginVClient(server)
{
	w = 300;  h = 375;
	engine = 0;
	sz = 0;
	binsz = bsz = 0;  bins = 0;
	lutsz = lsz = 0;  lut = 0;
	wsz = 0;          wts = 0;
	a = 0;  b = 0;
}

HistEqMain::~HistEqMain()
{
	delete engine;
	delete [] bins;
	delete [] lut;
	delete [] wts;
}

const char* HistEqMain::plugin_title() { return N_("HistEq"); }
int HistEqMain::is_realtime() { return 1; }


NEW_WINDOW_MACRO(HistEqMain, HistEqWindow)

LOAD_CONFIGURATION_MACRO(HistEqMain, HistEqConfig)

void HistEqMain::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	((HistEqWindow*)thread->window)->lock_window("HistEqMain::update_gui");
	HistEqWindow* gui = (HistEqWindow*)thread->window;
	gui->update();
	gui->unlock_window();
}

void HistEqMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("HISTEQ");
	output.tag.set_property("W", w);
	output.tag.set_property("H", h);
	output.tag.set_property("SPLIT", config.split);
	output.tag.set_property("PLOT", config.plot);
	output.tag.set_property("BLEND", config.blend);
	output.tag.set_property("GAIN", config.gain);
	output.append_tag();
	output.tag.set_title("/HISTEQ");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void HistEqMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	int result = 0;
	while( !(result=input.read_tag()) ) {
		if( input.tag.title_is("HISTEQ") ) {
			if(is_defaults()) {
				w = input.tag.get_property("W", w);
				h = input.tag.get_property("H", h);
			}
			config.split = input.tag.get_property("SPLIT", config.split);
			config.plot = input.tag.get_property("PLOT", config.plot);
			config.blend = input.tag.get_property("BLEND", config.blend);
			config.gain = input.tag.get_property("GAIN", config.gain);
		}
	}
}

void HistEqMain::render_gui(void *data)
{
	if( !thread ) return;
	((HistEqWindow*)thread->window)->lock_window("HistEqMain::render_gui 1");
	HistEqWindow *gui = (HistEqWindow*)thread->window;
	gui->canvas->update((HistEqMain *)data);
	gui->unlock_window();
}

// regression line
static void fit(float *dat, int n, double &a, double &b)
{
	double sy = 0;
	for( int i=0; i<n; ++i ) sy += dat[i];
	double s = 0, mx = (n-1)/2., my = sy / n;
	for( int i=0; i<n; ++i ) s += (i-mx) * dat[i];
	double m = (n+1)/2;
	double ss = 2. * ((m+1)*m*(m-1)/3. + m*(m-1)/2.);
	a = s / ss;
	b = my - mx*a;
}

int HistEqMain::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
{
	load_configuration();
	read_frame(frame, 0, start_position, frame_rate, 0);
	this->input = this->output = frame;

	int colormodel = frame->get_color_model();
	lutsz = 0x10000;
	binsz = (3*0xffff) + 1;
	switch( colormodel ) {
	case BC_RGB888:
	case BC_RGBA8888:
		binsz = (3*0xff) + 1;
		lutsz = 0x100;
		break;
	case BC_RGB_FLOAT:
	case BC_RGBA_FLOAT:
		binsz = (3*0x5555) + 1;
		break;
	}
	if( binsz != bsz ) { delete bins;  bins = 0;  bsz = 0; }
	if( !bins ) bins = new int[bsz = binsz];
	if( binsz+1 != wsz ) { delete wts;   wts = 0;   wsz = 0; }
	if( !wts ) wts = new float[wsz = binsz+1];
	if( lutsz != lsz ) { delete lut;   lut = 0;   lsz = 0; }
	if( !lut  ) lut  = new int[lsz = lutsz];

	if(!engine) engine = new HistEqEngine(this,
		get_project_smp() + 1, get_project_smp() + 1);
	engine->process_packages(HistEqEngine::HISTEQ, frame);

// sum the results, not all clients may have run
	memset(bins, 0, binsz*sizeof(bins[0]));
	for( int i=0, n=engine->get_total_clients(); i<n; ++i ) {
		HistEqUnit *unit = (HistEqUnit*)engine->get_client(i);
		if( !unit->valid ) continue;
		for( int i=0; i<binsz; ++i ) bins[i] += unit->bins[i];
	}

// Remove top and bottom from calculations.
//  Doesn't work in high precision colormodels.
	int n = frame->get_w() * frame->get_h();
	int binsz1 = binsz-1, lutsz1 = lutsz-1;
	n -= bins[0];       bins[0] = 0;
	n -= bins[binsz1];  bins[binsz1] = 0;
	sz = n;

// integrate and normalize
	for( int i=0,t=0; i<binsz; ++i ) { t += bins[i];  wts[i] = (float)t / n; }
	wts[binsz] = 1.f;
// exclude margins (+-2%)
	float fmn =  0.02f, fmx = 1. - fmn;
	int mn = 0;    while( mn < binsz && wts[mn] < fmn ) ++mn;
	int mx = binsz; while( mx > mn  && wts[mx-1] > fmx ) --mx;
	n = mx-mn+1;  fit(&wts[mn], n, a, b);
// home y intercept
	b -= a * mn;
	if( (a*n + b) < 1 ) a = (1 - b) / n;
	if( b > 0 ) { a = (a*n + b) / n;  b = 0; }
	double blend = config.blend, blend1 = 1-blend;
	double r = (double)binsz1 / lutsz1;
	float g0 = config.gain, g1 = 1-g0;
	for( int i=0; i<lutsz; ++i ) {
		double is = i * r;
		int is0 = is, is1 = is0+1;
		double s0 = is-is0, s1 = 1-s0;
// piecewise linear interp btwn wts[is]..wts[is+1]
		float u = wts[is0]*s0 + wts[is1]*s1;
// regression line
		float v = is*a + b;
// blend bins eq with linear regression, add scalar gain
		float t = u*blend + v*blend1;
		int iy = (t*lutsz1)*g0 + i*g1;
		lut[i] = bclip(iy, 0, lutsz1);
	}
	if( config.plot && gui_open() )
		send_render_gui(this);

	engine->process_packages(HistEqEngine::APPLY, frame);
	return 0;
}

HistEqPackage::HistEqPackage()
 : LoadPackage()
{
}

HistEqUnit::HistEqUnit(HistEqEngine *server, HistEqMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
	bins = 0;
	binsz = 0;
	valid = 0;
}

HistEqUnit::~HistEqUnit()
{
	delete [] bins;
}

void HistEqUnit::process_package(LoadPackage *package)
{
	if( binsz != plugin->binsz ) {
		delete bins;  bins = 0;  binsz = 0;
	}
	if( !bins ) {
		bins = new int[binsz = plugin->binsz];
	}
	if( !valid ) {
		valid = 1;
		bzero(bins, binsz*sizeof(bins[0]));
	}

	HistEqPackage *pkg = (HistEqPackage*)package;
	switch( server->operation ) {
	case HistEqEngine::HISTEQ: {

#define HISTEQ_HEAD(type) { \
	type **rows = (type**)data->get_rows(); \
	for( int iy=pkg->y0; iy<pkg->y1; ++iy ) { \
		type *row = rows[iy]; \
		for( int ix=0; ix<w; ++ix ) {

#define HISTEQ_TAIL(components) \
			++bins[i]; \
			row += components; \
		} \
	} \
}
		VFrame *data = server->data;
		int colormodel = data->get_color_model();
		int w = data->get_w(), comps = BC_CModels::components(colormodel);

		switch( colormodel ) {
		case BC_RGB888:
		case BC_RGBA8888:
			HISTEQ_HEAD(unsigned char)
			int r = row[0], g = row[1], b = row[2];
			int i = r + g + b;
			HISTEQ_TAIL(comps)
			break;
		case BC_RGB161616:
		case BC_RGBA16161616:
			HISTEQ_HEAD(uint16_t)
			int r = row[0], g = row[1], b = row[2];
			int i = r + g + b;
			HISTEQ_TAIL(comps)
			break;
		case BC_RGB_FLOAT:
		case BC_RGBA_FLOAT:
			HISTEQ_HEAD(float)
			int r = (int)(row[0] * 0x5555);
			int g = (int)(row[1] * 0x5555);
			int b = (int)(row[2] * 0x5555);
			int i = r + g + b;  bclamp(i, 0,0xffff);
			HISTEQ_TAIL(comps)
			break;
		case BC_YUV888:
		case BC_YUVA8888:
			HISTEQ_HEAD(unsigned char)
			int y = (row[0]<<8) + row[0];
			int u = (row[1]<<8) + row[1];
			int v = (row[2]<<8) + row[2];
			int r, g, b;
			YUV::yuv.yuv_to_rgb_16(r, g, b, y, u, v);
			int i = r + g + b;
			HISTEQ_TAIL(comps)
			break;
		case BC_YUV161616:
		case BC_YUVA16161616:
			HISTEQ_HEAD(uint16_t)
			int r, g, b;
			YUV::yuv.yuv_to_rgb_16(r, g, b, row[0], row[1], row[2]);
			int i = r + g + b;
			HISTEQ_TAIL(comps)
			break;
		}
		break; }
	case HistEqEngine::APPLY: {
#define PROCESS_RGB(type, components, max) { \
	type **rows = (type**)input->get_rows(); \
	for( int iy=pkg->y0; iy<pkg->y1; ++iy ) { \
		type *row = rows[iy]; \
		int x1 = !split ? w : (iy * w) / h; \
		for( int x=0; x<x1; ++x ) { \
			int r = row[0], g = row[1], b = row[2]; \
			row[0] = lut[r];  row[1] = lut[g];  row[2] = lut[b]; \
			row += components; \
		} \
	} \
}
#define PROCESS_YUV(type, components, max) { \
	type **rows = (type**)input->get_rows(); \
	for( int iy=pkg->y0; iy<pkg->y1; ++iy ) { \
		type *row = rows[iy]; \
		int x1 = !split ? w : (iy * w) / h; \
		for( int ix=0; ix<x1; ++ix ) { \
			int r, g, b, y, u, v; \
			if( max == 0xff ) { \
				y = (row[0] << 8) | row[0]; \
				u = (row[1] << 8) | row[1]; \
				v = (row[2] << 8) | row[2]; \
			} \
			else { \
				y = row[0]; u = row[1]; v = row[2]; \
			} \
			YUV::yuv.yuv_to_rgb_16(r, g, b, y, u, v); \
			YUV::yuv.rgb_to_yuv_16(lut[r], lut[g], lut[b], y, u, v); \
			if( max == 0xff ) { \
				row[0] = y >> 8; \
				row[1] = u >> 8; \
				row[2] = v >> 8; \
			} \
			else { \
				row[0] = y; row[1] = u; row[2] = v; \
			} \
			row += components; \
		} \
	} \
}
#define PROCESS_FLOAT(components) { \
	float **rows = (float**)input->get_rows(); \
	for( int iy=pkg->y0; iy<pkg->y1; ++iy ) { \
		float *row = rows[iy]; \
		int x1 = !split ? w : (iy * w) / h; \
		for( int ix=0; ix<x1; ++ix ) { \
			int r = row[0]*0xffff, g = row[1]*0xffff, b = row[2]*0xffff; \
			bclamp(r, 0,0xffff);  bclamp(g, 0,0xffff);  bclamp(b, 0,0xffff); \
			row[0] = (float)lut[r] / 0xffff; \
			row[1] = (float)lut[g] / 0xffff; \
			row[2] = (float)lut[b] / 0xffff; \
			row += components; \
		} \
	} \
}

		VFrame *input = plugin->input;
		int w = input->get_w(), h = input->get_h();
		int split = plugin->config.split;
		int *lut = plugin->lut;

		switch(input->get_color_model()) {
		case BC_RGB888:
			PROCESS_RGB(unsigned char, 3, 0xff)
			break;
		case BC_RGBA8888:
			PROCESS_RGB(unsigned char, 4, 0xff)
			break;
		case BC_RGB161616:
			PROCESS_RGB(uint16_t, 3, 0xffff)
			break;
		case BC_RGBA16161616:
			PROCESS_RGB(uint16_t, 4, 0xffff)
			break;
		case BC_RGB_FLOAT:
			PROCESS_FLOAT(3);
			break;
		case BC_RGBA_FLOAT:
			PROCESS_FLOAT(4);
			break;
		case BC_YUV888:
			PROCESS_YUV(unsigned char, 3, 0xff)
			break;
		case BC_YUVA8888:
			PROCESS_YUV(unsigned char, 4, 0xff)
			break;
		case BC_YUV161616:
			PROCESS_YUV(uint16_t, 3, 0xffff)
			break;
		case BC_YUVA16161616:
			PROCESS_YUV(uint16_t, 4, 0xffff)
			break;
		}
		break; }
	}
}

HistEqEngine::HistEqEngine(HistEqMain *plugin,
	int total_clients,
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void HistEqEngine::init_packages()
{
	int h = data->get_h();
	int y0 = 0;

	for( int i=0, n=get_total_packages(); i<n; ) {
		HistEqPackage *pkg = (HistEqPackage *)get_package(i);
		int y1 = ++i * h / n;
		pkg->y0 = y0;  pkg->y1 = y1;
		y0 = y1;
	}
	for( int i=0, n=get_total_clients(); i<n; ++i ) {
		HistEqUnit *unit = (HistEqUnit*)get_client(i);
		unit->valid = 0; // set if unit runs
	}
}

LoadClient* HistEqEngine::new_client()
{
	return new HistEqUnit(this, plugin);
}

LoadPackage* HistEqEngine::new_package()
{
	return new HistEqPackage;
}

void HistEqEngine::process_packages(int operation, VFrame *data)
{
	this->data = data;
	this->operation = operation;

	LoadServer::process_packages();
}


