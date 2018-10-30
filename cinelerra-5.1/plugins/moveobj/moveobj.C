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

#include "affine.h"
#include "clip.h"
#include "filexml.h"
#include "moveobj.h"
#include "moveobjwindow.h"
#include "language.h"
#include "transportque.inc"

REGISTER_PLUGIN(MoveObj)

#define MAX_COUNT 250
#define WIN_SIZE 20

MoveObjConfig::MoveObjConfig()
{
	draw_vectors = 0;
	do_stabilization = 1;
	block_size = 20;
	search_radius = 10;
	settling_speed = 5;
}

int MoveObjConfig::equivalent(MoveObjConfig &that)
{
	return draw_vectors == that.draw_vectors &&
		do_stabilization == that.do_stabilization &&
		block_size == that.block_size &&
		search_radius == that.search_radius &&
		settling_speed == that.settling_speed;
}

void MoveObjConfig::copy_from(MoveObjConfig &that)
{
	draw_vectors = that.draw_vectors;
	do_stabilization = that.do_stabilization;
	block_size = that.block_size;
	search_radius = that.search_radius;
	settling_speed = that.settling_speed;
}

void MoveObjConfig::interpolate( MoveObjConfig &prev, MoveObjConfig &next, 
	long prev_frame, long next_frame, long current_frame)
{
	copy_from(next);
}

void MoveObjConfig::limits()
{
	bclamp(block_size, 5, 100);
	bclamp(search_radius, 1, 100);
	bclamp(settling_speed, 0, 100);
}


MoveObj::MoveObj(PluginServer *server)
 : PluginVClient(server)
{
	affine = 0;
	prev_position = next_position = -1;
	x_accum = y_accum = 0;
	angle_accum = 0;
	prev_corners = 0;
	next_corners = 0;
}

MoveObj::~MoveObj()
{
	delete affine;
	delete prev_corners;
	delete next_corners;
}

const char* MoveObj::plugin_title() { return N_("MoveObj"); }
int MoveObj::is_realtime() { return 1; }

NEW_WINDOW_MACRO(MoveObj, MoveObjWindow);
LOAD_CONFIGURATION_MACRO(MoveObj, MoveObjConfig)

void MoveObj::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("MOVEOBJ");
	output.tag.set_property("DRAW_VECTORS", config.draw_vectors);
	output.tag.set_property("DO_STABILIZATION", config.do_stabilization);
	output.tag.set_property("BLOCK_SIZE", config.block_size);
	output.tag.set_property("SEARCH_RADIUS", config.search_radius);
	output.tag.set_property("SETTLING_SPEED", config.settling_speed);
	output.append_tag();
	output.append_newline();
	output.tag.set_title("/MOVEOBJ");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void MoveObj::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("MOVEOBJ") ) {
			config.draw_vectors = input.tag.get_property("DRAW_VECTORS", config.draw_vectors);
			config.do_stabilization = input.tag.get_property("DO_STABILIZATION", config.do_stabilization);
			config.block_size = input.tag.get_property("BLOCK_SIZE", config.block_size);
			config.search_radius = input.tag.get_property("SEARCH_RADIUS", config.search_radius);
			config.settling_speed = input.tag.get_property("SETTLING_SPEED", config.settling_speed);
			config.limits();
		}
		else if( input.tag.title_is("/MOVEOBJ") )
			result = 1;
	}
}

void MoveObj::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	thread->window->lock_window("MoveObj::update_gui");
	MoveObjWindow *window = (MoveObjWindow*)thread->window;

	window->vectors->update(config.draw_vectors);
	window->do_stabilization->update(config.do_stabilization);
	window->block_size->update(config.block_size);
	window->search_radius->update(config.search_radius);
	window->settling_speed->update(config.settling_speed);

	thread->window->unlock_window();
}

void MoveObj::to_mat(Mat &mat, int mcols, int mrows,
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
	int ibpl = inp->get_bytes_per_line(), obpl = mcols * mbpp;
	int icolor_model = inp->get_color_model();
	BC_CModels::transfer(mat_rows, mcolor_model, 0,0, mcols,mrows, obpl,
		inp_rows, icolor_model, ix,iy, mcols,mrows, ibpl, 0);
//	VFrame vfrm(mat_rows[0], -1, mcols,mrows, mcolor_model, mat_rows[1]-mat_rows[0]);
//	static int vfrm_no = 0; char vfn[64]; sprintf(vfn,"/tmp/dat/%06d.png", vfrm_no++);
//	vfrm.write_png(vfn);
}

