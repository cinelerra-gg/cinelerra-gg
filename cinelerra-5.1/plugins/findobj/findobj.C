
/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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
 */

#include "affine.h"
#include "bccolors.h"
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "findobj.h"
#include "findobjwindow.h"
#include "mutex.h"
#include "overlayframe.h"
#include "plugin.h"
#include "pluginserver.h"
#include "track.h"

#include <errno.h>
#include <exception>
#include <unistd.h>

REGISTER_PLUGIN(FindObjMain)

FindObjConfig::FindObjConfig()
{
	reset();
}

void FindObjConfig::reset()
{
	algorithm = NO_ALGORITHM;
	use_flann = 1;
	mode = MODE_QUADRILATERAL;
	draw_match = 0;
	aspect = 1;
	scale = 1;
	translate = 1;
	rotate = 1;
	draw_keypoints = 0;
	draw_scene_border = 0;
	replace_object = 0;
	draw_object_border = 0;
	draw_replace_border = 0;
	object_x = 50;  object_y = 50;
	object_w = 100; object_h = 100;
	drag_object = 0;
	scene_x = 50;   scene_y = 50;
	scene_w = 100;  scene_h = 100;
	drag_replace = 0;
	replace_x = 50;   replace_y = 50;
	replace_w = 100;  replace_h = 100;
	replace_dx = 0;   replace_dy = 0;
	drag_scene = 0;
	scene_layer = 0;
	object_layer = 1;
	replace_layer = 2;
	blend = 100;
}

void FindObjConfig::boundaries()
{
	bclamp(object_x, 0, 100);  bclamp(object_y, 0, 100);
	bclamp(object_w, 0, 100);  bclamp(object_h, 0, 100);
	bclamp(scene_x, 0, 100);   bclamp(scene_y, 0, 100);
	bclamp(scene_w, 0, 100);   bclamp(scene_h, 0, 100);
	bclamp(object_layer, MIN_LAYER, MAX_LAYER);
	bclamp(replace_layer, MIN_LAYER, MAX_LAYER);
	bclamp(scene_layer, MIN_LAYER, MAX_LAYER);
	bclamp(blend, MIN_BLEND, MAX_BLEND);
}

int FindObjConfig::equivalent(FindObjConfig &that)
{
	int result =
		algorithm == that.algorithm &&
		use_flann == that.use_flann &&
		mode == that.mode &&
		aspect == that.aspect &&
		scale == that.scale &&
		translate == that.translate &&
		rotate == that.rotate &&
		draw_keypoints == that.draw_keypoints &&
		draw_match == that.draw_match &&
		draw_scene_border == that.draw_scene_border &&
		replace_object == that.replace_object &&
		draw_object_border == that.draw_object_border &&
		draw_replace_border == that.draw_replace_border &&
		object_x == that.object_x && object_y == that.object_y &&
		object_w == that.object_w && object_h == that.object_h &&
		drag_object == that.drag_object &&
		scene_x == that.scene_x && scene_y == that.scene_y &&
		scene_w == that.scene_w && scene_h == that.scene_h &&
		drag_scene == that.drag_scene &&
		replace_x == that.replace_x && replace_y == that.replace_y &&
		replace_w == that.replace_w && replace_h == that.replace_h &&
		replace_dx == that.replace_dx && replace_dy == that.replace_dy &&
		drag_replace == that.drag_replace &&
		object_layer == that.object_layer &&
		replace_layer == that.replace_layer &&
		scene_layer == that.scene_layer &&
		blend == that.blend;
        return result;
}

void FindObjConfig::copy_from(FindObjConfig &that)
{
	algorithm = that.algorithm;
	use_flann = that.use_flann;
	mode = that.mode;
	aspect = that.aspect;
	scale = that.scale;
	translate = that.translate;
	rotate = that.rotate;
	draw_keypoints = that.draw_keypoints;
	draw_match = that.draw_match;
	draw_scene_border = that.draw_scene_border;
	replace_object = that.replace_object;
	draw_object_border = that.draw_object_border;
	draw_replace_border = that.draw_replace_border;
	object_x = that.object_x;  object_y = that.object_y;
	object_w = that.object_w;  object_h = that.object_h;
	drag_object = that.drag_object;
	scene_x = that.scene_x;    scene_y = that.scene_y;
	scene_w = that.scene_w;    scene_h = that.scene_h;
	drag_scene = that.drag_scene;
	replace_x = that.replace_x;   replace_y = that.replace_y;
	replace_w = that.replace_w;   replace_h = that.replace_h;
	replace_dx = that.replace_dx; replace_dy = that.replace_dy;
	drag_replace = that.drag_replace;
	object_layer = that.object_layer;
	replace_layer = that.replace_layer;
	scene_layer = that.scene_layer;
	blend = that.blend;
}

