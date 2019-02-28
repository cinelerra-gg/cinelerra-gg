
/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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
#include "cstrdup.h"
#include "file.h"
#include "filelist.h"
#include "guicast.h"
#include "interlacemodes.h"
#include "mutex.h"
#include "mwindow.inc"
#include "render.h"
#include "renderfarmfsserver.inc"
#include "vframe.h"
#include "mainerror.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


FileList::FileList(Asset *asset,
	File *file,
	const char *list_prefix,
	const char *file_extension,
	int frame_type,
	int list_type)
 : FileBase(asset, file)
{
	reset_parameters();
	path_list.set_array_delete();
	asset->video_data = 1;
	this->list_prefix = list_prefix;
	this->file_extension = file_extension;
	this->frame_type = frame_type;
	this->list_type = list_type;
	table_lock = new Mutex("FileList::table_lock");
}

FileList::~FileList()
{
	close_file();
	delete table_lock;
}

int FileList::reset_parameters_derived()
{
	data = 0;
	writer = 0;
	temp = 0;
	first_number = 0;
	return 0;
}

int FileList::open_file(int rd, int wr)
{
	int result = 1;

// skip header for write
	if(file->wr)
	{
		int fd = open(asset->path, O_CREAT+O_TRUNC+O_RDWR, 0777);
		if( fd >= 0 ) {
			close(fd);
			result = 0;
// Frame files are created in write_frame and list index is created when
// file is closed.
// Look for the starting number in the path but ignore the starting character
// and total digits since these are used by the header.
			Render::get_starting_number(asset->path,
				first_number, number_start, number_digits);
			path_list.remove_all_objects();
			writer = new FrameWriter(this,
			asset->format == list_type ? file->cpus : 1);
		}
		else
			eprintf(_("Error while opening \"%s\" for writing. \n%m\n"), asset->path);
	}
	else
	if(file->rd)
	{
// Determine type of file.
// Header isn't used for background rendering, in which case everything known
// by the file encoder is known by the decoder.
//printf("FileList::open_file %d %d\n", __LINE__, asset->use_header);
		if(asset->use_header)
		{
			FILE *stream = fopen(asset->path, "rb");
//printf("FileList::open_file %d asset->path=%s\n", __LINE__, asset->path);
			if(stream)
			{
				char string[BCTEXTLEN];
				int len = strlen(list_prefix);
				int ret = fread(string, 1, strlen(list_prefix), stream);
				fclose(stream);
				result = len == ret ? 0 : 1;
				if( !result && !strncasecmp(string, list_prefix, len)) {
					asset->format = list_type;
					result = read_list_header();
					if( !result )
						result = read_frame_header(path_list.values[0]);
				}
				else if( !result && frame_type != FILE_UNKNOWN ) {
					asset->format = frame_type;
					result = read_frame_header(asset->path);
					asset->video_length = -1;
				}
				else
					result = 1;
				if( !result ) {
					int width = asset->width;
					int height = asset->height;
					asset->actual_width = asset->width;
					if( width ) asset->width = width;
					asset->actual_height = asset->height;
					if( height ) asset->height = height;
					asset->layers = 1;
					if( !asset->frame_rate )
						asset->frame_rate = 1;
				}
			}
			else
				eprintf(_("Error while opening \"%s\" for reading. \n%m\n"), asset->path);
		}
		else
		{
			Render::get_starting_number(asset->path,
				first_number, number_start, number_digits, 6);
			result = 0;
		}
	}

	file->current_frame = 0;
// Compressed data storage
	data = new VFrame;

	return result;
}


int FileList::close_file()
{
//	path_list.total, asset->format, list_type, wr);
	if(asset->format == list_type && path_list.total)
	{
		if(file->wr && asset->use_header) write_list_header();
		path_list.remove_all_objects();
	}
	if(data) delete data;
	if(writer) delete writer;
	if(temp) delete temp;
	reset_parameters();

	FileBase::close_file();
	return 0;
}

