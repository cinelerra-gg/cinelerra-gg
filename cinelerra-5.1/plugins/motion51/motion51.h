
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef __MOTION_51_H__
#define __MOTION_51_H__

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "affine.inc"
#include "bchash.inc"
#include "filexml.inc"
#include "keyframe.inc"
#include "loadbalance.h"
#include "motion51.inc"
#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"

class FFT;
class Motion51Config;
class Motion51Main;
class Motion51ScanUnit;
class Motion51ScanPackage;
class Motion51Scan;
class Motion51VVFrame;

class Motion51Config
{
public:
	Motion51Config();

	void init();
	int equivalent(Motion51Config &that);
	void copy_from(Motion51Config &that);
	void interpolate(Motion51Config &prev, Motion51Config &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame);

	float block_x, block_y, block_w, block_h;
	float sample_r;
	int sample_steps;
	float horiz_limit, vert_limit, twist_limit;
	float shake_fade, twist_fade;
	int draw_vectors;

	char tracking_file[BCTEXTLEN];
	int tracking;
};


class Motion51Main : public PluginVClient
{
public:
	Motion51Main(PluginServer *server);
	~Motion51Main();

	int process_buffer(VFrame **frame, int64_t position, double frame_rate);
	int is_multichannel();
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	void reset_sample(int sz, double r);
	void get_samples(VFrame *img, double *pix[3], double x, double y);
	void centroid(double *pix[3], double *ctr_v, double *ctr_x, double *ctr_y);
	void draw_vectors(VFrame *img);

	int64_t get_ref_position();
	void update_tracking_cache();
	int load_tracking_cache(int64_t position);
	void save_tracking_cache(int64_t position);

	void match(VFrame *ref, VFrame *cur);
	int transform_target(int use_opengl);
	int handle_opengl();
	VFrame* new_temp(VFrame *&tmp, VFrame *ref);

	PLUGIN_CLASS_MEMBERS2(Motion51Config)

	char cache_file[BCTEXTLEN];
	FILE *cache_fp, *active_fp;
	void set_tracking_path();
	int open_cache_file();
	void close_cache_file();
	int load_cache_line();
	int locate_cache_line(int64_t key);
	int get_cache_line(int64_t key);
	int put_cache_line(const char *line);
	void update_cache_file();
	void (*get_pixel)(double *pix[3], double rx, double ry,
		uint8_t **rows, int psz, int iw1, int ih1);

	VFrame *out;
	VFrame *out_frame;
	int64_t out_position;
	VFrame *ref_frame;
	int64_t ref_position;
	VFrame *tmp_frame;
	AffineEngine *affine;
	Motion51Scan *motion_scan;

	char cache_line[BCSTRLEN];
	int64_t cache_key, active_key;
	int64_t tracking_position;
	float out_w, out_h, out_r;
	float dx, dy, dt;
	double rx, ry, rr, rw, rh;
	float current_dx, current_dy;
	int x_steps, y_steps, r_steps;
	double cir_r;  int cir_sz;
	double *xpts, *ypts;
	float total_dx, total_dy;
	float total_angle;
};

class Motion51ScanPackage : public LoadPackage
{
public:
	Motion51ScanPackage() {}
	~Motion51ScanPackage() {}
	double x, y;
};
class Motion51ScanUnit : public LoadClient
{
public:
	Motion51ScanUnit(Motion51Scan *server, Motion51Main *plugin);
	~Motion51ScanUnit();
	void process_package(LoadPackage *package);

	Motion51Scan *server;
	Motion51Main *plugin;
	FFT *fft;
};

class Motion51Scan : public LoadServer
{
public:
	Motion51Scan(Motion51Main *plugin, int total_clients, int x_steps, int y_steps);
	~Motion51Scan();
	friend class Motion51ScanUnit;

	Motion51Main *plugin;
	Mutex *result_lock;
	FFT *fft;
	VFrame *cur, *ref;

	int x_steps, y_steps;
	double bx, by, br, bw, bh;
	int rpix_sz;
	double *rpix[3], rpwr[3];
	double rctr_v[3], rctr_x[3], rctr_y[3];
	double *ref_real[3], *ref_imag[3];
	double cor_value, value;
	double dx_result, dy_result, dt_result;

	void init_packages();
	LoadClient *new_client() { return new Motion51ScanUnit(this, plugin); }
	LoadPackage *new_package() { return new Motion51ScanPackage(); }
	void scan(VFrame *ref, VFrame *cur, int sz);
	double compare(double cx, double cy, double ct);
};

class Motion51VVFrame : public VFrame
{
public:
	Motion51VVFrame(VFrame *vfrm, int n);
	int draw_pixel(int x, int y);
	int n;
};

#endif
