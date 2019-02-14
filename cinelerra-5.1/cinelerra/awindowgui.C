
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

#include "arender.h"
#include "asset.h"
#include "assetedit.h"
#include "assetpopup.h"
#include "assetremove.h"
#include "assets.h"
#include "audiodevice.h"
#include "awindow.h"
#include "awindowgui.h"
#include "bccmodels.h"
#include "bcsignals.h"
#include "bchash.h"
#include "binfolder.h"
#include "cache.h"
#include "cstrdup.h"
#include "clip.h"
#include "clipedls.h"
#include "clippopup.h"
#include "cursors.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "edits.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "effectlist.h"
#include "file.h"
#include "filesystem.h"
#include "folderlistmenu.h"
#include "indexable.h"
#include "keys.h"
#include "language.h"
#include "labels.h"
#include "labelpopup.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "preferences.h"
#include "proxy.h"
#include "proxypopup.h"
#include "renderengine.h"
#include "samples.h"
#include "theme.h"
#include "tracks.h"
#include "track.h"
#include "transportque.h"
#include "vframe.h"
#include "vicon.h"
#include "vrender.h"
#include "vwindowgui.h"
#include "vwindow.h"

#include "data/lad_picon_png.h"
#include "data/ff_audio_png.h"
#include "data/ff_video_png.h"

#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>


const char *AWindowGUI::folder_names[] =
{
	N_("Audio Effects"),
	N_("Video Effects"),
	N_("Audio Transitions"),
	N_("Video Transitions"),
	N_("Labels"),
	N_("Clips"),
	N_("Media"),
	N_("Proxy"),
};


AssetVIcon::AssetVIcon(AssetPicon *picon, int w, int h, double framerate, int64_t length)
 : VIcon(w, h, framerate), Garbage("AssetVIcon")
{
	this->picon = picon;
	this->length = length;
	temp = 0;
}

AssetVIcon::~AssetVIcon()
{
	picon->gui->vicon_thread->del_vicon(this);
	delete temp;
}

VFrame *AssetVIcon::frame()
{
	AssetVIconThread *avt = picon->gui->vicon_thread;
	Asset *asset = (Asset *)picon->indexable;
	if( !asset )
		return *images[0];
	if( !asset->video_data && audio_data && audio_size && length > 0 ) {
		if( !temp ) temp = new VFrame(0, -1, w, h, BC_RGB888, -1);
		temp->clear_frame();
		int sample_rate = asset->get_sample_rate();
		int64_t audio_samples = asset->get_audio_samples();
		double duration = (double)audio_samples / sample_rate;
		picon->draw_hue_bar(temp, duration);
		int secs = length / frame_rate + 0.5;
		if( !secs ) secs = 1;
		int bfrsz = VICON_SAMPLE_RATE;
		int samples = audio_size/sizeof(vicon_audio_t);
		double line_pos = (double)seq_no/(length-1);
		int audio_pos = samples * line_pos * (secs-1)/secs;
		vicon_audio_t *audio_data = ((vicon_audio_t *)this->audio_data) + audio_pos;
		static const int mx = (1<<(8*sizeof(*audio_data)-1)) - 1;
		double data[bfrsz], sample_scale = 1./mx;
		int len = samples - audio_pos;
		if( len > bfrsz ) len = bfrsz;
		int i = 0;
		while( i < len ) data[i++] = *audio_data++ * sample_scale;
		while( i < bfrsz ) data[i++] = 0;
		picon->draw_wave(temp, data, bfrsz, RED, GREEN);
		int x = (w-1) * line_pos;
		temp->pixel_rgb = RED;
		temp->draw_line(x,0, x,h);
		return temp;
	}
	int vw = avt->vw, vh = avt->vh, vicon_cmodel = avt->vicon_cmodel;
	if( !asset->video_data ) {
		if( !temp ) {
			temp = new VFrame(0, -1, vw, vh, BC_RGB888, -1);
			temp->clear_frame();
		}
		return temp;
	}
	if( seq_no >= images.size() ) {
		MWindow *mwindow = picon->mwindow;
		File *file = mwindow->video_cache->check_out(asset, mwindow->edl, 1);
		if( !file ) return 0;
		if( temp && (temp->get_w() != asset->width || temp->get_h() != asset->height) ) {
			delete temp;  temp = 0;
		}
		if( !temp )
			temp = new VFrame(0, -1, asset->width, asset->height, BC_RGB888, -1);
		while( seq_no >= images.size() ) {
			mwindow->video_cache->check_in(asset);
			Thread::yield();
			file = mwindow->video_cache->check_out(asset, mwindow->edl, 0);
			if( !file ) { usleep(1000);  continue; }
			file->set_layer(0);
			int64_t pos = images.size() / picon->gui->vicon_thread->refresh_rate * frame_rate;
			file->set_video_position(pos,0);
			if( file->read_frame(temp) ) temp->clear_frame();
			add_image(temp, vw, vh, vicon_cmodel);
		}
		mwindow->video_cache->check_in(asset);
	}
	return *images[seq_no];
}

int64_t AssetVIcon::set_seq_no(int64_t no)
{
	if( no >= length ) no = 0;
	return seq_no = no;
}

int AssetVIcon::get_vx()
{
	BC_ListBox *lbox = picon->gui->asset_list;
	return lbox->get_icon_x(picon);
}
int AssetVIcon::get_vy()
{
	BC_ListBox *lbox = picon->gui->asset_list;
	return lbox->get_icon_y(picon);
}

void AssetVIcon::load_audio()
{
	MWindow *mwindow = picon->mwindow;
	Asset *asset = (Asset *)picon->indexable;
	File *file = mwindow->audio_cache->check_out(asset, mwindow->edl, 1);
	int channels = asset->get_audio_channels();
	if( channels > 2 ) channels = 2;
	int sample_rate = asset->get_sample_rate();
	int bfrsz = sample_rate;
	Samples samples(bfrsz);
	double time_scale = (double)sample_rate / VICON_SAMPLE_RATE;
	vicon_audio_t *audio_data = (vicon_audio_t *)this->audio_data;
	static const int mx = (1<<(8*sizeof(*audio_data)-1)) - 1;
	double sample_scale = (double)mx / channels;
	int audio_pos = 0, audio_len = audio_size/sizeof(vicon_audio_t);
	while( audio_pos < audio_len ) {
		int64_t pos = audio_pos * time_scale;
		for( int ch=0; ch<channels; ++ch ) {
			file->set_channel(ch);
			file->set_audio_position(pos);
			file->read_samples(&samples, bfrsz);
			double *data = samples.get_data();
			for( int64_t k=audio_pos; k<audio_len; ++k ) {
				int i = k * time_scale - pos;
				if( i >= bfrsz ) break;
				int v = audio_data[k] + data[i] * sample_scale;
				audio_data[k] = CLIP(v, -mx,mx);
			}
		}
		audio_pos = (pos + bfrsz) / time_scale;
	}
	mwindow->audio_cache->check_in(asset);
}


AssetVIconAudio::AssetVIconAudio(AWindowGUI *gui)
 : Thread(1, 0, 0)
{
	this->gui = gui;
	audio = new AudioDevice(gui->mwindow);
	interrupted = 0;
	vicon = 0;
}
AssetVIconAudio::~AssetVIconAudio()
{
	delete audio;
}

void AssetVIconAudio::run()
{
	int channels = 2;
	int64_t bfrsz = VICON_SAMPLE_RATE;
	MWindow *mwindow = gui->mwindow;
	EDL *edl = mwindow->edl;
	EDLSession *session = edl->session;
	AudioOutConfig *aconfig = session->playback_config->aconfig;
	audio->open_output(aconfig, VICON_SAMPLE_RATE, bfrsz, channels, 0);
	audio->start_playback();
	double out0[bfrsz], out1[bfrsz], *out[2] = { out0, out1 };
	vicon_audio_t *audio_data = (vicon_audio_t *)vicon->audio_data;
	static const int mx = (1<<(8*sizeof(*audio_data)-1)) - 1;

	int audio_len = vicon->audio_size/sizeof(vicon_audio_t);
	while( !interrupted ) {
		int len = audio_len - audio_pos;
		if( len <= 0 ) break;
		if( len > bfrsz ) len = bfrsz;
		int k = audio_pos;
		for( int i=0; i<len; ++i,++k )
			out0[i] = out1[i] = (double)audio_data[k] / mx;
		audio_pos = k;
		audio->write_buffer(out, channels, len);
	}

	if( !interrupted )
		audio->set_last_buffer();
	audio->stop_audio(interrupted ? 0 : 1);
	audio->close_all();
}

void AssetVIconAudio::start(AssetVIcon *vicon)
{
	if( running() ) return;
	interrupted = 0;
	audio_pos = 0;
	this->vicon = vicon;
	Thread::start();
}

void AssetVIconAudio::stop(int wait)
{
	if( running() && !interrupted ) {
		interrupted = 1;
		audio->stop_audio(wait);
	}
	Thread::join();
	if( vicon ) {
		vicon->playing_audio = 0;
		vicon = 0;
	}
}

void AssetVIcon::start_audio()
{
	if( playing_audio < 0 ) return;
	picon->gui->vicon_audio->stop(0);
	playing_audio = 1;
	picon->gui->vicon_audio->start(this);
}

void AssetVIcon::stop_audio()
{
	if( playing_audio > 0 )
		picon->gui->vicon_audio->stop(0);
	playing_audio = 0;
}

AssetViewPopup::AssetViewPopup(VIconThread *vt, int draw_mode,
		int x, int y, int w, int h)
 : ViewPopup(vt, x, y, w, h)
{
	this->draw_mode = draw_mode;
	this->bar_h = (VIEW_POPUP_BAR_H * h) / 200;
}

AssetViewPopup::~AssetViewPopup()
{
}

int AssetViewPopup::button_press_event()
{
	if( !is_event_win() ) return 0;
	AssetVIconThread *avt = (AssetVIconThread *)vt;
	if( !avt->vicon ) return 0;

	switch( draw_mode ) {
	case ASSET_VIEW_MEDIA_MAP:
	case ASSET_VIEW_FULL:
		break;
	default:
		return 0;
	}

	int dir = 1;
	switch( get_buttonpress() ) {
	case LEFT_BUTTON:
		break;
	case WHEEL_DOWN:
		dir = -1;
		// fall thru
	case WHEEL_UP:
		if( draw_mode != ASSET_VIEW_FULL )
			return avt->zoom_scale(dir);
		// fall thru
	default:
		return 0;
	}

	int x = get_cursor_x(), y = get_cursor_y();
	AssetVIcon *vicon = (AssetVIcon *)avt->vicon;
	AssetPicon *picon = vicon->picon;
	MWindow *mwindow = picon->mwindow;
	EDL *edl = mwindow->edl;
	dragging = 0;
	if( y < get_h()/2 ) {
		Indexable *idxbl =
			picon->indexable ? picon->indexable :
			picon->edl ? picon->edl : 0;
		if( !idxbl ) return 0;
		double sample_rate = idxbl->get_sample_rate();
		double audio_length = sample_rate > 0 && idxbl->have_audio() ?
			idxbl->get_audio_samples() / sample_rate : 0;
		double frame_rate = idxbl->get_frame_rate();
		double video_length = frame_rate > 0 && idxbl->have_video() ?
			idxbl->get_video_frames() / frame_rate : 0;
		double idxbl_length = bmax(audio_length, video_length);
		double pos = x * idxbl_length / get_w();
		double start = 0, end = idxbl_length;
		double lt = DBL_MAX, rt = DBL_MAX;
		for( Track *track=edl->tracks->first; track!=0; track=track->next ) {
			for( Edit *edit=track->edits->first; edit!=0; edit=edit->next ) {
				Indexable *indexable = (Indexable *)edit->asset;
				if( !indexable ) indexable = (Indexable *)edit->nested_edl;
				if( !indexable ) continue;
				if( indexable->id == idxbl->id ||
				    (!indexable->is_asset == !idxbl->is_asset &&
				     !strcmp(indexable->path, idxbl->path)) ) {
					double start_pos = track->from_units(edit->startsource);
					double end_pos = track->from_units(edit->startsource + edit->length);
					double dlt = pos - start_pos, drt = end_pos - pos;
					if( dlt >= 0 &&  dlt < lt ) { lt = dlt;  start = start_pos; }
					else if( dlt < 0 && -dlt < rt ) { rt = -dlt;  end = start_pos; }
					if( drt >= 0 &&  drt < rt ) { rt = drt;  end = end_pos; }
					else if( drt < 0 && -drt < lt ) { lt = -drt; start = end_pos; }
				}
			}
		}
		mwindow->gui->lock_window("AssetVIcon::popup_button_press");
		VWindow *vwindow = mwindow->get_viewer(1, 0);
		vwindow->change_source(idxbl);
		mwindow->gui->unlock_window();
		EDL *vedl = vwindow->get_edl();
		vedl->set_inpoint(start);
		vedl->set_outpoint(end);
		vedl->local_session->set_selectionstart(start);
		vedl->local_session->set_selectionend(end);
		vwindow->update_position();
		return 1;
	}
	else {
		dragging = 1;
		if( !ctrl_down() && !shift_down() )
			return cursor_motion_event();
		Indexable *idxbl =
			picon->indexable ? picon->indexable :
			picon->edl ? picon->edl : 0;
		if( !idxbl ) return 0;
		double total_length = mwindow->edl->tracks->total_length();
		double pos = x * total_length / get_w();
		double start = 0, end = total_length;
		double lt = DBL_MAX, rt = DBL_MAX;
		for( Track *track=edl->tracks->first; track!=0; track=track->next ) {
			for( Edit *edit=track->edits->first; edit!=0; edit=edit->next ) {
				Indexable *indexable = (Indexable *)edit->asset;
				if( !indexable ) indexable = (Indexable *)edit->nested_edl;
				if( !indexable ) continue;
				if( indexable->id == idxbl->id ||
				    (!indexable->is_asset == !idxbl->is_asset &&
				     !strcmp(indexable->path, idxbl->path)) ) {
					double start_pos = track->from_units(edit->startproject);
					double end_pos = track->from_units(edit->startproject + edit->length);
					double dlt = pos - start_pos, drt = end_pos - pos;
					if( dlt >= 0 &&  dlt < lt ) { lt = dlt;  start = start_pos; }
					else if( dlt < 0 && -dlt < rt ) { rt = -dlt;  end = start_pos; }
					if( drt >= 0 &&  drt < rt ) { rt = drt;  end = end_pos; }
					else if( drt < 0 && -drt < lt ) { lt = -drt; start = end_pos; }
				}
			}
		}
		mwindow->gui->lock_window("AssetVIcon::popup_button_press");
		mwindow->select_point(!ctrl_down() && !shift_down() ? pos : start);
		edl->local_session->set_selectionstart(start);
		edl->local_session->set_selectionend(!shift_down() ? start : end);
		mwindow->zoom_sample(edl->local_session->zoom_sample);
		mwindow->gui->unlock_window();
		return 1;
	}
	return 0;
}

