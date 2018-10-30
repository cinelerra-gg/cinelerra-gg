/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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
#include "assets.h"
#include "awindowgui.h"
#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "cstrdup.h"
#include "edl.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "indexstate.h"
#include "interlacemodes.h"

#include <stdio.h>
#include <string.h>


Asset::Asset()
 : Indexable(1), ListItem<Asset>()
{
	init_values();
}

Asset::Asset(Asset &asset)
 : Indexable(1), ListItem<Asset>()
{
	init_values();
	this->copy_from(&asset, 1);
}

Asset::Asset(const char *path)
 : Indexable(1), ListItem<Asset>()
{
	init_values();
	strcpy(this->path, path);
}

Asset::Asset(const int plugin_type, const char *plugin_title)
 : Indexable(1), ListItem<Asset>()
{
	init_values();
}

Asset::~Asset()
{
}

int Asset::init_values()
{
	path[0] = 0;
// Has to be unknown for file probing to succeed
	format = FILE_UNKNOWN;
	bits = 0;
	byte_order = 0;
	signed_ = 0;
	header = 0;
	dither = 0;
	reset_audio();
	reset_video();

	strcpy(vcodec, "");
	strcpy(acodec, "");

	strcpy(fformat,"mp4");
	ff_audio_options[0] = 0;
	ff_sample_format[0] = 0;
	ff_audio_bitrate = 0;
	ff_audio_quality = -1;
	ff_video_options[0] = 0;
	ff_pixel_format[0] = 0;
	ff_video_bitrate = 0;
	ff_video_quality = -1;

	jpeg_quality = 80;
	aspect_ratio = -1;
	interlace_autofixoption = ILACE_AUTOFIXOPTION_AUTO;
	interlace_mode = ILACE_MODE_UNDETECTED;
	interlace_fixmethod = ILACE_FIXMETHOD_NONE;

	mp3_bitrate = 224;
	ampeg_bitrate = 256;
	ampeg_derivative = 3;

	vorbis_vbr = 0;
	vorbis_min_bitrate = -1;
	vorbis_bitrate = 128000;
	vorbis_max_bitrate = -1;

	theora_fix_bitrate = 1;
	theora_bitrate = 860000;
	theora_quality = 16;
	theora_sharpness = 2;
	theora_keyframe_frequency = 64;
	theora_keyframe_force_frequency = 64;

// mpeg parameters
	vmpeg_iframe_distance = 45;
	vmpeg_pframe_distance = 0;
	vmpeg_progressive = 0;
	vmpeg_denoise = 1;
	vmpeg_bitrate = 1000000;
	vmpeg_derivative = 1;
	vmpeg_quantization = 15;
	vmpeg_cmodel = BC_YUV420P;
	vmpeg_fix_bitrate = 0;
	vmpeg_seq_codes = 0;
	vmpeg_preset = 0;
	vmpeg_field_order = 0;

	ac3_bitrate = 128;

	png_use_alpha = 0;

	exr_use_alpha = 0;
	exr_compression = 0;

	tiff_cmodel = 0;
	tiff_compression = 0;
	mov_sphere = 0;
	jpeg_sphere = 0;

	use_header = 1;
	id = EDL::next_id();

	return 0;
}

void Asset::reset_audio()
{
	audio_data = 0;
	channels = 0;
	sample_rate = 0;
	audio_length = 0;
}

void Asset::reset_video()
{
	video_data = 0;
	layers = 0;
	actual_width = width = 0;
	actual_height = height = 0;
	video_length = 0;
	single_frame = 0;
	vmpeg_cmodel = BC_YUV420P;
	frame_rate = 0;
	program = -1;
}


void Asset::boundaries()
{
//printf("Asset::boundaries %d %d %f\n", __LINE__, sample_rate, frame_rate);
// sample_rate & frame_rate are user defined
// 	CLAMP(sample_rate, 1, 1000000);
// 	CLAMP(frame_rate, 0.001, 1000000);
 	CLAMP(channels, 0, 16);
 	CLAMP(width, 0, 10000);
 	CLAMP(height, 0, 10000);
//printf("Asset::boundaries %d %d %f\n", __LINE__, sample_rate, frame_rate);
}

void Asset::copy_from(Asset *asset, int do_index)
{
	copy_location(asset);
	copy_format(asset, do_index);
}

void Asset::copy_location(Asset *asset)
{
	strcpy(path, asset->path);
	folder_no = asset->folder_no;
}

