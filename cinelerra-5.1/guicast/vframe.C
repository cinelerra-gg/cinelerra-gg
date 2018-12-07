
/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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

#include <errno.h>
#include <png.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/mman.h>

#include "bcbitmap.h"
#include "bchash.h"
#include "bcpbuffer.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bctexture.h"
#include "bcwindowbase.h"
#include "clip.h"
#include "bccmodels.h"
#include "vframe.h"

class PngReadFunction
{
public:
	static void png_read_function(png_structp png_ptr,
			png_bytep data, png_size_t length)
	{
		VFrame *frame = (VFrame*)png_get_io_ptr(png_ptr);
		if(frame->image_size - frame->image_offset < (long)length)
		{
			printf("PngReadFunction::png_read_function %d: overrun\n", __LINE__);
			length = frame->image_size - frame->image_offset;
		}

		memcpy(data, &frame->image[frame->image_offset], length);
		frame->image_offset += length;
	};
};







VFrameScene::VFrameScene()
{
}

VFrameScene::~VFrameScene()
{
}







//static BCCounter counter;

VFramePng::VFramePng(unsigned char *png_data, double s)
{
	long image_size =
		((long)png_data[0] << 24) | ((long)png_data[1] << 16) |
		((long)png_data[2] << 8)  |  (long)png_data[3];
	if( !s ) s = BC_WindowBase::get_resources()->icon_scale;
	read_png(png_data+4, image_size, s, s);
}

VFramePng::VFramePng(unsigned char *png_data, long image_size, double xs, double ys)
{
	if( !xs ) xs = BC_WindowBase::get_resources()->icon_scale;
	if( !ys ) ys = BC_WindowBase::get_resources()->icon_scale;
	read_png(png_data, image_size, xs, ys);
}

VFramePng::~VFramePng()
{
}

VFrame *VFramePng::vframe_png(int fd, double xs, double ys)
{
	struct stat st;
	if( fstat(fd, &st) ) return 0;
	long len = st.st_size;
	if( !len ) return 0;
	int w = 0, h = 0;
	unsigned char *bfr = (unsigned char *)
		::mmap (NULL, len, PROT_READ, MAP_SHARED, fd, 0);
	if( bfr == MAP_FAILED ) return 0;
	VFrame *vframe = new VFramePng(bfr, len, xs, ys);
	if( (w=vframe->get_w()) <= 0 || (h=vframe->get_h()) <= 0 ||
	    vframe->get_data() == 0 ) { delete vframe;  vframe = 0; }
	::munmap(bfr, len);
	return vframe;
}
VFrame *VFramePng::vframe_png(const char *png_path, double xs, double ys)
{
	VFrame *vframe = 0;
	int fd = ::open(png_path, O_RDONLY);
	if( fd >= 0 ) {
		vframe = vframe_png(fd, xs, ys);
		::close(fd);
	}
	return vframe;
}

VFrame::VFrame(VFrame &frame)
{
	reset_parameters(1);
	params = new BC_Hash;
	use_shm = frame.use_shm;
	allocate_data(0, -1, 0, 0, 0, frame.w, frame.h,
		frame.color_model, frame.bytes_per_line);
	copy_vframe(&frame);
}


VFrame::VFrame(int w, int h, int color_model, long bytes_per_line)
{
	reset_parameters(1);
//  use bytes_per_line == 0 to allocate default unshared
	if( !bytes_per_line ) { bytes_per_line = -1;  use_shm = 0; }
	params = new BC_Hash;
	allocate_data(data, -1, 0, 0, 0, w, h,
		color_model, bytes_per_line);
}

VFrame::VFrame(unsigned char *data, int shmid, int w, int h,
	int color_model, long bytes_per_line)
{
	reset_parameters(1);
	params = new BC_Hash;
	allocate_data(data, shmid, 0, 0, 0, w, h,
		color_model, bytes_per_line);
}

VFrame::VFrame(unsigned char *data, int shmid,
		long y_offset, long u_offset, long v_offset,
		int w, int h, int color_model, long bytes_per_line)
{
	reset_parameters(1);
	params = new BC_Hash;
	allocate_data(data, shmid, y_offset, u_offset, v_offset, w, h,
		color_model, bytes_per_line);
}

VFrame::VFrame(BC_Bitmap *bitmap, int w, int h,
		 int color_model, long bytes_per_line)
{
	reset_parameters(1);
	params = new BC_Hash;
	int shmid = 0;
	unsigned char *data = 0;
	if( bitmap->is_shared() )
		shmid = bitmap->get_shmid();
	else
		data = bitmap->get_data();
	allocate_data(data, shmid,
		bitmap->get_y_offset(),
		bitmap->get_u_offset(),
		bitmap->get_v_offset(),
		w, h, color_model, bytes_per_line);
}

VFrame::VFrame()
{
	reset_parameters(1);
	params = new BC_Hash;
	this->color_model = BC_COMPRESSED;
}



VFrame::~VFrame()
{
	clear_objects(1);
// Delete effect stack
	prev_effects.remove_all_objects();
	next_effects.remove_all_objects();
	delete params;
	delete scene;
}

int VFrame::equivalent(VFrame *src, int test_stacks)
{
	return (src->get_color_model() == get_color_model() &&
		src->get_w() == get_w() &&
		src->get_h() == get_h() &&
		src->bytes_per_line == bytes_per_line &&
		(!test_stacks || equal_stacks(src)));
}

int VFrame::data_matches(VFrame *frame)
{
	if(data && frame->get_data() &&
		frame->params_match(get_w(), get_h(), get_color_model()) &&
		get_data_size() == frame->get_data_size())
	{
		int data_size = get_data_size();
		unsigned char *ptr1 = get_data();
		unsigned char *ptr2 = frame->get_data();
		for(int i = 0; i < data_size; i++)
		{
			if(*ptr1++ != *ptr2++) return 0;
		}
		return 1;
	}
	return 0;
}

// long VFrame::set_shm_offset(long offset)
// {
// 	shm_offset = offset;
// 	return 0;
// }
//
// long VFrame::get_shm_offset()
// {
// 	return shm_offset;
// }
//
int VFrame::get_memory_type()
{
	return memory_type;
}

int VFrame::params_match(int w, int h, int color_model)
{
	return (this->w == w &&
		this->h == h &&
		this->color_model == color_model);
}


int VFrame::reset_parameters(int do_opengl)
{
	status = 1;
	scene = 0;
	field2_offset = -1;
	memory_type = VFrame::PRIVATE;
//	shm_offset = 0;
	shmid = -1;
	use_shm = 1;
	bytes_per_line = 0;
	data = 0;
	rows = 0;
	color_model = 0;
	compressed_allocated = 0;
	compressed_size = 0;   // Size of current image
	w = 0;
	h = 0;
	y = u = v = a = 0;
	y_offset = 0;
	u_offset = 0;
	v_offset = 0;
	sequence_number = -1;
	timestamp = -1.;
	is_keyframe = 0;
	pixel_rgb = 0x000000; // BLACK
	pixel_yuv = 0x008080;
	stipple = 0;

	if(do_opengl)
	{
// By default, anything is going to be done in RAM
		opengl_state = VFrame::RAM;
		pbuffer = 0;
		texture = 0;
	}

	prev_effects.set_array_delete();
	next_effects.set_array_delete();
	return 0;
}

