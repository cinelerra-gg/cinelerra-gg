/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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

#include "assets.h"
#include "atrack.h"
#include "automation.h"
#include "aedits.h"
#include "bcsignals.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "intauto.h"
#include "intautos.h"
#include "labels.h"
#include "localsession.h"
#include "mainundo.h"
#include "module.h"
#include "mainsession.h"
#include "pluginserver.h"
#include "pluginset.h"
#include "plugin.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "trackscroll.h"
#include "transition.h"
#include "transportque.h"
#include "vtrack.h"
#include <string.h>

int Tracks::blade(double position)
{
	for( Track *track=first; track!=0; track=track->next ) {
		if( !track->record ) continue;
		track->blade(position);
	}
	return 0;
}

int Tracks::clear(double start, double end, int clear_plugins, int edit_autos)
{
	Track *current_track;

	for(current_track = first;
		current_track;
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->clear(start,
				end,
				1, // edits
				1, // labels
				clear_plugins, // edit_plugins
				edit_autos,
				1, // convert_units
				0); // trim_edits
		}
	}
	return 0;
}

void Tracks::clear_automation(double selectionstart, double selectionend)
{
	Track* current_track;

	for(current_track = first; current_track; current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->clear_automation(selectionstart,
				selectionend,
				0,
				0);
		}
	}
}

void Tracks::clear_transitions(double start, double end)
{
	for(Track *current_track = first;
		current_track;
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			int64_t start_units = current_track->to_units(start, 0);
			int64_t end_units = current_track->to_units(end, 0);

			for(Edit *current_edit = current_track->edits->first;
				current_edit;
				current_edit = current_edit->next)
			{
				if(current_edit->startproject >= start_units &&
					current_edit->startproject < end_units &&
					current_edit->transition)
				{
					current_edit->detach_transition();
				}
			}
		}
	}
}

void Tracks::shuffle_edits(double start, double end)
{
// This doesn't affect automation or effects
// Labels follow the first track.
	int first_track = 1;
	for(Track *current_track = first;
		current_track;
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->shuffle_edits(start, end, first_track);

			first_track = 0;
		}
	}
}

void Tracks::reverse_edits(double start, double end)
{
// This doesn't affect automation or effects
// Labels follow the first track.
	int first_track = 1;
	for(Track *current_track = first;
		current_track;
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->reverse_edits(start, end, first_track);

			first_track = 0;
		}
	}
}
void Tracks::align_edits(double start, double end)
{
// This doesn't affect automation or effects
	ArrayList<double> times;

	for(Track *current_track = first;
		current_track;
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->align_edits(start, end, &times);
		}
	}
}

