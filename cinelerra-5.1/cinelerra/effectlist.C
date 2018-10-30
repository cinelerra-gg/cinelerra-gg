/*
 * CINELERRA
 * Copyright (C) 2006 Pierre Dumuid
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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

#include "awindow.h"
#include "awindowgui.h"
#include "clip.h"
#include "cstrdup.h"
#include "effectlist.h"
#include "guicast.h"
#include "language.h"
#include "mwindow.h"
#include "pluginserver.h"

EffectTipDialog::EffectTipDialog(MWindow *mwindow, AWindow *awindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->awindow = awindow;
	effect = 0;
	text = 0;
}

EffectTipDialog::~EffectTipDialog()
{
	close_window();
	delete [] effect;
	delete [] text;
}

void EffectTipDialog::start(int x, int y, const char *effect, const char *text)
{
	close_window();
	AWindowGUI *gui = awindow->gui;
	char string[BCTEXTLEN];
	sprintf(string, _("Effect info: %s"), _(effect));
	int effect_w = BC_Title::calculate_w(gui, string);
	int text_w = BC_Title::calculate_w(gui, text);
	int text_h = BC_Title::calculate_h(gui, text);
	this->w = bmax(text_w + 30, bmax(effect_w + 30, 120));
	this->h = bmax(text_h + 100, 120);
	this->x = x - this->w / 2;
	this->y = y - this->h / 2;
	delete [] this->effect;  this->effect = cstrdup(string);
	delete [] this->text;    this->text = cstrdup(text);
	BC_DialogThread::start();
}

BC_Window* EffectTipDialog::new_gui()
{
	AWindowGUI *gui = awindow->gui;
	effect_gui = new EffectTipWindow(gui, this);
	effect_gui->create_objects();
	return effect_gui;
};


EffectTipWindow::EffectTipWindow(AWindowGUI *gui, EffectTipDialog *thread)
 : BC_Window(_(PROGRAM_NAME ": Effect Info"),
	thread->x + thread->w/2, thread->y + thread->h/2,
	thread->w, thread->h, thread->w, thread->h, 0, 0, 1)
{
	this->gui = gui;
        this->thread = thread;
}
EffectTipWindow::~EffectTipWindow()
{
}

void EffectTipWindow::create_objects()
{
	lock_window("EffectTipWindow::create_objects");
	int x = 10, y = 10;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, thread->effect));
	y += title->get_h() + 10;
	add_subwindow(tip_text = new BC_Title(x+5, y, thread->text));
	add_subwindow(new BC_OKButton(this));
	show_window(1);
	unlock_window();
}

EffectTipItem::EffectTipItem(AWindowGUI *gui)
 : BC_MenuItem(_("Info"))
{
	this->gui = gui;
}
EffectTipItem::~EffectTipItem()
{
}

int EffectTipItem::handle_event()
{
	AssetPicon *result = (AssetPicon*)gui->asset_list->get_selection(0,0);
	if( result && result->plugin ) {
		const char *info = result->plugin->tip;
		if( !info ) info = _("No info available");
		int cur_x, cur_y;
		gui->get_abs_cursor(cur_x, cur_y, 0);
		gui->awindow->effect_tip->start(cur_x, cur_y,
			result->plugin->title, info);
	}
	return 1;
}


EffectListMenu::EffectListMenu(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

EffectListMenu:: ~EffectListMenu()
{
}

void EffectListMenu::create_objects()
{
	add_item(new EffectTipItem(gui));
	add_item(format = new AWindowListFormat(mwindow, gui));
	add_item(new AWindowListSort(mwindow, gui));
}

void EffectListMenu::update()
{
	format->update();
}

