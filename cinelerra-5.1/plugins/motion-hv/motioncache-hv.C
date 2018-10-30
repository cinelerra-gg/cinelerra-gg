/*
 * CINELERRA
 * Copyright (C) 2016 Adam Williams <broadcast at earthling dot net>
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



// store previously scaled images here for operations 
// with multiple motion scans on the same frame.


#include "bcsignals.h"
#include "clip.h"
#include "motioncache-hv.h"
#include "mutex.h"
#include <stdint.h>
#include "vframe.h"


MotionHVCacheItem::MotionHVCacheItem()
{
}

MotionHVCacheItem::~MotionHVCacheItem()
{
//printf("MotionHVCacheItem::~MotionHVCacheItem %d image=%p\n", __LINE__, image);
	if(image)
	{
		delete image;
	}
}



MotionHVCache::MotionHVCache()
{
	lock = new Mutex("MotionHVCache::lock");
}

MotionHVCache::~MotionHVCache()
{
//printf("MotionHVCache::~MotionHVCache %d this=%p\n", __LINE__, this);

	delete lock;
	clear();
}

void MotionHVCache::clear()
{
	images.remove_all_objects();
}



#define DOWNSAMPLE(type, temp_type, components, max) \
{ \
	temp_type r; \
	temp_type g; \
	temp_type b; \
	temp_type a; \
	type **in_rows = (type**)src->get_rows(); \
	type **out_rows = (type**)dst->get_rows(); \
 \
	for(int i = 0; i < h; i += downsample) \
	{ \
		int y1 = MAX(i, 0); \
		int y2 = MIN(i + downsample, h); \
 \
 \
		for(int j = 0; \
			j < w; \
			j += downsample) \
		{ \
			int x1 = MAX(j, 0); \
			int x2 = MIN(j + downsample, w); \
 \
			temp_type scale = (x2 - x1) * (y2 - y1); \
			if(x2 > x1 && y2 > y1) \
			{ \
 \
/* Read in values */ \
				r = 0; \
				g = 0; \
				b = 0; \
				if(components == 4) a = 0; \
 \
				for(int k = y1; k < y2; k++) \
				{ \
					type *row = in_rows[k] + x1 * components; \
					for(int l = x1; l < x2; l++) \
					{ \
						r += *row++; \
						g += *row++; \
						b += *row++; \
						if(components == 4) a += *row++; \
					} \
				} \
 \
/* Write average */ \
				r /= scale; \
				g /= scale; \
				b /= scale; \
				if(components == 4) a /= scale; \
 \
				type *row = out_rows[y1 / downsample] + \
					x1 / downsample * components; \
				*row++ = r; \
				*row++ = g; \
				*row++ = b; \
				if(components == 4) *row++ = a; \
			} \
		} \
/*printf("DOWNSAMPLE 3 %d\n", i);*/ \
	} \
}




void MotionHVCache::downsample_frame(VFrame *dst, 
	VFrame *src, 
	int downsample)
{
	int h = src->get_h();
	int w = src->get_w();

//PRINT_TRACE
//printf("downsample=%d w=%d h=%d dst=%d %d\n", downsample, w, h, dst->get_w(), dst->get_h());
	switch(src->get_color_model())
	{
		case BC_RGB888:
			DOWNSAMPLE(uint8_t, int64_t, 3, 0xff)
			break;
		case BC_RGB_FLOAT:
			DOWNSAMPLE(float, float, 3, 1.0)
			break;
		case BC_RGBA8888:
			DOWNSAMPLE(uint8_t, int64_t, 4, 0xff)
			break;
		case BC_RGBA_FLOAT:
			DOWNSAMPLE(float, float, 4, 1.0)
			break;
		case BC_YUV888:
			DOWNSAMPLE(uint8_t, int64_t, 3, 0xff)
			break;
		case BC_YUVA8888:
			DOWNSAMPLE(uint8_t, int64_t, 4, 0xff)
			break;
	}
//PRINT_TRACE
}

VFrame* MotionHVCache::get_image(int ratio, 
	int is_previous,
	int downsampled_w,
	int downsampled_h,
	VFrame *src)
{
	lock->lock("MotionHVCache::get_image 1");
	
	for(int i = 0; i < images.size(); i++)
	{
		if(images.get(i)->ratio == ratio &&
			images.get(i)->is_previous == is_previous)
		{
			VFrame *result = images.get(i)->image;
			lock->unlock();
			return result;
		}
	}


//PRINT_TRACE
	VFrame *result =
		new VFrame(downsampled_w+1, downsampled_h+1, src->get_color_model(), 0);
	downsample_frame(result, src, ratio);
	
	MotionHVCacheItem *item = new MotionHVCacheItem();
	item->image = result;
	item->is_previous = is_previous;
	item->ratio = ratio;
	images.append(item);

	lock->unlock();
	
	return result;
}



