int VFrame::clear_objects(int do_opengl)
{
// Remove texture
	if(do_opengl)
	{
		delete texture;
		texture = 0;

		delete pbuffer;
		pbuffer = 0;
	}

#ifdef LEAKER
if( memory_type != VFrame::SHARED )
  printf("==del %p from %p\n", data, __builtin_return_address(0));
#endif

// Delete data
	switch(memory_type)
	{
		case VFrame::PRIVATE:
// Memory check
// if(this->w * this->h > 1500 * 1100)
// printf("VFrame::clear_objects 2 this=%p data=%p\n", this, data);
			if(data)
			{
//printf("VFrame::clear_objects %d this=%p shmid=%p data=%p\n", __LINE__, this, shmid, data);
				if(shmid >= 0)
					shmdt(data);
				else
					free(data);
//PRINT_TRACE
			}

			data = 0;
			shmid = -1;
			break;

		case VFrame::SHMGET:
			if(data)
				shmdt(data);
			data = 0;
			shmid = -1;
			break;
	}

// Delete row pointers
	switch(color_model)
	{
		case BC_COMPRESSED:
		case BC_YUV410P:
		case BC_YUV411P:
		case BC_YUV420P:
		case BC_YUV420PI:
		case BC_YUV422P:
		case BC_YUV444P:
		case BC_RGB_FLOATP:
		case BC_RGBA_FLOATP:
		case BC_GBRP:
			break;

		default:
			delete [] rows;
			rows = 0;
			break;
	}


	return 0;
}

int VFrame::get_field2_offset()
{
	return field2_offset;
}

int VFrame::set_field2_offset(int value)
{
	this->field2_offset = value;
	return 0;
}

void VFrame::set_keyframe(int value)
{
	this->is_keyframe = value;
}

int VFrame::get_keyframe()
{
	return is_keyframe;
}

void VFrame::get_temp(VFrame *&vfrm, int w, int h, int color_model)
{
	if( vfrm && ( vfrm->color_model != color_model ||
	    vfrm->get_w() != w || vfrm->get_h() != h ) ) {
		delete vfrm;  vfrm = 0;
	}
	if( !vfrm ) vfrm = new VFrame(w, h, color_model, 0);
}



VFrameScene* VFrame::get_scene()
{
	return scene;
}

int VFrame::calculate_bytes_per_pixel(int color_model)
{
	return BC_CModels::calculate_pixelsize(color_model);
}

long VFrame::get_bytes_per_line()
{
	return bytes_per_line;
}

long VFrame::get_data_size()
{
	return calculate_data_size(w, h, bytes_per_line, color_model) - 4;
}

long VFrame::calculate_data_size(int w, int h, int bytes_per_line, int color_model)
{
	return BC_CModels::calculate_datasize(w, h, bytes_per_line, color_model);
}

void VFrame::create_row_pointers()
{
	int sz = w * h;
	switch(color_model) {
	case BC_YUV410P:
		if( this->v_offset ) break;
		this->y_offset = 0;
		this->u_offset = sz;
		this->v_offset = sz + w / 4 * h / 4;
		break;

	case BC_YUV420P:
	case BC_YUV420PI:
	case BC_YUV411P:
		if( this->v_offset ) break;
		this->y_offset = 0;
		this->u_offset = sz;
		this->v_offset = sz + sz / 4;
		break;

	case BC_YUV422P:
		if( this->v_offset ) break;
		this->y_offset = 0;
		this->u_offset = sz;
		this->v_offset = sz + sz / 2;
		break;
	case BC_YUV444P:
		if( this->v_offset ) break;
		this->y_offset = 0;
		this->u_offset = sz;
		this->v_offset = sz + sz;
		break;
	case BC_GBRP:
		if( this->v_offset ) break;
		this->y_offset = 0;
		this->u_offset = sz * sizeof(uint8_t);
		this->v_offset = 2 * sz * sizeof(uint8_t);
		break;
	case BC_RGBA_FLOATP:
		if( this->v_offset || a ) break;
		a = this->data + 3 * sz * sizeof(float);
	case BC_RGB_FLOATP:
		if( this->v_offset ) break;
		this->y_offset = 0;
		this->u_offset = sz * sizeof(float);
		this->v_offset = 2 * sz * sizeof(float);
		break;

	default:
		rows = new unsigned char*[h];
		for(int i = 0; i < h; i++)
			rows[i] = &this->data[i * this->bytes_per_line];
		return;
	}
	y = this->data + this->y_offset;
	u = this->data + this->u_offset;
	v = this->data + this->v_offset;
}

int VFrame::allocate_data(unsigned char *data, int shmid,
		long y_offset, long u_offset, long v_offset, int w, int h,
		int color_model, long bytes_per_line)
{
	this->w = w;
	this->h = h;
	this->color_model = color_model;
	this->bytes_per_pixel = calculate_bytes_per_pixel(color_model);
	this->y_offset = this->u_offset = this->v_offset = 0;
//	if(shmid == 0) {
//		printf("VFrame::allocate_data %d shmid == 0\n", __LINE__, shmid);
//	}

	this->bytes_per_line = bytes_per_line >= 0 ?
		bytes_per_line : this->bytes_per_pixel * w;

// Allocate data + padding for MMX
	if(data) {
//printf("VFrame::allocate_data %d %p\n", __LINE__, this->data);
		memory_type = VFrame::SHARED;
		this->data = data;
		this->shmid = -1;
		this->y_offset = y_offset;
		this->u_offset = u_offset;
		this->v_offset = v_offset;
	}
	else if(shmid >= 0) {
		memory_type = VFrame::SHMGET;
		this->data = (unsigned char*)shmat(shmid, NULL, 0);
//printf("VFrame::allocate_data %d shmid=%d data=%p\n", __LINE__, shmid, this->data);
		this->shmid = shmid;
		this->y_offset = y_offset;
		this->u_offset = u_offset;
		this->v_offset = v_offset;
	}
	else {
		memory_type = VFrame::PRIVATE;
		this->data = 0;
		int size = calculate_data_size(this->w, this->h,
			this->bytes_per_line, this->color_model);
		if( use_shm && size >= SHM_MIN_SIZE &&
		    BC_WindowBase::get_resources()->use_vframe_shm() ) {
			this->shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777);
			if( this->shmid >= 0 ) {
				this->data = (unsigned char*)shmat(this->shmid, NULL, 0);
//printf("VFrame::allocate_data %d %d %d %p\n", __LINE__, size, this->shmid, this->data);
// This causes it to automatically delete when the program exits.
				shmctl(this->shmid, IPC_RMID, 0);
			}
			else {
				printf("VFrame::allocate_data %d could not allocate"
					" shared memory, %dx%d (model %d) size=0x%08x\n",
					__LINE__, w, h, color_model, size);
				BC_Trace::dump_shm_stats(stdout);
			}
		}
// Have to use malloc for libpng
		if( !this->data ) {
			this->data = (unsigned char *)malloc(size);
			this->shmid = -1;
		}
// Memory check
// if(this->w * this->h > 1500 * 1100)
// printf("VFrame::allocate_data 2 this=%p w=%d h=%d this->data=%p\n",
// this, this->w, this->h, this->data);

		if(!this->data)
			printf("VFrame::allocate_data %dx%d: memory exhausted.\n", this->w, this->h);
#ifdef LEAKER
printf("==new %p from %p sz %d\n", this->data, __builtin_return_address(0), size);
#endif

//printf("VFrame::allocate_data %d %p data=%p %d %d\n", __LINE__, this, this->data, this->w, this->h);
//if(size > 1000000) printf("VFrame::allocate_data %d\n", size);
	}

