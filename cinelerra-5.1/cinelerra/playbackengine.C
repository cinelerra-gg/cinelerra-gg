
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

#include "bchash.h"
#include "bcsignals.h"
#include "cache.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mbuttons.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "tracking.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "renderengine.h"
#include "mainsession.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "vrender.h"


PlaybackEngine::PlaybackEngine(MWindow *mwindow, Canvas *output)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->output = output;
	is_playing_back = 0;
	tracking_position = 0;
	tracking_active = 0;
	audio_cache = 0;
	video_cache = 0;
	last_command = STOP;
	tracking_lock = new Mutex("PlaybackEngine::tracking_lock");
	renderengine_lock = new Mutex("PlaybackEngine::renderengine_lock");
	tracking_done = new Condition(1, "PlaybackEngine::tracking_done");
	pause_lock = new Condition(0, "PlaybackEngine::pause_lock");
	start_lock = new Condition(0, "PlaybackEngine::start_lock");
	render_engine = 0;
	debug = 0;
}

PlaybackEngine::~PlaybackEngine()
{
	done = 1;
	que->send_command(STOP,
		CHANGE_NONE,
		0,
		0);
	interrupt_playback();

	Thread::join();
	delete preferences;
	delete command;
	delete que;
	delete_render_engine();
	delete audio_cache;
	delete video_cache;
	delete tracking_lock;
	delete tracking_done;
	delete pause_lock;
	delete start_lock;
	delete renderengine_lock;
}

void PlaybackEngine::create_objects()
{
	preferences = new Preferences;
	command = new TransportCommand;
	que = new TransportQue;
// Set the first change to maximum
	que->command.change_type = CHANGE_ALL;

	preferences->copy_from(mwindow->preferences);

	done = 0;
	Thread::start();
	start_lock->lock("PlaybackEngine::create_objects");
}

ChannelDB* PlaybackEngine::get_channeldb()
{
	PlaybackConfig *config = command->get_edl()->session->playback_config;
	switch(config->vconfig->driver)
	{
		case VIDEO4LINUX2JPEG:
			return mwindow->channeldb_v4l2jpeg;
	}
	return 0;
}

int PlaybackEngine::create_render_engine()
{
// Fix playback configurations
	delete_render_engine();
	render_engine = new RenderEngine(this, preferences, output, 0);
//printf("PlaybackEngine::create_render_engine %d\n", __LINE__);
	return 0;
}

void PlaybackEngine::delete_render_engine()
{
	renderengine_lock->lock("PlaybackEngine::delete_render_engine");
	delete render_engine;
	render_engine = 0;
	renderengine_lock->unlock();
}

void PlaybackEngine::arm_render_engine()
{
	if(render_engine)
		render_engine->arm_command(command);
}

void PlaybackEngine::start_render_engine()
{
	if(render_engine) render_engine->start_command();
}

void PlaybackEngine::wait_render_engine()
{
	if(command->realtime && render_engine)
	{
		render_engine->join();
	}
}

void PlaybackEngine::create_cache()
{
	if(audio_cache) { delete audio_cache;  audio_cache = 0; }
	if(video_cache) { delete video_cache;  video_cache = 0; }
	if(!audio_cache) audio_cache = new CICache(preferences);
	if(!video_cache) video_cache = new CICache(preferences);
}


void PlaybackEngine::perform_change()
{
	switch(command->change_type)
	{
		case CHANGE_ALL:
			create_cache();
		case CHANGE_EDL:
			create_render_engine();
		case CHANGE_PARAMS:
			if(command->change_type != CHANGE_EDL &&
				(uint32_t)command->change_type != CHANGE_ALL)
				render_engine->get_edl()->synchronize_params(command->get_edl());
		case CHANGE_NONE:
			break;
	}
}

void PlaybackEngine::sync_parameters(EDL *edl)
{
// TODO: lock out render engine from keyframe deletions
	command->get_edl()->synchronize_params(edl);
	if(render_engine) render_engine->get_edl()->synchronize_params(edl);
}


