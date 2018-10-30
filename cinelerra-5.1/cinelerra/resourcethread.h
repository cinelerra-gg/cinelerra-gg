
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef RESOURCETHREAD_H
#define RESOURCETHREAD_H

// This thread tries to draw picons into the timeline, asynchronous
// of the navigation.

// TrackCanvas draws the picons which are in the cache and makes a table of
// picons and locations which need to be decompressed.  Then ResourceThread
// decompresses the picons and draws them one by one, refreshing the
// entire trackcanvas in the process.


#include "arraylist.h"
#include "bctimer.inc"
#include "condition.inc"
#include "file.inc"
#include "indexable.inc"
#include "maxchannels.h"
#include "mwindow.inc"
#include "renderengine.inc"
#include "samples.inc"
#include "thread.h"
#include "vframe.inc"


class ResourceThreadItem
{
public:
	ResourceThreadItem(ResourcePixmap *pixmap,
		int pane_number,
		Indexable *indexable,
		int data_type,
		int operation_count);
	virtual ~ResourceThreadItem();

	ResourcePixmap *pixmap;
	Indexable *indexable;
	int data_type;
	int operation_count;
	int last;
	int pane_number;
};


class AResourceThreadItem : public ResourceThreadItem
{
public:
	AResourceThreadItem(ResourcePixmap *pixmap,
		int pane_number,
		Indexable *indexable,
		int x,
		int channel,
		int64_t start,
		int64_t end,
		int operation_count);
	~AResourceThreadItem();
	int x;
	int channel;
	int64_t start;
	int64_t end;
};

class VResourceThreadItem : public ResourceThreadItem
{
public:
	VResourceThreadItem(ResourcePixmap *pixmap,
		int pane_number,
		int picon_x,
		int picon_y,
		int picon_w,
		int picon_h,
		double frame_rate,
		int64_t position,
		int layer,
		Indexable *indexable,
		int operation_count);
	~VResourceThreadItem();



	int picon_x;
	int picon_y;
	int picon_w;
	int picon_h;
	double frame_rate;
	int64_t position;
	int layer;
};


class ResourceThread : public Thread
{
public:
	ResourceThread(MWindow *mwindow, MWindowGUI *gui);
	~ResourceThread();


	void create_objects();
// reset - delete all picons.  Used for index building.
	void stop_draw(int reset);
	void start_draw();

// Be sure to stop_draw before changing the asset table,
// closing files.
	void add_picon(ResourcePixmap *pixmap,
		int pane_number,
		int picon_x,
		int picon_y,
		int picon_w,
		int picon_h,
		double frame_rate,
		int64_t position,
		int layer,
		Indexable *indexable);

	void add_wave(ResourcePixmap *pixmap,
		int pane_number,
		Indexable *indexable,
		int x,
		int channel,
// samples relative to asset rate
		int64_t source_start,
		int64_t source_end);

	void run();
	void stop();

	void do_video(VResourceThreadItem *item);
	void do_audio(AResourceThreadItem *item);

	void open_render_engine(EDL *nested_edl,
		int do_audio,
		int do_video);

	File *get_video_source(Asset *asset);
	File *get_audio_source(Asset *asset);

	MWindow *mwindow;
	MWindowGUI *gui;
	Condition *draw_lock;
	Mutex *item_lock;
	ArrayList<ResourceThreadItem*> items;
	int interrupted;
	int done;
	VFrame *temp_picon;
	VFrame *temp_picon2;
// Render engine for nested EDL
	RenderEngine *render_engine;
// ID of nested EDL being rendered
	int render_engine_id;
	Asset *audio_asset;
	File *audio_source;
	Asset *video_asset;
	File *video_source;

// Current audio buffer for spanning multiple pixels
	Samples *audio_buffer;
// Temporary for nested EDL
	Samples *temp_buffer[MAX_CHANNELS];
	int audio_channel;
	int64_t audio_start;
	int audio_samples;
	int audio_asset_id;
// Timer for waveform refreshes
	Timer *timer;
// Waveform state
	int prev_x;
	double prev_h;
	double prev_l;
// Incremented after every start_draw to prevent overlapping operations
	int operation_count;
};



#endif