int AssetViewPopup::button_release_event()
{
	if( !is_event_win() ) return 0;
	dragging = 0;
	return 1;
}

int AssetViewPopup::cursor_motion_event()
{
	if( !is_event_win() ) return 0;
	AssetVIconThread *avt = (AssetVIconThread *)vt;
	if( !avt->vicon ||
	    ( draw_mode != ASSET_VIEW_FULL &&
	      draw_mode != ASSET_VIEW_MEDIA_MAP ) ) return 0;
	if( !get_button_down() || get_buttonpress() != LEFT_BUTTON ||
	    ctrl_down() || alt_down() || shift_down() )
		return 0;
	AssetVIcon *vicon = (AssetVIcon *)avt->vicon;
	MWindow *mwindow = vicon->picon->mwindow;
	EDL *edl = mwindow->edl;
	if( dragging ) {
		int x = get_cursor_x();
		double total_length = edl->tracks->total_length();
		double pos = x * total_length / get_w();
		mwindow->gui->lock_window("AssetVIcon::popup_cursor_motion");
		mwindow->select_point(pos);
		mwindow->zoom_sample(edl->local_session->zoom_sample);
		mwindow->gui->unlock_window();
		return 1;
	}
	return 0;
}

void AssetViewPopup::draw_vframe(VFrame *vframe) 
{
	switch( draw_mode ) {
	case ASSET_VIEW_MEDIA:
	case ASSET_VIEW_ICON:
		ViewPopup::draw_vframe(vframe);
	case ASSET_VIEW_NONE:
	default:
		return;
	case ASSET_VIEW_MEDIA_MAP:
	case ASSET_VIEW_FULL:
		break;
	}
	set_color(BLACK);
	draw_box(0,0,get_w(),get_h());
	int y1 = bar_h;
	int y2 = get_h()-bar_h;
	BC_WindowBase::draw_vframe(vframe, 0,y1, get_w(),y2-y1);
	AssetVIconThread *avt = (AssetVIconThread *)vt;
	AssetVIcon *vicon = (AssetVIcon *)avt->vicon;
	AssetPicon *picon = (AssetPicon *)vicon->picon;
	Indexable *idxbl =
		picon->indexable ? picon->indexable :
		picon->edl ? picon->edl : 0;
	if( !idxbl ) return;
	double sample_rate = idxbl->get_sample_rate();
	double audio_length = sample_rate > 0 && idxbl->have_audio() ?
		idxbl->get_audio_samples() / sample_rate : 0;
	double frame_rate = idxbl->get_frame_rate();
	double video_length = frame_rate > 0 && idxbl->have_video() ?
		idxbl->get_video_frames() / frame_rate : 0;
	double idxbl_length = bmax(audio_length, video_length);
	if( !idxbl_length ) idxbl_length = 1;

	EDL *edl = picon->mwindow->edl;
	double total_length = edl->tracks->total_length();
	if( !total_length ) total_length = 1;
	for( Track *track=edl->tracks->first; track!=0; track=track->next ) {
		if( !track->record ) continue;
		for( Edit *edit=track->edits->first; edit!=0; edit=edit->next ) {
			Indexable *indexable = (Indexable *)edit->asset;
			if( !indexable ) indexable = (Indexable *)edit->nested_edl;
			if( !indexable ) continue;
			if( indexable->id == idxbl->id ||
			    (!indexable->is_asset == !idxbl->is_asset &&
			     !strcmp(indexable->path, idxbl->path)) ) {
				double pos1 = track->from_units(edit->startsource);
				double pos2 = track->from_units(edit->startsource + edit->length);
				double xscale = get_w() / idxbl_length;
				int ex1 = pos1 * xscale, ex2 = pos2 * xscale;
				set_color(WHITE);
				draw_box(ex1,0, ex2-ex1,y1);
				set_color(BLACK);
				draw_line(ex1,0, ex1,y1);
				draw_line(ex2,0, ex2,y1);
				pos1 = track->from_units(edit->startproject);
				pos2 = track->from_units(edit->startproject + edit->length);
				xscale = get_w() / total_length;
				int px1 = pos1 * xscale, px2 = pos2 * xscale;
				set_color(RED);
				draw_box(px1,y2, px2-px1,get_h()-y2);
				set_color(BLACK);
				draw_line(px1,y2, px1,get_h()-1);
				draw_line(px2,y2, px2,get_h()-1);

				set_color(YELLOW);
				draw_line(ex1,y1, px1,y2);
				draw_line(ex2,y1, px2,y2);
			}
		}
	}
}

int AssetViewPopup::keypress_event()
{
	AssetVIconThread *avt = (AssetVIconThread *)vt;
	switch( avt->draw_mode ) {
	case ASSET_VIEW_MEDIA_MAP:
		switch( get_keypress() ) {
		case 'f':
		case 'F':
			avt->draw_mode = ASSET_VIEW_FULL;
			avt->viewing = 0;
			return 1;
		}
		break;
	case ASSET_VIEW_FULL:
		avt->draw_mode = ASSET_VIEW_MEDIA_MAP;
		avt->viewing = 0;
		return 1;
	}
	return ViewPopup::keypress_event();
}


AssetVIconThread::AssetVIconThread(AWindowGUI *gui, Preferences *preferences)
 : VIconThread(gui->asset_list, preferences->vicon_size * 16/9, preferences->vicon_size,
	4*preferences->awindow_picon_h * 16/9, 4*preferences->awindow_picon_h)
{
	this->gui = gui;
	draw_mode = ASSET_VIEW_NONE;
	int vicon_cmodel = BC_RGB8;
	switch( preferences->vicon_color_mode ) {
	case VICON_COLOR_MODE_LOW:   vicon_cmodel = BC_RGB8;    break;
	case VICON_COLOR_MODE_MED:   vicon_cmodel = BC_RGB565;  break;
	case VICON_COLOR_MODE_HIGH:  vicon_cmodel = BC_RGB888;  break;
	}
	this->vicon_cmodel = vicon_cmodel;
	this->draw_lock = new Mutex("AssetVIconThread::draw_lock");
}

AssetVIconThread::~AssetVIconThread()
{
	delete draw_lock;
}

void AssetVIconThread::drawing_started()
{
	draw_lock->lock("AssetVIconThread::drawing_started");
}

void AssetVIconThread::drawing_stopped()
{
	draw_lock->unlock();
}

void AssetVIconThread::set_view_popup(AssetVIcon *v, int draw_mode)
{
	gui->stop_vicon_drawing();
	int mode = !v ? ASSET_VIEW_NONE :
		draw_mode >= 0 ? draw_mode :
		this->draw_mode;
	this->vicon = v;
	this->draw_mode = mode;
	gui->start_vicon_drawing();
}

ViewPopup *AssetVIconThread::new_view_window()
{
	BC_WindowBase *parent = wdw->get_parent();
	int rx = 0, ry = 0, rw = 0, rh = 0;
	if( draw_mode != ASSET_VIEW_FULL ) {
		XineramaScreenInfo *info = parent->get_xinerama_info(-1);
		int cx = info ? info->x_org + info->width/2 : parent->get_root_w(0)/2;
		int cy = info ? info->y_org + info->height/2 : parent->get_root_h(0)/2;
		int vx = viewing->get_vx(), vy = viewing->get_vy();
		wdw->get_root_coordinates(vx, vy, &rx, &ry);
		rx += (rx >= cx ? -view_w+viewing->w/4 : viewing->w-viewing->w/4);
		ry += (ry >= cy ? -view_h+viewing->h/4 : viewing->h-viewing->h/4);
		rw = view_w;  rh = view_h;
	}
	else
		parent->get_fullscreen_geometry(rx, ry, rw, rh);
	AssetViewPopup *popup = new AssetViewPopup(this, draw_mode, rx, ry, rw, rh);
	if( draw_mode == ASSET_VIEW_MEDIA_MAP || draw_mode == ASSET_VIEW_FULL )
		vicon->playing_audio = -1;
	wdw->set_active_subwindow(popup);
	return popup;
}

void AssetVIconThread::close_view_popup()
{
	stop_drawing();
	drawing_started(); // waits for draw lock
	drawing_stopped();
}


AWindowFolderItem::AWindowFolderItem()
 : BC_ListBoxItem()
{
	parent = 0;
}

AWindowFolderItem::AWindowFolderItem(const char *text, int color)
 : BC_ListBoxItem(text, color)
{
	parent = 0;
}

AWindowFolderItem::AWindowFolderItem(const char *text, BC_Pixmap *icon, int color)
 : BC_ListBoxItem(text, icon, color)
{
	parent = 0;
}

AssetPicon *AWindowFolderItem::get_picon()
{
	AWindowFolderItem *item = this;
	while( item->parent ) { item = (AWindowFolderItem*)item->parent; }
	return (AssetPicon*)item;
}

int AWindowFolderSubItems::matches(const char *text)
{
	int i = names.size();
	while( --i >= 0 && strcmp(names[i], text) );
	if( i < 0 ) {
		ArrayList<BC_ListBoxItem *> *sublist = get_sublist();
		i = sublist ? sublist->size() : 0;
		while( --i >= 0 ) {
			AWindowFolderSubItems *item = (AWindowFolderSubItems *)sublist->get(i);
			if( item->matches(text) ) break;
		}
	}
	return i >= 0 ? 1 : 0;
}


AssetPicon::AssetPicon(MWindow *mwindow,
	AWindowGUI *gui, Indexable *indexable)
 : AWindowFolderItem()
{
	reset();
	this->mwindow = mwindow;
	this->gui = gui;
	this->indexable = indexable;
	indexable->add_user();
	this->id = indexable->id;
}

AssetPicon::AssetPicon(MWindow *mwindow,
	AWindowGUI *gui, EDL *edl)
 : AWindowFolderItem()
{
	reset();
	this->mwindow = mwindow;
	this->gui = gui;
	this->edl = edl;
	edl->add_user();
	this->id = edl->id;
}

AssetPicon::AssetPicon(MWindow *mwindow,
	AWindowGUI *gui, int folder, int persist)
 : AWindowFolderItem(_(AWindowGUI::folder_names[folder]),
	folder>=0 && folder<AWINDOW_FOLDERS ?
		gui->folder_icons[folder] : gui->folder_icon)
{
	reset();
	foldernum = folder;
	this->mwindow = mwindow;
	this->gui = gui;
	persistent = persist;
}

AssetPicon::AssetPicon(MWindow *mwindow,
	AWindowGUI *gui, int folder, const char *title)
 : AWindowFolderItem(title, gui->folder_icon)
{
	reset();
	foldernum = folder;
	this->mwindow = mwindow;
	this->gui = gui;
	persistent = 0;
}

AssetPicon::AssetPicon(MWindow *mwindow,
	AWindowGUI *gui, PluginServer *plugin)
 : AWindowFolderItem()
{
	reset();
	this->mwindow = mwindow;
	this->gui = gui;
	this->plugin = plugin;
}

AssetPicon::AssetPicon(MWindow *mwindow,
	AWindowGUI *gui, Label *label)
 : AWindowFolderItem()
{
	reset();
	this->mwindow = mwindow;
	this->gui = gui;
	this->label = label;
	indexable = 0;
	icon = 0;
	id = 0;
}

AssetPicon::~AssetPicon()
{
	if( vicon ) vicon->remove_user();
	delete vicon_frame;
	if( indexable ) indexable->remove_user();
	if( edl ) edl->remove_user();
	if( icon && !gui->protected_pixmap(icon) ) {
		delete icon;
		if( !plugin ) delete icon_vframe;
	}
}

void AssetPicon::draw_hue_bar(VFrame *frame, double duration)
{
	float t = duration > 1 ? (log(duration) / log(3600.f)) : 0;
	if( t > 1 ) t = 1;
	float h = 300 * t, s = 1., v = 1.;
	float r, g, b; // duration, 0..1hr == hue red..magenta
	HSV::hsv_to_rgb(r,g,b, h,s,v);
	int ih = frame->get_h()/8, iw = frame->get_w();
	int ir = r * 256;  CLAMP(ir, 0,255);
	int ig = g * 256;  CLAMP(ig, 0,255);
	int ib = b * 256;  CLAMP(ib, 0,255);
	unsigned char **rows = frame->get_rows();
	for( int y=0; y<ih; ++y ) {
		unsigned char *rp = rows[y];
		for( int x=0; x<iw; rp+=3,++x ) {
			rp[0] = ir;  rp[1] = ig;  rp[2] = ib;
		}
	}
}

void AssetPicon::draw_wave(VFrame *frame, double *dp, int len,
		int base_color, int line_color, int x, int y, int w, int h)
{
	int h1 = h-1, h2 = h/2, iy = h2;
	int rgb_color = frame->pixel_rgb;
	for( int ix=0,x1=0,x2=0; ix<w; ++ix,x1=x2 ) {
		double min = *dp, max = min;
		x2 = (len * (ix+1))/w;
		for( int i=x1; i<x2; ++i ) {
			double value = *dp++;
			if( value < min ) min = value;
			if( value > max ) max = value;
		}
		int ctr = (min + max) / 2;
		int y0 = (int)(h2 - ctr*h2);  CLAMP(y0, 0,h1);
		int y1 = (int)(h2 - min*h2);  CLAMP(y1, 0,h1);
		int y2 = (int)(h2 - max*h2);  CLAMP(y2, 0,h1);
		frame->pixel_rgb = line_color;
		frame->draw_line(ix+x,y1+y, ix+x,y2+y);
		frame->pixel_rgb = base_color;
		frame->draw_line(ix+x,iy+y, ix+x,y0+y);
		iy = y0;
	}
	frame->pixel_rgb = rgb_color;
}

void AssetPicon::reset()
{
	plugin = 0;
	label = 0;
	indexable = 0;
	edl = 0;
	parent = 0;
	sub_items = 0;
	foldernum = AW_NO_FOLDER;
	sort_key = -1;
	icon = 0;
	icon_vframe = 0;
	vicon = 0;
	vicon_frame = 0;
	in_use = 1;
	comments_time = 0;
	id = 0;
	persistent = 0;
}

