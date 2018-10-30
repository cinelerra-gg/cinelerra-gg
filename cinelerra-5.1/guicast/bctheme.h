
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

#ifndef BCTHEME_H
#define BCTHEME_H

#include "arraylist.h"
#include "bcresources.inc"
#include "bcwindowbase.inc"
#include "vframe.inc"
#include <stdarg.h>

class BC_ThemeSet;
class BC_ImageData;

class BC_ImageData {
public:
	char *name;
	unsigned char *data;
	int used;

	BC_ImageData(char *nm, unsigned char *dp) {
		name = nm;  data = dp;  used = 0;
	}
};


class BC_Theme
{
public:
	BC_Theme();
	virtual ~BC_Theme();

// Set pointer to binary object containing images and contents.
// Immediately loads the contents from the object.
	void set_data(unsigned char *ptr);

// Compose widgets using standard images.
// The arguments are copied into new VFrames for a new image set.
// The image set is put in the image table only if the title is nonzero.
	VFrame** new_button(const char *overlay_path,
		const char *up_path,
		const char *hi_path,
		const char *dn_path,
		const char *title);
	VFrame** new_button4(const char *overlay_path,
		const char *up_path,
		const char *hi_path,
		const char *dn_path,
		const char *disabled_path,
		const char *title);
	VFrame** new_button(const char *overlay_path,
		VFrame *up,
		VFrame *hi,
		VFrame *dn,
		const char *title);
	VFrame** new_toggle(const char *overlay_path,
		const char *up_path,
		const char *hi_path,
		const char *checked_path,
		const char *dn_path,
		const char *checkedhi_path,
		const char *title);
	VFrame** new_toggle(const char *overlay_path,
		VFrame *up,
		VFrame *hi,
		VFrame *checked,
		VFrame *dn,
		VFrame *checkedhi,
		const char *title);


// The two main routines for creating images are new_image_set and new_image.
// If the title already exists in the table, the existing entry is returned.
// These create image sets which are stored in the set table.
// Takes comma delimited char* pointers to filenames.
	VFrame** new_image_set(const char *title, int total, va_list *args);
	VFrame** new_image_set(const char *title, int total, ...);
	VFrame** new_image_set(int total, ...);
// Creates an image set from VFrame pointers.
// The images are considered not references and deleted with the image set.
// If the title already exists, the existing entry is deleted and overridden.
	VFrame** new_image_set_images(const char *title, int total, ...);

// Decompresses image and puts on images table before returning it.
	VFrame* new_image(const char *title, const char *path);
	VFrame* new_image(const char *path);


// These retrieve images based on case sensitive title
	VFrame* get_image(const char *title, int use_default = 1);
	VFrame** get_image_set(const char *title, int use_default = 1);
	BC_ThemeSet* get_image_set_object(const char *title);

// Loads compressed data into temporary
	unsigned char* get_image_data(const char *title, int log_errs=1);

// Verify all images have been used after initialization.
	void check_used();
	void sort_image_sets();

	void dump();
	BC_Resources* get_resources();

private:
	void overlay(VFrame *dst, VFrame *src, int in_x1 = -1, int in_x2 = -1, int shift = 0);
	void init_contents();



// Decompressed image storage.
// Sets of images.
	ArrayList<BC_ThemeSet*> image_sets;
	BC_ThemeSet *last_image_set;
	int image_sets_start;
	static int image_set_cmpr(const void *ap, const void *bp);
	void add_image_set(BC_ThemeSet *image_set);

// Compressed images are loaded in here.
	ArrayList<int> data_items;
	ArrayList<BC_ImageData *> images;
	BC_ImageData *last_image_data;
	static int images_cmpr(const void *ap, const void *bp);
};

class BC_ThemeSet
{
public:
// Set is_reference if the images are going to be created by new_image
	BC_ThemeSet(int total, int is_reference, const char *title);
	~BC_ThemeSet();

	VFrame **data;
	char *title;
	int total;
	int is_reference;
};



#endif
