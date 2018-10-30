
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

#include "aattachmentpoint.h"
#include "aedit.h"
#include "amodule.h"
#include "aplugin.h"
#include "arender.h"
#include "asset.h"
#include "atrack.h"
#include "automation.h"
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "floatautos.h"
#include "language.h"
#include "module.h"
#include "patch.h"
#include "plugin.h"
#include "pluginarray.h"
#include "preferences.h"
#include "renderengine.h"
#include "mainsession.h"
#include "samples.h"
#include "sharedlocation.h"
#include "theme.h"
#include "transition.h"
#include "transportque.h"
#include <string.h>








AModuleResample::AModuleResample(AModule *module)
 : Resample()
{
	this->module = module;
	bzero(nested_output, sizeof(Samples*) * MAX_CHANNELS);
	nested_allocation = 0;
}

AModuleResample::~AModuleResample()
{
	for(int i = 0; i < MAX_CHANNELS; i++)
		delete nested_output[i];
}

int AModuleResample::read_samples(Samples *buffer, int64_t start, int64_t len)
{
	int result = 0;


	if(module->asset)
	{
// Files only read going forward.
		if(get_direction() == PLAY_REVERSE)
		{
			start -= len;
		}

//printf("AModuleResample::read_samples start=%jd len=%jd\n", start, len);
		module->file->set_audio_position(start);
		module->file->set_channel(module->channel);
		result = module->file->read_samples(buffer, len);

// Reverse buffer so resampling filter renders forward.
		if(get_direction() == PLAY_REVERSE)
			Resample::reverse_buffer(buffer->get_data(), len);
	}
	else
	if(module->nested_edl)
	{


// Nested EDL generates reversed buffer.
		for(int i = 0; i < module->nested_edl->session->audio_channels; i++)
		{
			if(nested_allocation < len)
			{
				delete nested_output[i];
				nested_output[i] = 0;
			}

			if(!nested_output[i])
			{
				nested_output[i] = new Samples(len);
			}
		}


		result = module->nested_renderengine->arender->process_buffer(
			nested_output,
			len,
			start);
// printf("AModuleResample::read_samples buffer=%p module=%p len=%d\n",
// buffer,
// module,
// len);
		memcpy(buffer->get_data(),
			nested_output[module->channel]->get_data(),
			len * sizeof(double));

	}
	return result;
}











AModule::AModule(RenderEngine *renderengine,
	CommonRender *commonrender,
	PluginArray *plugin_array,
	Track *track)
 : Module(renderengine, commonrender, plugin_array, track)
{
	data_type = TRACK_AUDIO;
	transition_temp = 0;
	speed_temp = 0;
	bzero(nested_output, sizeof(Samples*) * MAX_CHANNELS);
	bzero(prev_head, SPEED_OVERLAP * sizeof(double));
	bzero(prev_tail, SPEED_OVERLAP * sizeof(double));
	meter_history = new MeterHistory();
	nested_allocation = 0;
	resample = 0;
	asset = 0;
	file = 0;
}




AModule::~AModule()
{
	delete transition_temp;
	delete speed_temp;
	delete meter_history;
	for(int i = 0; i < MAX_CHANNELS; i++)
		delete nested_output[i];
	delete resample;
}

AttachmentPoint* AModule::new_attachment(Plugin *plugin)
{
	return new AAttachmentPoint(renderengine, plugin);
}


void AModule::create_objects()
{
	Module::create_objects();
// Not needed in pluginarray
	if( commonrender ) {
		meter_history->init(1, ((ARender*)commonrender)->total_peaks);
		meter_history->reset_channel(0);
	}
}

int AModule::get_buffer_size()
{
	if(renderengine)
		return renderengine->fragment_len;
	else
		return plugin_array->get_bufsize();
}


CICache* AModule::get_cache()
{
	if(renderengine)
		return renderengine->get_acache();
	else
		return cache;
}