void AssetPicon::open_render_engine(EDL *edl, int is_audio)
{
	TransportCommand command;
	command.command = is_audio ? NORMAL_FWD : CURRENT_FRAME;
	command.get_edl()->copy_all(edl);
	command.change_type = CHANGE_ALL;
	command.realtime = 0;
	render_engine = new RenderEngine(0, mwindow->preferences, 0, 0);
	render_engine->set_vcache(mwindow->video_cache);
	render_engine->set_acache(mwindow->audio_cache);
	render_engine->arm_command(&command);
}
void AssetPicon::close_render_engine()
{
	delete render_engine;  render_engine = 0;
}
void AssetPicon::render_video(int64_t pos, VFrame *vfrm)
{
	if( !render_engine || !render_engine->vrender ) return;
	render_engine->vrender->process_buffer(vfrm, pos, 0);
}
void AssetPicon::render_audio(int64_t pos, Samples **samples, int len)
{
	if( !render_engine || !render_engine->arender ) return;
	render_engine->arender->process_buffer(samples, len, pos);
}

void AssetPicon::create_objects()
{
	FileSystem fs;
	char name[BCTEXTLEN];
	int pixmap_w, pixmap_h;

	int picon_h = mwindow->preferences->awindow_picon_h;
	pixmap_h = picon_h * BC_WindowBase::get_resources()->icon_scale;

	if( indexable ) {
		fs.extract_name(name, indexable->path);
		set_text(name);
	}
	else if( edl ) {
		set_text(strcpy(name, edl->local_session->clip_title));
		set_text(name);
	}

	if( indexable && indexable->is_asset ) {
		Asset *asset = (Asset*)indexable;
		if( asset->video_data ) {
			if( mwindow->preferences->use_thumbnails ) {
				gui->unlock_window();
				File *file = mwindow->video_cache->check_out(asset,
					mwindow->edl,
					1);

				if( file ) {
					int height = asset->height > 0 ? asset->height : 1;
					pixmap_w = pixmap_h * asset->width / height;

					file->set_layer(0);
					file->set_video_position(0, 0);

					if( gui->temp_picon &&
						(gui->temp_picon->get_w() != asset->width ||
						gui->temp_picon->get_h() != asset->height) ) {
						delete gui->temp_picon;
						gui->temp_picon = 0;
					}

					if( !gui->temp_picon ) {
						gui->temp_picon = new VFrame(0, -1,
							asset->width, asset->height,
							BC_RGB888, -1);
					}
					{ char string[BCTEXTLEN];
					sprintf(string, _("Reading %s"), name);
					mwindow->gui->lock_window("AssetPicon::create_objects");
					mwindow->gui->show_message(string);
					mwindow->gui->unlock_window(); }
					file->read_frame(gui->temp_picon);
					mwindow->video_cache->check_in(asset);

					gui->lock_window("AssetPicon::create_objects 0");
					icon = new BC_Pixmap(gui, pixmap_w, pixmap_h);
					icon->draw_vframe(gui->temp_picon,
						0, 0, pixmap_w, pixmap_h, 0, 0);
					icon_vframe = new VFrame(0,
						-1, pixmap_w, pixmap_h, BC_RGB888, -1);
					icon_vframe->transfer_from(gui->temp_picon);
					if( asset->folder_no == AW_MEDIA_FOLDER ||
					    asset->folder_no == AW_PROXY_FOLDER ) {
// vicon images
						double framerate = asset->get_frame_rate();
						if( !framerate ) framerate = VICON_RATE;
						int64_t frames = asset->get_video_frames();
						double secs = frames / framerate;
						if( secs > 5 ) secs = 5;
						int64_t length = secs * gui->vicon_thread->refresh_rate;
						vicon = new AssetVIcon(this, pixmap_w, pixmap_h, framerate, length);
						if( asset->audio_data && secs > 0 ) {
							gui->unlock_window();
							int audio_len = VICON_SAMPLE_RATE*secs+0.5;
							vicon->init_audio(audio_len*sizeof(vicon_audio_t));
							vicon->load_audio();
							gui->lock_window("AssetPicon::create_objects 1");
						}
						gui->vicon_thread->add_vicon(vicon);
					}
				}
				else {
					eprintf("Unable to open %s\nin asset: %s",asset->path, get_text());
					gui->lock_window("AssetPicon::create_objects 2");
					icon = gui->video_icon;
					icon_vframe = gui->video_vframe;
				}
			}
			else {
				icon = gui->video_icon;
				icon_vframe = gui->video_vframe;
			}
		}
		else
		if( asset->audio_data ) {
			if( mwindow->preferences->use_thumbnails ) {
				gui->unlock_window();
				File *file = mwindow->audio_cache->check_out(asset,
					mwindow->edl,
					1);
				if( file ) {
					pixmap_w = pixmap_h * 16/9;
					icon_vframe = new VFrame(0,
						-1, pixmap_w, pixmap_h, BC_RGB888, -1);
					icon_vframe->clear_frame();
					{ char string[BCTEXTLEN];
					sprintf(string, _("Reading %s"), name);
					mwindow->gui->lock_window("AssetPicon::create_objects 3");
					mwindow->gui->show_message(string);
					mwindow->gui->unlock_window(); }
					int sample_rate = asset->get_sample_rate();
					int channels = asset->get_audio_channels();
					if( channels > 2 ) channels = 2;
					int64_t audio_samples = asset->get_audio_samples();
					double duration = (double)audio_samples / sample_rate;
					draw_hue_bar(icon_vframe, duration);
					int bfrsz = sample_rate;
					Samples samples(bfrsz);
					static int line_colors[2] = { GREEN, YELLOW };
					static int base_colors[2] = { RED, PINK };
					for( int i=channels; --i>=0; ) {
						file->set_channel(i);
						file->set_audio_position(0);
						file->read_samples(&samples, bfrsz);
						draw_wave(icon_vframe, samples.get_data(), bfrsz,
							base_colors[i], line_colors[i]);
					}
					mwindow->audio_cache->check_in(asset);
					if( asset->folder_no == AW_MEDIA_FOLDER ) {
						double secs = duration;
						if( secs > 5 ) secs = 5;
						double refresh_rate = gui->vicon_thread->refresh_rate;
						int64_t length = secs * refresh_rate;
						vicon = new AssetVIcon(this, pixmap_w, pixmap_h, refresh_rate, length);
						int audio_len = VICON_SAMPLE_RATE*secs+0.5;
						vicon->init_audio(audio_len*sizeof(vicon_audio_t));
						vicon->load_audio();
						gui->vicon_thread->add_vicon(vicon);
					}
					gui->lock_window("AssetPicon::create_objects 4");
					icon = new BC_Pixmap(gui, pixmap_w, pixmap_h);
					icon->draw_vframe(icon_vframe,
						0, 0, pixmap_w, pixmap_h, 0, 0);
				}
				else {
					eprintf("Unable to open %s\nin asset: %s",asset->path, get_text());
					gui->lock_window("AssetPicon::create_objects 5");
					icon = gui->audio_icon;
					icon_vframe = gui->audio_vframe;
				}
			}
			else {
				icon = gui->audio_icon;
				icon_vframe = gui->audio_vframe;
			}

		}
		struct stat st;
		comments_time = !stat(asset->path, &st) ? st.st_mtime : 0;
	}
	else
	if( indexable && !indexable->is_asset ) {
		icon = gui->video_icon;
		icon_vframe = gui->video_vframe;
	}
	else
	if( edl ) {
		if( edl->tracks->playable_video_tracks() ) {
			if( mwindow->preferences->use_thumbnails ) {
				gui->unlock_window();
				AssetVIconThread *avt = gui->vicon_thread;
				char clip_icon_path[BCTEXTLEN];
				char *clip_icon = edl->local_session->clip_icon;
				VFrame *vframe = 0;
				if( clip_icon[0] ) {
					snprintf(clip_icon_path, sizeof(clip_icon_path),
						"%s/%s", File::get_config_path(), clip_icon);
					vframe = VFramePng::vframe_png(clip_icon_path);
				}
				if( vframe &&
				    ( vframe->get_w() != avt->vw || vframe->get_h() != avt->vh ) ) {
					delete vframe;  vframe = 0;
				}
				int edl_h = edl->get_h(), edl_w = edl->get_w();
				int height = edl_h > 0 ? edl_h : 1;
				int width = edl_w > 0 ? edl_w : 1;
				int color_model = edl->session->color_model;
				if( !vframe ) {
//printf("render clip: %s\n", name);
					VFrame::get_temp(gui->temp_picon, width, height, color_model);
					char string[BCTEXTLEN];
					sprintf(string, _("Rendering %s"), name);
					mwindow->gui->lock_window("AssetPicon::create_objects");
					mwindow->gui->show_message(string);
					mwindow->gui->unlock_window();
					open_render_engine(edl, 0);
					render_video(0, gui->temp_picon);
					close_render_engine();
					vframe = new VFrame(avt->vw, avt->vh, BC_RGB888);
					vframe->transfer_from(gui->temp_picon);
					if( clip_icon[0] )
						vframe->write_png(clip_icon_path);
				}
				pixmap_w = pixmap_h * width / height;
				vicon = new AssetVIcon(this, pixmap_w, pixmap_h, VICON_RATE, 1);
				vicon->add_image(vframe, avt->vw, avt->vh, avt->vicon_cmodel);
				icon_vframe = new VFrame(pixmap_w, pixmap_h, BC_RGB888);
				icon_vframe->transfer_from(vframe);
				delete vframe;
				gui->lock_window("AssetPicon::create_objects 0");
				icon = new BC_Pixmap(gui, pixmap_w, pixmap_h);
				icon->draw_vframe(icon_vframe,
					0, 0, pixmap_w, pixmap_h, 0, 0);
			}
			else {
				icon = gui->clip_icon;
				icon_vframe = gui->clip_vframe;
			}
		}
		else
		if( edl->tracks->playable_audio_tracks() ) {
			if( mwindow->preferences->use_thumbnails ) {
				gui->unlock_window();
				char clip_icon_path[BCTEXTLEN];
				char *clip_icon = edl->local_session->clip_icon;
				if( clip_icon[0] ) {
					snprintf(clip_icon_path, sizeof(clip_icon_path),
						"%s/%s", File::get_config_path(), clip_icon);
					icon_vframe = VFramePng::vframe_png(clip_icon_path);
				}
				if( !icon_vframe ) {
					pixmap_w = pixmap_h * 16/9;
					icon_vframe = new VFrame(0,
						-1, pixmap_w, pixmap_h, BC_RGB888, -1);
					icon_vframe->clear_frame();
					char string[BCTEXTLEN];
					sprintf(string, _("Rendering %s"), name);
					mwindow->gui->lock_window("AssetPicon::create_objects 3");
					mwindow->gui->show_message(string);
					mwindow->gui->unlock_window();
					int sample_rate = edl->get_sample_rate();
					int channels = edl->get_audio_channels();
					if( channels > 2 ) channels = 2;
					int64_t audio_samples = edl->get_audio_samples();
					double duration = (double)audio_samples / sample_rate;
					draw_hue_bar(icon_vframe, duration);
					Samples *samples[MAX_CHANNELS];
					int bfrsz = sample_rate;
					for( int i=0; i<MAX_CHANNELS; ++i )
						samples[i] = i<channels ? new Samples(bfrsz) : 0;
					open_render_engine(edl, 1);
					render_audio(0, samples, bfrsz);
					close_render_engine();
					gui->lock_window("AssetPicon::create_objects 4");
					static int line_colors[2] = { GREEN, YELLOW };
					static int base_colors[2] = { RED, PINK };
					for( int i=channels; --i>=0; ) {
						draw_wave(icon_vframe, samples[i]->get_data(), bfrsz,
							base_colors[i], line_colors[i]);
					}
					for( int i=0; i<channels; ++i ) delete samples[i];
					if( clip_icon[0] ) icon_vframe->write_png(clip_icon_path);
				}
				else {
					pixmap_w = icon_vframe->get_w();
					pixmap_h = icon_vframe->get_h();
				}
				icon = new BC_Pixmap(gui, pixmap_w, pixmap_h);
				icon->draw_vframe(icon_vframe,
					0, 0, pixmap_w, pixmap_h, 0, 0);
			}
			else {
				icon = gui->clip_icon;
				icon_vframe = gui->clip_vframe;
			}
		}
	}
	else
	if( plugin ) {
		strcpy(name, _(plugin->title));
		set_text(name);
		icon_vframe = plugin->get_picon();
		if( icon_vframe )
			icon = gui->create_pixmap(icon_vframe);
		else if( plugin->audio ) {
			if( plugin->transition ) {
				icon = gui->atransition_icon;
				icon_vframe = gui->atransition_vframe;
			}
			else if( plugin->is_ffmpeg() ) {
				icon = gui->ff_aud_icon;
				icon_vframe = gui->ff_aud_vframe;
			}
			else if( plugin->is_ladspa() ) {
				icon = gui->ladspa_icon;
				icon_vframe = gui->ladspa_vframe;
			}
			else {
				icon = gui->aeffect_icon;
				icon_vframe = gui->aeffect_vframe;
			}
		}
		else if( plugin->video ) {
			if( plugin->transition ) {
				icon = gui->vtransition_icon;
				icon_vframe = gui->vtransition_vframe;
			}
			else if( plugin->is_ffmpeg() ) {
				icon = gui->ff_vid_icon;
				icon_vframe = gui->ff_vid_vframe;
			}
			else {
				icon = gui->veffect_icon;
				icon_vframe = gui->veffect_vframe;
			}
		}
	}
	else
	if( label ) {
		Units::totext(name,
			      label->position,
			      mwindow->edl->session->time_format,
			      mwindow->edl->session->sample_rate,
			      mwindow->edl->session->frame_rate,
			      mwindow->edl->session->frames_per_foot);
		set_text(name);
		icon = gui->label_icon;
		icon_vframe = gui->label_vframe;
	}
	if( !icon ) {
		icon = gui->file_icon;
		icon_vframe = BC_WindowBase::get_resources()->type_to_icon[ICON_UNKNOWN];
	}
	set_icon(icon);
	set_icon_vframe(icon_vframe);
}

