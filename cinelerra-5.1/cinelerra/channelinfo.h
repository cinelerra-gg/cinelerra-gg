#ifndef CHANNELINFO_H
#define CHANNELINFO_H

#ifdef HAVE_DVB

#include "channel.inc"
#include "channeldb.h"
#include "channelinfo.inc"
#include "devicedvbinput.h"
#include "guicast.h"
#include "mwindow.inc"
#include "record.inc"
#include "recordbatches.h"
#include "signalstatus.h"
#include "videodevice.inc"
#include "vdevicempeg.inc"

#define LTBLACK 0x001c1c1c


class ChanSearch : public Thread
{
public:
	ChannelInfo *iwindow;
	ChanSearchGUI *gui;
	Mutex *window_lock;

	void start();
	void stop();
	void run();

	ChanSearch(ChannelInfo *iwindow);
	~ChanSearch();
};

class ChanSearchGUI : public BC_Window
{
public:
	ChanSearch *cswindow;
	ChannelInfo *iwindow;
	ChannelPanel *panel;
	ChanSearchText *search_text;
	ChanSearchTitleText *title_text;
	ChanSearchInfoText *info_text;
	ChanSearchMatchCase *match_case;
	ChanSearchStart *search_start;
	ChanSearchCancel *cancel;
	ChanSearchList *search_list;
	BC_Title *click_tip;
	BC_Title *results;

	int title_text_enable;
	int info_text_enable;
	int match_case_enable;
	ChannelEvent *highlighted_event;
	int search_x, search_y, text_x, text_y;
	int cancel_x, cancel_y, cancel_w, cancel_h;
	int list_x, list_y, list_w, list_h;
	int results_x, results_y;
	int sort_column, sort_order;

	const char *search_column_titles[3];
	int search_column_widths[3];
	int search_columns[3];
	ArrayList<BC_ListBoxItem*> search_items[3];
	ArrayList<ChannelEvent*> search_results;

	void create_objects();
	int close_event();
	int resize_event(int x, int y);
	void update();
	int search(const char *);
	void search();
	static int cmpr_text_dn(const void *a, const void *b);
	static int cmpr_Text_dn(const void *a, const void *b);
	static int cmpr_text_up(const void *a, const void *b);
	static int cmpr_Text_up(const void *a, const void *b);
	static int cmpr_time_dn(const void *a, const void *b);
	static int cmpr_time_up(const void *a, const void *b);
	static int cmpr_title_dn(const void *a, const void *b);
	static int cmpr_Title_dn(const void *a, const void *b);
	static int cmpr_title_up(const void *a, const void *b);
	static int cmpr_Title_up(const void *a, const void *b);
	void sort_events(int column, int order);
	void move_column(int src, int dst);

	ChanSearchGUI(ChanSearch *cswindow);
	~ChanSearchGUI();
};

class ChanSearchTitleText : public BC_CheckBox
{
public:
	ChanSearchGUI *gui;

	int handle_event();
	void update(int v) { set_value(gui->title_text_enable = v); }

	ChanSearchTitleText(ChanSearchGUI *gui, int x, int y);
	~ChanSearchTitleText();
};

class ChanSearchInfoText : public BC_CheckBox
{
public:
	ChanSearchGUI *gui;

	int handle_event();
	void update(int v) { set_value(gui->info_text_enable = v); }

	ChanSearchInfoText(ChanSearchGUI *gui, int x, int y);
	~ChanSearchInfoText();
};

class ChanSearchMatchCase : public BC_CheckBox
{
public:
	ChanSearchGUI *gui;

	int handle_event();

	ChanSearchMatchCase(ChanSearchGUI *gui, int x, int y);
	~ChanSearchMatchCase();
};

class ChanSearchText : public BC_TextBox
{
public:
	ChanSearchGUI *gui;

	int handle_event();
	int keypress_event();

	ChanSearchText(ChanSearchGUI *gui, int x, int y, int w);
	~ChanSearchText();
};

class ChanSearchStart : public BC_GenericButton
{
public:
	ChanSearchGUI *gui;

	int handle_event();

	ChanSearchStart(ChanSearchGUI *gui, int x, int y);
	~ChanSearchStart();
};

class ChanSearchCancel : public BC_CancelButton
{
public:
	ChanSearchGUI *gui;

	int handle_event();

