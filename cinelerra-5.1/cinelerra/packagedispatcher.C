
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

#include "asset.h"
#include "clip.h"
#include "confirmsave.h"
#include "edl.h"
#include "edlsession.h"
#include "labels.h"
#include "mutex.h"
#include "mwindow.h"
#include "packagedispatcher.h"
#include "packagerenderer.h"
#include "preferences.h"
#include "render.h"
#include "file.h"



PackageDispatcher::PackageDispatcher()
{
	packages = 0;
	package_lock = new Mutex("PackageDispatcher::package_lock");
	packaging_engine = 0;
}

PackageDispatcher::~PackageDispatcher()
{
	if(packages)
	{
		for(int i = 0; i < total_packages; i++)
			delete packages[i];
		delete [] packages;
	}
	if (packaging_engine)
		delete packaging_engine;
	delete package_lock;
}

int PackageDispatcher::create_packages(MWindow *mwindow, EDL *edl,
	Preferences *preferences, int strategy, Asset *default_asset,
	double total_start, double total_end, int test_overwrite)
{
	Label *label;
	int result = 0;

	this->mwindow = mwindow;
	this->edl = edl;
	this->preferences = preferences;
	this->strategy = strategy;
	this->default_asset = default_asset;
	this->total_start = total_start;
	this->total_end = total_end;

	nodes = preferences->get_enabled_nodes();
	audio_position = Units::to_int64(total_start * default_asset->sample_rate);
	video_position = Units::to_int64(total_start * default_asset->frame_rate);
	audio_end = Units::to_int64(total_end * default_asset->sample_rate);
	video_end = Units::to_int64(total_end * default_asset->frame_rate);
	current_package = 0;

// sleep(1);
// printf("PackageDispatcher::create_packages 1 %d %f %f\n",
// video_end,
// total_end,
// default_asset->frame_rate);

	switch( strategy ) {
	case SINGLE_PASS:
		total_len = this->total_end - this->total_start;
		package_len = total_len;
		min_package_len = total_len;
		total_packages = 1;
		total_allocated = 1;
		packages = new RenderPackage*[total_allocated];
		packages[0] = new RenderPackage;
		packages[0]->audio_start = audio_position;
		packages[0]->audio_end = audio_end;
		packages[0]->video_start = video_position;
		packages[0]->video_end = video_end;
		packages[0]->audio_do = default_asset->audio_data;
		packages[0]->video_do = default_asset->video_data;
		strcpy(packages[0]->path, default_asset->path);
		break;
	case SINGLE_PASS_FARM:
		packaging_engine = File::new_packaging_engine(default_asset);
		packaging_engine->create_packages_single_farm(edl, preferences,
				default_asset, total_start, total_end);
		break;
	case FILE_PER_LABEL:
	case FILE_PER_LABEL_FARM:
		label = edl->labels->first;
		total_packages = 0;
		packages = new RenderPackage*[edl->labels->total() + 2];

		Render::get_starting_number(default_asset->path,
			current_number, number_start, total_digits, 3);

		while( audio_position < audio_end ) {
			RenderPackage *package = new RenderPackage;
			packages[total_packages++] = package;
			package->audio_start = audio_position;
			package->video_start = video_position;
			package->audio_do = default_asset->audio_data;
			package->video_do = default_asset->video_data;

			while( label &&
				(label->position < (double)audio_position / default_asset->sample_rate ||
				EQUIV(label->position, (double)audio_position / default_asset->sample_rate)) ) {
				label = label->next;
			}

			if( !label ) {
				package->audio_end = Units::to_int64(total_end * default_asset->sample_rate);
				package->video_end = Units::to_int64(total_end * default_asset->frame_rate);
			}
			else {
				package->audio_end = Units::to_int64(label->position * default_asset->sample_rate);
				package->video_end = Units::to_int64(label->position * default_asset->frame_rate);
			}

			if( package->audio_end > audio_end ) {
				package->audio_end = audio_end;
			}

			if( package->video_end > video_end ) {
				package->video_end = video_end;
			}

			audio_position = package->audio_end;
			video_position = package->video_end;

// Create file number differently if image file sequence
			Render::create_filename(package->path, default_asset->path,
				current_number++, total_digits, number_start);
		}

		total_allocated = total_packages;
		break;
	case BRENDER_FARM:
		total_len = this->total_end - this->total_start;

// Create packages as they're requested.
		total_packages = 0;
		total_allocated = 0;
		packages = 0;

		Render::get_starting_number(default_asset->path,
			current_number, number_start, total_digits, 6);

// Master node only
		if( preferences->renderfarm_nodes.total == 1 ) {
			package_len = total_len;
			min_package_len = total_len;
		}
		else {
			package_len = preferences->brender_fragment /
				edl->session->frame_rate;
			min_package_len = 1.0 / edl->session->frame_rate;
		}
		break;
	}

// Test existence of every output file.
// Only if this isn't a background render or non interactive.
	if( strategy != BRENDER_FARM && test_overwrite && mwindow ) {
		ArrayList<char*> paths;
		get_package_paths(&paths);
		result = ConfirmSave::test_files(mwindow, &paths);
		paths.remove_all_objects();
	}

	return result;
}

