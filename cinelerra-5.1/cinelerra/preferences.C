
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

#include "asset.h"
#include "audioconfig.h"
#include "audiodevice.inc"
#include "bcmeter.inc"
#include "bcsignals.h"
#include "cache.inc"
#include "clip.h"
#include "bchash.h"
#include "file.h"
#include "filesystem.h"
#include "guicast.h"
#include "indexfile.h"
#include "maxchannels.h"
#include "mutex.h"
#include "preferences.h"
#include "probeprefs.h"
#include "shbtnprefs.h"
#include "theme.h"
#include "videoconfig.h"
#include "videodevice.inc"

#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


Preferences::Preferences()
{
// Set defaults
	FileSystem fs;
	preferences_lock = new Mutex("Preferences::preferences_lock");

// initial plugin path from build -DPLUGIN_DIR="..."
	sprintf(plugin_dir, "%s/", File::get_plugin_path());
	sprintf(index_directory, "%s/", File::get_config_path());
	if( strlen(index_directory) )
		fs.complete_path(index_directory);
	cache_size = 0x10000000;
	index_size = 0x400000;
	index_count = 500;
	use_thumbnails = 1;
	keyframe_reticle = HAIRLINE_DRAGGING;
	perpetual_session = 0;
	strcpy(lv2_path, DEFAULT_LV2_PATH);
	autostart_lv2ui = 0;
	trap_sigsegv = 1;
	trap_sigintr = 1;
	awindow_picon_h = 50;
	vicon_size = 50;
	vicon_color_mode = VICON_COLOR_MODE_LOW;
	theme[0] = 0;
	plugin_icons[0] = 0;
	strcpy(snapshot_path, "/tmp");
	use_renderfarm = 0;
	force_uniprocessor = 0;
	renderfarm_port = DEAMON_PORT;
	render_preroll = 0.5;
	brender_preroll = 0;
	renderfarm_mountpoint[0] = 0;
	renderfarm_vfs = 0;
	renderfarm_job_count = 20;
	renderfarm_watchdog_timeout = 60;
	project_smp = processors = calculate_processors(0);
	real_processors = calculate_processors(1);
	ffmpeg_marker_indexes = 1;
	warn_indexes = 1;
	warn_version = 1;
	bd_warn_root = 1;
	popupmenu_btnup = 1;
	grab_input_focus = 1;
	textbox_focus_policy = 0;
	forward_render_displacement = 0;
	dvd_yuv420p_interlace = 0;
	highlight_inverse = 0xffffff;
	yuv_color_space = BC_COLORS_BT601;
	yuv_color_range = BC_COLORS_JPEG;

// Default brender asset
	brender_asset = new Asset;
	brender_asset->audio_data = 0;
	brender_asset->video_data = 1;
	sprintf(brender_asset->path, "/tmp/brender");
	brender_asset->format = FILE_JPEG_LIST;
	brender_asset->jpeg_quality = 80;

	use_brender = 0;
	brender_fragment = 1;
	local_rate = 0.0;

	use_tipwindow = 1;
	scan_commercials = 0;

	android_remote = 0;
	android_port = 23432;
	strcpy(android_pin, "cinelerra");

	memset(channel_positions, 0, sizeof(channel_positions));
	int channels = 0;
	while( channels < MAXCHANNELS ) {
		int *positions = channel_positions[channels++];
		for( int i=0; i<channels; ++i )
			positions[i] = default_audio_channel_position(i, channels);
	}
}

Preferences::~Preferences()
{
	brender_asset->Garbage::remove_user();
	shbtn_prefs.remove_all_objects();
	file_probes.remove_all_objects();
	renderfarm_nodes.remove_all_objects();
	delete preferences_lock;
}

