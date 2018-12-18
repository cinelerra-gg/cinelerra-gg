
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

#ifndef __BCBITMAP_H__
#define __BCBITMAP_H__

#include <stdint.h>
#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>

#include "bcwindowbase.inc"
#include "bcbitmap.inc"
#include "bccmodels.h"
#include "bccolors.h"
#include "condition.h"
#include "linklist.h"
#include "mutex.h"
#include "sizes.h"
#include "vframe.inc"

#define MIN_BITMAP_BUFFERS 4
#define MAX_BITMAP_BUFFERS 32



class BC_BitmapImage : public ListItem<BC_BitmapImage> {
	int index;
	BC_Bitmap *bitmap;
	union { XImage *ximage; XvImage *xv_image; };
	BC_WindowBase *top_level;
	Drawable drawable;
	unsigned char *data;
	unsigned char **row_data;
	int bitsPerPixel;
	long bytesPerLine;
	long dataSize;
	friend class BC_Bitmap;
	friend class BC_XImage;
	friend class BC_XShmImage;
	friend class BC_XvImage;
	friend class BC_XvShmImage;
	friend class BC_ActiveBitmaps;
protected:
	int read_frame_rgb(VFrame* frame);
public:
	BC_BitmapImage(BC_Bitmap *bitmap, int index);
	virtual ~BC_BitmapImage();
	unsigned char *get_data() { return data; }
	unsigned char **get_row_data() { return row_data; }
	virtual long get_data_size() { return dataSize; }
	int bits_per_pixel() { return bitsPerPixel; }
	long bytes_per_line() { return bytesPerLine; }
	virtual long xv_offset(int i) { return 0; }
	virtual unsigned char* xv_plane(int i) { return 0; }
	virtual int get_shmid() { return 0; }
	virtual ShmSeg get_shmseg() { return 0; }
	long get_y_offset() { return xv_offset(0); }
	long get_u_offset() { return xv_offset(2); }
	long get_v_offset() { return xv_offset(1); }
	unsigned char *get_y_data() { return xv_plane(0); }
	unsigned char *get_u_data() { return xv_plane(2); }
	unsigned char *get_v_data() { return xv_plane(1); }
	virtual int write_drawable(Drawable &pixmap, GC &gc,
		int source_x, int source_y, int source_w, int source_h,
		int dest_x, int dest_y, int dest_w, int dest_h) {
		return 0;
	}
	virtual int read_drawable(Drawable &pixmap, int source_x, int source_y) {
		return 0;
	}
	bool is_avail();
	bool is_zombie() { return index < 0; }
};


class BC_XImage : public BC_BitmapImage {
public:
	BC_XImage(BC_Bitmap *bitmap, int idx, int w,int h, int color_model);
	~BC_XImage();
	int write_drawable(Drawable &pixmap, GC &gc,
		int source_x, int source_y, int source_w, int source_h,
		int dest_x, int dest_y, int dest_w, int dest_h);
	int read_drawable(Drawable &pixmap, int source_x, int source_y);
};

class BC_XShmImage : public BC_BitmapImage {
	XShmSegmentInfo shm_info;
	long shm_offset(int i) { return 0; }
	//long shm_offset(int i) {
	//  return (h*ximage->bytes_per_line) * BC_BitmapImage::index;
	//}
public:
	BC_XShmImage(BC_Bitmap *bitmap, int idx, int w,int h, int color_model);
	~BC_XShmImage();
	int get_shmid() { return shm_info.shmid; }
	ShmSeg get_shmseg() { return shm_info.shmseg; }
	int get_shm_size();
	int write_drawable(Drawable &pixmap, GC &gc,
		int source_x, int source_y, int source_w, int source_h,
		int dest_x, int dest_y, int dest_w, int dest_h);
	int read_drawable(Drawable &pixmap, int source_x, int source_y);
};

class BC_XvImage : public BC_BitmapImage {
	long xv_offset(int i) { return xv_image->offsets[i]; }
	unsigned char* xv_plane(int i) { return get_data() + xv_offset(i); }
public:
	BC_XvImage(BC_Bitmap *bitmap, int idx, int w,int h, int color_model);
	~BC_XvImage();
	int write_drawable(Drawable &pixmap, GC &gc,
		int source_x, int source_y, int source_w, int source_h,
		int dest_x, int dest_y, int dest_w, int dest_h);
};

class BC_XvShmImage : public BC_BitmapImage {
	XShmSegmentInfo shm_info;
	long shm_offset(int i) { return 0; }
	//long shm_offset(int i) {
	//	return xv_image->data_size*BC_BitmapImage::index;
	//}
	long xv_offset(int i) { return shm_offset(i) + xv_image->offsets[i]; }
	unsigned char* xv_plane(int i) { return data + xv_offset(i); }
public:
	BC_XvShmImage(BC_Bitmap *bitmap, int idx, int w,int h, int color_model);
	~BC_XvShmImage();
	int get_shmid() { return shm_info.shmid; }
	ShmSeg get_shmseg() { return shm_info.shmseg; }
	int get_shm_size() { return xv_image->data_size; }
	int write_drawable(Drawable &pixmap, GC &gc,
		int source_x, int source_y, int source_w, int source_h,
		int dest_x, int dest_y, int dest_w, int dest_h);
};



