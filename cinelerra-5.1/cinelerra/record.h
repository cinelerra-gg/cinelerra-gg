
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

#ifndef RECORD_H
#define RECORD_H

#include "asset.inc"
#include "assets.inc"
#include "audiodevice.h"
#include "batch.inc"
#include "bchash.inc"
#include "bitspopup.h"
#include "browsebutton.h"
#include "channel.inc"
#include "channeldb.inc"
#include "commercials.inc"
#include "devicedvbinput.inc"
#include "drivesync.inc"
#include "edl.inc"
#include "file.inc"
#include "filexml.inc"
#include "filethread.inc"
#include "formatpopup.h"
#include "formattools.inc"
#include "guicast.h"
#include "loadmode.inc"
#include "mediadb.inc"
#include "mwindow.inc"
#include "maxchannels.h"
#include "picture.inc"
#include "playbackengine.inc"
#include "record.inc"
#include "recordbatches.h"
#include "recordgui.inc"
#include "recordaudio.inc"
#include "recordmonitor.inc"
#include "recordthread.inc"
#include "recordvideo.inc"
#include "remotecontrol.h"
#include "videodevice.inc"

#define SESSION (mwindow->edl->session)

class Record;

class RecordMenuItem : public BC_MenuItem
{
public:
	RecordMenuItem(MWindow *mwindow);
	~RecordMenuItem();

	int handle_event();

	Record *record;
	MWindow *mwindow;
};

class RecordChannel : public Thread
{
public:
	int done;
	Record *record;
	Channel *new_channel;
	int audio_drain;
	Condition *channel_lock;
	Condition *change_channel;
	void run();
	int set(Channel *channel);
	void drain_audio();

	RecordChannel(Record *record);
	~RecordChannel();
};


class Record : public Thread
{
public:
	Record(MWindow *mwindow, RecordMenuItem *menu_item);
	~Record();

	void run();
	void stop(int wait=1);
	int load_defaults();
	int save_defaults();
	Batch* new_batch();
	int current_batch();
	int set_current_batch(int i);
	int editing_batch();
	int set_editing_batch(int i);
	void delete_index_file(Asset *asset);
	void delete_batch();
	void activate_batch(int number);
	void change_editing_batch(int number);
	void close_output_file();
	int get_fragment_samples();
	int get_buffer_samples();

	void close_video_input();
	void close_audio_input();
	void close_input_devices();
	void stop_audio_thread();
	void stop_video_thread();
	void stop_input_threads();
	void stop_playback();
	void open_audio_input();
	void open_video_input();
	void start();
	void start_audio_thread();
	void start_video_thread();
	void start_input_threads();
	void pause_input_threads();
	void resume_input_threads();
	void adevice_drain();
	int start_toc();
	int start_record(int fd);
	void start_writing_file();
	void flush_buffer();
	int stop_record();
	void stop_writing_file();
	void stop_writing();
	void start_cron_thread();
	void stop_cron_thread(const char *msg);
	void set_power_off(int value);

	void set_audio_monitoring(int mode);
	void set_audio_metering(int mode);
	void set_mute_gain(double gain);
	void set_play_gain(double gain);
	void set_video_monitoring(int mode);
	void stop_operation();
	void set_video_picture();
	void set_do_cursor();
// Set screencapture translation
	void set_translation(int x, int y);

// Set the channel in the current batch and the picture controls
	int set_channel_no(int chan_no);
	int channel_down();
	int channel_up();
	int set_channel_name(const char *name);
	int set_channel(Channel *channel);
	void set_batch_channel_no(int chan_no);
	void set_dev_channel(Channel *channel);
	int has_signal();
	int create_channeldb(ArrayList<Channel*> *channeldb);
// User defined TV stations and inputs to record from.
	ChannelDB *channeldb;
// Structure which stores what parameters the device supports
	Channel *master_channel;
	RecordChannel *record_channel;
	Channel *channel;
	Channel *current_channel;

	void toggle_label();
	void clear_labels();
// Set values in batch structures
	void configure_batches();
// Create first file in batch
	int open_output_file();
// Delete the output file for overwrite if it exists.
	int delete_output_file();
// Get the inputs supported by the device
	ArrayList<Channel*>* get_video_inputs();
	int cron_active();

// Copied to each batch for the files
	Asset *default_asset;
	int load_mode;
	int monitor_audio;
	int metering_audio;
	int monitor_video;
	int video_window_open;
// Compression is fixed by the driver
	int fixed_compression;
// Get next batch using activation or -1
	int get_next_batch(int incr=1);
// Information about the current batch.
	Batch* get_current_batch();
// Information about the batch being edited
	Batch* get_editing_batch();
	const char* current_mode();
	const char* current_source();
	Channel *get_current_channel();
	Channel *get_editing_channel();
	Asset* current_asset();
// Total number of samples since record sequence started
	int64_t timer_position();
	void reset_position(int64_t position);
	void update_position();
	int64_t adevice_position();
	int64_t sync_position();
	void resync();
// Current position for GUI relative to batch
	double current_display_position();
	int check_batch_complete();
// Rewind the current file in the current batch
	void start_over();

// skimming for commericials
	static int skimming(void *vp, int track);
	int skimming(int track);
	void start_skimming();
	void stop_skimming();
	void update_skimming(int v);
	int start_commercial_capture();
	int mark_commercial_capture(int action);
	int stop_commercial_capture(int run_job);
	void set_status_color(int color);
	void remote_fill_color(int color);
	int commercial_jobs();
	void clear_keybfr();
	void add_key(int ch);
	int remote_process_key(RemoteControl *remote_control, int key);
	int spawn(const char *fmt, ...);
	void display_video_text(int x, int y, const char *text, int font,
		int bg_color, int color, int alpha, double secs, double scale);
	void display_cut_icon(int x, int y);
	void display_vframe(VFrame *in, int x, int y, int alpha,
		double secs, double scale);
	int display_channel_info();
	int display_channel_schedule();
	void undisplay_vframe();
	DeviceDVBInput *dvb_device();

