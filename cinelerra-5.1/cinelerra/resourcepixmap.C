
/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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
#include "aedit.h"
#include "asset.h"
#include "asset.inc"
#include "automation.h"
#include "autos.h"
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "bccmodels.h"
#include "datatype.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "floatauto.h"
#include "floatautos.h"
#include "framecache.h"
#include "indexfile.h"
#include "language.h"
#include "localsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "renderengine.h"
#include "resourcethread.h"
#include "resourcepixmap.h"
#include "samples.h"
#include "theme.h"
#include "timelinepane.h"
#include "track.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "vedit.h"
#include "vframe.h"
#include "wavecache.h"


ResourcePixmap::ResourcePixmap(MWindow *mwindow,
	MWindowGUI *gui,
	Edit *edit,
	int pane_number,
	int w,
	int h)
 : BC_Pixmap(gui, w, h)
{
	reset();

	this->mwindow = mwindow;
	this->gui = gui;
	this->pane_number = pane_number;
	startsource = edit->startsource;
	data_type = edit->track->data_type;
	if( edit->asset ) {
		source_framerate = edit->asset->frame_rate;
		source_samplerate = edit->asset->sample_rate;
	}
	else
	if( edit->nested_edl ) {
		source_framerate = edit->nested_edl->session->frame_rate;
		source_samplerate = edit->nested_edl->session->sample_rate;
	}

	project_framerate = edit->edl->session->frame_rate;
	project_samplerate = edit->edl->session->sample_rate;
	edit_id = edit->id;  pixmap_w = w;  pixmap_h = h;
}

ResourcePixmap::~ResourcePixmap()
{
}


void ResourcePixmap::reset()
{
	edit_x = 0;
	pixmap_x = 0;
	pixmap_w = 0;
	pixmap_h = 0;
	zoom_sample = 0;
	zoom_track = 0;
	zoom_y = 0;
	visible = 1;
}

void ResourcePixmap::resize(int w, int h)
{
	int new_w = (w > get_w()) ? w : get_w();
	int new_h = (h > get_h()) ? h : get_h();

	BC_Pixmap::resize(new_w, new_h);
}

void ResourcePixmap::update_settings(Edit *edit,
	int64_t edit_x, int64_t edit_w,
	int64_t pixmap_x, int64_t pixmap_w, int64_t pixmap_h)
{
	this->edit_id = edit->id;
	this->edit_x = edit_x;
	this->pixmap_x = pixmap_x;
	this->pixmap_w = pixmap_w;
	this->pixmap_h = pixmap_h;

	startsource = edit->startsource;
	if( edit->asset )
		source_framerate = edit->asset->frame_rate;
	else
	if( edit->nested_edl )
		source_framerate = edit->nested_edl->session->frame_rate;
	if( edit->asset )
		source_samplerate = edit->asset->sample_rate;
	else if( edit->nested_edl )
		source_samplerate = edit->nested_edl->session->sample_rate;

	project_framerate = edit->edl->session->frame_rate;
	project_samplerate = edit->edl->session->sample_rate;
	zoom_sample = mwindow->edl->local_session->zoom_sample;
	zoom_track = mwindow->edl->local_session->zoom_track;
	zoom_y = mwindow->edl->local_session->zoom_y;
}

