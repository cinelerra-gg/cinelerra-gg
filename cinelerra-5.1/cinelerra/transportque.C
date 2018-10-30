
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

#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "tracks.h"
#include "transportque.h"

TransportCommand::TransportCommand()
{
// In rendering we want a master EDL so settings don't get clobbered
// in the middle of a job.
	edl = new EDL;
	edl->create_objects();
	command = 0;
	change_type = 0;
	reset();
}

TransportCommand::~TransportCommand()
{
	edl->Garbage::remove_user();
}

void TransportCommand::reset()
{
	playbackstart = 0;
	start_position = 0;
	end_position = 0;
	infinite = 0;
	realtime = 0;
	resume = 0;
	audio_toggle = 0;
	play_loop = 0;
	displacement = 0;
// Don't reset the change type for commands which don't perform the change
	if(command != STOP) change_type = 0;
	command = COMMAND_NONE;
}

EDL* TransportCommand::get_edl()
{
	return edl;
}

void TransportCommand::delete_edl()
{
	edl->Garbage::remove_user();
	edl = 0;
}

void TransportCommand::new_edl()
{
	edl = new EDL;
	edl->create_objects();
}


void TransportCommand::copy_from(TransportCommand *command)
{
	this->command = command->command;
	this->change_type = command->change_type;
	this->edl->copy_all(command->edl);
	this->start_position = command->start_position;
	this->end_position = command->end_position;
	this->playbackstart = command->playbackstart;
	this->realtime = command->realtime;
	this->resume = command->resume;
	this->audio_toggle = command->audio_toggle;
	this->play_loop = command->play_loop;
	this->displacement = command->displacement;
}

TransportCommand& TransportCommand::operator=(TransportCommand &command)
{
	copy_from(&command);
	return *this;
}

int TransportCommand::single_frame(int command)
{
	return (command == SINGLE_FRAME_FWD || command == SINGLE_FRAME_REWIND ||
		command == CURRENT_FRAME || command == LAST_FRAME);
}
int TransportCommand::single_frame()
{
	return single_frame(command);
}


int TransportCommand::get_direction(int command)
{
	switch(command) {
	case SINGLE_FRAME_FWD:
	case NORMAL_FWD:
	case FAST_FWD:
	case SLOW_FWD:
	case CURRENT_FRAME:
		return PLAY_FORWARD;

	case SINGLE_FRAME_REWIND:
	case NORMAL_REWIND:
	case FAST_REWIND:
	case SLOW_REWIND:
	case LAST_FRAME:
		return PLAY_REVERSE;

	default:
		break;
	}
	return PLAY_FORWARD;
}
int TransportCommand::get_direction()
{
	return get_direction(command);
}

float TransportCommand::get_speed(int command)
{
	switch(command) {
	case SLOW_FWD:
	case SLOW_REWIND:
		return 0.5;

	case NORMAL_FWD:
	case NORMAL_REWIND:
	case SINGLE_FRAME_FWD:
	case SINGLE_FRAME_REWIND:
	case CURRENT_FRAME:
	case LAST_FRAME:
		return 1.;

	case FAST_FWD:
	case FAST_REWIND:
		return 2.;
	}

	return 0.;
}
float TransportCommand::get_speed()
{
	return get_speed(command);
}

