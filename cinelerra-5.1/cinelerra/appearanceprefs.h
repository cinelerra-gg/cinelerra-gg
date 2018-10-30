
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

#ifndef APPEARANCEPREFS_H
#define APPEARANCEPREFS_H

#include "appearanceprefs.inc"
#include "browsebutton.h"
#include "deleteallindexes.inc"
#include "mwindow.inc"
#include "preferencesthread.h"
#include "shbtnprefs.inc"


class AppearancePrefs : public PreferencesDialog
{
public:
	AppearancePrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~AppearancePrefs();

	void create_objects();

	int update(int new_value);
	TimeFormatHMS *hms;
	TimeFormatHMSF *hmsf;
	TimeFormatSamples *samples;
	TimeFormatHex *hex;
	TimeFormatFrames *frames;
	TimeFormatFeet *feet;
	TimeFormatSeconds *seconds;
	ViewThumbnails *thumbnails;
	YuvColorSpace *yuv_color_space;
	YuvColorRange *yuv_color_range;
};


class TimeFormatHMS : public BC_Radial
{
public:
	TimeFormatHMS(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	AppearancePrefs *tfwindow;
};

class TimeFormatHMSF : public BC_Radial
{
public:
	TimeFormatHMSF(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	AppearancePrefs *tfwindow;
};

class TimeFormatSamples : public BC_Radial
{
public:
	TimeFormatSamples(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	AppearancePrefs *tfwindow;
};

class TimeFormatFrames : public BC_Radial
{
public:
	TimeFormatFrames(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	AppearancePrefs *tfwindow;
};

class TimeFormatHex : public BC_Radial
{
public:
	TimeFormatHex(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	AppearancePrefs *tfwindow;
};

class TimeFormatFeet : public BC_Radial
{
public:
	TimeFormatFeet(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	AppearancePrefs *tfwindow;
};

class TimeFormatSeconds : public BC_Radial
{
public:
	TimeFormatSeconds(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	AppearancePrefs *tfwindow;
};

class TimeFormatFeetSetting : public BC_TextBox
{
public:
	TimeFormatFeetSetting(PreferencesWindow *pwindow, int x, int y, char *string);
	int handle_event();
	PreferencesWindow *pwindow;
};



class ViewTheme : public BC_PopupMenu
{
public:
	ViewTheme(int x, int y, PreferencesWindow *pwindow);
	~ViewTheme();

	void create_objects();
	int handle_event();

	PreferencesWindow *pwindow;
};

class ViewThemeItem : public BC_MenuItem
{
public:
	ViewThemeItem(ViewTheme *popup, const char *text);
	int handle_event();
	ViewTheme *popup;
};

class ViewPluginIcons : public BC_PopupMenu
{
public:
	ViewPluginIcons(int x, int y, PreferencesWindow *pwindow);
	~ViewPluginIcons();

	void create_objects();
	int handle_event();

	PreferencesWindow *pwindow;
};

class ViewPluginIconItem : public BC_MenuItem
{
public:
	ViewPluginIconItem(ViewPluginIcons *popup, const char *text);
	int handle_event();
	ViewPluginIcons *popup;
};

class ViewThumbnails : public BC_CheckBox
{
public:
	ViewThumbnails(int x, int y, PreferencesWindow *pwindow);
	int handle_event();
	PreferencesWindow *pwindow;
};

class UseTipWindow : public BC_CheckBox
{
public:
	UseTipWindow(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class UseWarnIndecies : public BC_CheckBox
{
public:
	UseWarnIndecies(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class UseWarnVersion : public BC_CheckBox
{
public:
	UseWarnVersion(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class BD_WarnRoot : public BC_CheckBox
{
public:
	BD_WarnRoot(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class PopupMenuBtnup : public BC_CheckBox
{
public:
	PopupMenuBtnup(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class GrabFocusPolicy : public BC_CheckBox
{
public:
	GrabFocusPolicy(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class ActivateFocusPolicy : public BC_CheckBox
{
public:
	ActivateFocusPolicy(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class DeactivateFocusPolicy : public BC_CheckBox
{
public:
	DeactivateFocusPolicy(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class ForwardRenderDisplacement : public BC_CheckBox
{
public:
	ForwardRenderDisplacement(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class HighlightInverseColor : public BC_TextBox
{
public:
	HighlightInverseColor(PreferencesWindow *pwindow, int x, int y, const char *hex);
	int handle_event();
	PreferencesWindow *pwindow;
};

class YuvColorSpace : public BC_PopupMenu
{
	static const char *color_space[3];
public:
	YuvColorSpace(int x, int y, PreferencesWindow *pwindow);
	~YuvColorSpace();

	void create_objects();
	int handle_event();

	PreferencesWindow *pwindow;
};

class YuvColorSpaceItem : public BC_MenuItem
{
public:
	YuvColorSpaceItem(YuvColorSpace *popup, const char *text, int id);
	int handle_event();
	YuvColorSpace *popup;
	int id;
};

class YuvColorRange : public BC_PopupMenu
{
	static const char *color_range[2];
public:
	YuvColorRange(int x, int y, PreferencesWindow *pwindow);
	~YuvColorRange();

	void create_objects();
	int handle_event();

	PreferencesWindow *pwindow;
};

class YuvColorRangeItem : public BC_MenuItem
{
public:
	YuvColorRangeItem(YuvColorRange *popup, const char *text, int id);
	int handle_event();
	YuvColorRange *popup;
	int id;
};

class PerpetualSession : public BC_CheckBox
{
public:
	PerpetualSession(int x, int y, PreferencesWindow *pwindow);
	int handle_event();
	PreferencesWindow *pwindow;
};

#endif
