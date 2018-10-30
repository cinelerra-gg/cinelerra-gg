
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


#ifndef INDEXSTATE_H
#define INDEXSTATE_H

#include "arraylist.h"
#include "asset.inc"
#include "filexml.inc"
#include "garbage.h"
#include "indexstate.inc"
#include "mutex.inc"

#include <stdio.h>
#include <stdint.h>

class IndexItem {
public:
	float max, min;
};

class IndexMark {
public:
	int64_t no, pos;
	IndexMark() {}
	IndexMark(int64_t n, int64_t p) { no = n;  pos = p; }

	bool operator==(IndexMark &v) { return v.no == no; }
	bool operator >(IndexMark &v) { return v.no > no; }
};

class IndexChannel : public IndexItem {
public:
	IndexChannel(IndexState *state, int64_t length);
	~IndexChannel();

	IndexState *state;
	IndexItem *bfr, *inp;
	int64_t length, size;
	int zidx;

	void alloc(int64_t sz) { bfr = inp = new IndexItem[(size=sz)+1]; }
	int64_t used() { return inp - bfr; }
	int64_t avail() { return size - used(); }
	int64_t pos();
	void put_sample(float v, int zoom) {
		if( v > max ) max = v;
		if( v < min ) min = v;
		if( ++zidx >= zoom )
			put_entry();
	}
	void put_entry();
	void pad_data(int64_t pos);
};

class IndexChannels : public ArrayList<IndexChannel*> {
public:
	IndexChannels() {}
	~IndexChannels() { remove_all_objects(); }
};


class IndexEntry {
public:
	float *bfr;
// offsets / sizes of channels in index buffer in floats
	int64_t offset, size;
	IndexEntry(float *bp, int64_t ofs, int64_t sz) {
		bfr = bp;  offset = ofs;  size = sz;
	}
};

class IndexEntries : public ArrayList<IndexEntry*> {
public:
	IndexEntries() {}
	~IndexEntries() { remove_all_objects(); }
};

class IndexMarks : public ArrayList<IndexMark> {
public:
	IndexMarks() {}
	~IndexMarks() {}
	void add_mark(int64_t no, int64_t pos) { append(IndexMark(no, pos)); }
	int find(int64_t no);
};

class IndexMarkers : public ArrayList<IndexMarks *> {
public:
	IndexMarkers() {}
	~IndexMarkers() { remove_all_objects(); }
};

// Assets & nested EDLs store this information to have their indexes built
class IndexState : public Garbage
{
public:
	IndexState();
	~IndexState();

	void reset_index();
	void reset_markers();
	void init_scan(int64_t index_length);

// Supply asset to include encoding information in index file.
	void write_xml(FileXML *file);
	void read_xml(FileXML *file, int channels);
	int write_index(const char *index_path, Asset *asset, int64_t zoom, int64_t file_bytes);
	int create_index(const char *index_path, Asset *asset);
	int read_markers(char *index_dir, char *file_path);
	int read_marks(FILE *fp);
	int write_markers(const char *index_path);
	int64_t get_index_offset(int channel);
	int64_t get_index_size(int channel);
	float *get_channel_buffer(int channel);
	int64_t get_channel_used(int channel);
	void dump();

// index info
	int index_status;       // Macro from assets.inc
	int marker_status;
	int64_t index_zoom;     // zoom factor of index data
	int64_t index_start;    // byte start of index data in the index file
// Total bytes in the source file when the index was buillt
	int64_t index_bytes;
        IndexChannels index_channels;
	IndexEntries index_entries;
	Mutex *marker_lock;
	IndexMarkers video_markers;
	IndexMarkers audio_markers;

	void add_audio_channel(int64_t length) {
		index_channels.append(new IndexChannel(this, length));
	}
	void add_audio_stream(int nch, int64_t length) {
		for( int ch=0; ch<nch; ++ch ) add_audio_channel(length);
	}
	void add_video_markers(int n) {
		while( --n >= 0 ) video_markers.append(new IndexMarks());
	}
	void add_audio_markers(int n) {
		while( --n >= 0 ) audio_markers.append(new IndexMarks());
	}
	void add_index_entry(float *bfr, int64_t offset, int64_t size) {
		index_entries.append(new IndexEntry(bfr, offset, size));
	}

	void put_data(int ch, int nch, float *data, int64_t len) {
		IndexChannel *chn = index_channels[ch];
		int64_t avl = chn->avail() * index_zoom - chn->zidx;
		if( len > avl ) len = avl;
		for( int64_t i=len; --i>=0; data+=nch )
			chn->put_sample(*data, index_zoom);
	}
	void put_data(int ch, double *data, int64_t len) {
		IndexChannel *chn = index_channels[ch];
		int64_t avl = chn->avail() * index_zoom - chn->zidx;
		if( len > avl ) len = avl;
		for( int64_t i=len; --i>=0; )
			chn->put_sample(*data++, index_zoom);
	}
	int64_t pos(int ch) { return index_channels[ch]->pos(); }
	void pad_data(int ch, int nch, int64_t pos) {
		while( --nch >= 0 ) index_channels[ch++]->pad_data(pos);
	}

	void put_video_mark(int vidx, int64_t frm, int64_t pos) {
		if( vidx >= video_markers.size() ) return;
		IndexMarks &marks = *video_markers[vidx];
		if( marks.size()>0 && marks.last().no >= frm ) return;
		marks.add_mark(frm, pos);
	}
	void put_audio_mark(int aidx, int64_t smpl, int64_t pos) {
		if( aidx >= audio_markers.size() ) return;
		IndexMarks &marks = *audio_markers[aidx];
		if( marks.size()>0 && marks.last().no >= smpl ) return;
		marks.add_mark(smpl, pos);
	}
};

#endif

