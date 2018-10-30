
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

#include "bccapture.h"
#include "bcresources.h"
#include "bcwindowbase.h"
#include "bccmodels.h"
#include "bccolors.h"
#include "clip.h"
#include "language.h"
#include "vframe.h"
#include <stdlib.h>
#include <stdio.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>

// Byte orders:
// 24 bpp packed:         bgr
// 24 bpp unpacked:       0bgr


BC_Capture::BC_Capture(int w, int h, const char *display_path)
{
	this->w = w;
	this->h = h;

	data = 0;
	use_shm = 1;
	init_window(display_path);
	allocate_data();
}


BC_Capture::~BC_Capture()
{
	delete_data();
	XCloseDisplay(display);
}

int BC_Capture::init_window(const char *display_path)
{
	int bits_per_pixel;
	if( display_path && display_path[0] == 0 ) display_path = NULL;
	if( (display = XOpenDisplay(display_path)) == NULL ) {
  		printf(_("cannot connect to X server.\n"));
  		if( getenv("DISPLAY") == NULL )
    		printf(_("'DISPLAY' environment variable not set.\n"));
  		exit(-1);
		return 1;
 	}

	screen = DefaultScreen(display);
	rootwin = RootWindow(display, screen);
	vis = DefaultVisual(display, screen);
	default_depth = DefaultDepth(display, screen);
	client_byte_order = (*(const u_int32_t*)"a   ") & 0x00000001;
	server_byte_order = (XImageByteOrder(display) == MSBFirst) ? 0 : 1;
	char *data = 0;
	XImage *ximage;
	ximage = XCreateImage(display,
					vis,
					default_depth,
					ZPixmap,
					0,
					data,
					16,
					16,
					8,
					0);
	bits_per_pixel = ximage->bits_per_pixel;
	XDestroyImage(ximage);
	bitmap_color_model = BC_WindowBase::evaluate_color_model(client_byte_order, server_byte_order, bits_per_pixel);

// test shared memory
// This doesn't ensure the X Server is on the local host
    if( use_shm && !XShmQueryExtension(display) ) {
        use_shm = 0;
    }
	return 0;
}


int BC_Capture::allocate_data()
{
// try shared memory
	if( !display ) return 1;
    if( use_shm ) {
	    ximage = XShmCreateImage(display, vis, default_depth, ZPixmap, (char*)NULL, &shm_info, w, h);

		shm_info.shmid = shmget(IPC_PRIVATE, h * ximage->bytes_per_line, IPC_CREAT | 0600);
		if( shm_info.shmid == -1 ) {
			perror("BC_Capture::allocate_data shmget");
			abort();
		}
		data = (unsigned char *)shmat(shm_info.shmid, NULL, 0);
		shmctl(shm_info.shmid, IPC_RMID, 0);
		ximage->data = shm_info.shmaddr = (char*)data;  // setting ximage->data stops BadValue
		shm_info.readOnly = 0;

// Crashes here if remote server.
		BC_Resources::error = 0;
		XShmAttach(display, &shm_info);
    	XSync(display, False);
		if( BC_Resources::error ) {
			XDestroyImage(ximage);
			shmdt(shm_info.shmaddr);
			use_shm = 0;
		}
	}

	if( !use_shm ) {
// need to use bytes_per_line for some X servers
		data = 0;
		ximage = XCreateImage(display, vis, default_depth, ZPixmap, 0, (char*)data, w, h, 8, 0);
		data = (unsigned char*)malloc(h * ximage->bytes_per_line);
		XDestroyImage(ximage);

		ximage = XCreateImage(display, vis, default_depth, ZPixmap, 0, (char*)data, w, h, 8, 0);
	}

	row_data = new unsigned char*[h];
	for( int i = 0; i < h; i++ ) {
		row_data[i] = &data[i * ximage->bytes_per_line];
	}
// This differs from the depth parameter of the top_level.
	bits_per_pixel = ximage->bits_per_pixel;
	return 0;
}

int BC_Capture::delete_data()
{
	if( !display ) return 1;
	if( data ) {
		if( use_shm ) {
			XShmDetach(display, &shm_info);
			XDestroyImage(ximage);
			shmdt(shm_info.shmaddr);
		}
		else {
			XDestroyImage(ximage);
		}

// data is automatically freed by XDestroyImage
		data = 0;
		delete [] row_data;
	}
	return 0;
}


