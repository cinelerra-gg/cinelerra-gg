
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

#include "appearanceprefs.h"
#include "deleteallindexes.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "language.h"
#include "mwindow.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "shbtnprefs.h"
#include "theme.h"


AppearancePrefs::AppearancePrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
	hms = 0;
	hmsf = 0;
	samples = 0;
	frames = 0;
	hex = 0;
	feet = 0;
	thumbnails = 0;
	thumbnail_size = 0;
}

AppearancePrefs::~AppearancePrefs()
{
	delete hms;
	delete hmsf;
	delete samples;
	delete frames;
	delete hex;
	delete feet;
	delete thumbnails;
	delete thumbnail_size;
}


void AppearancePrefs::create_objects()
{
	BC_Resources *resources = BC_WindowBase::get_resources();
	int margin = mwindow->theme->widget_border;
	char string[BCTEXTLEN];
	int x0 = mwindow->theme->preferencesoptions_x;
	int y0 = mwindow->theme->preferencesoptions_y;
	int x = x0, y = y0, x1 = x + 100;

	add_subwindow(new BC_Title(x, y, _("Layout:"), LARGEFONT,
		resources->text_default));
	y += 35;
	int y1 = y;

	ViewTheme *theme;
	add_subwindow(new BC_Title(x, y, _("Theme:")));
	add_subwindow(theme = new ViewTheme(x1, y, pwindow));
	theme->create_objects();
	y += theme->get_h() + 5;

	x = x0;
	ViewPluginIcons *plugin_icons;
	add_subwindow(new BC_Title(x, y, _("Plugin Icons:")));
	add_subwindow(plugin_icons = new ViewPluginIcons(x1, y, pwindow));
	plugin_icons->create_objects();
	y += plugin_icons->get_h() + 15;
	x1 = get_w()/2;

	int x2 = x1 + 160, y2 = y;
	y = y1;
	add_subwindow(new BC_Title(x1, y, _("View thumbnail size:")));
	thumbnail_size = new ViewThumbnailSize(pwindow, this, x2, y);
	thumbnail_size->create_objects();
	y += thumbnail_size->get_h() + 5;
	add_subwindow(new BC_Title(x1, y, _("Vicon memory size:")));
	vicon_size = new ViewViconSize(pwindow, this, x2, y);
	vicon_size->create_objects();
	y += vicon_size->get_h() + 5;
	add_subwindow(new BC_Title(x1, y, _("Vicon color mode:")));
	add_subwindow(vicon_color_mode = new ViewViconColorMode(pwindow, x2, y));
	vicon_color_mode->create_objects();
	y += vicon_color_mode->get_h() + 5;
	y = bmax(y, y2);	

	y += 10;
	add_subwindow(new BC_Bar(5, y, 	get_w() - 10));
	y += 15;

	add_subwindow(new BC_Title(x, y, _("Time Format:"), LARGEFONT,
		resources->text_default));

	add_subwindow(new BC_Title(x1, y, _("Flags:"), LARGEFONT,
		resources->text_default));

	y += get_text_height(LARGEFONT) + 5;
	y += 10;
	y1 = y;

	add_subwindow(hms = new TimeFormatHMS(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_HMS,
		x, y));
	y += 20;
	add_subwindow(hmsf = new TimeFormatHMSF(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_HMSF,
		x, y));
	y += 20;
	add_subwindow(samples = new TimeFormatSamples(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_SAMPLES,
		x, y));
	y += 20;
	add_subwindow(hex = new TimeFormatHex(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_SAMPLES_HEX,
		x, y));
	y += 20;
	add_subwindow(frames = new TimeFormatFrames(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_FRAMES,
		x, y));
	y += 20;
	add_subwindow(feet = new TimeFormatFeet(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_FEET_FRAMES,
		x, y));
	x += feet->get_w() + 15;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Frames per foot:")));
	x += title->get_w() + margin;
	sprintf(string, "%0.2f", pwindow->thread->edl->session->frames_per_foot);
	add_subwindow(new TimeFormatFeetSetting(pwindow,
		x, y - 5, 	string));
	x = x0;
	y += 20;
	add_subwindow(seconds = new TimeFormatSeconds(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_SECONDS,
		x, y));
	x = x0;
	y += 35;
	add_subwindow(new BC_Bar(5, y, 	get_w()/2 - 30));
	y += 15;

	add_subwindow(new BC_Title(x, y, _("Color:"), LARGEFONT,
		resources->text_default));
	y += 35;
	add_subwindow(title = new BC_Title(x, y, _("Highlighting Inversion color:")));
	x += title->get_w() + margin;
	char hex_color[BCSTRLEN];
	sprintf(hex_color, "%06x", preferences->highlight_inverse);
        add_subwindow(new HighlightInverseColor(pwindow, x, y, hex_color));
	y += 35;

	x = x0;
	add_subwindow(title = new BC_Title(x, y, _("YUV color space:")));
	x += title->get_w() + margin;
	add_subwindow(yuv_color_space = new YuvColorSpace(x, y, pwindow));
	yuv_color_space->create_objects();
	y += yuv_color_space->get_h() + 5;

	x = x0;
	add_subwindow(title = new BC_Title(x, y, _("YUV color range:")));
	x += title->get_w() + margin;
	add_subwindow(yuv_color_range = new YuvColorRange(x, y, pwindow));
	yuv_color_range->create_objects();
	y += yuv_color_range->get_h() + 5;

	UseTipWindow *tip_win = new UseTipWindow(pwindow, x1, y1);
	add_subwindow(tip_win);
	y1 += tip_win->get_h() + 5;
	AutocolorAssets *autocolor_assets = new AutocolorAssets(pwindow, x1, y1);
	add_subwindow(autocolor_assets);
	y1 += autocolor_assets->get_h() + 5;
	UseWarnIndecies *idx_win = new UseWarnIndecies(pwindow, x1, y1);
	add_subwindow(idx_win);
	y1 += idx_win->get_h() + 5;
	UseWarnVersion *ver_win = new UseWarnVersion(pwindow, x1, y1);
	add_subwindow(ver_win);
	y1 += ver_win->get_h() + 5;
	BD_WarnRoot *bdwr_win = new BD_WarnRoot(pwindow, x1, y1);
	add_subwindow(bdwr_win);
	y1 += bdwr_win->get_h() + 5;
	PopupMenuBtnup *pop_win = new PopupMenuBtnup(pwindow, x1, y1);
	add_subwindow(pop_win);
	y1 += pop_win->get_h() + 5;
	GrabFocusPolicy *grab_input_focus = new GrabFocusPolicy(pwindow, x1, y1);
	add_subwindow(grab_input_focus);
	y1 += grab_input_focus->get_h() + 5;
	ActivateFocusPolicy *focus_activate = new ActivateFocusPolicy(pwindow, x1, y1);
	add_subwindow(focus_activate);
	y1 += focus_activate->get_h() + 5;
	DeactivateFocusPolicy *focus_deactivate = new DeactivateFocusPolicy(pwindow, x1, y1);
	add_subwindow(focus_deactivate);
	y1 += focus_deactivate->get_h() + 5;
	ForwardRenderDisplacement *displacement = new ForwardRenderDisplacement(pwindow, x1, y1);
	add_subwindow(displacement);
	y1 += displacement->get_h() + 5;
	add_subwindow(thumbnails = new ViewThumbnails(x1, y1, pwindow));
	y1 += thumbnails->get_h() + 5;
	PerpetualSession *perpetual = new PerpetualSession(x1, y1, pwindow);
	add_subwindow(perpetual);
	y1 += perpetual->get_h() + 5;
	if( y < y1 ) y = y1;
}

