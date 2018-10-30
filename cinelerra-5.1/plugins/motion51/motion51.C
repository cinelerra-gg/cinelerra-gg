#include "affine.h"
#include "bchash.h"
#include "clip.h"
#include "filexml.h"
#include "fourier.h"
#include "keyframe.h"
#include "language.h"
#include "mainerror.h"
#include "motion51.h"
#include "motionwindow51.h"
#include "mutex.h"
#include "transportque.inc"

#include <errno.h>
#include <math.h>
#include <unistd.h>

static const double passing = 0.92;
static bool passible(double v) { return v > passing; }


REGISTER_PLUGIN(Motion51Main)

void Motion51Config::init()
{
	sample_steps = 1024;
	sample_r = 50.f;
	block_x = block_y = 50.f;
	block_w = block_h = 50.f;
	horiz_limit = vert_limit = twist_limit = 50.f;
	shake_fade = twist_fade = 3.f;
	draw_vectors = 1;
	strcpy(tracking_file, TRACKING_FILE);
	tracking = 0;
}

Motion51Config::Motion51Config()
{
	init();
}

int Motion51Config::equivalent(Motion51Config &that)
{
	return horiz_limit == that.horiz_limit &&
		vert_limit == that.vert_limit &&
		twist_limit == that.twist_limit &&
		shake_fade == that.shake_fade &&
		twist_fade == that.twist_fade &&
		sample_r == that.sample_r &&
		sample_steps == that.sample_steps &&
		draw_vectors == that.draw_vectors &&
		EQUIV(block_x, that.block_x) &&
		EQUIV(block_y, that.block_y) &&
		block_w == that.block_w &&
		block_h == that.block_h &&
		!strcmp(tracking_file, that.tracking_file) &&
		tracking == that.tracking;
}

void Motion51Config::copy_from(Motion51Config &that)
{
	horiz_limit = that.horiz_limit;
	vert_limit = that.vert_limit;
	twist_limit = that.twist_limit;
	shake_fade = that.shake_fade;
	twist_fade = that.twist_fade;
	sample_r = that.sample_r;
	sample_steps = that.sample_steps;
	draw_vectors = that.draw_vectors;
	block_x = that.block_x;
	block_y = that.block_y;
	block_w = that.block_w;
	block_h = that.block_h;
	strcpy(tracking_file, that.tracking_file);
	tracking = that.tracking;
}

void Motion51Config::interpolate(Motion51Config &prev, Motion51Config &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	copy_from(prev);
}


Motion51Main::Motion51Main(PluginServer *server)
 : PluginVClient(server)
{
	out_frame = 0;  out_position = -1;
	ref_frame = 0;  ref_position = -1;
	tmp_frame = 0;
	affine = 0;
	motion_scan = 0;

	cache_file[0] = 0;
	cache_fp = active_fp = 0;
	cache_line[0] = 0;
	cache_key = active_key = -1;
	tracking_position = -1;

	out_w = out_h = out_r = 0;
	rx = ry = rw = rh = rr = 0;
	current_dx = current_dy = 0;
	x_steps = y_steps = 16;
	r_steps = 4;
	cir_sz = 0;  cir_r = 0;
	xpts = ypts = 0;
	total_dx = total_dy = 0;
	total_angle = 0;
}

Motion51Main::~Motion51Main()
{
	update_cache_file();
	delete out_frame;
	delete ref_frame;
	delete tmp_frame;
	delete affine;
	delete motion_scan;
	delete [] xpts;
	delete [] ypts;
}

const char* Motion51Main::plugin_title() { return N_("Motion51"); }
int Motion51Main::is_realtime() { return 1; }
int Motion51Main::is_multichannel() { return 1; }


NEW_WINDOW_MACRO(Motion51Main, Motion51Window)
LOAD_CONFIGURATION_MACRO(Motion51Main, Motion51Config)


void Motion51Main::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	thread->window->lock_window("Motion51Main::update_gui");
	Motion51Window *window = (Motion51Window*)thread->window;
	window->update_gui();
	thread->window->unlock_window();
}




void Motion51Main::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("MOTION51");
	output.tag.set_property("HORIZ_LIMIT", config.horiz_limit);
	output.tag.set_property("VERT_LIMIT", config.vert_limit);
	output.tag.set_property("TWIST_LIMIT", config.twist_limit);
	output.tag.set_property("SHAKE_FADE", config.shake_fade);
	output.tag.set_property("TWIST_FADE", config.twist_fade);
	output.tag.set_property("SAMPLE_R", config.sample_r);
	output.tag.set_property("SAMPLE_STEPS", config.sample_steps);
	output.tag.set_property("DRAW_VECTORS", config.draw_vectors);
	output.tag.set_property("BLOCK_W", config.block_w);
	output.tag.set_property("BLOCK_H", config.block_h);
	output.tag.set_property("BLOCK_X", config.block_x);
	output.tag.set_property("BLOCK_Y", config.block_y);
	output.tag.set_property("TRACKING_FILE", config.tracking_file);
	output.tag.set_property("TRACKING", config.tracking);
	output.append_tag();
	output.tag.set_title("/MOTION51");
	output.append_tag();
	output.terminate_string();
}

