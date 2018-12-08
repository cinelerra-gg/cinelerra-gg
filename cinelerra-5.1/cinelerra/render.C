
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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
#include "auto.h"
#include "awindow.h"
#include "awindowgui.h"
#include "batchrender.h"
#include "bcprogressbox.h"
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "compresspopup.h"
#include "condition.h"
#include "confirmsave.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "bchash.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "formatcheck.h"
#include "formatpopup.h"
#include "formattools.h"
#include "indexable.h"
#include "labels.h"
#include "language.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainprogress.h"
#include "mainsession.h"
#include "mainundo.h"
#include "module.h"
#include "mutex.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "packagedispatcher.h"
#include "packagerenderer.h"
#include "patchbay.h"
#include "playabletracks.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "renderfarm.h"
#include "render.h"
#include "renderprofiles.h"
#include "statusbar.h"
#include "theme.h"
#include "timebar.h"
#include "tracks.h"
#include "transportque.h"
#include "vedit.h"
#include "vframe.h"
#include "videoconfig.h"
#include "vrender.h"

#include <ctype.h>
#include <string.h>



RenderItem::RenderItem(MWindow *mwindow)
 : BC_MenuItem(_("Render..."), _("Shift-R"), 'R')
{
	this->mwindow = mwindow;
	set_shift(1);
}

int RenderItem::handle_event()
{
	mwindow->gui->unlock_window();
	mwindow->render->start_interactive();
	mwindow->gui->lock_window("RenderItem::handle_event");
	return 1;
}










RenderProgress::RenderProgress(MWindow *mwindow, Render *render)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->render = render;
	last_value = 0;
	Thread::set_synchronous(1);
}

RenderProgress::~RenderProgress()
{
	Thread::cancel();
	Thread::join();
}


void RenderProgress::run()
{
	Thread::disable_cancel();
	while(1)
	{
		if(render->total_rendered != last_value)
		{
			render->progress->update(render->total_rendered);
			last_value = render->total_rendered;

			if(mwindow) mwindow->preferences_thread->update_rates();
		}

		Thread::enable_cancel();
		sleep(1);
		Thread::disable_cancel();
	}
}



MainPackageRenderer::MainPackageRenderer(Render *render)
 : PackageRenderer()
{
	this->render = render;
}



MainPackageRenderer::~MainPackageRenderer()
{
}


int MainPackageRenderer::get_master()
{
	return 1;
}

int MainPackageRenderer::get_result()
{
	return render->result;
}

void MainPackageRenderer::set_result(int value)
{
	if(value)
		render->result = value;



}

void MainPackageRenderer::set_progress(int64_t value)
{
	render->counter_lock->lock("MainPackageRenderer::set_progress");
// Increase total rendered for all nodes
	render->total_rendered += value;

// Update frames per second for master node
	render->preferences->set_rate(frames_per_second, -1);

//printf("MainPackageRenderer::set_progress %d %ld %f\n", __LINE__, (long)value, frames_per_second);

// If non interactive, print progress out
	if(!render->progress)
		render->show_progress();

	render->counter_lock->unlock();

	if( mwindow )
		mwindow->preferences->copy_rates_from(preferences);
}

int MainPackageRenderer::progress_cancelled()
{
	return (render->progress && render->progress->is_cancelled()) ||
		render->batch_cancelled;
}


Render::Render(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	in_progress = 0;
	progress = 0;
	elapsed_time = 0.0;
	package_lock = new Mutex("Render::package_lock");
	counter_lock = new Mutex("Render::counter_lock");
	completion = new Condition(0, "Render::completion");
	progress_timer = new Timer;
	range_type = RANGE_BACKCOMPAT;
	preferences = new Preferences();
	thread = new RenderThread(mwindow, this);
	render_window = 0;
	asset = 0;
	result = 0;
	beep = 0;
}

Render::~Render()
{
	close_window();
	delete package_lock;
	delete counter_lock;
	delete completion;
	delete preferences;
	delete progress_timer;
	if( asset ) asset->Garbage::remove_user();
	delete thread;
}

