
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

#ifndef RECORDPREFS_H
#define RECORDPREFS_H

#include "adeviceprefs.inc"
#include "formattools.inc"
#include "mwindow.inc"
#include "preferencesthread.h"
#include "recordprefs.inc"
#include "vdeviceprefs.inc"

class RecordPrefs : public PreferencesDialog
{
public:
	RecordPrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~RecordPrefs();

	void create_objects();
	int show_window(int flush);

	FormatTools *recording_format;
	RecordRealtimeTOC *realtime_toc;
	ADevicePrefs *audio_in_device;
	VDevicePrefs *video_in_device;
	MWindow *mwindow;
};


class RecordFragment : public BC_PopupMenu
{
public:
	RecordFragment(int x,
		int y,
		PreferencesWindow *pwindow,
		RecordPrefs *record,
		char *text);
	int handle_event();
	PreferencesWindow *pwindow;
	RecordPrefs *record;
};

class RecordWriteLength : public BC_TextBox
{
public:
	RecordWriteLength(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, char *text);
	int handle_event();
	PreferencesWindow *pwindow;
};

/*
 * class DuplexEnable : public BC_CheckBox
 * {
 * public:
 * 	DuplexEnable(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, int value);
 * 	int handle_event();
 * 	PreferencesWindow *pwindow;
 * };
 *
 */
class RecordRealTime : public BC_CheckBox
{
public:
	RecordRealTime(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, int value);
	int handle_event();
	PreferencesWindow *pwindow;
};


class RecordSampleRate : public BC_TextBox
{
public:
	RecordSampleRate(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};


class RecordRealtimeTOC : public BC_CheckBox
{
public:
	RecordRealtimeTOC(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, int value);
	int handle_event();
	PreferencesWindow *pwindow;
};


class RecordPositioning : public BC_ListBox
{
	PreferencesWindow *pwindow;
	BC_TextBox *textbox;
public:
	ArrayList<BC_ListBoxItem*> position_type;
	RecordPositioning(PreferencesWindow *pwindow,BC_TextBox *textbox);
	~RecordPositioning();
	void create_objects();
	int handle_event();
};


class RecordSyncDrives : public BC_CheckBox
{
public:
	RecordSyncDrives(PreferencesWindow *pwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};


class RecordMap51_2 : public BC_CheckBox
{
public:
	RecordMap51_2(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, int value);
	int handle_event();
	PreferencesWindow *pwindow;
};


class VideoWriteLength : public BC_TextBox
{
public:
	VideoWriteLength(PreferencesWindow *pwindow, char *text, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class VideoCaptureLength : public BC_TextBox
{
public:
	VideoCaptureLength(PreferencesWindow *pwindow, char *text, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class CaptureLengthTumbler : public BC_Tumbler
{
public:
	CaptureLengthTumbler(PreferencesWindow *pwindow, BC_TextBox *text, int x, int y);
	int handle_up_event();
	int handle_down_event();
	PreferencesWindow *pwindow;
	BC_TextBox *text;
};

class RecordW : public BC_TextBox
{
public:
	RecordW(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class RecordH : public BC_TextBox
{
public:
	RecordH(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class RecordFrameRate : public BC_TextBox
{
public:
	RecordFrameRate(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class RecordFrameRateText : public BC_TextBox
{
	RecordFrameRateText(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class RecordChannels : public BC_TumbleTextBox
{
public:
	RecordChannels(PreferencesWindow *pwindow,
		BC_SubWindow *gui,
		int x,
		int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class RecordGain : public BC_TumbleTextBox
{
public:
	RecordGain(PreferencesWindow *pwindow,
		BC_SubWindow *gui,
		int x,
		int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

#endif
