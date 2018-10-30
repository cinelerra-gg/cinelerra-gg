
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

#ifndef NEW_H
#define NEW_H

#include "assets.inc"
#include "bcdialog.h"
#include "edl.inc"
#include "file.inc"
#include "filexml.inc"
#include "guicast.h"
#include "bchash.inc"
#include "formatpresets.h"
#include "mwindow.inc"

class NewThread;
class NewWindow;
class NewPresets;
class InterlacemodePulldown;
class ColormodelPulldown;

class New
{
public:
	New(MWindow *mwindow);
	~New();

	virtual void create_objects() = 0;
	int handle_event();
	int create_new_project(int load_mode);
	void create_new_edl();

	MWindow *mwindow;
	NewThread *thread;
	EDL *new_edl;
};

class NewProject : public BC_MenuItem, public New
{
public:
	NewProject(MWindow *mwindow);
	~NewProject();

	void create_objects();
	int handle_event() { return New::handle_event(); }
};

class AppendTracks : public BC_MenuItem, public New
{
public:
	AppendTracks(MWindow *mwindow);
	~AppendTracks();

	void create_objects();
	int handle_event() { return New::handle_event(); }
};

class NewThread : public BC_DialogThread
{
public:
	NewThread(MWindow *mwindow, New *new_project, const char *title, int load_mode);
	~NewThread();

	BC_Window* new_gui();
	void handle_done_event(int result);
	void handle_close_event(int result);

	int load_defaults();
	int save_defaults();
	int update_aspect();
	int auto_aspect;
	int auto_sizes;
	NewWindow *nwindow;
	MWindow *mwindow;
	New *new_project;
	const char *title;
	int load_mode;
};

class NewWindow : public BC_Window
{
public:
	NewWindow(MWindow *mwindow, NewThread *new_thread, int x, int y);
	~NewWindow();

	void create_objects();
	int update();

	MWindow *mwindow;
	NewThread *new_thread;
	EDL *new_edl;
	BC_TextBox *atracks;
	BC_TextBox *achannels;
	BC_TextBox *sample_rate;
	BC_TextBox *vtracks;
	BC_TextBox *vchannels;
	BC_TextBox *frame_rate;
	BC_TextBox *aspect_w_text, *aspect_h_text;
	BC_TextBox *output_w_text, *output_h_text;
	BC_TextBox *folder, *name;
	BC_RecentList *recent_folder;
	InterlacemodePulldown *interlace_pulldown;
	ColormodelPulldown *color_model;
	NewPresets *format_presets;
};

class NewPresets : public FormatPresets
{
public:
	NewPresets(MWindow *mwindow, NewWindow *gui, int x, int y);
	~NewPresets();
	int handle_event();
	EDL* get_edl();
};


