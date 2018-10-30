
/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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

#ifndef SYNTHESIZER_H
#define SYNTHESIZER_H



#include "filexml.inc"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"


class Synth;
class SynthWindow;

// Frequency table for piano
float keyboard_freqs[] =
{
	65.4064,
	69.2957,
	73.4162,
	77.7817,
	82.4069,
	87.3071,
	92.4986,
	97.9989,
	103.826,
	110.000,
	116.541,
	123.471,

	130.81,
	138.59,
	146.83,
	155.56,
	164.81,
	174.61,
	185.00,
	196.00,
	207.65,
	220.00,
	233.08,
	246.94,

	261.63,
	277.18,
	293.66,
	311.13,
	329.63,
	349.23,
	369.99,
	392.00,
	415.30,
	440.00,
	466.16,
	493.88,



	523.251,
	554.365,
	587.330,
	622.254,
	659.255,
	698.456,
	739.989,
	783.991,
	830.609,
	880.000,
	932.328,
	987.767,

	1046.50,
	1108.73,
	1174.66,
	1244.51,
	1318.51,
	1396.91,
	1479.98,
	1567.98,
	1661.22,
	1760.00,
	1864.66,
	1975.53,

	2093.00
};

#define MAX_FREQS 16
#define TOTALOSCILLATORS 1
#define OSCILLATORHEIGHT 40
#define TOTALNOTES ((int)(sizeof(keyboard_freqs) / sizeof(float)))
#define MIDDLE_C 24
#define FIRST_TITLE (MIDDLE_C - 12)
#define LAST_TITLE (MIDDLE_C + 12)
#define MARGIN 10

#define SINE 0
#define SAWTOOTH 1
#define SQUARE 2
#define TRIANGLE 3
#define PULSE 4
#define NOISE 5
#define DC    6


class SynthCanvas;
class SynthWaveForm;
class SynthBaseFreq;
class SynthFreqPot;
class SynthOscGUI;
class OscScroll;
class NoteScroll;
class SynthWetness;
class SynthNote;
class SynthMomentary;

class SynthWindow : public PluginClientWindow
{
public:
	SynthWindow(Synth *synth);
	~SynthWindow();

	void create_objects();
	int resize_event(int w, int h);
	void update_gui();
	int waveform_to_text(char *text, int waveform);
	void update_scrollbar();
	void update_oscillators();
	void update_notes();
	void update_note_selection();
	int keypress_event();
	void update_blackkey(int number, int *current_title, int x, int y);
	void update_whitekey(int number, int *current_title, int x, int y);


	Synth *synth;
	SynthCanvas *canvas;
	SynthWetness *wetness;
	SynthWaveForm *waveform;
	SynthBaseFreq *base_freq;
	SynthFreqPot *freqpot;
	BC_SubWindow *osc_subwindow;
	OscScroll *osc_scroll;
	BC_SubWindow *note_subwindow;
	NoteScroll *note_scroll;
	ArrayList<SynthOscGUI*> oscillators;
	SynthNote *notes[TOTALNOTES];
	BC_Title *note_titles[TOTALNOTES];
	BC_Title *note_instructions;
	SynthMomentary *momentary;
	VFrame *white_key[5];
	VFrame *black_key[5];
	int y1;
	int y2;
	int y3;
	int text_white_margin;
	int text_black_margin;
// Button press currently happening if > -1
	int current_note;
// If we are stopping or starting notes in a drag
	int starting_notes;
};

class SynthMomentary : public BC_CheckBox
{
public:
	SynthMomentary(SynthWindow *window, int x, int y, char *text);
	int handle_event();
	SynthWindow *window;
};

class SynthNote : public BC_Toggle
{
public:
	SynthNote(SynthWindow *window, VFrame **images, int number, int x, int y);
	void start_note();
	void stop_note();
	int keypress_event();
	int keyrelease_event();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int draw_face(int flash, int flush);
	int number;
	int note_on;
	SynthWindow *window;
};


class SynthOscGUILevel;
class SynthOscGUIPhase;
class SynthOscGUIFreq;

class SynthOscGUI
{
public:
	SynthOscGUI(SynthWindow *window, int number);
	~SynthOscGUI();

	void create_objects(int view_y);

	SynthOscGUILevel *level;
	SynthOscGUIPhase *phase;
	SynthOscGUIFreq *freq;
	BC_Title *title;

	int number;
	SynthWindow *window;
};

