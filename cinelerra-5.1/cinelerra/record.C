/*
 * CINELERRA
 * Copyright (C) 2009-2013 Adam Williams <broadcast at earthling dot net>
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
#include "assets.h"
#include "audiodevice.h"
#include "awindow.h"
#include "awindowgui.h"
#include "batch.h"
#include "bchash.h"
#include "channel.h"
#include "channeldb.h"
#include "channelpicker.h"
#include "clip.h"
#include "commercials.h"
#include "condition.h"
#include "cwindow.h"
#include "devicedvbinput.h"
#include "drivesync.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "filexml.h"
#include "filesystem.h"
#include "filethread.h"
#include "formatcheck.h"
#include "indexfile.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "libdv.h"
#include "libmjpeg.h"
#include "libzmpeg3.h"
#include "mainmenu.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "mutex.h"
#include "picture.h"
#include "playbackengine.h"
#include "preferences.h"
#include "record.h"
#include "recordaudio.h"
#include "recordconfig.h"
#include "recordgui.h"
#include "recordlabel.h"
#include "recordmonitor.h"
#include "recordprefs.inc"
#include "recordthread.h"
#include "recordvideo.h"
#include "removefile.h"
#include "mainsession.h"
#include "sighandler.h"
#include "testobject.h"
#include "theme.h"
#include "timebar.h"
#include "tracks.h"
#include "videoconfig.h"
#include "videodevice.h"

#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>


RecordMenuItem::RecordMenuItem(MWindow *mwindow)
 : BC_MenuItem(_("Record..."), "r", 'r')
{
	this->mwindow = mwindow;
	record = new Record(mwindow, this);
}

RecordMenuItem::~RecordMenuItem()
{
	delete record;
}

int RecordMenuItem::handle_event()
{
	record->start();
	return 1;
}


Record::Record(MWindow *mwindow, RecordMenuItem *menu_item)
 : Thread(1, 0, 0),
   record_batches(mwindow)
{
	this->mwindow = mwindow;
	this->menu_item = menu_item;
	mwindow->gui->record = this;
	adevice = 0;
	vdevice = 0;
	file = 0;
	picture = new PictureConfig();
	channeldb = new ChannelDB;
	master_channel = new Channel;
	record_channel = new RecordChannel(this);
	channel = 0;
	current_channel = 0;
	default_asset = 0;
	load_mode = 0 ;
	pause_lock = new Mutex("Record::pause_lock");
	init_lock = new Condition(0,"Record::init_lock");
	record_audio = 0;
	record_video = 0;
	record_thread = 0;
	record_monitor = 0;
	record_gui = 0;
	session_sample_offset = 0;
	device_sample_offset = 0;
	recording = 0;
	capturing = 0;
	single_frame = 0;
	writing_file = 0;
	drop_overrun_frames = 0;
	fill_underrun_frames = 0;
	power_off = 0;
	commercial_check = skimming_active = 0;
	commercial_start_time = -1;
	commercial_fd = -1;
	deletions = 0;
	status_color = -1;
	keybfr[0] = 0;
	last_key = -1;
	do_audio = 0;
	do_video = 0;
	current_frame = written_frames = total_frames = 0;
	current_sample = written_samples = total_samples = 0;
	audio_time = video_time = -1.;
	play_gain = mute_gain = 1.;
	drivesync = 0;
	input_threads_pausing = 0;
	window_lock = new Mutex("Record::window_lock");
	timer_lock = new Mutex("Record::timer_lock");
	file_lock = new Mutex("Record::file_lock");
	adevice_lock = new Mutex("Record::adevice_lock");
	vdevice_lock = new Mutex("Record::vdevice_lock");
	batch_lock = new Mutex("Record::batch_lock");
#ifdef HAVE_COMMERCIAL
	skim_thread = new SkimDbThread();
	cutads_status = new RecordCutAdsStatus(this);
	blink_status = new RecordBlinkStatus(this);
#endif
}

Record::~Record()
{
	mwindow->gui->record = 0;
	stop();
#ifdef HAVE_COMMERCIAL
	delete blink_status;
	delete cutads_status;
	stop_skimming();
	delete skim_thread;
#endif
	delete deletions;
	delete picture;
	delete channeldb;
	delete record_channel;
	delete master_channel;
	delete window_lock;
	delete timer_lock;
	delete record_audio;
	delete record_video;
	delete adevice_lock;
	delete vdevice_lock;
	delete batch_lock;
	delete file_lock;
	delete pause_lock;
	delete init_lock;
}

int Record::load_defaults()
{

	char string[BCTEXTLEN];
	BC_Hash *defaults = mwindow->defaults;
	EDLSession *session = SESSION;
	default_asset->copy_from(session->recording_format, 0);
	default_asset->channels = session->aconfig_in->channels;
	default_asset->sample_rate = session->aconfig_in->in_samplerate;
	default_asset->frame_rate = session->vconfig_in->in_framerate;
	default_asset->width = session->vconfig_in->w;
	default_asset->height = session->vconfig_in->h;
	default_asset->layers = 1;
// Fix encoding parameters depending on driver.
// These are locked by a specific driver.
	const char *vcodec = 0;
	switch( session->vconfig_in->driver ) {
	case CAPTURE_DVB:
	case VIDEO4LINUX2MPEG:
		break;
	case VIDEO4LINUX2JPEG:
		vcodec = CODEC_TAG_MJPEG;
		break;
	case CAPTURE_FIREWIRE:
	case CAPTURE_IEC61883:
		vcodec = CODEC_TAG_DVSD;
		break;
	}
	if( vcodec )
		strcpy(default_asset->vcodec, vcodec);

	record_batches.load_defaults(channeldb, this);

	int cur_chan_no = defaults->get("RECORD_CURRENT_CHANNEL", 0);
	current_channel = channeldb->get(cur_chan_no);
	load_mode = defaults->get("RECORD_LOADMODE", LOADMODE_PASTE);
	monitor_audio = defaults->get("RECORD_MONITOR_AUDIO", 1);
	metering_audio = defaults->get("RECORD_METERING_AUDIO", 1);
	monitor_video = defaults->get("RECORD_MONITOR_VIDEO", 1);
	video_window_open = defaults->get("RECORD_MONITOR_OPEN", 1);
	video_x = defaults->get("RECORD_VIDEO_X", 0);
	video_y = defaults->get("RECORD_VIDEO_Y", 0);
	video_zoom = defaults->get("RECORD_VIDEO_Z", (float)1);
	picture->load_defaults();
	reverse_interlace = defaults->get("REVERSE_INTERLACE", 0);
	do_cursor = defaults->get("RECORD_CURSOR", 0);
	do_big_cursor = defaults->get("RECORD_BIG_CURSOR", 0);
	for( int i=0; i<MAXCHANNELS; ++i ) {
		sprintf(string, "RECORD_DCOFFSET_%d", i);
		dc_offset[i] = defaults->get(string, 0);
	}
	drop_overrun_frames = defaults->get("DROP_OVERRUN_FRAMES", 0);
	fill_underrun_frames = defaults->get("FILL_UNDERRUN_FRAMES", 0);
	commercial_check = defaults->get("COMMERCIAL_CHECK", 0);
	return 0;
}

int Record::save_defaults()
{
	char string[BCTEXTLEN];
	BC_Hash *defaults = mwindow->defaults;
	set_editing_batch(0);
// Save default asset path but not the format because that's
// overridden by the driver.
// The format is saved in preferences.
	if( record_batches.total() )
		strcpy(default_asset->path, record_batches[0]->asset->path);
	default_asset->save_defaults(defaults, "RECORD_", 0, 0, 0, 0, 0);

	record_batches.save_defaults(channeldb);

	int cur_chan_no = channeldb->number_of(current_channel);
	defaults->update("RECORD_CURRENT_CHANNEL", cur_chan_no);
	defaults->update("RECORD_LOADMODE", load_mode);
	defaults->update("RECORD_MONITOR_AUDIO", monitor_audio);
	defaults->update("RECORD_METERING_AUDIO", metering_audio);
	defaults->update("RECORD_MONITOR_VIDEO", monitor_video);
	defaults->update("RECORD_MONITOR_OPEN", video_window_open);
	defaults->update("RECORD_VIDEO_X", video_x);
	defaults->update("RECORD_VIDEO_Y", video_y);
	defaults->update("RECORD_VIDEO_Z", video_zoom);
	picture->save_defaults();
	defaults->update("REVERSE_INTERLACE", reverse_interlace);
	defaults->update("RECORD_CURSOR", do_cursor);
	defaults->update("RECORD_BIG_CURSOR", do_big_cursor);
	for( int i=0; i<MAXCHANNELS; ++i ) {
		sprintf(string, "RECORD_DCOFFSET_%d", i);
		defaults->update(string, dc_offset[i]);
	}
	defaults->update("DROP_OVERRUN_FRAMES", drop_overrun_frames);
	defaults->update("FILL_UNDERRUN_FRAMES", fill_underrun_frames);
	defaults->update("COMMERCIAL_CHECK", commercial_check);
SET_TRACE

	return 0;
}

void Record::configure_batches()
{
// printf("Record::configure_batches %d\n",__LINE__);
// default_assset->dump();
	strcpy(record_batches[0]->asset->path, default_asset->path);
	int total_batches = record_batches.total();
	for( int i=0; i<total_batches; ++i ) {
		Batch *batch = record_batches[i];
		batch->asset->copy_format(default_asset);
	}
}

void Record::run()
{
	int result = 0;
	record_gui = 0;
// Default asset forms the first path in the batch capture
// and the file format for all operations.
	default_asset = new Asset;

// Determine information about the device.
	AudioInConfig *aconfig_in = SESSION->aconfig_in;
	VideoInConfig *vconfig_in = SESSION->vconfig_in;
	int driver = vconfig_in->driver;
	VideoDevice::load_channeldb(channeldb, vconfig_in);
	fixed_compression = VideoDevice::is_compressed(driver, 0, 1);
	load_defaults();
// Apply a major kludge
	if( fixed_compression ) {
		VideoDevice device;
		device.fix_asset(default_asset, driver);
	}
	default_asset->channels = aconfig_in->channels;
	VideoDevice::save_channeldb(channeldb, vconfig_in);
	save_defaults();
	mwindow->save_defaults();
	configure_batches();
	set_current_batch(0);
	set_editing_batch(0);

// Run recordgui
	edl = new EDL;
	edl->create_objects();
	edl->session->output_w = default_asset->width;
	edl->session->output_h = default_asset->height;
	edl->session->aspect_w = SESSION->aspect_w;
	edl->session->aspect_h = SESSION->aspect_h;

	window_lock->lock("Record::run 3");
	record_gui = new RecordGUI(mwindow, this);
	record_gui->create_objects();

	record_monitor = new RecordMonitor(mwindow, this);
	record_monitor->create_objects();
	record_gui->update_batch_sources();

	record_gui->show_window();
	record_gui->flush();

	if( mwindow->gui->remote_control->deactivate() )
		mwindow->gui->record_remote_handler->activate();

	if( video_window_open ) {
		record_monitor->window->show_window();
		record_monitor->window->raise_window();
		record_monitor->window->flush();
	}

	window_lock->unlock();
	channel = 0;
	start_input_threads();
	update_position();
	init_lock->unlock();

	result = record_gui->run_window();
// record gui operates here until window is closed
// wait for it
	if( record_gui->get_window_lock() ) {
		record_gui->lock_window();
		record_gui->unlock_window();
	}

// Need to stop everything this time

	int video_stream = -1;
#ifdef HAVE_COMMERCIAL
	stop_commercial_capture(0);
#endif
#ifdef HAVE_LIBZMPEG
	if( default_asset->format == FILE_MPEG ) {
		Channel *channel = get_current_channel();
		if( channel )
			video_stream = channel->video_stream;
	}
#endif
	stop(0);
	edl->Garbage::remove_user();

	if( mwindow->gui->remote_control->deactivate() )
		mwindow->gui->cwindow_remote_handler->activate();

// Save everything again
	save_defaults();

// Paste into EDL
	if( !result && load_mode != LOADMODE_NOTHING ) {
		mwindow->gui->lock_window("Record::run");
		ArrayList<EDL*> new_edls;
// Paste assets
		int total_batches = record_batches.total();
		for( int i=0; i<total_batches; ++i ) {
			Batch *batch = record_batches[i];
			Asset *asset = batch->asset;
			if( batch->recorded ) {
				EDL *new_edl = new EDL;
				mwindow->remove_asset_from_caches(asset);
				new_edl->create_objects();
				new_edl->copy_session(mwindow->edl);
				mwindow->asset_to_edl(new_edl, asset, batch->labels);
				new_edls.append(new_edl);
			}
		}

		if(new_edls.total) {
			mwindow->undo->update_undo_before();
// For pasting, clear the active region
			if(load_mode == LOADMODE_PASTE)
				mwindow->clear(0);
			int loadmode = load_mode == LOADMODE_RESOURCESONLY ?
				LOADMODE_ASSETSONLY : load_mode;
			mwindow->paste_edls(&new_edls, loadmode, 0, -1,
				SESSION->labels_follow_edits,
				SESSION->plugins_follow_edits,
				SESSION->autos_follow_edits,
				0);// overwrite
			for( int i=0; i<new_edls.total; ++i )
				new_edls.get(i)->Garbage::remove_user();
			new_edls.remove_all();
			if( video_stream >= 0 ) {
				mwindow->select_asset(video_stream, -1);
				mwindow->fit_selection();
			}
			mwindow->save_backup();
			mwindow->undo->update_undo_after(_("record"), LOAD_ALL);
			mwindow->restart_brender();
			mwindow->update_plugin_guis();
			mwindow->gui->update(1, 2, 1, 1, 1, 1, 0);
			mwindow->sync_parameters(CHANGE_ALL);
		}
		mwindow->gui->unlock_window();

		AWindowGUI *agui = mwindow->awindow->gui;
		agui->async_update_assets();
	}

// Delete everything
	record_batches.clear();
	default_asset->Garbage::remove_user();
}

void Record::stop(int wait)
{
	stop_operation();
	if( record_gui )
		record_gui->set_done(1);
	if( wait )
		join();
	window_lock->lock("Record::stop");
	delete record_thread;   record_thread = 0;
	delete record_monitor;  record_monitor = 0;
	delete record_gui;      record_gui = 0;
	window_lock->unlock();
}

void Record::activate_batch(int number)
{
	if( number != current_batch() ) {
		stop_writing();
		set_current_batch(number);
		record_gui->update_batches();
		record_gui->update_position(current_display_position());
		record_gui->update_batch_tools();
	}
}


void Record::delete_index_file(Asset *asset)
{
	IndexFile::delete_index(mwindow->preferences, asset, ".toc");
	IndexFile::delete_index(mwindow->preferences, asset, ".idx");
	IndexFile::delete_index(mwindow->preferences, asset, ".mkr");
}

void Record::delete_batch()
{
// Abort if one batch left
	int edit_batch = editing_batch();
	int total_batches = record_batches.total();
	if( total_batches > 1 && edit_batch < total_batches ) {
		int cur_batch = current_batch();
		Batch *batch = record_batches[edit_batch];
		record_batches.remove(batch);  delete batch;
		if( cur_batch == edit_batch ) stop_writing();
		record_gui->update_batches();
		record_gui->update_batch_tools();
	}
}

void Record::change_editing_batch(int number)
{
	set_editing_batch(number);
	record_gui->update_batch_tools();
}

Batch* Record::new_batch()
{
	Batch *batch = new Batch(mwindow, this);
//printf("Record::new_batch 1\n");
	batch->create_objects();
	record_batches.append(batch);
	batch->asset->copy_format(default_asset);

//printf("Record::new_batch 1\n");
	batch->create_default_path();
	Batch *edit_batch = get_editing_batch();
	if( edit_batch )
		batch->copy_from(edit_batch);
	int total_batches = record_batches.total();
	set_editing_batch(total_batches - 1);
//printf("Record::new_batch 1\n");
// Update GUI if created yet
	if(record_gui)
		record_gui->update_batch_tools();
//printf("Record::new_batch 2\n");
	return batch;
}

int Record::current_batch()
{
	return record_batches.current_item;
}

int Record::set_current_batch(int i)
{
	return record_batches.current_item = i;
}

int Record::editing_batch()
{
	return record_batches.editing_item;
}

int Record::set_editing_batch(int i)
{
	return record_batches.editing_item = i;
}

int Record::delete_output_file()
{
	Batch *batch = get_current_batch();
// Delete old file
 	if( !file && batch && !access(batch->asset->path, F_OK) ) {
		batch->set_notice(_("Deleting"));
		record_gui->update_batches();
		Asset *asset = batch->asset;
		remove_file(asset->path);
		mwindow->remove_asset_from_caches(asset);
		delete_index_file(asset);
		batch->clear_notice();
		record_gui->update_batches();
	} while(0);
	return 0;
}

int Record::open_output_file()
{
	int result = 0;
	file_lock->lock();
	Batch *batch = get_current_batch();
	if( !file && batch ) {
		delete_output_file();
		Asset *asset = batch->asset;
		asset->frame_rate = default_asset->frame_rate;
		asset->sample_rate = default_asset->sample_rate;
		asset->width = default_asset->width;
		asset->height = default_asset->height;
		int wr = 1;
		if( default_asset->format == FILE_MPEG ) {
			asset->layers = vdevice ? vdevice->total_video_streams() : 0;
			asset->channels = adevice ? adevice->total_audio_channels() : 0;
			wr = 0;
		}
		file = new File;
		result = file->open_file(mwindow->preferences, asset, 0, wr);
		if( !result ) {
			mwindow->sighandler->push_file(file);
			batch->recorded = 1;
			file->set_processors(mwindow->preferences->real_processors);
			record_gui->update_batches();
		}
		else {
			delete file;
			file = 0;
		}
	}
	file_lock->unlock();
	return result;
}

void Record::start()
{
	if( running() ) {
		window_lock->lock("Record::start");
		record_gui->lock_window("Record::start");
		record_gui->raise_window();
		record_gui->unlock_window();
		window_lock->unlock();
	}
	else {
		init_lock->reset();
		Thread::start();
	}
}

void Record::start_over()
{
	stop_writing();
	Batch *batch = get_current_batch();
	if( batch ) batch->start_over();
	record_gui->update_batches();
}

void Record::stop_operation()
{
	stop_writing();
	stop_input_threads();
}

int Record::cron_active()
{
	return !record_thread ? 0 : record_thread->cron_active;
}

void Record::close_output_file()
{
	file_lock->lock();
	if( file ) {
		mwindow->sighandler->pull_file(file);
		file->close_file();
		delete file;
		file = 0;
	}
	file_lock->unlock();
	record_gui->update_batches();
}

void Record::toggle_label()
{
	Batch *batch = get_current_batch();
	if( batch ) batch->toggle_label(current_display_position());
	record_gui->update_labels(current_display_position());
}

void Record::clear_labels()
{
	Batch *batch = get_current_batch();
	if( batch ) batch->clear_labels();
	record_gui->update_labels(0);
}

int Record::get_fragment_samples()
{
	const int min_samples = 1024, max_samples = 32768;
	int samples, low_bit;
	samples = default_asset->sample_rate / SESSION->record_speed;
	if( samples < min_samples ) samples = min_samples;
	else if( samples > max_samples ) samples = max_samples;
	// loop until only one bit left standing
	while( (low_bit=(samples & ~(samples-1))) != samples )
		samples += low_bit;
	return samples;
}

int Record::get_buffer_samples()
{
	int fragment_samples = get_fragment_samples();
	if( !fragment_samples ) return 0;
	int fragments = (SESSION->record_write_length+fragment_samples-1) / fragment_samples;
	if( fragments < 1 ) fragments = 1;
	return fragments * fragment_samples;
}

Batch* Record::get_current_batch()
{
	return record_batches.get_current_batch();
}

Batch* Record::get_editing_batch()
{
	return record_batches.get_editing_batch();
}

int Record::get_next_batch(int incr)
{
	int i = current_batch();
	if( i >= 0 ) {
		i += incr;
		int total_batches = record_batches.total();
		while( i < total_batches ) {
			if( record_batches[i]->enabled ) return i;
			++i;
		}
	}
	return -1;
}

const char* Record::current_mode()
{
	Batch *batch = get_current_batch();
	return batch ? Batch::mode_to_text(batch->record_mode) : "";
}

double Record::current_display_position()
{
//printf("Record::current_display_position "%jd %jd\n", total_samples, total_frames);
	double result = -1.;
	Asset *asset = default_asset;
	if( writing_file ) {
		if( asset->video_data && record_video )
			result = (double) written_frames / asset->frame_rate;
		else if( asset->audio_data && record_audio )
			result = (double) written_samples / asset->sample_rate;
	}
	if( result < 0. ) {
		if( asset->video_data && record_video )
			result = (double) total_frames / asset->frame_rate;
		else if( asset->audio_data && record_audio )
			result = (double) total_samples / asset->sample_rate;
		else
			result = (double) total_time.get_difference() / 1000.;
	}
	return result;
}

const char* Record::current_source()
{
	Batch *batch = get_current_batch();
	return batch ? batch->get_source_text() : _("Unknown");
}

Asset* Record::current_asset()
{
	Batch *batch = get_current_batch();
	return batch ? batch->asset : 0;
}

Channel *Record::get_current_channel()
{
	return current_channel;
}

Channel *Record::get_editing_channel()
{
	Batch *batch = get_editing_batch();
	return batch ? batch->channel : 0;
}

ArrayList<Channel*>* Record::get_video_inputs()
{
	return default_asset->video_data && vdevice ? vdevice->get_inputs() : 0;
}


int64_t Record::timer_position()
{
	timer_lock->lock("RecordAudio::timer_positioin");
	int samplerate = default_asset->sample_rate;
	int64_t result = timer.get_scaled_difference(samplerate);
	result += session_sample_offset;
	timer_lock->unlock();
	return result;
}

void Record::reset_position(int64_t position)
{
	timer_lock->lock("RecordAudio::reset_position");
	session_sample_offset = position;
	device_sample_offset = adevice ? adevice->current_position() : 0;
	timer.update();
	timer_lock->unlock();
}

void Record::update_position()
{
	int64_t position = sync_position();
	reset_position(position);
	current_frame = current_sample = 0;
	audio_time = video_time = -1.;
}

int64_t Record::adevice_position()
{
	timer_lock->lock("RecordAudio::adevice_position");
	int64_t result = adevice->current_position();
	result += session_sample_offset - device_sample_offset;
	timer_lock->unlock();
	return result;
}

int64_t Record::sync_position()
{
	int64_t sync_position = -1;
	double sync_time = -1.;
	double next_audio_time = -1.;
	double next_video_time = -1.;

	if( current_frame == 0 && current_sample == 0 ) {
		audio_time = video_time = -1.;
		reset_position(0);
	}
	switch( SESSION->record_positioning ) {
	case RECORD_POS_TIMESTAMPS:
		if( default_asset->video_data && record_video )
			next_video_time = vdevice->get_timestamp();
		if( default_asset->audio_data && record_audio ) {
			next_audio_time =
				 !adevice->is_monitoring() || writing_file > 0 ?
					adevice->get_itimestamp() :
					adevice->get_otimestamp();
		}
		if( next_audio_time >= 0. )
			sync_time = next_audio_time;
		else if( next_video_time > 0. )
			sync_time = next_video_time;
		if( sync_time >= 0. ) {
			sync_position = sync_time * default_asset->sample_rate;
		}
		if( sync_position >= 0 ) break;
	case RECORD_POS_DEVICE:
		if( default_asset->audio_data && record_audio )
			sync_position = adevice_position();
		if( sync_position > 0 ) break;
	case RECORD_POS_SOFTWARE:
		sync_position = timer_position();
		if( sync_position > 0 ) break;
	case RECORD_POS_SAMPLE:
	default:
		sync_position = current_sample;
		break;
	}

	if( next_video_time < 0. )
		next_video_time = current_frame / default_asset->frame_rate;
	if( next_video_time > video_time )
		video_time = next_video_time;
	if( next_audio_time < 0. )
		next_audio_time = (double)sync_position / default_asset->sample_rate;
	if( next_audio_time > audio_time )
		audio_time = next_audio_time;

	return sync_position;
}


void Record::adevice_drain()
{
	if( adevice && record_audio ) {
		adevice->stop_audio(0);
		adevice->start_recording();
	}
}


#define dbmsg if( debug ) printf
void Record::resync()
{
	dropped = behind = 0;
	int64_t audio_position = sync_position();
	double audiotime = (double) audio_position / default_asset->sample_rate;
	double diff = video_time - audiotime;
	const int debug = 0;
	dbmsg("resync video %f audio %f/%f diff %f",
		video_time, audiotime, audio_time, diff);
	if( fabs(diff) > 5. ) {
		dbmsg("  drain audio");
// jam job dvb file tesing, comment next line
		record_channel->drain_audio();
	}
	else if( diff > 0. ) {
		int64_t delay = (int64_t)(1000.0 * diff);
		dbmsg("  delay %8jd", delay);
		if( delay > 500 ) {
			video_time = audio_time = -1.;
			delay = 500;
		}
		if( delay > 1 )
			Timer::delay(delay);
	}
	else {
		behind = (int)(-diff * default_asset->frame_rate);
		dbmsg("  behind %d", behind);
		if( behind > 1 && fill_underrun_frames ) {
			dropped = behind > 3 ? 3 : behind-1;
			dbmsg("  dropped %d", dropped);
		}
	}
	dbmsg("\n");
	int frames = dropped + 1;
	current_frame += frames;
	total_frames += frames;
	record_gui->update_video(dropped, behind);
	if( dropped > 0 && drop_overrun_frames )
		if( vdevice ) vdevice->drop_frames(dropped);
}


void Record::close_audio_input()
{
	adevice_lock->lock();
	if( adevice ) {
		adevice->close_all();
		delete adevice;
		adevice = 0;
	}
	adevice_lock->unlock();
}

void Record::close_video_input()
{
	vdevice_lock->lock();
	if( vdevice ) {
		vdevice->close_all();
		delete vdevice;
		vdevice = 0;
	}
	vdevice_lock->unlock();
}

void Record::close_input_devices()
{
	close_audio_input();
	close_video_input();
}

void Record::stop_audio_thread()
{
	if( record_audio ) {
		record_audio->stop_recording();
		delete record_audio;
		record_audio = 0;
	}
	close_audio_input();
	recording = 0;
}

void Record::stop_video_thread()
{
	if( record_video ) {
		record_video->stop_recording();
		delete record_video;
		record_video = 0;
	}
	close_video_input();
	capturing = 0;
	single_frame = 0;
}

void Record::stop_input_threads()
{
	pause_lock->lock("Record::stop_input_threads");
	stop_skimming();
	stop_playback();
	stop_audio_thread();
	stop_video_thread();
	pause_lock->unlock();
}

void Record::stop_playback()
{
	if( record_monitor )
		record_monitor->stop_playback();
}

void Record::open_audio_input()
{
	close_audio_input();
	adevice_lock->lock();
// Create devices
	if( default_asset->audio_data )
		adevice = new AudioDevice(mwindow);
// Configure audio
	if( adevice ) {
		int sw_pos = SESSION->record_positioning == RECORD_POS_SOFTWARE;
		adevice->set_software_positioning(sw_pos);
		adevice->open_input(SESSION->aconfig_in, SESSION->vconfig_in,
			default_asset->sample_rate, get_fragment_samples(),
			default_asset->channels, SESSION->real_time_record);
		adevice->start_recording();
		adevice->open_monitor(SESSION->playback_config->aconfig,monitor_audio);
		adevice->set_vdevice(vdevice);
		if( vdevice ) vdevice->set_adevice(adevice);
	}
	adevice_lock->unlock();
}

void Record::open_video_input()
{
	close_video_input();
	vdevice_lock->lock();
	if( default_asset->video_data )
		vdevice = new VideoDevice(mwindow);
// Initialize video
	if( vdevice ) {
		vdevice->set_quality(default_asset->jpeg_quality);
		vdevice->open_input(SESSION->vconfig_in, video_x, video_y,
			video_zoom, default_asset->frame_rate);
// Get configuration parameters from device probe
		color_model = vdevice->get_best_colormodel(default_asset);
		master_channel->copy_usage(vdevice->channel);
		picture->copy_usage(vdevice->picture);
		vdevice->set_field_order(reverse_interlace);
		vdevice->set_do_cursor(do_cursor, do_big_cursor);
		vdevice->set_adevice(adevice);
		if( adevice ) adevice->set_vdevice(vdevice);
		set_dev_channel(get_current_channel());
	}
	vdevice_lock->unlock();
}

void Record::start_audio_thread()
{
	open_audio_input();
	if( !record_audio ) {
		total_samples = current_sample = 0;  audio_time = -1.;
		record_audio = new RecordAudio(mwindow,this);
		record_audio->arm_recording();
		record_audio->start_recording();
	}
	recording = 1;
}

void Record::start_video_thread()
{
	open_video_input();
	if( !record_video ) {
		total_frames = current_frame = 0;  video_time = -1.;
		record_video = new RecordVideo(mwindow,this);
		record_video->arm_recording();
		record_video->start_recording();
	}
	capturing = 1;
}

void Record::start_input_threads()
{
	behind = 0;
	input_threads_pausing = 0;
	if( default_asset->audio_data )
		start_audio_thread();
	if( default_asset->video_data )
		start_video_thread();
	start_skimming();
}

void Record::pause_input_threads()
{
	pause_lock->lock("Record::pause_input_threads");
	input_threads_pausing = 1;
	stop_skimming();
	adevice_lock->lock();
	vdevice_lock->lock();
	if( record_audio )
		record_audio->pause_recording();
	if( record_video )
		record_video->pause_recording();
}

void Record::resume_input_threads()
{
	if( record_audio )
		record_audio->resume_recording();
	if( record_video )
		record_video->resume_recording();
	vdevice_lock->unlock();
	adevice_lock->unlock();
	input_threads_pausing = 0;
	start_skimming();
	pause_lock->unlock();

}

int Record::start_toc()
{
	if( !mwindow->edl->session->record_realtime_toc ) return 0;
	Batch *batch = get_current_batch();
	Asset *asset = batch->asset;
	char source_filename[BCTEXTLEN], toc_path[BCTEXTLEN];
	IndexFile::get_index_filename(source_filename,
		mwindow->preferences->index_directory,
		toc_path, asset->path,".toc");
	if( default_asset->video_data )
		return vdevice->start_toc(asset->path, toc_path);
	if( default_asset->audio_data )
		return adevice->start_toc(asset->path, toc_path);
	return -1;
}

int Record::start_record(int fd)
{
	start_toc();
	if( default_asset->video_data )
		return vdevice->start_record(fd);
	if( default_asset->audio_data )
		return adevice->start_record(fd);
	return -1;
}

void Record::start_writing_file()
{
	if( !writing_file ) {
		written_frames = 0;
		written_samples = 0;
		do_video = File::renders_video(default_asset);
		do_audio = File::renders_audio(default_asset);
		if( single_frame ) do_audio = 0;
		if( !do_video && single_frame )
			single_frame = 0;
		else if( !open_output_file() )
			writing_file = default_asset->format == FILE_MPEG ? -1 : 1;
		if( do_audio && record_audio && writing_file > 0 ) {
			int buffer_samples = get_buffer_samples();
			record_audio->set_write_buffer_samples(buffer_samples);
			file->start_audio_thread(buffer_samples, FILE_RING_BUFFERS);
		}
		if( do_video && record_video && writing_file > 0 ) {
			int disk_frames = SESSION->video_write_length;
			int cmodel = vdevice->get_best_colormodel(default_asset);
			int cpress = vdevice->is_compressed(1, 0);
			file->start_video_thread(disk_frames, cmodel, FILE_RING_BUFFERS, cpress);
		}
		if( writing_file < 0 ) {
			int fd = file->record_fd();
			if( fd >= 0 )
				start_record(fd);
		}
		if( writing_file ) {
			record_gui->start_flash_batch();
			if( SESSION->record_sync_drives ) {
				drivesync = new DriveSync();
				drivesync->start();
			}
		}
	}
	update_position();
}

int Record::stop_record()
{
	if( default_asset->video_data )
		return vdevice->stop_record();
	if( default_asset->audio_data )
		return adevice->stop_record();
	return -1;
}

void Record::flush_buffer()
{
	if( record_audio )
		record_audio->flush_buffer();
	if( record_video )
		record_video->flush_buffer();
}

void Record::stop_writing_file()
{
	record_gui->stop_flash_batch();
	if( !writing_file ) return;
	if( writing_file > 0 )
		flush_buffer();
	if( writing_file < 0 )
		stop_record();
	close_output_file();
	writing_file = 0;
	Asset *asset = current_asset();
	asset->audio_length = written_samples;
	asset->video_length = written_frames;
	record_gui->flash_batch();
	if( drivesync ) {
		drivesync->done = 1;
		delete drivesync;
		drivesync = 0;
	}
}

void Record::stop_writing()
{
	if( writing_file ) {
		pause_input_threads();
		stop_writing_file();
		resume_input_threads();
	}
}


void Record::start_cron_thread()
{
	if( cron_active() < 0 ) {
		delete record_thread;
		record_thread = 0;
	}
	if( !record_thread ) {
		record_thread = new RecordThread(mwindow,this);
		record_thread->start();
		record_gui->disable_batch_buttons();
		record_gui->update_cron_status(_("Running"));
	}
}

void Record::stop_cron_thread(const char *msg)
{
	if( record_thread ) {
		delete record_thread;
		record_thread = 0;
		record_gui->enable_batch_buttons();
		record_gui->update_cron_status(msg);
	}
}

void Record::set_power_off(int value)
{
	power_off = value;
	record_gui->power_off->update(value);
}

void Record::set_video_picture()
{
	if( default_asset->video_data && vdevice )
		vdevice->set_picture(picture);
}

void Record::set_do_cursor()
{
	vdevice->set_do_cursor(do_cursor, do_big_cursor);
}

void Record::set_translation(int x, int y)
{
	video_x = x;
	video_y = y;
	if(default_asset->video_data && vdevice)
		vdevice->set_translation(video_x, video_y);
}

int Record::set_channel(Channel *channel)
{
	if( !channel ) return 1;
printf("set_channel %s\n",channel->title);
	if( record_channel->set(channel) ) return 1;
	RecordMonitorGUI *window = record_monitor->window;
	window->lock_window("Record::set_channel");
	window->channel_picker->channel_text->update(channel->title);
	window->unlock_window();
	return 0;
}

int Record::set_channel_no(int chan_no)
{
	if( chan_no < 0 || chan_no >= channeldb->size() ) return 1;
	Channel *channel = channeldb->get(chan_no);
	return set_channel(channel);
}

int Record::channel_down()
{
	Channel *channel = get_current_channel();
	if( !channel || !channeldb->size() ) return 1;
	int n = channeldb->number_of(channel);
	if( n < 0 ) return 1;
	if( --n < 0 ) n =  channeldb->size() - 1;
	return set_channel_no(n);
}

int Record::channel_up()
{
	Channel *channel = get_current_channel();
	if( !channel || !channeldb->size() ) return 1;
	int n = channeldb->number_of(channel);
	if( n < 0 ) return 1;
	if( ++n >=  channeldb->size() ) n = 0;
	return set_channel_no(n);
}

int Record::set_channel_name(const char *name)
{
	Channel *channel = 0;
	int ch = 0, nch = channeldb->size();
	while( ch < nch ) {
		channel = channeldb->get(ch);
		if( channel && !channel->cstrcmp(name) ) break;
		++ch;
	}
	if( ch >= nch || !channel ) return 1;
	return set_channel(channel);
}

void Record::set_batch_channel_no(int chan_no)
{
	Channel *channel = channeldb->get(chan_no);
	if( channel ) {
		Batch *batch = get_editing_batch();
		if( batch ) batch->channel = channel;
		record_gui->batch_source->update(channel->title);
	}
}

void Record::set_dev_channel(Channel *channel)
{
	// should be tuner device, not vdevice
	if( channel && vdevice &&
	    (!this->channel || *channel != *this->channel) ) {
		current_channel = channel;
		if( this->channel ) delete this->channel;
		this->channel = new Channel(channel);
		vdevice->set_channel(channel);
		int strk = !SESSION->decode_subtitles ? -1 : SESSION->subtitle_number;
		vdevice->set_captioning(strk);
		set_video_picture();
printf("new channel %s, has_signal=%d\n",channel->title,vdevice->has_signal());
		total_frames = 0;
		total_samples = 0;
		total_time.update();
	}
}

/* widget holds xlock and set_channel pauses rendering, deadlock */
/*   use a background thread to update channel after widget sets it */
RecordChannel::RecordChannel(Record *record)
 : Thread(1, 0, 0)
{
	this->record = record;
	change_channel = new Condition(0,"RecordSetChannel::change_channel");
	channel_lock = new Condition(1,"Record::channel_lock",1);
	new_channel = 0;
	Thread::start();
}