void FindObjConfig::interpolate(FindObjConfig &prev, FindObjConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	copy_from(prev);
}


FindObjMain::FindObjMain(PluginServer *server)
 : PluginVClient(server)
{
        affine = 0;
        overlayer = 0;

	cvmodel = BC_RGB888;
	w = h = 0;
        object = scene = replace = 0;
	object_x = object_y = 0;
	object_w = object_h = 0;
	scene_x = scene_y = 0;
	scene_w = scene_h = 0;
	object_layer = 0;
	scene_layer = 1;
	replace_layer = 2;

	match_x1 = 0;  match_y1 = 0;
	match_x2 = 0;  match_y2 = 0;
	match_x3 = 0;  match_y3 = 0;
	match_x4 = 0;  match_y4 = 0;
	shape_x1 = 0;  shape_y1 = 0;
	shape_x2 = 0;  shape_y2 = 0;
	shape_x3 = 0;  shape_y3 = 0;
	shape_x4 = 0;  shape_y4 = 0;
	out_x1 = 0;    out_y1 = 0;
	out_x2 = 0;    out_y2 = 0;
	out_x3 = 0;    out_y3 = 0;
	out_x4 = 0;    out_y4 = 0;

        init_border = 1;
}

FindObjMain::~FindObjMain()
{
	delete affine;
	delete overlayer;
}

const char* FindObjMain::plugin_title() { return N_("FindObj"); }
int FindObjMain::is_realtime() { return 1; }
int FindObjMain::is_multichannel() { return 1; }

NEW_WINDOW_MACRO(FindObjMain, FindObjWindow)
LOAD_CONFIGURATION_MACRO(FindObjMain, FindObjConfig)

void FindObjMain::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	FindObjWindow *window = (FindObjWindow*)thread->window;
	window->lock_window("FindObjMain::update_gui");
	window->update_gui();
	window->flush();
	window->unlock_window();
}

void FindObjMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("FINDOBJ");
	output.tag.set_property("ALGORITHM", config.algorithm);
	output.tag.set_property("USE_FLANN", config.use_flann);
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("ASPECT", config.aspect);
	output.tag.set_property("SCALE", config.scale);
	output.tag.set_property("TRANSLATE", config.translate);
	output.tag.set_property("ROTATE", config.rotate);
	output.tag.set_property("DRAG_OBJECT", config.drag_object);
	output.tag.set_property("OBJECT_X", config.object_x);
	output.tag.set_property("OBJECT_Y", config.object_y);
	output.tag.set_property("OBJECT_W", config.object_w);
	output.tag.set_property("OBJECT_H", config.object_h);
	output.tag.set_property("DRAG_SCENE", config.drag_scene);
	output.tag.set_property("SCENE_X", config.scene_x);
	output.tag.set_property("SCENE_Y", config.scene_y);
	output.tag.set_property("SCENE_W", config.scene_w);
	output.tag.set_property("SCENE_H", config.scene_h);
	output.tag.set_property("DRAG_REPLACE", config.drag_replace);
	output.tag.set_property("REPLACE_X", config.replace_x);
	output.tag.set_property("REPLACE_Y", config.replace_y);
	output.tag.set_property("REPLACE_W", config.replace_w);
	output.tag.set_property("REPLACE_H", config.replace_h);
	output.tag.set_property("REPLACE_DX", config.replace_dx);
	output.tag.set_property("REPLACE_DY", config.replace_dy);
	output.tag.set_property("DRAW_KEYPOINTS", config.draw_keypoints);
	output.tag.set_property("DRAW_MATCH", config.draw_match);
	output.tag.set_property("DRAW_SCENE_BORDER", config.draw_scene_border);
	output.tag.set_property("REPLACE_OBJECT", config.replace_object);
	output.tag.set_property("DRAW_OBJECT_BORDER", config.draw_object_border);
	output.tag.set_property("DRAW_REPLACE_BORDER", config.draw_replace_border);
	output.tag.set_property("OBJECT_LAYER", config.object_layer);
	output.tag.set_property("REPLACE_LAYER", config.replace_layer);
	output.tag.set_property("SCENE_LAYER", config.scene_layer);
	output.tag.set_property("BLEND", config.blend);
	output.append_tag();
	output.tag.set_title("/FINDOBJ");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void FindObjMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("FINDOBJ") ) {
			config.algorithm = input.tag.get_property("ALGORITHM", config.algorithm);
			config.use_flann = input.tag.get_property("USE_FLANN", config.use_flann);
			config.mode = input.tag.get_property("MODE", config.mode);
			config.aspect = input.tag.get_property("ASPECT", config.aspect);
			config.scale = input.tag.get_property("SCALE", config.scale);
			config.translate = input.tag.get_property("TRANSLATE", config.translate);
			config.rotate = input.tag.get_property("ROTATE", config.rotate);
			config.object_x = input.tag.get_property("OBJECT_X", config.object_x);
			config.object_y = input.tag.get_property("OBJECT_Y", config.object_y);
			config.object_w = input.tag.get_property("OBJECT_W", config.object_w);
			config.object_h = input.tag.get_property("OBJECT_H", config.object_h);
			config.drag_object = input.tag.get_property("DRAG_OBJECT", config.drag_object);
			config.scene_x = input.tag.get_property("SCENE_X", config.scene_x);
			config.scene_y = input.tag.get_property("SCENE_Y", config.scene_y);
			config.scene_w = input.tag.get_property("SCENE_W", config.scene_w);
			config.scene_h = input.tag.get_property("SCENE_H", config.scene_h);
			config.drag_scene = input.tag.get_property("DRAG_SCENE", config.drag_scene);
			config.replace_x = input.tag.get_property("REPLACE_X", config.replace_x);
			config.replace_y = input.tag.get_property("REPLACE_Y", config.replace_y);
			config.replace_w = input.tag.get_property("REPLACE_W", config.replace_w);
			config.replace_h = input.tag.get_property("REPLACE_H", config.replace_h);
			config.replace_dx = input.tag.get_property("REPLACE_DX", config.replace_dx);
			config.replace_dy = input.tag.get_property("REPLACE_DY", config.replace_dy);
			config.drag_replace = input.tag.get_property("DRAG_REPLACE", config.drag_replace);
			config.draw_keypoints = input.tag.get_property("DRAW_KEYPOINTS", config.draw_keypoints);
			config.draw_match = input.tag.get_property("DRAW_MATCH", config.draw_match);
			config.draw_scene_border = input.tag.get_property("DRAW_SCENE_BORDER", config.draw_scene_border);
			config.replace_object = input.tag.get_property("REPLACE_OBJECT", config.replace_object);
			config.draw_object_border = input.tag.get_property("DRAW_OBJECT_BORDER", config.draw_object_border);
			config.draw_replace_border = input.tag.get_property("DRAW_REPLACE_BORDER", config.draw_replace_border);
			config.object_layer = input.tag.get_property("OBJECT_LAYER", config.object_layer);
			config.replace_layer = input.tag.get_property("REPLACE_LAYER", config.replace_layer);
			config.scene_layer = input.tag.get_property("SCENE_LAYER", config.scene_layer);
			config.blend = input.tag.get_property("BLEND", config.blend);
		}
	}

	config.boundaries();
}

void FindObjMain::draw_line(VFrame *vframe, int x1, int y1, int x2, int y2)
{
	vframe->draw_line(x1, y1, x2, y2);
}

void FindObjMain::draw_quad(VFrame *vframe,
		int x1, int y1, int x2, int y2,
		int x3, int y3, int x4, int y4)
{
	int r = bmin(vframe->get_w(), vframe->get_h()) / 200 + 1;
	for( int i=r; --i>0; ) {
		draw_line(vframe, x1+i, y1+i, x2, y2);
		draw_line(vframe, x1-i, y1-i, x2, y2);
		draw_line(vframe, x2+i, y2+i, x3, y3);
		draw_line(vframe, x2-i, y2-i, x3, y3);
		draw_line(vframe, x3+i, y3+i, x4, y4);
		draw_line(vframe, x3-i, y3-i, x4, y4);
		draw_line(vframe, x4+i, y4+i, x1, y1);
		draw_line(vframe, x4-i, y4-i, x1, y1);
	}
	draw_line(vframe, x1, y1, x2, y2);
	draw_line(vframe, x2, y2, x3, y3);
	draw_line(vframe, x3, y3, x4, y4);
	draw_line(vframe, x4, y4, x1, y1);
}

void FindObjMain::draw_point(VFrame *vframe, int x1, int y1)
{
	int r = bmin(vframe->get_w(), vframe->get_h()) / 200 + 1;
	for( int i=r; --i>0; )
		draw_circle(vframe, x1, y1, i);
}

void FindObjMain::draw_rect(VFrame *vframe, int x1, int y1, int x2, int y2)
{
	int r = bmin(vframe->get_w(), vframe->get_h()) / 200 + 1;
	for( int i=r; --i>0; ) {
		--x2;  --y2;
		draw_line(vframe, x1, y1, x2, y1);
		draw_line(vframe, x2, y1, x2, y2);
		draw_line(vframe, x2, y2, x1, y2);
		draw_line(vframe, x1, y2, x1, y1);
		++x1;  ++y1;
	}
}

