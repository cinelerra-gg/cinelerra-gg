
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

#ifndef FORMATTOOLS_H
#define FORMATTOOLS_H

#include "asset.inc"
#include "guicast.h"
#include "bitspopup.h"
#include "browsebutton.h"
#include "compresspopup.h"
#include "file.inc"
#include "ffmpeg.h"
#include "formatpopup.h"
#include "formattools.inc"
#include "mwindow.inc"

class FormatTools
{
public:
	FormatTools(MWindow *mwindow, BC_WindowBase *window, Asset *asset);
	virtual ~FormatTools();

	void create_objects(int &init_x, int &init_y,
		int do_audio, int do_video,   // Include tools for audio, video
		int prompt_audio,  int prompt_video,  // Include checkbox for audio, video
		int prompt_audio_channels, int prompt_video_compression,
		const char *locked_compressor,  // Select compressors to be offered
		int recording, // Change captions for recording
		int *file_per_label,  // prompt if nonzero
		int brender,   // Supply file formats for background rendering
		int horizontal_layout = 0);
// In recording preferences, aspects of the format are locked
// depending on the driver used.
	void update_driver(int driver);
	virtual void update_format();


	void reposition_window(int &init_x, int &init_y);
// Put new asset's parameters in and change asset.
	void update(Asset *asset, int *file_per_label);
// Update filename extension when format is changed.
	void update_extension();
	void close_format_windows();
	Asset* get_asset();

// Handle change in path text.  Used in BatchRender.
	virtual int handle_event();

	int set_audio_options();
	int set_video_options();
	void set_w(int w);
	int get_w();

	BC_WindowBase *window;
	Asset *asset;

	FormatAParams *aparams_button;
	FormatVParams *vparams_button;
	FormatAThread *aparams_thread;
	FormatVThread *vparams_thread;
	BrowseButton *path_button;
	FormatPathText *path_textbox;
	BC_RecentList *path_recent;
	BC_Title *format_title;
	FormatFormat *format_button;
	BC_TextBox *format_text;
	FormatFFMPEG *format_ffmpeg;
	FFMpegType *ffmpeg_type;

	BC_Title *audio_title;
	FormatAudio *audio_switch;

	BC_Title *video_title;
	FormatVideo *video_switch;

	FormatFilePerLabel *labeled_files;

	MWindow *mwindow;
	const char *locked_compressor;
	int recording;
	int use_brender;
	int do_audio;
	int do_video;
	int prompt_audio;
	int prompt_audio_channels;
	int prompt_video;
	int prompt_video_compression;
	int *file_per_label;
	int w;
// Determines what the configuration buttons do.
	int video_driver;
};



class FormatPathText : public BC_TextBox
{
public:
	FormatPathText(int x, int y, FormatTools *format);
	~FormatPathText();
	int handle_event();

	FormatTools *format;
};



class FormatFormat : public FormatPopup
{
public:
	FormatFormat(int x, int y, FormatTools *format);
	~FormatFormat();

	int handle_event();
	FormatTools *format;
};

class FormatFFMPEG : public FFMPEGPopup
{
public:
	FormatFFMPEG(int x, int y, FormatTools *format);
	~FormatFFMPEG();

	int handle_event();
	FormatTools *format;

// squash show/hide window
	int show_window(int flush=1) { return 0; }
	int hide_window(int flush=1) { return 0; }
	int show(int flush=1) { return BC_SubWindow::show_window(flush); }
	int hide(int flush=1) { return BC_SubWindow::hide_window(flush); }
};

class FFMpegType : public BC_TextBox
{
public:
	FFMpegType(int x, int y, int w, int h, const char *text)
	 : BC_TextBox(x, y, w, h, text) {}
	~FFMpegType() {}
// squash show/hide window
	int show_window(int flush=1) { return 0; }
	int hide_window(int flush=1) { return 0; }
	int show(int flush=1) { return BC_SubWindow::show_window(flush); }
	int hide(int flush=1) { return BC_SubWindow::hide_window(flush); }
};

class FormatAParams : public BC_Button
{
public:
	FormatAParams(MWindow *mwindow, FormatTools *format, int x, int y);
	~FormatAParams();
	int handle_event();
	FormatTools *format;
};

class FormatVParams : public BC_Button
{
public:
	FormatVParams(MWindow *mwindow, FormatTools *format, int x, int y);
	~FormatVParams();
	int handle_event();
	FormatTools *format;
};


class FormatAThread : public Thread
{
public:
	FormatAThread(FormatTools *format);
	~FormatAThread();

	void run();
	void start();
	void join() { if( !joined ) { joined = 1; Thread::join(); } }

	FormatTools *format;
	File *file;
	int joined;
};

class FormatVThread : public Thread
{
public:
	FormatVThread(FormatTools *format);
	~FormatVThread();

	void run();
	void start();
	void join() { if( !joined ) { joined = 1; Thread::join(); } }

	FormatTools *format;
	File *file;
	int joined;
};

class FormatAudio : public BC_CheckBox
{
public:
	FormatAudio(int x, int y, FormatTools *format, int default_);
	~FormatAudio();
	int handle_event();
	FormatTools *format;
};

class FormatVideo : public BC_CheckBox
{
public:
	FormatVideo(int x, int y, FormatTools *format, int default_);
	~FormatVideo();
	int handle_event();
	FormatTools *format;
};


class FormatFilePerLabel : public BC_CheckBox
{
public:
	FormatFilePerLabel(FormatTools *format, int x, int y, int *output);
	~FormatFilePerLabel();
	int handle_event();
	void update(int *output);

	FormatTools *format;
	int *output;
};


#endif