void Render::start_interactive()
{
	if( !thread->running() ) {
		mode = Render::INTERACTIVE;
		BC_DialogThread::start();
	}
	else if( in_progress ) {
		int cx, cy;
		mwindow->gui->get_abs_cursor(cx, cy, 1);
		ErrorBox error_box(_(PROGRAM_NAME ": Error"), cx, cy);
		error_box.create_objects(_("Already rendering"));
		error_box.raise_window();
		error_box.run_window();
	}
	else if( render_window ) {
		render_window->raise_window();
	}
}


void Render::start_batches(ArrayList<BatchRenderJob*> *jobs)
{
	if(!thread->running())
	{
		mode = Render::BATCH;
		batch_cancelled = 0;
		this->jobs = jobs;
		completion->reset();
		start_render();
	}
	else if( in_progress ) {
		int cx, cy;
		mwindow->gui->get_abs_cursor(cx, cy, 1);
		ErrorBox error_box(_(PROGRAM_NAME ": Error"), cx, cy);
		error_box.create_objects("Already rendering");
		error_box.raise_window();
		error_box.run_window();
	}
	// raise the window if rendering hasn't started yet
	else if( render_window ) {
		render_window->raise_window();
	}
}

void Render::start_batches(ArrayList<BatchRenderJob*> *jobs,
	BC_Hash *boot_defaults, Preferences *batch_prefs)
{
	mode = Render::BATCH;
	batch_cancelled = 0;
	preferences->copy_from(batch_prefs);
	this->jobs = jobs;

	completion->reset();
	thread->run();
}


BC_Window* Render::new_gui()
{
	this->jobs = 0;
	batch_cancelled = 0;
	result = 0;

	if(mode == Render::INTERACTIVE) {
// Fix the asset for rendering
		if(!asset) asset = new Asset;
		load_defaults(asset);
		check_asset(mwindow->edl, *asset);
		int px = mwindow->gui->get_pop_cursor_x(1);
		int py = mwindow->gui->get_pop_cursor_y(1);
// Get format from user
		render_window = new RenderWindow(mwindow, this, asset, px, py);
		render_window->create_objects();
	}

	return render_window;
}

void Render::handle_done_event(int result)
{
	if(!result) {
		mwindow->edl->session->render_beep = beep;
		// add to recentlist only on OK
		render_window->render_format->path_recent->
			add_item(File::formattostr(asset->format), asset->path);
		setenv("CIN_RENDER", asset->path, 1);
	}
	render_window = 0;
}

void Render::handle_close_event(int result)
{
	const int debug = 0;
	double render_range = get_render_range();
	const char *err_msg = 0;

	if( !result && !render_range ) {
		err_msg = _("zero render range");
		result = 1;
	}
	if( !result && asset->video_data ) {
		double frame_rate = mwindow->edl->session->frame_rate;
		if( frame_rate > 0 && render_range+1e-3 < 1./frame_rate ) {
			err_msg = _("Video data and range less than 1 frame");
			result = 1;
		}
	}
	if( !result && asset->audio_data ) {
		double sample_rate = mwindow->edl->session->sample_rate;
		if( sample_rate > 0 && render_range+1e-6 < 1./sample_rate ) {
			err_msg = _("Audio data and range less than 1 sample");
			result = 1;
		}
	}
	if( !result && File::is_image_render(asset->format) ) {
		if( asset->video_data ) {
			double frames = render_range * mwindow->edl->session->frame_rate;
			if( !EQUIV(frames, 1.) ) {
				err_msg = _("Image format and not 1 frame");
				result = 1;
			}
		}
		else {
			err_msg = _("Image format and no video data");
			result = 1;
		}
	}
	if( err_msg ) {
		int cx, cy;
		mwindow->gui->get_abs_cursor(cx, cy, 1);
		ErrorBox error_box(_(PROGRAM_NAME ": Error"),cx, cy);
		error_box.create_objects(err_msg);
		error_box.raise_window();
		error_box.run_window();
	}

	if(!result) {
// Check the asset format for errors.
		FormatCheck format_check(asset);
		if( format_check.check_format() )
			result = 1;
	}

//PRINT_TRACE

	save_defaults(asset);
//PRINT_TRACE
	mwindow->save_defaults();
//PRINT_TRACE

	if( !result ) {
		if(debug) printf("Render::handle_close_event %d\n", __LINE__);
		if(!result) start_render();
		if(debug) printf("Render::handle_close_event %d\n", __LINE__);
	}
//PRINT_TRACE
}