void ResourcePixmap::draw_data(TrackCanvas *canvas,
	Edit *edit, int64_t edit_x, int64_t edit_w,
	int64_t pixmap_x, int64_t pixmap_w, int64_t pixmap_h,
	int mode, int indexes_only)
{
// Get new areas to fill in relative to pixmap
// Area to redraw relative to pixmap
	int refresh_x = 0;
	int refresh_w = 0;

// Ignore if called by resourcethread.
//	if( mode == IGNORE_THREAD ) return;

	int y = 0;
	if( edit->track->show_titles() )
		y += mwindow->theme->get_image("title_bg_data")->get_h();

// If want indexes only & index can't be drawn, don't do anything.
	int need_redraw = 0;
	int64_t index_zoom = 0;
	Indexable *indexable = 0;
	if( edit->asset ) indexable = edit->asset;
	if( edit->nested_edl ) indexable = edit->nested_edl;
	if( indexable && indexes_only ) {
		IndexFile indexfile(mwindow, indexable);
		if( !indexfile.open_index() ) {
			index_zoom = indexable->index_state->index_zoom;
			indexfile.close_index();
		}

		if( index_zoom ) {
			if( data_type == TRACK_AUDIO ) {
				double asset_over_session = (double)indexable->get_sample_rate() /
					mwindow->edl->session->sample_rate;
				if( index_zoom <= mwindow->edl->local_session->zoom_sample *
					asset_over_session )
					need_redraw = 1;
			}
		}

		if( !need_redraw )
			return;
	}

/* Incremental drawing is not possible with resource thread */
// Redraw everything
	refresh_x = 0;
	refresh_w = pixmap_w;

// Draw background image
	if( refresh_w > 0 ) {
		int x1 = refresh_x, x2 = x1 + refresh_w;
		int y1 = y, y2 = y1 + mwindow->edl->local_session->zoom_track;
		int color = mwindow->get_title_color(edit);
		mwindow->theme->draw_resource_bg(canvas, this, color,
			edit_x, edit_w, pixmap_x, x1,y1, x2,y2);
	}
//printf("ResourcePixmap::draw_data 70\n");


// Draw media which already exists
	Track *track = edit->track;
	if( track->draw ) {
		switch( track->data_type )
		{
			case TRACK_AUDIO:
				draw_audio_resource(canvas,
					edit, refresh_x, refresh_w);
				break;

			case TRACK_VIDEO:
				draw_video_resource(canvas, edit, edit_x, edit_w,
					pixmap_x, pixmap_w, refresh_x, refresh_w,
					mode);
				break;

			case TRACK_SUBTITLE:
				draw_subttl_resource(canvas, edit,
					refresh_x, refresh_w);
				break;
		}
	}
SET_TRACE
}


VFrame *ResourcePixmap::change_title_color(VFrame *title_bg, int color)
{
	int colormodel = title_bg->get_color_model();
	int bpp = BC_CModels::calculate_pixelsize(colormodel);
	int tw = title_bg->get_w(), tw1 = tw-1, th = title_bg->get_h();
	VFrame *title_bar = new VFrame(tw, th, colormodel);
	uint8_t cr = (color>>16), cg = (color>>8), cb = (color>>0);
	uint8_t **bar_rows = title_bar->get_rows();
	if( th > 0 ) {
		uint8_t *cp = bar_rows[0];
		for( int x=0; x<tw; ++x ) {
			cp[0] = cp[1] = cp[2] = 0;
			if( bpp > 3 ) cp[3] = 0xff;
			cp += bpp;
		}
	}
	for( int y=1; y<th; ++y ) {
		uint8_t *cp = bar_rows[y];
		if( tw > 0 ) {
			cp[0] = cp[1] = cp[2] = 0;
			if( bpp > 3 ) cp[3] = 0xff;
			cp += bpp;
		}
		for( int x=1; x<tw1; ++x ) {
			cp[0] = cr; cp[1] = cg; cp[2] = cb;
			if( bpp > 3 ) cp[3] = 0xff;
			cp += bpp;
		}
		if( tw > 1 ) {
			cp[0] = cp[1] = cp[2] = 0;
			if( bpp > 3 ) cp[3] = 0xff;
		}
	}
	return title_bar;
}

VFrame *ResourcePixmap::change_picon_alpha(VFrame *picon_frame, int alpha)
{
	uint8_t **picon_rows = picon_frame->get_rows();
	int w = picon_frame->get_w(), h = picon_frame->get_h();
	VFrame *frame = new VFrame(w, h, BC_RGBA8888);
	uint8_t **rows = frame->get_rows();
	for( int y=0; y<h; ++y ) {
		uint8_t *bp = picon_rows[y], *rp = rows[y];
		for( int x=0; x<w; ++x ) {
			rp[0] = bp[0];	rp[1] = bp[1];
			rp[2] = bp[2];	bp += 3;
			rp[3] = alpha;  rp += 4;
		}
	}
	return frame;
}

