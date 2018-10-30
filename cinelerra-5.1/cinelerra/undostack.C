
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
#include "bctimer.h"
#include "clip.h"
#include "cstrdup.h"
#include "filexml.h"
#include "undostack.h"
#include <string.h>

// undo can be incremental or direct, from key to current
//#define UNDO_INCREMENTAL

UndoStack::UndoStack() : List<UndoStackItem>()
{
	current = 0;
}

UndoStack::~UndoStack()
{
}

UndoStackItem *UndoStack::get_current_undo()
{
	UndoStackItem *item = current;
	if( item &&  (number_of(item) & 1) ) item = item->previous;
	if( item && !(number_of(item) & 1) ) item = item->previous;
	return item;
}

UndoStackItem *UndoStack::get_current_redo()
{
	UndoStackItem *item = current ? current : first;
	if( item &&  (number_of(item) & 1) ) item = item->next;
	if( item && !(number_of(item) & 1) ) item = item->next;
	return item;
}


UndoStackItem* UndoStack::push()
{
// delete oldest 2 undos if necessary
	if( total() > UNDOLEVELS ) {
		UndoStackItem *item = first, *key = item;
#ifdef UNDO_INCREMENTAL
		for( int i=2; --i>=0; item=item->next );
#else
		for( int i=2; --i>=0; item=item->next )
			if( item->is_key() ) key = item;
#endif
		char *data = !item->is_key() ? key->get_data() : 0;
		while( first != item ) {
			if( current == first ) current = first->next;
			delete first;
		}
		if( data ) {
			item->set_data(data);
			delete [] data;
		}
	}

// current is only 0 if before first undo
	current = current ? insert_after(current) : insert_before(first);
// delete future undos if necessary
	while( current->next ) remove(last);
	return current;
}

UndoStackItem* UndoStack::pull_next()
{
// use first entry if none
	if( !current )
		current = first;
	else
// use next entry if there is a next entry
	if( current->next )
		current = NEXT;
// don't change current if there is no next entry
	else
		return 0;

	return current;
}


void UndoStack::dump(FILE *fp)
{
	fprintf(fp,"UndoStack::dump\n");
	UndoStackItem *current = last;
// Dump most recent
	for( int i=0; current && i<32; ++i,current=PREVIOUS ) {
		fprintf(fp,"%c %2d %14p %-24s %8d %04jx %c\n", current->is_key() ? 'k' : ' ',
			i, current, current->get_description(), current->get_size(),
			current->get_flags(), current == this->current ? '*' : ' ');
//char fn[BCSTRLEN]; sprintf(fn,"/tmp/undo%d", i); FILE *fp = fopen(fn,"w");
//if( fp ) { char *cp=current->get_data(); fwrite(cp,strlen(cp),1,fp); fclose(fp); delete [] cp; }
	}
}

class undo_record {
public:
	int32_t new_ofs;
	int32_t new_len;
	int32_t old_len;
};

static unsigned char* apply_difference(unsigned char *before, int before_len,
		unsigned char *patch, int patch_len, int *result_len)
{
	XMLBuffer xbfr((const char *)patch, patch_len, 0);
	int32_t len = 0;
	xbfr.read((char *)&len, sizeof(len));
	char *result = new char[len];
	char *rp = result, *bp = (char*)before;

	while( xbfr.itell() < patch_len ) {
		undo_record urecd;
		xbfr.read((char*)&urecd, sizeof(urecd));
		int ofs = rp - result;
		if( urecd.new_ofs > ofs ) {
			int sz = urecd.new_ofs - ofs;
			memcpy(rp, bp, sz);
			bp += sz;  rp += sz;
		}
		if( urecd.new_len > 0 )
			xbfr.read(rp, urecd.new_len);
		bp += urecd.old_len;  rp += urecd.new_len;
	}

// All data from before_ptr to end of result buffer is identical
	int sz = bp - (char *)before;
	if( (before_len-=sz) > 0 ) {
		memcpy(rp, bp, before_len);
		rp += before_len;
	}

	*result_len = rp - result;
	return (unsigned char *)result;
}

