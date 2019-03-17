
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
// work around for __STDC_CONSTANT_MACROS
#include <lzma.h>

#include "asset.h"
#include "bcwindowbase.h"
#include "bitspopup.h"
#include "ctype.h"
#include "edl.h"
#include "ffmpeg.h"
#include "filebase.h"
#include "file.h"
#include "fileffmpeg.h"
#include "filesystem.h"
#include "indexfile.h"
#include "language.h"
#include "mainerror.h"
#include "mainprogress.h"
#include "mutex.h"
#include "preferences.h"
#include "videodevice.inc"

#ifdef FFMPEG3
#define url filename
#endif

FileFFMPEG::FileFFMPEG(Asset *asset, File *file)
 : FileBase(asset, file)
{
	ff = 0;
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_FFMPEG;
}

FileFFMPEG::~FileFFMPEG()
{
	delete ff;
}


FFMpegConfigNum::FFMpegConfigNum(BC_Window *window,
		int x, int y, char *title_text, int *output)
 : BC_TumbleTextBox(window, *output, -1, INT_MAX, 100, y, 100)
{
	this->window = window;
	this->x = x;  this->y = y;
	this->title_text = title_text;
	this->output = output;
}

FFMpegConfigNum::~FFMpegConfigNum()
{
}

void FFMpegConfigNum::create_objects()
{
	window->add_subwindow(title = new BC_Title(x, y, title_text));
	BC_TumbleTextBox::create_objects();
}

int FFMpegConfigNum::update_param(const char *param, const char *opts)
{
	char value[BCSTRLEN];
	if( !FFMPEG::get_ff_option(param, opts, value) ) {
		if( (*output = atoi(value)) < 0 ) {
			disable(1);
			return 0;
		}
	}
	BC_TumbleTextBox::update((int64_t)*output);
	enable();
	return 1;
}

int FFMpegConfigNum::handle_event()
{
	*output = atoi(get_text());
	return 1;
}

FFMpegAudioNum::FFMpegAudioNum(BC_Window *window,
		int x, int y, char *title_text, int *output)
 : FFMpegConfigNum(window, x, y, title_text, output)
{
}

int FFMpegAudioBitrate::handle_event()
{
	int ret = FFMpegAudioNum::handle_event();
	Asset *asset = window()->asset;
	if( asset->ff_audio_bitrate > 0 )
		window()->quality->disable();
	else if( !window()->quality->get_textbox()->is_hidden() )
		window()->quality->enable();
	return ret;
}

int FFMpegAudioQuality::handle_event()
{
	int ret = FFMpegAudioNum::handle_event();
	Asset *asset = window()->asset;
	if( asset->ff_audio_quality >= 0 )
		window()->bitrate->disable();
	else if( !window()->bitrate->get_textbox()->is_hidden() )
		window()->bitrate->enable();
	return ret;
}

FFMpegVideoNum::FFMpegVideoNum(BC_Window *window,
		int x, int y, char *title_text, int *output)
 : FFMpegConfigNum(window, x, y, title_text, output)
{
}

int FFMpegVideoBitrate::handle_event()
{
	int ret = FFMpegVideoNum::handle_event();
	Asset *asset = window()->asset;
	if( asset->ff_video_bitrate > 0 )
		window()->quality->disable();
	else if( !window()->quality->get_textbox()->is_hidden() )
		window()->quality->enable();
	return ret;
}

int FFMpegVideoQuality::handle_event()
{
	int ret = FFMpegVideoNum::handle_event();
	Asset *asset = window()->asset;
	if( asset->ff_video_quality >= 0 )
		window()->bitrate->disable();
	else if( !window()->bitrate->get_textbox()->is_hidden() )
		window()->bitrate->enable();
	return ret;
}

FFMpegPixelFormat::FFMpegPixelFormat(FFMPEGConfigVideo *vid_config,
	int x, int y, int w, int list_h)
 : BC_PopupTextBox(vid_config, 0, 0, x, y, w, list_h)
{
	this->vid_config = vid_config;
}

int FFMpegPixelFormat::handle_event()
{
	strncpy(vid_config->asset->ff_pixel_format, get_text(),
		sizeof(vid_config->asset->ff_pixel_format));
	return 1;
}

void FFMpegPixelFormat::update_formats()
{
	pixfmts.remove_all_objects();
	char video_codec[BCSTRLEN]; video_codec[0] = 0;
	const char *vcodec = vid_config->asset->vcodec;
	AVCodec *av_codec = !FFMPEG::get_codec(video_codec, "video", vcodec) ?
		avcodec_find_encoder_by_name(video_codec) : 0;
	const AVPixelFormat *pix_fmts = av_codec ? av_codec->pix_fmts : 0;
	if( pix_fmts ) {
		for( int i=0; pix_fmts[i]>=0; ++i ) {
			const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmts[i]);
			if( desc ) pixfmts.append(new BC_ListBoxItem(desc->name));
		}
	}
	update_list(&pixfmts);
}

FFMpegSampleFormat::FFMpegSampleFormat(FFMPEGConfigAudio *aud_config,
	int x, int y, int w, int list_h)
 : BC_PopupTextBox(aud_config, 0, 0, x, y, w, list_h)
{
	this->aud_config = aud_config;
}

int FFMpegSampleFormat::handle_event()
{
	strncpy(aud_config->asset->ff_sample_format, get_text(),
		sizeof(aud_config->asset->ff_sample_format));
	return 1;
}

void FFMpegSampleFormat::update_formats()
{
	samplefmts.remove_all_objects();
	char audio_codec[BCSTRLEN]; audio_codec[0] = 0;
	const char *acodec = aud_config->asset->acodec;
	AVCodec *av_codec = !FFMPEG::get_codec(audio_codec, "audio", acodec) ?
		avcodec_find_encoder_by_name(audio_codec) : 0;
	const AVSampleFormat *sample_fmts = av_codec ? av_codec->sample_fmts : 0;
	if( sample_fmts ) {
		for( int i=0; sample_fmts[i]>=0; ++i ) {
			const char *name = av_get_sample_fmt_name(sample_fmts[i]);
			if( name ) samplefmts.append(new BC_ListBoxItem(name));
		}
	}
	update_list(&samplefmts);
}

void FileFFMPEG::set_options(char *cp, int len, const char *bp)
{
	char *ep = cp + len-2, ch = 0;
	while( cp < ep && *bp != 0 ) { ch = *bp++; *cp++ = ch; }
	if( ch != '\n' ) *cp++ = '\n';
	*cp = 0;
}

void FileFFMPEG::get_parameters(BC_WindowBase *parent_window,
		Asset *asset, BC_WindowBase *&format_window,
		int audio_options, int video_options, EDL *edl)
{
	Asset *ff_asset = new Asset();
	ff_asset->copy_from(asset, 0);
	if( audio_options ) {
		FFMPEGConfigAudio *window = new FFMPEGConfigAudio(parent_window, ff_asset, edl);
		format_window = window;
		window->create_objects();
		if( !window->run_window() ) {
			asset->copy_from(ff_asset,0);
			set_options(asset->ff_audio_options,
				sizeof(asset->ff_audio_options),
				window->audio_options->get_text());
		}
		delete window;
	}
	else if( video_options ) {
		FFMPEGConfigVideo *window = new FFMPEGConfigVideo(parent_window, ff_asset, edl);
		format_window = window;
		window->create_objects();
		if( !window->run_window() ) {
			asset->copy_from(ff_asset,0);
			set_options(asset->ff_video_options,
				sizeof(asset->ff_video_options),
				window->video_options->get_text());
		}
		delete window;
	}
	ff_asset->remove_user();
}