void Render::stop_operation()
{
	if(thread->Thread::running())
	{
		batch_cancelled = 1;
// Wait for completion
		completion->lock("Render::stop_operation");
		completion->reset();
	}
}

int Render::check_asset(EDL *edl, Asset &asset)
{
	if(asset.video_data &&
		edl->tracks->playable_video_tracks() &&
		File::renders_video(&asset))
	{
		asset.video_data = 1;
		asset.layers = 1;
		asset.width = edl->session->output_w;
		asset.height = edl->session->output_h;
		asset.interlace_mode = edl->session->interlace_mode;
	}
	else
	{
		asset.video_data = 0;
		asset.layers = 0;
	}

	if(asset.audio_data &&
		edl->tracks->playable_audio_tracks() &&
		File::renders_audio(&asset))
	{
		asset.audio_data = 1;
		asset.channels = edl->session->audio_channels;
	}
	else
	{
		asset.audio_data = 0;
		asset.channels = 0;
	}

	if(!asset.audio_data &&
		!asset.video_data)
	{
		return 1;
	}
	return 0;
}

int Render::get_strategy(int use_renderfarm, int use_labels)
{
	return use_renderfarm ?
		(use_labels ? FILE_PER_LABEL_FARM : SINGLE_PASS_FARM) :
		(use_labels ? FILE_PER_LABEL      : SINGLE_PASS     ) ;
}
int Render::get_strategy()
{
	return get_strategy(preferences->use_renderfarm, use_labels);
}

void Render::start_progress()
{
	char filename[BCTEXTLEN];
	char string[BCTEXTLEN];
	FileSystem fs;

	progress_max = packages->get_progress_max();

	progress_timer->update();
	last_eta = 0;
	if(mwindow)
	{
// Generate the progress box
		fs.extract_name(filename, default_asset->path);
		sprintf(string, _("Rendering %s..."), filename);

// Don't bother with the filename since renderfarm defeats the meaning
		progress = mwindow->mainprogress->start_progress(_("Rendering..."),
			progress_max);
		render_progress = new RenderProgress(mwindow, this);
		render_progress->start();
	}
}

void Render::stop_progress()
{
	if(progress)
	{
		char string[BCTEXTLEN], string2[BCTEXTLEN];
		delete render_progress;
		progress->get_time(string);
		elapsed_time = progress->get_time();
		progress->stop_progress();
		delete progress;

		sprintf(string2, _("Rendering took %s"), string);
		mwindow->gui->lock_window("");
		mwindow->gui->show_message(string2);
		mwindow->gui->update_default_message();
		mwindow->gui->stop_hourglass();
		mwindow->gui->unlock_window();
	}
	progress = 0;
}

void Render::show_progress()
{
	int64_t current_eta = progress_timer->get_scaled_difference(1000);
	if (current_eta - last_eta < 1000 ) return;
	double eta = !total_rendered ? 0 :
		current_eta / 1000. * (progress_max / (double)total_rendered - 1.);
	char string[BCTEXTLEN];  Units::totext(string, eta, TIME_HMS2);
	printf("\r%d%% %s: %s      ",
		(int)(100 * (float)total_rendered / progress_max), _("ETA"), string);
	fflush(stdout);
	last_eta = current_eta;
}



void Render::start_render()
{
	in_progress = 0;
	elapsed_time = 0.0;
	result = 0;
	completion->reset();
	thread->start();
}


void Render::create_filename(char *path,
	char *default_path,
	int current_number,
	int total_digits,
	int number_start)
{
	int i, j;
	int len = strlen(default_path);
	char printf_string[BCTEXTLEN];

	for(i = 0, j = 0; i < number_start; i++, j++)
	{
		printf_string[j] = default_path[i];
	}

// Found the number
	sprintf(&printf_string[j], "%%0%dd", total_digits);
	j = strlen(printf_string);
	i += total_digits;

// Copy remainder of string
	for( ; i < len; i++, j++)
	{
		printf_string[j] = default_path[i];
	}
	printf_string[j] = 0;
// Print the printf argument to the path
	sprintf(path, printf_string, current_number);
}