void Tracks::set_edit_length(double start, double end, double length)
{
	int first_track = 1;
	for(Track *current_track = first;
		current_track;
		current_track = current_track->next)
	{
		if(current_track->record)
		{
#define USE_FLOATING_LENGTHS

#ifdef USE_FLOATING_LENGTHS


// The first edit anchors the length offsets.
// Round edits up & down so they end where they would if they all had floating point lengths.
			//int first_edit = 1;
			int64_t start_units = current_track->to_units(start, 0);
			int64_t end_units = current_track->to_units(end, 0);
// Total time of edits accumulated, in track units
			int64_t total_units = 0;
// Number of length offsets added so far
			int total_lengths = 0;

			for(Edit *current_edit = current_track->edits->last;
				current_edit;
				current_edit = current_edit->previous)
			{
				if(current_edit->startproject >= start_units &&
					current_edit->startproject + current_edit->length <= end_units)
				{
// Calculate true length based on number of length offsets & total time
					double end_time = (1 + total_lengths) * length;
					int64_t length_units = current_track->to_units(end_time, 0) -
						total_units;
					if(length_units < 1) length_units = 1;
printf("Tracks::set_edit_length %d %f %f\n", __LINE__, end_time, current_track->from_units(total_units));
					total_units += length_units;

// Go in using the edit handle interface
					int64_t starting_length = current_edit->length;

					if(length_units < current_edit->length)
					{
						current_edit->shift_end_in(MOVE_ALL_EDITS,
							current_edit->startproject + length_units,
							current_edit->startproject + current_edit->length,
							1,
							edl->session->labels_follow_edits,
							edl->session->plugins_follow_edits,
							edl->session->autos_follow_edits,
							0);
					}
					else
					{
						current_edit->shift_end_out(MOVE_ALL_EDITS,
							current_edit->startproject + length_units,
							current_edit->startproject + current_edit->length,
							1,
							edl->session->labels_follow_edits,
							edl->session->plugins_follow_edits,
							edl->session->autos_follow_edits,
							0);
					}

					int64_t ending_length = current_edit->length;

					if(edl->session->labels_follow_edits && first_track)
					{
// printf("Tracks::set_edit_length %d %f %f\n",
// __LINE__,
// current_track->from_units(current_edit->startproject + starting_length),
// current_track->from_units(current_edit->startproject + ending_length));
						 edl->labels->modify_handles(
							current_track->from_units(current_edit->startproject + starting_length),
							current_track->from_units(current_edit->startproject + ending_length),
							1,
							MOVE_ALL_EDITS,
							1);
					}


					//first_edit = 0;
					total_lengths++;
				}
			}



#else // USE_FLOATING_LENGTHS

// The first edit anchors the length offsets.
// The idea was to round edits up & down so they end where they should
// if they all had floating point lengths.  It's easier just to make sure the framerate
// is divisible by the required length.
//			int first_edit = 1;
			int64_t start_units = current_track->to_units(start, 0);
			int64_t end_units = current_track->to_units(end, 0);
			int64_t length_units = current_track->to_units(length, 1);
// Starting time of the length offsets in seconds
//			double start_time = 0;
// Number of length offsets added so far
//			int total_lengths = 0;

			for(Edit *current_edit = current_track->edits->last;
				current_edit;
				current_edit = current_edit->previous)
			{
				if(current_edit->startproject >= start_units &&
					current_edit->startproject + current_edit->length <= end_units)
				{
// Calculate starting time of length offsets
//					if(first_edit)
//					{
//						start_time = current_track->from_units(current_edit->startproject);
//					}

// Calculate true length based on number of length offsets
//					double end_time = start_time + (1 + total_lengths) * length;
//					int64_t length_units = current_track->to_units(end_time, 0) -
//						current_edit->startproject;
//					if(length_units < 1) length_units = 1;

// Go in using the edit handle interface
					int64_t starting_length = current_edit->length;

					if(length_units < current_edit->length)
					{
						current_edit->shift_end_in(MOVE_ALL_EDITS,
							current_edit->startproject + length_units,
							current_edit->startproject + current_edit->length,
							1,
							edl->session->labels_follow_edits,
							edl->session->plugins_follow_edits,
							edl->session->autos_follow_edits,
							0);
					}
					else
					{
						current_edit->shift_end_out(MOVE_ALL_EDITS,
							current_edit->startproject + length_units,
							current_edit->startproject + current_edit->length,
							1,
							edl->session->labels_follow_edits,
							edl->session->plugins_follow_edits,
							edl->session->autos_follow_edits,
							0);
					}

					int64_t ending_length = current_edit->length;

					if(edl->session->labels_follow_edits && first_track)
					{
// printf("Tracks::set_edit_length %d %f %f\n",
// __LINE__,
// current_track->from_units(current_edit->startproject + starting_length),
// current_track->from_units(current_edit->startproject + ending_length));
						 edl->labels->modify_handles(
							current_track->from_units(current_edit->startproject + starting_length),
							current_track->from_units(current_edit->startproject + ending_length),
							1,
							MOVE_ALL_EDITS,
							1);
					}


//					first_edit = 0;
//					total_lengths++;
				}
			}
#endif // !USE_FLOATING_LENGTHS

			first_track = 0;
		}
	}
}

