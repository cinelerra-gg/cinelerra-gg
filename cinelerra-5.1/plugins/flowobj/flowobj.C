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

#include "clip.h"
#include "filexml.h"
#include "flowobj.h"
#include "flowobjwindow.h"
#include "language.h"
#include "transportque.inc"

REGISTER_PLUGIN(FlowObj)

#define MAX_COUNT 250
#define WIN_SIZE 20

FlowObjConfig::FlowObjConfig()
{
	draw_vectors = 0;
	do_stabilization = 1;
	block_size = 20;
	search_radius = 10;
	settling_speed = 5;
}

int FlowObjConfig::equivalent(FlowObjConfig &that)
{
	return draw_vectors == that.draw_vectors &&
		do_stabilization == that.do_stabilization &&
		block_size == that.block_size &&
		search_radius == that.search_radius &&
		settling_speed == that.settling_speed;
}

void FlowObjConfig::copy_from(FlowObjConfig &that)
{
	draw_vectors = that.draw_vectors;
	do_stabilization = that.do_stabilization;
	block_size = that.block_size;
	search_radius = that.search_radius;
	settling_speed = that.settling_speed;
}

void FlowObjConfig::interpolate( FlowObjConfig &prev, FlowObjConfig &next, 
	long prev_frame, long next_frame, long current_frame)
{
	copy_from(next);
}

void FlowObjConfig::limits()
{
	bclamp(block_size, 5, 100);
	bclamp(search_radius, 1, 100);
	bclamp(settling_speed, 0, 100);
}


FlowObj::FlowObj(PluginServer *server)
 : PluginVClient(server)
{
	flow_engine = 0;
	overlay = 0;
	accum = 0;
	prev_position = next_position = -1;
	x_accum = y_accum = 0;
	angle_accum = 0;
	prev_corners = 0;
	next_corners = 0;
}

FlowObj::~FlowObj()
{
	delete flow_engine;
	delete overlay;
	delete prev_corners;
	delete next_corners;
}

const char* FlowObj::plugin_title() { return N_("FlowObj"); }
int FlowObj::is_realtime() { return 1; }

NEW_WINDOW_MACRO(FlowObj, FlowObjWindow);
LOAD_CONFIGURATION_MACRO(FlowObj, FlowObjConfig)

void FlowObj::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("FLOWOBJ");
	output.tag.set_property("DRAW_VECTORS", config.draw_vectors);
	output.tag.set_property("DO_STABILIZATION", config.do_stabilization);
	output.tag.set_property("BLOCK_SIZE", config.block_size);
	output.tag.set_property("SEARCH_RADIUS", config.search_radius);
	output.tag.set_property("SETTLING_SPEED", config.settling_speed);
	output.append_tag();
	output.append_newline();
	output.tag.set_title("/FLOWOBJ");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void FlowObj::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("FLOWOBJ") ) {
			config.draw_vectors = input.tag.get_property("DRAW_VECTORS", config.draw_vectors);
			config.do_stabilization = input.tag.get_property("DO_STABILIZATION", config.do_stabilization);
			config.block_size = input.tag.get_property("BLOCK_SIZE", config.block_size);
			config.search_radius = input.tag.get_property("SEARCH_RADIUS", config.search_radius);
			config.settling_speed = input.tag.get_property("SETTLING_SPEED", config.settling_speed);
			config.limits();
		}
		else if( input.tag.title_is("/FLOWOBJ") )
			result = 1;
	}
}

void FlowObj::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	thread->window->lock_window("FlowObj::update_gui");
	FlowObjWindow *window = (FlowObjWindow*)thread->window;

	window->vectors->update(config.draw_vectors);
	window->do_stabilization->update(config.do_stabilization);
	window->block_size->update(config.block_size);
	window->search_radius->update(config.search_radius);
	window->settling_speed->update(config.settling_speed);

	thread->window->unlock_window();
}

void FlowObj::to_mat(Mat &mat, int mcols, int mrows,
	VFrame *inp, int ix,int iy, int mcolor_model)
{
	int mcomp = BC_CModels::components(mcolor_model);
	int mbpp = BC_CModels::calculate_pixelsize(mcolor_model);
	int psz = mbpp / mcomp;
	int mdepth = psz < 2 ? CV_8U : psz < 4 ? CV_16U : CV_32F;
	if( mat.dims != 2 || mat.depth() != mdepth || mat.channels() != mcomp ||
	    mat.cols != mcols || mat.rows != mrows ) {
		mat.release();
	}
	if( mat.empty() ) {
		int type = CV_MAKETYPE(mdepth, mcomp);
		mat.create(mrows, mcols, type);
	}
	uint8_t *mat_rows[mrows];
	for( int y=0; y<mrows; ++y ) mat_rows[y] = mat.ptr(y);
	uint8_t **inp_rows = inp->get_rows();
	int ibpl = inp->get_bytes_per_line(), mbpl = mcols * mbpp;
	int icolor_model = inp->get_color_model();
	BC_CModels::transfer(mat_rows, mcolor_model, 0,0, mcols,mrows, mbpl,
		inp_rows, icolor_model, ix,iy, mcols,mrows, ibpl, 0);
//	VFrame vfrm(mat_rows[0], -1, mcols,mrows, mcolor_model, mat_rows[1]-mat_rows[0]);
//	static int vfrm_no = 0; char vfn[64]; sprintf(vfn,"/tmp/idat/%06d.png", vfrm_no++);
//	vfrm.write_png(vfn);
}

