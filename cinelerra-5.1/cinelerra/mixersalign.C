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

#include "mixersalign.h"
#include "auto.h"
#include "autos.h"
#include "arender.h"
#include "clip.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainprogress.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "panauto.h"
#include "panautos.h"
#include "preferences.h"
#include "track.h"
#include "tracks.h"
#include "renderengine.h"
#include "samples.h"
#include "track.h"
#include "transportque.h"
#include "zwindow.h"

MixersAlignMixer::MixersAlignMixer(Mixer *mix)
{
	this->mixer = mix;
	this->nudge = 0;
	mx = 0;  mi = -1;
	aidx = -1;
}

const char *MixersAlignMixerList::mix_titles[MIX_SZ] = {
	N_("Mixer"), N_("Nudge"),
};

int MixersAlignMixerList::mix_widths[MIX_SZ] = {
	130, 50,
};

MixersAlignMixerList::MixersAlignMixerList(MixersAlignWindow *gui,
		MixersAlign *dialog, int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT)
{
	set_selection_mode(LISTBOX_MULTIPLE);
	this->dialog = dialog;
	this->gui = gui;
	for( int i=MIX_SZ; --i>=0; ) {
		col_widths[i] = mix_widths[i];
		col_titles[i] = _(mix_titles[i]);
	}
}

MixersAlignMixerList::~MixersAlignMixerList()
{
	clear();
}

void MixersAlignMixerList::clear()
{
	for( int i=MIX_SZ; --i>=0; )
		cols[i].remove_all_objects();
}

void MixersAlignMixerList::add_mixer(MixersAlignMixer *mixer)
{
	char mixer_text[BCSTRLEN];
	snprintf(mixer_text, sizeof(mixer_text), "%d: %s",
		mixer->mixer->idx, mixer->mixer->title);
	cols[MIX_MIXER].append(new BC_ListBoxItem(mixer_text));
	char nudge_text[BCSTRLEN];
	sprintf(nudge_text, _("%0.4f"), mixer->nudge);
	cols[MIX_NUDGE].append(new BC_ListBoxItem(nudge_text));
}

void MixersAlignMixerList::load_list()
{
	clear();
	for( int i=0,sz=dialog->mixers.size(); i<sz; ++i )
		add_mixer(dialog->mixers[i]);
}

void MixersAlignMixerList::update()
{
	int xpos = get_xposition(), ypos = get_yposition();
	BC_ListBox::update(&cols[0], &col_titles[0],&col_widths[0],MIX_SZ, xpos,ypos);
}

int MixersAlignMixerList::selection_changed()
{
	for( int m=0; m<dialog->mixers.size(); ++m ) {
		MixersAlignMixer *mix = dialog->mixers[m];
		if( !is_selected(m) ) {
			for( int i=0; i<dialog->atracks.size(); ++i ) {
				if( m != dialog->amixer_of(i) ) continue;
				gui->atrack_list->set_selected(i, 0);
			}
			mix->aidx = -1;
			mix->mx = 0;
			mix->mi = -1;
			mix->nudge = 0;
		}
		else if( mix->aidx < 0 ) {
			MixersAlignATrack *best = 0;  int idx = -1;
			for( int i=0; i<dialog->atracks.size(); ++i ) {
				if( m != dialog->amixer_of(i) ) continue;
				MixersAlignATrack *atrk = dialog->atracks[i];
				if( atrk->mi < 0 ) continue;
				gui->atrack_list->set_selected(i, 0);
				if( best && best->mx >= atrk->mx ) continue;
				best = atrk;  idx = i;
			}
			if( idx >= 0 && best ) {
				gui->atrack_list->set_selected(idx, 1);
				MixersAlignMixer *mix = dialog->mixers[m];
				mix->aidx = idx;
				mix->mx = best->mx;
				mix->mi = best->mi;
				mix->nudge = best->nudge;
			}
			else {
				for( int i=0; i<dialog->atracks.size(); ++i ) {
					if( m != dialog->amixer_of(i) ) continue;
					gui->atrack_list->set_selected(i, 1);
				}
			}
		}
	}
	gui->atrack_list->update();

	load_list();
	for( int i=0; i<dialog->atracks.size(); ++i ) {
		if( !gui->atrack_list->is_selected(i) ) continue;
		int m = dialog->amixer_of(i);
		if( m >= 0 ) set_selected(m, 1);
	}
	update();
	return 0;
}

MixersAlignMTrack::MixersAlignMTrack(Track *trk, int no)
{
	this->track = trk;
	this->no = no;
}


const char *MixersAlignMTrackList::mtk_titles[MTK_SZ] = {
	N_("No"), N_("Mixer"), N_("Track"),
};

