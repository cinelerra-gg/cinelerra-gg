
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
#include "bcsignals.h"
#include "cachebase.h"
#include "edl.h"
#include "mutex.h"

#include <string.h>




CacheItemBase::CacheItemBase()
 : ListItem<CacheItemBase>()
{
	age = 0;
	source_id = -1;
	path = 0;
}

CacheItemBase::~CacheItemBase()
{
	delete [] path;
}


int CacheItemBase::get_size()
{
	return 0;
}

CacheBase::CacheBase()
 : List<CacheItemBase>()
{
	lock = new Mutex("CacheBase::lock");
	current_item = 0;
	total_items = 0;
}

CacheBase::~CacheBase()
{
	delete lock;
}



int CacheBase::get_age()
{
	return EDL::next_id();
}


// Called when done with the item returned by get_.
// Ignore if item was 0.
void CacheBase::unlock()
{
	lock->unlock();
}

void CacheBase::remove_all()
{
//printf("CacheBase::remove_all: removed %d entries\n", total_items);
	lock->lock("CacheBase::remove_all");
	while(last) delete last;
	total_items = 0;
	current_item = 0;
	lock->unlock();
}


void CacheBase::remove_asset(Asset *asset)
{
	CacheItemBase *current, *next;
	lock->lock("CacheBase::remove_id");
	for( current=first; current; current=next ) {
		next = current->next;
		if( (current->path && !strcmp(current->path, asset->path)) ||
			current->source_id == asset->id)
			del_item(current);
	}
	lock->unlock();
//printf("CacheBase::remove_asset: removed %d entries for %s\n", total, asset->path);
}


void CacheBase::del_item(CacheItemBase *item)
{
	--total_items;
	if( current_item == item )
		current_item = 0;
	delete item;
}

int CacheBase::delete_oldest()
{
	int result = 0;
	lock->lock("CacheBase::delete_oldest");
	CacheItemBase *oldest, *current;
	for( oldest=current=first; current; current=NEXT ) {
		if( current->age < oldest->age )
			oldest = current;
	}
	if( oldest && oldest->position >= 0 ) {
		result = oldest->get_size();
		del_item(oldest);
	}
	lock->unlock();
	return result;
}

int CacheBase::delete_item(CacheItemBase *item)
{
	lock->lock("CacheBase::delete_item");
// Too much data to debug if audio.
// printf("CacheBase::delete_oldest: deleted position=%jd %d bytes\n",
// oldest_item->position, oldest_item->get_size());
	del_item(item);
	lock->unlock();
	return 0;
}


int64_t CacheBase::get_memory_usage()
{
	int64_t result = 0;
	lock->lock("CacheBase::get_memory_usage");
	for(CacheItemBase *current = first; current; current = NEXT)
	{
		result += current->get_size();
	}
	lock->unlock();
	return result;
}


void CacheBase::put_item(CacheItemBase *item)
{
// Get first position >= item
	int64_t position = item->position;
	if( !current_item && first ) {
		int64_t lt_dist = first ? position - first->position : 0;
		if( lt_dist < 0 ) lt_dist = 0;
		int64_t rt_dist = last ? last->position - position : 0;
		if( rt_dist < 0 ) rt_dist = 0;
		current_item = lt_dist < rt_dist ? first : last;
	}
	while(current_item && current_item->position > position)
		current_item = current_item->previous;
	if( !current_item ) current_item = first;
	while(current_item && position > current_item->position)
		current_item = current_item->next;
	insert_before(current_item, item);
	++total_items;
}


// Get first item from list with matching position or 0 if none found.
CacheItemBase* CacheBase::get_item(int64_t position)
{
// Get first position >= item
	int64_t dist = 0x7fffffffffffffff;
	if( current_item ) {
		dist = current_item->position - position;
		if( dist < 0 ) dist = -dist;
	}
	if( first ) {
		int64_t lt_dist = position - first->position;
		int64_t rt_dist = last->position - position;
		if( lt_dist < rt_dist ) {
			if( lt_dist < dist ) current_item = first;
		}
		else {
			if( rt_dist < dist ) current_item = last;
		}
	}
	while(current_item && current_item->position < position)
		current_item = current_item->next;
	while(current_item && current_item->position >= position )
		current_item = current_item->previous;
// forward one item
	current_item = current_item ? current_item->next : first;
	CacheItemBase *result = current_item && current_item->position==position ?
		current_item : 0;
	return result;
}


void CacheBase::age_cache(int64_t max_items)
{
	lock->lock("CacheBase::age_cache");
	if( total_items > max_items ) {
		CacheItemBase *items[total_items];
		int n = 0;
		for( CacheItemBase *current=first; current; current=NEXT ) {
			// insert items into heap
			CacheItemBase *item = current;
			int i, k;
			int v = item->age;
			for( i=n++; i>0 && v<items[k=(i-1)/2]->age; i=k )
				items[i] = items[k];
			items[i] = item;
		}
		//int starting_items = total_items;
		while( n > 0 && total_items > max_items ) {
			// delete lowest heap item
			CacheItemBase *item = items[0];
			del_item(item);
			int i, k;
			for( i=0; (k=2*(i+1)) < n; i=k ) {
				if( items[k]->age > items[k-1]->age ) --k;
				items[i] = items[k];
			}
			item = items[--n];
			int v = item->age;
			for( ; i>0 && v<items[k=(i-1)/2]->age; i=k )
				items[i] = items[k];
			items[i] = item;
		}
	}
	lock->unlock();
//printf("age_cache total_items=%d+%d items\n", total_items,starting_items-total_items);
}

