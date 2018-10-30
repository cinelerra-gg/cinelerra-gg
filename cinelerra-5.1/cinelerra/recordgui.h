
/*
 * CINELERRA
 * Copyright (C) 2008-2017 Adam Williams <broadcast at earthling dot net>
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

#ifndef RECORDGUI_H
#define RECORDGUI_H


class RecordGUIBatches;
class RecordBatch;
class RecordStartType;
class RecordStart;
class RecordPath;
class BrowseButton;
class RecordDuration;
class RecordSource;
class RecordNews;
class RecordGUISave;
class RecordGUIStartOver;
class RecordGUICancel;
class RecordGUIMonitorVideo;
class RecordGUIMonitorAudio;
class RecordGUIAudioMeters;
class RecordGUINewBatch;
class RecordGUIDeleteBatch;
class RecordGUIStartBatches;
class RecordGUIStopBatches;
class RecordGUIActivateBatch;
class RecordStatusThread;
class RecordGUIFlash;

class RecordGUIDCOffset;
class RecordGUIDropFrames;
class RecordGUIFillFrames;
class RecordGUIPowerOff;
class RecordGUICommCheck;
class RecordGUILabel;
class RecordGUIClearLabels;
class RecordGUILoop;
class RecordGUILoopHr;
class RecordGUILoopMin;
class RecordGUILoopSec;
class RecordGUIModeMenu;
class RecordGUIMode;
class RecordGUIOK;
class RecordGUIReset;
class RecordStartoverThread;
class EndRecordThread;

#include "browsebutton.inc"
#include "condition.inc"
#include "guicast.h"
#include "loadmode.inc"
#include "maxchannels.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "question.inc"
#include "recordgui.inc"
#include "record.inc"
#include "recordbatches.h"
#include "recordmonitor.inc"
#include "recordtransport.inc"
#include "timeentry.h"

class RecordGUIBatches : public RecordBatchesGUI
{
public:
	RecordGUI *gui;

	int handle_event();
	int selection_changed();

	RecordGUIBatches(RecordGUI *gui, int x, int y, int w, int h);
};

class RecordGUI : public BC_Window
{
	static int max(int a,int b) { return a>b ? a : b; }
public:
	RecordGUI(MWindow *mwindow, Record *record);
	~RecordGUI();

	void create_objects();
	int resize_event(int w, int h);
	int translation_event();

	BC_Title *current_operation;
	BC_Title *position_title;
	BC_Title *prev_label_title;
	BC_Title *frames_behind;
	BC_Title *frames_dropped;
	BC_Title *framerate;
	BC_Title *samples_clipped;
	BC_Title *cron_status;
	MWindow *mwindow;
	Record *record;
	RecordGUIBatches *batch_bay;
	RecordPath *batch_path;
	RecordStatusThread *status_thread;
	TimeEntry *batch_start;
	TimeEntry *batch_duration;
	RecordStartType *start_type;
	RecordTransport *record_transport;
	BrowseButton *batch_browse;
	RecordSource *batch_source;
	RecordGUIModeMenu *batch_mode;
	RecordGUINewBatch *new_batch;
	RecordGUIDeleteBatch *delete_batch;
	RecordGUIStartBatches *start_batches;
	RecordGUIStopBatches *stop_batches;
	RecordGUIActivateBatch *activate_batch;
	RecordGUILabel *label_button;
	RecordGUIClearLabels *clrlbls_button;
	RecordGUIDropFrames *drop_frames;
	RecordGUIFillFrames *fill_frames;
	RecordGUIPowerOff *power_off;
	RecordGUICommCheck *commercial_check;
	RecordGUIMonitorVideo *monitor_video;
	RecordGUIMonitorAudio *monitor_audio;
	RecordGUIAudioMeters *meter_audio;
	RecordGUIFlash *batch_flash;
	RecordStartoverThread *startover_thread;
	EndRecordThread *interrupt_thread;
	LoadMode *load_mode;
	int flash_color;

	RecordGUILoopHr *loop_hr;
	RecordGUILoopMin *loop_min;
	RecordGUILoopSec *loop_sec;
	RecordGUIReset *reset;
	RecordGUIDCOffset *dc_offset_button;
	RecordGUIDCOffsetText *dc_offset_text[MAXCHANNELS];
	RecordMonitor *monitor_video_window;
	BC_Meter *meter[MAXCHANNELS];
	int total_dropped_frames;
	int total_clipped_samples;

	int set_loop_status(int value);
	int keypress_event();
	int set_translation(int x, int y, float z);

	void update_batches();
	Batch *get_current_batch();
	Batch *get_editing_batch();
	void start_flash_batch();
	void stop_flash_batch();
	void flash_batch();
// Update the batch channel table when edited
	void update_batch_sources();
// Update the batch editing tools
	void update_batch_tools();
	void enable_batch_buttons();
	void disable_batch_buttons();

	void reset_audio();
	void reset_video();
	void update_frames_behind(long value);
	void update_dropped_frames(long value);
	void update_position(double value);
	void update_clipped_samples(long value);
	void update_framerate(double value);
	void update_video(int dropped, int behind);
	void update_audio(int channels, double *max, int *over);
	void update_cron_status(const char *status);
	void update_power_off(int value);
	void update_labels(double new_position);
	int update_prev_label(long new_position);
	int update_title(BC_Title *title, double position);
};

class RecordGUISave : public BC_Button
{
public:
	RecordGUISave(RecordGUI *gui);
	int handle_event();
	int keypress_event();
	RecordGUI *gui;
};


class RecordGUICancel : public BC_CancelButton
{
public:
	RecordGUICancel(RecordGUI *record_gui);
	int handle_event();
	int keypress_event();
	RecordGUI *gui;
	Record *record;
};

class RecordGUIOK : public BC_OKButton
{
public:
	RecordGUIOK(RecordGUI *record_gui);
	int handle_event();
	RecordGUI *gui;
};

class RecordGUIStartBatches : public RecordBatchesGUI::StartBatches
{
public:
	RecordGUI *gui;
	int handle_event();
	RecordGUIStartBatches(RecordGUI *gui, int x, int y);
};


class RecordGUIStopBatches : public RecordBatchesGUI::StopBatches
{
public:
	RecordGUI *gui;
	int handle_event();
	RecordGUIStopBatches(RecordGUI *gui, int x, int y);
};

class RecordGUIActivateBatch : public RecordBatchesGUI::ActivateBatch
{
public:
	RecordGUIActivateBatch(RecordGUI *gui, int x, int y);
	int handle_event();
	RecordGUI *gui;
};


class RecordGUIStartOver : public BC_GenericButton
{
public:
	RecordGUIStartOver(RecordGUI *gui, int x, int y);
	~RecordGUIStartOver();

	int handle_event();

	RecordGUI *gui;
};

class EndRecordThread : public Thread
{
public:
	EndRecordThread(RecordGUI *record_gui);
	~EndRecordThread();

	void start(int is_ok);
	void run();

	RecordGUI *gui;
	QuestionWindow *window;
// OK Button was pressed
	int is_ok;
};

class RecordStartoverThread : public Thread
{
public:
	RecordStartoverThread(RecordGUI *record_gui);
	~RecordStartoverThread();
	void run();

	RecordGUI *gui;
	QuestionWindow *window;
};

class RecordBatch : public BC_PopupTextBox
{
public:
	RecordBatch(RecordGUI *gui, int x, int y);
	int handle_event();
	RecordGUI *gui;
};

class RecordPath : public RecordBatchesGUI::Path
{
public:
	RecordGUI *gui;
	int handle_event();
	RecordPath(RecordGUI *gui, int x, int y);
};

class RecordStart : public RecordBatchesGUI::StartTime
{
public:
	RecordGUI *gui;
	int handle_event();
	RecordStart(RecordGUI *gui, int x, int y);
};

class RecordDuration : public RecordBatchesGUI::Duration
{
public:
	RecordGUI *gui;
	int handle_event();
	RecordDuration(RecordGUI *gui, int x, int y);
};

class RecordSource : public RecordBatchesGUI::Source
{
public:
	RecordGUI *gui;
	int handle_event();
	RecordSource(RecordGUI *gui, int x, int y);
};

class RecordNews : public RecordBatchesGUI::News
{
public:
	RecordNews(RecordGUI *gui, int x, int y);
	int handle_event();
	RecordGUI *gui;
};

class RecordGUIDropFrames : public BC_CheckBox
{
public:
	RecordGUIDropFrames(RecordGUI *gui, int x, int y);
	int handle_event();
	int keypress_event();
	RecordGUI *gui;
};

class RecordGUIFillFrames : public BC_CheckBox
{
public:
	RecordGUIFillFrames(RecordGUI *gui, int x, int y);
	int handle_event();
	int keypress_event();
	RecordGUI *gui;
};

class RecordGUIPowerOff : public BC_CheckBox
{
public:
	RecordGUIPowerOff(RecordGUI *gui, int x, int y);
	int handle_event();
	int keypress_event();
	RecordGUI *gui;
};

class RecordGUICommCheck : public BC_CheckBox
{
public:
	RecordGUICommCheck(RecordGUI *gui, int x, int y);
	int handle_event();
	int keypress_event();
	RecordGUI *gui;
};

class RecordGUIMonitorVideo : public BC_CheckBox
{
public:
	RecordGUIMonitorVideo(RecordGUI *gui, int x, int y);
	int handle_event();
	int keypress_event();
	RecordGUI *gui;
};

class RecordGUIMonitorAudio : public BC_CheckBox
{
public:
	RecordGUIMonitorAudio(RecordGUI *gui, int x, int y);
	int handle_event();
	int keypress_event();
	RecordGUI *gui;
};

class RecordGUIAudioMeters : public BC_CheckBox
{
public:
	RecordGUIAudioMeters(RecordGUI *gui, int x, int y);
	int handle_event();
	int keypress_event();
	RecordGUI *gui;
};

class RecordGUINewBatch : public RecordBatchesGUI::NewBatch
{
public:
	RecordGUI *gui;
	int handle_event();
	RecordGUINewBatch(RecordGUI *gui, int x, int y);
};


class RecordGUIDeleteBatch : public RecordBatchesGUI::DeleteBatch
{
public:
	RecordGUI *gui;
	int handle_event();
	RecordGUIDeleteBatch(RecordGUI *gui, int x, int y);
};

class RecordGUILabel : public BC_GenericButton
{
public:
	RecordGUILabel(RecordGUI *gui, int x, int y);
	~RecordGUILabel();

	int handle_event();
	int keypress_event();
	RecordGUI *gui;
};

class RecordGUIClearLabels : public BC_GenericButton
{
public:
	RecordGUIClearLabels(RecordGUI *gui, int x, int y);
	~RecordGUIClearLabels();

	int handle_event();
	RecordGUI *gui;
};

// Stop GUI blocking the recording when X11 is busy
class RecordStatusThread : public Thread
{
public:
	RecordStatusThread(MWindow *mwindow, RecordGUI *gui);
	~RecordStatusThread();

	void reset_video();
	void reset_audio();
	void update_frames_behind(long value);
	void update_dropped_frames(long value);
	void update_position(double value);
	void update_clipped_samples(long value);
	void update_framerate(double value);
	void get_window_lock();
	template<class T>
	void update_status_string(const char *fmt,T &new_value,T &displayed_value,BC_Title *widgit);
	void run();

	MWindow *mwindow;
	RecordGUI *gui;
	long new_dropped_frames, displayed_dropped_frames;
	long new_frames_behind, displayed_frames_behind;
	double new_framerate, displayed_framerate;
	long new_clipped_samples, displayed_clipped_samples;
	double new_position;
	int window_locked;
	int done;
	Condition *input_lock;
};

class RecordGUIFlash : public Thread
{
public:
	RecordGUI *record_gui;
	Condition *flash_lock;
	int done;

	void run();
	RecordGUIFlash(RecordGUI *gui);
	~RecordGUIFlash();
};

class RecordGUIModeMenu;

class RecordGUIModeTextBox : public BC_TextBox
{
public:
	RecordGUIModeTextBox(RecordGUIModeMenu *mode_menu,
		int x, int y, int w,const char *text);
	~RecordGUIModeTextBox();
	RecordGUI *record_gui;
	RecordGUIModeMenu *mode_menu;
	int handle_event();
};

class RecordGUIModeListBox : public BC_ListBox
{
public:
	RecordGUIModeListBox(RecordGUIModeMenu *mode_menu);
	~RecordGUIModeListBox();
	RecordGUI *record_gui;
	RecordGUIModeMenu *mode_menu;
	int handle_event();
};

class RecordGUIModeMenu
{
public:
	RecordGUIModeMenu(RecordGUI *record_gui,int x, int y, int w, const char *text);
	~RecordGUIModeMenu();

	int handle_event();
	void create_objects();
	void update(int value);
	int get_w();
	int get_h();

	RecordGUI *record_gui;
	RecordGUIModeTextBox *textbox;
	RecordGUIModeListBox *listbox;
	ArrayList<BC_ListBoxItem*> modes;
	int value;
};


class RecordGUIDCOffset : public BC_Button
{
public:
	RecordGUIDCOffset(MWindow *mwindow, int y);
	~RecordGUIDCOffset();

	int handle_event();
	int keypress_event();
};

class RecordGUIDCOffsetText : public BC_TextBox
{
public:
	RecordGUIDCOffsetText(char *text, int y, int number);
	~RecordGUIDCOffsetText();

	int handle_event();
	int number;
};

class RecordGUIReset : public BC_Button
{
public:
	RecordGUIReset(MWindow *mwindow, RecordGUI *gui, int y);
	~RecordGUIReset();

	int handle_event();
	RecordGUI *gui;
};

class RecordGUIResetTranslation : public BC_Button
{
public:
	RecordGUIResetTranslation(MWindow *mwindow, RecordGUI *gui, int y);
	~RecordGUIResetTranslation();

	int handle_event();
	RecordGUI *gui;
};







#endif
