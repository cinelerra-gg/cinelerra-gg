
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
#include "audiodevice.h"
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
#include "meterpanel.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "record.h"
#include "recordaudio.h"
#include "recordconfig.h"
#include "recordgui.h"
#include "recordmonitor.h"
#include "renderengine.h"
#include "samples.h"


RecordAudio::RecordAudio(MWindow *mwindow, Record *record)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->record = record;
	this->gui = record->record_gui;
	trigger_lock = new Condition(0, "RecordAudio::trigger_lock");
	pause_record_lock = new Condition(0, "RecordAudio::pause_record_lock");
	record_paused_lock = new Condition(0, "RecordAudio::record_paused_lock");
	reset_parameters();
}

RecordAudio::~RecordAudio()
{
	delete_buffer();
	delete trigger_lock;
	delete pause_record_lock;
	delete record_paused_lock;
}

void RecordAudio::reset_parameters()
{
	buffer_channels = 0;
	write_result = 0;
	grab_result = 0;
	buffer = 0;
	input = 0;
	channels = 0;
	fragment_samples = 0;
	fragment_position = 0;
	buffer_samples = 0;
	writing_file = 0;
	write_buffer_samples = 0;
	recording_paused = 0;
	pause_record_lock->reset();
	record_paused_lock->reset();
	trigger_lock->reset();
}


void RecordAudio::arm_recording()
{
	reset_parameters();
	Thread::start();
}

void RecordAudio::start_recording()
{
	trigger_lock->unlock();
}

void RecordAudio::set_monitoring(int mode)
{
	if( record->adevice )
		record->adevice->set_monitoring(mode);
}

int RecordAudio::stop_recording()
{
	done = 1;
	if( record->adevice )
		record->adevice->interrupt_crash();
	Thread::join();
	return 0;
}

void RecordAudio::delete_buffer()
{
	if( buffer ) {
		for( int i=0; i<buffer_channels; ++i )
			delete buffer[i];
		delete [] buffer;
		buffer = 0;
		buffer_channels = 0;
	}
	input = 0;
	fragment_position = 0;
}

void RecordAudio::set_write_buffer_samples(int samples)
{
	write_buffer_samples = samples;
}

Samples **RecordAudio::get_buffer()
{
	Samples **result = 0;
	fragment_position = 0;
	record->file_lock->lock();
	writing_file = record->writing_file > 0 && record->do_audio ? 1 : 0;
	if( writing_file ) {
		channels = record->file->asset->channels;
		result = record->file->get_audio_buffer();
	}
	record->file_lock->unlock();
	if( !result ) {
		// when not writing, buffer is only one fragment
		int new_fragment_samples = record->get_fragment_samples();
		if( new_fragment_samples != buffer_samples )
			delete_buffer();
		int record_channels = record->default_asset->channels;
		if( !buffer || buffer_channels != record_channels ) {
			int i = 0;
			Samples **new_buffer = new Samples *[record_channels];
			if( buffer_channels < record_channels ) {
				for( ; i<buffer_channels; ++i )
					new_buffer[i] = buffer[i];
				while( i < record_channels ) // more channels
					new_buffer[i++] = new Samples(new_fragment_samples);
			}
			else {
				for( ; i<record_channels; ++i )
					new_buffer[i] = buffer[i];
				while( i < buffer_channels ) // fewer channels
					delete buffer[i++];
			}
			delete buffer;
			buffer = new_buffer;
			buffer_channels = record_channels;
			fragment_samples = new_fragment_samples;
			buffer_samples = new_fragment_samples;
		}
		set_write_buffer_samples(0);
		channels = record->default_asset->channels;
		result = buffer;
	}
	return result;
}


void RecordAudio::config_update()
{
	AudioDevice *adevice = record->adevice;
	adevice->stop_audio(0);
	adevice->config_update();
	int channels = adevice->get_ichannels();
	int sample_rate = adevice->get_irate();
	int bits = adevice->get_ibits();
	Asset *rf_asset = SESSION->recording_format;
	Asset *df_asset = record->default_asset;
	rf_asset->channels    = df_asset->channels    = channels;
	rf_asset->sample_rate = df_asset->sample_rate = sample_rate;
	rf_asset->bits        = df_asset->bits        = bits;
	adevice->start_recording();
}

void RecordAudio::run()
{
	gui->reset_audio();
	done = 0;

	trigger_lock->lock("RecordAudio::run");

	while( !done && !write_result ) {
		if( recording_paused ) {
			set_monitoring(0);
			flush_buffer();
			delete_buffer();
			pause_record_lock->unlock();
			record_paused_lock->lock();
			set_monitoring(record->monitor_audio);
		}
		if( done ) break;
		AudioDevice *adevice = record->adevice;
		if( !input )
			input = get_buffer();
		if( !input || !channels ) {
			printf("RecordAudio::run: no input/channels\n");
			Timer::delay(250);
			continue;
		}
		int over[channels];  double max[channels];
		grab_result = !input ? 1 :
			adevice->read_buffer(input, channels,
				fragment_samples, over, max, fragment_position);
		if( done ) break;
		if( adevice->config_updated() ) {
			flush_buffer();
			delete_buffer();
			config_update();
			record->update_position();
			set_monitoring(record->monitor_audio);
			record->record_monitor->redraw();
			gui->reset_audio();
			continue;
		}
		if( grab_result ) {
 			int samplerate = record->default_asset->sample_rate;
			int delay = samplerate ? (1000 * fragment_samples) / samplerate : 250;
			if( delay > 250 ) delay = 250;
			Timer::delay(delay);
			continue;
		}
		write_buffer(fragment_samples);
		record->current_sample += fragment_samples;
		record->total_samples += fragment_samples;
		if( !record->writing_file || !record->is_behind() )
			gui->update_audio(channels,max,over);
		record->check_batch_complete();
	}

	if( write_result && !record->default_asset->video_data ) {
		ErrorBox error_box(_(PROGRAM_NAME ": Error"),
			mwindow->gui->get_abs_cursor_x(1),
			mwindow->gui->get_abs_cursor_y(1));
		error_box.create_objects(_("No space left on disk."));
		error_box.run_window();
		done = 1;
	}

	flush_buffer(); // write last buffer
	delete_buffer();
}

int RecordAudio::flush_buffer()
{
	record->file_lock->lock();
	if( record->writing_file ) {
		record->written_samples += fragment_position;
		if( writing_file ) {
			write_result = (record->file->write_audio_buffer(fragment_position), 0); // HACK
			// defeat audio errors if recording video
			if( record->default_asset->video_data ) write_result = 0;
		}
	}
	record->file_lock->unlock();
	input = 0;
	fragment_position = 0;
	return write_result;
}

int RecordAudio::write_buffer(int samples)
{
	int result = 0;
	fragment_position += samples;
	if( fragment_position >= write_buffer_samples )
		result = flush_buffer();
	return result;
}


void RecordAudio::pause_recording()
{
	recording_paused = 1;
	record->adevice->interrupt_recording();
	pause_record_lock->lock();
}

void RecordAudio::resume_recording()
{
	recording_paused = 0;
	record->adevice->resume_recording();
	record_paused_lock->unlock();
}


