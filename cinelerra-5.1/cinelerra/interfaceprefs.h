
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

#ifndef INTERFACEPREFS_H
#define INTERFACEPREFS_H

#include "browsebutton.h"
#include "deleteallindexes.inc"
#include "interfaceprefs.inc"
#include "mwindow.inc"
#include "preferencesthread.h"
#include "probeprefs.inc"
#include "shbtnprefs.inc"


class InterfacePrefs : public PreferencesDialog
{
public:
	InterfacePrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~InterfacePrefs();

	void create_objects();
// must delete each derived class
	int update(int new_value);
	const char* behavior_to_text(int mode);
	int start_shbtn_dialog();
	void start_probe_dialog();

	BrowseButton *ipath;
	IndexSize *isize;
	IndexCount *icount;
	IndexPathText *ipathtext;
	DeleteAllIndexes *del_indexes;
	DeleteAllIndexes *del_clipngs;
	IndexFFMPEGMarkerFiles *ffmpeg_marker_files;

	ViewBehaviourText *button1, *button2, *button3;
	MeterMinDB *min_db;
	MeterMaxDB *max_db;

	ShBtnEditDialog *shbtn_dialog;
	KeyframeReticle *keyframe_reticle;
	PrefsYUV420P_DVDlace *yuv420p_dvdlace;
	FileProbeDialog *file_probe_dialog;
	PrefsFileProbes *file_probes;
	PrefsTrapSigSEGV *trap_segv;
	PrefsTrapSigINTR *trap_intr;
	SnapshotPathText *snapshot_path;
	PrefsReloadPlugins *reload_plugins;
};


class IndexPathText : public BC_TextBox
{
public:
	IndexPathText(int x, int y, PreferencesWindow *pwindow, char *text);
	~IndexPathText();
	int handle_event();
	PreferencesWindow *pwindow;
};

class IndexSize : public BC_TextBox
{
public:
	IndexSize(int x, int y, PreferencesWindow *pwindow, char *text);
	int handle_event();
	PreferencesWindow *pwindow;
};


class IndexCount : public BC_TextBox
{
public:
	IndexCount(int x, int y, PreferencesWindow *pwindow, char *text);
	int handle_event();
	PreferencesWindow *pwindow;
};


class ViewBehaviourText : public BC_PopupMenu
{
public:
	ViewBehaviourText(int x, int y, const char *text,
		PreferencesWindow *pwindow, int *output);
	~ViewBehaviourText();

	int handle_event();  // user copies text to value here
	void create_objects();         // add initial items
	InterfacePrefs *tfwindow;
	int *output;
};

class ViewBehaviourItem : public BC_MenuItem
{
public:
	ViewBehaviourItem(ViewBehaviourText *popup, char *text, int behaviour);
	~ViewBehaviourItem();

	int handle_event();
	ViewBehaviourText *popup;
	int behaviour;
};


class MeterMinDB : public BC_TextBox
{
public:
	MeterMinDB(PreferencesWindow *pwindow, char *text, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};


class MeterMaxDB : public BC_TextBox
{
public:
	MeterMaxDB(PreferencesWindow *pwindow, char *text, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class ScanCommercials : public BC_CheckBox
{
public:
	ScanCommercials(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class AndroidRemote : public BC_CheckBox
{
public:
	AndroidRemote(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class AndroidPIN : public BC_TextBox
{
public:
	PreferencesWindow *pwindow;
	int handle_event();
	AndroidPIN(PreferencesWindow *pwindow, int x, int y);
};

class AndroidPort : public BC_TextBox
{
public:
	PreferencesWindow *pwindow;
	int handle_event();
	AndroidPort(PreferencesWindow *pwindow, int x, int y);
};

class ShBtnPrefs : public BC_GenericButton
{
public:
	PreferencesWindow *pwindow;
	InterfacePrefs *iface_prefs;

	int handle_event();
	ShBtnPrefs(PreferencesWindow *pwindow,
		InterfacePrefs *iface_prefs, int x, int y);
};
class StillImageUseDuration : public BC_CheckBox
{
public:
	StillImageUseDuration(PreferencesWindow *pwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class StillImageDuration : public BC_TextBox
{
public:
	StillImageDuration(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class KeyframeReticle : public BC_PopupMenu
{
public:
	KeyframeReticle(PreferencesWindow *pwindow,
		InterfacePrefs *iface_prefs, int x, int y, int *output);
	~KeyframeReticle();

	const char *hairline_to_string(int type);
	void create_objects();

	PreferencesWindow *pwindow;
	InterfacePrefs *iface_prefs;
	int *output;
};

class HairlineItem : public BC_MenuItem
{
public:
	HairlineItem(KeyframeReticle *popup, int hairline);
	~HairlineItem();

	KeyframeReticle *popup;
	int handle_event();
	int hairline;
};

class IndexFFMPEGMarkerFiles : public BC_CheckBox
{
public:
	IndexFFMPEGMarkerFiles(InterfacePrefs *iface_prefs, int x, int y);
	~IndexFFMPEGMarkerFiles();

	int handle_event();

	InterfacePrefs *iface_prefs;
};


class PrefsTrapSigSEGV : public BC_CheckBox
{
public:
	PrefsTrapSigSEGV(InterfacePrefs *subwindow, int x, int y);
	~PrefsTrapSigSEGV();
	int handle_event();

	InterfacePrefs *subwindow;
};

class PrefsTrapSigINTR : public BC_CheckBox
{
public:
	PrefsTrapSigINTR(InterfacePrefs *subwindow, int x, int y);
	~PrefsTrapSigINTR();
	int handle_event();

	InterfacePrefs *subwindow;
};


class PrefsFileProbes : public BC_GenericButton
{
public:
	PreferencesWindow *pwindow;
	InterfacePrefs *subwindow;

	int handle_event();
	PrefsFileProbes(PreferencesWindow *pwindow, InterfacePrefs *subwindow, int x, int y);
};


class PrefsYUV420P_DVDlace : public BC_CheckBox
{
public:
	PrefsYUV420P_DVDlace(PreferencesWindow *pwindow,
		InterfacePrefs *subwindow, int x, int y);
	int handle_event();

	InterfacePrefs *subwindow;
	PreferencesWindow *pwindow;
};

class SnapshotPathText : public BC_TextBox
{
public:
	SnapshotPathText(PreferencesWindow *pwindow,
		InterfacePrefs *subwindow, int x, int y, int w);
	~SnapshotPathText();

	int handle_event();
	PreferencesWindow *pwindow;
	InterfacePrefs *subwindow;
};

class PrefsAutostartLV2UI : public BC_CheckBox
{
public:
	PrefsAutostartLV2UI(int x, int y, PreferencesWindow *pwindow);
	int handle_event();
	PreferencesWindow *pwindow;
};

class PrefsLV2PathText : public BC_TextBox
{
public:
	PrefsLV2PathText(PreferencesWindow *pwindow,
		InterfacePrefs *subwindow, int x, int y, int w);
	~PrefsLV2PathText();

	int handle_event();
	PreferencesWindow *pwindow;
	InterfacePrefs *subwindow;
};

class PrefsReloadPlugins : public BC_GenericButton
{
public:
	PreferencesWindow *pwindow;
	InterfacePrefs *iface_prefs;

	int handle_event();
	PrefsReloadPlugins(PreferencesWindow *pwindow,
		InterfacePrefs *iface_prefs, int x, int y);
};

#endif
