
/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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
#include "assets.h"
#include "atrack.h"
#include "autoconf.h"
#include "automation.h"
#include "awindowgui.h"
#include "bccmodels.h"
#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "cstrdup.h"
#include "clipedls.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "guicast.h"
#include "indexstate.h"
#include "interlacemodes.h"
#include "labels.h"
#include "localsession.h"
#include "maskautos.h"
#include "mutex.h"
#include "panauto.h"
#include "panautos.h"
#include "playbackconfig.h"
#include "playabletracks.h"
#include "plugin.h"
#include "pluginset.h"
#include "preferences.h"
#include "recordconfig.h"
#include "recordlabel.h"
#include "sharedlocation.h"
#include "theme.h"
#include "tracks.h"
#include "transportque.inc"
#include "versioninfo.h"
#include "vedit.h"
#include "vtrack.h"




EDL::EDL(EDL *parent_edl)
 : Indexable(0)
{
	this->parent_edl = parent_edl;
	tracks = 0;
	labels = 0;
	local_session = 0;
	id = next_id();
	path[0] = 0;
}


EDL::~EDL()
{

	delete tracks;
	delete labels;
	delete local_session;
	remove_vwindow_edls();
	if( !parent_edl ) {
		delete assets;
		delete session;
	}
}


void EDL::create_objects()
{
	tracks = new Tracks(this);
	assets = !parent_edl ? new Assets(this) : parent_edl->assets;
	session = !parent_edl ? new EDLSession(this) : parent_edl->session;
	local_session = new LocalSession(this);
	labels = new Labels(this, "LABELS");
}

EDL& EDL::operator=(EDL &edl)
{
printf("EDL::operator= 1\n");
	copy_all(&edl);
	return *this;
}

int EDL::load_defaults(BC_Hash *defaults)
{
	if( !parent_edl )
		session->load_defaults(defaults);

	local_session->load_defaults(defaults);
	return 0;
}

int EDL::save_defaults(BC_Hash *defaults)
{
	if( !parent_edl )
		session->save_defaults(defaults);

	local_session->save_defaults(defaults);
	return 0;
}

void EDL::boundaries()
{
	session->boundaries();
	local_session->boundaries();
}

int EDL::create_default_tracks()
{

	for( int i=0; i<session->video_tracks; ++i ) {
		tracks->add_video_track(0, 0);
	}
	for( int i=0; i<session->audio_tracks; ++i ) {
		tracks->add_audio_track(0, 0);
	}
	return 0;
}

int EDL::load_xml(FileXML *file, uint32_t load_flags)
{
	int result = 0;
	folders.clear();

	if( (load_flags & LOAD_ALL) == LOAD_ALL ) {
		remove_vwindow_edls();
	}


// Search for start of master EDL.

// The parent_edl test caused clip creation to fail since those XML files
// contained an EDL tag.

// The parent_edl test is required to make EDL loading work because
// when loading an EDL the EDL tag is already read by the parent.

	if( !parent_edl ) {
		do {
		  result = file->read_tag();
		} while(!result &&
			!file->tag.title_is("XML") &&
			!file->tag.title_is("EDL"));
	}
	return result ? result : read_xml(file, load_flags);
}

int EDL::read_xml(FileXML *file, uint32_t load_flags)
{
	int result = 0;
// Track numbering offset for replacing undo data.
	int track_offset = 0;

// Get path for backups
	file->tag.get_property("path", path);

// Erase everything
	if( (load_flags & LOAD_ALL) == LOAD_ALL ||
		(load_flags & LOAD_EDITS) == LOAD_EDITS ) {
		while(tracks->last) delete tracks->last;
	}

	if( (load_flags & LOAD_ALL) == LOAD_ALL ) {
		clips.clear();
		mixers.remove_all_objects();
	}

	if( load_flags & LOAD_TIMEBAR ) {
		while(labels->last) delete labels->last;
		local_session->unset_inpoint();
		local_session->unset_outpoint();
	}

// This was originally in LocalSession::load_xml
	if( load_flags & LOAD_SESSION ) {
		local_session->clipboard_length = 0;
	}

	do {
		result = file->read_tag();

		if( !result ) {
			if( file->tag.title_is("/XML") ||
				file->tag.title_is("/EDL") ||
				file->tag.title_is("/CLIP_EDL") ||
				file->tag.title_is("/NESTED_EDL") ||
				file->tag.title_is("/VWINDOW_EDL") ) {
				result = 1;
			}
			else
			if( file->tag.title_is("CLIPBOARD") ) {
				local_session->clipboard_length =
					file->tag.get_property("LENGTH", (double)0);
			}
			else
			if( file->tag.title_is("VIDEO") ) {
				if( (load_flags & LOAD_VCONFIG) &&
					(load_flags & LOAD_SESSION) )
					session->load_video_config(file, 0, load_flags);
				else
					result = file->skip_tag();
			}
			else
			if( file->tag.title_is("AUDIO") ) {
				if( (load_flags & LOAD_ACONFIG) &&
					(load_flags & LOAD_SESSION) )
					session->load_audio_config(file, 0, load_flags);
				else
					result = file->skip_tag();
			}
			else
			if( file->tag.title_is("FOLDERS") ) {
				result = folders.load_xml(file);
			}
			else
			if( file->tag.title_is("MIXERS") ) {
				if( (load_flags & LOAD_SESSION) )
					mixers.load(file);
				else
					result = file->skip_tag();
			}
			else
			if( file->tag.title_is("ASSETS") ) {
				if( load_flags & LOAD_ASSETS )
					assets->load(file, load_flags);
				else
					result = file->skip_tag();
			}
			else
			if( file->tag.title_is(labels->xml_tag) ) {
				if( load_flags & LOAD_TIMEBAR )
					labels->load(file, load_flags);
				else
					result = file->skip_tag();
			}
			else
			if( file->tag.title_is("LOCALSESSION") ) {
				if( (load_flags & LOAD_SESSION) ||
					(load_flags & LOAD_TIMEBAR) )
					local_session->load_xml(file, load_flags);
				else
					result = file->skip_tag();
			}
			else
			if( file->tag.title_is("SESSION") ) {
				if( (load_flags & LOAD_SESSION) &&
					!parent_edl )
					session->load_xml(file, 0, load_flags);
				else
					result = file->skip_tag();
			}
			else
			if( file->tag.title_is("TRACK") ) {
				tracks->load(file, track_offset, load_flags);
			}
			else
// Sub EDL.
// Causes clip creation to fail because that involves an opening EDL tag.
			if( file->tag.title_is("CLIP_EDL") && !parent_edl ) {
				EDL *new_edl = new EDL(this);
				new_edl->create_objects();
				new_edl->read_xml(file, LOAD_ALL);
				if( (load_flags & LOAD_ALL) == LOAD_ALL )
					clips.add_clip(new_edl);
				new_edl->remove_user();
			}
			else
			if( file->tag.title_is("NESTED_EDL") ) {
				EDL *nested_edl = new EDL;
				nested_edl->create_objects();
				nested_edl->read_xml(file, LOAD_ALL);
				if( (load_flags & LOAD_ALL) == LOAD_ALL )
					nested_edls.get_nested(nested_edl);
				nested_edl->remove_user();
			}
			else
			if( file->tag.title_is("VWINDOW_EDL") && !parent_edl ) {
				EDL *new_edl = new EDL(this);
				new_edl->create_objects();
				new_edl->read_xml(file, LOAD_ALL);


				if( (load_flags & LOAD_ALL) == LOAD_ALL ) {
//						if( vwindow_edl && !vwindow_edl_shared )
//							vwindow_edl->remove_user();
//						vwindow_edl_shared = 0;
//						vwindow_edl = new_edl;

					append_vwindow_edl(new_edl, 0);

				}
				else
// Discard if not replacing EDL
				{
					new_edl->remove_user();
					new_edl = 0;
				}
			}
		}
	} while(!result);

	boundaries();
//dump();

	return 0;
}