	ChanSearchCancel(ChanSearchGUI *gui, int x, int y);
	~ChanSearchCancel();
};

class ChanSearchList : public BC_ListBox
{
public:
	ChanSearchGUI *gui;

	int handle_event();
	int sort_order_event();
	int move_column_event();

	ChanSearchList(ChanSearchGUI *gui, int x, int y, int w, int h);
	~ChanSearchList();
};


class ChannelProgress : public Thread, public BC_SubWindow
{
	ChannelInfoGUI *gui;
	BC_Title *eta;
	BC_ProgressBar *bar;
	Timer *eta_timer;
	double length, value;
	int done;
public:
	void create_objects();
	void run();
	void start();
	void stop();
	int update();
	void set_value(int v) { value = v; }

	ChannelProgress(ChannelInfoGUI *gui, int x, int y, int w, int h, int len);
	~ChannelProgress();
};



class ChannelPanel : public BC_SubWindow
{
public:
	ChannelInfoGUI *gui;
	TimeLine *time_line;
	ChannelData *channel_data;
	ChannelFrame *channel_frame;
	ChannelScroll *channel_scroll;
	TimeLineScroll *time_line_scroll;
	ArrayList<TimeLineItem*> time_line_items;
	ArrayList<ChannelDataItem*> channel_data_items;
	ArrayList<ChannelEventLine *> channel_line_items;
	ArrayList<ChannelEvent*> channel_event_items;

	int x0, y0, x1, y1, t0, t1;
	int iwd, iht;
	int x_moved, y_moved;
	int iwindow_w, iwindow_h;
	int path_w, path_h, hhr_w, x_now;
	int frame_x, frame_y;
	int frame_w, frame_h;
	int x_scroll, y_scroll;
	struct timeval tv;
	struct timezone tz;
	time_t st_org;

	void create_objects();
	ChannelEventLine *NewChannelLine(int y, int h, int color);
	ChannelEventLine *NewChannelLine(int y) {
		return NewChannelLine(y, path_h, LTBLACK);
	}
	int separator(int y) {
		NewChannelLine(y+path_h/8, path_h/4, BLACK);
		return path_h/2;
	}
	void resize(int w, int h);
	void bounding_box(int ix0, int iy0, int ix1, int iy1);
	void set_x_scroll(int v);
	void set_y_scroll(int v);
	void reposition();
	void get_xtime(int x, char *text);
	void time_line_update(int ix0, int ix1);
	int button_press_event();

	ChannelPanel(ChannelInfoGUI *gui, int x, int y, int w, int h);
	~ChannelPanel();
};

class TimeLineItem : public BC_Title
{
public:
	ChannelPanel *panel;
	int x0, y0;

	TimeLineItem(ChannelPanel *panel, int x, int y, char *text);
	~TimeLineItem();
};

class TimeLine : public BC_SubWindow
{
public:
	ChannelPanel *panel;
	int resize_event(int w, int h);

	TimeLine(ChannelPanel *panel);
	~TimeLine();
};

class ChannelDataItem : public BC_Title
{
public:
	ChannelPanel *panel;
	int x0, y0;
	const char *tip_info;

	int repeat_event(int64_t duration);
	void set_tooltip(const char *tip);

	ChannelDataItem(ChannelPanel *panel, int x, int y, int w,
		int color, const char *text);
	~ChannelDataItem();
};

class ChannelData : public BC_SubWindow
{
public:
	ChannelPanel *panel;

	int resize_event(int w, int h);

	ChannelData(ChannelPanel *panel, int x, int y, int w, int h);
	~ChannelData();
};

class ChannelScroll : public BC_ScrollBar
{
public:
	ChannelPanel *panel;

	int handle_event();

	ChannelScroll(ChannelPanel *panel, int x, int y, int h);
	~ChannelScroll();
};

class TimeLineScroll : public BC_ScrollBar
{
public:
	ChannelPanel *panel;

	int handle_event();

	TimeLineScroll(ChannelPanel *panel, int x, int y, int w);
	~TimeLineScroll();
};



class ChannelEvent : public BC_GenericButton
{
public:
	ChannelEventLine *channel_line;
	time_t start_time, end_time;
	Channel *channel;
	int x0, y0, no;
	const char *tip_info;

	int handle_event();
	void set_tooltip(const char *tip);

