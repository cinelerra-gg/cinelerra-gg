
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


// Inherited by objects which can be indexed by IndexFile

#ifndef INDEXABLE_H
#define INDEXABLE_H

#include "bcwindowbase.inc"
#include "garbage.h"
#include "indexstate.h"
#include <stdint.h>

class Indexable : public Garbage
{
public:
	Indexable(int is_asset);
	virtual ~Indexable();

	virtual int get_audio_channels();
	virtual int get_sample_rate();
	virtual int64_t get_audio_samples();
	virtual int have_audio();

	virtual int get_w();
	virtual int get_h();
	virtual double get_frame_rate();
	virtual int have_video();
	virtual int get_video_layers();
	virtual int64_t get_video_frames();

        void copy_indexable(Indexable *src);
	void update_path(const char *new_path);
	void update_index(Indexable *src);
	const char *get_title();

	IndexState *index_state;

// In an Asset, the path to the file.

// In the top EDL, this is the path it was loaded from.  Restores
// project titles from backups.  This is only used for loading backups & nested EDLs.
// All other loads keep the path in mainsession->filename.
// This can't use the output_path argument to save_xml because that points
// to the backup file, not the project file.
	char path[BCTEXTLEN];
// Folder in resource manager
	int folder_no;

	int is_asset;
// unique ID of this object for comparison
	int id;
};



#endif