int MixersAlignMTrackList::mtk_widths[MTK_SZ] = {
	32, 48, 110,
};

MixersAlignMTrackList::MixersAlignMTrackList(MixersAlignWindow *gui,
		MixersAlign *dialog, int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT)
{
	set_selection_mode(LISTBOX_SINGLE);
	this->dialog = dialog;
	this->gui = gui;
	for( int i=MTK_SZ; --i>=0; ) {
		col_widths[i] = mtk_widths[i];
		col_titles[i] = _(mtk_titles[i]);
	}
}

MixersAlignMTrackList::~MixersAlignMTrackList()
{
	clear();
}

void MixersAlignMTrackList::clear()
{
	for( int i=MTK_SZ; --i>=0; )
		cols[i].remove_all_objects();
}

void MixersAlignMTrackList::add_mtrack(MixersAlignMTrack *mtrk)
{
	char no_text[BCSTRLEN];
	snprintf(no_text, sizeof(no_text),"%d", mtrk->no+1);
	cols[MTK_NO].append(new BC_ListBoxItem(no_text));
	char mixer_text[BCSTRLEN];
	Track *track = mtrk->track;
	int midx = -1, m = dialog->mixer_of(track, midx);
	snprintf(mixer_text, sizeof(mixer_text),"%d:%d", m+1,midx);
	cols[MTK_MIXER].append(new BC_ListBoxItem(mixer_text));
	cols[MTK_TRACK].append(new BC_ListBoxItem(track->title));
}

void MixersAlignMTrackList::load_list()
{
	clear();
	for( int i=0,sz=dialog->mtracks.size(); i<sz; ++i )
		add_mtrack(dialog->mtracks[i]);
}

void MixersAlignMTrackList::update()
{
	int xpos = get_xposition(), ypos = get_yposition();
	BC_ListBox::update(&cols[0], &col_titles[0],&col_widths[0],MTK_SZ, xpos,ypos);
}


MixersAlignATrack::MixersAlignATrack(Track *trk, int no)
{
	this->track = trk;
	this->no = no;
	this->nudge = 0;
	mx = 0;  mi = -1;
}


const char *MixersAlignATrackList::atk_titles[ATK_SZ] = {
	N_("Track"), N_("Audio"), N_("Nudge"), N_("R"), N_("pos"),
};

int MixersAlignATrackList::atk_widths[ATK_SZ] = {
	40, 100, 60, 60, 60,
};

MixersAlignATrackList::MixersAlignATrackList(MixersAlignWindow *gui,
		MixersAlign *dialog, int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT)
{
	set_selection_mode(LISTBOX_MULTIPLE);
	this->dialog = dialog;
	this->gui = gui;
	for( int i=ATK_SZ; --i>=0; ) {
		col_widths[i] = atk_widths[i];
		col_titles[i] = _(atk_titles[i]);
	}
}

MixersAlignATrackList::~MixersAlignATrackList()
{
	clear();
}

void MixersAlignATrackList::clear()
{
	for( int i=ATK_SZ; --i>=0; )
		cols[i].remove_all_objects();
}

void MixersAlignATrackList::add_atrack(MixersAlignATrack *atrack)
{
	char atrack_text[BCSTRLEN];
	Track *track = atrack->track;
	int midx = -1, m = dialog->mixer_of(track, midx);
	snprintf(atrack_text, sizeof(atrack_text), "%d/%d", atrack->no+1,m+1);
	cols[ATK_TRACK].append(new BC_ListBoxItem(atrack_text));
	cols[ATK_AUDIO].append(new BC_ListBoxItem(track->title));
	char nudge_text[BCSTRLEN];
	sprintf(nudge_text, "%0.4f", atrack->nudge);
	cols[ATK_NUDGE].append(new BC_ListBoxItem(nudge_text));
	char mx_text[BCSTRLEN];
	sprintf(mx_text, "%0.4f", atrack->mx);
	cols[ATK_MX].append(new BC_ListBoxItem(mx_text));
	char mi_text[BCSTRLEN];
	sprintf(mi_text, "%jd", atrack->mi);
	cols[ATK_MI].append(new BC_ListBoxItem(mi_text));
}


void MixersAlignATrackList::load_list()
{
	clear();
	for( int i=0,sz=dialog->atracks.size(); i<sz; ++i )
		add_atrack(dialog->atracks[i]);
}

void MixersAlignATrackList::update()
{
	int xpos = get_xposition(), ypos = get_yposition();
	BC_ListBox::update(&cols[0], &col_titles[0],&col_widths[0],ATK_SZ, xpos,ypos);
}