// Create row pointers
	create_row_pointers();
	return 0;
}

void VFrame::set_memory(unsigned char *data,
	int shmid,
	long y_offset,
	long u_offset,
	long v_offset)
{
	clear_objects(0);

	if(data)
	{
		memory_type = VFrame::SHARED;
		this->data = data;
		this->shmid = -1;
		this->y_offset = y_offset;
		this->u_offset = u_offset;
		this->v_offset = v_offset;
	}
	else
	if(shmid >= 0)
	{
		memory_type = VFrame::SHMGET;
		this->data = (unsigned char*)shmat(shmid, NULL, 0);
		this->shmid = shmid;
	}

	y = this->data + this->y_offset;
	u = this->data + this->u_offset;
	v = this->data + this->v_offset;

	create_row_pointers();
}

void VFrame::set_memory(BC_Bitmap *bitmap)
{
	int shmid = 0;
	unsigned char *data = 0;
	if( bitmap->is_shared() && !bitmap->is_zombie() )
		shmid = bitmap->get_shmid();
	else
		data = bitmap->get_data();
	set_memory(data, shmid,
		bitmap->get_y_offset(),
		bitmap->get_u_offset(),
		bitmap->get_v_offset());
}

void VFrame::set_compressed_memory(unsigned char *data,
	int shmid,
	int data_size,
	int data_allocated)
{
	clear_objects(0);

	if(data)
	{
		memory_type = VFrame::SHARED;
		this->data = data;
		this->shmid = -1;
	}
	else
	if(shmid >= 0)
	{
		memory_type = VFrame::SHMGET;
		this->data = (unsigned char*)shmat(shmid, NULL, 0);
		this->shmid = shmid;
	}

	this->compressed_allocated = data_allocated;
	this->compressed_size = data_size;
}


// Reallocate uncompressed buffer with or without alpha
int VFrame::reallocate(
	unsigned char *data,
	int shmid,
	long y_offset,
	long u_offset,
	long v_offset,
	int w,
	int h,
	int color_model,
	long bytes_per_line)
{
//	if(shmid == 0) printf("VFrame::reallocate %d shmid=%d\n", __LINE__, shmid);
	clear_objects(0);
//	reset_parameters(0);
	allocate_data(data,
		shmid,
		y_offset,
		u_offset,
		v_offset,
		w,
		h,
		color_model,
		bytes_per_line);
	return 0;
}

int VFrame::allocate_compressed_data(long bytes)
{
	if(bytes < 1) return 1;

// Want to preserve original contents
	if(data && compressed_allocated < bytes)
	{
		int new_shmid = -1;
		unsigned char *new_data = 0;
		if(BC_WindowBase::get_resources()->use_vframe_shm() && use_shm)
		{
			new_shmid = shmget(IPC_PRIVATE,
				bytes,
				IPC_CREAT | 0777);
			new_data = (unsigned char*)shmat(new_shmid, NULL, 0);
			shmctl(new_shmid, IPC_RMID, 0);
		}
		else
		{
// Have to use malloc for libpng
			new_data = (unsigned char *)malloc(bytes);
		}

		bcopy(data, new_data, compressed_allocated);
UNBUFFER(data);

		if(memory_type == VFrame::PRIVATE)
		{
			if(shmid > 0) {
				if(data)
					shmdt(data);
			}
			else
				free(data);
		}
		else
		if(memory_type == VFrame::SHMGET)
		{
			if(data)
				shmdt(data);
		}

		data = new_data;
		shmid = new_shmid;
		compressed_allocated = bytes;
	}
	else
	if(!data)
	{
		if(BC_WindowBase::get_resources()->use_vframe_shm() && use_shm)
		{
			shmid = shmget(IPC_PRIVATE,
				bytes,
				IPC_CREAT | 0777);
			data = (unsigned char*)shmat(shmid, NULL, 0);
			shmctl(shmid, IPC_RMID, 0);
		}
		else
		{
// Have to use malloc for libpng
			data = (unsigned char *)malloc(bytes);
		}

		compressed_allocated = bytes;
		compressed_size = 0;
	}

	return 0;
}

int VFramePng::read_png(const unsigned char *data, long sz, double xscale, double yscale)
{
// Test for RAW format
	if(data[0] == 'R' && data[1] == 'A' && data[2] == 'W' && data[3] == ' ') {
		int new_color_model = BC_RGBA8888;
		w = data[4] | (data[5] << 8) | (data[6]  << 16) | (data[7]  << 24);
		h = data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24);
		int components = data[12];
		new_color_model = components == 3 ? BC_RGB888 : BC_RGBA8888;
// This shares the data directly
// 		reallocate(data + 20, 0, 0, 0, w, h, new_color_model, -1);

// Can't use shared data for theme since button constructions overlay the
// images directly.
		reallocate(NULL, -1, 0, 0, 0, w, h, new_color_model, -1);
		memcpy(get_data(), data + 16, w * h * components);

	}
	else if(data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G') {
		int have_alpha = 0;
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop info_ptr = png_create_info_struct(png_ptr);
		int new_color_model;

		image_offset = 0;
		image = data;  image_size = sz;
		png_set_read_fn(png_ptr, this, PngReadFunction::png_read_function);
		png_read_info(png_ptr, info_ptr);

		w = png_get_image_width(png_ptr, info_ptr);
		h = png_get_image_height(png_ptr, info_ptr);

		int src_color_model = png_get_color_type(png_ptr, info_ptr);

		/* tell libpng to strip 16 bit/color files down to 8 bits/color */
		png_set_strip_16(png_ptr);

		/* extract multiple pixels with bit depths of 1, 2, and 4 from a single
		 * byte into separate bytes (useful for paletted and grayscale images).
		 */
		png_set_packing(png_ptr);

		/* expand paletted colors into true RGB triplets */
		if (src_color_model == PNG_COLOR_TYPE_PALETTE)
			png_set_expand(png_ptr);

		/* expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
		if (src_color_model == PNG_COLOR_TYPE_GRAY && png_get_bit_depth(png_ptr, info_ptr) < 8)
			png_set_expand(png_ptr);

		if (src_color_model == PNG_COLOR_TYPE_GRAY ||
		    src_color_model == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(png_ptr);

		/* expand paletted or RGB images with transparency to full alpha channels
		 * so the data will be available as RGBA quartets */
		if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)){
			have_alpha = 1;
			png_set_expand(png_ptr);
		}

		switch(src_color_model)
		{
			case PNG_COLOR_TYPE_GRAY:
			case PNG_COLOR_TYPE_RGB:
				new_color_model = BC_RGB888;
				break;

			case PNG_COLOR_TYPE_GRAY_ALPHA:
			case PNG_COLOR_TYPE_RGB_ALPHA:
			default:
				new_color_model = BC_RGBA8888;
				break;

			case PNG_COLOR_TYPE_PALETTE:
				if(have_alpha)
					new_color_model = BC_RGBA8888;
				else
					new_color_model = BC_RGB888;
		}

		reallocate(NULL, -1, 0, 0, 0, w, h, new_color_model, -1);

