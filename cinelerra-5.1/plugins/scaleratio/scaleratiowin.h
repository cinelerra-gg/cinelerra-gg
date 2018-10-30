
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

#ifndef TRANSLATEWIN_H
#define TRANSLATEWIN_H

#include "guicast.h"

class ScaleRatioThread;
class ScaleRatioWin;
class ScaleRatioTypeItem;
class ScaleRatioApply;
class ScaleRatioType;
class ScaleRatioCoord;
class ScaleRatioRatio;


#include "filexml.h"
#include "mutex.h"
#include "pluginclient.h"
#include "rescale.h"
#include "scaleratio.h"


class ScaleRatioWin : public PluginClientWindow
{
public:
	ScaleRatioWin(ScaleRatioMain *client);
	~ScaleRatioWin();

	void create_objects();
	void update_config();
	void update_gui();

	ScaleRatioMain *client;
	ScaleRatioCoord *src_x, *src_y, *src_w, *src_h;
	ScaleRatioCoord *dst_x, *dst_y, *dst_w, *dst_h;
	ScaleRatioCoord *in_w, *in_h, *out_w, *out_h;
	ScaleRatioRatio *in_r, *out_r;
	BC_GenericButton *apply_button;
	ScaleRatioType *type_popup;
};

class ScaleRatioCoord : public BC_TumbleTextBox
{
public:
	ScaleRatioCoord(ScaleRatioWin *win, ScaleRatioMain *client,
		int x, int y, int s, float *value);
	~ScaleRatioCoord();
	int handle_event();

	ScaleRatioMain *client;
	ScaleRatioWin *win;
	float *value;
};

class ScaleRatioTumbler : public BC_TumbleTextBox {
public:
	ScaleRatioTumbler(ScaleRatioRatio *ratio, int value, int x, int y);
	~ScaleRatioTumbler();
	int handle_event();
	ScaleRatioRatio *ratio;
};

class ScaleRatioRatio : public BC_TextBox
{
public:
	ScaleRatioRatio(ScaleRatioWin *win, ScaleRatioMain *client,
		int x, int y, float *value);
	~ScaleRatioRatio();
	int handle_event();
	void create_objects();
	void update_ratio();

	ScaleRatioMain *client;
	ScaleRatioWin *win;
	float *value;
	int tx, ty, tw, th;
	ScaleRatioTumbler *aw, *ah;

	int get_tw() { return tw; }
	int get_th() { return th; }
};

class ScaleRatioTypeItem : public BC_MenuItem
{
public:
	ScaleRatioTypeItem(ScaleRatioType *popup, int type, const char *text);
	~ScaleRatioTypeItem();

	int handle_event();

	ScaleRatioType *popup;
	int type;
};

class ScaleRatioApply : public BC_GenericButton {
public:
	ScaleRatioWin *win;

	ScaleRatioApply(ScaleRatioWin *win, int x, int y)
	 : BC_GenericButton(x, y, "Apply") {
		this->win = win;
	}
	~ScaleRatioApply() {}

	int handle_event();
};

class ScaleRatioType : public BC_PopupMenu
{
public:
	ScaleRatioType(ScaleRatioWin *win, int x, int y, int *value);
	~ScaleRatioType();

	void create_objects();
	void update(int v);
	int handle_event();

	ScaleRatioWin *win;
	int *value;

	void set_value(int v) { set_text(Rescale::scale_types[v]); }
};

#endif