class SynthOscGUILevel : public BC_FPot
{
public:
	SynthOscGUILevel(Synth *synth, SynthOscGUI *gui, int y);
	~SynthOscGUILevel();

	int handle_event();

	Synth *synth;
	SynthOscGUI *gui;
};

class SynthOscGUIPhase : public BC_IPot
{
public:
	SynthOscGUIPhase(Synth *synth, SynthOscGUI *gui, int y);
	~SynthOscGUIPhase();

	int handle_event();

	Synth *synth;
	SynthOscGUI *gui;
};

class SynthOscGUIFreq : public BC_IPot
{
public:
	SynthOscGUIFreq(Synth *synth, SynthOscGUI *gui, int y);
	~SynthOscGUIFreq();

	int handle_event();

	Synth *synth;
	SynthOscGUI *gui;
};

class OscScroll : public BC_ScrollBar
{
public:
	OscScroll(Synth *synth, SynthWindow *window, int x, int y, int h);
	~OscScroll();

	int handle_event();

	Synth *synth;
	SynthWindow *window;
};

class NoteScroll : public BC_ScrollBar
{
public:
	NoteScroll(Synth *synth, SynthWindow *window, int x, int y, int w);
	~NoteScroll();

	int handle_event();

	Synth *synth;
	SynthWindow *window;
};

class SynthAddOsc : public BC_GenericButton
{
public:
	SynthAddOsc(Synth *synth, SynthWindow *window, int x, int y);
	~SynthAddOsc();

	int handle_event();

	Synth *synth;
	SynthWindow *window;
};


class SynthDelOsc : public BC_GenericButton
{
public:
	SynthDelOsc(Synth *synth, SynthWindow *window, int x, int y);
	~SynthDelOsc();

	int handle_event();

	Synth *synth;
	SynthWindow *window;
};

class SynthClear : public BC_GenericButton
{
public:
	SynthClear(Synth *synth, int x, int y);
	~SynthClear();
	int handle_event();
	Synth *synth;
};

class SynthWaveForm : public BC_PopupMenu
{
public:
	SynthWaveForm(Synth *synth, int x, int y, char *text);
	~SynthWaveForm();

	void create_objects();
	Synth *synth;
};

class SynthWaveFormItem : public BC_MenuItem
{
public:
	SynthWaveFormItem(Synth *synth, char *text, int value);
	~SynthWaveFormItem();

	int handle_event();

	int value;
	Synth *synth;
};

class SynthBaseFreq : public BC_TextBox
{
public:
	SynthBaseFreq(Synth *synth, SynthWindow *window, int x, int y);
	~SynthBaseFreq();
	int handle_event();
	Synth *synth;
	SynthFreqPot *freq_pot;
	SynthWindow *window;
};

class SynthFreqPot : public BC_QPot
{
public:
	SynthFreqPot(Synth *synth, SynthWindow *window, int x, int y);
	~SynthFreqPot();
	int handle_event();
	SynthWindow *window;
	Synth *synth;
	SynthBaseFreq *freq_text;
};

class SynthWetness : public BC_FPot
{
public:
	SynthWetness(Synth *synth, int x, int y);
	int handle_event();
	Synth *synth;
};


class SynthCanvas : public BC_SubWindow
{
public:
	SynthCanvas(Synth *synth,
		SynthWindow *window,
		int x,
		int y,
		int w,
		int h);
	~SynthCanvas();

	int update();
	Synth *synth;
	SynthWindow *window;
};




// ======================= level calculations
class SynthLevelZero : public BC_MenuItem
{
public:
	SynthLevelZero(Synth *synth);
	~SynthLevelZero();
	int handle_event();
	Synth *synth;
};

class SynthLevelMax : public BC_MenuItem
{
public:
	SynthLevelMax(Synth *synth);
	~SynthLevelMax();
	int handle_event();
	Synth *synth;
};

class SynthLevelNormalize : public BC_MenuItem
{
public:
	SynthLevelNormalize(Synth *synth);
	~SynthLevelNormalize();
	int handle_event();
	Synth *synth;
};

class SynthLevelSlope : public BC_MenuItem
{
public:
	SynthLevelSlope(Synth *synth);
	~SynthLevelSlope();
	int handle_event();
	Synth *synth;
};

class SynthLevelRandom : public BC_MenuItem
{
public:
	SynthLevelRandom(Synth *synth);
	~SynthLevelRandom();
	int handle_event();
	Synth *synth;
};

class SynthLevelInvert : public BC_MenuItem
{
public:
	SynthLevelInvert(Synth *synth);
	~SynthLevelInvert();
	int handle_event();
	Synth *synth;
};