void Preferences::copy_rates_from(Preferences *preferences)
{
	preferences_lock->lock("Preferences::copy_rates_from");
// Need to match node titles in case the order changed and in case
// one of the nodes in the source is the master node.
	local_rate = preferences->local_rate;

	for( int j=0; j<preferences->renderfarm_nodes.total; ++j ) {
		double new_rate = preferences->renderfarm_rate.values[j];
// Put in the master node
		if( preferences->renderfarm_nodes.values[j][0] == '/' ) {
			if( !EQUIV(new_rate, 0.0) )
				local_rate = new_rate;
		}
		else
// Search for local node and copy it to that node
		if( !EQUIV(new_rate, 0.0) ) {
			for( int i=0; i<renderfarm_nodes.total; ++i ) {
				if( !strcmp(preferences->renderfarm_nodes.values[j], renderfarm_nodes.values[i]) &&
					preferences->renderfarm_ports.values[j] == renderfarm_ports.values[i] ) {
					renderfarm_rate.values[i] = new_rate;
					break;
				}
			}
		}
	}

//printf("Preferences::copy_rates_from 1 %f %f\n", local_rate, preferences->local_rate);
	preferences_lock->unlock();
}

void Preferences::copy_from(Preferences *that)
{
// ================================= Performance ================================
	strcpy(index_directory, that->index_directory);
	index_size = that->index_size;
	index_count = that->index_count;
	use_thumbnails = that->use_thumbnails;
	keyframe_reticle = that->keyframe_reticle;
	perpetual_session = that->perpetual_session;
	awindow_picon_h = that->awindow_picon_h;
	vicon_size = that->vicon_size;
	vicon_color_mode = that->vicon_color_mode;
	strcpy(theme, that->theme);
	strcpy(plugin_icons, that->plugin_icons);
	strcpy(snapshot_path, that->snapshot_path);

	use_tipwindow = that->use_tipwindow;
	scan_commercials = that->scan_commercials;
	android_remote = that->android_remote;
	android_port = that->android_port;
	strcpy(android_pin, that->android_pin);
	this->shbtn_prefs.remove_all_objects();
	for( int i=0; i<that->shbtn_prefs.size(); ++i )
		this->shbtn_prefs.append(new ShBtnPref(*that->shbtn_prefs[i]));
	this->file_probes.remove_all_objects();
	for( int i=0; i<that->file_probes.size(); ++i )
		this->file_probes.append(new ProbePref(*that->file_probes[i]));
	cache_size = that->cache_size;
	project_smp = that->project_smp;
	force_uniprocessor = that->force_uniprocessor;
	strcpy(lv2_path, that->lv2_path);
	autostart_lv2ui = that->autostart_lv2ui;
	trap_sigsegv = that->trap_sigsegv;
	trap_sigintr = that->trap_sigintr;
	processors = that->processors;
	real_processors = that->real_processors;
	ffmpeg_marker_indexes = that->ffmpeg_marker_indexes;
	warn_indexes = that->warn_indexes;
	warn_version = that->warn_version;
	bd_warn_root = that->bd_warn_root;
	popupmenu_btnup = that->popupmenu_btnup;
	grab_input_focus = that->grab_input_focus;
	textbox_focus_policy = that->textbox_focus_policy;
	forward_render_displacement = that->forward_render_displacement;
	dvd_yuv420p_interlace = that->dvd_yuv420p_interlace;
	highlight_inverse = that->highlight_inverse;
	yuv_color_space = that->yuv_color_space;
	yuv_color_range = that->yuv_color_range;
	renderfarm_nodes.remove_all_objects();
	renderfarm_ports.remove_all();
	renderfarm_enabled.remove_all();
	renderfarm_rate.remove_all();
	local_rate = that->local_rate;
	for( int i=0; i<that->renderfarm_nodes.size(); ++i ) {
		add_node(that->renderfarm_nodes.get(i),
			that->renderfarm_ports.get(i),
			that->renderfarm_enabled.get(i),
			that->renderfarm_rate.get(i));
	}
	use_renderfarm = that->use_renderfarm;
	renderfarm_port = that->renderfarm_port;
	render_preroll = that->render_preroll;
	brender_preroll = that->brender_preroll;
	renderfarm_job_count = that->renderfarm_job_count;
	renderfarm_watchdog_timeout = that->renderfarm_watchdog_timeout;
	renderfarm_vfs = that->renderfarm_vfs;
	strcpy(renderfarm_mountpoint, that->renderfarm_mountpoint);
	renderfarm_consolidate = that->renderfarm_consolidate;
	use_brender = that->use_brender;
	brender_fragment = that->brender_fragment;
	brender_asset->copy_from(that->brender_asset, 0);

// Check boundaries

	FileSystem fs;
	if( strlen(index_directory) ) {
		fs.complete_path(index_directory);
		fs.add_end_slash(index_directory);
	}

// 	if( strlen(global_plugin_dir) )
// 	{
// 		fs.complete_path(global_plugin_dir);
// 		fs.add_end_slash(global_plugin_dir);
// 	}
//

// Redo with the proper value of force_uniprocessor
	processors = calculate_processors(0);
	boundaries();
}

