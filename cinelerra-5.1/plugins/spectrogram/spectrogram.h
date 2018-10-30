
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

#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H






#include "bchash.inc"
#include "bctimer.inc"
#include "fourier.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"




class Spectrogram;

#define MIN_XZOOM 1
#define MAX_XZOOM 256
#define MIN_WINDOW 1024
#define MAX_WINDOW 65536
#define DIVISIONS 10
#define DIVISION_W 60
#define MARGIN 10
#define MAX_COLUMNS 1024
#define MAX_HISTORY 32
#define MIN_HISTORY 1
#define MIN_FREQ 1
#define MAX_FREQ 30000

// mode
#define VERTICAL 0
#define HORIZONTAL 1


class SpectrogramLevel : public BC_FPot
{
public:
	SpectrogramLevel(Spectrogram *plugin, int x, int y);
	int handle_event();
	Spectrogram *plugin;
};

class SpectrogramMode : public BC_PopupMenu
{
public:
	SpectrogramMode(Spectrogram *plugin,
		int x,
		int y);
	int handle_event();
	static const char* mode_to_text(int mode);
	static int text_to_mode(const char *text);
	void create_objects();
	Spectrogram *plugin;
};

class SpectrogramHistory : public BC_IPot
{
public:
	SpectrogramHistory(Spectrogram *plugin,
		int x,
		int y);
	int handle_event();
	Spectrogram *plugin;
};

class SpectrogramWindowSize : public BC_PopupMenu
{
public:
	SpectrogramWindowSize(Spectrogram *plugin,
		int x,
		int y,
		char *text);
	int handle_event();
	Spectrogram *plugin;
};

class SpectrogramWindowSizeTumbler : public BC_Tumbler
{
public:
	SpectrogramWindowSizeTumbler(Spectrogram *plugin, int x, int y);
	int handle_up_event();
	int handle_down_event();
	Spectrogram *plugin;
};

class SpectrogramFragmentSize : public BC_PopupMenu
{
public:
	SpectrogramFragmentSize(Spectrogram *plugin,
		int x,
		int y,
		char *text);
	int handle_event();
	Spectrogram *plugin;
};

class SpectrogramFragmentSizeTumbler : public BC_Tumbler
{
public:
	SpectrogramFragmentSizeTumbler(Spectrogram *plugin, int x, int y);
	int handle_up_event();
	int handle_down_event();
	Spectrogram *plugin;
};

class SpectrogramNormalize : public BC_CheckBox
{
public:
	SpectrogramNormalize(Spectrogram *plugin, int x, int y);
	int handle_event();
	Spectrogram *plugin;
};

class SpectrogramFreq : public BC_TextBox
{
public:
	SpectrogramFreq(Spectrogram *plugin, int x, int y);
	int handle_event();
	Spectrogram *plugin;
};

class SpectrogramXZoom : public BC_IPot
{
public:
	SpectrogramXZoom(Spectrogram *plugin, int x, int y);
	int handle_event();
	Spectrogram *plugin;
};

class SpectrogramCanvas : public BC_SubWindow
{
public:
	SpectrogramCanvas(Spectrogram *plugin, int x, int y, int w, int h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	void calculate_point();
	void draw_overlay();

	enum
	{
		NONE,
		DRAG
	};

	int current_operation;
	Spectrogram *plugin;
};


class SpectrogramWindow : public PluginClientWindow
{
public:
	SpectrogramWindow(Spectrogram *plugin);
	~SpectrogramWindow();

	void create_objects();
	void update_gui();
	int resize_event(int w, int h);
	void calculate_frequency(int x, int y, int do_overlay);

	SpectrogramCanvas *canvas;

	BC_Title *level_title;
	SpectrogramLevel *level;
	BC_Title *window_size_title;
	SpectrogramWindowSize *window_size;
	SpectrogramWindowSizeTumbler *window_size_tumbler;

	BC_Title *mode_title;
	SpectrogramMode *mode;
	BC_Title *history_title;
	SpectrogramHistory *history;

	BC_Title *freq_title;
//	SpectrogramFreq *freq;
	BC_Title *amplitude_title;


	BC_Title *xzoom_title;
	SpectrogramXZoom *xzoom;
//	SpectrogramFragmentSizeTumbler *window_fragment_tumbler;

	SpectrogramNormalize *normalize;
	Spectrogram *plugin;
	int probe_x, probe_y;
};






class SpectrogramConfig
{
public:
	SpectrogramConfig();
	int equivalent(SpectrogramConfig &that);
	void copy_from(SpectrogramConfig &that);
	void interpolate(SpectrogramConfig &prev,
		SpectrogramConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
	double level;
	int window_size;
// Generate this many columns for each window
	int xzoom;
// Frequency probed
	int frequency;
	int normalize;
	int mode;
	int history_size;
};

// Header for data buffer
typedef struct
{
	int window_size;
// Total windows in this buffer
	int total_windows;
// Samples per fragment
	int window_fragment;
// Samplerate
	int sample_rate;
// Linearized user level
	float level;
// Nothing goes after this
// 1st sample in each window is the max
	float samples[1];
} data_header_t;

class SpectrogramFrame
{
public:
	SpectrogramFrame(int data_size);
	~SpectrogramFrame();

	int data_size;
	float *data;
// Draw immediately
	int force;
};

class Spectrogram : public PluginAClient
{
public:
	Spectrogram(PluginServer *server);
	~Spectrogram();

	PLUGIN_CLASS_MEMBERS2(SpectrogramConfig)
	int is_realtime();
	int process_buffer(int64_t size,
		Samples *buffer,
		int64_t start_position,
		int sample_rate);
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void update_gui();
	void render_gui(void *data, int size);
	void render_stop();

	void reset();

	int done;
	int need_reconfigure;
	FFT *fft;
// Data buffer for frequency & magnitude
	unsigned char *data;
// Accumulate data for windowing
	Samples *audio_buffer;
// Total samples in the buffer
	int buffer_size;
// Last window size rendered
	int window_size;
// Temporaries for the FFT
	double *freq_real;
	double *freq_imag;
// Total windows sent to current GUI
	int total_windows;
// Starting sample in audio_buffer.
	int64_t audio_buffer_start;
// Total floats allocated in data buffer
	int allocated_data;
// Accumulates canvas pixels until the next update_gui
	ArrayList<SpectrogramFrame*> frame_buffer;
// History for vertical mode
// Probing data for horizontal mode
	ArrayList<SpectrogramFrame*> frame_history;
// Header from last data buffer
	data_header_t header;
// Time of last GUI update
	Timer *timer;
// Window dimensions
	int w, h;
};


#endif