void Asset::copy_format(Asset *asset, int do_index)
{
	if(do_index) update_index(asset);

	audio_data = asset->audio_data;
	format = asset->format;
	strcpy(fformat, asset->fformat);
	channels = asset->channels;
	sample_rate = asset->sample_rate;
	bits = asset->bits;
	byte_order = asset->byte_order;
	signed_ = asset->signed_;
	header = asset->header;
	dither = asset->dither;
	mp3_bitrate = asset->mp3_bitrate;
	use_header = asset->use_header;
	aspect_ratio = asset->aspect_ratio;
	interlace_autofixoption = asset->interlace_autofixoption;
	interlace_mode = asset->interlace_mode;
	interlace_fixmethod = asset->interlace_fixmethod;

	video_data = asset->video_data;
	layers = asset->layers;
	program = asset->program;
	frame_rate = asset->frame_rate;
	width = asset->width;
	height = asset->height;
	actual_width = asset->actual_width;
	actual_height = asset->actual_height;
	strcpy(vcodec, asset->vcodec);
	strcpy(acodec, asset->acodec);

	strcpy(ff_audio_options, asset->ff_audio_options);
	strcpy(ff_sample_format, asset->ff_sample_format);
	ff_audio_bitrate = asset->ff_audio_bitrate;
	ff_audio_quality = asset->ff_audio_quality;
	strcpy(ff_video_options, asset->ff_video_options);
	strcpy(ff_pixel_format, asset->ff_pixel_format);
	ff_video_bitrate = asset->ff_video_bitrate;
	ff_video_quality = asset->ff_video_quality;

	this->audio_length = asset->audio_length;
	this->video_length = asset->video_length;
	this->single_frame = asset->single_frame;

	ampeg_bitrate = asset->ampeg_bitrate;
	ampeg_derivative = asset->ampeg_derivative;


	vorbis_vbr = asset->vorbis_vbr;
	vorbis_min_bitrate = asset->vorbis_min_bitrate;
	vorbis_bitrate = asset->vorbis_bitrate;
	vorbis_max_bitrate = asset->vorbis_max_bitrate;


	theora_fix_bitrate = asset->theora_fix_bitrate;
	theora_bitrate = asset->theora_bitrate;
	theora_quality = asset->theora_quality;
	theora_sharpness = asset->theora_sharpness;
	theora_keyframe_frequency = asset->theora_keyframe_frequency;
	theora_keyframe_force_frequency = asset->theora_keyframe_frequency;


	jpeg_quality = asset->jpeg_quality;

// mpeg parameters
	vmpeg_iframe_distance = asset->vmpeg_iframe_distance;
	vmpeg_pframe_distance = asset->vmpeg_pframe_distance;
	vmpeg_progressive = asset->vmpeg_progressive;
	vmpeg_denoise = asset->vmpeg_denoise;
	vmpeg_bitrate = asset->vmpeg_bitrate;
	vmpeg_derivative = asset->vmpeg_derivative;
	vmpeg_quantization = asset->vmpeg_quantization;
	vmpeg_cmodel = asset->vmpeg_cmodel;
	vmpeg_fix_bitrate = asset->vmpeg_fix_bitrate;
	vmpeg_seq_codes = asset->vmpeg_seq_codes;
	vmpeg_preset = asset->vmpeg_preset;
	vmpeg_field_order = asset->vmpeg_field_order;

	ac3_bitrate = asset->ac3_bitrate;

	png_use_alpha = asset->png_use_alpha;
	exr_use_alpha = asset->exr_use_alpha;
	exr_compression = asset->exr_compression;

	tiff_cmodel = asset->tiff_cmodel;
	tiff_compression = asset->tiff_compression;
	
	
	mov_sphere = asset->mov_sphere;
	jpeg_sphere = asset->jpeg_sphere;
}

int64_t Asset::get_index_offset(int channel)
{
	return index_state->get_index_offset(channel);
}

int64_t Asset::get_index_size(int channel)
{
	return index_state->get_index_size(channel);
}


char* Asset::get_compression_text(int audio, int video)
{
	if(audio) {
		switch(format) {
		case FILE_FFMPEG:
			if( acodec[0] ) return acodec;
			break;
		}
	}
	if(video) {
		switch(format) {
		case FILE_FFMPEG:
			if( vcodec[0] ) return vcodec;
			break;
		}
	}
	return 0;
}