void Preferences::boundaries()
{
	renderfarm_job_count = MAX(renderfarm_job_count, 1);
	CLAMP(cache_size, MIN_CACHE_SIZE, MAX_CACHE_SIZE);
}

Preferences& Preferences::operator=(Preferences &that)
{
printf("Preferences::operator=\n");
	copy_from(&that);
	return *this;
}

void Preferences::print_channels(char *string, int *channel_positions, int channels)
{
	char *cp = string, *ep = cp+BCTEXTLEN-1;
	for( int i=0; i<channels; ++i ) {
		if( i ) cp += snprintf(cp, ep-cp, ", ");
		cp += snprintf(cp, ep-cp, "%d", channel_positions[i]);
	}
	*cp = 0;
}

void Preferences::scan_channels(char *string, int *channel_positions, int channels)
{
	char *cp = string;
	int current_channel = 0;
	for(;;) {
		while( isspace(*cp) ) ++cp;
		if( !cp ) break;
		channel_positions[current_channel++] = strtol(cp, &cp, 0);
		if( current_channel >= channels ) break;
		while( isspace(*cp) ) ++cp;
		if( *cp == ',' ) ++cp;
	}
	while( current_channel < channels ) {
		int pos = default_audio_channel_position(current_channel, channels);
		channel_positions[current_channel++] = pos;
	}
}