int MixersAlignATrackList::selection_changed()
{
	int idx = get_buttonpress() == LEFT_BUTTON ? get_highlighted_item() : -1;
	int m = idx >= 0 ? dialog->amixer_of(idx) : -1;
	if( m >= 0 ) {
		int sel = 0;
		MixersAlignMixer *mix = dialog->mixers[m];
		for( int i=0; i<dialog->atracks.size(); ++i ) {
			int k = dialog->amixer_of(i);
			if( k < 0 ) { set_selected(i, 0);  continue; }
			if( m != k ) continue;
			MixersAlignATrack *atrk = dialog->atracks[i];
			if( atrk->mi < 0 ) continue;
			int is_sel = is_selected(i);
			set_selected(i, 0);
			if( i != idx ) continue;
			if( is_sel ) {
				mix->aidx = idx;
				mix->mx = atrk->mx;
				mix->mi = atrk->mi;
				mix->nudge = atrk->nudge;
				sel = 1;
			}
			else {
				mix->aidx = -1;
				mix->mx = 0;
				mix->mi = -1;
				mix->nudge = 0;
			}
		}
		if( sel )
			set_selected(idx, 1);
	}
	update();

	gui->mixer_list->load_list();
	for( int i=0; i<dialog->atracks.size(); ++i ) {
		if( !is_selected(i) ) continue;
		int m = dialog->amixer_of(i);
		if( m < 0 ) continue;
		gui->mixer_list->set_selected(m, 1);
	}
	gui->mixer_list->update();
	return 0;
}

MixersAlignReset::MixersAlignReset(MixersAlignWindow *gui,
		MixersAlign *dialog, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->gui = gui;
	this->dialog = dialog;
}

int MixersAlignReset::calculate_width(BC_WindowBase *gui)
{
	return BC_GenericButton::calculate_w(gui, _("Reset"));
}

int MixersAlignReset::handle_event()
{
	dialog->load_mixers();
	dialog->load_mtracks();
	dialog->load_atracks();
	gui->load_lists();
	gui->default_selection();
	return 1;
}

MixersAlignMatch::MixersAlignMatch(MixersAlignWindow *gui,
		MixersAlign *dialog, int x, int y)
 : BC_GenericButton(x, y, _("Match"))
{
	this->gui = gui;
	this->dialog = dialog;
}

int MixersAlignMatch::handle_event()
{
	if( !dialog->thread->running() ) {
		gui->reset->disable();
		gui->match->disable();
		gui->apply->disable();
		gui->undo->disable();
		dialog->thread->start();
	}
	return 1;
}

MixersAlignApply::MixersAlignApply(MixersAlignWindow *gui,
		MixersAlign *dialog, int x, int y)
 : BC_GenericButton(x, y, _("Apply"))
{
	this->gui = gui;
	this->dialog = dialog;
}

int MixersAlignApply::calculate_width(BC_WindowBase *gui)
{
	return BC_GenericButton::calculate_w(gui, _("Apply"));
}

int MixersAlignApply::handle_event()
{
	dialog->apply();
	return 1;
}

MixersAlignUndo::MixersAlignUndo(MixersAlignWindow *gui,
		MixersAlign *dialog, int x, int y)
 : BC_GenericButton(x, y, _("Undo"))
{
	this->gui = gui;
	this->dialog = dialog;
}

int MixersAlignUndo::handle_event()
{
	MWindow *mwindow = dialog->mwindow;
	mwindow->edl->copy_all(dialog->undo_edl);
	mwindow->gui->lock_window("MixersAlignUndo::handle_event");
	mwindow->update_gui(1);
	mwindow->gui->unlock_window();
	return gui->reset->handle_event();
}


MixersAlignWindow::MixersAlignWindow(MixersAlign *dialog, int x, int y)
 : BC_Window(_("Align Mixers"), x, y, 880, 380, 880, 380, 1)
{
	this->dialog = dialog;
}
MixersAlignWindow::~MixersAlignWindow()
{
}

