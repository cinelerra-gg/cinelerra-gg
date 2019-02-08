
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

#include "asset.h"
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "condition.h"
#include "datatype.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "overlayframe.h"
#include "playabletracks.h"
#include "playbackengine.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "renderengine.h"
#include "strategies.inc"
#include "tracks.h"
#include "transportque.h"
#include "units.h"
#include "vedit.h"
#include "vframe.h"
#include "videoconfig.h"
#include "videodevice.h"
#include "virtualconsole.h"
#include "virtualvconsole.h"
#include "vmodule.h"
#include "vrender.h"
#include "vtrack.h"





VRender::VRender(RenderEngine *renderengine)
 : CommonRender(renderengine)
{
	data_type = TRACK_VIDEO;
	transition_temp = 0;
	overlayer = new OverlayFrame(renderengine->preferences->project_smp);
	input_temp = 0;
	vmodule_render_fragment = 0;
	playback_buffer = 0;
	session_frame = 0;
	asynchronous = 0;     // render 1 frame at a time
	framerate_counter = 0;
	video_out = 0;
	render_strategy = -1;
}

VRender::~VRender()
{
	if(input_temp) delete input_temp;
	if(transition_temp) delete transition_temp;
	if(overlayer) delete overlayer;
}


VirtualConsole* VRender::new_vconsole_object()
{
	return new VirtualVConsole(renderengine, this);
}

int VRender::get_total_tracks()
{
	return renderengine->get_edl()->tracks->total_video_tracks();
}

Module* VRender::new_module(Track *track)
{
	return new VModule(renderengine, this, 0, track);
}

int VRender::flash_output()
{
	if(video_out)
		return renderengine->video->write_buffer(video_out, renderengine->get_edl());
	else
		return 0;
}

int VRender::process_buffer(VFrame *video_out,
	int64_t input_position,
	int use_opengl)
{
// process buffer for non realtime
	int64_t render_len = 1;
	int reconfigure = 0;


	this->video_out = video_out;

	current_position = input_position;

	reconfigure = vconsole->test_reconfigure(input_position,
		render_len);

	if(reconfigure) restart_playback();
	return process_buffer(input_position, use_opengl);
}


int VRender::process_buffer(int64_t input_position,
	int use_opengl)
{
	VEdit *playable_edit = 0;
	int colormodel;
	int use_vconsole = 1;
	int use_brender = 0;
	int result = 0;
	int use_cache = renderengine->command->single_frame();
// 	int use_asynchronous = 
// 		renderengine->command->realtime && 
// 		renderengine->get_edl()->session->video_every_frame &&
// 		renderengine->get_edl()->session->video_asynchronous;
	const int debug = 0;

// Determine the rendering strategy for this frame.
	use_vconsole = get_use_vconsole(&playable_edit, input_position, use_brender);
	if(debug) printf("VRender::process_buffer %d use_vconsole=%d\n", __LINE__, use_vconsole);

// Negotiate color model
	colormodel = get_colormodel(playable_edit, use_vconsole, use_brender);
	if(debug) printf("VRender::process_buffer %d\n", __LINE__);


// Get output buffer from device
	if(renderengine->command->realtime && !renderengine->is_nested)
	{
		renderengine->video->new_output_buffer(&video_out, 
			colormodel, 
			renderengine->get_edl());
	}

	if(debug) printf("VRender::process_buffer %d video_out=%p\n", __LINE__, video_out);

// printf("VRender::process_buffer use_vconsole=%d colormodel=%d video_out=%p\n",
// use_vconsole,
// colormodel,
// video_out);
// Read directly from file to video_out
	if(!use_vconsole)
	{

		if(use_brender)
		{
			Asset *asset = renderengine->preferences->brender_asset;
			File *file = renderengine->get_vcache()->check_out(asset,
				renderengine->get_edl());

			if(file)
			{
				int64_t corrected_position = current_position;
				if(renderengine->command->get_direction() == PLAY_REVERSE)
					corrected_position--;

// Cache single frames only
//				if(use_asynchronous)
//					file->start_video_decode_thread();
//				else
					file->stop_video_thread();
				if(use_cache) file->set_cache_frames(1);
				int64_t normalized_position = (int64_t)(corrected_position *
					asset->frame_rate /
					renderengine->get_edl()->session->frame_rate);

				file->set_video_position(normalized_position,
					0);
				file->read_frame(video_out);


				if(use_cache) file->set_cache_frames(0);
				renderengine->get_vcache()->check_in(asset);
			}

		}
		else
		if(playable_edit)
		{
			if(debug) printf("VRender::process_buffer %d\n", __LINE__);
			result = ((VEdit*)playable_edit)->read_frame(video_out,
				current_position,
				renderengine->command->get_direction(),
				renderengine->get_vcache(),
				1,
				use_cache,
				0);
//				use_asynchronous);
			if(debug) printf("VRender::process_buffer %d\n", __LINE__);
		}



		video_out->set_opengl_state(VFrame::RAM);
	}
	else
// Read into virtual console
	{

// process this buffer now in the virtual console
		result = ((VirtualVConsole*)vconsole)->process_buffer(input_position,
			use_opengl);
	}

	return result;
}