	ChannelEvent(ChannelEventLine *channel_line, Channel *channel,
		time_t start_time, time_t end_time, int x, int y, int w,
		const char *text);
	~ChannelEvent();
};

class ChannelEventLine : public BC_SubWindow
{
public:
	ChannelPanel *panel;
	int x0, y0;

	void resize(int w, int h);

	ChannelEventLine(ChannelPanel *panel, int x, int y, int w, int h, int color);
	~ChannelEventLine();
};

class ChannelFrame : public BC_SubWindow
{
public:
	ChannelPanel *panel;

	void resize(int w, int h);

	ChannelFrame(ChannelPanel *panel);
	~ChannelFrame();
};



class ChannelInfoGUI : public BC_Window
{
public:
	ChannelInfo *iwindow;
	ChanSearch *channel_search;
	ChannelPanel *panel;
	ChannelProgress *progress;
	ChannelStatus *channel_status;
	ChannelInfoGUIBatches *batch_bay;
	ChannelDir *channel_dir;
	ChannelPath *channel_path;
	ChannelStart *channel_start;
	ChannelDuration *channel_duration;
	ChannelSource *channel_source;
	ChannelClearBatch *channel_clear_batch;
	ChannelNewBatch *channel_new_batch;
	ChannelDeleteBatch *channel_delete_batch;
	TimeEntryTumbler *early_time, *late_time;
	BC_Title *directory_title;
	BC_Title *path_title;
	BC_Title *start_title;
	BC_Title *duration_title;
	BC_Title *source_title;
	BC_OKButton *ok;
	BC_CancelButton *cancel;
	ChannelInfoCron *channel_cron;
	ChannelInfoPowerOff *channel_poweroff;
	ChannelInfoFind *channel_find;
	int x0, y0, title_w, data_w, pad;
	int path_w, path_h, status_w;
	int panel_w, panel_h, max_bay_w;
	int bay_x, bay_y, bay_w, bay_h;
	const char *cron_caption, *power_caption;
	int cron_x, cron_y, cron_w, cron_h;
	int power_x, power_y, power_w, power_h;
	int find_x, find_y, find_h;
	int ok_x, ok_y, ok_w, ok_h;
	int cancel_x, cancel_y, cancel_w, cancel_h;

	void create_objects();
	void stop(int v);
	int translation_event();
	int resize_event(int w, int h);
	int close_event();
	void update_channel_tools();
	void incr_event(int start_time_incr, int duration_incr);
	void update_progress(int n) { progress->set_value(n); }

	ChannelInfoGUI(ChannelInfo *iwindow, int x, int y, int w, int h);
	~ChannelInfoGUI();
};

class ChannelInfoOK : public BC_OKButton
{
public:
	ChannelInfoGUI *gui;

	int button_press_event();
	int keypress_event();

	ChannelInfoOK(ChannelInfoGUI *gui, int x, int y);
	~ChannelInfoOK();
};

class ChannelInfoCancel : public BC_CancelButton
{
public:
	ChannelInfoGUI *gui;

	int button_press_event();

	ChannelInfoCancel(ChannelInfoGUI *gui, int x, int y);
	~ChannelInfoCancel();
};

class ChannelInfoCron : public BC_CheckBox
{
public:
	ChannelInfoGUI *gui;

	int handle_event();
	ChannelInfoCron(ChannelInfoGUI *gui, int x, int y, int *value);
	~ChannelInfoCron();
};

class ChannelInfoPowerOff : public BC_CheckBox
{
public:
	ChannelInfoGUI *gui;

	int handle_event();

	ChannelInfoPowerOff(ChannelInfoGUI *gui, int x, int y, int *value);
	~ChannelInfoPowerOff();
};

class ChannelInfoFind : public BC_GenericButton
{
public:
	ChannelInfoGUI *gui;

	int handle_event();
	ChannelInfoFind(ChannelInfoGUI *gui, int x, int y);
	~ChannelInfoFind();
};


class ChannelInfoGUIBatches : public RecordBatchesGUI
{
public:
	ChannelInfoGUI *gui;

	int handle_event();
	int selection_changed();

	ChannelInfoGUIBatches(ChannelInfoGUI *gui,
		int x, int y, int w, int h);
	~ChannelInfoGUIBatches();
};

