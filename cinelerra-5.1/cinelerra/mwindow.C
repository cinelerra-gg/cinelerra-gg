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

#include "aboutprefs.h"
#include "asset.h"
#include "assets.h"
#include "atrack.h"
#include "audioalsa.h"
#include "autos.h"
#include "awindowgui.h"
#include "awindow.h"
#include "batchrender.h"
#include "bcdisplayinfo.h"
#include "bcprogressbox.h"
#include "bcsignals.h"
#include "bctimer.h"
#include "bctrace.h"
#include "bdcreate.h"
#include "brender.h"
#include "cache.h"
#include "channel.h"
#include "channeldb.h"
#include "channelinfo.h"
#include "clip.h"
#include "clipedls.h"
#include "bccmodels.h"
#include "commercials.h"
#include "confirmsave.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "cwindowtool.h"
#include "bchash.h"
#include "devicedvbinput.inc"
#include "dvdcreate.h"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "fileformat.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "floatautos.h"
#include "framecache.h"
#include "gwindow.h"
#include "gwindowgui.h"
#include "keyframegui.h"
#include "indexfile.h"
#include "intautos.h"
#include "interlacemodes.h"
#include "language.h"
#include "levelwindowgui.h"
#include "levelwindow.h"
#include "loadfile.inc"
#include "localsession.h"
#include "maincursor.h"
#include "mainerror.h"
#include "mainindexes.h"
#include "mainmenu.h"
#include "mainprogress.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mbuttons.h"
#include "mutex.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "new.h"
#include "panautos.h"
#include "patchbay.h"
#include "playback3d.h"
#include "playbackengine.h"
#include "plugin.h"
#include "pluginserver.h"
#include "pluginset.h"
#include "preferences.h"
#include "proxy.h"
#include "record.h"
#include "recordmonitor.h"
#include "recordlabel.h"
#include "removefile.h"
#include "render.h"
#include "resourcethread.h"
#include "savefile.inc"
#include "samplescroll.h"
#include "sha1.h"
#include "shuttle.h"
#include "sighandler.h"
#include "splashgui.h"
#include "statusbar.h"
#include "theme.h"
#include "threadloader.h"
#include "timebar.h"
#include "timelinepane.h"
#include "tipwindow.h"
#include "trackcanvas.h"
#include "track.h"
#include "tracking.h"
#include "trackscroll.h"
#include "tracks.h"
#include "transition.h"
#include "transportque.h"
#include "vframe.h"
#include "vtrack.h"
#include "versioninfo.h"
#include "vicon.h"
#include "videodevice.inc"
#include "videowindow.h"
#include "vplayback.h"
#include "vwindowgui.h"
#include "vwindow.h"
#include "wavecache.h"
#include "wwindow.h"
#include "zoombar.h"
#include "zwindow.h"
#include "zwindowgui.h"
#include "exportedl.h"

#include "defaultformats.h"
#include "ntsczones.h"

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <limits.h>
#include <errno.h>


extern "C"
{




// Hack for libdv to remove glib dependancy

// void
// g_log (const char    *log_domain,
//        int  log_level,
//        const char    *format,
//        ...)
// {
// }
//
// void
// g_logv (const char    *log_domain,
//        int  log_level,
//        const char    *format,
//        ...)
// {
// }
//

}


extern long cin_timezone;

ArrayList<PluginServer*>* MWindow::plugindb = 0;
Commercials* MWindow::commercials = 0;


MWindow::MWindow()
 : Thread(1, 0, 0)
{
	run_lock = new Mutex("MWindow::run_lock");
	plugin_gui_lock = new Mutex("MWindow::plugin_gui_lock");
	dead_plugin_lock = new Mutex("MWindow::dead_plugin_lock");
	vwindows_lock = new Mutex("MWindow::vwindows_lock");
	zwindows_lock = new Mutex("MWindow::zwindows_lock");
	brender_lock = new Mutex("MWindow::brender_lock");
	keyframe_gui_lock = new Mutex("MWindow::keyframe_gui_lock");

	playback_3d = 0;
	splash_window = 0;
	undo = 0;
	undo_command = COMMAND_NONE;
	defaults = 0;
	assets = 0;
	//commercials = 0;
	commercial_active = 0;
	audio_cache = 0;
	video_cache = 0;
	frame_cache = 0;
	wave_cache = 0;
	preferences = 0;
	session = 0;
	theme = 0;
	mainindexes = 0;
	mainprogress = 0;
	brender = 0;
	brender_active = 0;
	strcpy(cin_lang,"en");
	channeldb_buz =  new ChannelDB;
	channeldb_v4l2jpeg =  new ChannelDB;
	plugin_guis = 0;
	dead_plugins = 0;
	keyframe_threads = 0;
	create_bd = 0;
	create_dvd = 0;
	batch_render = 0;
	render = 0;
	edl = 0;
	gui = 0;
	cwindow = 0;
	awindow = 0;
	gwindow = 0;
	twindow = 0;
	wwindow = 0;
	lwindow = 0;
	sighandler = 0;
	restart_status = 0;
	screens = 1;
	in_destructor = 0;
	speed_edl = 0;
	proxy_beep = 0;
	shuttle = 0;
}


// Need to delete brender temporary here.
MWindow::~MWindow()
{
	run_lock->lock("MWindow::~MWindow");
	in_destructor = 1;
//printf("MWindow::~MWindow %d\n", __LINE__);
	if( wwindow && wwindow->is_running() )
		wwindow->close_window();
	if( twindow && twindow->is_running() )
		twindow->close_window();
	gui->remote_control->deactivate();
	gui->record->stop();
#ifdef HAVE_DVB
	gui->channel_info->stop();
#endif
	delete proxy_beep;
	delete create_bd;       create_bd = 0;
	delete create_dvd;      create_dvd = 0;
	delete shuttle;         shuttle = 0;
	delete batch_render;    batch_render = 0;
	delete render;          render = 0;
	commit_commercial();
	if( commercials && !commercials->remove_user() ) commercials = 0;
	close_mixers();
	if( speed_edl ) { speed_edl->remove_user();  speed_edl = 0; }
// Save defaults for open plugins
	plugin_gui_lock->lock("MWindow::~MWindow");
	for(int i = 0; i < plugin_guis->size(); i++) {
		plugin_guis->get(i)->hide_gui();
		delete_plugin(plugin_guis->get(i));
	}
	plugin_gui_lock->unlock();
	hide_keyframe_guis();
	clean_indexes();
	save_defaults();
// Give up and go to a movie
//  cant run valgrind if this is used

	gui->del_keyboard_listener(
		(int (BC_WindowBase::*)(BC_WindowBase *))
		&MWindowGUI::keyboard_listener);
#if 0
// release the hounds
	if( awindow && awindow->gui ) awindow->gui->close(0);
	if( cwindow && cwindow->gui ) cwindow->gui->close(0);
	if( lwindow && lwindow->gui ) lwindow->gui->close(0);
	if( gwindow && gwindow->gui ) gwindow->gui->close(0);
	vwindows.remove_all_objects();
	zwindows.remove_all_objects();
	gui->close(0);
	if( awindow ) awindow->join();
	if( cwindow ) cwindow->join();
	if( lwindow ) lwindow->join();
	if( gwindow ) gwindow->join();
	join();
#else
// one at a time, or nouveau chokes
#define close_gui(win) if( win ) { \
  if( win->gui ) win->gui->close(0); \
  win->join(); }
	close_gui(awindow);
	close_gui(cwindow);
	close_gui(lwindow);
	close_gui(gwindow);
	vwindows.remove_all_objects();
	zwindows.remove_all_objects();
	gui->close(0);
	join();
#endif
	reset_caches();
	dead_plugins->remove_all_objects();
// must delete theme before destroying plugindb
//  theme destructor will be deleted by delete_plugins
	delete theme;           theme = 0;
	delete_plugins();
	finit_error();
	keyframe_threads->remove_all_objects();
	colormodels.remove_all_objects();
	delete awindow;         awindow = 0;
	delete lwindow;         lwindow = 0;
	delete twindow;         twindow = 0;
	delete wwindow;         wwindow = 0;
	delete gwindow;         gwindow = 0;
	delete cwindow;         cwindow = 0;
	delete gui;		gui = 0;
	delete mainindexes;     mainindexes = 0;
	delete mainprogress;    mainprogress = 0;
	delete audio_cache;     audio_cache = 0;  // delete the cache after the assets
	delete video_cache;     video_cache = 0;  // delete the cache after the assets
	delete frame_cache;     frame_cache = 0;
	delete wave_cache;      wave_cache = 0;
	delete plugin_guis;     plugin_guis = 0;
	delete dead_plugins;    dead_plugins = 0;
	delete keyframe_threads;  keyframe_threads = 0;
	delete undo;            undo = 0;
	delete preferences;     preferences = 0;
	delete exportedl;	exportedl = 0;
	delete session;         session = 0;
	delete defaults;        defaults = 0;
	delete assets;          assets = 0;
	delete splash_window;   splash_window = 0;
	if( !edl->Garbage::remove_user() ) edl = 0;
	delete channeldb_buz;
	delete channeldb_v4l2jpeg;
// This must be last thread to exit
	delete playback_3d;	playback_3d = 0;
	delete dead_plugin_lock;
	delete plugin_gui_lock;
	delete vwindows_lock;
	delete zwindows_lock;
	delete brender_lock;
	delete keyframe_gui_lock;
	colormodels.remove_all_objects();
	interlace_project_modes.remove_all_objects();
	interlace_asset_modes.remove_all_objects();
	interlace_asset_fixmethods.remove_all_objects();
	sighandler->terminate();
	delete sighandler;
	delete run_lock;
}


void MWindow::quit()
{
	gui->set_done(0);
}

void MWindow::init_error()
{
	MainError::init_error(this);
}

void MWindow::finit_error()
{
	MainError::finit_error();
}

void MWindow::create_defaults_path(char *string, const char *config_file)
{
// set the .bcast path
	FileSystem fs;

	sprintf(string, "%s/", File::get_config_path());
	fs.complete_path(string);
	if(!fs.is_dir(string))
		fs.create_dir(string);

// load the defaults
	strcat(string, config_file);
}
const char *MWindow::default_std()
{
	char buf[BCTEXTLEN], *p = 0;

	int fd = open(TIMEZONE_NAME, O_RDONLY);
	if( fd >= 0 ) {
		int l = read(fd, buf, sizeof(buf)-1);
		close(fd);
		if( l > 0 ) {
			if( buf[l-1] == '\n' ) --l;
			buf[l] = 0;
			p = buf;
		}
	}
	if( !p ) {
		int l = readlink(LOCALTIME_LINK, buf, sizeof(buf)-1);
		if( l > 0 ) {
			buf[l] = 0;
			if( !(p=strstr(buf, ZONEINFO_STR)) != 0 ) return "PAL";
			p += strlen(ZONEINFO_STR);
		}
	}

	if( p ) {
		for( int i=0; ntsc_zones[i]; ++i ) {
			if( !strcmp(ntsc_zones[i], p) )
				return "NTSC";
		}
	}

	p = getenv("TZ");
	if( p ) {
		for( int i=0; ntsc_zones[i]; ++i ) {
			if( !strcmp(ntsc_zones[i], p) )
				return "NTSC";
		}
	}

// cin_timezone: Seconds west of UTC.  240sec/deg
	double tz_deg =  -cin_timezone / 240.;
// from Honolulu = -10, to New York = -5, 15deg/hr   lat -150..-75
	return tz_deg >= -10*15 && tz_deg <= -5*15 ? "NTSC" : "PAL";
}

void MWindow::fill_preset_defaults(const char *preset, EDLSession *session)
{
	struct formatpresets *fpr;

	for(fpr = &format_presets[0]; fpr->name; fpr++)
	{
		if(strcmp(_(fpr->name), preset) == 0)
		{
			session->audio_channels = fpr->audio_channels;
			session->audio_tracks = fpr->audio_tracks;
			session->sample_rate = fpr->sample_rate;
			session->video_channels = fpr->video_channels;
			session->video_tracks = fpr->video_tracks;
			session->frame_rate = fpr->frame_rate;
			session->output_w = fpr->output_w;
			session->output_h = fpr->output_h;
			session->aspect_w = fpr->aspect_w;
			session->aspect_h = fpr->aspect_h;
			session->interlace_mode = fpr->interlace_mode;
			session->color_model = fpr->color_model;
			return;
		}
	}
}

const char *MWindow::get_preset_name(int index)
{
	if(index < 0 || index >= (int)MAX_NUM_PRESETS)
		return "Error";
	return _(format_presets[index].name);
}


void MWindow::init_defaults(BC_Hash* &defaults, char *config_path)
{
	char path[BCTEXTLEN];
// Use user supplied path
	if(config_path[0])
		strcpy(path, config_path);
	else
		create_defaults_path(path, CONFIG_FILE);

	delete defaults;
	defaults = new BC_Hash(path);
	defaults->load();
}


void MWindow::check_language()
{
	char curr_lang[BCTEXTLEN]; curr_lang[0] = 0;
	const char *env_lang = getenv("LANGUAGE");
	if( !env_lang ) env_lang = getenv("LC_ALL");
	if( !env_lang ) env_lang = getenv("LANG");
	if( !env_lang ) {
		snprintf(curr_lang, sizeof(curr_lang), "%s_%s-%s",
			BC_Resources::language, BC_Resources::region, BC_Resources::encoding);
		env_lang = curr_lang;
	}
	char last_lang[BCTEXTLEN]; last_lang[0] = 0;
	defaults->get("LAST_LANG",last_lang);
	if( strcmp(env_lang,last_lang)) {
		printf("lang changed from '%s' to '%s'\n", last_lang, env_lang);
		defaults->update("LAST_LANG",env_lang);
		char plugin_path[BCTEXTLEN];
		create_defaults_path(plugin_path, PLUGIN_FILE);
		::remove(plugin_path);
		char ladspa_path[BCTEXTLEN];
		create_defaults_path(ladspa_path, LADSPA_FILE);
		::remove(ladspa_path);
		defaults->save();
	}
	if( strlen(env_lang) > 1 &&
	    ( env_lang[2] == 0 || env_lang[2] == '_'  || env_lang[2] == '.' ) ) {
		cin_lang[0] = env_lang[0];  cin_lang[1] = env_lang[1];  cin_lang[2] = 0;
	}
	else
		strcpy(cin_lang, "en");
}

void MWindow::get_plugin_path(char *path, const char *plug_dir, const char *fs_path)
{
	char *base_path = FileSystem::basepath(fs_path), *bp = base_path;
	if( plug_dir ) {
		const char *dp = plug_dir;
		while( *bp && *dp && *bp == *dp ) { ++bp; ++dp; }
		bp = !*dp && *bp == '/' ? bp+1 : base_path;
	}
	strcpy(path, bp);
	delete [] base_path;
}

int MWindow::load_plugin_index(MWindow *mwindow, FILE *fp, const char *plugin_dir)
{
	if( !fp ) return -1;
// load index
	fseek(fp, 0, SEEK_SET);
	int ret = 0;
	char index_line[BCTEXTLEN];
	int index_version = -1, len = strlen(plugin_dir);
	if( !fgets(index_line, BCTEXTLEN, fp) ||
	    sscanf(index_line, "%d", &index_version) != 1 ||
	    index_version != PLUGIN_FILE_VERSION ||
	    !fgets(index_line, BCTEXTLEN, fp) ||
	    (int)strlen(index_line)-1 != len || index_line[len] != '\n' ||
	    strncmp(index_line, plugin_dir, len) != 0 ) ret = 1;

	ArrayList<PluginServer*> plugins;
	while( !ret && !feof(fp) && fgets(index_line, BCTEXTLEN, fp) ) {
		if( index_line[0] == ';' ) continue;
		if( index_line[0] == '#' ) continue;
		int type = PLUGIN_TYPE_UNKNOWN;
		char path[BCTEXTLEN], title[BCTEXTLEN];
		int64_t mtime = 0;
		if( PluginServer::scan_table(index_line, type, path, title, mtime) ) {
			ret = 1; continue;
		}
		PluginServer *server = 0;
		switch( type ) {
		case PLUGIN_TYPE_BUILTIN:
		case PLUGIN_TYPE_EXECUTABLE:
		case PLUGIN_TYPE_LADSPA: {
			char plugin_path[BCTEXTLEN];  struct stat st;
			sprintf(plugin_path, "%s/%s", plugin_dir, path);
			if( stat(plugin_path, &st) || st.st_mtime != mtime ) {
				ret = 1; continue;
			}
			server = new PluginServer(mwindow, plugin_path, type);
			break; }
		case PLUGIN_TYPE_FFMPEG: {
			server = new_ffmpeg_server(mwindow, path);
			break; }
		case PLUGIN_TYPE_LV2: {
			server = new_lv2_server(mwindow, path);
			break; }
		}
		if( !server ) continue;
		plugins.append(server);
// Create plugin server from index entry
		server->set_title(title);
		if( server->read_table(index_line) ) {
			ret = 1;  continue;
		}
	}

	if( !ret )
		ret = check_plugin_index(plugins, plugin_dir, ".");

	if( !ret )
		add_plugins(plugins);
	else
		plugins.remove_all_objects();

	return ret;
}

int MWindow::check_plugin_index(ArrayList<PluginServer*> &plugins,
	const char *plug_dir, const char *plug_path)
{
	char plugin_path[BCTEXTLEN];
	sprintf(plugin_path, "%s/%s", plug_dir, plug_path);
	FileSystem fs;
	fs.set_filter( "[*.plugin][*.so]" );
	if( fs.update(plugin_path) )
		return 1;

	for( int i=0; i<fs.dir_list.total; ++i ) {
		char fs_path[BCTEXTLEN];
		get_plugin_path(fs_path, 0, fs.dir_list[i]->path);
		if( fs.is_dir(fs_path) ) {
			get_plugin_path(plugin_path, plug_dir, fs_path);
			if( check_plugin_index(plugins, plug_dir, plugin_path) )
				return 1;
		}
		else if( !plugin_exists(fs_path, plugins) )
			return 1;
	}
	return 0;
}