RecordChannel::~RecordChannel()
{
	done = 1;
	change_channel->unlock();
	Thread::join();
	delete change_channel;
	delete channel_lock;
}

int RecordChannel::set(Channel *channel)
{
	if( !channel || new_channel ) return 1;
	new_channel = channel;
	channel_lock->lock();        // block has_signal
	change_channel->unlock();    // resume channel changer thread
	return 0;
}

void RecordChannel::drain_audio()
{
	if( !audio_drain ) {
		audio_drain = 1;
		change_channel->unlock();
	}
}

void RecordChannel::run()
{
	done = 0;
	while( !done ) {
		change_channel->lock();
		if( done ) break;
		if( !new_channel && !audio_drain ) continue;
		record->pause_input_threads();
		if( new_channel ) {
			record->set_dev_channel(new_channel);
			record->record_gui->reset_video();
			record->record_gui->reset_audio();
			audio_drain = 0;
		}
		if( audio_drain ) {
			audio_drain = 0;
			record->adevice_drain();
		}
		record->update_position();
		record->resume_input_threads();
		if( new_channel ) {
			new_channel = 0;
			channel_lock->unlock();
		}
	}
}

int Record::has_signal()
{
	record_channel->channel_lock->lock("Record::has_signal");
	record_channel->channel_lock->unlock();
	vdevice_lock->lock();
	int result = vdevice ? vdevice->has_signal() : 0;
	vdevice_lock->unlock();
	return result;
}

