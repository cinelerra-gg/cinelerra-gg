
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

#ifndef CACHEBASE_H
#define CACHEBASE_H


#include "asset.inc"
#include "linklist.h"
#include "mutex.inc"
#include <stdint.h>

// Store rendered elements from files but not the files.
// Drawing caches must be separate from file caches to avoid
// delaying other file accesses for the drawing routines.


class CacheItemBase : public ListItem<CacheItemBase>
{
public:
	CacheItemBase();
	virtual ~CacheItemBase();

	virtual int get_size();

// source_id - supplied by user if the cache is not part of a file.
// Used for fast accesses.
	int source_id;
// path is needed since the item may need to be deleted based on file.
// Used for deletion.
	char *path;
// Number of last get or put operation involving this object.
	int age;
// Starting point of item in asset's native rate.
	int64_t position;
};



class CacheBase : public List<CacheItemBase>
{
public:
	CacheBase();
	virtual ~CacheBase();

	void age_cache(int64_t max_items);
	int get_age();
	int cache_items() { return total_items; }
	void remove_all();

// Insert item in list in ascending position order.
	void put_item(CacheItemBase *item);
// Get first item from list with matching position or 0 if none found.
	CacheItemBase* get_item(int64_t position);
// Delete item.
	void del_item(CacheItemBase *item);
	int delete_item(CacheItemBase *item);
// Remove all items with the asset id.
	void remove_asset(Asset *asset);
// Called when done with the item returned by get_.
// Ignore if item was 0.
	void unlock();
// Delete oldest item.  
// Return number of bytes freed if successful.  Return 0 if nothing to delete.
	int delete_oldest();
// Calculate current size of cache in bytes
	int64_t get_memory_usage();

	int total_items;
	Mutex *lock;
// Current position of search
	CacheItemBase *current_item;
};



#endif