// Assume starting without pause
void TransportCommand::set_playback_range(EDL *edl,
	int use_inout, int toggle_audio, int loop_play, int use_displacement)
{
	if(!edl) edl = this->edl;
	double length = edl->tracks->total_playable_length();
	double frame_period = 1.0 / edl->session->frame_rate;

	displacement = 0;
	audio_toggle = toggle_audio;
	play_loop = loop_play;

	start_position = use_inout && edl->local_session->inpoint_valid() ?
		edl->local_session->get_inpoint() :
		!loop_play ? edl->local_session->get_selectionstart(1) : 0;
	end_position = use_inout && edl->local_session->outpoint_valid() ?
		edl->local_session->get_outpoint() :
		!loop_play ? edl->local_session->get_selectionend(1) : length;

	if( !use_inout && EQUIV(start_position, end_position) ) {
// starting play at or past end_position, play to end_position of media (for mixers)
		if( start_position >= length )
			length = edl->tracks->total_length();
		switch( command ) {
		case SLOW_FWD:
		case FAST_FWD:
		case NORMAL_FWD: {
			end_position = length;
// this prevents a crash if start_position position is after the loop when playing forwards
			if( edl->local_session->loop_playback &&
			    start_position > edl->local_session->loop_end ) {
				start_position = edl->local_session->loop_start;
			}
			break; }

		case SLOW_REWIND:
		case FAST_REWIND:
		case NORMAL_REWIND:
			start_position = 0;
// this prevents a crash if start_position position is before the loop when playing backwards
			if( edl->local_session->loop_playback &&
			    end_position <= edl->local_session->loop_start ) {
					end_position = edl->local_session->loop_end;
			}
			break;

		case CURRENT_FRAME:
		case LAST_FRAME:
		case SINGLE_FRAME_FWD:
			end_position = start_position + frame_period;
			break;

		case SINGLE_FRAME_REWIND:
			start_position = end_position - frame_period;
			break;
		}

		if( use_displacement && (
		    (command != CURRENT_FRAME && get_direction() == PLAY_FORWARD ) ||
		    (command != LAST_FRAME    && get_direction() == PLAY_REVERSE ) ) ) {
			start_position += frame_period;
			end_position += frame_period;
			displacement = 1;
		}
	}

	if( end_position < start_position )
		end_position = start_position;

	playbackstart = get_direction() == PLAY_FORWARD ?
		start_position : end_position;
}

void TransportCommand::playback_range_adjust_inout()
{
	if(edl->local_session->inpoint_valid() ||
		edl->local_session->outpoint_valid())
	{
		playback_range_inout();
	}
}

void TransportCommand::playback_range_inout()
{
	if(edl->local_session->inpoint_valid())
		start_position = edl->local_session->get_inpoint();
	else
		start_position = 0;

	if(edl->local_session->outpoint_valid())
		end_position = edl->local_session->get_outpoint();
	else {
		end_position = edl->tracks->total_playable_length();
		if( start_position >= end_position )
			end_position = edl->tracks->total_length();
	}
}

void TransportCommand::playback_range_project()
{
	start_position = 0;
	end_position = edl->tracks->total_playable_length();
}

void TransportCommand::playback_range_1frame()
{
	start_position = end_position = edl->local_session->get_selectionstart(1);
	if( edl->session->frame_rate > 0 ) end_position += 1./edl->session->frame_rate;
}


TransportQue::TransportQue()
{
	input_lock = new Condition(1, "TransportQue::input_lock");
	output_lock = new Condition(0, "TransportQue::output_lock", 1);
}

TransportQue::~TransportQue()
{
	delete input_lock;
	delete output_lock;
}

int TransportQue::send_command(int command, int change_type,
		EDL *new_edl, int realtime, int resume, int use_inout,
		int toggle_audio, int loop_play, int use_displacement)
{
	input_lock->lock("TransportQue::send_command 1");
	this->command.command = command;
// Mutually exclusive operation
	this->command.change_type |= change_type;
	this->command.realtime = realtime;
	this->command.resume = resume;

	if(new_edl)
	{
// Just change the EDL if the change requires it because renderengine
// structures won't point to the new EDL otherwise and because copying the
// EDL for every cursor movement is slow.
		if(change_type == CHANGE_EDL ||
			(uint32_t)change_type == CHANGE_ALL)
		{
// Copy EDL
			this->command.get_edl()->copy_all(new_edl);
		}
		else
		if(change_type == CHANGE_PARAMS)
		{
			this->command.get_edl()->synchronize_params(new_edl);
		}

// Set playback range
		this->command.set_playback_range(new_edl, use_inout,
				toggle_audio, loop_play, use_displacement);
	}

	input_lock->unlock();

	output_lock->unlock();
	return 0;
}

void TransportQue::update_change_type(int change_type)
{
	input_lock->lock("TransportQue::update_change_type");
	this->command.change_type |= change_type;
	input_lock->unlock();
}