// Output path is the path of the output file if name truncation is desired.
// It is a "" if complete names should be used.
// Called recursively by copy for clips, thus the string can't be terminated.
// The string is not terminated in this call.
int EDL::save_xml(FileXML *file, const char *output_path)
{
	copy(COPY_EDL, file, output_path, 0);
	return 0;
}

int EDL::copy_all(EDL *edl)
{
	if( this == edl ) return 0;
	folder_no = edl->folder_no;
	update_index(edl);
	copy_session(edl);
	copy_assets(edl);
	copy_clips(edl);
	copy_nested(edl);
	copy_mixers(edl);
	tracks->copy_from(edl->tracks);
	labels->copy_from(edl->labels);
	return 0;
}

void EDL::copy_clips(EDL *edl)
{
	if( this == edl ) return;

	remove_vwindow_edls();

//	if( vwindow_edl && !vwindow_edl_shared )
//		vwindow_edl->remove_user();
//	vwindow_edl = 0;
//	vwindow_edl_shared = 0;

	for( int i=0; i<edl->total_vwindow_edls(); ++i ) {
		EDL *new_edl = new EDL(this);
		new_edl->create_objects();
		new_edl->copy_all(edl->get_vwindow_edl(i));
		append_vwindow_edl(new_edl, 0);
	}

	clips.clear();
	for( int i=0; i<edl->clips.size(); ++i ) add_clip(edl->clips[i]);
}

void EDL::copy_nested(EDL *edl)
{
	if( this == edl ) return;
	nested_edls.copy_nested(edl->nested_edls);
}

void EDL::copy_assets(EDL *edl)
{
	if( this == edl ) return;

	if( !parent_edl ) {
		assets->copy_from(edl->assets);
	}
}

void EDL::copy_mixers(EDL *edl)
{
	if( this == edl ) return;
	mixers.copy_from(edl->mixers);
}

void EDL::copy_session(EDL *edl, int session_only)
{
	if( this == edl ) return;

	if( !session_only ) {
		strcpy(this->path, edl->path);
		folders.copy_from(&edl->folders);
	}

	if( !parent_edl ) {
		session->copy(edl->session);
	}

	if( session_only <= 0 ) {
		local_session->copy_from(edl->local_session);
	}
}

int EDL::copy_assets(int copy_flags, double start, double end,
		FileXML *file, const char *output_path)
{
	ArrayList<Asset*> asset_list;
	Track* current;

	file->tag.set_title("ASSETS");
	file->append_tag();
	file->append_newline();

// Copy everything for a save
	if( (copy_flags & COPY_ALL_ASSETS) ) {
		for( Asset *asset=assets->first; asset; asset=asset->next ) {
			asset_list.append(asset);
		}
	}
	if( (copy_flags & COPY_USED_ASSETS) ) {
// Copy just the ones being used.
		for( current = tracks->first; current; current = NEXT ) {
			if( !current->record ) continue;
			current->copy_assets(start, end, &asset_list);
		}
	}

// Paths relativised here
	for( int i=0; i<asset_list.size(); ++i ) {
		asset_list[i]->write(file, 0, output_path);
	}

	file->tag.set_title("/ASSETS");
	file->append_tag();
	file->append_newline();
	file->append_newline();
	return 0;
}


int EDL::copy(int copy_flags, double start, double end,
	FileXML *file, const char *output_path, int rewind_it)
{
	file->tag.set_title("EDL");
	file->tag.set_property("VERSION", CINELERRA_VERSION);
// Save path for restoration of the project title from a backup.
	if( this->path[0] ) file->tag.set_property("PATH", path);
	return copy_xml(copy_flags, start, end, file, "/EDL", output_path, rewind_it);
}
int EDL::copy(int copy_flags, FileXML *file, const char *output_path, int rewind_it)
{
	return copy(copy_flags, 0., tracks->total_length(),
		file, output_path, rewind_it);
}

int EDL::copy_clip(int copy_flags, double start, double end,
	FileXML *file, const char *output_path, int rewind_it)
{
	file->tag.set_title("CLIP_EDL");
	return copy_xml(copy_flags, start, end, file, "/CLIP_EDL", output_path, rewind_it);
}
int EDL::copy_clip(int copy_flags, FileXML *file, const char *output_path, int rewind_it)
{
	return copy_clip(copy_flags, 0., tracks->total_length(),
		file, output_path, rewind_it);
}

int EDL::copy_nested(int copy_flags, double start, double end,
	FileXML *file, const char *output_path, int rewind_it)
{
	file->tag.set_title("NESTED_EDL");
	if( this->path[0] ) file->tag.set_property("PATH", path);
	return copy_xml(copy_flags, start, end, file, "/NESTED_EDL", output_path, rewind_it);
}
int EDL::copy_nested(int copy_flags, FileXML *file, const char *output_path, int rewind_it)
{
	return copy_nested(copy_flags, 0., tracks->total_length(),
		file, output_path, rewind_it);
}

int EDL::copy_vwindow(int copy_flags, double start, double end,
	FileXML *file, const char *output_path, int rewind_it)
{
	file->tag.set_title("VWINDOW_EDL");
	return copy_xml(copy_flags, start, end, file, "/VWINDOW_EDL", output_path, rewind_it);
}
int EDL::copy_vwindow(int copy_flags, FileXML *file, const char *output_path, int rewind_it)
{
	return copy_vwindow(copy_flags, 0., tracks->total_length(),
		file, output_path, rewind_it);
}

