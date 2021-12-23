
/*
 * CINELERRA
 * Copyright (C) 2014 Adam Williams <broadcast at earthling dot net>
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
#include "bcsignals.h"
#include "file.h"
#include "filegif.h"
#include "gif_lib.h"
#include "mainerror.h"
#include "interlacemodes.h"
#include "vframe.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

FileGIF::FileGIF(Asset *asset, File *file)
 : FileBase(asset, file)
{
	offset = 0;
	err = 0;
	gif_file = 0;
	eof = 1;
	row_size = 0;
	rows = 0;
	depth = 8;
	fp = 0;
	fd = -1;
	writes = -1;
	buffer = 0;
	bg = 0;
	output = 0;
}

FileGIF::~FileGIF()
{
	close_file();
}

int FileGIF::check_sig(Asset *asset)
{
	FILE *stream = fopen(asset->path, "rb");
	if( stream ) {
		char test[8];
		int ret = fread(test, 1, 6, stream);
		fclose(stream);
		if( ret >= 6 &&
		    test[0] == 'G' && test[1] == 'I' && test[2] == 'F' &&
		    test[3] == '8' && (test[4] == '7' || test[4] == '9') &&
		    test[5] == 'a' ) return 1;
	}
	return 0;
}

int FileGIF::colormodel_supported(int colormodel)
{
	return BC_RGB888;
}

int FileGIF::get_best_colormodel(Asset *asset, int driver)
{
	return BC_RGB888;
}


int FileGIF::read_frame_header(char *path)
{
	FILE *stream = fopen(path, "rb");
	if( stream ) {
		unsigned char test[16];
		int ret = fread(test, 16, 1, stream);
		fclose(stream);
		if( ret < 1 ) return 1;
		asset->format = FILE_GIF;
		asset->width = test[6] | (test[7] << 8);
		asset->height = test[8] | (test[9] << 8);
		return 0;
	}

	perror(path);
	return 1;
}

static int input_file(GifFileType *gif_file, GifByteType *buffer, int bytes)
{
	FileGIF *file = (FileGIF*)gif_file->UserData;
	fseek(file->fp, file->offset, SEEK_SET);
	bytes = fread(buffer, 1, bytes, file->fp);
	file->offset += bytes;
	return bytes;
}

int FileGIF::open_file(int rd, int wr)
{
	return  rd ? ropen_path(asset->path) :
		wr ? wopen_path(asset->path) :
		0 ;
}

int FileGIF::ropen_path(const char *path)
{
	fp = fopen(path, "r");
	int result = !fp ? 1 : 0;
	if( !result ) {
		offset = 0;  eof = 0;
		gif_file = DGifOpen(this, input_file, &err);
		if( !gif_file ) {
			eprintf("FileGIF::ropen_path %d: %s\n", __LINE__, GifErrorString(err));
			result = 1;
		}
	}
	if( !result )
		result = open_gif();
	return result;
}

int FileGIF::wopen_path(const char *path)
{
	fd = open(path, O_CREAT+O_TRUNC+O_WRONLY, 0777);
	int result = fd < 0 ? 1 : 0;
	if( !result ) {
		gif_file = EGifOpenFileHandle(fd, &err);
		if( !gif_file ) {
			eprintf("FileGIF::wopen_path %d: %s\n", __LINE__, GifErrorString(err));
			result = 1;
		}
	}
	if( !result ) {
		writes = 0;
	}
	return result;
}

int FileGIF::write_frames(VFrame ***frames, int len)
{
	int result = !gif_file ? 1 : 0;
	for( int i=0; i<len && !result; ++i )
		result = write_frame(frames[0][i]);
	return result;
}

int FileGIF::open_gif()
{
	file_pos.remove_all();
	int width = asset->width;
	int height = asset->height;
	int result = read_frame_header(asset->path);
	if( !result ) {
		asset->actual_width = asset->width;
		if( width ) asset->width = width;
		asset->actual_height = asset->height;
		if( height ) asset->height = height;
		asset->layers = 1;
		if( !asset->frame_rate )
			asset->frame_rate = 10;
		asset->video_data = 1;
		row_size = gif_file->SWidth * sizeof(GifPixelType);
		bg = (GifRowType)malloc(row_size);
		for( int i=0; i<gif_file->SWidth; ++i )
			bg[i] = gif_file->SBackGroundColor;
		rows = gif_file->SHeight;
		buffer = (GifRowType*)malloc(sizeof(GifRowType) * rows);
		for( int i=0; i<gif_file->SHeight; ++i ) {
			buffer[i] = (GifRowType)malloc(row_size);
			memcpy(buffer[i], bg, row_size);
		}
		result = scan_gif();
		asset->video_length = file_pos.size();
	}
	return result;
}

int FileGIF::close_file()
{
	if( gif_file ) {
		EGifCloseFile(gif_file, &err);
		gif_file = 0;
	}
	if( fp ) {
		fclose(fp);  fp = 0;
	}
	if( fd >= 0 ) {
		close(fd);  fd = -1;
	}
	if( bg ) { free(bg);  bg = 0; }
	if( buffer ) {
		for( int k=0; k<rows; ++k )
			free(buffer[k]);
		free(buffer);  buffer = 0;
		rows = 0;
	}
	offset = 0;
	row_size = 0;
	err = 0;
	writes = -1;
	eof = 1;
	output = 0;
	FileBase::close_file();
	return 0;
}

int FileGIF::scan_gif()
{
	int file_eof = eof;
	int64_t file_offset = offset;
	file_pos.remove_all();
	int image_pos = offset, ret;
// read all imgs, build file_pos index
	while( (ret=read_next_image(0)) > 0 ) {
		file_pos.append(image_pos);
		image_pos = offset;
	}
	eof = file_eof;
	offset = file_offset;
	return ret;
}

int FileGIF::set_video_position(int64_t pos)
{
	if( !gif_file || !asset->video_length ) return 1;
	int64_t sz = file_pos.size();
	eof = pos < 0 || pos >= sz ? 1 : 0;
	offset = !eof ? file_pos[pos] : 0;
	return 0;
}

int FileGIF::read_frame(VFrame *output)
{
	if( !gif_file ) return 1;
	for( int i=0; i<gif_file->SHeight; ++i )
		memcpy(buffer[i], bg, row_size);
	int ret = read_next_image(output) > 0 ? 0 : 1;
	return ret;
}

// ret = -1:err, 0:eof, 1:frame
int FileGIF::read_next_image(VFrame *output)
{
	int ret = 0;
	GifRecordType record_type;

	while( !ret && !eof ) {
		if( DGifGetRecordType(gif_file, &record_type) == GIF_ERROR ) {
			err = gif_file->Error;
			eprintf("FileGIF::read_frame %d: %s\n", __LINE__, GifErrorString(err));
			ret = -1;
			break;
		}

		switch( record_type ) {
		case IMAGE_DESC_RECORD_TYPE: {
			if( DGifGetImageDesc(gif_file) == GIF_ERROR ) {
				err = gif_file->Error;
				eprintf("FileGIF::read_frame %d: %s\n", __LINE__, GifErrorString(err));
				break;
			}
			int row = gif_file->Image.Top;
			int col = gif_file->Image.Left;
			int width = gif_file->Image.Width;
			int height = gif_file->Image.Height;
			if( gif_file->Image.Left + gif_file->Image.Width > gif_file->SWidth ||
	 		    gif_file->Image.Top + gif_file->Image.Height > gif_file->SHeight )
				ret = -1;
			if( !ret && gif_file->Image.Interlace ) {
				static int InterlacedOffset[] = { 0, 4, 2, 1 };
				static int InterlacedJumps[] = { 8, 8, 4, 2 };
/* Need to perform 4 passes on the images: */
				for( int i=0; i<4; ++i ) {
					int j = row + InterlacedOffset[i];
					for( ; !ret && j<row + height; j+=InterlacedJumps[i] ) {
						if( DGifGetLine(gif_file, &buffer[j][col], width) == GIF_ERROR )
							ret = -1;
					}
				}
			}
			else {
				for( int i=0; !ret && i<height; ++i ) {
					if (DGifGetLine(gif_file, &buffer[row++][col], width) == GIF_ERROR)
						ret = -1;
				}
			}
			ret = 1;
			break; }
		case EXTENSION_RECORD_TYPE: {
			int ExtFunction = 0;
			GifByteType *ExtData = 0;
			if( DGifGetExtension(gif_file, &ExtFunction, &ExtData) == GIF_ERROR )
				ret = -1;
			while( !ret && ExtData ) {
				if( DGifGetExtensionNext(gif_file, &ExtData) == GIF_ERROR )
					ret = -1;
			}
			break; }
		case TERMINATE_RECORD_TYPE:
			eof = 1;
			break;
		default:
			ret = -1;
			break;
		}
	}

	ColorMapObject *color_map = 0;
	if( ret > 0 ) {
		color_map = gif_file->Image.ColorMap;
		if( !color_map ) color_map = gif_file->SColorMap;
		if( !color_map ) ret = -1;
	}
	if( ret > 0 && output ) {
		int screen_width = gif_file->SWidth;
		int screen_height = gif_file->SHeight;
		for( int i=0; i<screen_height; ++i ) {
			GifRowType row = buffer[i];
			unsigned char *out_ptr = output->get_rows()[i];
			for( int j=0; j<screen_width; ++j ) {
				GifColorType *color_map_entry = &color_map->Colors[row[j]];
				*out_ptr++ = color_map_entry->Red;
				*out_ptr++ = color_map_entry->Green;
				*out_ptr++ = color_map_entry->Blue;
			}
		}
	}
	return ret;
}

