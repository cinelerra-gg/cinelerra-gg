
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



// This program converts compressed PNG to uncompressed RAW for faster
// theme loading.

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
	if(argc < 2) return 1;

	for(argc--; argc > 0; argc--)
	{
		FILE *in;
		FILE *out;
		int w;
		int h;
		int components;
		int i;
		char variable[1024], header_fn[1024], output_fn[1024], *suffix, *prefix;

		in = fopen(argv[argc], "rb");
		if(!in) continue;

// Replace .png with .raw
		strcpy(output_fn, argv[argc]);
		suffix = strrchr(output_fn, '.');
		if(suffix) *suffix = 0;
		strcat(output_fn, ".raw");

		out = fopen(output_fn, "w");
		if(!out)
		{
			fclose(in);
			continue;
		}


// Decompress input
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop info_ptr = png_create_info_struct(png_ptr);
		int new_color_model;

  		png_init_io(png_ptr, in);
		png_read_info(png_ptr, info_ptr);

		w = png_get_image_width(png_ptr, info_ptr);
		h = png_get_image_height(png_ptr, info_ptr);

		int src_color_model = png_get_color_type(png_ptr, info_ptr);
		switch(src_color_model)
		{
			case PNG_COLOR_TYPE_RGB:
				components = 3;
				break;

			case PNG_COLOR_TYPE_GRAY_ALPHA:
				printf("pngtoraw %d: %s is alpha\n", __LINE__, argv[argc]);
				break;


			case PNG_COLOR_TYPE_RGB_ALPHA:
			default:
				components = 4;
				break;
		}

		unsigned char *buffer = (unsigned char*)malloc(w * h * components);
		unsigned char **rows = (unsigned char**)malloc(sizeof(unsigned char*) * h);
		for(i = 0; i < h; i++)
			rows[i] = buffer + w * components * i;
		png_read_image(png_ptr, rows);
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

		fputc('R', out);
		fputc('A', out);
		fputc('W', out);
		fputc(' ', out);

		fputc((w & 0xff), out);
		fputc(((w >> 8) & 0xff), out);
		fputc(((w >> 16) & 0xff), out);
		fputc(((w >> 24) & 0xff), out);


		fputc((h & 0xff), out);
		fputc(((h >> 8) & 0xff), out);
		fputc(((h >> 16) & 0xff), out);
		fputc(((h >> 24) & 0xff), out);

		fputc((components & 0xff), out);
		fputc(((components >> 8) & 0xff), out);
		fputc(((components >> 16) & 0xff), out);
		fputc(((components >> 24) & 0xff), out);

		fwrite(buffer, w * h * components, 1, out);
		free(buffer);

		fclose(out);
		fclose(in);
	}
}






