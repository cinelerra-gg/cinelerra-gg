#include "asset.h"
#include "bchash.h"
#include "bdcreate.h"
#include "clip.h"
#include "edl.h"
#include "edit.h"
#include "edits.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "interlacemodes.h"
#include "keyframe.h"
#include "mainerror.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugin.h"
#include "pluginset.h"
#include "preferences.h"
#include "rescale.h"
#include "track.h"
#include "tracks.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statfs.h>

// BD Creation

// selected by timezone
#define BD_1920x1080_2997i	0
#define BD_1920x1080_25i	3

static struct bd_format {
	const char *name;
	int w, h;
	double framerate;
	int wide, interlaced;
} bd_formats[] = {
// framerates are frames, not fields, per second, *=not standard
	{ "1920x1080 29.97i",	1920,1080, 29.97,  1, ILACE_MODE_TOP_FIRST },
	{ "1920x1080 29.97p*",	1920,1080, 29.97,  1, ILACE_MODE_NOTINTERLACED },
	{ "1920x1080 24p",	1920,1080, 24.,    1, ILACE_MODE_NOTINTERLACED },
	{ "1920x1080 25i",	1920,1080, 25.,    1, ILACE_MODE_TOP_FIRST },
	{ "1920x1080 23.976p",	1920,1080, 23.976, 1, ILACE_MODE_NOTINTERLACED },
	{ "1440x1080 29.97i",	1440,1080, 29.97, -1, ILACE_MODE_TOP_FIRST },
	{ "1440x1080 25i",	1440,1080, 25.,	  -1, ILACE_MODE_TOP_FIRST },
	{ "1440x1080 24p",	1440,1080, 24.,	  -1, ILACE_MODE_NOTINTERLACED },
	{ "1440x1080 23.976p",	1440,1080, 23.976,-1, ILACE_MODE_NOTINTERLACED },
	{ "1280x720  59.94p",	1280,720,  59.94,  1, ILACE_MODE_NOTINTERLACED },
	{ "1280x720  50p",	1280,720,  50.,    1, ILACE_MODE_NOTINTERLACED },
	{ "1280x720  24p",	1280,720,  24.,    1, ILACE_MODE_NOTINTERLACED },
	{ "1280x720  23.976p",	1280,720,  23.976, 1, ILACE_MODE_NOTINTERLACED },
	{ "720x576   25i",	 720,576,  25.,    0, ILACE_MODE_BOTTOM_FIRST },
	{ "720x576   25p*",	 720,576,  25.,    0, ILACE_MODE_NOTINTERLACED },
	{ "720x480   29.97i",	 720,480,  29.97,  0, ILACE_MODE_BOTTOM_FIRST },
	{ "720x480   29.97p*",	 720,480,  29.97,  0, ILACE_MODE_NOTINTERLACED },
};

const int64_t CreateBD_Thread::BD_SIZE = 25000000000;
const int CreateBD_Thread::BD_STREAMS = 1;
const int CreateBD_Thread::BD_WIDTH = 1920;
const int CreateBD_Thread::BD_HEIGHT = 1080;
const double CreateBD_Thread::BD_WIDE_ASPECT_WIDTH = 16.;
const double CreateBD_Thread::BD_WIDE_ASPECT_HEIGHT = 9.;
const double CreateBD_Thread::BD_ASPECT_WIDTH = 4.;
const double CreateBD_Thread::BD_ASPECT_HEIGHT = 3.;
const double CreateBD_Thread::BD_FRAMERATE = 24000. / 1001.;
//const int CreateBD_Thread::BD_MAX_BITRATE = 40000000;
const int CreateBD_Thread::BD_MAX_BITRATE = 10000000;
const int CreateBD_Thread::BD_CHANNELS = 2;
const int CreateBD_Thread::BD_WIDE_CHANNELS = 6;
const double CreateBD_Thread::BD_SAMPLERATE = 48000;
const double CreateBD_Thread::BD_KAUDIO_RATE = 192;
const int CreateBD_Thread::BD_INTERLACE_MODE = ILACE_MODE_NOTINTERLACED;

CreateBD_MenuItem::CreateBD_MenuItem(MWindow *mwindow)
 : BC_MenuItem(_("BD Render..."), _("Ctrl-d"), 'd')
{
	set_ctrl(1);
	this->mwindow = mwindow;
}

int CreateBD_MenuItem::handle_event()
{
	mwindow->create_bd->start();
	return 1;
}

BD_BatchRenderJob::BD_BatchRenderJob(Preferences *preferences, int labeled, int farmed)
 : BatchRenderJob("BD_JOB", preferences, labeled, farmed)
{
}