int Preferences::load_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

	use_tipwindow = defaults->get("USE_TIPWINDOW", use_tipwindow);
	scan_commercials = defaults->get("SCAN_COMMERCIALS", scan_commercials);
	android_remote = defaults->get("ANDROID_REMOTE", android_remote);
	android_port = defaults->get("ANDROID_PORT", android_port);
	defaults->get("ANDROID_PIN", android_pin);
	defaults->get("INDEX_DIRECTORY", index_directory);
	index_size = defaults->get("INDEX_SIZE", index_size);
	index_count = defaults->get("INDEX_COUNT", index_count);
	keyframe_reticle = defaults->get("KEYFRAME_RETICLE", keyframe_reticle);
	perpetual_session = defaults->get("PERPETUAL_SESSION", perpetual_session);
	strcpy(lv2_path, DEFAULT_LV2_PATH);
	defaults->get("LV2_PATH", lv2_path);
	autostart_lv2ui = defaults->get("AUTOSTART_LV2UI", autostart_lv2ui);
	trap_sigsegv = defaults->get("TRAP_SIGSEGV", trap_sigsegv);
	trap_sigintr = defaults->get("TRAP_SIGINTR", trap_sigintr);

	awindow_picon_h = defaults->get("AWINDOW_PICON_H", awindow_picon_h);
	vicon_size = defaults->get("VICON_SIZE",vicon_size);
	vicon_color_mode = defaults->get("VICON_COLOR_MODE",vicon_color_mode);
	strcpy(theme, _(DEFAULT_THEME));
	strcpy(plugin_icons, DEFAULT_PICON);
	defaults->get("THEME", theme);
	defaults->get("PLUGIN_ICONS", plugin_icons);
	strcpy(snapshot_path, "/tmp");
	defaults->get("SNAPSHOT_PATH", snapshot_path);

	for( int i=0; i<MAXCHANNELS; ++i ) {
		char string2[BCTEXTLEN];
		sprintf(string, "CHANNEL_POSITIONS%d", i);
		print_channels(string2, &channel_positions[i][0], i+1);
		defaults->get(string, string2);
		scan_channels(string2, &channel_positions[i][0], i+1);
	}
	brender_asset->load_defaults(defaults, "BRENDER_", 1, 1, 1, 0, 0);

	project_smp = defaults->get("PROJECT_SMP", project_smp);
	force_uniprocessor = defaults->get("FORCE_UNIPROCESSOR", force_uniprocessor);
	ffmpeg_marker_indexes = defaults->get("FFMPEG_MARKER_INDEXES", ffmpeg_marker_indexes);
	warn_indexes = defaults->get("WARN_INDEXES", warn_indexes);
	warn_version = defaults->get("WARN_VERSION", warn_version);
	bd_warn_root = defaults->get("BD_WARN_ROOT", bd_warn_root);
	popupmenu_btnup = defaults->get("POPUPMENU_BTNUP", popupmenu_btnup);
	grab_input_focus = defaults->get("GRAB_FOCUS", grab_input_focus);
	textbox_focus_policy = defaults->get("TEXTBOX_FOCUS_POLICY", textbox_focus_policy);
	forward_render_displacement = defaults->get("FORWARD_RENDER_DISPLACEMENT", forward_render_displacement);
	dvd_yuv420p_interlace = defaults->get("DVD_YUV420P_INTERLACE", dvd_yuv420p_interlace);
	highlight_inverse = defaults->get("HIGHLIGHT_INVERSE", highlight_inverse);
	yuv_color_space = defaults->get("YUV_COLOR_SPACE", yuv_color_space);
	yuv_color_range = defaults->get("YUV_COLOR_RANGE", yuv_color_range);
	use_brender = defaults->get("USE_BRENDER", use_brender);
	brender_fragment = defaults->get("BRENDER_FRAGMENT", brender_fragment);
	cache_size = defaults->get("CACHE_SIZE", cache_size);
	local_rate = defaults->get("LOCAL_RATE", local_rate);
	use_renderfarm = defaults->get("USE_RENDERFARM", use_renderfarm);
	renderfarm_port = defaults->get("RENDERFARM_PORT", renderfarm_port);
	render_preroll = defaults->get("RENDERFARM_PREROLL", render_preroll);
	brender_preroll = defaults->get("BRENDER_PREROLL", brender_preroll);
	renderfarm_job_count = defaults->get("RENDERFARM_JOBS_COUNT", renderfarm_job_count);
	renderfarm_watchdog_timeout = defaults->get("RENDERFARM_WATCHDOG_TIMEOUT", renderfarm_watchdog_timeout);
	renderfarm_consolidate = defaults->get("RENDERFARM_CONSOLIDATE", renderfarm_consolidate);
