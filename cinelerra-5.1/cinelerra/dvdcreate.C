#include "asset.h"
#include "bchash.h"
#include "clip.h"
#include "dvdcreate.h"
#include "edl.h"
#include "edit.h"
#include "edits.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "keyframe.h"
#include "labels.h"
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


#define DVD_PAL_4x3	0
#define DVD_PAL_16x9	1
#define DVD_NTSC_4x3	2
#define DVD_NTSC_16x9	3

#define DVD_NORM_PAL	0
#define DVD_NORM_NTSC	1

#define DVD_ASPECT_4x3	0
#define DVD_ASPECT_16x9 1

static struct dvd_norm {
	const char *name;
	int w, h;
	double framerate;
} dvd_norms[] = {
	{ "PAL",  720,576, 25 },
	{ "NTSC", 720,480, 29.97 },
};

static struct dvd_aspect {
	int w, h;
} dvd_aspects[] = {
	{ 4, 3, },
	{ 16, 9, },
};

// DVD Creation

static struct dvd_format {
	int norm, aspect;
} dvd_formats[] = {
	{ DVD_NORM_PAL,  DVD_ASPECT_4x3, },
	{ DVD_NORM_PAL,  DVD_ASPECT_16x9, },
	{ DVD_NORM_NTSC, DVD_ASPECT_4x3, },
	{ DVD_NORM_NTSC, DVD_ASPECT_16x9, },
};

const int64_t CreateDVD_Thread::DVD_SIZE = 4700000000;
const int CreateDVD_Thread::DVD_STREAMS = 1;
const int CreateDVD_Thread::DVD_WIDTH = 720;
const int CreateDVD_Thread::DVD_HEIGHT = 480;
const double CreateDVD_Thread::DVD_ASPECT_WIDTH = 4.;
const double CreateDVD_Thread::DVD_ASPECT_HEIGHT = 3.;
const double CreateDVD_Thread::DVD_WIDE_ASPECT_WIDTH = 16.;
const double CreateDVD_Thread::DVD_WIDE_ASPECT_HEIGHT = 9.;
const double CreateDVD_Thread::DVD_FRAMERATE = 30000. / 1001.;
const int CreateDVD_Thread::DVD_MAX_BITRATE = 8000000;
const int CreateDVD_Thread::DVD_CHANNELS = 2;
const int CreateDVD_Thread::DVD_WIDE_CHANNELS = 6;
const double CreateDVD_Thread::DVD_SAMPLERATE = 48000;
const double CreateDVD_Thread::DVD_KAUDIO_RATE = 224;


CreateDVD_MenuItem::CreateDVD_MenuItem(MWindow *mwindow)
 : BC_MenuItem(_("DVD Render..."), _("Alt-d"), 'd')
{
	set_alt(1);
	this->mwindow = mwindow;
}

int CreateDVD_MenuItem::handle_event()
{
	mwindow->create_dvd->start();
	return 1;
}


DVD_BatchRenderJob::DVD_BatchRenderJob(Preferences *preferences,
		int labeled, int farmed, int standard, int muxed)
 : BatchRenderJob("DVD_JOB", preferences, labeled, farmed)
{
	this->standard = standard;
	this->muxed = muxed;

	chapter = -1;
	edl = 0;
	fp =0;
}

void DVD_BatchRenderJob::copy_from(DVD_BatchRenderJob *src)
{
	standard = src->standard;
	muxed = src->muxed;
	BatchRenderJob::copy_from(src);
}

DVD_BatchRenderJob *DVD_BatchRenderJob::copy()
{
	DVD_BatchRenderJob *t = new DVD_BatchRenderJob(preferences,
		labeled, farmed, standard, muxed);
	t->copy_from(this);
	return t;
}

void DVD_BatchRenderJob::load(FileXML *file)
{
	standard = file->tag.get_property("STANDARD", standard);
	muxed = file->tag.get_property("MUXED", muxed);
	BatchRenderJob::load(file);
}

void DVD_BatchRenderJob::save(FileXML *file)
{
	file->tag.set_property("STANDARD", standard);
	file->tag.set_property("MUXED", muxed);
	BatchRenderJob::save(file);
}

void DVD_BatchRenderJob::create_chapter(double pos)
{
	fprintf(fp,"%s", !chapter++? "\" chapters=\"" : ",");
	int secs = pos, mins = secs/60;
	int frms = (pos-secs) * edl->session->frame_rate;
	fprintf(fp,"%d:%02d:%02d.%d", mins/60, mins%60, secs%60, frms);
}