int MWindow::init_plugins(MWindow *mwindow, Preferences *preferences)
{
	if( !plugindb )
		plugindb = new ArrayList<PluginServer*>;
	init_ffmpeg();
	char index_path[BCTEXTLEN], plugin_path[BCTEXTLEN];
	create_defaults_path(index_path, PLUGIN_FILE);
	char *plugin_dir = FileSystem::basepath(preferences->plugin_dir);
	strcpy(plugin_path, plugin_dir);  delete [] plugin_dir;
	FILE *fp = fopen(index_path,"a+");
	if( !fp ) {
		fprintf(stderr,_("MWindow::init_plugins: "
			"can't open plugin index: %s\n"), index_path);
		return -1;
	}
	int fd = fileno(fp), ret = -1;
	if( !flock(fd, LOCK_EX) ) {
		fseek(fp, 0, SEEK_SET);
		ret = load_plugin_index(mwindow, fp, plugin_path);
	}
	if( ret > 0 ) {
		ftruncate(fd, 0);
		fseek(fp, 0, SEEK_SET);
		printf("init plugin index: %s\n", plugin_path);
		fprintf(fp, "%d\n", PLUGIN_FILE_VERSION);
		fprintf(fp, "%s\n", plugin_path);
		init_plugin_index(mwindow, preferences, fp, plugin_path);
		init_ffmpeg_index(mwindow, preferences, fp);
		init_lv2_index(mwindow, preferences, fp);
		fseek(fp, 0, SEEK_SET);
		ret = load_plugin_index(mwindow, fp, plugin_path);
	}
	if( ret ) {
		fprintf(stderr,_("MWindow::init_plugins: "
			"can't %s plugin index: %s\n"),
			ret>0 ? _("create") : _("lock"), index_path);
	}
	fclose(fp);
	return ret;
}

int MWindow::init_ladspa_plugins(MWindow *mwindow, Preferences *preferences)
{
	char *path = getenv("LADSPA_PATH");
	char ladspa_path[BCTEXTLEN];
	if( !path ) {
		strncpy(ladspa_path, File::get_ladspa_path(), sizeof(ladspa_path));
		path = ladspa_path;
	}
	for( int len=0; *path; path+=len ) {
		char *cp = strchr(path,':');
		len = !cp ? strlen(path) : cp-path;
		char index_path[BCTEXTLEN], plugin_path[BCTEXTLEN];
		memcpy(plugin_path, path, len);  plugin_path[len] = 0;
		if( cp ) ++len;
		char *plugin_dir = FileSystem::basepath(plugin_path);
		strcpy(plugin_path, plugin_dir);  delete [] plugin_dir;
		create_defaults_path(index_path, LADSPA_FILE);
		cp = index_path + strlen(index_path);
		for( char *bp=plugin_path; *bp!=0; ++bp )
			*cp++ = *bp=='/' ? '_' : *bp;
		*cp = 0;
		FILE *fp = fopen(index_path,"a+");
		if( !fp ) {
			fprintf(stderr,_("MWindow::init_ladspa_plugins: "
				"can't open ladspa plugin index: %s\n"), index_path);
			continue;
		}
		int fd = fileno(fp), ret = -1;
		if( !flock(fd, LOCK_EX) ) {
			fseek(fp, 0, SEEK_SET);
			ret = load_plugin_index(mwindow, fp, plugin_path);
		}
		if( ret > 0 ) {
			ftruncate(fd, 0);
			fseek(fp, 0, SEEK_SET);
			init_ladspa_index(mwindow, preferences, fp, plugin_path);
			fseek(fp, 0, SEEK_SET);
			ret = load_plugin_index(mwindow, fp, plugin_path);
		}
		if( ret ) {
			fprintf(stderr,_("MWindow::init_ladspa_plugins: "
				"can't %s ladspa plugin index: %s\n"),
				ret>0 ? _("create") : _("lock"), index_path);
		}
		fclose(fp);
	}
	return 1;
}

void MWindow::init_plugin_index(MWindow *mwindow, Preferences *preferences,
	FILE *fp, const char *plugin_dir)
{
	int idx = PLUGIN_IDS;
	if( plugindb ) {
		for( int i=0; i<plugindb->size(); ++i ) {
			PluginServer *server = plugindb->get(i);
			if( server->dir_idx >= idx )
				idx = server->dir_idx+1;
		}
	}
	scan_plugin_index(mwindow, preferences, fp, plugin_dir, ".", idx);
}

void MWindow::scan_plugin_index(MWindow *mwindow, Preferences *preferences, FILE *fp,
	const char *plug_dir, const char *plug_path, int &idx)
{
	char plugin_path[BCTEXTLEN];
	sprintf(plugin_path, "%s/%s", plug_dir, plug_path);
	FileSystem fs;
	fs.set_filter( "[*.plugin][*.so]" );
	int result = fs.update(plugin_path);
	if( result || !fs.dir_list.total ) return;
	int vis_id = idx++;

	for( int i=0; i<fs.dir_list.total; ++i ) {
		char fs_path[BCTEXTLEN], path[BCTEXTLEN];
		get_plugin_path(fs_path, 0, fs.dir_list[i]->path);
		get_plugin_path(path, plug_dir, fs_path);
		if( fs.is_dir(fs_path) ) {
			scan_plugin_index(mwindow, preferences, fp, plug_dir, path, idx);
			continue;
		}
		if( plugin_exists(path) ) continue;
		struct stat st;
		if( stat(fs_path, &st) ) continue;
		int64_t mtime = st.st_mtime;
		PluginServer server(mwindow, fs_path, PLUGIN_TYPE_UNKNOWN);
		result = server.open_plugin(1, preferences, 0, 0);
		if( !result ) {
			server.write_table(fp, path, vis_id, mtime);
			server.close_plugin();
		}
		else if( result == PLUGINSERVER_IS_LAD ) {
			int lad_index = 0;
			for(;;) {
				PluginServer ladspa(mwindow, fs_path, PLUGIN_TYPE_LADSPA);
				ladspa.set_lad_index(lad_index++);
				result = ladspa.open_plugin(1, preferences, 0, 0);
				if( result ) break;
				ladspa.write_table(fp, path, PLUGIN_LADSPA_ID, mtime);
				ladspa.close_plugin();
			}
		}
	}
}

void MWindow::add_plugins(ArrayList<PluginServer*> &plugins)
{
	for( int i=0; i<plugins.size(); ++i )
		plugindb->append(plugins[i]);
	plugins.remove_all();
}

void MWindow::init_plugin_tips(ArrayList<PluginServer*> &plugins, const char *lang)
{
	const char *dat_path = File::get_cindat_path();
	char msg_path[BCTEXTLEN];
	FILE *fp = 0;
	if( BC_Resources::language[0] ) {
		snprintf(msg_path, sizeof(msg_path), "%s/info/plugins.%s",
			dat_path, lang);
		fp = fopen(msg_path, "r");
	}
	if( !fp ) {
		snprintf(msg_path, sizeof(msg_path), "%s/info/plugins.txt",
			dat_path);
		fp = fopen(msg_path, "r");
	}
	if( !fp ) return;
	char text[BCTEXTLEN];
	char *tp = text, *ep = tp + sizeof(text)-1;
	char title[BCTEXTLEN];
	title[0] = 0;
	int no = 0;
	for(;;) {
		++no;  int done = 1;
		char line[BCTEXTLEN], *cp = line;
		if( fgets(line,sizeof(line)-1,fp) ) {
			if( *cp == '#' ) continue;
			done = *cp == ' ' || *cp == '\t' ? 0 : -1;
		}
		if( done ) {
			if( tp > text && *--tp == '\n' ) *tp = 0;
			if( title[0] ) {
				int idx = plugins.size();
				while( --idx>=0 && strcmp(plugins[idx]->title, title) );
				if( idx >= 0 ) {
					delete [] plugins[idx]->tip;
					plugins[idx]->tip = cstrdup(text);
				}
				title[0] = 0;
			}
			if( done > 0 ) break;
			tp = text;  *tp = 0;
			char *dp = strchr(cp, ':');
			if( !dp ) {
				printf("plugin tips: error on line %d\n", no);
				continue;
			}
			char *bp = title;
			while( cp < dp ) *bp++ = *cp++;
			*bp = 0;
			++cp;
		}

		while( *cp == ' ' || *cp == '\t' ) ++cp;
		for( ; tp<ep && (*tp=*cp)!=0; ++tp,++cp );
	}
	fclose(fp);
}

void MWindow::delete_plugins()
{
	plugindb->remove_all_objects();
	delete plugindb;
	plugindb = 0;
}

void MWindow::search_plugindb(int do_audio,
		int do_video,
		int is_realtime,
		int is_transition,
		int is_theme,
		ArrayList<PluginServer*> &results)
{
// Get plugins
	for(int i = 0; i < MWindow::plugindb->total; i++)
	{
		PluginServer *current = MWindow::plugindb->get(i);

		if(current->audio == do_audio &&
			current->video == do_video &&
			(current->realtime == is_realtime || is_realtime < 0) &&
			current->transition == is_transition &&
			current->theme == is_theme)
			results.append(current);
	}

// Alphabetize list by title
	int done = 0;
	while(!done)
	{
		done = 1;

		for(int i = 0; i < results.total - 1; i++)
		{
			PluginServer *value1 = results[i];
			PluginServer *value2 = results[i + 1];
			if(strcmp(_(value1->title), _(value2->title)) > 0)
			{
				done = 0;
				results[i] = value2;
				results[i + 1] = value1;
			}
		}
	}
}

PluginServer* MWindow::scan_plugindb(char *title,
		int data_type)
{
// 	if(data_type < 0)
// 	{
// 		printf("MWindow::scan_plugindb data_type < 0\n");
// 		return 0;
// 	}

	for(int i = 0; i < plugindb->total; i++)
	{
		PluginServer *server = plugindb->get(i);
		if(server->title &&
			!strcasecmp(server->title, title) &&
			(data_type < 0 ||
				(data_type == TRACK_AUDIO && server->audio) ||
				(data_type == TRACK_VIDEO && server->video)))
			return plugindb->get(i);
	}
	return 0;
}

// repair session files with xlated plugin titles
void MWindow::fix_plugin_title(char *title)
{
	for(int i = 0; i < plugindb->total; i++) {
		PluginServer *server = plugindb->get(i);
		if( !server->title ) continue;
		const char *server_title = server->title;
		if( !bstrcasecmp(title, _(server_title)) ) {
			strcpy(title, server_title);
			return;
		}
	}
}

int MWindow::plugin_exists(const char *plugin_path, ArrayList<PluginServer*> &plugins)
{
	for( int i=0; i<plugins.size(); ++i )
		if( !strcmp(plugin_path, plugins[i]->get_path()) ) return 1;
	return 0;
}

int MWindow::plugin_exists(char *plugin_path)
{
	return !plugindb ? 0 : plugin_exists(plugin_path, *plugindb);
}

void MWindow::remove_plugin_index()
{
	char index_path[BCTEXTLEN];
	MWindow::create_defaults_path(index_path, PLUGIN_FILE);
	::remove(index_path);
}

void MWindow::init_preferences()
{
	preferences = new Preferences;
	preferences->load_defaults(defaults);
	const char *lv2_path = getenv("LV2_PATH");
	if( lv2_path && strcmp(lv2_path, preferences->lv2_path) ) {
		strncpy(preferences->lv2_path, lv2_path, sizeof(preferences->lv2_path));
		remove_plugin_index();
	}
	else if( !lv2_path && preferences->lv2_path[0] ) {
		File::setenv_path("LV2_PATH",preferences->lv2_path, 0);
	}
	session = new MainSession(this);
	session->load_defaults(defaults);
	// set x11_host, screens, window_config
	screens = session->set_default_x11_host();
	BC_Signals::set_trap_hook(trap_hook, this);
	BC_Signals::set_catch_segv(preferences->trap_sigsegv);
	BC_Signals::set_catch_intr(preferences->trap_sigintr);
	if( preferences->trap_sigsegv || preferences->trap_sigintr ) {
		BC_Trace::enable_locks();
	}
	else {
		BC_Trace::disable_locks();
	}
	BC_WindowBase::get_resources()->popupmenu_btnup = preferences->popupmenu_btnup;
	BC_WindowBase::get_resources()->textbox_focus_policy = preferences->textbox_focus_policy;
	BC_WindowBase::get_resources()->grab_input_focus = preferences->grab_input_focus;
	YUV::yuv.yuv_set_colors(preferences->yuv_color_space, preferences->yuv_color_range);
}

void MWindow::clean_indexes()
{
	FileSystem fs;
	int total_excess;
	long oldest = 0;
	int oldest_item = -1;
	int result;
	char string[BCTEXTLEN];
	char string2[BCTEXTLEN];

// Delete extra indexes
	fs.set_filter("*.idx");
	fs.complete_path(preferences->index_directory);
	fs.update(preferences->index_directory);
//printf("MWindow::clean_indexes 1 %d\n", fs.dir_list.total);

// Eliminate directories
	result = 1;
	while(result)
	{
		result = 0;
		for(int i = 0; i < fs.dir_list.total && !result; i++)
		{
			fs.join_names(string, preferences->index_directory, fs.dir_list[i]->name);
			if(fs.is_dir(string))
			{
				delete fs.dir_list[i];
				fs.dir_list.remove_number(i);
				result = 1;
			}
		}
	}
	total_excess = fs.dir_list.total - preferences->index_count;

//printf("MWindow::clean_indexes 2 %d\n", fs.dir_list.total);
	while(total_excess > 0)
	{
// Get oldest
		for(int i = 0; i < fs.dir_list.total; i++)
		{
			fs.join_names(string, preferences->index_directory, fs.dir_list[i]->name);

			if(i == 0 || fs.get_date(string) <= oldest)
			{
				oldest = fs.get_date(string);
				oldest_item = i;
			}
		}

		if(oldest_item >= 0)
		{
// Remove index file
			fs.join_names(string,
				preferences->index_directory,
				fs.dir_list[oldest_item]->name);
//printf("MWindow::clean_indexes 1 %s\n", string);
			if(remove(string))
				perror("delete_indexes");
			delete fs.dir_list[oldest_item];
			fs.dir_list.remove_number(oldest_item);

// Remove table of contents if it exists
			strcpy(string2, string);
			char *ptr = strrchr(string2, '.');
			if(ptr)
			{
//printf("MWindow::clean_indexes 2 %s\n", string2);
				sprintf(ptr, ".toc");
				remove(string2);
				sprintf(ptr, ".mkr");
				remove(string2);
			}
		}

		total_excess--;
	}
}

void MWindow::init_awindow()
{
	awindow = new AWindow(this);
	awindow->create_objects();
}

void MWindow::init_gwindow()
{
	gwindow = new GWindow(this);
	gwindow->create_objects();
}

void MWindow::init_tipwindow()
{
	TipWindow::load_tips(cin_lang);
	if( !twindow )
		twindow = new TipWindow(this);
	twindow->start();
}

void MWindow::show_warning(int *do_warning, const char *text)
{
	if( do_warning && !*do_warning ) return;
	if( !wwindow )
		wwindow = new WWindow(this);
	wwindow->show_warning(do_warning, text);
}

int MWindow::wait_warning()
{
	return wwindow->wait_result();
}

void MWindow::init_theme()
{
	Timer timer;
	theme = 0;

	PluginServer *theme_plugin = 0;
	for(int i = 0; i < plugindb->total && !theme_plugin; i++) {
		if( plugindb->get(i)->theme &&
		    !strcasecmp(preferences->theme, plugindb->get(i)->title) )
			theme_plugin = plugindb->get(i);
	}

	if( !theme_plugin )
		fprintf(stderr, _("MWindow::init_theme: prefered theme %s not found.\n"),
			 preferences->theme);

	const char *default_theme = DEFAULT_THEME;
	if( !theme_plugin && strcasecmp(preferences->theme, default_theme) ) {
		fprintf(stderr, _("MWindow::init_theme: trying default theme %s\n"),
			default_theme);
		for(int i = 0; i < plugindb->total && !theme_plugin; i++) {
			if( plugindb->get(i)->theme &&
			    !strcasecmp(default_theme, plugindb->get(i)->title) )
				theme_plugin = plugindb->get(i);
		}
	}

	if(!theme_plugin) {
		fprintf(stderr, _("MWindow::init_theme: theme_plugin not found.\n"));
		exit(1);
	}

	PluginServer *plugin = new PluginServer(*theme_plugin);
	if( plugin->open_plugin(0, preferences, 0, 0) ) {
		fprintf(stderr, _("MWindow::init_theme: unable to load theme %s\n"),
			theme_plugin->title);
		exit(1);
	}

	theme = plugin->new_theme();
	theme->mwindow = this;
	strcpy(theme->path, plugin->path);
	delete plugin;

// Load default images & settings
	theme->Theme::initialize();
// Load user images & settings
	theme->initialize();
// Create menus with user colors
	theme->build_menus();
	init_menus();

	theme->sort_image_sets();
	theme->check_used();
//printf("MWindow::init_theme %d total_time=%d\n", __LINE__, (int)timer.get_difference());
}

void MWindow::init_3d()
{
	playback_3d = new Playback3D(this);
	playback_3d->create_objects();
}

void MWindow::init_edl()
{
	edl = new EDL;
	edl->create_objects();
	fill_preset_defaults(default_standard, edl->session);
	edl->load_defaults(defaults);
	edl->session->brender_start = edl->session->brender_end = 0;
	edl->create_default_tracks();
	edl->tracks->update_y_pixels(theme);
}

void MWindow::init_compositor()
{
	cwindow = new CWindow(this);
	cwindow->create_objects();
}

void MWindow::init_levelwindow()
{
	lwindow = new LevelWindow(this);
	lwindow->create_objects();
}

VWindow *MWindow::get_viewer(int start_it, int idx)
{
	vwindows_lock->lock("MWindow::get_viewer");
	VWindow *vwindow = idx >= 0 && idx < vwindows.size() ? vwindows[idx] : 0;
	if( !vwindow ) idx = vwindows.size();
	while( !vwindow && --idx >= 0 ) {
		VWindow *vwin = vwindows[idx];
		if( !vwin->is_running() || !vwin->get_edl() )
			vwindow = vwin;
	}
	if( !vwindow ) {
		vwindow = new VWindow(this);
		vwindow->load_defaults();
		vwindow->create_objects();
		vwindows.append(vwindow);
	}
	vwindows_lock->unlock();
	if( start_it ) vwindow->start();
	return vwindow;
}

ZWindow *MWindow::get_mixer(Mixer *&mixer)
{
	zwindows_lock->lock("MWindow::get_mixer");
	if( !mixer ) mixer = edl->mixers.new_mixer();
	ZWindow *zwindow = 0;
	for( int i=0; !zwindow && i<zwindows.size(); ++i )
		if( zwindows[i]->idx < 0 ) zwindow = zwindows[i];
	if( !zwindow )
		zwindows.append(zwindow = new ZWindow(this));
	zwindow->idx = mixer->idx;
	zwindows_lock->unlock();
	return zwindow;
}