void Motion51Main::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	int result = 0;

	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("MOTION51") ) {
			config.horiz_limit = input.tag.get_property("HORIZ_LIMIT", config.horiz_limit);
			config.vert_limit = input.tag.get_property("VERT_LIMIT", config.vert_limit);
			config.twist_limit = input.tag.get_property("TWIST_LIMIT", config.twist_limit);
			config.shake_fade = input.tag.get_property("SHAKE_FADE", config.shake_fade);
			config.twist_fade = input.tag.get_property("TWIST_FADE", config.twist_fade);
			config.sample_r = input.tag.get_property("SAMPLE_R", config.sample_r);
			config.sample_steps = input.tag.get_property("SAMPLE_STEPS", config.sample_steps);
			config.draw_vectors = input.tag.get_property("DRAW_VECTORS", config.draw_vectors);
			config.block_w = input.tag.get_property("BLOCK_W", config.block_w);
			config.block_h = input.tag.get_property("BLOCK_H", config.block_h);
			config.block_x = input.tag.get_property("BLOCK_X", config.block_x);
			config.block_y = input.tag.get_property("BLOCK_Y", config.block_y);
			input.tag.get_property("TRACKING_FILE", config.tracking_file);
			config.tracking = input.tag.get_property("TRACKING", config.tracking);
		}
	}
}

#if 0
static void snap(const char *fn, VFrame *img, float x, float y, float r)
{
	VFrame vfrm(img->get_w(),img->get_h(),img->get_color_model());
	vfrm.copy_from(img);
	vfrm.draw_smooth(x-r,y, x-r,y+r, x,y+r);
	vfrm.draw_smooth(x,y+r, x+r,y+r, x+r,y);
	vfrm.draw_smooth(x+r,y, x+r,y-r, x,y-r);
	vfrm.draw_smooth(x,y-r, x-r,y-r, x-r,y);
	vfrm.write_png(fn);
}
#endif

#if 0
// nearest sample
static void nearest_uint8(double *pix[3], double rx, double ry,
		uint8_t **rows, int psz, int iw1, int ih1)
{
	int ix = (int)rx, iy = (int)ry;
	bclamp(ix, 0, iw1);  bclamp(iy, 0, ih1);
	uint8_t *cp = (uint8_t*)(rows[iy] + psz * ix);
	for( int i=0; i<3; ++pix[i], ++cp, ++i ) *pix[i] = *cp;
}
static void nearest_float(double *pix[3], double rx, double ry,
		uint8_t **rows, int psz, int iw1, int ih1)
{
	int ix = (int)rx, iy = (int)ry;
	bclamp(ix, 0, iw1);  bclamp(iy, 0, ih1);
	float *fp = (float*)(rows[iy] + psz * ix);
	for( int i=0; i<3; ++pix[i], ++fp, ++i ) *pix[i] = *fp;
}
#endif

// corner interpolation sample
static void corner_uint8(double *pix[3], double rx, double ry,
		uint8_t **rows, int psz, int iw1, int ih1)
{
	bclamp(rx, 0, iw1);  bclamp(ry, 0, ih1);
	int iy = (int)ry;
	double yf1 = ry - iy, yf0 = 1.0 - yf1;
	uint8_t *row0 = rows[iy];
	if( iy < ih1 ) ++iy;
	uint8_t *row1 = rows[iy];
	int ix = (int)rx;
	double xf1 = rx - ix, xf0 = 1.0 - xf1;
	int i0 = psz * ix;
	if( ix < iw1 ) ++ix;
	int i1 = psz * ix;
	uint8_t *cp00 = (uint8_t*)&row0[i0], *cp01 = (uint8_t*)&row0[i1];
	uint8_t *cp10 = (uint8_t*)&row1[i0], *cp11 = (uint8_t*)&row1[i1];
	double a00 = xf0 * yf0, a01 = xf1 * yf0;
	double a10 = xf0 * yf1, a11 = xf1 * yf1;
	for( int i=0; i<3; ++pix[i], ++cp00, ++cp01, ++cp10, ++cp11, ++i )
		*pix[i] = *cp00*a00 + *cp01*a01 + *cp10*a10 + *cp11*a11;
}
static void corner_float(double *pix[3], double rx, double ry,
		uint8_t **rows, int psz, int iw1, int ih1)
{
	bclamp(rx, 0, iw1);  bclamp(ry, 0, ih1);
	int iy = (int)ry;
	double yf1 = ry - iy, yf0 = 1.0 - yf1;
	uint8_t *row0 = rows[iy];
	if( iy < ih1 ) ++iy;
	uint8_t *row1 = rows[iy];
	int ix = (int)rx;
	double xf1 = rx - ix, xf0 = 1.0 - xf1;
	int i0 = psz * ix;
	if( ix < iw1 ) ++ix;
	int i1 = psz * ix;
	float *fp00 = (float*)&row0[i0], *fp01 = (float*)&row0[i1];
	float *fp10 = (float*)&row1[i0], *fp11 = (float*)&row1[i1];
	double a00 = xf0 * yf0, a01 = xf1 * yf0;
	double a10 = xf0 * yf1, a11 = xf1 * yf1;
	for( int i=0; i<3; ++pix[i], ++fp00, ++fp01, ++fp10, ++fp11, ++i )
		*pix[i] = *fp00*a00 + *fp01*a01 + *fp10*a10 + *fp11*a11;
}