int EDL::copy_xml(int copy_flags, double start, double end,
	 FileXML *file, const char *closer, const char *output_path,
	int rewind_it)
{
	file->append_tag();
	file->append_newline();
// Set clipboard samples only if copying to clipboard
	if( (copy_flags & COPY_LENGTH) ) {
		file->tag.set_title("CLIPBOARD");
		file->tag.set_property("LENGTH", end - start);
		file->append_tag();
		file->tag.set_title("/CLIPBOARD");
		file->append_tag();
		file->append_newline();
		file->append_newline();
	}
//printf("EDL::copy 1\n");

// Sessions
	if( (copy_flags & COPY_LOCAL_SESSION) )
		local_session->save_xml(file, start);

// Top level stuff.
// Need to copy all this from child EDL if pasting is desired.

	if( (copy_flags & COPY_SESSION) )
		session->save_xml(file);

	if( (copy_flags & COPY_VIDEO_CONFIG) )
		session->save_video_config(file);

	if( (copy_flags & COPY_AUDIO_CONFIG) )
		session->save_audio_config(file);

	if( (copy_flags & COPY_FOLDERS) )
		folders.save_xml(file);

	if( (copy_flags & (COPY_ALL_ASSETS | COPY_USED_ASSETS)) )
		copy_assets(copy_flags, start, end, file, output_path);

	if( (copy_flags & COPY_NESTED_EDL) ) {
		for( int i=0; i<nested_edls.size(); ++i )
			nested_edls[i]->copy_nested(copy_flags,
				file, output_path, 0);
	}
// Clips
	if( (copy_flags & COPY_CLIPS) ) {
		for( int i=0; i<clips.size(); ++i )
			clips[i]->copy_clip(copy_flags, file, output_path, 0);
	}

	if( (copy_flags & COPY_VWINDOWS) ) {
		for( int i=0; i<total_vwindow_edls(); ++i )
			get_vwindow_edl(i)->copy_vwindow(copy_flags,
				file, output_path, 0);
	}

	if( (copy_flags & COPY_MIXERS) )
		mixers.save(file);

	file->append_newline();
	file->append_newline();

	if( (copy_flags & COPY_LABELS) )
		labels->copy(start, end, file);

	tracks->copy(copy_flags, start, end, file, output_path);

// terminate file
	file->tag.set_title(closer);
	file->append_tag();
	file->append_newline();

// For editing operations we want to rewind it for immediate pasting.
// For clips and saving to disk leave it alone.
	if( rewind_it ) {
		file->terminate_string();
		file->rewind();
	}
	return 0;
}

void EDL::copy_indexables(EDL *edl)
{
	for( Track *track=edl->tracks->first; track; track=track->next ) {
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			if( edit->asset )
				assets->update(edit->asset);
			if( edit->nested_edl )
				nested_edls.get_nested(edit->nested_edl);
		}
	}
}

EDL *EDL::new_nested(EDL *edl, const char *path)
{
	EDL *nested = new EDL;  // no parent for nested edl
	nested->create_objects();
	nested->copy_session(edl);
	nested->set_path(path);
	nested->update_index(edl);
	nested->copy_indexables(edl);
	nested->tracks->copy_from(edl->tracks);
	nested_edls.append(nested);
	return nested;
}

EDL *EDL::create_nested_clip(EDL *nested)
{
	EDL *new_edl = new EDL(this);  // parent for clip edl
	new_edl->create_objects();
	new_edl->create_nested(nested);
	return new_edl;
}

void EDL::create_nested(EDL *nested)
{
// Keep frame rate, sample rate, and output size unchanged.
// Nest all video & audio outputs
	session->video_tracks = 1;
	session->audio_tracks = nested->session->audio_channels;
	create_default_tracks();
	insert_asset(0, nested, 0, 0, 0);
}

void EDL::retrack()
{
	int min_w = session->output_w, min_h = session->output_h;
	for( Track *track=tracks->first; track!=0; track=track->next ) {
		if( track->data_type != TRACK_VIDEO ) continue;
		int w = min_w, h = min_h;
		for( Edit *current=track->edits->first; current!=0; current=NEXT ) {
			Indexable* indexable = current->get_source();
			if( !indexable ) continue;
			int edit_w = indexable->get_w(), edit_h = indexable->get_h();
			if( w < edit_w ) w = edit_w;
			if( h < edit_h ) h = edit_h;
		}
		if( track->track_w == w && track->track_h == h ) continue;
		((MaskAutos*)track->automation->autos[AUTOMATION_MASK])->
			translate_masks( (w - track->track_w) / 2, (h - track->track_h) / 2);
		track->track_w = w;  track->track_h = h;
	}
}

void EDL::rechannel()
{
	for( Track *current=tracks->first; current; current=NEXT ) {
		if( current->data_type == TRACK_AUDIO ) {
			PanAutos *autos = (PanAutos*)current->automation->autos[AUTOMATION_PAN];
			((PanAuto*)autos->default_auto)->rechannel();
			for( PanAuto *keyframe = (PanAuto*)autos->first;
			     keyframe; keyframe = (PanAuto*)keyframe->next ) {
				keyframe->rechannel();
			}
		}
	}
}

void EDL::resample(double old_rate, double new_rate, int data_type)
{
	for( Track *current=tracks->first; current; current=NEXT ) {
		if( current->data_type == data_type ) {
			current->resample(old_rate, new_rate);
		}
	}
}


void EDL::synchronize_params(EDL *edl)
{
	local_session->synchronize_params(edl->local_session);
	for( Track *this_track=tracks->first, *that_track=edl->tracks->first;
	     this_track && that_track;
	     this_track=this_track->next, that_track=that_track->next ) {
		this_track->synchronize_params(that_track);
	}
}

int EDL::trim_selection(double start,
	double end,
	int edit_labels,
	int edit_plugins,
	int edit_autos)
{
	if( start != end ) {
// clear the data
		clear(0,
			start,
			edit_labels,
			edit_plugins,
			edit_autos);
		clear(end - start,
			tracks->total_length(),
			edit_labels,
			edit_plugins,
			edit_autos);
	}
	return 0;
}


int EDL::equivalent(double position1, double position2)
{
	double threshold = session->cursor_on_frames ?
		0.5 / session->frame_rate : 1.0 / session->sample_rate;
	return fabs(position2 - position1) < threshold ? 1 : 0;
}

double EDL::equivalent_output(EDL *edl)
{
	double result = -1;
	session->equivalent_output(edl->session, &result);
	tracks->equivalent_output(edl->tracks, &result);
	return result;
}


void EDL::set_path(const char *path)
{
	if( &this->path[0] == path ) return;
	strcpy(this->path, path);
}