int FileGIF::write_frame(VFrame *frame)
{
	int w = frame->get_w(), h = frame->get_h();
	ColorMapObject *cmap = 0;
	int cmap_sz = depth >= 0 ? 1 << depth : 0;
	int64_t len = w * h * sizeof(GifByteType);
	GifByteType *bfr = (GifByteType *) malloc(len);
	int result = !bfr ? 1 : 0;
	if( !result ) {
		VFrame gbrp(w, h, BC_GBRP);
		gbrp.transfer_from(frame);
		if( !(cmap = GifMakeMapObject(cmap_sz, 0)) )
			result = 1;
		if( !result ) {
			GifByteType *gp = (GifByteType *)gbrp.get_r();
			GifByteType *bp = (GifByteType *)gbrp.get_g();
			GifByteType *rp = (GifByteType *)gbrp.get_b();
			if( GifQuantizeBuffer(w, h, &cmap_sz, rp, gp, bp,
					bfr, cmap->Colors) == GIF_ERROR )
				result = 1;
		}
	}
	if( !result && !writes &&
	    EGifPutScreenDesc(gif_file, w, h, depth, 0, 0) == GIF_ERROR )
		result = 1;
	if( !result &&
	    EGifPutImageDesc(gif_file, 0, 0, w, h, 0, cmap) == GIF_ERROR )
		result = 1;

	GifByteType *bp = bfr;
	for( int y=0; !result && y<h; ++y ) {
	        if( EGifPutLine(gif_file, bp, w) == GIF_ERROR )
			result = 1;
		bp += w;
	}
	GifFreeMapObject(cmap);
	if( bfr ) free(bfr);
	++writes;
	return result;
}