int AppearancePrefs::update(int new_value)
{
	pwindow->thread->redraw_times = 1;
	pwindow->thread->edl->session->time_format = new_value;
	hms->update(new_value == TIME_HMS);
	hmsf->update(new_value == TIME_HMSF);
	samples->update(new_value == TIME_SAMPLES);
	hex->update(new_value == TIME_SAMPLES_HEX);
	frames->update(new_value == TIME_FRAMES);
	feet->update(new_value == TIME_FEET_FRAMES);
	seconds->update(new_value == TIME_SECONDS);
	return 0;
}


TimeFormatHMS::TimeFormatHMS(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_HMS_TEXT)
{ this->pwindow = pwindow; this->tfwindow = tfwindow; }

int TimeFormatHMS::handle_event()
{
	tfwindow->update(TIME_HMS);
	return 1;
}

TimeFormatHMSF::TimeFormatHMSF(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_HMSF_TEXT)
{ this->pwindow = pwindow; this->tfwindow = tfwindow; }

int TimeFormatHMSF::handle_event()
{
	tfwindow->update(TIME_HMSF);
	return 1;
}

TimeFormatSamples::TimeFormatSamples(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_SAMPLES_TEXT)
{ this->pwindow = pwindow; this->tfwindow = tfwindow; }

int TimeFormatSamples::handle_event()
{
	tfwindow->update(TIME_SAMPLES);
	return 1;
}

TimeFormatFrames::TimeFormatFrames(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_FRAMES_TEXT)
{ this->pwindow = pwindow; this->tfwindow = tfwindow; }

