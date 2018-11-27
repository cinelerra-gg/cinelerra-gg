
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

#ifndef VFRAME_H
#define VFRAME_H

#include "arraylist.h"
#include "bcbitmap.inc"
#include "bchash.inc"
#include "bcpbuffer.inc"
#include "bctexture.inc"
#include "bcwindowbase.inc"
#include "bccmodels.h"
#include "bccmodels.h"
#include "vframe.inc"

// Maximum number of prev or next effects to be pushed onto the stacks.
#define MAX_STACK_ELEMENTS 255
#define SHM_MIN_SIZE 2048

// Scene graph for 3D models
// Defined by the subclass
class VFrameScene
{
public:
	VFrameScene();
	virtual ~VFrameScene();
};



class VFrame
{
	friend class VFramePng;
	friend class PngReadFunction;
public:
// Create new frame with shared data if *data is nonzero.
// Pass 0 to *data & -1 to shmid if private data is desired.
	VFrame(int w,
		int h,
		int color_model,
		long bytes_per_line = -1);
	VFrame(unsigned char *data,
		int shmid,
		int w,
		int h,
		int color_model /* = BC_RGBA8888 */,
		long bytes_per_line /* = -1 */);
	VFrame(unsigned char *data,  // 0
		int shmid, // -1
		long y_offset,
		long u_offset,
		long v_offset,
		int w,
		int h,
		int color_model,  /* = BC_RGBA8888 */
		long bytes_per_line /* = -1 */);
	VFrame(BC_Bitmap *bitmap,
		int w,
		int h,
		int color_model,
		long bytes_per_line);

	VFrame(VFrame &vframe);
// Create new frame for compressed data.
	VFrame();
	virtual ~VFrame();

// Return 1 if the colormodel and dimensions are the same
// Used by FrameCache
	int equivalent(VFrame *src, int test_stacks = 0);

// Reallocate a frame without deleting the class
	int reallocate(
		unsigned char *data,   // Data if shared
		int shmid,             // shmid if IPC  -1 if not
		long y_offset,         // plane offsets if shared YUV
		long u_offset,
		long v_offset,
		int w,
		int h,
		int color_model,
		long bytes_per_line);        // -1 if unused

	void set_memory(unsigned char *data,
		int shmid,
		long y_offset,
		long u_offset,
		long v_offset);
	void set_memory(BC_Bitmap *bitmap);

	void set_compressed_memory(unsigned char *data,
		int shmid,
		int data_size,
		int data_allocated);

// Write a PNG/PPM for debugging
	int write_png(const char *path);
	static void write_ppm(VFrame *vfrm, const char *fmt, ...);
	void write_ppm(const char *path) { write_ppm(this, "%s", path); }
//static int n = 0; write_ppm(vframe, "/tmp/data/f%05d", ++n);

// if frame points to the same data as this return 1
	int equals(VFrame *frame);
// Test if frame already matches parameters
	int params_match(int w, int h, int color_model);
// Test if data values in the frame match
	int data_matches(VFrame *frame);

//	long set_shm_offset(long offset);
//	long get_shm_offset();
	int get_shmid() { return shmid; }
	void set_use_shm(int value) { use_shm = value; }
	int get_use_shm() { return use_shm; }

// direct copy with no alpha
	int copy_from(VFrame *frame);
	int copy_vframe(VFrame *src);
// BC_CModels::transfer
	int transfer_from(VFrame *frame, int bg_color, int in_x, int in_y, int in_w, int in_h);
	int transfer_from(VFrame *frame, int bg_color=0) {
		return transfer_from(frame, bg_color, 0,0, frame->get_w(),frame->get_h());
	}
// Required for YUV
	int clear_frame();
	int allocate_compressed_data(long bytes);

// Sequence number. -1 means invalid.  Passing frames to the encoder is
// asynchronous.  The sequence number must be preserved in the image itself
// to encode discontinuous frames.
	long get_number() { return sequence_number; }
	void set_number(long number) { sequence_number = number; }

	int get_color_model() { return color_model; }
// Get the data pointer
	unsigned char* get_data() { return data; }
	long get_compressed_allocated() { return compressed_allocated; }
	long get_compressed_size() { return compressed_size; }
	void set_compressed_size(long size) { compressed_size = size; }
	double get_timestamp() { return timestamp; }
	void set_timestamp(double time) { timestamp = time; }

// return an array of pointers to rows
	unsigned char** get_rows() { return rows; }
	int get_memory_usage();
// accessors
	int get_w() { return w; }
	int get_h() { return h; }
	int get_w_fixed() { return w - 1; }
	int get_h_fixed() { return h - 1; }
	unsigned char *get_y() { return y; }
	unsigned char *get_u() { return u; }
	unsigned char *get_v() { return v; }
// return rgba planes
	unsigned char *get_r() { return y; }
	unsigned char *get_g() { return u; }
	unsigned char *get_b() { return v; }
	unsigned char *get_a() { return a; }
	void set_a(unsigned char *ap) { a = ap; }
// return yuv planes
	static int get_scale_tables(int *column_table, int *row_table,
			int in_x1, int in_y1, int in_x2, int in_y2,
			int out_x1, int out_y1, int out_x2, int out_y2);
	int get_bytes_per_pixel() { return bytes_per_pixel; }
	long get_bytes_per_line();
	int get_memory_type();

