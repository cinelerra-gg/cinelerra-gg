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



#include "opencvwrapper.h"
#include "vframe.h"

#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/features2d/features2d.hpp"


#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>




using namespace std;


// define whether to use approximate nearest-neighbor search
#define USE_FLANN

// Sizes must be quantized a certain amount for OpenCV
#define QUANTIZE 8



double
compareSURFDescriptors( const float* d1, const float* d2, double best, int length )
{
    double total_cost = 0;
    assert( length % 4 == 0 );
    for( int i = 0; i < length; i += 4 )
    {
        double t0 = d1[i  ] - d2[i  ];
        double t1 = d1[i+1] - d2[i+1];
        double t2 = d1[i+2] - d2[i+2];
        double t3 = d1[i+3] - d2[i+3];
        total_cost += t0*t0 + t1*t1 + t2*t2 + t3*t3;
        if( total_cost > best )
            break;
    }
    return total_cost;
}


int
naiveNearestNeighbor( const float* vec, int laplacian,
                      const CvSeq* model_keypoints,
                      const CvSeq* model_descriptors )
{
    int length = (int)(model_descriptors->elem_size/sizeof(float));
    int i, neighbor = -1;
    double d, dist1 = 1e6, dist2 = 1e6;
    CvSeqReader reader, kreader;
    cvStartReadSeq( model_keypoints, &kreader, 0 );
    cvStartReadSeq( model_descriptors, &reader, 0 );

    for( i = 0; i < model_descriptors->total; i++ )
    {
        const CvSURFPoint* kp = (const CvSURFPoint*)kreader.ptr;
        const float* mvec = (const float*)reader.ptr;
    	CV_NEXT_SEQ_ELEM( kreader.seq->elem_size, kreader );
        CV_NEXT_SEQ_ELEM( reader.seq->elem_size, reader );
        if( laplacian != kp->laplacian )
            continue;
        d = compareSURFDescriptors( vec, mvec, dist2, length );
        if( d < dist1 )
        {
            dist2 = dist1;
            dist1 = d;
            neighbor = i;
        }
        else if ( d < dist2 )
            dist2 = d;
    }
    if ( dist1 < 0.6*dist2 )
        return neighbor;
    return -1;
}

void
findPairs( const CvSeq* objectKeypoints, const CvSeq* objectDescriptors,
           const CvSeq* imageKeypoints, const CvSeq* imageDescriptors, vector<int>& ptpairs )
{
    int i;
    CvSeqReader reader, kreader;
    cvStartReadSeq( objectKeypoints, &kreader );
    cvStartReadSeq( objectDescriptors, &reader );
    ptpairs.clear();

    for( i = 0; i < objectDescriptors->total; i++ )
    {
        const CvSURFPoint* kp = (const CvSURFPoint*)kreader.ptr;
        const float* descriptor = (const float*)reader.ptr;
        CV_NEXT_SEQ_ELEM( kreader.seq->elem_size, kreader );
        CV_NEXT_SEQ_ELEM( reader.seq->elem_size, reader );
        int nearest_neighbor = naiveNearestNeighbor( descriptor, kp->laplacian, imageKeypoints, imageDescriptors );
        if( nearest_neighbor >= 0 )
        {
            ptpairs.push_back(i);
            ptpairs.push_back(nearest_neighbor);
        }
    }
}