int Record::create_channeldb(ArrayList<Channel*> *channeldb)
{
	return vdevice ? vdevice->create_channeldb(channeldb) : 1;
}


void Record::set_audio_monitoring(int mode)
{
	update_position();
	monitor_audio = mode;
	if( record_audio )
		record_audio->set_monitoring(mode);
	record_gui->lock_window("Record::set_audio_monitoring");
	if( record_gui->monitor_audio )
		record_gui->monitor_audio->update(mode);
	record_gui->unlock_window();
}

void Record::set_audio_metering(int mode)
{
	metering_audio = mode;
	record_gui->lock_window("Record::set_audio_metering");
	if( record_gui->meter_audio )
		record_gui->meter_audio->update(mode);
	record_gui->unlock_window();
	RecordMonitorGUI *window = record_monitor->window;
	window->lock_window("Record::set_audio_metering 1");
	window->resize_event(window->get_w(),window->get_h());
	window->unlock_window();
}


void Record::set_video_monitoring(int mode)
{
	monitor_video = mode;
	record_gui->lock_window("Record::set_video_monitoring");
	if( record_gui->monitor_video )
		record_gui->monitor_video->update(mode);
	record_gui->flush();
	record_gui->unlock_window();
}

int Record::get_time_format()
{
	return SESSION->time_format;
}

