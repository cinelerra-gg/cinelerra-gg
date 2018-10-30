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
#include "puzzleobj.h"
#include "puzzleobjwindow.h"
#include "language.h"
#include "mutex.h"

REGISTER_PLUGIN(PuzzleObj)


PuzzleObjConfig::PuzzleObjConfig()
{
	pixels = 400;
	iterations = 10;
}

int PuzzleObjConfig::equivalent(PuzzleObjConfig &that)
{
	return 1;
}

void PuzzleObjConfig::copy_from(PuzzleObjConfig &that)
{
}

void PuzzleObjConfig::interpolate( PuzzleObjConfig &prev, PuzzleObjConfig &next, 
	long prev_frame, long next_frame, long current_frame)
{
	copy_from(next);
}

void PuzzleObjConfig::limits()
{
}


PuzzleObj::PuzzleObj(PluginServer *server)
 : PluginVClient(server)
{
	ss_width = 0;  ss_height = 0;  ss_channels = 0;
	ss_pixels = 0; ss_levels = 0;  ss_prior = 0;
	ss_hist_bins = 0;
}

PuzzleObj::~PuzzleObj()
{
}

const char* PuzzleObj::plugin_title() { return N_("PuzzleObj"); }
int PuzzleObj::is_realtime() { return 1; }

NEW_WINDOW_MACRO(PuzzleObj, PuzzleObjWindow);
LOAD_CONFIGURATION_MACRO(PuzzleObj, PuzzleObjConfig)

void PuzzleObj::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("PUZZLEOBJ");
	output.tag.set_property("PIXELS", config.pixels);
	output.tag.set_property("ITERATIONS", config.iterations);
	output.append_tag();
	output.append_newline();
	output.tag.set_title("/PUZZLEOBJ");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void PuzzleObj::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("PUZZLEOBJ") ) {
			config.pixels     = input.tag.get_property("PIXELS", config.pixels);
			config.iterations = input.tag.get_property("ITERATIONS", config.iterations);
			config.limits();
		}
		else if( input.tag.title_is("/PUZZLEOBJ") )
			result = 1;
	}
}

void PuzzleObj::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	thread->window->lock_window("PuzzleObj::update_gui");
	PuzzleObjWindow *window = (PuzzleObjWindow*)thread->window;
	window->unlock_window();
}

void PuzzleObj::to_mat(Mat &mat, int mcols, int mrows,
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

int PuzzleObj::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
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
	int prior = 2;
	int levels = 4;
	int hist_bins = 5;
	int channels = BC_CModels::components(cv_color_model);

// load next image
	to_mat(next_img, width,height, iframe, 0,0, cv_color_model);
	Mat img; //, img1;
	cvtColor(next_img, img, COLOR_RGB2HSV);
	if( !sseeds.empty() &&
	    ( ss_width != width || ss_height != height || ss_channels != channels ||
	    ss_pixels != config.pixels || ss_levels != levels || ss_prior != prior ||
	    ss_hist_bins != hist_bins ) )
		sseeds.release();
	if( sseeds.empty() )
		sseeds = ximgproc::createSuperpixelSEEDS(
			ss_width=width, ss_height=height, ss_channels=channels,
	    		ss_pixels=config.pixels, ss_levels=levels, ss_prior=prior,
			ss_hist_bins=hist_bins, false);
	sseeds->iterate(img, config.iterations);
//	sseeds->getLabels(img1);
	sseeds->getLabelContourMask(img, false);
	uint8_t **irows = input->get_rows();
	int ibpp = BC_CModels::calculate_pixelsize(color_model);
	uint8_t **orows = output->get_rows();
	int obpp = BC_CModels::calculate_pixelsize(color_model);
	if( BC_CModels::is_float(color_model) ) {
		for( int y=0; y<height; ++y ) {
			uint8_t *rp = img.ptr(y);
			float *ip = (float*)irows[y], *op = (float*)orows[y];
			for( int x=0; x<width; ++x,++rp ) {
				if( *rp ) { op[0] = 1.0f; op[1] = 1.0f; op[2] = 1.0f; }
				else { op[0] = ip[0];  op[1] = ip[1];  op[2] = ip[2]; }
				ip = (float*)((char*)ip + ibpp);
				op = (float*)((char*)op + obpp);
			}
		}
	}
	else if( BC_CModels::is_yuv(color_model) ) {
		for( int y=0; y<height; ++y ) {
			uint8_t *rp = img.ptr(y), *ip = irows[y], *op = orows[y];
			for( int x=0; x<width; ++x,++rp ) {
				if( *rp ) { op[0] = 0xff; op[1] = 0x80; op[2] = 0x80; }
				else { op[0] = ip[0];  op[1] = ip[1];  op[2] = ip[2]; }
				ip += ibpp;  op += obpp;
			}
		}
	}
	else {
		for( int y=0; y<height; ++y ) {
			uint8_t *rp = img.ptr(y), *ip = irows[y], *op = orows[y];
			for( int x=0; x<width; ++x,++rp ) {
				if( *rp ) { op[0] = 0xff; op[1] = 0xff; op[2] = 0xff; }
				else { op[0] = ip[0];  op[1] = ip[1];  op[2] = ip[2]; }
				ip += ibpp;  op += obpp;
			}
		}
	}
	return 0;
}