void MWindow::del_mixer(ZWindow *zwindow)
{
	zwindows_lock->lock("MWindow::del_mixer 0");
	edl->mixers.del_mixer(zwindow->idx);
	if( session->selected_zwindow >= 0 ) {
		int i = zwindows.number_of(zwindow);
		if( i >= 0 && i < session->selected_zwindow )
			--session->selected_zwindow;
		else if( i == session->selected_zwindow )
			session->selected_zwindow = -1;
	}
	zwindows_lock->unlock();
	gui->lock_window("MWindow::del_mixer 1");
	gui->update_mixers(0, -1);
	gui->unlock_window();
}

void MWindow::start_mixer()
{
	Mixer *mixer = 0;
	ZWindow *zwindow = get_mixer(mixer);
	const char *title = 0;

	for( Track *track=edl->tracks->first; track!=0; track=track->next ) {
		PatchGUI *patchgui = get_patchgui(track);
		if( !patchgui || !patchgui->mixer ) continue;
		mixer->mixer_ids.append(track->get_mixer_id());
		if( !title ) title = track->title;
	}

	session->selected_zwindow = -1;
	gui->lock_window("MWindow::start_mixer");
	gui->update_mixers(0, 0);
	gui->unlock_window();

	zwindow->set_title(title);
	zwindow->start();
	refresh_mixers();
}

int MWindow::mixer_track_active(Track *track)
{
	int i = session->selected_zwindow;
	if( i < 0 || i >= zwindows.size() ) return 0;
	ZWindow *zwindow = zwindows[i];
	Mixer *mixer = edl->mixers.get_mixer(zwindow->idx);
	if( !mixer ) return 0;
	int n = mixer->mixer_ids.number_of(track->get_mixer_id());
	return n >= 0 ? 1 : 0;
}

void MWindow::update_mixer_tracks()
{
	zwindows_lock->lock("MixPatch::handle_event");
	int i = session->selected_zwindow;
	if( i >= 0 && i < zwindows.size() ) {
		ZWindow *zwindow = zwindows[i];
		zwindow->update_mixer_ids();
	}
	zwindows_lock->unlock();
}

void MWindow::handle_mixers(EDL *edl, int command, int wait_tracking,
		int use_inout, int toggle_audio, int loop_play, float speed)
{
	zwindows_lock->lock("MWindow::handle_mixers");
	for( int vidx=0; vidx<zwindows.size(); ++vidx ) {
		ZWindow *zwindow = zwindows[vidx];
		if( zwindow->idx < 0 ) continue;
		Mixer *mixer = edl->mixers.get_mixer(zwindow->idx);
		if( !mixer || !mixer->mixer_ids.size() ) continue;
		int k = -1;
		for( Track *track = edl->tracks->first; k<0 && track!=0; track=track->next ) {
			if( track->data_type != TRACK_VIDEO ) continue;
			int mixer_id = track->get_mixer_id();
			k = mixer->mixer_ids.size();
			while( --k >= 0 && mixer_id != mixer->mixer_ids[k] );
		}
		if( k < 0 ) continue;
		EDL *mixer_edl = new EDL(this->edl);
		mixer_edl->create_objects();
		mixer_edl->copy_all(edl);
		mixer_edl->remove_vwindow_edls();
		for( Track *track = mixer_edl->tracks->first; track!=0; track=track->next ) {
			k = mixer->mixer_ids.size();
			while( --k >= 0 && track->get_mixer_id() != mixer->mixer_ids[k] );
			if( k >= 0 ) {
				track->record = 1;
				track->play = track->data_type == TRACK_VIDEO ? 1 : 0;
			}
			else
				track->record = track->play = 0;
		}
		zwindow->change_source(mixer_edl);
		zwindow->handle_mixer(command, 0,
				use_inout, toggle_audio, loop_play, speed);
	}
	zwindows_lock->unlock();
}

void MWindow::refresh_mixers(int dir)
{
	int command = dir >= 0 ? CURRENT_FRAME : LAST_FRAME;
	handle_mixers(edl, command, 0, 0, 0, 0, 0);
}

void MWindow::stop_mixers()
{
	for( int vidx=0; vidx<zwindows.size(); ++vidx ) {
		ZWindow *zwindow = zwindows[vidx];
		if( zwindow->idx < 0 ) continue;
		zwindow->handle_mixer(STOP, 0, 0, 0, 0, 0);
	}
}

void MWindow::close_mixers(int destroy)
{
	ArrayList<ZWindow*> closed;
	zwindows_lock->lock("MWindow::close_mixers");
	for( int i=zwindows.size(); --i>=0; ) {
		ZWindow *zwindow = zwindows[i];
		if( zwindow->idx < 0 ) continue;
		zwindow->destroy = destroy;
		ZWindowGUI *zgui = zwindow->zgui;
		zgui->lock_window("MWindow::select_zwindow 0");
		zgui->set_done(0);
		zgui->unlock_window();
		closed.append(zwindow);
	}
	zwindows_lock->unlock();
	for( int i=0; i<closed.size(); ++i ) {
		ZWindow *zwindow = closed[i];
		zwindow->join();
	}
}

ZWindow *MWindow::create_mixer(Indexable *indexable)
{
	ArrayList<Indexable*> new_assets;
	new_assets.append(indexable);
	Track *track = edl->tracks->last;
	load_assets(&new_assets, 0, LOADMODE_NEW_TRACKS, 0, 0, 0, 0, 0, 0);
	track = !track ? edl->tracks->first : track->next;
	Mixer *mixer = 0;
	ZWindow *zwindow = get_mixer(mixer);
	while( track ) {
		track->play = track->record = 0;
		if( track->data_type == TRACK_VIDEO ) {
			sprintf(track->title, _("Mixer %d"), zwindow->idx);
		}
		mixer->mixer_ids.append(track->get_mixer_id());
		track = track->next;
	}
	if(  indexable->is_asset ) {
		char *path = indexable->path;
		char *tp = strrchr(path, '/');
		if( !tp ) tp = path; else ++tp;
		zwindow->set_title(tp);
	}
	else {
		char *title = ((EDL*)indexable)->local_session->clip_title;
		zwindow->set_title(title);
	}
	return zwindow;
}

void MWindow::create_mixers()
{
	if( !session->drag_assets->size() &&
	    !session->drag_clips->size() ) return;
	undo_before();

	select_zwindow(0);
	ArrayList<ZWindow *>new_mixers;

	for( int i=0; i<session->drag_assets->size(); ++i ) {
		Indexable *indexable = session->drag_assets->get(i);
		if( !indexable->have_video() ) continue;
		ZWindow *zwindow = create_mixer(indexable);
		new_mixers.append(zwindow);
	}
	for( int i=0; i<session->drag_clips->size(); ++i ) {
		Indexable *indexable = (Indexable*)session->drag_clips->get(i);
		if( !indexable->have_video() ) continue;
		ZWindow *zwindow = create_mixer(indexable);
		new_mixers.append(zwindow);
	}

	tile_mixers();
	for( int i=0; i<new_mixers.size(); ++i )
		new_mixers[i]->start();

	refresh_mixers();
	save_backup();
	undo_after(_("create mixers"), LOAD_ALL);
	restart_brender();
	gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 0);
	sync_parameters(CHANGE_ALL);
}

void MWindow::open_mixers()
{
	for( int i=0; i<edl->mixers.size(); ++i ) {
		Mixer *mixer = edl->mixers[i];
		ZWindow *zwindow = get_mixer(mixer);
		zwindow->set_title(mixer->title);
		zwindow->start();
	}
        refresh_mixers();
}

int MWindow::select_zwindow(ZWindow *zwindow)
{
	int ret = 0, n = zwindows.number_of(zwindow);
	if( session->selected_zwindow != n ) {
		session->selected_zwindow = n;
		for( int i=0; i<zwindows.size(); ++i ) {
			ZWindow *zwindow = zwindows[i];
			if( zwindow->idx < 0 ) continue;
			ZWindowGUI *zgui = zwindow->zgui;
			zgui->lock_window("MWindow::select_zwindow 0");
			zwindow->highlighted = i == n ? 1 : 0;
			if( zgui->draw_overlays() )
				zgui->canvas->get_canvas()->flash(1);
			zgui->unlock_window();
		}
		ret = 1;
		gui->lock_window("MWindow::select_window 1");
		gui->update_mixers(0, -1);
		gui->unlock_window();
	}
	return ret;
}

void MWindow::tile_mixers()
{
	int nz = 0;
	for( int i=0; i<zwindows.size(); ++i ) {
		ZWindow *zwindow = zwindows[i];
		if( zwindow->idx < 0 ) continue;
		++nz;
	}
	if( !nz ) return;
	int zn = ceil(sqrt(nz));
	int x1 = 1 + gui->get_x(), x2 = cwindow->gui->get_x();
	int y1 = 1, y2 = gui->get_y();
	int rw = gui->get_root_w(0), rh = gui->get_root_h(0);
	if( x1 < 0 ) x1 = 0;
	if( y1 < 0 ) y1 = 0;
	if( x2 > rw ) x2 = rw;
	if( y2 > rh ) y2 = rh;
	int dx = x2 - x1, dy = y2 - y1;
	int zw = dx / zn;
	int lt = BC_DisplayInfo::get_left_border();
	int top = BC_DisplayInfo::get_top_border();
	int bw = lt + BC_DisplayInfo::get_right_border();  // borders
	int bh = top + BC_DisplayInfo::get_bottom_border();
	int zx = 0, zy = 0;  // window origins
	int mw = 10+10, mh = 10+10; // canvas margins
	int rsz = 0, n = 0, dz = 0;
	int ow = edl->session->output_w, oh = edl->session->output_h;
	for( int i=0; i<zwindows.size(); ++i ) {
		ZWindow *zwindow = zwindows[i];
		if( zwindow->idx < 0 ) continue;
		int ww = zw - bw, hh = (ww - mw) * oh / ow + mh, zh = hh + bh;
		if( rsz < hh ) rsz = hh;
		int xx = zx + x1, yy = zy + y1;
		int mx = x2 - zw, my = y2 - zh;
		if( xx > mx ) xx = mx;
		if( yy > my ) yy = my;
		xx += lt + dz;  yy += top + dz;
		zwindow->reposition(xx,yy, ww,hh);
		if( zwindow->running() ) {
			ZWindowGUI *gui = (ZWindowGUI *)zwindow->get_gui();
			gui->lock_window("MWindow::tile_mixers");
			gui->BC_WindowBase::reposition_window(xx,yy, ww,hh);
			gui->unlock_window();
		}
		if( ++n >= zn ) {
			n = 0;  rsz += bh;
			if( (zy += rsz) > (dy - rsz) ) dz += 10;
			rsz = 0;
			zx = 0;
		}
		else
			zx += zw;
	}
}

void MWindow::init_cache()
{
	audio_cache = new CICache(preferences);
	video_cache = new CICache(preferences);
	frame_cache = new FrameCache;
	wave_cache = new WaveCache;
}

void MWindow::init_channeldb()
{
	channeldb_buz->load("channeldb_buz");
	channeldb_v4l2jpeg->load("channeldb_v4l2jpeg");
}

void MWindow::init_menus()
{
	char string[BCTEXTLEN];

	// Color Models
	BC_CModels::to_text(string, BC_RGB888);
	colormodels.append(new ColormodelItem(string, BC_RGB888));
	BC_CModels::to_text(string, BC_RGBA8888);
	colormodels.append(new ColormodelItem(string, BC_RGBA8888));
//	BC_CModels::to_text(string, BC_RGB161616);
//	colormodels.append(new ColormodelItem(string, BC_RGB161616));
//	BC_CModels::to_text(string, BC_RGBA16161616);
//	colormodels.append(new ColormodelItem(string, BC_RGBA16161616));
	BC_CModels::to_text(string, BC_RGB_FLOAT);
	colormodels.append(new ColormodelItem(string, BC_RGB_FLOAT));
	BC_CModels::to_text(string, BC_RGBA_FLOAT);
	colormodels.append(new ColormodelItem(string, BC_RGBA_FLOAT));
	BC_CModels::to_text(string, BC_YUV888);
	colormodels.append(new ColormodelItem(string, BC_YUV888));
	BC_CModels::to_text(string, BC_YUVA8888);
	colormodels.append(new ColormodelItem(string, BC_YUVA8888));
//	BC_CModels::to_text(string, BC_YUV161616);
//	colormodels.append(new ColormodelItem(string, BC_YUV161616));
//	BC_CModels::to_text(string, BC_YUVA16161616);
//	colormodels.append(new ColormodelItem(string, BC_YUVA16161616));

#define ILACEPROJECTMODELISTADD(x) ilacemode_to_text(string, x); \
                           interlace_project_modes.append(new InterlacemodeItem(string, x));

#define ILACEASSETMODELISTADD(x) ilacemode_to_text(string, x); \
                           interlace_asset_modes.append(new InterlacemodeItem(string, x));

#define ILACEFIXMETHODLISTADD(x) ilacefixmethod_to_text(string, x); \
                           interlace_asset_fixmethods.append(new InterlacefixmethodItem(string, x));

	// Interlacing Modes
	ILACEASSETMODELISTADD(ILACE_MODE_UNDETECTED); // Not included in the list for the project options.

	ILACEASSETMODELISTADD(ILACE_MODE_TOP_FIRST);
	ILACEPROJECTMODELISTADD(ILACE_MODE_TOP_FIRST);

	ILACEASSETMODELISTADD(ILACE_MODE_BOTTOM_FIRST);
	ILACEPROJECTMODELISTADD(ILACE_MODE_BOTTOM_FIRST);

	ILACEASSETMODELISTADD(ILACE_MODE_NOTINTERLACED);
	ILACEPROJECTMODELISTADD(ILACE_MODE_NOTINTERLACED);

	// Interlacing Fixing Methods
	ILACEFIXMETHODLISTADD(ILACE_FIXMETHOD_NONE);
	ILACEFIXMETHODLISTADD(ILACE_FIXMETHOD_UPONE);
	ILACEFIXMETHODLISTADD(ILACE_FIXMETHOD_DOWNONE);
}

void MWindow::init_indexes()
{
	mainindexes = new MainIndexes(this);
	mainindexes->start_loop();
}

void MWindow::init_gui()
{
	gui = new MWindowGUI(this);
	gui->create_objects();
	gui->load_defaults(defaults);
}

void MWindow::init_signals()
{
	sighandler = new SigHandler;
	sighandler->initialize("/tmp/cinelerra_%d.dmp");
ENABLE_BUFFER
}

void MWindow::init_render()
{
	render = new Render(this);
	create_bd = new CreateBD_Thread(this);
	create_dvd = new CreateDVD_Thread(this);
	batch_render = new BatchRenderThread(this);
}

void MWindow::init_exportedl()
{
	exportedl = new ExportEDL(this);
}

void MWindow::init_shuttle()
{
#ifdef HAVE_SHUTTLE
	int ret = Shuttle::probe();
	if( ret >= 0 ) {
		shuttle = new Shuttle(this);
		if( shuttle->read_config_file() > 0 ) {
			printf("shuttle: bad config file\n");
			delete shuttle;  shuttle = 0;
			return;
		}
		shuttle->start(ret);
	}
#endif
}

void MWindow::init_brender()
{
	if(preferences->use_brender && !brender)
	{
		brender_lock->lock("MWindow::init_brender 1");
		brender = new BRender(this);
		brender->initialize();
		session->brender_end = 0;
		brender_lock->unlock();
	}
	else
	if(!preferences->use_brender && brender)
	{
		brender_lock->lock("MWindow::init_brender 2");
		delete brender;
		brender = 0;
		session->brender_end = 0;
		brender_lock->unlock();
	}
	brender_active = 0;
	stop_brender();
}

void MWindow::restart_brender()
{
//printf("MWindow::restart_brender 1\n");
	if( !brender_active || !preferences->use_brender ) return;
	if( !brender ) return;
	brender->restart(edl);
}

void MWindow::stop_brender()
{
	if( !brender ) return;
// cannot be holding mwindow->gui display lock
	brender->stop();
}

int MWindow::brender_available(int position)
{
	int result = 0;
	brender_lock->lock("MWindow::brender_available 1");
	if(brender && brender_active)
	{
		if(brender->map_valid)
		{
			brender->map_lock->lock("MWindow::brender_available 2");
			if(position < brender->map_size &&
				position >= 0)
			{
//printf("MWindow::brender_available 1 %d %d\n", position, brender->map[position]);
				if(brender->map[position] == BRender::RENDERED)
					result = 1;
			}
			brender->map_lock->unlock();
		}
	}
	brender_lock->unlock();
	return result;
}

void MWindow::set_brender_active(int v, int update)
{
	if( !preferences->use_brender ) v = 0;
	brender_active = v;
	gui->mainmenu->brender_active->set_checked(v);
	if( v != 0 ) {
		edl->session->brender_start = edl->local_session->get_selectionstart(1);
		edl->session->brender_end = edl->local_session->get_selectionend(1);

		if(EQUIV(edl->session->brender_end, edl->session->brender_start)) {
			edl->session->brender_end = edl->tracks->total_video_length();
		}
		restart_brender();
	}
	else {
		edl->session->brender_start = edl->session->brender_end = 0;
		gui->unlock_window();
		stop_brender();
		gui->lock_window("MWindow::set_brender_active");
	}
	if( update ) {
		gui->update_timebar(0);
		gui->draw_overlays(1);
	}
}

int MWindow::has_commercials()
{
#ifdef HAVE_COMMERCIAL
	return theme->use_commercials;
#else
	return 0;
#endif
}

void MWindow::init_commercials()
{
#ifdef HAVE_COMMERCIAL
	if( !commercials ) {
		commercials = new Commercials(this);
		commercial_active = 0;
	}
	else
		commercials->add_user();
#endif
}

void MWindow::commit_commercial()
{
#ifdef HAVE_COMMERCIAL
	if( !commercial_active ) return;
	commercial_active = 0;
	if( !commercials ) return;
	commercials->commitDb();
#endif
}

void MWindow::undo_commercial()
{
#ifdef HAVE_COMMERCIAL
	if( !commercial_active ) return;
	commercial_active = 0;
	if( !commercials ) return;
	commercials->undoDb();
#endif
}