void MixersAlignWindow::create_objects()
{
	int x = 10, y = 10, w4 = (get_w()-x-10)/4, lw = w4 + 20;
	int x1 = x, x2 = x1 + lw , x3 = x2 + lw, x4 = get_w()-x;
	mixer_title = new BC_Title(x1,y, _("Mixers:"), MEDIUMFONT, YELLOW);
	add_subwindow(mixer_title);
	mtrack_title = new BC_Title(x2,y, _("Master Track:"), MEDIUMFONT, YELLOW);
	add_subwindow(mtrack_title);
	atrack_title = new BC_Title(x3,y, _("Audio Tracks:"), MEDIUMFONT, YELLOW);
	add_subwindow(atrack_title);
	y += mixer_title->get_h() + 10;
	int y1 = y, y2 = get_h() - BC_OKButton::calculate_h() - 15;
	int lh = y2 - y1;
	mixer_list = new MixersAlignMixerList(this, dialog, x1, y, x2-x1-20, lh);
	add_subwindow(mixer_list);
	mtrack_list = new MixersAlignMTrackList(this, dialog, x2, y, x3-x2-20, lh);
	add_subwindow(mtrack_list);
	atrack_list = new MixersAlignATrackList(this, dialog, x3, y, x4-x3-20, lh);
	add_subwindow(atrack_list);
	int xr = x2-10 - MixersAlignReset::calculate_width(this);
	add_subwindow(reset = new MixersAlignReset(this, dialog, xr, y2+20));
	add_subwindow(match = new MixersAlignMatch(this, dialog, x2+10, y2+20));
	int xa = x3-10 - MixersAlignApply::calculate_width(this);
	add_subwindow(apply = new MixersAlignApply(this, dialog, xa, y2+20));
	add_subwindow(undo = new MixersAlignUndo(this, dialog, x3+10, y2+20));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
}

int MixersAlignWindow::resize_event(int w, int h)
{
	int x = 10, y = 10, w4 = (w-x-10)/4, lw = w4 + 20;
	int x1 = x, x2 = x1 + lw , x3 = x2 + lw, x4 = w-x;
	mixer_title->reposition_window(x1, y);
	mtrack_title->reposition_window(x2, y);
	atrack_title->reposition_window(x3, y);
	y += mixer_title->get_h() + 10;
	int y1 = y, y2 = h - BC_OKButton::calculate_h() - 15;
	int lh = y2 - y1;
	mixer_list->reposition_window(x1, y, x2-x1-20, lh);
	mtrack_list->reposition_window(x2, y, x3-x2-20, lh);
	atrack_list->reposition_window(x3, y, x4-x3-20, lh);
	int xr = x2-10 - MixersAlignReset::calculate_width(this);
	reset->reposition_window(xr, y2+20);
	match->reposition_window(x2+10, y2+20);
	int xa = x3-10 - MixersAlignApply::calculate_width(this);
	apply->reposition_window(xa, y2+20);
	undo->reposition_window(x3+10, y2+20);
	return 0;
}

void MixersAlignWindow::load_lists()
{
	mixer_list->load_list();
	mtrack_list->load_list();
	atrack_list->load_list();
}

void MixersAlignWindow::default_selection()
{
// mixers selects all mixers
	mixer_list->set_all_selected(1);
// master selects first mixer audio track
	for( int i=0; i<dialog->mtracks.size(); ++i ) {
		if( dialog->mmixer_of(i) >= 0 ) {
			mtrack_list->set_selected(i, 1);
			break;
		}
	}
// audio selects all mixer audio tracks
	for( int i=0; i<dialog->atracks.size(); ++i ) {
		if( dialog->amixer_of(i) >= 0 )
			atrack_list->set_selected(i, 1);
	}
	update_gui();
}

void MixersAlignWindow::update_gui()
{
	mixer_list->update();
	mtrack_list->update();
	atrack_list->update();
}


MixersAlign::MixersAlign(MWindow *mwindow)
{
	this->mwindow = mwindow;
	this->ma_gui = 0;
	farming = new Mutex("MixersAlign::farming");
	total_lock = new Mutex("MixersAlign::total_lock");
	thread = new MixersAlignThread(this);
	total_rendered = 0;
	master_r = 0;
	master_i = 0;
	master_len = 0;
	progress = 0;
	failed = 0;
	undo_edl = 0;
	master_start = master_end = 0;
	audio_start = audio_end = 0;
}

MixersAlign::~MixersAlign()
{
	failed = -1;
	farming->lock("MixersAlign::~MixersAlign");
	close_window();
	delete farming;
	delete thread;
	delete total_lock;
	delete progress;
	delete [] master_r;
	delete [] master_i;
}

void MixersAlign::start_dialog(int wx, int wy)
{
	this->wx = wx;
	this->wy = wy;
	this->undo_edl = new EDL();
	undo_edl->create_objects();
	undo_edl->copy_all(mwindow->edl);
	start();
}

BC_Window *MixersAlign::new_gui()
{
	load_mixers();
	load_mtracks();
	load_atracks();
	ma_gui = new MixersAlignWindow(this, wx, wy);
	ma_gui->lock_window("MixersAlign::new_gui");
	ma_gui->create_objects();
	ma_gui->load_lists();
	ma_gui->default_selection();
	ma_gui->show_window(1);
	ma_gui->unlock_window();
	return ma_gui;
}

