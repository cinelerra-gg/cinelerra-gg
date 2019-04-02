
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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

#include "amodule.h"
#include "arender.h"
#include "asset.h"
#include "audiodevice.h"
#include "bcsignals.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "meterhistory.h"
#include "mutex.h"
#include "mwindow.h"
#include "playbackengine.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "renderengine.h"
#include "mainsession.h"
#include "tracks.h"
#include "transportque.h"
#include "videodevice.h"
#include "vrender.h"
#include "workarounds.h"



RenderEngine::RenderEngine(PlaybackEngine *playback_engine,
 	Preferences *preferences,
	Canvas *output,
	int is_nested)
 : Thread(1, 0, 0)
{
	this->playback_engine = playback_engine;
	this->output = output;
	this->is_nested = is_nested;
	audio = 0;
	video = 0;
	config = new PlaybackConfig;
	arender = 0;
	vrender = 0;
	do_audio = 0;
	do_video = 0;
	interrupted = 0;
 	this->preferences = new Preferences;
 	this->command = new TransportCommand;
 	this->preferences->copy_from(preferences);
	edl = 0;

	audio_cache = 0;
	video_cache = 0;
	mwindow = !playback_engine ? 0 : playback_engine->mwindow;

	input_lock = new Condition(1, "RenderEngine::input_lock");
	start_lock = new Condition(1, "RenderEngine::start_lock");
	output_lock = new Condition(1, "RenderEngine::output_lock");
	render_active = new Condition(1,"RenderEngine::render_active");
	interrupt_lock = new Condition(1, "RenderEngine::interrupt_lock");
	first_frame_lock = new Condition(1, "RenderEngine::first_frame_lock");
}

RenderEngine::~RenderEngine()
{
	close_output();
	delete command;
	delete preferences;
	if(arender) delete arender;
	if(vrender) delete vrender;
	delete input_lock;
	delete start_lock;
	delete output_lock;
	delete render_active;
	delete interrupt_lock;
	delete first_frame_lock;
	delete config;
	if( edl ) edl->Garbage::remove_user();
}

EDL* RenderEngine::get_edl()
{
//	return command->get_edl();
	return edl;
}

int RenderEngine::arm_command(TransportCommand *command)
{
	const int debug = 0;
// Prevent this renderengine from accepting another command until finished.
// Since the renderengine is often deleted after the input_lock command it must
// be locked here as well as in the calling routine.
	if(debug) printf("RenderEngine::arm_command %d\n", __LINE__);


	input_lock->lock("RenderEngine::arm_command");

	if(!edl)
	{
		edl = new EDL;
		edl->create_objects();
		edl->copy_all(command->get_edl());
	}
	this->command->copy_from(command);

// Fix background rendering asset to use current dimensions and ignore
// headers.
	preferences->brender_asset->frame_rate = command->get_edl()->session->frame_rate;
	preferences->brender_asset->width = command->get_edl()->session->output_w;
	preferences->brender_asset->height = command->get_edl()->session->output_h;
	preferences->brender_asset->use_header = 0;
	preferences->brender_asset->layers = 1;
	preferences->brender_asset->video_data = 1;

	done = 0;
	interrupted = 0;

// Retool configuration for this node
	this->config->copy_from(command->get_edl()->session->playback_config);
	VideoOutConfig *vconfig = this->config->vconfig;
	AudioOutConfig *aconfig = this->config->aconfig;
	if(command->realtime)
	{
		if(command->single_frame() && vconfig->driver != PLAYBACK_X11_GL)
		{
			vconfig->driver = PLAYBACK_X11;
		}
	}
	else
	{
		vconfig->driver = PLAYBACK_X11;
	}



	get_duty();

	if(do_audio)
	{
		fragment_len = aconfig->fragment_size;
// Larger of audio_module_fragment and fragment length adjusted for speed
// Extra memory must be allocated for rendering slow motion.
		float speed = command->get_speed();
		adjusted_fragment_len = speed >= 1.0 ?
			(int64_t)(aconfig->fragment_size * speed + 0.5) :
			(int64_t)(aconfig->fragment_size / speed + 0.5) ;
		if(adjusted_fragment_len < aconfig->fragment_size)
			adjusted_fragment_len = aconfig->fragment_size;
	}

// Set lock so audio doesn't start until video has started.
	if(do_video)
	{
		while(first_frame_lock->get_value() > 0)
			first_frame_lock->lock("RenderEngine::arm_command");
	}
	else
// Set lock so audio doesn't wait for video which is never to come.
	{
		while(first_frame_lock->get_value() <= 0)
			first_frame_lock->unlock();
	}

	open_output();
	create_render_threads();
	arm_render_threads();
	if(debug) printf("RenderEngine::arm_command %d\n", __LINE__);

	return 0;
}