static int write_data(GifFileType *gif_file, const GifByteType *bfr, int bytes)
{
	FileGIF *file = (FileGIF*)gif_file->UserData;
	VFrame *output = file->output;
	long size = output->get_compressed_size();
	long alloc = output->get_compressed_allocated();
	long len = size + bytes;
	if( len > alloc )
		output->allocate_compressed_data(2*size + bytes);
	unsigned char *data = output->get_data() + size;
	memcpy(data, bfr, bytes);
	output->set_compressed_size(len);
	return bytes;
}

int FileGIF::wopen_data(VFrame *output)
{
	int result = 0;
	gif_file = EGifOpen(this, write_data, &err);
	if( !gif_file ) {
		eprintf("FileGIF::wopen_data %d: %s\n", __LINE__, GifErrorString(err));
		result = 1;
	}
	if( !result ) {
		output->set_compressed_size(0);
		this->output = output;
		writes = 0;
	}
	return result;
}


FileGIFList::FileGIFList(Asset *asset, File *file)
 : FileList(asset, file, "GIFLIST", ".gif", FILE_UNKNOWN, FILE_GIF_LIST)
{
}

FileGIFList::~FileGIFList()
{
}

int FileGIFList::check_sig(Asset *asset)
{
	FILE *stream = fopen(asset->path, "rb");
	if( stream ) {
		unsigned char test[16];
		int ret = fread(test, 16, 1, stream);
		fclose(stream);
		if( ret < 1 ) return 1;
		if( test[0] == 'G' && test[1] == 'I' && test[2] == 'F' &&
		    test[3] == 'L' && test[4] == 'I' && test[5] == 'S' && test[6] == 'T')
			return 1;
	}
	return 0;
}

