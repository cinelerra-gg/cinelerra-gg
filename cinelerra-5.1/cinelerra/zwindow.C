
/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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

#include "bcdialog.h"
#include "edl.h"
#include "filexml.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "patchgui.h"
#include "playbackengine.h"
#include "renderengine.h"
#include "timelinepane.h"
#include "track.h"
#include "transportque.h"
#include "zwindow.h"
#include "zwindowgui.h"

Mixers::Mixers()
{
}

Mixers::~Mixers()
{
	remove_all_objects();
}

Mixer *Mixers::new_mixer()
{
	int idx = 0;
	for( int i=0; i<size(); ++i ) {
		Mixer *mixer = get(i);
		if( idx < mixer->idx ) idx = mixer->idx;
	}
	return append(new Mixer(idx+1));
}

Mixer *Mixers::get_mixer(int idx)
{
	for( int i=0; i<size(); ++i ) {
		Mixer *mixer = get(i);
		if( mixer->idx == idx ) return mixer;
	}
	return 0;
}

void Mixers::del_mixer(int idx)
{
	Mixer *mixer = get_mixer(idx);
	if( mixer ) remove_object(mixer);
}

void Mixer::set_title(const char *tp)
{
	if( tp == title ) return;
	strncpy(title, !tp ? "" : tp, sizeof(title));
	title[sizeof(title)-1] = 0;
}

void Mixers::save(FileXML *file)
{
	file->tag.set_title("MIXERS");
	file->append_tag();
	file->append_newline();
	for( int i=0; i<size(); ++i ) {
		Mixer *mixer = get(i);
		mixer->save(file);
	}
	file->tag.set_title("/MIXERS");
	file->append_tag();
	file->append_newline();
}

int Mixers::load(FileXML *file)
{
	int result = 0;
	while( !(result = file->read_tag()) ) {
		if( file->tag.title_is("/MIXERS") ) break;
		if( file->tag.title_is("MIXER") ) {
			Mixer *mixer = new_mixer();
			file->tag.get_property("TITLE", mixer->title);
			mixer->x = file->tag.get_property("X", mixer->x);
			mixer->y = file->tag.get_property("Y", mixer->y);
			mixer->w = file->tag.get_property("W", mixer->w);
			mixer->h = file->tag.get_property("H", mixer->h);
			mixer->load(file);
		}
	}
	return 0;
}

void Mixers::copy_from(Mixers &that)
{
	remove_all_objects();
	for( int i=0; i<that.size(); ++i ) {
		Mixer *mixer = new_mixer();
		mixer->copy_from(*that[i]);
	}
}

void Mixer::save(FileXML *file)
{
	file->tag.set_title("MIXER");
	file->tag.set_property("TITLE",title);
	file->tag.set_property("X",x);
	file->tag.set_property("Y",y);
	file->tag.set_property("W",w);
	file->tag.set_property("H",h);
	file->append_tag();
	file->append_newline();
	for( int i=0; i<mixer_ids.size(); ++i ) {
		file->tag.set_title("MIX");
		file->tag.set_property("ID", mixer_ids[i]);
		file->append_tag();
		file->tag.set_title("/MIX");
		file->append_tag();
		file->append_newline();
	}
	file->tag.set_title("/MIXER");
	file->append_tag();
	file->append_newline();
}

Mixer::Mixer(int idx)
{
	this->idx = idx;
	title[0] = 0;
	x = y = 100 + idx*64;
	w = 400;  h = 300;
}
void Mixer::reposition(int x, int y, int w, int h)
{
	this->x = x;  this->y = y;
	this->w = w;  this->h = h;
}

int Mixer::load(FileXML *file)
{
	int result = 0;
	while( !(result = file->read_tag()) ) {
		if( file->tag.title_is("/MIXER") ) break;
		if( file->tag.title_is("MIX") ) {
			mixer_ids.append(file->tag.get_property("ID", 0));
		}
	}
	return 0;
}

void Mixer::copy_from(Mixer &that)
{
	mixer_ids.remove_all();
	strncpy(title, that.title, sizeof(title));
	title[sizeof(title)-1] = 0;
	x = that.x;  y = that.y;
	w = that.w;  h = that.h;
	for( int i=0; i<that.mixer_ids.size(); ++i )
		mixer_ids.append(that.mixer_ids[i]);
}


ZWindow::ZWindow(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	idx = -1;
	edl = 0;
	highlighted = 0;
	destroy = 1;
	title[0] = 0;
	zgui = 0;
}

ZWindow::~ZWindow()
{
	close_window();
	if( edl )
		edl->remove_user();
}

BC_Window* ZWindow::new_gui()
{
	Mixer *mixer = mwindow->edl->mixers.get_mixer(idx);
	zgui = new ZWindowGUI(mwindow, this, mixer);
	zgui->create_objects();
	return zgui;
}

void ZWindow::handle_done_event(int result)
{
	if( destroy )
		mwindow->del_mixer(this);
	idx = -1;
}
void ZWindow::handle_close_event(int result)
{
	zgui = 0;
}

void ZWindow::change_source(EDL *edl)
{
	if( this->edl && edl != this->edl )
		this->edl->remove_user();
	this->edl = edl;
	if( edl != 0 ) {
		zgui->playback_engine->refresh_frame(CHANGE_ALL, edl);
	}
}

void ZWindow::stop_playback(int wait)
{
	zgui->playback_engine->stop_playback(wait);
}

void ZWindow::issue_command(int command, int wait_tracking,
		int use_inout, int update_refresh, int toggle_audio, int loop_play)
{
	zgui->playback_engine->issue_command(edl, command,
			wait_tracking, use_inout, update_refresh, toggle_audio, loop_play);
}

void ZWindow::update_mixer_ids()
{
	if( !running() ) return;
	Mixer *mixer = mwindow->edl->mixers.get_mixer(idx);
	if( !mixer ) return;
	mixer->mixer_ids.remove_all();
	PatchBay *patchbay = mwindow->gui->pane[0]->patchbay;
	for( int i=0; i<patchbay->patches.size(); ++i ) {
		PatchGUI *patchgui = patchbay->patches[i];
		if( !patchgui->mixer ) continue;
		int mixer_id = patchgui->track->get_mixer_id();
		if( mixer->mixer_ids.number_of(mixer_id) >= 0 ) continue;
		mixer->mixer_ids.append(mixer_id);
	}
}

void ZWindow::set_title(const char *tp)
{
	Mixer *mixer = mwindow->edl->mixers.get_mixer(idx);
	if( mixer ) mixer->set_title(tp);
	char *cp = title, *ep = cp + sizeof(title)-1;
	cp += snprintf(title, ep-cp, _("Mixer %d"), idx);
	if( tp ) cp += snprintf(cp, ep-cp, ": %s", tp);
	*cp = 0;
}

void ZWindow::reposition(int x, int y, int w, int h)
{
	Mixer *mixer = mwindow->edl->mixers.get_mixer(idx);
	if( !mixer ) return;
	mixer->reposition(x, y, w, h);
}