static inline double cor(int n, double *ap, double *bp)
{
	double s = 0;
	while( --n >= 0 ) s += *ap++ * *bp++;
	return s;
}

static inline double sqr(double v) { return v*v; }

static inline void cj_product(int n, int sf, double *rp, double *ip,
		double *arp, double *aip, double *brp, double *bip)
{
	int m = !sf ? n-1 : n/2, i = 0;
	while( i <= m ) {
		double ar = arp[i], ai = aip[i];
		double br = brp[i], bi = -bip[i];
		rp[i] = ar*br - ai*bi;  // complex a*ib'
		ip[i] = ar*bi + ai*br;
		++i;
	}
	if( !sf ) return;
	while( --m > 0 ) { rp[i] = rp[m];  ip[i] = -ip[m];  ++i; }
}

typedef struct { double x, y, d; } coord_t;
static int coord_cmpr(const void *ap, const void *bp)
{
	coord_t *a = (coord_t *)ap, *b = (coord_t *)bp;
	return a->d == b->d ? 0 : a->d < b->d ? -1 : 1;
}


int64_t Motion51Main::get_ref_position()
{
	int64_t position = out_position - 1;
	if( position < 0 || position != ref_position ) {
// clip to edit boundaries
		int64_t pos = get_source_start();
		if( position < pos )
			position = pos;
		else if( position >= (pos += get_total_len()) )
			position = pos-1;
// clip to keyframe boundaries
		KeyFrame *next_keyframe = get_next_keyframe(out_position, 1);
		int64_t keyframe_end = next_keyframe->position;
		if( (pos=next_keyframe->position) > 0 && position > keyframe_end )
			position = keyframe_end;
		KeyFrame *prev_keyframe = get_prev_keyframe(out_position, 1);
		int64_t keyframe_start = prev_keyframe->position;
		if( keyframe_start > 0 && position < keyframe_start )
			position = keyframe_start;
	}
	return position;
}

void Motion51Main::set_tracking_path()
{
	const char *sp = TRACKING_FILE;
	char *cp = config.tracking_file, *ep = cp+sizeof(config.tracking_file)-1;
	while( cp < ep && *sp != 0 ) *cp++ = *sp++;
	if( cp < ep && (sp=get_source_path()) ) {
		*cp++ = '-';
		const char *bp = strrchr(sp,'/');
		if( bp ) sp = bp+1;
		while( cp < ep && *sp != 0 ) {
			*cp++ = (*sp>='a' && *sp<='z') ||
				(*sp>='A' && *sp<='Z') ||
				(*sp>='0' && *sp<='9') ? *sp : '_';
			++sp;
		}
	}
	*cp = 0;
}

void Motion51Main::update_tracking_cache()
{
	if( (!config.tracking && cache_fp) || (config.tracking && !cache_fp) ||
	    (active_fp && active_key > get_source_position()) )
		update_cache_file();
}

int Motion51Main::load_tracking_cache(int64_t position)
{
	if( !config.tracking ) return 1;
	if( get_cache_line(position) ) return 1;
	if( sscanf(cache_line, "%jd %f %f %f", &position, &dx, &dy, &dt) != 4 ) return 1;
	return 0;
}

void Motion51Main::save_tracking_cache(int64_t position)
{
	if( !config.tracking ) return;
	char line[BCSTRLEN];
	snprintf(line, sizeof(line), "%jd %f %f %f\n", position, dx, dy, dt);
	put_cache_line(line);
}

void Motion51Main::match(VFrame *ref, VFrame *cur)
{
	if( !motion_scan ) {
		int cpus = get_project_smp()+1;
		motion_scan = new Motion51Scan(this, cpus, x_steps, y_steps);
	}

	motion_scan->scan(ref, cur, config.sample_steps);

	if( passible(motion_scan->cor_value) ) {
		dx = motion_scan->dx_result - rx;
		dy = motion_scan->dy_result - ry;
		dt = motion_scan->dt_result * 180./M_PI;
	}
	else {
		total_dx = total_dy = total_angle = 0;
		dx = dy = dt = 0;
	}

}

