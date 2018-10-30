
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

#include "asset.h"
#include "batch.h"
#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "filethread.h"
#include "language.h"
#include "libmjpeg.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "record.h"
#include "recordaudio.h"
#include "recordgui.h"
#include "recordvideo.h"
#include "recordmonitor.h"
#include "units.h"
#include "vframe.h"
#include "videodevice.h"

#include <unistd.h>


RecordVideo::RecordVideo(MWindow *mwindow, Record *record)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->record = record;
	this->gui = record->record_gui;
	trigger_lock = new Condition(0, "RecordVideo::trigger_lock");
        pause_record_lock = new Condition(0, "RecordVideo::pause_record_lock");
        record_paused_lock = new Condition(0, "RecordVideo::record_paused_lock");
	frame_ptr = 0;
	buffer_frame = 0;
	reset_parameters();
	done = -1;
}

RecordVideo::~RecordVideo()
{
	stop_recording();
	delete_buffer();
	delete trigger_lock;
        delete pause_record_lock;
        delete record_paused_lock;
}

void RecordVideo::reset_parameters()
{
	write_result = 0;
	writing_file = 0;
	grab_result = 0;
	buffer_frames = mwindow->edl->session->video_write_length;
	buffer_position = 0;
	recording_paused = 0;
	record_start = 0;
	trigger_lock->reset();
        pause_record_lock->reset();
        record_paused_lock->reset();
}

void RecordVideo::arm_recording()
{
	reset_parameters();
	done = 0;
	Thread::start();
}

void RecordVideo::start_recording()
{
	trigger_lock->unlock();
}

void RecordVideo::stop_recording()
{
	if( done ) return;
// Device won't exist if interrupting a cron job
	done = 1;
	if( record->vdevice ) {
// Interrupt IEEE1394 crashes
		record->vdevice->interrupt_crash();
// Interrupt video4linux crashes
		if( record->vdevice->get_failed() )
			Thread::cancel();
	}
	Thread::join();
// Joined in RecordThread
}


VFrame *RecordVideo::get_buffer()
{
	VFrame *result = 0;
	record->file_lock->lock();
	writing_file = record->writing_file > 0 && record->do_video ? 1 : 0;
	if( writing_file ) {
		if( !frame_ptr ) {
			frame_ptr = record->file->get_video_buffer();
			buffer_position = 0;
		}
		result = frame_ptr[0][buffer_position];

	}
	record->file_lock->unlock();
	if( !result ) {
		if( !buffer_frame ) {
			if( !record->fixed_compression ) {
				Asset *asset = record->default_asset;
				int w = asset->width, h = asset->height;
				int cmodel = record->vdevice->get_best_colormodel(asset);
				buffer_frame = new VFrame(w, h, cmodel);
			}
			else
				buffer_frame = new VFrame;
		}
		buffer_position = 0;
		result = buffer_frame;
	}
	return result;
}

void RecordVideo::delete_buffer()
{
	RecordMonitorThread *thr = !record->record_monitor ?
		 0 : record->record_monitor->thread;
	if( thr && thr->running() ) thr->lock_input();
	frame_ptr = 0;
	if( buffer_frame ) {
		delete buffer_frame;
		buffer_frame = 0;
	}
	if( thr && thr->running() ) thr->unlock_input();
}

void RecordVideo::config_update()
{
	VideoDevice *vdevice = record->vdevice;
	vdevice->config_update();
	int width = vdevice->get_iwidth();
	int height = vdevice->get_iheight();
	double frame_rate = vdevice->get_irate();
	float awidth, aheight;
	MWindow::create_aspect_ratio(awidth, aheight, width, height);
	EDLSession *session = record->edl->session;
	SESSION->aspect_w = session->aspect_w = awidth;
	SESSION->aspect_h = session->aspect_h = aheight;
	SESSION->output_w = session->output_w = width;
	SESSION->output_h = session->output_h = height;
	Asset *rf_asset = SESSION->recording_format;
	Asset *df_asset = record->default_asset;
	rf_asset->width  = df_asset->width  = width;
	rf_asset->height = df_asset->height = height;
	rf_asset->frame_rate = df_asset->frame_rate = frame_rate;
}