int Asset::equivalent(Asset &asset, int test_audio, int test_video, EDL *edl)
{
	int result = format == asset.format ? 1 : 0;
	if( result && strcmp(asset.path, path) ) {
		char *out_path = edl ? FileSystem::basepath(edl->path) : 0;
		char *sp = out_path ? strrchr(out_path,'/') : 0;
		if( sp ) *++sp = 0;
		char *apath = FileSystem::basepath(asset.path);
		char *tpath = FileSystem::basepath(this->path);
		if( out_path ) {
			if( *apath != '/' ) {
				char *cp = cstrcat(2, out_path, apath);
				delete [] apath;  apath = FileSystem::basepath(cp);
				delete [] cp;
			}
			if( *tpath != '/' ) {
				char *cp = cstrcat(2, out_path, tpath);
				delete [] tpath;  tpath = FileSystem::basepath(cp);
				delete [] cp;
			}
		}
		if( strcmp(apath, tpath) ) result = 0;
		delete [] apath;
		delete [] tpath;
		delete [] out_path;
	}

	if(result && format == FILE_FFMPEG && strcmp(fformat, asset.fformat) )
		result = 0;

	if(test_audio && result)
	{
		result = (channels == asset.channels &&
			sample_rate == asset.sample_rate &&
			bits == asset.bits &&
			byte_order == asset.byte_order &&
			signed_ == asset.signed_ &&
			header == asset.header &&
			dither == asset.dither &&
			!strcmp(acodec, asset.acodec));
		if(result && format == FILE_FFMPEG)
			result = !strcmp(ff_audio_options, asset.ff_audio_options) &&
				!strcmp(ff_sample_format, asset.ff_sample_format) &&
				ff_audio_bitrate == asset.ff_audio_bitrate &&
				ff_audio_quality == asset.ff_audio_quality;
	}


	if(test_video && result)
	{
		result = (layers == asset.layers &&
			program == asset.program &&
			frame_rate == asset.frame_rate &&
			asset.interlace_autofixoption == interlace_autofixoption &&
			asset.interlace_mode    == interlace_mode &&
			interlace_fixmethod     == asset.interlace_fixmethod &&
			width == asset.width &&
			height == asset.height &&
			!strcmp(vcodec, asset.vcodec) &&
			mov_sphere == asset.mov_sphere &&
			jpeg_sphere == asset.jpeg_sphere);
		if(result && format == FILE_FFMPEG)
			result = !strcmp(ff_video_options, asset.ff_video_options) &&
				!strcmp(ff_pixel_format, asset.ff_pixel_format) &&
				ff_video_bitrate == asset.ff_video_bitrate &&
				ff_video_quality == asset.ff_video_quality;
	}

	return result;
}

int Asset::test_path(const char *path)
{
	if(!strcasecmp(this->path, path))
		return 1;
	else
		return 0;
}

int Asset::read(FileXML *file,
	int expand_relative)
{
	int result = 0;

// Check for relative path.
	if(expand_relative)
	{
		char new_path[BCTEXTLEN];
		char asset_directory[BCTEXTLEN];
		char input_directory[BCTEXTLEN];
		FileSystem fs;

		strcpy(new_path, path);
		fs.set_current_dir("");

		fs.extract_dir(asset_directory, path);

// No path in asset.
// Take path of XML file.
		if(!asset_directory[0])
		{
			fs.extract_dir(input_directory, file->filename);

// Input file has a path
			if(input_directory[0])
			{
				fs.join_names(path, input_directory, new_path);
			}
		}
	}


	while(!result)
	{
		result = file->read_tag();
		if(!result)
		{
			if(file->tag.title_is("/ASSET"))
			{
				result = 1;
			}
			else
			if(file->tag.title_is("AUDIO"))
			{
				read_audio(file);
			}
			else
			if(file->tag.title_is("AUDIO_OMIT"))
			{
				read_audio(file);
			}
			else
			if(file->tag.title_is("FORMAT"))
			{
				const char *string = file->tag.get_property("TYPE");
				format = File::strtoformat(string);
				use_header =
					file->tag.get_property("USE_HEADER", use_header);
				file->tag.get_property("FFORMAT", fformat);
			}
			else
			if(file->tag.title_is("FOLDER"))
			{
				folder_no = file->tag.get_property("NUMBER", AW_MEDIA_FOLDER);
			}
			else
			if(file->tag.title_is("VIDEO"))
			{
				read_video(file);
			}
			else
			if(file->tag.title_is("VIDEO_OMIT"))
			{
				read_video(file);
			}
			else
			if(file->tag.title_is("INDEX"))
			{
				read_index(file);
			}
		}
	}

	boundaries();
//printf("Asset::read 2\n");
	return 0;
}

int Asset::read_audio(FileXML *file)
{
	if(file->tag.title_is("AUDIO")) audio_data = 1;
	channels = file->tag.get_property("CHANNELS", 2);
// This is loaded from the index file after the EDL but this
// should be overridable in the EDL.
	if(!sample_rate) sample_rate = file->tag.get_property("RATE", 48000);
	bits = file->tag.get_property("BITS", 16);
	byte_order = file->tag.get_property("BYTE_ORDER", 1);
	signed_ = file->tag.get_property("SIGNED", 1);
	header = file->tag.get_property("HEADER", 0);
	dither = file->tag.get_property("DITHER", 0);

	audio_length = file->tag.get_property("AUDIO_LENGTH", (int64_t)0);
	acodec[0] = 0;
	file->tag.get_property("ACODEC", acodec);
	return 0;
}

