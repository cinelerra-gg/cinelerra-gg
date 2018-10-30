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


#ifndef MOTIONCACHE_HV_H
#define MOTIONCACHE_HV_H

// store previously scaled intermediates here for operations 
// with multiple motion scans on the same frame.

#include "arraylist.h"
#include "mutex.inc"
#include "vframe.inc"

class MotionHVCacheItem
{
public:
	MotionHVCacheItem();
	~MotionHVCacheItem();

	VFrame *image;
	int ratio;
	int is_previous;
};


class MotionHVCache
{
public:
	MotionHVCache();
	~MotionHVCache();
	
	VFrame *get_image(int ratio, 
		int is_previous,
		int downsampled_w,
		int downsampled_h,
		VFrame *src);
	
	void clear();

	void downsample_frame(VFrame *dst, 
		VFrame *src, 
		int downsample);

	ArrayList<MotionHVCacheItem*> images;
	Mutex *lock;
};





#endif