int BC_Capture::get_w() { return w; }
int BC_Capture::get_h() { return h; }

// Capture a frame from the screen
#define RGB_TO_YUV(y, u, v, r, g, b) { \
 YUV::yuv.rgb_to_yuv_8(r, g, b, y, u, v); \
 bclamp(y, 0,0xff); bclamp(u, 0,0xff); bclamp(v, 0,0xff); }

int BC_Capture::capture_frame(VFrame *frame, int &x1, int &y1, 
	int do_cursor) // the scale of the cursor if nonzero
{
	if( !display ) return 1;
	if( x1 < 0 ) x1 = 0;
	if( y1 < 0 ) y1 = 0;
	if( x1 > get_top_w() - w ) x1 = get_top_w() - w;
	if( y1 > get_top_h() - h ) y1 = get_top_h() - h;


// Read the raw data
	if( use_shm )
		XShmGetImage(display, rootwin, ximage, x1, y1, 0xffffffff);
	else
		XGetSubImage(display, rootwin, x1, y1, w, h, 0xffffffff, ZPixmap, ximage, 0, 0);

	BC_CModels::transfer(frame->get_rows(), row_data,
		frame->get_y(), frame->get_u(), frame->get_v(), 0,
		0, 0, 0, 0, w, h, 0, 0,
		frame->get_w(), frame->get_h(),
		bitmap_color_model, frame->get_color_model(),
		0, frame->get_w(), w);
	
	if( do_cursor ) {
		XFixesCursorImage *cursor;
		cursor = XFixesGetCursorImage(display);
		if( cursor ) {
//printf("BC_Capture::capture_frame %d cursor=%p colormodel=%d\n", 
// __LINE__, cursor, frame->get_color_model());
			int scale = do_cursor;
			int cursor_x = cursor->x - x1 - cursor->xhot * scale;
			int cursor_y = cursor->y - y1 - cursor->yhot * scale;
			int w = frame->get_w();
			int h = frame->get_h();
			for( int i = 0; i < cursor->height; i++ ) {
				for( int yscale = 0; yscale < scale; yscale++ ) {
					if( cursor_y + i * scale + yscale >= 0 && 
						cursor_y + i * scale + yscale < h ) {
						unsigned char *src = (unsigned char*)(cursor->pixels + 
							i * cursor->width);
						int dst_y = cursor_y + i * scale + yscale;
						int dst_x = cursor_x;
						for( int j = 0; j < cursor->width; j++ ) {
							for( int xscale = 0; xscale < scale ; xscale++ ) {
								if( cursor_x + j * scale + xscale >= 0 && 
									cursor_x + j * scale + xscale < w ) {
									int a = src[3];
									int invert_a = 0xff - a;
									int r = src[2];
									int g = src[1];
									int b = src[0];
									switch( frame->get_color_model() ) {
									case BC_RGB888: {
										unsigned char *dst = frame->get_rows()[dst_y] +
											dst_x * 3;
										dst[0] = (r * a + dst[0] * invert_a) / 0xff;
										dst[1] = (g * a + dst[1] * invert_a) / 0xff;
										dst[2] = (b * a + dst[2] * invert_a) / 0xff;
										break; }

									case BC_YUV420P: {
										unsigned char *dst_y_ = frame->get_y() + 
											dst_y * w + dst_x;
										unsigned char *dst_u = frame->get_u() + 
											(dst_y / 2) * (w / 2) + (dst_x / 2);
										unsigned char *dst_v = frame->get_v() + 
											(dst_y / 2) * (w / 2) + (dst_x / 2);
										int y, u, v;
										RGB_TO_YUV(y, u, v, r, g, b);
											
										*dst_y_ = (y * a + *dst_y_ * invert_a) / 0xff;
										*dst_u = (u * a + *dst_u * invert_a) / 0xff;
										*dst_v = (v * a + *dst_v * invert_a) / 0xff;
										break; }
									}
								}
								dst_x++;
							}
							src += sizeof(long);
						}
					}
				}
			}

// This frees cursor->pixels
			XFree(cursor);
		}
	}

	return 0;
}

int BC_Capture::get_top_w()
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	return WidthOfScreen(screen_ptr);
}

int BC_Capture::get_top_h()
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	return HeightOfScreen(screen_ptr);
}