void RenderEngine::get_duty()
{
	do_audio = 0;
	do_video = 0;

//printf("RenderEngine::get_duty %d\n", __LINE__);
	if( get_edl()->tracks->playable_audio_tracks() &&
	    get_edl()->session->audio_channels )
	{
		do_audio = !command->single_frame() ? 1 : 0;
		if( command->toggle_audio ) do_audio = !do_audio;
	}

//printf("RenderEngine::get_duty %d\n", __LINE__);
	if(get_edl()->tracks->playable_video_tracks())
	{
//printf("RenderEngine::get_duty %d\n", __LINE__);
		do_video = 1;
	}
}

void RenderEngine::create_render_threads()
{
	if(do_video && !vrender)
	{
		vrender = new VRender(this);
	}

	if(do_audio && !arender)
	{
		arender = new ARender(this);
	}
}


int RenderEngine::get_output_w()
{
	return get_edl()->session->output_w;
}

int RenderEngine::get_output_h()
{
	return get_edl()->session->output_h;
}

int RenderEngine::brender_available(int position, int direction)
{
	if(playback_engine)
	{
		int64_t corrected_position = position;
		if(direction == PLAY_REVERSE)
			corrected_position--;
		return playback_engine->brender_available(corrected_position);
	}
	else
		return 0;
}


CICache* RenderEngine::get_acache()
{
	if(playback_engine)
		return playback_engine->audio_cache;
	else
		return audio_cache;
}

CICache* RenderEngine::get_vcache()
{
	if(playback_engine)
		return playback_engine->video_cache;
	else
		return video_cache;
}

void RenderEngine::set_acache(CICache *cache)
{
	this->audio_cache = cache;
}

void RenderEngine::set_vcache(CICache *cache)
{
	this->video_cache = cache;
}


double RenderEngine::get_tracking_position()
{
	if(playback_engine)
		return playback_engine->get_tracking_position();
	else
		return 0;
}

int RenderEngine::open_output()
{
	if(command->realtime && !is_nested)
	{
// Allocate devices
		if(do_audio)
		{
			audio = new AudioDevice(mwindow);
		}

		if(do_video)
		{
			video = new VideoDevice(mwindow);
		}

// Initialize sharing


// Start playback
		if(do_audio && do_video)
		{
			video->set_adevice(audio);
			audio->set_vdevice(video);
		}



// Retool playback configuration
		if(do_audio)
		{
			if(audio->open_output(config->aconfig,
				get_edl()->session->sample_rate,
				adjusted_fragment_len,
				get_edl()->session->audio_channels,
				get_edl()->session->real_time_playback))
				do_audio = 0;
			else
			{
				audio->set_software_positioning(
					get_edl()->session->playback_software_position);
				audio->start_playback();
			}
		}

		if(do_video)
		{
			video->open_output(config->vconfig,
				get_edl()->session->frame_rate,
				get_output_w(),
				get_output_h(),
				output,
				command->single_frame());
			video->set_quality(80);
			video->set_cpus(preferences->processors);
		}
	}

	return 0;
}

void RenderEngine::reset_sync_position()
{
	timer.update();
}

