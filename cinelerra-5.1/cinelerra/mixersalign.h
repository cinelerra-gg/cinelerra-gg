/*
 * CINELERRA
 * Copyright (C) 2008-2015 Adam Williams <broadcast at earthling dot net>
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

#ifndef __MIXERSALIGN_H__
#define __MIXERSALIGN_H__

#include "edl.inc"
#include "fourier.h"
#include "guicast.h"
#include "language.h"
#include "loadbalance.h"
#include "mainprogress.inc"
#include "mixersalign.inc"
#include "mwindow.inc"
#include "renderengine.h"
#include "samples.inc"
#include "track.inc"
#include "zwindow.inc"

class MixersAlignMixer
{
public:
	MixersAlignMixer(Mixer *mix);
	Mixer *mixer;
	double nudge;
	double mx;
	int64_t mi;
	int aidx;
};

class MixersAlignMixers : public ArrayList<MixersAlignMixer*>
{
public:
	MixersAlignMixers() {}
	~MixersAlignMixers() { remove_all_objects(); }
};

class MixersAlignMixerList : public BC_ListBox
{
	enum { MIX_MIXER, MIX_NUDGE, MIX_SZ };
	static const char *mix_titles[MIX_SZ];
	static int mix_widths[MIX_SZ];
public:
	MixersAlignMixerList(MixersAlignWindow *gui,
		MixersAlign *dialog, int x, int y, int w, int h);
	~MixersAlignMixerList();

	void clear();

	void add_mixer(MixersAlignMixer *mixer);
	void load_list();
	void update();
	int selection_changed();

	MixersAlignWindow *gui;
	MixersAlign *dialog;
	MixersAlignMixers mixers;

	const char *col_titles[MIX_SZ];
	int col_widths[MIX_SZ];
	ArrayList<BC_ListBoxItem*> cols[MIX_SZ];

	void set_all_selected(int v) { BC_ListBox::set_all_selected(cols, v); }
	void set_selected(int idx, int v) { BC_ListBox::set_selected(cols, idx, v); }
	bool is_selected(int idx) { return cols[0][idx]->get_selected() != 0; }
};


class MixersAlignMTrack
{
public:
	MixersAlignMTrack(Track *trk, int no);

	Track *track;
	int no;
};

class MixersAlignMTracks : public ArrayList<MixersAlignMTrack*>
{
public:
	MixersAlignMTracks() {}
	~MixersAlignMTracks() { remove_all_objects(); }
};


class MixersAlignMTrackList : public BC_ListBox
{
	enum { MTK_NO, MTK_MIXER, MTK_TRACK, MTK_SZ };
	static const char *mtk_titles[MTK_SZ];
	static int mtk_widths[MTK_SZ];
public:
	MixersAlignMTrackList(MixersAlignWindow *gui,
		MixersAlign *dialog, int x, int y, int w, int h);
	~MixersAlignMTrackList();

	void clear();
	void add_mtrack(MixersAlignMTrack *mtrk);
	void load_list();
	void update();

	MixersAlignWindow *gui;
	MixersAlign *dialog;

	ArrayList<BC_ListBoxItem*> cols[MTK_SZ];
	const char *col_titles[MTK_SZ];
	int col_widths[MTK_SZ];

	void set_all_selected(int v) { BC_ListBox::set_all_selected(cols, v); }
	void set_selected(int idx, int v) { BC_ListBox::set_selected(cols, idx, v); }
	bool is_selected(int idx) { return cols[0][idx]->get_selected() != 0; }
};


class MixersAlignATrack
{
public:
	MixersAlignATrack(Track *trk, int no);
	Track *track;
	int no;
	double nudge;
	double mx;
	int64_t mi;
};

class MixersAlignATracks : public ArrayList<MixersAlignATrack*>
{
public:
	MixersAlignATracks() {}
	~MixersAlignATracks() { remove_all_objects(); }
};

class MixersAlignATrackList : public BC_ListBox
{
	enum { ATK_TRACK, ATK_AUDIO, ATK_NUDGE, ATK_MX, ATK_MI, ATK_SZ };
	static const char *atk_titles[ATK_SZ];
	static int atk_widths[ATK_SZ];
public:
	MixersAlignATrackList(MixersAlignWindow *gui,
		MixersAlign *dialog, int x, int y, int w, int h);
	~MixersAlignATrackList();

	void clear();
	void add_atrack(MixersAlignATrack *track);
	void load_list();
	void update();
	int selection_changed();

	MixersAlignWindow *gui;
	MixersAlign *dialog;

	ArrayList<BC_ListBoxItem*> cols[ATK_SZ];
	const char *col_titles[ATK_SZ];
	int col_widths[ATK_SZ];

	void set_all_selected(int v) { BC_ListBox::set_all_selected(cols, v); }
	void set_selected(int idx, int v) { BC_ListBox::set_selected(cols, idx, v); }
	bool is_selected(int idx) { return cols[0][idx]->get_selected() != 0; }
};

class MixersAlignReset : public BC_GenericButton
{
public:
	MixersAlignReset(MixersAlignWindow *gui, MixersAlign *dialog, int x, int y);
	int handle_event();
	static int calculate_width(BC_WindowBase *gui);

	MixersAlign *dialog;
	MixersAlignWindow *gui;
};

class MixersAlignThread : public Thread
{
public:
	MixersAlignThread(MixersAlign *dialog);
	~MixersAlignThread();
	void run();

	MixersAlign *dialog;
};

class MixersAlignMatch : public BC_GenericButton
{
public:
	MixersAlignMatch(MixersAlignWindow *gui, MixersAlign *dialog, int x, int y);
	int handle_event();

	MixersAlign *dialog;
	MixersAlignWindow *gui;
};

class MixersAlignApply : public BC_GenericButton
{
public:
	MixersAlignApply(MixersAlignWindow *gui, MixersAlign *dialog, int x, int y);
	int handle_event();
	static int calculate_width(BC_WindowBase *gui);

	MixersAlign *dialog;
	MixersAlignWindow *gui;
};

class MixersAlignUndo : public BC_GenericButton
{
public:
	MixersAlignUndo(MixersAlignWindow *gui, MixersAlign *dialog, int x, int y);
	int handle_event();

	MixersAlign *dialog;
	MixersAlignWindow *gui;
};


class MixersAlignWindow : public BC_Window
{
public:
	MixersAlignWindow(MixersAlign *dialog, int x, int y);
	~MixersAlignWindow();

	void create_objects();
	int resize_event(int w, int h);
	void load_lists();
	void default_selection();
	void update_gui();

	MixersAlign *dialog;
	BC_Title *mixer_title, *mtrack_title, *atrack_title;

	MixersAlignMixerList *mixer_list;
	MixersAlignMTrackList *mtrack_list;
	MixersAlignATrackList *atrack_list;
	MixersAlignMatch *match;
	MixersAlignReset *reset;
	MixersAlignApply *apply;
	MixersAlignUndo *undo;
};


class MixersAlignARender : public RenderEngine
{
public:
	MixersAlignARender(MWindow *mwindow, EDL *edl);
	~MixersAlignARender();

	int render(Samples **samples, int64_t len, int64_t pos);
};

class MixersAlignPackage : public LoadPackage
{
public:
	MixersAlignPackage();
	~MixersAlignPackage();

	MixersAlignMixer *mixer;
};

class MixersAlignClient : public LoadClient
{
public:
	MixersAlignClient(MixersAlignFarm *farm);
	~MixersAlignClient();

        void process_package(LoadPackage *pkg);
};

class MixersAlignFarm : public LoadServer
{
public:
	MixersAlignFarm(MixersAlign *dialog, int n);
	~MixersAlignFarm();
	void init_packages();
	LoadClient *new_client();
	LoadPackage *new_package();

	MixersAlign *dialog;
};

class MixersAlign : public BC_DialogThread, public FFT
{
public:
	MixersAlign(MWindow *mwindow);
	~MixersAlign();

	void start_dialog(int wx, int wy);
	BC_Window *new_gui();
	void load_mixers();
	void load_mtracks();
	void load_atracks();
	void handle_done_event(int result);
	void handle_close_event(int result);

	int mixer_of(Track *track, int &midx);
	int mixer_of(Track *track) { int midx = -1; return mixer_of(track, midx); }
	int mmixer_of(int mi) {
		return mi>=0 && mi<mtracks.size() ? mixer_of(mtracks[mi]->track) : -1;
	}
	int amixer_of(int ai) {
		return ai>=0 && ai<atracks.size() ? mixer_of(atracks[ai]->track) : -1;
	}

	EDL *mixer_audio_clip(Mixer *mixer);
	EDL *mixer_master_clip(Track *track);
	int64_t mixer_tracks_total();
	void load_master_audio(Track *track);
	void scan_mixer_audio();
	void update_progress(int64_t len);
	void update_match();
	void update();
	void process_package(MixersAlignFarm *farm, MixersAlignPackage *package);
	void apply();

	MixersAlignWindow *ma_gui;
	int wx, wy;
	MixersAlignMixers mixers;
	MixersAlignMTracks mtracks;
	MixersAlignATracks atracks;
	MWindow *mwindow;

	EDL *undo_edl;
	Mutex *farming;
	MainProgressBar *progress;
	MixersAlignThread *thread;
	Mutex *total_lock;
	int64_t total_rendered;
	int failed;
	int64_t master_len;
	double *master_r, *master_i;
	double master_start, master_end, master_ss;
	double audio_start, audio_end;
};

#endif