void RecordVideo::run()
{
// Number of frames for user to know about.
	gui->reset_video();

// Wait for trigger
	trigger_lock->lock("RecordVideo::run");

	while( !done && !write_result ) {
		if( recording_paused ) {
			pause_record_lock->unlock();
			record_paused_lock->lock();
		}
		if( done ) break;
		VideoDevice *vdevice = record->vdevice;
		VFrame *capture_frame = get_buffer();
		vdevice->set_field_order(record->reverse_interlace);
		record->set_do_cursor();
// Capture a frame
		grab_result = read_buffer(capture_frame);
		if( done ) break;
		if( vdevice->config_updated() ) {
			flush_buffer();
			delete_buffer();
			config_update();
			gui->reset_video();
			record->record_monitor->reconfig();
			continue;
		}
		if( grab_result ) {
			Timer::delay(250);
			continue;
		}
		decompress_buffer(capture_frame);
		record->resync();
		write_buffer();
		if( record->monitor_video && capture_frame->get_data() )
			if( !writing_file || !record->is_behind() )
				record->record_monitor->update(capture_frame);
		if( writing_file && record->fill_underrun_frames ) {
			VFrame *last_frame = capture_frame;
			int fill = record->dropped;
			while( --fill >= 0 ) {
				capture_frame = get_buffer();
				capture_frame->copy_from(last_frame);
				last_frame = capture_frame;
				write_buffer();
			}
		}
		else
			record->written_frames += record->dropped;
		if( record->single_frame ) {
			record->single_frame = 0;
			record->stop_writing_file();
		}
		if( done ) break;
		if( !done ) done = write_result;
		if( done ) break;
		record->check_batch_complete();
	}

SET_TRACE
	flush_buffer();
	delete_buffer();
SET_TRACE
//TRACE("RecordVideo::run 2");
	if( write_result ) {
		ErrorBox error_box(_(PROGRAM_NAME ": Error"),
			mwindow->gui->get_abs_cursor_x(1),
			mwindow->gui->get_abs_cursor_y(1));
			error_box.create_objects(_("No space left on disk."));
		error_box.run_window();
	}
SET_TRACE
}

int RecordVideo::read_buffer(VFrame *frame)
{
	return record->vdevice->read_buffer(frame);
}

void RecordVideo::decompress_buffer(VFrame *frame)
{
	if( !strcmp(record->default_asset->vcodec, CODEC_TAG_MJPEG) &&
		record->vdevice->is_compressed(0, 1)) {
		unsigned char *data = frame->get_data();
		int64_t size = frame->get_compressed_size();
		//int64_t allocation = frame->get_compressed_allocated();
		if( data ) {
			int64_t field2_offset = mjpeg_get_field2(data, size);
			frame->set_compressed_size(size);
			frame->set_field2_offset(field2_offset);
		}
	}
}

int RecordVideo::flush_buffer()
{
	record->file_lock->lock();
	if( writing_file && frame_ptr ) {
		int result = record->file->write_video_buffer(buffer_position);
		if( result ) write_result = 1;
		frame_ptr = 0;
	}
	record->file_lock->unlock();
	buffer_position = 0;
	return write_result;
}

int RecordVideo::write_buffer()
{
	++record->written_frames;
	if( ++buffer_position >= buffer_frames )
		flush_buffer();
// HACK
write_result = 0;
	return write_result;
}

void RecordVideo::pause_recording()
{
	recording_paused = 1;
	pause_record_lock->lock();
}

void RecordVideo::resume_recording()
{
	recording_paused = 0;
	record_paused_lock->unlock();
}