int Motion51Main::transform_target(int use_opengl)
{
	if( dx || dy || dt ) {
		int cpus = get_project_smp()+1;
		if( !affine ) affine = new AffineEngine(cpus, cpus);
		affine->set_in_pivot(rx, ry);
		affine->set_out_pivot(rx-dx, ry-dy);
		if( use_opengl )
			return run_opengl();
		new_temp(out_frame, out)->copy_from(out);
		out->clear_frame();
		affine->rotate(out, out_frame, dt);
//printf("transform_target at %jd: rotate(%f, %f, %f)\n", out_position, dx, dy, dt);
	}
        return 0;
}

int Motion51Main::handle_opengl()
{
#ifdef HAVE_GL
        affine->set_opengl(1);
	affine->rotate(out, out, dt);
	out->screen_to_ram();
        affine->set_opengl(0);
#endif
        return 0;
}


int Motion51Main::process_buffer(VFrame **frame, int64_t position, double frame_rate)
{
	int need_reconfigure = load_configuration();

	int target_layer = 0;
	int reference_layer = PluginClient::total_in_buffers-1;
	VFrame *ref_layer = frame[reference_layer];
	out = frame[target_layer];
	out_position = position;
	if( !out_position ) total_dx = total_dy = total_angle = 0;
	get_pixel = BC_CModels::is_float(out->get_color_model()) ?
		&corner_float : &corner_uint8;

	int use_opengl = get_use_opengl();
	read_frame(out, target_layer, out_position, frame_rate, use_opengl);
	out_w = out->get_w();
	out_h = out->get_h();
	out_r = 0.5 * (out_w < out_h ? out_w : out_h);
	rw = out_w * config.block_w/100.;
	rh = out_h * config.block_h/100.;
	rx = out_w * config.block_x/100.;
	ry = out_h * config.block_y/100.;
	rr = out_r * config.sample_r/100.;
	reset_sample(config.sample_steps, rr);
	dx = 0;  dy = 0;  dt = 0;

	update_tracking_cache();
	if( load_tracking_cache(out_position) ) {
		int64_t ref_pos = get_ref_position();
		if( !ref_frame || ref_pos != ref_position || need_reconfigure ) {
			new_temp(ref_frame, ref_layer);
			read_frame(ref_frame, reference_layer, ref_pos, frame_rate, 0);
			total_dx = total_dy = 0;  total_angle = 0;
			ref_position = ref_pos;
		}
		VFrame *cur_frame = out;
		if( reference_layer != target_layer ) {
			new_temp(tmp_frame, ref_layer);
			read_frame(tmp_frame, reference_layer, out_position, frame_rate, 0);
			cur_frame = tmp_frame;
		}
		match(ref_frame, cur_frame);
		save_tracking_cache(out_position);
	}
	current_dx = dx;  current_dy = dy;
	double sf = 1. - config.shake_fade/100.;
	dx += total_dx * sf;  dy += total_dy * sf;
	double rf = 1. - config.twist_fade/100.;
	dt += total_angle * rf;
	if( dt < -180. ) dt += 360.;
	else if( dt > 180. ) dt -= 360.;

	float tot_dx = out_w * config.horiz_limit/100.;
	bclamp(dx, -tot_dx, tot_dx);
	float tot_dy = out_h * config.vert_limit/100.;
	bclamp(dy, -tot_dy, tot_dy);
	float tot_dt = 180. * config.twist_limit/100.;
	bclamp(dt, -tot_dt, +tot_dt);
	total_dx = dx;  total_dy = dy;  total_angle = dt;
	if( ref_frame && reference_layer == target_layer &&
	    ref_position+1 == out_position &&
	    ref_frame->get_w() == out->get_w() &&
	    ref_frame->get_h() == out->get_h() &&
	    ref_frame->get_color_model() == out->get_color_model() ) {
		ref_frame->copy_from(out);
		ref_position = out_position;
	}
	transform_target(use_opengl);

	if( config.draw_vectors )
		draw_vectors(out);

	return 0;
}

VFrame* Motion51Main::new_temp(VFrame *&tmp, VFrame *ref)
{
	if( tmp && (tmp->get_w() != ref->get_w() || tmp->get_h() != ref->get_h() ||
	    tmp->get_color_model() != ref->get_color_model()) ) {
		delete tmp; tmp = 0;
	}
	if( !tmp )
		tmp = new VFrame(ref->get_w(), ref->get_h(), ref->get_color_model(), 0);
	return tmp;
}