int BD_BatchRenderJob::get_udfs_mount(char *udfs, char *mopts, char *mntpt)
{
	int ret = 0;
// default: mount -t udf -o loop $dir/bd.udfs $dir/udfs
	strcpy(udfs,"$dir/bd.udfs");
	strcpy(mopts,"-t udf -o loop $dir/bd.udfs ");
	strcpy(mntpt,"$dir/udfs");
	const char *home = getenv("HOME");
	if( !home ) return ret;
	FILE *fp = fopen("/etc/fstab","r");
	if( !fp ) return ret;
	int len = strlen(home);
	char line[BCTEXTLEN], typ[BCTEXTLEN], file[BCTEXTLEN];
	char mpnt[BCTEXTLEN], opts[BCTEXTLEN];
	while( fgets(line,sizeof(line),fp) ) {
// search "/etc/fstab" for: $HOME/img_file $HOME/bluray udf noauto,loop,rw,user
		if( line[0] == '#' || line[0] == ';' ) continue;
		if( sscanf(line,"%s %s %s %s ", &file[0], &mpnt[0], &typ[0], &opts[0]) != 4 )
			continue;
		if( strcmp("udf", typ) ) continue;
		if( strncmp(home, file, len) || file[len] != '/' ) continue;
		if( strncmp(home, mpnt, len) ) continue;
		if( strcmp(&mpnt[len], "/bluray") ) continue;
		int loop = 0, user = 0, rw = 0, noauto = 0;
		char *op = opts, *ep = op + sizeof(opts)-1;
		while( op < ep && *op ) {
			char opt[BCTEXTLEN], *cp = opt;
			while( op < ep && *op && (*cp=*op++)!=',' ) ++cp;
			*cp = 0;
			if( !strcmp("loop", opt) ) loop = 1;
			else if( !strcmp("user", opt) ) user = 1;
			else if( !strcmp("noauto", opt) ) noauto = 1;
			else if( !strcmp("rw", opt) ) rw = 1;
		}
		if( loop && user && rw && noauto ) {
			strcpy(udfs, file);
			strcpy(mopts, "");
			strcpy(mntpt, mpnt);
			ret = 1;
			break;
		}
	}
	fclose(fp);
	return ret;
}

char *BD_BatchRenderJob::create_script(EDL *edl, ArrayList<Indexable *> *idxbls)
{
	char script[BCTEXTLEN];
	strcpy(script, edl_path);
	FILE *fp = 0;
	char *bp = strrchr(script,'/');
	int fd = -1;
	if( bp ) {
		strcpy(bp, "/bd.sh");
		fd = open(script, O_WRONLY+O_CREAT+O_TRUNC, 0755);
	}
	if( fd >= 0 )
		fp = fdopen(fd, "w");
	if( !fp ) {
		char err[BCTEXTLEN], msg[BCTEXTLEN];
		strerror_r(errno, err, sizeof(err));
		sprintf(msg, _("Unable to save: %s\n-- %s"), script, err);
		MainError::show_error(msg);
		return 0;
	}
	char udfs[BCTEXTLEN], mopts[BCTEXTLEN], mntpt[BCTEXTLEN];
	int is_usr_mnt = get_udfs_mount(udfs, mopts, mntpt);
	const char *exec_path = File::get_cinlib_path();
	fprintf(fp,"#!/bin/bash -ex\n");
	fprintf(fp,"dir=`dirname $0`\n");
	fprintf(fp,"PATH=$PATH:%s\n",exec_path);
	fprintf(fp,"mkdir -p $dir/udfs\n");
	fprintf(fp,"sz=`du -cb $dir/bd.m2ts* | tail -1 | sed -e 's/[ \t].*//'`\n");
	fprintf(fp,"blks=$((sz/2048 + 4096))\n");
	fprintf(fp,"rm -f %s\n", udfs);
	fprintf(fp,"mkudffs -b 2048 %s $blks\n", udfs);
	fprintf(fp,"mount %s%s\n", mopts, mntpt);
	fprintf(fp,"bdwrite %s $dir/bd.m2ts*\n",mntpt);
	fprintf(fp,"umount %s\n",mntpt);
	if( is_usr_mnt )
		fprintf(fp,"mv -f %s $dir/bd.udfs\n", udfs);
	fprintf(fp,"echo To burn bluray, load writable media and run:\n");
	fprintf(fp,"echo for WORM: growisofs -dvd-compat -Z /dev/bd=$dir/bd.udfs\n");
	fprintf(fp,"echo for RW:   dd if=$dir/bd.udfs of=/dev/bd bs=2048000\n");
	fprintf(fp,"kill $$\n");
	fprintf(fp,"\n");
	fclose(fp);
	return cstrdup(script);
}


CreateBD_Thread::CreateBD_Thread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->gui = 0;
	this->use_deinterlace = 0;
	this->use_scale = Rescale::none;
	this->use_histogram = 0;
	this->use_inverse_telecine = 0;
	this->use_wide_audio = 0;
	this->use_resize_tracks = 0;
	this->use_labeled = 0;
	this->use_farmed = 0;

	this->bd_size = BD_SIZE;
	this->bd_width = BD_WIDTH;
	this->bd_height = BD_HEIGHT;
	this->bd_aspect_width = BD_WIDE_ASPECT_WIDTH;
	this->bd_aspect_height = BD_WIDE_ASPECT_HEIGHT;
	this->bd_framerate = BD_FRAMERATE;
	this->bd_samplerate = BD_SAMPLERATE;
	this->bd_max_bitrate = BD_MAX_BITRATE;
	this->bd_kaudio_rate = BD_KAUDIO_RATE;
	this->max_w = this->max_h = 0;
}

CreateBD_Thread::~CreateBD_Thread()
{
	close_window();
}

