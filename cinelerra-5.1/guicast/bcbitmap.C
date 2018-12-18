
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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

#include "bcbitmap.h"
#include "bcipc.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcwindow.h"
#include "bccmodels.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>
#include <X11/extensions/Xvlib.h>

int BC_Bitmap::max_active_buffers = 0;
int BC_Bitmap::zombies = 0;


BC_BitmapImage::BC_BitmapImage(BC_Bitmap *bitmap, int index)
{
	this->index = index;
	this->bitmap = bitmap;
	this->top_level = bitmap->top_level;
        this->drawable = 0;
        this->data = 0;
        this->row_data = 0;
	this->dataSize = 0;
        this->bytesPerLine = 0;
        this->bitsPerPixel = 0;
}

BC_BitmapImage::~BC_BitmapImage()
{
	delete [] data;
	delete [] row_data;
}

bool BC_BitmapImage::is_avail()
{
	return list == &bitmap->avail;
}

BC_BitmapImage *BC_Bitmap::cur_bfr()
{
	if( !active_bfr ) {
		avail_lock->lock("BC_Bitmap::cur_bfr 1");
		if( (!use_shm || top_level->is_running()) &&
		     (active_bfr=avail.first) != 0 )
			avail.remove_pointer(active_bfr);
		else {
			update_buffers(buffer_count+1, 0);
			if( (active_bfr=avail.first) != 0 )
				avail.remove_pointer(active_bfr);
			else
				active_bfr = new_buffer(type, -1);
		}
		avail_lock->unlock();
	}
	return active_bfr;
}

int BC_BitmapImage::
read_frame_rgb(VFrame *frame)
{
	int w = bitmap->w, h = bitmap->h;
	if( frame->get_w() != w || frame->get_h() != h ||
	    frame->get_color_model() != BC_RGB888 )
		frame->reallocate(0, 0, 0, 0, 0, w, h, BC_RGB888, -1);
	uint8_t *dp = frame->get_data();
	int rmask = ximage->red_mask;
	int gmask = ximage->green_mask;
	int bmask = ximage->blue_mask;
//due to a bug in the xserver, the mask is being cleared by XShm.C:402
//  when a faulty data in a XShmGetImage reply returns a zero visual
//workaround, use the original visual for the mask data
	if ( !rmask ) rmask = bitmap->top_level->vis->red_mask;
	if ( !gmask ) gmask = bitmap->top_level->vis->green_mask;
	if ( !bmask ) bmask = bitmap->top_level->vis->blue_mask;
	double fr = 255.0 / rmask;
	double fg = 255.0 / gmask;
	double fb = 255.0 / bmask;
	int nbyte = (ximage->bits_per_pixel+7)/8;
	int b0 = ximage->byte_order == MSBFirst ? 0 : nbyte-1;
	int bi = ximage->byte_order == MSBFirst ? 1 : -1;
	uint8_t *data = (uint8_t *)ximage->data;

	for( int y=0;  y<h; ++y ) {
		uint8_t *cp = data + y*ximage->bytes_per_line + b0;
		for( int x=0; x<w; ++x, cp+=nbyte ) {
			uint8_t *bp = cp;  uint32_t n = *bp;
			for( int i=nbyte; --i>0; n=(n<<8)|*bp ) bp += bi;
			*dp++ = (int)((n&rmask)*fr + 0.5);
			*dp++ = (int)((n&gmask)*fg + 0.5);
			*dp++ = (int)((n&bmask)*fb + 0.5);
		}
	}
	return 0;
}

void BC_Bitmap::reque(BC_BitmapImage *bfr)
{
	avail_lock->lock("BC_Bitmap::reque");
	--active_buffers;
	avail.append(bfr);
	avail_lock->unlock();
}