void EDL::set_inpoint(double position)
{
	if( equivalent(local_session->get_inpoint(), position) &&
		local_session->get_inpoint() >= 0 ) {
		local_session->unset_inpoint();
	}
	else {
		local_session->set_inpoint(align_to_frame(position, 0));
		if( local_session->get_outpoint() <= local_session->get_inpoint() )
			local_session->unset_outpoint();
	}
}

void EDL::set_outpoint(double position)
{
	if( equivalent(local_session->get_outpoint(), position) &&
		local_session->get_outpoint() >= 0 ) {
		local_session->unset_outpoint();
	}
	else {
		local_session->set_outpoint(align_to_frame(position, 0));
		if( local_session->get_inpoint() >= local_session->get_outpoint() )
			local_session->unset_inpoint();
	}
}

void EDL::unset_inoutpoint()
{
	local_session->unset_inpoint();
	local_session->unset_outpoint();
}

int EDL::blade(double position)
{
	return tracks->blade(position);
}

int EDL::clear(double start, double end,
	int clear_labels, int clear_plugins, int edit_autos)
{
	if( start == end ) {
		double distance = 0;
		tracks->clear_handle(start,
			end,
			distance,
			clear_labels,
			clear_plugins,
			edit_autos);
		if( clear_labels && distance > 0 )
			labels->paste_silence(start,
				start + distance);
	}
	else {
		tracks->clear(start,
			end,
			clear_plugins,
			edit_autos);
		if( clear_labels )
			labels->clear(start,
				end,
				1);
	}

// Need to put at beginning so a subsequent paste operation starts at the
// right position.
	double position = local_session->get_selectionstart();
	local_session->set_selectionend(position);
	local_session->set_selectionstart(position);
	return 0;
}

static int dead_edit_cmp(Edit**ap, Edit**bp)
{
	Edit *a = *ap, *b = *bp;
	if( a->track != b->track ) return 0;
	return a->startproject > b->startproject ? -1 : 1;
}

void EDL::delete_edits(ArrayList<Edit*> *edits, int collapse)
{
	edits->sort(dead_edit_cmp);
	for( int i=0; i<edits->size(); ++i ) {
		Edit *edit = edits->get(i);
		Track *track = edit->track;
		int64_t start = edit->startproject;
		int64_t length = edit->length;
		int64_t end = start + length;
		if( session->autos_follow_edits ) {
			track->automation->clear(start, end, 0, collapse);
		}
		if( session->plugins_follow_edits ) {
			for( int k=0; k<track->plugin_set.size(); ++k ) {
				PluginSet *plugin_set = track->plugin_set[k];
				plugin_set->clear(start, end, 1);
				if( !collapse )
					plugin_set->paste_silence(start, end, 1);
				plugin_set->optimize();
			}
		}
		Edit *dead_edit = edit;
		if( collapse ) {
			while( (edit=edit->next) )
				edit->startproject -= length;
		}
		delete dead_edit;
	}
	optimize();
}

class Range {
public:
	static int cmp(Range *ap, Range *bp);
	double start, end;
	bool operator ==(Range &that) { return this->start == that.start; }
	bool operator >(Range &that) { return this->start > that.start; }
};
int Range::cmp(Range *ap, Range *bp) {
	return ap->start < bp->start ? -1 : ap->start == bp->start ? 0 : 1;
}

static void get_edit_regions(ArrayList<Edit*> *edits, ArrayList<Range> &regions)
{
// move edit inclusive labels by regions
	for( int i=0; i<edits->size(); ++i ) {
		Edit *edit = edits->get(i);
		double pos = edit->track->from_units(edit->startproject);
		double end = edit->track->from_units(edit->startproject + edit->length);
		int n = regions.size(), k = n;
		while( --k >= 0 ) {
			Range &range = regions[k];
			if( pos >= range.end ) continue;
			if( range.start >= end ) continue;
			int expand = 0;
			if( range.start > pos ) { range.start = pos;  expand = 1; }
			if( range.end < end ) { range.end = end;  expand = 1; }
			if( !expand ) break;
			k = n;
		}
		if( k < 0 ) {
			Range &range = regions.append();
			range.start = pos;  range.end = end;
		}
	}
	regions.sort(Range::cmp);
}

void EDL::delete_edit_labels(ArrayList<Edit*> *edits, int collapse)
{
	ArrayList<Range> regions;
	get_edit_regions(edits, regions);
	int n = regions.size(), k = n;
	while( --k >= 0 ) {
		Range &range = regions[k];
		labels->clear(range.start, range.end, collapse);
	}
}

void EDL::move_edit_labels(ArrayList<Edit*> *edits, double dist)
{
	ArrayList<Range> regions;
	get_edit_regions(edits, regions);
	int n = regions.size(), k = n;
	Labels moved(this, 0);
	while( --k >= 0 ) {
		Range &range = regions[k];
		Label *label = labels->label_of(range.start);
		for( Label *next=0; label && label->position <= range.end; label=next ) {
			next = label->next;
			labels->remove_pointer(label);
			label->position += dist;
			moved.append(label);
		}
		Label *current = labels->first;
		while( (label=moved.first) ) {
			moved.remove_pointer(label);
			while( current && current->position < label->position )
				current = current->next;
			if( current && current->position == label->position ) {
				delete label;  continue;
			}
			labels->insert_before(current, label);
		}
	}
}

void EDL::modify_edithandles(double oldposition, double newposition,
	int currentend, int handle_mode, int edit_labels,
	int edit_plugins, int edit_autos, int group_id)
{
	tracks->modify_edithandles(oldposition, newposition,
		currentend, handle_mode, edit_labels,
		edit_plugins, edit_autos, group_id);
}

void EDL::modify_pluginhandles(double oldposition, double newposition,
	int currentend, int handle_mode, int edit_labels,
	int edit_autos, Edits *trim_edits)
{
	tracks->modify_pluginhandles(oldposition, newposition,
		currentend, handle_mode, edit_labels,
		edit_autos, trim_edits);
	optimize();
}

void EDL::paste_silence(double start, double end,
	int edit_labels, int edit_plugins, int edit_autos)
{
	if( edit_labels )
		labels->paste_silence(start, end);
	tracks->paste_silence(start, end, edit_plugins, edit_autos);
}


void EDL::remove_from_project(ArrayList<EDL*> *clips)
{
	for( int i=0; i<clips->size(); ++i ) {
		this->clips.remove_clip(clips->get(i));
	}
}