int FileFFMPEG::check_sig(Asset *asset)
{
	char *ptr = strstr(asset->path, ".pcm");
	if( ptr ) return 0;
	ptr = strstr(asset->path, ".raw");
	if( ptr ) return 0;

	FFMPEG ffmpeg(0);
	int ret = !ffmpeg.init_decoder(asset->path) &&
		!ffmpeg.open_decoder() ? 1 : 0;
	return ret;
}

void FileFFMPEG::get_info(char *path, char *text, int len)
{
	char *cp = text;
	FFMPEG ffmpeg(0);
	cp += sprintf(cp, _("file path: %s\n"), path);
	struct stat st;
	int ret = 0;
	if( stat(path, &st) < 0 ) {
		cp += sprintf(cp, _(" err: %s\n"), strerror(errno));
		ret = 1;
	}
	else {
		cp += sprintf(cp, _("  %jd bytes\n"), st.st_size);
	}
	if( !ret ) ret = ffmpeg.init_decoder(path);
	if( !ret ) ret = ffmpeg.open_decoder();
	if( !ret ) {
		cp += sprintf(cp, _("info:\n"));
		ffmpeg.info(cp, len-(cp-text));
	}
	else
		sprintf(cp, _("== open failed\n"));
}

int FileFFMPEG::get_video_info(int track, int &pid, double &framerate,
		int &width, int &height, char *title)
{
	if( !ff ) return -1;
	pid = ff->ff_video_pid(track);
	framerate = ff->ff_frame_rate(track);
	width = ff->ff_video_width(track);
	height = ff->ff_video_height(track);
	if( title ) *title = 0;
	return 0;
}

int FileFFMPEG::get_audio_for_video(int vstream, int astream, int64_t &channel_mask)
{
	if( !ff ) return 1;
	return ff->ff_audio_for_video(vstream, astream, channel_mask);
}

int FileFFMPEG::select_video_stream(Asset *asset, int vstream)
{
	if( !ff || !asset->video_data ) return 1;
	asset->width = ff->ff_video_width(vstream);
	asset->height = ff->ff_video_height(vstream);
	if( (asset->video_length = ff->ff_video_frames(vstream)) < 2 )
		asset->video_length = asset->video_length < 0 ? 0 : -1;
	asset->frame_rate = ff->ff_frame_rate(vstream);
	return 0;
}

int FileFFMPEG::select_audio_stream(Asset *asset, int astream)
{
	if( !ff || !asset->audio_data ) return 1;
	asset->sample_rate = ff->ff_sample_rate(astream);
	asset->audio_length = ff->ff_audio_samples(astream);
	return 0;
}

int FileFFMPEG::open_file(int rd, int wr)
{
	int result = 0;
	if( ff ) return 1;
	ff = new FFMPEG(this);

	if( rd ) {
		result = ff->init_decoder(asset->path);
		if( !result ) result = ff->open_decoder();
		if( !result ) {
			int audio_channels = ff->ff_total_audio_channels();
			if( audio_channels > 0 ) {
				asset->audio_data = 1;
				asset->channels = audio_channels;
				asset->sample_rate = ff->ff_sample_rate(0);
		        	asset->audio_length = ff->ff_audio_samples(0);
				strcpy(asset->acodec, ff->ff_audio_format(0));
			}
			int video_layers = ff->ff_total_video_layers();
			if( video_layers > 0 ) {
				asset->video_data = 1;
				if( !asset->layers ) asset->layers = video_layers;
				asset->actual_width = ff->ff_video_width(0);
				asset->actual_height = ff->ff_video_height(0);
				if( !asset->width ) asset->width = asset->actual_width;
				if( !asset->height ) asset->height = asset->actual_height;
				if( !asset->video_length &&
				    (asset->video_length = ff->ff_video_frames(0)) < 2 )
					asset->video_length = asset->video_length < 0 ? 0 : -1;
				if( !asset->frame_rate ) asset->frame_rate = ff->ff_frame_rate(0);
				strcpy(asset->vcodec, ff->ff_video_format(0));
			}
			IndexState *index_state = asset->index_state;
			index_state->read_markers(file->preferences->index_directory, asset->path);
		}
	}
	else if( wr ) {
		result = ff->init_encoder(asset->path);
		// must be in this order or dvdauthor will fail
		if( !result && asset->video_data )
			result = ff->open_encoder("video", asset->vcodec);
		if( !result && asset->audio_data )
			result = ff->open_encoder("audio", asset->acodec);
	}
	return result;
}

int FileFFMPEG::close_file()
{
	delete ff;
	ff = 0;
	return 0;
}


int FileFFMPEG::write_samples(double **buffer, int64_t len)
{
	if( !ff || len < 0 ) return -1;
	int stream = 0;
	int ret = ff->encode(stream, buffer, len);
	return ret;
}

int FileFFMPEG::write_frames(VFrame ***frames, int len)
{
	if( !ff ) return -1;
	int ret = 0, layer = 0;
	for(int i = 0; i < 1; i++) {
		for(int j = 0; j < len && !ret; j++) {
			VFrame *frame = frames[i][j];
			ret = ff->encode(layer, frame);
		}
	}
	return ret;
}


int FileFFMPEG::read_samples(double *buffer, int64_t len)
{
	if( !ff || len < 0 ) return -1;
	int ch = file->current_channel;
	int64_t pos = file->current_sample;
	int ret = ff->decode(ch, pos, buffer, len);
	if( ret > 0 ) return 0;
	memset(buffer,0,len*sizeof(*buffer));
	return -1;
}

int FileFFMPEG::read_frame(VFrame *frame)
{
	if( !ff ) return -1;
	int layer = file->current_layer;
	int64_t pos = asset->video_length >= 0 ? file->current_frame : 0;
	int ret = ff->decode(layer, pos, frame);
	frame->set_status(ret);
	if( ret >= 0 ) return 0;
	frame->clear_frame();
	return -1;
}


int64_t FileFFMPEG::get_memory_usage()
{
	return 0;
}

int FileFFMPEG::colormodel_supported(int colormodel)
{
	return colormodel;
}


int FileFFMPEG::get_best_colormodel(int driver, int vstream)
{
	if( vstream < 0 ) vstream = 0;
	int is_mpeg = !ff ? 0 : ff->ff_video_mpeg_color_range(vstream);

	switch(driver) {
	case PLAYBACK_X11:
	case PLAYBACK_X11_GL: return is_mpeg ? BC_YUV888 : BC_RGB888;
	case PLAYBACK_X11_XV: return BC_YUV420P;
	}

	return BC_RGB888;
}

