/*
 * CINELERRA
 * Copyright (C) 2014 Adam Williams <broadcast at earthling dot net>
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
#include "clip.h"
#include "language.h"
#include "gaborobj.h"
#include "gaborobjwindow.h"
#include "theme.h"

GaborObjWindow::GaborObjWindow(GaborObj *plugin)
 : PluginClientWindow(plugin, 320, 240, 320, 240, 0)
{
	this->plugin = plugin; 
}

GaborObjWindow::~GaborObjWindow()
{
}

void GaborObjWindow::create_objects()
{
	int x = 10, y = 10;
	BC_Title *title = new BC_Title(x, y, _("GaborObj"));
	add_subwindow(title);
	show_window(1);
}

