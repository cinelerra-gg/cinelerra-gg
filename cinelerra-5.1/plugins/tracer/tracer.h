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



#ifndef __TRACER_H__
#define __TRACER_H__

#include "loadbalance.h"
#include "pluginvclient.h"

class Tracer;

enum { PT_X, PT_Y, PT_SZ };

class TracerPoint
{
public:
	float x, y;

	TracerPoint(float x, float y);
	~TracerPoint();
};
class TracerPoints : public ArrayList<TracerPoint *>
{
public:
	TracerPoints() {}
	~TracerPoints() { remove_all_objects(); }
};

class TracerConfig
{
public:
	TracerConfig();
	~TracerConfig();

	int equivalent(TracerConfig &that);
	void copy_from(TracerConfig &that);
	void interpolate(TracerConfig &prev, TracerConfig &next,
		long prev_frame, long next_frame, long current_frame);
	void limits();

	TracerPoints points;
	int add_point(float x, float y);
	int add_point();
	void del_point(int i);

	int drag, draw, fill;
	int invert, feather;
	float radius; 
	int selected;
};

class TracePoint
{
public:
	int x, y, n;
	TracePoint() {}
	TracePoint(int x, int y) {
		this->x = x; this->y = y;
		this->n = 0;
	}
};

class TracePoints : public ArrayList<TracePoint>
{
public:
	TracePoints() {}
	void add(int x, int y) {
		TracePoint *np = &append();
		np->x = x;  np->y = y;
	}
	void clear() { remove_all(); }
};


class Tracer : public PluginVClient
{
public:
	Tracer(PluginServer *server);
	~Tracer();
// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS2(TracerConfig)
	int is_realtime();
	void update_gui();
	int new_point();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int process_buffer(VFrame *frame, int64_t start_position, double frame_rate);
	void draw_edge();
	void draw_mask();
	void draw_point(TracerPoint *pt);
	void draw_points();
	int step();
	void trace(int i0, int i1);
	int smooth();
	void feather(int r, double s);
	int load_configuration1();

	VFrame *edg, *msk, *frm;
	uint8_t **edg_rows;
	uint8_t **msk_rows;
	uint8_t **frm_rows;
	TracePoints points;
	int w, w1, h, h1, pts;
	int ax, ay, bx, by, cx, cy;
	int ex, ey, nx, ny;
	int color_model, bpp;
	int is_float, is_yuv, has_alpha;
	int comps, comp;
};

#endif