char *DVD_BatchRenderJob::create_script(EDL *edl, ArrayList<Indexable *> *idxbls)
{
	char script[BCTEXTLEN];
	strcpy(script, edl_path);
	this->edl = edl;
	this->fp = 0;
	char *bp = strrchr(script,'/');
	int fd = -1;
	if( bp ) {
		strcpy(bp, "/dvd.sh");
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

	fprintf(fp,"#!/bin/bash\n");
	fprintf(fp,"sdir=`dirname $0`\n");
	fprintf(fp,"dir=`cd \"$sdir\"; pwd`\n");
	fprintf(fp,"echo \"running %s\"\n", script);
	fprintf(fp,"\n");
	const char *exec_path = File::get_cinlib_path();
	fprintf(fp,"PATH=$PATH:%s\n",exec_path);
	int file_seq = farmed || labeled ? 1 : 0;
 	if( !muxed ) {
		if( file_seq ) {
			fprintf(fp, "cat > $dir/dvd.m2v $dir/dvd.m2v0*\n");
			fprintf(fp, "mplex -M -f 8 -o $dir/dvd.mpg $dir/dvd.m2v $dir/dvd.ac3\n");
			file_seq = 0;
		}
		else
			fprintf(fp, "mplex -f 8 -o $dir/dvd.mpg $dir/dvd.m2v $dir/dvd.ac3\n");
	}
	fprintf(fp,"rm -rf $dir/iso\n");
	fprintf(fp,"mkdir -p $dir/iso\n");
	fprintf(fp,"\n");
// dvdauthor ver 0.7.0 requires this to work
	int norm = dvd_formats[standard].norm;
	const char *name = dvd_norms[norm].name;
	fprintf(fp,"export VIDEO_FORMAT=%s\n", name);
	fprintf(fp,"dvdauthor -x - <<eof\n");
	fprintf(fp,"<dvdauthor dest=\"$dir/iso\">\n");
	fprintf(fp,"  <vmgm>\n");
	fprintf(fp,"    <fpc> jump title 1; </fpc>\n");
	fprintf(fp,"  </vmgm>\n");
	fprintf(fp,"  <titleset>\n");
	fprintf(fp,"    <titles>\n");
	char std[BCSTRLEN], *cp = std;
	for( const char *np=name; *np!=0; ++cp,++np) *cp = *np + 'a'-'A';
	*cp = 0;
	EDLSession *session = edl->session;
	fprintf(fp,"    <video format=\"%s\" aspect=\"%d:%d\" resolution=\"%dx%d\"/>\n",
		std, (int)session->aspect_w, (int)session->aspect_h,
		session->output_w, session->output_h);
	fprintf(fp,"    <audio format=\"ac3\" lang=\"en\"/>\n");
	fprintf(fp,"    <pgc>\n");
	int total_idxbls = !file_seq ? 1 : idxbls->size();
	int secs = 0;
	double vob_pos = 0;
	double total_length = edl->tracks->total_length();
	Label *label = edl->labels->first;
	for( int i=0; i<total_idxbls; ++i ) {
		Indexable *idxbl = idxbls->get(i);
		double video_length = idxbl->have_video() && idxbl->get_frame_rate() > 0 ?
			(double)idxbl->get_video_frames() / idxbl->get_frame_rate() : 0 ;
		double audio_length = idxbl->have_audio() && idxbl->get_sample_rate() > 0 ?
			(double)idxbl->get_audio_samples() / idxbl->get_sample_rate() : 0 ;
		double length = idxbl->have_video() && idxbl->have_audio() ?
				bmin(video_length, audio_length) :
			idxbl->have_video() ? video_length :
			idxbl->have_audio() ? audio_length : 0;
		fprintf(fp,"      <vob file=\"%s", !file_seq ? "$dir/dvd.mpg" : idxbl->path);
		chapter = 0;
		double vob_end = i+1>=total_idxbls ? total_length : vob_pos + length;
		if( labeled ) {
			while( label && label->position < vob_end ) {
				create_chapter(label->position - vob_pos);
				label = label->next;
			}
		}
		else {
			while( secs < vob_end ) {
				create_chapter(secs - vob_pos);
				secs += 10*60;  // ch every 10 minutes
			}
		}
		fprintf(fp,"\"/>\n");
		vob_pos = vob_end;
	}
	fprintf(fp,"    </pgc>\n");
	fprintf(fp,"    </titles>\n");
	fprintf(fp,"  </titleset>\n");
	fprintf(fp,"</dvdauthor>\n");
	fprintf(fp,"eof\n");
	fprintf(fp,"\n");
	fprintf(fp,"echo To burn dvd, load blank media and run:\n");
	fprintf(fp,"echo growisofs -dvd-compat -Z /dev/dvd -dvd-video $dir/iso\n");
	fprintf(fp,"kill $$\n");
	fprintf(fp,"\n");
	fclose(fp);
	return cstrdup(script);
}


CreateDVD_Thread::CreateDVD_Thread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->gui = 0;
	this->use_deinterlace = 0;
	this->use_scale = 0;
	this->use_histogram = 0;
	this->use_inverse_telecine = 0;
	this->use_wide_audio = 0;
	this->use_ffmpeg = 0;
	this->use_resize_tracks = 0;
	this->use_labeled = 0;
	this->use_farmed = 0;

	this->dvd_size = DVD_SIZE;
	this->dvd_width = DVD_WIDTH;
	this->dvd_height = DVD_HEIGHT;
	this->dvd_aspect_width = DVD_ASPECT_WIDTH;
	this->dvd_aspect_height = DVD_ASPECT_HEIGHT;
	this->dvd_framerate = DVD_FRAMERATE;
	this->dvd_samplerate = DVD_SAMPLERATE;
	this->dvd_max_bitrate = DVD_MAX_BITRATE;
	this->dvd_kaudio_rate = DVD_KAUDIO_RATE;
	this->max_w = this->max_h = 0;
}

