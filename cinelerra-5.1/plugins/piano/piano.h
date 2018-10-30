
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

#ifndef PIANO_H
#define PIANO_H



#include "filexml.inc"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"


class Piano;
class PianoWindow;


class PianoCanvas;
class PianoWaveForm;
class PianoBaseFreq;
class PianoFreqPot;
class PianoOscGUI;
class PianoScroll;
class PianoSubWindow;
class PianoWetness;

class PianoWindow : public BC_Window
{
public:
	PianoWindow(Piano *plugin, int x, int y);
	~PianoWindow();

	void create_objects();
	int close_event();
	int resize_event(int w, int h);
	void update_gui();
	int waveform_to_text(char *text, int waveform);
	void update_scrollbar();
	void update_oscillators();


	Piano *plugin;
	PianoCanvas *canvas;
};


class PianoCanvas : public BC_SubWindow
{
public:
	PianoCanvas(Piano *plugin,
		PianoWindow *window,
		int x,
		int y,
		int w,
		int h);
	~PianoCanvas();

	int update();
	Piano *plugin;
	PianoWindow *window;
};


class PianoThread : public Thread
{
public:
	PianoThread(Piano *synth);
	~PianoThread();

	void run();

	Mutex completion;
	Piano *synth;
	PianoWindow *window;
};



class PianoConfig
{
public:
	PianoConfig();
	~PianoConfig();

	int equivalent(PianoConfig &that);
	void copy_from(PianoConfig &that);
	void interpolate(PianoConfig &prev,
		PianoConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
	void reset();

};


class Piano : public PluginAClient
{
public:
	Piano(PluginServer *server);
	~Piano();



	int is_realtime();
	int is_synthesis();
	int load_configuration();
	const char* plugin_title();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int show_gui();
	void raise_window();
	int set_string();
	int process_realtime(int64_t size, double *input_ptr, double *output_ptr);







	void add_oscillator();
	void delete_oscillator();
	double get_total_power();
	double get_oscillator_point(float x,
		double normalize_constant,
		int oscillator);
	double solve_eqn(double *output,
		double x1,
		double x2,
		double normalize_constant,
		int oscillator);
	double get_point(float x, double normalize_constant);
	double function_square(double x);
	double function_pulse(double x);
	double function_noise();
	double function_sawtooth(double x);
	double function_triangle(double x);
	void reconfigure();
	int overlay_synth(int64_t start, int64_t length, double *input, double *output);
	void update_gui();
	void reset();



	double *dsp_buffer;
	int need_reconfigure;
	PianoThread *thread;
	PianoConfig config;
	int w, h;
	DB db;
	int64_t waveform_length;           // length of loop buffer
	int64_t waveform_sample;           // current sample in waveform of loop
	int64_t samples_rendered;          // samples of the dsp_buffer rendered since last buffer redo
	float period;            // number of samples in a period for this frequency
};








#endif