int AModule::import_samples(AEdit *edit,
	int64_t start_project,
	int64_t edit_startproject,
	int64_t edit_startsource,
	int direction,
	int sample_rate,
	Samples *buffer,
	int64_t fragment_len)
{
	int result = 0;
// start in EDL samplerate
	int64_t start_source = start_project -
		edit_startproject +
		edit_startsource;
// fragment size adjusted for speed curve
	int64_t speed_fragment_len = fragment_len;
// boundaries of input fragment required for speed curve
	double max_position = 0;
	double min_position = 0;

	double speed_position = edit_startsource;
// position in source where speed curve starts reading
	double speed_position1 = speed_position;
// position in source where speed curve finishes
	double speed_position2 = speed_position;

// Need speed curve processing
	int have_speed = 0;
// Temporary buffer for rendering speed curve
	Samples *speed_buffer = buffer;
	const int debug = 0;

if(debug) printf("AModule::import_samples %d edit=%p nested_edl=%p\n",
__LINE__,
edit,
nested_edl);
	if(nested_edl && edit->channel >= nested_edl->session->audio_channels)
		return 1;
if(debug) printf("AModule::import_samples %d\n", __LINE__);

	this->channel = edit->channel;
if(debug) printf("AModule::import_samples %d speed_fragment_len=%jd\n",
__LINE__,
speed_fragment_len);




// apply speed curve to source position so the timeline agrees with the playback
	if(track->has_speed())
	{
// get speed adjusted position from start of edit.
		FloatAuto *previous = 0;
		FloatAuto *next = 0;
		FloatAutos *speed_autos = (FloatAutos*)track->automation->autos[AUTOMATION_SPEED];
		speed_position += speed_autos->automation_integral(edit_startproject,
				start_project-edit_startproject, PLAY_FORWARD);
		speed_position1 = speed_position;


// calculate boundaries of input fragment required for speed curve
		max_position = speed_position;
		min_position = speed_position;
		for(int64_t i = start_project; i < start_project + fragment_len; i++)
		{
			double speed = speed_autos->get_value(i,
				PLAY_FORWARD,
				previous,
				next);
			speed_position += speed;
			if(speed_position > max_position) max_position = speed_position;
			if(speed_position < min_position) min_position = speed_position;
		}

		speed_position2 = speed_position;
		if(speed_position2 < speed_position1)
		{
			max_position += 1.0;
//			min_position -= 1.0;
			speed_fragment_len = (int64_t)(max_position - min_position);
		}
		else
		{
			max_position += 1.0;
			speed_fragment_len = (int64_t)(max_position - min_position);
		}

//printf("AModule::import_samples %d %f %f %f %f\n",
// __LINE__, min_position, max_position, speed_position1, speed_position2);

// new start of source to read from file
		start_source = (int64_t)min_position;
		have_speed = 1;



// swap in the temp buffer
		if(speed_temp && speed_temp->get_allocated() < speed_fragment_len)
		{
			delete speed_temp;
			speed_temp = 0;
		}

		if(!speed_temp)
		{
			speed_temp = new Samples(speed_fragment_len);
		}

		speed_buffer = speed_temp;
	}



	if(speed_fragment_len == 0)
		return 1;



// Source is a nested EDL
	if(edit->nested_edl)
	{
		int command;
		asset = 0;

		if(direction == PLAY_REVERSE)
			command = NORMAL_REWIND;
		else
			command = NORMAL_FWD;

if(debug) printf("AModule::import_samples %d\n", __LINE__);
		if(!nested_edl || nested_edl->id != edit->nested_edl->id)
		{
			nested_edl = edit->nested_edl;
			if(nested_renderengine)
			{
				delete nested_renderengine;
				nested_renderengine = 0;
			}

			if(!nested_command)
			{
				nested_command = new TransportCommand;
			}


			if(!nested_renderengine)
			{
				nested_command->command = command;
				nested_command->get_edl()->copy_all(nested_edl);
				nested_command->change_type = CHANGE_ALL;
				nested_command->realtime = renderengine->command->realtime;
				nested_renderengine = new RenderEngine(0, get_preferences(), 0, 1);
				nested_renderengine->set_acache(get_cache());
// Must use a private cache for the audio
// 				if(!cache)
// 				{
// 					cache = new CICache(get_preferences());
// 					private_cache = 1;
// 				}
// 				nested_renderengine->set_acache(cache);
				nested_renderengine->arm_command(nested_command);
			}
		}
if(debug) printf("AModule::import_samples %d speed_fragment_len=%d\n", __LINE__, (int)speed_fragment_len);

// Allocate output buffers for all channels
		for(int i = 0; i < nested_edl->session->audio_channels; i++)
		{
			if(nested_allocation < speed_fragment_len)
			{
				delete nested_output[i];
				nested_output[i] = 0;
			}

			if(!nested_output[i])
			{
				nested_output[i] = new Samples(speed_fragment_len);
			}
		}
if(debug) printf("AModule::import_samples %d\n", __LINE__);

		if(nested_allocation < speed_fragment_len)
			nested_allocation = speed_fragment_len;

// Update direction command
		nested_renderengine->command->command = command;

// Render the segment
		if(!nested_renderengine->arender)
		{
			bzero(speed_buffer->get_data(), speed_fragment_len * sizeof(double));
		}
		else
		if(sample_rate != nested_edl->session->sample_rate)
		{
// Read through sample rate converter.
			if(!resample)
			{
				resample = new AModuleResample(this);
			}

if(debug) printf("AModule::import_samples %d %d %d\n",
__LINE__,
(int)sample_rate,
(int)nested_edl->session->sample_rate);
			result = resample->resample(speed_buffer,
				speed_fragment_len,
				nested_edl->session->sample_rate,
				sample_rate,
				start_source,
				direction);
// Resample reverses to keep it running forward.
if(debug) printf("AModule::import_samples %d\n", __LINE__);
		}
		else
		{
// Render without resampling
if(debug) printf("AModule::import_samples %d\n", __LINE__);
			result = nested_renderengine->arender->process_buffer(
				nested_output,
				speed_fragment_len,
				start_source);
if(debug) printf("AModule::import_samples %d\n", __LINE__);
			memcpy(speed_buffer->get_data(),
				nested_output[edit->channel]->get_data(),
				speed_fragment_len * sizeof(double));
if(debug) printf("AModule::import_samples %d\n", __LINE__);

// Reverse fragment so ::render can apply transitions going forward.
			if(direction == PLAY_REVERSE)
			{
				Resample::reverse_buffer(speed_buffer->get_data(), speed_fragment_len);
			}
		}

if(debug) printf("AModule::import_samples %d\n", __LINE__);
	}
	else
// Source is an asset
	if(edit->asset)
	{
		nested_edl = 0;
if(debug) printf("AModule::import_samples %d\n", __LINE__);
		asset = edit->asset;

if(debug) printf("AModule::import_samples %d\n", __LINE__);
		get_cache()->age();

if(debug) printf("AModule::import_samples %d\n", __LINE__);
		if(nested_renderengine)
		{
			delete nested_renderengine;
			nested_renderengine = 0;
		}

if(debug) printf("AModule::import_samples %d\n", __LINE__);

		if(!(file = get_cache()->check_out(
			asset,
			get_edl())))
		{
// couldn't open source file / skip the edit
			printf(_("AModule::import_samples Couldn't open %s.\n"), asset->path);
			result = 1;
		}
		else
		{
			result = 0;


			if(sample_rate != asset->sample_rate)
			{
// Read through sample rate converter.
				if(!resample)
				{
					resample = new AModuleResample(this);
				}

if(debug) printf("AModule::import_samples %d %d %d\n",
__LINE__,
sample_rate,
asset->sample_rate);
				result = resample->resample(speed_buffer,
					speed_fragment_len,
					asset->sample_rate,
					sample_rate,
					start_source,
					direction);
// Resample reverses to keep it running forward.
			}
			else
			{

if(debug)
printf("AModule::import_samples %d channel=%d start_source=%jd len=%d\n", __LINE__, edit->channel, start_source, (int)speed_fragment_len);
				file->set_audio_position(start_source);
				file->set_channel(edit->channel);
				result = file->read_samples(speed_buffer, speed_fragment_len);
// Reverse fragment so ::render can apply transitions going forward.
if(debug) printf("AModule::import_samples %d speed_buffer=%p data=%p speed_fragment_len=%d\n",
__LINE__,
(void*)speed_buffer,
(void*)speed_buffer->get_data(),
(int)speed_fragment_len);
				if(direction == PLAY_REVERSE)
				{
					Resample::reverse_buffer(speed_buffer->get_data(), speed_fragment_len);
				}
if(debug) printf("AModule::import_samples %d\n", __LINE__);
			}

if(debug) printf("AModule::import_samples %d\n", __LINE__);
			get_cache()->check_in(asset);
if(debug) printf("AModule::import_samples %d\n", __LINE__);
			file = 0;





		}
	}
	else
	{
		nested_edl = 0;
		asset = 0;
if(debug) printf("AModule::import_samples %d %p %d\n", __LINE__, speed_buffer->get_data(), (int)speed_fragment_len);
		if(speed_fragment_len > 0) bzero(speed_buffer->get_data(), speed_fragment_len * sizeof(double));
if(debug) printf("AModule::import_samples %d\n", __LINE__);
	}
if(debug) printf("AModule::import_samples %d\n", __LINE__);









// Stretch it to fit the speed curve
// Need overlapping buffers to get the interpolation to work, but this
// screws up sequential effects.
	if(have_speed)
	{
		FloatAuto *previous = 0;
		FloatAuto *next = 0;
		FloatAutos *speed_autos = (FloatAutos*)track->automation->autos[AUTOMATION_SPEED];
		double *buffer_samples = buffer->get_data();
		double *speed_samples = speed_buffer->get_data();

//printf("AModule::import_samples %d %lld\n", __LINE__, speed_fragment_len);

		if(speed_fragment_len == 0)
		{
			bzero(buffer_samples, fragment_len * sizeof(double));
			bzero(prev_tail, SPEED_OVERLAP * sizeof(double));
			bzero(prev_head, SPEED_OVERLAP * sizeof(double));
		}
		else
		{
// buffer is now reversed
			if(direction == PLAY_REVERSE)
			{
				int out_offset = 0;
				speed_position = speed_position2;
	//printf("AModule::import_samples %d %lld %lld\n", __LINE__, start_project, speed_fragment_len);
				for(int64_t i = start_project + fragment_len;
					i != start_project;
					i--)
				{
	// funky sample reordering, because the source is a reversed buffer
					int in_offset = (int64_t)(speed_fragment_len - 1 - speed_position);
					CLAMP(in_offset, 0, speed_fragment_len - 1);
					buffer_samples[out_offset++] = speed_samples[in_offset];
					double speed = speed_autos->get_value(i,
						PLAY_REVERSE,
						previous,
						next);
					speed_position -= speed;
				}
	//printf("AModule::import_samples %d %f\n", __LINE__, speed_position);
 			}
			else
			{
				int out_offset = 0;
// position in buffer to read
				speed_position = speed_position1 - start_source;

//printf("AModule::import_samples %d %f\n", __LINE__, speed_position);
				for(int64_t i = start_project; i < start_project + fragment_len; i++)
				{
					double speed = speed_autos->get_value(i,
						PLAY_FORWARD,
						previous,
						next);
					double next_speed_position = speed_position + speed;

					int in_offset = (int)(speed_position);
					if(fabs(speed) >= 1.0)
					{
						int total = abs(speed);
						double accum = 0;
						for(int j = 0; j < total; j++)
						{
							int in_offset2 = in_offset + (speed > 0 ? j : -j);

							CLAMP(in_offset2, 0, speed_fragment_len - 1);
							accum += speed_samples[in_offset2];
						}


						buffer_samples[out_offset++] = accum / total;
					}
					else
					{


// if(in_offset < 0 || in_offset >= speed_fragment_len)
// printf("AModule::import_samples %d %d %d\n",
// __LINE__,
// in_offset,
// speed_fragment_len);

						int in_offset1 = in_offset;
						int in_offset2 = in_offset;

						if(speed < 0)
						{
							in_offset1 += SPEED_OVERLAP;
							in_offset2 = in_offset1 - 1;
						}
						else
						{
							in_offset1 -= SPEED_OVERLAP;
							in_offset2 = in_offset1 + 1;
						}

						CLAMP(in_offset1, -SPEED_OVERLAP, speed_fragment_len - 1 + SPEED_OVERLAP);
						CLAMP(in_offset2, -SPEED_OVERLAP, speed_fragment_len - 1 + SPEED_OVERLAP);

						double value1 = 0;
						if(in_offset1 >= speed_fragment_len)
						{
							value1 = prev_head[in_offset1 - speed_fragment_len];
						}
						else
						if(in_offset1 >= 0)
						{
							value1 = speed_samples[in_offset1];
						}
						else
						{
//printf("AModule::import_samples %d %d\n", __LINE__, in_offset1);
							value1 = prev_tail[SPEED_OVERLAP + in_offset1];
						}
#if 0
						double value2 = 0;
						if(in_offset2 >= speed_fragment_len)
						{
							value2 = prev_head[in_offset2 - speed_fragment_len];
						}
						else
						if(in_offset2 >= 0)
						{
							value2 = speed_samples()[in_offset2];
						}
						else
						{
							value2 = prev_tail[SPEED_OVERLAP + in_offset2];
						}

						double fraction = speed_position - floor(speed_position);
						buffer_samples[out_offset++] =
							value1 * (1.0 - fraction) +
							value2 * fraction;
#endif
						buffer_samples[out_offset++] = value1;


					}

					speed_position = next_speed_position;
				}
			}

			for(int i = 0; i < SPEED_OVERLAP; i++)
			{
				int offset = speed_fragment_len -
					SPEED_OVERLAP +
					i;
				CLAMP(offset, 0, speed_fragment_len - 1);
//printf("AModule::import_samples %d %d\n", __LINE__, offset, );
				prev_tail[i] = speed_samples[offset];
				offset = i;
				CLAMP(offset, 0, speed_fragment_len - 1);
				prev_head[i] = speed_samples[offset];
			}
		}
	}





	return result;
}