//printf("VFrame::read_png %d %d %d %p\n", __LINE__, w, h, get_rows());
		png_read_image(png_ptr, get_rows());
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	}
	else {
		printf("VFrame::read_png %d: unknown file format"
			" 0x%02x 0x%02x 0x%02x 0x%02x\n",
			__LINE__, data[4], data[5], data[6], data[7]);
		return 1;
	}
	int ww = w * xscale, hh = h * yscale;
	if( ww != w || hh != h ) {
		VFrame vframe(*this);
		reallocate(NULL, -1, 0, 0, 0, ww, hh, color_model, -1);
		transfer_from(&vframe);
	}
	return 0;
}

int VFrame::write_png(const char *path)
{
	VFrame *vframe = this;
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	FILE *out_fd = fopen(path, "w");
	if(!out_fd) {
		printf("VFrame::write_png %d %s %s\n", __LINE__, path, strerror(errno));
		return 1;
	}

	int png_cmodel = PNG_COLOR_TYPE_RGB;
	int bc_cmodel = get_color_model();
	switch( bc_cmodel ) {
	case BC_RGB888:                                          break;
	case BC_RGBA8888: png_cmodel = PNG_COLOR_TYPE_RGB_ALPHA; break;
	case BC_A8:       png_cmodel = PNG_COLOR_TYPE_GRAY;      break;
	default:
		bc_cmodel = BC_RGB888;
		if( BC_CModels::has_alpha(bc_cmodel) ) {
			bc_cmodel = BC_RGBA8888;
			png_cmodel = PNG_COLOR_TYPE_RGB_ALPHA;
		}
		vframe = new VFrame(get_w(), get_h(), bc_cmodel, 0);
		vframe->transfer_from(this);
		break;
	}
	png_init_io(png_ptr, out_fd);
	png_set_compression_level(png_ptr, 9);
	png_set_IHDR(png_ptr, info_ptr, get_w(), get_h(), 8, png_cmodel,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, vframe->get_rows());
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(out_fd);
	if( vframe != this ) delete vframe;
	return 0;
}

void VFrame::write_ppm(VFrame *vfrm, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	char fn[BCTEXTLEN];
	vsnprintf(fn, sizeof(fn), fmt, ap);
	va_end(ap);
	FILE *fp = fopen(fn,"w");
	if( !fp ) { perror("write_ppm"); return; }
	VFrame *frm = vfrm;
	if( frm->get_color_model() != BC_RGB888 ) {
		frm = new VFrame(frm->get_w(), frm->get_h(), BC_RGB888);
		frm->transfer_from(vfrm);
	}
	int w = frm->get_w(), h = frm->get_h();
	fprintf(fp,"P6\n%d %d\n255\n",w,h);
	unsigned char **rows = frm->get_rows();
	for( int i=0; i<h; ++i ) fwrite(rows[i],3,w,fp);
	fclose(fp);
	if( frm != vfrm ) delete frm;
}


#define ZERO_YUV(components, type, max) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		type *row = (type*)get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			row[j * components] = 0; \
			row[j * components + 1] = (max + 1) / 2; \
			row[j * components + 2] = (max + 1) / 2; \
			if(components == 4) row[j * components + 3] = 0; \
		} \
	} \
}

int VFrame::clear_frame()
{
	int sz = w * h;
//printf("VFrame::clear_frame %d\n", __LINE__);
	switch(color_model) {
	case BC_COMPRESSED:
		break;

	case BC_YUV410P:
		bzero(get_y(), sz);
		memset(get_u(), 0x80, w / 4 * h / 4);
		memset(get_v(), 0x80, w / 4 * h / 4);
		break;

	case BC_YUV411P:
	case BC_YUV420P:
	case BC_YUV420PI:
		bzero(get_y(), sz);
		memset(get_u(), 0x80, sz / 4);
		memset(get_v(), 0x80, sz / 4);
		break;

	case BC_YUV422P:
		bzero(get_y(), sz);
		memset(get_u(), 0x80, sz / 2);
		memset(get_v(), 0x80, sz / 2);
		break;

	case BC_GBRP:
		bzero(get_y(), sz);
		bzero(get_u(), sz);
		bzero(get_b(), sz);
		break;

	case BC_RGBA_FLOATP: if( a ) {
		float *ap = (float *)a;
		for( int i=sz; --i>=0; ++ap ) *ap = 0.f; }
	case BC_RGB_FLOATP: {
		float *rp = (float *)y;
		for( int i=sz; --i>=0; ++rp ) *rp = 0.f;
		float *gp = (float *)u;
		for( int i=sz; --i>=0; ++gp ) *gp = 0.f;
		float *bp = (float *)v;
		for( int i=sz; --i>=0; ++bp ) *bp = 0.f;
		break; }
	case BC_YUV444P:
		bzero(get_y(), sz);
		memset(get_u(), 0x80, sz);
		memset(get_v(), 0x80, sz);
		break;

	case BC_YUV888:
		ZERO_YUV(3, unsigned char, 0xff);
		break;

	case BC_YUVA8888:
		ZERO_YUV(4, unsigned char, 0xff);
		break;

	case BC_YUV161616:
		ZERO_YUV(3, uint16_t, 0xffff);
		break;

	case BC_YUVA16161616:
		ZERO_YUV(4, uint16_t, 0xffff);
		break;

	default:
		bzero(data, calculate_data_size(w, h, bytes_per_line, color_model));
		break;
	}
	return 0;
}

void VFrame::rotate90()
{
// Allocate new frame
	int new_w = h, new_h = w;
	VFrame new_frame(new_w, new_h, color_model);
	unsigned char **new_rows = new_frame.get_rows();
// Copy data
	for(int in_y = 0, out_x = new_w - 1; in_y < h; in_y++, out_x--)
	{
		for(int in_x = 0, out_y = 0; in_x < w; in_x++, out_y++)
		{
			for(int k = 0; k < bytes_per_pixel; k++)
			{
				new_rows[out_y][out_x * bytes_per_pixel + k] =
					rows[in_y][in_x * bytes_per_pixel + k];
			}
		}
	}

// Swap frames
// swap memory
	unsigned char *new_data = new_frame.data;
	new_frame.data = data;
	data = new_data;
// swap rows
	new_rows = new_frame.rows;
	new_frame.rows = rows;
	rows = new_rows;
// swap shmid
	int new_shmid = new_frame.shmid;
	new_frame.shmid = shmid;
	shmid = new_shmid;
// swap bytes_per_line
	int new_bpl = new_frame.bytes_per_line;
	new_frame.bytes_per_line = bytes_per_line;
	bytes_per_line = new_bpl;
	new_frame.clear_objects(0);

	w = new_frame.w;
	h = new_frame.h;
}