int FileGIFList::colormodel_supported(int colormodel) { return BC_RGB888; }
int FileGIFList::get_best_colormodel(Asset *asset, int driver) { return BC_RGB888; }

int FileGIFList::read_frame_header(char *path)
{
	FILE *stream = fopen(path, "rb");
	if( stream ) {
		unsigned char test[16];
		int ret = fread(test, 16, 1, stream);
		fclose(stream);
		if( ret < 1 ) return 1;
		asset->format = FILE_GIF_LIST;
		asset->width = test[6] | (test[7] << 8);
		asset->height = test[8] | (test[9] << 8);
		return 0;
	}
	perror(path);
	return 1;
}

int FileGIFList::read_frame(VFrame *output, char *path)
{
	Asset *asset = new Asset(path);
	FileGIF gif(asset, file);
	int ret = gif.ropen_path(path);
	if( !ret )
		ret = gif.read_frame(output);
	asset->remove_user();
	return ret;
}

int FileGIFList::write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit)
{
	int native_cmodel = BC_RGB888;
	if( frame->get_color_model() != native_cmodel ) {
		GIFUnit *gif_unit = (GIFUnit *)unit;
		if( !gif_unit->temp_frame ) gif_unit->temp_frame =
			new VFrame(frame->get_w(), frame->get_h(), native_cmodel);
		gif_unit->temp_frame->transfer_from(frame);
		frame = gif_unit->temp_frame;
	}

	FileGIF gif(asset, file);
	int ret = gif.wopen_data(data);
	if( !ret )
		ret = gif.write_frame(frame);
	return ret;
}

int FileGIFList::verify_file_list()
{
	// go through all .gif files in the list and
	// verify their sizes match or not.
	//printf("\nAsset Path: %s\n", asset->path);
	FILE *stream = fopen(asset->path, "rb");
	if (stream) {
		char string[BCTEXTLEN];
		int width, height, prev_width=-1, prev_height=-1;
		// build the path prefix
		char prefix[BCTEXTLEN], *bp = prefix, *cp = strrchr(asset->path, '/');
		for( int i=0, n=!cp ? 0 : cp-asset->path; i<n; ++i ) *bp++ = asset->path[i];
		*bp = 0;
		// read entire input file
		while( !feof(stream) && fgets(string, BCTEXTLEN, stream) ) {
			int len = strlen(string);
			if(!len || string[0] == '#' || string[0] == ' ' || isalnum(string[0])) continue;
			if( string[len-1] == '\n' ) string[len-1] = 0;
			// a possible .gif file path? fetch it
			char path[BCTEXTLEN], *pp = path, *ep = pp + sizeof(path)-1;
			if( string[0] == '.' && string[1] == '/' && prefix[0] )
				pp += snprintf(pp, ep-pp, "%s/", prefix);
			snprintf(pp, ep-pp, "%s", string);
			// check if a valid file exists
			if(!access(path, R_OK)) {
				// check file header for size
				FILE *gif_file_temp = fopen(path, "rb");
				if (gif_file_temp) {
					unsigned char test[16];
					int ret = fread(test, 16, 1, gif_file_temp);
					fclose(gif_file_temp);
					if( ret < 1 ) continue;
					// get height and width of gif file
					width = test[6] | (test[7] << 8);
					height = test[8] | (test[9] << 8);
					// test with previous
					if ( (prev_width == -1) && (prev_height == -1) ) {
						prev_width = width;
						prev_height = height;
						continue;
					}
					else if ( (prev_width != width) || (prev_height != height) ) {
						// this is the error case we are trying to avoid
						fclose(stream);
						return 0;
					}
				}

			}
		}
		fclose(stream);
		return 1;
	}
	// not sure if our function should be the one to raise not found error
	perror(asset->path);
	return 0;
}

FrameWriterUnit* FileGIFList::new_writer_unit(FrameWriter *writer)
{
	return new GIFUnit(this, writer);
}

GIFUnit::GIFUnit(FileGIFList *file, FrameWriter *writer)
 : FrameWriterUnit(writer)
{
	this->file = file;
	temp_frame = 0;
}

GIFUnit::~GIFUnit()
{
	delete temp_frame;
}

