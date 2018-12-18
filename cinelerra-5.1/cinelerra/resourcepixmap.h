
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

#ifndef RESOURCEPIXMAP_H
#define RESOURCEPIXMAP_H

#include "bctimer.inc"
#include "edit.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "trackcanvas.inc"


// Can't use garbage collection for GUI elements because they need to
// lock the window for deletion.
class ResourcePixmap : public BC_Pixmap
{
public:
	ResourcePixmap(MWindow *mwindow, MWindowGUI *gui, Edit *edit,
		int pane_number, int w, int h);
	~ResourcePixmap();

	void resize(int w, int h);
	void update_settings(Edit *edit,
		int64_t edit_x, int64_t edit_w,
		int64_t pixmap_x, int64_t pixmap_w, int64_t pixmap_h);
	void draw_data(TrackCanvas *canvas,
		Edit *edit, int64_t edit_x, int64_t edit_w,
		int64_t pixmap_x, int64_t pixmap_w, int64_t pixmap_h,
		int mode, int indexes_only);
	void draw_audio_resource(TrackCanvas *canvas,
		Edit *edit, int x, int w);
	void draw_video_resource(TrackCanvas *canvas,
		Edit *edit, int64_t edit_x, int64_t edit_w,
		int64_t pixmap_x, int64_t pixmap_w,
		int refresh_x, int refresh_w,
		int mode);
	void draw_audio_source(TrackCanvas *canvas,
		Edit *edit, int x, int w);
	void draw_subttl_resource(TrackCanvas *canvas,
		Edit *edit, int x, int w);
// Called by ResourceThread to update pixmap
	void draw_wave(TrackCanvas *canvas,
		int x, double high, double low);
	VFrame *change_title_color(VFrame *title_bg, int color);
	VFrame *change_picon_alpha(VFrame *picon_frame, int alpha);
	void draw_title(TrackCanvas *canvas,
		Edit *edit, int64_t edit_x, int64_t edit_w,
		int64_t pixmap_x, int64_t pixmap_w);
	void reset();
// Change to hourglass if timer expired
	void test_timer();

	void dump();

	MWindow *mwindow;
	MWindowGUI *gui;
// Visible in entire track canvas
	int visible;
// Section drawn
	int64_t edit_id;
	int pane_number;
	int64_t edit_x, pixmap_x, pixmap_w, pixmap_h;
	int64_t zoom_sample, zoom_track, zoom_y;
	int64_t startsource;
	double source_framerate, project_framerate;
	int64_t source_samplerate, project_samplerate;
	int data_type;
// Timer to cause an hourglass to appear
	Timer *timer;
};

#endif