CreateDVD_Thread::~CreateDVD_Thread()
{
	close_window();
}

int CreateDVD_Thread::create_dvd_jobs(ArrayList<BatchRenderJob*> *jobs, const char *asset_dir)
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

	session->video_channels = DVD_STREAMS;
	session->video_tracks = DVD_STREAMS;
	session->frame_rate = dvd_framerate;
	session->output_w = dvd_width;
	session->output_h = dvd_height;
	session->aspect_w = dvd_aspect_width;
	session->aspect_h = dvd_aspect_height;
	session->sample_rate = dvd_samplerate;
	session->audio_channels = session->audio_tracks =
		use_wide_audio ? DVD_WIDE_CHANNELS : DVD_CHANNELS;

	session->audio_channels = session->audio_tracks =
		!use_wide_audio ? DVD_CHANNELS : DVD_WIDE_CHANNELS;
	for( int i=0; i<MAX_CHANNELS; ++i )
		session->achannel_positions[i] = default_audio_channel_position(i, session->audio_channels);
	int audio_mapping = edl->tracks->recordable_audio_tracks() == DVD_WIDE_CHANNELS &&
		!use_wide_audio ? MWindow::AUDIO_5_1_TO_2 : MWindow::AUDIO_1_TO_1;
	mwindow->remap_audio(audio_mapping);

	double new_samplerate = session->sample_rate;
	double new_framerate = session->frame_rate;
	edl->retrack();
	edl->rechannel();
	edl->resample(old_samplerate, new_samplerate, TRACK_AUDIO);
	edl->resample(old_framerate, new_framerate, TRACK_VIDEO);

	int64_t aud_size = ((dvd_kaudio_rate * total_length)/8 + 1000-1) * 1000;
	int64_t vid_size = dvd_size*0.96 - aud_size;
	int64_t vid_bitrate = (vid_size * 8) / total_length;
	vid_bitrate /= 1000;  vid_bitrate *= 1000;
	if( vid_bitrate > dvd_max_bitrate )
		vid_bitrate = dvd_max_bitrate;

	char xml_filename[BCTEXTLEN];
	sprintf(xml_filename, "%s/dvd.xml", asset_dir);
	FileXML xml_file;
	edl->save_xml(&xml_file, xml_filename);
	xml_file.terminate_string();
	if( xml_file.write_to_file(xml_filename) ) {
		char msg[BCTEXTLEN];
		sprintf(msg, _("Unable to save: %s"), xml_filename);
		MainError::show_error(msg);
		return 1;
	}

	BatchRenderJob *job = new DVD_BatchRenderJob(mwindow->preferences,
		use_labeled, use_farmed, use_standard, 0);// use_ffmpeg);
	jobs->append(job);
	strcpy(&job->edl_path[0], xml_filename);
	Asset *asset = job->asset;

	asset->layers = DVD_STREAMS;
	asset->frame_rate = session->frame_rate;
	asset->width = session->output_w;
	asset->height = session->output_h;
	asset->aspect_ratio = session->aspect_w / session->aspect_h;

	if( use_ffmpeg ) {
		char option_path[BCTEXTLEN];
		sprintf(&asset->path[0],"%s/dvd.mpg", asset_dir);
		asset->format = FILE_FFMPEG;
		strcpy(asset->fformat, "dvd");
// if there are many renderfarm jobs, then there are small audio fragments of
// silence that are used at the end of a render to fill the last audio "block".
// this extra data gradually skews the audio/video sync.  Therefore, the audio
// is not rendered muxed for ffmpeg, and is remuxed as with mjpeg rendering.
// since this audio is in one file, the only fragment is at the end and is ok.
#if 0
		asset->audio_data = 1;
		asset->channels = session->audio_channels;
		asset->sample_rate = session->sample_rate;
		strcpy(asset->acodec, "dvd.dvd");
		FFMPEG::set_option_path(option_path, "audio/%s", asset->acodec);
		FFMPEG::load_options(option_path, asset->ff_audio_options,
			 sizeof(asset->ff_audio_options));
		asset->ff_audio_bitrate = dvd_kaudio_rate * 1000;

		asset->video_data = 1;
		strcpy(asset->vcodec, "dvd.dvd");
		FFMPEG::set_option_path(option_path, "video/%s", asset->vcodec);
		FFMPEG::load_options(option_path, asset->ff_video_options,
			 sizeof(asset->ff_video_options));
		asset->ff_video_bitrate = vid_bitrate;
		asset->ff_video_quality = -1;
		use_farmed = job->farmed;
#else
		asset->video_data = 1;
		strcpy(asset->vcodec, "raw.dvd");
		sprintf(&asset->path[0],"%s/dvd.m2v", asset_dir);
		FFMPEG::set_option_path(option_path, "video/%s", asset->vcodec);
		FFMPEG::load_options(option_path, asset->ff_video_options,
			 sizeof(asset->ff_video_options));
		asset->ff_video_bitrate = vid_bitrate;
		asset->ff_video_quality = -1;
		use_farmed = job->farmed;

		job = new BatchRenderJob(mwindow->preferences, 0, 0);
		jobs->append(job);
		strcpy(&job->edl_path[0], xml_filename);
		asset = job->asset;
		sprintf(&asset->path[0],"%s/dvd.ac3", asset_dir);
		asset->format = FILE_AC3;
		asset->audio_data = 1;
		asset->channels = session->audio_channels;
		asset->sample_rate = session->sample_rate;
		asset->ac3_bitrate = dvd_kaudio_rate;
#endif
	}
	else {
		sprintf(&asset->path[0],"%s/dvd.m2v", asset_dir);
		asset->video_data = 1;
		asset->format = FILE_VMPEG;
		asset->vmpeg_cmodel = BC_YUV420P;
		asset->vmpeg_fix_bitrate = 1;
		asset->vmpeg_bitrate = vid_bitrate;
		asset->vmpeg_quantization = 15;
		asset->vmpeg_iframe_distance = 15;
		asset->vmpeg_progressive = 0;
		asset->vmpeg_denoise = 0;
		asset->vmpeg_seq_codes = 0;
		asset->vmpeg_derivative = 2;
		asset->vmpeg_preset = 8;
		asset->vmpeg_field_order = 0;
		asset->vmpeg_pframe_distance = 0;
		use_farmed = job->farmed;
		job = new BatchRenderJob(mwindow->preferences, 0, 0);
		jobs->append(job);
		strcpy(&job->edl_path[0], xml_filename);
		asset = job->asset;

		sprintf(&asset->path[0],"%s/dvd.ac3", asset_dir);
		asset->audio_data = 1;
		asset->format = FILE_AC3;
		asset->channels = session->audio_channels;
		asset->sample_rate = session->sample_rate;
		asset->bits = 16;
		asset->byte_order = 0;
		asset->signed_ = 1;
		asset->header = 0;
		asset->dither = 0;
		asset->ac3_bitrate = dvd_kaudio_rate;
	}

	return 0;
}