int Asset::read_video(FileXML *file)
{
	char string[BCTEXTLEN];

	if(file->tag.title_is("VIDEO")) video_data = 1;
	actual_height = file->tag.get_property("ACTUAL_HEIGHT", actual_height);
	actual_width = file->tag.get_property("ACTUAL_WIDTH", actual_width);
	height = file->tag.get_property("HEIGHT", height);
	width = file->tag.get_property("WIDTH", width);
	layers = file->tag.get_property("LAYERS", layers);
	program = file->tag.get_property("PROGRAM", program);
// This is loaded from the index file after the EDL but this
// should be overridable in the EDL.
	if(EQUIV(frame_rate, 0)) frame_rate = file->tag.get_property("FRAMERATE", frame_rate);
	vcodec[0] = 0;
	file->tag.get_property("VCODEC", vcodec);

	video_length = file->tag.get_property("VIDEO_LENGTH", (int64_t)0);
	mov_sphere = file->tag.get_property("MOV_SPHERE", 0);
	jpeg_sphere = file->tag.get_property("JPEG_SPHERE", 0);
	single_frame = file->tag.get_property("SINGLE_FRAME", (int64_t)0);

	interlace_autofixoption = file->tag.get_property("INTERLACE_AUTOFIX",0);

	ilacemode_to_xmltext(string, ILACE_MODE_NOTINTERLACED);
	interlace_mode = ilacemode_from_xmltext(file->tag.get_property("INTERLACE_MODE",string), ILACE_MODE_NOTINTERLACED);

	ilacefixmethod_to_xmltext(string, ILACE_FIXMETHOD_NONE);
	interlace_fixmethod = ilacefixmethod_from_xmltext(file->tag.get_property("INTERLACE_FIXMETHOD",string), ILACE_FIXMETHOD_NONE);

	return 0;
}

int Asset::read_index(FileXML *file)
{
	index_state->read_xml(file, channels);
	return 0;
}

// Output path is the path of the output file if name truncation is desired.
// It is a "" if complete names should be used.

int Asset::write(FileXML *file,
	int include_index,
	const char *output_path)
{
	char new_path[BCTEXTLEN];
	char asset_directory[BCTEXTLEN];
	char output_directory[BCTEXTLEN];
	FileSystem fs;

// Make path relative
	fs.extract_dir(asset_directory, path);
	if(output_path && output_path[0])
		fs.extract_dir(output_directory, output_path);
	else
		output_directory[0] = 0;

// Asset and EDL are in same directory.  Extract just the name.
	if(!strcmp(asset_directory, output_directory))
	{
		fs.extract_name(new_path, path);
	}
	else
	{
		strcpy(new_path, path);
	}

	file->tag.set_title("ASSET");
	file->tag.set_property("SRC", new_path);
	file->append_tag();
	file->append_newline();

	file->tag.set_title("FOLDER");
	file->tag.set_property("NUMBER", folder_no);
	file->append_tag();
	file->tag.set_title("/FOLDER");
	file->append_tag();
	file->append_newline();

// Write the format information
	file->tag.set_title("FORMAT");

	file->tag.set_property("TYPE",
		File::formattostr(format));
	file->tag.set_property("USE_HEADER", use_header);
	file->tag.set_property("FFORMAT", fformat);

	file->append_tag();
	file->tag.set_title("/FORMAT");
	file->append_tag();
	file->append_newline();

// Requiring data to exist caused batch render to lose settings.
// But the only way to know if an asset doesn't have audio or video data
// is to not write the block.
// So change the block name if the asset doesn't have the data.
	write_audio(file);
	write_video(file);
// index goes after source
	if(index_state->index_status == INDEX_READY && include_index)
		write_index(file);

	file->tag.set_title("/ASSET");
	file->append_tag();
	file->append_newline();
	return 0;
}

int Asset::write_audio(FileXML *file)
{
// Let the reader know if the asset has the data by naming the block.
	if(audio_data)
		file->tag.set_title("AUDIO");
	else
		file->tag.set_title("AUDIO_OMIT");
// Necessary for PCM audio
	file->tag.set_property("CHANNELS", channels);
	file->tag.set_property("RATE", sample_rate);
	file->tag.set_property("BITS", bits);
	file->tag.set_property("BYTE_ORDER", byte_order);
	file->tag.set_property("SIGNED", signed_);
	file->tag.set_property("HEADER", header);
	file->tag.set_property("DITHER", dither);
	if(acodec[0])
		file->tag.set_property("ACODEC", acodec);

	file->tag.set_property("AUDIO_LENGTH", audio_length);



// Rely on defaults operations for these.

// 	file->tag.set_property("AMPEG_BITRATE", ampeg_bitrate);
// 	file->tag.set_property("AMPEG_DERIVATIVE", ampeg_derivative);
//
// 	file->tag.set_property("VORBIS_VBR", vorbis_vbr);
// 	file->tag.set_property("VORBIS_MIN_BITRATE", vorbis_min_bitrate);
// 	file->tag.set_property("VORBIS_BITRATE", vorbis_bitrate);
// 	file->tag.set_property("VORBIS_MAX_BITRATE", vorbis_max_bitrate);
//
// 	file->tag.set_property("MP3_BITRATE", mp3_bitrate);
//



	file->append_tag();
	if(audio_data)
	  file->tag.set_title("/AUDIO");
	else
          file->tag.set_title("/AUDIO_OMIT");
	file->append_tag();
	file->append_newline();
	return 0;
}