int MWindow::put_commercial()
{
	int result = 0;
#ifdef HAVE_COMMERCIAL
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	if( start >= end ) return 0;

	const char *errmsg = 0;
	int count = 0;
	Tracks *tracks = edl->tracks;
	//check it
	for(Track *track=tracks->first; track && !errmsg; track=track->next) {
		if( track->data_type != TRACK_VIDEO ) continue;
		if( !track->record ) continue;
		if( count > 0 ) { errmsg = _("multiple video tracks"); break; }
		++count;
		int64_t units_start = track->to_units(start,0);
		int64_t units_end = track->to_units(end,0);
		Edits *edits = track->edits;
		Edit *edit1 = edits->editof(units_start, PLAY_FORWARD, 0);
		Edit *edit2 = edits->editof(units_end, PLAY_FORWARD, 0);
		if(!edit1 && !edit2) continue;	// nothing selected
		if(!edit2) {			// edit2 beyond end of track
			edit2 = edits->last;
			units_end = edits->length();
		}
		if(edit1 != edit2) { errmsg = _("crosses edits"); break; }
		Indexable *indexable = edit1->get_source();
		if( !indexable->is_asset ) { errmsg = _("not asset"); break; }
	}
	//run it
	for(Track *track=tracks->first; track && !errmsg; track=track->next) {
		if( track->data_type != TRACK_VIDEO ) continue;
		if( !track->record ) continue;
		int64_t units_start = track->to_units(start,0);
		int64_t units_end = track->to_units(end,0);
		Edits *edits = track->edits;
		Edit *edit1 = edits->editof(units_start, PLAY_FORWARD, 0);
		Edit *edit2 = edits->editof(units_end, PLAY_FORWARD, 0);
		if(!edit1 && !edit2) continue;	// nothing selected
		if(!edit2) {			// edit2 beyond end of track
			edit2 = edits->last;
			units_end = edits->length();
		}
		Indexable *indexable = edit1->get_source();
		Asset *asset = (Asset *)indexable;
		File *file = video_cache->check_out(asset, edl);
		if( !file ) { errmsg = _("no file"); break; }
		int64_t edit_length = units_end - units_start;
		int64_t edit_start = units_start - edit1->startproject + edit1->startsource;
		result = commercials->put_clip(file, edit1->channel,
			track->from_units(edit_start), track->from_units(edit_length));
		video_cache->check_in(asset);
		if( result ) { errmsg = _("db failed"); break; }
	}
	if( errmsg ) {
		char string[BCTEXTLEN];
		sprintf(string, _("put_commercial: %s"), errmsg);
		MainError::show_error(string);
		undo_commercial();
		result = 1;
	}
#endif
	return result;
}

void MWindow::stop_playback(int wait)
{
	gui->stop_drawing();

	cwindow->stop_playback(wait);

	for(int i = 0; i < vwindows.size(); i++) {
		VWindow *vwindow = vwindows[i];
		if( !vwindow->is_running() ) continue;
		vwindow->stop_playback(wait);
	}
	for(int i = 0; i < zwindows.size(); i++) {
		ZWindow *zwindow = zwindows[i];
		if( zwindow->idx < 0 ) continue;
		zwindow->stop_playback(wait);
	}
}

void MWindow::stop_transport()
{
	gui->stop_transport(gui->get_window_lock() ? "MWindow::stop_transport" : 0);
}

void MWindow::undo_before(const char *description, void *creator)
{
	undo->update_undo_before(description, creator);
}

void MWindow::undo_after(const char *description, uint32_t load_flags, int changes_made)
{
	undo->update_undo_after(description, load_flags, changes_made);
}

void MWindow::beep(double freq, double secs, double gain)
{
	if( !proxy_beep ) proxy_beep = new ProxyBeep(this);
	proxy_beep->tone(freq, secs, gain);
}

int MWindow::load_filenames(ArrayList<char*> *filenames,
	int load_mode,
	int update_filename)
{
	ArrayList<EDL*> new_edls;
	ArrayList<Asset*> new_assets;
	ArrayList<File*> new_files;
	const int debug = 0;
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

//	save_defaults();
	gui->start_hourglass();

// Need to stop playback since tracking depends on the EDL not getting
// deleted.
	gui->unlock_window();
	stop_playback(1);
	gui->lock_window("MWindow::load_filenames 0");

if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
	undo_before();


if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

// Define new_edls and new_assets to load
	int result = 0, ftype = -1;

	for( int i=0; i<filenames->size(); ++i ) {
// Get type of file
		File *new_file = new File;
		Asset *new_asset = new Asset(filenames->get(i));
		EDL *new_edl = new EDL;
		char string[BCTEXTLEN];

		new_edl->create_objects();
		new_edl->copy_session(edl, -1);
		new_file->set_program(edl->session->program_no);

		sprintf(string, _("Loading %s"), new_asset->path);
		gui->show_message(string);
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

		ftype = new_file->open_file(preferences, new_asset, 1, 0);
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

		result = 1;
		switch( ftype ) {
// Convert media file to EDL
		case FILE_OK:
// Warn about odd image dimensions
			if( new_asset->video_data &&
			    ((new_asset->width % 2) || (new_asset->height % 2)) ) {
				char string[BCTEXTLEN];
				sprintf(string, _("%s's resolution is %dx%d.\nImages with odd dimensions may not decode properly."),
					new_asset->path, new_asset->width, new_asset->height);
				MainError::show_error(string);
			}

			if( new_asset->program >= 0 &&
			    edl->session->program_no != new_asset->program ) {
				char string[BCTEXTLEN];
				sprintf(string, _("%s's index was built for program number %d\n"
					"Playback preference is %d.\n  Using program %d."),
					new_asset->path, new_asset->program,
					edl->session->program_no, new_asset->program);
				MainError::show_error(string);
			}

			if( load_mode != LOADMODE_RESOURCESONLY ) {
				RecordLabels *labels = edl->session->label_cells ?
					new RecordLabels(new_file) : 0;
				asset_to_edl(new_edl, new_asset, labels);
				new_edls.append(new_edl);
				new_edl->add_user();
				delete labels;
			}
			else {
				new_assets.append(new_asset);
				new_asset->add_user();
			}

// Set filename to nothing for assets since save EDL would overwrite them.
			if( load_mode == LOADMODE_REPLACE ||
			    load_mode == LOADMODE_REPLACE_CONCATENATE ) {
				set_filename("");
// Reset timeline position
				for( int i=0; i<TOTAL_PANES; ++i ) {
					new_edl->local_session->view_start[i] = 0;
					new_edl->local_session->track_start[i] = 0;
				}
			}

			result = 0;
			break;

// File not found
		case FILE_NOT_FOUND:
			sprintf(string, _("Failed to open %s"), new_asset->path);
			gui->show_message(string, theme->message_error);
			gui->update_default_message();
			break;

// Unknown format
		case FILE_UNRECOGNIZED_CODEC: {
// Test index file
			{ IndexFile indexfile(this, new_asset);
			if( !(result = indexfile.open_index()) )
				indexfile.close_index(); }

// Test existing EDLs
			for( int j=0; result && j<new_edls.total; ++j ) {
				Asset *old_asset = new_edls[j]->assets->get_asset(new_asset->path);
				if( old_asset ) {
					new_asset->copy_from(old_asset,1);
					result = 0;
				}
			}
			if( result ) {
				Asset *old_asset = edl->assets->get_asset(new_asset->path);
				if( old_asset ) {
					new_asset->copy_from(old_asset,1);
					result = 0;
				}
			}

// Prompt user
			if( result ) {
				char string[BCTEXTLEN];
				FileSystem fs;
				fs.extract_name(string, new_asset->path);

				strcat(string, _("'s format couldn't be determined."));
				new_asset->audio_data = 1;
				new_asset->format = FILE_PCM;
				new_asset->channels = defaults->get("AUDIO_CHANNELS", 2);
				new_asset->sample_rate = defaults->get("SAMPLE_RATE", 44100);
				new_asset->bits = defaults->get("AUDIO_BITS", 16);
				new_asset->byte_order = defaults->get("BYTE_ORDER", 1);
				new_asset->signed_ = defaults->get("SIGNED_", 1);
				new_asset->header = defaults->get("HEADER", 0);

				FileFormat fwindow(this);
				fwindow.create_objects(new_asset, string);
				result = fwindow.run_window();

				defaults->update("AUDIO_CHANNELS", new_asset->channels);
				defaults->update("SAMPLE_RATE", new_asset->sample_rate);
				defaults->update("AUDIO_BITS", new_asset->bits);
				defaults->update("BYTE_ORDER", new_asset->byte_order);
				defaults->update("SIGNED_", new_asset->signed_);
				defaults->update("HEADER", new_asset->header);
				save_defaults();
			}

// Append to list
			if( !result ) {
// Recalculate length
				delete new_file;
				new_file = new File;
				result = new_file->open_file(preferences, new_asset, 1, 0);

				if( load_mode != LOADMODE_RESOURCESONLY ) {
					RecordLabels *labels = edl->session->label_cells ?
						new RecordLabels(new_file) : 0;
					asset_to_edl(new_edl, new_asset, labels);
					new_edls.append(new_edl);
					new_edl->add_user();
					delete labels;
				}
				else {
					new_assets.append(new_asset);
					new_asset->add_user();
				}
			}
			else {
				result = 1;
			}
			break; }

		case FILE_IS_XML: {
			FileXML xml_file;
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
			xml_file.read_from_file(filenames->get(i));
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
			const char *cin_version = 0;
			while( !xml_file.read_tag() ) {
				if( xml_file.tag.title_is("EDL") ) {
					cin_version = xml_file.tag.get_property("VERSION");
					break;
				}
			}
			xml_file.rewind();
			if( !cin_version ) {
				eprintf(_("XML file %s\n not from cinelerra."),filenames->get(i));
				char string[BCTEXTLEN];
				sprintf(string,_("Unknown %s"), filenames->get(i));
				gui->show_message(string);
				result = 1;
				break;
			}
			if( strcmp(cin_version, CINELERRA_VERSION) &&
			    strcmp(cin_version, "Unify") &&
			    strcmp(cin_version, "5.1") ) {
				char string[BCTEXTLEN];
				snprintf(string, sizeof(string),
					 _("Warning: XML from cinelerra version %s\n"
					"Session data may be incompatible."), cin_version);
				show_warning(&preferences->warn_version, string);
			}
			if( load_mode == LOADMODE_NESTED ) {
// Load temporary EDL for nesting.
				EDL *nested_edl = new EDL;
				nested_edl->create_objects();
				nested_edl->load_xml(&xml_file, LOAD_ALL);
				int groups = nested_edl->regroup(session->group_number);
				session->group_number += groups;
//printf("MWindow::load_filenames %p %s\n", nested_edl, nested_edl->project_path);
				new_edl->create_nested(nested_edl);
				new_edl->set_path(filenames->get(i));
				nested_edl->Garbage::remove_user();
			}
			else {
// Load EDL for pasting
				new_edl->load_xml(&xml_file, LOAD_ALL);
				int groups = new_edl->regroup(session->group_number);
				session->group_number += groups;
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
				test_plugins(new_edl, filenames->get(i));
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

				if( load_mode == LOADMODE_REPLACE ||
				    load_mode == LOADMODE_REPLACE_CONCATENATE ) {
					strcpy(session->filename, filenames->get(i));
					strcpy(new_edl->local_session->clip_title,
						filenames->get(i));
					if(update_filename)
						set_filename(new_edl->local_session->clip_title);
				}
				else if( load_mode == LOADMODE_RESOURCESONLY ) {
					strcpy(new_edl->local_session->clip_title,
						filenames->get(i));
					struct stat st;
					time_t t = !stat(filenames->get(i),&st) ?
						st.st_mtime : time(&t);
					ctime_r(&t, new_edl->local_session->clip_notes);
				}
			}

			new_edls.append(new_edl);
			new_edl->add_user();
			result = 0;
			break; }
		}

		new_edl->Garbage::remove_user();
		new_asset->Garbage::remove_user();

// Store for testing index
		new_files.append(new_file);
	}

if(debug) printf("MWindow::load_filenames %d\n", __LINE__);


	if(!result) {
		gui->reset_default_message();
		gui->default_message();
	}


if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

// Paste them.
// Don't back up here.
	if(new_edls.size())
	{
// For pasting, clear the active region
		if(load_mode == LOADMODE_PASTE ||
			load_mode == LOADMODE_NESTED)
		{
			double start = edl->local_session->get_selectionstart();
			double end = edl->local_session->get_selectionend();
			if(!EQUIV(start, end))
				edl->clear(start,
					end,
					edl->session->labels_follow_edits,
					edl->session->plugins_follow_edits,
					edl->session->autos_follow_edits);
		}

		paste_edls(&new_edls, load_mode, 0, -1,
			edl->session->labels_follow_edits,
			edl->session->plugins_follow_edits,
			edl->session->autos_follow_edits,
			0); // overwrite
	}




if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

// Add new assets to EDL and schedule assets for index building.
	int got_indexes = 0;
	for( int i=0; i<new_edls.size(); ++i ) {
		EDL *new_edl = new_edls[i];
		for( int j=0; j<new_edl->nested_edls.size(); ++j ) {
			mainindexes->add_next_asset(0, new_edl->nested_edls[j]);
			edl->nested_edls.update_index(new_edl->nested_edls[j]);
			got_indexes = 1;
		}

	}

if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
	for( int i=0; i<new_assets.size(); ++i ) {
		Asset *new_asset = new_assets[i];

		File *new_file = 0;
		int got_it = 0;
		for( int j=0; j<new_files.size(); ++j ) {
			new_file = new_files[j];
			if( !strcmp(new_file->asset->path, new_asset->path) ) {
				got_it = 1;
				break;
			}
		}

		mainindexes->add_next_asset(got_it ? new_file : 0, new_asset);
		edl->assets->update(new_asset);
		got_indexes = 1;
	}
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

// Start examining next batch of index files
	if(got_indexes) mainindexes->start_build();
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

// Open plugin GUIs
	Track *track = edl->tracks->first;
	while(track)
	{
		for(int j = 0; j < track->plugin_set.size(); j++)
		{
			PluginSet *plugins = track->plugin_set[j];
			Plugin *plugin = plugins->get_first_plugin();

			while(plugin)
			{
				if(load_mode == LOADMODE_REPLACE ||
					load_mode == LOADMODE_REPLACE_CONCATENATE)
				{
					if(plugin->plugin_type == PLUGIN_STANDALONE &&
						plugin->show)
					{
						show_plugin(plugin);
					}
				}

				plugin = (Plugin*)plugin->next;
			}
		}

		track = track->next;
	}

	// if just opening one new resource in replace mode
	if( ftype != FILE_IS_XML &&
	    ( load_mode == LOADMODE_REPLACE ||
	      load_mode == LOADMODE_REPLACE_CONCATENATE ) ) {
		select_asset(0, 0);
		edl->session->proxy_scale = 1;
		edl->session->proxy_disabled_scale = 1;
		edl->session->proxy_use_scaler = 0;
		edl->session->proxy_auto_scale = 0;
		edl->session->proxy_beep = 0;
		edl->local_session->preview_start = 0;
		edl->local_session->preview_end = -1;
		edl->local_session->loop_playback = 0;
		edl->local_session->set_selectionstart(0);
		edl->local_session->set_selectionend(0);
		set_brender_active(0, 0);
		fit_selection();
		goto_start();
	}

	if( ( edl->session->proxy_auto_scale && edl->session->proxy_scale != 1 ) &&
	    ( load_mode != LOADMODE_REPLACE && load_mode != LOADMODE_REPLACE_CONCATENATE ) ) {
		ArrayList<Indexable *> orig_idxbls;
		for( int i=0; i<new_assets.size(); ++i )
			orig_idxbls.append(new_assets.get(i));
		for( int i=0; i<new_edls.size(); ++i ) {
			EDL *new_edl = new_edls[i];
			for( Track *track=new_edl->tracks->first; track; track=track->next ) {
				if( track->data_type != TRACK_VIDEO ) continue;
				for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
					Indexable *idxbl = (Indexable *)edit->asset;
					if( !idxbl ) continue;
					if( !idxbl->have_video() ) continue;
					if( edit->channel != 0 ) continue; // first layer only
					orig_idxbls.append(edit->asset);
				}
			}
		}
		gui->unlock_window(); // to update progress bar
		int ret = render_proxy(orig_idxbls);
		gui->lock_window("MWindow::load_filenames");
		if( ret >= 0 && edl->session->proxy_beep ) {
			if( ret > 0 )
				beep(2000., 1.5, 0.5);
			else
				beep(4000., 0.25, 0.5);
		}
	}

// need to update undo before project, since mwindow is unlocked & a new load
// can begin here.  Should really prevent loading until we're done.
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
	undo_after(_("load"), LOAD_ALL);

	for(int i = 0; i < new_edls.size(); i++)
	{
		new_edls[i]->remove_user();
	}
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

	new_edls.remove_all();

	for(int i = 0; i < new_assets.size(); i++)
	{
		new_assets[i]->Garbage::remove_user();
	}

	new_assets.remove_all();
	new_files.remove_all_objects();


if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
	if(load_mode == LOADMODE_REPLACE ||
		load_mode == LOADMODE_REPLACE_CONCATENATE)
	{
		session->changes_made = 0;
	}
	else
	{
		session->changes_made = 1;
	}

	gui->stop_hourglass();


if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
	update_project(load_mode);

if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

	return 0;
}

int MWindow::render_proxy(ArrayList<Indexable *> &new_idxbls)
{
	Asset *format_asset = new Asset;
	format_asset->format = FILE_FFMPEG;
	format_asset->load_defaults(defaults, "PROXY_", 1, 1, 0, 0, 0);
	ProxyRender proxy_render(this, format_asset);
	int new_scale = edl->session->proxy_scale;
	int use_scaler = edl->session->proxy_use_scaler;

	for( int i=0; i<new_idxbls.size(); ++i ) {
		Indexable *orig = new_idxbls.get(i);
		Asset *proxy = proxy_render.add_original(orig, new_scale);
		if( !proxy ) continue;
		FileSystem fs;
		int exists = fs.get_size(proxy->path) > 0 ? 1 : 0;
		int got_it = exists && // if proxy exists, and is newer than orig
		    fs.get_date(proxy->path) > fs.get_date(orig->path) ? 1 : 0;
		if( got_it ) continue;
		proxy_render.add_needed(orig, proxy);
	}

// render needed proxies
	int result = proxy_render.create_needed_proxies(new_scale);
	if( !result ) {
		add_proxy(use_scaler,
			&proxy_render.orig_idxbls, &proxy_render.orig_proxies);
	}
	format_asset->remove_user();
	return !result ? proxy_render.needed_proxies.size() : -1;
}