void MixersAlign::apply()
{
	int idx = ma_gui->mtrack_list->get_selection_number(0, 0);
	int midx = -1, mid = mixer_of(mtracks[idx]->track, midx);
	EDL *edl = mwindow->edl;
	
	for( int m, i=0; (m=ma_gui->mixer_list->get_selection_number(0,i))>=0; ++i ) {
		if( m == mid ) continue;  // master does not move
		MixersAlignMixer *mix = mixers[m];
		Mixer *mixer = mix->mixer;
		for( int i=0; i<mixer->mixer_ids.size(); ++i ) {
			int id = mixer->mixer_ids[i];
			Track *track = edl->tracks->first;
			while( track && track->mixer_id != id ) track = track->next;
			if( !track ) continue;
			double nudge = mix->nudge;
			int record = track->record;  track->record = 1;
			if( nudge < 0 ) {
				edl->clear(0, -nudge,
					edl->session->labels_follow_edits,
					edl->session->plugins_follow_edits,
					edl->session->autos_follow_edits);
			}
			else if( nudge > 0 ) {
				edl->paste_silence(0, nudge,
					edl->session->labels_follow_edits,
					edl->session->plugins_follow_edits,
					edl->session->autos_follow_edits);
			}
			track->record = record;
		}
	}
	edl->optimize();

	mwindow->gui->lock_window("MixersAlign::handle_done_event");
	mwindow->update_gui(1);
	mwindow->gui->unlock_window();
}


MixersAlignThread::MixersAlignThread(MixersAlign *dialog)
 : Thread(1, 0, 0)
{
	this->dialog = dialog;
}
MixersAlignThread::~MixersAlignThread()
{
	join();
}

void MixersAlignThread::run()
{
	dialog->update_match();
	MixersAlignWindow *ma_gui = dialog->ma_gui;
	ma_gui->lock_window("MixersAlignThread::run");
	ma_gui->reset->enable();
	ma_gui->match->enable();
	ma_gui->apply->enable();
	ma_gui->undo->enable();
	ma_gui->unlock_window();
}


void MixersAlign::handle_done_event(int result)
{
	if( thread->running() ) {
		failed = -1;
		thread->join();
		return;
	}
	if( !result ) {
		EDL *edl = mwindow->edl;
		mwindow->edl = undo_edl;
		mwindow->undo_before();
		mwindow->edl = edl;
		mwindow->undo_after(_("align mixers"), LOAD_ALL);
	}
}

void MixersAlign::handle_close_event(int result)
{
	undo_edl->remove_user();
	undo_edl = 0;
	ma_gui = 0;
}

void MixersAlign::load_mixers()
{
	mixers.remove_all_objects();
	Mixers &edl_mixers = mwindow->edl->mixers;
	for( int i=0; i<edl_mixers.size(); ++i )
		mixers.append(new MixersAlignMixer(edl_mixers[i]));
}

void MixersAlign::load_mtracks()
{
	mtracks.remove_all_objects();
	Track *track=mwindow->edl->tracks->first;
	for( int no=0; track; ++no, track=track->next ) {
		if( track->data_type != TRACK_AUDIO ) continue;
		mtracks.append(new MixersAlignMTrack(track, no));
	}
}

void MixersAlign::load_atracks()
{
	atracks.remove_all_objects();
	Track *track=mwindow->edl->tracks->first;
	for( int no=0; track; ++no, track=track->next ) {
		if( track->data_type != TRACK_AUDIO ) continue;
		if( mixer_of(track) < 0 ) continue;
		atracks.append(new MixersAlignATrack(track, no));
	}
}


int MixersAlign::mixer_of(Track *track, int &midx)
{
	int id = track->mixer_id, k = mixers.size(), idx = -1;
	while( idx < 0 && --k >= 0 )
		idx = mixers[k]->mixer->mixer_ids.number_of(id);
	midx = idx;
	return k;
}

EDL *MixersAlign::mixer_audio_clip(Mixer *mixer)
{
	EDL *edl = new EDL(mwindow->edl);
	edl->create_objects();
	Track *track = mwindow->edl->tracks->first;
	for( int ch=0; track && ch<MAX_CHANNELS; track=track->next ) {
		int id = track->mixer_id;
		if( track->data_type != TRACK_AUDIO ) continue;
		if( mixer->mixer_ids.number_of(id) < 0 ) continue;
		Track *mixer_audio = edl->tracks->add_audio_track(0, 0);
		mixer_audio->copy_settings(track);
		mixer_audio->play = 1;
		mixer_audio->edits->copy_from(track->edits);
		PanAuto* pan_auto = (PanAuto*)mixer_audio->automation->
				autos[AUTOMATION_PAN]->default_auto;
		pan_auto->values[ch++] = 1.0;
	}
	return edl;
}

