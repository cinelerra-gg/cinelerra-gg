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
#include "gaborobj.h"
#include "gaborobjwindow.h"
#include "language.h"
#include "mutex.h"

REGISTER_PLUGIN(GaborObj)


GaborObjConfig::GaborObjConfig()
{
}

int GaborObjConfig::equivalent(GaborObjConfig &that)
{
	return 1;
}

void GaborObjConfig::copy_from(GaborObjConfig &that)
{
}

void GaborObjConfig::interpolate( GaborObjConfig &prev, GaborObjConfig &next, 
	long prev_frame, long next_frame, long current_frame)
{
	copy_from(next);
}

void GaborObjConfig::limits()
{
}


GaborObj::GaborObj(PluginServer *server)
 : PluginVClient(server)
{
	int steps = 16;
	Size ksize(31, 31);
	for( int i=0; i<steps; ++i ) {
		double theta = i * M_PI/steps;
		Mat kern = getGaborKernel(ksize, 4.0, theta, 10.0, 0.5, 0, CV_32F);
		float sum = 0;
		for( int k=0; k<kern.rows; ++k ) {
			float *kp = kern.ptr<float>(k);
			for( int j=0; j<kern.cols; ++j ) sum += *kp++;
		}
		kern /= 1.5*sum;
		filters.push_back(kern);
	}
	gabor_engine = 0;
	remap_lock = new ::Mutex("GaborObj::remap_lock");
}

GaborObj::~GaborObj()
{
	delete gabor_engine;
	delete remap_lock;
}

const char* GaborObj::plugin_title() { return N_("GaborObj"); }
int GaborObj::is_realtime() { return 1; }

NEW_WINDOW_MACRO(GaborObj, GaborObjWindow);
LOAD_CONFIGURATION_MACRO(GaborObj, GaborObjConfig)

void GaborObj::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("GABOROBJ");
	output.append_tag();
	output.append_newline();
	output.tag.set_title("/GABOROBJ");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void GaborObj::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("GABOROBJ") ) {
			config.limits();
		}
		else if( input.tag.title_is("/GABOROBJ") )
			result = 1;
	}
}

void GaborObj::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	thread->window->lock_window("GaborObj::update_gui");
	GaborObjWindow *window = (GaborObjWindow*)thread->window;
	window->unlock_window();
}

void GaborObj::to_mat(Mat &mat, int mcols, int mrows,
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

void GaborObj::from_mat(VFrame *out, int ox, int oy, int ow, int oh, Mat &mat, int mcolor_model)
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


int GaborObj::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
{

	//int need_reconfigure =
	load_configuration();
	input = get_input(0);
	output = get_output(0);
	width = input->get_w();
	height = input->get_h();
	color_model = input->get_color_model();

// load next image
	VFrame *iframe = new_temp(width,height, color_model);
	read_frame(iframe, 0, start_position, frame_rate, 0);
	input = iframe;
	to_mat(next_img, width,height, iframe, 0,0, color_model);
	if( !gabor_engine )
		gabor_engine = new GaborObjFilterEngine(this, PluginClient::smp + 1);

	output->clear_frame();
	gabor_engine->process_packages();
	return 0;
}


GaborObjFilterPackage::GaborObjFilterPackage()
 : LoadPackage()
{
}

GaborObjFilterUnit::GaborObjFilterUnit(GaborObjFilterEngine *engine, GaborObj *plugin)
 : LoadClient(engine)
{
        this->plugin = plugin;
}

GaborObjFilterUnit::~GaborObjFilterUnit()
{
}

void GaborObjFilterUnit::process_package(LoadPackage *package)
{
	GaborObjFilterPackage *pkg = (GaborObjFilterPackage*)package;
	process_filter(pkg);
}

void GaborObjFilterUnit::process_filter(GaborObjFilterPackage *pkg)
{
	int color_model = plugin->color_model;
	int w = plugin->output->get_w(), h = plugin->output->get_h();
	int ddepth = CV_8UC3;

	switch( color_model ) {
	case BC_RGB888:     ddepth = CV_8UC3;   break;
	case BC_RGBA8888:   ddepth = CV_8UC4;   break;
	case BC_RGB_FLOAT:  ddepth = CV_32FC3;  break;
	case BC_RGBA_FLOAT: ddepth = CV_32FC4;  break;
	case BC_YUV888:     ddepth = CV_8UC3;   break;
	case BC_YUVA8888:   ddepth = CV_8UC4;   break;
	}

	Mat &src = plugin->next_img, dst;
	Mat &kern = plugin->filters[pkg->i];
	filter2D(src, dst, ddepth, kern);

#define GABOR_OBJ_REMAP(type, components) { \
	type **out_rows = (type**)plugin->output->get_rows(); \
 \
        for( int y=0; y<h; ++y ) { \
		type *out_row = out_rows[y]; \
		type *dst_row = (type *)dst.ptr<type>(y); \
                for( int x=0; x<w; ++x ) { \
			for( int i=0; i<components; ++i ) { \
				*out_row = bmax(*out_row, *dst_row); \
				++out_row;  ++dst_row; \
			} \
		} \
	} \
}
// technically, this is needed... but it really can slow it down.
//	plugin->remap_lock->lock("GaborObjFilterUnit::process_filter");
	switch( color_model ) {
	case BC_RGB888:     GABOR_OBJ_REMAP(unsigned char, 3);  break;
	case BC_RGBA8888:   GABOR_OBJ_REMAP(unsigned char, 4);  break;
	case BC_RGB_FLOAT:  GABOR_OBJ_REMAP(float, 3);          break;
	case BC_RGBA_FLOAT: GABOR_OBJ_REMAP(float, 4);          break;
	case BC_YUV888:     GABOR_OBJ_REMAP(unsigned char, 3);  break;
	case BC_YUVA8888:   GABOR_OBJ_REMAP(unsigned char, 4);  break;
	}
//	plugin->remap_lock->unlock();
}


GaborObjFilterEngine::GaborObjFilterEngine(GaborObj *plugin, int cpus)
 : LoadServer(cpus+1, plugin->filters.size())
{
	this->plugin = plugin;
}

GaborObjFilterEngine::~GaborObjFilterEngine()
{
}

void GaborObjFilterEngine::init_packages()
{
	int n = LoadServer::get_total_packages();
	for( int i=0; i<n; ++i ) {
		GaborObjFilterPackage *pkg = (GaborObjFilterPackage*)LoadServer::get_package(i);
		pkg->i = i;
	}
}

LoadClient* GaborObjFilterEngine::new_client()
{
	return new GaborObjFilterUnit(this, plugin);
}

LoadPackage* GaborObjFilterEngine::new_package()
{
	return new GaborObjFilterPackage();
}