void VFrame::rotate270()
{
// Allocate new frame
	int new_w = h, new_h = w;
	VFrame new_frame(new_w, new_h, color_model);
	unsigned char **new_rows = new_frame.get_rows();
// Copy data
	for(int in_y = 0, out_x = 0; in_y < h; in_y++, out_x++)
	{
		for(int in_x = 0, out_y = new_h - 1; in_x < w; in_x++, out_y--)
		{
			for(int k = 0; k < bytes_per_pixel; k++)
			{
				new_rows[out_y][out_x * bytes_per_pixel + k] =
					rows[in_y][in_x * bytes_per_pixel + k];
			}
		}
	}

// Swap frames
// swap memory
	unsigned char *new_data = new_frame.data;
	new_frame.data = data;
	data = new_data;
// swap rows
	new_rows = new_frame.rows;
	new_frame.rows = rows;
	rows = new_rows;
// swap shmid
	int new_shmid = new_frame.shmid;
	new_frame.shmid = shmid;
	shmid = new_shmid;
// swap bytes_per_line
	int new_bpl = new_frame.bytes_per_line;
	new_frame.bytes_per_line = bytes_per_line;
	bytes_per_line = new_bpl;
	new_frame.clear_objects(0);

	w = new_frame.w;
	h = new_frame.h;
}

void VFrame::flip_vert()
{
	unsigned char temp[bytes_per_line];
	for( int i=0, j=h; --j>i; ++i ) {
		memcpy(temp, rows[j], bytes_per_line);
		memcpy(rows[j], rows[i], bytes_per_line);
		memcpy(rows[i], temp, bytes_per_line);
	}
}

void VFrame::flip_horiz()
{
	unsigned char temp[32];
	for(int i = 0; i < h; i++)
	{
		unsigned char *row = rows[i];
		for(int j = 0; j < bytes_per_line / 2; j += bytes_per_pixel)
		{
			memcpy(temp, row + j, bytes_per_pixel);
			memcpy(row + j, row + bytes_per_line - j - bytes_per_pixel, bytes_per_pixel);
			memcpy(row + bytes_per_line - j - bytes_per_pixel, temp, bytes_per_pixel);
		}
	}
}



int VFrame::copy_from(VFrame *frame)
{
	if(this->w != frame->get_w() ||
		this->h != frame->get_h())
	{
		printf("VFrame::copy_from %d sizes differ src %dx%d != dst %dx%d\n",
			__LINE__,
			frame->get_w(),
			frame->get_h(),
			get_w(),
			get_h());
		return 1;
	}

	int w = MIN(this->w, frame->get_w());
	int h = MIN(this->h, frame->get_h());
	timestamp = frame->timestamp;

	switch(frame->color_model)
	{
		case BC_COMPRESSED:
			allocate_compressed_data(frame->compressed_size);
			memcpy(data, frame->data, frame->compressed_size);
			this->compressed_size = frame->compressed_size;
			break;

		case BC_YUV410P:
			memcpy(get_y(), frame->get_y(), w * h);
			memcpy(get_u(), frame->get_u(), w / 4 * h / 4);
			memcpy(get_v(), frame->get_v(), w / 4 * h / 4);
			break;

		case BC_YUV420P:
		case BC_YUV420PI:
		case BC_YUV411P:
//printf("%d %d %p %p %p %p %p %p\n", w, h, get_y(), get_u(), get_v(), frame->get_y(), frame->get_u(), frame->get_v());
			memcpy(get_y(), frame->get_y(), w * h);
			memcpy(get_u(), frame->get_u(), w * h / 4);
			memcpy(get_v(), frame->get_v(), w * h / 4);
			break;

		case BC_YUV422P:
//printf("%d %d %p %p %p %p %p %p\n", w, h, get_y(), get_u(), get_v(), frame->get_y(), frame->get_u(), frame->get_v());
			memcpy(get_y(), frame->get_y(), w * h);
			memcpy(get_u(), frame->get_u(), w * h / 2);
			memcpy(get_v(), frame->get_v(), w * h / 2);
			break;

		case BC_YUV444P:
//printf("%d %d %p %p %p %p %p %p\n", w, h, get_y(), get_u(), get_v(), frame->get_y(), frame->get_u(), frame->get_v());
			memcpy(get_y(), frame->get_y(), w * h);
			memcpy(get_u(), frame->get_u(), w * h);
			memcpy(get_v(), frame->get_v(), w * h);
			break;
		default:
// printf("VFrame::copy_from %d\n", calculate_data_size(w,
// 				h,
// 				-1,
// 				frame->color_model));
// Copy without extra 4 bytes in case the source is a hardware device
			memmove(data, frame->data, get_data_size());
			break;
	}

	return 0;
}

int VFrame::transfer_from(VFrame *that, int bg_color, int in_x, int in_y, int in_w, int in_h)
{
	timestamp = that->timestamp;
	copy_params(that);

	if( this->get_color_model() == that->get_color_model() &&
	    this->get_w() == that->get_w() && this->get_h() == that->get_h() &&
	    this->get_bytes_per_line() == that->get_bytes_per_line() )
		return this->copy_from(that);

#if 0
	BC_CModels::transfer(
		this->get_rows(), that->get_rows(),	     // Packed data out/in
		this->get_y(), this->get_u(), this->get_v(), // Planar data out/in
		that->get_y(), that->get_u(), that->get_v(),
		0, 0, that->get_w(), that->get_h(),	     // Dimensions in/out
		0, 0, this->get_w(), this->get_h(),
		that->get_color_model(), this->get_color_model(), // Color models in/out
		bg_color,				     // alpha blend bg_color
		that->get_bytes_per_line(),
		this->get_bytes_per_line()); 		     // rowspans (of luma for YUV)
#else
	unsigned char *in_ptrs[4], *out_ptrs[4];
	unsigned char **inp, **outp;
	if( BC_CModels::is_planar(that->get_color_model()) ) {
		in_ptrs[0] = that->get_y();
		in_ptrs[1] = that->get_u();
		in_ptrs[2] = that->get_v();
		in_ptrs[3] = that->get_a();
		inp = in_ptrs;
	}
	else
		inp = that->get_rows();
	if( BC_CModels::is_planar(this->get_color_model()) ) {
		out_ptrs[0] = this->get_y();
		out_ptrs[1] = this->get_u();
		out_ptrs[2] = this->get_v();
		out_ptrs[3] = this->get_a();
		outp = out_ptrs;
	}
	else
		outp = this->get_rows();
	BC_CModels::transfer(outp, this->get_color_model(),
			0, 0, this->get_w(), this->get_h(),
			this->get_bytes_per_line(),
		inp, that->get_color_model(),
			in_x, in_y, in_w, in_h,
			that->get_bytes_per_line(),
		bg_color);
#endif
	return 0;
}


