#ifdef HAVE_CIN_3RDPARTY
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
// work around (centos) for __STDC_CONSTANT_MACROS
#include <lzma.h>
#ifndef INT64_MAX
#   define INT64_MAX 9223372036854775807LL
#endif

extern "C"
{
#include "libavcodec/avcodec.h"
}

#include "asset.h"
#include "clip.h"
#include "fileac3.h"
#include "file.h"
#include "filempeg.h"
#include "language.h"
#include "mainerror.h"



#include <string.h>

FileAC3::FileAC3(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset_parameters();
	mpg_file = 0;
}

FileAC3::~FileAC3()
{
	if( mpg_file ) delete mpg_file;
	close_file();
}

int FileAC3::reset_parameters_derived()
{
	codec = 0;
	codec_context = 0;
	resample_context = 0;
	fd = 0;
	temp_raw = 0;
	temp_raw_size = 0;
	temp_raw_allocated = 0;
	return 0;
}

void FileAC3::get_parameters(BC_WindowBase *parent_window,
		Asset *asset, BC_WindowBase* &format_window,
		int audio_options, int video_options, EDL *edl)
{
	if(audio_options)
	{

		AC3ConfigAudio *window = new AC3ConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

int FileAC3::check_sig()
{
// Libmpeg3 does this in FileMPEG.
	return 0;
}

int64_t FileAC3::get_channel_layout(int channels)
{
	switch( channels ) {
	case 1: return AV_CH_LAYOUT_MONO;
	case 2: return AV_CH_LAYOUT_STEREO;
	case 3: return AV_CH_LAYOUT_SURROUND;
	case 4: return AV_CH_LAYOUT_QUAD;
	case 5: return AV_CH_LAYOUT_5POINT0;
	case 6: return AV_CH_LAYOUT_5POINT1;
	case 8: return AV_CH_LAYOUT_7POINT1;
	}
	return 0;
}

int FileAC3::open_file(int rd, int wr)
{
	int result = 0;

	if(rd)
	{
		if( !mpg_file )
			mpg_file = new FileMPEG(file->asset, file);
		result = mpg_file->open_file(1, 0);
		if( result ) {
			eprintf(_("Error while opening \"%s\" for reading. \n%m\n"), asset->path);
		}
	}

	if( !result && wr )
	{
		codec = avcodec_find_encoder(AV_CODEC_ID_AC3);
		if(!codec)
		{
			eprintf(_("FileAC3::open_file codec not found.\n"));
			result = 1;
		}
		if( !result && !(fd = fopen(asset->path, "w")))
		{
			eprintf(_("Error while opening \"%s\" for writing. \n%m\n"), asset->path);
			result = 1;
		}
		if( !result ) {
			int channels = asset->channels;
			int sample_rate = asset->sample_rate;
			int64_t layout = get_channel_layout(channels);
			int bitrate = asset->ac3_bitrate * 1000;
			av_init_packet(&avpkt);
			codec_context = avcodec_alloc_context3(codec);
			codec_context->bit_rate = bitrate;
			codec_context->sample_rate = sample_rate;
			codec_context->channels = channels;
			codec_context->channel_layout = layout;
			codec_context->sample_fmt = codec->sample_fmts[0];
			resample_context = swr_alloc_set_opts(NULL,
					layout, codec_context->sample_fmt, sample_rate,
					layout, AV_SAMPLE_FMT_S16, sample_rate,
					0, NULL);
			swr_init(resample_context);
			if(avcodec_open2(codec_context, codec, 0))
			{
				eprintf(_("FileAC3::open_file failed to open codec.\n"));
				result = 1;
			}
		}
	}

	return result;
}

int FileAC3::close_file()
{
	if( mpg_file )
	{
		delete mpg_file;
		mpg_file = 0;
	}
	if(codec_context)
	{
		encode_flush();
		avcodec_close(codec_context);
		avcodec_free_context(&codec_context);
		codec = 0;
	}
	if( resample_context )
		swr_free(&resample_context);
	if(fd)
	{
		fclose(fd);
		fd = 0;
	}
	if(temp_raw)
	{
		delete [] temp_raw;
		temp_raw = 0;
	}
	reset_parameters();
	FileBase::close_file();
	return 0;
}

int FileAC3::read_samples(double *buffer, int64_t len)
{
	return !mpg_file ? 0 : mpg_file->read_samples(buffer, len);
}

int FileAC3::get_index(IndexFile *index_file, MainProgressBar *progress_bar)
{
	return !mpg_file ? 1 : mpg_file->get_index(index_file, progress_bar);
}

// Channel conversion matrices because ffmpeg encodes a
// different channel order than liba52 decodes.
// Each row is an output channel.
// Each column is an input channel.
// static int channels5[] =
// {
// 	{ }
// };
//
// static int channels6[] =
// {
// 	{ }
// };

int FileAC3::write_packet()
{
	AVCodecContext *&avctx = codec_context;
	int ret = avcodec_receive_packet(avctx, &avpkt);
	if( ret >= 0 ) {
		ret = 0;
		if( avpkt.data && avpkt.size > 0 ) {
			int sz = fwrite(avpkt.data, 1, avpkt.size, fd);
			if( sz == avpkt.size ) ret = 1;
		}
		if( !ret )
			eprintf(_("Error while writing samples. \n%m\n"));
		av_packet_unref(&avpkt);
	}
	else if( ret == AVERROR_EOF )
		ret = 0;
	return ret;
}

int FileAC3::encode_frame(AVFrame *frame)
{
	AVCodecContext *&avctx = codec_context;
	int ret = 0, pkts = 0;
	for( int retry=100; --retry>=0; ) {
		ret = avcodec_send_frame(avctx, frame);
		if( ret >= 0 ) return pkts;
		if( ret != AVERROR(EAGAIN) ) break;
		if( (ret=write_packet()) < 0 ) break;
		if( !ret ) return pkts;
		++pkts;
	}
	if( ret < 0 ) {
		char errmsg[BCTEXTLEN];
		av_strerror(ret, errmsg, sizeof(errmsg));
		fprintf(stderr, "FileAC3::encode_frame: encode failed: %s\n", errmsg);
	}
	return ret;
}

int FileAC3::encode_flush()
{
	AVCodecContext *&avctx = codec_context;
	int ret = avcodec_send_frame(avctx, 0);
	while( (ret=write_packet()) > 0 );
	if( ret < 0 ) {
		char errmsg[BCTEXTLEN];
		av_strerror(ret, errmsg, sizeof(errmsg));
		fprintf(stderr, "FileAC3::encode_flush: encode failed: %s\n", errmsg);
	}
	return ret;
}

int FileAC3::write_samples(double **buffer, int64_t len)
{
// Convert buffer to encoder format
	if(temp_raw_size + len > temp_raw_allocated)
	{
		int new_allocated = temp_raw_size + len;
		int16_t *new_raw = new int16_t[new_allocated * asset->channels];
		if(temp_raw)
		{
			memcpy(new_raw,
				temp_raw,
				sizeof(int16_t) * temp_raw_size * asset->channels);
			delete [] temp_raw;
		}
		temp_raw = new_raw;
		temp_raw_allocated = new_allocated;
	}

// Append buffer to temp raw
	int16_t *out_ptr = temp_raw + temp_raw_size * asset->channels;
	for(int i = 0; i < len; i++)
	{
		for(int j = 0; j < asset->channels; j++)
		{
			int sample = (int)(buffer[j][i] * 32767);
			CLAMP(sample, -32768, 32767);
			*out_ptr++ = sample;
		}
	}
	temp_raw_size += len;

	AVCodecContext *&avctx = codec_context;
	int frame_size = avctx->frame_size;
	int cur_sample = 0, ret = 0;
	for(cur_sample = 0; ret >= 0 &&
		cur_sample + frame_size <= temp_raw_size;
		cur_sample += frame_size)
	{
		AVFrame *frame = av_frame_alloc();
		frame->nb_samples = frame_size;
		frame->format = avctx->sample_fmt;
		frame->channel_layout = avctx->channel_layout;
		frame->sample_rate = avctx->sample_rate;
		ret = av_frame_get_buffer(frame, 0);
		if( ret >= 0 ) {
			const uint8_t *samples = (uint8_t *)temp_raw +
				cur_sample * sizeof(int16_t) * asset->channels;
			ret = swr_convert(resample_context,
				(uint8_t **)frame->extended_data, frame_size,
				&samples, frame_size);
		}
		if( ret >= 0 ) {
			frame->pts = avctx->sample_rate && avctx->time_base.num ?
				file->get_audio_position() : AV_NOPTS_VALUE ;
			ret = encode_frame(frame);
		}
		av_frame_free(&frame);
	}

// Shift buffer back
	memcpy(temp_raw,
		temp_raw + cur_sample * asset->channels,
		(temp_raw_size - cur_sample) * sizeof(int16_t) * asset->channels);
	temp_raw_size -= cur_sample;

	return 0;
}







AC3ConfigAudio::AC3ConfigAudio(BC_WindowBase *parent_window,
	Asset *asset)
 : BC_Window(_(PROGRAM_NAME ": Audio Compression"),
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	500,
	BC_OKButton::calculate_h() + 100,
	500,
	BC_OKButton::calculate_h() + 100,
	0,
	0,
	1)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

void AC3ConfigAudio::create_objects()
{
	int x = 10, y = 10;
	int x1 = 150;
	lock_window("AC3ConfigAudio::create_objects");
	add_tool(new BC_Title(x, y, _("Bitrate (kbps):")));
	AC3ConfigAudioBitrate *bitrate;
	add_tool(bitrate =
		new AC3ConfigAudioBitrate(this,
			x1,
			y));
	bitrate->create_objects();

	add_subwindow(new BC_OKButton(this));
	show_window(1);
	unlock_window();
}

int AC3ConfigAudio::close_event()
{
	set_done(0);
	return 1;
}






AC3ConfigAudioBitrate::AC3ConfigAudioBitrate(AC3ConfigAudio *gui,
	int x,
	int y)
 : BC_PopupMenu(x,
 	y,
	150,
	AC3ConfigAudioBitrate::bitrate_to_string(gui->string, gui->asset->ac3_bitrate))
{
	this->gui = gui;
}

char* AC3ConfigAudioBitrate::bitrate_to_string(char *string, int bitrate)
{
	sprintf(string, "%d", bitrate);
	return string;
}

void AC3ConfigAudioBitrate::create_objects()
{
	add_item(new BC_MenuItem("32"));
	add_item(new BC_MenuItem("40"));
	add_item(new BC_MenuItem("48"));
	add_item(new BC_MenuItem("56"));
	add_item(new BC_MenuItem("64"));
	add_item(new BC_MenuItem("80"));
	add_item(new BC_MenuItem("96"));
	add_item(new BC_MenuItem("112"));
	add_item(new BC_MenuItem("128"));
	add_item(new BC_MenuItem("160"));
	add_item(new BC_MenuItem("192"));
	add_item(new BC_MenuItem("224"));
	add_item(new BC_MenuItem("256"));
	add_item(new BC_MenuItem("320"));
	add_item(new BC_MenuItem("384"));
	add_item(new BC_MenuItem("448"));
	add_item(new BC_MenuItem("512"));
	add_item(new BC_MenuItem("576"));
	add_item(new BC_MenuItem("640"));
}

int AC3ConfigAudioBitrate::handle_event()
{
	gui->asset->ac3_bitrate = atol(get_text());
	return 1;
}

#endif
