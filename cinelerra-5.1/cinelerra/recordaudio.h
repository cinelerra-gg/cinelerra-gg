
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

#ifndef RECORDAUDIO_H
#define RECORDAUDIO_H

#include "audiodevice.inc"
#include "condition.inc"
#include "guicast.h"
#include "file.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "record.inc"
#include "recordgui.inc"
#include "samples.inc"
#include "thread.h"

class RecordAudio : public Thread
{
public:
	RecordAudio(MWindow *mwindow, Record *record);
	~RecordAudio();

	void run();
	void arm_recording();
	void start_recording();
	int stop_recording();
	void pause_recording();
	void resume_recording();
	void reset_parameters();
	void delete_buffer();
	void config_update();
	Samples **get_buffer();
	int flush_buffer();
	int write_buffer(int samples);
	void wait_startup();
	void set_write_buffer_samples(int samples);
	void set_monitoring(int mode);

private:
	MWindow *mwindow;
	Record *record;
	int done, recording_paused;
	int grab_result, write_result;
	int64_t current_sample;
	int buffer_channels, buffer_samples, write_buffer_samples;
	int channels, writing_file, fragment_samples;
	int64_t fragment_position;
	Samples **input, **buffer;
	RecordGUI *gui;
	Condition *trigger_lock;
	Condition *pause_record_lock;
	Condition *record_paused_lock;
};

#endif
