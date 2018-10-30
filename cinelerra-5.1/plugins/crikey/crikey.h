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



#ifndef __CRIKEY_H__
#define __CRIKEY_H__

#include "loadbalance.h"
#include "pluginvclient.h"

class CriKeyEngine;
class CriKey;

#define DRAW_ALPHA      0
#define DRAW_EDGE       1
#define DRAW_MASK       2
#define DRAW_MODES      3

enum { PT_E, PT_X, PT_Y, PT_T, PT_TAG, PT_SZ }; // enable, x,y,threshold, tag

class CriKeyPoint
{
public:
	int tag, e;
	float x, y, t;

	CriKeyPoint(int tag, int e, float x, float y, float t);
	~CriKeyPoint();
};
class CriKeyPoints : public ArrayList<CriKeyPoint *>
{
public:
	CriKeyPoints() {}
	~CriKeyPoints() { remove_all_objects(); }
};

class CriKeyConfig
{
public:
	CriKeyConfig();
	~CriKeyConfig();

	int equivalent(CriKeyConfig &that);
	void copy_from(CriKeyConfig &that);
	void interpolate(CriKeyConfig &prev, CriKeyConfig &next,
		long prev_frame, long next_frame, long current_frame);
	void limits();

	CriKeyPoints points;
	int add_point(int tag, int e, float x, float y, float t);
	int add_point();
	void del_point(int i);

	float threshold;
	int draw_mode;
	int drag, selected;
};

class CriKeyPackage : public LoadPackage
{
public:
	CriKeyPackage() : LoadPackage() {}
	int y1, y2;
};

class CriKeyEngine : public LoadServer
{
public:
	CriKeyEngine(CriKey *plugin, int total_clients, int total_packages)
	 : LoadServer(total_clients, total_packages) { this->plugin = plugin; }
	~CriKeyEngine() {}

	void init_packages();
	LoadPackage* new_package();
	LoadClient* new_client();

	CriKey *plugin;
	int set_color(int x, int y, float t);
	float color[3], threshold;
};

class CriKeyUnit : public LoadClient
{
public:
	CriKeyUnit(CriKeyEngine *server)
	 : LoadClient(server) { this->server = server; }
	~CriKeyUnit() {}

	float edge_detect(float *data, float max, int do_max);
	void process_package(LoadPackage *package);
	CriKeyEngine *server;
};


class CriKey : public PluginVClient
{
public:
	CriKey(PluginServer *server);
	~CriKey();
// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS2(CriKeyConfig)
	int is_realtime();
	void update_gui();
	int new_point();
	int set_target(float *color, int x, int y);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int process_buffer(VFrame *frame, int64_t start_position, double frame_rate);
	void draw_alpha(VFrame *msk);
	void draw_edge(VFrame *frm);
	void draw_mask(VFrame *frm);
	void draw_point(VFrame *msk, CriKeyPoint *pt);

	CriKeyEngine *engine;
	VFrame *src, *edg, *msk;
	int w, h, color_model, bpp, comp;
	int is_yuv, is_float;
};

#endif