int Record::set_record_mode(int value)
{
	Batch *batch = get_editing_batch();
	batch->record_mode = value;
	record_gui->update_batches();
	return 0;
}

int Record::check_batch_complete()
{
	int result = 0;
	batch_lock->lock();
	if( writing_file && cron_active() > 0 ) {
		Batch *batch = get_current_batch();
		if( batch && batch->recorded > 0 &&
			batch->record_mode == RECORD_TIMED &&
			current_display_position() >= batch->record_duration ) {
			batch->recorded = -1;
			batch->enabled = 0;
			record_gui->update_batches();
			record_thread->batch_timed_lock->unlock();
			result = 1;
		}
	}
	batch_lock->unlock();
	return result;
}

#ifdef HAVE_COMMERCIAL
int Record::skimming(void *vp, int track)
{
	return ((Record*)vp)->skimming(track);
}

int Record::skimming(int track)
{
	int64_t framenum; uint8_t *tdat; int mw, mh;
	if( !vdevice ) return -1;
	if( vdevice->get_thumbnail(track, framenum, tdat, mw, mh) ) return 1;
	int pid, width, height;  double framerate;
	if( vdevice->get_video_info(track, pid, framerate, width, height, 0) ) return 1;
	if( !framerate ) return 1;
	return skim_thread->skim(pid,framenum,framerate, tdat,mw,mh,width,height);
}

