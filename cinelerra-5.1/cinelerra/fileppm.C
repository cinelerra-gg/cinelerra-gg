
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
#include "bccmodels.h"
#include "file.h"
#include "fileppm.h"
#include "interlacemodes.h"
#include "mainerror.h"

#include <string.h>
#include <unistd.h>


FilePPM::FilePPM(Asset *asset, File *file)
 : FileList(asset, file, "PPMLIST", ".ppm", FILE_PPM, FILE_PPM_LIST)
{
	reset();
	if( asset->format == FILE_UNKNOWN )
		asset->format = FILE_PPM;
}

FilePPM::~FilePPM()
{
	close_file();
}

void FilePPM::reset()
{
}

int FilePPM::check_sig(Asset *asset)
{
	FILE *stream = fopen(asset->path, "r");
	if( stream ) {
		char test[10];
		(void)fread(test, 10, 1, stream);
		fclose(stream);
		if( !strncmp("PPMLIST",test,6) ) return 1;
		if( !strncmp("P6\n",test,3) ) return 1;
	}
	return 0;
}

int FilePPM::check_frame_header(FILE *fp)
{
	char text[BCSTRLEN];
	if( !fgets(text, sizeof(text), fp) ) return 1;
	if( strcmp("P6\n",text) ) return 1;
	int ch = getc(fp);
	while( ch == '#' ) { while( (ch=getc(fp))>=0 && ch!='\n' ); }
	ungetc(ch,fp);
	int w, h;
	if( !fgets(text, sizeof(text), fp) ||
	    sscanf(text, "%d %d\n", &w, &h) != 2 ) return 1;
	if( !fgets(text, sizeof(text), fp) ||
	    sscanf(text, "%d\n", &ch) != 1 || ch != 255 ) return 1;

	asset->width = w;  asset->height = h;
	asset->interlace_mode = ILACE_MODE_NOTINTERLACED;
	return 0;
}

int FilePPM::read_frame_header(char *path)
{
	int ret = 1;
	FILE *fp = fopen(path, "r");
	if( fp ) {
		ret = check_frame_header(fp);
		fclose(fp);
	}
	return ret;
}

int FilePPM::read_ppm(FILE *fp, VFrame *frame)
{
	int ch;
	char text[BCSTRLEN];
	if( !fgets(text, sizeof(text), fp) ) return 1;
	if( strcmp("P6\n",text) ) {
		printf("FilePPM::read_ppm: header err\n");
		return 1;
	}
	while( (ch=getc(fp)) == '#' ) { while( (ch=getc(fp))>=0 && ch!='\n' ); }
	ungetc(ch,fp);
	int w, h;
	if( !fgets(text, sizeof(text), fp) ||
	    sscanf(text, "%d %d\n", &w, &h) != 2 ) {
		printf("FilePPM::read_ppm: geom err\n");
		return 1;
	}
	if( w != frame->get_w() || h != frame->get_h() ) {
		printf("FilePPM::read_ppm: geom mismatch\n");
		return 1;
	}
	if( !fgets(text, sizeof(text), fp) ||
	    sscanf(text, "%d\n", &ch) != 1 || ch != 255 ) {
		printf("FilePPM::read_ppm: mask err\n");
		return 1;
	}

	unsigned char **rows = frame->get_rows();
	int bpl = 3*w;
	for( int y=0; y<h; ++y ) {
		int ret = fread(rows[y],1,bpl,fp);
		if( ret != bpl ) {
			printf("FilePPM::read_ppm: read (%d,%d) err: %m\n", bpl, ret);
			return 1;
		}
	}
	return 0;
}

int FilePPM::read_frame(VFrame *frame, char *path)
{
	int result = 1;
	FILE *fp = fopen(path,"r");
	if( fp ) {
		result = read_ppm(fp, frame);
		fclose(fp);
	}
	if( result )
		eprintf("FilePPM::read_frame: cant read file %s\n", path);
	return result;
}

PPMUnit::PPMUnit(FilePPM *file, FrameWriter *writer)
 : FrameWriterUnit(writer)
{
	this->file = file;
	temp_frame = 0;
}
PPMUnit::~PPMUnit()
{
	delete temp_frame;
}

FrameWriterUnit* FilePPM::new_writer_unit(FrameWriter *writer)
{
	return new PPMUnit(this, writer);
}

int FilePPM::write_frame(VFrame *frame, VFrame *output,
		FrameWriterUnit *frame_writer_unit)
{
	int w = asset->width, h = asset->height;
	char prefix[BCTEXTLEN];
	int pfx = sprintf(prefix, "P6\n%d %d\n%d\n", w, h, 255);
	int bpl = 3*w, image_length = h*bpl, total_length = pfx + image_length;
	if( output->get_compressed_allocated() < total_length ) {
		int new_length = total_length + 255;
		output->allocate_compressed_data(new_length);
	}
	unsigned char *dp = output->get_data(), *bp = dp;
	memcpy(dp, prefix, pfx);  dp += pfx;
	unsigned char *rows[h+1], **rp = rows;
	for( int y=h; --y>=0; dp+=bpl ) *rp++ = dp;
	BC_CModels::transfer(rows, frame->get_rows(),
                0, 0, 0, frame->get_y(), frame->get_u(), frame->get_v(),
                0, 0, frame->get_w(), frame->get_h(), 0, 0, w, h,
                frame->get_color_model(), BC_RGB888, 0,
		frame->get_bytes_per_line(), bpl);

	output->set_compressed_size(dp - bp);
	return 0;
}

int FilePPM::colormodel_supported(int colormodel)
{
	return BC_RGB888;
}

int FilePPM::get_best_colormodel(Asset *asset, int driver)
{
	return BC_RGB888;
}

PPMConfigVideo::PPMConfigVideo(BC_WindowBase *gui, Asset *asset)
 : BC_Window(_(PROGRAM_NAME ": Video Compression"),
 	gui->get_abs_cursor_x(1), gui->get_abs_cursor_y(1), 200, 100)
{
	this->gui = gui;
	this->asset = asset;
}

void PPMConfigVideo::create_objects()
{
	lock_window("PPMConfigVideo::create_objects");
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, _("PPM, RGB raw only")));
	add_subwindow(new BC_OKButton(this));
	show_window(1);
	unlock_window();
}

void FilePPM::get_parameters(BC_WindowBase *parent_window,
	Asset *asset, BC_WindowBase* &format_window,
	int audio_options, int video_options, EDL *edl)
{
	if(video_options) {
		PPMConfigVideo *window = new PPMConfigVideo(parent_window, asset);
		window->create_objects();
		format_window = window;
		window->run_window();
		delete window;
	}
}

int FilePPM::use_path()
{
	return 1;
}