int MWindow::enable_proxy()
{
	int ret = 0;
	if( edl->session->proxy_scale == 1 &&
	    edl->session->proxy_disabled_scale != 1 ) {
		int new_scale = edl->session->proxy_disabled_scale;
		int new_use_scaler = edl->session->proxy_use_scaler;
		edl->session->proxy_disabled_scale = 1;
		Asset *asset = new Asset;
		asset->format = FILE_FFMPEG;
		asset->load_defaults(defaults, "PROXY_", 1, 1, 0, 0, 0);
		ret = to_proxy(asset, new_scale, new_use_scaler);
		asset->remove_user();
		if( ret > 0 )
			beep(2000., 1.5, 0.5);
	}
	return 1;
}

int MWindow::disable_proxy()
{
	if( edl->session->proxy_scale != 1 &&
	    edl->session->proxy_disabled_scale == 1 ) {
		int new_scale = 1;
		int new_use_scaler = edl->session->proxy_use_scaler;
		edl->session->proxy_disabled_scale = edl->session->proxy_scale;
		Asset *asset = new Asset;
		asset->format = FILE_FFMPEG;
		asset->load_defaults(defaults, "PROXY_", 1, 1, 0, 0, 0);
		to_proxy(asset, new_scale, new_use_scaler);
		asset->remove_user();
	}
	return 1;
}

int MWindow::to_proxy(Asset *asset, int new_scale, int new_use_scaler)
{
	ArrayList<Indexable*> orig_idxbls;
	ArrayList<Indexable*> proxy_assets;

	edl->Garbage::add_user();
	save_backup();
	undo_before(_("proxy"), this);
	ProxyRender proxy_render(this, asset);

// revert project to original size from current size
// remove all session proxy assets at the at the current proxy_scale
	int proxy_scale = edl->session->proxy_scale;

	if( proxy_scale > 1 ) {
		Asset *orig_asset = edl->assets->first;
		for( ; orig_asset; orig_asset=orig_asset->next ) {
			char new_path[BCTEXTLEN];
			proxy_render.to_proxy_path(new_path, orig_asset, proxy_scale);
// test if proxy asset was already added to proxy_assets
			int got_it = 0;
			for( int i = 0; !got_it && i<proxy_assets.size(); ++i )
				got_it = !strcmp(proxy_assets[i]->path, new_path);
			if( got_it ) continue;
			Asset *proxy_asset = edl->assets->get_asset(new_path);
			if( !proxy_asset ) continue;
// add pointer to existing EDL asset if it exists
// EDL won't delete it unless it's the same pointer.
			proxy_assets.append(proxy_asset);
			proxy_asset->add_user();
			orig_idxbls.append(orig_asset);
			orig_asset->add_user();
		}
		for( int i=0,n=edl->nested_edls.size(); i<n; ++i ) {
			EDL *orig_nested = edl->nested_edls[i];
			char new_path[BCTEXTLEN];
			if( !ProxyRender::from_proxy_path(new_path, orig_nested, proxy_scale) )
				continue;
			proxy_render.to_proxy_path(new_path, orig_nested, proxy_scale);
// test if proxy asset was already added to proxy_assets
			int got_it = 0;
			for( int i = 0; !got_it && i<proxy_assets.size(); ++i )
				got_it = !strcmp(proxy_assets[i]->path, new_path);
			if( got_it ) continue;
			Asset *proxy_nested = edl->assets->get_asset(new_path);
			if( !proxy_nested ) continue;
// add pointer to existing EDL asset if it exists
// EDL won't delete it unless it's the same pointer.
			proxy_assets.append(proxy_nested);
			proxy_nested->add_user();
			orig_idxbls.append(orig_nested);
			orig_nested->add_user();
		}

// convert from the proxy assets to the original assets
		edl->set_proxy(1, 0, &proxy_assets, &orig_idxbls);

// remove the references
		for( int i=0; i<proxy_assets.size(); ++i ) {
			Asset *proxy = (Asset *) proxy_assets[i];
			proxy->width = proxy->actual_width;
			proxy->height = proxy->actual_height;
			proxy->remove_user();
			edl->assets->remove_pointer(proxy);
			proxy->remove_user();
		}
		proxy_assets.remove_all();
		for( int i = 0; i < orig_idxbls.size(); i++ )
			orig_idxbls[i]->remove_user();
		orig_idxbls.remove_all();
	}

	ArrayList<char *> confirm_paths;    // test for new files
	confirm_paths.set_array_delete();

// convert to new size if not original size
	if( new_scale != 1 ) {
		FileSystem fs;
		Asset *orig = edl->assets->first;
		for( ; orig; orig=orig->next ) {
			Asset *proxy = proxy_render.add_original(orig, new_scale);
			if( !proxy ) continue;
			int exists = fs.get_size(proxy->path) > 0 ? 1 : 0;
			int got_it = exists && // if proxy exists, and is newer than orig
			    fs.get_date(proxy->path) > fs.get_date(orig->path) ? 1 : 0;
			if( !got_it ) {
				if( exists ) // prompt user to overwrite
					confirm_paths.append(cstrdup(proxy->path));
				proxy_render.add_needed(orig, proxy);
			}
		}
		for( int i=0,n=edl->nested_edls.size(); i<n; ++i ) {
			EDL *orig_nested = edl->nested_edls[i];
			Asset *proxy = proxy_render.add_original(orig_nested, new_scale);
			if( !proxy ) continue;
			int exists = fs.get_size(proxy->path) > 0 ? 1 : 0;
			int got_it = exists && // if proxy exists, and is newer than orig_nested
			    fs.get_date(proxy->path) > fs.get_date(orig_nested->path) ? 1 : 0;
			if( !got_it ) {
				if( exists ) // prompt user to overwrite
					confirm_paths.append(cstrdup(proxy->path));
				proxy_render.add_needed(orig_nested, proxy);
			}
		}
	}

	int result = 0;
// test for existing files
	if( confirm_paths.size() ) {
		result = ConfirmSave::test_files(this, &confirm_paths);
		confirm_paths.remove_all_objects();
	}

	if( !result )
		result = proxy_render.create_needed_proxies(new_scale);

	if( !result ) // resize project
		edl->set_proxy(new_scale, new_use_scaler,
			&proxy_render.orig_idxbls, &proxy_render.orig_proxies);

	undo_after(_("proxy"), LOAD_ALL);
	edl->Garbage::remove_user();
	restart_brender();

	gui->lock_window("MWindow::to_proxy");
	update_project(LOADMODE_REPLACE);
	gui->unlock_window();

	return !result ? proxy_render.needed_proxies.size() : -1;
}

void MWindow::test_plugins(EDL *new_edl, char *path)
{
	char string[BCTEXTLEN];

// Do a check whether plugins exist
	for( Track *track=new_edl->tracks->first; track; track=track->next ) {
		for( int k=0; k<track->plugin_set.total; ++k ) {
			PluginSet *plugin_set = track->plugin_set[k];
			for( Plugin *plugin = (Plugin*)plugin_set->first;
					plugin; plugin = (Plugin*)plugin->next ) {
				if( plugin->plugin_type != PLUGIN_STANDALONE ) continue;
// ok we need to find it in plugindb
				PluginServer *server =
					scan_plugindb(plugin->title, track->data_type);
				if( !server || server->transition ) {
					sprintf(string,
	_("The %s '%s' in file '%s' is not part of your installation of Cinelerra.\n"
	  "The project won't be rendered as it was meant and Cinelerra might crash.\n"),
						"effect", _(plugin->title), path);
					MainError::show_error(string);
				}
			}
		}

		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			if( !edit->transition ) continue;
// ok we need to find transition in plugindb
			PluginServer *server =
				scan_plugindb(edit->transition->title, track->data_type);
			if( !server || !server->transition ) {
				sprintf(string,
	_("The %s '%s' in file '%s' is not part of your installation of Cinelerra.\n"
	  "The project won't be rendered as it was meant and Cinelerra might crash.\n"),
					"transition", _(edit->transition->title), path);
				MainError::show_error(string);
			}
		}
	}
}


void MWindow::init_shm(const char *pfn, int64_t min)
{
	int64_t result = 0;
// Fix shared memory
	FILE *fd = fopen(pfn, "r");
	if( fd ) {
		fscanf(fd, "%jd", &result);
		fclose(fd);
		if( result >= min ) return;
	}

	fd = fopen(pfn, "w");
	if( !fd ) return;
	fprintf(fd, "0x%jx", min);
	fclose(fd);

	fd = fopen(pfn, "r");
	if( !fd ) {
		eprintf(_("MWindow::init_shm: couldn't open %s for reading.\n"), pfn);
		return;
	}

	fscanf(fd, "%jd", &result);
	fclose(fd);
	if( result < min ) {
		eprintf(_("MWindow::init_shm: %s is %p.\n"
			"you probably need to be root, or:\n"
			"as root, run: echo 0x%jx > %s\n"
			"before trying to start cinelerra.\n"
			"It should be at least 0x%jx for Cinelerra.\n"),
			pfn, (void *)result, min, pfn, min);
	}
}

void MWindow::create_objects(int want_gui,
	int want_new,
	char *config_path)
{
	const int debug = 0;
	if(debug) PRINT_TRACE

// For some reason, init_signals must come after show_splash or the signals won't
// get trapped.
	init_signals();
	if(debug) PRINT_TRACE
	init_3d();

	if(debug) PRINT_TRACE
	show_splash();

	if(debug) PRINT_TRACE
	default_standard = default_std();
	init_defaults(defaults, config_path);
	check_language();
	init_preferences();
	if(splash_window)
		splash_window->update_status(_("Initializing Plugins"));
	init_plugins(this, preferences);
	if(debug) PRINT_TRACE
	init_ladspa_plugins(this, preferences);
	if(debug) PRINT_TRACE
	init_plugin_tips(*plugindb, cin_lang);
	if(splash_window)
		splash_window->update_status(_("Initializing GUI"));
	if(debug) PRINT_TRACE
	init_theme();

	if(debug) PRINT_TRACE
	init_error();

	if(splash_window)
		splash_window->update_status(_("Initializing Fonts"));
	char string[BCTEXTLEN];
	strcpy(string, preferences->plugin_dir);
	strcat(string, "/" FONT_SEARCHPATH);
	BC_Resources::init_fontconfig(string);
	if(debug) PRINT_TRACE

// Default project created here
	init_edl();
	if(debug) PRINT_TRACE

	init_cache();
	if(debug) PRINT_TRACE

	Timer timer;

	init_awindow();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	init_compositor();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

//printf("MWindow::create_objects %d session->show_vwindow=%d\n", __LINE__, session->show_vwindow);
	if(session->show_vwindow)
		get_viewer(1, DEFAULT_VWINDOW);
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	init_gui();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	init_levelwindow();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	init_indexes();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	init_channeldb();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	init_gwindow();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	init_render();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	init_shuttle();
	init_brender();
	init_exportedl();
	init_commercials();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
	mainprogress = new MainProgress(this, gui);
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
	undo = new MainUndo(this);

	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	plugin_guis = new ArrayList<PluginServer*>;
	dead_plugins = new ArrayList<PluginServer*>;
	keyframe_threads = new ArrayList<KeyFrameThread*>;

	if(debug) printf("MWindow::create_objects %d vwindows=%d show_vwindow=%d\n",
		__LINE__,
		vwindows.size(),
		session->show_vwindow);

// Show all vwindows
// 	if(session->show_vwindow) {
// 		for(int j = 0; j < vwindows.size(); j++) {
// 			VWindow *vwindow = vwindows[j];
// 			if( !vwindow->is_running() ) continue;
// 			if(debug) printf("MWindow::create_objects %d vwindow=%p\n",
// 				__LINE__,
// 				vwindow);
// 			if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
// 			vwindow->gui->lock_window("MWindow::create_objects 1");
// 			if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
// 			vwindow->gui->show_window();
// 			if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
// 			vwindow->gui->unlock_window();
// 		}
// 	}


	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	if(session->show_cwindow)
	{
		cwindow->gui->lock_window("MWindow::create_objects 1");
		cwindow->gui->show_window();
		cwindow->gui->unlock_window();
	}

	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
	if(session->show_awindow)
	{
		awindow->gui->lock_window("MWindow::create_objects 1");
		awindow->gui->show_window();
		awindow->gui->unlock_window();
	}

	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
	if(session->show_lwindow)
	{
		lwindow->gui->lock_window("MWindow::create_objects 1");
		lwindow->gui->show_window();
		lwindow->gui->unlock_window();
	}

	if(debug) printf("MWindow::create_objects %d total_time=%d gwindow=%p\n",
		__LINE__,
		(int)timer.get_difference(),
		gwindow->gui);
	if(session->show_gwindow)
	{
		gwindow->gui->lock_window("MWindow::create_objects 1");
		gwindow->gui->show_window();
		gwindow->gui->unlock_window();
	}

	if(debug) PRINT_TRACE

	gui->lock_window("MWindow::create_objects 1");
	gui->mainmenu->load_defaults(defaults);
	gui->mainmenu->update_toggles(0);
	gui->update_patchbay();
	gui->draw_canvas(0, 0);
	gui->draw_cursor(1);
	gui->show_window();
	gui->raise_window();
	gui->unlock_window();

	if(debug) PRINT_TRACE

	if(preferences->use_tipwindow)
		init_tipwindow();
	if(debug) PRINT_TRACE

	gui->add_keyboard_listener(
		(int (BC_WindowBase::*)(BC_WindowBase *))
		&MWindowGUI::keyboard_listener);

	hide_splash();
	init_shm("/proc/sys/kernel/shmmax", 0x7fffffff);
	init_shm("/proc/sys/kernel/shmmni", 0x4000);
	if(debug) PRINT_TRACE

	BC_WindowBase::get_resources()->vframe_shm = 1;
}


void MWindow::show_splash()
{
#include "data/heroine_logo9_png.h"
	VFrame *frame = new VFramePng(heroine_logo9_png);
	BC_DisplayInfo dpyi;
	int rw = dpyi.get_root_w(), rh = dpyi.get_root_h();
	int rr = (rw < rh ? rw : rh) / 4;
	splash_window = new SplashGUI(frame, rr, rr);
	splash_window->create_objects();
}

void MWindow::hide_splash()
{
	if(splash_window)
		delete splash_window;
	splash_window = 0;
}


void MWindow::start()
{
ENABLE_BUFFER
//PRINT_TRACE
//	vwindows[DEFAULT_VWINDOW]->start();
	awindow->start();
//PRINT_TRACE
	cwindow->start();
//PRINT_TRACE
	lwindow->start();
//PRINT_TRACE
	gwindow->start();
//PRINT_TRACE
//	Thread::start();
//PRINT_TRACE
	playback_3d->start();
//PRINT_TRACE
}

void MWindow::run()
{
	run_lock->lock("MWindow::run");
	gui->run_window();
	stop_playback(1);

	brender_lock->lock("MWindow::run 1");
	delete brender;         brender = 0;
	brender_lock->unlock();

	interrupt_indexes();
	clean_indexes();
	save_defaults();
	run_lock->unlock();
}

void MWindow::show_vwindow()
{
	int total_running = 0;
	session->show_vwindow = 1;

//printf("MWindow::show_vwindow %d %d\n", __LINE__, vwindows.size());
// Raise all windows which are visible
	for(int j = 0; j < vwindows.size(); j++) {
		VWindow *vwindow = vwindows[j];
		if( !vwindow->is_running() ) continue;
		vwindow->gui->lock_window("MWindow::show_vwindow");
		vwindow->gui->show_window(0);
		vwindow->gui->raise_window();
		vwindow->gui->flush();
		vwindow->gui->unlock_window();
		total_running++;
	}

// If no windows visible
	if(!total_running)
	{
		get_viewer(1, DEFAULT_VWINDOW);
	}

	gui->mainmenu->show_vwindow->set_checked(1);
}

void MWindow::show_awindow()
{
	session->show_awindow = 1;
	awindow->gui->lock_window("MWindow::show_awindow");
	awindow->gui->show_window();
	awindow->gui->raise_window();
	awindow->gui->flush();
	awindow->gui->unlock_window();
	gui->mainmenu->show_awindow->set_checked(1);
}

char *MWindow::get_cwindow_display()
{
	char *x11_host = screens < 2 || session->window_config == 0 ?
		session->a_x11_host : session->b_x11_host;
	return *x11_host ? x11_host : 0;
}

void MWindow::show_cwindow()
{
	session->show_cwindow = 1;
	cwindow->show_window();
	gui->mainmenu->show_cwindow->set_checked(1);
}

void MWindow::show_gwindow()
{
	session->show_gwindow = 1;

	gwindow->gui->lock_window("MWindow::show_gwindow");
	gwindow->gui->show_window();
	gwindow->gui->raise_window();
	gwindow->gui->flush();
	gwindow->gui->unlock_window();

	gui->mainmenu->show_gwindow->set_checked(1);
}
void MWindow::hide_gwindow()
{
	session->show_gwindow = 0;

	gwindow->gui->lock_window("MWindow::show_gwindow");
	gwindow->gui->hide_window();
	gwindow->gui->unlock_window();
}

void MWindow::show_lwindow()
{
	session->show_lwindow = 1;
	lwindow->gui->lock_window("MWindow::show_lwindow");
	lwindow->gui->show_window();
	lwindow->gui->raise_window();
	lwindow->gui->flush();
	lwindow->gui->unlock_window();
	gui->mainmenu->show_lwindow->set_checked(1);
}

void MWindow::restore_windows()
{
	if( !session->show_vwindow ) {
		for( int i=0, n=vwindows.size(); i<n; ++i ) {
			VWindow *vwindow = vwindows[i];
			if( !vwindow || !vwindow->is_running() ) continue;
			vwindow->gui->lock_window("MWindow::restore_windows");
			vwindow->gui->close(1);
			vwindow->gui->unlock_window();
		}
	}
	else
		show_vwindow();

	if( !session->show_awindow && !awindow->gui->is_hidden() ) {
		awindow->gui->lock_window("MWindow::restore_windows");
		awindow->gui->close_event();
		awindow->gui->unlock_window();
	}
	else if( session->show_awindow && awindow->gui->is_hidden() )
		show_awindow();

	if( !session->show_cwindow && !cwindow->gui->is_hidden() ) {
		cwindow->gui->lock_window("MWindow::restore_windows");
		cwindow->hide_window();
		cwindow->gui->unlock_window();
	}
	else if( session->show_cwindow && cwindow->gui->is_hidden() )
		cwindow->show_window();

	if( !session->show_gwindow && !gwindow->gui->is_hidden() ) {
		gwindow->gui->lock_window("MWindow::restore_windows");
		gwindow->gui->close_event();
		gwindow->gui->unlock_window();
	}
	else if( session->show_gwindow && gwindow->gui->is_hidden() )
		show_gwindow();

	if( !session->show_lwindow && !lwindow->gui->is_hidden() ) {
		lwindow->gui->lock_window("MWindow::restore_windows");
		lwindow->gui->close_event();
		lwindow->gui->unlock_window();
	}
	else if( session->show_lwindow && lwindow->gui->is_hidden() )
		show_lwindow();

	gui->focus();
}