void CreateDVD_Thread::handle_close_event(int result)
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
		double dvd_aspect = dvd_aspect_height > 0 ? dvd_aspect_width/dvd_aspect_height : 1;

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
				Rescale out(dvd_width, dvd_height, dvd_aspect);
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
	snprintf(jobs_path, sizeof(jobs_path), "%s/dvd.jobs", asset_dir);
	mwindow->batch_render->reset(jobs_path);
	int ret = create_dvd_jobs(&mwindow->batch_render->jobs, asset_dir);
	mwindow->undo->update_undo_after(_("create dvd"), LOAD_ALL);
	mwindow->resync_guis();
	if( ret ) return;
	mwindow->batch_render->save_jobs();
	mwindow->batch_render->start(-use_farmed, -use_labeled);
}

BC_Window* CreateDVD_Thread::new_gui()
{
	strcpy(tmp_path,"/tmp");
	mwindow->defaults->get("WORK_DIRECTORY", tmp_path);
	memset(asset_title,0,sizeof(asset_title));
	time_t dt;  time(&dt);
	struct tm dtm;  localtime_r(&dt, &dtm);
	sprintf(asset_title, "dvd_%02d%02d%02d-%02d%02d%02d",
		dtm.tm_year+1900, dtm.tm_mon+1, dtm.tm_mday,
		dtm.tm_hour, dtm.tm_min, dtm.tm_sec);
	use_deinterlace = 0;
	use_scale = Rescale::none;
	use_histogram = 0;
	use_inverse_telecine = 0;
	use_wide_audio = 0;
	use_ffmpeg = 0;
	use_resize_tracks = 0;
	use_labeled = 0;
	use_farmed = 0;
	use_standard = DVD_NTSC_4x3;

	dvd_size = DVD_SIZE;
	dvd_width = DVD_WIDTH;
	dvd_height = DVD_HEIGHT;
	dvd_aspect_width = DVD_ASPECT_WIDTH;
	dvd_aspect_height = DVD_ASPECT_HEIGHT;
	dvd_framerate = DVD_FRAMERATE;
	dvd_samplerate = DVD_SAMPLERATE;
	dvd_max_bitrate = DVD_MAX_BITRATE;
	dvd_kaudio_rate = DVD_KAUDIO_RATE;
	max_w = 0; max_h = 0;

	int has_standard = -1;
	if( mwindow->edl ) {
		EDLSession *session = mwindow->edl->session;
		double framerate = session->frame_rate;
		double aspect_ratio = session->aspect_h > 0 ?
			session->aspect_w / session->aspect_h > 0 : 1;
		int output_w = session->output_w, output_h = session->output_h;
// match the session to any known standard
		for( int i=0; i<(int)(sizeof(dvd_formats)/sizeof(dvd_formats[0])); ++i ) {
			int norm = dvd_formats[i].norm;
			if( !EQUIV(framerate, dvd_norms[norm].framerate) ) continue;
			if( output_w != dvd_norms[norm].w ) continue;
			if( output_h != dvd_norms[norm].h ) continue;
			int aspect = dvd_formats[i].aspect;
			double dvd_aspect_ratio =
				(double)dvd_aspects[aspect].w / dvd_aspects[aspect].h;
			if( !EQUIV(aspect_ratio, dvd_aspect_ratio) ) continue;
			has_standard = i;  break;
		}
		if( has_standard < 0 ) {
// or use the default standard
			if( !strcmp(mwindow->default_standard, "NTSC") ) has_standard = DVD_NTSC_4x3;
			else if( !strcmp(mwindow->default_standard, "PAL") ) has_standard = DVD_PAL_4x3;
		}
	}
	if( has_standard >= 0 )
		use_standard = has_standard;

	option_presets();
	int scr_x = mwindow->gui->get_screen_x(0, -1);
	int scr_w = mwindow->gui->get_screen_w(0, -1);
	int scr_h = mwindow->gui->get_screen_h(0, -1);
	int w = 520, h = 280;
	int x = scr_x + scr_w/2 - w/2, y = scr_h/2 - h/2;

	gui = new CreateDVD_GUI(this, x, y, w, h);
	gui->create_objects();
	return gui;
}