int FileFFMPEG::get_best_colormodel(Asset *asset, int driver)
{
	switch(driver) {
// the direct X11 color model requires scaling in the codec
	case SCREENCAPTURE:
	case PLAYBACK_X11:
	case PLAYBACK_X11_GL: return BC_RGB888;
	case PLAYBACK_X11_XV: return BC_YUV420P;
	}

	return BC_YUV420P;
}

//======

FFMPEGConfigAudio::FFMPEGConfigAudio(BC_WindowBase *parent_window, Asset *asset, EDL *edl)
 : BC_Window(_(PROGRAM_NAME ": Audio Preset"),
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	420, 420)
{
	this->parent_window = parent_window;
	this->asset = asset;
	this->edl = edl;
	preset_popup = 0;

	bitrate = 0;
	audio_options = 0;
	ff_options_dialog = 0;
}

FFMPEGConfigAudio::~FFMPEGConfigAudio()
{
	delete ff_options_dialog;
	lock_window("FFMPEGConfigAudio::~FFMPEGConfigAudio");
	delete preset_popup;
	presets.remove_all_objects();
	unlock_window();
}

void FFMPEGConfigAudio::load_options()
{
	FFMPEG::load_audio_options(asset, edl);
}

void FFMPEGConfigAudio::create_objects()
{
	int x = 10, y = 10;
	lock_window("FFMPEGConfigAudio::create_objects");

	FileSystem fs;
	char option_path[BCTEXTLEN];
	FFMPEG::set_option_path(option_path, "audio");
	fs.update(option_path);
	int total_files = fs.total_files();
	for(int i = 0; i < total_files; i++) {
		const char *name = fs.get_entry(i)->get_name();
		if( asset->fformat[0] != 0 ) {
			const char *ext = strrchr(name,'.');
			if( !ext ) continue;
			if( strcmp(asset->fformat, ++ext) ) continue;
		}
		presets.append(new BC_ListBoxItem(name));
	}

	if( asset->acodec[0] ) {
		int k = presets.size();
		while( --k >= 0 && strcmp(asset->acodec, presets[k]->get_text()) );
		if( k < 0 ) asset->acodec[0] = 0;
	}

	if( !asset->acodec[0] && presets.size() > 0 )
		strcpy(asset->acodec, presets[0]->get_text());

	add_tool(new BC_Title(x, y, _("Preset:")));
	y += 25;
	preset_popup = new FFMPEGConfigAudioPopup(this, x, y);
	preset_popup->create_objects();

	y += 50;
	bitrate = new FFMpegAudioBitrate(this, x, y, _("Bitrate:"), &asset->ff_audio_bitrate);
	bitrate->create_objects();
	bitrate->set_increment(1000);
	bitrate->set_boundaries((int64_t)0, (int64_t)INT_MAX);
	y += bitrate->get_h() + 5;
	quality = new FFMpegAudioQuality(this, x, y, _("Quality:"), &asset->ff_audio_quality);
	quality->create_objects();
	quality->set_increment(1);
	quality->set_boundaries((int64_t)-1, (int64_t)51);
	y += quality->get_h() + 10;

	add_subwindow(new BC_Title(x, y, _("Samples:")));
	sample_format = new FFMpegSampleFormat(this, x+90, y, 100, 120);
	sample_format->create_objects();
	if( asset->acodec[0] ) {
		sample_format->update_formats();
		if( !asset->ff_audio_options[0] )
			load_options();
	}
	if( !asset->ff_sample_format[0] ) strcpy(asset->ff_sample_format, _("None"));
	sample_format->update(asset->ff_sample_format);
	y += sample_format->get_h() + 10;

	BC_Title *title = new BC_Title(x, y, _("Audio Options:"));
	add_subwindow(title);

	ff_options_dialog = new FFOptionsAudioDialog(this);
	int x1 = x + title->get_w() + 8;
	add_subwindow(new FFOptionsViewAudio(this, x1, y, _("view")));

	y += 25;
	audio_options = new FFAudioOptions(this, x, y, get_w()-x-20, 8,
		 sizeof(asset->ff_audio_options)-1, asset->ff_audio_options);
	audio_options->create_objects();
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window(1);

	bitrate->update_param("cin_bitrate", asset->ff_audio_options);
	quality->update_param("cin_quality", asset->ff_audio_options);

	if( asset->ff_audio_bitrate > 0 ) quality->disable();
	else if( asset->ff_audio_quality >= 0 ) bitrate->disable();

	unlock_window();
}

int FFMPEGConfigAudio::close_event()
{
	set_done(1);
	return 1;
}

FFAudioOptions::FFAudioOptions(FFMPEGConfigAudio *audio_popup,
	int x, int y, int w, int rows, int size, char *text)
 : BC_ScrollTextBox(audio_popup, x, y, w, rows, text, size)
{
	this->audio_popup = audio_popup;
}


FFMPEGConfigAudioPopup::FFMPEGConfigAudioPopup(FFMPEGConfigAudio *popup, int x, int y)
 : BC_PopupTextBox(popup, &popup->presets, popup->asset->acodec, x, y, 300, 300)
{
	this->popup = popup;
}

int FFMPEGConfigAudioPopup::handle_event()
{
	strcpy(popup->asset->acodec, get_text());
	popup->sample_format->update_formats();
	Asset *asset = popup->asset;
	asset->ff_audio_bitrate = 0;  asset->ff_audio_quality = -1;
	popup->load_options();
	popup->audio_options->update(asset->ff_audio_options);
	popup->audio_options->set_text_row(0);

	popup->bitrate->update_param("cin_bitrate", asset->ff_audio_options);
	popup->quality->update_param("cin_quality", asset->ff_audio_options);
	popup->sample_format->update(asset->ff_sample_format);
	return 1;
}


FFMPEGConfigAudioToggle::FFMPEGConfigAudioToggle(FFMPEGConfigAudio *popup,
	char *title_text, int x, int y, int *output)
 : BC_CheckBox(x, y, *output, title_text)
{
	this->popup = popup;
	this->output = output;
}
int FFMPEGConfigAudioToggle::handle_event()
{
	*output = get_value();
	return 1;
}

//======

FFMPEGConfigVideo::FFMPEGConfigVideo(BC_WindowBase *parent_window, Asset *asset, EDL *edl)
 : BC_Window(_(PROGRAM_NAME ": Video Preset"),
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	420, 420)
{
	this->parent_window = parent_window;
	this->asset = asset;
	this->edl = edl;
	preset_popup = 0;
	ff_options_dialog = 0;
	pixel_format = 0;

	bitrate = 0;
	quality = 0;
	video_options = 0;
}

FFMPEGConfigVideo::~FFMPEGConfigVideo()
{
	lock_window("FFMPEGConfigVideo::~FFMPEGConfigVideo");
	delete ff_options_dialog;
	delete pixel_format;
	delete preset_popup;
	presets.remove_all_objects();
	unlock_window();
}

void FFMPEGConfigVideo::load_options()
{
	FFMPEG::load_video_options(asset, edl);
}