int TimeFormatFrames::handle_event()
{
	tfwindow->update(TIME_FRAMES);
	return 1;
}

TimeFormatHex::TimeFormatHex(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_SAMPLES_HEX_TEXT)
{ this->pwindow = pwindow; this->tfwindow = tfwindow; }

int TimeFormatHex::handle_event()
{
	tfwindow->update(TIME_SAMPLES_HEX);
	return 1;
}

TimeFormatSeconds::TimeFormatSeconds(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_SECONDS_TEXT)
{
	this->pwindow = pwindow;
	this->tfwindow = tfwindow;
}

int TimeFormatSeconds::handle_event()
{
	tfwindow->update(TIME_SECONDS);
	return 1;
}

TimeFormatFeet::TimeFormatFeet(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_FEET_FRAMES_TEXT)
{ this->pwindow = pwindow; this->tfwindow = tfwindow; }

int TimeFormatFeet::handle_event()
{
	tfwindow->update(TIME_FEET_FRAMES);
	return 1;
}

TimeFormatFeetSetting::TimeFormatFeetSetting(PreferencesWindow *pwindow, int x, int y, char *string)
 : BC_TextBox(x, y, 90, 1, string)
{ this->pwindow = pwindow; }

int TimeFormatFeetSetting::handle_event()
{
	pwindow->thread->edl->session->frames_per_foot = atof(get_text());
	if(pwindow->thread->edl->session->frames_per_foot < 1) pwindow->thread->edl->session->frames_per_foot = 1;
	return 0;
}


ViewTheme::ViewTheme(int x, int y, PreferencesWindow *pwindow)
 : BC_PopupMenu(x, y, 200, pwindow->thread->preferences->theme, 1)
{
	this->pwindow = pwindow;
}
ViewTheme::~ViewTheme()
{
}

void ViewTheme::create_objects()
{
	ArrayList<PluginServer*> themes;
	MWindow::search_plugindb(0, 0, 0, 0, 1, themes);

	for(int i = 0; i < themes.total; i++) {
		add_item(new ViewThemeItem(this, themes.values[i]->title));
	}
}

int ViewTheme::handle_event()
{
	return 1;
}

ViewThemeItem::ViewThemeItem(ViewTheme *popup, const char *text)
 : BC_MenuItem(text)
{
	this->popup = popup;
}

int ViewThemeItem::handle_event()
{
	popup->set_text(get_text());
	strcpy(popup->pwindow->thread->preferences->theme, get_text());
	popup->handle_event();
	return 1;
}


ViewPluginIcons::ViewPluginIcons(int x, int y, PreferencesWindow *pwindow)
 : BC_PopupMenu(x, y, 200, pwindow->thread->preferences->plugin_icons, 1)
{
	this->pwindow = pwindow;
}
ViewPluginIcons::~ViewPluginIcons()
{
}

void ViewPluginIcons::create_objects()
{
	add_item(new ViewPluginIconItem(this, DEFAULT_PICON));
	FileSystem fs;
	const char *plugin_path = File::get_plugin_path();
	char picon_path[BCTEXTLEN];
	snprintf(picon_path,sizeof(picon_path)-1,"%s/picon", plugin_path);
	if( fs.update(picon_path) ) return;
	for( int i=0; i<fs.dir_list.total; ++i ) {
		char *fs_path = fs.dir_list[i]->path;
		if( !fs.is_dir(fs_path) ) continue;
		char *cp = strrchr(fs_path,'/');
		cp = !cp ? fs_path : cp+1;
		if( !strcmp(cp,DEFAULT_PICON) ) continue;
		add_item(new ViewPluginIconItem(this, cp));
	}
}

int ViewPluginIcons::handle_event()
{
	return 1;
}

ViewPluginIconItem::ViewPluginIconItem(ViewPluginIcons *popup, const char *text)
 : BC_MenuItem(text)
{
	this->popup = popup;
}

int ViewPluginIconItem::handle_event()
{
	popup->set_text(get_text());
	strcpy(popup->pwindow->thread->preferences->plugin_icons, get_text());
	popup->handle_event();
	return 1;
}


ViewThumbnails::ViewThumbnails(int x,
	int y,
	PreferencesWindow *pwindow)
 : BC_CheckBox(x,
 	y,
	pwindow->thread->preferences->use_thumbnails, _("Use thumbnails in resource window"))
{
	this->pwindow = pwindow;
}

int ViewThumbnails::handle_event()
{
	pwindow->thread->preferences->use_thumbnails = get_value();
	return 1;
}