CreateDVD_OK::CreateDVD_OK(CreateDVD_GUI *gui, int x, int y)
 : BC_OKButton(x, y)
{
	this->gui = gui;
	set_tooltip(_("end setup, start batch render"));
}

CreateDVD_OK::~CreateDVD_OK()
{
}

int CreateDVD_OK::button_press_event()
{
	if(get_buttonpress() == 1 && is_event_win() && cursor_inside()) {
		gui->set_done(0);
		return 1;
	}
	return 0;
}

int CreateDVD_OK::keypress_event()
{
	return 0;
}


CreateDVD_Cancel::CreateDVD_Cancel(CreateDVD_GUI *gui, int x, int y)
 : BC_CancelButton(x, y)
{
	this->gui = gui;
}

CreateDVD_Cancel::~CreateDVD_Cancel()
{
}

int CreateDVD_Cancel::button_press_event()
{
	if(get_buttonpress() == 1 && is_event_win() && cursor_inside()) {
		gui->set_done(1);
		return 1;
	}
	return 0;
}


CreateDVD_DiskSpace::CreateDVD_DiskSpace(CreateDVD_GUI *gui, int x, int y)
 : BC_Title(x, y, "", MEDIUMFONT, GREEN)
{
	this->gui = gui;
}

CreateDVD_DiskSpace::~CreateDVD_DiskSpace()
{
}

int64_t CreateDVD_DiskSpace::tmp_path_space()
{
	const char *path = gui->thread->tmp_path;
	if( access(path,R_OK+W_OK) ) return 0;
	struct statfs sfs;
	if( statfs(path, &sfs) ) return 0;
	return (int64_t)sfs.f_bsize * sfs.f_bfree;
}

void CreateDVD_DiskSpace::update()
{
	static const char *suffix[] = { "", "KB", "MB", "GB", "TB", "PB" };
	int64_t disk_space = tmp_path_space();
	double media_size = 15e9, msz = 0, m = 1;
	char sfx[BCSTRLEN];
	if( sscanf(gui->media_size->get_text(), "%lf%s", &msz, sfx) == 2 ) {
		int i = sizeof(suffix)/sizeof(suffix[0]);
		while( --i >= 0 && strcmp(sfx, suffix[i]) );
		while( --i >= 0 ) m *= 1000;
		media_size = msz * m;
	}
	m = gui->thread->use_ffmpeg ? 2 : 3;
	int color = disk_space < media_size*m ? RED : GREEN;
	int i = 0;
	for( int64_t space=disk_space; i<5 && (space/=1000)>0; disk_space=space, ++i );
	char text[BCTEXTLEN];
	sprintf(text, "%s%3jd%s", _("disk space: "), disk_space, suffix[i]);
	gui->disk_space->BC_Title::update(text);
	gui->disk_space->set_color(color);
}

