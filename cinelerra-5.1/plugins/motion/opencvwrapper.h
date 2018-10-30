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



#ifndef OPENCVWRAPPER_H
#define OPENCVWRAPPER_H

#include "opencv2/core/core_c.h"

#include "vframe.inc"

class OpenCVWrapper
{
public:
	OpenCVWrapper();
	~OpenCVWrapper();


	float get_dst_x(int number);
	float get_dst_y(int number);

	void grey_crop(unsigned char *dst, 
		VFrame *src, 
		int x1, 
		int y1,
		int x2,
		int y2,
		int dst_w,
		int dst_h);

// Returns 1 when it got something
	int scan(VFrame *object_frame,
		VFrame *scene_frame,
		int object_x1, 
		int object_y1,
		int object_x2,
		int object_y2,
		int scene_x1,
		int scene_y1,
		int scene_x2,
		int scene_y2);

private:
	int locatePlanarObject(const CvSeq* objectKeypoints, 
		const CvSeq* objectDescriptors,
    	const CvSeq* imageKeypoints, 
		const CvSeq* imageDescriptors,
    	const CvPoint src_corners[4], 
		int *(*point_pairs),
		int (*total_pairs));

// Images in the format OpenCV requires
	IplImage *object_image;
	IplImage *scene_image;
// Quantized sizes
	int object_image_w;
	int object_image_h;
	int scene_image_w;
	int scene_image_h;
	CvSeq *object_keypoints;
	CvSeq *object_descriptors;
	CvSeq *scene_keypoints;
	CvSeq *scene_descriptors;
	CvMemStorage *storage;
	int *point_pairs;
	int total_pairs;

// x, y pairs
	float dst_corners[8];
};




#endif


