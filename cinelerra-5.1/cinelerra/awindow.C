
/*
 * CINELERRA
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

#include "assetedit.h"
#include "assetremove.h"
#include "awindow.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "clipedit.h"
#include "effectlist.h"
#include "labeledit.h"
#include "labelpopup.h"

AWindow::AWindow(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	asset_remove = 0;
	clip_edit = 0;
	label_edit = 0;
}


AWindow::~AWindow()
{
	if(gui) {
		gui->lock_window("AWindow::~AWindow");
		gui->set_done(0);
		gui->unlock_window();
	}
	Thread::join();
	asset_editors.remove_all_objects();
	delete asset_remove;
	delete label_edit;
	delete clip_edit;
	delete effect_tip;
}

void AWindow::create_objects()
{
	gui = new AWindowGUI(mwindow, this);
	gui->create_objects();
	gui->async_update_assets();
	asset_remove = new AssetRemoveThread(mwindow);
	clip_edit = new ClipEdit(mwindow, this, 0);
	label_edit = new LabelEdit(mwindow, this, 0);
	effect_tip = new EffectTipDialog(mwindow, this);
}

int AWindow::save_defaults(BC_Hash *defaults)
{
	return gui->save_defaults(defaults);
}
int AWindow::load_defaults(BC_Hash *defaults)
{
	return gui->load_defaults(defaults);
}


AssetEdit *AWindow::get_asset_editor()
{
	AssetEdit *asset_edit = 0;
	for( int i=0; !asset_edit && i<asset_editors.size(); ++i ) {
		AssetEdit *thread = asset_editors[i];
		if( !thread->running() ) asset_edit = thread;
	}
	if( !asset_edit ) {
		asset_edit = new AssetEdit(mwindow);
		asset_editors.append(asset_edit);
	}
	return asset_edit;
}


void AWindow::run()
{
	gui->run_window();
	delete gui;  gui = 0;
}
