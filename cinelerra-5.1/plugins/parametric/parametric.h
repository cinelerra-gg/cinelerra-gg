
/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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

#ifndef PARAMETRIC_H
#define PARAMETRIC_H


#include "bchash.inc"
#include "fourier.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"


// This parametric EQ multiplies the data by a gaussian curve in frequency domain.
// It causes significant delay but is useful.


#define BANDS 3
#define MAXMAGNITUDE 15
#define MAX_WINDOW 262144



class ParametricConfig;
class ParametricThread;
class ParametricFreq;
class ParametricQuality;
class ParametricMagnitude;
class ParametricBandGUI;
class ParametricWindow;
class ParametricFFT;
class ParametricEQ;




class ParametricBand
{
public:
	ParametricBand();

	int equivalent(ParametricBand &that);
	void copy_from(ParametricBand &that);
	void interpolate(ParametricBand &prev,
		ParametricBand &next,
		double prev_scale,
		double next_scale);

	enum
	{
		NONE,
		LOWPASS,
		HIGHPASS,
		BANDPASS
	};

	int freq;
	float quality;
	float magnitude;
	int mode;
};


class ParametricConfig
{
public:
	ParametricConfig();

	int equivalent(ParametricConfig &that);
	void copy_from(ParametricConfig &that);
	void interpolate(ParametricConfig &prev,
		ParametricConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);

	ParametricBand band[BANDS];
	float wetness;
	int window_size;
};





class ParametricFreq : public BC_QPot
{
public:
	ParametricFreq(ParametricEQ *plugin, int x, int y, int band);

	int handle_event();

	int band;
	ParametricEQ *plugin;
};


class ParametricQuality : public BC_FPot
{
public:
	ParametricQuality(ParametricEQ *plugin, int x, int y, int band);

	int handle_event();

	int band;
	ParametricEQ *plugin;
};


class ParametricMagnitude : public BC_FPot
{
public:
	ParametricMagnitude(ParametricEQ *plugin, int x, int y, int band);

	int handle_event();

	int band;
	ParametricEQ *plugin;
};




class ParametricMode : public BC_PopupMenu
{
public:
	ParametricMode(ParametricEQ *plugin, int x, int y, int band);

	void create_objects();
	int handle_event();
	static int text_to_mode(char *text);
	static const char* mode_to_text(int mode);

	int band;
	ParametricEQ *plugin;
};





class ParametricBandGUI
{
public:
	ParametricBandGUI(ParametricEQ *plugin,
		ParametricWindow *window,
		int x,
		int y,
		int band);
	~ParametricBandGUI();

	void create_objects();
	void update_gui();

	int band;
	int x, y;
	ParametricEQ *plugin;
	ParametricWindow *window;
	ParametricFreq *freq;
	ParametricQuality *quality;
	ParametricMagnitude *magnitude;
	ParametricMode *mode;
};


class ParametricSize : public BC_PopupMenu
{
public:
	ParametricSize(ParametricWindow *window, ParametricEQ *plugin, int x, int y);

	int handle_event();
	void create_objects();         // add initial items
	void update(int size);

	ParametricWindow *window;
	ParametricEQ *plugin;
};




class ParametricWetness : public BC_FPot
{
public:
	ParametricWetness(ParametricEQ *plugin, int x, int y);
	int handle_event();
	ParametricEQ *plugin;
};


class ParametricWindow : public PluginClientWindow
{
public:
	ParametricWindow(ParametricEQ *plugin);
	~ParametricWindow();

	void create_objects();
	void update_gui();
	void update_canvas();

	BC_SubWindow *canvas;
	ParametricEQ *plugin;
	ParametricBandGUI* bands[BANDS];
	ParametricWetness *wetness;
	ParametricSize *size;
};


class ParametricGUIFrame : public PluginClientFrame
{
public:
	ParametricGUIFrame(int window_size, int sample_rate);
	virtual ~ParametricGUIFrame();
	double *data;
// Maximum of window in frequency domain
	double freq_max;
// Maximum of window in time domain
	double time_max;
	int window_size;
};


class ParametricFFT : public CrossfadeFFT
{
public:
	ParametricFFT(ParametricEQ *plugin);
	~ParametricFFT();

	int signal_process();
	int post_process();
	int read_samples(int64_t output_sample,
		int samples,
		Samples *buffer);

	ParametricEQ *plugin;
// Current GUI frame being filled
	ParametricGUIFrame *frame;
};


class ParametricEQ : public PluginAClient
{
public:
	ParametricEQ(PluginServer *server);
	~ParametricEQ();

	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_buffer(int64_t size,
		Samples *buffer,
		int64_t start_position,
		int sample_rate);

	void reset();
	void reconfigure();
	void update_gui();

	double calculate_envelope();
	double gauss(double sigma, double a, double x);

	double *envelope;
	int need_reconfigure;
	PLUGIN_CLASS_MEMBERS(ParametricConfig)
	ParametricFFT *fft;
// For refreshing the canvas
	ParametricGUIFrame *last_frame;
};



#endif