int Asset::write_video(FileXML *file)
{
	char string[BCTEXTLEN];

	if(video_data)
		file->tag.set_title("VIDEO");
	else
		file->tag.set_title("VIDEO_OMIT");
	file->tag.set_property("ACTUAL_HEIGHT", actual_height);
	file->tag.set_property("ACTUAL_WIDTH", actual_width);
	file->tag.set_property("HEIGHT", height);
	file->tag.set_property("WIDTH", width);
	file->tag.set_property("LAYERS", layers);
	file->tag.set_property("PROGRAM", program);
	file->tag.set_property("FRAMERATE", frame_rate);
	if(vcodec[0])
		file->tag.set_property("VCODEC", vcodec);

	file->tag.set_property("VIDEO_LENGTH", video_length);
	file->tag.set_property("MOV_SPHERE", mov_sphere);
	file->tag.set_property("JPEG_SPHERE", jpeg_sphere);
	file->tag.set_property("SINGLE_FRAME", single_frame);

	file->tag.set_property("INTERLACE_AUTOFIX", interlace_autofixoption);

	ilacemode_to_xmltext(string, interlace_mode);
	file->tag.set_property("INTERLACE_MODE", string);

	ilacefixmethod_to_xmltext(string, interlace_fixmethod);
	file->tag.set_property("INTERLACE_FIXMETHOD", string);

	file->append_tag();
	if(video_data)
		file->tag.set_title("/VIDEO");
	else
		file->tag.set_title("/VIDEO_OMIT");

	file->append_tag();
	file->append_newline();
	return 0;
}

int Asset::write_index(FileXML *file)
{
	index_state->write_xml(file);
	return 0;
}


char* Asset::construct_param(const char *param, const char *prefix, char *return_value)
{
	if(prefix)
		sprintf(return_value, "%s%s", prefix, param);
	else
		strcpy(return_value, param);
	return return_value;
}

#define UPDATE_DEFAULT(x, y) defaults->update(construct_param(x, prefix, string), y);
#define GET_DEFAULT(x, y) defaults->get(construct_param(x, prefix, string), y);