void Render::get_starting_number(char *path,
	int &current_number,
	int &number_start,
	int &total_digits,
	int min_digits)
{
	int len = strlen(path);
	char number_text[BCTEXTLEN];
	char *ptr = 0;
	char *ptr2 = 0;

	total_digits = 0;
	number_start = 0;

// Search for last /
	ptr2 = strrchr(path, '/');

// Search for first 0 after last /.
	if(ptr2)
		ptr = strchr(ptr2, '0');

	if(ptr && isdigit(*ptr))
	{
		number_start = ptr - path;

// Store the first number
		char *ptr2 = number_text;
		while(isdigit(*ptr))
			*ptr2++ = *ptr++;
		*ptr2++ = 0;
		current_number = atol(number_text);
		total_digits = strlen(number_text);
	}


// No number found or number not long enough
	if(total_digits < min_digits)
	{
		current_number = 1;
		number_start = len;
		total_digits = min_digits;
	}
}


int Render::load_defaults(Asset *asset)
{
	use_labels = mwindow->defaults->get("RENDER_FILE_PER_LABEL", 0);
	load_mode = mwindow->defaults->get("RENDER_LOADMODE", LOADMODE_NEW_TRACKS);
	range_type = mwindow->defaults->get("RENDER_RANGE_TYPE", RANGE_PROJECT);

// some defaults which work
	asset->video_data = 1;
	asset->audio_data = 1;
	asset->format = FILE_FFMPEG;
	strcpy(asset->acodec, "mp4.qt");
	strcpy(asset->vcodec, "mp4.qt");

	asset->load_defaults(mwindow->defaults,
		"RENDER_", 1, 1, 1, 1, 1);
	return 0;
}

int Render::load_profile(int profile_slot, Asset *asset)
{
	char string_name[100];
	sprintf(string_name, "RENDER_%i_FILE_PER_LABEL", profile_slot);
	use_labels = mwindow->defaults->get(string_name, 0);
// Load mode is not part of the profile
//	printf(string_name, "RENDER_%i_LOADMODE", profile_slot);
//	load_mode = mwindow->defaults->get(string_name, LOADMODE_NEW_TRACKS);
	sprintf(string_name, "RENDER_%i_RANGE_TYPE", profile_slot);
	range_type = mwindow->defaults->get(string_name, RANGE_PROJECT);

	sprintf(string_name, "RENDER_%i_", profile_slot);
	asset->load_defaults(mwindow->defaults,
		string_name, 1, 1, 1, 1, 1);
	return 0;
}


int Render::save_defaults(Asset *asset)
{
	mwindow->defaults->update("RENDER_FILE_PER_LABEL", use_labels);
	mwindow->defaults->update("RENDER_LOADMODE", load_mode);
	mwindow->defaults->update("RENDER_RANGE_TYPE", range_type);

	asset->save_defaults(mwindow->defaults,
		"RENDER_", 1, 1, 1, 1, 1);
	return 0;
}


static void run_script(const char *script, const char *arg)
{
	char *const argv[] = { (char*)script, (char*)arg, 0 };
	execvp(script ,&argv[0]);
	perror("execvp script failed");
	exit(1);
}

RenderThread::RenderThread(MWindow *mwindow, Render *render)
 : Thread(0, 0, 0)
{
	this->mwindow = mwindow;
	this->render = render;
}

RenderThread::~RenderThread()
{
}