//	renderfarm_vfs = defaults->get("RENDERFARM_VFS", renderfarm_vfs);
	defaults->get("RENDERFARM_MOUNTPOINT", renderfarm_mountpoint);
	int renderfarm_total = defaults->get("RENDERFARM_TOTAL", 0);

	for( int i = 0; i < renderfarm_total; i++ ) {
		sprintf(string, "RENDERFARM_NODE%d", i);
		char result[BCTEXTLEN];
		int result_port = 0;
		int result_enabled = 0;
		float result_rate = 0.0;

		result[0] = 0;
		defaults->get(string, result);

		sprintf(string, "RENDERFARM_PORT%d", i);
		result_port = defaults->get(string, renderfarm_port);

		sprintf(string, "RENDERFARM_ENABLED%d", i);
		result_enabled = defaults->get(string, result_enabled);

		sprintf(string, "RENDERFARM_RATE%d", i);
		result_rate = defaults->get(string, result_rate);

		if( result[0] != 0 ) {
			add_node(result, result_port, result_enabled, result_rate);
		}
	}

	shbtn_prefs.remove_all_objects();
	int shbtns_total = defaults->get("SHBTNS_TOTAL", -1);
	if( shbtns_total < 0 ) {
		shbtn_prefs.append(new ShBtnPref(_("Features"), "$CIN_BROWSER file://$CIN_DAT/doc/Features5.pdf", 0));
		shbtn_prefs.append(new ShBtnPref(_("Online Help"), "$CIN_BROWSER https://cinelerra-cv.org/docs.php", 0));
		shbtn_prefs.append(new ShBtnPref(_("Original Manual"), "$CIN_BROWSER file://$CIN_DAT/doc/cinelerra.html", 0));
		shbtn_prefs.append(new ShBtnPref(_("Setting Shell Commands"), "$CIN_BROWSER file://$CIN_DAT/doc/ShellCmds.html", 0));
		shbtn_prefs.append(new ShBtnPref(_("Shortcuts"), "$CIN_BROWSER file://$CIN_DAT/doc/shortcuts.html", 0));
		shbtn_prefs.append(new ShBtnPref(_("RenderMux"), "$CIN_DAT/doc/RenderMux.sh",0));
		shbtns_total = 0;
	}
	for( int i=0; i<shbtns_total; ++i ) {
		char name[BCTEXTLEN], commands[BCTEXTLEN];
		sprintf(string, "SHBTN%d_NAME", i);
		defaults->get(string, name);
		sprintf(string, "SHBTN%d_COMMANDS", i);
		defaults->get(string, commands);
		sprintf(string, "SHBTN%d_WARN", i);
		int warn = defaults->get(string, 0);
		shbtn_prefs.append(new ShBtnPref(name, commands, warn));
	}

	file_probes.remove_all_objects();
	int file_probe_total = defaults->get("FILE_PROBE_TOTAL", 0);
	for( int i=0; i<file_probe_total; ++i ) {
		char name[BCTEXTLEN];
		sprintf(string, "FILE_PROBE%d_NAME", i);
		defaults->get(string, name);
		sprintf(string, "FILE_PROBE%d_ARMED", i);
		int armed = defaults->get(string, 1);
		file_probes.append(new ProbePref(name, armed));
	}
	// append any missing probes
	for( int i=0; i<File::nb_probes; ++i ) {
		const char *nm = File::default_probes[i];
		int k = file_probes.size();
		while( --k>=0 && strcmp(nm, file_probes[k]->name) );
		if( k >= 0 ) continue;
		int armed = 1;
		if( !strcmp(nm, "FFMPEG_Late") ||
		    !strcmp(nm, "CR2") ) armed = 0;
		file_probes.append(new ProbePref(nm, armed));
	}

// Redo with the proper value of force_uniprocessor
	processors = calculate_processors(0);
	boundaries();
	return 0;
}

