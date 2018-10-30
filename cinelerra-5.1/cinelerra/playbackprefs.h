
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

#ifndef PLAYBACKPREFS_H
#define PLAYBACKPREFS_H


class PlaybackPrefs;
class PlaybackModuleFragment;
class PlaybackAudioOffset;
class PlaybackViewFollows;
class PlaybackSoftwareTimer;
class PlaybackRealTime;
class PlaybackMap51_2;
//class VideoAsynchronous;
class VideoEveryFrame;
class PlaybackPreload;
class PlaybackInterpolateRaw;
class PlaybackWhiteBalanceRaw;
class PlaybackSubtitle;
class PlaybackSubtitleNumber;
class PlaybackLabelCells;
class PlaybackProgramNumber;
class PlaybackGain;

#include "adeviceprefs.h"
#include "guicast.h"
#include "mwindow.inc"
#include "preferencesthread.h"
#include "vdeviceprefs.h"

class PlaybackPrefs : public PreferencesDialog
{
public:
	PlaybackPrefs(MWindow *mwindow, PreferencesWindow *pwindow, int config_number);
	~PlaybackPrefs();

	void create_objects();
//	int set_strategy(int strategy);

	static char* strategy_to_string(int strategy);
	void delete_strategy();

	int draw_framerate(int flush /* = 1 */);

	int config_number;
	ADevicePrefs *audio_device;
	VDevicePrefs *video_device;
	ArrayList<BC_ListBoxItem*> strategies;

	PlaybackConfig *playback_config;
	BC_Title *framerate_title;
	PlaybackInterpolateRaw *interpolate_raw;
	PlaybackWhiteBalanceRaw *white_balance_raw;
//	VideoAsynchronous *asynchronous;

	BC_Title *vdevice_title;
	PlaybackAudioOffset *audio_offset;
	PlaybackGain *play_gain;
};

class PlaybackModuleFragment : public BC_PopupMenu
{
public:
	PlaybackModuleFragment(int x,
		int y,
		PreferencesWindow *pwindow,
		PlaybackPrefs *playback,
		char *text);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackAudioOffset : public BC_TumbleTextBox
{
public:
	PlaybackAudioOffset(PreferencesWindow *pwindow,
		PlaybackPrefs *subwindow,
		int x,
		int y);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};


class PlaybackViewFollows : public BC_CheckBox
{
public:
	PlaybackViewFollows(PreferencesWindow *pwindow, int value, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class PlaybackSoftwareTimer : public BC_CheckBox
{
public:
	PlaybackSoftwareTimer(PreferencesWindow *pwindow, int value, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class PlaybackRealTime : public BC_CheckBox
{
public:
	PlaybackRealTime(PreferencesWindow *pwindow, int value, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

// class VideoAsynchronous : public BC_CheckBox
// {
// public:
// 	VideoAsynchronous(PreferencesWindow *pwindow, int x, int y);
// 	int handle_event();
// 	PreferencesWindow *pwindow;
// };

class PlaybackMap51_2 : public BC_CheckBox
{
public:
	PlaybackMap51_2(PreferencesWindow *pwindow,
		PlaybackPrefs *playback_prefs, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback_prefs;
};

class VideoEveryFrame : public BC_CheckBox
{
public:
	VideoEveryFrame(PreferencesWindow *pwindow,
		PlaybackPrefs *playback_prefs, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback_prefs;
};

class PlaybackPreload : public BC_TextBox
{
public:
	PlaybackPreload(int x,
		int y,
		PreferencesWindow *pwindow,
		PlaybackPrefs *playback,
		char *text);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackInterpolateRaw : public BC_CheckBox
{
public:
	PlaybackInterpolateRaw(int x,
		int y,
		PreferencesWindow *pwindow,
		PlaybackPrefs *playback);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackWhiteBalanceRaw : public BC_CheckBox
{
public:
	PlaybackWhiteBalanceRaw(int x,
		int y,
		PreferencesWindow *pwindow,
		PlaybackPrefs *playback);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackSubtitle : public BC_CheckBox
{
public:
	PlaybackSubtitle(int x,
		int y,
		PreferencesWindow *pwindow,
		PlaybackPrefs *playback);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackSubtitleNumber : public BC_TumbleTextBox
{
public:
	PlaybackSubtitleNumber(int x, int y,
		PreferencesWindow *pwindow,
		PlaybackPrefs *playback);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackLabelCells : public BC_CheckBox
{
public:
	PlaybackLabelCells(int x, int y,
		PreferencesWindow *pwindow,
		PlaybackPrefs *playback);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackProgramNumber : public BC_TumbleTextBox
{
public:
	PlaybackProgramNumber(int x, int y,
		PreferencesWindow *pwindow,
		PlaybackPrefs *playback);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackGain : public BC_TumbleTextBox
{
public:
	PlaybackGain(int x, int y,
		PreferencesWindow *pwindow,
		PlaybackPrefs *gui);
	int handle_event();
	PreferencesWindow *pwindow;
};

#endif