void Motion51Main::reset_sample(int sz, double r)
{
	if( cir_sz == sz && cir_r == r ) return;
	if( cir_sz != sz ) {
		cir_sz = sz;
		delete xpts;  xpts = new double[cir_sz];
		delete ypts;  ypts = new double[cir_sz];
	}
	cir_r = r;
	int n = cir_sz / r_steps;
	double dt = (2*M_PI)/n;
	double dr = r / r_steps;
	for( int it=0; it<n; ++it ) {
		double t = it * dt, cos_t = cos(t), sin_t = sin(t);
		for( int i=0; i<r_steps; ) {
			int k = i * n + it;
			double r = ++i * dr;
			xpts[k] = r * cos_t;
			ypts[k] = r * sin_t;
		}
	}
}

void Motion51Main::get_samples(VFrame *img, double *pix[3], double x, double y)
{
	int iw = img->get_w(), iw1 = iw-1;
	int ih = img->get_h(), ih1 = ih-1;
	uint8_t **rows = img->get_rows();
	int psz = BC_CModels::calculate_pixelsize(img->get_color_model());

	double *xp = xpts, *yp = ypts;
	for( int i=cir_sz; --i>=0; ++xp,++yp ) {
		double px = x + *xp, py = y + *yp;
		get_pixel(pix, px, py, rows, psz, iw1, ih1);
	}
}

void Motion51Main::centroid(double *pix[3], double *ctr_v, double *ctr_x, double *ctr_y)
{
	for( int i=0; i<3; ++i )
		ctr_v[i] = ctr_x[i] = ctr_y[i] = 0;
	double *xp = xpts, *yp = ypts;
	for( int k=cir_sz; --k>=0; ++xp,++yp ) {
		double x = rx + *xp, y = ry + *yp;
		for( int i=0; i<3; ++pix[i],++i ) {
			double v = *pix[i];
			ctr_v[i] += v;
			ctr_x[i] += v * x;
			ctr_y[i] += v * y;
		}
	}
	for( int i=0; i<3; ++i ) {
		if( !ctr_v[i] ) continue;
		ctr_x[i] /= ctr_v[i];
		ctr_y[i] /= ctr_v[i];
		ctr_v[i] /= cir_sz;
	}
}

Motion51VVFrame::Motion51VVFrame(VFrame *vfrm, int n)
 : VFrame(vfrm->get_data(), -1, vfrm->get_y()-vfrm->get_data(),
	vfrm->get_u()-vfrm->get_data(), vfrm->get_v()-vfrm->get_data(),
	vfrm->get_w(), vfrm->get_h(), vfrm->get_color_model(),
	vfrm->get_bytes_per_line())
{
	this->n = n;
}

int Motion51VVFrame::draw_pixel(int x, int y)
{
	VFrame::draw_pixel(x+0, y+0);
	for( int i=1; i<n; ++i ) {
		VFrame::draw_pixel(x-i, y-i);
		VFrame::draw_pixel(x-i, y+i);
		VFrame::draw_pixel(x+i, y-i);
		VFrame::draw_pixel(x+i, y+i);
	}
	return 0;
}

void Motion51Main::draw_vectors(VFrame *img)
{
	int iw = img->get_w(), ih = img->get_h();
	int mx = iw > ih ? iw : ih;
	int n = mx/800 + 1;
	Motion51VVFrame vfrm(img, n);
	vfrm.set_pixel_color(WHITE);
	int m = 2;  while( m < n ) m <<= 1;
	vfrm.set_stiple(2*m);

	vfrm.draw_arrow(rx, ry, rx+current_dx, ry+current_dy);
//	vfrm.draw_smooth(rx-rr,ry, rx-rr,ry+rr, rx,ry+rr);
//	vfrm.draw_smooth(rx,ry+rr, rx+rr,ry+rr, rx+rr,ry);
//	vfrm.draw_smooth(rx+rr,ry, rx+rr,ry-rr, rx,ry-rr);
//	vfrm.draw_smooth(rx,ry-rr, rx-rr,ry-rr, rx-rr,ry);

	float rx1 = rx - 0.5*rw;
	float ry1 = ry - 0.5*rh;
	float rx2 = rx1 + rw;
	float ry2 = ry1 + rh;

	vfrm.draw_line(rx1, ry1, rx2, ry1);
	vfrm.draw_line(rx2, ry1, rx2, ry2);
	vfrm.draw_line(rx2, ry2, rx1, ry2);
	vfrm.draw_line(rx1, ry2, rx1, ry1);

	float sx1 = rx1 - rr, sy1 = ry1 - rr;
	float sx2 = rx2 + rr, sy2 = ry2 + rr;

	vfrm.draw_smooth(sx1, ry1, sx1, sy1, rx1, sy1);
	vfrm.draw_line(rx1, sy1, rx2, sy1);
	vfrm.draw_smooth(rx2, sy1, sx2, sy1, sx2, ry1);
	vfrm.draw_line(sx2, ry1, sx2, ry2);
	vfrm.draw_smooth(sx2, ry2, sx2, sy2, rx2, sy2);
	vfrm.draw_line(rx2, sy2, rx1, sy2);
	vfrm.draw_smooth(rx1, sy2, sx1, sy2, sx1, ry2);
	vfrm.draw_line(sx1, ry2, sx1, ry1);

	double *xp = xpts, *yp = ypts;
	for( int i=cir_sz; --i>=0; ++xp, ++yp )
		vfrm.draw_pixel(rx+*xp, ry+*yp);
}