int AModule::render(Samples *buffer,
	int64_t input_len,
	int64_t start_position,
	int direction,
	int sample_rate,
	int use_nudge)
{
	int64_t edl_rate = get_edl()->session->sample_rate;
	const int debug = 0;

if(debug) printf("AModule::render %d\n", __LINE__);

	if(use_nudge)
		start_position += track->nudge *
			sample_rate /
			edl_rate;
	AEdit *playable_edit;
	int64_t end_position;
	if(direction == PLAY_FORWARD)
		end_position = start_position + input_len;
	else
		end_position = start_position - input_len;
	int buffer_offset = 0;
	int result = 0;


// // Flip range around so the source is always read forward.
// 	if(direction == PLAY_REVERSE)
// 	{
// 		start_project -= input_len;
// 		end_position -= input_len;
// 	}


// Clear buffer
	bzero(buffer->get_data(), input_len * sizeof(double));

// The EDL is normalized to the requested sample rate because
// the requested rate may be the project sample rate and a sample rate
// might as well be directly from the source rate to the requested rate.
// Get first edit containing the range
	if(direction == PLAY_FORWARD)
		playable_edit = (AEdit*)track->edits->first;
	else
		playable_edit = (AEdit*)track->edits->last;
if(debug) printf("AModule::render %d\n", __LINE__);

	while(playable_edit)
	{
		int64_t edit_start = playable_edit->startproject;
		int64_t edit_end = playable_edit->startproject + playable_edit->length;

// Normalize to requested rate
		edit_start = edit_start * sample_rate / edl_rate;
		edit_end = edit_end * sample_rate / edl_rate;

		if(direction == PLAY_FORWARD)
		{
			if(start_position < edit_end && end_position > edit_start)
			{
				break;
			}
			playable_edit = (AEdit*)playable_edit->next;
		}
		else
		{
			if(end_position < edit_end && start_position > edit_start)
			{
				break;
			}
			playable_edit = (AEdit*)playable_edit->previous;
		}
	}


if(debug) printf("AModule::render %d\n", __LINE__);





// Fill output one fragment at a time
	while(start_position != end_position)
	{
		int64_t fragment_len = input_len;

if(debug) printf("AModule::render %d %jd %jd\n", __LINE__, start_position, end_position);
// Clamp fragment to end of input
		if(direction == PLAY_FORWARD &&
			start_position + fragment_len > end_position)
			fragment_len = end_position - start_position;
		else
		if(direction == PLAY_REVERSE &&
			start_position - fragment_len < end_position)
			fragment_len = start_position - end_position;
if(debug) printf("AModule::render %d %jd\n", __LINE__, fragment_len);

// Normalize position here since update_transition is a boolean operation.
		update_transition(start_position *
				edl_rate /
				sample_rate,
			PLAY_FORWARD);

		if(playable_edit)
		{
			AEdit *previous_edit = (AEdit*)playable_edit->previous;

// Normalize EDL positions to requested rate
			int64_t edit_startproject = playable_edit->startproject;
			int64_t edit_endproject = playable_edit->startproject + playable_edit->length;
			int64_t edit_startsource = playable_edit->startsource;
if(debug) printf("AModule::render %d %jd\n", __LINE__, fragment_len);

			edit_startproject = edit_startproject * sample_rate / edl_rate;
			edit_endproject = edit_endproject * sample_rate / edl_rate;
			edit_startsource = edit_startsource * sample_rate / edl_rate;
if(debug) printf("AModule::render %d %jd\n", __LINE__, fragment_len);



// Clamp fragment to end of edit
			if(direction == PLAY_FORWARD &&
				start_position + fragment_len > edit_endproject)
				fragment_len = edit_endproject - start_position;
			else
			if(direction == PLAY_REVERSE &&
				start_position - fragment_len < edit_startproject)
				fragment_len = start_position - edit_startproject;
if(debug) printf("AModule::render %d %jd\n", __LINE__, fragment_len);

// Clamp to end of transition
			int64_t transition_len = 0;

			if(transition &&
				previous_edit)
			{
				transition_len = transition->length *
					sample_rate /
					edl_rate;
				if(direction == PLAY_FORWARD &&
					start_position < edit_startproject + transition_len &&
					start_position + fragment_len > edit_startproject + transition_len)
					fragment_len = edit_startproject + transition_len - start_position;
				else
				if(direction == PLAY_REVERSE &&
					start_position > edit_startproject + transition_len &&
					start_position - fragment_len < edit_startproject + transition_len)
					fragment_len = start_position - edit_startproject - transition_len;
			}
if(debug) printf("AModule::render %d buffer_offset=%d fragment_len=%jd\n",
__LINE__,
buffer_offset,
fragment_len);

			Samples output(buffer);
			output.set_offset(output.get_offset() + buffer_offset);
			if(import_samples(playable_edit,
				start_position,
				edit_startproject,
				edit_startsource,
				direction,
				sample_rate,
				&output,
				fragment_len)) result = 1;

if(debug) printf("AModule::render %d\n", __LINE__);


// Read transition into temp and render
			if(transition && previous_edit)
			{
				int64_t previous_startproject = previous_edit->startproject *
					sample_rate /
					edl_rate;
				int64_t previous_startsource = previous_edit->startsource *
					sample_rate /
					edl_rate;

// Allocate transition temp size
				int transition_fragment_len = fragment_len;
				if(direction == PLAY_FORWARD &&
					fragment_len + start_position > edit_startproject + transition_len)
					fragment_len = edit_startproject + transition_len - start_position;


// Read into temp buffers
// Temp + master or temp + temp ? temp + master
				if(transition_temp &&
					transition_temp->get_allocated() < fragment_len)
				{
					delete transition_temp;
					transition_temp = 0;
				}

				if(!transition_temp)
				{
					transition_temp = new Samples(fragment_len);
				}

if(debug) printf("AModule::render %d %jd\n", __LINE__, fragment_len);

				if(transition_fragment_len > 0)
				{
// Previous_edit is always the outgoing segment, regardless of direction
					import_samples(previous_edit,
						start_position,
						previous_startproject,
						previous_startsource,
						direction,
						sample_rate,
						transition_temp,
						transition_fragment_len);
					int64_t current_position;

// Reverse buffers here so transitions always render forward.
					if(direction == PLAY_REVERSE)
					{
						Resample::reverse_buffer(output.get_data(), transition_fragment_len);
						Resample::reverse_buffer(transition_temp->get_data(), transition_fragment_len);
						current_position = start_position -
							transition_fragment_len -
							edit_startproject;
					}
					else
					{
						current_position = start_position - edit_startproject;
					}

					transition_server->process_transition(
						transition_temp,
						&output,
						current_position,
						transition_fragment_len,
						transition->length);

// Reverse output buffer here so transitions always render forward.
					if(direction == PLAY_REVERSE)
						Resample::reverse_buffer(output.get_data(),
							transition_fragment_len);
				}
			}
if(debug) printf("AModule::render %d start_position=%jd end_position=%jd fragment_len=%jd\n",
__LINE__,
start_position,
end_position,
fragment_len);

			if(direction == PLAY_REVERSE)
			{
				if(playable_edit && start_position - fragment_len <= edit_startproject)
					playable_edit = (AEdit*)playable_edit->previous;
			}
			else
			{
				if(playable_edit && start_position + fragment_len >= edit_endproject)
					playable_edit = (AEdit*)playable_edit->next;
			}
		}

		if(fragment_len > 0)
		{
			buffer_offset += fragment_len;
			if(direction == PLAY_FORWARD)
				start_position += fragment_len;
			else
				start_position -= fragment_len;
		}
	}

if(debug) printf("AModule::render %d\n", __LINE__);

	return result;
}