void ResourcePixmap::draw_title(TrackCanvas *canvas,
	Edit *edit, int64_t edit_x, int64_t edit_w,
	int64_t pixmap_x, int64_t pixmap_w)
{
// coords relative to pixmap
	int64_t total_x = edit_x - pixmap_x, total_w = edit_w;
	int64_t x = total_x, w = total_w;
	int left_margin = 10;

	if( x < 0 ) { w -= -x; x = 0; }
	if( w > pixmap_w ) w -= w - pixmap_w;

	VFrame *title_bg = mwindow->theme->get_image("title_bg_data");
	int color = mwindow->get_title_color(edit);
	VFrame *title_bar = !color ? title_bg :
		change_title_color(title_bg, color);
	canvas->draw_3segmenth(x, 0, w, total_x, total_w, title_bar, this);
	if( title_bar != title_bg ) delete title_bar;

//	if( total_x > -BC_INFINITY ) {
		char title[BCTEXTLEN];
		edit->get_title(title);
		canvas->set_color(mwindow->theme->title_color);
		canvas->set_font(mwindow->theme->title_font);

// Justify the text on the left boundary of the edit if it is visible.
// Otherwise justify it on the left side of the screen.
		int text_x = total_x + left_margin;
		text_x = MAX(left_margin, text_x);
//printf("ResourcePixmap::draw_title 1 %d\n", text_x);
		canvas->draw_text(text_x,
			canvas->get_text_ascent(mwindow->theme->title_font) + 2,
			title, strlen(title), this);
//	}
}


// Need to draw one more x
void ResourcePixmap::draw_audio_resource(TrackCanvas *canvas, Edit *edit, int x, int w)
{
	if( w <= 0 ) return;
	if( !edit->asset && !edit->nested_edl ) return;
	Indexable *indexable = 0;
	if( edit->asset ) indexable = edit->asset;
	if( edit->nested_edl ) indexable = edit->nested_edl;
// printf("ResourcePixmap::draw_audio_resource %d x=%d w=%d\n", __LINE__, x, w);
SET_TRACE

	IndexState *index_state = indexable->index_state;
	double asset_over_session = (double)indexable->get_sample_rate() /
		mwindow->edl->session->sample_rate;

// Develop strategy for drawing
// printf("ResourcePixmap::draw_audio_resource %d %p %d\n",
// __LINE__,
// index_state,
// index_state->index_status);
	switch( index_state->index_status )
	{
		case INDEX_NOTTESTED:
			return;
			break;
// Disabled.  All files have an index.
//		case INDEX_TOOSMALL:
//			draw_audio_source(canvas, edit, x, w);
//			break;
		case INDEX_BUILDING:
		case INDEX_READY:
		{
			IndexFile indexfile(mwindow, indexable);
			if( !indexfile.open_index() ) {
				if( index_state->index_zoom >
						mwindow->edl->local_session->zoom_sample *
						asset_over_session ) {
//printf("ResourcePixmap::draw_audio_resource %d\n", __LINE__);

					draw_audio_source(canvas, edit, x, w);
				}
				else {
//printf("ResourcePixmap::draw_audio_resource %d\n", __LINE__);
					indexfile.draw_index(canvas,
						this,
						edit,
						x,
						w);
SET_TRACE
				}

				indexfile.close_index();
SET_TRACE
			}
			break;
		}
	}
}


