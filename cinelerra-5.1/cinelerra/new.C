
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

#include "browsebutton.h"
#include "clip.h"
#include "cplayback.h"
#include "cwindow.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "filexml.h"
#include "interlacemodes.h"
#include "language.h"
#include "levelwindow.h"
#include "mainerror.h"
#include "mainundo.h"
#include "mainmenu.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "newpresets.h"
#include "mainsession.h"
#include "patchbay.h"
#include "preferences.h"
#include "theme.h"
#include "transportque.h"
#include "track.h"
#include "tracks.h"
#include "videowindow.h"
#include "vplayback.h"
#include "vwindow.h"


#include <string.h>


#define WIDTH 640
// full height
#define HEIGHT0 585
// add tracks dialog
#define HEIGHT1 240
// offset for folder panel
#define HEIGHT2 440

New::New(MWindow *mwindow)
{
	this->mwindow = mwindow;
	new_edl = 0;
	thread = 0;
}

New::~New()
{
	delete thread;
}

int New::handle_event()
{
	mwindow->gui->unlock_window();
	mwindow->edl->save_defaults(mwindow->defaults);
	create_new_edl();
	thread->start();
	mwindow->gui->lock_window("New::handle_event");

	return 1;
}

void New::create_new_edl()
{
	if( !new_edl ) {
		new_edl = new EDL;
		new_edl->create_objects();
		new_edl->load_defaults(mwindow->defaults);
	}
}


int New::create_new_project(int load_mode)
{
	mwindow->stop_playback(0);
	mwindow->gui->lock_window();
	mwindow->reset_caches();

	memcpy(new_edl->session->achannel_positions,
		&mwindow->preferences->channel_positions[new_edl->session->audio_channels - 1],
		sizeof(new_edl->session->achannel_positions));
	new_edl->session->boundaries();
	new_edl->create_default_tracks();
	if( load_mode == LOADMODE_NEW_TRACKS ) {
		Tracks *tracks =  mwindow->edl->tracks;
		int vindex = tracks->total_video_tracks();
		int aindex = tracks->total_audio_tracks();
		for( Track *track=new_edl->tracks->first; track; track=track->next ) {
			switch( track->data_type ) {
			case TRACK_AUDIO:
				sprintf(track->title, _("Audio %d"), ++aindex);
				break;
			case TRACK_VIDEO:
				sprintf(track->title, _("Video %d"), ++vindex);
				break;
			}
		}
	}
	mwindow->undo->update_undo_before();
	mwindow->set_filename(new_edl->path);
	ArrayList<EDL *>new_edls;
	new_edls.append(new_edl);
	mwindow->paste_edls(&new_edls, load_mode, 0,0,0,0,0,0);
	new_edl->remove_user();
	new_edl = 0;

// Load file sequence
	mwindow->update_project(load_mode);
	mwindow->session->changes_made = 0;
	mwindow->undo->update_undo_after(load_mode == LOADMODE_REPLACE ?
		_("New Project") : _("Append to Project"), LOAD_ALL);
	mwindow->gui->unlock_window();
	return 0;
}

NewProject::NewProject(MWindow *mwindow)
 : BC_MenuItem(_("New Project..."), "n", 'n'), New(mwindow)
{
}
NewProject::~NewProject()
{
}

void NewProject::create_objects()
{
	thread = new NewThread(mwindow, this,
		_(PROGRAM_NAME ": New Project"), LOADMODE_REPLACE);
}

AppendTracks::AppendTracks(MWindow *mwindow)
 : BC_MenuItem(_("Append to Project..."), "Shift-N", 'N'), New(mwindow)
{
	set_shift(1);
}
AppendTracks::~AppendTracks()
{
}

void AppendTracks::create_objects()
{
	thread = new NewThread(mwindow, this,
		_(PROGRAM_NAME ": Append to Project"), LOADMODE_NEW_TRACKS);
}


NewThread::NewThread(MWindow *mwindow, New *new_project,
		const char *title, int load_mode)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->new_project = new_project;
	this->title = title;
	this->load_mode = load_mode;
	nwindow = 0;
}

NewThread::~NewThread()
{
	close_window();
}

