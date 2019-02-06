
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
#include "playbackengine.h"
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
	toggle_audio = 0;
	loop_play = 0;
	speed = 0;
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
	this->toggle_audio = command->toggle_audio;
	this->loop_play = command->loop_play;
	this->displacement = command->displacement;
	this->speed = command->speed;
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

float TransportCommand::get_speed(int command, float speed)
{
	switch(command) {
	case SLOW_FWD:
	case SLOW_REWIND:
		return speed ? speed : 0.5;

	case NORMAL_FWD:
	case NORMAL_REWIND:
	case SINGLE_FRAME_FWD:
	case SINGLE_FRAME_REWIND:
	case CURRENT_FRAME:
	case LAST_FRAME:
		return 1.;

	case FAST_FWD:
	case FAST_REWIND:
		return speed ? speed : 2.;
	}

	return 0.;
}

// Assume starting without pause
void TransportCommand::set_playback_range(EDL *edl, int use_inout, int do_displacement)
{
	if( !edl ) edl = this->edl;
	double length = edl->tracks->total_playable_length();
	double frame_period = 1.0 / edl->session->frame_rate;
	displacement = 0;

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

		if( realtime && do_displacement ) {
			if( (command != CURRENT_FRAME && get_direction() == PLAY_FORWARD ) ||
			    (command != LAST_FRAME    && get_direction() == PLAY_REVERSE ) ) {
				start_position += frame_period;
				end_position += frame_period;
				displacement = 1;
			}
		}
	}

//	if( start_position < 0 )
//		start_position = 0;
//	if( end_position > length )
//		end_position = length;
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