class BC_Bitmap
{
	friend class BC_XImage;
	friend class BC_XShmImage;
	friend class BC_XvImage;
	friend class BC_XvShmImage;
	friend class BC_BitmapImage;
	friend class BC_ActiveBitmaps;
	int buffer_count, max_buffer_count;
	int active_buffers;
	static int max_active_buffers;
	static int zombies;
	int w, h;
	int shm_reply, type;
	int color_model;
	int initialize(BC_WindowBase *parent_window, int w, int h, int color_model, int use_shm);
	BC_BitmapImage *new_buffer(int type, int idx);
	void update_buffers(int count, int lock_avail=1);
	int allocate_data();
	int delete_data();
	int get_default_depth();
	long best_buffer_size();
	int need_shm();

// Override top_level for small bitmaps
	int use_shm;
	BC_WindowBase *top_level;
	BC_WindowBase *parent_window;
	int xv_portid;

	static uint8_t bitswap[256];
	void transparency_bitswap(uint8_t *buf, int w, int h);
public:
	enum { bmXNone, bmXImage, bmXShmImage, bmXvImage, bmXvShmImage };

	BC_Bitmap(BC_WindowBase *parent_window, unsigned char *png_data, double scale=1);
	BC_Bitmap(BC_WindowBase *parent_window, VFrame *frame);

// Shared memory is a problem in X because it's asynchronous and there's
// no easy way to join with the blitting process.
	BC_Bitmap(BC_WindowBase *parent_window, int w, int h,
		int color_model, int use_shm = 1);
	virtual ~BC_Bitmap();

// transfer VFrame
	int read_frame(VFrame *frame,
		int in_x, int in_y, int in_w, int in_h,
		int out_x, int out_y, int out_w, int out_h,
		int bg_color);
// x1, y1, x2, y2 dimensions of output area
	int read_frame(VFrame *frame,
		int x1, int y1, int x2, int y2);
	int read_frame(VFrame *frame,
		int x1, int y1, int x2, int y2, int bg_color);
// Reset bitmap to match the new parameters
	int match_params(int w, int h, int color_model, int use_shm);
// Test if bitmap already matches parameters
	int params_match(int w, int h, int color_model, int use_shm);

// If dont_wait is true, the XSync comes before the flash.
// For YUV bitmaps, the image is scaled to fill dest_x ... w * dest_y ... h
	int write_drawable(Drawable &pixmap, GC &gc,
			int source_x, int source_y, int source_w, int source_h,
			int dest_x, int dest_y, int dest_w, int dest_h,
			int dont_wait);
	int write_drawable(Drawable &pixmap, GC &gc,
			int dest_x, int dest_y, int source_x, int source_y,
			int dest_w, int dest_h, int dont_wait);
// the bitmap must be wholly contained in the source during a GetImage
	int read_drawable(Drawable &pixmap, int source_x, int source_y, VFrame *frame=0);

	int rotate_90(int side);
// Data pointers for current ring buffer
	BC_BitmapImage **buffers;
	Mutex *avail_lock;
	List<BC_BitmapImage> avail;
	void reque(BC_BitmapImage *bfr);
	BC_BitmapImage *cur_bfr(), *active_bfr;
	unsigned char* get_data() { return cur_bfr()->get_data(); }
	unsigned char** get_row_pointers() { return cur_bfr()->get_row_data(); }
	long get_data_size() { return cur_bfr()->get_data_size(); }
	int get_shmid() { return cur_bfr()->get_shmid(); }
	unsigned char *get_y_plane() { return cur_bfr()->get_y_data(); }
	unsigned char *get_u_plane() { return cur_bfr()->get_u_data(); }
	unsigned char *get_v_plane() { return cur_bfr()->get_v_data(); }
	long get_y_offset() { return cur_bfr()->get_y_offset(); }
	long get_u_offset() { return cur_bfr()->get_u_offset(); }
	long get_v_offset() { return cur_bfr()->get_v_offset(); }
	int get_color_model() { return color_model; }
	int hardware_scaling() {
		return xv_portid < 0 ? 0 :
			(get_color_model() == BC_YUV420P ||
	//		get_color_model() == BC_YUV422P ||  not in bc_to_x
			get_color_model() == BC_YUV422) ? 1 : 0;
	}
	int get_w() { return w; }
	int get_h() { return h; }

	int get_image_type() { return type; }
	int is_xvideo() { return type==bmXvShmImage || type==bmXvImage; }
	int is_xwindow() { return type==bmXShmImage || type==bmXImage; }
	int is_shared() { return type==bmXvShmImage || type==bmXShmImage; }
	int is_unshared() { return type==bmXvImage  || type==bmXImage; }
	int is_zombie() { return cur_bfr()->is_zombie(); }

	int invert();
};

#endif