AWindowGUI::AWindowGUI(MWindow *mwindow, AWindow *awindow)
 : BC_Window(_(PROGRAM_NAME ": Resources"),
	mwindow->session->awindow_x, mwindow->session->awindow_y,
	mwindow->session->awindow_w, mwindow->session->awindow_h,
	100, 100, 1, 1, 1)
{
	this->mwindow = mwindow;
	this->awindow = awindow;

	file_vframe = 0;		file_icon = 0;
	folder_vframe = 0;		folder_icon = 0;
	audio_vframe = 0;		audio_icon = 0;
	video_vframe = 0;		video_icon = 0;
	label_vframe = 0;		label_icon = 0;

	aeffect_folder_vframe = 0;	aeffect_folder_icon = 0;
	atransition_folder_vframe = 0;	atransition_folder_icon = 0;
	clip_folder_vframe = 0;		clip_folder_icon = 0;
	label_folder_vframe = 0;	label_folder_icon = 0;
	media_folder_vframe = 0;	media_folder_icon = 0;
	proxy_folder_vframe = 0;	proxy_folder_icon = 0;
	veffect_folder_vframe = 0;	veffect_folder_icon = 0;
	vtransition_folder_vframe = 0;	vtransition_folder_icon = 0;

	ladspa_vframe = 0;		ladspa_icon = 0;
	ff_aud_vframe = 0;		ff_aud_icon = 0;
	ff_vid_vframe = 0;		ff_vid_icon = 0;

	clip_vframe = 0;		clip_icon = 0;
	atransition_vframe = 0;		atransition_icon = 0;
	vtransition_vframe = 0;		vtransition_icon = 0;
	aeffect_vframe = 0;		aeffect_icon = 0;
	veffect_vframe = 0;		veffect_icon = 0;

	plugin_visibility = ((uint64_t)1<<(8*sizeof(uint64_t)-1))-1;
	asset_menu = 0;
	effectlist_menu = 0;
	assetlist_menu = 0;
	cliplist_menu = 0;
	proxylist_menu = 0;
	labellist_menu = 0;
	folderlist_menu = 0;
	temp_picon = 0;
	search_text = 0;
	allow_iconlisting = 1;
	remove_plugin = 0;
	vicon_thread = 0;
	vicon_audio = 0;
	vicon_drawing = 1;
	displayed_folder = AW_NO_FOLDER;
	new_folder_thread = 0;
	modify_folder_thread = 0;
	folder_lock = new Mutex("AWindowGUI::folder_lock");
}

AWindowGUI::~AWindowGUI()
{
	assets.remove_all_objects();
	folders.remove_all_objects();
	aeffects.remove_all_objects();
	veffects.remove_all_objects();
	atransitions.remove_all_objects();
	vtransitions.remove_all_objects();
	labellist.remove_all_objects();
	displayed_assets[1].remove_all_objects();

	delete new_folder_thread;
	delete modify_folder_thread;
	delete vicon_thread;
	delete vicon_audio;

	delete search_text;
	delete temp_picon;
	delete remove_plugin;

	delete file_vframe;		delete file_icon;
	delete folder_vframe;		delete folder_icon;
	delete audio_vframe;		delete audio_icon;
	delete video_vframe;		delete video_icon;
	delete label_vframe;		delete label_icon;
	delete clip_vframe;		delete clip_icon;
	delete aeffect_folder_vframe;	delete aeffect_folder_icon;
	delete atransition_folder_vframe; delete atransition_folder_icon;
	delete veffect_folder_vframe;	delete veffect_folder_icon;
	delete vtransition_folder_vframe; delete vtransition_folder_icon;
	delete clip_folder_vframe;	delete clip_folder_icon;
	delete label_folder_vframe;	delete label_folder_icon;
	delete media_folder_vframe;	delete media_folder_icon;
	delete proxy_folder_vframe;	delete proxy_folder_icon;
	delete ladspa_vframe;		delete ladspa_icon;
	delete ff_aud_vframe;		delete ff_aud_icon;
	delete ff_vid_vframe;		delete ff_vid_icon;
	delete atransition_vframe;	delete atransition_icon;
	delete vtransition_vframe;	delete vtransition_icon;
	delete aeffect_vframe;		delete aeffect_icon;
	delete veffect_vframe;		delete veffect_icon;
	delete folder_lock;
}

bool AWindowGUI::protected_pixmap(BC_Pixmap *icon)
{
	return  icon == file_icon ||
		icon == folder_icon ||
		icon == audio_icon ||
		icon == video_icon ||
		icon == clip_icon ||
		icon == label_icon ||
		icon == vtransition_icon ||
		icon == atransition_icon ||
		icon == veffect_icon ||
		icon == aeffect_icon ||
		icon == ladspa_icon ||
		icon == ff_aud_icon ||
		icon == ff_vid_icon ||
		icon == aeffect_folder_icon ||
		icon == veffect_folder_icon ||
		icon == atransition_folder_icon ||
		icon == vtransition_folder_icon ||
		icon == label_folder_icon ||
		icon == clip_folder_icon ||
		icon == media_folder_icon ||
		icon == proxy_folder_icon;
}

VFrame *AWindowGUI::get_picon(const char *name, const char *plugin_icons)
{
	char png_path[BCTEXTLEN];
	char *pp = png_path, *ep = pp + sizeof(png_path)-1;
	snprintf(pp, ep-pp, "%s/picon/%s/%s.png",
		File::get_plugin_path(), plugin_icons, name);
	if( access(png_path, R_OK) ) return 0;
	return VFramePng::vframe_png(png_path,0,0);
}

VFrame *AWindowGUI::get_picon(const char *name)
{
	VFrame *vframe = get_picon(name, mwindow->preferences->plugin_icons);
	if( !vframe ) {
		char png_name[BCSTRLEN], *pp = png_name, *ep = pp + sizeof(png_name)-1;
		snprintf(pp, ep-pp, "%s.png", name);
		unsigned char *data = mwindow->theme->get_image_data(png_name);
		if( data ) vframe = new VFramePng(data, 0.);
	}
	return vframe;
}

void AWindowGUI::resource_icon(VFrame *&vfrm, BC_Pixmap *&icon, const char *fn, int idx)
{
	vfrm = get_picon(fn);
	if( !vfrm ) vfrm = BC_WindowBase::get_resources()->type_to_icon[idx];
	icon = new BC_Pixmap(this, vfrm, PIXMAP_ALPHA);
}
void AWindowGUI::theme_icon(VFrame *&vfrm, BC_Pixmap *&icon, const char *fn)
{
	vfrm = get_picon(fn);
	if( !vfrm ) vfrm = mwindow->theme->get_image(fn);
	icon = new BC_Pixmap(this, vfrm, PIXMAP_ALPHA);
}
void AWindowGUI::plugin_icon(VFrame *&vfrm, BC_Pixmap *&icon, const char *fn, unsigned char *png)
{
	vfrm = get_picon(fn);
	if( !vfrm ) vfrm = new VFramePng(png);
	icon = new BC_Pixmap(this, vfrm, PIXMAP_ALPHA);
}

void AWindowGUI::create_objects()
{
	lock_window("AWindowGUI::create_objects");
	asset_titles[0] = C_("Title");
	asset_titles[1] = _("Comments");

	set_icon(mwindow->theme->get_image("awindow_icon"));

	resource_icon(file_vframe,   file_icon,   "film_icon",   ICON_UNKNOWN);
	resource_icon(folder_vframe, folder_icon, "folder_icon", ICON_FOLDER);
	resource_icon(audio_vframe,  audio_icon,  "audio_icon",  ICON_SOUND);
	resource_icon(video_vframe,  video_icon,  "video_icon",  ICON_FILM);
	resource_icon(label_vframe,  label_icon,  "label_icon",  ICON_LABEL);

	theme_icon(aeffect_folder_vframe,      aeffect_folder_icon,     "aeffect_folder");
	theme_icon(atransition_folder_vframe,  atransition_folder_icon, "atransition_folder");
	theme_icon(clip_folder_vframe,         clip_folder_icon,        "clip_folder");
	theme_icon(label_folder_vframe,        label_folder_icon,       "label_folder");
	theme_icon(media_folder_vframe,        media_folder_icon,       "media_folder");
	theme_icon(proxy_folder_vframe,        proxy_folder_icon,       "proxy_folder");
	theme_icon(veffect_folder_vframe,      veffect_folder_icon,     "veffect_folder");
	theme_icon(vtransition_folder_vframe,  vtransition_folder_icon, "vtransition_folder");

	folder_icons[AW_AEFFECT_FOLDER] = aeffect_folder_icon;
	folder_icons[AW_VEFFECT_FOLDER] = veffect_folder_icon;
	folder_icons[AW_ATRANSITION_FOLDER] = atransition_folder_icon;
	folder_icons[AW_VTRANSITION_FOLDER] = vtransition_folder_icon;
	folder_icons[AW_LABEL_FOLDER] = label_folder_icon;
	folder_icons[AW_CLIP_FOLDER] = clip_folder_icon;
	folder_icons[AW_MEDIA_FOLDER] = media_folder_icon;
	folder_icons[AW_PROXY_FOLDER] = proxy_folder_icon;

	theme_icon(clip_vframe,        clip_icon,        "clip_icon");
	theme_icon(atransition_vframe, atransition_icon, "atransition_icon");
	theme_icon(vtransition_vframe, vtransition_icon, "vtransition_icon");
	theme_icon(aeffect_vframe,     aeffect_icon,     "aeffect_icon");
	theme_icon(veffect_vframe,     veffect_icon,     "veffect_icon");

	plugin_icon(ladspa_vframe, ladspa_icon, "lad_picon", lad_picon_png);
	plugin_icon(ff_aud_vframe, ff_aud_icon, "ff_audio",  ff_audio_png);
	plugin_icon(ff_vid_vframe, ff_vid_icon, "ff_video",  ff_video_png);
	folder_lock->lock("AWindowGUI::create_objects");
// Mandatory folders
	folders.append(new AssetPicon(mwindow, this, AW_AEFFECT_FOLDER, 1));
	folders.append(new AssetPicon(mwindow, this, AW_VEFFECT_FOLDER, 1));
	folders.append(new AssetPicon(mwindow, this, AW_ATRANSITION_FOLDER, 1));
	folders.append(new AssetPicon(mwindow, this, AW_VTRANSITION_FOLDER, 1));
	folders.append(new AssetPicon(mwindow, this, AW_LABEL_FOLDER, 1));
	folders.append(new AssetPicon(mwindow, this, AW_CLIP_FOLDER, 1));
	folders.append(new AssetPicon(mwindow, this, AW_PROXY_FOLDER, 1));
	folders.append(new AssetPicon(mwindow, this, AW_MEDIA_FOLDER, 1));

	create_label_folder();
	folder_lock->unlock();

	mwindow->theme->get_awindow_sizes(this);
	load_defaults(mwindow->defaults);
	new_folder_thread = new NewFolderThread(this);
	modify_folder_thread = new ModifyFolderThread(this);

	int x1 = mwindow->theme->alist_x, y1 = mwindow->theme->alist_y;
	int w1 = mwindow->theme->alist_w, h1 = mwindow->theme->alist_h;
	search_text = new AWindowSearchText(mwindow, this, x1, y1+5);
	search_text->create_objects();
	int dy = search_text->get_h() + 10;
	y1 += dy;  h1 -= dy;
	add_subwindow(asset_list = new AWindowAssets(mwindow, this, x1, y1, w1, h1));

	vicon_thread = new AssetVIconThread(this, mwindow->preferences);
	asset_list->update_vicon_area();
	vicon_thread->start();
	vicon_audio = new AssetVIconAudio(this);

	add_subwindow(divider = new AWindowDivider(mwindow, this,
		mwindow->theme->adivider_x, mwindow->theme->adivider_y,
		mwindow->theme->adivider_w, mwindow->theme->adivider_h));

	divider->set_cursor(HSEPARATE_CURSOR, 0, 0);

	int fx = mwindow->theme->afolders_x, fy = mwindow->theme->afolders_y;
	int fw = mwindow->theme->afolders_w, fh = mwindow->theme->afolders_h;
	VFrame **images = mwindow->theme->get_image_set("playpatch_data");
	AVIconDrawing::calculate_geometry(this, images, &avicon_w, &avicon_h);
	add_subwindow(avicon_drawing = new AVIconDrawing(this, fw-avicon_w, fy, images));
	add_subwindow(add_tools = new AddTools(mwindow, this, fx, fy, _("Visibility")));
	add_tools->create_objects();
	fy += add_tools->get_h();  fh -= add_tools->get_h();
	add_subwindow(folder_list = new AWindowFolders(mwindow,
		this, fx, fy, fw, fh));
	update_effects();
	folder_list->load_expanders();

	//int x = mwindow->theme->abuttons_x;
	//int y = mwindow->theme->abuttons_y;

	add_subwindow(asset_menu = new AssetPopup(mwindow, this));
	asset_menu->create_objects();
	add_subwindow(clip_menu = new ClipPopup(mwindow, this));
	clip_menu->create_objects();
	add_subwindow(label_menu = new LabelPopup(mwindow, this));
	label_menu->create_objects();
	add_subwindow(proxy_menu = new ProxyPopup(mwindow, this));
	proxy_menu->create_objects();

	add_subwindow(effectlist_menu = new EffectListMenu(mwindow, this));
	effectlist_menu->create_objects();
	add_subwindow(assetlist_menu = new AssetListMenu(mwindow, this));
	assetlist_menu->create_objects();
	add_subwindow(cliplist_menu = new ClipListMenu(mwindow, this));
	cliplist_menu->create_objects();
	add_subwindow(labellist_menu = new LabelListMenu(mwindow, this));
	labellist_menu->create_objects();
	add_subwindow(proxylist_menu = new ProxyListMenu(mwindow, this));
	proxylist_menu->create_objects();

	add_subwindow(folderlist_menu = new FolderListMenu(mwindow, this));
	folderlist_menu->create_objects();

	create_custom_xatoms();
	unlock_window();
}

int AWindowGUI::resize_event(int w, int h)
{
	mwindow->session->awindow_x = get_x();
	mwindow->session->awindow_y = get_y();
	mwindow->session->awindow_w = w;
	mwindow->session->awindow_h = h;

	mwindow->theme->get_awindow_sizes(this);
	mwindow->theme->draw_awindow_bg(this);
	reposition_objects();

//	int x = mwindow->theme->abuttons_x;
//	int y = mwindow->theme->abuttons_y;
// 	new_bin->reposition_window(x, y);
// 	x += new_bin->get_w();
// 	delete_bin->reposition_window(x, y);
// 	x += delete_bin->get_w();
// 	rename_bin->reposition_window(x, y);
// 	x += rename_bin->get_w();
// 	delete_disk->reposition_window(x, y);
// 	x += delete_disk->get_w();
// 	delete_project->reposition_window(x, y);
// 	x += delete_project->get_w();
// 	info->reposition_window(x, y);
// 	x += info->get_w();
// 	redraw_index->reposition_window(x, y);
// 	x += redraw_index->get_w();
// 	paste->reposition_window(x, y);
// 	x += paste->get_w();
// 	append->reposition_window(x, y);
// 	x += append->get_w();
// 	view->reposition_window(x, y);

	BC_WindowBase::resize_event(w, h);
	asset_list->update_vicon_area();
	return 1;
}