BC_Window* NewThread::new_gui()
{
	mwindow->edl->save_defaults(mwindow->defaults);
	new_project->create_new_edl();
	load_defaults();

	mwindow->gui->lock_window("NewThread::new_gui");
	int x = mwindow->gui->get_pop_cursor_x(0);
	int y = mwindow->gui->get_pop_cursor_y(0);

	nwindow = new NewWindow(mwindow, this, x, y);
	nwindow->create_objects();
	mwindow->gui->unlock_window();
	return nwindow;
}

void NewThread::handle_done_event(int result)
{
	if( !result && nwindow->folder && nwindow->name ) {
		const char *project_folder = nwindow->folder->get_text();
		const char *project_name = nwindow->name->get_text();
		if( project_folder[0] && project_name[0] ) {
			nwindow->recent_folder->add_item("PROJECT", project_folder);
			char project_path[BCTEXTLEN], *ep = project_path + sizeof(project_path);
			FileSystem fs;  fs.join_names(project_path, project_folder, project_name);
			char *bp = strrchr(project_path, '/');
			if( !bp ) bp = project_path;
			bp = strrchr(bp, '.');
			if( !bp ) bp = project_path + strlen(project_path);
			if( strcasecmp(bp,".xml") ) snprintf(bp, ep-bp, ".xml");
			fs.complete_path(project_path);
			EDL *new_edl = new_project->new_edl;
			bp = FileSystem::basepath(project_path);
			strcpy(new_edl->path, bp);  delete [] bp;
		}
	}
}

void NewThread::handle_close_event(int result)
{
	if( !new_project->new_edl ) return;

	FileSystem fs;
	char *path = new_project->new_edl->path;
	char *bp = path;
	while( !result ) {
		while( *bp == '/' ) ++bp;
		if( !(bp = strchr(bp, '/')) ) break;
		char dir_path[BCTEXTLEN];
		int len = bp - path;
		strncpy(dir_path, path, len);
		dir_path[len] = 0;
		if( fs.is_dir(dir_path) ) continue;
		if( fs.create_dir(dir_path) ) {
			eprintf(_("Cannot create and access project path:\n%s"),
				dir_path);
			result = 1;
		}
	}

	if( !result ) {
		new_project->new_edl->save_defaults(mwindow->defaults);
		mwindow->defaults->save();
		new_project->create_new_project(load_mode);
	}
	else { // Aborted
		new_project->new_edl->Garbage::remove_user();
		new_project->new_edl = 0;
	}
}

int NewThread::load_defaults()
{
	auto_aspect = mwindow->defaults->get("AUTOASPECT", 0);
	return 0;
}

int NewThread::save_defaults()
{
	mwindow->defaults->update("AUTOASPECT", auto_aspect);
	return 0;
}

int NewThread::update_aspect()
{
	if( auto_aspect ) {
		char string[BCTEXTLEN];
		mwindow->create_aspect_ratio(new_project->new_edl->session->aspect_w,
			new_project->new_edl->session->aspect_h,
			new_project->new_edl->session->output_w,
			new_project->new_edl->session->output_h);
		sprintf(string, "%.02f", new_project->new_edl->session->aspect_w);
		if( nwindow->aspect_w_text ) nwindow->aspect_w_text->update(string);
		sprintf(string, "%.02f", new_project->new_edl->session->aspect_h);
		if( nwindow->aspect_h_text )nwindow->aspect_h_text->update(string);
	}
	return 0;
}


NewWindow::NewWindow(MWindow *mwindow, NewThread *new_thread, int x, int y)
 : BC_Window(new_thread->title, x, y,
		WIDTH, new_thread->load_mode == LOADMODE_REPLACE ? HEIGHT0 : HEIGHT1,
		-1, -1, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->new_thread = new_thread;
	this->new_edl = new_thread->new_project->new_edl;
	format_presets = 0;
	atracks = 0;
	achannels = 0;
	sample_rate = 0;
	vtracks = 0;
	frame_rate = 0;
	output_w_text = 0;
	output_h_text = 0;
	aspect_w_text = 0;
	aspect_h_text = 0;
	interlace_pulldown = 0;
	color_model = 0;
	folder = 0;
	name = 0;
	recent_folder = 0;
}

