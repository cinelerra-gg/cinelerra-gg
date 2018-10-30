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
#include "stylizeobj.h"
#include "stylizeobjwindow.h"
#include "language.h"
#include "mutex.h"

REGISTER_PLUGIN(StylizeObj)


StylizeObjConfig::StylizeObjConfig()
{
	mode = MODE_EDGE_SMOOTH;
	smoothing = 10;
	edge_strength = 25;
	shade_factor = 40;
}

int StylizeObjConfig::equivalent(StylizeObjConfig &that)
{
	return 1;
}

void StylizeObjConfig::copy_from(StylizeObjConfig &that)
{
}

void StylizeObjConfig::interpolate( StylizeObjConfig &prev, StylizeObjConfig &next, 
	long prev_frame, long next_frame, long current_frame)
{
	copy_from(next);
}

void StylizeObjConfig::limits()
{
}


StylizeObj::StylizeObj(PluginServer *server)
 : PluginVClient(server)
{
}

StylizeObj::~StylizeObj()
{
}

const char* StylizeObj::plugin_title() { return N_("StylizeObj"); }
int StylizeObj::is_realtime() { return 1; }

NEW_WINDOW_MACRO(StylizeObj, StylizeObjWindow);
LOAD_CONFIGURATION_MACRO(StylizeObj, StylizeObjConfig)

void StylizeObj::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("STYLIZEOBJ");
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("SMOOTHING", config.smoothing);
	output.tag.set_property("EDGE_STRENGTH", config.edge_strength);
	output.tag.set_property("SHADE_FACTOR", config.shade_factor);
	output.append_tag();
	output.append_newline();
	output.tag.set_title("/STYLIZEOBJ");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void StylizeObj::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("STYLIZEOBJ") ) {
			config.mode          = input.tag.get_property("MODE", config.mode);
			config.smoothing     = input.tag.get_property("SMOOTHING", config.smoothing);
			config.edge_strength = input.tag.get_property("EDGE_STRENGTH", config.edge_strength);
			config.shade_factor  = input.tag.get_property("SHADE_FACTOR", config.shade_factor);
			config.limits();
		}
		else if( input.tag.title_is("/STYLIZEOBJ") )
			result = 1;
	}
}

void StylizeObj::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	thread->window->lock_window("StylizeObj::update_gui");
	StylizeObjWindow *window = (StylizeObjWindow*)thread->window;
	window->unlock_window();
}

void StylizeObj::to_mat(Mat &mat, int mcols, int mrows,
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

void StylizeObj::from_mat(VFrame *out, int ox, int oy, int ow, int oh, Mat &mat, int mcolor_model)
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


int StylizeObj::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
{

	//int need_reconfigure =
	load_configuration();
	input = get_input(0);
	output = get_output(0);
	width = input->get_w();
	height = input->get_h();
	color_model = input->get_color_model();
	VFrame *iframe = input;
	read_frame(iframe, 0, start_position, frame_rate, 0);
	int cv_color_model = BC_RGB888;
	if( color_model != cv_color_model ) {
		iframe = new_temp(width,height, cv_color_model);
		iframe->transfer_from(input);
	}

// load next image
	to_mat(next_img, width,height, iframe, 0,0, cv_color_model);
	output->clear_frame();
	Mat img, img1;
	switch( config.mode ) {
	case MODE_EDGE_SMOOTH:
		edgePreservingFilter(next_img,img,1); // normalized conv filter
		break;
	case MODE_EDGE_RECURSIVE:
		edgePreservingFilter(next_img,img,2); // recursive filter
		break;
	case MODE_DETAIL_ENHANCE:
		detailEnhance(next_img,img);
		break;
	case MODE_PENCIL_SKETCH: {
		float s = config.smoothing / 100;      s = s * s * 200;
		float r = config.edge_strength / 100;  r = r * r;
		float f = config.shade_factor / 100;   f = f * 0.1;
		pencilSketch(next_img,img, img1, s, r, f);
		cv_color_model = BC_GREY8;
		break; }
	case MODE_COLOR_SKETCH: {
		float s = config.smoothing / 100;      s = s * s * 200;
		float r = config.edge_strength / 100;  r = r * r;
		float f = config.shade_factor / 100;   f = f * 0.1;
		pencilSketch(next_img,img1, img, s, r, f);
		break; }
	case MODE_STYLIZATION:
		stylization(next_img,img);
		break;
	}
	from_mat(output, 0,0,width,height, img, cv_color_model);
	return 0;
}