class SynthLevelSine : public BC_MenuItem
{
public:
	SynthLevelSine(Synth *synth);
	~SynthLevelSine();
	int handle_event();
	Synth *synth;
};

// ============================ phase calculations

class SynthPhaseInvert : public BC_MenuItem
{
public:
	SynthPhaseInvert(Synth *synth);
	~SynthPhaseInvert();
	int handle_event();
	Synth *synth;
};

class SynthPhaseZero : public BC_MenuItem
{
public:
	SynthPhaseZero(Synth *synth);
	~SynthPhaseZero();
	int handle_event();
	Synth *synth;
};

class SynthPhaseSine : public BC_MenuItem
{
public:
	SynthPhaseSine(Synth *synth);
	~SynthPhaseSine();
	int handle_event();
	Synth *synth;
};

class SynthPhaseRandom : public BC_MenuItem
{
public:
	SynthPhaseRandom(Synth *synth);
	~SynthPhaseRandom();
	int handle_event();
	Synth *synth;
};


// ============================ freq calculations

class SynthFreqRandom : public BC_MenuItem
{
public:
	SynthFreqRandom(Synth *synth);
	~SynthFreqRandom();
	int handle_event();
	Synth *synth;
};

class SynthFreqEnum : public BC_MenuItem
{
public:
	SynthFreqEnum(Synth *synth);
	~SynthFreqEnum();
	int handle_event();
	Synth *synth;
};

class SynthFreqEven : public BC_MenuItem
{
public:
	SynthFreqEven(Synth *synth);
	~SynthFreqEven();
	int handle_event();
	Synth *synth;
};

class SynthFreqOdd : public BC_MenuItem
{
public:
	SynthFreqOdd(Synth *synth);
	~SynthFreqOdd();
	int handle_event();
	Synth *synth;
};

class SynthFreqFibonacci : public BC_MenuItem
{
public:
	SynthFreqFibonacci(Synth *synth);
	~SynthFreqFibonacci();
	int handle_event();
	Synth *synth;
};

class SynthFreqPrime : public BC_MenuItem
{
public:
	SynthFreqPrime(Synth *synth);
	~SynthFreqPrime();
	int handle_event();
	Synth *synth;
private:
	float get_next_prime(float number);
};



class SynthOscillatorConfig
{
public:
	SynthOscillatorConfig(int number);
	~SynthOscillatorConfig();

	int equivalent(SynthOscillatorConfig &that);
	void copy_from(SynthOscillatorConfig& that);
	void reset();
	void read_data(FileXML *file);
	void save_data(FileXML *file);
	int is_realtime();

	float level;
	float phase;
	float freq_factor;
	int number;
};



class SynthConfig
{
public:
	SynthConfig();
	~SynthConfig();

	int equivalent(SynthConfig &that);
	void copy_from(SynthConfig &that);
	void interpolate(SynthConfig &prev,
		SynthConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
	void reset();

	double wetness;
// base frequency for oscillators
// Freqs of 0 are unused.
	double base_freq[MAX_FREQS];
	int wavefunction;        // SINE, SAWTOOTH, etc
	ArrayList<SynthOscillatorConfig*> oscillator_config;
	int momentary_notes;
};


class Synth : public PluginAClient
{
public:
	Synth(PluginServer *server);
	~Synth();


	PLUGIN_CLASS_MEMBERS(SynthConfig)
	int is_realtime();
	int is_synthesis();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_realtime(int64_t size, Samples *input_ptr, Samples *output_ptr);




// Frequency is in the table of base_freqs
	int freq_exists(double freq);
// Manage frequency table
	void new_freq(double freq);
	void delete_freq(double freq);
	void delete_freqs();

	void add_oscillator();
	void delete_oscillator();
	double get_total_power();
	double get_oscillator_point(float x,
		double normalize_constant,
		int oscillator);
	double solve_eqn(double *output,
		int length,
		double freq,
		double normalize_constant,
		int oscillator);
	double get_point(float x, double normalize_constant);
	double function_square(double x);
	double function_pulse(double x);
	double function_noise();
	double function_sawtooth(double x);
	double function_triangle(double x);
	void reconfigure();
	int overlay_synth(double freq,
		int64_t length,
		double *input,
		double *output);
	void update_gui();
	void reset();



	int window_w, window_h;
	int need_reconfigure;
	DB db;
// Samples since last reconfiguration
	int64_t waveform_sample;
};








#endif
