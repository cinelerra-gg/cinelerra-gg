
/*
 * CINELERRA
 * Copyright (C) 2009-2015 Adam Williams <broadcast at earthling dot net>
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

#include "bcdisplayinfo.h"
#include "bcipc.h"
#include "bclistbox.inc"
#include "bcfontentry.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bcwindowbase.h"
#include "bccolors.h"
#include "bccmodels.h"
#include "cstrdup.h"
#include "fonts.h"
#include "language.h"
#include "vframe.h"
#include "workarounds.h"

#include <string.h>
#include <iconv.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>
#include <unistd.h>





int BC_Resources::error = 0;

VFrame* BC_Resources::bg_image = 0;
VFrame* BC_Resources::menu_bg = 0;

int BC_Resources::locale_utf8 = 0;
int BC_Resources::little_endian = 0;
char BC_Resources::language[LEN_LANG] = {0};
char BC_Resources::region[LEN_LANG] = {0};
char BC_Resources::encoding[LEN_ENCOD] = {0};
const char *BC_Resources::wide_encoding = 0;
ArrayList<BC_FontEntry*> *BC_Resources::fontlist = 0;
int BC_Resources::font_debug = 0;
const char *BC_Resources::fc_properties[] = { FC_SLANT, FC_WEIGHT, FC_WIDTH };
#define LEN_FCPROP (sizeof(BC_Resources::fc_properties) / sizeof(const char*))

static const char *def_small_font = "-*-helvetica-medium-r-normal-*-%d-*";  // 10
static const char *def_small_font2 = "-*-helvetica-medium-r-normal-*-%d-*"; // 11
static const char *def_medium_font = "-*-helvetica-bold-r-normal-*-%d-*";   // 14
static const char *def_medium_font2 = "-*-helvetica-bold-r-normal-*-%d-*";  // 14
static const char *def_large_font = "-*-helvetica-bold-r-normal-*-%d-*";    // 18
static const char *def_large_font2 = "-*-helvetica-bold-r-normal-*-%d-*";   // 20
static const char *def_big_font =
  "-*-bitstream charter-bold-r-normal-*-*-0-%d-%d-p-0-iso8859-1"; // 160
static const char *def_big_font2 =
  "-*-nimbus sans l-bold-r-normal-*-*-0-%d-%d-p-0-iso8859-1";     // 160
static const char *def_clock_font = "-*-helvetica-bold-r-normal-*-%d-*";      // 16
static const char *def_clock_font2 = "-*-helvetica-bold-r-normal-*-%d-*";     // 18
static const char *def_small_fontset =  "-*-helvetica-medium-r-normal-*-%d-*";// 10
static const char *def_medium_fontset = "-*-helvetica-bold-r-normal-*-%d-*";  // 14
static const char *def_large_fontset =  "-*-helvetica-bold-r-normal-*-%d-*";  // 18
static const char *def_big_fontset =    "-*-helvetica-bold-r-normal-*-%d-*";  // 24
static const char *def_clock_fontset = "-*-helvetica-bold-r-normal-*-%d-*";   // 16

static const char *def_small_font_xft = "Sans:pixelsize=%.4f";           // 10.6667
static const char *def_small_b_font_xft = "Sans:bold:pixelsize=%.4f";    // 10.6667
static const char *def_medium_font_xft = "Sans:pixelsize=%.4f";          // 13.3333
static const char *def_medium_b_font_xft = "Sans:bold:pixelsize=%.4f";   // 13.3333
static const char *def_large_font_xft = "Sans:pixelsize=%.4f";           // 21.3333
static const char *def_large_b_font_xft = "Sans:bold:pixelsize=%.4f";    // 21.3333
static const char *def_big_font_xft = "Sans:pixelsize=37.3333";          // 37.3333
static const char *def_big_b_font_xft = "Sans:bold:pixelsize=37.33333";  // 37.3333
static const char *def_clock_font_xft = "Sans:pixelsize=%.4f";           // 16

#define default_font_xft2 "-microsoft-verdana-*-*-*-*-*-*-*-*-*-*-*-*"

const char* BC_Resources::small_font = 0;
const char* BC_Resources::small_font2 = 0;
const char* BC_Resources::medium_font = 0;
const char* BC_Resources::medium_font2 = 0;
const char* BC_Resources::large_font = 0;
const char* BC_Resources::large_font2 = 0;
const char* BC_Resources::big_font = 0;
const char* BC_Resources::big_font2 = 0;
const char* BC_Resources::clock_font = 0;
const char* BC_Resources::clock_font2 = 0;
const char* BC_Resources::small_fontset = 0;
const char* BC_Resources::medium_fontset = 0;
const char* BC_Resources::large_fontset = 0;
const char* BC_Resources::big_fontset = 0;
const char* BC_Resources::clock_fontset = 0;
const char* BC_Resources::small_font_xft = 0;
const char* BC_Resources::small_font_xft2 = 0;
const char* BC_Resources::small_b_font_xft = 0;
const char* BC_Resources::medium_font_xft = 0;
const char* BC_Resources::medium_font_xft2 = 0;
const char* BC_Resources::medium_b_font_xft = 0;
const char* BC_Resources::large_font_xft = 0;
const char* BC_Resources::large_font_xft2 = 0;
const char* BC_Resources::large_b_font_xft = 0;
const char* BC_Resources::big_font_xft = 0;
const char* BC_Resources::big_font_xft2 = 0;
const char* BC_Resources::big_b_font_xft = 0;
const char* BC_Resources::clock_font_xft = 0;
const char* BC_Resources::clock_font_xft2 = 0;

#define def_font(v, s...) do { sprintf(string,def_##v,s); v = cstrdup(string); } while(0)
#define set_font(v, s) do { sprintf(string, "%s", s); v = cstrdup(string); } while(0)
#define iround(v) ((int)(v+0.5))
void BC_Resources::init_font_defs(double scale)
{
	char string[BCTEXTLEN];
	def_font(small_font,       iround(scale*11));
	def_font(small_font2,      iround(scale*11));
	def_font(medium_font,      iround(scale*14));
	def_font(medium_font2,     iround(scale*14));
	def_font(large_font,       iround(scale*18));
	def_font(large_font2,      iround(scale*20));
	def_font(big_font,         iround(scale*160), iround(scale*160));
	def_font(big_font2,        iround(scale*16), iround(scale*16));
	def_font(clock_font,       iround(scale*16));
	def_font(clock_font2,      iround(scale*18));
	def_font(small_fontset,    iround(scale*10));
	def_font(medium_fontset,   iround(scale*14));
	def_font(large_fontset,    iround(scale*18));
	def_font(big_fontset,      iround(scale*24));
	def_font(clock_fontset,    iround(scale*16));
	def_font(small_font_xft,   (scale*10.6667));
	def_font(small_b_font_xft, (scale*10.6667));
	def_font(medium_font_xft,  (scale*13.3333));
	def_font(medium_b_font_xft,(scale*13.3333));
	def_font(large_font_xft,   (scale*21.3333));
	def_font(large_b_font_xft, (scale*21.3333));
	def_font(big_font_xft,     (scale*37.3333));
	def_font(big_b_font_xft,   (scale*37.3333));
	def_font(clock_font_xft,   (scale*16.));

	set_font(small_font_xft2,  default_font_xft2);
	set_font(medium_font_xft2, default_font_xft2);
	set_font(large_font_xft2,  default_font_xft2);
	set_font(big_font_xft2,    default_font_xft2);
	set_font(clock_font_xft2,  default_font_xft2);
}
void BC_Resources::finit_font_defs()
{
	delete [] small_font;
	delete [] small_font2;
	delete [] medium_font;
	delete [] medium_font2;
	delete [] large_font;
	delete [] large_font2;
	delete [] big_font;
	delete [] big_font2;
	delete [] clock_font;
	delete [] clock_font2;
	delete [] small_fontset;
	delete [] medium_fontset;
	delete [] large_fontset;
	delete [] big_fontset;
	delete [] clock_fontset;
	delete [] small_font_xft;
	delete [] small_b_font_xft;
	delete [] medium_font_xft;
	delete [] medium_b_font_xft;
	delete [] large_font_xft;
	delete [] large_b_font_xft;
	delete [] big_font_xft;
	delete [] big_b_font_xft;
	delete [] clock_font_xft;

	delete [] small_font_xft2;
	delete [] medium_font_xft2;
	delete [] large_font_xft2;
	delete [] big_font_xft2;
	delete [] clock_font_xft2;
}


suffix_to_type_t BC_Resources::suffix_to_type[] =
{
    { "aac", ICON_SOUND },
    { "ac3", ICON_SOUND },
    { "dts", ICON_SOUND },
    { "f4a", ICON_SOUND },
    { "flac", ICON_SOUND },
    { "m4a", ICON_SOUND },
    { "mp2", ICON_SOUND },
    { "mp3", ICON_SOUND },
    { "mpc", ICON_SOUND },
    { "oga", ICON_SOUND },
    { "ogg", ICON_SOUND },
    { "opus", ICON_SOUND },
    { "ra", ICON_SOUND },
    { "tta", ICON_SOUND },
    { "vox", ICON_SOUND },
    { "wav", ICON_SOUND },
    { "wma", ICON_SOUND },
    { "3gp", ICON_FILM },
    { "avi", ICON_FILM },
    { "bmp", ICON_FILM },
    { "cr2", ICON_FILM },
    { "dnxhd", ICON_FILM },
    { "drc", ICON_FILM },
    { "dv", ICON_FILM },
    { "dvd", ICON_FILM },
    { "exr", ICON_FILM },
    { "f4v", ICON_FILM },
    { "flv", ICON_FILM },
    { "gif", ICON_FILM },
    { "gxf", ICON_FILM },
    { "h263", ICON_FILM },
    { "h264", ICON_FILM },
    { "h265", ICON_FILM },
    { "hevc", ICON_FILM },
    { "ifo", ICON_FILM },
    { "jpeg", ICON_FILM },
    { "jpg", ICON_FILM },
    { "m2t", ICON_FILM },
    { "m2ts", ICON_FILM },
    { "m2v", ICON_FILM },
    { "m4p", ICON_FILM },
    { "m4v", ICON_FILM },
    { "mkv", ICON_FILM },
    { "mov", ICON_FILM },
    { "mp4", ICON_FILM },
    { "mpe", ICON_FILM },
    { "mpeg", ICON_FILM },
    { "mpg", ICON_FILM },
    { "mpv", ICON_FILM },
    { "mts", ICON_FILM },
    { "mxf", ICON_FILM },
    { "ogv", ICON_FILM },
    { "pcm", ICON_FILM },
    { "pgm", ICON_FILM },
    { "png", ICON_FILM },
    { "ppm", ICON_FILM },
    { "qt", ICON_FILM },
    { "rm", ICON_FILM },
    { "rmvb", ICON_FILM },
    { "rv", ICON_FILM },
    { "swf", ICON_FILM },
    { "tif", ICON_FILM },
    { "tiff", ICON_FILM },
    { "ts",  ICON_FILM },
    { "vob", ICON_FILM },
    { "vts", ICON_FILM },
    { "webm", ICON_FILM },
    { "webp", ICON_FILM },
    { "wmv", ICON_FILM },
    { "xml", ICON_FILM },
    { "y4m", ICON_FILM },
    { 0, 0 },
};

BC_Signals* BC_Resources::signal_handler = 0;
Mutex BC_Resources::fontconfig_lock("BC_Resources::fonconfig_lock");

int BC_Resources::x_error_handler(Display *display, XErrorEvent *event)
{
#if defined(OUTPUT_X_ERROR)
	char string[1024];
	XGetErrorText(event->display, event->error_code, string, 1024);
	fprintf(stderr, "BC_Resources::x_error_handler: error_code=%d opcode=%d,%d %s\n",
		event->error_code,
		event->request_code,
		event->minor_code,
		string);
#endif

	BC_Resources::error = 1;
// This bug only happens in 32 bit mode.
//	if(sizeof(long) == 4)
//		BC_WindowBase::get_resources()->use_xft = 0;
	return 0;
}

int BC_Resources::machine_cpus = 1;

int BC_Resources::get_machine_cpus()
{
	int cpus = 1;
	FILE *proc = fopen("/proc/cpuinfo", "r");
	if( proc ) {
		char string[BCTEXTLEN], *cp;
		while(!feof(proc) && fgets(string, sizeof(string), proc) ) {
			if( !strncasecmp(string, "processor", 9) &&
			    (cp = strchr(string, ':')) != 0 ) {
				int n = atol(cp+1) + 1;
				if( n > cpus ) cpus = n;
			}
			else if( !strncasecmp(string, "cpus detected", 13) &&
			    (cp = strchr(string, ':')) != 0 )
				cpus = atol(cp+1);
		}
		fclose(proc);
	}
	return cpus;
}

void BC_Resources::new_vframes(int n, VFrame *vframes[], ...)
{
	va_list ap;
	va_start(ap, vframes);
	for( int i=0; i<n; ++i )
		vframes[i] = va_arg(ap, VFrame *);
	va_end(ap);
}

VFrame *BC_Resources::default_type_to_icon[6] = { 0, };
VFrame *BC_Resources::default_bar = 0;
VFrame *BC_Resources::default_cancel_images[3] = { 0, };
VFrame *BC_Resources::default_ok_images[3] = { 0, };
VFrame *BC_Resources::default_usethis_images[3] = { 0, };
#if 0
VFrame *BC_Resources::default_checkbox_images[5] = { 0, };
VFrame *BC_Resources::default_radial_images[5] = { 0, };
VFrame *BC_Resources::default_label_images[5] = { 0, };
#endif
VFrame *BC_Resources::default_menuitem_data[3] = { 0, };
VFrame *BC_Resources::default_menubar_data[3] = { 0, };
VFrame *BC_Resources::default_menu_popup_bg = 0;
VFrame *BC_Resources::default_menu_bar_bg = 0;
VFrame *BC_Resources::default_check_image = 0;
VFrame *BC_Resources::default_filebox_text_images[3] = { 0, };
VFrame *BC_Resources::default_filebox_icons_images[3] = { 0, };
VFrame *BC_Resources::default_filebox_updir_images[3] = { 0, };
VFrame *BC_Resources::default_filebox_newfolder_images[3] = { 0, };
VFrame *BC_Resources::default_filebox_rename_images[3] = { 0, };
VFrame *BC_Resources::default_filebox_delete_images[3] = { 0, };
VFrame *BC_Resources::default_filebox_reload_images[3] = { 0, };
VFrame *BC_Resources::default_filebox_szfmt_images[12] = { 0, };
VFrame *BC_Resources::default_listbox_button[4] = { 0, };
VFrame *BC_Resources::default_listbox_bg = 0;
VFrame *BC_Resources::default_listbox_expand[5] = { 0, };
VFrame *BC_Resources::default_listbox_column[3] = { 0, };
VFrame *BC_Resources::default_listbox_up = 0;
VFrame *BC_Resources::default_listbox_dn = 0;
VFrame *BC_Resources::default_pot_images[3] = { 0, };
VFrame *BC_Resources::default_progress_images[2] = { 0, };
VFrame *BC_Resources::default_medium_7segment[20] = { 0, };
VFrame *BC_Resources::default_vscroll_data[10] = { 0, };
VFrame *BC_Resources::default_hscroll_data[10] = { 0, };
VFrame *BC_Resources::default_icon_img = 0;

BC_Resources::BC_Resources()
{
	synchronous = 0;
	vframe_shm = 0;
	double default_scale = 1.0; // display_size/1000.;
	char *env = getenv("BC_FONT_DEBUG");
	font_debug = env ? atoi(env) : 0;
	env = getenv("BC_FONT_SCALE");
	font_scale = env ? atof(env) : default_scale;
	if( font_scale <= 0 ) font_scale = 1;
	init_font_defs(font_scale);
	env = getenv("BC_ICON_SCALE");
	icon_scale = env ? atof(env) : default_scale;
	if( icon_scale <= 0 ) icon_scale = 1;

	id_lock = new Mutex("BC_Resources::id_lock");
	create_window_lock = new Mutex("BC_Resources::create_window_lock", 1);
	id = 0;
	machine_cpus = get_machine_cpus();

	for(int i = 0; i < FILEBOX_HISTORY_SIZE; i++)
		filebox_history[i].path[0] = 0;

#ifdef HAVE_XFT
	xftInit(0);
	xftInitFtLibrary();
#endif

	little_endian = (*(const u_int32_t*)"\01\0\0\0") & 1;
	wide_encoding = little_endian ?  "UTF32LE" : "UTF32BE";
	use_xvideo = 1;

#include "images/file_folder_png.h"
#include "images/file_unknown_png.h"
#include "images/file_film_png.h"
#include "images/file_sound_png.h"
#include "images/file_label_png.h"
#include "images/file_column_png.h"
new_vframes(6,default_type_to_icon,
	new VFramePng(file_folder_png),
	new VFramePng(file_unknown_png),
	new VFramePng(file_film_png),
	new VFramePng(file_sound_png),
	new VFramePng(file_label_png),
	new VFramePng(file_column_png));

#include "images/bar_png.h"
	default_bar = new VFramePng(bar_png);

#include "images/cancel_up_png.h"
#include "images/cancel_hi_png.h"
#include "images/cancel_dn_png.h"
new_vframes(3,default_cancel_images,
	new VFramePng(cancel_up_png),
	new VFramePng(cancel_hi_png),
	new VFramePng(cancel_dn_png));

#include "images/ok_up_png.h"
#include "images/ok_hi_png.h"
#include "images/ok_dn_png.h"
new_vframes(3,default_ok_images,
	new VFramePng(ok_up_png),
	new VFramePng(ok_hi_png),
	new VFramePng(ok_dn_png));

#include "images/usethis_up_png.h"
#include "images/usethis_uphi_png.h"
#include "images/usethis_dn_png.h"
new_vframes(3,default_usethis_images,
	new VFramePng(usethis_up_png),
	new VFramePng(usethis_uphi_png),
	new VFramePng(usethis_dn_png));

#if 0
#include "images/checkbox_checked_png.h"
#include "images/checkbox_dn_png.h"
#include "images/checkbox_checkedhi_png.h"
#include "images/checkbox_up_png.h"
#include "images/checkbox_hi_png.h"
new_vframes(5,default_checkbox_images,
	new VFramePng(checkbox_up_png),
	new VFramePng(checkbox_hi_png),
	new VFramePng(checkbox_checked_png),
	new VFramePng(checkbox_dn_png),
	new VFramePng(checkbox_checkedhi_png));

#include "images/radial_checked_png.h"
#include "images/radial_dn_png.h"
#include "images/radial_checkedhi_png.h"
#include "images/radial_up_png.h"
#include "images/radial_hi_png.h"
new_vframes(5,default_radial_images,
	new VFramePng(radial_up_png),
	new VFramePng(radial_hi_png),
	new VFramePng(radial_checked_png),
	new VFramePng(radial_dn_png),
	new VFramePng(radial_checkedhi_png));

new_vframes(5,default_label_images,
	new VFramePng(radial_up_png),
	new VFramePng(radial_hi_png),
	new VFramePng(radial_checked_png),
	new VFramePng(radial_dn_png),
	new VFramePng(radial_checkedhi_png));
#endif

#include "images/menuitem_up_png.h"
#include "images/menuitem_hi_png.h"
#include "images/menuitem_dn_png.h"
new_vframes(3,default_menuitem_data,
	new VFramePng(menuitem_up_png),
	new VFramePng(menuitem_hi_png),
	new VFramePng(menuitem_dn_png));

#include "images/menubar_up_png.h"
#include "images/menubar_hi_png.h"
#include "images/menubar_dn_png.h"
new_vframes(3,default_menubar_data,
	new VFramePng(menubar_up_png),
	new VFramePng(menubar_hi_png),
	new VFramePng(menubar_dn_png));

#include "images/menu_popup_bg_png.h"
	default_menu_popup_bg = new VFramePng(menu_popup_bg_png);

#include "images/menubar_bg_png.h"
	default_menu_bar_bg = new VFramePng(menubar_bg_png);

#include "images/check_png.h"
	default_check_image = new VFramePng(check_png);

#include "images/file_text_up_png.h"
#include "images/file_text_hi_png.h"
#include "images/file_text_dn_png.h"
new_vframes(3,default_filebox_text_images,
	new VFramePng(file_text_up_png),
	new VFramePng(file_text_hi_png),
	new VFramePng(file_text_dn_png));

#include "images/file_icons_up_png.h"
#include "images/file_icons_hi_png.h"
#include "images/file_icons_dn_png.h"
new_vframes(3,default_filebox_icons_images,
	new VFramePng(file_icons_up_png),
	new VFramePng(file_icons_hi_png),
	new VFramePng(file_icons_dn_png));

#include "images/file_updir_up_png.h"
#include "images/file_updir_hi_png.h"
#include "images/file_updir_dn_png.h"
new_vframes(3,default_filebox_updir_images,
	new VFramePng(file_updir_up_png),
	new VFramePng(file_updir_hi_png),
	new VFramePng(file_updir_dn_png));

#include "images/file_newfolder_up_png.h"
#include "images/file_newfolder_hi_png.h"
#include "images/file_newfolder_dn_png.h"
new_vframes(3,default_filebox_newfolder_images,
	new VFramePng(file_newfolder_up_png),
	new VFramePng(file_newfolder_hi_png),
	new VFramePng(file_newfolder_dn_png));

#include "images/file_rename_up_png.h"
#include "images/file_rename_hi_png.h"
#include "images/file_rename_dn_png.h"
new_vframes(3,default_filebox_rename_images,
	new VFramePng(file_rename_up_png),
	new VFramePng(file_rename_hi_png),
	new VFramePng(file_rename_dn_png));

#include "images/file_delete_up_png.h"
#include "images/file_delete_hi_png.h"
#include "images/file_delete_dn_png.h"
new_vframes(3,default_filebox_delete_images,
	new VFramePng(file_delete_up_png),
	new VFramePng(file_delete_hi_png),
	new VFramePng(file_delete_dn_png));

#include "images/file_reload_up_png.h"
#include "images/file_reload_hi_png.h"
#include "images/file_reload_dn_png.h"
new_vframes(3,default_filebox_reload_images,
	new VFramePng(file_reload_up_png),
	new VFramePng(file_reload_hi_png),
	new VFramePng(file_reload_dn_png));

#include "images/file_size_capb_up_png.h"
#include "images/file_size_capb_hi_png.h"
#include "images/file_size_capb_dn_png.h"
#include "images/file_size_lwrb_up_png.h"
#include "images/file_size_lwrb_hi_png.h"
#include "images/file_size_lwrb_dn_png.h"
#include "images/file_size_semi_up_png.h"
#include "images/file_size_semi_hi_png.h"
#include "images/file_size_semi_dn_png.h"
#include "images/file_size_zero_up_png.h"
#include "images/file_size_zero_hi_png.h"
#include "images/file_size_zero_dn_png.h"
new_vframes(12,default_filebox_szfmt_images,
	new VFramePng(file_size_zero_up_png),
	new VFramePng(file_size_zero_hi_png),
	new VFramePng(file_size_zero_dn_png),
	new VFramePng(file_size_lwrb_up_png),
	new VFramePng(file_size_lwrb_hi_png),
	new VFramePng(file_size_lwrb_dn_png),
	new VFramePng(file_size_capb_up_png),
	new VFramePng(file_size_capb_hi_png),
	new VFramePng(file_size_capb_dn_png),
	new VFramePng(file_size_semi_up_png),
	new VFramePng(file_size_semi_hi_png),
	new VFramePng(file_size_semi_dn_png));

#include "images/listbox_button_dn_png.h"
#include "images/listbox_button_hi_png.h"
#include "images/listbox_button_up_png.h"
#include "images/listbox_button_disabled_png.h"
new_vframes(4,default_listbox_button,
	new VFramePng(listbox_button_up_png),
	new VFramePng(listbox_button_hi_png),
	new VFramePng(listbox_button_dn_png),
	new VFramePng(listbox_button_disabled_png));

default_listbox_bg = 0;

#include "images/listbox_expandchecked_png.h"
#include "images/listbox_expandcheckedhi_png.h"
#include "images/listbox_expanddn_png.h"
#include "images/listbox_expandup_png.h"
#include "images/listbox_expanduphi_png.h"
new_vframes(5,default_listbox_expand,
	new VFramePng(listbox_expandup_png),
	new VFramePng(listbox_expanduphi_png),
	new VFramePng(listbox_expandchecked_png),
	new VFramePng(listbox_expanddn_png),
	new VFramePng(listbox_expandcheckedhi_png));

#include "images/listbox_columnup_png.h"
#include "images/listbox_columnhi_png.h"
#include "images/listbox_columndn_png.h"
new_vframes(3,default_listbox_column,
	new VFramePng(listbox_columnup_png),
	new VFramePng(listbox_columnhi_png),
	new VFramePng(listbox_columndn_png));

#include "images/listbox_up_png.h"
	default_listbox_up = new VFramePng(listbox_up_png);

#include "images/listbox_dn_png.h"
	default_listbox_dn = new VFramePng(listbox_dn_png);

#include "images/pot_hi_png.h"
#include "images/pot_up_png.h"
#include "images/pot_dn_png.h"
new_vframes(3,default_pot_images,
	new VFramePng(pot_up_png),
	new VFramePng(pot_hi_png),
	new VFramePng(pot_dn_png));

#include "images/progress_up_png.h"
#include "images/progress_hi_png.h"
new_vframes(2,default_progress_images,
	new VFramePng(progress_up_png),
	new VFramePng(progress_hi_png));

#include "images/7seg_small/0_png.h"
#include "images/7seg_small/1_png.h"
#include "images/7seg_small/2_png.h"
#include "images/7seg_small/3_png.h"
#include "images/7seg_small/4_png.h"
#include "images/7seg_small/5_png.h"
#include "images/7seg_small/6_png.h"
#include "images/7seg_small/7_png.h"
#include "images/7seg_small/8_png.h"
#include "images/7seg_small/9_png.h"
#include "images/7seg_small/colon_png.h"
#include "images/7seg_small/period_png.h"
#include "images/7seg_small/a_png.h"
#include "images/7seg_small/b_png.h"
#include "images/7seg_small/c_png.h"
#include "images/7seg_small/d_png.h"
#include "images/7seg_small/e_png.h"
#include "images/7seg_small/f_png.h"
#include "images/7seg_small/space_png.h"
#include "images/7seg_small/dash_png.h"
new_vframes(20,default_medium_7segment,
	new VFramePng(_0_png),
	new VFramePng(_1_png),
	new VFramePng(_2_png),
	new VFramePng(_3_png),
	new VFramePng(_4_png),
	new VFramePng(_5_png),
	new VFramePng(_6_png),
	new VFramePng(_7_png),
	new VFramePng(_8_png),
	new VFramePng(_9_png),
	new VFramePng(colon_png),
	new VFramePng(period_png),
	new VFramePng(a_png),
	new VFramePng(b_png),
	new VFramePng(c_png),
	new VFramePng(d_png),
	new VFramePng(e_png),
	new VFramePng(f_png),
	new VFramePng(space_png),
	new VFramePng(dash_png));

#include "images/hscroll_handle_up_png.h"
#include "images/hscroll_handle_hi_png.h"
#include "images/hscroll_handle_dn_png.h"
#include "images/hscroll_handle_bg_png.h"
#include "images/hscroll_left_up_png.h"
#include "images/hscroll_left_hi_png.h"
#include "images/hscroll_left_dn_png.h"
#include "images/hscroll_right_up_png.h"
#include "images/hscroll_right_hi_png.h"
#include "images/hscroll_right_dn_png.h"
new_vframes(10,default_hscroll_data,
	new VFramePng(hscroll_handle_up_png),
	new VFramePng(hscroll_handle_hi_png),
	new VFramePng(hscroll_handle_dn_png),
	new VFramePng(hscroll_handle_bg_png),
	new VFramePng(hscroll_left_up_png),
	new VFramePng(hscroll_left_hi_png),
	new VFramePng(hscroll_left_dn_png),
	new VFramePng(hscroll_right_up_png),
	new VFramePng(hscroll_right_hi_png),
	new VFramePng(hscroll_right_dn_png));

#include "images/vscroll_handle_up_png.h"
#include "images/vscroll_handle_hi_png.h"
#include "images/vscroll_handle_dn_png.h"
#include "images/vscroll_handle_bg_png.h"
#include "images/vscroll_left_up_png.h"
#include "images/vscroll_left_hi_png.h"
#include "images/vscroll_left_dn_png.h"
#include "images/vscroll_right_up_png.h"
#include "images/vscroll_right_hi_png.h"
#include "images/vscroll_right_dn_png.h"
new_vframes(10,default_vscroll_data,
	new VFramePng(vscroll_handle_up_png),
	new VFramePng(vscroll_handle_hi_png),
	new VFramePng(vscroll_handle_dn_png),
	new VFramePng(vscroll_handle_bg_png),
	new VFramePng(vscroll_left_up_png),
	new VFramePng(vscroll_left_hi_png),
	new VFramePng(vscroll_left_dn_png),
	new VFramePng(vscroll_right_up_png),
	new VFramePng(vscroll_right_hi_png),
	new VFramePng(vscroll_right_dn_png));

#include "images/default_icon_png.h"
	default_icon_img = new VFramePng(default_icon_png);

	type_to_icon = default_type_to_icon;
	bar_data = default_bar;
	check = default_check_image;
	listbox_button = default_listbox_button;
	listbox_bg = default_listbox_bg;
	listbox_expand = default_listbox_expand;
	listbox_column = default_listbox_column;
	listbox_up = default_listbox_up;
	listbox_dn = default_listbox_dn;
	hscroll_data = default_hscroll_data;
	vscroll_data = default_vscroll_data;
	default_icon = default_icon_img;

	listbox_title_overlap = 0;
	listbox_title_margin = 0;
	listbox_title_color = BLACK;
	listbox_title_hotspot = 5;

	listbox_border1 = DKGREY;
	listbox_border2_hi = RED;
	listbox_border2 = BLACK;
	listbox_border3_hi = RED;
	listbox_border3 = MEGREY;
	listbox_border4 = WHITE;
	listbox_selected = BLUE;
	listbox_highlighted = LTGREY;
	listbox_inactive = WHITE;
	listbox_text = BLACK;

	pan_data = 0;
	pan_text_color = YELLOW;

	generic_button_margin = 15;
	draw_clock_background=1;

	use_shm = -1;
	shm_reply = 1;

// Initialize
	bg_color = ORANGE;
	bg_shadow1 = DKGREY;
	bg_shadow2 = BLACK;
	bg_light1 = WHITE;
	bg_light2 = bg_color;


	border_light1 = bg_color;
	border_light2 = MEGREY;
	border_shadow1 = BLACK;
	border_shadow2 = bg_color;

	default_text_color = BLACK;
	disabled_text_color = DMGREY;

	button_light = MEGREY;           // bright corner
//	button_highlighted = LTGREY;  // face when highlighted
	button_highlighted = 0xffe000;  // face when highlighted
	button_down = MDGREY;         // face when down
//	button_up = MEGREY;           // face when up
	button_up = 0xffc000;           // face when up
	button_shadow = BLACK;       // dark corner
	button_uphighlighted = RED;   // upper side when highlighted

	tumble_data = 0;
	tumble_duration = 150;

	ok_images = default_ok_images;
	cancel_images = default_cancel_images;
	usethis_button_images = default_usethis_images;
	filebox_descend_images = default_ok_images;

	menu_light = LTCYAN;
	menu_highlighted = LTBLUE;
	menu_down = MDCYAN;
	menu_up = MECYAN;
	menu_shadow = DKCYAN;

	menu_title_bg = default_menubar_data;
	menu_popup_bg = default_menu_popup_bg;
	menu_bar_bg = default_menu_bar_bg;

	popupmenu_images = 0;


	popupmenu_margin = 10;
	popupmenu_btnup = 1;
	popupmenu_triangle_margin = 10;

	min_menu_w = 0;
	menu_title_text = BLACK;
	popup_title_text = BLACK;
	menu_item_text = BLACK;
	menu_highlighted_fontcolor = BLACK;
	progress_text = BLACK;
	grab_input_focus = 1;

	text_default = BLACK;
	highlight_inverse = WHITE ^ BLUE;
	text_background = WHITE;
	text_background_disarmed = 0xc08080;
	text_background_hi = LTYELLOW;
	text_background_noborder_hi = LTGREY;
	text_background_noborder = -1;
	text_border1 = DKGREY;
	text_border2 = BLACK;
	text_border2_hi = RED;
	text_border3 = MEGREY;
	text_border3_hi = LTPINK;
	text_border4 = WHITE;
	text_highlight = BLUE;
	text_selected_highlight = SLBLUE;
	text_inactive_highlight = MEGREY;

	toggle_highlight_bg = 0;
	toggle_text_margin = 0;

// Delays must all be different for repeaters
	double_click = 300;
	blink_rate = 250;
	scroll_repeat = 150;
	tooltip_delay = 1000;
	tooltip_bg_color = YELLOW;
	tooltips_enabled = 1;
	textbox_focus_policy = 0;

	filebox_margin = 110;
	dirbox_margin = 90;
	filebox_mode = LISTBOX_TEXT;
	sprintf(filebox_filter, "*");
	filebox_w = 640;
	filebox_h = 480;
	filebox_columntype[0] = FILEBOX_NAME;
	filebox_columntype[1] = FILEBOX_SIZE;
	filebox_columntype[2] = FILEBOX_DATE;
	filebox_columntype[3] = FILEBOX_EXTENSION;
	filebox_columnwidth[0] = 200;
	filebox_columnwidth[1] = 100;
	filebox_columnwidth[2] = 100;
	filebox_columnwidth[3] = 100;
	dirbox_columntype[0] = FILEBOX_NAME;
	dirbox_columntype[1] = FILEBOX_DATE;
	dirbox_columnwidth[0] = 200;
	dirbox_columnwidth[1] = 100;

	filebox_text_images = default_filebox_text_images;
	filebox_icons_images = default_filebox_icons_images;
	filebox_updir_images = default_filebox_updir_images;
	filebox_newfolder_images = default_filebox_newfolder_images;
	filebox_rename_images = default_filebox_rename_images;
	filebox_delete_images = default_filebox_delete_images;
	filebox_reload_images = default_filebox_reload_images;
	filebox_szfmt_images = default_filebox_szfmt_images;
	filebox_size_format = FILEBOX_SIZE_RAW;
	directory_color = BLUE;
	file_color = BLACK;

	filebox_sortcolumn = 0;
	filebox_sortorder = BC_ListBox::SORT_ASCENDING;
	dirbox_sortcolumn = 0;
	dirbox_sortorder = BC_ListBox::SORT_ASCENDING;


	pot_images = default_pot_images;
	pot_offset = 2;
	pot_x1 = pot_images[0]->get_w() / 2 - pot_offset;
	pot_y1 = pot_images[0]->get_h() / 2 - pot_offset;
	pot_r = pot_x1;
	pot_needle_color = BLACK;

	progress_images = default_progress_images;

	xmeter_images = 0;
	ymeter_images = 0;
	meter_font = SMALLFONT_3D;
	meter_font_color = RED;
	meter_title_w = 20;
	meter_3d = 1;
	medium_7segment = default_medium_7segment;

	audiovideo_color = RED;

	use_fontset = 0;

// Xft has priority over font set
#ifdef HAVE_XFT
// But Xft dies in 32 bit mode after some amount of drawing.
	use_xft = 1;
#else
	use_xft = 0;
#endif


	drag_radius = 10;
	recursive_resizing = 1;


}

void BC_Resources::del_vframes(VFrame *vframes[], int n)
{
	while( --n >= 0 ) delete vframes[n];
}

BC_Resources::~BC_Resources()
{
	delete id_lock;
	delete create_window_lock;
	del_vframes(default_type_to_icon, 6);
	delete default_bar;
	del_vframes(default_cancel_images, 3);
	del_vframes(default_ok_images, 3);
	del_vframes(default_usethis_images, 3);
#if 0
	del_vframes(default_checkbox_images, 5);
	del_vframes(default_radial_images, 5);
	del_vframes(default_label_images, 5);
#endif
	del_vframes(default_menuitem_data, 3);
	del_vframes(default_menubar_data, 3);
	delete default_menu_popup_bg;
	delete default_menu_bar_bg;
	delete default_check_image;
	del_vframes(default_filebox_text_images, 3);
	del_vframes(default_filebox_icons_images, 3);
	del_vframes(default_filebox_updir_images, 3);
	del_vframes(default_filebox_newfolder_images, 3);
	del_vframes(default_filebox_rename_images, 3);
	del_vframes(default_filebox_delete_images, 3);
	del_vframes(default_filebox_reload_images, 3);
	del_vframes(default_filebox_szfmt_images, 12);
	del_vframes(default_listbox_button, 4);
	delete default_listbox_bg;
	del_vframes(default_listbox_expand, 5);
	del_vframes(default_listbox_column, 3);
	delete default_listbox_up;
	delete default_listbox_dn;
	del_vframes(default_pot_images, 3);
	del_vframes(default_progress_images, 2);
	del_vframes(default_medium_7segment, 20);
	del_vframes(default_vscroll_data, 10);
	del_vframes(default_hscroll_data, 10);
	if( fontlist ) {
		fontlist->remove_all_objects();
		delete fontlist;
	}
	delete default_icon_img;
	finit_font_defs();
}

int BC_Resources::initialize_display(BC_WindowBase *window)
{
// Set up IPC cleanup handlers
//	bc_init_ipc();

// Test for shm.  Must come before yuv test
	init_shm(window);
	return 0;
}


void BC_Resources::init_shm(BC_WindowBase *window)
{
	use_shm = 0;
	int (*old_handler)(Display *, XErrorEvent *) =
		XSetErrorHandler(BC_Resources::x_error_handler);

	if(XShmQueryExtension(window->display))
	{
		XShmSegmentInfo test_shm;
		memset(&test_shm,0,sizeof(test_shm));
		XImage *test_image = XShmCreateImage(window->display, window->vis,
			window->default_depth, ZPixmap, (char*)NULL, &test_shm, 5, 5);
		BC_Resources::error = 0;
		test_shm.shmid = shmget(IPC_PRIVATE, 5 * test_image->bytes_per_line, (IPC_CREAT | 0600));
		if(test_shm.shmid != -1) {
			char *data = (char *)shmat(test_shm.shmid, NULL, 0);
			if(data != (void *)-1) {
				shmctl(test_shm.shmid, IPC_RMID, 0);
				test_shm.shmaddr = data;
				test_shm.readOnly = 0;

				if(XShmAttach(window->display, &test_shm)) {
					XSync(window->display, False);
					use_shm = 1;
				}
				shmdt(data);
			}
		}

		XDestroyImage(test_image);
		if(BC_Resources::error) use_shm = 0;
	}
	XSetErrorHandler(old_handler);
}




BC_Synchronous* BC_Resources::get_synchronous()
{
	return synchronous;
}

void BC_Resources::set_synchronous(BC_Synchronous *synchronous)
{
	this->synchronous = synchronous;
}




int BC_Resources::get_bg_color() { return bg_color; }

int BC_Resources::get_bg_shadow1() { return bg_shadow1; }

int BC_Resources::get_bg_shadow2() { return bg_shadow2; }

int BC_Resources::get_bg_light1() { return bg_light1; }

int BC_Resources::get_bg_light2() { return bg_light2; }


int BC_Resources::get_id()
{
	id_lock->lock("BC_Resources::get_id");
	int result = id++;
	id_lock->unlock();
	return result;
}

int BC_Resources::get_filebox_id()
{
	id_lock->lock("BC_Resources::get_filebox_id");
	int result = filebox_id++;
	id_lock->unlock();
	return result;
}


void BC_Resources::set_signals(BC_Signals *signal_handler)
{
	BC_Resources::signal_handler = signal_handler;
}

int BC_Resources::init_fontconfig(const char *search_path)
{
	if( fontlist ) return 0;
	fontlist = new ArrayList<BC_FontEntry*>;

#define get_str(str,sep,ptr,cond) do { char *out = str; \
  while( *ptr && !strchr(sep,*ptr) && (cond) ) *out++ = *ptr++; \
  *out = 0; \
} while(0)

#define skip_str(sep, ptr) do { \
  while( *ptr && strchr(sep,*ptr) ) ++ptr; \
} while(0)

	char find_command[BCTEXTLEN];
	char *fp = find_command, *ep = fp+sizeof(find_command)-1;
	fp += snprintf(fp, ep-fp, "%s", "find");
	const char *bc_font_path = getenv("BC_FONT_PATH");
// if BC_FONT_PATH starts with ':', omit default path
	if( !(bc_font_path && bc_font_path[0] == ':') )
		fp += snprintf(fp, ep-fp, " '%s'", search_path);
	if( bc_font_path ) {
		const char *path = bc_font_path;
		while( *path ) {
			char font_path[BCTEXTLEN];
	                const char *cp = strchr(path,':');
			int len = !cp ? strlen(path) : cp-path;
			if( len > 0 ) {
				memcpy(font_path, path, len);
				font_path[len] = 0;  path += len;
				fp += snprintf(fp, ep-fp, " '%s'", font_path);
			}
			if( cp ) ++path;
		}
        }
	fp += snprintf(fp, ep-fp, " -name 'fonts.scale' -print -exec cat {} \\;");
	FILE *in = popen(find_command, "r");

	FT_Library freetype_library = 0;
//	FT_Face freetype_face = 0;
//	ft_Init_FreeType(&freetype_library);

	char line[BCTEXTLEN], current_dir[BCTEXTLEN];
	current_dir[0] = 0;

	while( !feof(in) && fgets(line, BCTEXTLEN, in) ) {
		if(!strlen(line)) break;

		char *in_ptr = line;

// Get current directory
		if(line[0] == '/') {
			get_str(current_dir, "\n", in_ptr,1);
			for( int i=strlen(current_dir); --i>=0 && current_dir[i]!='/'; )
				current_dir[i] = 0;
			continue;
		}

//printf("TitleMain::build_fonts %s\n", line);
		BC_FontEntry *entry = new BC_FontEntry;
		char string[BCTEXTLEN];
// Path
		get_str(string, "\n", in_ptr, in_ptr[0]!=' ' || in_ptr[1]!='-');
		entry->path = cstrcat(2, current_dir, string);
// Foundary
		skip_str(" -", in_ptr);
		get_str(string, "-\n", in_ptr, 1);
		if( !string[0] ) { delete entry;  continue; }
		entry->foundry = cstrdup(string);
		if(*in_ptr == '-') in_ptr++;
// Family
		get_str(string, "-\n", in_ptr, 1);
		if( !string[0] ) { delete entry;  continue; }
		entry->family = cstrdup(string);
		if(*in_ptr == '-') in_ptr++;
// Weight
		get_str(string, "-\n", in_ptr, 1);
		entry->weight = cstrdup(string);
		if(*in_ptr == '-') in_ptr++;
// Slant
		get_str(string, "-\n", in_ptr, 1);
		entry->slant = cstrdup(string);
		if(*in_ptr == '-') in_ptr++;
// SWidth
		get_str(string, "-\n", in_ptr, 1);
		entry->swidth = cstrdup(string);
		if(*in_ptr == '-') in_ptr++;
// Adstyle
		get_str(string, "-\n", in_ptr, 1);
		entry->adstyle = cstrdup(string);
		if(*in_ptr == '-') in_ptr++;
// pixelsize
		get_str(string, "-\n", in_ptr, 1);
		entry->pixelsize = atol(string);
		if(*in_ptr == '-') in_ptr++;
// pointsize
		get_str(string, "-\n", in_ptr, 1);
		entry->pointsize = atol(string);
		if(*in_ptr == '-') in_ptr++;
// xres
		get_str(string, "-\n", in_ptr, 1);
		entry->xres = atol(string);
		if(*in_ptr == '-') in_ptr++;
// yres
		get_str(string, "-\n", in_ptr, 1);
		entry->yres = atol(string);
		if(*in_ptr == '-') in_ptr++;
// spacing
		get_str(string, "-\n", in_ptr, 1);
		entry->spacing = cstrdup(string);
		if(*in_ptr == '-') in_ptr++;
// avg_width
		get_str(string, "-\n", in_ptr, 1);
		entry->avg_width = atol(string);
		if(*in_ptr == '-') in_ptr++;
// registry
		get_str(string, "-\n", in_ptr, 1);
		entry->registry = cstrdup(string);
		if(*in_ptr == '-') in_ptr++;
// encoding
		get_str(string, "-\n", in_ptr, 1);
		entry->encoding = cstrdup(string);
		if(*in_ptr == '-') in_ptr++;

// Add to list
//printf("TitleMain::build_fonts 1 %s\n", entry->path);
// This takes a real long time to do.  Instead just take all fonts
// 		if(!load_freetype_face(freetype_library,
// 			freetype_face, entry->path) )
// Fix parameters
		sprintf(line, "%s (%s)", entry->family, entry->foundry);
		entry->displayname = cstrdup(line);

		if(!strcasecmp(entry->weight, "demibold")) {
			entry->fixed_style |= BC_FONT_BOLD;
			entry->style |= FL_WEIGHT_DEMIBOLD;
		}
		else if(!strcasecmp(entry->weight, "bold")) {
			entry->fixed_style |= BC_FONT_BOLD;
			entry->style |= FL_WEIGHT_BOLD;
		}
		else {
			entry->style |= FL_WEIGHT_NORMAL;
		}

		if(!strcasecmp(entry->slant, "r")) {
			entry->style |= FL_SLANT_ROMAN;
		}
		else if(!strcasecmp(entry->slant, "i")) {
			entry->style |= FL_SLANT_ITALIC;
			entry->fixed_style |= BC_FONT_ITALIC;
		}
		else if(!strcasecmp(entry->slant, "o")) {
			entry->style |= FL_SLANT_OBLIQUE;
			entry->fixed_style |= BC_FONT_ITALIC;
		}

		if(!strcasecmp(entry->swidth, "normal"))
			entry->style |= FL_WIDTH_NORMAL;
		else if(!strcasecmp(entry->swidth, "ultracondensed"))
			entry->style |= FL_WIDTH_ULTRACONDENSED;
		else if(!strcasecmp(entry->swidth, "extracondensed"))
			entry->style |= FL_WIDTH_EXTRACONDENSED;
		else if(!strcasecmp(entry->swidth, "condensed"))
			entry->style |= FL_WIDTH_CONDENSED;
		else if(!strcasecmp(entry->swidth, "semicondensed"))
			entry->style |= FL_WIDTH_SEMICONDENSED;
		else if(!strcasecmp(entry->swidth, "semiexpanded"))
			entry->style |= FL_WIDTH_SEMIEXPANDED;
		else if(!strcasecmp(entry->swidth, "expanded"))
			entry->style |= FL_WIDTH_EXPANDED;
		else if(!strcasecmp(entry->swidth, "extraexpanded"))
			entry->style |= FL_WIDTH_EXTRAEXPANDED;
		else if(!strcasecmp(entry->swidth, "ultraexpanded"))
			entry->style |= FL_WIDTH_ULTRAEXPANDED;
		else
			entry->style |= FL_WIDTH_NORMAL;

		fontlist->append(entry);
		if( font_debug )
			dump_font_entry(stdout, "font 0: ", entry);

//		printf("TitleMain::build_fonts %s: success\n",	entry->path);
//printf("TitleMain::build_fonts 2\n");
	}
	pclose(in);

	if( bc_font_path && bc_font_path[0] == ':' )
		return 0;

// Load all the fonts from fontconfig
	FcPattern *pat;
	FcFontSet *fs;
	FcObjectSet *os;
	FcChar8 *family, *file, *foundry, *style, *format;
	int slant, spacing, width, weight;
	int force_style = 0;
// if you want limit search to TrueType put 1
	int limit_to_trutype = 1;
	FcConfig *config;
	int i;
	char tmpstring[BCTEXTLEN];
	if(!fcInit())
		return 1;
	config = fcConfigGetCurrent();
	fcConfigSetRescanInterval(config, 0);

	pat = fcPatternCreate();
	os = fcObjectSetBuild( FC_FAMILY, FC_FILE, FC_FOUNDRY, FC_WEIGHT,
		FC_WIDTH, FC_SLANT, FC_FONTFORMAT, FC_SPACING, FC_STYLE, (char *) 0);
	fcPatternAddBool(pat, FC_SCALABLE, true);

	if(language[0]) {
		char langstr[LEN_LANG * 3];
		strcpy(langstr, language);

		if(region[0]) {
			strcat(langstr, "-");
			strcat(langstr, region);
		}

		FcLangSet *ls =  fcLangSetCreate();
		if(fcLangSetAdd(ls, (const FcChar8*)langstr))
		if(fcPatternAddLangSet(pat, FC_LANG, ls))
		fcLangSetDestroy(ls);
	}

	fs = fcFontList(config, pat, os);
	fcPatternDestroy(pat);
	fcObjectSetDestroy(os);

	for (i = 0; fs && i < fs->nfont; i++) {
		FcPattern *font = fs->fonts[i];
		force_style = 0;
		fcPatternGetString(font, FC_FONTFORMAT, 0, &format);
		//on this point you can limit font search
		if(limit_to_trutype && strcmp((char *)format, "TrueType"))
			continue;

		sprintf(tmpstring, "%s", format);
		BC_FontEntry *entry = new BC_FontEntry;
		if(fcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
			entry->path = cstrdup((char*)file);
		}

		if(fcPatternGetString(font, FC_FOUNDRY, 0, &foundry) == FcResultMatch) {
			entry->foundry = cstrdup((char*)foundry);
		}

		if(fcPatternGetInteger(font, FC_WEIGHT, 0, &weight) == FcResultMatch) {
			switch(weight) {
			case FC_WEIGHT_THIN:
			case FC_WEIGHT_EXTRALIGHT:
			case FC_WEIGHT_LIGHT:
			case FC_WEIGHT_BOOK:
				force_style = 1;
				entry->weight = cstrdup("medium");
				break;

			case FC_WEIGHT_NORMAL:
			case FC_WEIGHT_MEDIUM:
			default:
				entry->weight = cstrdup("medium");
				break;

			case FC_WEIGHT_BLACK:
			case FC_WEIGHT_SEMIBOLD:
			case FC_WEIGHT_BOLD:
				entry->weight = cstrdup("bold");
				entry->fixed_style |= BC_FONT_BOLD;
				break;

			case FC_WEIGHT_EXTRABOLD:
			case FC_WEIGHT_EXTRABLACK:
				force_style = 1;
				entry->weight = cstrdup("bold");
				entry->fixed_style |= BC_FONT_BOLD;
				break;
			}
		}

		if(fcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch)
			entry->family = cstrdup((char*)family);

		if(fcPatternGetInteger(font, FC_SLANT, 0, &slant) == FcResultMatch) {
			switch(slant) {
			case FC_SLANT_ROMAN:
			default:
				entry->slant = cstrdup("r");
				entry->style |= FL_SLANT_ROMAN;
				break;
			case FC_SLANT_ITALIC:
				entry->slant = cstrdup("i");
				entry->style |= FL_SLANT_ITALIC;
				entry->fixed_style |= BC_FONT_ITALIC;
				break;
			case FC_SLANT_OBLIQUE:
				entry->slant = cstrdup("o");
				entry->style |= FL_SLANT_OBLIQUE;
				entry->fixed_style |= BC_FONT_ITALIC;
				break;
			}
		}

		if(fcPatternGetInteger(font, FC_WIDTH, 0, &width) == FcResultMatch) {
			switch(width) {
			case FC_WIDTH_ULTRACONDENSED:
				entry->swidth = cstrdup("ultracondensed");
				break;

			case FC_WIDTH_EXTRACONDENSED:
				entry->swidth = cstrdup("extracondensed");
				break;

			case FC_WIDTH_CONDENSED:
				entry->swidth = cstrdup("condensed");
				break;
			case FC_WIDTH_SEMICONDENSED:
				entry->swidth = cstrdup("semicondensed");
				break;

			case FC_WIDTH_NORMAL:
			default:
				entry->swidth = cstrdup("normal");
				break;

			case FC_WIDTH_SEMIEXPANDED:
				entry->swidth = cstrdup("semiexpanded");
				break;

			case FC_WIDTH_EXPANDED:
				entry->swidth = cstrdup("expanded");
				break;

			case FC_WIDTH_EXTRAEXPANDED:
				entry->swidth = cstrdup("extraexpanded");
				break;

			case FC_WIDTH_ULTRAEXPANDED:
				entry->swidth = cstrdup("ultraexpanded");
				break;
			}
		}

		if(fcPatternGetInteger(font, FC_SPACING, 0, &spacing) == FcResultMatch) {
			switch(spacing) {
			case 0:
			default:
				entry->spacing = cstrdup("p");
				break;

			case 90:
				entry->spacing = cstrdup("d");
				break;

			case 100:
				entry->spacing = cstrdup("m");
				break;

			case 110:
				entry->spacing = cstrdup("c");
				break;
			}
		}

		// Add fake stuff for compatibility
		entry->adstyle = cstrdup(" ");
		entry->pixelsize = 0;
		entry->pointsize = 0;
		entry->xres = 0;
		entry->yres = 0;
		entry->avg_width = 0;
		entry->registry = cstrdup("utf");
		entry->encoding = cstrdup("8");

		if( fcPatternGetString(font, FC_STYLE, 0, &style) != FcResultMatch )
			force_style = 0;

		// If font has a style unmanaged by titler plugin, force style to be displayed on name
		// in this way we can shown all available fonts styles.
		if(force_style) {
			sprintf(tmpstring, "%s (%s)", entry->family, style);
			entry->displayname = cstrdup(tmpstring);
		}
		else {
			if(strcmp(entry->foundry, "unknown")) {
				sprintf(tmpstring, "%s (%s)", entry->family, entry->foundry);
				entry->displayname = cstrdup(tmpstring);
			}
			else {
				sprintf(tmpstring, "%s", entry->family);
				entry->displayname = cstrdup(tmpstring);
			}

		}
		fontlist->append(entry);
		if( font_debug )
			dump_font_entry(stdout, "font 1: ", entry);
	}

	fcFontSetDestroy(fs);
	if(freetype_library)
		ft_Done_FreeType(freetype_library);
// for(int i = 0; i < fonts->total; i++)
//	fonts->values[i]->dump();

	fcConfigAppFontAddDir(0, (const FcChar8*)search_path);
	fcConfigSetRescanInterval(0, 0);

	os = fcObjectSetBuild(FC_FAMILY, FC_FILE, FC_FOUNDRY, FC_WEIGHT,
		FC_WIDTH, FC_SLANT, FC_SPACING, FC_STYLE, (char *)0);
	pat = fcPatternCreate();
	fcPatternAddBool(pat, FC_SCALABLE, true);

	if(language[0])
	{
		char langstr[LEN_LANG * 3];
		strcpy(langstr, language);

		if(region[0])
		{
			strcat(langstr, "-");
			strcat(langstr, region);
		}

		FcLangSet *ls =  fcLangSetCreate();
		if(fcLangSetAdd(ls, (const FcChar8*)langstr))
			if(fcPatternAddLangSet(pat, FC_LANG, ls))
		fcLangSetDestroy(ls);
	}

	fs = fcFontList(0, pat, os);
	fcPatternDestroy(pat);
	fcObjectSetDestroy(os);

	for(int i = 0; i < fs->nfont; i++)
	{
		FcPattern *font = fs->fonts[i];
		BC_FontEntry *entry = new BC_FontEntry;

		FcChar8 *strvalue;
		if(fcPatternGetString(font, FC_FILE, 0, &strvalue) == FcResultMatch)
		{
			entry->path = new char[strlen((char*)strvalue) + 1];
			strcpy(entry->path, (char*)strvalue);
		}

		if(fcPatternGetString(font, FC_FOUNDRY, 0, &strvalue) == FcResultMatch)
		{
			entry->foundry = new char[strlen((char*)strvalue) + 1];
			strcpy(entry->foundry, (char *)strvalue);
		}

		if(fcPatternGetString(font, FC_FAMILY, 0, &strvalue) == FcResultMatch)
		{
			entry->family = new char[strlen((char*)strvalue) + 2];
			strcpy(entry->family, (char*)strvalue);
		}

		int intvalue;
		if(fcPatternGetInteger(font, FC_SLANT, 0, &intvalue) == FcResultMatch)
		{
			switch(intvalue)
			{
			case FC_SLANT_ROMAN:
			default:
				entry->style |= FL_SLANT_ROMAN;
				break;

			case FC_SLANT_ITALIC:
				entry->style |= FL_SLANT_ITALIC;
				break;

			case FC_SLANT_OBLIQUE:
				entry->style |= FL_SLANT_OBLIQUE;
				break;
			}
		}

		if(fcPatternGetInteger(font, FC_WEIGHT, 0, &intvalue) == FcResultMatch)
		{
			switch(intvalue)
			{
			case FC_WEIGHT_THIN:
				entry->style |= FL_WEIGHT_THIN;
				break;

			case FC_WEIGHT_EXTRALIGHT:
				entry->style |= FL_WEIGHT_EXTRALIGHT;
				break;

			case FC_WEIGHT_LIGHT:
				entry->style |= FL_WEIGHT_LIGHT;
				break;

			case FC_WEIGHT_BOOK:
				entry->style |= FL_WEIGHT_BOOK;
				break;

			case FC_WEIGHT_NORMAL:
			default:
				entry->style |= FL_WEIGHT_NORMAL;
				break;

			case FC_WEIGHT_MEDIUM:
				entry->style |= FL_WEIGHT_MEDIUM;
				break;

			case FC_WEIGHT_DEMIBOLD:
				entry->style |= FL_WEIGHT_DEMIBOLD;
				break;

			case FC_WEIGHT_BOLD:
				entry->style |= FL_WEIGHT_BOLD;
				break;

			case FC_WEIGHT_EXTRABOLD:
				entry->style |= FL_WEIGHT_EXTRABOLD;
				break;

			case FC_WEIGHT_BLACK:
				entry->style |= FL_WEIGHT_BLACK;
				break;

			case FC_WEIGHT_EXTRABLACK:
				entry->style |= FL_WEIGHT_EXTRABLACK;
				break;
			}
		}

		if(fcPatternGetInteger(font, FC_WIDTH, 0, &intvalue) == FcResultMatch)
		{
			switch(intvalue)
			{
			case FC_WIDTH_ULTRACONDENSED:
				entry->style |= FL_WIDTH_ULTRACONDENSED;
				break;

			case FC_WIDTH_EXTRACONDENSED:
				entry->style |= FL_WIDTH_EXTRACONDENSED;
				break;

			case FC_WIDTH_CONDENSED:
				entry->style |= FL_WIDTH_CONDENSED;
				break;

			case FC_WIDTH_SEMICONDENSED:
				entry->style = FL_WIDTH_SEMICONDENSED;
				break;

			case FC_WIDTH_NORMAL:
			default:
				entry->style |= FL_WIDTH_NORMAL;
				break;

			case FC_WIDTH_SEMIEXPANDED:
				entry->style |= FL_WIDTH_SEMIEXPANDED;
				break;

			case FC_WIDTH_EXPANDED:
				entry->style |= FL_WIDTH_EXPANDED;
				break;

			case FC_WIDTH_EXTRAEXPANDED:
				entry->style |= FL_WIDTH_EXTRAEXPANDED;
				break;

			case FC_WIDTH_ULTRAEXPANDED:
				entry->style |= FL_WIDTH_ULTRAEXPANDED;
				break;
			}
		}
		if(fcPatternGetInteger(font, FC_SPACING, 0, &intvalue) == FcResultMatch)
		{
			switch(intvalue)
			{
			case FC_PROPORTIONAL:
			default:
				entry->style |= FL_PROPORTIONAL;
				break;

			case FC_DUAL:
				entry->style |= FL_DUAL;
				break;

			case FC_MONO:
				entry->style |= FL_MONO;
				break;

			case FC_CHARCELL:
				entry->style |= FL_CHARCELL;
				break;
			}
		}
		if(entry->foundry && strcmp(entry->foundry, "unknown"))
		{
			char tempstr[BCTEXTLEN];
			sprintf(tempstr, "%s (%s)", entry->family, entry->foundry);
			entry->displayname = new char[strlen(tempstr) + 1];
			strcpy(entry->displayname, tempstr);
		}
		else
		{
			entry->displayname = new char[strlen(entry->family) + 1];
			strcpy(entry->displayname, entry->family);
		}
		fontlist->append(entry);
		if( font_debug )
			dump_font_entry(stdout, "font 2: ", entry);
	}
	fcFontSetDestroy(fs);
	return 0;
}

#define STYLE_MATCH(fst, stl, msk) ((fst) & (msk) & (stl)) && \
       !((fst) & ~(style) & (msk))

BC_FontEntry *BC_Resources::find_fontentry(const char *displayname, int style,
	int mask, int preferred)
{
	BC_FontEntry *entry, *style_match, *preferred_match;

	if(!fontlist)
		return 0;

	style_match = 0;
	preferred_match = 0;

	if(displayname)
	{
		for(int i = 0; i < fontlist->total; i++)
		{
			entry = fontlist->values[i];

			if(strcmp(entry->displayname, displayname) == 0 &&
					STYLE_MATCH(entry->style, style, mask))
			{
				if(!style_match)
					style_match = entry;
				if(!preferred_match && entry->fixed_style == preferred)
					preferred_match = entry;
			}
		}
		if(preferred_match)
			return preferred_match;

		if(style_match)
			return style_match;
	}

// No exact match - assume normal width font
	style |= FL_WIDTH_NORMAL;
	mask |= FL_WIDTH_MASK;
	style_match = 0;
	preferred_match = 0;

	for(int i = 0; i < fontlist->total; i++)
	{
		entry = fontlist->values[i];

		if(STYLE_MATCH(entry->style, style, mask))
		{
			if(!style_match)
				style_match = entry;

			if(!preferred_match && (entry->style & preferred))
				preferred_match = entry;

			if(!strncasecmp(displayname, entry->family,
					strlen(entry->family)))
			return entry;
		}
	}

	if(preferred_match)
		return preferred_match;

	return style_match;
}


class utf8conv {
	uint8_t *obfr, *out, *oend;
	uint8_t *ibfr, *inp, *iend;
public:
	utf8conv(void *out, int olen, void *inp, int ilen) {
		this->obfr = this->out = (uint8_t*)out;
		this->oend = this->out + olen;
		this->ibfr = this->inp = (uint8_t*)inp;
		this->iend = this->inp + ilen;
	}
	int cur() { return inp>=iend ? -1 : *inp; }
	int next() { return inp>=iend ? -1 : *inp++; }
	int next(int ch) { return out>=oend ? -1 : *out++ = ch; }
	int ilen() { return inp-ibfr; }
	int olen() { return out-obfr; }
	int wnext();
	int wnext(unsigned int v);
};

int utf8conv::
wnext(unsigned int v)
{
  if( v < 0x00000080 ) { next(v);  return 1; }
  int n = v < 0x00000800 ? 2 : v < 0x00010000 ? 3 :
          v < 0x00200000 ? 4 : v < 0x04000000 ? 5 : 6;
  int m = (0xff00 >> n), i = n-1;
  next((v>>(6*i)) | m);
  while( --i >= 0 ) next(((v>>(6*i)) & 0x3f) | 0x80);
  return n;
}

int utf8conv::
wnext()
{
  int v = 0, n = 0, ch = next();
  if( ch >= 0x80 ) {
    static const unsigned char byts[] = {
      1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 5,
    };
    int i = ch - 0xc0;
    n = i<0 ? 0 : byts[i/4];
    for( v=ch, i=n; --i>=0; v+=next() ) v <<= 6;
    static const unsigned int ofs[6] = {
      0x00000000U, 0x00003080U, 0x000E2080U,
      0x03C82080U, 0xFA082080U, 0x82082080U
    };
    v -= ofs[n];
  }
  else
    v = ch;
  return v;
}


size_t BC_Resources::encode(const char *from_enc, const char *to_enc,
	char *input, int input_length, char *output, int output_length)
{
	if( !from_enc || *from_enc == 0 ||
	    !strcmp(from_enc,"UTF8") || !strcmp(from_enc, "US-ASCII") )
		from_enc = "UTF-8";

	if( !to_enc || *to_enc == 0 ||
	    !strcmp(to_enc,"UTF8") || !strcmp(to_enc, "US-ASCII") )
		to_enc = "UTF-8";

	iconv_t cd;
	char *outbase = output;
	size_t inbytes = input_length < 0 ? strlen(input) : input_length;
	size_t outbytes = output_length;

	if( inbytes && outbytes ) {
		if( !strcmp(from_enc, to_enc) ) {
			if( inbytes > outbytes ) inbytes = outbytes;
			memcpy(output,  input, inbytes);
			output += inbytes;
			outbytes -= inbytes;
		}
		else if( !strcmp(from_enc, "UTF-8") && !strcmp(to_enc,"UTF32LE") ) {
			utf8conv uc(0,0, input,inbytes);
			uint32_t *op = (uint32_t *)output;
			uint32_t *ep = (uint32_t *)(output+output_length);
			for( int wch; op<ep && (wch=uc.wnext())>=0; *op++=wch );
			output = (char *)op;
			outbytes = (char*)ep - output;
		}
		else if( !strcmp(from_enc, "UTF32LE") && !strcmp(to_enc,"UTF-8") ) {
			utf8conv uc(output,output_length, 0,0);
			uint32_t *ip = (uint32_t *)input;
			uint32_t *ep = (uint32_t *)(input+inbytes);
			for( ; ip<ep && uc.wnext(*ip)>=0; ++ip );
			output += uc.olen();
			outbytes -= uc.olen();
		}
		else if( (cd = iconv_open(to_enc, from_enc)) != (iconv_t)-1 ) {
			iconv(cd, &input, &inbytes, &output, &outbytes);
			iconv_close(cd);
		}
		else {
			printf(_("Conversion from %s to %s is not available\n"),
				from_enc, to_enc);
		}
	}
	if( outbytes > sizeof(uint32_t) )
		outbytes = sizeof(uint32_t);
	for( uint32_t i = 0; i < outbytes; i++)
		output[i] = 0;
	return output - outbase;
}

void BC_Resources::encode_to_utf8(char *buffer, int buflen)
{
        if(BC_Resources::locale_utf8) return;
	char lbuf[buflen];
	encode(encoding, 0, buffer, buflen, lbuf, buflen);
	strcpy(buffer, lbuf);
}

int BC_Resources::find_font_by_char(FT_ULong char_code, char *path_new, const FT_Face oldface)
{
	FcPattern *font, *ofont;
	FcChar8 *file;
	int result = 0;

	*path_new = 0;

	// Do not search control codes
	if(char_code < ' ')
		return 0;

	if( (ofont = fcFreeTypeQueryFace(oldface, (const FcChar8*)"", 4097, 0)) != 0 ) {
		if( (font = find_similar_font(char_code, ofont)) != 0 ) {
			if(fcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
				strcpy(path_new, (char*)file);
				result = 1;
			}
			fcPatternDestroy(font);
		}
		fcPatternDestroy(ofont);
	}
	return result;
}

FcPattern* BC_Resources::find_similar_font(FT_ULong char_code, FcPattern *oldfont)
{
	FcPattern *pat, *font;
	FcFontSet *fs;
	FcObjectSet *os;
	FcCharSet *fcs;
	int ival;

	// Do not search control codes
	if(char_code < ' ')
		return 0;

	fontconfig_lock.lock("BC_Resources::find_similar_font");
	pat = fcPatternCreate();
	os = fcObjectSetBuild(FC_FILE, FC_CHARSET, FC_SCALABLE, FC_FAMILY,
		FC_SLANT, FC_WEIGHT, FC_WIDTH, (char *)0);

	fcPatternAddBool(pat, FC_SCALABLE, true);
	fcs = fcCharSetCreate();
	if(fcCharSetAddChar(fcs, char_code))
		fcPatternAddCharSet(pat, FC_CHARSET, fcs);
	fcCharSetDestroy(fcs);
	for( int i=0; i<(int)LEN_FCPROP; ++i ) {
		if(fcPatternGetInteger(oldfont, fc_properties[i], 0, &ival) == FcResultMatch)
			fcPatternAddInteger(pat, fc_properties[i], ival);
	}
	fs = fcFontList(0, pat, os);
	for( int i=LEN_FCPROP; --i>=0 && !fs->nfont; ) {
		fcFontSetDestroy(fs);
		fcPatternDel(pat, fc_properties[i]);
		fs = fcFontList(0, pat, os);
	}
	fcPatternDestroy(pat);
	fcObjectSetDestroy(os);

	pat = 0;

	for (int i = 0; i < fs->nfont; i++)
	{
		font = fs->fonts[i];
		if(fcPatternGetCharSet(font, FC_CHARSET, 0, &fcs) == FcResultMatch)
		{
			if(fcCharSetHasChar(fcs, char_code))
			{
				pat =  fcPatternDuplicate(font);
				break;
			}
		}
	}
	fcFontSetDestroy(fs);
	fontconfig_lock.unlock();

	return pat;
}

void BC_Resources::dump_fonts(FILE *fp)
{
	for( int i=0; i<fontlist->total; ++i )
		dump_font_entry(fp, "", fontlist->values[i]);
}

void BC_Resources::dump_font_entry(FILE *fp, const char *cp,  BC_FontEntry *ep)
{
	fprintf(fp,"%s%s = %s\n",cp,ep->displayname,ep->path);
	fprintf(fp,"  %s:%s:%s:%s:%s:%s:%d:%d:%d:%d:%d:%s:%d:%s:%s:%d\n",
		ep->foundry, ep->family, ep->weight, ep->slant, ep->swidth, ep->adstyle,
		ep->pixelsize, ep->pointsize, ep->xres, ep->yres, ep->style, ep->spacing,
		ep->avg_width, ep->registry, ep->encoding, ep->fixed_style);
}

