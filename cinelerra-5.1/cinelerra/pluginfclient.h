#ifndef __PLUGINFCLIENT_H__
#define __PLUGINFCLIENT_H__

#include "arraylist.h"
#include "bclistbox.h"
#include "bclistboxitem.h"
#include "bcbutton.h"
#include "bcpopupmenu.h"
#include "bcmenuitem.h"
#include "bctextbox.h"
#include "ffmpeg.h"
#include "pluginclient.h"
#include "pluginaclient.h"
#include "pluginvclient.h"
#include "pluginserver.h"
#include "pluginfclient.inc"
#include "preferences.inc"

extern "C" {
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

class PluginFClient_OptName : public BC_ListBoxItem {
public:
	PluginFClient_Opt *opt;

	PluginFClient_OptName(PluginFClient_Opt *opt);
};

class PluginFClient_OptValue : public BC_ListBoxItem {
public:
	PluginFClient_Opt *opt;

	void update();
	PluginFClient_OptValue(PluginFClient_Opt *opt);
};

class PluginFClient_Opt {
public:
	PluginFClientConfig *conf;
	const AVOption *opt;
	PluginFClient_OptName *item_name;
	PluginFClient_OptValue *item_value;

	char *get(char *vp, int sz=-1);
	void set(const char *val);
	int types(char *rp);
	int scalar(double d, char *rp);
	int ranges(char *rp);
	int units(ArrayList<const AVOption *> &opts);
	int units(char *rp);
	int get_range(float &fmn, float &fmx);
	float get_float();
	void set_float(float v);

	const char *tip();
	void *filter_config();
	const AVClass *filter_class();

	PluginFClient_Opt(PluginFClientConfig *conf, const AVOption *opt);
	~PluginFClient_Opt();
};

class PluginFFilter {
	PluginFFilter(PluginFFilter &that) {} //disable assign/copy
public:
	const AVFilter *filter;
	AVFilterGraph *graph;
	AVFilterContext *fctx;

	void *filter_config() { return fctx->priv; }
	const AVClass *filter_class() { return filter->priv_class; }
	const char *description() { return filter->description; }

	int init(const char *name, PluginFClientConfig *conf);
	void uninit();
	static PluginFFilter *new_ffilter(const char *name, PluginFClientConfig *conf=0);

	PluginClient* new_plugin(PluginServer*);
	const char *filter_name() { return filter->name; }
	bool is_audio();
	bool is_video();

	PluginFFilter();
	~PluginFFilter();
};

class PluginFClientConfig : public ArrayList<PluginFClient_Opt *>
{
public:
	PluginFFilter *ffilt;
	void *filter_config() { return ffilt->filter_config(); }
	const AVClass *filter_class() { return ffilt->filter_class(); }
	PluginFClient_Opt *get(int i) { return ArrayList<PluginFClient_Opt *>::get(i); }

	const char *get(const char *name);
	void set(const char *name, const char *val);
	void copy_from(PluginFClientConfig &that);
	int equivalent(PluginFClientConfig &that);
	void interpolate(PluginFClientConfig &prev, PluginFClientConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame);
	void initialize(const char *name);
	int update();
	void dump(FILE *fp=stdout);

	PluginFClientConfig();
	~PluginFClientConfig();
};


class PluginFClient_OptPanel : public BC_ListBox {
public:
	PluginFClient_OptPanel(PluginFClientWindow *fwin, int x, int y, int w, int h);
	~PluginFClient_OptPanel();
	void create_objects();
	int cursor_leave_event();

	PluginFClientWindow *fwin;
	ArrayList<BC_ListBoxItem*> items[2];
	ArrayList<BC_ListBoxItem*> &opts;
	ArrayList<BC_ListBoxItem*> &vals;

	int selection_changed();
	int update();
};

class PluginFClientReset : public BC_GenericButton {
public:
	PluginFClientWindow *fwin;

	PluginFClientReset(PluginFClientWindow *fwin, int x, int y);
	~PluginFClientReset();
	int handle_event();
};

class PluginFClientUnits : public BC_PopupMenu {
public:
	PluginFClientWindow *fwin;

	PluginFClientUnits(PluginFClientWindow *fwin, int x, int y, int w);
	~PluginFClientUnits();
	int handle_event();
};

class PluginFClientText : public BC_TextBox {
public:
	PluginFClientWindow *fwin;

	PluginFClientText(PluginFClientWindow *fwin, int x, int y, int w);
	~PluginFClientText();
	int handle_event();
};

class PluginFClientApply : public BC_GenericButton {
public:
	PluginFClientWindow *fwin;

	PluginFClientApply(PluginFClientWindow *fwin, int x, int y);
	~PluginFClientApply();
	int handle_event();
};

class PluginFClientPot : public BC_FPot
{
public:
	PluginFClientPot(PluginFClientWindow *fwin, int x, int y);
	int handle_event();
	PluginFClientWindow *fwin;
};

class PluginFClientSlider : public BC_FSlider
{
public:
	PluginFClientSlider(PluginFClientWindow *fwin, int x, int y);
	int handle_event();
	PluginFClientWindow *fwin;
};

class PluginFClientWindow : public PluginClientWindow
{
public:
	PluginFClientWindow(PluginFClient *ffmpeg);
	~PluginFClientWindow();

	void create_objects();
	void update(PluginFClient_Opt *oip);
	int update();
	void update_selected();
	void draw();
	int resize_event(int w, int h);

	PluginFClient *ffmpeg;
	PluginFClient_OptPanel *panel;
	int panel_x, panel_y, panel_w, panel_h;
	BC_Title *type, *range;
	PluginFClient_Opt *selected;

	PluginFClientReset *reset;
	PluginFClientUnits *units;
	PluginFClientText *text;
	PluginFClientApply *apply;
	PluginFClientPot *pot;
	PluginFClientSlider *slider;
};

class PluginFLogLevel {
	int level;
public:
	PluginFLogLevel(int lvl) {
		level = av_log_get_level();
		if( level > lvl ) av_log_set_level(lvl);
	}
	~PluginFLogLevel() { av_log_set_level(level); }
};

class PluginFClient {
public:
	const char *name;
	PluginClient *plugin;
	PluginFFilter *ffilt;
	AVFilterContext *fsrc, *fsink;
	int64_t plugin_position, filter_position;
	int activated;
	char title[BCSTRLEN];

	PluginFClient(PluginClient *plugin, const char *name);
	~PluginFClient();
	static bool is_audio(const AVFilter *fp);
	static bool is_video(const AVFilter *fp);

	int64_t get_source_position() {
		return plugin->get_source_position();
	}
	KeyFrame* get_prev_keyframe(int64_t position, int is_local=1) {
		return plugin->get_prev_keyframe(position, is_local);
	}
	KeyFrame* get_next_keyframe(int64_t position, int is_local=1) {
		return plugin->get_next_keyframe(position, is_local);
	}
	int64_t edl_to_local(int64_t position) {
		return plugin->edl_to_local(position);
	}

	void update_gui();
	char *to_upper(char *bp, const char *sp);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void render_gui(void *data, int size);
	int activate();
	void reactivate();

	PluginFClientConfig curr_config;
	PLUGIN_CLASS_MEMBERS(PluginFClientConfig)
};

class PluginFAClient : public PluginAClient, public PluginFClient
{
public:
	PluginFAClient(PluginServer *server, const char *name);
	~PluginFAClient();
	const char *plugin_title() { return PluginFClient::plugin_title(); }
	PluginClientWindow *new_window() { return PluginFClient::new_window(); }
	int activate();

	int load_configuration() { return PluginFClient::load_configuration(); }
	void save_data(KeyFrame *keyframe) { PluginFClient::save_data(keyframe); }
	void read_data(KeyFrame *keyframe) { PluginFClient::read_data(keyframe); }
	void update_gui() { PluginFClient::update_gui(); }
	void render_gui(void *data, int size) { PluginFClient::render_gui(data, size); }

	int is_realtime() { return 1; }
	int is_multichannel() { return 1; }
	int uses_gui() { return 1; }
	int is_synthesis() { return 1; }
	int get_inchannels();
	int get_outchannels();
	int process_buffer(int64_t size, Samples **buffer, int64_t start_position, int sample_rate);
};

class PluginFVClient : public PluginVClient, public PluginFClient, public FFVideoConvert
{
public:
	PluginFVClient(PluginServer *server, const char *name);
	~PluginFVClient();
	const char *plugin_title() { return PluginFClient::plugin_title(); }
	PluginClientWindow *new_window() { return PluginFClient::new_window(); }
	int activate(int width, int height, int color_model);

	int load_configuration() { return PluginFClient::load_configuration(); }
	void save_data(KeyFrame *keyframe) { PluginFClient::save_data(keyframe); }
	void read_data(KeyFrame *keyframe) { PluginFClient::read_data(keyframe); }
	void update_gui() { PluginFClient::update_gui(); }
	void render_gui(void *data, int size) { PluginFClient::render_gui(data, size); }

	int is_realtime() { return 1; }
	int is_multichannel() { return 1; }
	int uses_gui() { return 1; }
	int is_synthesis() { return 1; }
	int process_buffer(VFrame **frames, int64_t start_position, double frame_rate);
};

#endif