void FFMPEGConfigVideo::create_objects()
{
	int x = 10, y = 10;
	lock_window("FFMPEGConfigVideo::create_objects");

	add_subwindow(new BC_Title(x, y, _("Compression:")));
	y += 25;

	FileSystem fs;
	char option_path[BCTEXTLEN];
	FFMPEG::set_option_path(option_path, "video");
	fs.update(option_path);
	int total_files = fs.total_files();
	for(int i = 0; i < total_files; i++) {
		const char *name = fs.get_entry(i)->get_name();
		if( asset->fformat[0] != 0 ) {
			const char *ext = strrchr(name,'.');
			if( !ext ) continue;
			if( strcmp(asset->fformat, ++ext) ) continue;
		}
		presets.append(new BC_ListBoxItem(name));
	}

	if( asset->vcodec[0] ) {
		int k = presets.size();
		while( --k >= 0 && strcmp(asset->vcodec, presets[k]->get_text()) );
		if( k < 0 ) asset->vcodec[0] = 0;
	}

	if( !asset->vcodec[0] && presets.size() > 0 )
		strcpy(asset->vcodec, presets[0]->get_text());

	preset_popup = new FFMPEGConfigVideoPopup(this, x, y);
	preset_popup->create_objects();

	if( asset->ff_video_bitrate > 0 && asset->ff_video_quality >= 0 ) {
		asset->ff_video_bitrate = 0;  asset->ff_video_quality = -1;
	}

	y += 50;
	bitrate = new FFMpegVideoBitrate(this, x, y, _("Bitrate:"), &asset->ff_video_bitrate);
	bitrate->create_objects();
	bitrate->set_increment(100000);
	bitrate->set_boundaries((int64_t)0, (int64_t)INT_MAX);
	y += bitrate->get_h() + 5;
	quality = new FFMpegVideoQuality(this, x, y, _("Quality:"), &asset->ff_video_quality);
	quality->create_objects();
	quality->set_increment(1);
	quality->set_boundaries((int64_t)-1, (int64_t)51);
	y += quality->get_h() + 10;

	add_subwindow(new BC_Title(x, y, _("Pixels:")));
	pixel_format = new FFMpegPixelFormat(this, x+90, y, 100, 120);
	pixel_format->create_objects();
	if( asset->vcodec[0] ) {
		pixel_format->update_formats();
		if( !asset->ff_video_options[0] )
			load_options();
	}
	if( !asset->ff_pixel_format[0] ) strcpy(asset->ff_pixel_format, _("None"));
	pixel_format->update(asset->ff_pixel_format);
	y += pixel_format->get_h() + 10;

	BC_Title *title = new BC_Title(x, y, _("Video Options:"));
	add_subwindow(title);

	ff_options_dialog = new FFOptionsVideoDialog(this);
	int x1 = x + title->get_w() + 8;
	add_subwindow(new FFOptionsViewVideo(this, x1, y, _("view")));

	y += 25;
	video_options = new FFVideoOptions(this, x, y, get_w()-x-20, 8,
		 sizeof(asset->ff_video_options)-1, asset->ff_video_options);
	video_options->create_objects();
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window(1);

	bitrate->update_param("cin_bitrate", asset->ff_video_options);
	quality->update_param("cin_quality", asset->ff_video_options);

	if( asset->ff_video_bitrate > 0 ) quality->disable();
	else if( asset->ff_video_quality >= 0 ) bitrate->disable();
	unlock_window();
}

int FFMPEGConfigVideo::close_event()
{
	set_done(1);
	return 1;
}

FFVideoOptions::FFVideoOptions(FFMPEGConfigVideo *video_popup,
	int x, int y, int w, int rows, int size, char *text)
 : BC_ScrollTextBox(video_popup, x, y, w, rows, text, size)
{
	this->video_popup = video_popup;
}


FFMPEGConfigVideoPopup::FFMPEGConfigVideoPopup(FFMPEGConfigVideo *popup, int x, int y)
 : BC_PopupTextBox(popup, &popup->presets, popup->asset->vcodec, x, y, 300, 300)
{
	this->popup = popup;
}

int FFMPEGConfigVideoPopup::handle_event()
{
	strcpy(popup->asset->vcodec, get_text());
	popup->pixel_format->update_formats();
	Asset *asset = popup->asset;
	asset->ff_video_bitrate = 0;  asset->ff_video_quality = -1;
	popup->load_options();
	popup->video_options->update(asset->ff_video_options);
	popup->video_options->set_text_row(0);

	popup->bitrate->update_param("cin_bitrate", asset->ff_video_options);
	popup->quality->update_param("cin_quality", asset->ff_video_options);
	popup->pixel_format->update(asset->ff_pixel_format);
	return 1;
}


FFMPEGConfigVideoToggle::FFMPEGConfigVideoToggle(FFMPEGConfigVideo *popup,
	char *title_text, int x, int y, int *output)
 : BC_CheckBox(x, y, *output, title_text)
{
	this->popup = popup;
	this->output = output;
}
int FFMPEGConfigVideoToggle::handle_event()
{
	*output = get_value();
	return 1;
}

FFMPEGScanProgress::FFMPEGScanProgress(IndexFile *index_file, MainProgressBar *progress_bar,
		const char *title, int64_t length, int64_t *position, int *canceled)
 : Thread(1, 0, 0)
{
	this->index_file = index_file;
	this->progress_bar = progress_bar;
	strcpy(this->progress_title, title);
	this->length = length;
	this->position = position;
	this->canceled = canceled;
	done = 0;
	start();
}

FFMPEGScanProgress::~FFMPEGScanProgress()
{
	done = 1;
	cancel();
	join();
}

void FFMPEGScanProgress::run()
{
	while( !done ) {
		if( progress_bar->update(*position) ) {
			*canceled = done = 1;
			break;
		}
		index_file->redraw_edits(0);
		usleep(500000);
	}
}

int FileFFMPEG::get_index(IndexFile *index_file, MainProgressBar *progress_bar)
{
	if( !ff ) return -1;
	if( !file->preferences->ffmpeg_marker_indexes ) return 1;

	IndexState *index_state = index_file->get_state();
	if( index_state->index_status != INDEX_NOTTESTED ) return 0;
	index_state->reset_index();
	index_state->reset_markers();
	index_state->index_status = INDEX_BUILDING;

	for( int aidx=0; aidx<ff->ffaudio.size(); ++aidx ) {
		FFAudioStream *aud = ff->ffaudio[aidx];
		index_state->add_audio_stream(aud->channels, aud->length);
	}

	FileSystem fs;
	int64_t file_bytes = fs.get_size(ff->fmt_ctx->url);
	char *index_path = index_file->index_filename;

	int canceled = 0;
	int64_t scan_position = 0;
	FFMPEGScanProgress *scan_progress = 0;
	if( progress_bar ) {
		char progress_title[BCTEXTLEN];
		sprintf(progress_title, _("Creating %s\n"), index_path);
		progress_bar->update_title(progress_title, 1);
		progress_bar->update_length(file_bytes);
		scan_progress = new FFMPEGScanProgress(index_file,
				progress_bar, progress_title, file_bytes,
				&scan_position, &canceled);
	}

	index_state->index_bytes = file_bytes;
	index_state->init_scan(file->preferences->index_size);

	if( ff->scan(index_state, &scan_position, &canceled) || canceled ) {
		index_state->reset_index();
		index_state->reset_markers();
		canceled = 1;
	}

	delete scan_progress;
	if( canceled ) return 1;

	index_state->marker_status = MARKERS_READY;
	return index_state->create_index(index_path, asset);
}


