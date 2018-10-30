
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

#ifndef KEYFRAMES_H
#define KEYFRAMES_H

#include <stdio.h>
#include <stdint.h>

#include "autos.h"
#include "filexml.inc"
#include "keyframe.inc"


// Keyframes inherit from Autos to reuse the editing commands but
// keyframes don't belong to tracks.  Instead they belong to plugins
// because their data is specific to plugins.


class KeyFrames : public Autos
{
public:
	KeyFrames(EDL *edl, Track *track);
	~KeyFrames();

// Get keyframes for editing with automatic creation if enabled.
// The direction is always assumed to be forward.
	virtual KeyFrame* get_keyframe();
// Get the previous, first, or default keyframe depending on how many keyframes
// exist.
	KeyFrame* get_prev_keyframe(int64_t position,
		int direction);

// Keyframe updates using rules.
// If a range is selected, the changed parameter is copied to all
// the keyframes.
// If no range is selected, a keyframe is created based on auto keyframe rules.
	void update_parameter(KeyFrame *src);

	Auto* new_auto();
	void dump(FILE *fp=stdout);
};

#endif