void Tracks::set_transition_length(double start, double end, double length)
{
	for(Track *current_track = first;
		current_track;
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			int64_t start_units = current_track->to_units(start, 0);
			int64_t end_units = current_track->to_units(end, 0);

			for(Edit *current_edit = current_track->edits->first;
				current_edit;
				current_edit = current_edit->next)
			{
				if(current_edit->startproject >= start_units &&
					current_edit->startproject < end_units &&
					current_edit->transition)
				{
					current_edit->transition->length =
						current_track->to_units(length, 1);
					if( current_edit == current_track->edits->last &&
					    current_edit->silence() ) {
						current_edit->length = current_edit->transition->length;
					}
				}
			}
		}
	}
}

void Tracks::set_transition_length(Transition *transition, double length)
{
// Must verify existence of transition
	int done = 0;
	if(!transition) return;
	for(Track *current_track = first;
		current_track && !done;
		current_track = current_track->next)
	{
		for(Edit *current_edit = current_track->edits->first;
			current_edit && !done;
			current_edit = current_edit->next)
		{
			if(current_edit->transition == transition)
			{
				transition->length = current_track->to_units(length, 1);
				if( current_edit == current_track->edits->last &&
				    current_edit->silence() ) {
					current_edit->length = current_edit->transition->length;
				}
				done = 1;
			}
		}
	}
}

void Tracks::paste_transitions(double start, double end, int track_type, char* title)
{
	int count = 0;
	for( Track *track=first; track; track=track->next ) {
		if( !track->record || track->data_type != track_type ) continue;
		for( Edit *edit=track->edits->first;  edit; edit=edit->next ) {
			if( !edit->is_selected ) continue;
			edit->insert_transition(title);
			++count;
		}
	}
	if( count > 0 ) {
		clear_selected_edits();
		return;
	}

	for( Track *track=first; track; track=track->next ) {
		if( !track->record || track->data_type != track_type ) continue;
		int64_t start_units = track->to_units(start, 0);
		int64_t end_units = track->to_units(end, 0);
		if( start_units == end_units ) {
			for( Edit *edit = track->edits->first; edit; edit = edit->next) {
				int64_t edit_start = edit->startproject;
				int64_t edit_end = edit_start + edit->length;
				if( edit_start > start_units ) continue;
				if( start_units == track->edits->length() ) {
					double length = edl->session->default_transition_length;
					int64_t units = track->to_units(length, 1);
					edit = track->edits->
						create_silence(start_units, start_units+units);
				}
				else if( start_units >= edit_end ) continue;
				edit->insert_transition(title);
			}
		}
		else {
			for( Edit *edit=track->edits->first; edit; edit=edit->next) {
				int64_t edit_start = edit->startproject;
				if( !edit_start ) continue;
				if( edit_start >= start_units && edit_start < end_units ) {
					edit->insert_transition(title);
				}
			}
		}
	}
}

void Tracks::set_automation_mode(double selectionstart,
	double selectionend,
	int mode)
{
	Track* current_track;

	for(current_track = first; current_track; current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->set_automation_mode(selectionstart,
				selectionend,
				mode);
		}
	}
}

int Tracks::clear_default_keyframe()
{
	for(Track *current = first; current; current = NEXT)
	{
		if(current->record)
			current->clear_automation(0, 0, 0, 1);
	}
	return 0;
}

int Tracks::clear_handle(double start,
	double end,
	double &longest_distance,
	int clear_labels,
	int clear_plugins,
	int edit_autos)
{
	Track* current_track;
	double distance;

	for(current_track = first; current_track; current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->clear_handle(start,
				end,
				clear_labels,
				clear_plugins,
				edit_autos,
				distance);
			if(distance > longest_distance) longest_distance = distance;
		}
	}

	return 0;
}