// Determine if virtual console is needed
int VRender::get_use_vconsole(VEdit **playable_edit,
	int64_t position, int &use_brender)
{
	*playable_edit = 0;

// Background rendering completed
	if((use_brender = renderengine->brender_available(position,
		renderengine->command->get_direction())) != 0)
		return 0;

// Descend into EDL nest
	return renderengine->get_edl()->get_use_vconsole(playable_edit,
		position, renderengine->command->get_direction(),
		vconsole->playable_tracks);
}


int VRender::get_colormodel(VEdit *playable_edit, int use_vconsole, int use_brender)
{
	EDL *edl = renderengine->get_edl();
	int colormodel = renderengine->get_edl()->session->color_model;
	VideoOutConfig *vconfig = renderengine->config->vconfig;
// check for playback: no plugins, not single frame
	if( !use_vconsole && !renderengine->command->single_frame() ) {
// Get best colormodel supported by the file
// colormodel yuv/rgb affects mpeg/jpeg color range,
// dont mix them or loose color acccuracy
		int64_t source_position = 0;
		Asset *asset = use_brender ?
			renderengine->preferences->brender_asset :
			playable_edit->get_nested_asset(&source_position, current_position,
				renderengine->command->get_direction());
		if( asset ) {
			File *file = renderengine->get_vcache()->check_out(asset, edl);
			if( file ) {
// damn the color range, full speed ahead
				if( vconfig->driver == PLAYBACK_X11 && vconfig->use_direct_x11 &&
				    file->colormodel_supported(BC_BGR8888) == BC_BGR8888 )
					colormodel = BC_BGR8888;
				else {
// file favorite colormodel may mismatch rgb/yuv
					int vstream = playable_edit ? playable_edit->channel : -1;
					int best_colormodel = file->get_best_colormodel(vconfig->driver, vstream);
					if( BC_CModels::is_yuv(best_colormodel) == BC_CModels::is_yuv(colormodel) )
						colormodel = best_colormodel;
				}
				renderengine->get_vcache()->check_in(asset);
			}
		}
	}

	return colormodel;
}