void ResourcePixmap::draw_audio_source(TrackCanvas *canvas, Edit *edit, int x, int w)
{
	w++;
	Indexable *indexable = edit->get_source();
	int center_pixel = mwindow->edl->local_session->zoom_track / 2;
	if( edit->track->show_titles() )
		center_pixel += mwindow->theme->get_image("title_bg_data")->get_h();
	int64_t scale_y = mwindow->edl->local_session->zoom_y;
	int y_max = center_pixel + scale_y / 2 - 1;

	double project_zoom = mwindow->edl->local_session->zoom_sample;
	FloatAutos *speed_autos = !edit->track->has_speed() ? 0 :
		(FloatAutos *)edit->track->automation->autos[AUTOMATION_SPEED];
	int64_t edit_position = (x + pixmap_x - edit_x) * project_zoom;
	int64_t start_position = edit->startsource;
	start_position += !speed_autos ? edit_position :
		speed_autos->automation_integral(edit->startproject, edit_position, PLAY_FORWARD);
	int64_t end_position = edit->startsource;
	edit_position = (x + w + pixmap_x - edit_x) * project_zoom;
	end_position += !speed_autos ? edit_position :
		speed_autos->automation_integral(edit->startproject, edit_position, PLAY_FORWARD);

	double session_sample_rate = mwindow->edl->session->sample_rate;
	double asset_over_session = (double)indexable->get_sample_rate() / session_sample_rate;
	start_position *= asset_over_session;
	end_position *= asset_over_session;
	int sample_size = end_position - start_position;
	if( sample_size < 0 ) sample_size = 0;
	int source_samples = sample_size + 1;

// Single sample zoom
	if( mwindow->edl->local_session->zoom_sample == 1 ) {
		Samples *buffer = 0;
		int result = 0;
		canvas->set_color(mwindow->theme->audio_color);

		if( indexable->is_asset ) {
			mwindow->gui->unlock_window();
			File *source = mwindow->audio_cache->check_out(edit->asset, mwindow->edl);
			mwindow->gui->lock_window("draw_audio_source");

			if( !source ) {
				printf(_("ResourcePixmap::draw_audio_source: failed to check out %s for drawing.\n"), edit->asset->path);
				return;
			}

			source->set_audio_position(start_position);
			source->set_channel(edit->channel);
			buffer = new Samples(source_samples);
			result = source->read_samples(buffer, source_samples);
			mwindow->audio_cache->check_in(edit->asset);
		}
		else {
			if( mwindow->gui->render_engine &&
				mwindow->gui->render_engine_id != indexable->id ) {
				delete mwindow->gui->render_engine;
				mwindow->gui->render_engine = 0;
			}

			if( !mwindow->gui->render_engine ) {
				TransportCommand command;
				command.command = NORMAL_FWD;
				command.get_edl()->copy_all(edit->nested_edl);
				command.change_type = CHANGE_ALL;
				command.realtime = 0;
				mwindow->gui->render_engine = new RenderEngine(0,
					mwindow->preferences, 0, 0);
				mwindow->gui->render_engine_id = edit->nested_edl->id;
				mwindow->gui->render_engine->set_acache(mwindow->audio_cache);
				mwindow->gui->render_engine->arm_command(&command);
			}

			if( mwindow->gui->render_engine->arender ) {
				Samples *buffers[MAX_CHANNELS];
				memset(buffers, 0, sizeof(buffers));
				int nch = indexable->get_audio_channels(), ch = edit->channel;
				for( int i=0; i<nch; ++i )
					buffers[i] = new Samples(source_samples);
				mwindow->gui->render_engine->arender->process_buffer(
					buffers, source_samples, start_position);
				for( int i=0; i<nch; ++i )
					if( i != ch ) delete buffers[i];
				buffer = buffers[ch];
			}
		}

		if( !result ) {
			double *samples = buffer->get_data();
			int y1 = center_pixel - samples[0] * scale_y / 2;
			int y2 = CLIP(y1, 0, y_max);

			for( int x0=0; x0<w; ++x0 ) {
				int x1 = x0 + x, x2 = x1 + 1;
				edit_position = (x1 + pixmap_x - edit_x) * project_zoom;
				int64_t speed_position = edit->startsource;
				speed_position += !speed_autos ? edit_position :
					speed_autos->automation_integral(
						edit->startproject, edit_position, PLAY_FORWARD);
				int j = speed_position * asset_over_session - start_position;
				CLAMP(j, 0, sample_size);
				int y0 = y2;
				y1 = center_pixel - samples[j] * scale_y / 2;
				y2 = CLIP(y1, 0, y_max);
//printf("ResourcePixmap::draw_audio_source %d %d %d\n", __LINE__, y1, y2);
				canvas->draw_line(x0, y0, x2, y2, this);
			}
		}

		delete buffer;
	}
	else {
		edit_position = (x + pixmap_x - edit_x) * project_zoom;
		int64_t speed_position = edit->startsource;
		speed_position += !speed_autos ? edit_position :
			speed_autos->automation_integral(
				edit->startproject, edit_position, PLAY_FORWARD);
		int64_t next_position = asset_over_session * speed_position;
// Multiple sample zoom
		int first_pixel = 1, prev_y1 = -1, prev_y2 = y_max;
		canvas->set_color(mwindow->theme->audio_color);
		++x;

// Draw each pixel from the cache
//printf("ResourcePixmap::draw_audio_source %d x=%d w=%d\n", __LINE__, x, w);
		for( int x2=x+w; x<x2; ++x ) {
			int64_t prev_position = next_position;
			edit_position = (x + pixmap_x - edit_x) * project_zoom;
			speed_position = edit->startsource;
			speed_position += !speed_autos ? edit_position :
				speed_autos->automation_integral(
					edit->startproject, edit_position, PLAY_FORWARD);
			next_position = speed_position * asset_over_session;
// Starting sample of pixel relative to asset rate.
			WaveCacheItem *item = mwindow->wave_cache->get_wave(indexable->id,
					edit->channel, prev_position, next_position);
			if( item ) {
//printf("ResourcePixmap::draw_audio_source %d\n", __LINE__);
				int y_lo = (int)(center_pixel - item->low * scale_y / 2);
				int y1 = CLIP(y_lo, 0, y_max);
				int y_hi = (int)(center_pixel - item->high * scale_y / 2);
				int y2 = CLIP(y_hi, 0, y_max);
				if( !first_pixel ) {
					y_lo = MIN(y1,prev_y2);
					y_hi = MAX(y2,prev_y1);
				}
				else {
					first_pixel = 0;
					y_lo = y1;  y_hi = y2;
				}
				prev_y1 = y1;  prev_y2 = y2;
				canvas->draw_line(x, y_lo, x, y_hi, this);
//printf("ResourcePixmap::draw_audio_source %d %d %d %d\n", __LINE__, x, y1, y2);
				mwindow->wave_cache->unlock();
			}
			else {
//printf("ResourcePixmap::draw_audio_source %d\n", __LINE__);
				gui->resource_thread->add_wave(this,
					canvas->pane->number, indexable, x,
					edit->channel, prev_position, next_position);
				first_pixel = 1;  prev_y1 = -1;  prev_y2 = y_max;
			}
		}
	}

	canvas->test_timer();
}