void RenderThread::render_single(int test_overwrite, Asset *asset, EDL *edl,
	int strategy, int range_type)
{
// Total length in seconds
	double total_length;
	RenderFarmServer *farm_server = 0;
	FileSystem fs;
	const int debug = 0;

	render->in_progress = 1;


	render->default_asset = asset;
	render->progress = 0;
	render->result = 0;

// Create rendering command
	TransportCommand *command = new TransportCommand;
	command->command = NORMAL_FWD;
	command->get_edl()->copy_all(edl);
	command->change_type = CHANGE_ALL;

	switch( range_type ) {
	case RANGE_BACKCOMPAT:
// Get highlighted playback range
		command->set_playback_range();
// Adjust playback range with in/out points
		command->playback_range_adjust_inout();
		break;
	case RANGE_PROJECT:
		command->playback_range_project();
		break;
	case RANGE_SELECTION:
		command->set_playback_range();
		break;
	case RANGE_INOUT:
		command->playback_range_inout();
		break;
	case RANGE_1FRAME:
		command->playback_range_1frame();
		break;
	}

	render->packages = new PackageDispatcher;

// Create caches
	CICache *audio_cache = new CICache(render->preferences);
	CICache *video_cache = new CICache(render->preferences);

	render->default_asset->frame_rate = command->get_edl()->session->frame_rate;
	render->default_asset->sample_rate = command->get_edl()->session->sample_rate;

// Conform asset to EDL.  Find out if any tracks are playable.
	render->result = render->check_asset(command->get_edl(),
		*render->default_asset);

	if(!render->result)
	{
// Get total range to render
		render->total_start = command->start_position;
		render->total_end = command->end_position;
		total_length = render->total_end - render->total_start;

// Nothing to render
		if(EQUIV(total_length, 0))
		{
			render->result = 1;
		}
	}

// Generate packages
	if(!render->result)
	{
// Stop background rendering
		if(mwindow) mwindow->stop_brender();

		fs.complete_path(render->default_asset->path);

		render->result = render->packages->create_packages(mwindow,
			command->get_edl(),
			render->preferences,
			strategy,
			render->default_asset,
			render->total_start,
			render->total_end,
			test_overwrite);
	}

	render->total_rendered = 0;

	if(!render->result)
	{
// Start dispatching external jobs
		if(mwindow)
		{
			mwindow->gui->lock_window("Render::render 1");
			mwindow->gui->show_message(_("Starting render farm"));
			mwindow->gui->start_hourglass();
			mwindow->gui->unlock_window();
		}
		else
		{
			printf("Render::render: starting render farm\n");
		}

		if(strategy == SINGLE_PASS_FARM || strategy == FILE_PER_LABEL_FARM)
		{
			farm_server = new RenderFarmServer(mwindow,
				render->packages,
				render->preferences,
				1,
				&render->result,
				&render->total_rendered,
				render->counter_lock,
				render->default_asset,
				command->get_edl(),
				0);
			render->result = farm_server->start_clients();

			if(render->result)
			{
				if(mwindow)
				{
					mwindow->gui->lock_window("Render::render 2");
					mwindow->gui->show_message(_("Failed to start render farm"),
						mwindow->theme->message_error);
					mwindow->gui->stop_hourglass();
					mwindow->gui->unlock_window();
				}
				else
				{
					printf("Render::render: Failed to start render farm\n");
				}
			}
		}
	}

// Perform local rendering


	if(!render->result)
	{
		render->start_progress();




		MainPackageRenderer package_renderer(render);
		render->result = package_renderer.initialize(mwindow,
				command->get_edl(),   // Copy of master EDL
				render->preferences,
				render->default_asset);







		while(!render->result)
		{
// Get unfinished job
			RenderPackage *package;

			if(strategy == SINGLE_PASS_FARM)
			{
				package = render->packages->get_package(
					package_renderer.frames_per_second,
					-1,
					1);
			}
			else
			{
				package = render->packages->get_package(0, -1, 1);
			}

// Exit point
			if(!package)
			{
				break;
			}

			if(package_renderer.render_package(package))
				render->result = 1;


		} // file_number



printf("Render::render_single: Session finished.\n");





		if( strategy == SINGLE_PASS_FARM ||
		    strategy == FILE_PER_LABEL_FARM ) {
			if( !render->progress ) {
				while( farm_server->active_clients() > 0 ) {
					sleep(1);
					render->show_progress();
				}
			}
			farm_server->wait_clients();
			render->result |= render->packages->packages_are_done();
		}

if(debug) printf("Render::render %d\n", __LINE__);

// Notify of error
		if(render->result &&
			(!render->progress || !render->progress->is_cancelled()) &&
			!render->batch_cancelled)
		{
if(debug) printf("Render::render %d\n", __LINE__);
			if(mwindow)
			{
if(debug) printf("Render::render %d\n", __LINE__);
				int cx, cy;
				mwindow->gui->get_abs_cursor(cx, cy, 1);
				ErrorBox error_box(_(PROGRAM_NAME ": Error"), cx, cy);
				error_box.create_objects(_("Error rendering data."));
				error_box.raise_window();
				error_box.run_window();
if(debug) printf("Render::render %d\n", __LINE__);
			}
			else
			{
				printf("Render::render: Error rendering data\n");
			}
		}
if(debug) printf("Render::render %d\n", __LINE__);

// Delete the progress box
		render->stop_progress();

if(debug) printf("Render::render %d\n", __LINE__);
	}


// Paste all packages into timeline if desired

	if(!render->result &&
		render->load_mode != LOADMODE_NOTHING &&
		mwindow &&
		render->mode != Render::BATCH)
	{
if(debug) printf("Render::render %d\n", __LINE__);
		mwindow->gui->lock_window("Render::render 3");
if(debug) printf("Render::render %d\n", __LINE__);

		mwindow->undo->update_undo_before();

if(debug) printf("Render::render %d\n", __LINE__);


		ArrayList<Indexable*> *assets = render->packages->get_asset_list();
if(debug) printf("Render::render %d\n", __LINE__);
		if(render->load_mode == LOADMODE_PASTE)
			mwindow->clear(0);
if(debug) printf("Render::render %d\n", __LINE__);
		mwindow->load_assets(assets, -1, render->load_mode, 0, 0,
			mwindow->edl->session->labels_follow_edits,
			mwindow->edl->session->plugins_follow_edits,
			mwindow->edl->session->autos_follow_edits,
			0); // overwrite
if(debug) printf("Render::render %d\n", __LINE__);
		for(int i = 0; i < assets->size(); i++)
			assets->get(i)->Garbage::remove_user();
		delete assets;
if(debug) printf("Render::render %d\n", __LINE__);


		mwindow->save_backup();
if(debug) printf("Render::render %d\n", __LINE__);
		mwindow->undo->update_undo_after(_("render"), LOAD_ALL);
if(debug) printf("Render::render %d\n", __LINE__);
		mwindow->update_plugin_guis();
if(debug) printf("Render::render %d\n", __LINE__);
		mwindow->gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 0);
if(debug) printf("Render::render %d\n", __LINE__);
		mwindow->sync_parameters(CHANGE_ALL);
if(debug) printf("Render::render %d\n", __LINE__);
		mwindow->gui->unlock_window();


		mwindow->awindow->gui->async_update_assets();

if(debug) printf("Render::render %d\n", __LINE__);
	}