int VFrame::get_scale_tables(int *column_table, int *row_table,
			int in_x1, int in_y1, int in_x2, int in_y2,
			int out_x1, int out_y1, int out_x2, int out_y2)
{
	int i;
	float w_in = in_x2 - in_x1;
	float h_in = in_y2 - in_y1;
	int w_out = out_x2 - out_x1;
	int h_out = out_y2 - out_y1;

	float hscale = w_in / w_out;
	float vscale = h_in / h_out;

	for(i = 0; i < w_out; i++)
	{
		column_table[i] = (int)(hscale * i);
	}

	for(i = 0; i < h_out; i++)
	{
		row_table[i] = (int)(vscale * i) + in_y1;
	}
	return 0;
}

void VFrame::push_prev_effect(const char *name)
{
	char *ptr;
	prev_effects.append(ptr = new char[strlen(name) + 1]);
	strcpy(ptr, name);
	if(prev_effects.total > MAX_STACK_ELEMENTS) prev_effects.remove_object(0);
}

void VFrame::pop_prev_effect()
{
	if(prev_effects.total)
		prev_effects.remove_object(prev_effects.last());
}

void VFrame::push_next_effect(const char *name)
{
	char *ptr;
	next_effects.append(ptr = new char[strlen(name) + 1]);
	strcpy(ptr, name);
	if(next_effects.total > MAX_STACK_ELEMENTS) next_effects.remove_object(0);
}

void VFrame::pop_next_effect()
{
	if(next_effects.total)
		next_effects.remove_object(next_effects.last());
}

const char* VFrame::get_next_effect(int number)
{
	if(!next_effects.total) return "";
	else
	if(number > next_effects.total - 1) number = next_effects.total - 1;

	return next_effects.values[next_effects.total - number - 1];
}

const char* VFrame::get_prev_effect(int number)
{
	if(!prev_effects.total) return "";
	else
	if(number > prev_effects.total - 1) number = prev_effects.total - 1;

	return prev_effects.values[prev_effects.total - number - 1];
}

BC_Hash* VFrame::get_params()
{
	return params;
}

void VFrame::clear_stacks()
{
	next_effects.remove_all_objects();
	prev_effects.remove_all_objects();
	params->clear();
	status = 1;
}

void VFrame::copy_params(VFrame *src)
{
	status = src->status;
	params->copy_from(src->params);
}

void VFrame::copy_stacks(VFrame *src)
{
	clear_stacks();

	for( int i=0; i < src->next_effects.total; ++i )
		next_effects.append(cstrdup(src->next_effects[i]));
	for( int i=0; i < src->prev_effects.total; ++i )
		prev_effects.append(cstrdup(src->prev_effects[i]));

	copy_params(src);
}

int VFrame::copy_vframe(VFrame *frame)
{
	copy_stacks(frame);
	return copy_from(frame);
}

int VFrame::equal_stacks(VFrame *src)
{
	for(int i = 0; i < src->next_effects.total && i < next_effects.total; i++)
	{
		if(strcmp(src->next_effects.values[i], next_effects.values[i])) return 0;
	}

	for(int i = 0; i < src->prev_effects.total && i < prev_effects.total; i++)
	{
		if(strcmp(src->prev_effects.values[i], prev_effects.values[i])) return 0;
	}

	if(!params->equivalent(src->params)) return 0;
	return 1;
}

void VFrame::dump_stacks()
{
	printf("VFrame::dump_stacks\n");
	printf("	next_effects:\n");
	for(int i = next_effects.total - 1; i >= 0; i--)
		printf("		%s\n", next_effects.values[i]);
	printf("	prev_effects:\n");
	for(int i = prev_effects.total - 1; i >= 0; i--)
		printf("		%s\n", prev_effects.values[i]);
}

void VFrame::dump_params()
{
	params->dump();
}

void VFrame::dump()
{
	printf("VFrame::dump %d this=%p\n", __LINE__, this);
	printf("    w=%d h=%d colormodel=%d rows=%p use_shm=%d shmid=%d\n",
		w, h, color_model, rows, use_shm, shmid);
}


int VFrame::get_memory_usage()
{
	if(get_compressed_allocated()) return get_compressed_allocated();
	return get_h() * get_bytes_per_line();
}

// rgb component colors (eg. from colors.h)
// a (~alpha) transparency, 0x00==solid .. 0xff==transparent
void VFrame::set_pixel_color(int rgb, int a)
{
	pixel_rgb = (rgb&0xffffff) | ~a<<24;
	int ir = 0xff & (pixel_rgb >> 16);
	int ig = 0xff & (pixel_rgb >> 8);
	int ib = 0xff & (pixel_rgb >> 0);
	YUV::yuv.rgb_to_yuv_8(ir, ig, ib);
	pixel_yuv =  (~a<<24) | (ir<<16) | (ig<<8) | (ib<<0);
}

void VFrame::set_stiple(int mask)
{
	stipple = mask;
}

int VFrame::draw_pixel(int x, int y)
{
	if( x < 0 || y < 0 || x >= get_w() || y >= get_h() ) return 1;

#define DRAW_PIXEL(type, r, g, b, comps, a) { \
	type **rows = (type**)get_rows(); \
	type *rp = rows[y], *bp = rp + x*comps; \
	bp[0] = r; \
	if( comps > 1 ) { bp[1] = g; bp[2] = b; } \
	if( comps == 4 )  bp[3] = a; \
}
	float fr = 0, fg = 0, fb = 0, fa = 0;
	int pixel_color = BC_CModels::is_yuv(color_model) ? pixel_yuv : pixel_rgb;
	int ir = (0xff & (pixel_color >> 16));
	int ig = (0xff & (pixel_color >> 8));
	int ib = (0xff & (pixel_color >> 0));
	int ia = (0xff & (pixel_color >> 24)) ^ 0xff;  // transparency, not opacity
	if( (x+y) & stipple ) {
		ir = 255 - ir;  ig = 255 - ig;  ib = 255 - ib;
	}
	int rr = (ir<<8) | ir, gg = (ig<<8) | ig, bb = (ib<<8) | ib, aa = (ia<<8) | ia;
	if( BC_CModels::is_float(color_model) ) {
		fr = rr/65535.f;  fg = gg/65535.f;  fb = bb/65535.f;  fa = aa/65535.f;
	}

	switch(get_color_model()) {
	case BC_A8:
		DRAW_PIXEL(uint8_t, ib, 0, 0, 1, 0);
		break;
	case BC_RGB888:
	case BC_YUV888:
		DRAW_PIXEL(uint8_t, ir, ig, ib, 3, 0);
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		DRAW_PIXEL(uint8_t, ir, ig, ib, 4, ia);
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		DRAW_PIXEL(uint16_t, rr, gg, bb, 3, 0);
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
		DRAW_PIXEL(uint16_t, rr, gg, bb, 4, aa);
		break;
	case BC_RGB_FLOAT:
		DRAW_PIXEL(float, fr, fg, fb, 3, 0);
		break;
	case BC_RGBA_FLOAT:
		DRAW_PIXEL(float, fr, fg, fb, 4, fa);
		break;
	}
	return 0;
}