void Asset::load_defaults(BC_Hash *defaults,
	const char *prefix,
	int do_format,
	int do_compression,
	int do_path,
	int do_data_types,
	int do_bits)
{
	char string[BCTEXTLEN];

// Can't save codec here because it's specific to render, record, and effect.
// The codec has to be UNKNOWN for file probing to work.

	if(do_path)
	{
		GET_DEFAULT("PATH", path);
	}

	if(do_compression)
	{
		GET_DEFAULT("AUDIO_CODEC", acodec);
		GET_DEFAULT("VIDEO_CODEC", vcodec);
	}

	if(do_format)
	{
		format = GET_DEFAULT("FORMAT", format);
		use_header = GET_DEFAULT("USE_HEADER", use_header);
		GET_DEFAULT("FFORMAT", fformat);
	}

	if(do_data_types)
	{
		audio_data = GET_DEFAULT("AUDIO", 1);
		video_data = GET_DEFAULT("VIDEO", 1);
	}

	if(do_bits)
	{
		bits = GET_DEFAULT("BITS", 16);
		dither = GET_DEFAULT("DITHER", 0);
		signed_ = GET_DEFAULT("SIGNED", 1);
		byte_order = GET_DEFAULT("BYTE_ORDER", 1);



		channels = GET_DEFAULT("CHANNELS", 2);
		if(!sample_rate) sample_rate = GET_DEFAULT("RATE", 48000);
		header = GET_DEFAULT("HEADER", 0);
		audio_length = GET_DEFAULT("AUDIO_LENGTH", (int64_t)0);



		height = GET_DEFAULT("HEIGHT", height);
		width = GET_DEFAULT("WIDTH", width);
		actual_height = GET_DEFAULT("ACTUAL_HEIGHT", actual_height);
		actual_width = GET_DEFAULT("ACTUAL_WIDTH", actual_width);
		program = GET_DEFAULT("PROGRAM", program);
		layers = GET_DEFAULT("LAYERS", layers);
		if(EQUIV(frame_rate, 0)) frame_rate = GET_DEFAULT("FRAMERATE", frame_rate);
		video_length = GET_DEFAULT("VIDEO_LENGTH", (int64_t)0);
		single_frame = GET_DEFAULT("SINGLE_FRAME", (int64_t)0);
	}

	ampeg_bitrate = GET_DEFAULT("AMPEG_BITRATE", ampeg_bitrate);
	ampeg_derivative = GET_DEFAULT("AMPEG_DERIVATIVE", ampeg_derivative);

	vorbis_vbr = GET_DEFAULT("VORBIS_VBR", vorbis_vbr);
	vorbis_min_bitrate = GET_DEFAULT("VORBIS_MIN_BITRATE", vorbis_min_bitrate);
	vorbis_bitrate = GET_DEFAULT("VORBIS_BITRATE", vorbis_bitrate);
	vorbis_max_bitrate = GET_DEFAULT("VORBIS_MAX_BITRATE", vorbis_max_bitrate);

	theora_fix_bitrate = GET_DEFAULT("THEORA_FIX_BITRATE", theora_fix_bitrate);
	theora_bitrate = GET_DEFAULT("THEORA_BITRATE", theora_bitrate);
	theora_quality = GET_DEFAULT("THEORA_QUALITY", theora_quality);
	theora_sharpness = GET_DEFAULT("THEORA_SHARPNESS", theora_sharpness);
	theora_keyframe_frequency = GET_DEFAULT("THEORA_KEYFRAME_FREQUENCY", theora_keyframe_frequency);
	theora_keyframe_force_frequency = GET_DEFAULT("THEORA_FORCE_KEYFRAME_FREQUENCY", theora_keyframe_force_frequency);

	GET_DEFAULT("FF_AUDIO_OPTIONS", ff_audio_options);
	GET_DEFAULT("FF_SAMPLE_FORMAT", ff_sample_format);
	ff_audio_bitrate = GET_DEFAULT("FF_AUDIO_BITRATE", ff_audio_bitrate);
	ff_audio_quality = GET_DEFAULT("FF_AUDIO_QUALITY", ff_audio_quality);
	GET_DEFAULT("FF_VIDEO_OPTIONS", ff_video_options);
	GET_DEFAULT("FF_PIXEL_FORMAT", ff_pixel_format);
	ff_video_bitrate = GET_DEFAULT("FF_VIDEO_BITRATE", ff_video_bitrate);
	ff_video_quality = GET_DEFAULT("FF_VIDEO_QUALITY", ff_video_quality);

	mp3_bitrate = GET_DEFAULT("MP3_BITRATE", mp3_bitrate);

	jpeg_quality = GET_DEFAULT("JPEG_QUALITY", jpeg_quality);
	aspect_ratio = GET_DEFAULT("ASPECT_RATIO", aspect_ratio);

	interlace_autofixoption	= ILACE_AUTOFIXOPTION_AUTO;
	interlace_mode         	= ILACE_MODE_UNDETECTED;
	interlace_fixmethod    	= ILACE_FIXMETHOD_UPONE;

// MPEG format information
	vmpeg_iframe_distance = GET_DEFAULT("VMPEG_IFRAME_DISTANCE", vmpeg_iframe_distance);
	vmpeg_pframe_distance = GET_DEFAULT("VMPEG_PFRAME_DISTANCE", vmpeg_pframe_distance);
	vmpeg_progressive = GET_DEFAULT("VMPEG_PROGRESSIVE", vmpeg_progressive);
	vmpeg_denoise = GET_DEFAULT("VMPEG_DENOISE", vmpeg_denoise);
	vmpeg_bitrate = GET_DEFAULT("VMPEG_BITRATE", vmpeg_bitrate);
	vmpeg_derivative = GET_DEFAULT("VMPEG_DERIVATIVE", vmpeg_derivative);
	vmpeg_quantization = GET_DEFAULT("VMPEG_QUANTIZATION", vmpeg_quantization);
	vmpeg_cmodel = GET_DEFAULT("VMPEG_CMODEL", vmpeg_cmodel);
	vmpeg_fix_bitrate = GET_DEFAULT("VMPEG_FIX_BITRATE", vmpeg_fix_bitrate);
	vmpeg_seq_codes = GET_DEFAULT("VMPEG_SEQ_CODES", vmpeg_seq_codes);
	vmpeg_preset = GET_DEFAULT("VMPEG_PRESET", vmpeg_preset);
	vmpeg_field_order = GET_DEFAULT("VMPEG_FIELD_ORDER", vmpeg_field_order);

	theora_fix_bitrate = GET_DEFAULT("THEORA_FIX_BITRATE", theora_fix_bitrate);
	theora_bitrate = GET_DEFAULT("THEORA_BITRATE", theora_bitrate);
	theora_quality = GET_DEFAULT("THEORA_QUALITY", theora_quality);
	theora_sharpness = GET_DEFAULT("THEORA_SHARPNESS", theora_sharpness);
	theora_keyframe_frequency = GET_DEFAULT("THEORA_KEYFRAME_FREQUENCY", theora_keyframe_frequency);
	theora_keyframe_force_frequency = GET_DEFAULT("THEORA_FORCE_KEYFRAME_FEQUENCY", theora_keyframe_force_frequency);


	ac3_bitrate = GET_DEFAULT("AC3_BITRATE", ac3_bitrate);

	png_use_alpha = GET_DEFAULT("PNG_USE_ALPHA", png_use_alpha);
	exr_use_alpha = GET_DEFAULT("EXR_USE_ALPHA", exr_use_alpha);
	exr_compression = GET_DEFAULT("EXR_COMPRESSION", exr_compression);
	tiff_cmodel = GET_DEFAULT("TIFF_CMODEL", tiff_cmodel);
	tiff_compression = GET_DEFAULT("TIFF_COMPRESSION", tiff_compression);

	mov_sphere = GET_DEFAULT("MOV_SPHERE", mov_sphere);
	jpeg_sphere = GET_DEFAULT("JPEG_SPHERE", jpeg_sphere);
	boundaries();
}