int CreateBD_Thread::create_bd_jobs(ArrayList<BatchRenderJob*> *jobs, const char *asset_dir)
{
	EDL *edl = mwindow->edl;
	if( !edl || !edl->session ) {
		char msg[BCTEXTLEN];
		sprintf(msg, _("No EDL/Session"));
		MainError::show_error(msg);
		return 1;
	}
	EDLSession *session = edl->session;

	double total_length = edl->tracks->total_length();
	if( total_length <= 0 ) {
		char msg[BCTEXTLEN];
		sprintf(msg, _("No content: %s"), asset_title);
		MainError::show_error(msg);
		return 1;
	}

	if( mkdir(asset_dir, 0777) ) {
		char err[BCTEXTLEN], msg[BCTEXTLEN];
		strerror_r(errno, err, sizeof(err));
		sprintf(msg, _("Unable to create directory: %s\n-- %s"), asset_dir, err);
		MainError::show_error(msg);
		return 1;
	}

	double old_samplerate = session->sample_rate;
	double old_framerate = session->frame_rate;

	session->video_channels = BD_STREAMS;
	session->video_tracks = BD_STREAMS;
	session->frame_rate = bd_framerate;
	session->output_w = bd_width;
	session->output_h = bd_height;
	session->aspect_w = bd_aspect_width;
	session->aspect_h = bd_aspect_height;
	session->sample_rate = bd_samplerate;
	session->audio_channels = session->audio_tracks =
		use_wide_audio ? BD_WIDE_CHANNELS : BD_CHANNELS;
	session->interlace_mode = bd_interlace_mode;

	session->audio_channels = session->audio_tracks =
		!use_wide_audio ? BD_CHANNELS : BD_WIDE_CHANNELS;
	for( int i=0; i<MAX_CHANNELS; ++i )
		session->achannel_positions[i] = default_audio_channel_position(i, session->audio_channels);
	int audio_mapping = edl->tracks->recordable_audio_tracks() == BD_WIDE_CHANNELS &&
		!use_wide_audio ? MWindow::AUDIO_5_1_TO_2 : MWindow::AUDIO_1_TO_1;
	mwindow->remap_audio(audio_mapping);

	double new_samplerate = session->sample_rate;
	double new_framerate = session->frame_rate;
	edl->retrack();
	edl->rechannel();
	edl->resample(old_samplerate, new_samplerate, TRACK_AUDIO);
	edl->resample(old_framerate, new_framerate, TRACK_VIDEO);

	int64_t aud_size = ((bd_kaudio_rate * total_length)/8 + 1000-1) * 1000;
	int64_t vid_size = bd_size*0.96 - aud_size;
	int64_t vid_bitrate = (vid_size * 8) / total_length;
	vid_bitrate /= 1000;  vid_bitrate *= 1000;
	if( vid_bitrate > bd_max_bitrate ) vid_bitrate = bd_max_bitrate;

	char xml_filename[BCTEXTLEN];
	sprintf(xml_filename, "%s/bd.xml", asset_dir);
	FileXML xml_file;
	edl->save_xml(&xml_file, xml_filename);
	xml_file.terminate_string();
	if( xml_file.write_to_file(xml_filename) ) {
		char msg[BCTEXTLEN];
		sprintf(msg, _("Unable to save: %s"), xml_filename);
		MainError::show_error(msg);
		return 1;
	}

	BatchRenderJob *job = new BD_BatchRenderJob(mwindow->preferences,
		use_labeled, use_farmed);
	jobs->append(job);
	strcpy(&job->edl_path[0], xml_filename);
	Asset *asset = job->asset;

	asset->layers = BD_STREAMS;
	asset->frame_rate = session->frame_rate;
	asset->width = session->output_w;
	asset->height = session->output_h;
	asset->aspect_ratio = session->aspect_w / session->aspect_h;
	asset->interlace_mode = session->interlace_mode;

	char option_path[BCTEXTLEN];
	sprintf(&asset->path[0],"%s/bd.m2ts", asset_dir);
	asset->format = FILE_FFMPEG;
	strcpy(asset->fformat, "m2ts");

	asset->audio_data = 1;
	strcpy(asset->acodec, "bluray.m2ts");
	//mwindow->defaults->get("DEFAULT_BLURAY_ACODEC", asset->acodec);
	FFMPEG::set_option_path(option_path, "audio/%s", asset->acodec);
	FFMPEG::load_options(option_path, asset->ff_audio_options,
			 sizeof(asset->ff_audio_options));
	asset->ff_audio_bitrate = bd_kaudio_rate * 1000;

	asset->video_data = 1;
	strcpy(asset->vcodec, "bluray.m2ts");
	//mwindow->defaults->get("DEFAULT_BLURAY_VCODEC", asset->vcodec);
	FFMPEG::set_option_path(option_path, "video/%s", asset->vcodec);
	FFMPEG::load_options(option_path, asset->ff_video_options,
		 sizeof(asset->ff_video_options));
	const char *opts = 0;
	switch( asset->interlace_mode ) {
	case ILACE_MODE_TOP_FIRST:    opts = ":tff\n";  break;
	case ILACE_MODE_BOTTOM_FIRST: opts = ":bff\n";  break;
	}
	if( opts ) {
		int len = strlen(asset->ff_video_options);
		char *cp = asset->ff_video_options + len-1;
		strncpy(cp, opts, sizeof(asset->ff_video_options)-len);
	}
	asset->ff_video_bitrate = vid_bitrate;
	asset->ff_video_quality = -1;
	return 0;
}