void MWindow::save_layout(int no)
{
	char layout_path[BCTEXTLEN];
	snprintf(layout_path, sizeof(layout_path), "%s/" LAYOUT_FILE,
		File::get_config_path(), no);
	session->save_file(layout_path);
}

void MWindow::load_layout(int no)
{
	char layout_path[BCTEXTLEN];
	snprintf(layout_path, sizeof(layout_path), "%s/" LAYOUT_FILE,
		File::get_config_path(), no);
	session->load_file(layout_path);
	restore_windows();
	gui->default_positions();
	save_defaults();
}

int MWindow::tile_windows(int window_config)
{
	int need_reload = 0;
	int play_config = window_config==0 ? 0 : 1;
	edl->session->playback_config->load_defaults(defaults, play_config);
	session->default_window_positions(window_config);
	if( screens == 1 ) {
		gui->default_positions();
		sync_parameters(CHANGE_ALL);
	}
	else
		need_reload = 1;
	return need_reload;
}

void MWindow::toggle_loop_playback()
{
	edl->local_session->loop_playback = !edl->local_session->loop_playback;
	set_loop_boundaries();
	save_backup();

	gui->draw_overlays(1);
	sync_parameters(CHANGE_PARAMS);
}

void MWindow::set_screens(int value)
{
	screens = value;
}

void MWindow::set_auto_keyframes(int value)
{
	edl->session->auto_keyframes = value;
	gui->mbuttons->edit_panel->keyframe->update(value);
	gui->flush();
	cwindow->gui->lock_window("MWindow::set_auto_keyframes");
	cwindow->gui->edit_panel->keyframe->update(value);
	cwindow->gui->flush();
	cwindow->gui->unlock_window();
}

void MWindow::set_auto_visibility(Autos *autos, int value)
{
	if( autos->type == Autos::AUTOMATION_TYPE_PLUGIN )
		edl->session->auto_conf->plugins = value;
	else if( autos->autoidx >= 0 )
		edl->session->auto_conf->autos[autos->autoidx] = value;
	else
		return;

	gui->update(0, NORMAL_DRAW, 0, 0, 0, 0, 0);
	gui->mainmenu->update_toggles(1);
	gui->unlock_window();
	gwindow->gui->update_toggles(1);
	gui->lock_window("MWindow::set_auto_visibility");
}

void MWindow::set_keyframe_type(int mode)
{
	gui->lock_window("MWindow::set_keyframe_type");
	edl->local_session->floatauto_type = mode;
	gui->mainmenu->update_toggles(0);
	gui->unlock_window();
}

int MWindow::set_editing_mode(int new_editing_mode)
{
	edl->session->editing_mode = new_editing_mode;
	gui->mbuttons->edit_panel->editing_mode = edl->session->editing_mode;
	gui->mbuttons->edit_panel->update();
	gui->set_editing_mode(1);

	cwindow->gui->lock_window("MWindow::set_editing_mode");
	cwindow->gui->edit_panel->update();
	cwindow->gui->edit_panel->editing_mode = edl->session->editing_mode;
	cwindow->gui->unlock_window();
	return 0;
}

void MWindow::toggle_editing_mode()
{
	int mode = edl->session->editing_mode;
	if( mode == EDITING_ARROW )
		set_editing_mode(EDITING_IBEAM);
	else
		set_editing_mode(EDITING_ARROW);
}

void MWindow::toggle_camera_xyz()
{
	gwindow->gui->lock_window("MWindow::toggle_camera_xyz");
	gwindow->gui->toggle_camera_xyz();
	gwindow->gui->unlock_window();
}

void MWindow::toggle_projector_xyz()
{
	gwindow->gui->lock_window("MWindow::toggle_projector_xyz");
	gwindow->gui->toggle_projector_xyz();
	gwindow->gui->unlock_window();
}

void MWindow::set_labels_follow_edits(int value)
{
	edl->session->labels_follow_edits = value;
	gui->mbuttons->edit_panel->locklabels->update(value);
	gui->mainmenu->labels_follow_edits->set_checked(value);
	gui->flush();
}

void MWindow::sync_parameters(int change_type)
{
	if( in_destructor ) return;

// Sync engines which are playing back
	if( cwindow->playback_engine->is_playing_back ) {
		if( change_type == CHANGE_PARAMS ) {
// TODO: block keyframes until synchronization is done
			cwindow->playback_engine->sync_parameters(edl);
		}
		else {
// Stop and restart
			int command = cwindow->playback_engine->command->command;
// Waiting for tracking to finish would make the restart position more
// accurate but it can't lock the window to stop tracking for some reason.
// Not waiting for tracking gives a faster response but restart position is
// only as accurate as the last tracking update.
			cwindow->playback_engine->transport_stop(0);
			cwindow->playback_engine->next_command->realtime = 1;
			cwindow->playback_engine->transport_command(command, change_type, edl, 0);
		}
	}
	else {
		cwindow->refresh_frame(change_type);
	}
}

void MWindow::age_caches()
{
// printf("MWindow::age_caches %d %lld %lld %lld %lld\n",
// __LINE__,
// preferences->cache_size,
// audio_cache->get_memory_usage(1),
// video_cache->get_memory_usage(1),
// frame_cache->get_memory_usage(),
// wave_cache->get_memory_usage(),
// memory_usage);
	frame_cache->age_cache(512);
	wave_cache->age_cache(8192);

	int64_t memory_usage;
	int64_t prev_memory_usage = 0;
	int result = 0;
	int64_t video_size = (int64_t)(preferences->cache_size * 0.80);
	while( !result && prev_memory_usage !=
		(memory_usage=video_cache->get_memory_usage(1)) &&
		memory_usage > video_size ) {
		video_cache->delete_oldest();
		prev_memory_usage = memory_usage;
	}

	prev_memory_usage = 0;
	result = 0;
	int64_t audio_size = (int64_t)(preferences->cache_size * 0.20);
	while( !result && prev_memory_usage !=
		(memory_usage=audio_cache->get_memory_usage(1)) &&
		memory_usage > audio_size ) {
		audio_cache->delete_oldest();
		prev_memory_usage = memory_usage;
	}
}

void MWindow::reset_android_remote()
{
	gui->use_android_remote(preferences->android_remote);
}


void MWindow::show_keyframe_gui(Plugin *plugin)
{
	keyframe_gui_lock->lock("MWindow::show_keyframe_gui");
// Find existing thread
	for(int i = 0; i < keyframe_threads->size(); i++)
	{
		if(keyframe_threads->get(i)->plugin == plugin)
		{
			keyframe_threads->get(i)->start_window(plugin, 0);
			keyframe_gui_lock->unlock();
			return;
		}
	}

// Find unused thread
	for(int i = 0; i < keyframe_threads->size(); i++)
	{
		if(!keyframe_threads->get(i)->plugin)
		{
			keyframe_threads->get(i)->start_window(plugin, 0);
			keyframe_gui_lock->unlock();
			return;
		}
	}

// Create new thread
	KeyFrameThread *thread = new KeyFrameThread(this);
	keyframe_threads->append(thread);
	thread->start_window(plugin, 0);

	keyframe_gui_lock->unlock();
}





void MWindow::show_plugin(Plugin *plugin)
{
	int done = 0;

SET_TRACE
// Remove previously deleted plugin GUIs
	dead_plugin_lock->lock("MWindow::show_plugin");
	dead_plugins->remove_all_objects();
	dead_plugin_lock->unlock();

//printf("MWindow::show_plugin %d\n", __LINE__);
SET_TRACE


	plugin_gui_lock->lock("MWindow::show_plugin");
	for(int i = 0; i < plugin_guis->total; i++)
	{
// Pointer comparison
		if(plugin_guis->get(i)->plugin == plugin)
		{
			plugin_guis->get(i)->raise_window();
			done = 1;
			break;
		}
	}
SET_TRACE

//printf("MWindow::show_plugin 1\n");
	if( !done && !plugin->track ) {
		printf("MWindow::show_plugin track not defined.\n");
		done = 1;
	}
	if( !done ) {
		PluginServer *server = scan_plugindb(plugin->title,
			plugin->track->data_type);

//printf("MWindow::show_plugin %p %d\n", server, server->uses_gui);
		if(server && server->uses_gui)
		{
			PluginServer *gui = new PluginServer(*server);
			plugin_guis->append(gui);
// Needs mwindow to do GUI
			gui->set_mwindow(this);
			gui->open_plugin(0, preferences, edl, plugin);
			plugin->show = 1;
			gui->show_gui();
		}
	}
	plugin_gui_lock->unlock();
//printf("MWindow::show_plugin %d\n", __LINE__);
SET_TRACE
//sleep(1);
//printf("MWindow::show_plugin 2\n");
}

void MWindow::hide_plugin(Plugin *plugin, int lock)
{
	plugin->show = 0;
// Update the toggle
	gui->lock_window("MWindow::hide_plugin");
	gui->update(0, NORMAL_DRAW, 0, 0, 0, 0, 0);
	gui->unlock_window();

	if(lock) plugin_gui_lock->lock("MWindow::hide_plugin");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->get(i)->plugin == plugin)
		{
			PluginServer *ptr = plugin_guis->get(i);
			plugin_guis->remove(ptr);
			if(lock) plugin_gui_lock->unlock();
// Last command executed in client side close
// Schedule for deletion
			ptr->hide_gui();
			delete_plugin(ptr);
//sleep(1);
			return;
		}
	}
	if(lock) plugin_gui_lock->unlock();
}

void MWindow::delete_plugin(PluginServer *plugin)
{
	dead_plugin_lock->lock("MWindow::delete_plugin");
	dead_plugins->append(plugin);
	dead_plugin_lock->unlock();
}

void MWindow::hide_plugins()
{
	plugin_gui_lock->lock("MWindow::hide_plugins 1");
	while(plugin_guis->size())
	{
		PluginServer *ptr = plugin_guis->get(0);
		plugin_guis->remove(ptr);
		plugin_gui_lock->unlock();
// Last command executed in client side close
// Schedule for deletion
		ptr->hide_gui();
		delete_plugin(ptr);
		plugin_gui_lock->lock("MWindow::hide_plugins 2");
	}
	plugin_gui_lock->unlock();

	hide_keyframe_guis();
}

void MWindow::hide_keyframe_guis()
{
	keyframe_gui_lock->lock("MWindow::hide_keyframe_guis");
	for(int i = 0; i < keyframe_threads->size(); i++)
	{
		keyframe_threads->get(i)->close_window();
	}
	keyframe_gui_lock->unlock();
}

void MWindow::hide_keyframe_gui(Plugin *plugin)
{
	keyframe_gui_lock->lock("MWindow::hide_keyframe_gui");
	for(int i = 0; i < keyframe_threads->size(); i++)
	{
		if(keyframe_threads->get(i)->plugin == plugin)
		{
			keyframe_threads->get(i)->close_window();
			break;
		}
	}
	keyframe_gui_lock->unlock();
}

int MWindow::get_hash_color(Edit *edit)
{
	Indexable *idxbl = edit->asset ?
		(Indexable*)edit->asset : (Indexable*)edit->nested_edl;
	if( !idxbl ) return 0;
	char path[BCTEXTLEN];
	if( !edit->asset || edit->track->data_type != TRACK_VIDEO ||
	    edl->session->proxy_scale == 1 ||
	    ProxyRender::from_proxy_path(path, idxbl, edl->session->proxy_scale) )
		strcpy(path, idxbl->path);
	char *cp = strrchr(path, '/');
	cp = !cp ? path : cp+1;
	uint8_t *bp = (uint8_t*)cp;
	int v = 0;
	while( *bp ) v += *bp++;
	return get_hash_color(v);
}

int MWindow::get_hash_color(int v)
{
	int hash = 0x303030;
	if( v & 0x01 ) hash ^= 0x000040;
	if( v & 0x02 ) hash ^= 0x004000;
	if( v & 0x04 ) hash ^= 0x400000;
	if( v & 0x08 ) hash ^= 0x080000;
	if( v & 0x10 ) hash ^= 0x000800;
	if( v & 0x20 ) hash ^= 0x000008;
	if( v & 0x40 ) hash ^= 0x404040;
	if( v & 0x80 ) hash ^= 0x080808;
	return hash;
}

int MWindow::get_group_color(int v)
{
	int color = 0x606060;
	if( v & 0x01 ) color ^= 0x000080;
	if( v & 0x02 ) color ^= 0x008000;
	if( v & 0x04 ) color ^= 0x800000;
	if( v & 0x08 ) color ^= 0x100000;
	if( v & 0x10 ) color ^= 0x001000;
	if( v & 0x20 ) color ^= 0x000010;
	if( v & 0x40 ) color ^= 0x080808;
	if( v & 0x80 ) color ^= 0x909090;
	return color;
}

int MWindow::get_title_color(Edit *edit)
{
        unsigned color = edit->color & 0xffffff;
	unsigned alpha = (~edit->color>>24) & 0xff;
	if( !color ) {
		if( edit->group_id )
			color = get_group_color(edit->group_id);
		else if( preferences->autocolor_assets )
			color = get_hash_color(edit);
		else
			return 0;
	}
	if( alpha == 0xff )
		alpha = session->title_bar_alpha*255;
	return color | (~alpha<<24);
}

void MWindow::update_keyframe_guis()
{
// Send new configuration to keyframe GUI's
	keyframe_gui_lock->lock("MWindow::update_keyframe_guis");
	for(int i = 0; i < keyframe_threads->size(); i++)
	{
		KeyFrameThread *ptr = keyframe_threads->get(i);
		if(edl->tracks->plugin_exists(ptr->plugin))
			ptr->update_gui(1);
		else
		{
			ptr->close_window();
		}
	}
	keyframe_gui_lock->unlock();
}

void MWindow::update_plugin_guis(int do_keyframe_guis)
{
// Send new configuration to plugin GUI's
	plugin_gui_lock->lock("MWindow::update_plugin_guis");

	for(int i = 0; i < plugin_guis->size(); i++)
	{
		PluginServer *ptr = plugin_guis->get(i);
		if(edl->tracks->plugin_exists(ptr->plugin))
			ptr->update_gui();
		else
		{
// Schedule for deletion if no plugin
			plugin_guis->remove_number(i);
			i--;

			ptr->hide_gui();
			delete_plugin(ptr);
		}
	}


// Change plugin variable if not visible
	Track *track = edl->tracks->first;
	while(track)
	{
		for(int i = 0; i < track->plugin_set.size(); i++)
		{
			Plugin *plugin = (Plugin*)track->plugin_set[i]->first;
			while(plugin)
			{
				int got_it = 0;
				for(int i = 0; i < plugin_guis->size(); i++)
				{
					PluginServer *server = plugin_guis->get(i);
					if(server->plugin == plugin)
					{
						got_it = 1;
						break;
					}
				}

				if(!got_it) plugin->show = 0;

				plugin = (Plugin*)plugin->next;
			}
		}

		track = track->next;
	}

	plugin_gui_lock->unlock();


	if(do_keyframe_guis) update_keyframe_guis();
}

int MWindow::plugin_gui_open(Plugin *plugin)
{
	int result = 0;
	plugin_gui_lock->lock("MWindow::plugin_gui_open");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->get(i)->plugin->identical_location(plugin))
		{
			result = 1;
			break;
		}
	}
	plugin_gui_lock->unlock();
	return result;
}

void MWindow::render_plugin_gui(void *data, Plugin *plugin)
{
	plugin_gui_lock->lock("MWindow::render_plugin_gui");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->get(i)->plugin->identical_location(plugin))
		{
			plugin_guis->get(i)->render_gui(data);
			break;
		}
	}
	plugin_gui_lock->unlock();
}

void MWindow::render_plugin_gui(void *data, int size, Plugin *plugin)
{
	plugin_gui_lock->lock("MWindow::render_plugin_gui");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->get(i)->plugin->identical_location(plugin))
		{
			plugin_guis->get(i)->render_gui(data, size);
			break;
		}
	}
	plugin_gui_lock->unlock();
}


void MWindow::update_plugin_states()
{
	plugin_gui_lock->lock("MWindow::update_plugin_states");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		int result = 0;
// Get a plugin GUI
		Plugin *src_plugin = plugin_guis->get(i)->plugin;
		PluginServer *src_plugingui = plugin_guis->get(i);

// Search for plugin in EDL.  Only the master EDL shows plugin GUIs.
		for(Track *track = edl->tracks->first;
			track && !result;
			track = track->next)
		{
			for(int j = 0;
				j < track->plugin_set.total && !result;
				j++)
			{
				PluginSet *plugin_set = track->plugin_set[j];
				for(Plugin *plugin = (Plugin*)plugin_set->first;
					plugin && !result;
					plugin = (Plugin*)plugin->next)
				{
					if(plugin == src_plugin &&
						!strcmp(plugin->title, src_plugingui->title)) result = 1;
				}
			}
		}


// Doesn't exist anymore
		if(!result)
		{
			hide_plugin(src_plugin, 0);
			i--;
		}
	}
	plugin_gui_lock->unlock();
}


void MWindow::update_plugin_titles()
{
	for(int i = 0; i < plugin_guis->total; i++)
	{
		plugin_guis->get(i)->update_title();
	}
}

int MWindow::asset_to_edl(EDL *new_edl,
	Asset *new_asset,
	RecordLabels *labels)
{
const int debug = 0;
if(debug) printf("MWindow::asset_to_edl %d new_asset->layers=%d\n",
__LINE__,
new_asset->layers);
// Keep frame rate, sample rate, and output size unchanged.
// These parameters would revert the project if VWindow displayed an asset
// of different size than the project.
	if(new_asset->video_data)
	{
		new_edl->session->video_tracks = new_asset->layers;
	}
	else
		new_edl->session->video_tracks = 0;

if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);





	if(new_asset->audio_data)
	{
		new_edl->session->audio_tracks = new_asset->channels;
	}
	else
		new_edl->session->audio_tracks = 0;
//printf("MWindow::asset_to_edl 2 %d %d\n", new_edl->session->video_tracks, new_edl->session->audio_tracks);