NewWindow::~NewWindow()
{
	lock_window("NewWindow::~NewWindow");
	delete format_presets;
	unlock_window();
}

void NewWindow::create_objects()
{
	int x = 10, y = 10, x1, y1;
	BC_TextBox *textbox;
	BC_Title *title;

	lock_window("NewWindow::create_objects");
	mwindow->theme->draw_new_bg(this);

	add_subwindow( new BC_Title(x, y, new_thread->load_mode == LOADMODE_REPLACE ?
			_("Parameters for the new project:") :
			_("Parameters for additional tracks:") ) );
	y += 20;

	format_presets = new NewPresets(mwindow,
		this,
		x,
		y);
	format_presets->create_objects();
	x = format_presets->x;
	y = format_presets->y;

	y += 40;
	y1 = y;
	add_subwindow(new BC_Title(x, y, _("Audio"), LARGEFONT));
	y += 30;

	x1 = x;
	add_subwindow(new BC_Title(x1, y, _("Tracks:")));
	x1 += 100;
	add_subwindow(atracks = new NewATracks(this, "", x1, y));
	x1 += atracks->get_w();
	add_subwindow(new NewATracksTumbler(this, x1, y));
	y += atracks->get_h() + 5;

	if( new_thread->load_mode == LOADMODE_REPLACE ) {
		x1 = x;
		add_subwindow(new BC_Title(x1, y, _("Channels:")));
		x1 += 100;
		add_subwindow(achannels = new NewAChannels(this, "", x1, y));
		x1 += achannels->get_w();
		add_subwindow(new NewAChannelsTumbler(this, x1, y));
		y += achannels->get_h() + 5;

		x1 = x;
		add_subwindow(new BC_Title(x1, y, _("Samplerate:")));
		x1 += 100;
		add_subwindow(sample_rate = new NewSampleRate(this, "", x1, y));
		x1 += sample_rate->get_w();
		add_subwindow(new SampleRatePulldown(mwindow, sample_rate, x1, y));
	}

	x += 250;
	y = y1;
	add_subwindow(new BC_Title(x, y, _("Video"), LARGEFONT));
	y += 30;
	x1 = x;
	add_subwindow(new BC_Title(x1, y, _("Tracks:")));
	x1 += 115;
	add_subwindow(vtracks = new NewVTracks(this, "", x1, y));
	x1 += vtracks->get_w();
	add_subwindow(new NewVTracksTumbler(this, x1, y));
	y += vtracks->get_h() + 5;

	if( new_thread->load_mode == LOADMODE_REPLACE ) {
// 		x1 = x;
//	 	add_subwindow(new BC_Title(x1, y, _("Channels:")));
// 		x1 += 100;
// 		add_subwindow(vchannels = new NewVChannels(this, "", x1, y));
// 		x1 += vchannels->get_w();
// 		add_subwindow(new NewVChannelsTumbler(this, x1, y));
// 		y += vchannels->get_h() + 5;
		x1 = x;
		add_subwindow(new BC_Title(x1, y, _("Framerate:")));
		x1 += 115;
		add_subwindow(frame_rate = new NewFrameRate(this, "", x1, y));
		x1 += frame_rate->get_w();
		add_subwindow(new FrameRatePulldown(mwindow, frame_rate, x1, y));
		y += frame_rate->get_h() + 5;
	}
//	x1 = x;
//	add_subwindow(new BC_Title(x1, y, _("Canvas size:")));
// 	x1 += 100;
// 	add_subwindow(canvas_w_text = new NewTrackW(this, x1, y));
// 	x1 += canvas_w_text->get_w() + 2;
// 	add_subwindow(new BC_Title(x1, y, "x"));
// 	x1 += 10;
// 	add_subwindow(canvas_h_text = new NewTrackH(this, x1, y));
// 	x1 += canvas_h_text->get_w();
// 	add_subwindow(new FrameSizePulldown(mwindow,
// 		canvas_w_text,
// 		canvas_h_text,
// 		x1,
// 		y));
//	x1 += 100;
//	add_subwindow(new NewCloneToggle(mwindow, this, x1, y));
//	y += canvas_h_text->get_h() + 5;

	x1 = x;
	add_subwindow(new BC_Title(x1, y, new_thread->load_mode == LOADMODE_REPLACE ?
			_("Canvas size:") : _("Track size:")));
	x1 += 115;
	add_subwindow(output_w_text = new NewOutputW(this, x1, y));
	x1 += output_w_text->get_w() + 2;
	add_subwindow(new BC_Title(x1, y, "x"));
	x1 += 10;
	add_subwindow(output_h_text = new NewOutputH(this, x1, y));
	x1 += output_h_text->get_w();
	FrameSizePulldown *pulldown;
	add_subwindow(pulldown = new FrameSizePulldown(mwindow->theme,
		output_w_text,
		output_h_text,
		x1,
		y));
	x1 += pulldown->get_w() + 10;
	add_subwindow(new NewSwapExtents(mwindow, this, x1, y));
	y += output_h_text->get_h() + 5;

	if( new_thread->load_mode == LOADMODE_REPLACE ) {
		x1 = x;
		add_subwindow(new BC_Title(x1, y, _("Aspect ratio:")));
		x1 += 115;
		add_subwindow(aspect_w_text = new NewAspectW(this, "", x1, y));
		x1 += aspect_w_text->get_w() + 2;
		add_subwindow(new BC_Title(x1, y, ":"));
		x1 += 10;
		add_subwindow(aspect_h_text = new NewAspectH(this, "", x1, y));
		x1 += aspect_h_text->get_w();
		add_subwindow(new AspectPulldown(mwindow,
			aspect_w_text, aspect_h_text, x1, y));

		x1 = aspect_w_text->get_x();
		y += aspect_w_text->get_h() + 5;
		add_subwindow(new NewAspectAuto(this, x1, y));
		y += 40;
		add_subwindow(title = new BC_Title(x, y, _("Color model:")));
		x1 = x + title->get_w();
		y1 = y;  y += title->get_h() + 10;
		add_subwindow(title = new BC_Title(x, y, _("Interlace mode:")));
		int x2 = x + title->get_w();
		int y2 = y;  y += title->get_h() + 10;
		if( x1 < x2 ) x1 = x2;
		x1 += 20;
		add_subwindow(textbox = new BC_TextBox(x1, y1, 150, 1, ""));
		add_subwindow(color_model = new ColormodelPulldown(mwindow,
			textbox, &new_edl->session->color_model, x1+textbox->get_w(), y1));
		add_subwindow(textbox = new BC_TextBox(x1, y2, 150, 1, ""));
		add_subwindow(interlace_pulldown = new InterlacemodePulldown(mwindow,
			textbox, &new_edl->session->interlace_mode,
			(ArrayList<BC_ListBoxItem*>*)&mwindow->interlace_project_modes,
			x1+textbox->get_w(), y2));

		x = 20;  y = HEIGHT2;
		add_subwindow(title = new BC_Title(x, y, _("Create project folder in:")));
		x1 = x;  y += title->get_h() + 5;
		add_subwindow(folder = new BC_TextBox(x1, y, get_w()-x1-64, 1, ""));
		x1 += folder->get_w() + 10;
		add_subwindow(recent_folder = new BC_RecentList("FOLDER", mwindow->defaults, folder));
		recent_folder->load_items("PROJECT");
		x1 = recent_folder->get_x() + recent_folder->get_w();
		add_subwindow(new BrowseButton(mwindow->theme, this, folder, x1, y, "",
                        _("Project Directory"), _("Project Directory Path:"), 1));
		y += folder->get_h() + 10;  x1 = x;
		add_subwindow(title = new BC_Title(x1, y, _("Project Name:")));
		x1 += title->get_w() + 10;
		add_subwindow(name = new BC_TextBox(x1, y, get_w()-x1-64, 1, ""));
	}

	add_subwindow(new BC_OKButton(this,
		mwindow->theme->get_image_set("new_ok_images")));
	add_subwindow(new BC_CancelButton(this,
		mwindow->theme->get_image_set("new_cancel_images")));
	flash();
	update();
	show_window();
	unlock_window();
}