void CreateBD_Thread::handle_close_event(int result)
{
	if( result ) return;
	mwindow->defaults->update("WORK_DIRECTORY", tmp_path);
	mwindow->batch_render->load_defaults(mwindow->defaults);
	mwindow->undo->update_undo_before();
	KeyFrame keyframe;  char data[BCTEXTLEN];
	if( use_deinterlace ) {
		sprintf(data,"<DEINTERLACE MODE=1>");
		keyframe.set_data(data);
		insert_video_plugin("Deinterlace", &keyframe);
	}
	if( use_inverse_telecine ) {
		sprintf(data,"<IVTC FRAME_OFFSET=0 FIRST_FIELD=0 "
			"AUTOMATIC=1 AUTO_THRESHOLD=2.0e+00 PATTERN=2>");
		keyframe.set_data(data);
		insert_video_plugin("Inverse Telecine", &keyframe);
	}
	if( use_scale != Rescale::none ) {
		double bd_aspect = bd_aspect_height > 0 ? bd_aspect_width / bd_aspect_height : 1;

		Tracks *tracks = mwindow->edl->tracks;
		for( Track *vtrk=tracks->first; vtrk; vtrk=vtrk->next ) {
			if( vtrk->data_type != TRACK_VIDEO ) continue;
			if( !vtrk->record ) continue;
			vtrk->expand_view = 1;
			PluginSet *plugin_set = new PluginSet(mwindow->edl, vtrk);
			vtrk->plugin_set.append(plugin_set);
			Edits *edits = vtrk->edits;
			for( Edit *edit=edits->first; edit; edit=edit->next ) {
				Indexable *indexable = edit->get_source();
				if( !indexable ) continue;
				Rescale in(indexable);
				Rescale out(bd_width, bd_height, bd_aspect);
				float src_w, src_h, dst_w, dst_h;
				in.rescale(out,use_scale, src_w,src_h, dst_w,dst_h);
				sprintf(data,"<SCALERATIO TYPE=%d"
					" IN_W=%d IN_H=%d IN_ASPECT_RATIO=%f"
					" OUT_W=%d OUT_H=%d OUT_ASPECT_RATIO=%f"
					" SRC_X=%f SRC_Y=%f SRC_W=%f SRC_H=%f"
					" DST_X=%f DST_Y=%f DST_W=%f DST_H=%f>", use_scale,
					in.w, in.h, in.aspect, out.w, out.h, out.aspect,
					0., 0., src_w, src_h, 0., 0., dst_w, dst_h);
				keyframe.set_data(data);
				plugin_set->insert_plugin(_("Scale Ratio"),
					edit->startproject, edit->length,
					PLUGIN_STANDALONE, 0, &keyframe, 0);
			}
			vtrk->optimize();
		}
	}
	if( use_resize_tracks )
		resize_tracks();
	if( use_histogram ) {
#if 0
		sprintf(data, "<HISTOGRAM OUTPUT_MIN_0=0 OUTPUT_MAX_0=1 "
			"OUTPUT_MIN_1=0 OUTPUT_MAX_1=1 "
			"OUTPUT_MIN_2=0 OUTPUT_MAX_2=1 "
			"OUTPUT_MIN_3=0 OUTPUT_MAX_3=1 "
			"AUTOMATIC=0 THRESHOLD=9.0-01 PLOT=0 SPLIT=0>"
			"<POINTS></POINTS><POINTS></POINTS><POINTS></POINTS>"
			"<POINTS><POINT X=6.0e-02 Y=0>"
				"<POINT X=9.4e-01 Y=1></POINTS>");
#else
		sprintf(data, "<HISTOGRAM AUTOMATIC=0 THRESHOLD=1.0e-01 "
			"PLOT=0 SPLIT=0 W=440 H=500 PARADE=0 MODE=3 "
			"LOW_OUTPUT_0=0 HIGH_OUTPUT_0=1 LOW_INPUT_0=0 HIGH_INPUT_0=1 GAMMA_0=1 "
			"LOW_OUTPUT_1=0 HIGH_OUTPUT_1=1 LOW_INPUT_1=0 HIGH_INPUT_1=1 GAMMA_1=1 "
			"LOW_OUTPUT_2=0 HIGH_OUTPUT_2=1 LOW_INPUT_2=0 HIGH_INPUT_2=1 GAMMA_2=1 "
			"LOW_OUTPUT_3=0 HIGH_OUTPUT_3=1 LOW_INPUT_3=0.044 HIGH_INPUT_3=0.956 "
			"GAMMA_3=1>");
#endif
		keyframe.set_data(data);
		insert_video_plugin("Histogram", &keyframe);
	}

	char asset_dir[BCTEXTLEN], jobs_path[BCTEXTLEN];
	snprintf(asset_dir, sizeof(asset_dir), "%s/%s", tmp_path, asset_title);
	snprintf(jobs_path, sizeof(jobs_path), "%s/bd.jobs", asset_dir);
	mwindow->batch_render->reset(jobs_path);
	int ret = create_bd_jobs(&mwindow->batch_render->jobs, asset_dir);
	mwindow->undo->update_undo_after(_("create bd"), LOAD_ALL);
	mwindow->resync_guis();
	if( ret ) return;
	mwindow->batch_render->save_jobs();
	mwindow->batch_render->start(-use_farmed, -use_labeled);
}

