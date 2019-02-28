
/*
 * CINELERRA
 * Copyright (C) 2010-2013 Adam Williams <broadcast at earthling dot net>
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
#include "clip.h"
#include "guicast.h"
#include "file.h"
#include "filesystem.h"
#include "formattools.h"
#include "language.h"
#ifdef HAVE_DV
#include "libdv.h"
#endif
#include "libmjpeg.h"
#include "maxchannels.h"
#include "mwindow.h"
#include "preferences.h"
#include "theme.h"
#include "videodevice.inc"
#include <string.h>
#include <unistd.h>
#include <ctype.h>


FormatTools::FormatTools(MWindow *mwindow,
				BC_WindowBase *window,
				Asset *asset)
{
	this->mwindow = mwindow;
	this->window = window;
	this->asset = asset;

	aparams_button = 0;
	vparams_button = 0;
	aparams_thread = 0;
	vparams_thread = 0;
	audio_switch = 0;
	video_switch = 0;
	path_textbox = 0;
	path_button = 0;
	path_recent = 0;
	format_title = 0;
	format_button = 0;
	format_text = 0;
	audio_title = 0;
	video_title = 0;
	labeled_files = 0;
	w = window->get_w();

	recording = 0;
	use_brender = 0;
	do_audio = 0;
	do_video = 0;
	prompt_audio = 0;
	prompt_audio_channels = 0;
	prompt_video = 0;
	prompt_video_compression = 0;
	file_per_label = 0;
	locked_compressor = 0;
	video_driver = 0;
}

FormatTools::~FormatTools()
{
	delete path_recent;
SET_TRACE
	delete path_button;
SET_TRACE
	delete path_textbox;
SET_TRACE
	delete format_button;
SET_TRACE

	if(aparams_button) delete aparams_button;
SET_TRACE
	if(vparams_button) delete vparams_button;
SET_TRACE
	if(aparams_thread) delete aparams_thread;
SET_TRACE
	if(vparams_thread) delete vparams_thread;
SET_TRACE
}

void FormatTools::create_objects(
		int &init_x, int &init_y,
		int do_audio, int do_video,   // Include support for audio, video
		int prompt_audio, int prompt_video, // Include checkbox for audio, video
		int prompt_audio_channels,
		int prompt_video_compression,
		const char *locked_compressor,
		int recording,
		int *file_per_label,
		int brender,
		int horizontal_layout)
{
	int x = init_x;
	int y = init_y;
	int ylev = init_y;
	int margin = mwindow->theme->widget_border;

	this->locked_compressor = locked_compressor;
	this->recording = recording;
	this->use_brender = brender;
	this->do_audio = do_audio;
	this->do_video = do_video;
	this->prompt_audio = prompt_audio;
	this->prompt_audio_channels = prompt_audio_channels;
	this->prompt_video = prompt_video;
	this->prompt_video_compression = prompt_video_compression;
	this->file_per_label = file_per_label;

//printf("FormatTools::create_objects 1\n");

	if(!recording)
	{
		int px = x;
		window->add_subwindow(path_textbox = new FormatPathText(px, y, this));
		px += path_textbox->get_w() + 5;
		path_recent = new BC_RecentList("PATH", mwindow->defaults,
					path_textbox, 10, px, y, 300, 100);
		window->add_subwindow(path_recent);
		path_recent->load_items(File::formattostr(asset->format));
		px += path_recent->get_w();
		window->add_subwindow(path_button = new BrowseButton(
			mwindow->theme, window, path_textbox, px, y, asset->path,
			_("Output to file"), _("Select a file to write to:"), 0));

// Set w for user.
		w = MAX(w, 305);
		y += path_textbox->get_h() + 10;
	}
	else
	{
//		w = x + 305;
		w = 305;
	}

	x = init_x;
	window->add_subwindow(format_title = new BC_Title(x, y, _("File Format:")));
	x += format_title->get_w() + margin;
	window->add_subwindow(format_text = new BC_TextBox(x, y, 160, 1,
		File::formattostr(asset->format)));
	x += format_text->get_w() + margin;
//printf("FormatTools::create_objects %d %p\n", __LINE__, window);
	window->add_subwindow(format_button = new FormatFormat(x, y, this));
	format_button->create_objects();
	x += format_button->get_w() + 5;
	window->add_subwindow(ffmpeg_type = new FFMpegType(x, y, 70, 1, asset->fformat));
	FFMPEG::set_asset_format(asset, mwindow->edl, asset->fformat);
	x += ffmpeg_type->get_w();
	window->add_subwindow(format_ffmpeg = new FormatFFMPEG(x, y, this));
	format_ffmpeg->create_objects();
	x = init_x;
	y += format_button->get_h() + 10;
	if( do_audio ) {
		window->add_subwindow(audio_title = new BC_Title(x, y, _("Audio:"), LARGEFONT,
			BC_WindowBase::get_resources()->audiovideo_color));
		x += audio_title->get_w() + margin;
		window->add_subwindow(aparams_button = new FormatAParams(mwindow, this, x, y));
		x += aparams_button->get_w() + margin;
		if(prompt_audio) {
			window->add_subwindow(audio_switch = new FormatAudio(x, y, this, asset->audio_data));
		}
		x = init_x;
		ylev = y;
		y += aparams_button->get_h() + 10;

//printf("FormatTools::create_objects 6\n");
		aparams_thread = new FormatAThread(this);
	}

//printf("FormatTools::create_objects 7\n");
	if( do_video ) {
		if( horizontal_layout && do_audio ) {
			x += 370;
			y = ylev;
		}

//printf("FormatTools::create_objects 8\n");
		window->add_subwindow(video_title = new BC_Title(x, y, _("Video:"), LARGEFONT,
			BC_WindowBase::get_resources()->audiovideo_color));
		x += video_title->get_w() + margin;
		if(prompt_video_compression) {
			window->add_subwindow(vparams_button = new FormatVParams(mwindow, this, x, y));
			x += vparams_button->get_w() + margin;
		}

//printf("FormatTools::create_objects 9\n");
		if(prompt_video) {
			window->add_subwindow(video_switch = new FormatVideo(x, y, this, asset->video_data));
			y += video_switch->get_h();
		}
		else {
			y += vparams_button->get_h();
		}

//printf("FormatTools::create_objects 10\n");
		y += 10;
		vparams_thread = new FormatVThread(this);
	}

//printf("FormatTools::create_objects 11\n");

	x = init_x;
	if( file_per_label ) {
		labeled_files = new FormatFilePerLabel(this, x, y, file_per_label);
		window->add_subwindow(labeled_files);
		y += labeled_files->get_h() + 10;
	}

//printf("FormatTools::create_objects 12\n");

	init_y = y;
	update_format();
}

void FormatTools::update_driver(int driver)
{
	this->video_driver = driver;

	locked_compressor = 0;
	switch(driver)
	{
		case CAPTURE_DVB:
		case VIDEO4LINUX2MPEG:
// Just give the user information about how the stream is going to be
// stored but don't change the asset.
// Want to be able to revert to user settings.
			if(asset->format == FILE_MPEG) break;
			asset->format = FILE_MPEG;
			format_text->update(File::formattostr(asset->format));
			asset->audio_data = 1;
			asset->video_data = 1;
			audio_switch->update(1);
			video_switch->update(1);
			break;

		case CAPTURE_IEC61883:
		case CAPTURE_FIREWIRE:
		case VIDEO4LINUX2JPEG:
		case CAPTURE_JPEG_WEBCAM:
			asset->format = FILE_FFMPEG;
			format_text->update(File::formattostr(asset->format));

			switch(driver) {
#ifdef HAVE_DV
			case CAPTURE_IEC61883:
			case CAPTURE_FIREWIRE:
				locked_compressor = (char*)CODEC_TAG_DVSD;
				break;
#endif
			case VIDEO4LINUX2JPEG:
				locked_compressor = (char*)CODEC_TAG_MJPEG;
				break;

			case CAPTURE_JPEG_WEBCAM:
				locked_compressor = (char*)CODEC_TAG_JPEG;
				break;
			}
			if( locked_compressor )
				strcpy(asset->vcodec, locked_compressor);

			audio_switch->update(asset->audio_data);
			video_switch->update(asset->video_data);
			break;

		default:
			format_text->update(File::formattostr(asset->format));
			audio_switch->update(asset->audio_data);
			video_switch->update(asset->video_data);
			break;
	}
	close_format_windows();
	update_format();
}

void FormatTools::update_format()
{
	if( do_audio && prompt_audio && audio_switch ) {
		audio_switch->update(asset->audio_data);
		if( File::renders_audio(asset) )
			audio_switch->enable();
		else
			audio_switch->disable();
	}
	if( do_video && prompt_video && video_switch ) {
		video_switch->update(asset->video_data);
		if( File::renders_video(asset) )
			video_switch->enable();
		else
			video_switch->disable();
	}
	if( asset->format == FILE_FFMPEG ) {
		ffmpeg_type->show();
		format_ffmpeg->show();
	}
	else {
		ffmpeg_type->hide();
		format_ffmpeg->hide();
	}
}

int FormatTools::handle_event()
{
	return 0;
}

Asset* FormatTools::get_asset()
{
	return asset;
}

void FormatTools::update_extension()
{
	const char *extension = File::get_tag(asset->format);
// split multiple extensions
	ArrayList<const char*> extensions;
	int len = !extension ? -1 : strlen(extension);
	const char *extension_ptr = extension;
	for(int i = 0; i <= len; i++)
	{
		if(extension[i] == '/' || extension[i] == 0)
		{
			extensions.append(extension_ptr);
			extension_ptr = extension + i + 1;
		}
	}

	if(extensions.size())
	{
		char *ptr = strrchr(asset->path, '.');
		if(!ptr)
		{
			ptr = asset->path + strlen(asset->path);
			*ptr = '.';
		}
		ptr++;

// test for equivalent extension
		int need_extension = 1;
		//int extension_len = 0;
		for(int i = 0; i < extensions.size() && need_extension; i++)
		{
			char *ptr1 = ptr;
			extension_ptr = extensions.get(i);
// test an extension
			need_extension = 0;
			while(*ptr1 != 0 && *extension_ptr != 0 && *extension_ptr != '/')
			{
				if(tolower(*ptr1) != tolower(*extension_ptr))
				{
					need_extension = 1;
					break;
				}
				ptr1++;
				extension_ptr++;
			}

			if( (!*ptr1 && (*extension_ptr && *extension_ptr != '/')) ||
			    (*ptr1 && (!*extension_ptr || *extension_ptr == '/')) )
				need_extension = 1;
		}

//printf("FormatTools::update_extension %d %d\n", __LINE__, need_extension);
// copy extension
		if(need_extension)
		{
			char *ptr1 = ptr;
			extension_ptr = asset->format != FILE_FFMPEG ?
				extensions.get(0) : asset->fformat;
			while(*extension_ptr != 0 && *extension_ptr != '/')
				*ptr1++ = *extension_ptr++;
			*ptr1 = 0;
		}

		int character1 = ptr - asset->path;
		int character2 = strlen(asset->path);
//		*(asset->path + character2) = 0;
		if(path_textbox)
		{
			path_textbox->update(asset->path);
			path_textbox->set_selection(character1, character2, character2);
		}
	}
}

void FormatTools::update(Asset *asset, int *file_per_label)
{
	this->asset = asset;
	this->file_per_label = file_per_label;
	if( file_per_label ) labeled_files->update(file_per_label);
	if( path_textbox ) path_textbox->update(asset->path);
	format_text->update(File::formattostr(asset->format));
	update_format();
	close_format_windows();
}

void FormatTools::close_format_windows()
{
// This is done in ~file
	if( aparams_thread ) {
		if( aparams_thread->running() )
			aparams_thread->file->close_window();
		aparams_thread->join();
	}
	if( vparams_thread ) {
		if( vparams_thread->running() )
			vparams_thread->file->close_window();
		vparams_thread->join();
	}
}

int FormatTools::get_w()
{
	return asset->format != FILE_FFMPEG ? w :
		format_ffmpeg->get_x() + format_ffmpeg->get_w();
}

void FormatTools::set_w(int w)
{
	this->w = w;
}

void FormatTools::reposition_window(int &init_x, int &init_y)
{
	int x = init_x;
	int y = init_y;

	if(path_textbox)
	{
		int px = x;
		path_textbox->reposition_window(px, y);
		px += path_textbox->get_w() + 5;
		path_recent->reposition_window(px, y);
		px += path_recent->get_w() + 8;
		path_button->reposition_window(px, y);
		y += path_textbox->get_h() + 10;
	}

	format_title->reposition_window(x, y);
	x += 90;
	format_text->reposition_window(x, y);
	x += format_text->get_w();
	format_button->reposition_window(x, y);

	x = init_x;
	y += format_button->get_h() + 10;

	if(do_audio)
	{
		audio_title->reposition_window(x, y);
		x += 80;
		aparams_button->reposition_window(x, y);
		x += aparams_button->get_w() + 10;
		if(prompt_audio) audio_switch->reposition_window(x, y);

		x = init_x;
		y += aparams_button->get_h() + 10;
	}


	if(do_video)
	{
		video_title->reposition_window(x, y);
		x += 80;
		if(prompt_video_compression)
		{
			vparams_button->reposition_window(x, y);
			x += vparams_button->get_w() + 10;
		}

		if(prompt_video)
		{
			video_switch->reposition_window(x, y);
			y += video_switch->get_h();
		}
		else
		{
			y += vparams_button->get_h();
		}

		y += 10;
		x = init_x;
	}

	if( file_per_label ) {
		labeled_files->reposition_window(x, y);
		y += labeled_files->get_h() + 10;
	}

	init_y = y;
}


int FormatTools::set_audio_options()
{
//	if(video_driver == CAPTURE_DVB)
//	{
//		return 0;
//	}

	if(!aparams_thread->running())
	{
		aparams_thread->start();
	}
	else
	{
		aparams_thread->file->raise_window();
	}
	return 0;
}

int FormatTools::set_video_options()
{
//	if(video_driver == CAPTURE_DVB)
//	{
//		return 0;
//	}

	if(!vparams_thread->running())
	{
		vparams_thread->start();
	}
	else
	{
		vparams_thread->file->raise_window();
	}

	return 0;
}





FormatAParams::FormatAParams(MWindow *mwindow, FormatTools *format, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("wrench"))
{
	this->format = format;
	set_tooltip(_("Configure audio compression"));
}

FormatAParams::~FormatAParams()
{
}

int FormatAParams::handle_event()
{
	format->set_audio_options();
	format->handle_event();
	return 1;
}





FormatVParams::FormatVParams(MWindow *mwindow, FormatTools *format, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("wrench"))
{
	this->format = format;
	set_tooltip(_("Configure video compression"));
}

FormatVParams::~FormatVParams()
{
}

int FormatVParams::handle_event()
{
	format->set_video_options();
	format->handle_event();
	return 1;
}





FormatAThread::FormatAThread(FormatTools *format)
 : Thread(1, 0, 0)
{
	this->format = format;
	file = new File;
	joined = 1;
}

FormatAThread::~FormatAThread()
{
	delete file;  file = 0;
	join();
}

void FormatAThread::start()
{
	join();
	joined = 0;
	Thread::start();
}


void FormatAThread::run()
{
	file->get_options(format, 1, 0);
}




FormatVThread::FormatVThread(FormatTools *format)
 : Thread(1, 0, 0)
{
	this->format = format;
	file = new File;
	joined = 1;
}

FormatVThread::~FormatVThread()
{
	delete file;  file = 0;
	join();
}

void FormatVThread::start()
{
	join();
	joined = 0;
	Thread::start();
}

void FormatVThread::run()
{
	file->get_options(format, 0, 1);
}





FormatPathText::FormatPathText(int x, int y, FormatTools *format)
 : BC_TextBox(x, y, format->w - x -
		2*format->mwindow->theme->get_image_set("wrench")[0]->get_w() - 20, 1,
	format->asset->path)
{
	this->format = format;
}

FormatPathText::~FormatPathText()
{
}
int FormatPathText::handle_event()
{
	calculate_suggestions();
	strcpy(format->asset->path, get_text());
	format->handle_event();
	return 1;
}




FormatAudio::FormatAudio(int x, int y, FormatTools *format, int default_)
 : BC_CheckBox(x,
 	y,
	default_,
	(char*)(format->recording ? _("Record audio tracks") : _("Render audio tracks")))
{
	this->format = format;
}

FormatAudio::~FormatAudio() {}
int FormatAudio::handle_event()
{
	format->asset->audio_data = get_value();
	format->handle_event();
	return 1;
}


FormatVideo::FormatVideo(int x, int y, FormatTools *format, int default_)
 : BC_CheckBox(x,
 	y,
	default_,
	(char*)(format->recording ? _("Record video tracks") : _("Render video tracks")))
{
this->format = format;
}

FormatVideo::~FormatVideo() {}
int FormatVideo::handle_event()
{
	format->asset->video_data = get_value();
	format->handle_event();
	return 1;
}




FormatFormat::FormatFormat(int x, int y, FormatTools *format)
 : FormatPopup(x, y, format->do_audio, format->do_video, format->use_brender)
{
	this->format = format;
}

FormatFormat::~FormatFormat()
{
}

int FormatFormat::handle_event()
{
	BC_ListBoxItem *selection = get_selection(0, 0);
	if( selection ) {
		int new_format = File::strtoformat(get_selection(0, 0)->get_text());
//		if(new_format != format->asset->format)
		{
			Asset *asset = format->asset;
			asset->format = new_format;
			asset->audio_data = File::renders_audio(asset);
			asset->video_data = File::renders_video(asset);
			asset->ff_audio_options[0] = 0;
			asset->ff_video_options[0] = 0;
			format->format_text->update(selection->get_text());
			if( !format->use_brender )
				format->update_extension();
			format->close_format_windows();
			if (format->path_recent) format->path_recent->
				load_items(File::formattostr(format->asset->format));
			format->update_format();
		}
		format->handle_event();
	}
	return 1;
}


FormatFFMPEG::FormatFFMPEG(int x, int y, FormatTools *format)
 : FFMPEGPopup(x, y)
{
	this->format = format;
}

FormatFFMPEG::~FormatFFMPEG()
{
}

int FormatFFMPEG::handle_event()
{
	BC_ListBoxItem *selection = get_selection(0, 0);
	if( selection ) {
		char *text = get_selection(0, 0)->get_text();
		format->ffmpeg_type->update(text);
		format->asset->ff_audio_options[0] = 0;
		format->asset->ff_video_options[0] = 0;
		FFMPEG::set_asset_format(format->asset, format->mwindow->edl, text);
		format->update_extension();
		format->close_format_windows();
		format->update_format();
		format->handle_event();
	}
	return 1;
}


FormatFilePerLabel::FormatFilePerLabel(FormatTools *format,
	int x, int y, int *output)
 : BC_CheckBox(x, y, *output, _("Create new file at each label"))
{
	this->format = format;
	this->output = output;
}

FormatFilePerLabel::~FormatFilePerLabel()
{
}

int FormatFilePerLabel::handle_event()
{
	*output = get_value();
	format->handle_event();
	return 1;
}

void FormatFilePerLabel::update(int *output)
{
	this->output = output;
	set_value(*output ? 1 : 0);
}