int Preferences::save_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];


	defaults->update("USE_TIPWINDOW", use_tipwindow);
	defaults->update("SCAN_COMMERCIALS", scan_commercials);
	defaults->update("ANDROID_REMOTE", android_remote);
	defaults->update("ANDROID_PIN", android_pin);
	defaults->update("ANDROID_PORT", android_port);

	defaults->update("CACHE_SIZE", cache_size);
	defaults->update("INDEX_DIRECTORY", index_directory);
	defaults->update("INDEX_SIZE", index_size);
	defaults->update("INDEX_COUNT", index_count);
	defaults->update("USE_THUMBNAILS", use_thumbnails);
	defaults->update("KEYFRAME_RETICLE", keyframe_reticle);
	defaults->update("PERPETUAL_SESSION", perpetual_session);
	defaults->update("LV2_PATH", lv2_path);
	defaults->update("AUTOSTART_LV2UI", autostart_lv2ui);
	defaults->update("TRAP_SIGSEGV", trap_sigsegv);
	defaults->update("TRAP_SIGINTR", trap_sigintr);
	defaults->update("AWINDOW_PICON_H", awindow_picon_h);
	defaults->update("VICON_SIZE",vicon_size);
	defaults->update("VICON_COLOR_MODE",vicon_color_mode);
	defaults->update("THEME", theme);
	defaults->update("PLUGIN_ICONS", plugin_icons);
	defaults->update("SNAPSHOT_PATH", snapshot_path);

	for( int i = 0; i < MAXCHANNELS; i++ ) {
		char string2[BCTEXTLEN];
		sprintf(string, "CHANNEL_POSITIONS%d", i);
		print_channels(string2, &channel_positions[i][0], i + 1);
		defaults->update(string, string2);
	}

	defaults->update("PROJECT_SMP", project_smp);
	defaults->update("FORCE_UNIPROCESSOR", force_uniprocessor);
	defaults->update("FFMPEG_MARKER_INDEXES", ffmpeg_marker_indexes);
	defaults->update("WARN_INDEXES", warn_indexes);
	defaults->update("WARN_VERSION", warn_version);
	defaults->update("BD_WARN_ROOT", bd_warn_root);
	defaults->update("POPUPMENU_BTNUP", popupmenu_btnup);
	defaults->update("GRAB_FOCUS", grab_input_focus);
	defaults->update("TEXTBOX_FOCUS_POLICY", textbox_focus_policy);
	defaults->update("FORWARD_RENDER_DISPLACEMENT", forward_render_displacement);
	defaults->update("DVD_YUV420P_INTERLACE", dvd_yuv420p_interlace);
	defaults->update("HIGHLIGHT_INVERSE", highlight_inverse);
	defaults->update("YUV_COLOR_SPACE", yuv_color_space);
	defaults->update("YUV_COLOR_RANGE", yuv_color_range);
	brender_asset->save_defaults(defaults, "BRENDER_", 1, 1, 1, 0, 0);
	defaults->update("USE_BRENDER", use_brender);
	defaults->update("BRENDER_FRAGMENT", brender_fragment);
	defaults->update("USE_RENDERFARM", use_renderfarm);
	defaults->update("LOCAL_RATE", local_rate);
	defaults->update("RENDERFARM_PORT", renderfarm_port);
	defaults->update("RENDERFARM_PREROLL", render_preroll);
	defaults->update("BRENDER_PREROLL", brender_preroll);
//	defaults->update("RENDERFARM_VFS", renderfarm_vfs);
	defaults->update("RENDERFARM_MOUNTPOINT", renderfarm_mountpoint);
	defaults->update("RENDERFARM_JOBS_COUNT", renderfarm_job_count);
	defaults->update("RENDERFARM_WATCHDOG_TIMEOUT", renderfarm_watchdog_timeout);
	defaults->update("RENDERFARM_CONSOLIDATE", renderfarm_consolidate);
	defaults->update("RENDERFARM_TOTAL", (int64_t)renderfarm_nodes.total);
	for( int i = 0; i < renderfarm_nodes.total; i++ ) {
		sprintf(string, "RENDERFARM_NODE%d", i);
		defaults->update(string, renderfarm_nodes.values[i]);
		sprintf(string, "RENDERFARM_PORT%d", i);
		defaults->update(string, renderfarm_ports.values[i]);
		sprintf(string, "RENDERFARM_ENABLED%d", i);
		defaults->update(string, renderfarm_enabled.values[i]);
		sprintf(string, "RENDERFARM_RATE%d", i);
		defaults->update(string, renderfarm_rate.values[i]);
	}
	defaults->update("SHBTNS_TOTAL", shbtn_prefs.size());
	for( int i=0; i<shbtn_prefs.size(); ++i ) {
		ShBtnPref *pref = shbtn_prefs[i];
		sprintf(string, "SHBTN%d_NAME", i);
		defaults->update(string, pref->name);
		sprintf(string, "SHBTN%d_COMMANDS", i);
		defaults->update(string, pref->commands);
		sprintf(string, "SHBTN%d_WARN", i);
		defaults->update(string, pref->warn);
	}
	defaults->update("FILE_PROBE_TOTAL", file_probes.size());
	for( int i=0; i<file_probes.size(); ++i ) {
		ProbePref *pref = file_probes[i];
		sprintf(string, "FILE_PROBE%d_NAME", i);
		defaults->update(string, pref->name);
		sprintf(string, "FILE_PROBE%d_ARMED", i);
		defaults->update(string, pref->armed);
	}
	return 0;
}


