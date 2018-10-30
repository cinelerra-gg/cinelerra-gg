
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

#include "bcbutton.h"
#include "bcdialog.h"
#include "bcdisplayinfo.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bctoggle.h"
#include "cstrdup.h"
#include "file.h"
#include "keys.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"
#include "preferences.h"
#include "theme.h"
#include "tipwindow.h"


// Table of tips of the day
static Tips tips;


TipWindow::TipWindow(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
}

TipWindow::~TipWindow()
{
	close_window();
}

void TipWindow::load_tips(const char *lang)
{
	tips.remove_all_objects();
	char tip[0x10000];  tip[0] = 0;
	char string[BCTEXTLEN];
	sprintf(string, "%s/%s.%s", File::get_cindat_path(), TIPS_FILE, lang);
	FILE *fp = fopen(string, "r");
	if( !fp ) {
		sprintf(tip, _("tip file missing:\n %s"), string);
		tips.add(tip);
		return;
	}

	while( fgets(string, sizeof(string), fp) ) {
		if( string[0] == '\n' && string[1] == 0 && strlen(tip) > 0 ) {
			tips.add(tip);
			tip[0] = 0;
			continue;
		}
		strcat(tip, string);
	}

	fclose(fp);
}

void TipWindow::handle_close_event(int result)
{
	gui = 0;
}

BC_Window* TipWindow::new_gui()
{
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x();
	int y = display_info.get_abs_cursor_y();
	TipWindowGUI *gui = this->gui = new TipWindowGUI(mwindow, this, x, y);
	gui->create_objects();
	return gui;
}

const char* TipWindow::get_current_tip(int n)
{
	mwindow->session->current_tip += n;
	if( mwindow->session->current_tip < 0 )
		mwindow->session->current_tip = tips.size()-1;
	else if( mwindow->session->current_tip >= tips.size() )
		mwindow->session->current_tip = 0;
	const char *result = tips[mwindow->session->current_tip];
	mwindow->save_defaults();
	return result;
}

void TipWindow::next_tip()
{
	gui->tip_text->update(get_current_tip(1));
}

void TipWindow::prev_tip()
{
	gui->tip_text->update(get_current_tip(-1));
}



TipWindowGUI::TipWindowGUI(MWindow *mwindow, TipWindow *thread, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Tip of the day"), x, y,
		640, 100, 640, 100, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

void TipWindowGUI::create_objects()
{
	lock_window("TipWindowGUI::create_objects");
	int x = 10, y = 10;
	add_subwindow(tip_text = new BC_Title(x, y, thread->get_current_tip(0)));
	y = get_h() - 30;
	BC_CheckBox *checkbox;
	add_subwindow(checkbox = new TipDisable(mwindow, this, x, y));
	BC_Button *button;
	y = get_h() - TipClose::calculate_h(mwindow) - 10;
	x = get_w() - TipClose::calculate_w(mwindow) - 10;
	add_subwindow(button = new TipClose(mwindow, this, x, y));
	x -= TipNext::calculate_w(mwindow) + 10;
	add_subwindow(button = new TipNext(mwindow, this, x, y));
	x -= TipPrev::calculate_w(mwindow) + 10;
	add_subwindow(button = new TipPrev(mwindow, this, x, y));
	x += button->get_w() + 10;

	show_window();
	raise_window();
	unlock_window();
}

int TipWindowGUI::keypress_event()
{
	switch(get_keypress()) {
	case RETURN:
	case ESC:
	case 'w':
		set_done(0);
		break;
	}
	return 0;
}



TipDisable::TipDisable(MWindow *mwindow, TipWindowGUI *gui, int x, int y)
 : BC_CheckBox(x, y, mwindow->preferences->use_tipwindow,
	_("Show tip of the day."))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int TipDisable::handle_event()
{
	mwindow->preferences->use_tipwindow = get_value();
	return 1;
}



TipNext::TipNext(MWindow *mwindow, TipWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("next_tip"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Next tip"));
}
int TipNext::handle_event()
{
	gui->thread->next_tip();
	return 1;
}

int TipNext::calculate_w(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("next_tip")[0]->get_w();
}



TipPrev::TipPrev(MWindow *mwindow, TipWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("prev_tip"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Previous tip"));
}

int TipPrev::handle_event()
{
	gui->thread->prev_tip();
	return 1;
}
int TipPrev::calculate_w(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("prev_tip")[0]->get_w();
}


TipClose::TipClose(MWindow *mwindow, TipWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("close_tip"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Close"));
}

int TipClose::handle_event()
{
	gui->set_done(0);
	return 1;
}

int TipClose::calculate_w(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("close_tip")[0]->get_w();
}
int TipClose::calculate_h(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("close_tip")[0]->get_h();
}