FFOptions_OptPanel::
FFOptions_OptPanel(FFOptionsWindow *fwin, int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT), opts(items[0]), vals(items[1])
{
	this->fwin = fwin;
	update();  // init col/wid/columns
}

FFOptions_OptPanel::
~FFOptions_OptPanel()
{
}

void FFOptions_OptPanel::create_objects()
{
	const char *cols[] = { _("option"), _("value"), };
	const int col1_w = 150;
	int wids[] = { col1_w, get_w()-col1_w };
	BC_ListBox::update(&items[0], &cols[0], &wids[0], sizeof(items)/sizeof(items[0]));
}

int FFOptions_OptPanel::update()
{
	opts.remove_all();
	vals.remove_all();
	FFOptions &options = fwin->options;
	for( int i=0; i<options.size(); ++i ) {
		FFOptions_Opt *opt = options[i];
		opts.append(opt->item_name);
		vals.append(opt->item_value);
	}
	draw_items(1);
	return 0;
}

int FFOptions_OptPanel::cursor_leave_event()
{
	hide_tooltip();
	return 0;
}


FFOptions_OptName::FFOptions_OptName(FFOptions_Opt *opt, const char *nm)
{
	this->opt = opt;
	set_text(nm);
}

FFOptions_OptName::~FFOptions_OptName()
{
}

FFOptions_OptValue::FFOptions_OptValue(FFOptions_Opt *opt)
{
	this->opt = opt;
}


void FFOptions_OptValue::update()
{
	if( !opt ) return;
	char val[BCTEXTLEN];  val[0] = 0;
	opt->get(val, sizeof(val));
	update(val);
}

void FFOptions_OptValue::update(const char *v)
{
	set_text(v);
}

FFOptions_Opt::FFOptions_Opt(FFOptions *options, const AVOption *opt, const char *nm)
{
	this->options = options;
	this->opt = opt;
	item_name = new FFOptions_OptName(this, nm);
	item_value = new FFOptions_OptValue(this);
}

FFOptions_Opt::~FFOptions_Opt()
{
	delete item_name;
	delete item_value;
}

char *FFOptions_Opt::get(char *vp, int sz)
{
	char *cp = vp;
	*cp = 0;
	if( !opt ) return cp;

	void *obj = (void *)options->obj;
	uint8_t *bp = 0;
	if( av_opt_get(obj, opt->name, 0, &bp) >= 0 && bp != 0 ) {
	const char *val = (const char *)bp;
		if( opt->unit && *val ) {
			int id = atoi(val);
			const char *uid = unit_name(id);
			if( uid ) val = uid;
		}
		cp = sz >= 0 ? strncpy(vp,val,sz) : strcpy(vp, val);
		if( sz > 0 ) vp[sz-1] = 0;
		av_freep(&bp);
	}

	return cp;
}

void FFOptions_Opt::set(const char *val)
{
	void *obj = (void *)options->obj;
	if( !obj || !opt ) return;
	av_opt_set(obj, opt->name, val, 0);
}


FFOptionsKindItem::FFOptionsKindItem(FFOptionsKind *kind, const char *text, int idx)
 : BC_MenuItem(text)
{
	this->kind = kind;
	this->idx = idx;
}

FFOptionsKindItem::~FFOptionsKindItem()
{
}

int FFOptionsKindItem::handle_event()
{
	FFOptionsWindow *fwin = kind->fwin;
	FFOptions &options = fwin->options;
	options.initialize(fwin, idx);
	fwin->draw();
	return 1;
}

const char *FFOptionsKind::kinds[] = {
	N_("codec"),	// FF_KIND_CODEC
	N_("ffmpeg"),	// FF_KIND_FFMPEG
};

FFOptionsKind::
FFOptionsKind(FFOptionsWindow *fwin, int x, int y, int w)
 : BC_PopupMenu(x, y, w, "")
{
	this->fwin = fwin;
}

FFOptionsKind::
~FFOptionsKind()
{
}

void FFOptionsKind::create_objects()
{
	for( int i=0; i<(int)(sizeof(kinds)/sizeof(kinds[0])); ++i )
		add_item(new FFOptionsKindItem(this, _(kinds[i]), i));
}

int FFOptionsKind::handle_event()
{
	return 1;
}

void FFOptionsKind::set(int k)
{
	this->kind = k;
	set_text(_(kinds[k]));
}

FFOptionsText::
FFOptionsText(FFOptionsWindow *fwin, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, (char *)"")
{
	this->fwin = fwin;
}

FFOptionsText::
~FFOptionsText()
{
}

int FFOptionsText::handle_event()
{
	return 0;
}

FFOptionsUnits::
FFOptionsUnits(FFOptionsWindow *fwin, int x, int y, int w)
 : BC_PopupMenu(x, y, w, "")
{
	this->fwin = fwin;
}

FFOptionsUnits::
~FFOptionsUnits()
{
}

int FFOptionsUnits::handle_event()
{
	const char *text = get_text();
	if( text && fwin->selected ) {
		fwin->selected->set(text);
		fwin->selected->item_value->update();
		av_dict_set(&fwin->dialog->ff_opts,
			fwin->selected->item_name->get_text(),
			fwin->selected->item_value->get_text(), 0);
		fwin->draw();
	}
	return 1;
}

FFOptionsApply::
FFOptionsApply(FFOptionsWindow *fwin, int x, int y)
 : BC_GenericButton(x, y, _("Apply"))
{
	this->fwin = fwin;
}

FFOptionsApply::
~FFOptionsApply()
{
}

int FFOptionsApply::handle_event()
{
	const char *text = fwin->text->get_text();
	if( text && fwin->selected ) {
		fwin->selected->set(text);
		fwin->selected->item_value->update();
		av_dict_set(&fwin->dialog->ff_opts,
			fwin->selected->item_name->get_text(),
			fwin->selected->item_value->get_text(), 0);
		fwin->draw();
	}
	return 1;
}

FFOptions::FFOptions()
{
	avctx = 0;
	obj = 0;
}

FFOptions::~FFOptions()
{
	remove_all_objects();
	avcodec_free_context(&avctx);
}

int FFOptions::cmpr(const void *a, const void *b)
{
	FFOptions_Opt *ap = *(FFOptions_Opt **)a;
	FFOptions_Opt *bp = *(FFOptions_Opt **)b;
	const char *vap = ap->item_name->get_text();
	const char *vbp = bp->item_name->get_text();
	return strcmp(vap, vbp);
}