int AWindowGUI::translation_event()
{
	mwindow->session->awindow_x = get_x();
	mwindow->session->awindow_y = get_y();
	return 0;
}

void AWindowGUI::reposition_objects()
{
	int x1 = mwindow->theme->alist_x, y1 = mwindow->theme->alist_y;
	int w1 = mwindow->theme->alist_w, h1 = mwindow->theme->alist_h;
	search_text->reposition_window(x1, y1+5, w1);
	int dy = search_text->get_h() + 10;
	y1 += dy;  h1 -= dy;
	asset_list->reposition_window(x1, y1, w1, h1);
	divider->reposition_window(
		mwindow->theme->adivider_x, mwindow->theme->adivider_y,
		mwindow->theme->adivider_w, mwindow->theme->adivider_h);
	int fx = mwindow->theme->afolders_x, fy = mwindow->theme->afolders_y;
	int fw = mwindow->theme->afolders_w, fh = mwindow->theme->afolders_h;
	add_tools->resize_event(fw-avicon_w, add_tools->get_h());
	avicon_drawing->reposition_window(fw-avicon_w, fy);
	fy += add_tools->get_h();  fh -= add_tools->get_h();
	folder_list->reposition_window(fx, fy, fw, fh);
}

int AWindowGUI::save_defaults(BC_Hash *defaults)
{
	defaults->update("PLUGIN_VISIBILTY", plugin_visibility);
	defaults->update("VICON_DRAWING", vicon_drawing);
	return 0;
}

int AWindowGUI::load_defaults(BC_Hash *defaults)
{
	plugin_visibility = defaults->get("PLUGIN_VISIBILTY", plugin_visibility);
	vicon_drawing = defaults->get("VICON_DRAWING", vicon_drawing);
	return 0;
}

int AWindowGUI::close_event()
{
	hide_window();
	mwindow->session->show_awindow = 0;
	unlock_window();

	mwindow->gui->lock_window("AWindowGUI::close_event");
	mwindow->gui->mainmenu->show_awindow->set_checked(0);
	mwindow->gui->unlock_window();

	lock_window("AWindowGUI::close_event");
	save_defaults(mwindow->defaults);
	mwindow->save_defaults();
	return 1;
}

void AWindowGUI::start_vicon_drawing()
{
	if( !vicon_drawing || !vicon_thread->interrupted ) return;
	if( mwindow->edl->session->awindow_folder == AW_MEDIA_FOLDER ||
	    mwindow->edl->session->awindow_folder == AW_PROXY_FOLDER ||
	    mwindow->edl->session->awindow_folder == AW_CLIP_FOLDER ||
	    mwindow->edl->session->awindow_folder >= AWINDOW_USER_FOLDERS ) {
		switch( mwindow->edl->session->assetlist_format ) {
		case ASSETS_ICONS:
		case ASSETS_ICONS_PACKED:
		case ASSETS_ICON_LIST:
			asset_list->update_vicon_area();
			vicon_thread->start_drawing();
			break;
		default:
			break;
		}
	}
}

void AWindowGUI::stop_vicon_drawing()
{
	if( vicon_thread->interrupted ) return;
	vicon_thread->stop_drawing();
}

void AWindowGUI::close_view_popup()
{
	vicon_thread->close_view_popup();
}

VFrame *AssetPicon::get_vicon_frame()
{
	if( !vicon ) return 0;
	if( gui->vicon_thread->interrupted ) return 0;
	VFrame *frame = vicon->frame();
	if( !frame ) return 0;
	if( !vicon_frame )
		vicon_frame = new VFrame(vicon->w, vicon->h, frame->get_color_model());
	vicon_frame->transfer_from(frame);
	return vicon_frame;
}

int AWindowGUI::cycle_assetlist_format()
{
	EDLSession *session = mwindow->edl->session;
	int format = ASSETS_TEXT;
	if( allow_iconlisting ) {
		switch( session->assetlist_format ) {
		case ASSETS_TEXT:
			format = ASSETS_ICONS;
			break;
		case ASSETS_ICONS:
			format = ASSETS_ICONS_PACKED;
			break;
		case ASSETS_ICONS_PACKED:
			format = ASSETS_ICON_LIST;
			break;
		case ASSETS_ICON_LIST:
			format = ASSETS_TEXT;
			break;
		}
	}
	stop_vicon_drawing();
	session->assetlist_format = format;
	asset_list->update_format(session->assetlist_format, 0);
	async_update_assets();
	start_vicon_drawing();
	return 1;
}

AWindowRemovePluginGUI::
AWindowRemovePluginGUI(AWindow *awindow, AWindowRemovePlugin *thread,
	int x, int y, PluginServer *plugin)
 : BC_Window(_(PROGRAM_NAME ": Remove plugin"), x,y, 500,200, 50, 50, 1, 0, 1, -1, "", 1)
{
	this->awindow = awindow;
	this->thread = thread;
	this->plugin = plugin;
	VFrame *vframe = plugin->get_picon();
	icon = vframe ? create_pixmap(vframe) : 0;
	plugin_list.append(new BC_ListBoxItem(plugin->title, icon));
}

AWindowRemovePluginGUI::
~AWindowRemovePluginGUI()
{
	if( !awindow->gui->protected_pixmap(icon) )
		delete icon;
	plugin_list.remove_all();
}

void AWindowRemovePluginGUI::create_objects()
{
	lock_window("AWindowRemovePluginGUI::create_objects");
	BC_Button *ok_button = new BC_OKButton(this);
	add_subwindow(ok_button);
	BC_Button *cancel_button = new BC_CancelButton(this);
	add_subwindow(cancel_button);
	int x = 10, y = 10;
	BC_Title *title = new BC_Title(x, y, _("remove plugin?"));
	add_subwindow(title);
	y += title->get_h() + 5;
	list = new BC_ListBox(x, y,
		get_w() - 20, ok_button->get_y() - y - 5, LISTBOX_TEXT, &plugin_list,
		0, 0, 1, 0, 0, LISTBOX_SINGLE, ICON_LEFT, 0);
	add_subwindow(list);
	show_window();
	unlock_window();
}

int AWindowRemovePlugin::remove_plugin(PluginServer *plugin, ArrayList<BC_ListBoxItem*> &folder)
{
	int ret = 0;
	for( int i=0; i<folder.size(); ) {
		AssetPicon *picon = (AssetPicon *)folder[i];
		if( picon->plugin == plugin ) {
			folder.remove_object_number(i);
			++ret;
			continue;
		}
		++i;
	}
	return ret;
}

void AWindowRemovePlugin::handle_close_event(int result)
{
	if( !result ) {
		printf(_("remove %s\n"), plugin->path);
		awindow->gui->lock_window("AWindowRemovePlugin::handle_close_event");
		ArrayList<BC_ListBoxItem*> *folder =
			plugin->audio ? plugin->transition ?
				&awindow->gui->atransitions :
				&awindow->gui->aeffects :
			plugin->video ?  plugin->transition ?
				&awindow->gui->vtransitions :
				&awindow->gui->veffects :
			0;
		if( folder ) remove_plugin(plugin, *folder);
		MWindow *mwindow = awindow->mwindow;
		awindow->gui->unlock_window();
		char plugin_path[BCTEXTLEN];
		strcpy(plugin_path, plugin->path);
		mwindow->plugindb->remove(plugin);
		remove(plugin_path);
		char index_path[BCTEXTLEN];
		mwindow->create_defaults_path(index_path, PLUGIN_FILE);
		remove(index_path);
		char picon_path[BCTEXTLEN];
		FileSystem fs;
		snprintf(picon_path, sizeof(picon_path), "%s/picon",
			File::get_plugin_path());
		char png_name[BCSTRLEN], png_path[BCTEXTLEN];
		plugin->get_plugin_png_name(png_name);
		fs.update(picon_path);
		for( int i=0; i<fs.dir_list.total; ++i ) {
			char *fs_path = fs.dir_list[i]->path;
			if( !fs.is_dir(fs_path) ) continue;
			snprintf(png_path, sizeof(picon_path), "%s/%s",
				fs_path, png_name);
			remove(png_path);
		}
		delete plugin;  plugin = 0;
		awindow->gui->async_update_assets();
	}
}

AWindowRemovePlugin::
AWindowRemovePlugin(AWindow *awindow, PluginServer *plugin)
 : BC_DialogThread()
{
	this->awindow = awindow;
	this->plugin = plugin;
}

AWindowRemovePlugin::
~AWindowRemovePlugin()
{
	close_window();
}

BC_Window* AWindowRemovePlugin::new_gui()
{
	int x = awindow->gui->get_abs_cursor_x(0);
	int y = awindow->gui->get_abs_cursor_y(0);
	AWindowRemovePluginGUI *gui = new AWindowRemovePluginGUI(awindow, this, x, y, plugin);
	gui->create_objects();
	return gui;
}

int AWindowGUI::keypress_event()
{
	switch( get_keypress() ) {
	case 'w': case 'W':
		if( ctrl_down() ) {
			close_event();
			return 1;
		}
		break;
	case 'o':
		if( !ctrl_down() && !shift_down() ) {
			assetlist_menu->load_file->handle_event();
			return 1;
		}
		break;
	case 'v':
		return cycle_assetlist_format();
	case DELETE:
		if( shift_down() && ctrl_down() ) {
			PluginServer* plugin = selected_plugin();
			if( !plugin ) break;
			remove_plugin = new AWindowRemovePlugin(awindow, plugin);
			unlock_window();
			remove_plugin->start();
			lock_window("AWindowGUI::keypress_event 1");
			return 1;
		}
		collect_assets();
		if( shift_down() ) {
			mwindow->awindow->asset_remove->start();
			return 1;
		}
		unlock_window();
		mwindow->remove_assets_from_project(1, 1,
			mwindow->session->drag_assets,
			mwindow->session->drag_clips);
		lock_window("AWindowGUI::keypress_event 2");
		return 1;
	case KEY_F1:
	case KEY_F2:
	case KEY_F3:
	case KEY_F4:
		if( shift_down() && ctrl_down() ) {
			resend_event(mwindow->gui);
			return 1;
		}
		break;
	}
	return 0;
}



int AWindowGUI::create_custom_xatoms()
{
	UpdateAssetsXAtom = create_xatom("CWINDOWGUI_UPDATE_ASSETS");
	return 0;
}
int AWindowGUI::receive_custom_xatoms(xatom_event *event)
{
	if( event->message_type == UpdateAssetsXAtom ) {
		update_assets();
		return 1;
	}
	return 0;
}

void AWindowGUI::async_update_assets()
{
	xatom_event event;
	event.message_type = UpdateAssetsXAtom;
	send_custom_xatom(&event);
}


void AWindowGUI::update_folder_list()
{
	for( int i = 0; i < folders.total; i++ ) {
		AssetPicon *picon = (AssetPicon*)folders.values[i];
		picon->in_use = 0;
	}

// Search assets for folders
	for( int i = 0; i < mwindow->edl->folders.total; i++ ) {
		BinFolder *bin_folder = mwindow->edl->folders[i];
		int exists = 0;

		for( int j = 0; j < folders.total; j++ ) {
			AssetPicon *picon = (AssetPicon*)folders[j];
			if( !strcasecmp(picon->get_text(), bin_folder->title) ) {
				exists = 1;
				picon->in_use = 1;
				break;
			}
		}

		if( !exists ) {
			const char *title = bin_folder->title;
			int folder = bin_folder->awindow_folder;
			AssetPicon *picon = new AssetPicon(mwindow, this, folder, title);
			picon->create_objects();
			folders.append(picon);
		}
	}

// Delete unused non-persistent folders
	int do_autoplace = 0;
	for( int i=folders.total; --i>=0; ) {
		AssetPicon *picon = (AssetPicon*)folders.values[i];
		if( !picon->in_use && !picon->persistent ) {
			delete picon;
			folders.remove_number(i);
			do_autoplace = 1;
		}
	}
	if( do_autoplace )
		folder_list->set_autoplacement(&folders, 0, 1);
}

void AWindowGUI::create_persistent_folder(ArrayList<BC_ListBoxItem*> *output,
	int do_audio, int do_video, int is_realtime, int is_transition)
{
	ArrayList<PluginServer*> plugin_list;
// Get pointers to plugindb entries
	mwindow->search_plugindb(do_audio, do_video, is_realtime, is_transition,
			0, plugin_list);

	for( int i = 0; i < plugin_list.total; i++ ) {
		PluginServer *server = plugin_list.values[i];
		int visible = plugin_visibility & (1<<server->dir_idx);
		if( !visible ) continue;
// Create new listitem
		AssetPicon *picon = new AssetPicon(mwindow, this, server);
		picon->create_objects();
		output->append(picon);
	}
}

void AWindowGUI::create_label_folder()
{
	Label *current;
	for( current = mwindow->edl->labels->first; current; current = NEXT ) {
		AssetPicon *picon = new AssetPicon(mwindow, this, current);
		picon->create_objects();
		labellist.append(picon);
	}
}


