
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


#include "arraylist.h"
#include "asset.h"
#include "clip.h"
#include "filexml.h"
#include "indexfile.h"
#include "indexstate.h"
#include "language.h"
#include "mainerror.h"
#include "mutex.h"

#include <stdio.h>
#include <string.h>

int IndexMarks::find(int64_t no)
{
	int l = -1, r = size();
        while( (r - l) > 1 ) {
		int i = (l+r) >> 1;
		if( no > values[i].no ) l = i; else r = i;
	}
	return l;
}

IndexChannel::IndexChannel(IndexState *state, int64_t length)
{
	this->state = state;
	this->length = length;
	min = max = 0;
	bfr = inp = 0;
	size = 0;
	zidx = 0;
}

IndexChannel::~IndexChannel()
{
	delete [] bfr;
}

void IndexChannel::put_entry()
{
	inp->min = min;
	inp->max = max;
	++inp;
	max = -1; min = 1;
	zidx = 0;
}

int64_t IndexChannel::pos()
{
	return used() * state->index_zoom + zidx;
}

void IndexChannel::pad_data(int64_t pos)
{
	CLAMP(pos, 0, length);
	while( this->pos() < pos ) put_entry();
	inp = bfr + pos / state->index_zoom;
	zidx = pos % state->index_zoom;
}

void IndexState::reset_index()
{
	index_status = INDEX_NOTTESTED;
	index_start = 0;
	index_zoom = 0;
	index_bytes = 0;
	index_entries.remove_all_objects();
	index_channels.remove_all_objects();
}

void IndexState::reset_markers()
{
	marker_status = MARKERS_NOTTESTED;
	video_markers.remove_all_objects();
	audio_markers.remove_all_objects();
}

IndexState::IndexState()
 : Garbage("IndexState")
{
	marker_lock = new Mutex("IndexState::marker_lock");
	reset_index();
	reset_markers();
}

IndexState::~IndexState()
{
	reset_index();
	reset_markers();
	delete marker_lock;
}

void IndexState::init_scan(int64_t index_length)
{
	index_zoom = 1;
	int channels = index_channels.size();
	if( !channels ) return;
	int64_t max_samples = 0;
	for( int ch=0; ch<channels; ++ch ) {
		int64_t len = index_channels[ch]->length;
		if( max_samples < len ) max_samples = len;
	}
	int64_t items = index_length / sizeof(IndexItem);
	int64_t count = (items - channels) / channels + 1;
	while( count*index_zoom < max_samples ) index_zoom *= 2;
	for( int ch=0; ch<channels; ++ch ) {
		IndexChannel *chn = index_channels[ch];
		int64_t len = chn->length / index_zoom + 1;
		chn->alloc(len);
		chn->put_entry();
	}
	reset_markers();
}

void IndexState::dump()
{
	printf("IndexState::dump this=%p\n", this);
	printf("    index_status=%d index_zoom=%jd index_bytes=%jd\n",
		index_status, index_zoom, index_bytes);
	printf("    index entries=%d\n", index_entries.size());
	for( int i=0; i<index_entries.size(); ++i )
		printf("  %d. ofs=%jd, sz=%jd\n", i,
			index_entries[i]->offset, index_entries[i]->size);
	printf("\n");
}

void IndexState::write_xml(FileXML *file)
{
	file->tag.set_title("INDEX");
	file->tag.set_property("ZOOM", index_zoom);
	file->tag.set_property("BYTES", index_bytes);
	file->append_tag();
	file->append_newline();

	for( int i=0; i<index_entries.size(); ++i ) {
		file->tag.set_title("OFFSET");
		file->tag.set_property("FLOAT", index_entries[i]->offset);
		file->append_tag();
		file->tag.set_title("/OFFSET");
		file->append_tag();
		file->tag.set_title("SIZE");
		file->tag.set_property("FLOAT", index_entries[i]->size);
		file->append_tag();
		file->tag.set_title("/SIZE");
		file->append_tag();
		file->append_newline();
	}

	file->append_newline();
	file->tag.set_title("/INDEX");
	file->append_tag();
	file->append_newline();
}

void IndexState::read_xml(FileXML *file, int channels)
{
	index_entries.remove_all_objects();
	for( int i=0; i<channels; ++i ) add_index_entry(0, 0, 0);

	int current_offset = 0;
	int current_size = 0;
	int result = 0;

	index_zoom = file->tag.get_property("ZOOM", 1);
	index_bytes = file->tag.get_property("BYTES", (int64_t)0);

	while(!result) {
		result = file->read_tag();
		if(!result) {
			if(file->tag.title_is("/INDEX")) {
				result = 1;
			}
			else if(file->tag.title_is("OFFSET")) {
				if(current_offset < channels) {
					int64_t offset = file->tag.get_property("FLOAT", (int64_t)0);
					index_entries[current_offset++]->offset = offset;
//printf("Asset::read_index %d %d\n", current_offset - 1, index_offsets[current_offset - 1]);
				}
			}
			else if(file->tag.title_is("SIZE")) {
				if(current_size < channels) {
					int64_t size = file->tag.get_property("FLOAT", (int64_t)0);
					index_entries[current_size++]->size = size;
				}
			}
		}
	}
}