void PackageDispatcher::get_package_paths(ArrayList<char*> *path_list)
{
		if (strategy == SINGLE_PASS_FARM)
			packaging_engine->get_package_paths(path_list);
		else {
			for( int i=0; i<total_allocated; ++i )
				path_list->append(strdup(packages[i]->path));
			path_list->set_free();
		}

}

RenderPackage* PackageDispatcher::get_package(double frames_per_second,
		int client_number, int use_local_rate)
{
	package_lock->lock("PackageDispatcher::get_package");

// Store new frames per second for the node
	if( !EQUIV(frames_per_second, 0) ) {
		preferences->set_rate(frames_per_second, client_number);
		if(mwindow) mwindow->preferences->copy_rates_from(preferences);
	}
	else {
// Use previous frames per second
		frames_per_second = preferences->get_rate(client_number);
	}

	float avg_frames_per_second = preferences->get_avg_rate(use_local_rate);

	RenderPackage *result = 0;
	switch( strategy ) {
//printf("PackageDispatcher::get_package 1 %d\n", strategy);
	case SINGLE_PASS:
	case FILE_PER_LABEL:
	case FILE_PER_LABEL_FARM:
		if( current_package < total_packages )
			result = packages[current_package++];
		break;
	case SINGLE_PASS_FARM:
		result = packaging_engine->get_package_single_farm(frames_per_second,
					client_number, use_local_rate);
		break;
	case BRENDER_FARM:
//printf("Dispatcher::get_package 1 %d %d\n", video_position, video_end);
		if( video_position < video_end ) {
// Allocate new packages
			if( total_packages == 0 ) {
				total_allocated = 256;
				packages = new RenderPackage*[total_allocated];
			}
			else if( total_packages >= total_allocated ) {
				RenderPackage **old_packages = packages;
				total_allocated *= 2;
				packages = new RenderPackage*[total_allocated];
				memcpy(packages,
					old_packages,
					total_packages * sizeof(RenderPackage*));
				delete [] old_packages;
			}

// Calculate package.
			result = packages[total_packages] = new RenderPackage;
			double scaled_len;

// No load balancing data exists
			if( EQUIV(frames_per_second, 0) ||
			     EQUIV(avg_frames_per_second, 0)) {
				scaled_len = package_len;
			}
			else {
// Load balancing data exists
				scaled_len = package_len *
					frames_per_second / avg_frames_per_second;
			}

			scaled_len = MAX(scaled_len, min_package_len);

// Always an image file sequence
			result->audio_start = audio_position;
			result->video_start = video_position;
			result->audio_end = result->audio_start +
				Units::to_int64(scaled_len * default_asset->sample_rate);
			result->video_end = result->video_start +
				Units::to_int64(scaled_len * default_asset->frame_rate);
			if(result->video_end == result->video_start) result->video_end++;
			audio_position = result->audio_end;
			video_position = result->video_end;
			result->audio_do = default_asset->audio_data;
			result->video_do = default_asset->video_data;


// The frame numbers are read from the vframe objects themselves.
			Render::create_filename(result->path, default_asset->path,
					0, total_digits, number_start);
//printf("PackageDispatcher::get_package 2 %s\n", result->path);
			++current_number;
			++total_packages;
			++current_package;
		}
		break;
	}

	package_lock->unlock();
	return result;
}


int PackageDispatcher::get_asset_list(ArrayList<Indexable*> &idxbls)
{
	if( strategy == SINGLE_PASS_FARM )
		return packaging_engine->get_asset_list(idxbls);
	for( int i=0; i<current_package; ++i ) {
		Asset *asset = new Asset;
		asset->copy_from(default_asset, 1);
		strcpy(asset->path, packages[i]->path);
		asset->video_length = packages[i]->video_end - packages[i]->video_start;
		asset->audio_length = packages[i]->audio_end - packages[i]->audio_start;
		idxbls.append(asset);
	}
	return current_package;
}

int64_t PackageDispatcher::get_progress_max()
{
	return strategy == SINGLE_PASS_FARM ?
		packaging_engine->get_progress_max() :
		Units::to_int64(default_asset->sample_rate * (total_end - total_start)) +
			Units::to_int64(preferences->render_preroll *
				total_allocated * default_asset->sample_rate);
}

int PackageDispatcher::get_total_packages()
{
	return total_allocated;
}

int PackageDispatcher::packages_are_done()
{
	return packaging_engine ? packaging_engine->packages_are_done() : 0;
}

