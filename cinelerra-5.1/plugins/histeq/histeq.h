
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#ifndef HISTOGRAM_H
#define HISTOGRAM_H


#include "histeq.inc"
#include "loadbalance.h"
#include "bccolors.h"
#include "pluginvclient.h"

class HistEqConfig {
public:
	HistEqConfig();
	~HistEqConfig();

	int split;
	int plot;
	float blend;
	float gain;

	void copy_from(HistEqConfig &that);
	int equivalent(HistEqConfig &that);
	void interpolate(HistEqConfig &prev, HistEqConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame);
};

class HistEqSplit : public BC_CheckBox
{
public:
	HistEqSplit(HistEqWindow *gui, HistEqMain *plugin, int x, int y);
	~HistEqSplit();
	int handle_event();

	HistEqWindow *gui;
	HistEqMain *plugin;
};

class HistEqPlot : public BC_CheckBox
{
public:
	HistEqPlot(HistEqWindow *gui, HistEqMain *plugin, int x, int y);
	~HistEqPlot();
	int handle_event();

	HistEqWindow *gui;
	HistEqMain *plugin;
};

class HistEqBlend : public BC_FSlider
{
public:
	HistEqBlend(HistEqWindow *gui, HistEqMain *plugin, int x, int y);
	~HistEqBlend();
	int handle_event();

	HistEqWindow *gui;
	HistEqMain *plugin;
};

class HistEqGain : public BC_FSlider
{
public:
	HistEqGain(HistEqWindow *gui, HistEqMain *plugin, int x, int y);
	~HistEqGain();
	int handle_event();

	HistEqWindow *gui;
	HistEqMain *plugin;
};

class HistEqCanvas : public BC_SubWindow
{
public:
        HistEqCanvas(HistEqWindow *gui, HistEqMain *plugin,
                int x, int y, int w, int h);
	void clear();
	void draw_bins(HistEqMain *plugin);
	void draw_wts(HistEqMain *plugin);
	void draw_reg(HistEqMain *plugin);
	void draw_lut(HistEqMain *plugin);
	void update(HistEqMain *plugin);

        HistEqMain *plugin;
        HistEqWindow *gui;
};

class HistEqWindow : public PluginClientWindow
{
public:
	HistEqWindow(HistEqMain *plugin);
	~HistEqWindow();

	HistEqMain *plugin;
	HistEqSplit *split;
	HistEqPlot *plot;
	HistEqBlend *blend;
	HistEqGain *gain;
	HistEqCanvas *canvas;

	void create_objects();
	void update();
};

class HistEqMain : public PluginVClient
{
public:
	HistEqMain(PluginServer *server);
	~HistEqMain();

	int process_buffer(VFrame *frame, int64_t start_position, double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	void render_gui(void *data);

	PLUGIN_CLASS_MEMBERS(HistEqConfig)

	VFrame *input, *output;
	HistEqEngine *engine;
	int w, h;
	int sz;
	double a, b;
	int binsz, bsz, *bins;
	int lutsz, lsz, *lut;
	int wsz;  float *wts;
};

class HistEqPackage : public LoadPackage
{
public:
	HistEqPackage();
	int y0, y1;
};

class HistEqUnit : public LoadClient
{
public:
	HistEqUnit(HistEqEngine *server, HistEqMain *plugin);
	~HistEqUnit();
	void process_package(LoadPackage *package);
	HistEqEngine *server;
	HistEqMain *plugin;
	int valid;
	int binsz, *bins;
};

class HistEqEngine : public LoadServer
{
public:
	HistEqEngine(HistEqMain *plugin, int total_clients, int total_packages);
	void process_packages(int operation, VFrame *data);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	HistEqMain *plugin;
	int operation;

	enum { HISTEQ, APPLY };
	VFrame *data;
};

#endif