void
flannFindPairs( const CvSeq*, 
	const CvSeq* objectDescriptors,
    const CvSeq*, 
	const CvSeq* imageDescriptors, 
	vector<int>& ptpairs )
{
	int length = (int)(objectDescriptors->elem_size/sizeof(float));

    cv::Mat m_object(objectDescriptors->total, length, CV_32F);
	cv::Mat m_image(imageDescriptors->total, length, CV_32F);


	// copy descriptors
    CvSeqReader obj_reader;
	float* obj_ptr = m_object.ptr<float>(0);
    cvStartReadSeq( objectDescriptors, &obj_reader );
    for(int i = 0; i < objectDescriptors->total; i++ )
    {
        const float* descriptor = (const float*)obj_reader.ptr;
        CV_NEXT_SEQ_ELEM( obj_reader.seq->elem_size, obj_reader );
        memcpy(obj_ptr, descriptor, length*sizeof(float));
        obj_ptr += length;
    }
    CvSeqReader img_reader;
	float* img_ptr = m_image.ptr<float>(0);
    cvStartReadSeq( imageDescriptors, &img_reader );
    for(int i = 0; i < imageDescriptors->total; i++ )
    {
        const float* descriptor = (const float*)img_reader.ptr;
        CV_NEXT_SEQ_ELEM( img_reader.seq->elem_size, img_reader );
        memcpy(img_ptr, descriptor, length*sizeof(float));
        img_ptr += length;
    }

    // find nearest neighbors using FLANN
    cv::Mat m_indices(objectDescriptors->total, 2, CV_32S);
    cv::Mat m_dists(objectDescriptors->total, 2, CV_32F);
    cv::flann::Index flann_index(m_image, cv::flann::KDTreeIndexParams(4));  // using 4 randomized kdtrees
    flann_index.knnSearch(m_object, m_indices, m_dists, 2, cv::flann::SearchParams(64) ); // maximum number of leafs checked

    int* indices_ptr = m_indices.ptr<int>(0);
    float* dists_ptr = m_dists.ptr<float>(0);
//printf("flannFindPairs %d m_indices.rows=%d\n", __LINE__, m_indices.rows);
    for (int i = 0; i < m_indices.rows; ++i) 
	{
//printf("flannFindPairs %d dists=%f %f\n", __LINE__, dists_ptr[2 * i], 0.6 * dists_ptr[2 * i + 1]);
    	if (dists_ptr[2 * i] < 0.6 * dists_ptr[2 * i + 1]) 
		{
//printf("flannFindPairs %d pairs=%d\n", __LINE__, ptpairs.size());
    		ptpairs.push_back(i);
    		ptpairs.push_back(indices_ptr[2*i]);
    	}
    }
}


/* a rough implementation for object location */
int OpenCVWrapper::locatePlanarObject(const CvSeq* objectKeypoints, 
	const CvSeq* objectDescriptors,
    const CvSeq* imageKeypoints, 
	const CvSeq* imageDescriptors,
    const CvPoint src_corners[4], 
	int *(*point_pairs),
	int (*total_pairs))
{
    double h[9];
    CvMat _h = cvMat(3, 3, CV_64F, h);
    vector<int> ptpairs;
    vector<CvPoint2D32f> pt1, pt2;
    CvMat _pt1, _pt2;
    int i, n;
	
	(*point_pairs) = 0;
	(*total_pairs) = 0;

#ifdef USE_FLANN
    flannFindPairs( objectKeypoints, objectDescriptors, imageKeypoints, imageDescriptors, ptpairs );
#else
    findPairs( objectKeypoints, objectDescriptors, imageKeypoints, imageDescriptors, ptpairs );
#endif


// Store keypoints
	(*point_pairs) = (int*)calloc(ptpairs.size(), sizeof(int));
	(*total_pairs) = ptpairs.size() / 2;
	
	
    for(int i = 0; i < (int)ptpairs.size(); i++)
    {
		(*point_pairs)[i] = ptpairs[i];
    }



    n = (int)(ptpairs.size()/2);
    if( n < 4 )
        return 0;

    pt1.resize(n);
    pt2.resize(n);
    for( i = 0; i < n; i++ )
    {
        pt1[i] = ((CvSURFPoint*)cvGetSeqElem(objectKeypoints,ptpairs[i*2]))->pt;
        pt2[i] = ((CvSURFPoint*)cvGetSeqElem(imageKeypoints,ptpairs[i*2+1]))->pt;
    }

    _pt1 = cvMat(1, n, CV_32FC2, &pt1[0] );
    _pt2 = cvMat(1, n, CV_32FC2, &pt2[0] );
    if( !cvFindHomography( &_pt1, &_pt2, &_h, CV_RANSAC, 5 ))
        return 0;

    for( i = 0; i < 4; i++ )
    {
        double x = src_corners[i].x, y = src_corners[i].y;
        double Z = 1./(h[6]*x + h[7]*y + h[8]);
        double X = (h[0]*x + h[1]*y + h[2])*Z;
        double Y = (h[3]*x + h[4]*y + h[5])*Z;
        dst_corners[i * 2] = X;
		dst_corners[i * 2 + 1] = Y;
    }

    return 1;
}