void EDL::remove_from_project(ArrayList<Indexable*> *assets)
{
// Remove from clips
	if( !parent_edl )
		for( int j=0; j<clips.size(); ++j ) {
			clips[j]->remove_from_project(assets);
		}

// Remove from VWindow EDLs
	for( int i=0; i<total_vwindow_edls(); ++i )
		get_vwindow_edl(i)->remove_from_project(assets);

	for( int i=0; i<assets->size(); ++i ) {
// Remove from tracks
		for( Track *track=tracks->first; track; track=track->next ) {
			track->remove_asset(assets->get(i));
		}

// Remove from assets
		if( !parent_edl && assets->get(i)->is_asset ) {
			this->assets->remove_asset((Asset*)assets->get(i));
		}
		else
		if( !parent_edl && !assets->get(i)->is_asset ) {
			this->nested_edls.remove_clip((EDL*)assets->get(i));
		}
	}
}

void EDL::update_assets(EDL *src)
{
	for( Asset *current=src->assets->first; current; current=NEXT ) {
		assets->update(current);
	}
}

int EDL::get_tracks_height(Theme *theme)
{
	int total_pixels = 0;
	for( Track *current=tracks->first; current; current=NEXT ) {
		total_pixels += current->vertical_span(theme);
	}
	return total_pixels;
}

int64_t EDL::get_tracks_width()
{
	int64_t total_pixels = 0;
	for( Track *current=tracks->first; current; current=NEXT ) {
		int64_t pixels = current->horizontal_span();
		if( pixels > total_pixels ) total_pixels = pixels;
	}
//printf("EDL::get_tracks_width %d\n", total_pixels);
	return total_pixels;
}

// int EDL::calculate_output_w(int single_channel)
// {
// 	if( single_channel ) return session->output_w;
//
// 	int widest = 0;
// 	for( int i=0; i<session->video_channels; ++i )
// 	{
// 		if( session->vchannel_x[i] + session->output_w > widest ) widest = session->vchannel_x[i] + session->output_w;
// 	}
// 	return widest;
// }
//
// int EDL::calculate_output_h(int single_channel)
// {
// 	if( single_channel ) return session->output_h;
//
// 	int tallest = 0;
// 	for( int i=0; i<session->video_channels; ++i )
// 	{
// 		if( session->vchannel_y[i] + session->output_h > tallest ) tallest = session->vchannel_y[i] + session->output_h;
// 	}
// 	return tallest;
// }

// Get the total output size scaled to aspect ratio
void EDL::calculate_conformed_dimensions(int single_channel, float &w, float &h)
{
	if( (float)session->output_w / session->output_h > get_aspect_ratio() )
		h = (w = session->output_w) / get_aspect_ratio();
	else
		w = (h = session->output_h) * get_aspect_ratio();
}

float EDL::get_aspect_ratio()
{
	return session->aspect_w / session->aspect_h;
}

int EDL::dump(FILE *fp)
{
	if( parent_edl )
		fprintf(fp,"CLIP\n");
	else
		fprintf(fp,"EDL\n");
	fprintf(fp,"  clip_title: %s\n"
		"  parent_edl: %p\n", local_session->clip_title, parent_edl);
	fprintf(fp,"  selectionstart %f\n  selectionend %f\n  loop_start %f\n  loop_end %f\n",
		local_session->get_selectionstart(1),
		local_session->get_selectionend(1),
		local_session->loop_start,
		local_session->loop_end);
	for( int i=0; i<TOTAL_PANES; ++i ) {
		fprintf(fp,"  pane %d view_start=%jd track_start=%d\n", i,
			local_session->view_start[i],
			local_session->track_start[i]);
	}

	if( !parent_edl ) {
		fprintf(fp,"audio_channels: %d audio_tracks: %d sample_rate: %jd\n",
			session->audio_channels,
			session->audio_tracks,
			session->sample_rate);
		fprintf(fp,"  video_channels: %d\n"
			"  video_tracks: %d\n"
			"  frame_rate: %.2f\n"
			"  frames_per_foot: %.2f\n"
			"  output_w: %d\n"
			"  output_h: %d\n"
			"  aspect_w: %f\n"
			"  aspect_h: %f\n"
			"  color_model: %d\n",
				session->video_channels,
				session->video_tracks,
				session->frame_rate,
				session->frames_per_foot,
	    			session->output_w,
				session->output_h,
				session->aspect_w,
				session->aspect_h,
				session->color_model);

		fprintf(fp," CLIPS");
		fprintf(fp,"  total: %d\n", clips.size());
		for( int i=0; i<clips.size(); ++i ) {
			fprintf(fp,"\n\n");
			clips[i]->dump(fp);
			fprintf(fp,"\n\n");
		}
		fprintf(fp," NESTED_EDLS");
		fprintf(fp,"  total: %d\n", nested_edls.size());
		for( int i=0; i<nested_edls.size(); ++i )
			fprintf(fp,"   %s\n", nested_edls[i]->path);

		fprintf(fp," VWINDOW EDLS");
		fprintf(fp,"  total: %d\n", total_vwindow_edls());

		for( int i=0; i<total_vwindow_edls(); ++i ) {
			fprintf(fp,"   %s\n", get_vwindow_edl(i)->local_session->clip_title);
		}

		fprintf(fp," ASSETS\n");
		assets->dump(fp);
	}
	fprintf(fp," LABELS\n");
	labels->dump(fp);
	fprintf(fp," TRACKS\n");
	tracks->dump(fp);
//printf("EDL::dump 2\n");
	return 0;
}

EDL* EDL::add_clip(EDL *edl)
{
// Copy argument.  New edls are deleted from MWindow::load_filenames.
	EDL *new_edl = new EDL(this);
	new_edl->create_objects();
	new_edl->copy_all(edl);
	new_edl->folder_no = AW_CLIP_FOLDER;
	clips.append(new_edl);
	return new_edl;
}

void EDL::insert_asset(Asset *asset,
	EDL *nested_edl,
	double position,
	Track *first_track,
	RecordLabels *labels)
{
// Insert asset into asset table
	Asset *new_asset = 0;
	EDL *new_nested_edl = 0;

	if( asset ) new_asset = assets->update(asset);
	if( nested_edl ) new_nested_edl = nested_edls.get_nested(nested_edl);

// Paste video
	int vtrack = 0;
	Track *current = first_track ? first_track : tracks->first;


// Fix length of single frame
	double length = 0.;
	int layers = 0;
	int channels = 0;

	if( new_nested_edl ) {
		length = new_nested_edl->tracks->total_length();
		layers = 1;
		channels = new_nested_edl->session->audio_channels;
	}

	if( new_asset ) {
// Insert 1 frame for undefined length
		if( new_asset->video_length < 0 ) {
			length = session->si_useduration ?
				session->si_duration :
				1.0 / session->frame_rate;
		}
		else {
			length = new_asset->frame_rate > 0 ?
				(double)new_asset->video_length / new_asset->frame_rate :
				1.0 / session->frame_rate;
		}
		layers = new_asset->layers;
		channels = new_asset->channels;
	}

	for( ; current && vtrack<layers; current=NEXT ) {
		if( !current->record || current->data_type != TRACK_VIDEO ) continue;
		current->insert_asset(new_asset, new_nested_edl,
			length, position, vtrack++);
	}

	int atrack = 0;
	if( new_asset ) {
		if( new_asset->audio_length < 0 ) {
// Insert 1 frame for undefined length & video
			if( new_asset->video_data )
				length = (double)1.0 / new_asset->frame_rate;
			else
// Insert 1 second for undefined length & no video
				length = 1.0;
		}
		else
			length = (double)new_asset->audio_length /
					new_asset->sample_rate;
	}

	current = tracks->first;
	for( ; current && atrack < channels; current=NEXT ) {
		if( !current->record || current->data_type != TRACK_AUDIO ) continue;
		current->insert_asset(new_asset, new_nested_edl,
			length, position, atrack++);
	}

// Insert labels from a recording window.
	if( labels ) {
		for( RecordLabel *label=labels->first; label; label=label->next ) {
			this->labels->toggle_label(label->position, label->position);
		}
	}
}