ViewThumbnailSize::ViewThumbnailSize(PreferencesWindow *pwindow,
		AppearancePrefs *aprefs, int x, int y)
 : BC_TumbleTextBox(aprefs,
	pwindow->thread->preferences->awindow_picon_h,
	16, 512, x, y, 80)

{
	this->pwindow = pwindow;
	this->aprefs = aprefs;
}

int ViewThumbnailSize::handle_event()
{
	int v = atoi(get_text());
	bclamp(v, 16,512);
	pwindow->thread->preferences->awindow_picon_h = v;
	return 1;
}

ViewViconSize::ViewViconSize(PreferencesWindow *pwindow,
		AppearancePrefs *aprefs, int x, int y)
 : BC_TumbleTextBox(aprefs,
	pwindow->thread->preferences->vicon_size,
	16, 512, x, y, 80)

{
	this->pwindow = pwindow;
	this->aprefs = aprefs;
}

int ViewViconSize::handle_event()
{
	int v = atoi(get_text());
	bclamp(v, 16,512);
	pwindow->thread->preferences->vicon_size = v;
	return 1;
}

ViewViconColorMode::ViewViconColorMode(PreferencesWindow *pwindow, int x, int y)
 : BC_PopupMenu(x, y, 100,
	_(vicon_color_modes[pwindow->thread->preferences->vicon_color_mode]), 1)
{
	this->pwindow = pwindow;
}
ViewViconColorMode::~ViewViconColorMode()
{
}

const char *ViewViconColorMode::vicon_color_modes[] = {
	N_("Low"),
	N_("Med"),
	N_("High"),
};

void ViewViconColorMode::create_objects()
{
	for( int id=0,nid=sizeof(vicon_color_modes)/sizeof(vicon_color_modes[0]); id<nid; ++id )
		add_item(new ViewViconColorModeItem(this, _(vicon_color_modes[id]), id));
	handle_event();
}

int ViewViconColorMode::handle_event()
{
	set_text(_(vicon_color_modes[pwindow->thread->preferences->vicon_color_mode]));
	return 1;
}

ViewViconColorModeItem::ViewViconColorModeItem(ViewViconColorMode *popup, const char *text, int id)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->id = id;
}

int ViewViconColorModeItem::handle_event()
{
	popup->set_text(get_text());
	popup->pwindow->thread->preferences->vicon_color_mode = id;
	return popup->handle_event();
}


UseTipWindow::UseTipWindow(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x,
 	y,
	pwindow->thread->preferences->use_tipwindow,
	_("Show tip of the day"))
{
	this->pwindow = pwindow;
}
int UseTipWindow::handle_event()
{
	pwindow->thread->preferences->use_tipwindow = get_value();
	return 1;
}


UseWarnIndecies::UseWarnIndecies(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->warn_indexes,
	_("ffmpeg probe warns rebuild indexes"))
{
	this->pwindow = pwindow;
}

int UseWarnIndecies::handle_event()
{
	pwindow->thread->preferences->warn_indexes = get_value();
	return 1;
}

UseWarnVersion::UseWarnVersion(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->warn_version,
	_("EDL version warns if mismatched"))
{
	this->pwindow = pwindow;
}

int UseWarnVersion::handle_event()
{
	pwindow->thread->preferences->warn_version = get_value();
	return 1;
}

BD_WarnRoot::BD_WarnRoot(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->bd_warn_root,
	_("Create Bluray warns if not root"))
{
	this->pwindow = pwindow;
}

int BD_WarnRoot::handle_event()
{
	pwindow->thread->preferences->bd_warn_root = get_value();
	return 1;
}

PopupMenuBtnup::PopupMenuBtnup(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->popupmenu_btnup,
	_("Popups activate on button up"))
{
	this->pwindow = pwindow;
}

int PopupMenuBtnup::handle_event()
{
	pwindow->thread->preferences->popupmenu_btnup = get_value();
	return 1;
}

GrabFocusPolicy::GrabFocusPolicy(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, (pwindow->thread->preferences->grab_input_focus) != 0,
	_("Set Input Focus when window entered"))
{
	this->pwindow = pwindow;
}

int GrabFocusPolicy::handle_event()
{
	pwindow->thread->preferences->grab_input_focus = get_value();
	return 1;
}

ActivateFocusPolicy::ActivateFocusPolicy(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, (pwindow->thread->preferences->textbox_focus_policy & CLICK_ACTIVATE) != 0,
	_("Click to activate text focus"))
{
	this->pwindow = pwindow;
}

int ActivateFocusPolicy::handle_event()
{
	if( get_value() )
		pwindow->thread->preferences->textbox_focus_policy |= CLICK_ACTIVATE;
	else
		pwindow->thread->preferences->textbox_focus_policy &= ~CLICK_ACTIVATE;
	return 1;
}