void Record::start_skimming()
{
	if( commercial_check && vdevice && !skimming_active ) {
		skimming_active = 1;
		skim_thread->start(mwindow->commercials);
		vdevice->set_skimming(0, 0, skimming, this);
	}
}

void Record::stop_skimming()
{
	if( skimming_active && vdevice ) {
		skimming_active = 0;
		vdevice->set_skimming(0, 0, 0, 0);
		skim_thread->stop();
	}
}

void Record::update_skimming(int v)
{
	if( (commercial_check=v) != 0 )
		start_skimming();
	else
		stop_skimming();
}

#else
int Record::skimming(void *vp, int track) { return 1; }
int Record::skimming(int track) { return 1; }
void Record::start_skimming() {}
void Record::stop_skimming() {}
void Record::update_skimming(int v) {}
#endif

RecordRemoteHandler::RecordRemoteHandler(RemoteControl *remote_control)
 : RemoteHandler(remote_control->gui, GREEN)
{
}

RecordRemoteHandler::~RecordRemoteHandler()
{
}

void Record::
display_video_text(int x, int y, const char *text, int font,
	int bg_color, int color, int alpha, double secs, double scale)
{
// gotta have a window for resources
	record_monitor->window->
		display_video_text(x, y, text, font,
			bg_color, color, alpha, secs, scale);
}