	static int calculate_bytes_per_pixel(int colormodel);
// Get size + 4 for assembly language
	static long calculate_data_size(int w, int h,
		int bytes_per_line = -1, int color_model = BC_RGB888);
// Get size of uncompressed frame buffer without extra 4 bytes
	long get_data_size();
// alloc/reset temp vframe to spec
	static void get_temp(VFrame *&vfrm, int w, int h, int color_model);

	void rotate270();
	void rotate90();
	void flip_vert();
	void flip_horiz();

// Convenience storage.
// Returns -1 if not set.
	int get_field2_offset();
	int set_field2_offset(int value);
// Set keyframe status
	void set_keyframe(int value);
	int get_keyframe();

// If the opengl state is RAM, transfer image from RAM to the texture
// referenced by this frame.
// If the opengl state is TEXTURE, do nothing.
// If the opengl state is SCREEN, switch the current drawable to the pbuffer and
// transfer the image to the texture with screen_to_texture.
// The opengl state is changed to TEXTURE.
// If no textures exist, textures are created.
// If the textures already exist, they are reused.
// Textures are resized to match the current dimensions.
// Must be called from a synchronous opengl thread after enable_opengl.
	void to_texture();

// Transfer from PBuffer to RAM.
//   used in Playback3D::overlay_sync, plugin Overlay::handle_opengl
	void screen_to_ram();

// Transfer contents of current pbuffer to texture,
// creating a new texture if necessary.
// Coordinates are the coordinates in the drawable to copy.
	void screen_to_texture(int x = -1, int y = -1, int w = -1, int h = -1);

// Transfer contents of texture to the current drawable.
// Just calls the vertex functions but doesn't initialize.
// The coordinates are relative to the VFrame size and flipped to make
// the texture upright.
// The default coordinates are the size of the VFrame.
// flip_y flips the texture in the vertical direction and only used when
// writing to the final surface.
	void draw_texture(float in_x1, float in_y1, float in_x2, float in_y2,
		float out_x1, float out_y1, float out_x2, float out_y2,
		int flip_y = 0);
// Draw the texture using the frame's size as the input and output coordinates.
	void draw_texture(int flip_y = 0);







// ================================ OpenGL functions ===========================
// Defined in vframe3d.C
// Location of working image if OpenGL playback
	int get_opengl_state();
	void set_opengl_state(int value);
// OpenGL states
	enum
	{
// Undefined
		UNKNOWN,
// OpenGL image is in RAM
		RAM,
// OpenGL image is in texture
		TEXTURE,
// OpenGL image is composited in PBuffer or back buffer
		SCREEN
	};

// Texture ID
	int get_texture_id();
	void set_texture_id(int id);
// Get window ID the texture is bound to
	int get_window_id();
	int get_texture_w();
	int get_texture_h();
	int get_texture_components();


// Binds the opengl context to this frame's PBuffer
	void enable_opengl();

// Clears the pbuffer with the right values depending on YUV
	void clear_pbuffer();

// Get the pbuffer
	BC_PBuffer* get_pbuffer();

// Bind the frame's texture to GL_TEXTURE_2D and enable it.
// If a texture_unit is supplied, the texture unit is made active
// and the commands are run in the right sequence to
// initialize it to our preferred specifications.
	void bind_texture(int texture_unit = -1);



// Create a frustum with 0,0 in the upper left and w,-h in the bottom right.
// Set preferred opengl settings.
	static void init_screen(int w, int h);
// Calls init_screen with the current frame's dimensions.
	void init_screen();

// Compiles and links the shaders into a program.
// Adds the program with put_shader.
// Returns the program handle.
// Requires a null terminated argument list of shaders to link together.
// if fragments is not NULL, it is a a zero terminated list of frags
// if fragments is NULL, then a zero terminated list of va_args frags
// At least one shader argument must have a main() function.  make_shader
// replaces all the main() functions with unique functions and calls them in
// sequence, so multiple independant shaders can be linked.
	static unsigned int make_shader(const char **fragments, ...);
	static void dump_shader(int shader_id);

// Because OpenGL is faster if multiple effects are combined, we need
// to provide ways for effects to aggregate.
// The prev_effect is the object providing the data to read_frame.
// The next_effect is the object which called read_frame.
// Push and pop are only called from Cinelerra internals, so
// if an object calls read_frame with a temporary, the stack before and after
// the temporary is lost.
	void push_prev_effect(const char *name);
	void pop_prev_effect();
	void push_next_effect(const char *name);
	void pop_next_effect();
// These are called by plugins to determine aggregation.
// They access any member of the stack based on the number argument.
// next effect 0 is the one that called read_frame most recently.
// prev effect 0 is the one that filled our call to read_frame.
	const char* get_next_effect(int number = 0);
	const char* get_prev_effect(int number = 0);

// It isn't enough to know the name of the neighboring effects.
// Relevant configuration parameters must be passed on.
	BC_Hash* get_params();

// get/set read status  -1/err, 0/noxfer, 1/ok
	int get_status() { return status; }
	void set_status(int v) { status = v; }

// Compare stacks and params from 2 images and return 1 if equal.
	int equal_stacks(VFrame *src);

// Copy stacks and params from another frame
// Replaces the stacks with the src stacks but only updates the params.
	void copy_stacks(VFrame *src);
// Updates the params with values from src
	void copy_params(VFrame *src);

// This clears the stacks and the param table
	void clear_stacks();