// Bresenham's
void VFrame::draw_line(int x1, int y1, int x2, int y2)
{
	if( y1 > y2 ) {
		int tx = x1;  x1 = x2;  x2 = tx;
		int ty = y1;  y1 = y2;  y2 = ty;
	}

	int x = x1, y = y1;
	int dx = x2-x1, dy = y2-y1;
	int dx2 = 2*dx, dy2 = 2*dy;
	if( dx < 0 ) dx = -dx;
	int r = dx > dy ? dx : dy, n = r;
	int dir = 0;
	if( dx2 < 0 ) dir += 1;
	if( dy >= dx ) {
		if( dx2 >= 0 ) do {	/* +Y, +X */
			draw_pixel(x, y++);
			if( (r -= dx2) < 0 ) { r += dy2;  ++x; }
		} while( --n >= 0 );
		else do {		/* +Y, -X */
			draw_pixel(x, y++);
			if( (r += dx2) < 0 ) { r += dy2;  --x; }
		} while( --n >= 0 );
	}
	else {
		if( dx2 >= 0 ) do {	/* +X, +Y */
			draw_pixel(x++, y);
			if( (r -= dy2) < 0 ) { r += dx2;  ++y; }
		} while( --n >= 0 );
		else do {		/* -X, +Y */
			draw_pixel(x--, y);
			if( (r -= dy2) < 0 ) { r -= dx2;  ++y; }
		} while( --n >= 0 );
	}
}

// g++ -dD -E - < /dev/null | grep DBL_EPSILON
#ifndef __DBL_EPSILON__
#define __DBL_EPSILON__ ((double)2.22044604925031308085e-16L)
#endif
// weakest fraction * graphics integer range
#define RND_EPSILON (__DBL_EPSILON__*65536)

class smooth_line {
	int rnd(double v) { return round(v)+RND_EPSILON; }
	VFrame *vframe;
public:
	int sx, sy, ex, ey;	/* current point, end point */
	int xs, ys;		/* x/y quadrant sign -1/1 */
	int64_t A, B, C; 	/* quadratic coefficients */
	int64_t r, dx, dy;	/* residual, dr/dx and dr/dy */
	int xmxx, xmxy;		/* x,y at apex */
	int done;

	void init0(int x1,int y1, int x2,int y2, int x3,int y3, int top);
	void init1(int x1,int y1, int x2,int y2, int x3,int y3);
	int64_t rx() { return r + xs*8*dx + 4*A; }
	void moveX(int64_t r) {
		dx += xs*A;   dy -= xs*B;
		this->r = r;  sx += xs;
	}
	int64_t ry() { return r + 8*dy + 4*C; }
	void moveY(int64_t r) {
		dx -= B;      dy += C;
		this->r = r;  ++sy;
	}
	void draw();

	smooth_line(VFrame *vframe) { this->vframe = vframe; this->done = 0; }
};


void smooth_line::draw()
{
	if( done ) return;
	if( abs(dy) >= abs(dx) ) {
		if( xs*(sx-xmxx) >= 0 ) {
			if( ys > 0 ) { done = 1;  return; }
			if( dy < 0 || ry() < 0 ) { moveY(ry()); goto xit; }
			xmxx = ex;  xmxy = ey;
			ys = 1;  xs = -xs;
		}
		moveX(rx());
		int64_t rr = ry();
		if( abs(rr) < abs(r) )
			moveY(rr);
	}
	else {
		if( sy >= xmxy ) {
			if( ys > 0 ) { done = 1;  return; }
			xmxx = ex;  xmxy = ey;
			ys = 1;  xs = -xs;
		}
		moveY(ry());
		int64_t rr = rx();
		if( abs(rr) < abs(r) )
			moveX(rr);
	}
xit:	vframe->draw_pixel(sx, sy);
}

void VFrame::draw_smooth(int x1, int y1, int x2, int y2, int x3, int y3)
{
	if( (x1 == x2 && y1 == y2) || (x2 == x3 && y2 == y3) )
		draw_line(x1,y1, x3,y3);
	else if( x1 == x3 && y1 == y3 )
		draw_line(x1,y1, x2,y2);
	else if( (x2-x1) * (y2-y3) == (x2-x3) * (y2-y1) ) {
		// co-linear, draw line from min to max
		if( x1 < x3 ) {
			if( x2 < x1 ) { x1 = x2;  y1 = y2; }
			if( x2 > x3 ) { x3 = x2;  y3 = y2; }
		}
		else {
			if( x2 > x1 ) { x1 = x2;  y1 = y2; }
			if( x2 < x3 ) { x3 = x2;  y3 = y2; }
		}
		draw_line(x1,y1, x3,y3);
	}
	else
		smooth_draw(x1, y1, x2, y2, x3, y3);
}

/*
  Non-Parametric Smooth Curve Generation. Don Kelly 1984

     P+-----+Q'= virtual
     /     /       origin
    /     /
  Q+-----+R

   Let the starting point be P. the ending point R. and the tangent vertex Q.
   A general point Z on the curve is then
        Z = (P + R - Q) + (Q - P) sin t + (Q - R) cos t

   Expanding the Cartesian coordinates around (P + R - Q) gives
        [x y] = Z - (P + R - Q)
        [a c] = Q - P
        [b d] = Q - R
        x = a*sin(t) + b*cos(t)
        y = c*sin(t) + d*cos(t)

   from which t can now be eliminated via
        c*x - a*y = (c*b - a*d)*cos(t)
        d*x - b*y = (a*d - c*b)*sin(t)

   giving the Cartesian equation for the ellipse as
        f(x, y) = (c*x - a*y)**2 + (d*x - b*y)**2 - (a*d - c*b)**2 = 0

   or:  f(x, y) = A*x**2 - 2*B*x*y + C*y**2 + B**2 - A*C = 0
   where: A = c**2 + d**2,  B = a*c + b*d,  C = a**2 + b**2

   The maximum y extent of the ellipse may now be derived as follows:
        let df/dx = 0,  2*A*x = 2*B*y,  x = y*B/A
        f(x, y) == B**2 * y**2 / A - 2*B**2 * y**2 / A + C*y**2 + B**2 - A*C = 0
           (A*C - B**2)*y = (A*C - B**2)*A
           max x = sqrt(C), at y = B/sqrt(C)
           max y = sqrt(A), at x = B/sqrt(A)

 */


/* x1,y1 = P, x2,y2 = Q, x3,y3=R,
 *  draw from P to Q to R   if top=0
 *    or from P to (x,ymax) if top>0
 *    or from Q to (x,ymax) if top<0
 */
void smooth_line::init0(int x1,int y1, int x2,int y2, int x3,int y3, int top)
{
	int x0 = x1+x3-x2, y0 = y1+y3-y2; // Q'

	int a = x2-x1,  c = y2-y1;
	int b = x2-x3,  d = y2-y3;
	A = c*c + d*d;  C = a*a + b*b;  B = a*c + b*d;

	sx = top >= 0 ? x1 : x3;
	sy = top >= 0 ? y1 : y3;
	xs = x2 > sx || (x2==sx && (x1+x3-sx)>=x2) ? 1 : -1;
	int64_t px = sx-x0, py = sy-y0;
	dx = A*px - B*py;  dy = C*py - B*px;
	r = 0;

	if( top ) {
		double ymy = sqrt(A), ymx = B/ymy;
		ex = x0 + rnd(ymx);
		ey = y0 + rnd(ymy);
	}
	else {
		ex = x3;  ey = y3;
	}

	ys = a*b > 0 && (!top || top*xs*(b*c - a*d) > 0) ? -1 : 1;
	if( ys < 0 ) {
		double xmx = xs*sqrt(C), xmy = B/xmx;
		xmxx = x0 + rnd(xmx);
		xmxy = y0 + rnd(xmy);
	}
	else {
		xmxx = ex; xmxy = ey;
	}
}