BC_XvShmImage::BC_XvShmImage(BC_Bitmap *bitmap, int index,
	int w, int h, int color_model)
 : BC_BitmapImage(bitmap, index)
{
	Display *display = top_level->get_display();
	int id = BC_CModels::bc_to_x(color_model);
// Create the XvImage
	xv_image = XvShmCreateImage(display, bitmap->xv_portid, id,
			0, w, h, &shm_info);
	dataSize = xv_image->data_size;
// Create the shared memory
	shm_info.shmid = shmget(IPC_PRIVATE,
		dataSize + 8, IPC_CREAT | 0777);
	if(shm_info.shmid < 0)
		perror("BC_XvShmImage::BC_XvShmImage shmget");
	data = (unsigned char *)shmat(shm_info.shmid, NULL, 0);
// This causes it to automatically delete when the program exits.
	shmctl(shm_info.shmid, IPC_RMID, 0);
// setting ximage->data stops BadValue
	xv_image->data = shm_info.shmaddr = (char*)data;
	shm_info.readOnly = 0;
// Get the real parameters
	w = xv_image->width;
	h = xv_image->height;
	if(!XShmAttach(top_level->display, &shm_info))
		perror("BC_XvShmImage::BC_XvShmImage XShmAttach");
	if( color_model == BC_YUV422 || color_model == BC_UVY422 ) {
	 	bytesPerLine = w*2;
		bitsPerPixel = 12;
		row_data = new unsigned char*[h];
		for( int i=0; i<h; ++i )
			row_data[i] = &data[i*bytesPerLine];
	}
}

BC_XvShmImage::~BC_XvShmImage()
{
	data = 0;
	XFree(xv_image);
	XShmDetach(top_level->display, &shm_info);
	shmdt(shm_info.shmaddr);
}


BC_XShmImage::BC_XShmImage(BC_Bitmap *bitmap, int index,
	int w, int h, int color_model)
 : BC_BitmapImage(bitmap, index)
{
	Display *display = top_level->display;
	Visual *visual = top_level->vis;
	int default_depth = bitmap->get_default_depth();
    	ximage = XShmCreateImage(display, visual,
		default_depth, default_depth == 1 ? XYBitmap : ZPixmap,
		(char*)NULL, &shm_info, w, h);
// Create shared memory
 	bytesPerLine = ximage->bytes_per_line;
	bitsPerPixel = ximage->bits_per_pixel;
	dataSize = h * bytesPerLine;
	shm_info.shmid = shmget(IPC_PRIVATE,
		dataSize + 8, IPC_CREAT | 0777);
	if(shm_info.shmid < 0)
		perror("BC_XShmImage::BC_XShmImage shmget");
	data = (unsigned char *)shmat(shm_info.shmid, NULL, 0);
// This causes it to automatically delete when the program exits.
	shmctl(shm_info.shmid, IPC_RMID, 0);
	ximage->data = shm_info.shmaddr = (char*)data;
	shm_info.readOnly = 0;
// Get the real parameters
	if(!XShmAttach(top_level->display, &shm_info))
		perror("BC_XShmImage::BC_XShmImage XShmAttach");
	row_data = new unsigned char*[h];
	for( int i=0; i<h; ++i )
		row_data[i] = &data[i*bytesPerLine];
}

BC_XShmImage::~BC_XShmImage()
{
	data = 0;
	ximage->data = 0;
	XDestroyImage(ximage);
	XShmDetach(top_level->display, &shm_info);
	XFlush(top_level->display);
	shmdt(shm_info.shmaddr);
}



BC_XvImage::BC_XvImage(BC_Bitmap *bitmap, int index,
	int w, int h, int color_model)
 : BC_BitmapImage(bitmap, index)
{
	Display *display = top_level->display;
	int id = BC_CModels::bc_to_x(color_model);
	xv_image = XvCreateImage(display, bitmap->xv_portid, id, 0, w, h);
	dataSize = xv_image->data_size;
	long data_sz = dataSize + 8;
	data = new unsigned char[data_sz];
	memset(data, 0, data_sz);
	xv_image->data = (char *) data;
	w = xv_image->width;
	h = xv_image->height;
	if( color_model == BC_YUV422 || color_model == BC_UVY422 ) {
	 	int bytesPerLine = w*2;
		row_data = new unsigned char*[h];
		for( int i=0; i<h; ++i )
			row_data[i] = &data[i*bytesPerLine];
	}
}

