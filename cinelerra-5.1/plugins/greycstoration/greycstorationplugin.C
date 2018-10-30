/*
 * GreyCStoration plugin for Cinelerra
 * Copyright (C) 2013 Slock Ruddy
 * Copyright (C) 2014-2015 Nicola Ferralis <feranick at hotmail dot com>
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
#include "bccmodels.h"
#include "bchash.h"
#include "filexml.h"
#include "greycstorationplugin.h"
#include "greycstorationwindow.h"
#include "language.h"
#include "vframe.h"


#include <stdint.h>
#include <string.h>

#define cimg_plugin "greycstoration.h"
#include "CImg.h"

using namespace cimg_library;

REGISTER_PLUGIN(GreyCStorationMain)


GreyCStorationConfig::GreyCStorationConfig()
{
	amplitude    = 40.0f; //cimg_option("-dt",40.0f,"Regularization strength for one iteration (>=0)");
	sharpness    = 0.8f; // cimg_option("-p",0.8f,"Contour preservation for regularization (>=0)");
	anisotropy   = 0.8f; // cimg_option("-a",0.8f,"Regularization anisotropy (0<=a<=1)");
	noise_scale = 0.8f; // const float alpha = 0.6f; // cimg_option("-alpha",0.6f,"Noise scale(>=0)");
}

void GreyCStorationConfig::copy_from(GreyCStorationConfig &that)
{
	amplitude = that.amplitude;
	sharpness = that.sharpness;
	anisotropy = that.anisotropy;
	noise_scale = that.noise_scale;
}

int GreyCStorationConfig::equivalent(GreyCStorationConfig &that)
{
	return
		anisotropy == that.anisotropy &&
		sharpness == that.sharpness &&
		noise_scale == that.noise_scale &&
		amplitude == that.amplitude;
}

void GreyCStorationConfig::interpolate(GreyCStorationConfig &prev,
	GreyCStorationConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->amplitude = prev.amplitude * prev_scale + next.amplitude * next_scale;
	this->sharpness = prev.sharpness * prev_scale + next.sharpness * next_scale;
	this->anisotropy = prev.anisotropy * prev_scale + next.anisotropy * next_scale;
	this->noise_scale = prev.noise_scale * prev_scale + next.noise_scale * next_scale;
}



GreyCStorationMain::GreyCStorationMain(PluginServer *server)
 : PluginVClient(server)
{
	alpha = 0;
	alpha_size = 0;
}

GreyCStorationMain::~GreyCStorationMain()
{
	delete [] alpha;
}

const char* GreyCStorationMain::plugin_title() { return N_("GreyCStoration"); }
int GreyCStorationMain::is_realtime() { return 1; }


void GreyCStorationMain::
GREYCSTORATION(VFrame *frame)
{
	int w = frame->get_w(), h = frame->get_h();
	CImg<float> image(w,h,1,3);
	int frm_colormodel = frame->get_color_model();
	int components = BC_CModels::components(frm_colormodel);
	uint8_t *frm[components], **frmp = frm;
	if( BC_CModels::is_planar(frm_colormodel) ) {
		frmp[0] = frame->get_y();
		frmp[1] = frame->get_u();
		frmp[2] = frame->get_v();
	}
	else
		frmp = frame->get_rows();
	uint8_t *img[components], **imgp = img;
	for( int i=0; i<3; ++i )
		imgp[i] = (uint8_t *)image.data(0,0,0,i);
	int img_colormodel = BC_RGB_FLOATP;
	if( BC_CModels::has_alpha(frm_colormodel) ) {
		int sz = w*h*components * sizeof(float);
		if( sz != alpha_size ) { delete [] alpha; alpha = 0; }
		if( !alpha ) { alpha = new uint8_t[alpha_size = sz]; }
		imgp[3] = alpha;
		img_colormodel = BC_RGBA_FLOATP;
	}
	BC_CModels::transfer(
                imgp, img_colormodel, 0,0, w,h, w,
		frmp, frm_colormodel, 0,0, w,h, w, 0);

//cimg_option("-iter",3,"Number of regularization iterations (>0)");
//	const unsigned int nb_iter  = 1;
// cimg_option("-sigma",1.1f,"Geometry regularity (>=0)");
	const float sigma           = 1.1f;
// cimg_option("-fast",true,"Use fast approximation for regularization (0 or 1)");
	const bool fast_approx      = false;
// cimg_option("-prec",2.0f,"Precision of the gaussian function for regularization (>0)");
	const float gauss_prec      = 2.0f;
// cimg_option("-dl",0.8f,"Spatial integration step for regularization (0<=dl<=1)");
	const float dl              = 0.8f;
// cimg_option("-da",30.0f,"Angular integration step for regulatization (0<=da<=90)");
	const float da              = 30.0f;
// cimg_option("-interp",0,"Interpolation type (0=Nearest-neighbor, 1=Linear, 2=Runge-Kutta)");
	const unsigned int interp   = 0;
// cimg_option("-tile",0,"Use tiled mode (reduce memory usage");
	const unsigned int tile     = 0;
// cimg_option("-btile",4,"Size of tile overlapping regions");
	const unsigned int btile    = 4;
//cimg_option("-threads",1,"Number of threads used");
	const unsigned int threads  = 0;

	image.greycstoration_run(config.amplitude, config.sharpness,
		config.anisotropy, config.noise_scale, sigma, 1.0f,
		dl, da, gauss_prec, interp, fast_approx, tile, btile,
		threads);

	BC_CModels::transfer(
		frmp, frm_colormodel, 0,0, w,h, w,
                imgp, img_colormodel, 0,0, w,h, w, 0);
}

int GreyCStorationMain::process_buffer(VFrame *frame,
		int64_t start_position, double frame_rate)
{
	load_configuration();
	read_frame(frame, 0, get_source_position(), get_framerate(), get_use_opengl());
	GREYCSTORATION(frame);
	return 0;
}


NEW_WINDOW_MACRO(GreyCStorationMain, GreyCStorationWindow)
LOAD_CONFIGURATION_MACRO(GreyCStorationMain, GreyCStorationConfig)

void GreyCStorationMain::update_gui()
{
	if(thread) {
		load_configuration();
		GreyCStorationWindow *wdw = (GreyCStorationWindow*)thread->window;
		wdw->lock_window();
		wdw->greycamp_slider->update((int)config.amplitude);
		wdw->greycsharp_slider->update(config.sharpness);
		wdw->greycani_slider->update(config.anisotropy);
		wdw->greycnoise_slider->update(config.noise_scale);
		wdw->unlock_window();
	}
}

// these will end up into your project file (xml)
void GreyCStorationMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("GREYCSTORATION");

	output.tag.set_property("AMPLITUDE", config.amplitude);
	output.tag.set_property("SHARPNESS", config.sharpness);
	output.tag.set_property("ANISOTROPHY", config.anisotropy);
	output.tag.set_property("NOISE_SCALE", config.noise_scale);
	output.append_tag();

	output.tag.set_title("/GREYCSTORATION");
	output.append_tag();
	output.terminate_string();
// data is now in *text
}

void GreyCStorationMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	while( !input.read_tag() ) {
		if(input.tag.title_is("GREYCSTORATION")) {
			config.amplitude = input.tag.get_property("AMPLITUDE", config.amplitude);
			config.sharpness = input.tag.get_property("SHARPNESS", config.sharpness);
			config.anisotropy = input.tag.get_property("ANISOTROPHY", config.anisotropy);
			config.noise_scale = input.tag.get_property("NOISE_SCALE", config.noise_scale);
		}
	}
}