int FileList::write_list_header()
{
	FILE *stream = fopen(asset->path, "w");
	if( !stream ) return 1;
// Use sprintf instead of fprintf for VFS.
	fprintf(stream, "%s\n", list_prefix);
	fprintf(stream, "# First line is always %s\n", list_prefix);
	fprintf(stream, "# Frame rate:\n");
	fprintf(stream, "%f\n", asset->frame_rate);
	fprintf(stream, "# Width:\n");
	fprintf(stream, "%d\n", asset->width);
	fprintf(stream, "# Height:\n");
	fprintf(stream, "%d\n", asset->height);
	fprintf(stream, "# List of image files follows\n");

	char *cp = strrchr(asset->path, '/');
	int dir_len = !cp ? 0 : cp - asset->path;

	for(int i = 0; i < path_list.total; i++) {
		const char *path = path_list.values[i];
// Fix path for VFS but leave leading slash
		if( !strncmp(path, RENDERFARM_FS_PREFIX, strlen(RENDERFARM_FS_PREFIX)) )
			path += strlen(RENDERFARM_FS_PREFIX);
// ./path for relative list access
		else if( dir_len > 0 && !strncmp(path, asset->path, dir_len) ) {
			fprintf(stream, ".");  path += dir_len;
		}
		fprintf(stream, "%s\n", path);
	}
	fclose(stream);
	return 0;
}

int FileList::read_list_header()
{
	char string[BCTEXTLEN];

	FILE *stream = fopen(asset->path, "r");
	if( !stream ) return 1;
// Get information about the frames
	do {
		if( feof(stream) || !fgets(string, BCTEXTLEN, stream) ) return 1;
	} while(string[0] == '#' || string[0] == ' ' || isalpha(string[0]));

// Don't want a user configured frame rate to get destroyed
	if(asset->frame_rate == 0)
		asset->frame_rate = atof(string);

	do {
		if( feof(stream) || !fgets(string, BCTEXTLEN, stream) ) return 1;
	} while(string[0] == '#' || string[0] == ' ');
	if( (asset->width = atol(string)) <= 0 ) return 1;

	do {
		if( feof(stream) || !fgets(string, BCTEXTLEN, stream) ) return 1;
	} while(string[0] == '#' || string[0] == ' ');
	if( (asset->height = atol(string)) <= 0 ) return 1;

	asset->interlace_mode = ILACE_MODE_UNDETECTED;
	asset->layers = 1;
	asset->audio_data = 0;
	asset->video_data = 1;

	char prefix[BCTEXTLEN], *bp = prefix, *cp = strrchr(asset->path, '/');
	for( int i=0, n=!cp ? 0 : cp-asset->path; i<n; ++i ) *bp++ = asset->path[i];
	*bp = 0;

// Get all the paths, expand relative paths
	int missing = 0;
	while( !feof(stream) && fgets(string, BCTEXTLEN, stream) ) {
		int len = strlen(string);
		if( !len || string[0] == '#' || string[0] == ' ' ) continue;
		if( string[len-1] == '\n' ) string[len-1] = 0;
		char path[BCTEXTLEN], *pp = path, *ep = pp + sizeof(path)-1;
		if( string[0] == '.' && string[1] == '/' && prefix[0] )
			pp += snprintf(pp, ep-pp, "%s/", prefix);
		snprintf(pp, ep-pp, "%s", string);
		if( access(path, R_OK) && !missing++ )
			eprintf(_("%s:no such file"), path);
		path_list.append(cstrdup(path));
	}

//for(int i = 0; i < path_list.total; i++) printf("%s\n", path_list.values[i]);
	fclose(stream);
	if( !(asset->video_length = path_list.total) )
		eprintf(_("%s:\nlist empty"), asset->path);
	if( missing )
		eprintf(_("%s:\n%d files not found"), asset->path, missing);
	return 0;
}