if(debug) printf("Render::render %d\n", __LINE__);

// Disable hourglass
	if(mwindow)
	{
		mwindow->gui->lock_window("Render::render 3");
		mwindow->gui->stop_hourglass();
		mwindow->gui->unlock_window();
	}

//printf("Render::render 110\n");
// Need to restart because brender always stops before render.
	if(mwindow)
		mwindow->restart_brender();
	if(farm_server) delete farm_server;
	delete command;
	delete audio_cache;
	delete video_cache;
// Must delete packages after server
	delete render->packages;

	render->packages = 0;
	render->in_progress = 0;
if(debug) printf("Render::render %d\n", __LINE__);
}

void RenderThread::run()
{
	if( mwindow )
		render->preferences->copy_from(mwindow->preferences);

	if(render->mode == Render::INTERACTIVE)
	{
		render_single(1, render->asset, mwindow->edl,
			render->get_strategy(), render->range_type);
	}
	else
	if(render->mode == Render::BATCH)
	{
// PRINT_TRACE
// printf("RenderThread::run %d %d %d\n",
// __LINE__,
// render->jobs->total,
// render->result);
		for(int i = 0; i < render->jobs->total && !render->result; i++)
		{
//PRINT_TRACE
			BatchRenderJob *job = render->jobs->values[i];
//PRINT_TRACE
			if(job->enabled)
			{
				if( *job->edl_path == '@' )
				{
					run_script(job->edl_path+1, job->asset->path);
				}

				if(mwindow)
				{
					mwindow->batch_render->update_active(i);
				}
				else
				{
					printf("Render::run: %s\n", job->edl_path);
				}

//PRINT_TRACE

				FileXML *file = new FileXML;
				EDL *edl = new EDL;
				edl->create_objects();
				file->read_from_file(job->edl_path);
				edl->load_xml(file, LOAD_ALL);
//PRINT_TRACE
				render_single(0, job->asset, edl, job->get_strategy(), RANGE_BACKCOMPAT);

//PRINT_TRACE
				edl->Garbage::remove_user();
				delete file;
				if(!render->result)
				{
					if(mwindow)
						mwindow->batch_render->update_done(i, 1, render->elapsed_time);
					else
					{
						char string[BCTEXTLEN];
						render->elapsed_time =
							(double)render->progress_timer->get_scaled_difference(1);
						Units::totext(string,
							render->elapsed_time,
							TIME_HMS2);
						printf("Render::run: done in %s\n", string);
					}
				}
				else
				{
					if(mwindow)
						mwindow->batch_render->update_active(-1);
					else
						printf("Render::run: failed\n");
				}
			}
//PRINT_TRACE
		}

		if(mwindow)
		{
			mwindow->batch_render->update_active(-1);
			mwindow->batch_render->update_done(-1, 0, 0);
		}
	}
	render->completion->unlock();

	if( render->mode == Render::INTERACTIVE && render->beep )
		mwindow->beep(3000., 1.5, 0.5);
}