void AWindowGUI::update_asset_list()
{
	ArrayList<AssetPicon *> new_assets;
	for( int i = 0; i < assets.total; i++ ) {
		AssetPicon *picon = (AssetPicon*)assets.values[i];
		picon->in_use = 0;
	}

	mwindow->gui->lock_window("AWindowGUI::update_asset_list");
// Synchronize EDL clips
	for( int i=0; i<mwindow->edl->clips.size(); ++i ) {
		int exists = 0;

// Look for clip in existing listitems
		for( int j = 0; j < assets.total && !exists; j++ ) {
			AssetPicon *picon = (AssetPicon*)assets.values[j];

			if( picon->id == mwindow->edl->clips[i]->id ) {
				picon->edl = mwindow->edl->clips[i];
				picon->set_text(mwindow->edl->clips[i]->local_session->clip_title);
				exists = 1;
				picon->in_use = 1;
			}
		}

// Create new listitem
		if( !exists ) {
			AssetPicon *picon = new AssetPicon(mwindow,
				this, mwindow->edl->clips[i]);
			new_assets.append(picon);
		}
	}

// Synchronize EDL assets
	for( Asset *current=mwindow->edl->assets->first; current; current=NEXT ) {
		int exists = 0;

// Look for asset in existing listitems
		for( int j = 0; j < assets.total && !exists; j++ ) {
			AssetPicon *picon = (AssetPicon*)assets.values[j];

			if( picon->id == current->id ) {
				picon->indexable = current;
				picon->in_use = 1;
				exists = 1;
			}
		}

// Create new listitem
		if( !exists ) {
			AssetPicon *picon = new AssetPicon(mwindow,
				this, current);
			new_assets.append(picon);
		}
	}

// Synchronize nested EDLs
	for( int i=0; i<mwindow->edl->nested_edls.size(); ++i ) {
		int exists = 0;
		EDL *nested_edl = mwindow->edl->nested_edls[i];

// Look for asset in existing listitems
		for( int j=0; j<assets.total && !exists; ++j ) {
			AssetPicon *picon = (AssetPicon*)assets.values[j];

			if( picon->id == nested_edl->id ) {
				picon->indexable = nested_edl;
				picon->in_use = 1;
				exists = 1;
			}
		}

// Create new listitem
		if( !exists ) {
			AssetPicon *picon = new AssetPicon(mwindow,
				this, (Indexable*)nested_edl);
			new_assets.append(picon);
		}
	}
	mwindow->gui->unlock_window();

	for( int i=0; i<new_assets.size(); ++i ) {
		AssetPicon *picon = new_assets[i];
		picon->create_objects();
		if( picon->indexable )
			picon->foldernum = AW_MEDIA_FOLDER;
		else if( picon->edl )
			picon->foldernum = AW_CLIP_FOLDER;
		assets.append(picon);
	}

	mwindow->gui->lock_window();
	mwindow->gui->default_message();
	mwindow->gui->unlock_window();

	for( int i = assets.size() - 1; i >= 0; i-- ) {
		AssetPicon *picon = (AssetPicon*)assets.get(i);
		if( !picon->in_use ) {
			delete picon;
			assets.remove_number(i);
			continue;
		}
		if( picon->indexable && picon->indexable->is_asset ) {
			struct stat st;
			picon->comments_time = !stat(picon->indexable->path, &st) ?
				st.st_mtime : 0;
		}
	}
}

void AWindowGUI::update_picon(Indexable *indexable)
{
	VIcon *vicon = 0;
	for( int i = 0; i < assets.total; i++ ) {
		AssetPicon *picon = (AssetPicon*)assets.values[i];
		if( picon->indexable == indexable ||
		    picon->edl == (EDL *)indexable ) {
			char name[BCTEXTLEN];
			FileSystem fs;
			fs.extract_name(name, indexable->path);
			picon->set_text(name);
			vicon = picon->vicon;
			break;
		}
	}
	if( vicon ) {
		stop_vicon_drawing();
		vicon->clear_images();
		vicon->reset(indexable->get_frame_rate());
		start_vicon_drawing();
	}
}

void AWindowGUI::sort_assets()
{
	folder_lock->lock("AWindowGUI::sort_assets");
	switch( mwindow->edl->session->awindow_folder ) {
	case AW_AEFFECT_FOLDER:
		sort_picons(&aeffects);
		break;
	case AW_VEFFECT_FOLDER:
		sort_picons(&veffects);
		break;
	case AW_ATRANSITION_FOLDER:
		sort_picons(&atransitions);
		break;
	case AW_VTRANSITION_FOLDER:
		sort_picons(&vtransitions);
		break;
	case AW_LABEL_FOLDER:
		sort_picons(&labellist);
		break;
	default:
		sort_picons(&assets);
		break;
	}
// reset xyposition
	asset_list->update_format(asset_list->get_format(), 0);
	folder_lock->unlock();
	update_assets();
}

void AWindowGUI::sort_folders()
{
	folder_lock->lock("AWindowGUI::update_assets");
//	folder_list->collapse_recursive(&folders, 0);
	folder_list->set_autoplacement(&folders, 0, 1);
	sort_picons(&folders);
	folder_list->update_format(folder_list->get_format(), 0);
	folder_lock->unlock();
	update_assets();
}

EDL *AWindowGUI::collect_proxy(Indexable *indexable)
{
	Asset *proxy_asset = (Asset *)indexable;
	char path[BCTEXTLEN];
	int proxy_scale = mwindow->edl->session->proxy_scale;
	ProxyRender::from_proxy_path(path, proxy_asset, proxy_scale);
	Asset *unproxy_asset = mwindow->edl->assets->get_asset(path);
	if( !unproxy_asset || !unproxy_asset->layers ) return 0;
// make a clip from proxy video tracks and unproxy audio tracks
	EDL *proxy_edl = new EDL(mwindow->edl);
	proxy_edl->create_objects();
	proxy_edl->set_path(proxy_asset->path);
	FileSystem fs;  fs.extract_name(path, proxy_asset->path);
	strcpy(proxy_edl->local_session->clip_title, path);
	strcpy(proxy_edl->local_session->clip_notes, _("Proxy clip"));
	proxy_edl->session->video_tracks = proxy_asset->layers;
	proxy_edl->session->audio_tracks = unproxy_asset->channels;
	proxy_edl->create_default_tracks();
	double length = proxy_asset->frame_rate > 0 ?
		( proxy_asset->video_length >= 0 ?
			( proxy_asset->video_length / proxy_asset->frame_rate ) :
			( proxy_edl->session->si_useduration ?
				proxy_edl->session->si_duration :
				1.0 / proxy_asset->frame_rate ) ) :
		1.0 / proxy_edl->session->frame_rate;
	Track *current = proxy_edl->tracks->first;
	for( int vtrack=0; current; current=NEXT ) {
		if( current->data_type != TRACK_VIDEO ) continue;
		current->insert_asset(proxy_asset, 0, length, 0, vtrack++);
	}
	length = (double)unproxy_asset->audio_length / unproxy_asset->sample_rate;
	current = proxy_edl->tracks->first;
	for( int atrack=0; current; current=NEXT ) {
		if( current->data_type != TRACK_AUDIO ) continue;
		current->insert_asset(unproxy_asset, 0, length, 0, atrack++);
	}
	proxy_edl->folder_no = AW_PROXY_FOLDER;
	return proxy_edl;
}


void AWindowGUI::collect_assets(int proxy)
{
	mwindow->session->drag_assets->remove_all();
	mwindow->session->drag_clips->remove_all();
	int i = 0;  AssetPicon *result;
	while( (result = (AssetPicon*)asset_list->get_selection(0, i++)) != 0 ) {
		Indexable *indexable = result->indexable;
		if( proxy && indexable && indexable->is_asset &&
		    indexable->folder_no == AW_PROXY_FOLDER ) {
			EDL *drag_edl = collect_proxy(indexable);
			if( drag_edl ) mwindow->session->drag_clips->append(drag_edl);
			continue;
		}
		if( indexable ) {
			mwindow->session->drag_assets->append(indexable);
			continue;
		}
		if( result->edl ) {
			mwindow->session->drag_clips->append(result->edl);
			continue;
		}
	}
}

void AWindowGUI::copy_picons(AssetPicon *picon, ArrayList<BC_ListBoxItem*> *src)
{
// Remove current pointers
	ArrayList<BC_ListBoxItem*> *dst = displayed_assets;
	dst[0].remove_all();
	dst[1].remove_all_objects();

	AWindowFolderSubItems *sub_items = picon ? picon->sub_items : 0;
	int folder = mwindow->edl->session->awindow_folder;
	BinFolder *bin_folder = folder < AWINDOW_USER_FOLDERS ? 0 :
		mwindow->edl->get_folder(folder);

// Create new pointers
	for( int i = 0; i < src->total; i++ ) {
		int visible = folder >= AW_CLIP_FOLDER ? 0 : 1;
		picon = (AssetPicon*)src->values[i];
		picon->sort_key = -1;
		if( !visible && bin_folder ) {
			Indexable *idxbl = bin_folder->is_clips ? (Indexable *)picon->edl :
			    picon->indexable ? picon->indexable :
			    picon->edl ? picon->edl->get_proxy_asset() : 0;
			if( idxbl ) {
				picon->sort_key = mwindow->edl->folders.matches_indexable(folder, idxbl);
				if( picon->sort_key < 0 ) continue;
				visible = 1;
			}
		}
		if( !visible && picon->indexable && picon->indexable->folder_no == folder )
			visible = 1;
		if( !visible && picon->edl && picon->edl->folder_no == folder )
			visible = 1;
		if( visible && sub_items ) {
			if( !sub_items->matches(picon->get_text()) )
				visible = 0;
		}
		if( visible ) {
			const char *text = search_text->get_text();
			if( text && text[0] )
				visible = bstrcasestr(picon->get_text(), text) ? 1 : 0;
		}
		if( visible && picon->vicon && picon->vicon->hidden )
			picon->vicon->hidden = 0;
		if( visible ) {
			BC_ListBoxItem *item2, *item1;
			dst[0].append(item1 = picon);
			if( picon->edl )
				dst[1].append(item2 = new BC_ListBoxItem(picon->edl->local_session->clip_notes));
			else
			if( picon->label )
				dst[1].append(item2 = new BC_ListBoxItem(picon->label->textstr));
			else if( picon->comments_time ) {
				char date_time[BCSTRLEN];
				struct tm stm;  localtime_r(&picon->comments_time, &stm);
				sprintf(date_time,"%04d.%02d.%02d %02d:%02d:%02d",
					 stm.tm_year+1900, stm.tm_mon+1, stm.tm_mday,
					 stm.tm_hour, stm.tm_min, stm.tm_sec);
				dst[1].append(item2 = new BC_ListBoxItem(date_time));
			}
			else
				dst[1].append(item2 = new BC_ListBoxItem(""));
			item1->set_autoplace_text(1);  item1->set_autoplace_icon(1);
			item2->set_autoplace_text(1);  item2->set_autoplace_icon(1);
		}
	}
}

void AWindowGUI::sort_picons(ArrayList<BC_ListBoxItem*> *src)
{
	int done = 0, changed = 0;
	while( !done ) {
		done = 1;
		for( int i=0; i<src->total-1; ++i ) {
			AssetPicon *item1 = (AssetPicon *)src->values[i];
			AssetPicon *item2 = (AssetPicon *)src->values[i + 1];
			double v = item2->sort_key - item1->sort_key;
			if( v > 0 ) continue;
			if( v == 0 ) {
				const char *cp1 = item1->get_text();
				const char *bp1 = strrchr(cp1, '/');
				if( bp1 ) cp1 = bp1 + 1;
				const char *cp2 = item2->get_text();
				const char *bp2 = strrchr(cp2, '/');
				if( bp2 ) cp2 = bp2 + 1;
				if( strcmp(cp2, cp1) >= 0 ) continue;
			}
			src->values[i + 1] = item1;
			src->values[i] = item2;
			done = 0;  changed = 1;
		}
	}
	if( changed ) {
		for( int i=0; i<src->total; ++i ) {
			AssetPicon *item = (AssetPicon *)src->values[i];
			item->set_autoplace_icon(1);
			item->set_autoplace_text(1);
		}
	}
}

void AWindowGUI::filter_displayed_assets()
{
	//allow_iconlisting = 1;
	asset_titles[0] = C_("Title");
	asset_titles[1] = _("Comments");
	AssetPicon *picon = 0;
	int selected_folder = mwindow->edl->session->awindow_folder;
	// Ensure the current folder icon is highlighted
	for( int i = 0; i < folders.total; i++ ) {
		AssetPicon *folder_item = (AssetPicon *)folders.values[i];
		int selected = folder_item->foldernum == selected_folder ? 1 : 0;
		folder_item->set_selected(selected);
		if( selected ) picon = folder_item;
	}

	ArrayList<BC_ListBoxItem*> *src = &assets;
	switch( selected_folder ) {
	case AW_AEFFECT_FOLDER:  src = &aeffects;  break;
	case AW_VEFFECT_FOLDER:  src = &veffects;  break;
	case AW_ATRANSITION_FOLDER:  src = &atransitions;  break;
	case AW_VTRANSITION_FOLDER:  src = &vtransitions;  break;
	case AW_LABEL_FOLDER:  src = &labellist;
		asset_titles[0] = _("Time Stamps");
		asset_titles[1] = C_("Title");
		//allow_iconlisting = 0;
		break;
	}
	copy_picons(picon, src);
}


void AWindowGUI::update_assets()
{
	stop_vicon_drawing();
	folder_lock->lock("AWindowGUI::update_assets");
	update_folder_list();
	update_asset_list();
	labellist.remove_all_objects();
	create_label_folder();

	if( displayed_folder != mwindow->edl->session->awindow_folder )
		search_text->clear();
	vicon_thread->hide_vicons();
	filter_displayed_assets();
	folder_lock->unlock();

	if( mwindow->edl->session->folderlist_format != folder_list->get_format() ) {
		folder_list->update_format(mwindow->edl->session->folderlist_format, 0);
	}
	int folder_xposition = folder_list->get_xposition();
	int folder_yposition = folder_list->get_yposition();
	folder_list->update(&folders, 0, 0, 1, folder_xposition, folder_yposition, -1);

	if( mwindow->edl->session->assetlist_format != asset_list->get_format() ) {
		asset_list->update_format(mwindow->edl->session->assetlist_format, 0);
	}
	int asset_xposition = asset_list->get_xposition();
	int asset_yposition = asset_list->get_yposition();
	if( displayed_folder != mwindow->edl->session->awindow_folder ) {
		displayed_folder = mwindow->edl->session->awindow_folder;
		asset_xposition = asset_yposition = 0;
	}
	asset_list->update(displayed_assets, asset_titles,
		mwindow->edl->session->asset_columns, ASSET_COLUMNS,
		asset_xposition, asset_yposition, -1, 0);
	asset_list->center_selection();

	flush();
	start_vicon_drawing();
	return;
}

void AWindowGUI::update_effects()
{
	aeffects.remove_all_objects();
	create_persistent_folder(&aeffects, 1, 0, 1, 0);
	veffects.remove_all_objects();
	create_persistent_folder(&veffects, 0, 1, 1, 0);
	atransitions.remove_all_objects();
	create_persistent_folder(&atransitions, 1, 0, 0, 1);
	vtransitions.remove_all_objects();
	create_persistent_folder(&vtransitions, 0, 1, 0, 1);
}

int AWindowGUI::drag_motion()
{
	if( get_hidden() ) return 0;

	int result = 0;
	return result;
}

int AWindowGUI::drag_stop()
{
	if( get_hidden() ) return 0;
	return 0;
}

Indexable* AWindowGUI::selected_asset()
{
	AssetPicon *picon = (AssetPicon*)asset_list->get_selection(0, 0);
	return picon ? picon->indexable : 0;
}

PluginServer* AWindowGUI::selected_plugin()
{
	AssetPicon *picon = (AssetPicon*)asset_list->get_selection(0, 0);
	return picon ? picon->plugin : 0;
}

AssetPicon* AWindowGUI::selected_folder()
{
	AssetPicon *picon = (AssetPicon*)folder_list->get_selection(0, 0);
	return picon;
}