int FileList::read_frame(VFrame *frame)
{
	int result = 0;

//	PRINT_TRACE
// printf("FileList::read_frame %d %d use_header=%d current_frame=%d total=%d\n",
// __LINE__,
// result,
// asset->use_header,
// file->current_frame,
// path_list.total);

	if(file->current_frame < 0 ||
		(asset->use_header && file->current_frame >= path_list.total &&
			asset->format == list_type))
		return 1;

	if(asset->format == list_type)
	{
		char string[BCTEXTLEN];
		char *path;
		if(asset->use_header)
		{
			path = path_list.values[file->current_frame];
		}
		else
		{
			path = calculate_path(file->current_frame, string);
		}

		FILE *in;

// Fix path for VFS.  Not used anymore.
		if(!strncmp(asset->path, RENDERFARM_FS_PREFIX, strlen(RENDERFARM_FS_PREFIX)))
			sprintf(string, "%s%s", RENDERFARM_FS_PREFIX, path);
		else
			strcpy(string, path);



		if(!use_path() || frame->get_color_model() == BC_COMPRESSED)
		{
			if(!(in = fopen(string, "rb"))) {
				eprintf(_("Error while opening \"%s\" for reading. \n%m\n"), string);
			}
			else
			{
				struct stat ostat;
				stat(string, &ostat);

				switch(frame->get_color_model())
				{
					case BC_COMPRESSED:
						frame->allocate_compressed_data(ostat.st_size);
						frame->set_compressed_size(ostat.st_size);
						(void)fread(frame->get_data(), ostat.st_size, 1, in);
						break;
					default:
						data->allocate_compressed_data(ostat.st_size);
						data->set_compressed_size(ostat.st_size);
						(void)fread(data->get_data(), ostat.st_size, 1, in);
						result = read_frame(frame, data);
						break;
				}

				fclose(in);
			}
		}
		else
		{
//printf("FileList::read_frame %d %s\n", __LINE__, string);
			result = read_frame(frame, string);
		}
	}
	else
	{
		asset->single_frame = 1;
// Allocate and decompress single frame into new temporary
//printf("FileList::read_frame %d\n", frame->get_color_model());
		if( !temp || temp->get_color_model() != frame->get_color_model() ) {
			delete temp;  temp = 0;
			int aw = asset->actual_width, ah = asset->actual_height;
			int color_model = frame->get_color_model();
			switch( color_model ) {
			case BC_YUV420P:
			case BC_YUV420PI:
			case BC_YUV422P:
				aw = (aw+1) & ~1;  ah = (ah+1) & ~1;
				break;
			case BC_YUV410P:
			case BC_YUV411P:
				aw = (aw+3) & ~3;  ah = (ah+3) & ~3;
				break;
			}
			if( !use_path() || color_model == BC_COMPRESSED ) {
				FILE *fd = fopen(asset->path, "rb");
				if( fd ) {
					struct stat ostat;
					stat(asset->path, &ostat);

					switch(frame->get_color_model()) {
					case BC_COMPRESSED:
						frame->allocate_compressed_data(ostat.st_size);
						frame->set_compressed_size(ostat.st_size);
						(void)fread(frame->get_data(), ostat.st_size, 1, fd);
						break;
					default:
						data->allocate_compressed_data(ostat.st_size);
						data->set_compressed_size(ostat.st_size);
						(void)fread(data->get_data(), ostat.st_size, 1, fd);
						temp = new VFrame(aw, ah, color_model);
						read_frame(temp, data);
						break;
					}

					fclose(fd);
				}
				else {
					eprintf(_("Error while opening \"%s\" for reading. \n%m\n"), asset->path);
					result = 1;
				}
			}
			else {
				temp = new VFrame(aw, ah, color_model);
				read_frame(temp, asset->path);
			}
		}

		if(!temp) return result;

//printf("FileList::read_frame frame=%d temp=%d\n",
// frame->get_color_model(), // temp->get_color_model());
		frame->transfer_from(temp);
	}


// printf("FileList::read_frame %d %d\n", __LINE__, result);
//
// if(frame->get_y())
// for(int i = 0; i < 100000; i++)
// {
// 	frame->get_y()[i] = 0xff;
// }
// if(frame->get_rows())
// for(int i = 0; i < 100000; i++)
// {
// 	frame->get_rows()[0][i] = 0xff;
// }


	return result;
}

int FileList::write_frames(VFrame ***frames, int len)
{
	return_value = 0;

//printf("FileList::write_frames 1\n");
	if(frames[0][0]->get_color_model() == BC_COMPRESSED)
	{
		for(int i = 0; i < asset->layers && !return_value; i++)
		{
			for(int j = 0; j < len && !return_value; j++)
			{
				VFrame *frame = frames[i][j];
				char *path = create_path(frame->get_number());
//printf("FileList::write_frames %d %jd\n", __LINE__, frame->get_number());


				FILE *fd = fopen(path, "wb");
				if(fd)
				{
					return_value = !fwrite(frames[i][j]->get_data(),
						frames[i][j]->get_compressed_size(),
						1,
						fd);

					fclose(fd);
				}
				else
				{
					eprintf(_("Error while opening \"%s\" for writing. \n%m\n"), asset->path);
					return_value++;
				}
			}
		}
	}
	else
	{
//printf("FileList::write_frames 2\n");
		writer->write_frames(frames, len);
//printf("FileList::write_frames 100\n");
	}
	return return_value;
}









void FileList::add_return_value(int amount)
{
	table_lock->lock("FileList::add_return_value");
	return_value += amount;
	table_lock->unlock();
}

