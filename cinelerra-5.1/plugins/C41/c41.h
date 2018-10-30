#ifndef __C41_H__
#define __C41_H__

#include "guicast.h"
#include "pluginvclient.h"
#include "vframe.inc"

// C41_FAST_POW increases processing speed more than 10 times
// Depending on gcc version, used optimizations and cpu C41_FAST_POW may not work
// With gcc versiion >= 4.4 it seems safe to enable C41_FAST_POW
// Test some samples after you have enabled it
#define C41_FAST_POW

#ifdef C41_FAST_POW
#define POWF myPow
#else
#define POWF powf
#endif

// Shave the image in order to avoid black borders
// The min max pixel value difference must be at least 0.05
#define C41_SHAVE_TOLERANCE 0.05
#define C41_SHAVE_MARGIN 0.1

#include <stdint.h>
#include <string.h>

class C41Effect;
class C41Window;
class C41Config;
class C41Enable;
class C41TextBox;
class C41Button;
class C41BoxButton;
class C41Slider;

struct magic
{
	float min_r, max_r;
	float min_g, max_g;
	float min_b, max_b;
	float light;
	float gamma_g, gamma_b;
	float coef1, coef2;
	int shave_min_row, shave_max_row;
	int shave_min_col, shave_max_col;
};

class C41Config
{
public:
	C41Config();

	void copy_from(C41Config &src);
	int equivalent(C41Config &src);
	void interpolate(C41Config &prev, C41Config &next,
		long prev_frame, long next_frame, long current_frame);

	int active, compute_magic;
	int postproc, show_box;
	float fix_min_r, fix_min_g, fix_min_b;
	float fix_light, fix_gamma_g, fix_gamma_b;
	float fix_coef1, fix_coef2;
	int min_col, max_col, min_row, max_row;
	int window_w, window_h;
};

class C41Enable : public BC_CheckBox
{
public:
	C41Enable(C41Effect *plugin, int *output, int x, int y, const char *text);
	int handle_event();

	C41Effect *plugin;
	int *output;
};

class C41TextBox : public BC_TextBox
{
public:
	C41TextBox(C41Effect *plugin, float *value, int x, int y);
	int handle_event();

	C41Effect *plugin;
	float *boxValue;
};

class C41Button : public BC_GenericButton
{
public:
	C41Button(C41Effect *plugin, C41Window *window, int x, int y);
	int handle_event();

	C41Effect *plugin;
	C41Window *window;
};

class C41BoxButton : public BC_GenericButton
{
public:
	C41BoxButton(C41Effect *plugin, C41Window *window, int x, int y);
	int handle_event();

	C41Effect *plugin;
	C41Window *window;
};

class C41Slider : public BC_ISlider
{
public:
	C41Slider(C41Effect *plugin, int *output, int x, int y, int is_row);

	int handle_event();
	int update(int v);

	C41Effect *plugin;
	int is_row, max;
	int *output;
};

class C41Window : public PluginClientWindow
{
public:
	C41Window(C41Effect *client);

	void update();
	void update_magic();

	C41Enable *active;
	C41Enable *compute_magic;
	C41Enable *postproc;
	C41Enable *show_box;
	BC_Title *min_r;
	BC_Title *min_g;
	BC_Title *min_b;
	BC_Title *light;
	BC_Title *gamma_g;
	BC_Title *gamma_b;
	BC_Title *coef1;
	BC_Title *coef2;
	BC_Title *box_col_min;
	BC_Title *box_col_max;
	BC_Title *box_row_min;
	BC_Title *box_row_max;
	C41TextBox *fix_min_r;
	C41TextBox *fix_min_g;
	C41TextBox *fix_min_b;
	C41TextBox *fix_light;
	C41TextBox *fix_gamma_g;
	C41TextBox *fix_gamma_b;
	C41TextBox *fix_coef1;
	C41TextBox *fix_coef2;
	C41Button *lock;
	C41BoxButton *boxlock;
	C41Slider *min_row;
	C41Slider *max_row;
	C41Slider *min_col;
	C41Slider *max_col;

	int slider_max_col;
	int slider_max_row;
};

class C41Effect : public PluginVClient
{
public:
	C41Effect(PluginServer *server);
	~C41Effect();

	PLUGIN_CLASS_MEMBERS(C41Config);
	int is_realtime();
	int is_synthesis();

	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void render_gui(void* data);
#if defined(C41_FAST_POW)
	float myLog2(float i) __attribute__ ((optimize(0)));
	float myPow2(float i) __attribute__ ((optimize(0)));
	float myPow(float a, float b);
#endif
	void pix_fix(float &pix, float min, float gamma);
	VFrame *frame, *tmp_frame, *blurry_frame;
	struct magic values;

	int shave_min_row, shave_max_row, shave_min_col, shave_max_col;
	int min_col, max_col, min_row, max_row;
	float pix_max;
	int pix_len;
};


#endif
