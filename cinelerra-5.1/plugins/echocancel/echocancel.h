
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

#ifndef ECHOCANCEL_H
#define ECHOCANCEL_H






#include "bchash.inc"
#include "bctimer.inc"
#include "fourier.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"




class EchoCancel;

#define MIN_XZOOM 1
#define MAX_XZOOM 6
#define MIN_PEAKS 1
#define MAX_PEAKS 8
#define MIN_DAMP 1
#define MAX_DAMP 8
#define MIN_CUTOFF 10
#define MAX_CUTOFF 1000
#define MIN_WINDOW 1024
#define MAX_WINDOW 262144
#define DIVISIONS 10
#define DIVISION_W 60
#define MARGIN 10
#define MAX_HISTORY 32
#define MIN_HISTORY 1
#define MIN_FREQ 1
#define MAX_FREQ 30000

#define CANCEL_OFF 0
#define CANCEL_ON  1
#define CANCEL_MAN 2

class EchoCancelLevel : public BC_FPot
{
public:
	EchoCancelLevel(EchoCancel *plugin, int x, int y);
	int handle_event();
	EchoCancel *plugin;
};

class EchoCancelMode : public BC_PopupMenu
{
public:
	EchoCancelMode(EchoCancel *plugin, int x, int y);
	int handle_event();
	static const char *to_text(int mode);
	static int to_mode(const char *text);
	void create_objects();
	EchoCancel *plugin;
};

class EchoCancelHistory : public BC_IPot
{
public:
	EchoCancelHistory(EchoCancel *plugin, int x, int y);
	int handle_event();
	EchoCancel *plugin;
};

class EchoCancelWindowSize : public BC_PopupMenu
{
public:
	EchoCancelWindowSize(EchoCancel *plugin, int x, int y, const char *text);
	int handle_event();
	static const char *to_text(int size);
	static int to_size(const char *text);
	EchoCancel *plugin;
};

class EchoCancelWindowSizeTumbler : public BC_Tumbler
{
public:
	EchoCancelWindowSizeTumbler(EchoCancel *plugin, int x, int y);
	int handle_up_event();
	int handle_down_event();
	EchoCancel *plugin;
};

class EchoCancelNormalize : public BC_CheckBox
{
public:
	EchoCancelNormalize(EchoCancel *plugin, int x, int y);
	int handle_event();
	EchoCancel *plugin;
};

class EchoCancelXZoom : public BC_IPot
{
public:
	EchoCancelXZoom(EchoCancel *plugin, int x, int y);
	int handle_event();
	EchoCancel *plugin;
};

class EchoCancelPeaks : public BC_IPot
{
public:
	EchoCancelPeaks(EchoCancel *plugin, int x, int y);
	int handle_event();
	EchoCancel *plugin;
};

class EchoCancelDamp : public BC_IPot
{
public:
	EchoCancelDamp(EchoCancel *plugin, int x, int y);
	int handle_event();
	EchoCancel *plugin;
};

class EchoCancelCutoff : public BC_IPot
{
public:
	EchoCancelCutoff(EchoCancel *plugin, int x, int y);
	int handle_event();
	EchoCancel *plugin;
};

class EchoCancelCanvas : public BC_SubWindow
{
public:
	EchoCancelCanvas(EchoCancel *plugin, int x, int y, int w, int h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	void calculate_point(int do_overlay);
	void draw_frame(float *data, int len, int hh);
	void draw_overlay();
	enum { NONE, DRAG };
	int current_operation;
	bool is_dragging() { return current_operation == DRAG; }
	EchoCancel *plugin;
};


class EchoCancelPrefix {
public:
	const char *prefix;
	EchoCancelPrefix(const char *p) : prefix(p) {}
	~EchoCancelPrefix() {}
};

class EchoCancelTitle : public EchoCancelPrefix, public BC_Title {
	char string[BCSTRLEN];
	char *preset(int value) {
		sprintf(string, "%s%d", prefix, value);
		return string;
	}
	char *preset(double value) {
		sprintf(string, "%s%.3f", prefix, value);
		return string;
	}
public:
	void update(int value) { BC_Title::update(preset(value)); }
	void update(double value) { BC_Title::update(preset(value)); }

	EchoCancelTitle(int x, int y, const char *pfx, int value)
		: EchoCancelPrefix(pfx), BC_Title(x, y, preset(value)) {}
	EchoCancelTitle(int x, int y, const char *pfx, double value)
		:  EchoCancelPrefix(pfx),BC_Title(x, y, preset(value)) {}
	~EchoCancelTitle() {}
};



class EchoCancelWindow : public PluginClientWindow
{
public:
	EchoCancelWindow(EchoCancel *plugin);
	~EchoCancelWindow();

	void create_objects();
	void update_gui();
	int resize_event(int w, int h);
	void calculate_frequency(int x, int y, int do_overlay);

	EchoCancelCanvas *canvas;

