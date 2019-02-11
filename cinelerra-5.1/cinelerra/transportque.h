
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

#ifndef __TRANSPORTQUE_H__
#define __TRANSPORTQUE_H__

#include "canvas.inc"
#include "condition.inc"
#include "edl.inc"
#include "playbackengine.inc"
#include "transportque.inc"

class TransportCommand
{
public:
	TransportCommand();
	~TransportCommand();

	void reset();
	void copy_from(TransportCommand *command);
	TransportCommand& operator=(TransportCommand &command);
// Get the range to play back from the EDL
	void set_playback_range(EDL *edl, int use_inout, int do_displacement);
	static int single_frame(int command);
	static int get_direction(int command);
	static float get_speed(int command, float speed=0);

// Adjust playback range with in/out points for rendering
	void playback_range_adjust_inout();
// Set playback range to in/out points for rendering
	void playback_range_inout();
// Set playback range to whole project for rendering
	void playback_range_project();
	void playback_range_1frame();

	EDL* get_edl();
	void delete_edl();
	void new_edl();

	int command;
	int change_type;
// playback range
	double start_position, end_position;
	int infinite;
// Position used when starting playback
	double playbackstart;
// start at this=0/next=1 frame
	int displacement;
// Send output to device
	int realtime;
// command must execute
	int locked;
// Use persistant starting point
	int resume;
// reverse audio duty
	int toggle_audio;
// playback loop
	int loop_play;
// SLOW,FAST play speed
	float speed;

	int single_frame() { return single_frame(command); }
	int get_direction() { return get_direction(command); }
	float get_speed() { return get_speed(command, speed); }
	void set_playback_range() { set_playback_range(0, 0, 0); }
private:
// Copied to render engines
	EDL *edl;
};

#endif