int Tracks::copy_automation(double selectionstart,
	double selectionend,
	FileXML *file,
	int default_only,
	int autos_only)
{
// called by MWindow::copy_automation for copying automation alone
	Track* current_track;

	file->tag.set_title("AUTO_CLIPBOARD");
	file->tag.set_property("LENGTH", selectionend - selectionstart);
	file->tag.set_property("FRAMERATE", edl->session->frame_rate);
	file->tag.set_property("SAMPLERATE", edl->session->sample_rate);
	file->append_tag();
	file->append_newline();
	file->append_newline();

	for(current_track = first;
		current_track;
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->copy_automation(selectionstart,
				selectionend,
				file,
				default_only,
				autos_only);
		}
	}

	file->tag.set_title("/AUTO_CLIPBOARD");
	file->append_tag();
	file->append_newline();
	file->terminate_string();
	return 0;
}

// int Tracks::copy_default_keyframe(FileXML *file)
// {
// 	copy_automation(0, 0, file, 1, 0);
// 	return 0;
// }

int Tracks::delete_tracks()
{
	int total_deleted = 0;
	int done = 0;

	while(!done)
	{
		done = 1;
		Track *next_track = 0;
		for (Track* current = first; current && done; current = next_track)
		{
			next_track = current->next;
			if(current->record)
			{
				delete_track(current);
				current = NULL;
				total_deleted++;
				done = 0;
				break;
			}
		}
	}
	return total_deleted;
}

void Tracks::move_edits(ArrayList<Edit*> *edits,
	Track *track,
	double position,
	int edit_labels,  // Ignored
	int edit_plugins,  // Ignored
	int edit_autos, // Ignored
	int behaviour)
{
//printf("Tracks::move_edits 1\n");
	for(Track *dest_track = track; dest_track; dest_track = dest_track->next)
	{
		if(dest_track->record)
		{
// Need a local copy of the source edit since the original source edit may
// change in the editing operation.
			Edit *source_edit = 0;
			Track *source_track = 0;


// Get source track
			if(dest_track->data_type == TRACK_AUDIO)
			{
				int current_aedit = 0;

				while(current_aedit < edits->total &&
					edits->values[current_aedit]->track->data_type != TRACK_AUDIO)
					current_aedit++;

				if(current_aedit < edits->total)
				{
					source_edit = edits->values[current_aedit];
					source_track = source_edit->track;
					edits->remove_number(current_aedit);
				}
			}
			else
			if(dest_track->data_type == TRACK_VIDEO)
			{
				int current_vedit = 0;
				while(current_vedit < edits->total &&
					edits->values[current_vedit]->track->data_type != TRACK_VIDEO)
					current_vedit++;

				if(current_vedit < edits->total)
				{
					source_edit = edits->values[current_vedit];
					source_track = source_edit->track;
					edits->remove_number(current_vedit);
				}
			}

//printf("Tracks::move_edits 2 %s %s %d\n", source_track->title, dest_track->title, source_edit->length);
			if(source_edit)
			{
				int64_t position_i = source_track->to_units(position, 0);
// Source edit changes
				int64_t source_length = source_edit->length;
				int64_t source_startproject = source_edit->startproject;

				if (behaviour == 0)
				{
				// This works like this: CUT edit, INSERT edit at final position, keyframes also follow
				// FIXME: there should be a GUI way to tell whenever user also wants to move autos or not
// this is all screwed up
//  inserts defaults/bogus everywhere
#if 0
// Copy keyframes
					FileXML temp;
					AutoConf temp_autoconf;

					temp_autoconf.set_all(1);

					source_track->automation->copy(source_edit->startproject,
						source_edit->startproject + source_edit->length,
						&temp,
						0,
						0);
					temp.terminate_string();
					temp.rewind();
// Insert new keyframes
//printf("Tracks::move_edits 2 %d %p\n", result->startproject, result->asset);
					source_track->automation->clear(source_edit->startproject,
						source_edit->startproject + source_edit->length,
						&temp_autoconf,
						1);
					int64_t position_a = position_i;
					if (dest_track == source_track)
					{
						if (position_a > source_edit->startproject)
							position_a -= source_length;
					}

					dest_track->automation->paste_silence(position_a,
						position_a + source_length);
					while(!temp.read_tag())
						dest_track->automation->paste(position_a,
							source_length, 1.0, &temp, 0, 1,
							&temp_autoconf);
#endif
// Insert new edit
					Edit *dest_edit = dest_track->edits->shift(position_i,
						source_length);
					Edit *result = dest_track->edits->insert_before(dest_edit,
						dest_track->edits->create_edit());
					result->copy_from(source_edit);
					result->startproject = position_i;
					result->length = source_length;

// Clear source
					source_track->edits->clear(source_edit->startproject,
						source_edit->startproject + source_length);

	/*
//this is outline for future thinking how it is supposed to be done trough C&P mechanisms
					temp.reset_tag();
					source_track->cut(source_edit->startproject,
						source_edit->startproject + source_edit->length,
						&temp,
						NULL);
					temp.terminate_string();
					temp.rewind();
					dest_track->paste_silence(position_a,
						position_a + source_length,
						edit_plugins);
					while(!temp.read_tag())
						dest_track->paste(position_a,          // MISSING PIECE OF FUNCTIONALITY
							source_length,
							1.0,
							&temp,
							0,
							&temp_autoconf);
	*/


				} else
				if (behaviour == 1)
				// ONLY edit is moved, all other edits stay where they are
				{
					// Copy edit to temp, delete the edit, insert the edit
					Edit *temp_edit = dest_track->edits->create_edit();
					temp_edit->copy_from(source_edit);
					// we call the edits directly since we do not want to move keyframes or anything else
					source_track->edits->clear(source_startproject,
						source_startproject + source_length);
					source_track->edits->paste_silence(source_startproject,
						source_startproject + source_length);

					dest_track->edits->clear(position_i,
						position_i + source_length);
					Edit *dest_edit = dest_track->edits->shift(position_i,  source_length);
					Edit *result = dest_track->edits->insert_before(dest_edit,
						dest_track->edits->create_edit());
					result->copy_from(temp_edit);
					result->startproject = position_i;
					result->length = source_length;
					delete temp_edit;
				}
				source_track->optimize();
				dest_track->optimize();
			}
		}
	}
}

