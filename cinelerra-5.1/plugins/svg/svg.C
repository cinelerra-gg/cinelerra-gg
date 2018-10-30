
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
 */

#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "mainerror.h"
#include "svg.h"
#include "svgwin.h"
#include "overlayframe.inc"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>


REGISTER_PLUGIN(SvgMain)

SvgConfig::SvgConfig()
{
	out_x = 0;   out_y = 0;
	out_w = 640; out_h = 480;
	dpi = 96;
	strcpy(svg_file, "");
	ms_time = 0;
}

int SvgConfig::equivalent(SvgConfig &that)
{
	return EQUIV(dpi, that.dpi) &&
		EQUIV(out_x, that.out_x) &&
		EQUIV(out_y, that.out_y) &&
		EQUIV(out_w, that.out_w) &&
		EQUIV(out_h, that.out_h) &&
		!strcmp(svg_file, that.svg_file) &&
		ms_time != 0 && that.ms_time != 0 &&
		ms_time == that.ms_time;
}

void SvgConfig::copy_from(SvgConfig &that)
{
	out_x = that.out_x;
	out_y = that.out_y;
	out_w = that.out_w;
	out_h = that.out_h;
	dpi = that.dpi;
	strcpy(svg_file, that.svg_file);
	ms_time = that.ms_time;
}

void SvgConfig::interpolate(SvgConfig &prev, SvgConfig &next,
	long prev_frame, long next_frame, long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->out_x = prev.out_x * prev_scale + next.out_x * next_scale;
	this->out_y = prev.out_y * prev_scale + next.out_y * next_scale;
	this->out_w = prev.out_w * prev_scale + next.out_w * next_scale;
	this->out_h = prev.out_h * prev_scale + next.out_h * next_scale;
	this->dpi = prev.dpi;
	strcpy(this->svg_file, prev.svg_file);
	this->ms_time = prev.ms_time;
}


SvgMain::SvgMain(PluginServer *server)
 : PluginVClient(server)
{
	ofrm = 0;
	overlayer = 0;
	need_reconfigure = 1;
}

SvgMain::~SvgMain()
{
	delete ofrm;
	delete overlayer;
}

const char* SvgMain::plugin_title() { return N_("SVG via Inkscape"); }
int SvgMain::is_realtime() { return 1; }
int SvgMain::is_synthesis() { return 1; }


LOAD_CONFIGURATION_MACRO(SvgMain, SvgConfig)

void SvgMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("SVG");
	output.tag.set_property("OUT_X", config.out_x);
	output.tag.set_property("OUT_Y", config.out_y);
	output.tag.set_property("OUT_W", config.out_w);
	output.tag.set_property("OUT_H", config.out_h);
	output.tag.set_property("DPI", config.dpi);
	output.tag.set_property("SVG_FILE", config.svg_file);
	output.tag.set_property("MS_TIME", config.ms_time);
	output.append_tag();
	output.tag.set_title("/SVG");
	output.append_tag();

	output.terminate_string();
}

void SvgMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);
	int result = 0;

	while( !(result = input.read_tag()) ) {
		if(input.tag.title_is("SVG")) {
			config.out_x = input.tag.get_property("OUT_X", config.out_x);
			config.out_y = input.tag.get_property("OUT_Y", config.out_y);
			config.out_w = input.tag.get_property("OUT_W", config.out_w);
			config.out_h = input.tag.get_property("OUT_H", config.out_h);
			config.dpi = input.tag.get_property("DPI", config.dpi);
			input.tag.get_property("SVG_FILE", config.svg_file);
			config.ms_time = input.tag.get_property("MS_TIME", config.ms_time);
		}
	}
}

static int exec_command(char* const*argv)
{
        pid_t pid = vfork();
        if( pid < 0 ) return -1;
        if( pid > 0 ) {
                int stat = 0;
		waitpid(pid, &stat, 0);
                if( stat ) {
                        char msg[BCTEXTLEN];
                        sprintf(msg, "%s: error exit status %d", argv[0], stat);
                        MainError::show_error(msg);
                }
                return 0;
	}
        execvp(argv[0], &argv[0]);
        return -1;
}


int SvgMain::process_realtime(VFrame *input, VFrame *output)
{
	if( input != output )
		output->copy_from(input);

	int need_export = 0;
	float last_dpi = config.dpi;
	char last_svg_file[BCTEXTLEN];
	strcpy(last_svg_file, config.svg_file);
	int64_t last_ms_time = config.ms_time;
	need_reconfigure = load_configuration();
	if( !ofrm || last_dpi != config.dpi )
		need_export = 1;
	if( strcmp(last_svg_file, config.svg_file) ||
	    last_ms_time != config.ms_time )
		need_reconfigure = 1;

	if( need_reconfigure || need_export ) {
		need_reconfigure = 0;
		if( config.svg_file[0] == 0 ) return 0;
		delete ofrm;  ofrm = 0;
		char filename_png[BCTEXTLEN];
		strcpy(filename_png, config.svg_file);
		strncat(filename_png, ".png", sizeof(filename_png));
		struct stat st_png;
		int64_t ms_time = need_export || stat(filename_png, &st_png) ? 0 :
			st_png.st_mtim.tv_sec*1000 + st_png.st_mtim.tv_nsec/1000000;
		int fd = ms_time < config.ms_time ? -1 : open(filename_png, O_RDWR);
		if( fd < 0 ) { // file does not exist, export it
			char command[BCTEXTLEN], dpi[BCSTRLEN];
			snprintf(command, sizeof(command),
				"inkscape --without-gui --export-background=0x000000 "
				"--export-background-opacity=0 -d %f %s --export-png=%s",
				config.dpi, config.svg_file, filename_png);
			printf(_("Running command %s\n"), command);
			snprintf(dpi, sizeof(dpi), "%f", config.dpi);
			snprintf(command, sizeof(command), "--export-png=%s",filename_png);
			char *const argv[] = {
				(char*)"inkscape",
				(char*)"--without-gui",
				(char*)"--export-background=0x000000",
                                (char*)"--export-background-opacity=0",
				(char*)"-d", dpi, config.svg_file, command,
				0, };
			exec_command(argv);
			// in order for lockf to work it has to be open for writing
			fd = open(filename_png, O_RDWR);
			if( fd < 0 )
				printf(_("Export of %s to %s failed\n"), config.svg_file, filename_png);
		}
		if( fd >= 0 ) {
			ofrm = VFramePng::vframe_png(fd);
			close(fd);
			if( ofrm && ofrm->get_color_model() != output->get_color_model() ) {
				VFrame *vfrm = new VFrame(ofrm->get_w(), ofrm->get_h(),
					output->get_color_model(), 0);
				vfrm->transfer_from(ofrm);
				delete ofrm;  ofrm = vfrm;
			}
			if( !ofrm )
				printf (_("The file %s that was generated from %s is not in PNG format."
					  " Try to delete all *.png files.\n"), filename_png, config.svg_file);
		}
	}
	if( ofrm ) {
		if(!overlayer) overlayer = new OverlayFrame(smp + 1);
		overlayer->overlay(output, ofrm,
			 0, 0, ofrm->get_w(), ofrm->get_h(),
			config.out_x, config.out_y,
			config.out_x + config.out_w,
			config.out_y + config.out_h,
			1, TRANSFER_NORMAL, LINEAR_LINEAR);
	}
	return 0;
}


NEW_WINDOW_MACRO(SvgMain, SvgWin)

void SvgMain::update_gui()
{
	if(thread) {
		load_configuration();
		SvgWin *window = (SvgWin*)thread->window;
		window->update_gui(config);
	}
}

