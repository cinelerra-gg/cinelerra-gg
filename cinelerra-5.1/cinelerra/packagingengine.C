
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

#include "packagingengine.h"
#include "preferences.h"
#include "edlsession.h"
#include "asset.h"
#include "render.h"
#include "clip.h"


// Default packaging engine implementation, simply split the range up, and consider client's speed

PackagingEngineDefault::PackagingEngineDefault()
{
	packages = 0;
}

PackagingEngineDefault::~PackagingEngineDefault()
{
	if(packages)
	{
		for(int i = 0; i < total_packages; i++)
			delete packages[i];
		delete [] packages;
	}
}


int PackagingEngineDefault::create_packages_single_farm(EDL *edl,
		Preferences *preferences, Asset *default_asset,
		double total_start, double total_end)
{
	this->total_start = total_start;
	this->total_end = total_end;

	this->preferences = preferences;
	this->default_asset = default_asset;
	audio_position = Units::to_int64(total_start * default_asset->sample_rate);
	video_position = Units::to_int64(total_start * default_asset->frame_rate);
	audio_end = Units::to_int64(total_end * default_asset->sample_rate);
	video_end = Units::to_int64(total_end * default_asset->frame_rate);
	current_package = 0;
	current_position = 0;

	double total_len = total_end - total_start;
 	total_packages = preferences->renderfarm_job_count;
	total_allocated = total_packages + preferences->get_enabled_nodes();
	packages = new RenderPackage*[total_allocated];
	package_len = total_len / total_packages;
	min_package_len = 2.0 / edl->session->frame_rate;

	int current_number;    // The number being injected into the filename.
	int number_start;      // Character in the filename path at which the number begins
	int total_digits;      // Total number of digits including padding the user specified.

	Render::get_starting_number(default_asset->path,
		current_number, number_start, total_digits, 3);

	for( int i=0; i<total_allocated; ++i ) {
		RenderPackage *package = packages[i] = new RenderPackage;

// Create file number differently if image file sequence
		Render::create_filename(package->path, default_asset->path,
			current_number, total_digits, number_start);
		current_number++;
	}
	return 0;
}

RenderPackage* PackagingEngineDefault::get_package_single_farm(double frames_per_second,
		int client_number, int use_local_rate)
{
	RenderPackage *result = 0;
	float avg_frames_per_second = preferences->get_avg_rate(use_local_rate);
	double length = package_len;
	int scaled_length = 0;

	if( (default_asset->audio_data &&
		(audio_position < audio_end && !EQUIV(audio_position, audio_end))) ||
	    (default_asset->video_data &&
		(video_position < video_end && !EQUIV(video_position, video_end))) ) {
// Last package
		result = packages[current_package];
		result->audio_start = audio_position;
		result->video_start = video_position;
		result->video_do = default_asset->video_data;
		result->audio_do = default_asset->audio_data;

		if( current_package >= total_allocated-1 ) {
			result->audio_end = audio_end;
			result->video_end = video_end;
 			audio_position = result->audio_end;
			video_position = result->video_end;
		}
		else {
			if( frames_per_second > 0 && 
			    !EQUIV(frames_per_second, 0) && !EQUIV(avg_frames_per_second, 0) ) {
// package size to fit the requestor.
				length *= frames_per_second / avg_frames_per_second;
				scaled_length = 1;
			}
			if( length < min_package_len )
				length = min_package_len;
			double end_position = current_position + length;

			if( result->video_do ) {
				int64_t video_end = end_position * default_asset->frame_rate;
				result->video_end = MIN(this->video_end, video_end);
				end_position = video_end / default_asset->frame_rate;
			}
			if( result->audio_do ) {
				int64_t audio_end = end_position * default_asset->sample_rate;
				result->audio_end = MIN(this->audio_end, audio_end);
			}
			audio_position = result->audio_end;
			video_position = result->video_end;
			current_position = end_position;

// Package size is no longer touched between total_packages and total_allocated
			if( scaled_length && current_package < total_packages-1 ) {
				double remaining =
					result->audio_do ? (double)(audio_end - audio_position) /
						default_asset->sample_rate :
					result->video_do ? (double)(video_end - video_position) /
						default_asset->frame_rate : 0;
				if( remaining > 0 ) {
					int jobs = total_packages - current_package;
					package_len = remaining / jobs;
				}
			}
		}

		current_package++;
//printf("Dispatcher::get_package 50 %lld %lld %lld %lld\n",
// result->audio_start, result->video_start, result->audio_end, result->video_end);
	}
	return result;
}

void PackagingEngineDefault::get_package_paths(ArrayList<char*> *path_list)
{
	for( int i=0; i<total_allocated; ++i ) {
		path_list->append(strdup(packages[i]->path));
	}
	path_list->set_free();
}

int PackagingEngineDefault::get_asset_list(ArrayList<Indexable *> &idxbls)
{
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

int64_t PackagingEngineDefault::get_progress_max()
{
	return Units::to_int64(default_asset->sample_rate * (total_end - total_start)) +
		Units::to_int64(preferences->render_preroll * 2 * default_asset->sample_rate);
}

int PackagingEngineDefault::packages_are_done()
{
	return 0;
}