void FlowObj::from_mat(VFrame *out, int ox, int oy, int ow, int oh, Mat &mat, int mcolor_model)
{
	int mbpp = BC_CModels::calculate_pixelsize(mcolor_model);
	int mrows = mat.rows, mcols = mat.cols;
	uint8_t *mat_rows[mrows];
	for( int y=0; y<mrows; ++y ) mat_rows[y] = mat.ptr(y);
	uint8_t **out_rows = out->get_rows();
	int obpl = out->get_bytes_per_line(), mbpl = mcols * mbpp;
	int ocolor_model = out->get_color_model();
	BC_CModels::transfer(out_rows, ocolor_model, ox,oy, ow,oh, obpl,
		mat_rows, mcolor_model, 0,0, mcols,mrows, mbpl,  0);
//	static int vfrm_no = 0; char vfn[64]; sprintf(vfn,"/tmp/odat/%06d.png", vfrm_no++);
//	out->write_png(vfn);
}


int FlowObj::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
{

	//int need_reconfigure =
	load_configuration();
	input = get_input(0);
	output = get_output(0);
	width = input->get_w();
	height = input->get_h();
	color_model = input->get_color_model();

	if( accum_matrix.empty() ) {
		accum_matrix = Mat::eye(3,3, CV_64F);
	}
	if( !prev_corners ) prev_corners = new ptV();
	if( !next_corners ) next_corners = new ptV();

// Get the position of previous reference frame.
	int64_t actual_previous_number = start_position;
	int skip_current = 0;
	if( get_direction() == PLAY_REVERSE ) {
		if( ++actual_previous_number < get_source_start() + get_total_len() ) {
			KeyFrame *keyframe = get_next_keyframe(start_position, 1);
			if( keyframe->position > 0 &&
			    actual_previous_number >= keyframe->position )
				skip_current = 1;
		}
		else
			skip_current = 1;
	}
	else {
		if( --actual_previous_number >= get_source_start() ) {
			KeyFrame *keyframe = get_prev_keyframe(start_position, 1);
			if( keyframe->position > 0 &&
			    actual_previous_number < keyframe->position)
				skip_current = 1;
		}
		else
			skip_current = 1;
	}
	

// move currrent image to previous position
	if( next_position >= 0 && next_position == actual_previous_number ) {
		Mat mat = prev_mat;  prev_mat = next_mat;  next_mat = mat;
		ptV *pts = prev_corners;  prev_corners = next_corners;  next_corners = pts;
		prev_position = next_position;
	}
	else
// load previous image
	if( actual_previous_number >= 0 ) {
		read_frame(input, 0, actual_previous_number, frame_rate, 0);
		to_mat(prev_mat, width,height, input, 0,0, BC_GREY8);
	}

	if( skip_current || prev_position != actual_previous_number ) {
		skip_current = 1;
		accum_matrix = Mat::eye(3,3, CV_64F);
	}


// load next image
	next_position = start_position;
	VFrame *iframe = new_temp(width,height, color_model);
	read_frame(iframe, 0, start_position, frame_rate, 0);
	input = iframe;
	to_mat(next_mat, width,height, iframe, 0,0, BC_GREY8);

	int corner_count = MAX_COUNT;
	int block_size = config.block_size;
	int min_distance = config.search_radius;

	goodFeaturesToTrack(next_mat,
		*next_corners, corner_count, 0.01,        // quality_level
		min_distance, noArray(), block_size,
		false,       // use_harris
		0.04);       // k

	if( !next_mat.empty() && next_corners->size() > 3 ) {
		cornerSubPix(next_mat, *next_corners, Size(WIN_SIZE, WIN_SIZE), Size(-1,-1),
			cvTermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.03));
	}

	Mat flow;
	if( !prev_mat.empty() && prev_corners->size() > 3 ) {
// optical flow
		calcOpticalFlowFarneback(prev_mat, next_mat, flow, 0.5, 3, 15, 3, 5, 1.2, 0);
	}

	if( config.do_stabilization && !flow.empty() ) {
		if( !flow_engine )
			flow_engine = new FlowObjRemapEngine(this, PluginClient::smp);
		flow_mat = &flow;
		flow_engine->process_packages();
	}
	else
		output->copy_from(input);

	if( config.settling_speed > 0 ) {
		if( accum && (accum->get_color_model() != color_model ||
		    accum->get_w() != width || accum->get_h() != height) ) {
			delete accum;  accum = 0;
		}
		if( !accum ) {
			accum = new VFrame(width, height, color_model, 0);
			accum->clear_frame();
		}
		if( !overlay )
			overlay = new OverlayFrame(PluginClient::smp + 1);
		float alpha = 1 - config.settling_speed/100.;
		overlay->overlay(accum, output,
			0,0, width,height, 0,0, width, height,
       			alpha, TRANSFER_NORMAL, NEAREST_NEIGHBOR);
		output->copy_from(accum);
	}

	if( !flow.empty() && config.draw_vectors ) {
		output->set_pixel_color(GREEN);
		int nv = 24; float s = 2;
		int gw = width/nv + 1, gh = height/nv + 1;
		int sz = bmin(width,height) / 200 + 4;
		for( int y=gh/2; y<height; y+=gh ) {
			Point2f *row = (Point2f *)flow.ptr(y);
			for( int x=gw/2; x<width; x+=gw ) {
				Point2f *ds = row + x;
				float x0 = x+.5, x1 = x+s*ds->x, y0 = y+.5, y1 = y+s*ds->y;
				output->draw_arrow(x0,y0, x1,y1, sz);
			}
		}
	}

	return 0;
}


