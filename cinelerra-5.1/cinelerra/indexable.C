
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

#include "bcsignals.h"
#include "edl.h"
#include "indexable.h"
#include "language.h"
#include "localsession.h"

#include <string.h>

Indexable::Indexable(int is_asset) : Garbage(is_asset ? "Asset" : "EDL")
{
	index_state = new IndexState;
	this->is_asset = is_asset;
	this->folder_no = AW_MEDIA_FOLDER;
}


Indexable::~Indexable()
{
	index_state->remove_user();
}

int Indexable::get_audio_channels()
{
	return 0;
}

int Indexable::get_sample_rate()
{
	return 1;
}

int64_t Indexable::get_audio_samples()
{
	return 0;
}

void Indexable::update_path(const char *new_path)
{
	strcpy(path, new_path);
}

void Indexable::update_index(Indexable *src)
{
	if( index_state == src->index_state ) return;
	if( index_state ) index_state->remove_user();
	index_state = src->index_state;
	if( index_state ) index_state->add_user();
}




void Indexable::copy_indexable(Indexable *src)
{
	if( this == src ) return;
	folder_no = src->folder_no;
	update_path(src->path);
	update_index(src);
}

int Indexable::have_audio()
{
	return 0;
}

int Indexable::get_w()
{
	return 0;
}

int Indexable::get_h()
{
	return 0;
}

double Indexable::get_frame_rate()
{
	return 0;
}

int Indexable::have_video()
{
	return 0;
}

int Indexable::get_video_layers()
{
	return 0;
}

int64_t Indexable::get_video_frames()
{
	return 0;
}

const char *Indexable::get_title()
{
	if( is_asset ) return path;
	EDL *edl = (EDL*)this;
	if( !edl->parent_edl || folder_no == AW_PROXY_FOLDER ) return path;
	return edl->local_session->clip_title;
}