	BC_Title *level_title;
	EchoCancelLevel *level;
	BC_Title *window_size_title;
	EchoCancelWindowSize *window_size;
	EchoCancelWindowSizeTumbler *window_size_tumbler;
	BC_Title *mode_title;
	EchoCancelMode *mode;
	BC_Title *history_title;
	EchoCancelHistory *history;

	EchoCancelTitle *gain_title;
	EchoCancelTitle *offset_title;
	BC_Title *freq_title;
	BC_Title *amplitude_title;

	BC_Title *xzoom_title;
	EchoCancelXZoom *xzoom;
	BC_Title *peaks_title;
	EchoCancelPeaks *peaks;
	BC_Title *damp_title;
	EchoCancelDamp *damp;
	BC_Title *cutoff_title;
	EchoCancelCutoff *cutoff;

	EchoCancelNormalize *normalize;
	EchoCancel *plugin;
	int probe_x, probe_y;
};





class EchoCancelConfig
{
public:
	EchoCancelConfig();
	int equivalent(EchoCancelConfig &that);
	void copy_from(EchoCancelConfig &that);
	void interpolate(EchoCancelConfig &prev,
		EchoCancelConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
	double level;
	int normalize;
	int xzoom;
	int peaks;
	int damp;
	int cutoff;
	double gain;
	int offset;
	int window_size;
	int mode;
	int history_size;
};

//unattributed data class
class DataHeader {
public:
	int window_size;
	int total_windows;     // Total windows in this buffer
	int interrupted;       // playback stopped, config changed
	int sample_rate;       // Samplerate
	float level;           // Linearized user level
	float samples[0];
	int size() { return sizeof(DataHeader)+ window_size/2 * total_windows; }
};

// data buffer
class DataBuffer {
	int allocated, data_len;
	DataHeader *data_header;
	float *sample_data;  // offset sample data
public:
	void set_buffer(int ofs, int len);
	float *get_buffer() { return sample_data; }
	DataHeader *new_data_header(int data_size) {
		int size = sizeof(DataHeader) + data_size*sizeof(float);
		return (DataHeader*)(new char[size]);
	}
	DataHeader *get_header() { return data_header; }

	DataBuffer(int len) : allocated(0),data_header(0) {
		//printf("DataBuffer new %p\n",this);
		set_buffer(0,len); }
	~DataBuffer() {
		//printf("DataBuffer delete %p\n",this);
		delete [] (char*) data_header; }
};

class AudioBuffer {
	int allocated, data_len;
	Samples *samples;
	double *sample_data;
public:
	double *get_buffer(int ofs=0) {
		return samples->get_data() + (ofs>=0 ? ofs : buffer_size()+ofs);
	}
	double *get_data(int ofs=0) { return sample_data + ofs; }
	void set_buffer(int ofs, int len);
	int buffer_size() { return get_data(data_len)-get_buffer(); }
	int size() { return data_len; }
	void append(double *bfr, int len);
	void remove(int len);

	AudioBuffer(int len) : allocated(0),samples(0) { set_buffer(0,len); }
	~AudioBuffer() { delete samples; }

};

class EchoCancelFrame {
	float *data;
	int len;
public:
	int cut_offset;

	EchoCancelFrame(int n);
	~EchoCancelFrame();
	float scale() { return data[0]; }
	float *samples() { return &data[1]; }
	int size() { return len; }
	void rescale(float s) {
		data[0] = s;
		for( int i=1; i<=len; ++i ) data[i] *= s;
	}
	void todb() {
		for( int i=1; i<=len; ++i ) data[i] = DB::todb(data[i]);
	}
};

class EchoCancel : public PluginAClient
{
public:
	EchoCancel(PluginServer *server);
	~EchoCancel();

	PLUGIN_CLASS_MEMBERS2(EchoCancelConfig)
	int is_realtime();
	int process_buffer(int64_t size, Samples *buffer,
		int64_t start_position, int sample_rate);
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void update_gui();
	void render_gui(void *data, int size);
	void render_stop();

	void reset();
	void cepstrum(double *audio);
	void calculate_envelope(int sample_rate, int peaks, int bsz);
	void create_envelope(int sample_rate);
	int cancel_echo(double *bp, int size);
	void delete_buffers();
	void alloc_buffers(int fft_sz);

	FFT *fft;
// Data buffer for frequency & magnitude
	DataBuffer *data;
// time weighted data
	float *time_frame;
	int time_frames;
// envelope
	double *envelope, *env_data;
	double *env_real, *env_imag;
// correlation
	double *cor, *aud_real, *aud_imag;
// Accumulate data for windowing
	AudioBuffer *audio_buffer;
// Header from last data buffer
	DataHeader header;
// Last window size rendered
	int window_size, half_window;
	int interrupted;
// Accumulates canvas pixels until the next update_gui
	ArrayList<EchoCancelFrame*> frame_buffer;
// Probing data for horizontal mode
	ArrayList<EchoCancelFrame*> frame_history;
// Time of last GUI update
	Timer *timer;
// Window dimensions
	int w, h;
// Help for the wicked
	static void dfile_dump(const char *fn, double *dp, int n);
	static void ffile_dump(const char *fn, float *dp, int n);
};


#endif