if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);
	new_edl->create_default_tracks();
//printf("MWindow::asset_to_edl 2 %d %d\n", new_edl->session->video_tracks, new_edl->session->audio_tracks);
if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);



//printf("MWindow::asset_to_edl 3\n");
	new_edl->insert_asset(new_asset,
		0,
		0,
		0,
		labels);
//printf("MWindow::asset_to_edl 3\n");
if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);

// Align cursor on frames:: clip the new_edl to the minimum of the last joint frame.
	if(edl->session->cursor_on_frames)
	{
		double length = new_edl->tracks->total_length();
		double edl_length = new_edl->tracks->total_length_framealigned(edl->session->frame_rate);
		new_edl->tracks->clear(length, edl_length, 1, 1);
	}




	char string[BCTEXTLEN];
	FileSystem fs;
	fs.extract_name(string, new_asset->path);
//printf("MWindow::asset_to_edl 3\n");

	strcpy(new_edl->local_session->clip_title, string);
//printf("MWindow::asset_to_edl 4 %s\n", string);
if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);

	return 0;
}


// Reset everything after a load.
void MWindow::update_project(int load_mode)
{
	const int debug = 0;

	if(debug) PRINT_TRACE
	edl->tracks->update_y_pixels(theme);
	session->update_clip_number();

	if(debug) PRINT_TRACE

	if( load_mode == LOADMODE_REPLACE ||
	    load_mode == LOADMODE_REPLACE_CONCATENATE ) {
		delete gui->keyvalue_popup;
		gui->keyvalue_popup = 0;
		gui->load_panes();
	}

	gui->update(1, NORMAL_DRAW, 1, 1, 1, 1, 1);
	if(debug) PRINT_TRACE
	gui->unlock_window();
	init_brender();

	cwindow->gui->lock_window("MWindow::update_project 1");
	cwindow->update(0, 0, 1, 1, 1);
	cwindow->gui->unlock_window();

	if(debug) PRINT_TRACE

// Close all the vwindows
	if( load_mode == LOADMODE_REPLACE ||
	    load_mode == LOADMODE_REPLACE_CONCATENATE ) {
		if(debug) PRINT_TRACE
		int first_vwindow = 0;
		if(session->show_vwindow) first_vwindow = 1;
// Change visible windows to no source
		for(int i = 0; i < first_vwindow && i < vwindows.size(); i++) {
			VWindow *vwindow = vwindows[i];
			if( !vwindow->is_running() ) continue;
			vwindow->change_source(-1);
		}

// Close remaining windows
		for(int i = first_vwindow; i < vwindows.size(); i++) {
			VWindow *vwindow = vwindows[i];
			if( !vwindow->is_running() ) continue;
			vwindow->close_window();
		}
		for( int i=0; i<edl->vwindow_edls.size(); ++i ) {
			VWindow *vwindow = get_viewer(1, -1);
			vwindow->change_source(i);
		}
		if(debug) PRINT_TRACE
		select_zwindow(0);
		close_mixers(0);

		for( int i=0; i<edl->mixers.size(); ++i ) {
			Mixer *mixer = edl->mixers[i];
			ZWindow *zwindow = get_mixer(mixer);
			zwindow->set_title(mixer->title);
			zwindow->start();
		}
	}
	update_vwindow();

	if(debug) PRINT_TRACE
	cwindow->gui->lock_window("MWindow::update_project 2");
	cwindow->gui->timebar->update(0);
	cwindow->gui->unlock_window();

	if(debug) PRINT_TRACE
	cwindow->refresh_frame(CHANGE_ALL);

	awindow->gui->async_update_assets();
	if(debug) PRINT_TRACE

	gui->lock_window("MWindow::update_project");
	gui->update_mixers(0, 0);
	gui->flush();
	if(debug) PRINT_TRACE
}

void MWindow::update_vwindow()
{
	for( int i=0; i<vwindows.size(); ++i ) {
		VWindow *vwindow = vwindows[i];
		if( vwindow->is_running() ) {
			vwindow->gui->lock_window("MWindow::update_vwindow");
			vwindow->update(1);
			vwindow->gui->unlock_window();
		}
	}
}

void MWindow::remove_indexfile(Indexable *indexable)
{
	if( !indexable->is_asset ) return;
	Asset *asset = (Asset *)indexable;
// Erase file
	IndexFile::delete_index(preferences, asset, ".toc");
	IndexFile::delete_index(preferences, asset, ".idx");
	IndexFile::delete_index(preferences, asset, ".mkr");
}

void MWindow::rebuild_indices()
{
	for(int i = 0; i < session->drag_assets->total; i++)
	{
		Indexable *indexable = session->drag_assets->get(i);
//printf("MWindow::rebuild_indices 1 %s\n", indexable->path);
		remove_indexfile(indexable);
// Schedule index build
		IndexState *index_state = indexable->index_state;
		index_state->index_status = INDEX_NOTTESTED;
		if( indexable->is_asset ) {
			Asset *asset = (Asset *)indexable;
			if( asset->format != FILE_PCM ) {
				asset->format = FILE_UNKNOWN;
				asset->reset_audio();
			}
			asset->reset_video();
			remove_asset_from_caches(asset);
//			File file; // re-probe the asset
//			file.open_file(preferences, asset, 1, 0);
		}
		mainindexes->add_next_asset(0, indexable);
	}
	mainindexes->start_build();
}


void MWindow::save_backup()
{
	FileXML file;
	edl->optimize();
	edl->set_path(session->filename);
	char backup_path[BCTEXTLEN], backup_path1[BCTEXTLEN];
	snprintf(backup_path, sizeof(backup_path), "%s/%s",
		File::get_config_path(), BACKUP_FILE);
	snprintf(backup_path1, sizeof(backup_path1), "%s/%s",
		File::get_config_path(), BACKUP_FILE1);
	rename(backup_path, backup_path1);
	edl->save_xml(&file, backup_path);
	file.terminate_string();
	FileSystem fs;
	fs.complete_path(backup_path);

	if(file.write_to_file(backup_path))
	{
		char string2[256];
		sprintf(string2, _("Couldn't open %s for writing."), backup_path);
		gui->show_message(string2);
	}
}

void MWindow::load_backup()
{
	ArrayList<char*> path_list;
	path_list.set_array_delete();
	char *out_path;
	char backup_path[BCTEXTLEN];
	snprintf(backup_path, sizeof(backup_path), "%s/%s",
		File::get_config_path(), BACKUP_FILE);
	FileSystem fs;
	fs.complete_path(backup_path);

	path_list.append(out_path = new char[strlen(backup_path) + 1]);
	strcpy(out_path, backup_path);

	load_filenames(&path_list, LOADMODE_REPLACE, 0);
	edl->local_session->clip_title[0] = 0;
// This is unique to backups since the path of the backup is different than the
// path of the project.
	set_filename(edl->path);
	path_list.remove_all_objects();
	save_backup();
}


void MWindow::save_undo_data()
{
	undo_before();
	undo_after(_("perpetual session"), LOAD_ALL);
	char perpetual_path[BCTEXTLEN];
	snprintf(perpetual_path, sizeof(perpetual_path), "%s/%s",
		File::get_config_path(), PERPETUAL_FILE);
	FILE *fp = fopen(perpetual_path,"w");
	if( !fp ) return;
	undo->save(fp);
	fclose(fp);
}

void MWindow::load_undo_data()
{
	char perpetual_path[BCTEXTLEN];
	snprintf(perpetual_path, sizeof(perpetual_path), "%s/%s",
		File::get_config_path(), PERPETUAL_FILE);
	FILE *fp = fopen(perpetual_path,"r");
	if( !fp ) return;
	undo->load(fp);
	fclose(fp);
}


int MWindow::copy_target(const char *path, const char *target)
{
	int ifd = ::open(path, O_RDONLY);
	if( ifd < 0 ) {
		eprintf("Cannot open asset: %s", path);
		return 1;
	}
	int ret = 0;
	int ofd = ::open(target, O_CREAT+O_TRUNC+O_WRONLY, 0777);
	if( ofd >= 0 ) {
		struct stat st;
		int64_t total_bytes = !fstat(ifd, &st) ? st.st_size : 0;
		char progress_title[BCTEXTLEN];
		sprintf(progress_title, _("Copying: %s\n"), target);
		BC_ProgressBox progress(-1, -1, progress_title, total_bytes);

		int64_t count = 0, len = -1;
		int bfrsz = 0x100000;
		uint8_t *bfr = new uint8_t[bfrsz];
		while( (len=::read(ifd, bfr, bfrsz)) > 0 ) {
			if( len != ::write(ofd, bfr, len) ) {
				eprintf("Error writing: %s", target);
				break;
			}
			if( progress.is_cancelled() ) {
				ret = 1;
				break;
			}
			progress.update(count += len, 1);
		}
		delete [] bfr;
		::close(ofd);

		progress.stop_progress();
		if( len < 0 ) {
			eprintf("Error reading: %s", path);
			ret = 1;
		}
	}
	else
		eprintf("Cannot create asset target: %s", target);
	::close(ifd);
	return ret;
}

int MWindow::link_target(const char *real_path, const char *link_path, int relative)
{
	char target[BCTEXTLEN];
	if( relative ) {
		const char *bp = real_path, *cp = bp;
		const char *lp = link_path, *np = lp;
		char *tp = target, *ep = tp+sizeof(target)-1, lch;
		while( *lp && *bp && (lch=*lp++) == *bp++ ) {
			if( lch == '/' ) { np = lp;  cp = bp; }
		}
		while( tp<ep && *np ) {
			if( *np++ != '/' ) continue;
			*tp++ = '.';  *tp++ = '.';  *tp++ = '/';
		}
		while( tp<ep && *cp ) *tp++ = *cp++;
		*tp = 0;
	}
	else
		strcpy(target, real_path);
	if( symlink(target, link_path) ) {
		eprintf("Cannot create symlink: %s", link_path);
		return 1;
	}
	return 0;
}

void MWindow::save_project(const char *dir, int save_mode, int overwrite, int reload)
{
	char dir_path[BCTEXTLEN];
	strcpy(dir_path, dir);
	FileSystem fs;
	fs.complete_path(dir_path);

	struct stat st;
	if( !stat(dir_path, &st) ) {
		if( !S_ISDIR(st.st_mode) ) {
			eprintf("Path exists and is not a directory\n%s", dir_path);
			return;
		}
	}
	else {
		if( mkdir(dir_path, S_IRWXU | S_IRWXG | S_IRWXO) ) {
			eprintf("Cannot create directory\n%s", dir_path);
			return;
		}
	}
	char *real_dir = realpath(dir_path, 0);
	strcpy(dir_path, real_dir);
	free(real_dir);

	EDL *save_edl = new EDL;
	save_edl->create_objects();
	save_edl->copy_all(edl);

	char progress_title[BCTEXTLEN];
	sprintf(progress_title, _("Saving to %s:\n"), dir);
	int total_assets = save_edl->assets->total();
	MainProgressBar *progress = mainprogress->start_progress(progress_title, total_assets);
	int ret = 0;
	Asset *current = save_edl->assets->first;
	for( int i=0; !ret && current; ++i, current=NEXT ) {
		char *path = current->path;
		if( ::stat(path, &st) ) {
			eprintf("Asset not found: %s", path);
			continue;
		}
		char *real_path = realpath(path, 0);
		const char *cp = strrchr(path, '/'), *bp = !cp ? path : cp+1;
		char link_path[BCTEXTLEN];
		snprintf(link_path, sizeof(link_path), "%s/%s", dir_path, bp);
		int skip = 0;
		if( strcmp(real_path, link_path) ) {
			if( !::lstat(link_path, &st) ) {
				if( overwrite )
					::remove(link_path);
				else
					skip = 1;
			}
		}
		else {
			eprintf("copy/link to self, skippped: %s", path);
			skip = 1;
		}
		if( !skip ) {
			if( save_mode == SAVE_PROJECT_COPY ) {
				if( copy_target(real_path, link_path) )
					ret = 1;
			}
			else {
				link_target(real_path, link_path,
					save_mode == SAVE_PROJECT_RELLINK ? 1 : 0);
			}
		}
		free(real_path);
		strcpy(path, link_path);

		if( progress->is_cancelled() ) break;
		progress->update(i);
	}

	progress->stop_progress();
	delete progress;

	char *cp = strrchr(dir_path,'/');
	char *bp = cp ? cp+1 : dir_path;
	char filename[BCTEXTLEN];
	snprintf(filename, sizeof(filename), "%s/%s.xml", dir_path, bp);
	save_edl->set_path(filename);
	FileXML file;
	save_edl->save_xml(&file, filename);
	file.terminate_string();

	if( !file.write_to_file(filename) ) {
		char string[BCTEXTLEN];
		sprintf(string, _("\"%s\" %dC written"), filename, (int)strlen(file.string()));
		gui->lock_window("SaveProject::run 2");
		gui->show_message(string);
		gui->unlock_window();
		gui->mainmenu->add_load(filename);
	}
	else
		eprintf(_("Couldn't open %s."), filename);

	save_edl->remove_user();

	if( reload ) {
		gui->lock_window("MWindow::save_project");
		ArrayList<char*> filenames;
		filenames.append(filename);
		load_filenames(&filenames, LOADMODE_REPLACE);
		gui->unlock_window();
	}
}


static inline int gcd(int m, int n)
{
	int r;
	if( m < n ) { r = m;  m = n;  n = r; }
	while( (r = m % n) != 0 ) { m = n;  n = r; }
	return n;
}

int MWindow::create_aspect_ratio(float &w, float &h, int width, int height)
{
	w = 1;  h = 1;
	if(!width || !height) return 1;
	if( width == 720 && (height == 480 || height == 576) ) {
		w = 4;  h = 3;  return 0; // for NTSC and PAL
	}
	double ar = (double)width / height;
// square-ish pixels
	if( EQUIV(ar, 1.0000) ) return 0;
	if( EQUIV(ar, 1.3333) ) { w = 4;  h = 3;  return 0; }
	if( EQUIV(ar, 1.7777) ) { w = 16; h = 9;  return 0; }
	if( EQUIV(ar, 2.1111) ) { w = 19; h = 9;  return 0; }
	if( EQUIV(ar, 2.2222) ) { w = 20; h = 9;  return 0; }
	if( EQUIV(ar, 2.3333) ) { w = 21; h = 9;  return 0; }
	int ww = width, hh = height;
	// numerator, denominator must be under mx
	int mx = 255, n = gcd(ww, hh);
	if( n > 1 ) { ww /= n; hh /= n; }
	// search near height in case extra/missing lines
	if( ww >= mx || hh >= mx ) {
		double err = height;  // +/- 2 percent height
		for( int m=2*height/100, i=1; m>0; i=i>0 ? -i : (--m, -i+1) ) {
			int iw = width, ih = height+i;
			if( (n=gcd(iw, ih)) > 1 ) {
				int u = iw/n;  if( u >= mx ) continue;
				int v = ih/n;  if( v >= mx ) continue;
				double r = (double) u/v, er = fabs(ar-r);
				if( er >= err ) continue;
				err = er;  ww = u;  hh = v;
			}
		}
	}

	w = ww;  h = hh;
	return 0;
}

void MWindow::reset_caches()
{
	frame_cache->remove_all();
	wave_cache->remove_all();
	audio_cache->remove_all();
	video_cache->remove_all();
	if( cwindow->playback_engine ) {
		if( cwindow->playback_engine->audio_cache )
			cwindow->playback_engine->audio_cache->remove_all();
		if( cwindow->playback_engine->video_cache )
			cwindow->playback_engine->video_cache->remove_all();
	}
	for(int i = 0; i < vwindows.size(); i++) {
		VWindow *vwindow = vwindows[i];
		if( !vwindow->is_running() ) continue;
		if( !vwindow->playback_engine ) continue;
		if( vwindow->playback_engine->audio_cache )
			vwindow->playback_engine->audio_cache->remove_all();
		if( vwindow->playback_engine->video_cache )
			vwindow->playback_engine->video_cache->remove_all();
	}
}

void MWindow::remove_asset_from_caches(Asset *asset)
{
	frame_cache->remove_asset(asset);
	wave_cache->remove_asset(asset);
	audio_cache->delete_entry(asset);
	video_cache->delete_entry(asset);
	if( cwindow->playback_engine && cwindow->playback_engine->audio_cache )
		cwindow->playback_engine->audio_cache->delete_entry(asset);
	if( cwindow->playback_engine && cwindow->playback_engine->video_cache )
		cwindow->playback_engine->video_cache->delete_entry(asset);
	for(int i = 0; i < vwindows.size(); i++) {
		VWindow *vwindow = vwindows[i];
		if( !vwindow->is_running() ) continue;
		if( !vwindow->playback_engine ) continue;
		if( vwindow->playback_engine->audio_cache )
			vwindow->playback_engine->audio_cache->delete_entry(asset);
		if( vwindow->playback_engine->video_cache )
			vwindow->playback_engine->video_cache->delete_entry(asset);
	}
}


void MWindow::remove_assets_from_project(int push_undo, int redraw,
		ArrayList<Indexable*> *drag_assets, ArrayList<EDL*> *drag_clips)
{
	awindow->gui->close_view_popup();

	for(int i = 0; i < drag_assets->total; i++) {
		Indexable *indexable = drag_assets->get(i);
		if(indexable->is_asset) remove_asset_from_caches((Asset*)indexable);
	}

// Remove from VWindow.
	for(int i = 0; i < session->drag_clips->total; i++) {
		for(int j = 0; j < vwindows.size(); j++) {
			VWindow *vwindow = vwindows[j];
			if( !vwindow->is_running() ) continue;
			if(session->drag_clips->get(i) == vwindow->get_edl()) {
				vwindow->gui->lock_window("MWindow::remove_assets_from_project 1");
				vwindow->delete_source(1, 1);
				vwindow->gui->unlock_window();
			}
		}
	}

	for(int i = 0; i < drag_assets->size(); i++) {
		for(int j = 0; j < vwindows.size(); j++) {
			VWindow *vwindow = vwindows[j];
			if( !vwindow->is_running() ) continue;
			if(drag_assets->get(i) == vwindow->get_source()) {
				vwindow->gui->lock_window("MWindow::remove_assets_from_project 2");
				vwindow->delete_source(1, 1);
				vwindow->gui->unlock_window();
			}
		}
	}

	for(int i = 0; i < drag_assets->size(); i++) {
		Indexable *indexable = drag_assets->get(i);
		remove_indexfile(indexable);
	}

//printf("MWindow::rebuild_indices 1 %s\n", indexable->path);
	if(push_undo) undo_before();
	if(drag_assets) edl->remove_from_project(drag_assets);
	if(drag_clips) edl->remove_from_project(drag_clips);
	if(redraw) save_backup();
	if(push_undo) undo_after(_("remove assets"), LOAD_ALL);
	if(redraw) {
		restart_brender();

		gui->lock_window("MWindow::remove_assets_from_project 3");
		gui->update(1, NORMAL_DRAW, 1, 1, 0, 1, 0);
		gui->unlock_window();

	// Removes from playback here
		sync_parameters(CHANGE_ALL);
	}

	awindow->gui->async_update_assets();
}