EDL *MixersAlign::mixer_master_clip(Track *track)
{
	EDL *edl = new EDL(mwindow->edl);
	edl->create_objects();
	Track *master_audio = edl->tracks->add_audio_track(0, 0);
	master_audio->copy_settings(track);
	master_audio->play = 1;
	master_audio->edits->copy_from(track->edits);
	PanAuto* pan_auto = (PanAuto*)master_audio->automation->
			autos[AUTOMATION_PAN]->default_auto;
	pan_auto->values[0] = 1.0;
	return edl;
}

int64_t MixersAlign::mixer_tracks_total()
{
	int64_t total_len = 0;
	int64_t sample_rate = mwindow->edl->get_sample_rate();
	int m = -1, idx = 0;
	for( ; (m=ma_gui->mixer_list->get_selection_number(0, idx))>=0 ; ++idx ) {
		Mixer *mixer = mixers[m]->mixer;
		double render_end = 0;
		Track *track = mwindow->edl->tracks->first;
		for( ; track; track=track->next ) {
			int id = track->mixer_id;
			if( track->data_type != TRACK_AUDIO ) continue;
			if( mixer->mixer_ids.number_of(id) < 0 ) continue;
			double track_end = track->get_length();
			if( render_end < track_end ) render_end = track_end;
		}
		if( render_end > audio_end ) render_end = audio_end;
		double len = render_end - audio_start;
		if( len > 0 ) total_len += len * sample_rate;
	}
	return total_len;
}

MixersAlignARender::MixersAlignARender(MWindow *mwindow, EDL *edl)
 : RenderEngine(0, mwindow->preferences, 0, 0)
{
	TransportCommand command;
	command.command = NORMAL_FWD;
	command.get_edl()->copy_all(edl);
	command.change_type = CHANGE_ALL;
	command.realtime = 0;
	set_vcache(mwindow->video_cache);
	set_acache(mwindow->audio_cache);
	arm_command(&command);
}

MixersAlignARender::~MixersAlignARender()
{
}

int MixersAlignARender::render(Samples **samples, int64_t len, int64_t pos)
{
	return arender ? arender->process_buffer(samples, len, pos) : -1;
}

MixersAlignPackage::MixersAlignPackage()
{
	mixer = 0;
}

MixersAlignPackage::~MixersAlignPackage()
{
}

MixersAlignFarm::MixersAlignFarm(MixersAlign *dialog, int n)
 : LoadServer(bmin(dialog->mwindow->preferences->processors, n), n)
{
	this->dialog = dialog;
	dialog->farming->lock("MixersAlignFarm::MixersAlignFarm");
}
MixersAlignFarm::~MixersAlignFarm()
{
	dialog->farming->unlock();
}

LoadPackage* MixersAlignFarm::new_package()
{
	return new MixersAlignPackage();
}

void MixersAlignFarm::init_packages()
{
	for( int i = 0; i < get_total_packages(); ++i ) {
		int m = dialog->ma_gui->mixer_list->get_selection_number(0, i);
		MixersAlignPackage *package = (MixersAlignPackage *)get_package(i);
		package->mixer = dialog->mixers[m];
	}
}

LoadClient* MixersAlignFarm::new_client()
{
	return new MixersAlignClient(this);
}

MixersAlignClient::MixersAlignClient(MixersAlignFarm *farm)
 : LoadClient(farm)
{
}

MixersAlignClient::~MixersAlignClient()
{
}

void MixersAlignClient::process_package(LoadPackage *pkg)
{
	MixersAlignFarm *farm = (MixersAlignFarm *)server;
	MixersAlign *dialog = farm->dialog;
	MixersAlignPackage *package = (MixersAlignPackage *)pkg;
	dialog->process_package(farm, package);
}

static inline void conj_product(int n, double *rp, double *ip,
		double *arp, double *aip, double *brp, double *bip)
{
	for( int i=0; i<n; ++i ) {
		double ar = arp[i], ai = aip[i];
		double br = brp[i], bi = -bip[i];
		rp[i] = ar*br - ai*bi;
		ip[i] = ar*bi + ai*br;
	}
}