void VRender::run()
{
	int reconfigure;
	const int debug = 0;

// Want to know how many samples rendering each frame takes.
// Then use this number to predict the next frame that should be rendered.
// Be suspicious of frames that render late so have a countdown
// before we start dropping.
	int64_t current_sample, start_sample, end_sample; // Absolute counts.
	int64_t skip_countdown = VRENDER_THRESHOLD;    // frames remaining until drop
	int64_t delay_countdown = 0;  // Frames remaining until delay
// Number of frames before next reconfigure
	int64_t current_input_length;
// Number of frames to skip.
	int64_t frame_step = 1;
	int use_opengl = (renderengine->video &&
		renderengine->video->out_config->driver == PLAYBACK_X11_GL);

	first_frame = 1;

// Number of frames since start of rendering
	session_frame = 0;
	framerate_counter = 0;
	framerate_timer.update();

	start_lock->unlock();
	if(debug) printf("VRender::run %d\n", __LINE__);


	while(!done && !interrupt )
	{
// Perform the most time consuming part of frame decompression now.
// Want the condition before, since only 1 frame is rendered
// and the number of frames skipped after this frame varies.
		current_input_length = 1;

		reconfigure = vconsole->test_reconfigure(current_position,
			current_input_length);


		if(debug) printf("VRender::run %d\n", __LINE__);
		if(reconfigure) restart_playback();

		if(debug) printf("VRender::run %d\n", __LINE__);
		process_buffer(current_position, use_opengl);


		if(debug) printf("VRender::run %d\n", __LINE__);

		if(renderengine->command->single_frame())
		{
			if(debug) printf("VRender::run %d\n", __LINE__);
			flash_output();
			frame_step = 1;
			done = 1;
		}
		else
// Perform synchronization
		{
// Determine the delay until the frame needs to be shown.
			current_sample = (int64_t)(renderengine->sync_position() *
				renderengine->command->get_speed());
// latest sample at which the frame can be shown.
			end_sample = Units::tosamples(session_frame + 1,
				renderengine->get_edl()->session->sample_rate,
				renderengine->get_edl()->session->frame_rate);
// earliest sample by which the frame needs to be shown.
			start_sample = Units::tosamples(session_frame,
				renderengine->get_edl()->session->sample_rate,
				renderengine->get_edl()->session->frame_rate);

			if(first_frame || end_sample < current_sample)
			{
// Frame rendered late or this is the first frame.  Flash it now.
//printf("VRender::run %d\n", __LINE__);
				flash_output();

				if(renderengine->get_edl()->session->video_every_frame)
				{
// User wants every frame.
					frame_step = 1;
				}
				else
				if(skip_countdown > 0)
				{
// Maybe just a freak.
					frame_step = 1;
					skip_countdown--;
				}
				else
				{
// Get the frames to skip.
					delay_countdown = VRENDER_THRESHOLD;
					frame_step = 1;
					frame_step += (int64_t)Units::toframes(current_sample,
							renderengine->get_edl()->session->sample_rate,
							renderengine->get_edl()->session->frame_rate);
					frame_step -= (int64_t)Units::toframes(end_sample,
								renderengine->get_edl()->session->sample_rate,
								renderengine->get_edl()->session->frame_rate);
				}
			}
			else
			{
// Frame rendered early or just in time.
				frame_step = 1;

				if(delay_countdown > 0)
				{
// Maybe just a freak
					delay_countdown--;
				}
				else
				{
					skip_countdown = VRENDER_THRESHOLD;
					if(start_sample > current_sample)
					{
						int64_t delay_time = (int64_t)((float)(start_sample - current_sample) *
							1000 / renderengine->get_edl()->session->sample_rate);
						if( delay_time > 1000 ) delay_time = 1000;
						timer.delay(delay_time);
					}
					else
					{
// Came after the earliest sample so keep going
					}
				}

// Flash frame now.
//printf("VRender::run %d %jd\n", __LINE__, current_input_length);
				flash_output();
			}
		}
		if(debug) printf("VRender::run %d\n", __LINE__);

// Trigger audio to start
		if(first_frame)
		{
			renderengine->first_frame_lock->unlock();
			first_frame = 0;
			renderengine->reset_sync_position();
		}
		if(debug) printf("VRender::run %d\n", __LINE__);

		session_frame += frame_step;

// advance position in project
		current_input_length = frame_step;


// Subtract frame_step in a loop to allow looped playback to drain
// printf("VRender::run %d %d %d %d\n",
// __LINE__,
// done,
// frame_step,
// current_input_length);
		while(frame_step && current_input_length)
		{
// trim current_input_length to range
			get_boundaries(current_input_length);
// advance 1 frame
			advance_position(current_input_length);
			frame_step -= current_input_length;
			current_input_length = frame_step;
			if(done) break;
// printf("VRender::run %d %d %d %d\n",
// __LINE__,
// done,
// frame_step,
// current_input_length);
		}

		if(debug) printf("VRender::run %d current_position=%jd done=%d\n",
			__LINE__, current_position, done);

// Update tracking.
		if(renderengine->command->realtime && renderengine->playback_engine &&
			renderengine->command->command != CURRENT_FRAME &&
			renderengine->command->command != LAST_FRAME)
		{
			renderengine->playback_engine->update_tracking(fromunits(current_position));
		}
		if(debug) printf("VRender::run %d\n", __LINE__);

// Calculate the framerate counter
		framerate_counter++;
		if(framerate_counter >= renderengine->get_edl()->session->frame_rate &&
			renderengine->command->realtime)
		{
			renderengine->update_framerate((float)framerate_counter /
				((float)framerate_timer.get_difference() / 1000));
			framerate_counter = 0;
			framerate_timer.update();
		}
		if(debug) printf("VRender::run %d done=%d\n", __LINE__, done);
		if( !interrupt ) interrupt = renderengine->interrupted;
		if( !interrupt ) interrupt = renderengine->video->interrupt;
		if( !interrupt ) interrupt = vconsole->interrupt;
	}


// In case we were interrupted before the first loop
	renderengine->first_frame_lock->unlock();
	stop_plugins();
	if(debug) printf("VRender::run %d done=%d\n", __LINE__, done);
}

int VRender::start_playback()
{
// start reading input and sending to vrenderthread
// use a thread only if there's a video device
	if(renderengine->command->realtime)
	{
		start();
	}
	return 0;
}

int64_t VRender::tounits(double position, int round)
{
	if(round)
		return Units::round(position * renderengine->get_edl()->session->frame_rate);
	else
		return Units::to_int64(position * renderengine->get_edl()->session->frame_rate);
}

double VRender::fromunits(int64_t position)
{
	return (double)position / renderengine->get_edl()->session->frame_rate;
}

