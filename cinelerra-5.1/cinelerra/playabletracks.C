
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

#include "automation.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "patchbay.h"
#include "playabletracks.h"
#include "plugin.h"
#include "preferences.h"
#include "intauto.h"
#include "intautos.h"
#include "tracks.h"
#include "transportque.h"


PlayableTracks::PlayableTracks(EDL *edl,
	int64_t current_position,
	int direction,
	int data_type,
	int use_nudge)
 : ArrayList<Track*>()
{
	this->data_type = data_type;

	for(Track *current_track = edl->tracks->first;
		current_track;
		current_track = current_track->next)
	{
		if(is_playable(current_track, current_position, direction, use_nudge))
		{
// printf("PlayableTracks::PlayableTracks %d this=%p current_track=%p total=%d current_position=%jd\n",
// __LINE__,
// this,
// current_track,
// total,
// current_position);
			append(current_track);
		}
	}
// printf("PlayableTracks::PlayableTracks %d data_type=%d total=%d current_position=%jd\n",
// __LINE__,
// data_type,
// total,
// current_position);
}

PlayableTracks::~PlayableTracks()
{
}


int PlayableTracks::is_playable(Track *current_track,
	int64_t position,
	int direction,
	int use_nudge)
{
	int result = 1;
	if(use_nudge) position += current_track->nudge;
	if(current_track->data_type != data_type) result = 0;

// Track is off screen and not bounced to other modules
	if(result)
	{
		if(!current_track->plugin_used(position, direction) &&
			!current_track->is_playable(position, direction))
			result = 0;
	}

// Test play patch
	if(!current_track->play)
	{
		result = 0;
	}

	if(result)
	{
// Test for playable edit
		if(!current_track->playable_edit(position, direction))
		{
// Test for playable effect
			if(!current_track->is_synthesis(position,
						direction))
			{
				result = 0;
			}
		}
	}

	return result;
}


int PlayableTracks::is_listed(Track *track)
{
	for(int i = 0; i < total; i++)
	{
		if(values[i] == track) return 1;
	}
	return 0;
}