BC_Window* CreateBD_Thread::new_gui()
{
	strcpy(tmp_path, "/tmp");
	mwindow->defaults->get("WORK_DIRECTORY", tmp_path);
	memset(asset_title,0,sizeof(asset_title));
	time_t dt;	time(&dt);
	struct tm dtm;	localtime_r(&dt, &dtm);
	sprintf(asset_title, "bd_%02d%02d%02d-%02d%02d%02d",
		dtm.tm_year+1900, dtm.tm_mon+1, dtm.tm_mday,
		dtm.tm_hour, dtm.tm_min, dtm.tm_sec);
	use_deinterlace = 0;
	use_scale = Rescale::none;
	use_histogram = 0;
	use_inverse_telecine = 0;
	use_wide_audio = 0;
	use_resize_tracks = 0;
	use_labeled = 0;
	use_farmed = 0;
	use_standard = !strcmp(mwindow->default_std(),"NTSC") ?
		 BD_1920x1080_2997i : BD_1920x1080_25i;
	bd_size = BD_SIZE;
	bd_width = BD_WIDTH;
	bd_height = BD_HEIGHT;
	bd_aspect_width = BD_WIDE_ASPECT_WIDTH;
	bd_aspect_height = BD_WIDE_ASPECT_HEIGHT;
	bd_framerate = BD_FRAMERATE;
	bd_samplerate = BD_SAMPLERATE;
	bd_max_bitrate = BD_MAX_BITRATE;
	bd_kaudio_rate = BD_KAUDIO_RATE;
	bd_interlace_mode = BD_INTERLACE_MODE;
	max_w = 0; max_h = 0;

	int has_standard = -1;
	if( mwindow->edl ) {
		EDLSession *session = mwindow->edl->session;
// match the session to any known standard
		for( int i=0; i<(int)(sizeof(bd_formats)/sizeof(bd_formats[0])); ++i ) {
			if( !EQUIV(session->frame_rate, bd_formats[i].framerate) ) continue;
			if( session->output_w != bd_formats[i].w ) continue;
			if( session->output_h != bd_formats[i].h ) continue;
			has_standard = i;  break;
		}
	}
	if( has_standard >= 0 )
		use_standard = has_standard;

	option_presets();
	int scr_x = mwindow->gui->get_screen_x(0, -1);
	int scr_w = mwindow->gui->get_screen_w(0, -1);
	int scr_h = mwindow->gui->get_screen_h(0, -1);
	int w = 500, h = 290;
	int x = scr_x + scr_w/2 - w/2, y = scr_h/2 - h/2;

	gui = new CreateBD_GUI(this, x, y, w, h);
	gui->create_objects();

	if( getuid() != 0 ) {
		mwindow->show_warning(
			&mwindow->preferences->bd_warn_root,
			_("Must be root to mount UDFS images\n"));
	}
	return gui;
}


CreateBD_OK::CreateBD_OK(CreateBD_GUI *gui, int x, int y)
 : BC_OKButton(x, y)
{
	this->gui = gui;
	set_tooltip(_("end setup, start batch render"));
}

CreateBD_OK::~CreateBD_OK()
{
}

int CreateBD_OK::button_press_event()
{
	if(get_buttonpress() == 1 && is_event_win() && cursor_inside()) {
		gui->set_done(0);
		return 1;
	}
	return 0;
}

int CreateBD_OK::keypress_event()
{
	return 0;
}


CreateBD_Cancel::CreateBD_Cancel(CreateBD_GUI *gui, int x, int y)
 : BC_CancelButton(x, y)
{
	this->gui = gui;
}

CreateBD_Cancel::~CreateBD_Cancel()
{
}

int CreateBD_Cancel::button_press_event()
{
	if(get_buttonpress() == 1 && is_event_win() && cursor_inside()) {
		gui->set_done(1);
		return 1;
	}
	return 0;
}


CreateBD_DiskSpace::CreateBD_DiskSpace(CreateBD_GUI *gui, int x, int y)
 : BC_Title(x, y, "", MEDIUMFONT, GREEN)
{
	this->gui = gui;
}

CreateBD_DiskSpace::~CreateBD_DiskSpace()
{
}

int64_t CreateBD_DiskSpace::tmp_path_space()
{
	const char *path = gui->thread->tmp_path;
	if( access(path,R_OK+W_OK) ) return 0;
	struct statfs sfs;
	if( statfs(path, &sfs) ) return 0;
	return (int64_t)sfs.f_bsize * sfs.f_bfree;
}

void CreateBD_DiskSpace::update()
{
	static const char *suffix[] = { "", "KB", "MB", "GB", "TB", "PB" };
	int64_t disk_space = tmp_path_space();
	double media_size = 100e9, msz = 0, m = 1;
	char sfx[BCSTRLEN];
	if( sscanf(gui->media_size->get_text(), "%lf%s", &msz, sfx) == 2 ) {
		int i = sizeof(suffix)/sizeof(suffix[0]);
		while( --i >= 0 && strcmp(sfx, suffix[i]) );
		while( --i >= 0 ) m *= 1000;
		media_size = msz * m;
	}
	int color = disk_space < media_size*2 ? RED : GREEN;
	int i = 0;
	for( int64_t space=disk_space; i<5 && (space/=1000)>0; disk_space=space, ++i );
	char text[BCTEXTLEN];
	sprintf(text, "%s%3jd%s", _("disk space: "), disk_space, suffix[i]);
	gui->disk_space->BC_Title::update(text);
	gui->disk_space->set_color(color);
}

CreateBD_TmpPath::CreateBD_TmpPath(CreateBD_GUI *gui, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, -(int)sizeof(gui->thread->tmp_path),
		gui->thread->tmp_path, 1, MEDIUMFONT)
{
	this->gui = gui;
}

CreateBD_TmpPath::~CreateBD_TmpPath()
{
}

int CreateBD_TmpPath::handle_event()
{
	get_text();
	gui->disk_space->update();
	return 1;
}


CreateBD_AssetTitle::CreateBD_AssetTitle(CreateBD_GUI *gui, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, -(int)sizeof(gui->thread->asset_title),
		gui->thread->asset_title, 1, MEDIUMFONT)
{
	this->gui = gui;
}

CreateBD_AssetTitle::~CreateBD_AssetTitle()
{
}

int CreateBD_AssetTitle::handle_event()
{
	get_text();
	return 1;
}

CreateBD_Deinterlace::CreateBD_Deinterlace(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_deinterlace, _("Deinterlace"))
{
	this->gui = gui;
}

CreateBD_Deinterlace::~CreateBD_Deinterlace()
{
}

int CreateBD_Deinterlace::handle_event()
{
	if( get_value() ) {
		gui->need_inverse_telecine->set_value(0);
		gui->thread->use_inverse_telecine = 0;
	}
	return BC_CheckBox::handle_event();
}


CreateBD_InverseTelecine::CreateBD_InverseTelecine(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_inverse_telecine, _("Inverse Telecine"))
{
	this->gui = gui;
}

CreateBD_InverseTelecine::~CreateBD_InverseTelecine()
{
}

