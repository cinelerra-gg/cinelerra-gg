
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

#ifndef APATCHGUI_H
#define APATCHGUI_H

#include "apatchgui.inc"
#include "atrack.inc"
#include "floatauto.inc"
#include "guicast.h"
#include "panauto.inc"
#include "patchgui.h"


class APatchGUI : public PatchGUI
{
public:
	APatchGUI(MWindow *mwindow, PatchBay *patchbay, ATrack *track, int x, int y);
	~APatchGUI();

	void create_objects();
	int reposition(int x, int y);
	int update(int x, int y);
	void update_faders(float v);

	ATrack *atrack;
	AFadePatch *fade;
	APanPatch *pan;
	AMeterPatch *meter;
};

class AFadePatch : public BC_FSlider
{
public:
	AFadePatch(APatchGUI *patch, int x, int y, int w, float v);
	static FloatAuto* get_keyframe(MWindow *mwindow, APatchGUI *patch);
	virtual int handle_event();
	APatchGUI *patch;
};

class AKeyFadePatch : public BC_SubWindow
{
public:
	AKeyFadePatch(MWindow *mwindow, APatchGUI *patch, int x, int y);
	void create_objects();
	void update(float v);

	MWindow *mwindow;
	APatchGUI *patch;
	AKeyFadeOK *akey_fade_ok;
	AKeyFadeText *akey_fade_text;
	AKeyFadeSlider *akey_fade_slider;
};

class AKeyFadeOK : public BC_Button
{
public:
	AKeyFadeOK(AKeyFadePatch *akey_fade_patch, int x, int y, VFrame **images);
	int handle_event();

	AKeyFadePatch *akey_fade_patch;
};

class AKeyFadeText : public BC_TextBox
{
public:
	AKeyFadeText(AKeyFadePatch *akey_fade_patch, int x, int y, int w, float v);
	int handle_event();

	AKeyFadePatch *akey_fade_patch;
};

class AKeyFadeSlider : public AFadePatch
{
public:
	AKeyFadeSlider(AKeyFadePatch *akey_fade_patch, int x, int y, int w, float v);
	int handle_event();

	AKeyFadePatch *akey_fade_patch;
};


class APanPatch : public BC_Pan
{
public:
	APanPatch(MWindow *mwindow, APatchGUI *patch, int x, int y);
	static PanAuto* get_keyframe(MWindow *mwindow, APatchGUI *patch);
	virtual int handle_event();
	MWindow *mwindow;
	APatchGUI *patch;
};

class AKeyPanPatch : public APanPatch
{
public:
	AKeyPanPatch(MWindow *mwindow, APatchGUI *patch);
	int handle_event();
};

class AMeterPatch : public BC_Meter
{
public:
	AMeterPatch(MWindow *mwindow, APatchGUI *patch, int x, int y);
	int button_press_event();
	MWindow *mwindow;
	APatchGUI *patch;
};

class AMixPatch : public MixPatch
{
public:
	AMixPatch(MWindow *mwindow, APatchGUI *patch, int x, int y);
	~AMixPatch();
};

#endif