char* FileList::calculate_path(int number, char *string)
{
// Synthesize filename.
// If a header is used, the filename number must be in a different location.
	if(asset->use_header)
	{
		int k;
		strcpy(string, asset->path);
		for(k = strlen(string) - 1; k > 0 && string[k] != '.'; k--)
			;
		if(k <= 0) k = strlen(string);

		sprintf(&string[k], "%06d%s",
			number,
			file_extension);
	}
	else
// Without a header, the original filename can be altered.
	{
		Render::create_filename(string,
			asset->path,
			number,
			number_digits,
			number_start);
	}

	return string;
}

char* FileList::create_path(int number_override)
{
	if(asset->format != list_type) return asset->path;

	table_lock->lock("FileList::create_path");



	char *path = 0;
	char output[BCTEXTLEN];
	if(file->current_frame >= path_list.total || !asset->use_header)
	{
		int number;
		if(number_override < 0)
			number = file->current_frame++;
		else
		{
			number = number_override;
			file->current_frame++;
		}

		if(!asset->use_header)
		{
			number += first_number;
		}

		calculate_path(number, output);

		path = new char[strlen(output) + 1];
		strcpy(path, output);
		path_list.append(path);
	}
	else
	{
// Overwrite an old path
		path = path_list.values[file->current_frame];
	}


	table_lock->unlock();

	return path;
}

FrameWriterUnit* FileList::new_writer_unit(FrameWriter *writer)
{
	return new FrameWriterUnit(writer);
}

int64_t FileList::get_memory_usage()
{
	int64_t result = 0;
	if(data) result += data->get_compressed_allocated();
	if(temp) result += temp->get_data_size();
// printf("FileList::get_memory_usage %d %p %s %jd\n",
// __LINE__,
// this,
// file->asset->path,
// result);
	return result;
}

int FileList::get_units()
{
	return !writer ? 0 : writer->get_total_clients();
}

FrameWriterUnit* FileList::get_unit(int number)
{
	return !writer ? 0 : (FrameWriterUnit*)writer->get_client(number);
}

int FileList::use_path()
{
	return 0;
}






FrameWriterPackage::FrameWriterPackage()
{
}

FrameWriterPackage::~FrameWriterPackage()
{
}











FrameWriterUnit::FrameWriterUnit(FrameWriter *server)
 : LoadClient(server)
{
// Don't use server here since subclasses call this with no server.
	this->server = server;
	output = new VFrame;
}

FrameWriterUnit::~FrameWriterUnit()
{
	delete output;
}

void FrameWriterUnit::process_package(LoadPackage *package)
{
//printf("FrameWriterUnit::process_package 1\n");
	FrameWriterPackage *ptr = (FrameWriterPackage*)package;

	FILE *file;

//printf("FrameWriterUnit::process_package 2 %s\n", ptr->path);
	if(!(file = fopen(ptr->path, "wb")))
	{
		eprintf(_("Error while opening \"%s\" for writing. \n%m\n"), ptr->path);
		return;
	}
//printf("FrameWriterUnit::process_package 3");


	int result = server->file->write_frame(ptr->input, output, this);

//printf("FrameWriterUnit::process_package 4 %s %d\n", ptr->path, output->get_compressed_size());
	if(!result) result = !fwrite(output->get_data(), output->get_compressed_size(), 1, file);
//TRACE("FrameWriterUnit::process_package 4");
	fclose(file);
//TRACE("FrameWriterUnit::process_package 5");

	server->file->add_return_value(result);
//TRACE("FrameWriterUnit::process_package 6");
}











FrameWriter::FrameWriter(FileList *file, int cpus)
 : LoadServer(cpus, 0)
{
	this->file = file;
}


FrameWriter::~FrameWriter()
{
}

void FrameWriter::init_packages()
{
	for(int i = 0, layer = 0, number = 0;
		i < get_total_packages();
		i++)
	{
		FrameWriterPackage *package = (FrameWriterPackage*)get_package(i);
		package->input = frames[layer][number];
		package->path = file->create_path(package->input->get_number());
// printf("FrameWriter::init_packages 1 %p %d %s\n",
// package->input,
// package->input->get_number(),
// package->path);
		number++;
		if(number >= len)
		{
			layer++;
			number = 0;
		}
	}
}

void FrameWriter::write_frames(VFrame ***frames, int len)
{
	this->frames = frames;
	this->len = len;
	set_package_count(len * file->asset->layers);

	process_packages();
}

LoadClient* FrameWriter::new_client()
{
	return file->new_writer_unit(this);
}

LoadPackage* FrameWriter::new_package()
{
	return new FrameWriterPackage;
}