BC_XvImage::~BC_XvImage()
{
	XFree(xv_image);
}


BC_XImage::BC_XImage(BC_Bitmap *bitmap, int index,
	int w, int h, int color_model)
 : BC_BitmapImage(bitmap, index)
{
	Display *display = top_level->display;
	Visual *visual = top_level->vis;
	int default_depth = bitmap->get_default_depth();
	ximage = XCreateImage(display, visual, default_depth,
		default_depth == 1 ? XYBitmap : ZPixmap,
		0, (char*)data, w, h, 8, 0);
 	bytesPerLine = ximage->bytes_per_line;
	bitsPerPixel = ximage->bits_per_pixel;
	dataSize = h * bytesPerLine;
	long data_sz = dataSize + 8;
	data = new unsigned char[data_sz];
	memset(data, 0, data_sz);
	ximage->data = (char*) data;
	row_data = new unsigned char*[h];
	for( int i=0; i<h; ++i )
		row_data[i] = &data[i*bytesPerLine];
}

BC_XImage::~BC_XImage()
{
	delete [] data;
	data = 0;
	ximage->data = 0;
	XDestroyImage(ximage);
}


BC_Bitmap::BC_Bitmap(BC_WindowBase *parent_window, unsigned char *png_data, double scale)
{
// Decompress data into a temporary vframe
	VFramePng frame(png_data, scale);

	avail_lock = 0;
// Initialize the bitmap
	initialize(parent_window,
		frame.get_w(),
		frame.get_h(),
		parent_window->get_color_model(),
		0);

// Copy the vframe to the bitmap
	read_frame(&frame, 0, 0, w, h);
}

BC_Bitmap::BC_Bitmap(BC_WindowBase *parent_window, VFrame *frame)
{
	avail_lock = 0;
// Initialize the bitmap
	initialize(parent_window,
		frame->get_w(),
		frame->get_h(),
		parent_window->get_color_model(),
		0);

// Copy the vframe to the bitmap
	read_frame(frame, 0, 0, w, h);
}

BC_Bitmap::BC_Bitmap(BC_WindowBase *parent_window,
	int w, int h, int color_model, int use_shm)
{
	avail_lock = 0;
	initialize(parent_window, w, h, color_model, use_shm);
}

BC_Bitmap::~BC_Bitmap()
{
	delete_data();
	delete avail_lock;
}

int BC_Bitmap::initialize(BC_WindowBase *parent_window,
	int w, int h, int color_model, int use_shm)
{
	BC_Resources *resources = parent_window->get_resources();
	this->parent_window = parent_window;
	this->top_level = parent_window->top_level;
	this->xv_portid = resources->use_xvideo ? top_level->xvideo_port_id : -1;
	if( w < 1 ) w = 1;
	if( h < 1 ) h = 1;
	this->w = w;
	this->h = h;
	this->color_model = color_model;
	this->use_shm = !use_shm ? 0 : need_shm();
        this->shm_reply = this->use_shm && resources->shm_reply ? 1 : 0;
	// dont use shm for less than one page
	if( !this->avail_lock )
		this->avail_lock = new Mutex("BC_Bitmap::avail_lock");
	else
		this->avail_lock->reset();
	this->buffers = 0;

	this->active_bfr = 0;
	this->buffer_count = 0;
	this->max_buffer_count = 0;
	this->active_buffers = 0;
	allocate_data();
	return 0;
}

int BC_Bitmap::match_params(int w, int h, int color_model, int use_shm)
{
	if( use_shm ) use_shm = need_shm();
	if(this->w /* != */ < w || this->h /* != */ < h ||
	   this->color_model != color_model || this->use_shm != use_shm) {
		delete_data();
		initialize(parent_window, w, h, color_model, use_shm);
	}

	return 0;
}