CreateDVD_TmpPath::CreateDVD_TmpPath(CreateDVD_GUI *gui, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, -(int)sizeof(gui->thread->tmp_path),
		gui->thread->tmp_path, 1, MEDIUMFONT)
{
	this->gui = gui;
}

CreateDVD_TmpPath::~CreateDVD_TmpPath()
{
}

int CreateDVD_TmpPath::handle_event()
{
	get_text();
	gui->disk_space->update();
	return 1;
}


CreateDVD_AssetTitle::CreateDVD_AssetTitle(CreateDVD_GUI *gui, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, -(int)sizeof(gui->thread->asset_title),
		gui->thread->asset_title, 1, MEDIUMFONT)
{
	this->gui = gui;
}

CreateDVD_AssetTitle::~CreateDVD_AssetTitle()
{
}

int CreateDVD_AssetTitle::handle_event()
{
	get_text();
	return 1;
}


CreateDVD_Deinterlace::CreateDVD_Deinterlace(CreateDVD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_deinterlace, _("Deinterlace"))
{
	this->gui = gui;
}

CreateDVD_Deinterlace::~CreateDVD_Deinterlace()
{
}

int CreateDVD_Deinterlace::handle_event()
{
	if( get_value() ) {
		gui->need_inverse_telecine->set_value(0);
		gui->thread->use_inverse_telecine = 0;
	}
	return BC_CheckBox::handle_event();
}


CreateDVD_InverseTelecine::CreateDVD_InverseTelecine(CreateDVD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_inverse_telecine, _("Inverse Telecine"))
{
	this->gui = gui;
}

CreateDVD_InverseTelecine::~CreateDVD_InverseTelecine()
{
}

int CreateDVD_InverseTelecine::handle_event()
{
	if( get_value() ) {
		gui->need_deinterlace->set_value(0);
		gui->thread->use_deinterlace = 0;
	}
	return BC_CheckBox::handle_event();
}


CreateDVD_ResizeTracks::CreateDVD_ResizeTracks(CreateDVD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_resize_tracks, _("Resize Tracks"))
{
	this->gui = gui;
}

CreateDVD_ResizeTracks::~CreateDVD_ResizeTracks()
{
}


CreateDVD_Histogram::CreateDVD_Histogram(CreateDVD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_histogram, _("Histogram"))
{
	this->gui = gui;
}

CreateDVD_Histogram::~CreateDVD_Histogram()
{
}

CreateDVD_LabelChapters::CreateDVD_LabelChapters(CreateDVD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_labeled, _("Chapters at Labels"))
{
	this->gui = gui;
}

CreateDVD_LabelChapters::~CreateDVD_LabelChapters()
{
}

CreateDVD_UseRenderFarm::CreateDVD_UseRenderFarm(CreateDVD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_farmed, _("Use render farm"))
{
	this->gui = gui;
}

CreateDVD_UseRenderFarm::~CreateDVD_UseRenderFarm()
{
}

CreateDVD_WideAudio::CreateDVD_WideAudio(CreateDVD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_wide_audio, _("Audio 5.1"))
{
	this->gui = gui;
}

CreateDVD_WideAudio::~CreateDVD_WideAudio()
{
}

CreateDVD_UseFFMpeg::CreateDVD_UseFFMpeg(CreateDVD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_ffmpeg, _("Use FFMPEG"))
{
	this->gui = gui;
}

CreateDVD_UseFFMpeg::~CreateDVD_UseFFMpeg()
{
}




CreateDVD_GUI::CreateDVD_GUI(CreateDVD_Thread *thread, int x, int y, int w, int h)
 : BC_Window(_(PROGRAM_NAME ": Create DVD"), x, y, w, h, 50, 50, 1, 0, 1)
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
	need_wide_audio = 0;
	need_labeled = 0;
	need_farmed = 0;
	ok = 0;
	cancel = 0;
}

CreateDVD_GUI::~CreateDVD_GUI()
{
}