void MixersAlign::process_package(MixersAlignFarm *farm,
		 MixersAlignPackage *package)
{
	if( progress->is_cancelled() ) failed = -1;
	if( failed ) return;
	MixersAlignMixer *amix = package->mixer;
	EDL *edl = mixer_audio_clip(amix->mixer);

	MixersAlignARender audio(mwindow, edl);
	int sample_rate = edl->get_sample_rate();
	int channels = edl->get_audio_channels();
	int64_t audio_samples = edl->get_audio_samples();
	int64_t cur_pos = audio_start * sample_rate;
	int64_t end_pos = audio_end * sample_rate;
	if( end_pos > audio_samples ) end_pos = audio_samples;
	int len = master_len, len2 = len/2;
	double *audio_r = new double[len];
	double *audio_i = new double[len];
	Samples *samples[2][MAX_CHANNELS];
	for( int k=0; k<2; ++k ) {
		for( int i=0; i<MAX_CHANNELS; ++i )
			samples[k][i] = i<channels ? new Samples(len2) : 0;
	}
	int k = 0;
	int64_t nxt_pos = cur_pos+len2;
	if( nxt_pos > end_pos ) nxt_pos = end_pos;
	int len1 = nxt_pos - cur_pos;
	int ret = audio.render(samples[k], len1, cur_pos);
	while( !ret && !failed && cur_pos < end_pos ) {
		update_progress(len1);
		int64_t pos = cur_pos;
		cur_pos = nxt_pos;  nxt_pos += len2;
		if( nxt_pos > end_pos ) nxt_pos = end_pos;
		len1 = nxt_pos - cur_pos;
		ret = audio.render(samples[1-k], len1, cur_pos);
		if( ret ) break;
		Track *track = edl->tracks->first;
		for( int ch=0; ch<channels && track; ++ch, track=track->next ) {
			int id = track->mixer_id, atk = atracks.size();
			while( --atk >= 0 && id != atracks[atk]->track->mixer_id );
			if( atk < 0 ) continue;
			int i = 0;
			double *lp = samples[k][ch]->get_data();
			for( int j=0; j<len2; ++j ) audio_r[i++] = *lp++;
			double *np = samples[1-k][ch]->get_data();
			for( int j=0; j<len1; ++j ) audio_r[i++] = *np++;
			while( i < len ) audio_r[i++] = 0;
			do_fft(len, 0, audio_r, 0, audio_r, audio_i);
			conj_product(len, audio_r, audio_i,
				audio_r, audio_i, master_r, master_i);
			do_fft(len, 1, audio_r, audio_i, audio_r, audio_i);
			double mx = 0;  int64_t mi = -1;
			for( int i=0; i<len2; ++i ) {
				double v = fabs(audio_r[i]);
				if( mx < v ) { mx = v;  mi = i + pos; }
			}
			mx /= master_ss;
			MixersAlignATrack *atrack= atracks[atk];
			if( atrack->mx < mx ) {
				atrack->mx = mx;
				atrack->mi = mi;
			}
		}
		k = 1-k;
		if( progress->is_cancelled() ) failed = -1;
	}
	if( ret && !failed ) {
		eprintf("Audio render failed:\n%s", edl->path);
		failed = 1;
	}
	delete [] audio_r;
	delete [] audio_i;
	for( int k=0; k<2; ++k ) {
		for( int i=channels; --i>=0; )
			delete samples[k][i];
	}
	edl->remove_user();
}


void MixersAlign::update_progress(int64_t len)
{
	total_lock->lock();
	total_rendered += len;
	total_lock->unlock();
	progress->update(total_rendered);
}

static inline uint64_t high_bit_mask(uint64_t n)
{
	n |= n >> 1;   n |= n >> 2;
	n |= n >> 4;   n |= n >> 8;
	n |= n >> 16;  n |= n >> 32;
	return n;
}

void MixersAlign::load_master_audio(Track *track)
{
	EDL *edl = mixer_master_clip(track);
	MixersAlignARender audio(mwindow, edl);
	int sample_rate = edl->get_sample_rate();
	int channels = edl->get_audio_channels();
	int64_t audio_samples = edl->get_audio_samples();
	int64_t cur_pos = master_start * sample_rate;
	int64_t end_pos = master_end * sample_rate;
	if( end_pos > audio_samples ) end_pos = audio_samples;
	if( cur_pos >= end_pos ) {
		eprintf(_("master audio track empty"));
		failed = 1;
		return;
	}
	int64_t audio_len = end_pos - cur_pos;
	if( audio_len > sample_rate * 60 ) {
		eprintf(_("master audio track length > 60 seconds"));
		failed = 1;
		return;
	}
	int64_t fft_len = (high_bit_mask(audio_len)+1) << 1;
	if( master_len != fft_len ) {
		master_len = fft_len;
		delete [] master_r;  master_r = new double[master_len];
		delete [] master_i;  master_i = new double[master_len];
	}
	{	Samples *samples[MAX_CHANNELS];
		for( int i=0; i<MAX_CHANNELS; ++i )
			samples[i] = i<channels ? new Samples(audio_len) : 0;
		int ret = audio.render(samples, audio_len, cur_pos);
		if( ret ) failed = 1;
		edl->remove_user();
		double *dp = samples[0]->get_data();
		int i = 0;  double ss = 0;
		for( ; i<audio_len; ++i ) { ss += *dp * *dp;  master_r[i] = *dp++; }
		master_ss = ss;
		for( ; i<fft_len; ++i ) master_r[i] = 0;
		for( int i=channels; --i>=0; )
			delete samples[i];
	}
	do_fft(fft_len, 0, master_r, 0, master_r, master_i);
}