AWindowDivider::AWindowDivider(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
AWindowDivider::~AWindowDivider()
{
}

int AWindowDivider::button_press_event()
{
	if( is_event_win() && cursor_inside() ) {
		mwindow->session->current_operation = DRAG_PARTITION;
		return 1;
	}
	return 0;
}

int AWindowDivider::cursor_motion_event()
{
	if( mwindow->session->current_operation == DRAG_PARTITION ) {
		int wmin = 25;
		int wmax = mwindow->session->awindow_w - mwindow->theme->adivider_w - wmin;
		int fw = gui->get_relative_cursor_x();
		if( fw > wmax ) fw = wmax;
		if( fw < wmin ) fw = wmin;
		mwindow->session->afolders_w = fw;
		mwindow->theme->get_awindow_sizes(gui);
		gui->reposition_objects();
		gui->flush();
	}
	return 0;
}

int AWindowDivider::button_release_event()
{
	if( mwindow->session->current_operation == DRAG_PARTITION ) {
		mwindow->session->current_operation = NO_OPERATION;
		return 1;
	}
	return 0;
}






AWindowFolders::AWindowFolders(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h,
		mwindow->edl->session->folderlist_format == FOLDERS_ICONS ?
			LISTBOX_ICONS : LISTBOX_TEXT,
		&gui->folders,    // Each column has an ArrayList of BC_ListBoxItems.
		0,                // Titles for columns.  Set to 0 for no titles
		0,                // width of each column
		1,                // Total columns.
		0,                // Pixel of top of window.
		0,                // If this listbox is a popup window
		LISTBOX_SINGLE,   // Select one item or multiple items
		ICON_TOP,         // Position of icon relative to text of each item
		1)                // Allow drags
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_drag_scroll(0);
	last_item0 = 0;
	last_item1 = 0;
}

AWindowFolders::~AWindowFolders()
{
}

int AWindowFolders::selection_changed()
{
	AWindowFolderItem *item0 = (AWindowFolderItem*)get_selection(0, 0);
	AWindowFolderItem *item1 = (AWindowFolderItem*)get_selection(0, 1);
// prefer expanded entry
	AWindowFolderItem *item = item1 ? item1 : item0;
	if( item0 && item1 && last_item0 == item0 && last_item1 == item1 ) {
		item1->set_selected(0);
		item1 = 0;
		item = item0;
	}
	last_item0 = item0;
	last_item1 = item1;
	if( item ) {
		AssetPicon *picon = item->get_picon();
		picon->sub_items = (AWindowFolderSubItems*)(!item->parent ? 0 : item);

		gui->stop_vicon_drawing();

		if( get_button_down() && get_buttonpress() == RIGHT_BUTTON ) {
			gui->folderlist_menu->update_titles();
			gui->folderlist_menu->activate_menu();
		}

		mwindow->edl->session->awindow_folder = picon->foldernum;
		gui->asset_list->draw_background();
		gui->async_update_assets();

		gui->start_vicon_drawing();
	}
	return 1;
}

int AWindowFolders::button_press_event()
{
	int result = BC_ListBox::button_press_event();

	if( !result ) {
		if( get_buttonpress() == RIGHT_BUTTON &&
		    is_event_win() && cursor_inside() ) {
			gui->folderlist_menu->update_titles();
			gui->folderlist_menu->activate_menu();
			result = 1;
		}
	}

	return result;
}

int AWindowFolders::drag_stop()
{
	int result = 0;
	if( get_hidden() ) return 0;
	if( mwindow->session->current_operation == DRAG_ASSET &&
	    gui->folder_list->cursor_above() ) { // check user folder
		int item_no = gui->folder_list->get_cursor_data_item_no();
		AssetPicon *picon = (AssetPicon *)(item_no < 0 ? 0 : gui->folders[item_no]);
		if( picon && picon->foldernum >= AWINDOW_USER_FOLDERS ) {
			BinFolder *folder = mwindow->edl->get_folder(picon->foldernum);
			ArrayList<Indexable *> *drags = folder->is_clips ?
				((ArrayList<Indexable *> *)mwindow->session->drag_clips) :
				((ArrayList<Indexable *> *)mwindow->session->drag_assets);
			if( folder && drags && !folder->add_patterns(drags, shift_down()) )
				flicker(1,30);
			mwindow->session->current_operation = ::NO_OPERATION;
			result = 1;
		}
	}
	return result;
}

AWindowFolderSubItems::AWindowFolderSubItems(AWindowFolderItem *parent, const char *text)
 : AWindowFolderItem(text)
{
	this->parent = parent;
}

int AWindowFolders::load_expanders()
{
	char expanders_path[BCTEXTLEN];
	mwindow->create_defaults_path(expanders_path, EXPANDERS_FILE);
	FILE *fp = fopen(expanders_path, "r");
	if( !fp ) {
		snprintf(expanders_path, sizeof(expanders_path), "%s/%s",
			File::get_cindat_path(), EXPANDERS_FILE);
		fp = fopen(expanders_path, "r");
	}

	if( !fp ) return 1;
	const char tab = '\t';
	char line[BCTEXTLEN];   line[0] = 0;
	AWindowFolderItem *item = 0, *parent;
	AWindowFolderSubItems *sub_items = 0;
	int k = 0;
	while( fgets(line,sizeof(line),fp) ) {
		if( line[0] == '#' ) continue;
		int i = strlen(line);
		if( i > 0 && line[i-1] == '\n' ) line[--i] = 0;
		if( i == 0 ) continue;
		i = 0;
		for( char *cp=line; *cp==tab; ++cp ) ++i;
		if( i == 0 ) {
			int i = gui->folders.size();
			while( --i >= 0 ) {
				AssetPicon *folder = (AssetPicon *)gui->folders[i];
				if( !strcmp(folder->get_text(),_(line)) ) break;
			}
			item = (AWindowFolderItem*)(i >= 0 ? gui->folders[i] : 0);
			sub_items = 0;
			k = 0;
			continue;
		}
		if( i > k+1 ) continue;
		if( i == k+1 ) {
			if( line[i] != '-' && sub_items ) {
				sub_items->names.append(cstrdup(_(&line[i])));
				continue;
			}
			parent = item;
			k = i;
		}
		else {
			while( i < k ) {
				item = item->parent;
				--k;
			}
			parent = item->parent;
		}
		ArrayList<BC_ListBoxItem*> *sublist = parent->get_sublist();
		if( !sublist ) sublist = parent->new_sublist(1);
		sub_items = new AWindowFolderSubItems(parent, &line[i]);
		sublist->append(item = sub_items);
	}
	fclose(fp);
	return 0;
}


AWindowAssets::AWindowAssets(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, !gui->allow_iconlisting ? LISTBOX_TEXT :
		mwindow->edl->session->assetlist_format == ASSETS_ICONS ? LISTBOX_ICONS :
		mwindow->edl->session->assetlist_format == ASSETS_ICONS_PACKED ? LISTBOX_ICONS_PACKED :
		mwindow->edl->session->assetlist_format == ASSETS_ICON_LIST ? LISTBOX_ICON_LIST :
			LISTBOX_TEXT,
		&gui->assets,  	  // Each column has an ArrayList of BC_ListBoxItems.
		gui->asset_titles,// Titles for columns.  Set to 0 for no titles
		mwindow->edl->session->asset_columns, // width of each column
		1,                // Total columns.
		0,                // Pixel of top of window.
		0,                // If this listbox is a popup window
		LISTBOX_MULTIPLE, // Select one item or multiple items
		ICON_TOP,         // Position of icon relative to text of each item
		-1)               // Allow drags, require shift for scrolling
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_drag_scroll(0);
	set_scroll_stretch(1, 1);
}

AWindowAssets::~AWindowAssets()
{
}

int AWindowAssets::button_press_event()
{
	AssetVIconThread *avt = gui->vicon_thread;
	if( avt->draw_mode != ASSET_VIEW_NONE && is_event_win() ) {
		int dir = 1, button = get_buttonpress();
		switch( button ) {
		case WHEEL_DOWN: dir = -1;  // fall thru
		case WHEEL_UP: {
			int x = get_cursor_x(), y = get_cursor_y();
			if( avt->cursor_inside(x, y) && avt->view_win )
				return avt->zoom_scale(dir);
			return 1; }
		}
	}

	int result = BC_ListBox::button_press_event();

	if( !result && get_buttonpress() == RIGHT_BUTTON &&
	    is_event_win() && cursor_inside() ) {
		BC_ListBox::deactivate_selection();
		int folder = mwindow->edl->session->awindow_folder;
		switch( folder ) {
		case AW_AEFFECT_FOLDER:
		case AW_VEFFECT_FOLDER:
		case AW_ATRANSITION_FOLDER:
		case AW_VTRANSITION_FOLDER:
			gui->effectlist_menu->update();
			gui->effectlist_menu->activate_menu();
			break;
		case AW_LABEL_FOLDER:
			gui->labellist_menu->update();
			gui->labellist_menu->activate_menu();
			break;
		case AW_CLIP_FOLDER:
			gui->cliplist_menu->update();
			gui->cliplist_menu->activate_menu();
			break;
		case AW_PROXY_FOLDER:
			gui->proxylist_menu->update();
			gui->proxylist_menu->activate_menu();
			break;
		default:
		case AW_MEDIA_FOLDER: {
			int shots =  folder==AW_MEDIA_FOLDER || folder>=AWINDOW_USER_FOLDERS;
			gui->assetlist_menu->update_titles(shots);
			gui->assetlist_menu->activate_menu();
			break; }
		}
		result = 1;
	}

	return result;
}


int AWindowAssets::handle_event()
{
	AssetPicon *asset_picon = (AssetPicon *)get_selection(0, 0);
	if( !asset_picon ) return 0;
	Indexable *picon_idxbl = asset_picon->indexable;
	EDL *picon_edl = asset_picon->edl;
	int proxy = 0;
	VWindow *vwindow = 0;
	switch( mwindow->edl->session->awindow_folder ) {
	case AW_AEFFECT_FOLDER:
	case AW_VEFFECT_FOLDER:
	case AW_ATRANSITION_FOLDER:
	case AW_VTRANSITION_FOLDER: return 1;
	case AW_PROXY_FOLDER:
		proxy = 1; // fall thru
	default:
		if( mwindow->vwindows.size() > DEFAULT_VWINDOW )
			vwindow = mwindow->vwindows.get(DEFAULT_VWINDOW);
		break;
	}
	if( !vwindow || !vwindow->is_running() ) return 1;
	if( proxy && picon_idxbl ) {
		picon_edl = gui->collect_proxy(picon_idxbl);
		picon_idxbl = 0;
	}

	if( picon_idxbl ) vwindow->change_source(picon_idxbl);
	else if( picon_edl ) vwindow->change_source(picon_edl);
	return 1;
}

int AWindowAssets::selection_changed()
{
// Show popup window
	AssetPicon *item;
	int folder = mwindow->edl->session->awindow_folder;
	if( get_button_down() && get_buttonpress() == RIGHT_BUTTON &&
	    (item = (AssetPicon*)get_selection(0, 0)) ) {
		switch( folder ) {
		case AW_AEFFECT_FOLDER:
		case AW_VEFFECT_FOLDER:
		case AW_ATRANSITION_FOLDER:
		case AW_VTRANSITION_FOLDER:
			gui->effectlist_menu->update();
			gui->effectlist_menu->activate_menu();
			break;
		case AW_LABEL_FOLDER:
			if( !item->label ) break;
			gui->label_menu->activate_menu();
			break;
		case AW_CLIP_FOLDER:
			if( !item->indexable && !item->edl ) break;
			gui->clip_menu->update();
			gui->clip_menu->activate_menu();
			break;
		case AW_PROXY_FOLDER:
			if( !item->indexable && !item->edl ) break;
			gui->proxy_menu->update();
			gui->proxy_menu->activate_menu();
			break;
		default:
			if( !item->indexable && !item->edl ) break;
			gui->asset_menu->update();
			gui->asset_menu->activate_menu();
			break;
		}

		deactivate_selection();
	}
	else if( gui->vicon_drawing && get_button_down() &&
		(item = (AssetPicon*)get_selection(0, 0)) != 0 ) {
		switch( folder ) {
		case AW_MEDIA_FOLDER:
		case AW_PROXY_FOLDER:
		case AWINDOW_USER_FOLDERS:
			if( get_buttonpress() == LEFT_BUTTON ||
			    get_buttonpress() == MIDDLE_BUTTON ) {
				AssetVIcon *vicon = 0;
				if( !gui->vicon_thread->vicon )
					vicon = item->vicon;
				int draw_mode = vicon && get_buttonpress() == MIDDLE_BUTTON ?
					ASSET_VIEW_MEDIA_MAP : ASSET_VIEW_MEDIA;
				gui->vicon_thread->set_view_popup(vicon, draw_mode);
			}
			break;
		case AW_CLIP_FOLDER:
			if( get_buttonpress() == LEFT_BUTTON ) {
				AssetVIcon *vicon = 0;
				if( !gui->vicon_thread->vicon )
					vicon = item->vicon;
				gui->vicon_thread->set_view_popup(vicon, ASSET_VIEW_ICON);
			}
			break;
		}
	}
	return 1;
}

void AWindowAssets::draw_background()
{
	clear_box(0,0,get_w(),get_h(),get_bg_surface());
	set_color(BC_WindowBase::get_resources()->audiovideo_color);
	set_font(LARGEFONT);
	int folder = mwindow->edl->session->awindow_folder;
	const char *title = mwindow->edl->get_folder_name(folder);
	draw_text(get_w() - get_text_width(LARGEFONT, title) - 4, 30,
		title, -1, get_bg_surface());
}

int AWindowAssets::drag_start_event()
{
	int collect_pluginservers = 0;
	int collect_assets = 0, proxy = 0;

	if( BC_ListBox::drag_start_event() ) {
		switch( mwindow->edl->session->awindow_folder ) {
		case AW_AEFFECT_FOLDER:
			mwindow->session->current_operation = DRAG_AEFFECT;
			collect_pluginservers = 1;
			break;
		case AW_VEFFECT_FOLDER:
			mwindow->session->current_operation = DRAG_VEFFECT;
			collect_pluginservers = 1;
			break;
		case AW_ATRANSITION_FOLDER:
			mwindow->session->current_operation = DRAG_ATRANSITION;
			collect_pluginservers = 1;
			break;
		case AW_VTRANSITION_FOLDER:
			mwindow->session->current_operation = DRAG_VTRANSITION;
			collect_pluginservers = 1;
			break;
		case AW_LABEL_FOLDER:
			// do nothing!
			break;
		case AW_PROXY_FOLDER:
			proxy = 1; // fall thru
		case AW_MEDIA_FOLDER:
		default:
			mwindow->session->current_operation = DRAG_ASSET;
			collect_assets = 1;
			break;
		}

		if( collect_pluginservers ) {
			int i = 0;
			mwindow->session->drag_pluginservers->remove_all();
			while(1)
			{
				AssetPicon *result = (AssetPicon*)get_selection(0, i++);
				if( !result ) break;

				mwindow->session->drag_pluginservers->append(result->plugin);
			}
		}

		if( collect_assets ) {
			gui->collect_assets(proxy);
		}

		return 1;
	}
	return 0;
}