void Preferences::add_node(const char *text, int port, int enabled, float rate)
{
	if( text[0] == 0 ) return;

	preferences_lock->lock("Preferences::add_node");
	char *new_item = new char[strlen(text) + 1];
	strcpy(new_item, text);
	renderfarm_nodes.append(new_item);
	renderfarm_nodes.set_array_delete();
	renderfarm_ports.append(port);
	renderfarm_enabled.append(enabled);
	renderfarm_rate.append(rate);
	preferences_lock->unlock();
}

void Preferences::delete_node(int number)
{
	preferences_lock->lock("Preferences::delete_node");
	if( number < renderfarm_nodes.total && number >= 0 ) {
		delete [] renderfarm_nodes.values[number];
		renderfarm_nodes.remove_number(number);
		renderfarm_ports.remove_number(number);
		renderfarm_enabled.remove_number(number);
		renderfarm_rate.remove_number(number);
	}
	preferences_lock->unlock();
}

void Preferences::delete_nodes()
{
	preferences_lock->lock("Preferences::delete_nodes");
	for( int i = 0; i < renderfarm_nodes.total; i++ )
		delete [] renderfarm_nodes.values[i];
	renderfarm_nodes.remove_all();
	renderfarm_ports.remove_all();
	renderfarm_enabled.remove_all();
	renderfarm_rate.remove_all();
	preferences_lock->unlock();
}

void Preferences::reset_rates()
{
	for( int i = 0; i < renderfarm_nodes.total; i++ ) {
		renderfarm_rate.values[i] = 0.0;
	}
	local_rate = 0.0;
}

float Preferences::get_rate(int node)
{
	if( node < 0 ) {
		return local_rate;
	}
	else {
		int total = 0;
		for( int i = 0; i < renderfarm_nodes.size(); i++ ) {
			if( renderfarm_enabled.get(i) ) total++;
			if( total == node + 1 ) {
				return renderfarm_rate.get(i);
			}
		}
	}

	return 0;
}

void Preferences::set_rate(float rate, int node)
{
//printf("Preferences::set_rate %f %d\n", rate, node);
	if( node < 0 ) {
		local_rate = rate;
	}
	else {
		int total = 0;
		for( int i = 0; i < renderfarm_nodes.size(); i++ ) {
			if( renderfarm_enabled.get(i) ) total++;
			if( total == node + 1 ) {
				renderfarm_rate.set(i, rate);
				return;
			}
		}
	}
}

float Preferences::get_avg_rate(int use_master_node)
{
	preferences_lock->lock("Preferences::get_avg_rate");
	float total = 0.0;
	if( renderfarm_rate.total ) {
		int enabled = 0;
		if( use_master_node ) {
			if( EQUIV(local_rate, 0.0) ) {
				preferences_lock->unlock();
				return 0.0;
			}
			else {
				enabled++;
				total += local_rate;
			}
		}

		for( int i = 0; i < renderfarm_rate.total; i++ ) {
			if( renderfarm_enabled.values[i] ) {
				enabled++;
				total += renderfarm_rate.values[i];
				if( EQUIV(renderfarm_rate.values[i], 0.0) ) {
					preferences_lock->unlock();
					return 0.0;
				}
			}
		}

		if( enabled )
			total /= enabled;
		else
			total = 0.0;
	}
	preferences_lock->unlock();

	return total;
}