int NewWindow::update()
{
	if( atracks ) atracks->update((int64_t)new_edl->session->audio_tracks);
	if( achannels ) achannels->update((int64_t)new_edl->session->audio_channels);
	if( sample_rate ) sample_rate->update((int64_t)new_edl->session->sample_rate);
	if( vtracks ) vtracks->update((int64_t)new_edl->session->video_tracks);
	if( frame_rate ) frame_rate->update((float)new_edl->session->frame_rate);
	if( output_w_text ) output_w_text->update((int64_t)new_edl->session->output_w);
	if( output_h_text ) output_h_text->update((int64_t)new_edl->session->output_h);
	if( aspect_w_text ) aspect_w_text->update((float)new_edl->session->aspect_w);
	if( aspect_h_text ) aspect_h_text->update((float)new_edl->session->aspect_h);
	if( interlace_pulldown ) interlace_pulldown->update(new_edl->session->interlace_mode);
	if( color_model ) color_model->update_value(new_edl->session->color_model);
	return 0;
}


NewPresets::NewPresets(MWindow *mwindow, NewWindow *gui, int x, int y)
 : FormatPresets(mwindow, gui, 0, x, y)
{
}

NewPresets::~NewPresets()
{
}

int NewPresets::handle_event()
{
	new_gui->update();
	return 1;
}