int64_t RenderEngine::sync_position()
{
// Use audio device
// No danger of race conditions because the output devices are closed after all
// threads join.
	if( do_audio )
		return audio->current_position();
	if( do_video ) {
		int64_t result = timer.get_scaled_difference(
			get_edl()->session->sample_rate);
		return result;
	}
	return 0;
}


int RenderEngine::start_command()
{
	if( command->realtime && !is_nested ) {
		interrupt_lock->lock("RenderEngine::start_command");
		start_lock->lock("RenderEngine::start_command 1");
		Thread::start();
		start_lock->lock("RenderEngine::start_command 2");
		start_lock->unlock();
	}
	return 0;
}

void RenderEngine::arm_render_threads()
{
	if( do_audio ) arender->arm_command();
	if( do_video ) vrender->arm_command();
}


void RenderEngine::start_render_threads()
{
// Synchronization timer.  Gets reset once again after the first video frame.
	timer.update();

	if( do_audio ) arender->start_command();
	if( do_video ) vrender->start_command();

	start_lock->unlock();
}

void RenderEngine::update_framerate(float framerate)
{
	playback_engine->mwindow->session->actual_frame_rate = framerate;
	playback_engine->mwindow->preferences_thread->update_framerate();
}

void RenderEngine::wait_render_threads()
{
	interrupt_lock->unlock();
	if( do_audio ) arender->Thread::join();
	if( do_video ) vrender->Thread::join();
	interrupt_lock->lock("RenderEngine::wait_render_threads");
}

void RenderEngine::interrupt_playback()
{
	if( interrupted ) return;
	interrupt_lock->lock("RenderEngine::interrupt_playback");
	if( !interrupted ) {
		interrupted = 1;
		if( do_audio && arender ) arender->interrupt_playback();
		if( do_video && vrender ) vrender->interrupt_playback();
	}
	interrupt_lock->unlock();
}

int RenderEngine::close_output()
{
// Nested engines share devices
	if( is_nested ) return 0;
	if( audio ) {
		audio->close_all();
		delete audio;  audio = 0;
	}
	if( video ) {
		video->close_all();
		delete video;  video = 0;
	}
	return 0;
}

void RenderEngine::get_output_levels(double *levels, int64_t position)
{
	if( do_audio ) {
		MeterHistory *meter_history = arender->meter_history;
		int64_t tolerance = 4*arender->meter_render_fragment;
		int pos = meter_history->get_nearest(position, tolerance);
		for( int i=0; i<MAXCHANNELS; ++i ) {
			if( !arender->audio_out[i] ) continue;
			levels[i] = meter_history->get_peak(i, pos);
		}
	}
}

void RenderEngine::get_module_levels(ArrayList<double> *module_levels, int64_t position)
{
	if( do_audio ) {
		int64_t tolerance = 4*arender->meter_render_fragment;
		for( int i=0; i<arender->total_modules; ++i ) {
			AModule *amodule = (AModule *)arender->modules[i];
			MeterHistory *meter_history = amodule->meter_history;
			int pos = meter_history->get_nearest(position, tolerance);
			module_levels->append(meter_history->get_peak(0, pos));
		}
	}
}


void RenderEngine::run()
{
	render_active->lock("RenderEngine::run");

	start_render_threads();
	wait_render_threads();

	close_output();

	if( playback_engine ) {
		double position = command->command == CURRENT_FRAME ? command->playbackstart :
			playback_engine->is_playing_back && !interrupted ?
				( command->get_direction() == PLAY_FORWARD ?
					command->end_position : command->start_position ) :
				playback_engine->get_tracking_position() ;
		if( command->displacement ) {
			position -= 1./command->get_edl()->session->frame_rate;
			if( position < 0 ) position = 0;
		}
		playback_engine->command->command = STOP;
		playback_engine->is_playing_back = 0;
		playback_engine->tracking_position = position;
		playback_engine->stop_tracking();
	}

	render_active->unlock();
	interrupt_lock->unlock();
	input_lock->unlock();
}

void RenderEngine::wait_done()
{
	render_active->lock("RenderEngine::wait_done");
	render_active->unlock();
}