	virtual int draw_pixel(int x, int y);
	int pixel_rgb, pixel_yuv, stipple;

	void set_pixel_color(int rgb, int a=0xff);
	void set_stiple(int mask);
	void draw_line(int x1, int y1, int x2, int y2);
	void draw_smooth(int x1, int y1, int x2, int y2, int x3, int y3);
	void smooth_draw(int x1, int y1, int x2, int y2, int x3, int y3);
	void draw_rect(int x1, int y1, int x2, int y2);
	void draw_arrow(int x1, int y1, int x2, int y2, int sz=10);
	void draw_x(int x1, int y1, int sz=2);
	void draw_t(int x1, int y1, int sz=2);
	void draw_oval(int x1, int y1, int x2, int y2);

// 3D scene graphs
// Not integrated with shmem because that only affects codecs
	VFrameScene* get_scene();

// Debugging
	void dump_stacks();
	void dump();

	void dump_params();

private:

// 3D scene graphs
// Not integrated with shmem because that only affects codecs
	VFrameScene *scene;

// Create a PBuffer matching this frame's dimensions and to be
// referenced by this frame.  Does nothing if the pbuffer already exists.
// If the frame is resized, the PBuffer is deleted.
// Called by enable_opengl.
// This allows PBuffers, textures, and bitmaps to travel through the entire
// rendering chain without requiring the user to manage a lot of objects.
// Must be called from a synchronous opengl thread after enable_opengl.
	void create_pbuffer();

	int clear_objects(int do_opengl);
	int reset_parameters(int do_opengl);
	void create_row_pointers();
	int allocate_data(unsigned char *data,
		int shmid,
		long y_offset,
		long u_offset,
		long v_offset,
		int w,
		int h,
		int color_model,
		long bytes_per_line);

// Convenience storage
	int field2_offset;
	int memory_type;
	enum
	{
		PRIVATE,
		SHARED,
		SHMGET
	};

// Data pointer is pointing to someone else's buffer.
//	long shm_offset;
// ID of shared memory if using IPC.
// The 1st 1 after reboot is 0.
	int shmid;
// Local setting for shm usage
	int use_shm;
// If not set by user, is calculated from color_model
	long bytes_per_line;
	int bytes_per_pixel;
// Image data
	unsigned char *data;
// Pointers to the start of each row
	unsigned char **rows;
// One of the #defines
	int color_model;
// Allocated space for compressed data
	long compressed_allocated;
// Size of stored compressed image
	long compressed_size;
// Pointers to yuv / rgb planes
	unsigned char *y, *u, *v;
	long y_offset;
	long u_offset;
	long v_offset;
// Pointer to alpha plane
	unsigned char *a;
// Dimensions of frame
	int w, h;
// Info for reading png images
	const unsigned char *image;
	long image_offset;
	long image_size;
// For writing discontinuous frames in background rendering
	long sequence_number;
	double timestamp;
// read status of input frame -1/err, 0/noxfr, 1/ok
	int status;
// OpenGL support
	int is_keyframe;
// State of the current texture
	BC_Texture *texture;
// State of the current PBuffer
	BC_PBuffer *pbuffer;

// Location of working image if OpenGL playback
	int opengl_state;

	ArrayList<char*> prev_effects;
	ArrayList<char*> next_effects;
	BC_Hash *params;
};

// Create a frame with the png image
class VFramePng : public VFrame {
// Read a PNG into the frame with alpha
	int read_png(const unsigned char *data, long image_size, double xscale, double yscale);
public:
	VFramePng(unsigned char *png_data, double s=0);
	VFramePng(unsigned char *png_data, long image_size, double xs=0, double ys=0);
	~VFramePng();
	static VFrame *vframe_png(int fd, double xs=1, double ys=1);
	static VFrame *vframe_png(const char *png_path, double xs=1, double ys=1);
};

#endif