class NewSwapExtents : public BC_Button
{
public:
	NewSwapExtents(MWindow *mwindow, NewWindow *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	NewWindow *gui;
};



class NewATracks : public BC_TextBox
{
public:
	NewATracks(NewWindow *nwindow, const char *text, int x, int y);
	int handle_event();
	NewWindow *nwindow;
};

class NewATracksTumbler : public BC_Tumbler
{
public:
	NewATracksTumbler(NewWindow *nwindow, int x, int y);
	int handle_up_event();
	int handle_down_event();
	NewWindow *nwindow;
};

class NewAChannels : public BC_TextBox
{
public:
	NewAChannels(NewWindow *nwindow, const char *text, int x, int y);
	int handle_event();
	NewWindow *nwindow;
};

class NewAChannelsTumbler : public BC_Tumbler
{
public:
	NewAChannelsTumbler(NewWindow *nwindow, int x, int y);
	int handle_up_event();
	int handle_down_event();
	NewWindow *nwindow;
};

class NewSampleRate : public BC_TextBox
{
public:
	NewSampleRate(NewWindow *nwindow, const char *text, int x, int y);
	int handle_event();
	NewWindow *nwindow;
};


class SampleRatePulldown : public BC_ListBox
{
public:
	SampleRatePulldown(MWindow *mwindow, BC_TextBox *output, int x, int y);
	int handle_event();
	MWindow *mwindow;
	BC_TextBox *output;
};








class NewVTracks : public BC_TextBox
{
public:
	NewVTracks(NewWindow *nwindow, const char *text, int x, int y);
	int handle_event();
	NewWindow *nwindow;
};

class NewVTracksTumbler : public BC_Tumbler
{
public:
	NewVTracksTumbler(NewWindow *nwindow, int x, int y);
	int handle_up_event();
	int handle_down_event();
	NewWindow *nwindow;
};

class NewVChannels : public BC_TextBox
{
public:
	NewVChannels(NewWindow *nwindow, const char *text, int x, int y);
	int handle_event();
	NewWindow *nwindow;
};

class NewVChannelsTumbler : public BC_Tumbler
{
public:
	NewVChannelsTumbler(NewWindow *nwindow, int x, int y);
	int handle_up_event();
	int handle_down_event();
	NewWindow *nwindow;
};

class NewFrameRate : public BC_TextBox
{
public:
	NewFrameRate(NewWindow *nwindow, const char *text, int x, int y);
	int handle_event();
	NewWindow *nwindow;
};

class FrameRatePulldown : public BC_ListBox
{
public:
	FrameRatePulldown(MWindow *mwindow, BC_TextBox *output, int x, int y);
	int handle_event();
	MWindow *mwindow;
	BC_TextBox *output;
};

class NewTrackW : public BC_TextBox
{
public:
	NewTrackW(NewWindow *nwindow, int x, int y);
	int handle_event();
	NewWindow *nwindow;
};

class NewTrackH : public BC_TextBox
{
public:
	NewTrackH(NewWindow *nwindow, int x, int y);
	int handle_event();
	NewWindow *nwindow;
};

class FrameSizePulldown : public BC_ListBox
{
public:
	FrameSizePulldown(Theme *theme,
		BC_TextBox *output_w,
		BC_TextBox *output_h,
		int x,
		int y);
	int handle_event();
	Theme *theme;
	BC_TextBox *output_w;
	BC_TextBox *output_h;
};

class NewOutputW : public BC_TextBox
{
public:
	NewOutputW(NewWindow *nwindow, int x, int y);
	int handle_event();
	NewWindow *nwindow;
};

class NewOutputH : public BC_TextBox
{
public:
	NewOutputH(NewWindow *nwindow, int x, int y);
	int handle_event();
	NewWindow *nwindow;
};

class NewAspectAuto : public BC_CheckBox
{
public:
	NewAspectAuto(NewWindow *nwindow, int x, int y);
	~NewAspectAuto();
	int handle_event();
	NewWindow *nwindow;
};

class NewAspectW : public BC_TextBox
{
public:
	NewAspectW(NewWindow *nwindow, const char *text, int x, int y);
	int handle_event();
	NewWindow *nwindow;
};

class NewAspectH : public BC_TextBox
{
public:
	NewAspectH(NewWindow *nwindow, const char *text, int x, int y);
	int handle_event();
	NewWindow *nwindow;
};

class AspectPulldown : public BC_ListBox
{
public:
	AspectPulldown(MWindow *mwindow,
		BC_TextBox *output_w,
		BC_TextBox *output_h,
		int x,
		int y);
	int handle_event();
	MWindow *mwindow;
	BC_TextBox *output_w;
	BC_TextBox *output_h;
};

class ColormodelItem : public BC_ListBoxItem
{
public:
	ColormodelItem(const char *text, int value);
	int value;
};

class ColormodelPulldown : public BC_ListBox
{
public:
	ColormodelPulldown(MWindow *mwindow,
		BC_TextBox *output_text,
		int *output_value,
		int x,
		int y);
	int handle_event();
	const char* colormodel_to_text();
	void update_value(int value);
	MWindow *mwindow;
	BC_TextBox *output_text;
	int *output_value;
};

class InterlacemodeItem : public BC_ListBoxItem
{
public:
	InterlacemodeItem(const char *text, int value);
	int value;
};

class InterlacemodePulldown : public BC_ListBox
{
public:
	InterlacemodePulldown(MWindow *mwindow,
				BC_TextBox *output_text,
				int *output_value,
				ArrayList<BC_ListBoxItem*> *data,
				int x,
				int y);
	int handle_event();
	const char* interlacemode_to_text();
	int update(int value);
	MWindow *mwindow;
	BC_TextBox *output_text;
	int *output_value;
private:
  	char string[BCTEXTLEN];
};

class InterlacefixmethodItem : public BC_ListBoxItem
{
public:
	InterlacefixmethodItem(const char *text, int value);
	int value;
};

class InterlacefixmethodPulldown : public BC_ListBox
{
public:
	InterlacefixmethodPulldown(MWindow *mwindow,
				   BC_TextBox *output_text,
				   int *output_value,
				   ArrayList<BC_ListBoxItem*> *data,
				   int x,
				   int y);
	int handle_event();
	const char* interlacefixmethod_to_text();
	MWindow *mwindow;
	BC_TextBox *output_text;
	int *output_value;
private:
  	char string[BCTEXTLEN];
};


#endif
