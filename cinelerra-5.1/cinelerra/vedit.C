
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

#include "asset.h"
#include "bcsignals.h"
#include "cache.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "mwindow.h"
#include "patch.h"
#include "playabletracks.h"
#include "preferences.h"
#include "mainsession.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "units.h"
#include "vedit.h"
#include "vedits.h"
#include "vframe.h"
#include "vtrack.h"

VEdit::VEdit(EDL *edl, Edits *edits)
 : Edit(edl, edits)
{
}


VEdit::~VEdit() { }

int VEdit::load_properties_derived(FileXML *xml)
{
	channel = xml->tag.get_property("CHANNEL", (int64_t)0);
	return 0;
}

Asset* VEdit::get_nested_asset(int64_t *source_position,
		int64_t position, int direction)
{
	double edit_frame_rate = nested_edl ?
		nested_edl->session->frame_rate : asset->frame_rate;
// Make position relative to edit
	double edit_position = (position - startproject + startsource) *
			edit_frame_rate / edl->session->frame_rate;
	*source_position = Units::to_int64(edit_position);
	if( !nested_edl ) return asset;

// Descend into nested EDLs
	PlayableTracks playable_tracks(nested_edl,
		*source_position, direction, TRACK_VIDEO, 1);
	if( !playable_tracks.size() ) return 0;
	VTrack *nested_track = (VTrack*)playable_tracks[0];
	VEdit* nested_edit = (VEdit*)nested_track->edits->
		editof(*source_position, direction, 1);
	if( !nested_edit ) return  0;
	return nested_edit->get_nested_asset(source_position,
		*source_position, direction);
}

int VEdit::read_frame(VFrame *video_out, int64_t input_position, int direction,
		CICache *cache, int use_nudge, int use_cache, int use_asynchronous)
{
	File *file = 0;
	int result = 0;
	int64_t source_position = 0;
	const int debug = 0;

	if(use_nudge) input_position += track->nudge;
if(debug) printf("VEdit::read_frame %d source_position=%jd input_position=%jd\n",
  __LINE__, source_position, input_position);

	Asset *asset = get_nested_asset(&source_position, input_position, direction);
	if( !asset ) result = 1;

if(debug) printf("VEdit::read_frame %d source_position=%jd input_position=%jd\n",
__LINE__, source_position, input_position);

	if( !result ) {
		file = cache->check_out(asset, edl);
		if( !file ) result = 1;
	}
if(debug) printf("VEdit::read_frame %d path=%s source_position=%jd\n",
__LINE__, asset->path, source_position);

	if( !result ) {
		if(direction == PLAY_REVERSE && source_position > 0)
			--source_position;
		if(use_asynchronous)
			file->start_video_decode_thread();
		else
			file->stop_video_thread();
if(debug) printf("VEdit::read_frame %d\n", __LINE__);

		file->set_layer(channel);
//printf("VEdit::read_frame %d %lld\n", __LINE__, source_position);
		file->set_video_position(source_position, 0);

		if(use_cache) file->set_cache_frames(use_cache);
		result = file->read_frame(video_out);
if(debug) printf("VEdit::read_frame %d\n", __LINE__);
		if(use_cache) file->set_cache_frames(0);

if(debug) printf("VEdit::read_frame %d\n", __LINE__);
		cache->check_in(asset);
if(debug) printf("VEdit::read_frame %d\n", __LINE__);
	}

//for(int i = 0; i < video_out->get_w() * 3 * 20; i++) video_out->get_rows()[0][i] = 128;
	return result;
}

int VEdit::copy_properties_derived(FileXML *xml, int64_t length_in_selection)
{
	return 0;
}

int VEdit::dump_derived()
{
	printf("	VEdit::dump_derived\n");
	printf("		startproject %jd\n", startproject);
	printf("		length %jd\n", length);
	return 0;
}

int64_t VEdit::get_source_end(int64_t default_)
{
	if(!nested_edl && !asset) return default_;   // Infinity

	if( nested_edl ) {
		return (int64_t)(nested_edl->tracks->total_length() *
			edl->session->frame_rate + 0.5);
	}

	return asset->video_length < 0 ? default_ :
		 (int64_t)((double)asset->video_length /
			asset->frame_rate * edl->session->frame_rate + 0.5);
}