void MixersAlign::scan_mixer_audio()
{
	for( int i=0; i<mixers.size(); ++i ) mixers[i]->aidx = -1;
	int n = 0;
	while( ma_gui->mixer_list->get_selection_number(0, n) >= 0 ) ++n;
	if( !n ) {
		eprintf(_("no mixers selected"));
		return;
	}

	Timer timer;
	total_rendered = 0;
	MixersAlignFarm farm(this, n);
	int64_t total_len = mixer_tracks_total();
	mwindow->gui->lock_window("MixersAlign::scan_mixer_audio");
	progress = mwindow->mainprogress->
		start_progress(_("Render mixer audio"), total_len);
	mwindow->gui->unlock_window();

	farm.process_packages();
	if( progress->is_cancelled() ) failed = -1;

	char text[BCSTRLEN];
	double secs = timer.get_difference()/1000.;
	sprintf(text, _("Render mixer done: %0.3f secs"), secs);
	mwindow->gui->lock_window("MixersAlign::scan_mixer_audio");
	progress->update(0);
	mwindow->mainprogress->end_progress(progress);
	progress = 0;
	mwindow->gui->show_message(text);
	mwindow->gui->unlock_window();
}

void MixersAlign::update_match()
{
	int midx = ma_gui->mtrack_list->get_selection_number(0, 0);
	if( midx < 0 ) {
		eprintf(_("selection (master) not set"));
		return;
	}
	master_start = mwindow->edl->local_session->get_inpoint();
	if( master_start < 0 ) {
		eprintf(_("in point selection (master start) must be set"));
		return;
	}
	master_end = mwindow->edl->local_session->get_outpoint();
	if( master_end < 0 ) {
		eprintf(_("out point selection (master end) must be set"));
		return;
	}
	if( master_start >= master_end ) {
		eprintf(_("in/out point selection (master start/end) invalid"));
		return;
	}
	audio_start = mwindow->edl->local_session->get_selectionstart();
	audio_end = mwindow->edl->local_session->get_selectionend();
	if( audio_start >= audio_end ) {
		eprintf(_("selection (audio start/end) invalid"));
		return;
	}

	failed = 0;
	if( !failed ) load_master_audio(mtracks[midx]->track);
        if( !failed ) scan_mixer_audio();
	if( !failed ) update();
	if( failed < 0 ) {
		mwindow->gui->lock_window("MixersAlign::update_match");
		mwindow->gui->show_message(_("mixer selection match canceled"));
		mwindow->gui->unlock_window();
	}
	else if( failed > 0 )
		eprintf(_("Error in match render."));
}

void MixersAlign::update()
{
// mixer best matches
	for( int m=0; m<mixers.size(); ++m ) {
		MixersAlignMixer *mix = mixers[m];
		Mixer *mixer = mix->mixer;
		mix->mx = 0;  mix->mi = -1;  mix->aidx = -1;
		for( int i=0; i<mixer->mixer_ids.size(); ++i ) {
			int id = mixer->mixer_ids[i], k = atracks.size();
			while( --k >= 0 && atracks[k]->track->mixer_id != id );
			if( k < 0 ) continue;
			MixersAlignATrack *atrk = atracks[k];
			atrk->nudge = atrk->mi < 0 ? 0 :
				master_start - atrk->track->from_units(atrk->mi);
			if( mix->mx >= atrk->mx ) continue;
			mix->aidx = k;
			mix->mx = atrk->mx;
			mix->mi = atrk->mi;
			mix->nudge = atrk->nudge;
		}
	}

	ma_gui->lock_window("MixersAlign::update");
	ma_gui->mixer_list->load_list();
	ma_gui->mixer_list->set_all_selected(1);
	ma_gui->mixer_list->update();

	ma_gui->atrack_list->load_list();
	for( int m=0; m<mixers.size(); ++m ) {
		int aidx = mixers[m]->aidx;
		if( aidx >= 0 ) ma_gui->atrack_list->set_selected(aidx, 1);
	}
	ma_gui->atrack_list->update();
	ma_gui->unlock_window();
}