int BC_Bitmap::params_match(int w, int h, int color_model, int use_shm)
{
	int result = 0;
	if( use_shm ) use_shm = need_shm();
	if(this->w == w && this->h == h && this->color_model == color_model) {
		if(use_shm == this->use_shm || use_shm == BC_INFINITY)
			result = 1;
	}
	return result;
}

BC_BitmapImage *BC_Bitmap::new_buffer(int type, int idx)
{
	BC_BitmapImage *buffer = 0;
	if( idx < 0 ) {
		if( type == bmXShmImage ) type = bmXImage;
		else if( type ==  bmXvShmImage ) type = bmXvImage;
	}
	switch( type ) {
	default:
	case bmXImage:
		buffer = new BC_XImage(this, idx, w, h, color_model);
		break;
	case bmXvImage:
		buffer = new BC_XvImage(this, idx, w, h, color_model);
		break;
	case bmXShmImage:
		buffer = new BC_XShmImage(this, idx, w, h, color_model);
		break;
	case bmXvShmImage:
		buffer = new BC_XvShmImage(this, idx, w, h, color_model);
		break;
	}
	if( buffer->is_zombie() ) ++zombies;
	return buffer;
}

void BC_Bitmap::
update_buffers(int count, int lock_avail)
{
	int i;
	// can deadlock in XReply (libxcb bug) without this lock
	//top_level->lock_window("BC_Bitmap::update_buffers");
	if( lock_avail ) avail_lock->lock("BC_Bitmap::update_buffers");
	if( count > max_buffer_count ) count = max_buffer_count;
	BC_BitmapImage **new_buffers = !count ? 0 : new BC_BitmapImage *[count];
	if( buffer_count < count ) {
		for( i=0; i<buffer_count; ++i )
			new_buffers[i] = buffers[i];
		while( i < count ) {
			BC_BitmapImage *buffer = new_buffer(type, i);
			new_buffers[i] = buffer;
			avail.append(buffer);
			++i;
		}
	}
	else {
		for( i=0; i<count; ++i )
			new_buffers[i] = buffers[i];
		while( i < buffer_count ) {
			BC_BitmapImage *buffer = buffers[i];
			if( buffer == active_bfr ) active_bfr = 0;
			if( buffer->is_avail() ) {
				delete buffer;
			}
			else {
				++zombies;
				buffer->index = -1;
			}
			++i;
		}
	}
	delete [] buffers;
	buffers = new_buffers;
	buffer_count = count;
	XFlush(top_level->display);
	if( lock_avail ) avail_lock->unlock();
	//top_level->unlock_window();
}

int BC_Bitmap::allocate_data()
{
	int count = 1;
	max_buffer_count = MAX_BITMAP_BUFFERS;
	if(use_shm) { // Use shared memory.
		int bsz = best_buffer_size();
		if( bsz >= 0x800000 ) max_buffer_count = 2;
		else if( bsz >= 0x400000 ) max_buffer_count /= 8;
		else if( bsz >= 0x100000 ) max_buffer_count /= 4;
		else if( bsz >= 0x10000 ) max_buffer_count /= 2;
		type = hardware_scaling() ? bmXvShmImage : bmXShmImage;
		count = MIN_BITMAP_BUFFERS;
	}
	else // use unshared memory.
		type = hardware_scaling() ? bmXvImage : bmXImage;
	update_buffers(count);
	return 0;
}

int BC_Bitmap::delete_data()
{
//printf("BC_Bitmap::delete_data 1\n");
	update_buffers(0);
	active_bfr = 0;
	buffer_count = 0;
	max_buffer_count = 0;
	return 0;
}

int BC_Bitmap::get_default_depth()
{
	return color_model == BC_TRANSPARENCY ? 1 : top_level->default_depth;
}

long BC_Bitmap::best_buffer_size()
{
	long pixelsize = BC_CModels::calculate_pixelsize(get_color_model());
	return pixelsize * w * h;
}