#define WIDTH 480
#define HEIGHT 480


RenderWindow::RenderWindow(MWindow *mwindow,
	Render *render,
	Asset *asset,
	int x,
	int y)
 : BC_Window(_(PROGRAM_NAME ": Render"), x, y,
 	WIDTH, HEIGHT, WIDTH, HEIGHT, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->render = render;
	this->asset = asset;
	render_format = 0;
	loadmode = 0;
	renderprofile = 0;
	rangeproject = 0;
	rangeselection = 0;
	rangeinout = 0;
	range1frame = 0;
}

RenderWindow::~RenderWindow()
{
	lock_window("RenderWindow::~RenderWindow");
	delete render_format;
	delete loadmode;
	delete renderprofile;
	unlock_window();
}


void RenderWindow::load_profile(int profile_slot)
{
	render->load_profile(profile_slot, asset);
	update_range_type(render->range_type);
	render_format->update(asset, &render->use_labels);
}


void RenderWindow::create_objects()
{
	int x = 10, y = 10;
	lock_window("RenderWindow::create_objects");
	add_subwindow(new BC_Title(x, y,
		(char*)(render->use_labels ?
			_("Select the first file to render to:") :
			_("Select a file to render to:"))));
	y += 25;

	render_format = new RenderFormat(mwindow, this, asset);
	render_format->create_objects(x, y,
		1, 1, 1, 1, 0, 1, 0, 0, &render->use_labels, 0);

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Render range:")));

	int is_image = File::is_image_render(asset->format);
	if( is_image )
		render->range_type = RANGE_1FRAME;

	int x1 = x + title->get_w() + 20, y1 = y;
	add_subwindow(rangeproject = new RenderRangeProject(this,
		render->range_type == RANGE_PROJECT, x1, y));
	int x2 = x1 + rangeproject->get_w();
	y += 20;
	add_subwindow(rangeselection = new RenderRangeSelection(this,
		render->range_type == RANGE_SELECTION, x1, y));
	int x3 = x1 + rangeselection->get_w();
	if( x2 < x3 ) x2 = x3;
	y += 20;
	add_subwindow(rangeinout = new RenderRangeInOut(this,
		render->range_type == RANGE_INOUT, x1, y));
	x3 = x1 + rangeinout->get_w();
	if( x2 < x3 ) x2 = x3;
	y += 20;
	add_subwindow(range1frame = new RenderRange1Frame(this,
		render->range_type == RANGE_1FRAME, x1, y));
	x3 = x1 + range1frame->get_w();
	if( x2 < x3 ) x2 = x3;
	y += 30;
	if( is_image )
		enable_render_range(0);

	x1 = x2 + 20;
	render->beep = mwindow->edl->session->render_beep;
	add_subwindow(beep_on_done = new RenderBeepOnDone(this, x1, y1));

	renderprofile = new RenderProfile(mwindow, this, x, y, 1);
	renderprofile->create_objects();
	y += 70;

	loadmode = new LoadMode(mwindow, this, x, y, &render->load_mode, 1);
	loadmode->create_objects();

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

	show_window();
	unlock_window();
}

