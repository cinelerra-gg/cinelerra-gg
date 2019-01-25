
/*
 * CINELERRA
 * Copyright (C) 2008-2017 Adam Williams <broadcast at earthling dot net>
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

#include "aedit.h"
#include "asset.h"
#include "assets.h"
#include "automation.h"
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "clipedls.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "filesystem.h"
#include "localsession.h"
#include "plugin.h"
#include "strategies.inc"
#include "track.h"
#include "transition.h"
#include "transportque.inc"

#include <string.h>

Edits::Edits(EDL *edl, Track *track)
 : List<Edit>()
{
	this->edl = edl;
	this->track = track;
}

Edits::~Edits()
{
}


void Edits::equivalent_output(Edits *edits, int64_t *result)
{
// For the case of plugin sets, a new plugin set may be created with
// plugins only starting after 0.  We only want to restart brender at
// the first plugin in this case.
	for(Edit *current = first, *that_current = edits->first;
		current || that_current;
		current = NEXT,
		that_current = that_current->next)
	{
//printf("Edits::equivalent_output 1 %d\n", *result);
		if(!current && that_current)
		{
			int64_t position1 = length();
			int64_t position2 = that_current->startproject;
			if(*result < 0 || *result > MIN(position1, position2))
				*result = MIN(position1, position2);
			break;
		}
		else
		if(current && !that_current)
		{
			int64_t position1 = edits->length();
			int64_t position2 = current->startproject;
			if(*result < 0 || *result > MIN(position1, position2))
				*result = MIN(position1, position2);
			break;
		}
		else
		{
//printf("Edits::equivalent_output 2 %d\n", *result);
			current->equivalent_output(that_current, result);
//printf("Edits::equivalent_output 3 %d\n", *result);
		}
	}
}

void Edits::copy_from(Edits *edits)
{
	while(last) delete last;
	for(Edit *current = edits->first; current; current = NEXT)
	{
		Edit *new_edit = append(create_edit());
		new_edit->copy_from(current);
	}
}


Edits& Edits::operator=(Edits& edits)
{
printf("Edits::operator= 1\n");
	copy_from(&edits);
	return *this;
}


void Edits::insert_asset(Asset *asset, EDL *nested_edl,
	int64_t length, int64_t position, int track_number)
{
	Edit *new_edit = insert_new_edit(position);

	new_edit->nested_edl = nested_edl;
	new_edit->asset = asset;
	new_edit->startsource = 0;
	new_edit->startproject = position;
	new_edit->length = length;

	if(nested_edl)
	{
		if(track->data_type == TRACK_AUDIO)
			new_edit->channel = track_number % nested_edl->session->audio_channels;
		else
			new_edit->channel = 0;
	}

	if(asset && !nested_edl)
	{
		if(asset->audio_data)
			new_edit->channel = track_number % asset->channels;
		else
		if(asset->video_data)
			new_edit->channel = track_number % asset->layers;
	}

//printf("Edits::insert_asset %d %d\n", new_edit->channel, new_edit->length);
	for(Edit *current = new_edit->next; current; current = NEXT)
	{
		current->startproject += length;
	}
}

void Edits::insert_edits(Edits *source_edits,
	int64_t position,
	int64_t min_length,
	int edit_autos)
{
	//int64_t clipboard_end = position + min_length;
// Length pasted so far
	int64_t source_len = 0;

// Fill region between end of edit table and beginning of pasted segment
// with silence.  Can't call from insert_new_edit because it's recursive.
	if(position > length())
	{
		paste_silence(length(), position);
	}


	for(Edit *source_edit = source_edits->first;
		source_edit;
		source_edit = source_edit->next)
	{
		EDL *dest_nested_edl = 0;
		if(source_edit->nested_edl)
			dest_nested_edl = edl->nested_edls.get_nested(source_edit->nested_edl);

// Update Assets
		Asset *dest_asset = 0;
		if(source_edit->asset)
			dest_asset = edl->assets->update(source_edit->asset);
// Open destination area
		Edit *dest_edit = insert_new_edit(position + source_edit->startproject);

		dest_edit->copy_from(source_edit);
		dest_edit->asset = dest_asset;
		dest_edit->nested_edl = dest_nested_edl;
		dest_edit->startproject = position + source_edit->startproject;



// Shift keyframes in source edit to their position in the
// destination edit for plugin case
		if(edit_autos) dest_edit->shift_keyframes(position);

// Shift following edits and keyframes in following edits by length
// in current source edit.
		for(Edit *future_edit = dest_edit->next;
			future_edit;
			future_edit = future_edit->next)
		{
			future_edit->startproject += dest_edit->length;
			future_edit->shift_keyframes(dest_edit->length);
		}

		source_len += source_edit->length;
	}




// Fill remaining clipboard length with silence
	if(source_len < min_length)
	{
//printf("Edits::insert_edits %d\n", __LINE__);
		paste_silence(position + source_len, position + min_length);
	}
}


// Native units
// Can't paste silence in here because it's used by paste_silence.
Edit* Edits::insert_new_edit(int64_t position)
{
//printf("Edits::insert_new_edit 1\n");
	Edit *current = split_edit(position);

//printf("Edits::insert_new_edit 1\n");
	Edit *new_edit = create_edit();
	if( current ) new_edit->hard_right = current->hard_left;
	if( current ) current = PREVIOUS;
	if( current ) new_edit->hard_left = current->hard_right;
//printf("Edits::insert_new_edit 1\n");
	insert_after(current, new_edit);
	new_edit->startproject = position;
//printf("Edits::insert_new_edit 2\n");
	return new_edit;
}

Edit* Edits::split_edit(int64_t position)
{
// Get edit containing position
	Edit *edit = editof(position, PLAY_FORWARD, 0);
	if(!edit) return 0;
// Split would have created a 0 length
//	if(edit->startproject == position) return edit;
// Create anyway so the return value comes before position

	Edit *new_edit = create_edit();
	insert_after(edit, new_edit);
	new_edit->copy_from(edit);
	new_edit->length = new_edit->startproject + new_edit->length - position;
	edit->length = position - edit->startproject;
	new_edit->startproject = position;
	new_edit->startsource += edit->length;

// Decide what to do with the transition
	if(edit->length && edit->transition) {
		delete new_edit->transition;
		new_edit->transition = 0;
	}

	if(edit->transition && edit->transition->length > edit->length)
		edit->transition->length = edit->length;
	if(new_edit->transition && new_edit->transition->length > new_edit->length)
		new_edit->transition->length = new_edit->length;
	return new_edit;
}

int Edits::save(FileXML *xml, const char *output_path)
{
	copy(0, length(), xml, output_path);
	return 0;
}

void Edits::resample(double old_rate, double new_rate)
{
	for(Edit *current = first; current; current = NEXT)
	{
		current->startproject = Units::to_int64((double)current->startproject /
			old_rate *
			new_rate);
		if(PREVIOUS) PREVIOUS->length = current->startproject - PREVIOUS->startproject;
		current->startsource = Units::to_int64((double)current->startsource /
			old_rate *
			new_rate);
		if(!NEXT) current->length = Units::to_int64((double)current->length /
			old_rate *
			new_rate);
		if(current->transition)
		{
			current->transition->length = Units::to_int64(
				(double)current->transition->length /
				old_rate *
				new_rate);
		}
		current->resample(old_rate, new_rate);
	}
}

int Edits::is_glitch(Edit *edit)
{
	if( track->data_type != TRACK_AUDIO ) return 0;
	int64_t threshold = edl->session->frame_rate > 0 ?
		0.5 * edl->session->sample_rate / edl->session->frame_rate : 0;
// audio edit shorter than .5 frames is a glitch
	return edit->length < threshold ? 1 : 0;
}

int Edits::optimize()
{
	int result = 1;
	Edit *current;

//printf("Edits::optimize %d\n", __LINE__);
// Sort edits by starting point
	while(result)
	{
		result = 0;

		for(current = first; current; current = NEXT)
		{
			Edit *next_edit = NEXT;

			if(next_edit && next_edit->startproject < current->startproject)
			{
				swap(next_edit, current);
				result = 1;
			}
		}
	}

// Insert silence between edits which aren't consecutive
	for(current = last; current; current = current->previous)
	{
		if(current->previous)
		{
			Edit *previous_edit = current->previous;
			if(current->startproject -
				previous_edit->startproject -
				previous_edit->length > 0)
			{
				Edit *new_edit = create_edit();
				insert_before(current, new_edit);
				new_edit->startproject = previous_edit->startproject + previous_edit->length;
				new_edit->length = current->startproject -
					previous_edit->startproject -
					previous_edit->length;
			}
		}
		else
		if(current->startproject > 0)
		{
			Edit *new_edit = create_edit();
			insert_before(current, new_edit);
			new_edit->length = current->startproject;
		}
	}

	result = 1;
	while(result) {
		result = 0;

// delete 0 length edits
		for( current = first; !result && current; ) {
			Edit* next = current->next;
			if( current->length == 0 ) {
				if( next && current->transition && !next->transition) {
					next->transition = current->transition;
					next->transition->edit = next;
					current->transition = 0;
				}
				delete current;
				result = 1;
				break;
			}
			current = next;
		}

// merge same files or transitions, and deglitch
		if( !result && track->data_type != TRACK_SUBTITLE ) {
			current = first;
			if( current && !current->hard_right &&
			    current->next && !current->next->hard_left &&
			    is_glitch(current) ) {
// if the first edit is a glitch, change it to silence
				current->asset = 0;
				current->nested_edl = 0;
			}
			Edit *next_edit = 0;
			for( ; current && (next_edit=current->next); current=NEXT ) {
// both edges are not hard edges
				if( current->hard_right || next_edit->hard_left ) continue;
// next edit is a glitch
				if( is_glitch(next_edit) )
					break;
// both edits are silence & not a plugin
				if( !current->is_plugin && current->silence() &&
				    !next_edit->is_plugin && next_edit->silence() )
					break;
// source channels are identical & assets are identical
				if( !result && current->channel == next_edit->channel &&
				    current->asset == next_edit->asset &&
				    current->nested_edl == next_edit->nested_edl ) {
//  and stop and start in the same frame
					int64_t current_end = current->startsource + current->length;
					int64_t next_start = next_edit->startsource;
					if( current_end == next_start ||
					    EQUIV(edl->frame_align(track->from_units(current_end), 1),
						  edl->frame_align(track->from_units(next_start), 1)) )
						break;
				}
			}
			if( next_edit ) {
				int64_t current_start = current->startproject;
				int64_t next_end = next_edit->startproject + next_edit->length;
				current->length = next_end - current_start;
				current->hard_right = next_edit->hard_right;
				remove(next_edit);
				result = 1;
			}
		}

		if( last && last->silence() &&
		    !last->transition && !last->hard_left && !last->hard_right ) {
			delete last;
			result = 1;
		}
	}

	return 0;
}


// ===================================== file operations

void Edits::load(FileXML *file, int track_offset)
{
	int64_t startproject = 0;

	while( last ) delete last;

 	while( !file->read_tag() ) {
//printf("Edits::load 1 %s\n", file->tag.get_title());
		if(!strcmp(file->tag.get_title(), "EDIT")) {
			load_edit(file, startproject, track_offset);
		}
		else if(!strcmp(file->tag.get_title(), "/EDITS"))
			break;
	}

//track->dump();
	optimize();
}

int Edits::load_edit(FileXML *file, int64_t &startproject, int track_offset)
{
	Edit* current = append_new_edit();
	current->load_properties(file, startproject);

	startproject += current->length;

	while( !file->read_tag() ) {
		if(file->tag.title_is("NESTED_EDL")) {
			char path[BCTEXTLEN];
			path[0] = 0;
			file->tag.get_property("SRC", path);
//printf("Edits::load_edit %d path=%s\n", __LINE__, path);
			if(path[0] != 0) {
				current->nested_edl = edl->nested_edls.load(path);
			}
// printf("Edits::load_edit %d nested_edl->path=%s\n",
// __LINE__, current->nested_edl->path);
		}
		else if(file->tag.title_is("FILE")) {
			char filename[BCTEXTLEN];
			filename[0] = 0;
			file->tag.get_property("SRC", filename);
// Extend path
			if(filename[0] != 0) {
				char directory[BCTEXTLEN], edl_directory[BCTEXTLEN];
				FileSystem fs;
				fs.set_current_dir("");
				fs.extract_dir(directory, filename);
				if(!strlen(directory)) {
					fs.extract_dir(edl_directory, file->filename);
					fs.join_names(directory, edl_directory, filename);
					strcpy(filename, directory);
				}
				current->asset = edl->assets->get_asset(filename);
			}
			else {
				current->asset = 0;
			}
//printf("Edits::load_edit 5\n");
		}
		else if(file->tag.title_is("TRANSITION")) {
			current->transition = new Transition(edl, current, "",
				track->to_units(edl->session->default_transition_length, 1));
				current->transition->load_xml(file);
		}
		else if(file->tag.title_is("/EDIT"))
			break;
	}

//printf("Edits::load_edit %d\n", __LINE__);
//track->dump();
//printf("Edits::load_edit %d\n", __LINE__);
	return 0;
}

// ============================================= accounting

int64_t Edits::length()
{
	return last ? last->startproject + last->length : 0;
}



Edit* Edits::editof(int64_t position, int direction, int use_nudge)
{
	Edit *current = 0;
	if(use_nudge && track) position += track->nudge;

	if(direction == PLAY_FORWARD) {
		for(current = last; current; current = PREVIOUS) {
			if(current->startproject <= position && current->startproject + current->length > position)
				return current;
		}
	}
	else
	if(direction == PLAY_REVERSE) {
		for(current = first; current; current = NEXT) {
			if(current->startproject < position && current->startproject + current->length >= position)
				return current;
		}
	}

	return 0;     // return 0 on failure
}

Edit* Edits::get_playable_edit(int64_t position, int use_nudge)
{
	Edit *current;
	if(track && use_nudge) position += track->nudge;

// Get the current edit
	for(current = first; current; current = NEXT) {
		if(current->startproject <= position &&
			current->startproject + current->length > position)
			break;
	}

// Get the edit's asset
// TODO: descend into nested EDLs
	if(current) {
		if(!current->asset)
			current = 0;
	}

	return current;     // return 0 on failure
}

// ================================================ editing



int Edits::copy(int64_t start, int64_t end, FileXML *file, const char *output_path)
{
	Edit *current_edit;

	file->tag.set_title("EDITS");
	file->append_tag();
	file->append_newline();

	for(current_edit = first; current_edit; current_edit = current_edit->next)
	{
		current_edit->copy(start, end, file, output_path);
	}

	file->tag.set_title("/EDITS");
	file->append_tag();
	file->append_newline();
	return 0;
}



void Edits::clear(int64_t start, int64_t end)
{
	if( start >= end ) return;

	Edit* edit1 = editof(start, PLAY_FORWARD, 0);
	Edit* edit2 = editof(end, PLAY_FORWARD, 0);
	Edit* current_edit;

	if(end == start) return;        // nothing selected
	if(!edit1 && !edit2) return;       // nothing selected


	if(!edit2) {                // edit2 beyond end of track
		edit2 = last;
		end = this->length();
	}

	if( edit1 && edit2 && edit1 != edit2)
	{
// in different edits

//printf("Edits::clear 3.5 %d %d %d %d\n", edit1->startproject, edit1->length, edit2->startproject, edit2->length);
		edit1->length = start - edit1->startproject;
		edit2->length -= end - edit2->startproject;
		edit2->startsource += end - edit2->startproject;
		edit2->startproject += end - edit2->startproject;

// delete
		for(current_edit = edit1->next; current_edit && current_edit != edit2;) {
			Edit* next = current_edit->next;
			remove(current_edit);
			current_edit = next;
		}
// shift
		for(current_edit = edit2; current_edit; current_edit = current_edit->next) {
			current_edit->startproject -= end - start;
		}
	}
	else {
// in same edit. paste_edit depends on this
// create a new edit
		current_edit = split_edit(start);
		if( current_edit ) {
			current_edit->length -= end - start;
			current_edit->startsource += end - start;
// shift
			while( (current_edit=current_edit->next) != 0 ) {
				current_edit->startproject -= end - start;
			}
		}
	}

	optimize();
}

// Used by edit handle and plugin handle movement but plugin handle movement
// can only effect other plugins.
void Edits::clear_recursive(int64_t start, int64_t end,
	int edit_edits, int edit_labels, int edit_plugins, int edit_autos,
	Edits *trim_edits)
{
//printf("Edits::clear_recursive 1\n");
	track->clear(start, end,
		edit_edits, edit_labels, edit_plugins, edit_autos, trim_edits);
}


int Edits::clear_handle(double start, double end,
	int edit_plugins, int edit_autos, double &distance)
{
	Edit *current_edit;

	distance = 0.0; // if nothing is found, distance is 0!
	for(current_edit = first;
		current_edit && current_edit->next;
		current_edit = current_edit->next) {



		if(current_edit->asset && current_edit->next->asset) {

			if(current_edit->asset->equivalent(*current_edit->next->asset, 0, 0, edl)) {

// Got two consecutive edits in same source
				if(edl->equivalent(track->from_units(current_edit->next->startproject),
					start)) {
// handle selected
					int length = -current_edit->length;
					current_edit->length = current_edit->next->startsource - current_edit->startsource;
					length += current_edit->length;

// Lengthen automation
					if(edit_autos)
						track->automation->paste_silence(current_edit->next->startproject,
							current_edit->next->startproject + length);

// Lengthen effects
					if(edit_plugins)
						track->shift_effects(current_edit->next->startproject,
								length, edit_autos, 0);

					for(current_edit = current_edit->next; current_edit; current_edit = current_edit->next)
					{
						current_edit->startproject += length;
					}

					distance = track->from_units(length);
					optimize();
					break;
				}
			}
		}
	}

	return 0;
}

int Edits::modify_handles(double oldposition, double newposition, int currentend,
	int edit_mode, int edit_edits, int edit_labels, int edit_plugins, int edit_autos,
	Edits *trim_edits, int group_id)
{
	int result = 0;
	Edit *current_edit;
	Edit *left = 0, *right = 0;
	if( group_id > 0 ) {
		double start = DBL_MAX, end = DBL_MIN;
		for( Edit *edit=first; edit; edit=edit->next ) {
			if( edit->group_id != group_id ) continue;
			double edit_start = edit->track->from_units(edit->startproject);
			if( edit_start < start ) { start = edit_start;  left = edit; }
			double edit_end = edit->track->from_units(edit->startproject+edit->length);
			if( edit_end > end ) { end = edit_end;  right = edit; }
		}
	}

//printf("Edits::modify_handles 1 %d %f %f\n", currentend, newposition, oldposition);
	if(currentend == 0) {
// left handle
		for(current_edit = first; current_edit && !result;) {
			if( group_id > 0 ? current_edit == left :
			    edl->equivalent(track->from_units(current_edit->startproject),
				oldposition) ) {
// edit matches selection
//printf("Edits::modify_handles 3 %f %f\n", newposition, oldposition);
				double delta = newposition - oldposition;
				oldposition = track->from_units(current_edit->startproject);
				if( group_id > 0 ) newposition = oldposition + delta;
				current_edit->shift_start(edit_mode,
					track->to_units(newposition, 0), track->to_units(oldposition, 0),
					edit_labels, edit_autos, edit_plugins, trim_edits);
				result = 1;
			}

			if(!result) current_edit = current_edit->next;
		}
	}
	else {
// right handle selected
		for(current_edit = first; current_edit && !result;) {
			if( group_id > 0 ? current_edit == right :
			    edl->equivalent(track->from_units(current_edit->startproject) +
					track->from_units(current_edit->length), oldposition) ) {
				double delta = newposition - oldposition;
				oldposition = track->from_units(current_edit->startproject) +
					track->from_units(current_edit->length);
				if( group_id > 0 ) newposition = oldposition + delta;
				result = 1;

				current_edit->shift_end(edit_mode,
					track->to_units(newposition, 0), track->to_units(oldposition, 0),
					edit_labels, edit_autos, edit_plugins, trim_edits);
			}

			if(!result) current_edit = current_edit->next;
//printf("Edits::modify_handle 8\n");
		}
	}

	optimize();
	return 0;
}

void Edits::paste_silence(int64_t start, int64_t end)
{
	Edit *new_edit = editof(start, PLAY_FORWARD, 0);
	if (!new_edit) return;

	if( !new_edit->silence() || new_edit->hard_right ) {
		new_edit = insert_new_edit(start);
		new_edit->length = end - start;
	}
	else
		new_edit->length += end - start;

	for(Edit *current = new_edit->next; current; current = NEXT) {
		current->startproject += end - start;
	}

	return;
}

Edit *Edits::create_silence(int64_t start, int64_t end)
{
	Edit *new_edit = insert_new_edit(start);
	new_edit->length = end - start;
	for(Edit *current = new_edit->next; current; current = NEXT) {
		current->startproject += end - start;
	}
	return new_edit;
}

Edit* Edits::shift(int64_t position, int64_t difference)
{
	Edit *new_edit = split_edit(position);

	for(Edit *current = first; current; current = NEXT) {
		if(current->startproject >= position) {
			current->shift(difference);
		}
	}
	return new_edit;
}


void Edits::shift_keyframes_recursive(int64_t position, int64_t length)
{
	track->shift_keyframes(position, length);
}

void Edits::shift_effects_recursive(int64_t position, int64_t length, int edit_autos)
{
	track->shift_effects(position, length, edit_autos, 0);
}