void Record::
display_cut_icon(int x, int y)
{
	VFrame *cut_icon = *mwindow->theme->get_image_set("commercial");
	double scale = 3;
	x += cut_icon->get_w()*scale;  y += cut_icon->get_h()*scale;
	display_vframe(cut_icon, x, y, 200, 1.0, scale);
}

#ifdef HAVE_DVB
DeviceDVBInput *Record::
dvb_device()
{
	DeviceDVBInput *dvb_dev = !vdevice ? 0 :
		(DeviceDVBInput *)vdevice->mpeg_device();
	if( !dvb_dev ) dvb_dev = !adevice ? 0 :
		(DeviceDVBInput *)adevice->mpeg_device();
	return dvb_dev;
}
#endif

#if 0

void Record::
display_vframe(VFrame *in, int x, int y, int alpha, double secs, double scale)
{
	if( !channel ) return;
	if( !vdevice || vdevice->in_config->driver != CAPTURE_DVB ) return;
	DeviceDVBInput *dvb_input = dvb_device();
	if( !dvb_input ) return 1;
	zmpeg3_t *fd = dvb_input->get_src();
	if( !fd ) return;
	scale *= fd->video_height(channel->video_stream) / 1080.;
	int ww = in->get_w() * scale, hh = in->get_h() * scale;
	VFrame out(ww, hh, BC_YUV444P);
	BC_WindowBase::get_cmodels()->transfer(out.get_rows(), in->get_rows(),
		out.get_y(), out.get_u(), out.get_v(),
		in->get_y(), in->get_u(), in->get_v(),
		0, 0, in->get_w(), in->get_h(),
		0, 0, out.get_w(), out.get_h(),
		in->get_color_model(), out.get_color_model(), 0,
		in->get_bytes_per_line(), ww);
	int sz = ww * hh;  uint8_t *yuv = out.get_data(), aimg[sz];
	uint8_t *yp = yuv, *up = yuv+sz, *vp = yuv+2*sz, *ap = 0;
	if( alpha > 0 ) memset(ap=aimg, alpha, sz);
	else if( alpha < 0 ) ap = yp;
	fd->display_subtitle(channel->video_stream, 1, 1,
				yp,up,vp,ap, x,y,ww,hh, 0, secs*1000);
	dvb_input->put_src();
}