EDL* NewPresets::get_edl()
{
	return new_gui->new_edl;
}


NewATracks::NewATracks(NewWindow *nwindow, const char *text, int x, int y)
 : BC_TextBox(x, y, 90, 1, text)
{
	this->nwindow = nwindow;
}

int NewATracks::handle_event()
{
	nwindow->new_edl->session->audio_tracks = atol(get_text());
	return 1;
}

NewATracksTumbler::NewATracksTumbler(NewWindow *nwindow, int x, int y)
 : BC_Tumbler(x, y)
{
	this->nwindow = nwindow;
}
int NewATracksTumbler::handle_up_event()
{
	nwindow->new_edl->session->audio_tracks++;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}
int NewATracksTumbler::handle_down_event()
{
	nwindow->new_edl->session->audio_tracks--;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}


NewAChannels::NewAChannels(NewWindow *nwindow, const char *text, int x, int y)
 : BC_TextBox(x, y, 90, 1, text)
{
	this->nwindow = nwindow;
}

int NewAChannels::handle_event()
{
	nwindow->new_edl->session->audio_channels = atol(get_text());
	return 1;
}


NewAChannelsTumbler::NewAChannelsTumbler(NewWindow *nwindow, int x, int y)
 : BC_Tumbler(x, y)
{
	this->nwindow = nwindow;
}
int NewAChannelsTumbler::handle_up_event()
{
	nwindow->new_edl->session->audio_channels++;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}
int NewAChannelsTumbler::handle_down_event()
{
	nwindow->new_edl->session->audio_channels--;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}


NewSampleRate::NewSampleRate(NewWindow *nwindow, const char *text, int x, int y)
 : BC_TextBox(x, y, 90, 1, text)
{
	this->nwindow = nwindow;
}

int NewSampleRate::handle_event()
{
	nwindow->new_edl->session->sample_rate = atol(get_text());
	return 1;
}


SampleRatePulldown::SampleRatePulldown(MWindow *mwindow, BC_TextBox *output, int x, int y)
 : BC_ListBox(x, y, 100, 200, LISTBOX_TEXT,
	&mwindow->theme->sample_rates, 0, 0, 1, 0, 1)
{
	this->mwindow = mwindow;
	this->output = output;
}
int SampleRatePulldown::handle_event()
{
	char *text = get_selection(0, 0)->get_text();
	output->update(text);
	output->handle_event();
	return 1;
}


NewVTracks::NewVTracks(NewWindow *nwindow, const char *text, int x, int y)
 : BC_TextBox(x, y, 90, 1, text)
{
	this->nwindow = nwindow;
}

int NewVTracks::handle_event()
{
	nwindow->new_edl->session->video_tracks = atol(get_text());
	return 1;
}


NewVTracksTumbler::NewVTracksTumbler(NewWindow *nwindow, int x, int y)
 : BC_Tumbler(x, y)
{
	this->nwindow = nwindow;
}
int NewVTracksTumbler::handle_up_event()
{
	nwindow->new_edl->session->video_tracks++;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}
int NewVTracksTumbler::handle_down_event()
{
	nwindow->new_edl->session->video_tracks--;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}