OpenCVWrapper::OpenCVWrapper()
{
	object_image = 0;
	scene_image = 0;
	object_image_w = 0;
	object_image_h = 0;
	scene_image_w = 0;
	scene_image_h = 0;
	storage = 0;
	object_keypoints = 0;
	object_descriptors = 0;
	scene_keypoints = 0;
	scene_descriptors = 0;
	point_pairs = 0;
	total_pairs = 0;
}






OpenCVWrapper::~OpenCVWrapper()
{
// This releases all the arrays
	if(storage) cvReleaseMemStorage(&storage);
	if(object_image) cvReleaseImage(&object_image);
	if(scene_image) cvReleaseImage(&scene_image);
	if(point_pairs) free(point_pairs);
}





int OpenCVWrapper::scan(VFrame *object_frame,
	VFrame *scene_frame,
	int object_x1, 
	int object_y1,
	int object_x2,
	int object_y2,
	int scene_x1,
	int scene_y1,
	int scene_x2,
	int scene_y2)
{
	int result = 0;
	int object_w = object_x2 - object_x1;
	int object_h = object_y2 - object_y1;
	int scene_w = scene_x2 - scene_x1;
	int scene_h = scene_y2 - scene_y1;


//object_frame->write_png("/tmp/object.png");
//scene_frame->write_png("/tmp/scene.png");

// Get quantized sizes
	int object_image_w = object_w;
	int object_image_h = object_h;
	int scene_image_w = scene_w;
	int scene_image_h = scene_h;
	if(object_w % QUANTIZE) object_image_w += QUANTIZE - (object_w % QUANTIZE);
	if(object_h % QUANTIZE) object_image_h += QUANTIZE - (object_h % QUANTIZE);
	if(scene_w % QUANTIZE) scene_image_w += QUANTIZE - (scene_w % QUANTIZE);
	if(scene_h % QUANTIZE) scene_image_h += QUANTIZE - (scene_h % QUANTIZE);

	if(object_image && 
		(object_image_w != this->object_image_w ||
		object_image_h != this->object_image_h))
	{
		cvReleaseImage(&object_image);
		object_image = 0;
	}
	this->object_image_w = object_image_w;
	this->object_image_h = object_image_h;

	if(scene_image && 
		(scene_image_w != this->scene_image_w ||
		scene_image_h != this->scene_image_h))
	{
		cvReleaseImage(&scene_image);
		scene_image = 0;
	}
	this->scene_image_w = scene_image_w;
	this->scene_image_h = scene_image_h;


	if(!object_image)
	{
// Only does greyscale
		object_image = cvCreateImage( 
			cvSize(object_image_w, object_image_h), 
			8, 
			1);
	}

	if(!scene_image)
	{
// Only does greyscale
		scene_image = cvCreateImage( 
			cvSize(scene_image_w, scene_image_h), 
			8, 
			1);
	}

// Select only region with image size
// Does this do anything?
	cvSetImageROI( object_image, cvRect( 0, 0, object_w, object_h ) );
	cvSetImageROI( scene_image, cvRect( 0, 0, scene_w, scene_h ) );

	grey_crop((unsigned char*)scene_image->imageData, 
		scene_frame, 
		scene_x1, 
		scene_y1, 
		scene_x2, 
		scene_y2,
		scene_image_w,
		scene_image_h);
	grey_crop((unsigned char*)object_image->imageData, 
		object_frame, 
		object_x1, 
		object_y1, 
		object_x2, 
		object_y2,
		object_image_w,
		object_image_h);


	if(!storage) storage = cvCreateMemStorage(0);
	CvSURFParams params = cvSURFParams(500, 1);


//printf("OpenCVWrapper::process_buffer %d\n", __LINE__);

// TODO: make the surf data persistent & check for image changes instead
	if(object_keypoints) cvClearSeq(object_keypoints);
	if(object_descriptors) cvClearSeq(object_descriptors);
	if(scene_keypoints) cvClearSeq(scene_keypoints);
	if(scene_descriptors) cvClearSeq(scene_descriptors);
	object_keypoints = 0;
	object_descriptors = 0;
	scene_keypoints = 0;
	scene_descriptors = 0;

// Free the image structures
	if(point_pairs) free(point_pairs);
	point_pairs = 0;


	cvExtractSURF(object_image, 
		0, 
		&object_keypoints, 
		&object_descriptors, 
		storage, 
		params,
		0);

//printf("OpenCVWrapper::scan %d object keypoints=%d\n", __LINE__, object_keypoints->total);
// Draw the keypoints
// 		for(int i = 0; i < object_keypoints->total; i++)
// 		{
//         	CvSURFPoint* r1 = (CvSURFPoint*)cvGetSeqElem( object_keypoints, i );
// 			int size = r1->size / 4;
// 			draw_rect(frame[object_layer], 
//   				r1->pt.x + object_x1 - size, 
//   				r1->pt.y + object_y1 - size, 
//   				r1->pt.x + object_x1 + size, 
//  				r1->pt.y + object_y1 + size);
// 		}


//printf("OpenCVWrapper::process_buffer %d\n", __LINE__);

	cvExtractSURF(scene_image, 
		0, 
		&scene_keypoints, 
		&scene_descriptors, 
		storage, 
		params,
		0);

// Draw the keypoints
// 		for(int i = 0; i < scene_keypoints->total; i++)
// 		{
//         	CvSURFPoint* r1 = (CvSURFPoint*)cvGetSeqElem( scene_keypoints, i );
// 			int size = r1->size / 4;
// 			draw_rect(frame[scene_layer], 
//   				r1->pt.x + scene_x1 - size, 
//   				r1->pt.y + scene_y1 - size, 
//   				r1->pt.x + scene_x1 + size, 
//  				r1->pt.y + scene_y1 + size);
// 		}

// printf("OpenCVWrapper::scan %d %d %d scene keypoints=%d\n", 
// __LINE__, 
// scene_w,
// scene_h,
// scene_keypoints->total);

	CvPoint src_corners[4] = 
	{
		{ 0, 0 }, 
		{ object_w, 0 }, 
		{ object_w, object_h }, 
		{ 0, object_h }
	};

	for(int i = 0; i < 8; i++)
	{
		dst_corners[i] = 0;
	}



//printf("OpenCVWrapper::process_buffer %d\n", __LINE__);
	if(scene_keypoints->total &&
		object_keypoints->total &&
		locatePlanarObject(object_keypoints, 
			object_descriptors, 
			scene_keypoints, 
			scene_descriptors, 
			src_corners, 
			&point_pairs,
			&total_pairs))
	{
		result = 1;
	}


	return result;
}






// Convert to greyscale & crop
void OpenCVWrapper::grey_crop(unsigned char *dst, 
	VFrame *src, 
	int x1, 
	int y1,
	int x2,
	int y2,
	int dst_w,
	int dst_h)
{
// Dimensions of dst frame
	int w = x2 - x1;
	int h = y2 - y1;

	bzero(dst, dst_w * dst_h);

//printf("OpenCVWrapper::grey_crop %d %d %d\n", __LINE__, w, h);
	for(int i = 0; i < h; i++)
	{
		switch(src->get_color_model())
		{
			case BC_RGB888:
				break;

			case BC_YUV888:
			{
				unsigned char *input = src->get_rows()[i + y1] + x1 * 3;
				unsigned char *output = dst + i * dst_w;

				for(int j = 0; j < w; j++)
				{
// Y channel only
					*output = *input;
					input += 3;
					output++;
				}
				break;
			}
		}
	}
}


float OpenCVWrapper::get_dst_x(int number)
{
	return dst_corners[number * 2];
}

float OpenCVWrapper::get_dst_y(int number)
{
	return dst_corners[number * 2 + 1];
}





