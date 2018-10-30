
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

#include "aboutprefs.h"
#include "bcsignals.h"
#include "file.h"
#include "language.h"
#include "mwindow.h"
#include "theme.h"
#include "vframe.h"
#include "versioninfo.h"

#ifndef COMPILEDATE
#define COMPILEDATE "built: " __DATE__ " " __TIME__
#endif
const char *AboutPrefs::build_timestamp = COMPILEDATE;

AboutPrefs::AboutPrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
}

AboutPrefs::~AboutPrefs()
{
	about.remove_all_objects();
}

void AboutPrefs::create_objects()
{
	lock_window("AboutPrefs::create_objects");
	int x, y;


	BC_Resources *resources = BC_WindowBase::get_resources();

// 	add_subwindow(new BC_Title(mwindow->theme->preferencestitle_x,
// 		mwindow->theme->preferencestitle_y,
// 		_("About"),
// 		LARGEFONT,
// 		resources->text_default));

	x = mwindow->theme->preferencesoptions_x;
	y = mwindow->theme->preferencesoptions_y +
		get_text_height(LARGEFONT);

	set_font(LARGEFONT);
	set_color(resources->text_default);
	draw_text(x, y, PROGRAM_NAME " " CINELERRA_VERSION);
	y += 2*get_text_height(MEDIUMFONT);

	set_font(MEDIUMFONT);
	draw_text(x, y, COPYRIGHTTEXT1
#if defined(COPYRIGHTTEXT2)
	COPYRIGHTTEXT2
#endif
	);
	y += get_text_height(MEDIUMFONT) * 3;


	const char *cfg_path = File::get_cindat_path();
	char msg_path[BCTEXTLEN];
	FILE *fp = 0;
	if( BC_Resources::language[0] ) {
		snprintf(msg_path, sizeof(msg_path), "%s/msg/%s",
			cfg_path, BC_Resources::language);
		fp = fopen(msg_path, "r");
	}
	if( !fp ) {
		snprintf(msg_path, sizeof(msg_path), "%s/msg/txt",
			cfg_path);
		fp = fopen(msg_path, "r");
	}
	if( fp ) {
		set_font(LARGEFONT);
		draw_text(x, y, _("About:"));
		y += get_text_height(LARGEFONT);
		char msg[BCTEXTLEN];
		while( fgets(msg, sizeof(msg), fp) ) {
			int len = strlen(msg);
			if( len > 0 && msg[len-1] == '\n' ) msg[len-1] = 0;
			about.append(new BC_ListBoxItem(msg));
		}
		BC_ListBox *listbox;
		add_subwindow(listbox = new BC_ListBox(x, y, 450, 280,
			LISTBOX_TEXT, &about, 0, 0, 1));
		y += listbox->get_h() + get_text_height(LARGEFONT) + 10;
	}
	else
		y += 300 + get_text_height(LARGEFONT) + 10;

	set_font(LARGEFONT);
	set_color(resources->text_default);
	draw_text(x, y, _("License:"));
	y += get_text_height(LARGEFONT);
	set_font(MEDIUMFONT);

	char license3[BCTEXTLEN];
	sprintf(license3, _(
"This program is free software; you can redistribute it and/or modify it under the terms\n"
"of the GNU General Public License as published by the Free Software Foundation; either version\n"
"2 of the License, or (at your option) any later version.\n"
"  This software uses libraries from the FFmpeg project under the LGPLv2.1.\n"
"This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;\n"
"without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR\n"
"PURPOSE.  See the GNU General Public License for more details.\n"));
	draw_text(x, y, license3);
	y += get_text_height(MEDIUMFONT, license3);

	draw_text(x, y, build_timestamp);
#if defined(REPOMAINTXT)
	y += get_text_height(MEDIUMFONT, build_timestamp);
	draw_text(x, y, REPOMAINTXT);
#endif

	x = get_w() - mwindow->theme->about_bg->get_w() - 10;
	y = mwindow->theme->preferencesoptions_y;
	BC_Pixmap *temp_pixmap = new BC_Pixmap(this,
		mwindow->theme->about_bg,
		PIXMAP_ALPHA);
	draw_pixmap(temp_pixmap,
		x,
		y);

	delete temp_pixmap;


	x += mwindow->theme->about_bg->get_w() + 10;
	y += get_text_height(LARGEFONT) * 2;


	flash();
	flush();
	unlock_window();
}