void EDL::set_index_file(Indexable *indexable)
{
	if( indexable->is_asset )
		assets->update_index((Asset*)indexable);
	else
		nested_edls.update_index((EDL*)indexable);
}

void EDL::optimize()
{
//printf("EDL::optimize 1\n");
	double length = tracks->total_length();
	double preview_start = local_session->preview_start;
	double preview_end = local_session->preview_end;
	if( preview_end < 0 || preview_end > length )
		preview_end = length;
	if( preview_start == 0 && preview_end >= length )
		local_session->preview_end = -1;
	if( preview_start > preview_end )
		local_session->preview_start = preview_end;
	for( Track *current=tracks->first; current; current=NEXT )
		current->optimize();
}

int EDL::next_id()
{
	static Mutex id_lock;
	id_lock.lock("EDL::next_id");
	int result = EDLSession::current_id++;
	id_lock.unlock();
	return result;
}

void EDL::get_shared_plugins(Track *source,
	ArrayList<SharedLocation*> *plugin_locations,
	int omit_recordable,
	int data_type)
{
	for( Track *track=tracks->first; track; track=track->next ) {
		if( track->record && omit_recordable ) continue;
		if( track == source || track->data_type != data_type ) continue;
		for( int i=0; i<track->plugin_set.size(); ++i ) {
			Plugin *plugin = track->get_current_plugin(
				local_session->get_selectionstart(1),
				i, PLAY_FORWARD, 1, 0);
			if( plugin && plugin->plugin_type != PLUGIN_STANDALONE ) continue;
			plugin_locations->append(new SharedLocation(tracks->number_of(track), i));
		}
	}
}

void EDL::get_shared_tracks(Track *track,
	ArrayList<SharedLocation*> *module_locations,
	int omit_recordable, int data_type)
{
	for( Track *current=tracks->first; current; current=NEXT ) {
		if( omit_recordable && current->record ) continue;
		if( current == track || current->data_type != data_type ) continue;
		module_locations->append(new SharedLocation(tracks->number_of(current), 0));
	}
}

// aligned frame time, account for sample truncation
double EDL::frame_align(double position, int round)
{
	if( !round && session->sample_rate > 0 ) {
		int64_t sample_pos = position * session->sample_rate;
		position = (sample_pos+2.) / session->sample_rate;
	}
	int64_t frame_pos = (position * session->frame_rate + (round ? 0.5 : 1e-6));
	return frame_pos / session->frame_rate;
}

// Convert position to frames if alignment is enabled.
double EDL::align_to_frame(double position, int round)
{
	if( session->cursor_on_frames )
		position = frame_align(position, round);
	return position;
}


BinFolder *EDL::get_folder(int no)
{
	for( int i=0; i<folders.size(); ++i ) {
		BinFolder *fp = folders[i];
		if( no == fp->awindow_folder ) return fp;
	}
	return 0;
}

int EDL::get_folder_number(const char *title)
{
	for( int i=0; i<AWINDOW_FOLDERS; ++i ) {
		if( !strcmp(title, AWindowGUI::folder_names[i]) )
			return i;
	}
	for( int i=0; i<folders.size(); ++i ) {
		if( !strcmp(title, folders[i]->title) )
			return folders[i]->awindow_folder;
	}
        return AW_NO_FOLDER;
}

const char *EDL::get_folder_name(int no)
{
	if( no >= 0 && no<AWINDOW_FOLDERS )
		return AWindowGUI::folder_names[no];
	BinFolder *fp = get_folder(no);
	return !fp ? "" : fp->title;
}

int EDL::new_folder(const char *title, int is_clips)
{
	if( !title[0] ) return 1;
	int ret = get_folder_number(title);
	if( ret >= 0 ) return 1;
	int idx = AWINDOW_FOLDERS;
	for( int i=0; i<folders.size(); ++i ) {
		BinFolder *fp = folders[i];
		int no = fp->awindow_folder;
		if( no >= idx ) idx = no+1;
	}
	folders.append(new BinFolder(idx, is_clips, title));
	return 0;
}

int EDL::delete_folder(const char *title)
{
	int k = folders.size();
	while( --k >= 0 && strcmp(title, folders[k]->title) );
	if( k >= 0 )
		folders.remove_object_number(k);
	return k;
}