NewVChannels::NewVChannels(NewWindow *nwindow, const char *text, int x, int y)
 : BC_TextBox(x, y, 90, 1, text)
{
	this->nwindow = nwindow;
}

int NewVChannels::handle_event()
{
	nwindow->new_edl->session->video_channels = atol(get_text());
	return 1;
}


NewVChannelsTumbler::NewVChannelsTumbler(NewWindow *nwindow, int x, int y)
 : BC_Tumbler(x, y)
{
	this->nwindow = nwindow;
}
int NewVChannelsTumbler::handle_up_event()
{
	nwindow->new_edl->session->video_channels++;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}
int NewVChannelsTumbler::handle_down_event()
{
	nwindow->new_edl->session->video_channels--;
	nwindow->new_edl->boundaries();
	nwindow->update();
	return 1;
}


NewFrameRate::NewFrameRate(NewWindow *nwindow, const char *text, int x, int y)
 : BC_TextBox(x, y, 90, 1, text)
{
	this->nwindow = nwindow;
}

int NewFrameRate::handle_event()
{
	nwindow->new_edl->session->frame_rate = Units::atoframerate(get_text());
	return 1;
}


FrameRatePulldown::FrameRatePulldown(MWindow *mwindow,
	BC_TextBox *output, int x, int y)
 : BC_ListBox(x, y, 150, 250, LISTBOX_TEXT,
	&mwindow->theme->frame_rates, 0, 0, 1, 0, 1)
{
	this->mwindow = mwindow;
	this->output = output;
}
int FrameRatePulldown::handle_event()
{
	char *text = get_selection(0, 0)->get_text();
	output->update(text);
	output->handle_event();
	return 1;
}

FrameSizePulldown::FrameSizePulldown(Theme *theme,
		BC_TextBox *output_w, BC_TextBox *output_h, int x, int y)
 : BC_ListBox(x, y, 180, 250, LISTBOX_TEXT,
	&theme->frame_sizes, 0, 0, 1, 0, 1)
{
	this->theme = theme;
	this->output_w = output_w;
	this->output_h = output_h;
}

int FrameSizePulldown::handle_event()
{
	char *text = get_selection(0, 0)->get_text();
	char string[BCTEXTLEN];
	int64_t w, h;
	char *ptr;

	strcpy(string, text);
	ptr = strrchr(string, 'x');
	if( ptr ) {
		ptr++;
		h = atol(ptr);

		*--ptr = 0;
		w = atol(string);
		output_w->update(w);
		output_h->update(h);
		output_w->handle_event();
		output_h->handle_event();
	}
	return 1;
}


NewOutputW::NewOutputW(NewWindow *nwindow, int x, int y)
 : BC_TextBox(x, y, 70, 1, nwindow->new_edl->session->output_w)
{
	this->nwindow = nwindow;
}
int NewOutputW::handle_event()
{
	nwindow->new_edl->session->output_w = MAX(1,atol(get_text()));
	nwindow->new_thread->update_aspect();
	return 1;
}


NewOutputH::NewOutputH(NewWindow *nwindow, int x, int y)
 : BC_TextBox(x, y, 70, 1, nwindow->new_edl->session->output_h)
{
	this->nwindow = nwindow;
}
int NewOutputH::handle_event()
{
	nwindow->new_edl->session->output_h = MAX(1, atol(get_text()));
	nwindow->new_thread->update_aspect();
	return 1;
}


NewAspectW::NewAspectW(NewWindow *nwindow, const char *text, int x, int y)
 : BC_TextBox(x, y, 70, 1, text)
{
	this->nwindow = nwindow;
}

int NewAspectW::handle_event()
{
	nwindow->new_edl->session->aspect_w = atof(get_text());
	return 1;
}


NewAspectH::NewAspectH(NewWindow *nwindow, const char *text, int x, int y)
 : BC_TextBox(x, y, 70, 1, text)
{
	this->nwindow = nwindow;
}

int NewAspectH::handle_event()
{
	nwindow->new_edl->session->aspect_h = atof(get_text());
	return 1;
}