int IndexState::write_index(const char *index_path, Asset *asset, int64_t zoom, int64_t file_bytes)
{
        FILE *fp = fopen(index_path, "wb");
        if( !fp ) {
		eprintf(_("IndexState::write_index Couldn't write index file %s to disk.\n"),
			index_path);
		return 1;
	}
	index_zoom = zoom;
	index_bytes = file_bytes;
	index_status = INDEX_READY;

	FileXML xml;
// write index_state as asset or directly.
	if( asset )
		asset->write(&xml, 1, "");
	else
		write_xml(&xml);
	int64_t len = xml.length() + FileXML::xml_header_size;
	index_start = sizeof(index_start) + len;
	fwrite(&index_start, sizeof(index_start), 1, fp);
	xml.write_to_file(fp);

	int channels = index_entries.size();
	int64_t max_size = 0;
	for( int ch=0; ch<channels; ++ch ) {
		IndexEntry *ent = index_entries[ch];
		float *bfr = ent->bfr;
		int64_t size = ent->size;
		if( max_size < size ) max_size = size;
		fwrite(bfr, sizeof(float), size, fp);
	}

	fclose(fp);
	return 0;
}

int IndexState::write_markers(const char *index_path)
{
	int vid_size = video_markers.size();
	int aud_size = audio_markers.size();
	if( !vid_size && !aud_size ) return 0;

	FILE *fp = 0;
	char marker_path[BCTEXTLEN];
	strcpy(marker_path, index_path);
	char *basename = strrchr(marker_path,'/');
	if( !basename ) basename = marker_path;
	char *ext = strrchr(basename, '.');
	if( ext ) {
		strcpy(ext, ".mkr");
		fp = fopen(marker_path, "wb");
	}

	char version[] = MARKER_MAGIC_VERSION;
        if( !fp || !fwrite(version, strlen(version), 1, fp) ) {
		eprintf(_("IndexState::write_markers Couldn't write marker file %s to disk.\n"),
			marker_path);
		return 1;
	}

	fwrite(&vid_size, sizeof(vid_size), 1, fp);
	for( int vidx=0; vidx<vid_size; ++vidx ) {
		IndexMarks &marks = *video_markers[vidx];
		int count = marks.size();
		fwrite(&count, sizeof(count), 1, fp);
		fwrite(&marks[0], sizeof(marks[0]), count, fp);
	}

	fwrite(&aud_size, sizeof(aud_size), 1, fp);
	for( int aidx=0; aidx<aud_size; ++aidx ) {
		IndexMarks &marks = *audio_markers[aidx];
		int count = marks.size();
		fwrite(&count, sizeof(count), 1, fp);
		fwrite(&marks[0], sizeof(marks[0]), marks.size(), fp);
	}

	fclose(fp);
	return 0;
}

int IndexState::read_markers(char *index_dir, char *file_path)
{
	int ret = 0;
	marker_lock->lock("IndexState::read_markers");
	if( marker_status == MARKERS_NOTTESTED ) {
		char src_path[BCTEXTLEN], marker_path[BCTEXTLEN];
		IndexFile::get_index_filename(src_path, index_dir, marker_path, file_path, ".mkr");
		FILE *fp = fopen(marker_path, "rb");
		int vsz = strlen(MARKER_MAGIC_VERSION);
		char version[vsz];
	        if( fp && fread(version, vsz, 1, fp) ) {
			if( memcmp(version, MARKER_MAGIC_VERSION, vsz) ) {
				eprintf(_("IndexState::read_markers marker file version mismatched\n: %s\n"),
					marker_path);
				return 1;
			}
			ret = read_marks(fp);
			if( !ret ) marker_status = MARKERS_READY;
			fclose(fp);
		}
	}
	marker_lock->unlock();
	return ret;
}

int IndexState::read_marks(FILE *fp)
{
	reset_markers();
	int vid_size = 0;
	if( !fread(&vid_size, sizeof(vid_size), 1, fp) ) return 1;
	add_video_markers(vid_size);
	for( int vidx=0; vidx<vid_size; ++vidx ) {
		int count = 0;
		if( !fread(&count, sizeof(count), 1, fp) ) return 1;
		IndexMarks &marks = *video_markers[vidx];
		marks.allocate(count);
		int len = fread(&marks[0], sizeof(marks[0]), count, fp);
		if( len != count ) return 1;
		marks.total = count;
	}
	int aud_size = 0;
	if( !fread(&aud_size, sizeof(aud_size), 1, fp) ) return 1;
	add_audio_markers(aud_size);
	for( int aidx=0; aidx<aud_size; ++aidx ) {
		int count = 0;
		if( !fread(&count, sizeof(count), 1, fp) ) return 1;
		IndexMarks &marks = *audio_markers[aidx];
		marks.allocate(count);
		int len = fread(&marks[0], sizeof(marks[0]), count, fp);
		if( len != count ) return 1;
		marks.total = count;
	}
	return 0;
}

int IndexState::create_index(const char *index_path, Asset *asset)
{
	index_entries.remove_all_objects();
	int channels = index_channels.size();
	int64_t offset = 0;
	for( int ch=0; ch<channels; ++ch ) {
		IndexChannel *chn = index_channels[ch];
		float *bfr = (float *)chn->bfr;
		int64_t size = 2 * chn->used();
		add_index_entry(bfr, offset, size);
		offset += size;
	}

	write_markers(index_path);
	return write_index(index_path, asset, index_zoom, index_bytes);
}

int64_t IndexState::get_index_offset(int channel)
{
	return channel >= index_entries.size() ? 0 :
		index_entries[channel]->offset;
}

int64_t IndexState::get_index_size(int channel)
{
	return channel >= index_entries.size() ? 0 :
		index_entries[channel]->size;
}

float *IndexState::get_channel_buffer(int channel)
{
	return channel >= index_channels.size() ? 0 :
		(float *)index_channels[channel]->bfr;
}

int64_t IndexState::get_channel_used(int channel)
{
	return channel >= index_channels.size() ? 0 :
		index_channels[channel]->used();
}

