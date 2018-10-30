
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

#ifndef RECORDVIDEO_H
#define RECORDVIDEO_H

#include "condition.inc"
#include "file.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "record.inc"
#include "recordgui.inc"
#include "thread.h"
#include "videodevice.inc"

// Default behavior is to read frames and flash to display.
// Optionally writes to disk.

class RecordVideo : public Thread
{
public:
	RecordVideo(MWindow *mwindow, Record *record);
	~RecordVideo();

	void reset_parameters();
	void run();
// Do all initialization
	void arm_recording();
// Trigger the loop to start
	void start_recording();
	void stop_recording();
	void pause_recording();
	void resume_recording();
	int flush_buffer();
	int write_buffer();
	void delete_buffer();
	void config_update();
	VFrame *get_buffer();
	int read_buffer(VFrame *frame);
	void decompress_buffer(VFrame *frame);
	void wait_startup();

private:
	MWindow *mwindow;
	Record *record;
	RecordGUI *gui;
	int writing_file, buffer_frames;
	int64_t buffer_position;   // Position in output buffer being captured
	VFrame *buffer_frame;
	int write_result, grab_result;
// Capture frame
	VFrame ***frame_ptr;
	long current_frame;
// Frame start of this recording in the file
	long record_start;
// Want one thread to dictate the other during shared device recording.
	int done, recording_paused;
	Condition *trigger_lock;
	Condition *pause_record_lock;
	Condition *record_paused_lock;
};


#endif