int AWindowAssets::drag_motion_event()
{
	BC_ListBox::drag_motion_event();
	unlock_window();

	mwindow->gui->lock_window("AWindowAssets::drag_motion_event");
	mwindow->gui->drag_motion();
	mwindow->gui->unlock_window();

	for( int i = 0; i < mwindow->vwindows.size(); i++ ) {
		VWindow *vwindow = mwindow->vwindows.get(i);
		if( !vwindow->is_running() ) continue;
		vwindow->gui->lock_window("AWindowAssets::drag_motion_event");
		vwindow->gui->drag_motion();
		vwindow->gui->unlock_window();
	}

	mwindow->cwindow->gui->lock_window("AWindowAssets::drag_motion_event");
	mwindow->cwindow->gui->drag_motion();
	mwindow->cwindow->gui->unlock_window();

	lock_window("AWindowAssets::drag_motion_event");
	if( mwindow->session->current_operation == DRAG_ASSET &&
	    gui->folder_list->cursor_above() ) { // highlight user folder
		BC_ListBoxItem *item = 0;
		int item_no = gui->folder_list->get_cursor_data_item_no(&item);
		if( item_no >= 0 ) {
			AssetPicon *folder = (AssetPicon *)gui->folders[item_no];
			if( folder->foldernum < AWINDOW_USER_FOLDERS ) item_no = -1;
		}
		if( item_no >= 0 )
			item_no = gui->folder_list->item_to_index(&gui->folders, item);
		int folder_xposition = gui->folder_list->get_xposition();
		int folder_yposition = gui->folder_list->get_yposition();
		gui->folder_list->update(&gui->folders, 0, 0, 1,
			folder_xposition, folder_yposition, item_no, 0, 1);
	}
	return 0;
}

int AWindowAssets::drag_stop_event()
{
	int result = 0;

	result = gui->drag_stop();

	unlock_window();

	if( !result ) {
		mwindow->gui->lock_window("AWindowAssets::drag_stop_event");
		result = mwindow->gui->drag_stop();
		mwindow->gui->unlock_window();
	}

	if( !result ) {
		for( int i = 0; !result && i < mwindow->vwindows.size(); i++ ) {
			VWindow *vwindow = mwindow->vwindows.get(i);
			if( !vwindow ) continue;
			if( !vwindow->is_running() ) continue;
			if( vwindow->gui->is_hidden() ) continue;
			vwindow->gui->lock_window("AWindowAssets::drag_stop_event");
			if( vwindow->gui->cursor_above() &&
			    vwindow->gui->get_cursor_over_window() ) {
				result = vwindow->gui->drag_stop();
			}
			vwindow->gui->unlock_window();
		}
	}

	if( !result ) {
		mwindow->cwindow->gui->lock_window("AWindowAssets::drag_stop_event");
		result = mwindow->cwindow->gui->drag_stop();
		mwindow->cwindow->gui->unlock_window();
	}

	lock_window("AWindowAssets::drag_stop_event");
	if( !result ) {
		result = gui->folder_list->drag_stop();
	}


	if( result )
		get_drag_popup()->set_animation(0);

	BC_ListBox::drag_stop_event();
// since NO_OPERATION is also defined in listbox, we have to reach for global scope...
	mwindow->session->current_operation = ::NO_OPERATION;

	return 1;
}

int AWindowAssets::column_resize_event()
{
	mwindow->edl->session->asset_columns[0] = get_column_width(0);
	mwindow->edl->session->asset_columns[1] = get_column_width(1);
	return 1;
}

int AWindowAssets::focus_in_event()
{
	int ret = BC_ListBox::focus_in_event();
	gui->start_vicon_drawing();
	return ret;
}

int AWindowAssets::focus_out_event()
{
	gui->stop_vicon_drawing();
	return BC_ListBox::focus_out_event();
}

void AWindowAssets::update_vicon_area()
{
	int x0 = 0, x1 = get_w();
	int y0 = get_title_h();
	int y1 = get_h();
	if( is_highlighted() ) {
		x0 += LISTBOX_BORDER;  x1 -= LISTBOX_BORDER;
		y0 += LISTBOX_BORDER;  y1 -= LISTBOX_BORDER;
	}
	gui->vicon_thread->set_drawing_area(x0,y0, x1,y1);
}

int AWindowAssets::mouse_over_event(int no)
{
	if( gui->vicon_thread->viewing &&
	    no >= 0 && no < gui->displayed_assets[0].size() ) {
		AssetPicon *picon = (AssetPicon *)gui->displayed_assets[0][no];
		gui->vicon_thread->set_view_popup(picon->vicon);
	}
	return 0;
}


AWindowSearchTextBox::AWindowSearchTextBox(AWindowSearchText *search_text, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, "")
{
	this->search_text = search_text;
}

int AWindowSearchTextBox::handle_event()
{
	return search_text->handle_event();
}

AWindowSearchText::AWindowSearchText(MWindow *mwindow, AWindowGUI *gui, int x, int y)
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->x = x;
	this->y = y;
}

void AWindowSearchText::create_objects()
{
	int x1 = x, y1 = y, margin = 10;
	gui->add_subwindow(text_title = new BC_Title(x1, y1, _("Search:")));
	x1 += text_title->get_w() + margin;
	int w1 = gui->get_w() - x1 - 2*margin;
	gui->add_subwindow(text_box = new AWindowSearchTextBox(this, x1, y1, w1));
}

int AWindowSearchText::handle_event()
{
	gui->async_update_assets();
	return 1;
}

int AWindowSearchText::get_w()
{
	return text_box->get_w() + text_title->get_w() + 10;
}

int AWindowSearchText::get_h()
{
	return bmax(text_box->get_h(),text_title->get_h());
}

void AWindowSearchText::reposition_window(int x, int y, int w)
{
	int x1 = x, y1 = y, margin = 10;
	text_title->reposition_window(x1, y1);
	x1 += text_title->get_w() + margin;
	int w1 = gui->get_w() - x1 - 2*margin;
	text_box->reposition_window(x1, y1, w1);
}

const char *AWindowSearchText::get_text()
{
	return text_box->get_text();
}

void AWindowSearchText::clear()
{
	text_box->update("");
}

AWindowDeleteDisk::AWindowDeleteDisk(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->deletedisk_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete asset from disk"));
}

int AWindowDeleteDisk::handle_event()
{
	return 1;
}

AWindowDeleteProject::AWindowDeleteProject(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->deleteproject_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete asset from project"));
}

int AWindowDeleteProject::handle_event()
{
	return 1;
}

// AWindowInfo::AWindowInfo(MWindow *mwindow, AWindowGUI *gui, int x, int y)
//  : BC_Button(x, y, mwindow->theme->infoasset_data)
// {
// 	this->mwindow = mwindow;
// 	this->gui = gui;
// 	set_tooltip(_("Edit information on asset"));
// }
// 
// int AWindowInfo::handle_event()
// {
// 	int cur_x, cur_y;
// 	gui->get_abs_cursor(cur_x, cur_y, 0);
// 	gui->awindow->asset_edit->edit_asset(gui->selected_asset(), cur_x, cur_y);
// 	return 1;
// }

AWindowRedrawIndex::AWindowRedrawIndex(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->redrawindex_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Redraw index"));
}

int AWindowRedrawIndex::handle_event()
{
	return 1;
}

AWindowPaste::AWindowPaste(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->pasteasset_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Paste asset on recordable tracks"));
}

int AWindowPaste::handle_event()
{
	return 1;
}

AWindowAppend::AWindowAppend(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->appendasset_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Append asset in new tracks"));
}

int AWindowAppend::handle_event()
{
	return 1;
}

AWindowView::AWindowView(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->viewasset_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("View asset"));
}

int AWindowView::handle_event()
{
	return 1;
}

AddTools::AddTools(MWindow *mwindow, AWindowGUI *gui, int x, int y, const char *title)
 : BC_PopupMenu(x, y, BC_Title::calculate_w(gui, title, MEDIUMFONT)+8, title, -1, 0, 4)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

void AddTools::create_objects()
{
	uint64_t vis = 0;
	add_item(new AddPluginItem(this, "ffmpeg", PLUGIN_FFMPEG_ID));
	vis |= 1 << PLUGIN_FFMPEG_ID;
	add_item(new AddPluginItem(this, "ladspa", PLUGIN_LADSPA_ID));
	vis |= 1 << PLUGIN_LADSPA_ID;
#ifdef HAVE_LV2
	add_item(new AddPluginItem(this, "lv2", PLUGIN_LV2_ID));
	vis |= 1 << PLUGIN_LV2_ID;
#endif
	for( int i=0; i<MWindow::plugindb->size(); ++i ) {
		PluginServer *plugin = MWindow::plugindb->get(i);
		if( !plugin->audio && !plugin->video ) continue;
		int idx = plugin->dir_idx;
		uint32_t msk = 1 << idx;
		if( (msk & vis) != 0 ) continue;
		vis |= msk;
		char parent[BCTEXTLEN];
		strcpy(parent, plugin->path);
		char *bp = strrchr(parent, '/');
		if( bp ) { *bp = 0;  bp = strrchr(parent, '/'); }
		if( !bp ) bp = parent; else ++bp;
		add_item(new AddPluginItem(this, bp, idx));
	}
}

#if 0
// plugin_dirs list from toplevel makefile include plugin_defs
N_("ffmpeg")
N_("ladspa")
N_("lv2")
N_("audio_tools")
N_("audio_transitions")
N_("blending")
N_("colors")
N_("exotic")
N_("transforms")
N_("tv_effects")
N_("video_tools")
N_("video_transitions")
#endif

AddPluginItem::AddPluginItem(AddTools *menu, char const *text, int idx)
 : BC_MenuItem(_(text))
{
	this->menu = menu;
	this->idx = idx;
	uint64_t msk = (uint64_t)1 << idx, vis = menu->gui->plugin_visibility;
	int chk = (msk & vis) ? 1 : 0;
	set_checked(chk);
}

int AddPluginItem::handle_event()
{
	int chk = get_checked() ^ 1;
	set_checked(chk);
	uint64_t msk = (uint64_t)1 << idx, vis = menu->gui->plugin_visibility;
	menu->gui->plugin_visibility = chk ? vis | msk : vis & ~msk;
	menu->gui->update_effects();
	menu->gui->save_defaults(menu->mwindow->defaults);
	menu->gui->async_update_assets();
	return 1;
}

AVIconDrawing::AVIconDrawing(AWindowGUI *agui, int x, int y, VFrame **images)
 : BC_Toggle(x, y, images, agui->vicon_drawing)
{
	this->agui = agui;
	set_tooltip(_("Preview"));
}

void AVIconDrawing::calculate_geometry(AWindowGUI *agui, VFrame **images, int *ww, int *hh)
{
	int text_line = -1, toggle_x = -1, toggle_y = -1;
	int text_x = -1, text_y = -1, text_w = -1, text_h = -1;
	BC_Toggle::calculate_extents(agui, images, 1,
		&text_line, ww, hh, &toggle_x, &toggle_y,
		&text_x, &text_y, &text_w, &text_h, "", MEDIUMFONT);
}

AVIconDrawing::~AVIconDrawing()
{
}

int AVIconDrawing::handle_event()
{
	agui->vicon_drawing = get_value();
	if( agui->vicon_drawing )
		agui->start_vicon_drawing();
	else
		agui->stop_vicon_drawing();
	return 1;
}


AWindowListFormat::AWindowListFormat(MWindow *mwindow, AWindowGUI *gui)
 : BC_MenuItem("","v",'v')
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int AWindowListFormat::handle_event()
{
	return gui->cycle_assetlist_format();
}

void AWindowListFormat::update()
{
	EDLSession *session = mwindow->edl->session;
	const char *text = 0;
	switch( session->assetlist_format ) {
	case ASSETS_TEXT:
		text = _("Display icons");
		break;
	case ASSETS_ICONS:
		text = _("Display icons packed");
		break;
	case ASSETS_ICONS_PACKED:
		text = _("Display icon list");
		break;
	case ASSETS_ICON_LIST:
		text = _("Display text");
		break;
	}
	set_text(text);
}

AWindowListSort::AWindowListSort(MWindow *mwindow, AWindowGUI *gui)
 : BC_MenuItem(_("Sort items"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int AWindowListSort::handle_event()
{
	gui->sort_assets();
	return 1;
}

AssetSelectUsedItem::AssetSelectUsedItem(AssetSelectUsed *select_used, const char *text, int action)
 : BC_MenuItem(text)
{
	this->select_used = select_used;
	this->action = action;
}

int AssetSelectUsedItem::handle_event()
{
	MWindow *mwindow = select_used->mwindow;
	AWindowGUI *gui = select_used->gui;
	AWindowAssets *asset_list = gui->asset_list;
	ArrayList<BC_ListBoxItem*> *data = gui->displayed_assets;

	switch( action ) {
	case SELECT_ALL:
	case SELECT_NONE:
		asset_list->set_all_selected(data, action==SELECT_ALL ? 1 : 0);
		break;
	case SELECT_USED:
	case SELECT_UNUSED:
		asset_list->set_all_selected(data, 0);
		for( int i = 0; i < data->total; i++ ) {
			AssetPicon *picon = (AssetPicon*)data->values[i];
			Indexable *idxbl = picon->indexable ? picon->indexable :
			    picon->edl ? picon->edl->get_proxy_asset() : 0;
			int used = idxbl && mwindow->edl->in_use(idxbl) ? 1 : 0;
			asset_list->set_selected(data, i, action==SELECT_USED ? used : !used);
		}
		break;
	}

	int asset_xposition = asset_list->get_xposition();
	int asset_yposition = asset_list->get_yposition();
	asset_list->update(gui->displayed_assets, gui->asset_titles,
		mwindow->edl->session->asset_columns, ASSET_COLUMNS,
		asset_xposition, asset_yposition, -1, 0);
	asset_list->center_selection();
	return 1;
}

AssetSelectUsed::AssetSelectUsed(MWindow *mwindow, AWindowGUI *gui)
 : BC_MenuItem(_("Select"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