int CreateBD_InverseTelecine::handle_event()
{
	if( get_value() ) {
		gui->need_deinterlace->set_value(0);
		gui->thread->use_deinterlace = 0;
	}
	return BC_CheckBox::handle_event();
}


CreateBD_ResizeTracks::CreateBD_ResizeTracks(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_resize_tracks, _("Resize Tracks"))
{
	this->gui = gui;
}

CreateBD_ResizeTracks::~CreateBD_ResizeTracks()
{
}


CreateBD_Histogram::CreateBD_Histogram(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_histogram, _("Histogram"))
{
	this->gui = gui;
}

CreateBD_Histogram::~CreateBD_Histogram()
{
}

CreateBD_LabelChapters::CreateBD_LabelChapters(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_labeled, _("Chapters at Labels"))
{
	this->gui = gui;
}

CreateBD_LabelChapters::~CreateBD_LabelChapters()
{
}

CreateBD_UseRenderFarm::CreateBD_UseRenderFarm(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_farmed, _("Use render farm"))
{
	this->gui = gui;
}

CreateBD_UseRenderFarm::~CreateBD_UseRenderFarm()
{
}


CreateBD_WideAudio::CreateBD_WideAudio(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_wide_audio, _("Audio 5.1"))
{
	this->gui = gui;
}

CreateBD_WideAudio::~CreateBD_WideAudio()
{
}


CreateBD_GUI::CreateBD_GUI(CreateBD_Thread *thread, int x, int y, int w, int h)
 : BC_Window(_(PROGRAM_NAME ": Create BD"), x, y, w, h, 50, 50, 1, 0, 1)
{
	this->thread = thread;
	at_x = at_y = tmp_x = tmp_y = 0;
	ok_x = ok_y = ok_w = ok_h = 0;
	cancel_x = cancel_y = cancel_w = cancel_h = 0;
	asset_title = 0;
	tmp_path = 0;
	btmp_path = 0;
	disk_space = 0;
	standard = 0;
	scale = 0;
	need_deinterlace = 0;
	need_inverse_telecine = 0;
	need_resize_tracks = 0;
	need_histogram = 0;
	non_standard = 0;
	need_wide_audio = 0;
	need_labeled = 0;
	need_farmed = 0;
	ok = 0;
	cancel = 0;
}

CreateBD_GUI::~CreateBD_GUI()
{
}

void CreateBD_GUI::create_objects()
{
	lock_window("CreateBD_GUI::create_objects");
	int pady = BC_TextBox::calculate_h(this, MEDIUMFONT, 0, 1) + 5;
	int padx = BC_Title::calculate_w(this, (char*)"X", MEDIUMFONT);
	int x = padx/2, y = pady/2;
	BC_Title *title = new BC_Title(x, y, _("Title:"), MEDIUMFONT, YELLOW);
	add_subwindow(title);
	at_x = x + title->get_w();  at_y = y;
	asset_title = new CreateBD_AssetTitle(this, at_x, at_y, get_w()-at_x-10);
	add_subwindow(asset_title);
	y += title->get_h() + pady/2;
	title = new BC_Title(x, y, _("Work path:"), MEDIUMFONT, YELLOW);
	add_subwindow(title);
	tmp_x = x + title->get_w();  tmp_y = y;
	tmp_path = new CreateBD_TmpPath(this, tmp_x, tmp_y,  get_w()-tmp_x-35);
	add_subwindow(tmp_path);
	btmp_path = new BrowseButton(thread->mwindow->theme, this, tmp_path,
		tmp_x+tmp_path->get_w(), tmp_y, "/tmp",
		_("Work path"), _("Select a Work directory:"), 1);
	add_subwindow(btmp_path);
	y += title->get_h() + pady/2;
	disk_space = new CreateBD_DiskSpace(this, x, y);
	add_subwindow(disk_space);
	int x0 = get_w() - 170;
	title = new BC_Title(x0, y, _("Media:"), MEDIUMFONT, YELLOW);
	add_subwindow(title);
	int x1 =  x0+title->get_w()+padx;
	media_size = new CreateBD_MediaSize(this, x1, y);
	media_size->create_objects();
	media_sizes.append(new BC_ListBoxItem("25GB"));
	media_sizes.append(new BC_ListBoxItem("50GB"));
	media_size->update_list(&media_sizes);
	media_size->update(media_sizes[0]->get_text());
	disk_space->update();
	y += disk_space->get_h() + pady/2;
	title = new BC_Title(x, y, _("Format:"), MEDIUMFONT, YELLOW);
	add_subwindow(title);
	standard = new CreateBD_Format(this, title->get_w() + padx, y);
	add_subwindow(standard);
	standard->create_objects();
	standard->set_text(bd_formats[thread->use_standard].name);
	x0 -= 30;
	title = new BC_Title(x0, y, _("Scale:"), MEDIUMFONT, YELLOW);
	add_subwindow(title);
	x1 = x0+title->get_w()+padx;
	scale = new CreateBD_Scale(this, x1, y);
	add_subwindow(scale);
	scale->create_objects();
	y += standard->get_h() + pady/2;
	x1 = x;  int y1 = y;
	need_deinterlace = new CreateBD_Deinterlace(this, x1, y);
	add_subwindow(need_deinterlace);
	y += need_deinterlace->get_h() + pady/2;
	need_histogram = new CreateBD_Histogram(this, x1, y);
	add_subwindow(need_histogram);
	y += need_histogram->get_h() + pady/2;
	non_standard = new BC_Title(x1, y+5, "", MEDIUMFONT, RED);
	add_subwindow(non_standard);
	x1 += 160;  y = y1;
	need_inverse_telecine = new CreateBD_InverseTelecine(this, x1, y);
	add_subwindow(need_inverse_telecine);
	y += need_inverse_telecine->get_h() + pady/2;
	need_wide_audio = new CreateBD_WideAudio(this, x1, y);
	add_subwindow(need_wide_audio);
	y += need_wide_audio->get_h() + pady/2;
	need_resize_tracks = new CreateBD_ResizeTracks(this, x1, y);
	add_subwindow(need_resize_tracks);
	x1 += 160;  y = y1;
	need_labeled = new CreateBD_LabelChapters(this, x1, y);
	add_subwindow(need_labeled);
	y += need_labeled->get_h() + pady/2;
	need_farmed = new CreateBD_UseRenderFarm(this, x1, y);
	add_subwindow(need_farmed);
	ok_w = BC_OKButton::calculate_w();
	ok_h = BC_OKButton::calculate_h();
	ok_x = 10;
	ok_y = get_h() - ok_h - 10;
	ok = new CreateBD_OK(this, ok_x, ok_y);
	add_subwindow(ok);
	cancel_w = BC_CancelButton::calculate_w();
	cancel_h = BC_CancelButton::calculate_h();
	cancel_x = get_w() - cancel_w - 10,
	cancel_y = get_h() - cancel_h - 10;
	cancel = new CreateBD_Cancel(this, cancel_x, cancel_y);
	add_subwindow(cancel);
	show_window();
	unlock_window();
}