int Motion51Main::open_cache_file()
{
	if( cache_fp ) return 0;
	if( !cache_file[0] ) return 1;
	if( !(cache_fp = fopen(cache_file, "r")) ) return 1;
// match timestamp, asset path
	char line[BCTEXTLEN], *cp = line, *ep = cp+sizeof(line);
	if( fgets(line,sizeof(line),cache_fp) ) {
		int64_t tm = strtoul(cp,&cp,0);
// time 0 matches everything
		if( !tm ) return 0;
		const char *sp = get_source_path();
		if( !sp ) return 0;
		if( cp < ep && *cp == ' ' ) ++cp;
		int n = strlen(cp);
		if( n > 0 && cp[n-1] == '\n' ) cp[n-1] = 0;
		struct stat st;
		if( !strcmp(cp, sp) && !stat(cp,&st) && st.st_mtime == tm )
			return 0;
	}
	fclose(cache_fp);  cache_fp = 0;
	return 1;
}

void Motion51Main::close_cache_file()
{
	if( !cache_fp ) return;
	fclose(cache_fp);
	cache_fp = 0; cache_key = -1;
	tracking_position = -1;
}

int Motion51Main::load_cache_line()
{
	cache_key = -1;
	if( open_cache_file() ) return 1;
	if( !fgets(cache_line, sizeof(cache_line), cache_fp) ) return 1;
	cache_key = strtol(cache_line, 0, 0);
	return 0;
}

int Motion51Main::get_cache_line(int64_t key)
{
	if( cache_key == key ) return 0;
	if( open_cache_file() ) return 1;
	if( cache_key >= 0 && key > cache_key ) {
		if( load_cache_line() ) return 1;
		if( cache_key == key ) return 0;
		if( cache_key > key ) return 1;
	}
// binary search file
	fseek(cache_fp, 0, SEEK_END);
	int64_t l = -1, r = ftell(cache_fp);
	while( (r - l) > 1 ) {
		int64_t m = (l + r) / 2;
		fseek(cache_fp, m, SEEK_SET);
// skip to start of next line
		if( !fgets(cache_line, sizeof(cache_line), cache_fp) )
			return -1;
		if( !load_cache_line() ) {
			if( cache_key == key )
				return 0;
			if( cache_key < key ) { l = m; continue; }
		}
		r = m;
	}
	return 1;
}

int Motion51Main::locate_cache_line(int64_t key)
{
	int ret = 1;
	if( key < 0 || !(ret=get_cache_line(key)) ||
	    ( cache_key >= 0 && cache_key < key ) )
		ret = load_cache_line();
	return ret;
}

int Motion51Main::put_cache_line(const char *line)
{
	int64_t key = strtol(line, 0, 0);
	if( key == active_key ) return 1;
	if( !active_fp ) {
		close_cache_file();
		snprintf(cache_file, sizeof(cache_file), "%s.bak", config.tracking_file);
		::rename(config.tracking_file, cache_file);
		if( !(active_fp = fopen(config.tracking_file, "w")) ) {
			perror(config.tracking_file);
			fprintf(stderr, "err writing key %jd\n", key);
			return -1;
		}
		const char *sp = get_source_path();
		int64_t tm = 0;
		if( sp ) {
			struct stat st;
			if( !stat(sp,&st) ) tm = st.st_mtime;
		}
		fprintf(active_fp, "%jd %s\n",tm, sp);
		active_key = -1;
	}

	if( active_key < key ) {
		locate_cache_line(active_key);
		while( cache_key >= 0 && key >= cache_key ) {
			if( key > cache_key )
				fputs(cache_line, active_fp);
			load_cache_line();
		}
	}

	active_key = key;
	fputs(line, active_fp);
	fflush(active_fp);
	return 0;
}

void Motion51Main::update_cache_file()
{
	if( active_fp ) {
		locate_cache_line(active_key);
		while( cache_key >= 0 ) {
			fputs(cache_line, active_fp);
			load_cache_line();
		}
		close_cache_file();
		::remove(cache_file);
		fclose(active_fp);  active_fp = 0;
		active_key = -1;
	}
	else
	  	close_cache_file();
	strcpy(cache_file, config.tracking_file);
}