void FFOptions::initialize(FFOptionsWindow *win, int kind)
{
	remove_all_objects();
	this->win = win;
	win->selected = 0;
	obj = 0;
	if( !avctx )
		avctx = avcodec_alloc_context3(win->dialog->codec);

	switch( kind ) {
	case FF_KIND_CODEC:
		obj = (const void *)avctx->priv_data;
		break;
	case FF_KIND_FFMPEG:
		obj = (const void *)avctx;
		break;
	}

	if( obj ) {
		FFOptions &conf = *this;
		const AVOption *opt = 0;
		while( (opt=av_opt_next(obj, opt)) != 0 ) {
			if( opt->type == AV_OPT_TYPE_CONST ) continue;
			int dupl = 0;
			for( int i=0; !dupl && i<size(); ++i ) {
				FFOptions_Opt *fopt = conf[i];
				const AVOption *op = fopt->opt;
				if( op->offset != opt->offset ) continue;
				if( op->type != opt->type ) continue;
				dupl = 1;
				if( strlen(op->name) < strlen(opt->name) )
					fopt->opt = opt;
			}
			if( dupl ) continue;
			FFOptions_Opt *fopt = new FFOptions_Opt(this, opt, opt->name);
			append(fopt);
			AVDictionaryEntry *elem = av_dict_get(win->dialog->ff_opts,
					opt->name, 0, AV_DICT_IGNORE_SUFFIX);
			if( elem && elem->value ) fopt->set(elem->value);
			char val[BCTEXTLEN], *vp = fopt->get(val, sizeof(val));
			fopt->item_value->update(vp);
		}
	}

	qsort(&values[0],size(),sizeof(values[0]),cmpr);
	win->kind->set(kind);
	win->panel->update();
	win->panel->set_yposition(0);
}

int FFOptions::update()
{
	int ret = 0;
	FFOptions &conf = *this;

	for( int i=0; i<size(); ++i ) {
		FFOptions_Opt *fopt = conf[i];
		char val[BCTEXTLEN], *vp = fopt->get(val, sizeof(val));
		if( !vp || !strcmp(val, fopt->item_value->get_text()) ) continue;
		fopt->item_value->update(val);
		++ret;
	}
	return ret;
}

void FFOptions::dump(FILE *fp)
{
	if( !obj ) return;
	const AVOption *opt = 0;
	FFOptions &conf = *this;

	while( (opt=av_opt_next(obj, opt)) != 0 ) {
		if( opt->type == AV_OPT_TYPE_CONST ) continue;
		int k = size();
		while( --k >= 0 && strcmp(opt->name, conf[k]->opt->name) );
		if( k < 0 ) continue;
		FFOptions_Opt *fopt = conf[k];
		char val[BCTEXTLEN], *vp = fopt->get(val,sizeof(val));
		fprintf(fp, "  %s:=%s", opt->name, vp);
		if( opt->unit ) {
			char unt[BCTEXTLEN], *up = unt;
			fopt->units(up);
			fprintf(fp, "%s", unt);
		}
		fprintf(fp, "\n");
	}
}


void FFOptionsWindow::update(FFOptions_Opt *opt)
{
	if( selected != opt ) {
		if( selected ) selected->item_name->set_selected(0);
		selected = opt;
		if( selected ) selected->item_name->set_selected(1);
	}
	clear_box(0,0, 0,panel->get_y());
	char str[BCTEXTLEN], *sp;
	*(sp=str) = 0;
	if( opt ) opt->types(sp);
	type->update(str);
	*(sp=str) = 0;
	if( opt ) opt->ranges(sp);
	range->update(str);
	while( units->total_items() ) units->del_item(0);
	char unit[BCSTRLEN];  strcpy(unit, "()");
	if( opt && opt->opt ) {
		ArrayList<const char *> names;
		int n = 0;
		if( opt->opt->unit ) {
 			n = opt->units(names);
			if( n > 0 ) strcpy(unit,opt->opt->unit);
		}
		for( int i=0; i<n; ++i )
			units->add_item(new BC_MenuItem(names[i], 0));
	}
	units->set_text(unit);
	char val[BCTEXTLEN];  val[0] = 0;
	if( opt ) opt->get(val, sizeof(val));
	text->update(val);

	panel->update();
}

void FFOptions_OptPanel::show_tip(const char *tip)
{
	if( !tip ) return;
	int len = strlen(tip);
	if( len > (int)sizeof(tip_text)-1 ) len = sizeof(tip_text)-1;
	strncpy(tip_text,tip,len);
	tip_text[len] = 0;
	int line_limit = 60;
	int limit2 = line_limit/2;
	int limit4 = line_limit/4-2;
	char *cp = tip_text, *dp = cp+len;
	int n;  char *bp, *ep, *pp, *sp;
	while( cp < dp ) {
		for( ep=cp; ep<dp && *ep!='\n'; ++ep );
		// target about half remaining line, constrain line_limit
		if( (n=(ep-1-cp)/2) < limit2 || n > line_limit )
			n = line_limit;
		// search for last punct, last space before line_limit
		for( bp=cp, pp=sp=0; --n>=0 && cp<ep; ++cp ) {
			if( ispunct(*cp) && isspace(*(cp+1)) ) pp = cp;
			else if( isspace(*cp) ) sp = cp;
		}
		// line not empty
		if( cp < ep ) {
			// first, after punctuation
			if( pp && pp-bp >= limit4 )
				cp = pp+1;
			// then, on spaces
			else if( sp ) {
				cp = sp;
			}
			// last, on next space
			else {
				while( cp<dp && !isspace(*cp) ) ++cp;
			}
			// add new line
			if( !*cp ) break;
			*cp++ = '\n';
		}
	}
	fwin->panel->set_tooltip(tip_text);
	fwin->panel->show_tooltip();
}

int FFOptions_OptPanel::selection_changed()
{
	FFOptions_Opt *opt = 0;
	BC_ListBoxItem *item = get_selection(0, 0);
	if( item ) {
		FFOptions_OptName *opt_name = (FFOptions_OptName *)item;
		opt = opt_name->opt;
	}
	fwin->update(opt);
	if( opt ) show_tip(opt->tip());
	return 1;
}