void Record::
undisplay_vframe()
{
	if( !channel ) return;
	if( !vdevice || vdevice->in_config->driver != CAPTURE_DVB ) return;
	DeviceDVBInput *dvb_input = dvb_device();
	if( !dvb_input ) return 1;
	zmpeg3_t *fd = dvb_input->get_src();
	if( !fd ) return;
	fd->delete_subtitle(channel->video_stream, 1, 1);
	dvb_input->put_src();
}

#else

void Record::
display_vframe(VFrame *in, int x, int y, int alpha, double secs, double scale)
{
	record_monitor->display_vframe(in, x, y, alpha, secs, scale);
}

void Record::
undisplay_vframe()
{
	record_monitor->undisplay_vframe();
}

#endif

int Record::
display_channel_info()
{
#ifdef HAVE_DVB
	if( !channel ) return 1;
	if( !vdevice || vdevice->in_config->driver != CAPTURE_DVB ) return 1;
	DeviceDVBInput *dvb_input = dvb_device();
	if( !dvb_input ) return 1;
	int elem = channel->element;
	time_t tt;  time(&tt);
	struct tm ttm;  localtime_r(&tt,&ttm);
	int result = 1;
	char text[BCTEXTLEN];
	zmpeg3_t *fd = dvb_input->get_src();
	if( !fd ) return 1;
	for( int ord=0, i=0; ord<0x80; ) {
		char *cp = text;
		int len = fd->dvb.get_chan_info(elem,ord,i++,cp,BCTEXTLEN-2);
		if( len < 0 ) { ++ord;  i = 0;  continue; }
		struct tm stm = ttm, etm = ttm;
		char wday[4], *bp = cp;  cp += len;
		if( sscanf(bp, "%02d:%02d:%02d-%02d:%02d:%02d",
			&stm.tm_hour, &stm.tm_min, &stm.tm_sec,
			&etm.tm_hour, &etm.tm_min, &etm.tm_sec) != 6 ) continue;
		while( bp<cp && *bp++!='\n' );
		if( sscanf(bp, "(%3s) %04d/%02d/%02d", &wday[0],
			&stm.tm_year, &stm.tm_mon, &stm.tm_mday) != 4 ) continue;
		stm.tm_year -= 1900;  stm.tm_mon -= 1;
		etm.tm_year = stm.tm_year;
		etm.tm_mon = stm.tm_mon;
		etm.tm_mday = stm.tm_mday;
		time_t st = mktime(&stm), et = mktime(&etm);
		if( et < st ) et += 24*3600;
		if( tt < st || tt >= et ) continue;
		int major=0, minor=0;
		fd->dvb.get_channel(elem, major, minor);
		char chan[64];  sprintf(chan, "%3d.%1d", major, minor);
		for( i=0; i<5 && chan[i]!=0; ++i ) *bp++ = chan[i];
		while( bp<cp && *bp++!='\n' );
		int n = 1;
		for( char *lp=bp; bp<cp; ++bp ) {
			if( *bp == '\n' || ((bp-lp)>=60 && *bp==' ') ) {
				if( ++n >= 10 ) break;
				*(lp=bp) = '\n';
			}
		}
		*bp = 0;
		while( bp > text && *--bp == '\n' ) *bp = 0;
		result = 0;
		break;
	}
	dvb_input->put_src();
	if( !result )
		display_video_text(20, 20, text,
				BIGFONT, WHITE, BLACK, 0, 3., 1.);
	return result;
#else
	return 1;
#endif
}

int Record::
display_channel_schedule()
{
#ifdef HAVE_DVB
	if( !channel ) return 1;
	if( !vdevice || vdevice->in_config->driver != CAPTURE_DVB ) return 1;
	DeviceDVBInput *dvb_input = dvb_device();
	if( !dvb_input ) return 1;
	int elem = channel->element;
	time_t tt;  time(&tt);
	struct tm ttm;  localtime_r(&tt,&ttm);
	char text[BCTEXTLEN];
	zmpeg3_t *fd = dvb_input->get_src();
	if( !fd ) return 1;
	RecordSchedule channel_schedule;
	for( int ord=0, i=0; ord<0x80; ) {
		char *cp = text;
		int len = fd->dvb.get_chan_info(elem,ord,i++,cp,BCTEXTLEN-2);
		if( len < 0 ) { ++ord;  i = 0;  continue; }
		struct tm stm = ttm, etm = ttm;
		char wday[4], *bp = cp;  cp += len;
		if( sscanf(bp, "%02d:%02d:%02d-%02d:%02d:%02d",
			&stm.tm_hour, &stm.tm_min, &stm.tm_sec,
			&etm.tm_hour, &etm.tm_min, &etm.tm_sec) != 6 ) continue;
		while( bp<cp && *bp++!='\n' );
		if( sscanf(bp, "(%3s) %04d/%02d/%02d", &wday[0],
			&stm.tm_year, &stm.tm_mon, &stm.tm_mday) != 4 ) continue;
		stm.tm_year -= 1900;  stm.tm_mon -= 1;
		etm.tm_year = stm.tm_year;
		etm.tm_mon = stm.tm_mon;
		etm.tm_mday = stm.tm_mday;
		time_t st = mktime(&stm), et = mktime(&etm);
		if( et < st ) et += 24*3600;
		if( tt >= et ) continue;
		if( bp > text )*--bp = 0;
		channel_schedule.append(new RecordScheduleItem(st, text));
	}
	dvb_input->put_src();

	channel_schedule.sort_times();
	char *cp = text, *ep = cp+sizeof(text)-1;
	for( int i=0, n=0; i<channel_schedule.size(); ++i ) {
		RecordScheduleItem *item = channel_schedule.get(i);
		for( char *bp=item->title; cp<ep && *bp!=0; ++bp ) *cp++ = *bp;
		if( cp < ep ) *cp++ = '\n';
		if( ++n >= 12 ) break;
	}
	*cp = 0;
	while( cp > text && *--cp == '\n' ) *cp = 0;

	display_video_text(20, 20, text,
				BIGFONT, WHITE, BLACK, 0, 3., 1.);
	return 0;
#else
	return 1;
#endif
}

void Record::clear_keybfr()
{
	keybfr[0] = 0;
	undisplay_vframe();
}

void Record::add_key(int ch)
{
	int n = 0;
	while( n<(int)sizeof(keybfr)-2 && keybfr[n] ) ++n;
	keybfr[n++] = ch;  keybfr[n] = 0;
	display_video_text(20, 20, keybfr,
				BIGFONT, WHITE, BLACK, 0, 3., 2.);
}

int RecordRemoteHandler::remote_process_key(RemoteControl *remote_control, int key)
{
	Record *record = remote_control->mwindow_gui->record;
	return record->remote_process_key(remote_control, key);
}