FlowObjRemapPackage::FlowObjRemapPackage()
 : LoadPackage()
{
}

FlowObjRemapUnit::FlowObjRemapUnit(FlowObjRemapEngine *engine, FlowObj *plugin)
 : LoadClient(engine)
{
        this->plugin = plugin;
}

FlowObjRemapUnit::~FlowObjRemapUnit()
{
}

void FlowObjRemapUnit::process_package(LoadPackage *package)
{
	FlowObjRemapPackage *pkg = (FlowObjRemapPackage*)package;
	process_remap(pkg);
}

void FlowObjRemapUnit::process_remap(FlowObjRemapPackage *pkg)
{
	int row1 = pkg->row1;
	int row2 = pkg->row2;
	int w = plugin->width, h = plugin->height;
	int w1 = w-1, h1 = h-1;
	int color_model = plugin->color_model;
	int bpp = BC_CModels::calculate_pixelsize(color_model);

#define FLOW_OBJ_REMAP(type, components) { \
	unsigned char **in_rows = plugin->input->get_rows(); \
	type **out_rows = (type**)plugin->output->get_rows(); \
 \
        for( int y=row1; y<row2; ++y ) { \
		Point2f *ipt = (Point2f *)plugin->flow_mat->ptr(y); \
		type *out_row = out_rows[y]; \
                for( int x=0; x<w; ++x,++ipt ) { \
			int ix = x - ipt->x, iy = y - ipt->y; \
			bclamp(ix, 0, w1);  bclamp(iy, 0, h1); \
			type *inp_row = (type *)(in_rows[iy] + ix*bpp); \
			for( int i=0; i<components; ++i ) \
				*out_row++ = *inp_row++; \
		} \
	} \
}
	switch( color_model ) {
	case BC_RGB888:     FLOW_OBJ_REMAP(unsigned char, 3);  break;
	case BC_RGBA8888:   FLOW_OBJ_REMAP(unsigned char, 4);  break;
	case BC_RGB_FLOAT:  FLOW_OBJ_REMAP(float, 3);          break;
	case BC_RGBA_FLOAT: FLOW_OBJ_REMAP(float, 4);          break;
	case BC_YUV888:     FLOW_OBJ_REMAP(unsigned char, 3);  break;
	case BC_YUVA8888:   FLOW_OBJ_REMAP(unsigned char, 4);  break;
	}
}


FlowObjRemapEngine::FlowObjRemapEngine(FlowObj *plugin, int cpus)
 : LoadServer(cpus+1, cpus+1)
{
	this->plugin = plugin;
}

FlowObjRemapEngine::~FlowObjRemapEngine()
{
}

void FlowObjRemapEngine::init_packages()
{
	int row1 = 0, row2 = 0, n = LoadServer::get_total_packages();
	for( int i=0; i<n; row1=row2 ) {
		FlowObjRemapPackage *package = (FlowObjRemapPackage*)LoadServer::get_package(i);
		row2 = plugin->get_input()->get_h() * ++i / n;
		package->row1 = row1;  package->row2 = row2;
	}
}

LoadClient* FlowObjRemapEngine::new_client()
{
	return new FlowObjRemapUnit(this, plugin);
}

LoadPackage* FlowObjRemapEngine::new_package()
{
	return new FlowObjRemapPackage();
}