int BC_Bitmap::need_shm()
{
// dont use shm for less than one page
	return best_buffer_size() < 0x1000 ? 0 :
		parent_window->get_resources()->use_shm > 0 ? 1 : 0;
}

int BC_Bitmap::invert()
{
	for( int j=0; j<buffer_count; ++j ) {
		unsigned char **rows = buffers[j]->get_row_data();
		if( !rows ) continue;
		long bytesPerLine = buffers[j]->bytes_per_line();
		for( int k=0; k<h; ++k ) {
			unsigned char *bp = rows[k];
			for( int i=0; i<bytesPerLine; ++i )
				*bp++ ^= 0xff;
		}
	}
	return 0;
}

int BC_XShmImage::get_shm_size()
{
	return bytesPerLine * bitmap->h;
}

int BC_Bitmap::write_drawable(Drawable &pixmap, GC &gc,
	int dest_x, int dest_y, int source_x, int source_y,
	int dest_w, int dest_h, int dont_wait)
{
	return write_drawable(pixmap, gc,
		source_x, source_y, get_w() - source_x, get_h() - source_y,
		dest_x, dest_y, dest_w, dest_h, dont_wait);
}

int BC_XImage::write_drawable(Drawable &pixmap, GC &gc,
	int source_x, int source_y, int source_w, int source_h,
	int dest_x, int dest_y, int dest_w, int dest_h)
{
	XPutImage(top_level->display, pixmap, gc,
		ximage, source_x, source_y,
		dest_x, dest_y, dest_w, dest_h);
	return 0;
}

int BC_XShmImage::write_drawable(Drawable &pixmap, GC &gc,
	int source_x, int source_y, int source_w, int source_h,
	int dest_x, int dest_y, int dest_w, int dest_h)
{
	XShmPutImage(top_level->display, pixmap, gc,
		ximage, source_x, source_y,
		dest_x, dest_y, dest_w, dest_h, bitmap->shm_reply);
	return 0;
}

int BC_XvImage::write_drawable(Drawable &pixmap, GC &gc,
	int source_x, int source_y, int source_w, int source_h,
	int dest_x, int dest_y, int dest_w, int dest_h)
{
	XvPutImage(top_level->display,
		bitmap->xv_portid, pixmap, gc, xv_image,
		source_x, source_y, source_w, source_h,
		dest_x, dest_y, dest_w, dest_h);
	return 0;
}

int BC_XvShmImage::write_drawable(Drawable &pixmap, GC &gc,
	int source_x, int source_y, int source_w, int source_h,
	int dest_x, int dest_y, int dest_w, int dest_h)
{
	XvShmPutImage(top_level->display,
		bitmap->xv_portid, pixmap, gc, xv_image,
		source_x, source_y, source_w, source_h,
		dest_x, dest_y, dest_w, dest_h, bitmap->shm_reply);
	return 0;
}

int BC_Bitmap::write_drawable(Drawable &pixmap, GC &gc,
		int source_x, int source_y, int source_w, int source_h,
		int dest_x, int dest_y, int dest_w, int dest_h,
		int dont_wait)
{
//printf("BC_Bitmap::write_drawable 1 %p %d\n", this, current_ringbuffer); fflush(stdout);
	//if( dont_wait ) XSync(top_level->display, False);
	BC_BitmapImage *bfr = cur_bfr();
	if( !bfr->is_zombie() && is_shared() && shm_reply ) {
//printf("activate %p %08lx\n",bfr,bfr->get_shmseg());
		top_level->active_bitmaps.insert(bfr, pixmap);
		if( ++active_buffers > max_active_buffers )
			max_active_buffers = active_buffers;
	}
	bfr->write_drawable(pixmap, gc,
		source_x, source_y, source_w, source_h,
		dest_x, dest_y, dest_w, dest_h);
	XFlush(top_level->display);
	avail_lock->lock(" BC_Bitmap::write_drawable");
	if( bfr->is_zombie() ) {
		delete bfr;
		--zombies;
	}
	else if( is_unshared() || !shm_reply )
		avail.append(bfr);
	active_bfr = 0;
	avail_lock->unlock();
	if( !dont_wait && !shm_reply )
		XSync(top_level->display, False);
	return 0;
}