int Record::remote_process_key(RemoteControl *remote_control, int key)
{
	int ch = key;

	switch( key ) {
	case KPENTER:
		if( last_key == KPENTER ) {
			set_channel_name(keybfr);
			clear_keybfr();
			break;
		}
		ch = '.';  // fall through
	case '0': if( last_key == '0' && ch == '0' ) {
		clear_keybfr();
		break;
	}
	case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		add_key(ch);
		break;
	//case UP: case DOWN: case LEFT: case RIGHT:
	//case KPPLAY: case KPBACK: case KPFORW:
#ifdef HAVE_COMMERCIAL
	case KPRECD:  case 'c': // start capture, mark endpoint
		if( !deletions ) {
			start_commercial_capture();
			if( adevice )
				set_play_gain(0.075);
		}
		else {
			mark_commercial_capture(DEL_MARK);
			blink_status->update();
		}
		display_cut_icon(10,20);
		break;
	case KPSTOP:  case 'd': // end capture, start cutting
		remote_control->set_color(YELLOW);
		stop_commercial_capture(1);
		break;
	case KPAUSE:  case 'x': // ignore current commercial
		mark_commercial_capture(DEL_SKIP);
		break;
#endif
	case KPBACK:  case 'a': // toggle mute audio
		if( !monitor_audio ) { set_mute_gain(1);  set_play_gain(1); }
		set_audio_monitoring(monitor_audio ? 0 : 1);
		break;
	case 'm': // toggle metering audio
		set_audio_metering(metering_audio ? 0 : 1);
		break;
#ifdef HAVE_COMMERCIAL
	case KPPLAY:  case 's': // ignore previous endpoint
		mark_commercial_capture(DEL_OOPS);
		break;
#endif
	case KPFWRD:  case KPSLASH:
		display_channel_info();
		break;
	case KPMAXW:  case KPSTAR:
		display_channel_schedule();
		break;
	case KPCHUP:  case KPPLUS:
		channel_up();
		break;
	case KPCHDN:  case KPMINUS:
		channel_down();
		break;
	case 'f': {
		Canvas *canvas = record_monitor->window->canvas;
		if( !canvas->get_fullscreen() )
			canvas->start_fullscreen();
		else
			canvas->stop_fullscreen();
		break; }
	default:
		return -1;
	}

	last_key = key;
	return 1;
}

#ifdef HAVE_COMMERCIAL
int Record::start_commercial_capture()
{
	if( deletions != 0 ) return 1;
	Channel *channel = get_current_channel();
	if( !channel ) return 1;
	int track = channel->video_stream;
	int pid = vdevice->get_video_pid(track);
	if( pid < 0 ) return 1;
	time_t st;  time(&st);
	struct tm stm;  localtime_r(&st, &stm);
	char file_path[BCTEXTLEN];
	sprintf(file_path,"/tmp/del%04d%02d%02d-%02d%02d%02d.ts",
		stm.tm_year+1900, stm.tm_mon+1, stm.tm_mday,
		stm.tm_hour, stm.tm_min, stm.tm_sec);
	commercial_fd = open(file_path,O_RDWR+O_CREAT,0666);
	if( commercial_fd < 0 ) return 1;
	if( vdevice->start_record(commercial_fd, 0x800000) ) {
		close(commercial_fd);
		commercial_fd = -1;
		return 1;
	}
	commercial_start_time = st;
	printf("del to %s\n", file_path);
	deletions = new Deletions(pid, file_path);
	mark_commercial_capture(DEL_START);
	blink_status->start();
	return 0;
}

int Record::mark_commercial_capture(int action)
{
	if( deletions == 0 ) return 1;
	double time = vdevice->get_timestamp();
	printf("dele %f\n", time);
	deletions->append(new Dele(time, action));
	return 0;
}
#endif

void Record::remote_fill_color(int color)
{
	mwindow->gui->remote_control->fill_color(color);
}

void Record::set_status_color(int color)
{
	status_color = color;
	remote_fill_color(status_color);
}

void Record::set_mute_gain(double gain)
{
	gain = (mute_gain = gain) * play_gain;
	if( adevice ) adevice->set_play_gain(gain);
}

void Record::set_play_gain(double gain)
{
	gain = mute_gain * (play_gain = gain);
	if( adevice ) adevice->set_play_gain(gain);
}

#ifdef HAVE_COMMERCIAL
int Record::stop_commercial_capture(int run_job)
{
	if( deletions == 0 ) return 1;
	if( run_job ) blink_status->stop();
	set_play_gain(1);

	vdevice->stop_record();
	commercial_start_time = -1;
	if( commercial_fd >= 0 ) {
		close(commercial_fd);
		commercial_fd = -1;
	}
	int result = -1;
	if( run_job ) {
		char del_filename[BCTEXTLEN];
		strcpy(del_filename, deletions->file_path());
		strcpy(strrchr(del_filename, '.'),".del");
		if( !deletions->write_dels(del_filename) ) {
			int pid = spawn("cutads %s",del_filename);
			if( pid > 0 ) {
				cut_pids.append(pid);
				cutads_status->start_waiting();
				result = 0;
			}
		}
	}
	else {
		remove_file(deletions->filepath);
	}
	delete deletions;  deletions = 0;
	return result;
}

int Record::
spawn(const char *fmt, ...)
{
	const char *exec_path = File::get_cinlib_path();
	char cmd[BCTEXTLEN], *cp = cmd, *ep = cp+sizeof(cmd)-1;
	va_list ap;  va_start(ap, fmt);
	cp += snprintf(cp, ep-cp, "exec %s/", exec_path);
	cp += vsnprintf(cp, ep-cp, fmt, ap);  va_end(ap);
	*cp = 0;
	pid_t pid = vfork();
	if( pid < 0 ) return -1;
	if( pid > 0 ) return pid;
	char *const argv[4] = { (char*) "/bin/sh", (char*) "-c", cmd, 0 };
	execvp(argv[0], &argv[0]);
	return -1;
}

int Record::
commercial_jobs()
{
	for( int i=0; i<cut_pids.size(); ) {
		int status, pid = cut_pids.get(i);
		if( waitpid(pid, &status, WNOHANG) ) {
			cut_pids.remove(pid);
			continue;
		}
 		++i;
	}
	return cut_pids.size();
}


RecordCutAdsStatus::
RecordCutAdsStatus(Record *record)
 : Thread(1, 0, 0)
{
	this->record = record;
	wait_lock = new Condition(0,"RecordCutAdsStatus::wait_lock",0);
	done = 0;
	start();
}

RecordCutAdsStatus::
~RecordCutAdsStatus()
{
	if( running() ) {
		done = 1;
		wait_lock->unlock();
		cancel();
		join();
	}
	delete wait_lock;
}

void RecordCutAdsStatus::
start_waiting()
{
	wait_lock->unlock();
}

void RecordCutAdsStatus::
run()
{
	int status, pgrp = getpgrp();
	while(!done) {
		wait_lock->lock("RecordCutAdsStatus::run");
		if( done ) break;
		if( !record->commercial_jobs() ) continue;
		record->set_status_color(YELLOW);
		while( !done ) {
			enable_cancel();
			waitpid(-pgrp, &status, 0);
			disable_cancel();
	 		if( !record->commercial_jobs() ) break;
		}
		record->set_status_color(GREEN);
	}
}

void RecordBlinkStatus::
remote_color(int color)
{
	record->remote_fill_color(color);
}

void RecordBlinkStatus::
update()
{
	timer.update();
}

void RecordBlinkStatus::
start()
{
	done = 0;
	update();
	Thread::start();
}

void RecordBlinkStatus::
stop()
{
	done = 1;
	if( running() ) {
		cancel();
		join();
	}
}

RecordBlinkStatus::
RecordBlinkStatus(Record *record)
 : Thread(1, 0, 0)
{
	this->record = record;
}

RecordBlinkStatus::
~RecordBlinkStatus()
{
	stop();
}

void RecordBlinkStatus::
run()
{
	int color = YELLOW;
	while( !done ) {
		remote_color(color);
		enable_cancel();
		usleep(500000);
		disable_cancel();
		color ^= YELLOW ^ BLUE;
		if( timer.get_difference() > 10*60*1000 ) { // 10 minutes
			record->stop_commercial_capture(0);
			done = 1;
		}
	}
	remote_color(record->status_color);
}

#endif