void Tracks::move_group(EDL *group, Track *first_track, double position)
{
	for( Track *track=first; track; track=track->next ) {
		if( !track->record ) continue;
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			if( !edit->is_selected ) continue;
			edit->mute();  edit->is_selected = 0;
		}
	}
	Track *src = group->tracks->first;
	for( Track *track=first_track; track && src; track=track->next ) {
		if( !track->record ) continue;
		int64_t pos = track->to_units(position, 0);
		for( Edit *edit=src->edits->first; edit; edit=edit->next ) {
			if( edit->silence() ) continue;
			int64_t start = pos + edit->startproject;
			int64_t end = start + edit->length;
			track->edits->clear(start, end);
			Edit *dst = track->edits->insert_new_edit(start);
			dst->copy_from(edit);
			dst->startproject = start;
			dst->is_selected = 1;
			while( (dst=dst->next) != 0 )
				dst->startproject += edit->length;
		}
		track->optimize();
		src = src->next;
	}
}

void Tracks::move_effect(Plugin *plugin, Track *track, int64_t position)
{
	Track *source_track = plugin->track;
	Plugin *result = 0;
// Create a new plugin set
	double start = track->from_units(position);
	double length = track->from_units(plugin->length);

	result = track->insert_effect("", &plugin->shared_location, 0, 0,
				start, length, plugin->plugin_type);
	result->copy_from(plugin);
	result->shift(position - plugin->startproject);

// Clear new plugin from old set
	plugin->plugin_set->clear(plugin->startproject,
		plugin->startproject + plugin->length,
		edl->session->autos_follow_edits);

	source_track->optimize();
}