void RenderWindow::update_range_type(int range_type)
{
	if( render->range_type == range_type ) return;
	render->range_type = range_type;
	rangeproject->update(range_type == RANGE_PROJECT);
	rangeselection->update(range_type == RANGE_SELECTION);
	rangeinout->update(range_type == RANGE_INOUT);
	range1frame->update(range_type == RANGE_1FRAME);
}

void RenderWindow::enable_render_range(int v)
{
	if( v ) {
		rangeproject->enable();
		rangeselection->enable();
		rangeinout->enable();
		range1frame->enable();
	}
	else {
		rangeproject->disable();
		rangeselection->disable();
		rangeinout->disable();
		range1frame->disable();
	}
}


RenderRangeProject::RenderRangeProject(RenderWindow *rwindow, int value, int x, int y)
 : BC_Radial(x, y, value, _("Project"))
{
	this->rwindow = rwindow;
}
int RenderRangeProject::handle_event()
{
	rwindow->update_range_type(RANGE_PROJECT);
	return 1;
}

RenderRangeSelection::RenderRangeSelection(RenderWindow *rwindow, int value, int x, int y)
 : BC_Radial(x, y, value, _("Selection"))
{
	this->rwindow = rwindow;
}
int RenderRangeSelection::handle_event()
{
	rwindow->update_range_type(RANGE_SELECTION);
	return 1;
}


RenderRangeInOut::RenderRangeInOut(RenderWindow *rwindow, int value, int x, int y)
 : BC_Radial(x, y, value, _("In/Out Points"))
{
	this->rwindow = rwindow;
}
int RenderRangeInOut::handle_event()
{
	rwindow->update_range_type(RANGE_INOUT);
	return 1;
}

RenderRange1Frame::RenderRange1Frame(RenderWindow *rwindow, int value, int x, int y)
 : BC_Radial(x, y, value, _("One Frame"))
{
	this->rwindow = rwindow;
}
int RenderRange1Frame::handle_event()
{
	rwindow->update_range_type(RANGE_1FRAME);
	return 1;
}

double Render::get_render_range()
{
	EDL *edl = mwindow->edl;
	double last = edl->tracks->total_playable_length();
	double framerate = edl->session->frame_rate;
	if( framerate <= 0 ) framerate = 1;
	double start = 0, end = last;
	switch( range_type ) {
	default:
	case RANGE_BACKCOMPAT:
		start = edl->local_session->get_selectionstart(1);
		end   = edl->local_session->get_selectionend(1);
		if( EQUIV(start, end) ) end = last;
		break;
	case RANGE_PROJECT:
		break;
	case RANGE_SELECTION:
		start = edl->local_session->get_selectionstart(1);
		end   = edl->local_session->get_selectionend(1);
		break;
	case RANGE_INOUT:
		start = edl->local_session->inpoint_valid() ?
			edl->local_session->get_inpoint() : 0;
		end   = edl->local_session->outpoint_valid() ?
			edl->local_session->get_outpoint() : last;
		break;
	case RANGE_1FRAME:
		start = end = edl->local_session->get_selectionstart(1);
		if( edl->session->frame_rate > 0 ) end += 1./edl->session->frame_rate;
		break;
	}
	if( start < 0 ) start = 0;
	if( end > last ) end = last;
	return end - start;
}

RenderFormat::RenderFormat(MWindow *mwindow, BC_WindowBase *window, Asset *asset)
 : FormatTools(mwindow, window, asset)
{
}
RenderFormat::~RenderFormat()
{
}

void RenderFormat::update_format()
{
	FormatTools::update_format();
	RenderWindow *render_window = (RenderWindow *)window;
	if( render_window->is_hidden() ) return;

	int is_image = File::is_image_render(asset->format);
	if( is_image ) {
		render_window->update_range_type(RANGE_1FRAME);
		render_window->enable_render_range(0);
	}
	else
		render_window->enable_render_range(1);
}

RenderBeepOnDone::RenderBeepOnDone(RenderWindow *rwindow, int x, int y)
 : BC_CheckBox(x, y, rwindow->render->beep, _("Beep on done"))
{
	this->rwindow = rwindow;
}

int RenderBeepOnDone::handle_event()
{
	rwindow->render->beep = get_value();
	return 1;
}