void MWindow::remove_assets_from_disk()
{
	remove_assets_from_project(1,
		1,
		session->drag_assets,
		session->drag_clips);

// Remove from disk
	for(int i = 0; i < session->drag_assets->total; i++)
	{
		remove(session->drag_assets->get(i)->path);
	}
}

void MWindow::dump_plugins(FILE *fp)
{
	if( !plugindb ) return;
	for(int i = 0; i < plugindb->total; i++)
	{
		fprintf(fp, "type=%d audio=%d video=%d rt=%d multi=%d"
			" synth=%d transition=%d theme=%d %s\n",
			plugindb->get(i)->plugin_type,
			plugindb->get(i)->audio,
			plugindb->get(i)->video,
			plugindb->get(i)->realtime,
			plugindb->get(i)->multichannel,
			plugindb->get(i)->get_synthesis(),
			plugindb->get(i)->transition,
			plugindb->get(i)->theme,
			plugindb->get(i)->title);
	}
}

void MWindow::dump_edl(FILE *fp)
{
	if( !edl ) return;
	edl->dump(fp);
}

void MWindow::dump_undo(FILE *fp)
{
	if( !undo ) return;
	undo->dump(fp);
}

void MWindow::dump_exe(FILE *fp)
{
	char proc_path[BCTEXTLEN], exe_path[BCTEXTLEN];
	sprintf(proc_path, "/proc/%d/exe", (int)getpid());

	int ret = -1, n = 100;
	for( int len; (len=readlink(proc_path, exe_path, sizeof(exe_path)))>0; --n ) {
		exe_path[len] = 0;  strcpy(proc_path, exe_path);
		ret = 0;
	}
	if( n < 0 || ret < 0 ) { fprintf(fp,"readlink: %m\n"); return; }

	struct stat st;
	if( stat(proc_path,&st) ) { fprintf(fp,"stat: %m\n"); return; }
	fprintf(fp, "path: %s = %9jd bytes\n",proc_path,st.st_size);
	struct tm *tm = localtime(&st.st_mtime);
	char mtime[256];
	strftime(mtime, sizeof(mtime), "%F %T", tm);
	fprintf(fp,"mtime: %s\n", mtime);
#if 0
// people hit ctl-c waiting for this
	int fd = open(proc_path,O_RDONLY+O_NONBLOCK);
	if( fd < 0 ) { fprintf(fp,"open: %m\n"); return; }
	uint8_t *bfr = 0;
	int64_t bfrsz = 0;
	int64_t pagsz = sysconf(_SC_PAGE_SIZE);
	int64_t maxsz = 1024*pagsz;
	int64_t size = st.st_size, pos = 0;
	SHA1 sha1;
	while( (bfrsz = size-pos) > 0 ) {
		if( bfrsz > maxsz ) bfrsz = maxsz;
		bfr = (uint8_t *)mmap(NULL, bfrsz, PROT_READ,
			MAP_PRIVATE+MAP_NORESERVE+MAP_POPULATE, fd, pos);
		if( bfr == MAP_FAILED ) break;
		sha1.addBytes(bfr, bfrsz);
		munmap(bfr, bfrsz);
		pos += bfrsz;
	}
	close(fd);
	ret = pos < size ? EIO : 0;
	fprintf(fp, "SHA1: ");
	uint8_t digest[20];  sha1.computeHash(digest);
	for( int i=0; i<20; ++i ) fprintf(fp, "%02x", digest[i]);
	if( ret < 0 ) fprintf(fp, " (ret %d)", ret);
	if( pos < st.st_size ) fprintf(fp, " (pos %jd)", pos);
#endif
	fprintf(fp, "\n");
}

void MWindow::trap_hook(FILE *fp, void *vp)
{
	MWindow *mwindow = (MWindow *)vp;
//	fprintf(fp, "\nPLUGINS:\n");
//	mwindow->dump_plugins(fp);
	fprintf(fp, "\nEDL:\n");
	mwindow->dump_edl(fp);
	fprintf(fp, "\nUNDO:\n");
	mwindow->dump_undo(fp);
	fprintf(fp, "\nEXE: %s\n", AboutPrefs::build_timestamp);
	mwindow->dump_exe(fp);
}






int MWindow::save_defaults()
{
	gui->save_defaults(defaults);
	edl->save_defaults(defaults);
	session->save_defaults(defaults);
	preferences->save_defaults(defaults);

	for(int i = 0; i < plugin_guis->total; i++)
	{
// Pointer comparison
		plugin_guis->get(i)->save_defaults();
	}
	awindow->save_defaults(defaults);

	defaults->save();
	return 0;
}

int MWindow::run_script(FileXML *script)
{
	int result = 0, result2 = 0;
	while(!result && !result2)
	{
		result = script->read_tag();
		if(!result)
		{
			if(script->tag.title_is("new_project"))
			{
// Run new in immediate mode.
//				gui->mainmenu->new_project->run_script(script);
			}
			else
			if(script->tag.title_is("record"))
			{
// Run record as a thread.  It is a terminal command.
				;
// Will read the complete scipt file without letting record read it if not
// terminated.
				result2 = 1;
			}
			else
			{
				printf("MWindow::run_script: Unrecognized command: %s\n",script->tag.get_title() );
			}
		}
	}
	return result2;
}

// ================================= synchronization


int MWindow::interrupt_indexes()
{
	mainprogress->cancelled = 1;
	mainindexes->interrupt_build();
	return 0;
}



void MWindow::next_time_format()
{
	switch(edl->session->time_format)
	{
		case TIME_HMS: edl->session->time_format = TIME_HMSF; break;
		case TIME_HMSF: edl->session->time_format = TIME_SAMPLES; break;
		case TIME_SAMPLES: edl->session->time_format = TIME_SAMPLES_HEX; break;
		case TIME_SAMPLES_HEX: edl->session->time_format = TIME_FRAMES; break;
		case TIME_FRAMES: edl->session->time_format = TIME_FEET_FRAMES; break;
		case TIME_FEET_FRAMES: edl->session->time_format = TIME_SECONDS; break;
		case TIME_SECONDS: edl->session->time_format = TIME_HMS; break;
	}

	time_format_common();
}

void MWindow::prev_time_format()
{
	switch(edl->session->time_format)
	{
		case TIME_HMS: edl->session->time_format = TIME_SECONDS; break;
		case TIME_SECONDS: edl->session->time_format = TIME_FEET_FRAMES; break;
		case TIME_FEET_FRAMES: edl->session->time_format = TIME_FRAMES; break;
		case TIME_FRAMES: edl->session->time_format = TIME_SAMPLES_HEX; break;
		case TIME_SAMPLES_HEX: edl->session->time_format = TIME_SAMPLES; break;
		case TIME_SAMPLES: edl->session->time_format = TIME_HMSF; break;
		case TIME_HMSF: edl->session->time_format = TIME_HMS; break;
	}

	time_format_common();
}

void MWindow::time_format_common()
{
	gui->lock_window("MWindow::next_time_format");
	gui->redraw_time_dependancies();


	char string[BCTEXTLEN], string2[BCTEXTLEN];
	sprintf(string, _("Using %s"), Units::print_time_format(edl->session->time_format, string2));
	gui->show_message(string);
	gui->flush();
	gui->unlock_window();
}


int MWindow::set_filename(const char *filename)
{
	if( filename != session->filename )
		strcpy(session->filename, filename);
	if( filename != edl->path )
		strcpy(edl->path, filename);

	if(gui)
	{
		if(filename[0] == 0)
		{
			gui->set_title(PROGRAM_NAME);
		}
		else
		{
			FileSystem dir;
			char string[BCTEXTLEN], string2[BCTEXTLEN];
			dir.extract_name(string, filename);
			sprintf(string2, PROGRAM_NAME ": %s", string);
			gui->set_title(string2);
		}
	}
	return 0;
}








int MWindow::set_loop_boundaries()
{
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();

	if(start !=
		end)
	{
		;
	}
	else
	if(edl->tracks->total_length())
	{
		start = 0;
		end = edl->tracks->total_length();
	}
	else
	{
		start = end = 0;
	}

	if(edl->local_session->loop_playback && start != end)
	{
		edl->local_session->loop_start = start;
		edl->local_session->loop_end = end;
	}
	return 0;
}







int MWindow::reset_meters()
{
	cwindow->gui->lock_window("MWindow::reset_meters 1");
	cwindow->gui->meters->reset_meters();
	cwindow->gui->unlock_window();

	for(int j = 0; j < vwindows.size(); j++) {
		VWindow *vwindow = vwindows[j];
		if( !vwindow->is_running() ) continue;
		vwindow->gui->lock_window("MWindow::reset_meters 2");
		vwindow->gui->meters->reset_meters();
		vwindow->gui->unlock_window();
	}

	lwindow->gui->lock_window("MWindow::reset_meters 3");
	lwindow->gui->panel->reset_meters();
	lwindow->gui->unlock_window();

	gui->lock_window("MWindow::reset_meters 4");
	gui->reset_meters();
	gui->unlock_window();
	return 0;
}


void MWindow::resync_guis()
{
// Update GUIs
	restart_brender();
	gui->lock_window("MWindow::resync_guis");
	gui->update(1, NORMAL_DRAW, 1, 1, 1, 1, 0);
	gui->unlock_window();

	cwindow->gui->lock_window("MWindow::resync_guis");
	cwindow->gui->resize_event(cwindow->gui->get_w(),
		cwindow->gui->get_h());
	int channels = edl->session->audio_channels;
	cwindow->gui->meters->set_meters(channels, 1);
	cwindow->gui->flush();
	cwindow->gui->unlock_window();

	for(int i = 0; i < vwindows.size(); i++) {
		VWindow *vwindow = vwindows[i];
		if( !vwindow->is_running() ) continue;
		vwindow->gui->lock_window("MWindow::resync_guis");
		vwindow->gui->resize_event(vwindow->gui->get_w(),
			vwindow->gui->get_h());
		vwindow->gui->meters->set_meters(channels, 1);
		vwindow->gui->flush();
		vwindow->gui->unlock_window();
	}

	lwindow->gui->lock_window("MWindow::resync_guis");
	lwindow->gui->panel->set_meters(channels, 1);
	lwindow->gui->flush();
	lwindow->gui->unlock_window();

// Warn user
	if(((edl->session->output_w % 4) ||
		(edl->session->output_h % 4)) &&
		edl->session->playback_config->vconfig->driver == PLAYBACK_X11_GL)
	{
		MainError::show_error(
			_("This project's dimensions are not multiples of 4 so\n"
			"it can't be rendered by OpenGL."));
	}


// Flash frame
	sync_parameters(CHANGE_ALL);
}

int MWindow::select_asset(Asset *asset, int vstream, int astream, int delete_tracks)
{
	File *file = new File;
	EDLSession *session = edl->session;
	double old_framerate = session->frame_rate;
	double old_samplerate = session->sample_rate;
	int old_auto_keyframes = session->auto_keyframes;
	session->auto_keyframes = 0;
	int result = file->open_file(preferences, asset, 1, 0);
	if( !result && delete_tracks > 0 )
		undo_before();
	int video_layers = asset->get_video_layers();
	if( !result && asset->video_data && vstream < video_layers ) {
		// try to get asset up to date, may fail
		file->select_video_stream(asset, vstream);
		// either way use what was/is there.
		double framerate = asset->get_frame_rate();
		int width = asset->get_w();
		int height = asset->get_h();
		// must be multiple of 4 for opengl
		width = (width+3) & ~3;  height = (height+3) & ~3;
		int driver = session->playback_config->vconfig->driver;
		int color_model = file->get_best_colormodel(asset, driver);
//		color_model = BC_CModels::is_yuv(color_model) ?
//				BC_CModels::has_alpha(color_model) ? BC_YUVA8888 : BC_YUV888 :
//			BC_CModels::is_float(color_model) ?
//				BC_CModels::has_alpha(color_model) ? BC_RGBA_FLOAT : BC_RGB_FLOAT :
//				BC_CModels::has_alpha(color_model) ? BC_RGBA8888 : BC_RGB888;
// have alpha for now
		color_model = BC_CModels::is_yuv(color_model) ?  BC_YUVA8888 :
			BC_CModels::is_float(color_model) ? BC_RGBA_FLOAT : BC_RGBA8888;
		session->color_model = color_model;
		session->output_w = width;
		session->output_h = height;
		session->frame_rate = framerate;
		// not, asset->actual_width/actual_height
		asset->width = session->output_w;
		asset->height = session->output_h;
		asset->frame_rate = session->frame_rate;
		create_aspect_ratio(session->aspect_w, session->aspect_h,
			session->output_w, session->output_h);
		Track *track = edl->tracks->first;
		for( Track *next_track=0; track; track=next_track ) {
			next_track = track->next;
			if( track->data_type != TRACK_VIDEO ) continue;
			if( delete_tracks ) {
				Edit *edit = track->edits->first;
				for( Edit *next_edit=0; edit; edit=next_edit ) {
					next_edit = edit->next;
					if( edit->channel != vstream ||
					    !edit->asset || !edit->asset->is_asset ||
					    !asset->equivalent(*edit->asset,1,1,edl) )
						delete edit;
				}
			}
			if( track->edits->first ) {
				track->track_w = edl->session->output_w;
				track->track_h = edl->session->output_h;
			}
			else if( delete_tracks )
				edl->tracks->delete_track(track);
		}
		edl->retrack();
		edl->resample(old_framerate, session->frame_rate, TRACK_VIDEO);
	}
	if( !result && asset->audio_data && asset->channels > 0 ) {
		session->sample_rate = asset->get_sample_rate();
		int64_t channel_mask = 0;
		int astrm = !asset->video_data || vstream >= video_layers ? -1 :
			file->get_audio_for_video(vstream, astream, channel_mask);
		if( astrm >= 0 ) file->select_audio_stream(asset, astrm);
		if( astrm < 0 || !channel_mask ) channel_mask = (1<<asset->channels)-1;
		int channels = 0;
		for( uint64_t mask=channel_mask; mask!=0; mask>>=1 ) channels += mask & 1;
		if( channels < 1 ) channels = 1;
		if( channels > 6 ) channels = 6;
		session->audio_tracks = session->audio_channels = channels;

		int *achannel_positions = preferences->channel_positions[session->audio_channels-1];
		memcpy(&session->achannel_positions, achannel_positions, sizeof(session->achannel_positions));
		remap_audio(MWindow::AUDIO_1_TO_1);

		if( delete_tracks ) {
			Track *track = edl->tracks->first;
			for( Track *next_track=0; track; track=next_track ) {
				next_track = track->next;
				if( track->data_type != TRACK_AUDIO ) continue;
				Edit *edit = track->edits->first;
					for( Edit *next_edit=0; edit; edit=next_edit ) {
					next_edit = edit->next;
					if( !((1<<edit->channel) & channel_mask) ||
					    !edit->asset || !edit->asset->is_asset ||
					    !asset->equivalent(*edit->asset,1,1,edl) )
						delete edit;
				}
				if( !track->edits->first )
					edl->tracks->delete_track(track);
			}
		}
		edl->rechannel();
		edl->resample(old_samplerate, session->sample_rate, TRACK_AUDIO);
	}
	delete file;
	session->auto_keyframes = old_auto_keyframes;
	if( !result && delete_tracks > 0 ) {
		save_backup();
		undo_after(_("select asset"), LOAD_ALL);
	}
	resync_guis();
	return result;
}

int MWindow::select_asset(int vtrack, int delete_tracks)
{
	Track *track = edl->tracks->get(vtrack, TRACK_VIDEO);
	if( !track )
		track = edl->tracks->get(vtrack, TRACK_AUDIO);
	if( !track ) return 1;
	Edit *edit = track->edits->first;
	if( !edit ) return 1;
	Asset *asset = edit->asset;
	if( !asset || !asset->is_asset ) return 1;
	return select_asset(asset, edit->channel, 0, delete_tracks);
}

void MWindow::dump_plugindb(FILE *fp)
{
	if( !plugindb ) return;
	for(int i = 0; i < plugindb->total; i++)
		plugindb->get(i)->dump(fp);
}

FloatAuto* MWindow::get_float_auto(PatchGUI *patch,int idx)
{
	Auto *current = 0;
	double unit_position = edl->local_session->get_selectionstart(1);
	unit_position = patch->track->to_units(unit_position, 0);

	FloatAutos *ptr = (FloatAutos*)patch->track->automation->autos[idx];
	return (FloatAuto*)ptr->get_prev_auto( (long)unit_position, PLAY_FORWARD, current);
}

IntAuto* MWindow::get_int_auto(PatchGUI *patch,int idx)
{
	Auto *current = 0;
	double unit_position = edl->local_session->get_selectionstart(1);
	unit_position = patch->track->to_units(unit_position, 0);

	IntAutos *ptr = (IntAutos*)patch->track->automation->autos[idx];
	return (IntAuto*)ptr->get_prev_auto( (long)unit_position, PLAY_FORWARD, current);
}

PanAuto* MWindow::get_pan_auto(PatchGUI *patch)
{
	Auto *current = 0;
	double unit_position = edl->local_session->get_selectionstart(1);
	unit_position = patch->track->to_units(unit_position, 0);

	PanAutos *ptr = (PanAutos*)patch->track->automation->autos[AUTOMATION_PAN];
	return (PanAuto*)ptr->get_prev_auto( (long)unit_position, PLAY_FORWARD, current);
}

PatchGUI *MWindow::get_patchgui(Track *track)
{
	PatchGUI *patchgui = 0;
	TimelinePane **panes = gui->pane;
        for( int i=0; i<TOTAL_PANES && !patchgui; ++i ) {
                if( !panes[i] ) continue;
                PatchBay *patchbay = panes[i]->patchbay;
                if( !patchbay ) continue;
                for( int j=0; j<patchbay->patches.total && !patchgui; ++j ) {
                        if( patchbay->patches.values[j]->track == track )
                                patchgui = patchbay->patches.values[j];
                }
        }
        return patchgui;
}

