
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

#ifndef PERFORMANCEPREFS_H
#define PERFORMANCEPREFS_H


#include "formattools.inc"
#include "mwindow.inc"
#include "performanceprefs.inc"
#include "preferencesthread.h"
#include "probeprefs.inc"


class PerformancePrefs : public PreferencesDialog
{
public:
	PerformancePrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~PerformancePrefs();

	void create_objects();

	void generate_node_list();
	void update_node_list();
	void update_rates();
	void start_probe_dialog();

	int hot_node;

	CICacheSize *cache_size;

	enum
	{
		ENABLED_COLUMN,
		HOSTNAME_COLUMN,
		PORT_COLUMN,
		RATE_COLUMN,
		TOTAL_COLUMNS
	};

	ArrayList<BC_ListBoxItem*> nodes[TOTAL_COLUMNS];
	PrefsRenderFarmEditNode *edit_node;
	PrefsRenderFarmPort *edit_port;
	PrefsRenderFarmNodes *node_list;
	PrefsRenderFarmWatchdog *renderfarm_watchdog;
	FormatTools *brender_tools;
	BC_Title *master_rate;
};


class PrefsUseBRender : public BC_CheckBox
{
public:
	PrefsUseBRender(PreferencesWindow *pwindow,
		int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class PrefsBRenderFragment : public BC_TumbleTextBox
{
public:
	PrefsBRenderFragment(PreferencesWindow *pwindow,
		PerformancePrefs *subwindow,
		int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class PrefsRenderPreroll : public BC_TumbleTextBox
{
public:
	PrefsRenderPreroll(PreferencesWindow *pwindow,
		PerformancePrefs *subwindow,
		int x,
		int y);
	~PrefsRenderPreroll();

	int handle_event();

	PreferencesWindow *pwindow;
};

class PrefsBRenderPreroll : public BC_TumbleTextBox
{
public:
	PrefsBRenderPreroll(PreferencesWindow *pwindow,
		PerformancePrefs *subwindow,
		int x,
		int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class PrefsForceUniprocessor : public BC_CheckBox
{
public:
	PrefsForceUniprocessor(PreferencesWindow *pwindow, int x, int y);
	~PrefsForceUniprocessor();
	int handle_event();

	PreferencesWindow *pwindow;
};

class PrefsFFMPEGMarkerIndecies : public BC_CheckBox
{
public:
	PrefsFFMPEGMarkerIndecies(PerformancePrefs *perf_prefs, int x, int y);
	~PrefsFFMPEGMarkerIndecies();

	int handle_event();

	PerformancePrefs *perf_prefs;
};

class PrefsRenderFarm : public BC_CheckBox
{
public:
	PrefsRenderFarm(PreferencesWindow *pwindow, int x, int y);
	~PrefsRenderFarm();

	int handle_event();

	PreferencesWindow *pwindow;
};

class PrefsRenderFarmConsolidate : public BC_CheckBox
{
public:
	PrefsRenderFarmConsolidate(PreferencesWindow *pwindow, int x, int y);
	~PrefsRenderFarmConsolidate();

	int handle_event();

	PreferencesWindow *pwindow;
};


class PrefsRenderFarmPort : public BC_TumbleTextBox
{
public:
	PrefsRenderFarmPort(PreferencesWindow *pwindow,
		PerformancePrefs *subwindow,
		int x, int y);
	~PrefsRenderFarmPort();

	int handle_event();

	PreferencesWindow *pwindow;
};

class PrefsProjectSMP : public BC_TumbleTextBox
{
public:
	PrefsProjectSMP(PreferencesWindow *pwindow,
		PerformancePrefs *subwindow, int x, int y);
	~PrefsProjectSMP();

	int handle_event();
	PreferencesWindow *pwindow;
};

class PrefsRenderFarmJobs : public BC_TumbleTextBox
{
public:
	PrefsRenderFarmJobs(PreferencesWindow *pwindow,
		PerformancePrefs *subwindow,
		int x, int y);
	~PrefsRenderFarmJobs();

	int handle_event();

	PreferencesWindow *pwindow;
};

class PrefsRenderFarmMountpoint : public BC_TextBox
{
public:
	PrefsRenderFarmMountpoint(PreferencesWindow *pwindow,
		PerformancePrefs *subwindow,
		int x, int y);
	~PrefsRenderFarmMountpoint();

	int handle_event();

	PreferencesWindow *pwindow;
	PerformancePrefs *subwindow;
};

class PrefsRenderFarmVFS : public BC_CheckBox
{
public:
	PrefsRenderFarmVFS(PreferencesWindow *pwindow,
		PerformancePrefs *subwindow,
		int x,
		int y);
	int handle_event();
	PreferencesWindow *pwindow;
	PerformancePrefs *subwindow;
};

class PrefsRenderFarmNodes : public BC_ListBox
{
public:
	PrefsRenderFarmNodes(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y);
	~PrefsRenderFarmNodes();

	int handle_event();
	int selection_changed();
	int column_resize_event();
	void update_list();

	const char *titles[PerformancePrefs::TOTAL_COLUMNS];
	int widths[PerformancePrefs::TOTAL_COLUMNS];

	PreferencesWindow *pwindow;
	PerformancePrefs *subwindow;
};

class PrefsRenderFarmEditNode : public BC_TextBox
{
public:
	PrefsRenderFarmEditNode(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y);
	~PrefsRenderFarmEditNode();

	int handle_event();

	PerformancePrefs *subwindow;
	PreferencesWindow *pwindow;
};

class PrefsRenderFarmNewNode : public BC_GenericButton
{
public:
	PrefsRenderFarmNewNode(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y);
	~PrefsRenderFarmNewNode();

	int handle_event();


	PerformancePrefs *subwindow;
	PreferencesWindow *pwindow;
};

class PrefsRenderFarmReplaceNode : public BC_GenericButton
{
public:
	PrefsRenderFarmReplaceNode(PreferencesWindow *pwindow,
		PerformancePrefs *subwindow,
		int x,
		int y);
	~PrefsRenderFarmReplaceNode();

	int handle_event();


	PerformancePrefs *subwindow;
	PreferencesWindow *pwindow;
};

class PrefsRenderFarmDelNode : public BC_GenericButton
{
public:
	PrefsRenderFarmDelNode(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y);
	~PrefsRenderFarmDelNode();

	int handle_event();

	PerformancePrefs *subwindow;
	PreferencesWindow *pwindow;
};

class PrefsRenderFarmSortNodes : public BC_GenericButton
{
public:
	PrefsRenderFarmSortNodes(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y);
	~PrefsRenderFarmSortNodes();

	int handle_event();

	PerformancePrefs *subwindow;
	PreferencesWindow *pwindow;
};

class PrefsRenderFarmReset : public BC_GenericButton
{
public:
	PrefsRenderFarmReset(PreferencesWindow *pwindow,
		PerformancePrefs *subwindow, int x, int y);

	int handle_event();

	PerformancePrefs *subwindow;
	PreferencesWindow *pwindow;
};

class PrefsRenderFarmWatchdog : public BC_TumbleTextBox
{
public:
	PrefsRenderFarmWatchdog(PreferencesWindow *pwindow,
		PerformancePrefs *subwindow, int x, int y);

	int handle_event();

	PerformancePrefs *subwindow;
	PreferencesWindow *pwindow;
};


class CICacheSize : public BC_TumbleTextBox
{
public:
	CICacheSize(int x, int y,
		PreferencesWindow *pwindow,
		PerformancePrefs *subwindow);

	int handle_event();

	PreferencesWindow *pwindow;
};


#endif
