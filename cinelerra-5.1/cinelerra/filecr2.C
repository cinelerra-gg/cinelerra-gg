
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

#include "asset.h"
#include "bchash.h"
#include "clip.h"
#include "bccmodels.h"
#include "file.h"
#include "filecr2.h"
#include "mutex.h"
#include <string.h>
#include <unistd.h>


#define NODEPS
#define NO_JASPER
#define NO_JPEG
#define NO_LCMS
#include "dcraw.h"

int FileCR2::dcraw_run(FileCR2 *file, int argc, const char **argv, VFrame *frame)
{
	DCRaw dcraw;
	if( frame ) {
		dcraw.data = (float**) frame->get_rows();
		dcraw.alpha = frame->get_color_model() == BC_RGBA_FLOAT ? 1 : 0;
	}
	int result = dcraw.main(argc, argv);
	if( file )
		file->format_to_asset(dcraw.info);
	if( frame ) {
// This was only used by the bayer interpolate plugin, which itself created
// too much complexity to use effectively.
// It required bypassing the cache any time a plugin parameter changed
// to store the color matrix from dcraw in the frame stack along with the new
// plugin parameters.  The cache couldn't know if a parameter in the stack came
// from dcraw or a plugin & replace it.
		char string[BCTEXTLEN];
		sprintf(string, "%f %f %f %f %f %f %f %f %f\n",
			dcraw.matrix[0], dcraw.matrix[1], dcraw.matrix[2],
			dcraw.matrix[3], dcraw.matrix[4], dcraw.matrix[5],
			dcraw.matrix[6], dcraw.matrix[7], dcraw.matrix[8]);
		frame->get_params()->update("DCRAW_MATRIX", string);
// frame->dump_params();
	}
	return result;
}

FileCR2::FileCR2(Asset *asset, File *file)
 : FileList(asset, file, "CR2LIST", ".cr2", FILE_CR2, FILE_CR2_LIST)
{
	reset();
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_CR2;
}

FileCR2::~FileCR2()
{
//printf("FileCR2::~FileCR2\n");
	close_file();
}


void FileCR2::reset()
{
}

int FileCR2::check_sig(Asset *asset)
{
	char *ptr = strstr(asset->path, ".pcm");
	if(ptr) return 0;
//printf("FileCR2::check_sig %d\n", __LINE__);
	FILE *stream = fopen(asset->path, "rb");
	if( stream ) {
		char test[10];
		(void)fread(test, 10, 1, stream);
		fclose(stream);
		if( !strncmp("CR2LIST",test,6) ) return 1;
	}
	char string[BCTEXTLEN];
	strcpy(string, asset->path);
	const char *argv[4] = { "dcraw", "-i", string, 0 };
	int argc = 3;
	int result = dcraw_run(0, argc, argv);
//printf("FileCR2::check_sig %d %d\n", __LINE__, result);
	return !result;
}

int FileCR2::read_frame_header(char *path)
{
	int argc = 3;
	const char *argv[4] = { "dcraw", "-i", path, 0 };
	int result = dcraw_run(this, argc, argv);
	return result;
}




// int FileCR2::close_file()
// {
// 	return 0;
// }
//
void FileCR2::format_to_asset(const char *info)
{
	asset->video_data = 1;
	asset->layers = 1;
	sscanf(info, "%d %d", &asset->width, &asset->height);
}


int FileCR2::read_frame(VFrame *frame, char *path)
{
//printf("FileCR2::read_frame\n");

// Want to disable interpolation if an interpolation plugin is on, but
// this is impractical because of the amount of caching.  The interpolation
// could not respond to a change in the plugin settings and it could not
// reload the frame after the plugin was added.  Also, since an 8 bit
// PBuffer would be required, it could never have enough resolution.
//	int interpolate = 0;
// 	if(!strcmp(frame->get_next_effect(), "Interpolate Pixels"))
// 		interpolate = 0;


// printf("FileCR2::read_frame %d\n", interpolate);
// frame->dump_stacks();
	int argc = 0;  const char *argv[10];
	argv[argc++] = "dcraw";
	argv[argc++] = "-c"; // output to stdout
	argv[argc++] = "-j"; // no rotation

// printf("FileCR2::read_frame %d interpolate=%d white_balance=%d\n",
// __LINE__, file->interpolate_raw, file->white_balance_raw);

// Use camera white balance.
// Before 2006, DCraw had no Canon white balance.
// In 2006 DCraw seems to support Canon white balance.
// Still no gamma support.
// Need to toggle this in preferences because it defeats dark frame subtraction.
	if(file->white_balance_raw)
		argv[argc++] = "-w";


	if(!file->interpolate_raw) {
// Trying to do everything but interpolate doesn't work because convert_to_rgb
// doesn't work with bayer patterns.
// Use document mode and hack dcraw to apply white balance in the write_ function.
		argv[argc++] = "-d";
	}

//printf("FileCR2::read_frame %d %s\n", __LINE__, path);
	argv[argc++] = path;
	argv[argc] = 0;

//Timer timer;
	int result = dcraw_run(0, argc, argv, frame);
	return result;
}

int FileCR2::colormodel_supported(int colormodel)
{
	if(colormodel == BC_RGB_FLOAT ||
		colormodel == BC_RGBA_FLOAT)
		return colormodel;
	return BC_RGB_FLOAT;
}


// Be sure to add a line to File::get_best_colormodel
int FileCR2::get_best_colormodel(Asset *asset, int driver)
{
//printf("FileCR2::get_best_colormodel %d\n", __LINE__);
	return BC_RGB_FLOAT;
}

// int64_t FileCR2::get_memory_usage()
// {
// 	int64_t result = asset->width * asset->height * sizeof(float) * 3;
//printf("FileCR2::get_memory_usage %d %jd\n", __LINE__, result);
// 	return result;
// }


int FileCR2::use_path()
{
	return 1;
}