UndoStackItem::UndoStackItem()
 : ListItem<UndoStackItem>()
{
	description = data = 0;
	data_size = 0;
	key = 0;
	creator = 0;
	session_filename = 0;
}

UndoStackItem::~UndoStackItem()
{
	delete [] description;
	delete [] data;
	delete [] session_filename;
}

void UndoStackItem::set_description(char *description)
{
	delete [] this->description;
	this->description= 0;
	this->description = new char[strlen(description) + 1];
	strcpy(this->description, description);
}

void UndoStackItem::set_filename(char *filename)
{
	delete [] session_filename;
	this->session_filename = cstrdup(filename);
}

char* UndoStackItem::get_filename()
{
	return session_filename;
}


const char* UndoStackItem::get_description()
{
	if( description )
		return description;
	else
		return "";
}

// undo diff

UndoHash::UndoHash(char *txt, int len, UndoHash *nxt)
{
	this->txt = txt;
	this->len = len;
	this->nxt = nxt;
	line[va] = line[vb] = -1;
	occurs[va] = occurs[vb] = 0;
}
UndoHash::~UndoHash()
{
}

UndoHashTable::UndoHashTable()
{
	for( int i=0; i<hash_sz; ++i ) table[i] = 0;
	bof = new UndoHash(0, 0, 0);
	eof = new UndoHash(0, 0, 0);
}
UndoHashTable::~UndoHashTable()
{
	delete bof;
	delete eof;
	for( int i=0; i<hash_sz; ++i ) {
		for( UndoHash *nxt=0, *hp=table[i]; hp; hp=nxt ) {
			nxt = hp->nxt;  delete hp;
		}
	}
}

UndoHash *UndoHashTable::add(char *txt, int len)
{
	uint8_t *bp = (uint8_t *)txt;
	int v = 0;
	for( int i=len; --i>=0; ++bp ) {
		v = (v<<1) + *bp;
		v = (v + (v>>hash_sz2)) & hash_sz1;
	}
	UndoHash *hp = table[v];
	while( hp && strncmp(hp->txt,txt,len) ) hp = hp->nxt;
	if( !hp ) {
		hp = new UndoHash(txt, len, table[v]);
		table[v] = hp;
	}
	return hp;
}

UndoLine::UndoLine(UndoHash *hash, char *tp)
{
	this->txt = tp;  this->len = 0;
	this->hash = hash;
	hash->occurs[va] = hash->occurs[vb] = 1;
}

UndoLine::UndoLine(UndoHashTable *hash, char *txt, int len)
{
	this->txt = txt;  this->len = len;
	this->hash = hash->add(txt, len);
}
UndoLine::~UndoLine()
{
}

int UndoLine::eq(UndoLine *ln)
{
	return hash == ln->hash ? 1 : 0;
}

void UndoVersion::scan_lines(UndoHashTable *hash, char *sp, char *ep)
{
	for( int line=1; sp<ep; ++line ) {
		char *txt = sp;
		while( sp<ep && *sp++ != '\n' );
		UndoLine *ln = new UndoLine(hash, txt, sp-txt);
		ln->hash->line[ver] = line;
		++ln->hash->occurs[ver];
		append(ln);
	}
}