void FindObjMain::draw_circle(VFrame *vframe, int x, int y, int r)
{
	int x1 = x-r, x2 = x+r;
	int y1 = y-r, y2 = y+r;
	vframe->draw_smooth(x1,y, x1,y1, x,y1);
	vframe->draw_smooth(x,y1, x2,y1, x2,y);
	vframe->draw_smooth(x2,y, x2,y2, x,y2);
	vframe->draw_smooth(x,y2, x1,y2, x1,y);
}

void FindObjMain::filter_matches(ptV &p1, ptV &p2, double ratio)
{
	DMatches::iterator it;
	for( it=pairs.begin(); it!=pairs.end(); ++it ) { 
		DMatchV &m = *it;
		if( m.size() == 2 && m[0].distance < m[1].distance*ratio ) {
			p1.push_back(obj_keypts[m[0].queryIdx].pt);
			p2.push_back(scn_keypts[m[0].trainIdx].pt);
		}
	}
}

void FindObjMain::to_mat(Mat &mat, int mcols, int mrows,
	VFrame *inp, int ix,int iy, BC_CModel mcolor_model)
{
	int mcomp = BC_CModels::components(mcolor_model);
	int mbpp = BC_CModels::calculate_pixelsize(mcolor_model);
	int psz = mbpp / mcomp;
	int mdepth = psz < 2 ? CV_8U : CV_16U;
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

void FindObjMain::detect(Mat &mat, KeyPointV &keypts,Mat &descrs)
{
	keypts.clear();
	descrs.release();
	try {
		detector->detectAndCompute(mat, noArray(), keypts, descrs);
	} catch(std::exception e) { printf(_("detector exception: %s\n"), e.what()); }
}

void FindObjMain::match()
{
	pairs.clear();
	try {
		matcher->knnMatch(obj_descrs, scn_descrs, pairs, 2);
	} catch(std::exception e) { printf(_("match execption: %s\n"), e.what()); }
}

Ptr<DescriptorMatcher> FindObjMain::flann_kdtree_matcher()
{ // trees=5
	const Ptr<flann::IndexParams>& indexParams =
		makePtr<flann::KDTreeIndexParams>(5);
	const Ptr<flann::SearchParams>& searchParams =
		makePtr<flann::SearchParams>();
	return makePtr<FlannBasedMatcher>(indexParams, searchParams);
}
Ptr<DescriptorMatcher> FindObjMain::flann_lshidx_matcher()
{ // table_number = 6#12, key_size = 12#20, multi_probe_level = 1#2
	const Ptr<flann::IndexParams>& indexParams =
		makePtr<flann::LshIndexParams>(6, 12, 1);
	const Ptr<flann::SearchParams>& searchParams =
		makePtr<flann::SearchParams>();
	return makePtr<FlannBasedMatcher>(indexParams, searchParams);
}
Ptr<DescriptorMatcher> FindObjMain::bf_matcher_norm_l2()
{
	return BFMatcher::create(NORM_L2);
}
Ptr<DescriptorMatcher> FindObjMain::bf_matcher_norm_hamming()
{
	return BFMatcher::create(NORM_HAMMING);
}

#ifdef _SIFT
void FindObjMain::set_sift()
{
	cvmodel = BC_GREY8;
	detector = SIFT::create();
	matcher = config.use_flann ?
		flann_kdtree_matcher() : bf_matcher_norm_l2();
}
#endif
#ifdef _SURF
void FindObjMain::set_surf()
{
	cvmodel = BC_GREY8;
	detector = SURF::create(800);
	matcher = config.use_flann ?
		flann_kdtree_matcher() : bf_matcher_norm_l2();
}
#endif
#ifdef _ORB
void FindObjMain::set_orb()
{
	cvmodel = BC_GREY8;
	detector = ORB::create();
	matcher = config.use_flann ?
		flann_lshidx_matcher() : bf_matcher_norm_hamming();
}
#endif
#ifdef _AKAZE
void FindObjMain::set_akaze()
{
	cvmodel = BC_GREY8;
	detector = AKAZE::create();
	matcher = config.use_flann ?
		flann_lshidx_matcher() : bf_matcher_norm_hamming();
}
#endif
#ifdef _BRISK
void FindObjMain::set_brisk()
{
	cvmodel = BC_GREY8;
	detector = BRISK::create();
	matcher = config.use_flann ?
		flann_lshidx_matcher() : bf_matcher_norm_hamming();
}
#endif


void FindObjMain::process_match()
{
	if( config.algorithm == NO_ALGORITHM ) return;

	if( detector.empty() ) {
		switch( config.algorithm ) {
#ifdef _SIFT
		case ALGORITHM_SIFT:   set_sift();   break;
#endif
#ifdef _SURF
		case ALGORITHM_SURF:   set_surf();   break;
#endif
#ifdef _ORB
		case ALGORITHM_ORB:    set_orb();    break;
#endif
#ifdef _AKAZE
		case ALGORITHM_AKAZE:  set_akaze();  break;
#endif
#ifdef _BRISK
		case ALGORITHM_BRISK:  set_brisk();  break;
#endif
		}
		obj_keypts.clear();  obj_descrs.release();
		to_mat(object_mat, object_w,object_h, object, object_x,object_y, cvmodel);
		detect(object_mat, obj_keypts, obj_descrs);
//printf("detect obj %d features\n", (int)obj_keypts.size());
	}

	to_mat(scene_mat, scene_w,scene_h, scene, scene_x,scene_y, cvmodel);
	detect(scene_mat, scn_keypts, scn_descrs);
//printf("detect scn %d features\n", (int)scn_keypts.size());
	match();
	ptV p1, p2;
	filter_matches(p1, p2);
	if( p1.size() < 4 ) return;
	Mat H = findHomography(p1, p2, RANSAC, 5.0);
	if( !H.dims || !H.rows || !H.cols ) {
//printf("fail, size p1=%d,p2=%d\n",(int)p1.size(),(int)p2.size());
		return;
	}

	ptV src, dst;
	float out_x1 = 0, out_x2 = object_w;
	float out_y1 = 0, out_y2 = object_h;
	src.push_back(Point2f(out_x1,out_y1));
	src.push_back(Point2f(out_x2,out_y1));
	src.push_back(Point2f(out_x2,out_y2));
	src.push_back(Point2f(out_x1,out_y2));
	perspectiveTransform(src, dst, H);

	match_x1 = dst[0].x + scene_x;  match_y1 = dst[0].y + scene_y;
	match_x2 = dst[1].x + scene_x;  match_y2 = dst[1].y + scene_y;
	match_x3 = dst[2].x + scene_x;  match_y3 = dst[2].y + scene_y;
	match_x4 = dst[3].x + scene_x;  match_y4 = dst[3].y + scene_y;
}


static double area(float x1, float y1, float x2, float y2,
		float x3, float y3, float x4, float y4)
{ // quadrelateral area, sign is +ccw,-cw, use abs
	double dx1 = x3-x1, dy1 = y3-y1;
	double dx2 = x4-x2, dy2 = y4-y2;
	return 0.5 * (dx1 * dy2 - dx2 * dy1);
}
static double dist(float x1,float y1, float x2, float y2)
{
	double dx = x2-x1, dy = y2-y1;
	return sqrt(dx*dx + dy*dy);
}
static int intersects(double x1, double y1, double x2, double y2,
		double x3, double y3, double x4, double y4)
{
	double dx12 = x2 - x1, dy12 = y2 - y1;
	double dx34 = x4 - x3, dy34 = y4 - y3;
	double d = dx12*dy34 - dx34*dy12;
	if( !d ) return 0; // parallel
	double dx13 = x3 - x1, dy13 = y3 - y1;
	double u = (dx13*dy34 - dx34*dy13) / d;
	if( u < 0 || u > 1 ) return 0;
	double v = (dx13*dy12 - dx12*dy13) / d;
	if( v < 0 || v > 1 ) return 0;
	return 1;
}

/*
 * 4---------3    1---------2
 * |0,h   w,h|    |0,0   w,0|
 * |    +    |    |    +    |
 * |0,0   w,0|    |0,h   w,h|
 * 1---------2    1---------2
 * pt locations    screen pts
 */
void FindObjMain::reshape()
{
	if( config.mode == MODE_NONE ) return;
	const double pi = M_PI;
	double x1 = match_x1, y1 = match_y1;
	double x2 = match_x2, y2 = match_y2;
	double x3 = match_x3, y3 = match_y3;
	double x4 = match_x4, y4 = match_y4;
	double ia = area(x1,y1, x2,y2, x3,y3, x4,y4);
// centroid
	double cx = (x1 + x2 + x3 + x4) / 4;
	double cy = (y1 + y2 + y3 + y4) / 4;
// centered
	x1 -= cx;  x2 -= cx;  x3 -= cx;  x4 -= cx;
	y1 -= cy;  y2 -= cy;  y3 -= cy;  y4 -= cy;
// bowtied
	if( intersects(x1,y1, x2,y2, x3,y3, x4,y4) ) {
		double x = x2, y = y2;
		x2 = x3;  y2 = y3;
		x3 = x;   y3 = y;
	}
	else if( intersects(x1,y1, x4,y4, x3,y3, x2,y2) ) {
		double x = x4, y = y4;
		x4 = x3;  y4 = y3;
		x3 = x;   y3 = y;
	}

// rotation, if mode is quad: reverse rotate
	double r = 0;
	if( (config.mode == MODE_QUADRILATERAL) ^ (config.rotate != 0) ) {
// edge centers
		double cx12 = (x1 + x2) / 2, cy12 = (y1 + y2) / 2;
		double cx23 = (x2 + x3) / 2, cy23 = (y2 + y3) / 2;
		double cx34 = (x3 + x4) / 2, cy34 = (y3 + y4) / 2;
		double cx41 = (x4 + x1) / 2, cy41 = (y4 + y1) / 2;
		double vx = cx34 - cx12, vy = cy34 - cy12;
		double hx = cx23 - cx41, hy = cy23 - cy41;
		double v = atan2(vy, vx);
		double h = atan2(hy, hx);
		r = (h + v - pi/2) / 2;
		if( config.mode != MODE_QUADRILATERAL ) r = -r;
	}
// diagonal length
	double a = dist(x1,y1, x3,y3) / 2;
	double b = dist(x2,y2, x4,y4) / 2;
	if( config.mode == MODE_SQUARE ||
	    config.mode == MODE_RECTANGLE )
		a = b = (a + b) / 2;
// central angles
	double a1 = atan2(y1, x1);
	double a2 = atan2(y2, x2);
	double a3 = atan2(y3, x3);
	double a4 = atan2(y4, x4);
// edge angles
	double a12 = a2 - a1, a23 = a3 - a2;
	double a34 = a4 - a3, a41 = a1 - a4;
	double dt = (a12 - a23 + a34 - a41)/4;
// mirrored
	if( ia < 0 ) { ia = -ia;  dt = -dt;  r = -r; }
	switch( config.mode ) {
	case MODE_SQUARE:
	case MODE_RHOMBUS:
		dt = pi/2;
	case MODE_RECTANGLE:
	case MODE_PARALLELOGRAM: {
		double t = -(pi+dt)/2;
		x1 = a*cos(t);  y1 = a*sin(t);  t += dt;
		x2 = b*cos(t);  y2 = b*sin(t);  t += pi - dt;
		x3 = a*cos(t);  y3 = a*sin(t);  t += dt;
		x4 = b*cos(t);  y4 = b*sin(t); }
	case MODE_QUADRILATERAL:
		break;
	}
// aspect
	if( !config.aspect ) {
		double cx12 = (x1 + x2) / 2, cy12 = (y1 + y2) / 2;
		double cx23 = (x2 + x3) / 2, cy23 = (y2 + y3) / 2;
		double cx34 = (x3 + x4) / 2, cy34 = (y3 + y4) / 2;
		double cx41 = (x4 + x1) / 2, cy41 = (y4 + y1) / 2;
		double iw = dist(cx41,cy41, cx23,cy23);
		double ih = dist(cx12,cy12, cx34,cy34);
		double ow = object_w, oh = object_h;
		double sx = iw && ih ? sqrt((ih*ow)/(iw*oh)) : 1;
		double sy = sx ? 1 / sx : 1;
		x1 *= sx;  x2 *= sx;  x3 *= sx;  x4 *= sx;
		y1 *= sy;  y2 *= sy;  y3 *= sy;  y4 *= sy;
	}
// rotation
	if( r ) {
		double ct = cos(r), st = sin(r), x, y;
		x = x1*ct + y1*st;  y = y1*ct - x1*st;  x1 = x;  y1 = y;
		x = x2*ct + y2*st;  y = y2*ct - x2*st;  x2 = x;  y2 = y;
		x = x3*ct + y3*st;  y = y3*ct - x3*st;  x3 = x;  y3 = y;
		x = x4*ct + y4*st;  y = y4*ct - x4*st;  x4 = x;  y4 = y;
	}
// scaling
	ia = !config.scale ? object_w * object_h : ia;
	double oa = abs(area(x1,y1, x2,y2, x3,y3, x4,y4));
	double sf =  oa ? sqrt(ia / oa) : 0;
	x1 *= sf;  x2 *= sf;  x3 *= sf;  x4 *= sf;
	y1 *= sf;  y2 *= sf;  y3 *= sf;  y4 *= sf;
// translation
	double ox = !config.translate ? object_x + object_w/2. : cx;
	double oy = !config.translate ? object_y + object_h/2. : cy;
	x1 += ox;  x2 += ox;  x3 += ox;  x4 += ox;
	y1 += oy;  y2 += oy;  y3 += oy;  y4 += oy;

	shape_x1 = x1;  shape_y1 = y1;
	shape_x2 = x2;  shape_y2 = y2;
	shape_x3 = x3;  shape_y3 = y3;
	shape_x4 = x4;  shape_y4 = y4;
}

int FindObjMain::process_buffer(VFrame **frame, int64_t start_position, double frame_rate)
{
	int prev_algorithm = config.algorithm;
	int prev_use_flann = config.use_flann;

	if( load_configuration() )
		init_border = 1;

	if( prev_algorithm != config.algorithm ||
	    prev_use_flann != config.use_flann ) {
		detector.release();
		matcher.release();
	}

	object_layer = config.object_layer;
	scene_layer = config.scene_layer;
	replace_layer = config.replace_layer;
	Track *track = server->plugin->track;
	w = track->track_w;
	h = track->track_h;

	int max_layer = PluginClient::get_total_buffers() - 1;
	object_layer = bclip(config.object_layer, 0, max_layer);
	scene_layer = bclip(config.scene_layer, 0, max_layer);
	replace_layer = bclip(config.replace_layer, 0, max_layer);

	int cfg_w = (int)(w * config.object_w / 100.);
	int cfg_h = (int)(h * config.object_h / 100.);
	int cfg_x1 = (int)(w * config.object_x / 100. - cfg_w / 2);
	int cfg_y1 = (int)(h * config.object_y / 100. - cfg_h / 2);
	int cfg_x2 = cfg_x1 + cfg_w;
	int cfg_y2 = cfg_y1 + cfg_h;
	bclamp(cfg_x1, 0, w);  object_x = cfg_x1;
	bclamp(cfg_y1, 0, h);  object_y = cfg_y1;
	bclamp(cfg_x2, 0, w);  object_w = cfg_x2 - cfg_x1;
	bclamp(cfg_y2, 0, h);  object_h = cfg_y2 - cfg_y1;

	cfg_w = (int)(w * config.scene_w / 100.);
	cfg_h = (int)(h * config.scene_h / 100.);
	cfg_x1 = (int)(w * config.scene_x / 100. - cfg_w / 2);
	cfg_y1 = (int)(h * config.scene_y / 100. - cfg_h / 2);
	cfg_x2 = cfg_x1 + cfg_w;
	cfg_y2 = cfg_y1 + cfg_h;
	bclamp(cfg_x1, 0, w);  scene_x = cfg_x1;
	bclamp(cfg_y1, 0, h);  scene_y = cfg_y1;
	bclamp(cfg_x2, 0, w);  scene_w = cfg_x2 - cfg_x1;
	bclamp(cfg_y2, 0, h);  scene_h = cfg_y2 - cfg_y1;

	cfg_w = (int)(w * config.replace_w / 100.);
	cfg_h = (int)(h * config.replace_h / 100.);
	cfg_x1 = (int)(w * config.replace_x / 100. - cfg_w / 2);
	cfg_y1 = (int)(h * config.replace_y / 100. - cfg_h / 2);
	cfg_x2 = cfg_x1 + cfg_w;
	cfg_y2 = cfg_y1 + cfg_h;
	bclamp(cfg_x1, 0, w);  replace_x = cfg_x1;
	bclamp(cfg_y1, 0, h);  replace_y = cfg_y1;
	bclamp(cfg_x2, 0, w);  replace_w = cfg_x2 - cfg_x1;
	bclamp(cfg_y2, 0, h);  replace_h = cfg_y2 - cfg_y1;

	int cfg_dx = (int)(w * config.replace_dx / 100.);
	int cfg_dy = (int)(h * config.replace_dy / 100.);
	bclamp(cfg_dx, -h, h);  replace_dx = cfg_dx;
	bclamp(cfg_dy, -w, w);  replace_dy = cfg_dy;

// Read in the input frames
	for( int i = 0; i < PluginClient::get_total_buffers(); i++ ) {
		read_frame(frame[i], i, start_position, frame_rate, 0);
	}

	object = frame[object_layer];
	scene = frame[scene_layer];
	replace = frame[replace_layer];

	shape_x1 = out_x1;  shape_y1 = out_y1;
	shape_x2 = out_x2;  shape_y2 = out_y2;
	shape_x3 = out_x3;  shape_y3 = out_y3;
	shape_x4 = out_x4;  shape_y4 = out_y4;

	if( scene_w > 0 && scene_h > 0 && object_w > 0 && object_h > 0 ) {
		process_match();
		reshape();
	}

	double w0 = init_border ? 1. : config.blend/100., w1 = 1. - w0;
	init_border = 0;
	out_x1 = shape_x1*w0 + out_x1*w1;  out_y1 = shape_y1*w0 + out_y1*w1;
	out_x2 = shape_x2*w0 + out_x2*w1;  out_y2 = shape_y2*w0 + out_y2*w1;
	out_x3 = shape_x3*w0 + out_x3*w1;  out_y3 = shape_y3*w0 + out_y3*w1;
	out_x4 = shape_x4*w0 + out_x4*w1;  out_y4 = shape_y4*w0 + out_y4*w1;
// Replace object in the scene layer
	if( config.replace_object ) {
		int cpus1 = get_project_smp() + 1;
		if( !affine )
			affine = new AffineEngine(cpus1, cpus1);
		if( !overlayer )
			overlayer = new OverlayFrame(cpus1);
		VFrame *temp = new_temp(w, h, scene->get_color_model());
		temp->clear_frame();
		affine->set_in_viewport(replace_x, replace_y, replace_w, replace_h);
		float ix1 = replace_x, ix2 = ix1 + replace_w;
		float iy1 = replace_y, iy2 = iy1 + replace_h;
		float dx = replace_dx, dy = replace_dy;
		float ox1 = out_x1+dx, ox2 = out_x2+dx, ox3 = out_x3+dx, ox4 = out_x4+dx;
		float oy1 = out_y1-dy, oy2 = out_y2-dy, oy3 = out_y3-dy, oy4 = out_y4-dy;
		affine->set_matrix(ix1,iy1, ix2,iy2, ox1,oy1, ox2,oy2, ox4,oy4, ox3,oy3);
		affine->process(temp, replace, 0,
			AffineEngine::TRANSFORM, 0,0, 100,0, 100,100, 0,100, 1);
		overlayer->overlay(scene, temp,  0,0, w,h,  0,0, w,h,
			1, TRANSFER_NORMAL, NEAREST_NEIGHBOR);

	}

	int wh = (w+h)>>8, ss = 1; while( wh>>=1 ) ss<<=1;
	if( config.draw_scene_border ) {
		scene->set_stiple(ss*2);
		scene->set_pixel_color(WHITE);
		draw_rect(scene, scene_x, scene_y, scene_x+scene_w, scene_y+scene_h);
	}
	if( config.draw_object_border ) {
		scene->set_stiple(ss*3);
		scene->set_pixel_color(YELLOW); 
		draw_rect(scene, object_x, object_y, object_x+object_w, object_y+object_h);
	}
	if( config.draw_replace_border ) {
		scene->set_stiple(ss*3);
		scene->set_pixel_color(GREEN);
		draw_rect(scene, replace_x, replace_y, replace_x+replace_w, replace_y+replace_h);
	}
	scene->set_stiple(0);
	if( config.draw_keypoints ) {
		scene->set_pixel_color(GREEN);
		for( int i=0,n=obj_keypts.size(); i<n; ++i ) {
			Point2f &pt = obj_keypts[i].pt;
			int r = obj_keypts[i].size * 1.2/9 * 2;
			int x = pt.x + object_x, y = pt.y + object_y;
			draw_circle(scene, x, y, r);
		}
		scene->set_pixel_color(RED);
		for( int i=0,n=scn_keypts.size(); i<n; ++i ) {
			Point2f &pt = scn_keypts[i].pt;
			int r = scn_keypts[i].size * 1.2/9 * 2;
			int x = pt.x + scene_x, y = pt.y + scene_y;
			draw_circle(scene, x, y, r);
		}
	}
	if( config.draw_match ) {
		scene->set_pixel_color(BLUE);
		draw_quad(scene, match_x1, match_y1, match_x2, match_y2,
				match_x3, match_y3, match_x4, match_y4);
		scene->set_pixel_color(LTGREEN);
		draw_point(scene, match_x1, match_y1);
	}

	if( gui_open() ) {
		if( config.drag_scene ) {
			scene->set_pixel_color(WHITE);
			DragCheckBox::draw_boundary(scene, scene_x, scene_y, scene_w, scene_h);
		}
		if( config.drag_object ) {
			scene->set_pixel_color(YELLOW);
			DragCheckBox::draw_boundary(scene, object_x, object_y, object_w, object_h);
		}
		if( config.drag_replace ) {
			scene->set_pixel_color(GREEN);
			DragCheckBox::draw_boundary(scene, replace_x, replace_y, replace_w, replace_h);
		}
	}

	scene->set_pixel_color(BLACK);
	scene->set_stiple(0);
	return 0;
}

