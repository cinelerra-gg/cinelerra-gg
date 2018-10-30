
/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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

#ifndef BLURWINDOW_H
#define BLURWINDOW_H


class BlurThread;
class BlurWindow;

#include "blur.inc"
#include "filexml.inc"
#include "guicast.h"
#include "mutex.h"
#include "thread.h"


class BlurVertical;
class BlurHorizontal;
class BlurRadius;
class BlurRadiusText;
class BlurA;
class BlurR;
class BlurG;
class BlurB;
class BlurAKey;

class BlurWindow : public PluginClientWindow
{
public:
	BlurWindow(BlurMain *client);
	~BlurWindow();

	void create_objects();

	BlurMain *client;
	BlurVertical *vertical;
	BlurHorizontal *horizontal;
	BlurRadius *radius;
	BlurRadiusText *radius_text;
	BlurA *a;
	BlurR *r;
	BlurG *g;
	BlurB *b;
	BlurAKey *a_key;
};

class BlurAKey : public BC_CheckBox
{
public:
	BlurAKey(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};

class BlurA : public BC_CheckBox
{
public:
	BlurA(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};
class BlurR : public BC_CheckBox
{
public:
	BlurR(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};
class BlurG : public BC_CheckBox
{
public:
	BlurG(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};
class BlurB : public BC_CheckBox
{
public:
	BlurB(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};


class BlurRadius : public BC_IPot
{
public:
	BlurRadius(BlurMain *client, BlurWindow *gui, int x, int y);
	~BlurRadius();
	int handle_event();
	BlurMain *client;
	BlurWindow *gui;
};

class BlurRadiusText : public BC_TextBox
{
public:
	BlurRadiusText(BlurMain *client, BlurWindow *gui, int x, int y, int w);
	int handle_event();
	BlurMain *client;
	BlurWindow *gui;
};

class BlurVertical : public BC_CheckBox
{
public:
	BlurVertical(BlurMain *client, BlurWindow *window, int x, int y);
	~BlurVertical();
	int handle_event();

	BlurMain *client;
	BlurWindow *window;
};

class BlurHorizontal : public BC_CheckBox
{
public:
	BlurHorizontal(BlurMain *client, BlurWindow *window, int x, int y);
	~BlurHorizontal();
	int handle_event();

	BlurMain *client;
	BlurWindow *window;
};


#endif
