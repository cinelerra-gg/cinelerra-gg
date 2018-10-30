
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

#include "clip.h"
#include "language.h"
#include "loadmode.h"
#include "mwindow.h"
#include "theme.h"


// Must match macros
static const char *mode_text[] =
{
	N_("Insert nothing"),
	N_("Replace current project"),
	N_("Replace current project and concatenate tracks"),
	N_("Append in new tracks"),
	N_("Concatenate to existing tracks"),
	N_("Paste at insertion point"),
	N_("Create new resources only"),
	N_("Nest sequence")
};


LoadModeItem::LoadModeItem(const char *text, int value)
 : BC_ListBoxItem(text)
{
	this->value = value;
}


LoadMode::LoadMode(MWindow *mwindow, BC_WindowBase *window,
	int x, int y, int *output, int use_nothing)
{
	this->mwindow = mwindow;
	this->window = window;
	this->x = x;
	this->y = y;
	this->output = output;
	this->use_nothing = use_nothing;
	int mode = LOADMODE_NOTHING;
	if(use_nothing)
		load_modes.append(new LoadModeItem(_(mode_text[mode]), mode));
	while( ++mode < TOTAL_LOADMODES )
		load_modes.append(new LoadModeItem(_(mode_text[mode]), mode));
}

LoadMode::~LoadMode()
{
	delete title;
	delete textbox;
	delete listbox;
	for(int i = 0; i < load_modes.total; i++)
		delete load_modes.values[i];
}

int LoadMode::calculate_w(BC_WindowBase *gui, Theme *theme)
{
	return theme->loadmode_w + 24;
}

int LoadMode::calculate_h(BC_WindowBase *gui, Theme *theme)
{
	return BC_Title::calculate_h(gui, _("Insertion strategy:"), MEDIUMFONT) +
		BC_TextBox::calculate_h(gui, MEDIUMFONT, 1, 1);
}

char* LoadMode::mode_to_text()
{
	for(int i = 0; i < load_modes.total; i++)
	{
		if(load_modes.values[i]->value == *output)
			return load_modes.values[i]->get_text();
	}
	return _("Unknown");
}

int LoadMode::create_objects()
{
	int x = this->x, y = this->y;
	char *default_text;
	default_text = mode_to_text();

	window->add_subwindow(title = new BC_Title(x, y, _("Insertion strategy:")));
	y += title->get_h();
	window->add_subwindow(textbox = new BC_TextBox(x, y,
		mwindow->theme->loadmode_w, 1, default_text));
	x += textbox->get_w();
	window->add_subwindow(listbox = new LoadModeListBox(window, this, x, y));

	return 0;
}

int LoadMode::get_h()
{
	int result = 0;
	result = MAX(result, title->get_h());
	result = MAX(result, textbox->get_h());
	return result;
}

int LoadMode::get_x()
{
	return x;
}

int LoadMode::get_y()
{
	return y;
}

int LoadMode::reposition_window(int x, int y)
{
	this->x = x;
	this->y = y;
	title->reposition_window(x, y);
	y += 20;
	textbox->reposition_window(x, y);
	x += textbox->get_w();
	listbox->reposition_window(x,
		y,
		mwindow->theme->loadmode_w);
	return 0;
}


LoadModeListBox::LoadModeListBox(BC_WindowBase *window,
	LoadMode *loadmode,
	int x,
	int y)
 : BC_ListBox(x,
 	y,
	loadmode->mwindow->theme->loadmode_w,
	150,
	LISTBOX_TEXT,
	(ArrayList<BC_ListBoxItem *>*)&loadmode->load_modes,
	0,
	0,
	1,
	0,
	1)
{
	this->window = window;
	this->loadmode = loadmode;
}

LoadModeListBox::~LoadModeListBox()
{
}

int LoadModeListBox::handle_event()
{
	LoadModeItem *item = (LoadModeItem *)get_selection(0, 0);
	if( item ) {
		loadmode->textbox->update(item->get_text());
		*(loadmode->output) = item->value;
	}
	return 1;
}