Motion51Scan::Motion51Scan(Motion51Main *plugin, int n_thds, int x_steps, int y_steps)
 : LoadServer(n_thds, x_steps*y_steps)
{
	this->plugin = plugin;
	this->x_steps = x_steps;
	this->y_steps = y_steps;
	this->fft = new FFT;
	this->result_lock = new Mutex("Motion51Scan::result_lock");

	cur = ref = 0;
	bx = by = br = bw = bh = 0;
	rpix_sz = 0;
	for( int i=0; i<3; ++i ) {
		rpix[i] = 0;  rpwr[i] = 0;
		rctr_v[i] = rctr_x[i] = rctr_y[i] = 0;
		ref_real[i] = ref_imag[i] = 0;
	}
	cor_value = value = 0;
	dx_result = dy_result = 0;
	dt_result = 0;
}


Motion51Scan::~Motion51Scan()
{
	for( int i=0; i<3; ++i ) {
		delete [] rpix[i];
		delete [] ref_real[i];
		delete [] ref_imag[i];
	}
	delete fft;
	delete result_lock;
}

// sum absolute diff of ref at (rx,ry) - (cur(cx,cy) rotated ct)
//   downsampled using corner_sample to x_steps, y_steps
double Motion51Scan::compare(double cx, double cy, double ct)
{
	int iw = ref->get_w(), iw1 = iw-1;
	int ih = ref->get_h(), ih1 = ih-1;
	int xsz = x_steps;// iw;
	int ysz = y_steps;// ih;
	int psz = BC_CModels::calculate_pixelsize(cur->get_color_model());
	uint8_t **ref_rows = ref->get_rows();
	uint8_t **cur_rows = cur->get_rows();
	double cos_ct = cos(ct), sin_ct = sin(ct);
	double rx = plugin->rx, ry = plugin->ry;
	double sx = (double)iw/xsz, sy = (double)ih/ysz;
	double cpix[3][xsz], rpix[3][xsz];
	double v = 0;
	for( int iy=0; iy<y_steps; ++iy ) {
		double y = iy * sy;
		double *ref_pix[3] = { rpix[0], rpix[1], rpix[2] };
		double *cur_pix[3] = { cpix[0], cpix[1], cpix[2] };
		for( int ix=0; ix<x_steps; ++ix ) {
			double x = ix * sx;
			plugin->get_pixel(ref_pix, x, y, ref_rows, psz, iw1, ih1);
			double tx = x-rx, ty = y-ry;
			double xt = cos_ct*tx - sin_ct*ty + cx;
			double yt = cos_ct*ty + sin_ct*tx + cy;
			plugin->get_pixel(cur_pix, xt, yt, cur_rows, psz, iw1, ih1);
		}
		for( int i=0; i<3; ++i ) {
			double *rp = rpix[i], *cp = cpix[i];
			for( int k=x_steps; --k>=0; ++rp,++cp ) v += fabs(*rp - *cp);
		}
	}
	double mx = BC_CModels::calculate_max(ref->get_color_model());
	v = 1.-v/(3*mx * x_steps*y_steps);
	return v;
}

void Motion51Scan::scan(VFrame *ref, VFrame *cur, int sz)
{
	this->ref = ref;
	this->cur = cur;
	if( this->rpix_sz != sz ) {
		this->rpix_sz = sz;
		for( int i=0; i<3; ++i ) {
			delete [] rpix[i];      rpix[i] = new double[sz];
			delete [] ref_real[i];  ref_real[i] = new double[sz];
			delete [] ref_imag[i];  ref_imag[i] = new double[sz];
		}
	}

	bw = plugin->rw;  bh = plugin->rh;
	bx = plugin->rx;  by = plugin->ry;
	br = plugin->rr;

	double *ref_pix[3] = { rpix[0], rpix[1], rpix[2] };
	plugin->get_samples(ref, ref_pix, bx, by);
	double *pix[3] = { rpix[0], rpix[1], rpix[2] };
	plugin->centroid(&pix[0], &rctr_v[0], &rctr_x[0], &rctr_y[0]);
	for( int i=0; i<3; ++i ) {
		fft->do_fft(sz, 0, rpix[i], 0, ref_real[i], ref_imag[i]);
		rpwr[i] = cor(sz, rpix[i], rpix[i]);
	}
	double scan_limit = 0.25; // quarter pixel resolution
//printf("frame: %jd\n", plugin->get_source_position());
	while( bw/x_steps > scan_limit || bh/y_steps > scan_limit ) {
		dx_result = dy_result = dt_result = 0;
		cor_value = value = 0;
//printf("  bx,by %6.2f,%-6.2f  bw,bh %6.2f,%-6.2f ",bx,by, bw,bh);
		process_packages();
		bx = dx_result;
		by = dy_result;
//printf(" r = %f(%f), %6.2f,%-6.2f\n",value,dt_result*180/M_PI,bx,by);
		bw *= 0.5;  bh *= 0.5;
	}
}