int EDL::get_use_vconsole(VEdit* *playable_edit,
	int64_t position, int direction, PlayableTracks *playable_tracks)
{
	int share_playable_tracks = 1;
	int result = 0;
	VTrack *playable_track = 0;
	const int debug = 0;
	*playable_edit = 0;

// Calculate playable tracks when being called as a nested EDL
	if( !playable_tracks ) {
		share_playable_tracks = 0;
		playable_tracks = new PlayableTracks(this,
			position, direction, TRACK_VIDEO, 1);
	}


// Total number of playable tracks is 1
	if( playable_tracks->size() != 1 ) {
		result = 1;
	}
	else {
		playable_track = (VTrack*)playable_tracks->get(0);
	}

// Don't need playable tracks anymore
	if( !share_playable_tracks ) {
		delete playable_tracks;
	}

if( debug ) printf("EDL::get_use_vconsole %d playable_tracks->size()=%d\n",
 __LINE__, playable_tracks->size());
	if( result ) return 1;


// Test mutual conditions between direct copy rendering and this.
	if( !playable_track->direct_copy_possible(position,
		direction,
		1) )
		return 1;
if( debug ) printf("EDL::get_use_vconsole %d\n", __LINE__);

	*playable_edit = (VEdit*)playable_track->edits->editof(position,
		direction, 0);
// No edit at current location
	if( !*playable_edit ) return 1;
if( debug ) printf("EDL::get_use_vconsole %d\n", __LINE__);


// Edit is nested EDL
	if( (*playable_edit)->nested_edl ) {
// Test nested EDL
		EDL *nested_edl = (*playable_edit)->nested_edl;
		int64_t nested_position = (int64_t)((position -
				(*playable_edit)->startproject +
				(*playable_edit)->startsource) *
			nested_edl->session->frame_rate /
			session->frame_rate);


		VEdit *playable_edit_temp = 0;
		if( session->output_w != nested_edl->session->output_w ||
			session->output_h != nested_edl->session->output_h ||
			nested_edl->get_use_vconsole(&playable_edit_temp,
				nested_position,
				direction,
				0) )
			return 1;

		return 0;
	}

if( debug ) printf("EDL::get_use_vconsole %d\n", __LINE__);
// Edit is not a nested EDL
	Asset *asset = (*playable_edit)->asset;
// Edit is silence
	if( !asset ) return 1;
if( debug ) printf("EDL::get_use_vconsole %d\n", __LINE__);

// Asset and output device must have the same dimensions
	if( asset->width != session->output_w ||
	    asset->height != session->output_h )
		return 1;


if( debug ) printf("EDL::get_use_vconsole %d\n", __LINE__);
// Asset and output device must have same resulting de-interlacing method
	if( ilaceautofixmethod2(session->interlace_mode,
	    asset->interlace_autofixoption, asset->interlace_mode,
	    asset->interlace_fixmethod) != ILACE_FIXMETHOD_NONE )
		return 1;

// If we get here the frame is going to be directly copied.  Whether it is
// decompressed in hardware depends on the colormodel.
	return 0;
}


// For Indexable
int EDL::get_audio_channels()
{
	return session->audio_channels;
}

int EDL::get_sample_rate()
{
	return session->sample_rate;
}

int64_t EDL::get_audio_samples()
{
	return (int64_t)(tracks->total_length() *
		session->sample_rate);
}

int EDL::have_audio()
{
	return 1;
}

int EDL::have_video()
{
	return 1;
}


int EDL::get_w()
{
	return session->output_w;
}

int EDL::get_h()
{
	return session->output_h;
}

double EDL::get_frame_rate()
{
	return session->frame_rate;
}

int EDL::get_video_layers()
{
	return 1;
}

int64_t EDL::get_video_frames()
{
	return (int64_t)(tracks->total_length() *
		session->frame_rate);
}

void EDL::remove_vwindow_edls()
{
	for( int i=0; i<total_vwindow_edls(); ++i ) {
		get_vwindow_edl(i)->remove_user();
	}
	vwindow_edls.remove_all();
}

void EDL::remove_vwindow_edl(EDL *edl)
{
	if( vwindow_edls.number_of(edl) >= 0 ) {
		edl->remove_user();
		vwindow_edls.remove(edl);
	}
}


EDL* EDL::get_vwindow_edl(int number)
{
	return vwindow_edls.get(number);
}

int EDL::total_vwindow_edls()
{
	return vwindow_edls.size();
}

void EDL::append_vwindow_edl(EDL *edl, int increase_counter)
{
	if(vwindow_edls.number_of(edl) >= 0) return;

	if(increase_counter) edl->add_user();
	vwindow_edls.append(edl);
}


double EDL::next_edit(double position)
{
	Units::fix_double(&position);
	double new_position = tracks->total_length();

	double max_rate = get_frame_rate();
	int sample_rate = get_sample_rate();
	if( sample_rate > max_rate ) max_rate = sample_rate;
	double min_movement = max_rate > 0 ? 1. / max_rate : 1e-6;

// Test for edit handles after position
	for( Track *track=tracks->first; track; track=track->next ) {
		if( !track->record ) continue;
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			double edit_end = track->from_units(edit->startproject + edit->length);
			Units::fix_double(&edit_end);
			if( fabs(edit_end-position) < min_movement ) continue;
			if( edit_end > position && edit_end < new_position )
				new_position = edit_end;
		}
	}
	return new_position;
}

double EDL::prev_edit(double position)
{
	Units::fix_double(&position);
	double new_position = -1;

	double max_rate = get_frame_rate();
	int sample_rate = get_sample_rate();
	if( sample_rate > max_rate ) max_rate = sample_rate;
	double min_movement = max_rate > 0 ? 1. / max_rate : 1e-6;

// Test for edit handles before cursor position
	for( Track *track=tracks->first; track; track=track->next ) {
		if( !track->record ) continue;
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			double edit_end = track->from_units(edit->startproject);
			Units::fix_double(&edit_end);
			if( fabs(edit_end-position) < min_movement ) continue;
			if( edit_end < position && edit_end > new_position )
				new_position = edit_end;
		}
	}
	return new_position;
}

void EDL::rescale_proxy(int orig_scale, int new_scale)
{
	if( orig_scale == new_scale ) return;
// project size
	float orig_w = (float)session->output_w * orig_scale;
	float orig_h = (float)session->output_h * orig_scale;
	if( !parent_edl ) {
		session->output_w = Units::round(orig_w / new_scale);
		session->output_h = Units::round(orig_h / new_scale);
	}

// track sizes
	for( Track *track=tracks->first; track; track=track->next ) {
		if( track->data_type != TRACK_VIDEO ) continue;
		orig_w = (float)track->track_w * orig_scale;
		orig_h = (float)track->track_h * orig_scale;
		track->track_w = Units::round(orig_w / new_scale);
		track->track_h = Units::round(orig_h / new_scale);
		((MaskAutos*)track->automation->autos[AUTOMATION_MASK])->
			set_proxy(orig_scale, new_scale);
		((FloatAutos*)track->automation->autos[AUTOMATION_CAMERA_X])->
			set_proxy(orig_scale, new_scale);
		((FloatAutos*)track->automation->autos[AUTOMATION_CAMERA_Y])->
			set_proxy(orig_scale, new_scale);
		((FloatAutos*)track->automation->autos[AUTOMATION_PROJECTOR_X])->
			set_proxy(orig_scale, new_scale);
		((FloatAutos*)track->automation->autos[AUTOMATION_PROJECTOR_Y])->
			set_proxy(orig_scale, new_scale);
	}
}