int MoveObj::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
{

	//int need_reconfigure =
	load_configuration();
	VFrame *input = get_input(0), *output = get_output(0);
	int w = input->get_w(), h = input->get_h();
	int color_model = input->get_color_model();

	if( accum_matrix.empty() ) {
		accum_matrix = Mat::eye(3,3, CV_64F);
	}
	if( !affine ) {
		int cpus1 = PluginClient::get_project_smp() + 1;
		affine = new AffineEngine(cpus1, cpus1);
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
		to_mat(prev_mat, w,h, input, 0,0, BC_GREY8);
	}

	if( skip_current || prev_position != actual_previous_number ) {
		skip_current = 1;
		accum_matrix = Mat::eye(3,3, CV_64F);
	}


// load next image
	next_position = start_position;
	VFrame *iframe = !config.do_stabilization ? input : new_temp(w,h, color_model);
	read_frame(iframe, 0, start_position, frame_rate, 0);
	to_mat(next_mat, w,h, iframe, 0,0, BC_GREY8);

	int corner_count = MAX_COUNT;
	int block_size = config.block_size;
	int min_distance = config.search_radius;

	goodFeaturesToTrack(next_mat,
		*next_corners, corner_count, 0.01,        // quality_level
		min_distance, noArray(), block_size,
		false,       // use_harris
		0.04);       // k

	ptV pt1, pt2;
	if( !next_mat.empty() && next_corners->size() > 3 ) {
		cornerSubPix(next_mat, *next_corners, Size(WIN_SIZE, WIN_SIZE), Size(-1,-1),
			cvTermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.03));
	}
	if( !prev_mat.empty() && prev_corners->size() > 3 ) {
// optical flow
		Mat st, err;
		ptV &prev = *prev_corners, &next = *next_corners;
		calcOpticalFlowPyrLK(prev_mat, next_mat, prev, next,
			st, err, Size(WIN_SIZE, WIN_SIZE), 5, 
			cvTermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.3), 0);
		float fails = 0.5 * w + 1;
		uint8_t *stp = st.ptr<uint8_t>();
		float *errp = err.ptr<float>();
		for( int i=0,n=next_corners->size(); i<n; ++i ) {
			if( stp[i] == 0 || errp[i] > fails ) continue;
			pt1.push_back(next[i]);  
			pt2.push_back(prev[i]);
		}
	}

	int points = pt1.size();
	if( points > 0 && !skip_current ) {
		if( config.draw_vectors ) {
			int sz = bmin(w,h) / 222 + 2;
			for( int i = 0; i < points; ++i )
				iframe->draw_arrow(pt1[i].x,pt1[i].y, pt2[i].x,pt2[i].y, sz);
		}
#ifdef _RANSAC
// ransac
		int ninliers = 0;
		Mat_<float> translationM =
			estimateGlobalMotionRansac(pt1, pt2, MM_TRANSLATION, 
				RansacParams::default2dMotion(MM_TRANSLATION),
				0, &ninliers);
		Mat_<float> rotationM =
			estimateGlobalMotionRansac(pt1, pt2, MM_ROTATION, 
				RansacParams::default2dMotion(MM_ROTATION),
				0, &ninliers);

		double temp[9];
		Mat temp_matrix = Mat(3, 3, CV_64F, temp);
		for( int i=0; i<9; ++i )
			temp[i] = i == 2 || i == 5 ?
				translationM(i / 3, i % 3) :
				rotationM(i / 3, i % 3);
		accum_matrix = temp_matrix * accum_matrix;
#else
// homography

		Mat M1(1, points, CV_32FC2, &pt1[0].x);  
		Mat M2(1, points, CV_32FC2, &pt2[0].x);  

//M2 = H*M1 , old = H*current  
		Mat H = findHomography(M1, M2, CV_RANSAC, 2);
		if( !H.dims || !H.rows || !H.cols )
			printf("MoveObj::process_buffer %d: Find Homography Fail!\n", __LINE__);  
		else
			accum_matrix = H * accum_matrix;
#endif
	}

	double *amat = accum_matrix.ptr<double>();
// deglitch
//	if( EQUIV(amat[0], 0) )	{
//printf("MoveObj::process_buffer %d\n", __LINE__);
//		accum_matrix = Mat::eye(3,3, CV_64F);
// 	}

	if( config.do_stabilization ) {
		Mat identity = Mat::eye(3,3, CV_64F);
 		double w0 = config.settling_speed/100., w1 = 1.-w0;
// interpolate with identity matrix
		accum_matrix = w0*identity + w1*accum_matrix;

		AffineMatrix &matrix = affine->matrix;
		for( int i=0,k=0; i<3; ++i )
			for( int j=0; j<3; ++j )
				matrix.values[i][j] = amat[k++];

//printf("MoveObj::process_buffer %d %jd matrix=\n", __LINE__, start_position);
//matrix.dump();

// iframe is always temp, if we get here
		output->clear_frame();
		affine->process(output, iframe, 0, AffineEngine::TRANSFORM,
			0,0, w,0, w,h, 0,h, 1);
	}

	return 0;
}
