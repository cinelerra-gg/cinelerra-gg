
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
#include "clip.h"
#include "cstrdup.h"
#include "edl.h"
#include "framecache.h"
#include "indexable.h"
#include "mutex.h"
#include "vframe.h"


#include <limits.h>
#include <math.h>
#include <string.h>
#include <unistd.h>



FrameCacheItem::FrameCacheItem()
 : CacheItemBase()
{
	data = 0;
	position = 0;
	frame_rate = (double)30000.0 / 1001;
}

FrameCacheItem::~FrameCacheItem()
{
	delete data;
}

int FrameCacheItem::get_size()
{
	if(data) return data->get_data_size() + (path ? strlen(path)+1 : 0);
	return 0;
}















FrameCache::FrameCache()
 : CacheBase()
{
}

FrameCache::~FrameCache()
{
}


// Returns 1 if frame exists in cache and copies it to the frame argument.
int FrameCache::get_frame(VFrame *frame,
	int64_t position,
	int layer,
	double frame_rate,
	int source_id)
{
	lock->lock("FrameCache::get_frame");
	FrameCacheItem *result = 0;

	if(frame_exists(frame,
		position,
		layer,
		frame_rate,
		&result,
		source_id))
	{
		if(result->data)
		{
// Frame may have come from the readahead thread.
// Those frames are in the codec color model.
// But to pass frame_exists, they must be identical.
// 			BC_CModels::transfer(frame->get_rows(),
// 				result->data->get_rows(),
// 				result->data->get_y(),
// 				result->data->get_u(),
// 				result->data->get_v(),
// 				frame->get_y(),
// 				frame->get_u(),
// 				frame->get_v(),
// 				0,
// 				0,
// 				result->data->get_w(),
// 				result->data->get_h(),
// 				0,
// 				0,
// 				frame->get_w(),
// 				frame->get_h(),
// 				result->data->get_color_model(),
// 				frame->get_color_model(),
// 				0,
// 				result->data->get_w(),
// 				frame->get_w());

// no context data since keyframe updates may vary input
			frame->copy_from(result->data);
		}
		result->age = get_age();
	}




	lock->unlock();
	if(result) return 1;
	return 0;
}


VFrame* FrameCache::get_frame_ptr(int64_t position,
	int layer,
	double frame_rate,
	int color_model,
	int w,
	int h,
	int source_id)
{
	lock->lock("FrameCache::get_frame_ptr");
	FrameCacheItem *result = 0;
	if(frame_exists(position,
		layer,
		frame_rate,
		color_model,
		w,
		h,
		&result,
		source_id))
	{
		result->age = get_age();
		return result->data;
	}


	lock->unlock();
	return 0;
}

// Puts frame in cache if enough space exists and the frame doesn't already
// exist.
void FrameCache::put_frame(VFrame *frame, int64_t position,
	int layer, double frame_rate, int use_copy, Indexable *indexable)
{
	lock->lock("FrameCache::put_frame");
	FrameCacheItem *item = 0;
	int source_id = -1;
	if(indexable) source_id = indexable->id;

//printf("FrameCache::put_frame %d position=%jd\n", __LINE__, position);

	if(frame_exists(frame, position, layer, frame_rate, &item, source_id)) {
		item->age = get_age();
		lock->unlock();
		return;
	}


	item = new FrameCacheItem;

	item->data = use_copy ? new VFrame(*frame) : frame;

// Copy metadata
	item->position = position;
	item->layer = layer;
	item->frame_rate = frame_rate;
	item->source_id = source_id;
	if(indexable)
		item->path = cstrdup(indexable->path);

	item->age = position < 0 ? INT_MAX : get_age();

//printf("FrameCache::put_frame %d position=%jd\n", __LINE__, position);
	put_item(item);
	lock->unlock();
}




int FrameCache::frame_exists(VFrame *format, int64_t position,
	int layer, double frame_rate, FrameCacheItem **item_return, int source_id)
{
	FrameCacheItem *item = (FrameCacheItem*)get_item(position);
// printf("FrameCache::frame_exists %d item=%p item->position=%jd position=%jd\n",
// __LINE__,
// item,
// item ? item->position : 0,
// position);

	while(item && item->position == position)
	{
// printf("FrameCache::frame_exists %d %f,%f %d,%d %d,%d format match=%d item->data=%p\n",
// __LINE__,
// item->frame_rate,
// frame_rate,
// item->layer,
// layer,
// item->source_id,
// source_id,
// format->equivalent(item->data, 1),
// item->data);
// format->dump_params();

// This originally tested the frame stacks because a change in the
// interpolate plugin could cause CR2 to interpolate or not interpolate.
// This was disabled.
		if(EQUIV(item->frame_rate, frame_rate) &&
			layer == item->layer &&
			format->equivalent(item->data, 0) &&
			(source_id == -1 || item->source_id == -1 || source_id == item->source_id))
		{
			*item_return = item;
			return 1;
		}
		else
			item = (FrameCacheItem*)item->next;
	}
	return 0;
}

int FrameCache::frame_exists(int64_t position,
	int layer,
	double frame_rate,
	int color_model,
	int w,
	int h,
	FrameCacheItem **item_return,
	int source_id)
{
	FrameCacheItem *item = (FrameCacheItem*)get_item(position);
	while(item && item->position == position)
	{
// printf("FrameCache::frame_exists %d %f,%f %d,%d %d,%d %d,%d\n",
// __LINE__,
// item->frame_rate,
// frame_rate,
// item->layer,
// layer,
// item->data->get_color_model(),
// color_model,
// item->data->get_w(),
// w,
// item->data->get_h(),
// h);

		if(EQUIV(item->frame_rate, frame_rate) &&
			layer == item->layer &&
			color_model == item->data->get_color_model() &&
			w == item->data->get_w() &&
			h == item->data->get_h() &&
			(source_id == -1 || item->source_id == -1 || source_id == item->source_id))
		{
			*item_return = item;
			return 1;
		}
		else
			item = (FrameCacheItem*)item->next;
	}
	return 0;
}


void FrameCache::dump()
{
// 	lock->lock("FrameCache::dump");
 	printf("FrameCache::dump 1 %d\n", total());
 	FrameCacheItem *item = (FrameCacheItem *)first;
	while( item ) {
 		printf("  position=%jd frame_rate=%f age=%d size=%ld\n",
 			item->position, item->frame_rate, item->age,
 			item->data->get_data_size());
		item = (FrameCacheItem*)item->next;
 	}
// 	lock->unlock();
}