void PlaybackEngine::interrupt_playback(int wait_tracking)
{
	renderengine_lock->lock("PlaybackEngine::interrupt_playback");
	if(render_engine)
		render_engine->interrupt_playback();
	renderengine_lock->unlock();

// Stop pausing
	pause_lock->unlock();

// Wait for tracking to finish if it is running
	if(wait_tracking)
	{
		tracking_done->lock("PlaybackEngine::interrupt_playback");
		tracking_done->unlock();
	}
}


// Return 1 if levels exist
int PlaybackEngine::get_output_levels(double *levels, long position)
{
	int result = 0;
	if(render_engine && render_engine->do_audio)
	{
		result = 1;
		render_engine->get_output_levels(levels, position);
	}
	return result;
}


int PlaybackEngine::get_module_levels(ArrayList<double> *module_levels, long position)
{
	int result = 0;
	if(render_engine && render_engine->do_audio)
	{
		result = 1;
		render_engine->get_module_levels(module_levels, position);
	}
	return result;
}

int PlaybackEngine::brender_available(long position)
{
	return 0;
}

void PlaybackEngine::init_cursor(int active)
{
}

void PlaybackEngine::init_meters()
{
}

void PlaybackEngine::stop_cursor()
{
}


void PlaybackEngine::init_tracking()
{
	tracking_active = !command->single_frame() ? 1 : 0;
	tracking_position = command->playbackstart;
	tracking_done->lock("PlaybackEngine::init_tracking");
	init_cursor(tracking_active);
	init_meters();
}

void PlaybackEngine::stop_tracking()
{
	tracking_active = 0;
	stop_cursor();
	tracking_done->unlock();
}

void PlaybackEngine::update_tracking(double position)
{
	tracking_lock->lock("PlaybackEngine::update_tracking");
	tracking_position = position;
// Signal that the timer is accurate.
	if(tracking_active) tracking_active = 2;
	tracking_timer.update();
	tracking_lock->unlock();
}

double PlaybackEngine::get_tracking_position()
{
	double result = 0;

	tracking_lock->lock("PlaybackEngine::get_tracking_position");


// Adjust for elapsed time since last update_tracking.
// But tracking timer isn't accurate until the first update_tracking
// so wait.
	if(tracking_active == 2)
	{
//printf("PlaybackEngine::get_tracking_position %d %d %d\n", command->get_direction(), tracking_position, tracking_timer.get_scaled_difference(command->get_edl()->session->sample_rate));


// Don't interpolate when every frame is played.
		if(command->get_edl()->session->video_every_frame &&
			render_engine &&
			render_engine->do_video)
		{
			result = tracking_position;
		}
		else
// Interpolate
		{
			double loop_start, loop_end;
			int play_loop = command->play_loop ? 1 : 0;
			EDL *edl = command->get_edl();
			int loop_playback = edl->local_session->loop_playback ? 1 : 0;
			if( play_loop || !loop_playback ) {
				loop_start = command->start_position;
				loop_end = command->end_position;
			}
			else {
				loop_start = edl->local_session->loop_start;
				loop_end = edl->local_session->loop_end;
				play_loop = 1;
			}
			double loop_size = loop_end - loop_start;

			if( command->get_direction() == PLAY_FORWARD ) {
// Interpolate
				result = tracking_position +
					command->get_speed() *
					tracking_timer.get_difference() /
					1000.0;

// Compensate for loop
//printf("PlaybackEngine::get_tracking_position 1 %d\n", command->get_edl()->local_session->loop_playback);
				if( play_loop && loop_size > 0 ) {
					while( result > loop_end ) result -= loop_size;
				}
			}
			else {
// Interpolate
				result = tracking_position -
					command->get_speed() *
					tracking_timer.get_difference() /
					1000.0;

// Compensate for loop
				if( play_loop && loop_size > 0 ) {
					while( result < loop_start ) result += loop_size;
				}
			}

		}
	}
	else
		result = tracking_position;

	tracking_lock->unlock();
//printf("PlaybackEngine::get_tracking_position %f %f %d\n", result, tracking_position, tracking_active);

// Adjust for loop

	return result;
}

void PlaybackEngine::update_transport(int command, int paused)
{
//	mwindow->gui->lock_window();
//	mwindow->gui->mbuttons->transport->update_gui_state(command, paused);
//	mwindow->gui->unlock_window();
}

