
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

#ifndef RECORDBATCH_H
#define RECORDBATCH_H

#include "asset.inc"
#include "channel.inc"
#include "edl.inc"
#include "file.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "record.inc"
#include "recordlabel.inc"

class Batch
{
public:
	Batch(MWindow *mwindow, Record *record);
	~Batch();

	void create_objects();
	static const char* mode_to_text(int record_mode);
	static int text_to_mode(const char *text);
	const char *get_source_text();
	Channel* get_current_channel();
	void calculate_news();
	void create_default_path();
	void copy_from(Batch *batch);
	void toggle_label(double position);
	void update_times();
	void clear_labels();
	void start_over();
	void set_notice(const char *text) { notice = text; }
	void clear_notice() { notice = 0; }

	MWindow *mwindow;
	Record *record;
	Asset *asset;
	RecordLabels *labels;
	Channel *channel;
	const char *notice;
	int record_mode;
	int recorded;
	int enabled;
	int file_exists;
	double start_time;
	double last_start_time;
	int start_day;
	int last_start_day;
	double duration;
	double record_duration;
	time_t time_start;
	time_t time_end;
	char news[BCTEXTLEN];
};

#endif