int BC_XImage::read_drawable(Drawable &pixmap, int source_x, int source_y)
{
	XGetSubImage(top_level->display, pixmap,
		source_x, source_y, bitmap->w, bitmap->h,
		0xffffffff, ZPixmap, ximage, 0, 0);
	return 0;
}

int BC_XShmImage::read_drawable(Drawable &pixmap, int source_x, int source_y)
{
	XShmGetImage(top_level->display, pixmap,
		ximage, source_x, source_y, 0xffffffff);
	return 0;
}

int BC_Bitmap::read_drawable(Drawable &pixmap, int source_x, int source_y, VFrame *frame)
{
	BC_BitmapImage *bfr = cur_bfr();
	int result = bfr->read_drawable(pixmap, source_x, source_y);
	if( !result && frame ) result = bfr->read_frame_rgb(frame);
	return result;
}


// ============================ Decoding VFrames

int BC_Bitmap::read_frame(VFrame *frame, int x1, int y1, int x2, int y2)
{
	return read_frame(frame, x1, y1, x2, y2, parent_window->get_bg_color());
}

int BC_Bitmap::read_frame(VFrame *frame, int x1, int y1, int x2, int y2, int bg_color)
{
	return read_frame(frame,
		0, 0, frame->get_w(), frame->get_h(),
		x1, y1, x2 - x1, y2 - y1, bg_color);
}


int BC_Bitmap::read_frame(VFrame *frame,
	int in_x, int in_y, int in_w, int in_h,
	int out_x, int out_y, int out_w, int out_h, int bg_color)
{
	BC_BitmapImage *bfr = cur_bfr();
	if( hardware_scaling() && frame->get_color_model() == color_model ) {
// Hardware accelerated bitmap
		switch(color_model) {
		case BC_YUV420P:
			memcpy(bfr->get_y_data(), frame->get_y(), w * h);
			memcpy(bfr->get_u_data(), frame->get_u(), w * h / 4);
			memcpy(bfr->get_v_data(), frame->get_v(), w * h / 4);
			break;
		case BC_YUV422P:
			memcpy(bfr->get_y_data(), frame->get_y(), w * h);
			memcpy(bfr->get_u_data(), frame->get_u(), w * h / 2);
			memcpy(bfr->get_v_data(), frame->get_v(), w * h / 2);
			break;
		default:
		case BC_YUV422:
		case BC_UVY422:
			memcpy(get_data(), frame->get_data(), w * h + w * h);
			break;
		}
	}
	else {
// Software only

//printf("BC_Bitmap::read_frame %d -> %d %d %d %d %d -> %d %d %d %d\n",
//  frame->get_color_model(), color_model,
//  in_x, in_y, in_w, in_h, out_x, out_y, out_w, out_h);
//if(color_model == 6 && frame->get_color_model() == 19)
//printf("BC_Bitmap::read_frame 1 %d %d %d %d\n", frame->get_w(), frame->get_h(), get_w(), get_h());
		BC_CModels::transfer(
			bfr->get_row_data(), frame->get_rows(),
			bfr->get_y_data(), bfr->get_u_data(), bfr->get_v_data(),
			frame->get_y(), frame->get_u(), frame->get_v(),
			in_x, in_y, in_w, in_h,
			out_x, out_y, out_w, out_h,
			frame->get_color_model(), color_model,
			bg_color, frame->get_w(), w);

		if( color_model == BC_TRANSPARENCY && !top_level->server_byte_order )
			transparency_bitswap(frame->get_data(), w, h);
//if(color_model == 6 && frame->get_color_model() == 19)
//printf("BC_Bitmap::read_frame 2\n");
	}

	return 0;
}