void PlaybackEngine::run()
{
	start_lock->unlock();

	while( !done ) {
// Wait for current command to finish
		que->output_lock->lock("PlaybackEngine::run");
		wait_render_engine();

// Read the new command
		que->input_lock->lock("PlaybackEngine::run");
		if(done) return;

		command->copy_from(&que->command);
		que->command.reset();
		que->input_lock->unlock();
//printf("PlaybackEngine::run 1 %d\n", command->command);

		switch( command->command ) {
// Parameter change only
		case COMMAND_NONE:
//			command->command = last_command;
			perform_change();
			break;

		case PAUSE:
			init_cursor(0);
			pause_lock->lock("PlaybackEngine::run");
			stop_cursor();
			break;

		case STOP:
// No changing
			break;

		case CURRENT_FRAME:
		case LAST_FRAME:
			last_command = command->command;
			perform_change();
			arm_render_engine();
// Dispatch the command
			start_render_engine();
			break;

		case SINGLE_FRAME_FWD:
		case SINGLE_FRAME_REWIND:
// fall through
		default:
			last_command = command->command;
			is_playing_back = 1;

			perform_change();
			arm_render_engine();

// Start tracking after arming so the tracking position doesn't change.
// The tracking for a single frame command occurs during PAUSE
			init_tracking();

// Dispatch the command
			start_render_engine();
			break;
		}


//printf("PlaybackEngine::run 100\n");
	}
}


void PlaybackEngine::stop_playback(int wait)
{
	que->send_command(STOP, CHANGE_NONE, 0, 0);
	interrupt_playback(wait);
	renderengine_lock->lock("PlaybackEngine::stop_playback");
	if(render_engine)
		render_engine->wait_done();
	renderengine_lock->unlock();
}


void PlaybackEngine::issue_command(EDL *edl, int command, int wait_tracking,
		int use_inout, int update_refresh, int toggle_audio, int loop_play)
{
//printf("PlaybackEngine::issue_command 1 %d\n", command);
// Stop requires transferring the output buffer to a refresh buffer.
	int do_stop = 0, resume = 0;
	int prev_command = this->command->command;
	int prev_single_frame = this->command->single_frame();
	int prev_audio = this->command->audio_toggle ?
		 !prev_single_frame : prev_single_frame;
	int cur_single_frame = TransportCommand::single_frame(command);
	int cur_audio = toggle_audio ?
		 !cur_single_frame : cur_single_frame;

// Dispatch command
	switch(command) {
	case FAST_REWIND:	// Commands that play back
	case NORMAL_REWIND:
	case SLOW_REWIND:
	case SINGLE_FRAME_REWIND:
	case SINGLE_FRAME_FWD:
	case SLOW_FWD:
	case NORMAL_FWD:
	case FAST_FWD:
	case CURRENT_FRAME:
	case LAST_FRAME:
		if( !prev_single_frame &&
		    prev_command == command &&
		    cur_audio == prev_audio ) {
// Same direction pressed twice and no change in audio state,  Stop
			do_stop = 1;
			break;
		}
// Resume or change direction
		switch( prev_command ) {
		default:
			que->send_command(STOP, CHANGE_NONE, 0, 0);
			interrupt_playback(wait_tracking);
			resume = 1;
// fall through
		case STOP:
		case COMMAND_NONE:
		case SINGLE_FRAME_FWD:
		case SINGLE_FRAME_REWIND:
		case CURRENT_FRAME:
		case LAST_FRAME:
// Start from scratch
			que->send_command(command, CHANGE_NONE, edl,
				1, resume, use_inout, toggle_audio, loop_play,
				mwindow->preferences->forward_render_displacement);
			break;
		}
		break;

// Commands that stop
	case STOP:
	case REWIND:
	case GOTO_END:
		do_stop = 1;
		break;
	}

	if( do_stop ) {
		que->send_command(STOP, CHANGE_NONE, 0, 0);
		interrupt_playback(wait_tracking);
	}
}

void PlaybackEngine::refresh_frame(int change_type, EDL *edl, int dir)
{
	que->send_command(dir >= 0 ? CURRENT_FRAME : LAST_FRAME,
		change_type, edl, 1);
}

