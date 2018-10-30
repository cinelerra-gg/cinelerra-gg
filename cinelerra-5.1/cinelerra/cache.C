
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

#include "asset.h"
#include "assets.h"
#include "bcsignals.h"
#include "cache.h"
#include "condition.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "mutex.h"
#include "thread.h"
#include "preferences.h"

#include <string.h>

// edl came from a command which won't exist anymore
CICache::CICache(Preferences *preferences)
 : List<CICacheItem>()
{
	this->preferences = preferences;
	edl = 0;
	check_out_lock = new Condition(0, "CICache::check_out_lock", 0);
	total_lock = new Mutex("CICache::total_lock");
	check_outs = 0;
}

CICache::~CICache()
{
	while(last)
	{
		CICacheItem *item = last;
//printf("CICache::~CICache: %s\n", item->asset->path);
		remove_pointer(item);
		item->Garbage::remove_user();
	}
	delete check_out_lock;
	delete total_lock;
}






File* CICache::check_out(Asset *asset, EDL *edl, int block)
{
	CICacheItem *current = 0;
	long tid = (long)Thread::get_self();
	if( !tid ) tid = 1;

	while(1)
	{
		File *file = 0;
		total_lock->lock("CICache::check_out");
// Scan directory for item
		current = first;
		while(current && strcmp(current->asset->path, asset->path) != 0)
			current = NEXT;
 		if(!current) { // Create new item
			current = new CICacheItem(this, edl, asset);
			append(current);  current->checked_out = tid;
			file = current->file;
			total_lock->unlock();
			int result = file->open_file(preferences, asset, 1, 0);
			if( result ) {
SET_TRACE
				delete file;
				file = 0;
			}
			total_lock->lock("CICache::check_out 2");
			if( !file ) {
				remove_pointer(current);
				current->file = 0;
				current->Garbage::remove_user();
				current = 0;
			}
			else
				current->Garbage::add_user();
		}
		else {
			file = current->file;
			if(!current->checked_out) {
// Return existing/new item
				current->Garbage::add_user();
				current->age = EDL::next_id();
				current->checked_out = tid;
			}
			else if( current->checked_out == tid )
				++check_outs;
			else
				current = 0;
		}
		total_lock->unlock();
		if(current || !file || !block) break;
// Try again after blocking
		check_out_lock->lock("CICache::check_out");
	}

//printf("check out %p %lx %s\n", current, tid, asset->path);
	return current ? current->file : 0;
}

int CICache::check_in(Asset *asset)
{
	total_lock->lock("CICache::check_in");
	if( !check_outs ) {
		CICacheItem *current = first;
		while(current && strcmp(current->asset->path, asset->path) != 0)
			current = NEXT;
		if(current && current->checked_out) {
			current->checked_out = 0;
			current->Garbage::remove_user();
		}
	}
	else
		--check_outs;
	total_lock->unlock();

// Release for blocking check_out operations
	check_out_lock->unlock();
//printf("check  in %p %lx %s\n", current, (long)Thread::get_self(), asset->path);
	age();
	return 0;
}

void CICache::remove_all()
{
	CICacheItem *current, *temp;
	List<CICacheItem> removed;
	total_lock->lock("CICache::remove_all");
	for(current=first; current; current=temp)
	{
		temp = NEXT;
// Must not be checked out because we need the pointer to check back in.
// Really need to give the user the CacheItem.
		if(!current->checked_out)
		{
//printf("CICache::remove_all: %s\n", current->asset->path);
			remove_pointer(current);
			removed.append(current);
		}
	}
	total_lock->unlock();
	while( (current=removed.first) != 0 )
	{
		removed.remove_pointer(current);
		current->Garbage::remove_user();
	}
}

int CICache::delete_entry(char *path)
{
	total_lock->lock("CICache::delete_entry");
	CICacheItem *current = first;
	while( current && strcmp(current->asset->path, path) !=0 )
		current = NEXT;
	if(current && !current->checked_out)
		remove_pointer(current);
	else
		current = 0;
//printf("CICache::delete_entry: %s\n", current->asset->path);
	total_lock->unlock();
	if(current)
		current->Garbage::remove_user();
	return 0;
}