AspectPulldown::AspectPulldown(MWindow *mwindow,
		BC_TextBox *output_w, BC_TextBox *output_h, int x, int y)
 : BC_ListBox(x, y, 100, 200, LISTBOX_TEXT,
	&mwindow->theme->aspect_ratios, 0, 0, 1, 0, 1)
{
	this->mwindow = mwindow;
	this->output_w = output_w;
	this->output_h = output_h;
}

int AspectPulldown::handle_event()
{
	char *text = get_selection(0, 0)->get_text();
	char string[BCTEXTLEN];
	float w, h;
	char *ptr;

	strcpy(string, text);
	ptr = strrchr(string, ':');
	if( ptr ) {
		ptr++;
		h = atof(ptr);

		*--ptr = 0;
		w = atof(string);
		output_w->update(w);
		output_h->update(h);
		output_w->handle_event();
		output_h->handle_event();
	}
	return 1;
}


ColormodelItem::ColormodelItem(const char *text, int value)
 : BC_ListBoxItem(text)
{
	this->value = value;
}

ColormodelPulldown::ColormodelPulldown(MWindow *mwindow,
		BC_TextBox *output_text, int *output_value, int x, int y)
 : BC_ListBox(x, y, 200, 150, LISTBOX_TEXT,
	(ArrayList<BC_ListBoxItem*>*)&mwindow->colormodels, 0, 0, 1, 0, 1)
{
	this->mwindow = mwindow;
	this->output_text = output_text;
	this->output_value = output_value;
	output_text->update(colormodel_to_text());
}

int ColormodelPulldown::handle_event()
{
	output_text->update(get_selection(0, 0)->get_text());
	*output_value = ((ColormodelItem*)get_selection(0, 0))->value;
	return 1;
}

const char* ColormodelPulldown::colormodel_to_text()
{
	for( int i=0; i<mwindow->colormodels.total; ++i )
		if( mwindow->colormodels.values[i]->value == *output_value )
			return mwindow->colormodels.values[i]->get_text();
	return _("Unknown");
}

void ColormodelPulldown::update_value(int value)
{
	*output_value = value;
	output_text->update(colormodel_to_text());
}


InterlacemodeItem::InterlacemodeItem(const char *text, int value)
 : BC_ListBoxItem(text)
{
	this->value = value;
}

InterlacemodePulldown::InterlacemodePulldown(MWindow *mwindow,
		BC_TextBox *output_text,
		int *output_value,
		ArrayList<BC_ListBoxItem*> *data,
		int x,
		int y)
 : BC_ListBox(x, y, 200, 150, LISTBOX_TEXT, data, 0, 0, 1, 0, 1)
{
	this->mwindow = mwindow;
	this->output_text = output_text;
	this->output_value = output_value;
	output_text->update(interlacemode_to_text());
}

int InterlacemodePulldown::handle_event()
{
	output_text->update(get_selection(0, 0)->get_text());
	*output_value = ((InterlacemodeItem*)get_selection(0, 0))->value;
	return 1;
}

const char* InterlacemodePulldown::interlacemode_to_text()
{
	ilacemode_to_text(this->string,*output_value);
	return (this->string);
}

int InterlacemodePulldown::update(int interlace_mode)
{
	*output_value = interlace_mode;
	output_text->update(interlacemode_to_text());
	return 1;
}


NewAspectAuto::NewAspectAuto(NewWindow *nwindow, int x, int y)
 : BC_CheckBox(x, y, nwindow->new_thread->auto_aspect, _("Auto aspect ratio"))
{
	this->nwindow = nwindow;
}
NewAspectAuto::~NewAspectAuto()
{
}
int NewAspectAuto::handle_event()
{
	nwindow->new_thread->auto_aspect = get_value();
	nwindow->new_thread->update_aspect();
	return 1;
}


NewSwapExtents::NewSwapExtents(MWindow *mwindow, NewWindow *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("swap_extents"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Swap dimensions"));
}

int NewSwapExtents::handle_event()
{
	int w = gui->new_edl->session->output_w;
	int h = gui->new_edl->session->output_h;
	gui->new_edl->session->output_w = h;
	gui->new_edl->session->output_h = w;
	gui->output_w_text->update((int64_t)h);
	gui->output_h_text->update((int64_t)w);
	gui->new_thread->update_aspect();
	return 1;
}