DeactivateFocusPolicy::DeactivateFocusPolicy(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, (pwindow->thread->preferences->textbox_focus_policy & CLICK_DEACTIVATE) != 0,
	_("Click to deactivate text focus"))
{
	this->pwindow = pwindow;
}

int DeactivateFocusPolicy::handle_event()
{
	if( get_value() )
		pwindow->thread->preferences->textbox_focus_policy |= CLICK_DEACTIVATE;
	else
		pwindow->thread->preferences->textbox_focus_policy &= ~CLICK_DEACTIVATE;
	return 1;
}

ForwardRenderDisplacement::ForwardRenderDisplacement(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->forward_render_displacement,
	_("Always show next frame"))
{
	this->pwindow = pwindow;
}

int ForwardRenderDisplacement::handle_event()
{
	pwindow->thread->preferences->forward_render_displacement = get_value();
	return 1;
}

AutocolorAssets::AutocolorAssets(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->autocolor_assets,
	_("Autocolor assets"))
{
	this->pwindow = pwindow;
}

int AutocolorAssets::handle_event()
{
	pwindow->thread->preferences->autocolor_assets = get_value();
	return 1;
}

HighlightInverseColor::HighlightInverseColor(PreferencesWindow *pwindow, int x, int y, const char *hex)
 : BC_TextBox(x, y, 80, 1, hex)
{
	this->pwindow = pwindow;
}

int HighlightInverseColor::handle_event()
{
	int inverse_color = strtoul(get_text(),0,16);
	if( (inverse_color &= 0xffffff) == 0 ) {
		inverse_color = 0xffffff;
		char string[BCSTRLEN];
		sprintf(string,"%06x", inverse_color);
		update(string);
	}
	pwindow->thread->preferences->highlight_inverse = inverse_color;
	return 1;
}


const char *YuvColorSpace::color_space[] = {
	N_("BT601"),
	N_("BT709"),
	N_("BT2020"),
};

YuvColorSpace::YuvColorSpace(int x, int y, PreferencesWindow *pwindow)
 : BC_PopupMenu(x, y, 100,
	_(color_space[pwindow->thread->preferences->yuv_color_space]), 1)
{
	this->pwindow = pwindow;
}
YuvColorSpace::~YuvColorSpace()
{
}

void YuvColorSpace::create_objects()
{
	for( int id=0,nid=sizeof(color_space)/sizeof(color_space[0]); id<nid; ++id )
		add_item(new YuvColorSpaceItem(this, _(color_space[id]), id));
	handle_event();
}

int YuvColorSpace::handle_event()
{
	set_text(_(color_space[pwindow->thread->preferences->yuv_color_space]));
	return 1;
}

YuvColorSpaceItem::YuvColorSpaceItem(YuvColorSpace *popup, const char *text, int id)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->id = id;
}

int YuvColorSpaceItem::handle_event()
{
	popup->set_text(get_text());
	popup->pwindow->thread->preferences->yuv_color_space = id;
	return popup->handle_event();
}


const char *YuvColorRange::color_range[] = {
	N_("JPEG"),
	N_("MPEG"),
};

YuvColorRange::YuvColorRange(int x, int y, PreferencesWindow *pwindow)
 : BC_PopupMenu(x, y, 100,
	_(color_range[pwindow->thread->preferences->yuv_color_range]), 1)
{
	this->pwindow = pwindow;
}
YuvColorRange::~YuvColorRange()
{
}

void YuvColorRange::create_objects()
{
	for( int id=0,nid=sizeof(color_range)/sizeof(color_range[0]); id<nid; ++id )
		add_item(new YuvColorRangeItem(this, _(color_range[id]), id));
	handle_event();
}

int YuvColorRange::handle_event()
{
	set_text(color_range[pwindow->thread->preferences->yuv_color_range]);
	return 1;
}

YuvColorRangeItem::YuvColorRangeItem(YuvColorRange *popup, const char *text, int id)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->id = id;
}

int YuvColorRangeItem::handle_event()
{
	popup->set_text(get_text());
	popup->pwindow->thread->preferences->yuv_color_range = id;
	return popup->handle_event();
}

PerpetualSession::PerpetualSession(int x, int y, PreferencesWindow *pwindow)
 : BC_CheckBox(x, y,
	pwindow->thread->preferences->perpetual_session, _("Perpetual session"))
{
	this->pwindow = pwindow;
}

int PerpetualSession::handle_event()
{
	pwindow->thread->preferences->perpetual_session = get_value();
	return 1;
}