int CICache::delete_entry(Asset *asset)
{
	return delete_entry(asset->path);
}

int CICache::age()
{
// delete old assets if memory usage is exceeded
	int64_t prev_memory_usage = 0;
	int result = 0;
	while( !result ) {
		int64_t memory_usage = get_memory_usage(1);
		if( prev_memory_usage == memory_usage ) break;
		if( preferences->cache_size >= memory_usage ) break;
//printf("CICache::age 3 %p %jd %jd\n", this, memory_usage, preferences->cache_size);
		prev_memory_usage = memory_usage;
		result = delete_oldest();
	}
	return result;
}

int64_t CICache::get_memory_usage(int use_lock)
{
	CICacheItem *current;
	int64_t result = 0;
	if(use_lock) total_lock->lock("CICache::get_memory_usage");
	for(current = first; current; current = NEXT)
	{
		File *file = current->file;
		if(file) result += file->get_memory_usage();
	}
	if(use_lock) total_lock->unlock();
	return result;
}

int CICache::get_oldest()
{
	CICacheItem *current;
	int oldest = 0x7fffffff;
	total_lock->lock("CICache::get_oldest");
	for(current = last; current; current = PREVIOUS)
	{
		if(current->age < oldest)
		{
			oldest = current->age;
		}
	}
	total_lock->unlock();

	return oldest;
}

int CICache::delete_oldest()
{
	int result = 0;
	total_lock->lock("CICache::delete_oldest");
	CICacheItem *oldest = 0;
// at least 2
	if( first != last ) {
		CICacheItem *current = first;
		oldest = current;
		while( (current=NEXT) != 0 ) {
			if( current->age < oldest->age )
				oldest = current;
		}
// Got the oldest file.  Try requesting cache purge from it.
		if( oldest->file )
			oldest->file->purge_cache();
// Delete the file if cache already empty and not checked out.
		if( !oldest->checked_out )
			remove_pointer(oldest);
		else
			oldest = 0;
	}
// settle for just deleting one frame
	else if( first )
		result = first->file->delete_oldest();
	total_lock->unlock();
	if( oldest ) {
		oldest->Garbage::remove_user();
		result = 1;
	}
	return result;
}

void CICache::dump()
{
	CICacheItem *current;
	total_lock->lock("CICache::dump");
	printf("CICache::dump total size %jd\n", get_memory_usage(0));
	for(current = first; current; current = NEXT)
	{
		printf("cache item %p asset %p %s age=%d\n",
			current, current->asset,
			current->asset->path, current->age);
	}
	total_lock->unlock();
}


CICacheItem::CICacheItem()
: Garbage("CICacheItem"), ListItem<CICacheItem>()
{
}


CICacheItem::CICacheItem(CICache *cache, EDL *edl, Asset *asset)
 : Garbage("CICacheItem"), ListItem<CICacheItem>()
{
	age = EDL::next_id();

	this->asset = new Asset;

	item_lock = new Condition(1, "CICacheItem::item_lock", 0);


// Must copy Asset since this belongs to an EDL which won't exist forever.
	this->asset->copy_from(asset, 1);
	this->cache = cache;
	checked_out = 0;

	file = new File;
	int cpus = cache->preferences->processors;
	file->set_processors(cpus);
	file->set_preload(edl->session->playback_preload);
	file->set_subtitle(edl->session->decode_subtitles ?
		edl->session->subtitle_number : -1);
	file->set_program(edl->session->program_no);
	file->set_interpolate_raw(edl->session->interpolate_raw);
	file->set_white_balance_raw(edl->session->white_balance_raw);

SET_TRACE
}

CICacheItem::~CICacheItem()
{
	if(file) delete file;
	if(asset) asset->Garbage::remove_user();
	if(item_lock) delete item_lock;
}
