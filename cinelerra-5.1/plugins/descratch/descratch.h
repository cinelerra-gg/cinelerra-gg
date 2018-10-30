#ifndef __DESCRATCH_H__
#define __DESCRATCH_H__

#include "bcbutton.h"
#include "bchash.h"
#include "bcmenuitem.h"
#include "bcpopupmenu.h"
#include "bcslider.h"
#include "bctitle.h"
#include "bctoggle.h"
#include "mutex.h"
#include "overlayframe.h"
#include "pluginvclient.h"

// so that write_pgm can create grey images of inf
#define SD_NULL 0
#define SD_EXTREM 0x80
#define SD_TESTED 0x40
#define SD_GOOD   0x20
#define SD_REJECT 0x10

#define MODE_NONE 0
#define MODE_LOW 1
#define MODE_HIGH 2
#define MODE_ALL 3

class DeScratchConfig;
class DeScratchMain;
class DeScratchWindow;
class DeScratchModeItem;
class DeScratchMode;
class DeScratchISlider;
class DeScratchFSlider;
class DeScratchMark;
class DeScratchEdgeOnly;
class DeScratchReset;


class DeScratchConfig
{
public:
	DeScratchConfig();
	~DeScratchConfig();
	void reset();
	int equivalent(DeScratchConfig &that);
	void copy_from(DeScratchConfig &that);
	void interpolate(DeScratchConfig &prev, DeScratchConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame);

	int threshold;
	float asymmetry;
	int min_width;
	int max_width;
	float min_len;
	float max_len;
	float max_angle;
	int blur_len;
	float gap_len;
	int mode_y;
	int mode_u;
	int mode_v;
	int mark;
	float ffade;
	int border;
	int edge_only;
};

class DeScratchMain : public PluginVClient
{
public:
	DeScratchMain(PluginServer *server);
	~DeScratchMain();

	PLUGIN_CLASS_MEMBERS(DeScratchConfig)
	uint8_t *inf;  int sz_inf;
	int src_w, src_h;
	VFrame *src, *dst;
	VFrame *tmpy, *blury;
	OverlayFrame *overlay_frame;
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	void set_extrems_plane(int width, int comp, int thresh);
	void close_gaps();
	void test_scratches();
	void mark_scratches_plane();
	void remove_scratches_plane(int comp);
	void blur(int scale);
	void copy(int comp);
	void pass(int comp, int thresh);
	void plane_pass(int comp, int mode);
	void plane_proc(int comp, int mode);
	int process_realtime(VFrame *input, VFrame *output);
};


class DeScratchWindow : public PluginClientWindow
{
public:
	DeScratchWindow(DeScratchMain *plugin);
	~DeScratchWindow();
	void update_gui();
	void create_objects();
	
	DeScratchMain *plugin;
	DeScratchMode *y_mode, *u_mode, *v_mode;
	DeScratchISlider *threshold;
	DeScratchFSlider *asymmetry;
	DeScratchISlider *min_width, *max_width;
	DeScratchFSlider *min_len, *max_len;
	DeScratchISlider *blur_len;
	DeScratchFSlider *gap_len;
	DeScratchFSlider *max_angle;
	DeScratchISlider *border;
	DeScratchMark *mark;
	DeScratchEdgeOnly *edge_only;
	DeScratchFSlider *ffade;
	DeScratchReset *reset;
};

class DeScratchModeItem : public BC_MenuItem
{
public:
	DeScratchModeItem(DeScratchMode *popup, int type, const char *text);
	~DeScratchModeItem();
	int handle_event();

	DeScratchMode *popup;
	int type;
};

class DeScratchMode : public BC_PopupMenu
{
public:
	DeScratchMode(DeScratchWindow *win, int x, int y, int *value);
	~DeScratchMode();
	void create_objects();
	int handle_event();
	void update(int v);
	void set_value(int v);

	DeScratchWindow *win;
	int *value;
};

class DeScratchISlider : public BC_ISlider
{
public:
	DeScratchISlider(DeScratchWindow *win,
		int x, int y, int w, int min, int max, int *output);
	~DeScratchISlider();
	int handle_event();

	DeScratchWindow *win;
	int *output;
};

class DeScratchFSlider : public BC_FSlider
{
public:
	DeScratchFSlider(DeScratchWindow *win,
		int x, int y, int w, float min, float max, float *output);
	~DeScratchFSlider();
	int handle_event();

	DeScratchWindow *win;
	float *output;
};

class DeScratchMark : public BC_CheckBox
{
public:
	DeScratchMark(DeScratchWindow *win, int x, int y);
	~DeScratchMark();
	int handle_event();

	DeScratchWindow *win;
};

class DeScratchEdgeOnly : public BC_CheckBox
{
public:
	DeScratchEdgeOnly(DeScratchWindow *win, int x, int y);
	~DeScratchEdgeOnly();
	int handle_event();

	DeScratchWindow *win;
};

class DeScratchReset : public BC_GenericButton
{
public:
        DeScratchReset(DeScratchWindow *win, int x, int y);
        int handle_event();

	DeScratchWindow *win;
};

#endif