void Tracks::move_effect(Plugin *plugin, PluginSet *plugin_set, int64_t position)
{
// src/dest track must be the same
// replace plugin in source plugin_set with silence
	PluginSet *src_plugin_set = plugin->plugin_set;
	Plugin *silent = new Plugin(edl, src_plugin_set, "");
	silent->startproject = plugin->startproject;
	silent->length = plugin->length;
	src_plugin_set->insert_after(plugin, silent);
	src_plugin_set->remove_pointer(plugin);
// truncate previous plugin
	Plugin *dest = (Plugin *)plugin_set->editof(position, PLAY_FORWARD, 0);
// add plugin after dest
	plugin_set->insert_after(dest, plugin);
	if( dest ) {
		dest->length = position - dest->startproject;
		if( dest->length < 0 ) dest->length = 0;
	}
// update plugin position
	plugin->startproject = position;
	plugin->plugin_set = plugin_set;
	plugin->edits = plugin_set;
	src_plugin_set->track->optimize();
}

int Tracks::concatenate_tracks(int edit_plugins, int edit_autos)
{
	Track *output_track, *first_output_track, *input_track;
	int i, data_type = TRACK_AUDIO;
	double output_start;
	int result = 0;

// Relocate tracks
	for(i = 0; i < 2; i++)
	{
// Get first output track
		for(output_track = first;
			output_track;
			output_track = output_track->next)
			if(output_track->data_type == data_type &&
				output_track->record) break;

		first_output_track = output_track;

// Get first input track
		for(input_track = first;
			input_track;
			input_track = input_track->next)
		{
			if(input_track->data_type == data_type &&
				input_track->play &&
				!input_track->record) break;
		}


		if(output_track && input_track)
		{
// Transfer input track to end of output track one at a time
			while(input_track)
			{
				output_start = output_track->get_length();
				output_track->insert_track(input_track,
					output_start,
					0,
					edit_plugins,
					edit_autos,
					0);

// Get next source and destination
				for(input_track = input_track->next;
					input_track;
					input_track = input_track->next)
				{

					if(input_track->data_type == data_type &&
						!input_track->record &&
						input_track->play) break;
				}

				for(output_track = output_track->next;
					output_track;
					output_track = output_track->next)
				{
					if(output_track->data_type == data_type &&
						output_track->record) break;
				}

				if(!output_track)
				{
					output_track = first_output_track;
				}
			}
			result = 1;
		}

		if(data_type == TRACK_AUDIO) data_type = TRACK_VIDEO;
	}

	return result;
}

int Tracks::delete_all_tracks()
{
	while(last) delete last;
	return 0;
}


void Tracks::change_modules(int old_location, int new_location, int do_swap)
{
	for(Track* current = first ; current; current = current->next)
	{
		current->change_modules(old_location, new_location, do_swap);
	}
}

void Tracks::change_plugins(SharedLocation &old_location, SharedLocation &new_location, int do_swap)
{
	for(Track* current = first ; current; current = current->next)
	{
		current->change_plugins(old_location, new_location, do_swap);
	}
}



// =========================================== EDL editing


int Tracks::copy(double start,
	double end,
	int all,
	FileXML *file,
	const char *output_path)
{
// nothing selected
	if(start == end && !all) return 1;

	Track* current;

	for(current = first;
		current;
		current = NEXT)
	{
		if(current->record || all)
		{
			current->copy(start, end, file,output_path);
		}
	}

	return 0;
}



int Tracks::move_track_up(Track *track)
{
	Track *next_track = track->previous;
	if(!next_track) next_track = last;

	change_modules(number_of(track), number_of(next_track), 1);

// printf("Tracks::move_track_up 1 %p %p\n", track, next_track);
// int count = 0;
// for(Track *current = first; current && count < 5; current = NEXT, count++)
// 	printf("Tracks::move_track_up %p %p %p\n", current->previous, current, current->next);
// printf("Tracks::move_track_up 2\n");
//
	swap(track, next_track);

// count = 0;
// for(Track *current = first; current && count < 5; current = NEXT, count++)
// 	printf("Tracks::move_track_up %p %p %p\n", current->previous, current, current->next);
// printf("Tracks::move_track_up 3\n");

	return 0;
}