void UndoStackItem::set_data(char *data)
{
	delete [] this->data;  this->data = 0;
	this->data_size = 0;   this->key = 0;
	int data_size = strlen(data)+1;

	UndoStackItem *current = this;
// Search for key buffer within interval
	for( int i=UNDO_KEY_INTERVAL; --i>=0 && current && !current->key; current=PREVIOUS );

	int need_key = current ? 0 : 1;
// setting need_key = 1 forces all undo data items to key (not diffd)
//	int need_key = 1;
	char *prev = 0;  int prev_size = 0;
	if( !need_key ) {
#ifdef UNDO_INCREMENTAL
		prev = previous->get_data();
		prev_size = prev ? strlen(prev)+1 : 0;
#else
		prev = current->data;
		prev_size = current->data_size;
#endif
		int min_size = data_size/16;
// if the previous is a sizable incremental diff
		if( previous && !previous->key &&
		    previous->data_size > min_size ) {
//  and this minimum diff distance is a lot different, need key
			int dist = abs(data_size - prev_size);
			if( abs(dist - previous->data_size) > min_size )
				need_key = 1;
		}
	}
	if( !need_key ) {
		int bfr_size = BCTEXTLEN;
		char *bfr = new char[bfr_size];
		XMLBuffer xbfr(bfr_size, bfr, 1);
		xbfr.write((char*)&data_size, sizeof(data_size));
		UndoHashTable hash;
		UndoVersion alines(va), blines(vb);
		char *asp = data, *aep = asp + data_size-1;
		char *bsp = prev, *bep = bsp + prev_size-1;
		alines.append(new UndoLine(hash.bof, asp));
		blines.append(new UndoLine(hash.bof, bsp));
		alines.scan_lines(&hash, asp, aep);
		blines.scan_lines(&hash, bsp, bep);
		int asz = alines.size(), bsz = blines.size();
// trim matching suffix
		while( asz > 0 && bsz > 0 && alines[asz-1]->eq(blines[bsz-1]) ) {
			aep = alines[--asz]->txt;  alines.remove_object();
			bep = blines[--bsz]->txt;  blines.remove_object();
		}
// include for matching last item
		alines.append(new UndoLine(hash.eof, aep));
		blines.append(new UndoLine(hash.eof, bep));
		hash.eof->line[va] = asz++;
		hash.eof->line[vb] = bsz++;

		int ai = 0, bi = 0;
		while( ai < asz || bi < bsz ) {
// skip running match
			if( alines[ai]->eq(blines[bi]) ) { ++ai;  ++bi;  continue; }
			char *adp = alines[ai]->txt, *bdp = blines[bi]->txt;
// find best unique match
			int ma = asz, mb = bsz;
			int mx = ma + mb + 1;
			for( int ia=ai; ia<asz && ia-ai<mx; ++ia ) {
				UndoHash *ah = alines[ia]->hash;
				if( ah->occurs[va] != 1 || ah->occurs[vb] != 1 ) continue;
				int m = (ah->line[va] - ai) + (ah->line[vb] - bi);
				if( m >= mx ) continue;
				ma = ah->line[va];  mb = ah->line[vb];  mx = m;
			}
// trim suffix
			while( ma > 0 && mb > 0 && alines[ma-1]->eq(blines[mb-1]) ) { --ma;  --mb; }
			char *ap = alines[ma]->txt, *bp = blines[mb]->txt;
			ai = ma;  bi = mb;
			undo_record urecd;
			urecd.new_ofs = adp - data;
			urecd.new_len = ap - adp;
			urecd.old_len = bp - bdp;
			xbfr.write((const char*)&urecd, sizeof(urecd));
			xbfr.write((const char*)adp, urecd.new_len);
		}

		int64_t pos = xbfr.otell();
		if( abs(data_size-prev_size) < pos/2 ) {
			this->data = new char[this->data_size = pos];
			xbfr.iseek(0);
			xbfr.read(this->data, pos);
#if 1
			int test_size = 0;
			char *test_data = (char*)apply_difference(
				(unsigned char *)prev, prev_size,
				(unsigned char *)this->data, this->data_size,
				&test_size);
			if( test_size != data_size || memcmp(test_data, data, test_size) ) {
				printf("UndoStackItem::set_data: *** incremental undo failed!\n");
				delete [] this->data;  this->data = 0;  this->data_size = 0;
				need_key = 1;
			}
			delete [] test_data;
#endif
		}
		else
			need_key = 1;
	}

#ifdef UNDO_INCREMENTAL
	delete [] prev;
#endif
	if( need_key ) {
		this->key = 1;
		this->data = new char[this->data_size = data_size];
		memcpy(this->data, data, this->data_size);
	}
}