void CreateDVD_GUI::create_objects()
{
	lock_window("CreateDVD_GUI::create_objects");
	int pady = BC_TextBox::calculate_h(this, MEDIUMFONT, 0, 1) + 5;
	int padx = BC_Title::calculate_w(this, (char*)"X", MEDIUMFONT);
	int x = padx/2, y = pady/2;
	BC_Title *title = new BC_Title(x, y, _("Title:"), MEDIUMFONT, YELLOW);
	add_subwindow(title);
	at_x = x + title->get_w();  at_y = y;
	asset_title = new CreateDVD_AssetTitle(this, at_x, at_y, get_w()-at_x-10);
	add_subwindow(asset_title);
	y += title->get_h() + pady/2;
	title = new BC_Title(x, y, _("Work path:"), MEDIUMFONT, YELLOW);
	add_subwindow(title);
	tmp_x = x + title->get_w();  tmp_y = y;
	tmp_path = new CreateDVD_TmpPath(this, tmp_x, tmp_y,  get_w()-tmp_x-35);
	add_subwindow(tmp_path);
	btmp_path = new BrowseButton(thread->mwindow->theme, this, tmp_path,
		tmp_x+tmp_path->get_w(), tmp_y, "/tmp",
		_("Work path"), _("Select a Work directory:"), 1);
	add_subwindow(btmp_path);
	y += title->get_h() + pady/2;
	disk_space = new CreateDVD_DiskSpace(this, x, y);
	add_subwindow(disk_space);
	int x0 = get_w() - 170;
	title = new BC_Title(x0, y, _("Media:"), MEDIUMFONT, YELLOW);
	add_subwindow(title);
	int x1 = x0+title->get_w()+padx;
	media_size = new CreateDVD_MediaSize(this, x1, y);
	media_size->create_objects();
	media_sizes.append(new BC_ListBoxItem("4.7GB"));
	media_sizes.append(new BC_ListBoxItem("8.3GB"));
	media_size->update_list(&media_sizes);
	media_size->update(media_sizes[0]->get_text());
	disk_space->update();
	y += disk_space->get_h() + pady/2;
	title = new BC_Title(x, y, _("Format:"), MEDIUMFONT, YELLOW);
	add_subwindow(title);
	standard = new CreateDVD_Format(this, title->get_w() + padx, y);
	add_subwindow(standard);
	standard->create_objects();
	x0 -= 30;
	title = new BC_Title(x0, y, _("Scale:"), MEDIUMFONT, YELLOW);
	add_subwindow(title);
	x1 = x0+title->get_w()+padx;
	scale = new CreateDVD_Scale(this, x1, y);
	add_subwindow(scale);
	scale->create_objects();
	y += standard->get_h() + pady/2;
	x1 = x;  int y1 = y;
	need_deinterlace = new CreateDVD_Deinterlace(this, x1, y);
	add_subwindow(need_deinterlace);
	y += need_deinterlace->get_h() + pady/2;
	need_histogram = new CreateDVD_Histogram(this, x, y);
	add_subwindow(need_histogram);
	y = y1;  x1 += 170;
	need_inverse_telecine = new CreateDVD_InverseTelecine(this, x1, y);
	add_subwindow(need_inverse_telecine);
	y += need_inverse_telecine->get_h() + pady/2;
	need_wide_audio = new CreateDVD_WideAudio(this, x1, y);
	add_subwindow(need_wide_audio);
	y += need_wide_audio->get_h() + pady/2;
	need_use_ffmpeg = new CreateDVD_UseFFMpeg(this, x1, y);
	add_subwindow(need_use_ffmpeg);
	y += need_use_ffmpeg->get_h() + pady/2;
	need_resize_tracks = new CreateDVD_ResizeTracks(this, x1, y);
	add_subwindow(need_resize_tracks);
	y = y1;  x1 += 170;
	need_labeled = new CreateDVD_LabelChapters(this, x1, y);
	add_subwindow(need_labeled);
	y += need_labeled->get_h() + pady/2;
	need_farmed = new CreateDVD_UseRenderFarm(this, x1, y);
	add_subwindow(need_farmed);
	ok_w = BC_OKButton::calculate_w();
	ok_h = BC_OKButton::calculate_h();
	ok_x = 10;
	ok_y = get_h() - ok_h - 10;
	ok = new CreateDVD_OK(this, ok_x, ok_y);
	add_subwindow(ok);
	cancel_w = BC_CancelButton::calculate_w();
	cancel_h = BC_CancelButton::calculate_h();
	cancel_x = get_w() - cancel_w - 10,
	cancel_y = get_h() - cancel_h - 10;
	cancel = new CreateDVD_Cancel(this, cancel_x, cancel_y);
	add_subwindow(cancel);
	show_window();
	unlock_window();
}

int CreateDVD_GUI::resize_event(int w, int h)
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

int CreateDVD_GUI::translation_event()
{
	return 1;
}

int CreateDVD_GUI::close_event()
{
	set_done(1);
	return 1;
}

void CreateDVD_GUI::update()
{
	scale->set_value(thread->use_scale);
	need_deinterlace->set_value(thread->use_deinterlace);
	need_inverse_telecine->set_value(thread->use_inverse_telecine);
	need_use_ffmpeg->set_value(thread->use_ffmpeg);
	need_resize_tracks->set_value(thread->use_resize_tracks);
	need_histogram->set_value(thread->use_histogram);
	need_wide_audio->set_value(thread->use_wide_audio);
	need_labeled->set_value(thread->use_labeled);
	need_farmed->set_value(thread->use_farmed);
}

int CreateDVD_Thread::
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