/*  x1,y1 = P, x2,y2 = Q, x3,y3=R,
 *  draw from (x,ymax) to P
 */
void smooth_line::init1(int x1,int y1, int x2,int y2, int x3,int y3)
{
	int x0 = x1+x3-x2, y0 = y1+y3-y2; // Q'

	int a = x2-x1,  c = y2-y1;
	int b = x2-x3,  d = y2-y3;
	A = c*c + d*d;  C = a*a + b*b;  B = a*c + b*d;

	double ymy = -sqrt(A), ymx = B/ymy;
	int64_t px = rnd(ymx), py = rnd(ymy);
	sx = x0 + px;  ex = x1;
	sy = y0 + py;  ey = y1;
	xs = x2 > x1 || (x2==x1 && x3>=x2) ? 1 : -1;
	dx = A*px - B*py;  dy = C*py - B*px;
	r = 4 * (A*px*px - 2*B*px*py + C*py*py + B*B - A*C);

	ys = a*b > 0 && xs*(b*c - a*d) < 0 ? -1 : 1;
	if( ys < 0 ) {
		double xmx = xs*sqrt(C), xmy = B/xmx;
		xmxx = x0 + rnd(xmx);
		xmxy = y0 + rnd(xmy);
	}
	else {
		xs = -xs;
		xmxx = ex; xmxy = ey;
	}
	if( xs > 0 )
		vframe->draw_pixel(sx, sy);
	while( xs*(sx-xmxx) < 0 && (xs*dx < 0 || rx() < 0) ) {
		moveX(rx());
		vframe->draw_pixel(sx, sy);
	}
}


void VFrame::smooth_draw(int x1, int y1, int x2, int y2, int x3, int y3)
{
//printf("p smooth_draw( %d,%d, %d,%d, %d,%d )\n", x1,y1,x2,y2,x3,y3);
	if( y1 > y3 ) {		// make y3 >= y1
		int xt = x1;  x1 = x3;  x3 = xt;
		int yt = y1;  y1 = y3;  y3 = yt;
	}
	if( y1 > y2 && y3 > y2 ) {
		smooth_line lt(this), rt(this);	// Q on bottom
		lt.init1(x1, y1, x2, y2, x3, y3);
		rt.init1(x3, y3, x2, y2, x1, y1);
		while( !lt.done || !rt.done ) {
			lt.draw();
			rt.draw();
		}
	}
	else if( y1 < y2 && y3 < y2 ) {
		smooth_line lt(this), rt(this);	// Q on top
		lt.init0(x1, y1, x2, y2, x3, y3, 1);
		draw_pixel(lt.sx, lt.sy);
		rt.init0(x1, y1, x2, y2, x3, y3, -1);
		draw_pixel(rt.sx, rt.sy);
		while( !lt.done || !rt.done ) {
			lt.draw();
			rt.draw();
		}
	}
	else {
		smooth_line pt(this);		// Q in between
		pt.init0(x1, y1, x2, y2, x3, y3, 0);
		draw_pixel(pt.sx, pt.sy);
		while( !pt.done ) {
			pt.draw();
		}
	}
}


void VFrame::draw_rect(int x1, int y1, int x2, int y2)
{
	draw_line(x1, y1, x2, y1);
	draw_line(x2, y1 + 1, x2, y2);
	draw_line(x2 - 1, y2, x1, y2);
	draw_line(x1, y2 - 1, x1, y1 + 1);
}


void VFrame::draw_oval(int x1, int y1, int x2, int y2)
{
	int w = x2 - x1;
	int h = y2 - y1;
	int center_x = (x2 + x1) / 2;
	int center_y = (y2 + y1) / 2;
	int x_table[h / 2];

//printf("VFrame::draw_oval %d %d %d %d %d\n", __LINE__, x1, y1, x2, y2);

	for(int i = 0; i < h / 2; i++) {
// A^2 = -(B^2) + C^2
		x_table[i] = (int)(sqrt(-SQR(h / 2 - i) + SQR(h / 2)) * w / h);
//printf("VFrame::draw_oval %d i=%d x=%d\n", __LINE__, i, x_table[i]);
	}

	for(int i = 0; i < h / 2 - 1; i++) {
		int x3 = x_table[i];
		int x4 = x_table[i + 1];

		if(x4 > x3 + 1) {
			for(int j = x3; j < x4; j++) {
				draw_pixel(center_x + j, y1 + i);
				draw_pixel(center_x - j, y1 + i);
				draw_pixel(center_x + j, y2 - i - 1);
				draw_pixel(center_x - j, y2 - i - 1);
			}
		}
		else {
			draw_pixel(center_x + x3, y1 + i);
			draw_pixel(center_x - x3, y1 + i);
			draw_pixel(center_x + x3, y2 - i - 1);
			draw_pixel(center_x - x3, y2 - i - 1);
		}
	}
	
	draw_pixel(center_x, y1);
	draw_pixel(center_x, y2 - 1);
 	draw_pixel(x1, center_y);
 	draw_pixel(x2 - 1, center_y);
 	draw_pixel(x1, center_y - 1);
 	draw_pixel(x2 - 1, center_y - 1);
}


void VFrame::draw_arrow(int x1, int y1, int x2, int y2, int sz)
{
	double angle = atan((float)(y2 - y1) / (float)(x2 - x1));
	double angle1 = angle + (float)145 / 360 * 2 * M_PI;
	double angle2 = angle - (float)145 / 360 * 2 * M_PI;
	int s = x2 < x1 ? -1 : 1;
	int x3 = x2 + s * (int)(sz * cos(angle1));
	int y3 = y2 + s * (int)(sz * sin(angle1));
	int x4 = x2 + s * (int)(sz * cos(angle2));
	int y4 = y2 + s * (int)(sz * sin(angle2));

// Main vector
	draw_line(x1, y1, x2, y2);
//	draw_line(x1, y1 + 1, x2, y2 + 1);

// Arrow line
	if(abs(y2 - y1) || abs(x2 - x1)) draw_line(x2, y2, x3, y3);
// Arrow line
	if(abs(y2 - y1) || abs(x2 - x1)) draw_line(x2, y2, x4, y4);
}

void VFrame::draw_x(int x, int y, int sz)
{
	draw_line(x-sz,y-sz, x+sz,y+sz);
	draw_line(x+sz,y-sz, x-sz,y+sz);
}
void VFrame::draw_t(int x, int y, int sz)
{
	draw_line(x,y-sz, x,y+sz);
	draw_line(x+sz,y, x-sz,y);
}