void ResourcePixmap::draw_wave(TrackCanvas *canvas,
	int x, double high, double low)
{
	int top_pixel = 0;
	if( mwindow->edl->session->show_titles )
		top_pixel += mwindow->theme->get_image("title_bg_data")->get_h();
	int center_pixel = mwindow->edl->local_session->zoom_track / 2 + top_pixel;
	int bottom_pixel = top_pixel + mwindow->edl->local_session->zoom_track;
	int y1 = (int)(center_pixel -
		low * mwindow->edl->local_session->zoom_y / 2);
	int y2 = (int)(center_pixel -
		high * mwindow->edl->local_session->zoom_y / 2);
	CLAMP(y1, top_pixel, bottom_pixel);
	CLAMP(y2, top_pixel, bottom_pixel);
	canvas->set_color(mwindow->theme->audio_color);
	canvas->draw_line(x, y1, x, y2, this);
}


void ResourcePixmap::draw_video_resource(TrackCanvas *canvas,
	Edit *edit, int64_t edit_x, int64_t edit_w, int64_t pixmap_x, int64_t pixmap_w,
	int refresh_x, int refresh_w, int mode)
{
//PRINT_TRACE
//BC_Signals::dump_stack();

// pixels spanned by a picon
	int64_t picon_w = Units::round(edit->picon_w());
	int64_t picon_h = edit->picon_h();
//	if( picon_w <= 0 || picon_w > edit_w ) return;
// Don't draw video if picon is empty, or edit only hairline
	if( picon_w < 1 || edit_w < 2 ) return;
// or bigger than edit and fills at less than 1.5 percent timeline
	if( picon_w > edit_w && edit_w < canvas->get_w()/64 ) return;

// Frames spanned by a picon
	double frame_w = edit->frame_w();
// pixels spanned by a frame
	if( frame_w < picon_w ) frame_w = picon_w;
// Current pixel relative to pixmap
	int y = 0;
	if( edit->track->show_titles() )
		y += mwindow->theme->get_image("title_bg_data")->get_h();

// Frame in project touched by current pixel
	FloatAutos *speed_autos = !edit->track->has_speed() ? 0 :
		(FloatAutos *)edit->track->automation->autos[AUTOMATION_SPEED];
	Indexable *indexable = edit->get_source();
	double session_sample_rate = mwindow->edl->session->sample_rate;
	double project_zoom = mwindow->edl->local_session->zoom_sample / session_sample_rate;
	int skip_frames = Units::to_int64(((int64_t)refresh_x + pixmap_x - edit_x) / frame_w);
	int x = Units::to_int64(skip_frames * frame_w) + edit_x - pixmap_x;

// Draw only cached frames
	while( x < refresh_x + refresh_w ) {
		int64_t edit_position =
			edit->track->to_units((x + pixmap_x - edit_x) * project_zoom, 0);
		int64_t speed_position = edit->startsource;
		speed_position += !speed_autos ? edit_position :
			 speed_autos->automation_integral(
				edit->startproject, edit_position, PLAY_FORWARD);
		VFrame *picon_frame = indexable->id < 0 ? 0 :
			mwindow->frame_cache->get_frame_ptr(speed_position, edit->channel,
				mwindow->edl->session->frame_rate, BC_RGB888,
				picon_w, picon_h, indexable->id);
		int bg_color = gui->get_bg_color();
		if( picon_frame ) {
			VFrame *frame = picon_frame;
			int color = mwindow->get_title_color(edit);
			if( color ) {
				int alpha = (~color >> 24) & 0xff;
				frame = change_picon_alpha(picon_frame, alpha);
				gui->set_bg_color(color & 0xffffff);
			}
			draw_vframe(frame, x, y, picon_w, picon_h, 0, 0);
			if( frame != picon_frame ) {
				delete frame;
				gui->set_bg_color(bg_color);
			}
			mwindow->frame_cache->unlock();
		}
		else if( mode != IGNORE_THREAD ) {
// Set picon thread to draw in background
// printf("ResourcePixmap::draw_video_resource %d %d %lld\n",
// __LINE__, mwindow->frame_cache->total(), source_frame);
			gui->resource_thread->add_picon(this, canvas->pane->number, x, y,
				picon_w, picon_h, mwindow->edl->session->frame_rate,
				speed_position, edit->channel, indexable);
		}
		x += frame_w;
		canvas->test_timer();
	}
}