void Asset::save_defaults(BC_Hash *defaults,
	const char *prefix,
	int do_format,
	int do_compression,
	int do_path,
	int do_data_types,
	int do_bits)
{
	char string[BCTEXTLEN];

	UPDATE_DEFAULT("PATH", path);




	if(do_format)
	{
		UPDATE_DEFAULT("FORMAT", format);
		UPDATE_DEFAULT("USE_HEADER", use_header);
		UPDATE_DEFAULT("FFORMAT", fformat);
	}

	if(do_data_types)
	{
		UPDATE_DEFAULT("AUDIO", audio_data);
		UPDATE_DEFAULT("VIDEO", video_data);
	}

	if(do_compression)
	{
		UPDATE_DEFAULT("AUDIO_CODEC", acodec);
		UPDATE_DEFAULT("VIDEO_CODEC", vcodec);

		UPDATE_DEFAULT("AMPEG_BITRATE", ampeg_bitrate);
		UPDATE_DEFAULT("AMPEG_DERIVATIVE", ampeg_derivative);

		UPDATE_DEFAULT("VORBIS_VBR", vorbis_vbr);
		UPDATE_DEFAULT("VORBIS_MIN_BITRATE", vorbis_min_bitrate);
		UPDATE_DEFAULT("VORBIS_BITRATE", vorbis_bitrate);
		UPDATE_DEFAULT("VORBIS_MAX_BITRATE", vorbis_max_bitrate);

		UPDATE_DEFAULT("FF_AUDIO_OPTIONS", ff_audio_options);
		UPDATE_DEFAULT("FF_SAMPLE_FORMAT", ff_sample_format);
		UPDATE_DEFAULT("FF_AUDIO_BITRATE", ff_audio_bitrate);
		UPDATE_DEFAULT("FF_AUDIO_QUALITY", ff_audio_quality);
		UPDATE_DEFAULT("FF_VIDEO_OPTIONS", ff_video_options);
		UPDATE_DEFAULT("FF_PIXEL_FORMAT",  ff_pixel_format);
		UPDATE_DEFAULT("FF_VIDEO_BITRATE", ff_video_bitrate);
		UPDATE_DEFAULT("FF_VIDEO_QUALITY", ff_video_quality);

		UPDATE_DEFAULT("THEORA_FIX_BITRATE", theora_fix_bitrate);
		UPDATE_DEFAULT("THEORA_BITRATE", theora_bitrate);
		UPDATE_DEFAULT("THEORA_QUALITY", theora_quality);
		UPDATE_DEFAULT("THEORA_SHARPNESS", theora_sharpness);
		UPDATE_DEFAULT("THEORA_KEYFRAME_FREQUENCY", theora_keyframe_frequency);
		UPDATE_DEFAULT("THEORA_FORCE_KEYFRAME_FREQUENCY", theora_keyframe_force_frequency);



		UPDATE_DEFAULT("MP3_BITRATE", mp3_bitrate);

		UPDATE_DEFAULT("JPEG_QUALITY", jpeg_quality);
		UPDATE_DEFAULT("ASPECT_RATIO", aspect_ratio);

// MPEG format information
		UPDATE_DEFAULT("VMPEG_IFRAME_DISTANCE", vmpeg_iframe_distance);
		UPDATE_DEFAULT("VMPEG_PFRAME_DISTANCE", vmpeg_pframe_distance);
		UPDATE_DEFAULT("VMPEG_PROGRESSIVE", vmpeg_progressive);
		UPDATE_DEFAULT("VMPEG_DENOISE", vmpeg_denoise);
		UPDATE_DEFAULT("VMPEG_BITRATE", vmpeg_bitrate);
		UPDATE_DEFAULT("VMPEG_DERIVATIVE", vmpeg_derivative);
		UPDATE_DEFAULT("VMPEG_QUANTIZATION", vmpeg_quantization);
		UPDATE_DEFAULT("VMPEG_CMODEL", vmpeg_cmodel);
		UPDATE_DEFAULT("VMPEG_FIX_BITRATE", vmpeg_fix_bitrate);
		UPDATE_DEFAULT("VMPEG_SEQ_CODES", vmpeg_seq_codes);
		UPDATE_DEFAULT("VMPEG_PRESET", vmpeg_preset);
		UPDATE_DEFAULT("VMPEG_FIELD_ORDER", vmpeg_field_order);


		UPDATE_DEFAULT("AC3_BITRATE", ac3_bitrate);


		UPDATE_DEFAULT("PNG_USE_ALPHA", png_use_alpha);
		UPDATE_DEFAULT("EXR_USE_ALPHA", exr_use_alpha);
		UPDATE_DEFAULT("EXR_COMPRESSION", exr_compression);
		UPDATE_DEFAULT("TIFF_CMODEL", tiff_cmodel);
		UPDATE_DEFAULT("TIFF_COMPRESSION", tiff_compression);



		UPDATE_DEFAULT("MOV_SPHERE", mov_sphere);
		UPDATE_DEFAULT("JPEG_SPHERE", jpeg_sphere);
	}




	if(do_bits)
	{
		UPDATE_DEFAULT("BITS", bits);
		UPDATE_DEFAULT("DITHER", dither);
		UPDATE_DEFAULT("SIGNED", signed_);
		UPDATE_DEFAULT("BYTE_ORDER", byte_order);






		UPDATE_DEFAULT("CHANNELS", channels);
		UPDATE_DEFAULT("RATE", sample_rate);
		UPDATE_DEFAULT("HEADER", header);
		UPDATE_DEFAULT("AUDIO_LENGTH", audio_length);



		UPDATE_DEFAULT("HEIGHT", height);
		UPDATE_DEFAULT("WIDTH", width);
		UPDATE_DEFAULT("ACTUAL_HEIGHT", actual_height);
		UPDATE_DEFAULT("ACTUAL_WIDTH", actual_width);
		UPDATE_DEFAULT("PROGRAM", program);
		UPDATE_DEFAULT("LAYERS", layers);
		UPDATE_DEFAULT("FRAMERATE", frame_rate);
		UPDATE_DEFAULT("VIDEO_LENGTH", video_length);
		UPDATE_DEFAULT("SINGLE_FRAME", single_frame);

	}
}