int CreateDVD_Thread::
resize_tracks()
{
	Tracks *tracks = mwindow->edl->tracks;
	int trk_w = max_w, trk_h = max_h;
	if( trk_w < dvd_width ) trk_w = dvd_width;
	if( trk_h < dvd_height ) trk_h = dvd_height;
	for( Track *vtrk=tracks->first; vtrk; vtrk=vtrk->next ) {
		if( vtrk->data_type != TRACK_VIDEO ) continue;
		if( !vtrk->record ) continue;
		vtrk->track_w = trk_w;
		vtrk->track_h = trk_h;
	}
	return 0;
}

int CreateDVD_Thread::
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

	int norm = dvd_formats[use_standard].norm;
	dvd_width = dvd_norms[norm].w;
	dvd_height = dvd_norms[norm].h;
	dvd_framerate = dvd_norms[norm].framerate;
	int aspect = dvd_formats[use_standard].aspect;
	dvd_aspect_width = dvd_aspects[aspect].w;
	dvd_aspect_height = dvd_aspects[aspect].h;
	double dvd_aspect = dvd_aspect_height > 0 ? dvd_aspect_width/dvd_aspect_height : 1;

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
				if( w != dvd_width ) use_scale = Rescale::scaled;
				int h = indexable->get_h();
				if( h > max_h ) max_h = h;
				if( h != dvd_height ) use_scale = Rescale::scaled;
				float aw, ah;
				MWindow::create_aspect_ratio(aw, ah, w, h);
				double aspect = ah > 0 ? aw / ah : 1;
				if( !EQUIV(aspect, dvd_aspect) ) use_scale = Rescale::scaled;
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
		if( max_w != dvd_width ) use_resize_tracks = 1;
		if( max_h != dvd_height ) use_resize_tracks = 1;
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
	if( !has_deinterlace && max_h > 2*dvd_height ) use_deinterlace = 1;
	Labels *labels = mwindow->edl->labels;
	use_labeled = labels && labels->first ? 1 : 0;

	if( tracks->recordable_audio_tracks() == DVD_WIDE_CHANNELS )
		use_wide_audio = 1;

	use_farmed = mwindow->preferences->use_renderfarm;
	return 0;
}



CreateDVD_FormatItem::CreateDVD_FormatItem(CreateDVD_Format *popup,
		int standard, const char *text)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->standard = standard;
}

CreateDVD_FormatItem::~CreateDVD_FormatItem()
{
}

int CreateDVD_FormatItem::handle_event()
{
	popup->set_text(get_text());
	popup->gui->thread->use_standard = standard;
	return popup->handle_event();
}


CreateDVD_Format::CreateDVD_Format(CreateDVD_GUI *gui, int x, int y)
 : BC_PopupMenu(x, y, 180, "", 1)
{
	this->gui = gui;
}

CreateDVD_Format::~CreateDVD_Format()
{
}

void CreateDVD_Format::create_objects()
{
	for( int i=0; i<(int)(sizeof(dvd_formats)/sizeof(dvd_formats[0])); ++i ) {
		int norm = dvd_formats[i].norm;
		int aspect = dvd_formats[i].aspect;
		char item_text[BCTEXTLEN];
		sprintf(item_text,"%4s (%5.2f) %dx%d",
			dvd_norms[norm].name, dvd_norms[norm].framerate,
			dvd_aspects[aspect].w, dvd_aspects[aspect].h);
		add_item(new CreateDVD_FormatItem(this, i, item_text));
	}
	set_value(gui->thread->use_standard);
}

int CreateDVD_Format::handle_event()
{
	gui->thread->option_presets();
	gui->update();
	return 1;
}


CreateDVD_ScaleItem::CreateDVD_ScaleItem(CreateDVD_Scale *popup,
		int scale, const char *text)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->scale = scale;
}

CreateDVD_ScaleItem::~CreateDVD_ScaleItem()
{
}

int CreateDVD_ScaleItem::handle_event()
{
	popup->gui->thread->use_scale = scale;
	popup->set_value(scale);
	return popup->handle_event();
}


CreateDVD_Scale::CreateDVD_Scale(CreateDVD_GUI *gui, int x, int y)
 : BC_PopupMenu(x, y, 100, "", 1)
{
	this->gui = gui;
}

CreateDVD_Scale::~CreateDVD_Scale()
{
}

void CreateDVD_Scale::create_objects()
{

	for( int i=0; i<(int)Rescale::n_scale_types; ++i ) {
		add_item(new CreateDVD_ScaleItem(this, i, Rescale::scale_types[i]));
	}
	set_value(gui->thread->use_scale);
}

int CreateDVD_Scale::handle_event()
{
	gui->update();
	return 1;
}


CreateDVD_MediaSize::CreateDVD_MediaSize(CreateDVD_GUI *gui, int x, int y)
 : BC_PopupTextBox(gui, 0, 0, x, y, 70,50)
{
	this->gui = gui;
}

CreateDVD_MediaSize::~CreateDVD_MediaSize()
{
}

int CreateDVD_MediaSize::handle_event()
{
	gui->disk_space->update();
	return 1;
}