int Tracks::move_track_down(Track *track)
{
	Track *next_track = track->next;
	if(!next_track) next_track = first;

	change_modules(number_of(track), number_of(next_track), 1);
	swap(track, next_track);
	return 0;
}


int Tracks::move_tracks_up()
{
	Track *track, *next_track;
	int result = 0;

	for(track = first;
		track;
		track = next_track)
	{
		next_track = track->next;

		if(track->record)
		{
			if(track->previous)
			{
				change_modules(number_of(track->previous), number_of(track), 1);

				swap(track->previous, track);
				result = 1;
			}
		}
	}

	return result;
}

int Tracks::move_tracks_down()
{
	Track *track, *previous_track;
	int result = 0;

	for(track = last;
		track;
		track = previous_track)
	{
		previous_track = track->previous;

		if(track->record)
		{
			if(track->next)
			{
				change_modules(number_of(track), number_of(track->next), 1);

				swap(track, track->next);
				result = 1;
			}
		}
	}

	return result;
}



void Tracks::paste_audio_transition(PluginServer *server)
{
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO &&
			current->record)
		{
			int64_t position = current->to_units(
				edl->local_session->get_selectionstart(), 0);
			Edit *current_edit = current->edits->editof(position,
				PLAY_FORWARD,
				0);
			if( !current_edit && position == current->edits->length() ) {
				double length = edl->session->default_transition_length;
				int64_t units = current->to_units(length, 1);
				current_edit = current->edits->create_silence(position, position+units);
			}
			if(current_edit)
			{
				paste_transition(server, current_edit);
			}
		}
	}
}

void Tracks::paste_automation(double selectionstart,
	FileXML *file,
	int default_only,
	int active_only,
	int typeless)
{
	Track* current_track = 0;
	Track* current_atrack = 0;
	Track* current_vtrack = 0;
	Track* dst_track = 0;
	int src_type;
	int result = 0;
	double length;
	double frame_rate = edl->session->frame_rate;
	int64_t sample_rate = edl->session->sample_rate;
	char string[BCTEXTLEN];
	string[0] = 0;

// Search for start
	do{
	  result = file->read_tag();
	}while(!result &&
		!file->tag.title_is("AUTO_CLIPBOARD"));

	if(!result)
	{
		length = file->tag.get_property("LENGTH", 0);
		frame_rate = file->tag.get_property("FRAMERATE", frame_rate);
		sample_rate = file->tag.get_property("SAMPLERATE", sample_rate);


		do
		{
			result = file->read_tag();

			if(!result)
			{
				if(file->tag.title_is("/AUTO_CLIPBOARD"))
				{
					result = 1;
				}
				else
				if(file->tag.title_is("TRACK"))
				{
					file->tag.get_property("TYPE", string);
					if(!strcmp(string, "AUDIO"))
					{
						src_type = TRACK_AUDIO;
					}
					else
					{
						src_type = TRACK_VIDEO;
					}

// paste to any media type
					if(typeless)
					{
						if(!current_track) current_track = first;
						while(current_track && !current_track->record)
							current_track = current_track->next;
						dst_track = current_track;
					}
					else
					if(!strcmp(string, "AUDIO"))
					{
// Get next audio track
						if(!current_atrack)
							current_atrack = first;
						else
							current_atrack = current_atrack->next;

						while(current_atrack &&
							(current_atrack->data_type != TRACK_AUDIO ||
							!current_atrack->record))
							current_atrack = current_atrack->next;
						dst_track = current_atrack;
					}
					else
					{
// Get next video track
						if(!current_vtrack)
							current_vtrack = first;
						else
							current_vtrack = current_vtrack->next;

						while(current_vtrack &&
							(current_vtrack->data_type != TRACK_VIDEO ||
							!current_vtrack->record))
							current_vtrack = current_vtrack->next;

						dst_track = current_vtrack;
					}

					if(dst_track)
					{
						double frame_rate2 = frame_rate;
						double sample_rate2 = sample_rate;

						if(src_type != dst_track->data_type)
						{
							frame_rate2 = sample_rate;
							sample_rate2 = frame_rate;
						}

						dst_track->paste_automation(selectionstart,
							length,
							frame_rate2,
							sample_rate2,
							file,
							default_only,
							active_only);
					}
				}
			}
		}while(!result);
	}
}

