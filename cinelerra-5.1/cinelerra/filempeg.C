
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
#include "bcprogressbox.h"
#include "bcsignals.h"
#include "bitspopup.h"
#include "byteorder.h"
#include "clip.h"
#include "commercials.h"
#include "condition.h"
#include "cstrdup.h"
#include "edit.h"
#include "file.h"
#include "filempeg.h"
#include "filesystem.h"
#include "guicast.h"
#include "indexfile.h"
#include "interlacemodes.h"
#include "indexstate.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "pipe.h"
#include "preferences.h"
#include "removefile.h"
#include "vframe.h"
#include "videodevice.inc"

#include <stdio.h>
#include <string.h>
#include <unistd.h>


#define HVPEG_EXE "/hveg2enc"
#define MJPEG_EXE "/mpeg2enc"


// M JPEG dependancies
static double frame_rate_codes[] =
{
	0,
	24000.0/1001.0,
	24.0,
	25.0,
	30000.0/1001.0,
	30.0,
	50.0,
	60000.0/1001.0,
	60.0
};

static double aspect_ratio_codes[] =
{
	0,
	1.0,
	1.333,
	1.777,
	2.21
};








FileMPEG::FileMPEG(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset_parameters();
// May also be VMPEG or AMPEG if write status.
	if(asset->format == FILE_UNKNOWN) asset->format = FILE_MPEG;
	asset->byte_order = 0;
	next_frame_lock = new Condition(0, "FileMPEG::next_frame_lock");
	next_frame_done = new Condition(0, "FileMPEG::next_frame_done");
	vcommand_line.set_array_delete();
}

FileMPEG::~FileMPEG()
{
	close_file();
	delete next_frame_lock;
	delete next_frame_done;
	vcommand_line.remove_all_objects();
}