char* UndoStackItem::get_data()
{
// Find latest key buffer
	UndoStackItem *current = this;
	while( current && !current->key )
		current = PREVIOUS;
	if( !current ) {
		printf("UndoStackItem::get_data: no key buffer found!\n");
		return 0;
	}

// This is the key buffer
	if( current == this ) {
		char *result = new char[data_size];
		memcpy(result, data, data_size);
		return result;
	}
#ifdef UNDO_INCREMENTAL
// Get key buffer
	char *current_data = current->get_data();
	int current_size = current->data_size;
	current = NEXT;
	while( current ) {
// Do incremental updates
		int new_size;
		char *new_data = (char*)apply_difference(
			(unsigned char*)current_data, current_size,
			(unsigned char*)current->data, current->data_size,
			&new_size);
		delete [] current_data;

		if( current == this )
			return new_data;
		current_data = new_data;
		current_size = new_size;
		current = NEXT;
	}
// Never hit this object.
	delete [] current_data;
	printf("UndoStackItem::get_data: lost starting object!\n");
	return 0;
#else
	int new_size;
	char *new_data = (char*)apply_difference(
		(unsigned char*)current->data, current->data_size,
		(unsigned char*)this->data, this->data_size, &new_size);
	return new_data;
#endif
}

int UndoStackItem::is_key()
{
	return key;
}

int UndoStackItem::get_size()
{
	return data_size;
}

void UndoStackItem::set_flags(uint64_t flags)
{
	this->load_flags = flags;
}

uint64_t UndoStackItem::get_flags()
{
	return load_flags;
}

void UndoStackItem::set_creator(void *creator)
{
	this->creator = creator;
}

void* UndoStackItem::get_creator()
{
	return creator;
}

void UndoStackItem::save(FILE *fp)
{
	fwrite(&key,1,sizeof(key),fp);
	fwrite(&load_flags,1,sizeof(load_flags),fp);
	fwrite(&data_size,1,sizeof(data_size),fp);
	fwrite(data,1,data_size,fp);
	for( char *bp=session_filename; *bp; ++bp ) fputc(*bp, fp);
	fputc(0, fp);
	for( char *bp=description; *bp; ++bp ) fputc(*bp, fp);
	fputc(0, fp);
}

void UndoStackItem::load(FILE *fp)
{
	fread(&key,1,sizeof(key),fp);
	fread(&load_flags,1,sizeof(load_flags),fp);
	fread(&data_size,1,sizeof(data_size),fp);
	fread(data=new char[data_size],1,data_size,fp);
	char filename[BCTEXTLEN], descr[BCTEXTLEN];
	char *bp = filename, *ep = bp+sizeof(filename)-1;
	for( int ch; bp<ep && (ch=fgetc(fp))>0; ++bp ) *bp = ch;
	*bp = 0;
	session_filename = cstrdup(filename);
	bp = descr;  ep = bp+sizeof(descr)-1;
	for( int ch; bp<ep && (ch=fgetc(fp))>0; ++bp ) *bp = ch;
	*bp = 0;
	description = cstrdup(descr);
//printf("read undo key=%d,flags=%jx,data_size=%d,data=%p,file=%s,descr=%s\n",
// key, load_flags, data_size, data, session_filename, description);
}

void UndoStack::save(FILE *fp)
{
	for( UndoStackItem *item=first; item; item=item->next ) {
		int is_current = item == current ? 1 : 0;
		fwrite(&is_current,1,sizeof(is_current),fp);
		item->save(fp);
//		if( item == current ) break; // stop at current
	}
}

void UndoStack::load(FILE *fp)
{
	while( last ) delete last;
	current = 0;
	UndoStackItem *current_item = 0;
	int is_current = 0;
	while( fread(&is_current,1,sizeof(is_current),fp) == sizeof(is_current) ) {
		UndoStackItem *item = push();
		item->load(fp);
		if( is_current ) current_item = item;
	}
	if( current_item ) current = current_item;
}