void EDL::set_proxy(int new_scale, int use_scaler,
	ArrayList<Indexable*> *orig_assets, ArrayList<Indexable*> *proxy_assets)
{
	int orig_scale = session->proxy_scale;
	int orig_use_scaler = session->proxy_use_scaler;

// rescale to full size asset in read_frame
	session->proxy_scale = new_scale;
	session->proxy_use_scaler = use_scaler;

	if( use_scaler ) {
		for( int i=0; i<proxy_assets->size(); ++i ) {
			Asset *proxy_asset = (Asset *)proxy_assets->get(i);
			proxy_asset->width = orig_assets->get(i)->get_w();
			proxy_asset->height = orig_assets->get(i)->get_h();
		}
		new_scale = 1;
	}

	if( !orig_use_scaler )
		rescale_proxy(orig_scale, new_scale);

// change original assets to proxy assets
	int folder_no = use_scaler || new_scale != 1 ? AW_PROXY_FOLDER : AW_MEDIA_FOLDER;
	for( int i=0,n=proxy_assets->size(); i<n; ++i ) {
		Indexable *proxy_idxbl = proxy_assets->get(i);
		proxy_idxbl->folder_no = folder_no;
		if( !proxy_idxbl->is_asset ) continue;
		Asset *proxy_asset = assets->update((Asset *)proxy_idxbl);
		if( proxy_asset == (Asset *)proxy_idxbl ) continue;
		proxy_asset->width = proxy_idxbl->get_w();
		proxy_asset->height = proxy_idxbl->get_h();
	}
// replace track contents
	for( Track *track=tracks->first; track; track=track->next ) {
		if( track->data_type != TRACK_VIDEO ) continue;
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			Indexable *idxbl = (Indexable *)edit->asset;
			if( !idxbl ) idxbl = (Indexable *)edit->nested_edl;
			if( !idxbl ) continue;
			int i = orig_assets->size();
			while( --i>=0 && strcmp(orig_assets->get(i)->path, idxbl->path) );
			if( i < 0 ) continue;
			Indexable *proxy_idxbl = proxy_assets->get(i);
			Asset *proxy_asset = proxy_idxbl->is_asset ?
				assets->update((Asset *)proxy_idxbl) : 0;
			EDL *proxy_edl = !proxy_idxbl->is_asset ?
				(EDL *)proxy_idxbl : 0;
			edit->asset = proxy_asset;
			edit->nested_edl = proxy_edl;
		}
	}
	for( int j=0,n=clips.size(); j<n; ++j ) {
		EDL *clip = clips[j];
		int has_proxy = 0;
		for( Track *track=clip->tracks->first; track; track=track->next ) {
			if( track->data_type != TRACK_VIDEO ) continue;
			for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
				Indexable *idxbl = (Indexable *)edit->asset;
				if( !idxbl ) idxbl = (Indexable *)edit->nested_edl;
				if( !idxbl ) continue;
				int i = orig_assets->size();
				while( --i>=0 && strcmp(orig_assets->get(i)->path, idxbl->path) );
				if( i < 0 ) continue;
				Indexable *proxy_idxbl = proxy_assets->get(i);
				Asset *proxy_asset = proxy_idxbl->is_asset ?
					assets->update((Asset *)proxy_idxbl) : 0;
				EDL *proxy_edl = !proxy_idxbl->is_asset ?
					(EDL *)proxy_idxbl : 0;
				edit->asset = proxy_asset;
				edit->nested_edl = proxy_edl;
				has_proxy = 1;
			}
		}
		if( has_proxy && !orig_use_scaler )
			clip->rescale_proxy(orig_scale, new_scale);
	}
}

void EDL::add_proxy(int use_scaler,
	ArrayList<Indexable*> *orig_assets, ArrayList<Indexable*> *proxy_assets)
{
	if( use_scaler ) {
		for( int i=0,n=proxy_assets->size(); i<n; ++i ) {
			Asset *proxy_asset = (Asset *)proxy_assets->get(i);
			proxy_asset->width = orig_assets->get(i)->get_w();
			proxy_asset->height = orig_assets->get(i)->get_h();
		}
	}

// change original assets to proxy assets
	for( int i=0,n=proxy_assets->size(); i<n; ++i ) {
		Asset *proxy_asset = assets->update((Asset *)proxy_assets->get(i));
		proxy_asset->folder_no = AW_PROXY_FOLDER;
// replace track contents
		for( Track *track=tracks->first; track; track=track->next ) {
			if( track->data_type != TRACK_VIDEO ) continue;
			for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
				if( !edit->asset ) continue;
				if( !strcmp(edit->asset->path, orig_assets->get(i)->path) ) {
					edit->asset = proxy_asset;
				}
			}
		}
	}
}

Asset *EDL::get_proxy_asset()
{
	return folder_no == AW_PROXY_FOLDER ?
		tracks->first->edits->first->asset : 0;
}

Track *EDL::add_new_track(int data_type)
{
	Track *new_track = 0;
	switch( data_type ) {
	case TRACK_VIDEO:
		++session->video_tracks;
		new_track = tracks->add_video_track(0, 0);
		break;
	case TRACK_AUDIO:
		++session->audio_tracks;
		new_track = tracks->add_audio_track(0, 0);
		break;
	case TRACK_SUBTITLE:
		new_track = tracks->add_subttl_track(0, 0);
		break;
	}
	return new_track;
}

double EDL::get_cursor_position(int cursor_x, int pane_no)
{
	return (double)cursor_x * local_session->zoom_sample / session->sample_rate +
		(double)local_session->view_start[pane_no] *
			local_session->zoom_sample / session->sample_rate;
}
int64_t EDL::get_position_cursorx(double position, int pane_no)
{
	return (int64_t)(position * session->sample_rate / local_session->zoom_sample)
			- local_session->view_start[pane_no];
}

int EDL::in_use(Indexable *indexable)
{
	for( Track *track=tracks->first; track; track=track->next ) {
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			Indexable *idxbl = (Indexable *)edit->asset;
			if( !idxbl ) idxbl = (Indexable *)edit->nested_edl;
			if( !idxbl ) continue;
			if( idxbl->id == indexable->id ) return 1;
			if( !indexable->is_asset != !idxbl->is_asset ) continue;
			if( !strcmp(idxbl->path, indexable->path) ) return 1;
		}
	}
	for( int i=0; i<clips.size(); ++i )
		if( clips[i]->in_use(indexable) ) return 1;
	for( int i=0; i<nested_edls.size(); ++i )
		if( nested_edls[i]->in_use(indexable) ) return 1;
	return 0;
}

int EDL::regroup(int next_id)
{
	ArrayList<int> new_groups;
	for( Track *track=tracks->first; track; track=track->next ) {
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			if( !edit->group_id ) continue;
			int k = new_groups.size();
			while( --k >= 0 && new_groups[k] != edit->group_id );
			if( k >= 0 ) continue;
			new_groups.append(edit->group_id);
		}
	}
	for( Track *track=tracks->first; track; track=track->next ) {
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			if( !edit->group_id ) continue;
			int k = new_groups.size();
			while( --k >= 0 && new_groups[k] != edit->group_id );
			if( k < 0 ) continue;
			edit->group_id = k + next_id;
		}
	}
	return new_groups.size();
}