int CreateBD_GUI::resize_event(int w, int h)
{
	asset_title->reposition_window(at_x, at_y, get_w()-at_x-10);
	tmp_path->reposition_window(tmp_x, tmp_y,  get_w()-tmp_x-35);
	btmp_path->reposition_window(tmp_x+tmp_path->get_w(), tmp_y);
	ok_y = h - ok_h - 10;
	ok->reposition_window(ok_x, ok_y);
	cancel_x = w - cancel_w - 10,
	cancel_y = h - cancel_h - 10;
	cancel->reposition_window(cancel_x, cancel_y);
	return 0;
}

int CreateBD_GUI::translation_event()
{
	return 1;
}

int CreateBD_GUI::close_event()
{
	set_done(1);
	return 1;
}

void CreateBD_GUI::update()
{
	scale->set_value(thread->use_scale);
	need_deinterlace->set_value(thread->use_deinterlace);
	need_inverse_telecine->set_value(thread->use_inverse_telecine);
	need_resize_tracks->set_value(thread->use_resize_tracks);
	need_histogram->set_value(thread->use_histogram);
	need_wide_audio->set_value(thread->use_wide_audio);
	need_labeled->set_value(thread->use_labeled);
	need_farmed->set_value(thread->use_farmed);
}

int CreateBD_Thread::
insert_video_plugin(const char *title, KeyFrame *default_keyframe)
{
	Tracks *tracks = mwindow->edl->tracks;
	for( Track *vtrk=tracks->first; vtrk; vtrk=vtrk->next ) {
		if( vtrk->data_type != TRACK_VIDEO ) continue;
		if( !vtrk->record ) continue;
		vtrk->expand_view = 1;
		PluginSet *plugin_set = new PluginSet(mwindow->edl, vtrk);
		vtrk->plugin_set.append(plugin_set);
		Edits *edits = vtrk->edits;
		for( Edit *edit=edits->first; edit; edit=edit->next ) {
			plugin_set->insert_plugin(_(title),
				edit->startproject, edit->length,
				PLUGIN_STANDALONE, 0, default_keyframe, 0);
		}
		vtrk->optimize();
	}
	return 0;
}

int CreateBD_Thread::
resize_tracks()
{
	Tracks *tracks = mwindow->edl->tracks;
	int trk_w = max_w, trk_h = max_h;
	if( trk_w < bd_width ) trk_w = bd_width;
	if( trk_h < bd_height ) trk_h = bd_height;
	for( Track *vtrk=tracks->first; vtrk; vtrk=vtrk->next ) {
		if( vtrk->data_type != TRACK_VIDEO ) continue;
		if( !vtrk->record ) continue;
		vtrk->track_w = trk_w;
		vtrk->track_h = trk_h;
	}
	return 0;
}