void Preferences::sort_nodes()
{
	int done = 0;

	while(!done)
	{
		done = 1;
		for( int i = 0; i < renderfarm_nodes.total - 1; i++ ) {
			if( strcmp(renderfarm_nodes.values[i], renderfarm_nodes.values[i + 1]) > 0 ) {
				char *temp = renderfarm_nodes.values[i];
				int temp_port = renderfarm_ports.values[i];

				renderfarm_nodes.values[i] = renderfarm_nodes.values[i + 1];
				renderfarm_nodes.values[i + 1] = temp;

				renderfarm_ports.values[i] = renderfarm_ports.values[i + 1];
				renderfarm_ports.values[i + 1] = temp_port;

				renderfarm_enabled.values[i] = renderfarm_enabled.values[i + 1];
				renderfarm_enabled.values[i + 1] = temp_port;

				renderfarm_rate.values[i] = renderfarm_rate.values[i + 1];
				renderfarm_rate.values[i + 1] = temp_port;
				done = 0;
			}
		}
	}
}

void Preferences::edit_node(int number,
	const char *new_text,
	int new_port,
	int new_enabled)
{
	char *new_item = new char[strlen(new_text) + 1];
	strcpy(new_item, new_text);

	delete [] renderfarm_nodes.values[number];
	renderfarm_nodes.values[number] = new_item;
	renderfarm_ports.values[number] = new_port;
	renderfarm_enabled.values[number] = new_enabled;
}

int Preferences::get_enabled_nodes()
{
	int result = 0;
	for( int i = 0; i < renderfarm_enabled.total; i++ )
		if( renderfarm_enabled.values[i] ) result++;
	return result;
}

const char* Preferences::get_node_hostname(int number)
{
	int total = 0;
	for( int i = 0; i < renderfarm_nodes.total; i++ ) {
		if( renderfarm_enabled.values[i] ) {
			if( total == number )
				return renderfarm_nodes.values[i];
			else
				total++;
		}
	}
	return "";
}

int Preferences::get_node_port(int number)
{
	int total = 0;
	for( int i = 0; i < renderfarm_ports.total; i++ ) {
		if( renderfarm_enabled.values[i] ) {
			if( total == number )
				return renderfarm_ports.values[i];
			else
				total++;
		}
	}
	return -1;
}

int Preferences::get_asset_file_path(Asset *asset, char *path)
{
	strcpy(path, asset->path);
	int result = !access(path, R_OK) ? 0 : -1;
	if( !result && ( asset->format == FILE_MPEG || asset->format == FILE_AC3 ||
		asset->format == FILE_VMPEG || asset->format == FILE_AMPEG ) ) {
		char source_filename[BCTEXTLEN], index_filename[BCTEXTLEN];
		IndexFile::get_index_filename(source_filename,
			index_directory, index_filename, asset->path, ".toc");
		strcpy(path, index_filename);
		if( access(path, R_OK) )
			result = 1;
	}
// result = 0, asset->path/toc exist, -1 no asset, 1 no toc
	return result;
}


int Preferences::calculate_processors(int interactive)
{
	if( force_uniprocessor && !interactive ) return 1;
	return BC_WindowBase::get_resources()->machine_cpus;
}

int Preferences::get_file_probe_armed(const char *nm)
{
	int k = file_probes.size();
	while( --k>=0 && strcmp(nm, file_probes[k]->name) );
	if( k < 0 ) return -1;
	return file_probes[k]->armed;
}

void Preferences::set_file_probe_armed(const char *nm, int v)
{
	int k = file_probes.size();
	while( --k>=0 && strcmp(nm, file_probes[k]->name) );
	if( k < 0 ) return;
	file_probes[k]->armed = v;
}