int FFOptions_Opt::types(char *rp)
{
	const char *cp = "";
	if( opt ) switch (opt->type) {
	case AV_OPT_TYPE_FLAGS: cp = N_("<flags>");  break;
	case AV_OPT_TYPE_INT: cp = N_("<int>"); break;
	case AV_OPT_TYPE_INT64: cp = N_("<int64>"); break;
	case AV_OPT_TYPE_DOUBLE: cp = N_("<double>"); break;
	case AV_OPT_TYPE_FLOAT: cp = N_("<float>"); break;
	case AV_OPT_TYPE_STRING: cp = N_("<string>"); break;
	case AV_OPT_TYPE_RATIONAL: cp = N_("<rational>"); break;
	case AV_OPT_TYPE_BINARY: cp = N_("<binary>"); break;
	case AV_OPT_TYPE_IMAGE_SIZE: cp = N_("<image_size>"); break;
	case AV_OPT_TYPE_VIDEO_RATE: cp = N_("<video_rate>"); break;
	case AV_OPT_TYPE_PIXEL_FMT: cp = N_("<pix_fmt>"); break;
	case AV_OPT_TYPE_SAMPLE_FMT: cp = N_("<sample_fmt>"); break;
	case AV_OPT_TYPE_DURATION: cp = N_("<duration>"); break;
	case AV_OPT_TYPE_COLOR: cp = N_("<color>"); break;
	case AV_OPT_TYPE_CHANNEL_LAYOUT: cp = N_("<channel_layout>");  break;
	case AV_OPT_TYPE_BOOL: cp = N_("<bool>");  break;
	default: cp = N_("<undef>");  break;
	}
	return sprintf(rp, "%s", _(cp));
}
int FFOptions_Opt::scalar(double d, char *rp)
{
	const char *cp = 0;
	     if( d == INT_MAX ) cp = "INT_MAX";
	else if( d == INT_MIN ) cp = "INT_MIN";
	else if( d == UINT32_MAX ) cp = "UINT32_MAX";
	else if( d == (double)INT64_MAX ) cp = "I64_MAX";
	else if( d == INT64_MIN ) cp = "I64_MIN";
	else if( d == FLT_MAX ) cp = "FLT_MAX";
	else if( d == FLT_MIN ) cp = "FLT_MIN";
	else if( d == -FLT_MAX ) cp = "-FLT_MAX";
	else if( d == -FLT_MIN ) cp = "-FLT_MIN";
	else if( d == DBL_MAX ) cp = "DBL_MAX";
	else if( d == DBL_MIN ) cp = "DBL_MIN";
	else if( d == -DBL_MAX ) cp = "-DBL_MAX";
	else if( d == -DBL_MIN ) cp = "-DBL_MIN";
	else if( d == 0 ) cp = signbit(d) ? "-0" : "0";
	else if( isnan(d) ) cp = signbit(d) ? "-NAN" : "NAN";
	else if( isinf(d) ) cp = signbit(d) ? "-INF" : "INF";
	else return sprintf(rp, "%g", d);
	return sprintf(rp, "%s", cp);
}

int FFOptions_Opt::ranges(char *rp)
{
	if( !opt ) return 0;
	void *obj = (void *)options->obj;
	if( !obj ) return 0;

	switch (opt->type) {
	case AV_OPT_TYPE_INT:
	case AV_OPT_TYPE_INT64:
	case AV_OPT_TYPE_DOUBLE:
	case AV_OPT_TYPE_FLOAT: break;
	default: return 0;;
	}
	AVOptionRanges *r = 0;
	char *cp = rp;
	if( av_opt_query_ranges(&r, obj, opt->name, AV_OPT_SEARCH_FAKE_OBJ) < 0 ) return 0;
	for( int i=0; i<r->nb_ranges; ++i ) {
		cp += sprintf(cp, " (");  cp += scalar(r->range[i]->value_min, cp);
		cp += sprintf(cp, "..");  cp += scalar(r->range[i]->value_max, cp);
		cp += sprintf(cp, ")");
	}
	av_opt_freep_ranges(&r);
	return cp - rp;
}

int FFOptions_Opt::units(ArrayList<const char *> &names)
{
	if( !opt || !opt->unit ) return 0;
	const void *obj = options->obj;
	if( !obj ) return 0;

	ArrayList<const AVOption *> opts;
	const AVOption *opt = NULL;
	while( (opt=av_opt_next(obj, opt)) != 0 ) {
		if( !opt->unit ) continue;
		if( opt->type != AV_OPT_TYPE_CONST ) continue;
		if( strcmp(this->opt->unit, opt->unit) ) continue;
		int i = opts.size();
		while( --i >= 0 ) {
			if( opts[i]->default_val.i64 != opt->default_val.i64 ) continue;
			if( strlen(opts[i]->name) < strlen(opt->name) ) opts[i] = opt;
			break;
		}
		if( i >= 0 ) continue;
		opts.append(opt);
	}

	for( int i=0; i<opts.size(); ++i )
		names.append(opts[i]->name);

	return names.size();
}

int FFOptions_Opt::units(char *rp)
{
	ArrayList<const char *> names;
	int n = units(names);
	if( !n ) return 0;
	char *cp = rp;
	cp += sprintf(cp, " [%s:", this->opt->unit);
	for( int i=0; i<n; ++i )
		cp += sprintf(cp, " %s", names[i]);
	cp += sprintf(cp, "]:");
	return cp - rp;
}

const char *FFOptions_Opt::unit_name(int id)
{
	if( !opt || !opt->unit ) return 0;
	const void *obj = options->obj;
	if( !obj ) return 0;

	const char *ret = 0;
	const AVOption *opt = NULL;
	while( (opt=av_opt_next(obj, opt)) != 0 ) {
		if( !opt->unit ) continue;
		if( opt->type != AV_OPT_TYPE_CONST ) continue;
		if( strcmp(this->opt->unit, opt->unit) ) continue;
		if( opt->default_val.i64 != id ) continue;
		if( !ret ) { ret = opt->name;  continue; }
		if( strlen(ret) < strlen(opt->name) ) ret = opt->name;
	}

	return ret;
}

const char *FFOptions_Opt::tip()
{
	return !opt ? 0 : opt->help;
}


FFOptionsWindow::FFOptionsWindow(FFOptionsDialog *dialog)
 : BC_Window(_(PROGRAM_NAME ": Options"), 60, 30, 640, 400)
{
	this->dialog = dialog;
	this->selected = 0;
}

FFOptionsWindow::~FFOptionsWindow()
{
}

void FFOptionsWindow::create_objects()
{
	lock_window("FFOptionsWindow::create_objects");
	BC_Title *title;
	int x0 = 10, y0 = 10;
	int x = x0, y = y0;
	add_subwindow(title = new BC_Title(x, y, _("Format: ")));
	x += title->get_w();
	add_subwindow(new BC_Title(x, y, dialog->format_name));
	x = x0 + 150;
	add_subwindow(title = new BC_Title(x, y, _("Codec: ")));
	x += title->get_w();
	add_subwindow(new BC_Title(x, y, dialog->codec_name));

	x = x0;  y += title->get_h() + 10;  y0 = y;
	add_subwindow(title = new BC_Title(x, y, _("Type: ")));
	x += title->get_w() + 8;
	add_subwindow(type = new BC_Title(x, y, (char *)""));
	x = x0 + 150;
	add_subwindow(title = new BC_Title(x, y, _("Range: ")));
	x += title->get_w() + 8;
	add_subwindow(range = new BC_Title(x, y, (char *)""));

	x = x0;  y += title->get_h() + 10;
	add_subwindow(units = new FFOptionsUnits(this, x, y, 120));
	x += units->get_w() + 8;
	int x1 = get_w() - BC_GenericButton::calculate_w(this, _("Apply")) - 8;
	add_subwindow(text = new FFOptionsText(this, x, y, x1-x - 8));
	add_subwindow(apply = new FFOptionsApply(this, x1, y));
	y += units->get_h() + 10;
	add_subwindow(kind = new FFOptionsKind(this, x1, y0, apply->get_w()));
	kind->create_objects();
	const char *kind_text = _("Kind:");
	x1 -= BC_Title::calculate_w(this, kind_text) + 8;
	add_subwindow(kind_title = new BC_Title(x1, y0, kind_text));
	y0 = y;

	panel_x = x0;  panel_y = y0;
	panel_w = get_w()-10 - panel_x;
	panel_h = get_h()-10 - panel_y - BC_OKButton::calculate_h();
	panel = new FFOptions_OptPanel(this, panel_x, panel_y, panel_w, panel_h);
	add_subwindow(panel);
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	panel->create_objects();
	options.initialize(this, FF_KIND_CODEC);
	draw();
	show_window(1);
	unlock_window();
}