class ChannelDir : public RecordBatchesGUI::Dir
{
public:
	ChannelInfoGUI *gui;

	ChannelDir(ChannelInfoGUI *gui, const char *dir, int x, int y);
};

class ChannelPath : public RecordBatchesGUI::Path
{
public:
	ChannelInfoGUI *gui;

	ChannelPath(ChannelInfoGUI *gui, int x, int y);
};

class ChannelStart : public RecordBatchesGUI::StartTime
{
public:
	ChannelInfoGUI *gui;

	ChannelStart(ChannelInfoGUI *gui, int x, int y);
};

class ChannelDuration : public RecordBatchesGUI::Duration
{
public:
	ChannelInfoGUI *gui;

	ChannelDuration(ChannelInfoGUI *gui, int x, int y, int w);
};

class ChannelEarlyTime : public TimeEntryTumbler
{
public:
	ChannelInfoGUI *gui;

	int handle_up_event();
	int handle_down_event();

	ChannelEarlyTime(ChannelInfoGUI *gui, int x, int y,
		double *output_time);
};

class ChannelLateTime : public TimeEntryTumbler
{
public:
	ChannelInfoGUI *gui;

	int handle_up_event();
	int handle_down_event();

	ChannelLateTime(ChannelInfoGUI *gui, int x, int y,
		double *output_time);
};

class ChannelSource : public RecordBatchesGUI::Source
{
public:
	ChannelInfoGUI *gui;
	void create_objects();
	int handle_event();
	ChannelSource(ChannelInfoGUI *gui, int x, int y);
};


class ChannelNewBatch : public RecordBatchesGUI::NewBatch
{
public:
	ChannelInfoGUI *gui;
	int handle_event();
	ChannelNewBatch(ChannelInfoGUI *gui, int x, int y);
};


class ChannelDeleteBatch : public RecordBatchesGUI::DeleteBatch
{
public:
	ChannelInfoGUI *gui;
	int handle_event();
	ChannelDeleteBatch(ChannelInfoGUI *gui, int x, int y);
};

class ChannelClearBatch : public RecordBatchesGUI::ClearBatch
{
public:
	ChannelInfoGUI *gui;
	int handle_event();
	ChannelClearBatch(ChannelInfoGUI *gui, int x, int y);
};



class ChannelInfo : public Thread
{
public:
	MWindow *mwindow;
	Record *record;
	VideoDevice *vdevice;
	Mutex *window_lock;
	Mutex *vdevice_lock;
	Mutex *progress_lock;
	DeviceDVBInput *dvb_input;
	RecordBatches record_batches;
	ChannelInfoGUI *gui;
	ChannelThread *thread;
	ChannelDB *channeldb;
	Condition *scan_lock;
	int cron_enable, poweroff_enable;
	int item;
	int done, gui_done;

	void run_scan();
	void toggle_scan();
	void start();
	void stop();
	void run();
	void close_vdevice();
	Batch *new_batch();
	void delete_batch();
	int current_batch() { return gui->batch_bay->current_batch(); }
	int editing_batch() { return gui->batch_bay->editing_batch(); }
	bool is_active() { return gui != 0; }

	ChannelInfo(MWindow *mwindow);
	~ChannelInfo();
};

class ChannelThread : public Thread
{
public:
	ChannelInfo *iwindow;
	ChannelInfoGUI *gui;
	ChannelPanel *panel;
	zmpeg3_t *fd;
	int done;

	int load_ident(int n, int y, char *ident);
	int load_info(Channel *channel, ChannelEventLine *channel_line);
	void start();
	void stop();
	void run();
	int total_channels() { return iwindow->channeldb->size(); }
	Channel *get_channel(int ch) { return iwindow->channeldb->get(ch); }
	int set_channel(Channel *chan);

	ChannelThread(ChannelInfoGUI *gui);
	~ChannelThread();
};

class ChannelScan : public BC_MenuItem
{
public:
	MWindow *mwindow;
	int handle_event();

	ChannelScan(MWindow *mwindow);
	~ChannelScan();
};

class ChannelStatus : public SignalStatus
{
public:
	ChannelInfoGUI *gui;

	ChannelStatus(ChannelInfoGUI *gui, int x, int y) :
		SignalStatus(gui, x, y) { this->gui = gui; }
	~ChannelStatus() {}
};


#endif
#endif