void FileMPEG::get_parameters(BC_WindowBase *parent_window,
	Asset *asset, BC_WindowBase* &format_window,
	int audio_options, int video_options, EDL *edl)
{
	if(audio_options && asset->format == FILE_AMPEG)
	{
		MPEGConfigAudio *window = new MPEGConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
	else
	if(video_options && asset->format == FILE_VMPEG)
	{
		MPEGConfigVideo *window = new MPEGConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

int FileMPEG::check_sig(Asset *asset)
{
	return mpeg3_check_sig(asset->path);
}

void FileMPEG::get_info(char *title_path, char *path, char *text, int len)
{
	mpeg3_t *fd;
	*text = 0;

	int result = 0;
	int zio_access = ZIO_UNBUFFERED+ZIO_SINGLE_ACCESS;
	if( !(fd=mpeg3_zopen(title_path, path, &result,zio_access)) ) result = 1;
	if( !result ) result = mpeg3_create_title(fd, 0);
	if( result ) return;

	char *cp = text, *ep = text + len-1;
	if( mpeg3_has_toc(fd) ) {
		cp += snprintf(cp,ep-cp, _("toc path:%s\n"), path);
		cp += snprintf(cp,ep-cp, _("title path:\n"));
		for( int i=0; i<100; ++i ) {
			char *title_path = mpeg3_title_path(fd,i);
			if( !title_path ) break;
			cp += snprintf(cp,ep-cp, " %2d. %s\n", i+1, title_path);
		}
	}
	else
		cp += snprintf(cp,ep-cp, _("file path:%s\n"), path);
	int64_t bytes = mpeg3_get_bytes(fd);
	char string[BCTEXTLEN];
	sprintf(string,"%jd",bytes);
	Units::punctuate(string);
	cp += snprintf(cp,ep-cp, _("size: %s"), string);

	if( mpeg3_is_program_stream(fd) )
	  cp += snprintf(cp,ep-cp, _("  program stream\n"));
	else if( mpeg3_is_transport_stream(fd) )
	  cp += snprintf(cp,ep-cp, _("  transport stream\n"));
	else if( mpeg3_is_video_stream(fd) )
	  cp += snprintf(cp,ep-cp, _("  video stream\n"));
	else if( mpeg3_is_audio_stream(fd) )
	  cp += snprintf(cp,ep-cp, _("  audio stream\n"));

	int64_t sdate = mpeg3_get_source_date(fd);
	if( !sdate ) {
		struct stat64 ostat;
		memset(&ostat,0,sizeof(struct stat64));
		sdate = stat64(path, &ostat) < 0 ? 0 : ostat.st_mtime;
	}
	time_t tm = (time_t)sdate;
	cp += snprintf(cp,ep-cp, _("date: %s\n"), ctime(&tm));

	int vtrks = mpeg3_total_vstreams(fd);
	cp += snprintf(cp,ep-cp, _("%d video tracks\n"), vtrks);
	for( int vtrk=0; vtrk<vtrks; ++vtrk ) {
		int cmdl = mpeg3_colormodel(fd, vtrk);
		int color_model = bc_colormodel(cmdl);
		char *cmodel = MPEGColorModel::cmodel_to_string(color_model);
		int width = mpeg3_video_width(fd, vtrk);
		int height = mpeg3_video_height(fd, vtrk);
		cp += snprintf(cp,ep-cp, _("  v%d %s %dx%d"), vtrk, cmodel, width, height);
		double frame_rate = mpeg3_frame_rate(fd, vtrk);
		int64_t frames = mpeg3_video_frames(fd, vtrk);
		cp += snprintf(cp,ep-cp, _(" (%5.2f), %jd frames"), frame_rate, frames);
		if( frame_rate > 0 ) {
			double secs = (double)frames / frame_rate;
			cp += snprintf(cp,ep-cp, _(" (%0.3f secs)"),secs);
		}
		*cp++ = '\n';
	}
	int atrks = mpeg3_total_astreams(fd);
	cp += snprintf(cp,ep-cp, _("%d audio tracks\n"), atrks);
	for( int atrk=0; atrk<atrks; ++atrk) {
		const char *format = mpeg3_audio_format(fd, atrk);
		cp += snprintf(cp,ep-cp, _(" a%d %s"), atrk, format);
		int channels = mpeg3_audio_channels(fd, atrk);
		int sample_rate = mpeg3_sample_rate(fd, atrk);
		cp += snprintf(cp,ep-cp, _(" ch%d (%d)"), channels, sample_rate);
		int64_t samples = mpeg3_audio_samples(fd, atrk);
		cp += snprintf(cp,ep-cp, " %jd",samples);
		int64_t nudge = mpeg3_get_audio_nudge(fd, atrk);
		*cp++ = nudge >= 0 ? '+' : (nudge=-nudge, '-');
		cp += snprintf(cp,ep-cp, _("%jd samples"),nudge);
		if( sample_rate > 0 ) {
			double secs = (double)(samples+nudge) / sample_rate;
			cp += snprintf(cp,ep-cp, _(" (%0.3f secs)"),secs);
		}
		*cp++ = '\n';
	}
	int stracks = mpeg3_subtitle_tracks(fd);
	if( stracks > 0 ) {
		cp += snprintf(cp,ep-cp, _("%d subtitles\n"), stracks);
	}
	int vts_titles = mpeg3_get_total_vts_titles(fd);
	if( vts_titles > 0 )
		cp += snprintf(cp,ep-cp, _("%d title sets, "), vts_titles);
	int interleaves = mpeg3_get_total_interleaves(fd);
	if( interleaves > 0 )
		cp += snprintf(cp,ep-cp, _("%d interleaves\n"), interleaves);
	int vts_title = mpeg3_set_vts_title(fd, -1);
	int angle = mpeg3_set_angle(fd, -1);
	int interleave = mpeg3_set_interleave(fd, -1);
	int program = mpeg3_set_program(fd, -1);
	cp += snprintf(cp,ep-cp, _("current program %d = title %d, angle %d, interleave %d\n\n"),
		program, vts_title, angle, interleave);

	ArrayList<double> cell_times;
	int cell_no = 0; double cell_time;
        while( !mpeg3_get_cell_time(fd, cell_no++, &cell_time) ) {
		cell_times.append(cell_time);
	}
	if( cell_times.size() > 1 ) {
		cp += snprintf(cp,ep-cp, _("cell times:"));
		for( int i=0; i<cell_times.size(); ++i ) {
			if( (i%4) == 0 ) *cp++ = '\n';
			cp += snprintf(cp,ep-cp,"  %3d.  %8.3f",i,cell_times.get(i));
		}
		cp += snprintf(cp,ep-cp, "\n");
	}

	int elements = mpeg3_dvb_channel_count(fd);
	if( elements <= 0 ) return;
	if( !mpeg3_dvb_get_system_time(fd, &sdate) ) {
		tm = (time_t)sdate;
		cp += snprintf(cp,ep-cp, _("\nsystem time: %s"), ctime_r(&tm,string));
	}
	cp += snprintf(cp,ep-cp, _("elements %d\n"), elements);

	for( int n=0; n<elements; ++n ) {
		char name[16], enc[8];  int vstream, astream;
		int major, minor, total_astreams, total_vstreams;
		if( mpeg3_dvb_get_channel(fd,n, &major, &minor) ||
		    mpeg3_dvb_get_station_id(fd,n,&name[0]) ||
		    mpeg3_dvb_total_vstreams(fd,n,&total_vstreams) ||
		    mpeg3_dvb_total_astreams(fd,n,&total_astreams) )	continue;
		cp += snprintf(cp,ep-cp, " %3d.%-3d %s", major, minor, &name[0]);
		for( int vidx=0; vidx<total_vstreams; ++vidx ) {
			if( mpeg3_dvb_vstream_number(fd,n,vidx,&vstream) ) continue;
			if( vstream < 0 ) continue;
			cp += snprintf(cp,ep-cp, " v%d", vstream);
		}
		for( int aidx=0; aidx<total_astreams; ++aidx ) {
			if( mpeg3_dvb_astream_number(fd,n,aidx,&astream,&enc[0]) ) continue;
			if( astream < 0 ) continue;
			cp += snprintf(cp,ep-cp, "    a%d %s", astream, &enc[0]);
			int atrack = 0;
			for(int i=0; i<astream; ++i )
				atrack += mpeg3_audio_channels(fd, i);
			int channels = mpeg3_audio_channels(fd, astream);
			cp += snprintf(cp,ep-cp, " trk %d-%d", atrack+1, atrack+channels);
			if( enc[0] ) cp += snprintf(cp,ep-cp," (%s)",enc);
		}
		cp += snprintf(cp,ep-cp, "\n");
	}

	for( int n=0; n<elements; ++n ) {
		int major, minor;
		if( mpeg3_dvb_get_channel(fd,n, &major, &minor) ) continue;
		cp += snprintf(cp,ep-cp, "\n**chan %3d.%-3d\n", major, minor);
		int len = mpeg3_dvb_get_chan_info(fd, n, -1, 0, cp, 1023);
                if( len < 0 ) len = snprintf(cp,ep-cp,_("no info"));
                cp += len;  *cp++ = '*';  *cp++ = '*';  *cp++ = '\n';
		for( int ord=0; ord<0x80; ++ord ) {
			for( int i=0; (len=mpeg3_dvb_get_chan_info(fd,n,ord,i,cp,1023)) >= 0; ++i ) {
				char *bp = cp;  cp += len;
				for( int k=2; --k>=0; ) {  // skip 2 lines
					while( bp<cp && *bp++!='\n' );
				}
				for( char *lp=bp; bp<cp; ++bp ) {  // add new lines
					if( *bp == '\n' || ((bp-lp)>=60 && *bp==' ') )
						*(lp=bp) = '\n';
				}
				*cp++ = '\n';  *cp = 0;  // trailing new line
			}
		}
	}

	*cp = 0;
	mpeg3_close(fd);
	return;
}

int FileMPEG::get_audio_for_video(int vstream, int astream, int64_t &channel_mask)
{
	channel_mask = 0;
	if( !fd ) return -1;
	int elements = mpeg3_dvb_channel_count(fd);
	if( elements <= 0 ) return -1;

	int pidx = -1;
	int total_astreams = 0, total_vstreams = 0;
	for( int n=0; pidx<0 && n<elements; ++n ) {
		total_astreams = total_vstreams = 0;
		if( mpeg3_dvb_total_vstreams(fd,n,&total_vstreams) ||
		    mpeg3_dvb_total_astreams(fd,n,&total_astreams) ) continue;
		if( !total_vstreams || !total_astreams ) continue;
		for( int i=0; pidx<0 && i<total_vstreams; ++i ) {
			int vstrm = -1;
			if( mpeg3_dvb_vstream_number(fd,n,i,&vstrm) ) continue;
			if( vstrm == vstream ) pidx = n;
		}
	}
	if( pidx < 0 ) return -1;
	int ret = -1;
	int64_t channels = 0;
	for( int i=0; i<total_astreams; ++i ) {
		int astrm = -1;
		if( mpeg3_dvb_astream_number(fd,pidx,i,&astrm,0) ) continue;
		if( astrm < 0 ) continue;
		if( ret < 0 ) ret = astrm;
		if( astream > 0 ) { --astream;  continue; }
		int atrack = 0;
		for(int i=0; i<astrm; ++i )
			atrack += mpeg3_audio_channels(fd, i);
		int64_t mask = (1 << mpeg3_audio_channels(fd, astrm)) - 1;
		channels |= mask << atrack;
		if( !astream ) break;
	}
	channel_mask = channels;
	return ret;
}

int FileMPEG::reset_parameters_derived()
{
	wrote_header = 0;
	mjpeg_out = 0;
	mjpeg_eof = 0;
	mjpeg_error = 0;
        recd_fd = -1;
	fd = 0;
	video_out = 0;
	prev_track = 0;
	temp_frame = 0;
	twolame_temp = 0;
	twolame_out = 0;
	twolame_allocation = 0;
	twolame_result = 0;
	twofp = 0;
	twopts = 0;
	lame_temp[0] = 0;
	lame_temp[1] = 0;
	lame_allocation = 0;
	lame_global = 0;
	lame_output = 0;
	lame_output_allocation = 0;
	lame_fd = 0;
	lame_started = 0;
	return 0;
}


int FileMPEG::open_file(int rd, int wr)
{
	int result = 0;

	if(rd) {
		fd = 0;
		char toc_name[BCTEXTLEN];
		result = file->preferences->get_asset_file_path(asset, toc_name);
		if( result >= 0 ) {
			int error = 0;
// if toc exists, use it otherwise just probe file
			char *path = !result ? toc_name : asset->path;
			fd = mpeg3_open_title(asset->path, path, &error);
			if( !fd ) {
				switch( error ) {
				case zmpeg3_t::ERR_INVALID_TOC_VERSION:
					eprintf(_("Couldn't open %s: invalid table of contents version.\n"),asset->path);
					result = 1;
					break;
				case zmpeg3_t::ERR_TOC_DATE_MISMATCH:
					eprintf(_("Couldn't open %s: table of contents out of date.\n"),asset->path);
					result = 1;
					break;
				default:
					eprintf(_("Couldn't open %s: table of contents corrupt.\n"),asset->path);
					result = -1;
					break;
				}
				if( result > 0 ) {
					eprintf(_("Rebuilding the table of contents\n"));
				}

			}
			else {
				if( mpeg3_has_toc(fd) )
					result = 0;
				else if( mpeg3_total_vstreams(fd) || mpeg3_total_astreams(fd) )
					result = 1;
				else {
					eprintf(_("Couldn't open %s: no audio or video.\n"),asset->path);
					result = -1;
				}
			}
		}

		if( result > 0 ) {
			if( fd ) { mpeg3_close(fd);  fd = 0; }
			result = create_toc(toc_name);
		}

		if( !result ) {
			mpeg3_set_cpus(fd, file->cpus < 4 ? file->cpus : 4);
			file->current_program = mpeg3_set_program(fd, -1);
			if( asset->program < 0 )
				asset->program = file->current_program;

			asset->audio_data = mpeg3_has_audio(fd);
			if(asset->audio_data) {
				asset->channels = 0;
				for(int i = 0; i < mpeg3_total_astreams(fd); i++) {
					asset->channels += mpeg3_audio_channels(fd, i);
				}
				if(!asset->sample_rate)
					asset->sample_rate = mpeg3_sample_rate(fd, 0);
				asset->audio_length = mpeg3_audio_samples(fd, 0);
				if( !asset->channels || !asset->sample_rate )
					result = 1;
			}

			asset->video_data = mpeg3_has_video(fd);
			if( !result && asset->video_data ) {
//TODO: this is not as easy as just looking at headers.
//most interlaced media is rendered as FRM, not TOP/BOT in coding ext hdrs.
//currently, just using the assetedit menu to set the required result as needed.
//				if( asset->interlace_mode == ILACE_MODE_UNDETECTED )
//					asset->interlace_mode = mpeg3_detect_interlace(fd, 0);
				if( !asset->layers ) {
					asset->layers = mpeg3_total_vstreams(fd);
				}
				asset->actual_width = mpeg3_video_width(fd, 0);
				if( !asset->width )
					asset->width = asset->actual_width;
				asset->actual_height = mpeg3_video_height(fd, 0);
				if( !asset->height )
					asset->height = asset->actual_height;
				if( !asset->video_length )
					asset->video_length = mpeg3_video_frames(fd, 0);
				if( !asset->vmpeg_cmodel )
					asset->vmpeg_cmodel = bc_colormodel(mpeg3_colormodel(fd, 0));
				if( !asset->frame_rate )
					asset->frame_rate = mpeg3_frame_rate(fd, 0);
			}
		}
		if( result ) {
			eprintf(_("Couldn't open %s: failed.\n"), asset->path);
		}
	}

	if( !result && wr && asset->format == FILE_VMPEG ) {
// Heroine Virtual encoder
//  this one is cinelerra-x.x.x/mpeg2enc
		if(asset->vmpeg_cmodel == BC_YUV422P)
		{
			char bitrate_string[BCTEXTLEN];
			char quant_string[BCTEXTLEN];
			char iframe_string[BCTEXTLEN];

			sprintf(bitrate_string, "%d", asset->vmpeg_bitrate);
			sprintf(quant_string, "%d", asset->vmpeg_quantization);
			sprintf(iframe_string, "%d", asset->vmpeg_iframe_distance);

// Construct command line
			if(!result)
			{
				const char *exec_path = File::get_cinlib_path();
				snprintf(mjpeg_command, sizeof(mjpeg_command),
					"%s/%s", exec_path, HVPEG_EXE);
				append_vcommand_line(mjpeg_command);

				if(asset->aspect_ratio > 0)
				{
					append_vcommand_line("-a");
// Square pixels
					if(EQUIV((double)asset->width / asset->height,
						asset->aspect_ratio))
						append_vcommand_line("1");
					else
					if(EQUIV(asset->aspect_ratio, 1.333))
						append_vcommand_line("2");
					else
					if(EQUIV(asset->aspect_ratio, 1.777))
						append_vcommand_line("3");
					else
					if(EQUIV(asset->aspect_ratio, 2.11))
						append_vcommand_line("4");
				}

				append_vcommand_line(asset->vmpeg_derivative == 1 ? "-1" : "");
				append_vcommand_line(asset->vmpeg_cmodel == BC_YUV422P ? "-422" : "");
				if(asset->vmpeg_fix_bitrate)
				{
					append_vcommand_line("-b");
					append_vcommand_line(bitrate_string);
				}
				else
				{
					append_vcommand_line("-q");
					append_vcommand_line(quant_string);
				}
				append_vcommand_line("-n");
				append_vcommand_line(iframe_string);
				append_vcommand_line(asset->vmpeg_progressive ? "-p" : "");
				append_vcommand_line(asset->vmpeg_denoise ? "-d" : "");
				append_vcommand_line(file->cpus <= 1 ? "-u" : "");
				append_vcommand_line(asset->vmpeg_seq_codes ? "-g" : "");
				append_vcommand_line(asset->path);

				video_out = new FileMPEGVideo(this);
				video_out->start();
			}
		}
		else
// mjpegtools encoder
//  this one is cinelerra-x.x.x/thirdparty/mjpegtools/mpeg2enc
		{
			const char *exec_path = File::get_cinlib_path();
			snprintf(mjpeg_command, sizeof(mjpeg_command),
				"%s/%s -v 0 ", exec_path, MJPEG_EXE);

// Must disable interlacing if MPEG-1
			switch (asset->vmpeg_preset)
			{
				case 0: asset->vmpeg_progressive = 1; break;
				case 1: asset->vmpeg_progressive = 1; break;
				case 2: asset->vmpeg_progressive = 1; break;
			}

			char string[BCTEXTLEN];
// The current usage of mpeg2enc requires bitrate of 0 when quantization is fixed and
// quantization of 1 when bitrate is fixed.  Perfectly intuitive.
			if(asset->vmpeg_fix_bitrate)
			{
				snprintf(string, sizeof(string),
					" -b %d -q 1", asset->vmpeg_bitrate / 1000);
			}
			else
			{
				snprintf(string, sizeof(string),
					" -b 0 -q %d", asset->vmpeg_quantization);
			}
			strncat(mjpeg_command, string, sizeof(mjpeg_command));

// Aspect ratio
			int aspect_ratio_code = -1;
			if(asset->aspect_ratio > 0)
			{
				int ncodes = sizeof(aspect_ratio_codes) / sizeof(double);
				for(int i = 1; i < ncodes; i++)
				{
					if(EQUIV(aspect_ratio_codes[i], asset->aspect_ratio))
					{
						aspect_ratio_code = i;
						break;
					}
				}
			}


// Square pixels
			if(EQUIV((double)asset->width / asset->height, asset->aspect_ratio))
				aspect_ratio_code = 1;

			if(aspect_ratio_code < 0)
			{
				eprintf(_("Unsupported aspect ratio %f\n"), asset->aspect_ratio);
				aspect_ratio_code = 2;
			}
			sprintf(string, " -a %d", aspect_ratio_code);
			strncat(mjpeg_command, string, sizeof(mjpeg_command));






// Frame rate
			int frame_rate_code = -1;
			int ncodes = sizeof(frame_rate_codes) / sizeof(double);
    			for(int i = 1; i < ncodes; ++i)
			{
				if(EQUIV(asset->frame_rate, frame_rate_codes[i]))
				{
					frame_rate_code = i;
					break;
				}
			}
			if(frame_rate_code < 0)
			{
				frame_rate_code = 4;
				eprintf(_("Unsupported frame rate %f\n"), asset->frame_rate);
			}
			sprintf(string, " -F %d", frame_rate_code);
			strncat(mjpeg_command, string, sizeof(mjpeg_command));





			strncat(mjpeg_command,
				asset->vmpeg_progressive ? " -I 0" : " -I 1",
				sizeof(mjpeg_command));



			sprintf(string, " -M %d", file->cpus);
			strncat(mjpeg_command, string, sizeof(mjpeg_command));


			if(!asset->vmpeg_progressive)
			{
				strncat(mjpeg_command,
					asset->vmpeg_field_order ? " -z b" : " -z t",
					sizeof(mjpeg_command));
			}


			snprintf(string, sizeof(string), " -f %d", asset->vmpeg_preset);
			strncat(mjpeg_command, string, sizeof(mjpeg_command));


			snprintf(string, sizeof(string),
				" -g %d -G %d", asset->vmpeg_iframe_distance, asset->vmpeg_iframe_distance);
			strncat(mjpeg_command, string, sizeof(mjpeg_command));


			if(asset->vmpeg_seq_codes)
				strncat(mjpeg_command, " -s", sizeof(mjpeg_command));


			snprintf(string, sizeof(string),
				" -R %d", CLAMP(asset->vmpeg_pframe_distance, 0, 2));
			strncat(mjpeg_command, string, sizeof(mjpeg_command));

			snprintf(string, sizeof(string), " -o '%s'", asset->path);
			strncat(mjpeg_command, string, sizeof(mjpeg_command));



			printf("FileMPEG::open_file: Running %s\n", mjpeg_command);
			if(!(mjpeg_out = popen(mjpeg_command, "w")))
			{
				perror("FileMPEG::open_file");
				eprintf(_("Error while opening \"%s\" for writing\n%m\n"), mjpeg_command);
				return 1;
			}

			video_out = new FileMPEGVideo(this);
			video_out->start();
		}
	}
	else if( !result && wr && asset->format == FILE_AMPEG) {
		//char encoder_string[BCTEXTLEN]; encoder_string[0] = 0;
//printf("FileMPEG::open_file 1 %d\n", asset->ampeg_derivative);

		if(asset->ampeg_derivative == 2)
		{
			twofp = fopen(asset->path, "w" );
			if( !twofp ) return 1;
			twopts = twolame_init();
			int channels = asset->channels >= 2 ? 2 : 1;
			twolame_set_num_channels(twopts, channels);
			twolame_set_in_samplerate(twopts, asset->sample_rate);
			twolame_set_mode(twopts, channels >= 2 ?
				TWOLAME_JOINT_STEREO : TWOLAME_MONO);
			twolame_set_bitrate(twopts, asset->ampeg_bitrate);
			twolame_init_params(twopts);
		}
		else
		if(asset->ampeg_derivative == 3)
		{
			lame_global = lame_init();
//			lame_set_brate(lame_global, asset->ampeg_bitrate / 1000);
			lame_set_brate(lame_global, asset->ampeg_bitrate);
			lame_set_quality(lame_global, 0);
			lame_set_in_samplerate(lame_global,
				asset->sample_rate);
			lame_set_num_channels(lame_global,
				asset->channels);
			if((result = lame_init_params(lame_global)) < 0)
			{
				eprintf(_("encode: lame_init_params returned %d\n"), result);
				lame_close(lame_global);
				lame_global = 0;
			}
			else
			if(!(lame_fd = fopen(asset->path, "w")))
			{
				perror("FileMPEG::open_file");
				eprintf(_("Error while opening \"%s\" for writing\n%m\n"), asset->path);
				lame_close(lame_global);
				lame_global = 0;
				result = 1;
			}
		}
		else
		{
			eprintf(_("ampeg_derivative=%d\n"), asset->ampeg_derivative);
			result = 1;
		}
	}

// Transport stream for DVB capture
	if( !result && !rd && !wr && asset->format == FILE_MPEG ) {
		if( (recd_fd = open(asset->path, O_CREAT+O_TRUNC+O_WRONLY,
			S_IRUSR+S_IWUSR + S_IRGRP+S_IWGRP)) < 0 ) {
			perror("FileMPEG::open_file");
			eprintf(_("Error while opening \"%s\" for writing\n%m\n"), asset->path);
			result = 1;
		}
	}


//asset->dump();
	return result;
}





int FileMPEG::set_skimming(int track, int skim, skim_fn fn, void *vp)
{
	return !fn ? mpeg3_set_thumbnail_callback(fd, track, 0, 0, 0, 0) :
		mpeg3_set_thumbnail_callback(fd, track, skim, 1, fn, vp);
}

int FileMPEG::skimming(void *vp, int track)
{
	File *file = (File *)vp;
	FileMPEG *mpeg = (FileMPEG *)file->file;
	return mpeg->skim_result = mpeg->skim_callback(mpeg->skim_data, track);
}

int FileMPEG::skim_video(int track, void *vp, skim_fn fn)
{
	skim_callback = fn;  skim_data = vp;
	mpeg3_set_thumbnail_callback(fd, track, 1, 1, skimming, (void*)file);
	skim_result = -1;
	while( skim_result < 0 && !mpeg3_end_of_video(fd, track) )
		mpeg3_drop_frames(fd, 1, track);
	mpeg3_set_thumbnail_callback(fd, track, 0, 0, 0, 0);
	return skim_result;
}



#ifdef HAVE_COMMERCIAL
int FileMPEG::toc_nail(void *vp, int track)
{
	File *file = (File *)vp;
	FileMPEG *mpeg = (FileMPEG *)file->file;
	int64_t framenum; uint8_t *tdat; int mw, mh;
	if( mpeg->get_thumbnail(track, framenum, tdat, mw, mh) ) return 1;
	int pid, width, height;  double framerate;
	if( mpeg->get_video_info(track, pid, framerate, width, height) ) return 1;
	if( pid < 0 || framerate <= 0 ) return 1;
	double position = framenum / framerate;
//printf("t%d/%03x f%jd, %dx%d %dx%d\n",track,pid,framenum,mw,mh,width,height);
	MWindow::commercials->get_frame(file, pid, position, tdat, mw, mh, width, height);
	return 0;
}
#endif

int FileMPEG::create_toc(char *toc_path)
{
// delete any existing toc files
	char toc_file[BCTEXTLEN];
	strcpy(toc_file, toc_path);
	if( strcmp(toc_file, asset->path) )
		remove(toc_file);
	char *bp = strrchr(toc_file, '/');
	if( !bp ) bp = toc_file;
	char *sfx = strrchr(bp,'.');
	if( sfx ) {
		strcpy(sfx+1,"toc");
		remove(toc_file);
	}

	int64_t total_bytes = 0, last_bytes = -1;
	fd = mpeg3_start_toc(asset->path, toc_file,
				file->current_program, &total_bytes);
	if( !fd ) {
		eprintf(_("cant start toc/idx for file: %s\n"), asset->path);
		return 1;
	}

// File needs a table of contents.
	struct timeval new_time, prev_time, start_time, current_time;
	gettimeofday(&prev_time, 0);  gettimeofday(&start_time, 0);
#ifdef HAVE_COMMERCIAL
	if( file->preferences->scan_commercials ) {
		set_skimming(-1, 1, toc_nail, file);
		if( MWindow::commercials->resetDb() != 0 )
			eprintf(_("cant access commercials database"));
	}
#endif
// This gets around the fact that MWindowGUI may be locked.
	char progress_title[BCTEXTLEN];
	sprintf(progress_title, _("Creating %s\n"), toc_file);
	BC_ProgressBox progress(-1, -1, progress_title, total_bytes);
	progress.start();

	int result = 0;
	while( !result ) {
		int64_t bytes_processed = 0;
		if( mpeg3_do_toc(fd, &bytes_processed) ) break;

		if( bytes_processed >= total_bytes ) break;
		if( bytes_processed == last_bytes ) {
			eprintf(_("toc scan stopped before eof"));
			break;
		}
		last_bytes = bytes_processed;

		gettimeofday(&new_time, 0);
		if( new_time.tv_sec - prev_time.tv_sec >= 1 ) {
			gettimeofday(&current_time, 0);
			int64_t elapsed_seconds = current_time.tv_sec - start_time.tv_sec;
			int64_t total_seconds = !bytes_processed ? 0 :
                                 elapsed_seconds * total_bytes / bytes_processed;
			int64_t eta = total_seconds - elapsed_seconds;
			progress.update(bytes_processed, 1);
			char string[BCTEXTLEN];
			sprintf(string, "%sETA: %jdm%jds",
				progress_title, eta / 60, eta % 60);
			progress.update_title(string, 1);
// 			fprintf(stderr, "ETA: %dm%ds        \r",
// 				bytes_processed * 100 / total_bytes,
// 				eta / 60, eta % 60);
// 			fflush(stdout);
			prev_time = new_time;
		}

		if( progress.is_cancelled() ) {
			result = 1;
			break;
		}
	}

#ifdef HAVE_COMMERCIAL
	if( file->preferences->scan_commercials ) {
		if( !result ) MWindow::commercials->write_ads(asset->path);
		MWindow::commercials->closeDb();
	}
#endif

	mpeg3_stop_toc(fd);
	fd = 0;

	progress.stop_progress();

	if( result ) {
		remove_file(toc_file);
		return 1;
	}

// Reopen file from toc path instead of asset path.
	int error = 0;
	fd = mpeg3_open(toc_file, &error);
	if( !fd ) {
		eprintf(_("mpeg3_open failed: %s"), toc_file);
		return 1;
	}
	return 0;
}

int FileMPEG::get_index(IndexFile *index_file, MainProgressBar *progress_bar)
{
	if( !fd ) return 1;
	IndexState *index_state = index_file->get_state();
	index_state->reset_index();
	index_state->reset_markers();

// Convert the index tables from tracks to channels.
	int ntracks = mpeg3_index_tracks(fd);
	if( !ntracks ) return 1;

	int index_zoom = mpeg3_index_zoom(fd);
	int64_t offset = 0;
	for( int i = 0; i < ntracks; ++i ) {
		int nch = mpeg3_index_channels(fd, i);
		for( int j = 0; j < nch; ++j ) {
			float *bfr = (float *)mpeg3_index_data(fd, i, j);
			int64_t size = 2*mpeg3_index_size(fd, i);
			index_state->add_index_entry(bfr, offset, size);
			offset += size;
		}
	}

	FileSystem fs;
	int64_t file_bytes = fs.get_size(asset->path);
	char *index_path = index_file->index_filename;
	return index_state->write_index(index_path, asset, index_zoom, file_bytes);
}






void FileMPEG::append_vcommand_line(const char *string)
{
	if(string[0])
	{
		char *argv = cstrdup(string);
		vcommand_line.append(argv);
	}
}

int FileMPEG::close_file()
{
	mjpeg_eof = 1;
	next_frame_lock->unlock();

	if(fd)
	{
		mpeg3_close(fd);
	}

	if(video_out)
	{
// End of sequence signal
		if(file->asset->vmpeg_cmodel == BC_YUV422P)
		{
			mpeg2enc_set_input_buffers(1, 0, 0, 0);
		}
		delete video_out;
		video_out = 0;
	}

	vcommand_line.remove_all_objects();

	if(twofp) {
		unsigned char opkt[1152*2];
		int ret = twolame_encode_flush(twopts, opkt, sizeof(opkt));
		if( ret > 0 )
			fwrite(opkt, 1, ret, twofp);
		else if( ret < 0 )
			fprintf(stderr, _("twolame error encoding audio: %d\n"), ret);
		fclose(twofp);  twofp = 0;
	}
	if( twopts ) { twolame_close(&twopts); twopts = 0; }

	if(lame_global)
		lame_close(lame_global);

	if(temp_frame) delete temp_frame;
	if(twolame_temp) delete [] twolame_temp;

	if(lame_temp[0]) delete [] lame_temp[0];
	if(lame_temp[1]) delete [] lame_temp[1];
	if(lame_output) delete [] lame_output;
	if(lame_fd) fclose(lame_fd);

	if(mjpeg_out) pclose(mjpeg_out);

	if( recd_fd >= 0 ) {
		close(recd_fd);
		recd_fd = -1;
	}

	reset_parameters();

	FileBase::close_file();
	return 0;
}

int FileMPEG::get_best_colormodel(Asset *asset, int driver)
{
//printf("FileMPEG::get_best_colormodel 1\n");
	switch(driver)
	{
		case PLAYBACK_X11:
//			return BC_RGB888;
// the direct X11 color model requires scaling in the codec
			return BC_BGR8888;
		case PLAYBACK_X11_XV:
		case PLAYBACK_ASYNCHRONOUS:
 			return zmpeg3_cmdl(asset->vmpeg_cmodel) > 0 ?
				asset->vmpeg_cmodel : BC_RGB888;
		case PLAYBACK_X11_GL:
 			return BC_YUV888;
		case PLAYBACK_DV1394:
		case PLAYBACK_FIREWIRE:
			return BC_YUV422P;
		case VIDEO4LINUX2:
 			return zmpeg3_cmdl(asset->vmpeg_cmodel) > 0 ?
				asset->vmpeg_cmodel : BC_RGB888;
		case VIDEO4LINUX2JPEG:
			return BC_COMPRESSED;
		case CAPTURE_DVB:
		case VIDEO4LINUX2MPEG:
			return BC_YUV422P;
		case CAPTURE_JPEG_WEBCAM:
			return BC_COMPRESSED;
		case CAPTURE_YUYV_WEBCAM:
			return BC_YUV422;
		case CAPTURE_FIREWIRE:
		case CAPTURE_IEC61883:
			return BC_YUV422P;
	}
	eprintf(_("unknown driver %d\n"),driver);
	return BC_RGB888;
}

int FileMPEG::colormodel_supported(int colormodel)
{
	return colormodel;
}

int FileMPEG::can_copy_from(Asset *asset, int64_t position)
{
	return 0;
}

int FileMPEG::set_audio_position(int64_t sample)
{
#if 0
	if(!fd) return 1;

	int channel, stream;
	to_streamchannel(file->current_channel, stream, channel);

//printf("FileMPEG::set_audio_position %d %d %d\n", sample, mpeg3_get_sample(fd, stream), last_sample);
	if(sample != mpeg3_get_sample(fd, stream) &&
		sample != last_sample)
	{
		if(sample >= 0 && sample < asset->audio_length)
		{
//printf("FileMPEG::set_audio_position seeking stream %d\n", sample);
			return mpeg3_set_sample(fd, sample, stream);
		}
		else
			return 1;
	}
#endif
	return 0;
}

int FileMPEG::set_video_position(int64_t pos)
{
	if( !fd || pos < 0 || pos >= asset->video_length )
		return 1;
//printf("FileMPEG::set_video_position 1 %jd\n", x);
	mpeg3_set_frame(fd, pos, file->current_layer);
	return 0;
}

int64_t FileMPEG::get_memory_usage()
{
	int64_t result = file->rd && fd ? mpeg3_memory_usage(fd) : 0;
//printf("FileMPEG::get_memory_usage %d  %jd\n", __LINE__, result);
	return result;
}

int FileMPEG::set_program(int no)
{
	return fd ? mpeg3_set_program(fd, no) : -1;
}

int FileMPEG::get_cell_time(int no, double &time)
{
	return fd ? mpeg3_get_cell_time(fd, no, &time) : -1;
}

int FileMPEG::get_system_time(int64_t &tm)
{
	return fd ? mpeg3_dvb_get_system_time(fd, &tm) : -1;
}

int FileMPEG::get_video_pid(int track)
{
	return fd ? mpeg3_video_pid(fd, track) : -1;
}

int FileMPEG::get_video_info(int track, int &pid,
		double &framerate, int &width, int &height, char *title)
{
	if( !fd ) return -1;
	pid = mpeg3_video_pid(fd, track);
	framerate = mpeg3_frame_rate(fd, track);
	width = mpeg3_video_width(fd, track);
	height = mpeg3_video_height(fd, track);
	if( !title ) return 0;
	*title = 0;

	int elements = mpeg3_dvb_channel_count(fd);
	for( int n=0; n<elements; ++n ) {
		int major, minor, total_vstreams, vstream, vidx;
		if( mpeg3_dvb_get_channel(fd,n, &major, &minor) ||
		    mpeg3_dvb_total_vstreams(fd,n,&total_vstreams) ) continue;
		for( vidx=0; vidx<total_vstreams; ++vidx ) {
			if( mpeg3_dvb_vstream_number(fd,n,vidx,&vstream) ) continue;
			if( vstream < 0 ) continue;
			if( vstream == track ) {
				sprintf(title, "%3d.%-3d", major, minor);
				return 0;
			}
		}
	}
	return 0;
}

int FileMPEG::select_video_stream(Asset *asset, int vstream)
{
	if( !fd ) return -1;
	asset->width = mpeg3_video_width(fd, vstream);
	asset->height = mpeg3_video_height(fd, vstream);
	asset->video_length = mpeg3_video_frames(fd, vstream);
	asset->frame_rate = mpeg3_frame_rate(fd, vstream);
	return 0;
}

int FileMPEG::select_audio_stream(Asset *asset, int astream)
{
	if( !fd ) return -1;
	asset->sample_rate = mpeg3_sample_rate(fd, astream);
	asset->audio_length = mpeg3_audio_samples(fd, astream);
	return 0;
}

int FileMPEG::get_thumbnail(int stream,
	int64_t &position, unsigned char *&thumbnail, int &ww, int &hh)
{
	return !fd ? -1 :
		mpeg3_get_thumbnail(fd, stream, &position, &thumbnail, &ww, &hh);
}

int FileMPEG::write_samples(double **buffer, int64_t len)
{
	int result = 0;

//printf("FileMPEG::write_samples 1\n");
	if(asset->ampeg_derivative == 2) {
// Convert to int16
		int channels = MIN(asset->channels, 2);
		int64_t audio_size = len * channels * 2;
		if(twolame_allocation < audio_size) {
			if(twolame_temp) delete [] twolame_temp;
			twolame_temp = new unsigned char[audio_size];
			twolame_allocation = audio_size;
			if(twolame_out) delete [] twolame_out;
			twolame_out = new unsigned char[audio_size + 1152];
		}

		for(int i = 0; i < channels; i++) {
			int16_t *output = ((int16_t*)twolame_temp) + i;
			double *input = buffer[i];
			for(int j = 0; j < len; j++) {
				int sample = (int)(*input * 0x7fff);
				*output = (int16_t)(CLIP(sample, -0x8000, 0x7fff));
				output += channels;
				input++;
			}
		}
		int ret = twolame_encode_buffer_interleaved(twopts,
				(int16_t*)twolame_temp, len,
				twolame_out, twolame_allocation+1152);
		if( ret > 0 )
			fwrite(twolame_out, 1, ret, twofp);
		else if( ret < 0 )
			fprintf(stderr, _("twolame error encoding audio: %d\n"), ret);
	}
	else
	if(asset->ampeg_derivative == 3)
	{
		int channels = MIN(asset->channels, 2);
		int64_t audio_size = len * channels;
		if(!lame_global) return 1;
		if(!lame_fd) return 1;
		if(lame_allocation < audio_size)
		{
			if(lame_temp[0]) delete [] lame_temp[0];
			if(lame_temp[1]) delete [] lame_temp[1];
			lame_temp[0] = new float[audio_size];
			lame_temp[1] = new float[audio_size];
			lame_allocation = audio_size;
		}

		if(lame_output_allocation < audio_size * 4)
		{
			if(lame_output) delete [] lame_output;
			lame_output_allocation = audio_size * 4;
			lame_output = new char[lame_output_allocation];
		}

		for(int i = 0; i < channels; i++)
		{
			float *output = lame_temp[i];
			double *input = buffer[i];
			for(int j = 0; j < len; j++)
			{
				*output++ = *input++ * (float)32768;
			}
		}

		result = lame_encode_buffer_float(lame_global,
			lame_temp[0],
			(channels > 1) ? lame_temp[1] : lame_temp[0],
			len,
			(unsigned char*)lame_output,
			lame_output_allocation);
		if(result > 0)
		{
			char *real_output = lame_output;
			int bytes = result;
			if(!lame_started)
			{
				for(int i = 0; i < bytes; i++)
					if(lame_output[i])
					{
						real_output = &lame_output[i];
						lame_started = 1;
						bytes -= i;
						break;
					}
			}
			if(bytes > 0 && lame_started)
			{
				result = !fwrite(real_output, 1, bytes, lame_fd);
				if(result) {
					perror("FileMPEG::write_samples");
					eprintf(_("write failed: %m"));
				}
			}
			else
				result = 0;
		}
		else
			result = 1;
	}

	return result;
}

int FileMPEG::write_frames(VFrame ***frames, int len)
{
	int result = 0;

	if(video_out)
	{
		int temp_w = (int)((asset->width + 15) / 16) * 16;
		int temp_h;

		int output_cmodel = asset->vmpeg_cmodel;
// verify colormodel supported in MPEG output
		switch( output_cmodel ) {
		case BC_YUV420P:
			if( file->preferences->dvd_yuv420p_interlace &&
			    ( asset->interlace_mode == ILACE_MODE_TOP_FIRST ||
			      asset->interlace_mode == ILACE_MODE_BOTTOM_FIRST ) )
				output_cmodel = BC_YUV420PI;
		case BC_YUV422P:
			break;
		default:
			return 1;
		}

// Height depends on progressiveness
		if(asset->vmpeg_progressive || asset->vmpeg_derivative == 1)
			temp_h = (int)((asset->height + 15) / 16) * 16;
		else
			temp_h = (int)((asset->height + 31) / 32) * 32;

//printf("FileMPEG::write_frames 1\n");

// Only 1 layer is supported in MPEG output
		for(int i = 0; i < 1; i++)
		{
			for(int j = 0; j < len && !result; j++)
			{
				VFrame *frame = frames[i][j];



				if(asset->vmpeg_cmodel == BC_YUV422P)
				{
					if(frame->get_w() == temp_w &&
						frame->get_h() == temp_h &&
						frame->get_color_model() == output_cmodel)
					{
						mpeg2enc_set_input_buffers(0,
							(char*)frame->get_y(),
							(char*)frame->get_u(),
							(char*)frame->get_v());
					}
					else
					{
						if(temp_frame &&
							(temp_frame->get_w() != temp_w ||
							temp_frame->get_h() != temp_h ||
							temp_frame->get_color_model() || output_cmodel))
						{
							delete temp_frame;
							temp_frame = 0;
						}


						if(!temp_frame)
						{
							temp_frame = new VFrame(temp_w, temp_h,
									output_cmodel, 0);
						}

						BC_CModels::transfer(temp_frame->get_rows(),
							frame->get_rows(),
							temp_frame->get_y(),
							temp_frame->get_u(),
							temp_frame->get_v(),
							frame->get_y(),
							frame->get_u(),
							frame->get_v(),
							0,
							0,
							frame->get_w(),
							frame->get_h(),
							0,
							0,
							temp_frame->get_w(),
							temp_frame->get_h(),
							frame->get_color_model(),
							temp_frame->get_color_model(),
							0,
							frame->get_w(),
							temp_frame->get_w());

						mpeg2enc_set_input_buffers(0,
							(char*)temp_frame->get_y(),
							(char*)temp_frame->get_u(),
							(char*)temp_frame->get_v());
					}
				}
				else
				{
// MJPEG uses the same dimensions as the input
//printf("FileMPEG::write_frames %d\n", __LINE__);sleep(1);
					if(frame->get_color_model() == output_cmodel)
					{
						mjpeg_y = frame->get_y();
						mjpeg_u = frame->get_u();
						mjpeg_v = frame->get_v();
					}
					else
					{
//printf("FileMPEG::write_frames %d\n", __LINE__);sleep(1);
						if(!temp_frame)
						{
							temp_frame = new VFrame(asset->width, asset->height,
										output_cmodel, 0);
						}

// printf("FileMPEG::write_frames %d temp_frame=%p %p %p %p frame=%p %p %p %p color_model=%p %p\n",
// __LINE__,
// temp_frame,
// temp_frame->get_w(),
// temp_frame->get_h(),
// frame,
// frame->get_w(),
// frame->get_h(),
// temp_frame->get_color_model(),
// frame->get_color_model()); sleep(1);
						temp_frame->transfer_from(frame);
//printf("FileMPEG::write_frames %d\n", __LINE__);sleep(1);

						mjpeg_y = temp_frame->get_y();
						mjpeg_u = temp_frame->get_u();
						mjpeg_v = temp_frame->get_v();
					}




					next_frame_lock->unlock();
					next_frame_done->lock("FileMPEG::write_frames");
					if(mjpeg_error) result = 1;
				}





			}
		}
	}



	return result;
}

int FileMPEG::zmpeg3_cmdl(int colormodel)
{
	switch( colormodel ) {
	case BC_BGR888:       return zmpeg3_t::cmdl_BGR888;
	case BC_BGR8888:      return zmpeg3_t::cmdl_BGRA8888;
	case BC_RGB565:       return zmpeg3_t::cmdl_RGB565;
	case BC_RGB888:       return zmpeg3_t::cmdl_RGB888;
	case BC_RGBA8888:     return zmpeg3_t::cmdl_RGBA8888;
	case BC_RGBA16161616: return zmpeg3_t::cmdl_RGBA16161616;
	case BC_YUV420P:      return zmpeg3_t::cmdl_YUV420P;
	case BC_YUV422P:      return zmpeg3_t::cmdl_YUV422P;
	case BC_YUV422:       return zmpeg3_t::cmdl_YUYV;
	case BC_YUV888:       return zmpeg3_t::cmdl_YUV888;
	case BC_YUVA8888:     return zmpeg3_t::cmdl_YUVA8888;
	}
	return -1;
}

int FileMPEG::bc_colormodel(int cmdl)
{
	switch( cmdl ) {
	case zmpeg3_t::cmdl_BGR888:       return BC_BGR888;
	case zmpeg3_t::cmdl_BGRA8888:     return BC_BGR8888;
	case zmpeg3_t::cmdl_RGB565:       return BC_RGB565;
	case zmpeg3_t::cmdl_RGB888:       return BC_RGB888;
	case zmpeg3_t::cmdl_RGBA8888:     return BC_RGBA8888;
	case zmpeg3_t::cmdl_RGBA16161616: return BC_RGBA16161616;
	case zmpeg3_t::cmdl_YUV420P:      return BC_YUV420P;
	case zmpeg3_t::cmdl_YUV422P:      return BC_YUV422P;
	case zmpeg3_t::cmdl_YUYV:         return BC_YUV422;
	case zmpeg3_t::cmdl_YUV888:       return BC_YUV888;
	case zmpeg3_t::cmdl_YUVA8888:     return BC_YUVA8888;
	}
	return -1;
}

const char *FileMPEG::zmpeg3_cmdl_name(int cmdl)
{
# define CMDL(nm) #nm
  static const char *cmdl_name[] = {
    CMDL(BGR888),
    CMDL(BGRA8888),
    CMDL(RGB565),
    CMDL(RGB888),
    CMDL(RGBA8888),
    CMDL(RGBA16161616),
    CMDL(UNKNOWN),
    CMDL(601_BGR888),
    CMDL(601_BGRA8888),
    CMDL(601_RGB888),
    CMDL(601_RGBA8888),
    CMDL(601_RGB565),
    CMDL(YUV420P),
    CMDL(YUV422P),
    CMDL(601_YUV420P),
    CMDL(601_YUV422P),
    CMDL(UYVY),
    CMDL(YUYV),
    CMDL(601_UYVY),
    CMDL(601_YUYV),
    CMDL(YUV888),
    CMDL(YUVA8888),
    CMDL(601_YUV888),
    CMDL(601_YUVA8888),
  };
  return cmdl>=0 && cmdl<lengthof(cmdl_name) ? cmdl_name[cmdl] : cmdl_name[6];
}


int FileMPEG::read_frame(VFrame *frame)
{
	if(!fd) return 1;
	int result = 0;
	int width = mpeg3_video_width(fd,file->current_layer);
	int height = mpeg3_video_height(fd,file->current_layer);
	int stream_cmdl = mpeg3_colormodel(fd,file->current_layer);
	int stream_color_model = bc_colormodel(stream_cmdl);
	int frame_color_model = frame->get_color_model();
	int frame_cmdl = asset->interlace_mode == ILACE_MODE_NOTINTERLACED ?
		zmpeg3_cmdl(frame_color_model) : -1;
	mpeg3_show_subtitle(fd, file->current_layer, file->playback_subtitle);

	switch( frame_color_model ) { // check for direct copy
	case BC_YUV420P:
		if( frame_cmdl < 0 ) break;
	case BC_YUV422P:
		if( stream_color_model == frame_color_model &&
			width == frame->get_w() && height == frame->get_h() ) {
			mpeg3_read_yuvframe(fd,
				(char*)frame->get_y(),
				(char*)frame->get_u(),
				(char*)frame->get_v(),
				0, 0, width, height,
				file->current_layer);
			return result;
		}
	}

	if( frame_cmdl >= 0 ) {        // supported by read_frame
		// cant rely on frame->get_rows(), format may not support it
		int uvs = 0;
		switch( frame_color_model ) {
		case BC_YUV420P: uvs = 2;  break;
		case BC_YUV422P: uvs = 1;  break;
		}
		//int w = frame->get_w();
		int h = frame->get_h();
		int bpl = frame->get_bytes_per_line();
		int uvw = !uvs ? 0 : bpl / 2;
		int uvh = !uvs ? 0 : h / uvs;
		uint8_t *rows[h + 2*uvh], *rp;
		int n = 0;
		if( (rp=frame->get_y()) ) {
			for( int i=0; i<h; ++i, rp+=bpl ) rows[n++] = rp;
			rp = frame->get_u();
			for( int i=0; i<uvh; ++i, rp+=uvw ) rows[n++] = rp;
			rp = frame->get_v();
			for( int i=0; i<uvh; ++i, rp+=uvw ) rows[n++] = rp;
		}
		else {
			rp = frame->get_data();  uvh *= 2;
			for( int i=0; i<h; ++i, rp+=bpl ) rows[n++] = rp;
			for( int i=0; i<uvh; ++i, rp+=uvw ) rows[n++] = rp;
		}

		mpeg3_read_frame(fd,
				rows,                /* start of each output row */
				0, 0, width, height, /* input box */
				frame->get_w(),      /* Dimensions of output_rows */
				frame->get_h(),
				frame_cmdl,
				file->current_layer);
		return result;
	}

	char *y, *u, *v;
	mpeg3_read_yuvframe_ptr(fd, &y, &u, &v, file->current_layer);
	if( y && u && v ) {
		if( stream_color_model == BC_YUV420P &&
		    file->preferences->dvd_yuv420p_interlace && (
			asset->interlace_mode == ILACE_MODE_TOP_FIRST ||
			asset->interlace_mode == ILACE_MODE_BOTTOM_FIRST ) )
				stream_color_model = BC_YUV420PI;
		BC_CModels::transfer(frame->get_rows(), 0,
			frame->get_y(), frame->get_u(), frame->get_v(),
			(unsigned char*)y, (unsigned char*)u, (unsigned char*)v,
			0,0, width,height,  0,0, frame->get_w(),frame->get_h(),
			stream_color_model, frame_color_model, 0, width, frame->get_w());
	}

	return result;
}


void FileMPEG::to_streamchannel(int channel, int &stream_out, int &channel_out)
{
	int total_astreams = mpeg3_total_astreams(fd);
	for(stream_out = 0; stream_out < total_astreams; ++stream_out )
	{
		int stream_channels = mpeg3_audio_channels(fd, stream_out);
		if( channel < stream_channels ) break;
		channel -= stream_channels;
	}
	channel_out = channel;
}

int FileMPEG::read_samples(double *buffer, int64_t len)
{
	if(!fd) return 0;
	if(len < 0) return 0;

// Translate pure channel to a stream and a channel in the mpeg stream
	int stream, channel;
	to_streamchannel(file->current_channel, stream, channel);

//printf("FileMPEG::read_samples 1 current_sample=%jd len=%jd channel=%d\n", file->current_sample, len, channel);

	mpeg3_set_sample(fd,
		file->current_sample,
		stream);
	mpeg3_read_audio_d(fd,
		buffer, /* Pointer to pre-allocated buffer of doubles */
		channel,          /* Channel to decode */
		len,         /* Number of samples to decode */
		stream);          /* Stream containing the channel */

//	last_sample = file->current_sample;
	return 0;
}






FileMPEGVideo::FileMPEGVideo(FileMPEG *file)
 : Thread(1, 0, 0)
{
	this->file = file;


	if(file->asset->vmpeg_cmodel == BC_YUV422P)
	{
		mpeg2enc_init_buffers();
		mpeg2enc_set_w(file->asset->width);
		mpeg2enc_set_h(file->asset->height);
		mpeg2enc_set_rate(file->asset->frame_rate);
	}
}

FileMPEGVideo::~FileMPEGVideo()
{
	Thread::join();
}

void FileMPEGVideo::run()
{
	if(file->asset->vmpeg_cmodel == BC_YUV422P)
	{
		printf("FileMPEGVideo::run ");
		for(int i = 0; i < file->vcommand_line.total; i++)
		printf("%s ", file->vcommand_line.values[i]);
		printf("\n");
		mpeg2enc(file->vcommand_line.total, file->vcommand_line.values);
	}
	else
	{
		while(1)
		{
//printf("FileMPEGVideo::run %d\n", __LINE__);
			file->next_frame_lock->lock("FileMPEGVideo::run");
//printf("FileMPEGVideo::run %d\n", __LINE__);
			if(file->mjpeg_eof)
			{
				file->next_frame_done->unlock();
				break;
			}



//printf("FileMPEGVideo::run %d\n", __LINE__);
// YUV4 sequence header
			if(!file->wrote_header)
			{
				file->wrote_header = 1;

				char interlace_string[BCTEXTLEN];
				if(!file->asset->vmpeg_progressive)
				{
					sprintf(interlace_string, file->asset->vmpeg_field_order ? "b" : "t");
				}
				else
				{
					sprintf(interlace_string, "p");
				}
				double aspect_ratio = file->asset->aspect_ratio >= 0 ?
					file->asset->aspect_ratio : 1.0;
//printf("FileMPEGVideo::run %d\n", __LINE__);
				fprintf(file->mjpeg_out, "YUV4MPEG2 W%d H%d F%d:%d I%s A%d:%d C%s\n",
					file->asset->width, file->asset->height,
					(int)(file->asset->frame_rate * 1001),
					1001, interlace_string,
					(int)(aspect_ratio * 1000), 1000,
					"420mpeg2");
//printf("FileMPEGVideo::run %d\n", __LINE__);
			}

// YUV4 frame header
//printf("FileMPEGVideo::run %d\n", __LINE__);
			fprintf(file->mjpeg_out, "FRAME\n");

// YUV data
//printf("FileMPEGVideo::run %d\n", __LINE__);
			if(!fwrite(file->mjpeg_y, file->asset->width * file->asset->height, 1, file->mjpeg_out))
				file->mjpeg_error = 1;
//printf("FileMPEGVideo::run %d\n", __LINE__);
			if(!fwrite(file->mjpeg_u, file->asset->width * file->asset->height / 4, 1, file->mjpeg_out))
				file->mjpeg_error = 1;
//printf("FileMPEGVideo::run %d\n", __LINE__);
			if(!fwrite(file->mjpeg_v, file->asset->width * file->asset->height / 4, 1, file->mjpeg_out))
				file->mjpeg_error = 1;
//printf("FileMPEGVideo::run %d\n", __LINE__);
			fflush(file->mjpeg_out);

//printf("FileMPEGVideo::run %d\n", __LINE__);
			file->next_frame_done->unlock();
//printf("FileMPEGVideo::run %d\n", __LINE__);
		}
		pclose(file->mjpeg_out);
		file->mjpeg_out = 0;
	}
}












MPEGConfigAudio::MPEGConfigAudio(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(_(PROGRAM_NAME ": Audio Compression"),
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	310, 120, -1, -1, 0, 0, 1)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

MPEGConfigAudio::~MPEGConfigAudio()
{
}

void MPEGConfigAudio::create_objects()
{
	int x = 10, y = 10;
	int x1 = 150;
	MPEGLayer *layer;

	lock_window("MPEGConfigAudio::create_objects");
	if(asset->format == FILE_MPEG)
	{
		add_subwindow(new BC_Title(x, y, _("No options for MPEG transport stream.")));
		unlock_window();
		return;
	}


	add_tool(new BC_Title(x, y, _("Layer:")));
	add_tool(layer = new MPEGLayer(x1, y, this));
	layer->create_objects();

	y += 30;
	add_tool(new BC_Title(x, y, _("Kbits per second:")));
	add_tool(bitrate = new MPEGABitrate(x1, y, this));
	bitrate->create_objects();


	add_subwindow(new BC_OKButton(this));
	show_window(1);
	unlock_window();
}

int MPEGConfigAudio::close_event()
{
	set_done(0);
	return 1;
}


MPEGLayer::MPEGLayer(int x, int y, MPEGConfigAudio *gui)
 : BC_PopupMenu(x, y, 100, layer_to_string(gui->asset->ampeg_derivative))
{
	this->gui = gui;
}

void MPEGLayer::create_objects()
{
	add_item(new BC_MenuItem(layer_to_string(2)));
	add_item(new BC_MenuItem(layer_to_string(3)));
}

int MPEGLayer::handle_event()
{
	gui->asset->ampeg_derivative = string_to_layer(get_text());
	gui->bitrate->set_layer(gui->asset->ampeg_derivative);
	return 1;
};

int MPEGLayer::string_to_layer(char *string)
{
	if(!strcasecmp(layer_to_string(2), string))
		return 2;
	if(!strcasecmp(layer_to_string(3), string))
		return 3;

	return 2;
}

char* MPEGLayer::layer_to_string(int layer)
{
	switch(layer)
	{
		case 2:
			return _("II");
			break;

		case 3:
			return _("III");
			break;

		default:
			return _("II");
			break;
	}
}







MPEGABitrate::MPEGABitrate(int x, int y, MPEGConfigAudio *gui)
 : BC_PopupMenu(x, y, 100,
 	bitrate_to_string(gui->string, gui->asset->ampeg_bitrate))
{
	this->gui = gui;
}

void MPEGABitrate::create_objects()
{
	set_layer(gui->asset->ampeg_derivative);
}

void MPEGABitrate::set_layer(int layer)
{
	while(total_items()) del_item(0);

	if(layer == 2)
	{
		add_item(new BC_MenuItem("160"));
		add_item(new BC_MenuItem("192"));
		add_item(new BC_MenuItem("224"));
		add_item(new BC_MenuItem("256"));
		add_item(new BC_MenuItem("320"));
		add_item(new BC_MenuItem("384"));
	}
	else
	{
		add_item(new BC_MenuItem("8"));
		add_item(new BC_MenuItem("16"));
		add_item(new BC_MenuItem("24"));
		add_item(new BC_MenuItem("32"));
		add_item(new BC_MenuItem("40"));
		add_item(new BC_MenuItem("48"));
		add_item(new BC_MenuItem("56"));
		add_item(new BC_MenuItem("64"));
		add_item(new BC_MenuItem("80"));
		add_item(new BC_MenuItem("96"));
		add_item(new BC_MenuItem("112"));
		add_item(new BC_MenuItem("128"));
		add_item(new BC_MenuItem("144"));
		add_item(new BC_MenuItem("160"));
		add_item(new BC_MenuItem("192"));
		add_item(new BC_MenuItem("224"));
		add_item(new BC_MenuItem("256"));
		add_item(new BC_MenuItem("320"));
	}
}

int MPEGABitrate::handle_event()
{
	gui->asset->ampeg_bitrate = string_to_bitrate(get_text());
	return 1;
};

int MPEGABitrate::string_to_bitrate(char *string)
{
	return atol(string);
}


char* MPEGABitrate::bitrate_to_string(char *string, int bitrate)
{
	sprintf(string, "%d", bitrate);
	return string;
}









MPEGConfigVideo::MPEGConfigVideo(BC_WindowBase *parent_window,
	Asset *asset)
 : BC_Window(_(PROGRAM_NAME ": Video Compression"),
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	500, 400, -1, -1, 0, 0, 1)
{
	this->parent_window = parent_window;
	this->asset = asset;
	reset_cmodel();
}

MPEGConfigVideo::~MPEGConfigVideo()
{
}

void MPEGConfigVideo::create_objects()
{
	int x = 10, y = 10;
	int x1 = x + 150;
	//int x2 = x + 300;

	lock_window("MPEGConfigVideo::create_objects");
	if(asset->format == FILE_MPEG)
	{
		add_subwindow(new BC_Title(x, y, _("No options for MPEG transport stream.")));
		unlock_window();
		return;
	}

	add_subwindow(new BC_Title(x, y, _("Color model:")));
	add_subwindow(cmodel = new MPEGColorModel(x1, y, this));
	cmodel->create_objects();
	y += 30;

	update_cmodel_objs();

	add_subwindow(new BC_OKButton(this));
	show_window(1);
	unlock_window();
}

int MPEGConfigVideo::close_event()
{
	set_done(0);
	return 1;
}


void MPEGConfigVideo::delete_cmodel_objs()
{
	delete preset;
	delete derivative;
	delete bitrate;
	delete fixed_bitrate;
	delete quant;
	delete fixed_quant;
	delete iframe_distance;
	delete pframe_distance;
	delete top_field_first;
	delete progressive;
	delete denoise;
	delete seq_codes;
	titles.remove_all_objects();
	reset_cmodel();
}

void MPEGConfigVideo::reset_cmodel()
{
	preset = 0;
	derivative = 0;
	bitrate = 0;
	fixed_bitrate = 0;
	quant = 0;
	fixed_quant = 0;
	iframe_distance = 0;
	pframe_distance = 0;
	top_field_first = 0;
	progressive = 0;
	denoise = 0;
	seq_codes = 0;
}

void MPEGConfigVideo::update_cmodel_objs()
{
	BC_Title *title;
	int x = 10;
	int y = 40;
	int x1 = x + 150;
	int x2 = x + 280;

	delete_cmodel_objs();

	if(asset->vmpeg_cmodel == BC_YUV420P)
	{
		add_subwindow(title = new BC_Title(x, y + 5, _("Format Preset:")));
		titles.append(title);
		add_subwindow(preset = new MPEGPreset(x1, y, this));
		preset->create_objects();
		y += 30;
	}

	add_subwindow(title = new BC_Title(x, y + 5, _("Derivative:")));
	titles.append(title);
	add_subwindow(derivative = new MPEGDerivative(x1, y, this));
	derivative->create_objects();
	y += 30;

	add_subwindow(title = new BC_Title(x, y + 5, _("Bitrate:")));
	titles.append(title);
	add_subwindow(bitrate = new MPEGBitrate(x1, y, this));
	add_subwindow(fixed_bitrate = new MPEGFixedBitrate(x2, y, this));
	y += 30;

	add_subwindow(title = new BC_Title(x, y, _("Quantization:")));
	titles.append(title);
	quant = new MPEGQuant(x1, y, this);
	quant->create_objects();
	add_subwindow(fixed_quant = new MPEGFixedQuant(x2, y, this));
	y += 30;

	add_subwindow(title = new BC_Title(x, y, _("I frame distance:")));
	titles.append(title);
	iframe_distance = new MPEGIFrameDistance(x1, y, this);
	iframe_distance->create_objects();
	y += 30;

	if(asset->vmpeg_cmodel == BC_YUV420P)
	{
  		add_subwindow(title = new BC_Title(x, y, _("P frame distance:")));
		titles.append(title);
		pframe_distance = new MPEGPFrameDistance(x1, y, this);
		pframe_distance->create_objects();
  		y += 30;

		add_subwindow(top_field_first = new BC_CheckBox(x, y, &asset->vmpeg_field_order, _("Bottom field first")));
  		y += 30;
	}

	add_subwindow(progressive = new BC_CheckBox(x, y, &asset->vmpeg_progressive, _("Progressive frames")));
	y += 30;
	add_subwindow(denoise = new BC_CheckBox(x, y, &asset->vmpeg_denoise, _("Denoise")));
	y += 30;
	add_subwindow(seq_codes = new BC_CheckBox(x, y, &asset->vmpeg_seq_codes, _("Sequence start codes in every GOP")));

}


MPEGDerivative::MPEGDerivative(int x, int y, MPEGConfigVideo *gui)
 : BC_PopupMenu(x, y, 150, derivative_to_string(gui->asset->vmpeg_derivative))
{
	this->gui = gui;
}

void MPEGDerivative::create_objects()
{
	add_item(new BC_MenuItem(derivative_to_string(1)));
	add_item(new BC_MenuItem(derivative_to_string(2)));
}

int MPEGDerivative::handle_event()
{
	gui->asset->vmpeg_derivative = string_to_derivative(get_text());
	return 1;
};

int MPEGDerivative::string_to_derivative(char *string)
{
	if( !strcasecmp(derivative_to_string(1), string) ) return 1;
	if( !strcasecmp(derivative_to_string(2), string) ) return 2;
	return 1;
}

char* MPEGDerivative::derivative_to_string(int derivative)
{
	switch(derivative) {
	case 1: return _("MPEG-1");
	case 2: return _("MPEG-2");
	}
	return _("MPEG-1");
}


MPEGPreset::MPEGPreset(int x, int y, MPEGConfigVideo *gui)
 : BC_PopupMenu(x, y, 200, value_to_string(gui->asset->vmpeg_preset))
{
	this->gui = gui;
}

void MPEGPreset::create_objects()
{
	for(int i = 0; i < 14; i++) {
		add_item(new BC_MenuItem(value_to_string(i)));
	}
}

int MPEGPreset::handle_event()
{
	gui->asset->vmpeg_preset = string_to_value(get_text());
	return 1;
}

int MPEGPreset::string_to_value(char *string)
{
	for(int i = 0; i < 14; i++) {
		if(!strcasecmp(value_to_string(i), string))
			return i;
	}
	return 0;
}

char* MPEGPreset::value_to_string(int derivative)
{
	switch( derivative ) {
	case 0: return _("Generic MPEG-1"); break;
	case 1: return _("standard VCD"); break;
	case 2: return _("user VCD"); break;
	case 3: return _("Generic MPEG-2"); break;
	case 4: return _("standard SVCD"); break;
	case 5: return _("user SVCD"); break;
	case 6: return _("VCD Still sequence"); break;
	case 7: return _("SVCD Still sequence"); break;
	case 8: return _("DVD NAV"); break;
	case 9: return _("DVD"); break;
	case 10: return _("ATSC 480i"); break;
	case 11: return _("ATSC 480p"); break;
	case 12: return _("ATSC 720p"); break;
	case 13: return _("ATSC 1080i"); break;
	}
	return _("Generic MPEG-1");
}











MPEGBitrate::MPEGBitrate(int x, int y, MPEGConfigVideo *gui)
 : BC_TextBox(x, y, 100, 1, gui->asset->vmpeg_bitrate)
{
	this->gui = gui;
}


int MPEGBitrate::handle_event()
{
	gui->asset->vmpeg_bitrate = atol(get_text());
	return 1;
};





MPEGQuant::MPEGQuant(int x, int y, MPEGConfigVideo *gui)
 : BC_TumbleTextBox(gui,
 	(int64_t)gui->asset->vmpeg_quantization,
	(int64_t)1,
	(int64_t)100,
	x,
	y,
	100)
{
	this->gui = gui;
}

int MPEGQuant::handle_event()
{
	gui->asset->vmpeg_quantization = atol(get_text());
	return 1;
};

MPEGFixedBitrate::MPEGFixedBitrate(int x, int y, MPEGConfigVideo *gui)
 : BC_Radial(x, y, gui->asset->vmpeg_fix_bitrate, _("Fixed bitrate"))
{
	this->gui = gui;
}

int MPEGFixedBitrate::handle_event()
{
	update(1);
	gui->asset->vmpeg_fix_bitrate = 1;
	gui->fixed_quant->update(0);
	return 1;
};

MPEGFixedQuant::MPEGFixedQuant(int x, int y, MPEGConfigVideo *gui)
 : BC_Radial(x, y, !gui->asset->vmpeg_fix_bitrate, _("Fixed quantization"))
{
	this->gui = gui;
}

int MPEGFixedQuant::handle_event()
{
	update(1);
	gui->asset->vmpeg_fix_bitrate = 0;
	gui->fixed_bitrate->update(0);
	return 1;
};









MPEGIFrameDistance::MPEGIFrameDistance(int x, int y, MPEGConfigVideo *gui)
 : BC_TumbleTextBox(gui,
 	(int64_t)gui->asset->vmpeg_iframe_distance,
	(int64_t)1,
	(int64_t)100,
	x,
	y,
	50)
{
	this->gui = gui;
}

int MPEGIFrameDistance::handle_event()
{
	gui->asset->vmpeg_iframe_distance = atoi(get_text());
	return 1;
}







MPEGPFrameDistance::MPEGPFrameDistance(int x, int y, MPEGConfigVideo *gui)
 : BC_TumbleTextBox(gui,
 	(int64_t)gui->asset->vmpeg_pframe_distance,
	(int64_t)0,
	(int64_t)2,
	x,
	y,
	50)
{
	this->gui = gui;
}

int MPEGPFrameDistance::handle_event()
{
	gui->asset->vmpeg_pframe_distance = atoi(get_text());
	return 1;
}








MPEGColorModel::MPEGColorModel(int x, int y, MPEGConfigVideo *gui)
 : BC_PopupMenu(x, y, 150, cmodel_to_string(gui->asset->vmpeg_cmodel))
{
	this->gui = gui;
}

void MPEGColorModel::create_objects()
{
	add_item(new BC_MenuItem(cmodel_to_string(BC_YUV420P)));
	add_item(new BC_MenuItem(cmodel_to_string(BC_YUV422P)));
}

int MPEGColorModel::handle_event()
{
	gui->asset->vmpeg_cmodel = string_to_cmodel(get_text());
	gui->update_cmodel_objs();
	gui->show_window(1);
	return 1;
};

int MPEGColorModel::string_to_cmodel(char *string)
{
	if(!strcasecmp(cmodel_to_string(BC_YUV420P), string))
		return BC_YUV420P;
	if(!strcasecmp(cmodel_to_string(BC_YUV422P), string))
		return BC_YUV422P;
	return BC_YUV420P;
}

char* MPEGColorModel::cmodel_to_string(int cmodel)
{
	switch(cmodel)
	{
		case BC_YUV420P: return _("YUV 4:2:0");
		case BC_YUV422P: return _("YUV 4:2:2");
		default: return _("YUV 4:2:0");
	}
}






