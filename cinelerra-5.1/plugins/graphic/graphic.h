
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

#ifndef GRAPHIC_H
#define GRAPHIC_H

#include "guicast.h"
#include "fourier.h"
#include "pluginaclient.h"


#define MAX_WINDOW 262144
//#define WINDOW_SIZE 16384
#define MAXMAGNITUDE 15
#define MAXFREQ 20000
#define MIN_DB -15
#define MAX_DB 15


class GraphicGUI;
class GraphicEQ;



class GraphicPoint
{
public:
	GraphicPoint();
// Frequency in Hz
	int freq;
// Amplitude in DB
	double value;
};



class GraphicConfig
{
public:
	GraphicConfig();
	~GraphicConfig();

	int equivalent(GraphicConfig &that);
	void copy_from(GraphicConfig &that);
	void interpolate(GraphicConfig &prev,
		GraphicConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
	void insert_point(GraphicPoint *point);
	void delete_point(int number);

	ArrayList<GraphicPoint*> points;
//	double wetness;
	int window_size;
};



class GraphicCanvas : public BC_SubWindow
{
public:
	GraphicCanvas(GraphicEQ *plugin, GraphicGUI *gui, int x, int y, int w, int h);
	virtual ~GraphicCanvas();
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();
	void process(int buttonpress, int motion, int draw);
	int freq_to_y(int freq,
		ArrayList<GraphicPoint*> *points,
		double *envelope);
	void insert_point(GraphicPoint *point);
	GraphicEQ *plugin;
	GraphicGUI *gui;

// Temporary envelope when editing
	void new_temps();
	void save_temps();

	ArrayList<GraphicPoint*> temp_points;
	double temp_envelope[MAX_WINDOW / 2];




	int state;
	enum
	{
		NONE,
		DRAG_POINT
	};
	int x_diff, y_diff;
};

class FreqTextBox : public BC_TextBox
{
public:
	FreqTextBox(GraphicEQ *plugin,
		GraphicGUI *gui,
		int x,
		int y,
		int w);
	int handle_event();
	void update(int freq);
	GraphicEQ *plugin;
	GraphicGUI *gui;
};

class ValueTextBox : public BC_TextBox
{
public:
	ValueTextBox(GraphicEQ *plugin,
		GraphicGUI *gui,
		int x,
		int y,
		int w);
	int handle_event();
	void update(float value);
	GraphicEQ *plugin;
	GraphicGUI *gui;
};

class GraphicReset : public BC_GenericButton
{
public:
	GraphicReset(GraphicEQ *plugin,
		GraphicGUI *gui,
		int x,
		int y);
	int handle_event();
	GraphicEQ *plugin;
	GraphicGUI *gui;
};


class GraphicSize : public BC_PopupMenu
{
public:
	GraphicSize(GraphicGUI *window, GraphicEQ *plugin, int x, int y);

	int handle_event();
	void create_objects();         // add initial items
	void update(int size);

	GraphicGUI *window;
	GraphicEQ *plugin;
};


class GraphicWetness : public BC_FPot
{
public:
	GraphicWetness(GraphicGUI *window, GraphicEQ *plugin, int x, int y);
	int handle_event();
	GraphicGUI *window;
	GraphicEQ *plugin;
};


class GraphicGUI : public PluginClientWindow
{
public:
	GraphicGUI(GraphicEQ *plugin);
	~GraphicGUI();

	void create_objects();
	int keypress_event();
	void update_canvas();
	int resize_event(int w, int h);
	void draw_ticks();
	void update_textboxes();

	FreqTextBox *freq_text;
	ValueTextBox *value_text;
	BC_Title *freq_title;
	BC_Title *level_title;
	BC_Title *size_title;

	GraphicCanvas *canvas;
	GraphicReset *reset;
	GraphicSize *size;
//	GraphicWetness *wetness;
	GraphicEQ *plugin;
};





class GraphicGUIFrame : public PluginClientFrame
{
public:
	GraphicGUIFrame(int window_size, int sample_rate);
	virtual ~GraphicGUIFrame();
	double *data;
// Maximum of window in frequency domain
	double freq_max;
// Maximum of window in time domain
	double time_max;
	int window_size;
};



class GraphicFFT : public CrossfadeFFT
{
public:
	GraphicFFT(GraphicEQ *plugin);
	~GraphicFFT();

	int post_process();
	int signal_process();
	int read_samples(int64_t output_sample,
		int samples,
		Samples *buffer);
// Current GUI frame being filled
	GraphicGUIFrame *frame;

	GraphicEQ *plugin;
};



class GraphicEQ : public PluginAClient
{
public:
	GraphicEQ(PluginServer *server);
	~GraphicEQ();

	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_buffer(int64_t size,
		Samples *buffer,
		int64_t start_position,
		int sample_rate);
	void update_gui();
	double freq_to_magnitude(double frequency,
		ArrayList<GraphicPoint*> *points,
		double *envelope);
	void calculate_envelope(ArrayList<GraphicPoint*> *points,
		double *envelope);
	int active_point_exists();
	void reconfigure();


	PLUGIN_CLASS_MEMBERS(GraphicConfig)



	double envelope[MAX_WINDOW / 2];
	int active_point;
// For refreshing the canvas
	GraphicGUIFrame *last_frame;
	GraphicFFT *fft;
	int need_reconfigure;
	int w, h;
};



#endif // GRAPHIC_H