	Condition *init_lock;
	RecordAudio *record_audio;
	RecordVideo *record_video;
	RecordThread *record_thread;
	RecordBatches record_batches;
	int capturing;
	int recording;
	int single_frame;
	int writing_file;  // -1/writing stream, 0/not writing, 1/transcoding
	int do_audio;      // output audio if writing_file
	int do_video;      // output video if writing_file
	int64_t current_frame, written_frames, total_frames;
	int64_t current_sample, written_samples, total_samples;
	double audio_time, video_time;
	double play_gain, mute_gain;

	Mutex *pause_lock;
	DriveSync *drivesync;

	LoadMode *loadmode;
	MWindow *mwindow;
	RecordGUI *record_gui;
	RecordMonitor *record_monitor;
	AudioDevice *adevice;
	VideoDevice *vdevice;
	Mutex *adevice_lock;
	Mutex *vdevice_lock;
	Mutex *batch_lock;
// File handle of last asset.in current batch
	File *file;

// Table for LML conversion
//	unsigned char _601_to_rgb_table[256];
// For video synchronization when no audio thread
	Timer timer, total_time;
	int64_t session_sample_offset;
	int64_t device_sample_offset;
// Translation of screencapture input
	int video_x;
	int video_y;
	float video_zoom;
// Reverse the interlace in the video window display only
	int reverse_interlace;
// record the cursor for screencapture
	int do_cursor;
	int do_big_cursor;
// Color model for uncompressed device interface
	int color_model;
// Picture quality and parameters the device supports
	PictureConfig *picture;
// Drop input frames when behind
	int drop_overrun_frames;
// Fill frames with duplicates when behind
	int fill_underrun_frames;
// power off system when batch record ends
	int power_off;
// check for commercials
	int commercial_check, skimming_active;
	int commercial_fd;
	int64_t commercial_start_time;
	SkimDbThread *skim_thread;
// cut commerical, update mediadb
	Deletions *deletions;
	RecordCutAdsStatus *cutads_status;
	RecordBlinkStatus *blink_status;
	ArrayList<int> cut_pids;
	int status_color;
	int last_key;
	char keybfr[8];

// Parameters for video monitor
	EDL *edl;
	Mutex *window_lock;
	Mutex *timer_lock;
	Mutex *file_lock;
	RecordMenuItem *menu_item;

	int get_time_format();
	int set_record_mode(int value);
	int is_behind() { return drop_overrun_frames && behind > 1 ? 1 : 0; }

	int64_t dc_offset[MAXCHANNELS];
	int frame_w;
	int frame_h;
	int video_window_w;       // Width of record video window
	int dropped, behind;
	int input_threads_pausing;
};

class RecordScheduleItem {
public:
	time_t start_time;
	char *title;

	RecordScheduleItem(time_t st, char *t)
	 : start_time(st), title(strdup(t)) {}
	RecordScheduleItem() { free(title); }
};

class RecordSchedule : public ArrayList<RecordScheduleItem *> {
public:
	static int cmpr(const void *a, const void*b) {
		const RecordScheduleItem *ap = *(const RecordScheduleItem **)a;
		const RecordScheduleItem *bp = *(const RecordScheduleItem **)b;
		return ap->start_time < bp->start_time ? -1 : 1;
	}
	void sort_times() { qsort(values,size(),sizeof(values[0]),&cmpr); }
	RecordSchedule() {}
	~RecordSchedule() { remove_all_objects(); }
};

class RecordRemoteHandler : public RemoteHandler
{
public:
	int remote_process_key(RemoteControl *remote_control, int key);
	int spawn(const char *fmt, ...);

	RecordRemoteHandler(RemoteControl *remote_control);
	~RecordRemoteHandler();
};

class RecordCutAdsStatus : public Thread
{
public:
	Record *record;
	Condition *wait_lock;

	int done;
	void start_waiting();
	void run();

	RecordCutAdsStatus(Record *record);
	~RecordCutAdsStatus();
};

class RecordBlinkStatus : public Thread
{
	int done;
	Timer timer;
public:
	Record *record;
	void update();
	void remote_color(int color);
	void start();
	void stop();
	void run();

	RecordBlinkStatus(Record *record);
	~RecordBlinkStatus();
};

#endif
