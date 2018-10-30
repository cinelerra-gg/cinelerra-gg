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

#include "affine.h"
#include "bcsignals.h"
#include "clip.h"
#include "filexml.h"
#include "edge.h"
#include "edgewindow.h"
#include "language.h"
#include "transportque.inc"
#include <string.h>

REGISTER_PLUGIN(Edge)

EdgeConfig::EdgeConfig()
{
	amount = 8;
}

int EdgeConfig::equivalent(EdgeConfig &that)
{
	if(this->amount != that.amount) return 0;
	return 1;
}

void EdgeConfig::copy_from(EdgeConfig &that)
{
	this->amount = that.amount;
}

void EdgeConfig::interpolate( EdgeConfig &prev, EdgeConfig &next,
		long prev_frame, long next_frame, long current_frame)
{
	copy_from(next);
}

void EdgeConfig::limits()
{
	CLAMP(amount, 0, 10);
}


Edge::Edge(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	dst = 0;
}

Edge::~Edge()
{
	delete engine;
	delete dst;
}

const char* Edge::plugin_title() { return N_("Edge"); }
int Edge::is_realtime() { return 1; }

NEW_WINDOW_MACRO(Edge, EdgeWindow);
LOAD_CONFIGURATION_MACRO(Edge, EdgeConfig)

void Edge::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("EDGE");
	output.tag.set_property("AMOUNT", config.amount);
	output.append_tag();
	output.append_newline();
	output.tag.set_title("/EDGE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void Edge::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	int result = 0;

	while( !(result=input.read_tag()) ) {
		if(input.tag.title_is("EDGE")) {
			config.amount = input.tag.get_property("AMOUNT", config.amount);
			config.limits();
		}
		else if(input.tag.title_is("/EDGE")) {
			result = 1;
		}
	}

}

void Edge::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	thread->window->lock_window("Edge::update_gui");
	EdgeWindow *window = (EdgeWindow*)thread->window;
	window->flush();
	thread->window->unlock_window();
}

int Edge::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
{
	load_configuration();
	src = frame;
	w = src->get_w(), h = src->get_h();
	color_model = frame->get_color_model();
	bpp = BC_CModels::calculate_pixelsize(color_model);

	if( dst && (dst->get_w() != w || dst->get_h() != h ||
	    dst->get_color_model() != color_model ) ) {
		delete dst;  dst = 0;
	}
	if( !dst )
		dst = new VFrame(w, h, color_model, 0);

	if( !engine )
		engine = new EdgeEngine(this,
			PluginClient::get_project_smp() + 1,
			PluginClient::get_project_smp() + 1);

	read_frame(frame, 0, start_position, frame_rate, 0);
	engine->process_packages();
	frame->copy_from(dst);
	return 0;
}


EdgePackage::EdgePackage()
 : LoadPackage()
{
}

EdgeUnit::EdgeUnit(EdgeEngine *server) : LoadClient(server)
{
	this->server = server;
}

EdgeUnit::~EdgeUnit()
{
}

#define EDGE_MACRO(type, max, components, is_yuv) \
{ \
	int comps = MIN(components, 3); \
	float amounts = amount * amount / max; \
	for( int y=y1; y<y2; ++y ) { \
		uint8_t *row0 = input_rows[y], *row1 = input_rows[y+1]; \
		uint8_t *outp = output_rows[y]; \
		for( int x=x1; x<x2; ++x ) { \
			type *r0 = (type *)row0, *r1 = (type *)row1, *op = (type *)outp; \
			float h_grad = 0, v_grad = 0; \
			for( int i=0; i<comps; ++i,++r0,++r1 ) { \
				float dh = -r0[0] - r0[components] + r1[0] + r1[components]; \
				if( (dh*=dh) > h_grad ) h_grad = dh; \
				float dv =  r0[0] - r0[components] + r1[0] - r1[components]; \
				if( (dv*=dv) > v_grad ) v_grad = dv; \
			} \
			float v = (h_grad + v_grad) * amounts; \
			type t = v > max ? max : v; \
			if( is_yuv ) { \
				*op++ = t;  *op++ = 0x80;  *op++ = 0x80; \
			} \
			else { \
				for( int i=0; i<comps; ++i ) *op++ = t; \
			} \
			if( components == 4 ) *op = *r0; \
			row0 += bpp;  row1 += bpp;  outp += bpp; \
		} \
	} \
} break


void EdgeUnit::process_package(LoadPackage *package)
{
	VFrame *src = server->plugin->src;
	uint8_t **input_rows = src->get_rows();
	VFrame *dst = server->plugin->dst;
	uint8_t **output_rows = dst->get_rows();
	float amount = (float)server->plugin->config.amount;
	EdgePackage *pkg = (EdgePackage*)package;
	int x1 = 0, x2 = server->plugin->w-1, bpp = server->plugin->bpp;
	int y1 = pkg->y1, y2 = pkg->y2;

	switch( server->plugin->color_model ) {
	case BC_RGB_FLOAT:  EDGE_MACRO(float, 1, 3, 0);
	case BC_RGBA_FLOAT: EDGE_MACRO(float, 1, 4, 0);
	case BC_RGB888:	    EDGE_MACRO(unsigned char, 0xff, 3, 0);
	case BC_YUV888:	    EDGE_MACRO(unsigned char, 0xff, 3, 1);
	case BC_RGBA8888:   EDGE_MACRO(unsigned char, 0xff, 4, 0);
	case BC_YUVA8888:   EDGE_MACRO(unsigned char, 0xff, 4, 1);
	}
}


EdgeEngine::EdgeEngine(Edge *plugin, int total_clients, int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

EdgeEngine::~EdgeEngine()
{
}


void EdgeEngine::init_packages()
{
	int y = 0, h1 = plugin->h-1;
	for(int i = 0; i < get_total_packages(); ) {
		EdgePackage *pkg = (EdgePackage*)get_package(i++);
		pkg->y1 = y;
		y = h1 * i / LoadServer::get_total_packages();
		pkg->y2 = y;
	}
}

LoadClient* EdgeEngine::new_client()
{
	return new EdgeUnit(this);
}

LoadPackage* EdgeEngine::new_package()
{
	return new EdgePackage;
}