int Asset::dump(FILE *fp)
{
	fprintf(fp,"  asset::dump\n");
	fprintf(fp,"   this=%p path=%s\n", this, path);
	fprintf(fp,"   index_status %d\n", index_state->index_status);
	fprintf(fp,"   format %d\n", format);
	fprintf(fp,"   fformat=\"%s\"\n", fformat);
	fprintf(fp,"   ff_audio_options=\"%s\"\n", ff_audio_options);
	fprintf(fp,"   ff_sample_format=\"%s\"\n", ff_sample_format);
	fprintf(fp,"   ff_audio_bitrate=%d\n", ff_audio_bitrate);
	fprintf(fp,"   ff_audio_quality=%d\n", ff_audio_quality);
	fprintf(fp,"   ff_video_options=\"%s\"\n", ff_video_options);
	fprintf(fp,"   ff_pixel_format=\"%s\"\n", ff_pixel_format);
	fprintf(fp,"   ff_video_bitrate=%d\n", ff_video_bitrate);
	fprintf(fp,"   ff_video_quality=%d\n", ff_video_quality);
	fprintf(fp,"   audio_data %d channels %d samplerate %d bits %d"
		" byte_order %d signed %d header %d dither %d acodec %4.4s\n",
		audio_data, channels, sample_rate, bits, byte_order, signed_,
		header, dither, acodec);
	fprintf(fp,"   audio_length %jd\n", audio_length);
	char string[BCTEXTLEN];
	ilacemode_to_xmltext(string, interlace_mode);
	fprintf(fp,"   video_data %d program %d layers %d framerate %f width %d"
		" height %d vcodec %4.4s aspect_ratio %f ilace_mode %s\n",
		video_data, layers, program, frame_rate, width, height,
		vcodec, aspect_ratio,string);
	fprintf(fp,"   video_length %jd repeat %d\n", video_length, single_frame);
	printf("   mov_sphere=%d jpeg_sphere=%d\n", mov_sphere, jpeg_sphere);
	return 0;
}


// For Indexable
int Asset::get_audio_channels()
{
	return channels;
}

int Asset::get_sample_rate()
{
	return sample_rate;
}

int64_t Asset::get_audio_samples()
{
	return audio_length;
}

int Asset::have_audio()
{
	return audio_data;
}

int Asset::have_video()
{
	return video_data;
}

int Asset::get_w()
{
	return width;
}

int Asset::get_h()
{
	return height;
}

double Asset::get_frame_rate()
{
	return frame_rate;
}


int Asset::get_video_layers()
{
	return layers;
}

int Asset::get_program()
{
	return program;
}

int64_t Asset::get_video_frames()
{
	return video_length;
}

double Asset::total_length_framealigned(double fps)
{
	if (video_data && audio_data) {
		double aud = floor(( (double)audio_length / sample_rate) * fps) / fps;
		double vid = floor(( (double)video_length / frame_rate) * fps) / fps;
		return MIN(aud,vid);
	}

	if (audio_data)
		return (double)audio_length / sample_rate;

	if (video_data)
		return (double)video_length / frame_rate;

	return 0;
}