// int Tracks::paste_default_keyframe(FileXML *file)
// {
// 	paste_automation(0, file, 1, 0);
// 	return 0;
// }

void Tracks::paste_transition(PluginServer *server, Edit *dest_edit)
{
	dest_edit->insert_transition(server->title);
}

void Tracks::paste_video_transition(PluginServer *server, int first_track)
{
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO &&
			current->record)
		{
			int64_t position = current->to_units(
				edl->local_session->get_selectionstart(), 0);
			Edit *current_edit = current->edits->editof(position,
				PLAY_FORWARD,
				0);
			if( !current_edit && position == current->edits->length() ) {
				double length = edl->session->default_transition_length;
				int64_t units = current->to_units(length, 1);
				current_edit = current->edits->create_silence(position, position+units);
			}
			if(current_edit)
			{
				paste_transition(server, current_edit);
			}
			if(first_track) break;
		}
	}
}


int Tracks::paste_silence(double start,
	double end,
	int edit_plugins,
	int edit_autos)
{
	Track* current_track;

	for(current_track = first;
		current_track;
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->paste_silence(start,
				end,
				edit_plugins,
				edit_autos);
		}
	}
	return 0;
}



int Tracks::select_auto(int cursor_x, int cursor_y)
{
	int result = 0;
	for(Track* current = first; current && !result; current = NEXT) { result = current->select_auto(&auto_conf, cursor_x, cursor_y); }
	return result;
}

int Tracks::move_auto(int cursor_x, int cursor_y, int shift_down)
{
	int result = 0;

	for(Track* current = first; current && !result; current = NEXT)
	{
		result = current->move_auto(&auto_conf, cursor_x, cursor_y, shift_down);
	}
	return 0;
}

int Tracks::modify_edithandles(double &oldposition,
	double &newposition,
	int currentend,
	int handle_mode,
	int edit_labels,
	int edit_plugins,
	int edit_autos)
{
	Track *current;

	for(current = first; current; current = NEXT)
	{
		if(current->record)
		{
			current->modify_edithandles(oldposition,
				newposition,
				currentend,
				handle_mode,
				edit_labels,
				edit_plugins,
				edit_autos);
		}
	}
	return 0;
}

int Tracks::modify_pluginhandles(double &oldposition,
	double &newposition,
	int currentend,
	int handle_mode,
	int edit_labels,
	int edit_autos,
	Edits *trim_edits)
{
	Track *current;

	for(current = first; current; current = NEXT)
	{
		if(current->record)
		{
			current->modify_pluginhandles(oldposition,
				newposition,
				currentend,
				handle_mode,
				edit_labels,
				edit_autos,
				trim_edits);
		}
	}
	return 0;
}



int Tracks::purge_asset(Asset *asset)
{
	Track *current_track;
	int result = 0;

	for(current_track = first; current_track; current_track = current_track->next)
	{
		result += current_track->purge_asset(asset);
	}
	return result;
}

int Tracks::asset_used(Asset *asset)
{
	Track *current_track;
	int result = 0;

	for(current_track = first; current_track; current_track = current_track->next)
	{
		result += current_track->asset_used(asset);
	}
	return result;
}

int Tracks::scale_time(float rate_scale, int ignore_record, int scale_edits, int scale_autos, int64_t start, int64_t end)
{
	Track *current_track;

	for(current_track = first;
		current_track;
		current_track = current_track->next)
	{
		if((current_track->record || ignore_record) &&
			current_track->data_type == TRACK_VIDEO)
		{
			current_track->scale_time(rate_scale, scale_edits, scale_autos, start, end);
		}
	}
	return 0;
}