#include "strack.h"

void ResourcePixmap::draw_subttl_resource(TrackCanvas *canvas, Edit *edit, int x, int w)
{
	SEdit *sedit = (SEdit *)edit;
	char *text = sedit->get_text();
	if( !*text || w < 10 ) return;
	int center_pixel = canvas->resource_h() / 2;
	if( edit->track->show_titles() )
		center_pixel += mwindow->theme->get_image("title_bg_data")->get_h();
	int64_t scale_y = mwindow->edl->local_session->zoom_y;
	int x0 = edit_x;
	if( x0 < 0 ) x0 = -x0;
	int x1 = (int)(pixmap_x - x0 + x);
	int y_max = center_pixel + scale_y / 2 - 1;
	int font = MEDIUMFONT, color = WHITE;
	canvas->set_font(font);
	canvas->set_color(color);
	int ch = canvas->get_text_height(font);
	int hh = canvas->get_text_height(font,text) + ch/2;
	int y1 = y_max - hh - 10;
	if( y1 < 0 ) y1 = 0;
	canvas->draw_text(x1, y1, text, -1, this);
}

void ResourcePixmap::dump()
{
	printf("ResourcePixmap %p\n", this);
	printf(" edit %jx edit_x %jd pixmap_x %jd pixmap_w %jd visible %d\n",
		edit_id, edit_x, pixmap_x, pixmap_w, visible);
}