uint8_t BC_Bitmap::bitswap[256]={
  0x00,0x80,0x40,0xc0,0x20,0xa0,0x60,0xe0,0x10,0x90,0x50,0xd0,0x30,0xb0,0x70,0xf0,
  0x08,0x88,0x48,0xc8,0x28,0xa8,0x68,0xe8,0x18,0x98,0x58,0xd8,0x38,0xb8,0x78,0xf8,
  0x04,0x84,0x44,0xc4,0x24,0xa4,0x64,0xe4,0x14,0x94,0x54,0xd4,0x34,0xb4,0x74,0xf4,
  0x0c,0x8c,0x4c,0xcc,0x2c,0xac,0x6c,0xec,0x1c,0x9c,0x5c,0xdc,0x3c,0xbc,0x7c,0xfc,
  0x02,0x82,0x42,0xc2,0x22,0xa2,0x62,0xe2,0x12,0x92,0x52,0xd2,0x32,0xb2,0x72,0xf2,
  0x0a,0x8a,0x4a,0xca,0x2a,0xaa,0x6a,0xea,0x1a,0x9a,0x5a,0xda,0x3a,0xba,0x7a,0xfa,
  0x06,0x86,0x46,0xc6,0x26,0xa6,0x66,0xe6,0x16,0x96,0x56,0xd6,0x36,0xb6,0x76,0xf6,
  0x0e,0x8e,0x4e,0xce,0x2e,0xae,0x6e,0xee,0x1e,0x9e,0x5e,0xde,0x3e,0xbe,0x7e,0xfe,
  0x01,0x81,0x41,0xc1,0x21,0xa1,0x61,0xe1,0x11,0x91,0x51,0xd1,0x31,0xb1,0x71,0xf1,
  0x09,0x89,0x49,0xc9,0x29,0xa9,0x69,0xe9,0x19,0x99,0x59,0xd9,0x39,0xb9,0x79,0xf9,
  0x05,0x85,0x45,0xc5,0x25,0xa5,0x65,0xe5,0x15,0x95,0x55,0xd5,0x35,0xb5,0x75,0xf5,
  0x0d,0x8d,0x4d,0xcd,0x2d,0xad,0x6d,0xed,0x1d,0x9d,0x5d,0xdd,0x3d,0xbd,0x7d,0xfd,
  0x03,0x83,0x43,0xc3,0x23,0xa3,0x63,0xe3,0x13,0x93,0x53,0xd3,0x33,0xb3,0x73,0xf3,
  0x0b,0x8b,0x4b,0xcb,0x2b,0xab,0x6b,0xeb,0x1b,0x9b,0x5b,0xdb,0x3b,0xbb,0x7b,0xfb,
  0x07,0x87,0x47,0xc7,0x27,0xa7,0x67,0xe7,0x17,0x97,0x57,0xd7,0x37,0xb7,0x77,0xf7,
  0x0f,0x8f,0x4f,0xcf,0x2f,0xaf,0x6f,0xef,0x1f,0x9f,0x5f,0xdf,0x3f,0xbf,0x7f,0xff,
};

void BC_Bitmap::transparency_bitswap(uint8_t *buf, int w, int h)
{
	w = ((w-1) | 7) + 1;
	int len = w * h / 8;
	int i;
	for(i=0 ; i+8<=len ; i+=8){
		buf[i+0] = bitswap[buf[i+0]];
		buf[i+1] = bitswap[buf[i+1]];
		buf[i+2] = bitswap[buf[i+2]];
		buf[i+3] = bitswap[buf[i+3]];
		buf[i+4] = bitswap[buf[i+4]];
		buf[i+5] = bitswap[buf[i+5]];
		buf[i+6] = bitswap[buf[i+6]];
		buf[i+7] = bitswap[buf[i+7]];
	}
	for( ; i<len ; i++)
		buf[i+0] = bitswap[buf[i+0]];
}