void Motion51Scan::init_packages()
{
// sort in increasing distance from current displacement
	double tx = plugin->rx + plugin->total_dx;
	double ty = plugin->ry + plugin->total_dy;
	int npkgs = get_total_packages();
	coord_t coords[npkgs];
	int i = 0;
	double x0 = bx - bw/2, y0 = by - bh/2;
	for( int iy=0; iy<y_steps; ++iy ) {
		double y = y0 + iy*bh/y_steps;
		for( int ix=0; ix<x_steps; ++ix ) {
			double x = x0 + ix*bw/x_steps;
			double d = sqrt(sqr(x-tx) + sqr(y-ty));
			coord_t *cp = coords + i++;
			cp->x = x; cp->y = y; cp->d = d;
		}
	}
	qsort(&coords,npkgs,sizeof(coords[0]),coord_cmpr);

	for( i=0; i<npkgs; ++i ) {
		coord_t *cp = coords + i;
		Motion51ScanPackage *pkg = (Motion51ScanPackage*)get_package(i);
		pkg->x = cp->x;  pkg->y = cp->y;
	}
}

Motion51ScanUnit::Motion51ScanUnit(Motion51Scan *server, Motion51Main *plugin)
 : LoadClient(server)
{
	this->server = server;
	this->plugin = plugin;
	fft = new FFT;
}
Motion51ScanUnit::~Motion51ScanUnit()
{
	delete fft;
}

void Motion51ScanUnit::process_package(LoadPackage *package)
{
	Motion51ScanPackage *pkg = (Motion51ScanPackage *)package;
	int sz = server->rpix_sz;
	double cur_real[3][sz], cur_imag[3][sz];
	double cpwr[3], cpix[3][sz], *cur_pix[3] = { cpix[0], cpix[1], cpix[2] };
	plugin->get_samples(server->cur, cur_pix, pkg->x, pkg->y);

	double *pix[3] = { cpix[0], cpix[1], cpix[2] };
	double cctr_v[3], cctr_x[3], cctr_y[3];
	plugin->centroid(&pix[0], &cctr_v[0], &cctr_x[0], &cctr_y[0]);
	double mx = BC_CModels::calculate_max(server->ref->get_color_model());
	for( int i=0; i<3; ++i ) {
		double v = 1. - fabs(server->rctr_v[i]-cctr_v[i]) / mx;
		if( !passible(v) ) return;
		double *rctr_x = server->rctr_x, *rctr_y = server->rctr_y;
		double d = sqrt(sqr(rctr_x[i]-cctr_x[i]) + sqr(rctr_y[i]-cctr_y[i]));
		v = 1 - d / plugin->cir_r;
		if( !passible(v) ) return;
	}
	for( int i=0; i<3; ++i ) {
		double cs = cor(sz, cpix[i], cpix[i]);
		double rs = server->rpwr[i];
		double ss = rs + cs;
		if( ss == 0 ) ss = 1;
		double v = 1. - fabs(rs - cs) / ss;
		if( ! passible(v) ) return;
		cpwr[i] = 1. / ss;
	}

	double cor_real[3][sz], cor_imag[3][sz];
	for( int i=0; i<3; ++i ) {
		fft->do_fft(sz, 0, cpix[i], 0, cur_real[i], cur_imag[i]);
		cj_product(sz, 0, cur_real[i], cur_imag[i],
			server->ref_real[i], server->ref_imag[i],
			cur_real[i], cur_imag[i]);
		fft->do_fft(sz, 1, cur_real[i], cur_imag[i], cor_real[i], cor_imag[i]);
	}
	double sv = 0;
	int st = 0;
	for( int t=0; t<sz; ++t ) {
		double v = 0;
		for( int i=0; i<3; ++i ) v += cor_real[i][t] * cpwr[i];
		v = ((2*v) / 3);
		if( sv >= v ) continue;
		sv = v;  st = t;
	}
	if( server->cor_value > sv ) return;
	server->cor_value = sv;
	if( st > sz/2 ) st -= sz;
	int steps = plugin->r_steps;
	double tt = steps*(2*M_PI);
	double th = st*tt/sz, dt = th;
	double value = 0;
	double dth = (2*M_PI)/sz;
	for( int i=-steps; i<=steps; ++i ) {
		double t = th + i*dth;
		double v = server->compare(pkg->x, pkg->y, -t);
		if( value >= v ) continue;
		value = v;  dt = t;
	}
//static int dbg = 0;
//if( dbg )
//printf("  %d. %.3f,%.3f %f = %f / %f + %f\n",
// package_number,pkg->x,pkg->y,dt*180./M_PI,value,sv);
	server->result_lock->lock("Motion51Scan::process_package");
	if( value > server->value ) {
		server->value = value;
		server->dt_result = dt;
		server->dx_result = pkg->x;
		server->dy_result = pkg->y;
	}
	server->result_lock->unlock();
}

