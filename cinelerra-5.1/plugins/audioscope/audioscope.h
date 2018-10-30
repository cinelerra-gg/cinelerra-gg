
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
#include "bccolors.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"




class AudioScope;

#define MIN_WINDOW 32
#define MAX_WINDOW 262144
#define DIVISIONS 10
#define DIVISION_W 60
#define MARGIN 10
#define MAX_COLUMNS 1024
#define CHANNELS 2
#define MIN_HISTORY 1
#define MAX_HISTORY 32

// Modes
#define XY_MODE 0
#define WAVEFORM_NO_TRIGGER 1
#define WAVEFORM_RISING_TRIGGER 2
#define WAVEFORM_FALLING_TRIGGER 3


#define CHANNEL0_COLOR WHITE
#define CHANNEL1_COLOR PINK

class AudioScopeHistory : public BC_IPot
{
public:
	AudioScopeHistory(AudioScope *plugin,
		int x,
		int y);
	int handle_event();
	AudioScope *plugin;
};

class AudioScopeWindowSize : public BC_PopupMenu
{
public:
	AudioScopeWindowSize(AudioScope *plugin,
		int x,
		int y,
		char *text);
	int handle_event();
	AudioScope *plugin;
};

class AudioScopeWindowSizeTumbler : public BC_Tumbler
{
public:
	AudioScopeWindowSizeTumbler(AudioScope *plugin, int x, int y);
	int handle_up_event();
	int handle_down_event();
	AudioScope *plugin;
};

class AudioScopeFragmentSize : public BC_PopupMenu
{
public:
	AudioScopeFragmentSize(AudioScope *plugin,
		int x,
		int y,
		char *text);
	int handle_event();
	AudioScope *plugin;
};

class AudioScopeFragmentSizeTumbler : public BC_Tumbler
{
public:
	AudioScopeFragmentSizeTumbler(AudioScope *plugin, int x, int y);
	int handle_up_event();
	int handle_down_event();
	AudioScope *plugin;
};


class AudioScopeMode : public BC_PopupMenu
{
public:
	AudioScopeMode(AudioScope *plugin,
		int x,
		int y);
	int handle_event();
	static const char* mode_to_text(int mode);
	static int text_to_mode(const char *text);
	void create_objects();
	AudioScope *plugin;
};

class AudioScopeTriggerLevel : public BC_FPot
{
public:
	AudioScopeTriggerLevel(AudioScope *plugin, int x, int y);
	int handle_event();
	AudioScope *plugin;
};



class AudioScopeCanvas : public BC_SubWindow
{
public:
	AudioScopeCanvas(AudioScope *plugin,
		int x,
		int y,
		int w,
		int h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	void calculate_point();

	enum
	{
		NONE,
		DRAG
	};

	int current_operation;
	AudioScope *plugin;
};


class AudioScopeWindow : public PluginClientWindow
{
public:
	AudioScopeWindow(AudioScope *plugin);
	~AudioScopeWindow();

	void create_objects();
	void update_gui();
	int resize_event(int w, int h);
	void calculate_probe(int x, int y, int do_overlay);
	void draw_overlay();
	void draw_trigger();
	void draw_probe();

	BC_Title *window_size_title;
	AudioScopeWindowSize *window_size;
	AudioScopeWindowSizeTumbler *window_size_tumbler;
	AudioScope *plugin;

	BC_Title *history_size_title;
	AudioScopeHistory *history_size;

	AudioScopeCanvas *canvas;

	BC_Title *mode_title;
	AudioScopeMode *mode;

	BC_Title *trigger_level_title;
	AudioScopeTriggerLevel *trigger_level;

	BC_Title *probe_sample;
	BC_Title *probe_level0;
	BC_Title *probe_level1;

	int probe_x;
	int probe_y;
};






class AudioScopeConfig
{
public:
	AudioScopeConfig();
	int equivalent(AudioScopeConfig &that);
	void copy_from(AudioScopeConfig &that);
	void interpolate(AudioScopeConfig &prev,
		AudioScopeConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
	int window_size;
	int history_size;

	int mode;
	double trigger_level;
};

// Header for data buffer
typedef struct
{
	int window_size;
// Total fragments in this buffer
	int total_windows;
// Samplerate
	int sample_rate;
	int channels;
// Nothing goes after this
	float samples[1];
} data_header_t;

class AudioScopeFrame
{
public:
	AudioScopeFrame(int data_size, int channels);
	~AudioScopeFrame();

	int size;
	int channels;
	float *data[CHANNELS];
// Draw immediately
	int force;
};

class AudioScope : public PluginAClient
{
public:
	AudioScope(PluginServer *server);
	~AudioScope();

	PLUGIN_CLASS_MEMBERS2(AudioScopeConfig)
	int is_realtime();
	int is_multichannel();
	int process_buffer(int64_t size,
		Samples **buffer,
		int64_t start_position,
		int sample_rate);
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void update_gui();
	void render_gui(void *data, int size);
	void render_stop();

	void reset();

	int need_reconfigure;
// Data buffer for frequency & magnitude
	unsigned char *data;
// Accumulate data for windowing
	Samples *audio_buffer[CHANNELS];
// Total samples in the buffer
	int buffer_size;
// Last window size rendered
	int window_size;
// Total windows sent to current GUI
	int total_windows;
// Starting sample in audio_buffer.
	int64_t audio_buffer_start;
// Total floats allocated in data buffer
	int allocated_data;
// Accumulates canvas pixels until the next update_gui
	ArrayList<AudioScopeFrame*> frame_buffer;
// Past frames for the shadow effect
	ArrayList<AudioScopeFrame*> frame_history;
// Data for probe
	AudioScopeFrame *current_frame;
// Header from last data buffer
	data_header_t header;
// Time of last GUI update
	Timer *timer;

	int w;
	int h;
};


#endif