void FFOptionsWindow::draw()
{
	update(selected);
}

int FFOptionsWindow::resize_event(int w, int h)
{
	int x1 = w - 8 - kind->get_w();
	int y = kind->get_y();
	kind->reposition_window(x1, y);
	x1 -= kind_title->get_w() + 8;
	kind_title->reposition_window(x1,y);
	x1 = get_w() - apply->get_w() - 8;
	int y1 = units->get_y();
	apply->reposition_window(x1, y1);
	int x0 = units->get_x() + units->get_w() + 8;
	int y0 = units->get_y();
	text->reposition_window(x0,y0, x1-x0-8);
	panel_w = get_w()-10 - panel_x;
	panel_h = get_h()-10 - panel_y;
	panel->reposition_window(panel_x,panel_y, panel_w, panel_h);
	return 1;
}

FFOptionsDialog::FFOptionsDialog()
 : BC_DialogThread()
{
	this->options_window = 0;
	this->codec_name = 0;
	this->codec = 0;
	this->ff_opts = 0;
	this->ff_len = 0;
}

FFOptionsDialog::~FFOptionsDialog()
{
	close_window();
	delete [] codec_name;
}

void FFOptionsDialog::load_options(const char *bp, int len)
{
	char line[BCTEXTLEN];
	char key[BCSTRLEN], val[BCTEXTLEN];
	const char *dp = bp + len-1;
	int no = 0;
	while( bp < dp && *bp != 0 ) {
		++no;
		char *cp = line, *ep = cp+sizeof(line)-1;
		while( *bp && cp<ep && (*cp=*bp++)!='\n' ) ++cp;
		*cp = 0;
		if( line[0] == '#' ) {
			sprintf(key,"#%d", no);
			av_dict_set(&ff_opts, key, line, 0);
			continue;
		}
		if( line[0] == '\n' ) continue;
		if( FFMPEG::scan_option_line(line, key, val) ) continue;
		av_dict_set(&ff_opts, key, val, 0);
	}
}

void FFOptionsDialog::store_options(char *cp, int len)
{
	char *ep = cp + len-1;
	AVDictionaryEntry *elem = 0;
	while( (elem=av_dict_get(ff_opts, "", elem, AV_DICT_IGNORE_SUFFIX)) != 0 ) {
		if( elem->key[0] == '#' ) {
			cp += snprintf(cp,ep-cp, "%s\n", elem->value);
			continue;
		}
		cp += snprintf(cp,ep-cp, "%s=%s\n", elem->key, elem->value);
	}
	*cp = 0;
}

void FFOptionsDialog::start(const char *format_name, const char *codec_name,
	AVCodec *codec, const char *options, int len)
{
	if( options_window ) {
		options_window->lock_window("FFOptionsDialog::start");
		options_window->raise_window();
		options_window->unlock_window();
		return;
	}

	this->format_name = cstrdup(format_name);
	this->codec_name = cstrdup(codec_name);
	this->codec = codec;
	this->ff_opts = 0;
	this->ff_len = len;
	load_options(options, len);

	BC_DialogThread::start();
}

BC_Window* FFOptionsDialog::new_gui()
{
	options_window = new FFOptionsWindow(this);
	options_window->create_objects();
	return options_window;
}

void FFOptionsDialog::handle_done_event(int result)
{
	if( !result ) {
		char options[ff_len];
		store_options(options, ff_len);
		update_options(options);
	}
	options_window = 0;
	delete [] format_name; format_name = 0;
	delete [] codec_name;  codec_name = 0;
	av_dict_free(&ff_opts);
}

FFOptionsAudioDialog::FFOptionsAudioDialog(FFMPEGConfigAudio *aud_config)
{
	this->aud_config = aud_config;
}

FFOptionsAudioDialog::~FFOptionsAudioDialog()
{
	close_window();
}

void FFOptionsAudioDialog::update_options(const char *options)
{
	aud_config->audio_options->update(options);
}

FFOptionsVideoDialog::FFOptionsVideoDialog(FFMPEGConfigVideo *vid_config)
{
	this->vid_config = vid_config;
}

FFOptionsVideoDialog::~FFOptionsVideoDialog()
{
	close_window();
}

void FFOptionsVideoDialog::update_options(const char *options)
{
	vid_config->video_options->update(options);
}


FFOptionsViewAudio::FFOptionsViewAudio(FFMPEGConfigAudio *aud_config, int x, int y, const char *text)
 : BC_GenericButton(x, y, text)
{
	this->aud_config = aud_config;
}

FFOptionsViewAudio::~FFOptionsViewAudio()
{
}

int FFOptionsViewAudio::handle_event()
{
	char audio_format[BCSTRLEN]; audio_format[0] = 0;
	char audio_codec[BCSTRLEN]; audio_codec[0] = 0;
	AVCodec *codec = 0;
	Asset *asset = aud_config->asset;
	const char *name = asset->acodec;
	if( !FFMPEG::get_format(audio_format, "audio", name) &&
	    !FFMPEG::get_codec(audio_codec, "audio", name) )
		codec = avcodec_find_encoder_by_name(audio_codec);
	if( !codec ) {
		eprintf(_("no codec named: %s: %s"), name, audio_codec);
		return 1;
	}
	aud_config->ff_options_dialog->start(audio_format, audio_codec, codec,
		asset->ff_audio_options, sizeof(asset->ff_audio_options));
	return 1;
}

FFOptionsViewVideo::FFOptionsViewVideo(FFMPEGConfigVideo *vid_config, int x, int y, const char *text)
 : BC_GenericButton(x, y, text)
{
	this->vid_config = vid_config;
}

FFOptionsViewVideo::~FFOptionsViewVideo()
{
}

int FFOptionsViewVideo::handle_event()
{
	char video_format[BCSTRLEN]; video_format[0] = 0;
	char video_codec[BCSTRLEN]; video_codec[0] = 0;
	AVCodec *codec = 0;
	Asset *asset = vid_config->asset;
	const char *name = asset->vcodec;
	if( !FFMPEG::get_format(video_format, "video", name) &&
	    !FFMPEG::get_codec(video_codec, "video", name) )
		codec = avcodec_find_encoder_by_name(video_codec);
	if( !codec ) {
		eprintf(_("no codec named: %s: %s"), name, video_codec);
		return 1;
	}
	vid_config->ff_options_dialog->start(video_format, video_codec, codec,
		asset->ff_video_options, sizeof(asset->ff_video_options));
	return 1;
}