int CreateBD_Thread::
option_presets()
{
// reset only probed options
	use_deinterlace = 0;
	use_scale = Rescale::none;
	use_resize_tracks = 0;
	use_wide_audio = 0;
	use_labeled = 0;
	use_farmed = 0;

	if( !mwindow->edl ) return 1;

	bd_width = bd_formats[use_standard].w;
	bd_height = bd_formats[use_standard].h;
	bd_framerate = bd_formats[use_standard].framerate;
	int wide = bd_formats[use_standard].wide;
	bd_aspect_width  = wide < 0 ? 1. :
		wide > 0 ? BD_WIDE_ASPECT_WIDTH  : BD_ASPECT_WIDTH;
	bd_aspect_height = wide < 0 ? 1. :
		wide > 0 ? BD_WIDE_ASPECT_HEIGHT : BD_ASPECT_HEIGHT;
	bd_interlace_mode = bd_formats[use_standard].interlaced;
	double bd_aspect = bd_aspect_width / bd_aspect_height;

	Tracks *tracks = mwindow->edl->tracks;
	max_w = 0;  max_h = 0;
	int has_deinterlace = 0, has_scale = 0;
	for( Track *trk=tracks->first; trk; trk=trk->next ) {
		if( !trk->record ) continue;
		Edits *edits = trk->edits;
		switch( trk->data_type ) {
		case TRACK_VIDEO:
			for( Edit *edit=edits->first; edit; edit=edit->next ) {
				if( edit->silence() ) continue;
				Indexable *indexable = edit->get_source();
				int w = indexable->get_w();
				if( w > max_w ) max_w = w;
				if( w != bd_width ) use_scale = Rescale::scaled;
				int h = indexable->get_h();
				if( h > max_h ) max_h = h;
				if( h != bd_height ) use_scale = Rescale::scaled;
				float aw, ah;
				MWindow::create_aspect_ratio(aw, ah, w, h);
				double aspect = ah > 0 ? aw / ah : 1;
				if( wide >= 0 && !EQUIV(aspect, bd_aspect) )
					use_scale = Rescale::scaled;
			}
			for( int i=0; i<trk->plugin_set.size(); ++i ) {
				for( Plugin *plugin = (Plugin*)trk->plugin_set[i]->first;
						plugin; plugin=(Plugin*)plugin->next ) {
					if( !strcmp(plugin->title, "Deinterlace") )
						has_deinterlace = 1;
					if( !strcmp(plugin->title, "Auto Scale") ||
					    !strcmp(plugin->title, "Scale Ratio") ||
					    !strcmp(plugin->title, "Scale") )
						has_scale = 1;
				}
			}
			break;
		}
	}
	if( has_scale )
		use_scale = Rescale::none;
	if( use_scale != Rescale::none ) {
		if( max_w != bd_width ) use_resize_tracks = 1;
		if( max_h != bd_height ) use_resize_tracks = 1;
	}
	for( Track *trk=tracks->first; trk && !use_resize_tracks; trk=trk->next ) {
		if( !trk->record ) continue;
		switch( trk->data_type ) {
		case TRACK_VIDEO:
			if( trk->track_w != max_w ) use_resize_tracks = 1;
			if( trk->track_h != max_h ) use_resize_tracks = 1;
			break;
		}
	}
	if( !has_deinterlace && max_h > 2*bd_height ) use_deinterlace = 1;

	if( tracks->recordable_audio_tracks() == BD_WIDE_CHANNELS )
		use_wide_audio = 1;

	return 0;
}


CreateBD_FormatItem::CreateBD_FormatItem(CreateBD_Format *popup,
		int standard, const char *name)
 : BC_MenuItem(name)
{
	this->popup = popup;
	this->standard = standard;
}

CreateBD_FormatItem::~CreateBD_FormatItem()
{
}

int CreateBD_FormatItem::handle_event()
{
	const char *text = get_text();
	int standard = this->standard;
	if( standard < 0 ) {
		BC_SubMenu *submenu = get_submenu();
		CreateBD_FormatItem *sub_item0 =
			(CreateBD_FormatItem *)submenu->get_item(0);
		standard = sub_item0->standard;
		text = sub_item0->get_text();
	}
	popup->gui->thread->use_standard = standard;
	popup->set_text(text);
	int n = strlen(text)-1;
	int not_standard = n >= 0 && text[n] == '*' ? 1 : 0;
	popup->gui->non_standard->update(not_standard ? _("* non-standard format") : "", 0);
	return popup->handle_event();
}


CreateBD_Format::CreateBD_Format(CreateBD_GUI *gui, int x, int y)
 : BC_PopupMenu(x, y, 180, bd_formats[gui->thread->use_standard].name, 1)
{
	this->gui = gui;
}

CreateBD_Format::~CreateBD_Format()
{
}

void CreateBD_Format::create_objects()
{
	BC_SubMenu *submenu = 0;
	CreateBD_FormatItem *item = 0;
	int ww = 0, hh = 0;
	for( int i=0; i<(int)(sizeof(bd_formats)/sizeof(bd_formats[0])); ++i ) {
		if( ww != bd_formats[i].w || hh != bd_formats[i].h ) {
			ww = bd_formats[i].w;  hh = bd_formats[i].h;
			char string[BCSTRLEN];
			sprintf(string, "%dx%d", ww,hh);
			add_item(item = new CreateBD_FormatItem(this, -1, string));
			item->add_submenu(submenu = new BC_SubMenu());
		}
		submenu->add_submenuitem(new CreateBD_FormatItem(this, i, bd_formats[i].name));
	}
}

int CreateBD_Format::handle_event()
{
	gui->thread->option_presets();
	gui->update();
	return 1;
}


CreateBD_ScaleItem::CreateBD_ScaleItem(CreateBD_Scale *popup,
		int scale, const char *text)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->scale = scale;
}

CreateBD_ScaleItem::~CreateBD_ScaleItem()
{
}

int CreateBD_ScaleItem::handle_event()
{
	popup->gui->thread->use_scale = scale;
	popup->set_value(scale);
	return popup->handle_event();
}


CreateBD_Scale::CreateBD_Scale(CreateBD_GUI *gui, int x, int y)
 : BC_PopupMenu(x, y, 100, "", 1)
{
	this->gui = gui;
}

CreateBD_Scale::~CreateBD_Scale()
{
}

void CreateBD_Scale::create_objects()
{

	for( int i=0; i<(int)Rescale::n_scale_types; ++i ) {
		add_item(new CreateBD_ScaleItem(this, i, Rescale::scale_types[i]));
	}
	set_value(gui->thread->use_scale);
}

int CreateBD_Scale::handle_event()
{
	gui->update();
	return 1;
}


CreateBD_MediaSize::CreateBD_MediaSize(CreateBD_GUI *gui, int x, int y)
 : BC_PopupTextBox(gui, 0, 0, x, y, 70,50)
{
	this->gui = gui;
}

CreateBD_MediaSize::~CreateBD_MediaSize()
{
}

int CreateBD_MediaSize::handle_event()
{
	gui->disk_space->update();
	return 1;
}

