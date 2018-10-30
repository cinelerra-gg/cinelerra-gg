
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

#ifndef AWINDOW_H
#define AWINDOW_H

#include "arraylist.h"
#include "assetedit.inc"
#include "assetremove.inc"
#include "awindowgui.inc"
#include "bchash.inc"
#include "bcwindowbase.inc"
#include "clipedit.inc"
#include "effectlist.inc"
#include "labeledit.inc"
#include "labelpopup.inc"
#include "mwindow.inc"
#include "thread.h"

class AWindow : public Thread
{
public:
	AWindow(MWindow *mwindow);
	~AWindow();

	void run();
	void create_objects();
	AssetEdit *get_asset_editor();
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);

	AWindowGUI *gui;
	MWindow *mwindow;
	ArrayList<AssetEdit*> asset_editors;
	AssetRemoveThread *asset_remove;
	ClipEdit *clip_edit;
	LabelEdit *label_edit;
	EffectTipDialog *effect_tip;
};

#endif
