
/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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

#include "assets.h"
#include "auto.h"
#include "batchrender.h"
#include "bcdisplayinfo.h"
#include "bchash.h"
#include "bcsignals.h"
#include "bdcreate.h"
#include "cache.h"
#include "channelinfo.h"
#include "cplayback.h"
#include "cropvideo.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "dbwindow.h"
#include "dvdcreate.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "floatauto.h"
#include "keys.h"
#include "language.h"
#include "levelwindow.h"
#include "loadfile.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "menuattacheffect.h"
#include "menuattachtransition.h"
#include "menuaeffects.h"
#include "menueditlength.h"
#include "menutransitionlength.h"
#include "menuveffects.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "new.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "preferences.h"
#include "proxy.h"
#include "preferencesthread.h"
#include "quit.h"
#include "record.h"
#include "render.h"
#include "savefile.h"
#include "setformat.h"
#include "swindow.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transition.h"
#include "transportque.h"
#include "viewmenu.h"
#include "zoombar.h"
#include "exportedl.h"

#include <string.h>


MainMenu::MainMenu(MWindow *mwindow, MWindowGUI *gui, int w)
 : BC_MenuBar(0, 0, w)
{
	this->gui = gui;
	this->mwindow = mwindow;
}

MainMenu::~MainMenu()
{
}

void MainMenu::create_objects()
{
	BC_Menu *viewmenu, *windowmenu, *settingsmenu, *trackmenu;
	PreferencesMenuitem *preferences;
	total_loads = 0;

	add_menu(filemenu = new BC_Menu(_("File")));
	filemenu->add_item(new_project = new NewProject(mwindow));
	new_project->create_objects();

// file loaders
	filemenu->add_item(load_file = new Load(mwindow, this));
	load_file->create_objects();

// new and load can be undone so no need to prompt save
	Save *save;                   //  affected by saveas
	filemenu->add_item(save = new Save(mwindow));
	SaveAs *saveas;
	filemenu->add_item(saveas = new SaveAs(mwindow));
	save->create_objects(saveas);
	saveas->set_mainmenu(this);
	SaveProject *save_project;
	filemenu->add_item(save_project = new SaveProject(mwindow));

	filemenu->add_item(record_menu_item = new RecordMenuItem(mwindow));
#ifdef HAVE_DVB
	filemenu->add_item(new ChannelScan(mwindow));
#endif
#ifdef HAVE_COMMERCIAL
	if( mwindow->has_commercials() )
		filemenu->add_item(new DbWindowScan(mwindow));
#endif
	filemenu->add_item(new SubttlSWin(mwindow));

	filemenu->add_item(render = new RenderItem(mwindow));
	filemenu->add_item(new ExportEDLItem(mwindow));
	filemenu->add_item(new BatchRenderMenuItem(mwindow));
	filemenu->add_item(new CreateBD_MenuItem(mwindow));
	filemenu->add_item(new CreateDVD_MenuItem(mwindow));
	filemenu->add_item(new BC_MenuItem("-"));
	filemenu->add_item(quit_program = new Quit(mwindow));
	quit_program->create_objects(save);
	filemenu->add_item(new DumpEDL(mwindow));
	filemenu->add_item(new DumpPlugins(mwindow));
	filemenu->add_item(new LoadBackup(mwindow));
	filemenu->add_item(new SaveBackup(mwindow));

	BC_Menu *editmenu;
	add_menu(editmenu = new BC_Menu(_("Edit")));
	editmenu->add_item(undo = new Undo(mwindow));
	editmenu->add_item(redo = new Redo(mwindow));
	editmenu->add_item(new BC_MenuItem("-"));
	editmenu->add_item(new Cut(mwindow));
	editmenu->add_item(new Copy(mwindow));
	editmenu->add_item(new Paste(mwindow));
	editmenu->add_item(new Clear(mwindow));
	editmenu->add_item(new PasteSilence(mwindow));
	editmenu->add_item(new MuteSelection(mwindow));
	editmenu->add_item(new TrimSelection(mwindow));
	editmenu->add_item(new SelectAll(mwindow));
	editmenu->add_item(new BC_MenuItem("-"));
	editmenu->add_item(new MenuEditShuffle(mwindow));
	editmenu->add_item(new MenuEditReverse(mwindow));
	editmenu->add_item(new MenuEditLength(mwindow));
	editmenu->add_item(new MenuEditAlign(mwindow));
	editmenu->add_item(new MenuTransitionLength(mwindow));
	editmenu->add_item(new DetachTransitions(mwindow));
	editmenu->add_item(new BC_MenuItem("-"));
	editmenu->add_item(new ClearLabels(mwindow));
	editmenu->add_item(new CutCommercials(mwindow));
	editmenu->add_item(new PasteSubttl(mwindow));

	BC_Menu *keyframemenu;
	add_menu(keyframemenu = new BC_Menu(_("Keyframes")));
	keyframemenu->add_item(new CutKeyframes(mwindow));
	keyframemenu->add_item(new CopyKeyframes(mwindow));
	keyframemenu->add_item(new PasteKeyframes(mwindow));
	keyframemenu->add_item(new ClearKeyframes(mwindow));
	keyframemenu->add_item(new StraightenKeyframes(mwindow));
	keyframemenu->add_item(new BendKeyframes(mwindow));
	keyframemenu->add_item(keyframe_curve_type = new KeyframeCurveType(mwindow));
	keyframe_curve_type->create_objects();
	keyframe_curve_type->update(mwindow->edl->local_session->floatauto_type);
	keyframemenu->add_item(new BC_MenuItem("-"));
	keyframemenu->add_item(new CopyDefaultKeyframe(mwindow));
	keyframemenu->add_item(new PasteDefaultKeyframe(mwindow));




	add_menu(audiomenu = new BC_Menu(_("Audio")));
	audiomenu->add_item(new AddAudioTrack(mwindow));
	audiomenu->add_item(new DefaultATransition(mwindow));
	audiomenu->add_item(new MapAudio1(mwindow));
	audiomenu->add_item(new MapAudio2(mwindow));
	audiomenu->add_item(new MenuAttachTransition(mwindow, TRACK_AUDIO));
	audiomenu->add_item(new MenuAttachEffect(mwindow, TRACK_AUDIO));
	audiomenu->add_item(aeffects = new MenuAEffects(mwindow));

	add_menu(videomenu = new BC_Menu(_("Video")));
	videomenu->add_item(new AddVideoTrack(mwindow));
	videomenu->add_item(new DefaultVTransition(mwindow));
	videomenu->add_item(new MenuAttachTransition(mwindow, TRACK_VIDEO));
	videomenu->add_item(new MenuAttachEffect(mwindow, TRACK_VIDEO));
	videomenu->add_item(veffects = new MenuVEffects(mwindow));

	add_menu(trackmenu = new BC_Menu(_("Tracks")));
	trackmenu->add_item(new MoveTracksUp(mwindow));
	trackmenu->add_item(new MoveTracksDown(mwindow));
	trackmenu->add_item(new DeleteTracks(mwindow));
	trackmenu->add_item(new DeleteTrack(mwindow));
	trackmenu->add_item(new ConcatenateTracks(mwindow));
	AppendTracks *append_tracks;
	trackmenu->add_item(append_tracks = new AppendTracks(mwindow));
	append_tracks->create_objects();
	trackmenu->add_item(new AddSubttlTrack(mwindow));

	add_menu(settingsmenu = new BC_Menu(_("Settings")));

	settingsmenu->add_item(new SetFormat(mwindow));
	settingsmenu->add_item(preferences = new PreferencesMenuitem(mwindow));
	ProxyMenuItem *proxy;
	settingsmenu->add_item(proxy = new ProxyMenuItem(mwindow));
	proxy->create_objects();
	mwindow->preferences_thread = preferences->thread;
	settingsmenu->add_item(labels_follow_edits = new LabelsFollowEdits(mwindow));
	settingsmenu->add_item(plugins_follow_edits = new PluginsFollowEdits(mwindow));
	settingsmenu->add_item(keyframes_follow_edits = new KeyframesFollowEdits(mwindow));
	settingsmenu->add_item(cursor_on_frames = new CursorOnFrames(mwindow));
	settingsmenu->add_item(typeless_keyframes = new TypelessKeyframes(mwindow));
	settingsmenu->add_item(new BC_MenuItem("-"));
	settingsmenu->add_item(new SaveSettingsNow(mwindow));
	settingsmenu->add_item(loop_playback = new LoopPlayback(mwindow));
	settingsmenu->add_item(brender_active = new SetBRenderActive(mwindow));
// set scrubbing speed
//	ScrubSpeed *scrub_speed;
//	settingsmenu->add_item(scrub_speed = new ScrubSpeed(mwindow));
//	if(mwindow->edl->session->scrub_speed == .5)
//		scrub_speed->set_text(_("Fast Shuttle"));






	add_menu(viewmenu = new BC_Menu(_("View")));
	viewmenu->add_item(show_assets = new ShowAssets(mwindow, "0"));
	viewmenu->add_item(show_titles = new ShowTitles(mwindow, "1"));
	viewmenu->add_item(show_transitions = new ShowTransitions(mwindow, "2"));
	viewmenu->add_item(fade_automation = new ShowAutomation(mwindow, _("Fade"), "3", AUTOMATION_FADE));
	viewmenu->add_item(mute_automation = new ShowAutomation(mwindow, _("Mute"), "4", AUTOMATION_MUTE));
	viewmenu->add_item(mode_automation = new ShowAutomation(mwindow, _("Overlay mode"), "5", AUTOMATION_MODE));
	viewmenu->add_item(pan_automation = new ShowAutomation(mwindow, _("Pan"), "6", AUTOMATION_PAN));
	viewmenu->add_item(plugin_automation = new PluginAutomation(mwindow, "7"));
	viewmenu->add_item(mask_automation = new ShowAutomation(mwindow, _("Mask"), "8", AUTOMATION_MASK));
	viewmenu->add_item(speed_automation = new ShowAutomation(mwindow, _("Speed"), "9", AUTOMATION_SPEED));

	camera_x = new ShowAutomation(mwindow, _("Camera X"), "Ctl-Shift-X", AUTOMATION_CAMERA_X);
	camera_x->set_ctrl();  camera_x->set_shift();   viewmenu->add_item(camera_x);
	camera_y = new ShowAutomation(mwindow, _("Camera Y"), "Ctl-Shift-Y", AUTOMATION_CAMERA_Y);
	camera_y->set_ctrl();  camera_y->set_shift();   viewmenu->add_item(camera_y);
	camera_z = new ShowAutomation(mwindow, _("Camera Z"), "Ctl-Shift-Z", AUTOMATION_CAMERA_Z);
	camera_z->set_ctrl();  camera_z->set_shift();  viewmenu->add_item(camera_z);
	project_x = new ShowAutomation(mwindow, _("Projector X"), "Alt-Shift-X", AUTOMATION_PROJECTOR_X);
	project_x->set_alt();  project_x->set_shift();  viewmenu->add_item(project_x);
	project_y = new ShowAutomation(mwindow, _("Projector Y"), "Alt-Shift-Y", AUTOMATION_PROJECTOR_Y);
	project_y->set_alt();  project_y->set_shift();  viewmenu->add_item(project_y);
	project_z = new ShowAutomation(mwindow, _("Projector Z"), "Alt-Shift-Z", AUTOMATION_PROJECTOR_Z);
	project_z->set_alt();  project_z->set_shift();  viewmenu->add_item(project_z);

	add_menu(windowmenu = new BC_Menu(_("Window")));
	windowmenu->add_item(show_vwindow = new ShowVWindow(mwindow));
	windowmenu->add_item(show_awindow = new ShowAWindow(mwindow));
	windowmenu->add_item(show_cwindow = new ShowCWindow(mwindow));
	windowmenu->add_item(show_gwindow = new ShowGWindow(mwindow));
	windowmenu->add_item(show_lwindow = new ShowLWindow(mwindow));
	windowmenu->add_item(new BC_MenuItem("-"));
	windowmenu->add_item(split_x = new SplitX(mwindow));
	windowmenu->add_item(split_y = new SplitY(mwindow));
	windowmenu->add_item(mixer_viewer = new MixerViewer(mwindow));
	windowmenu->add_item(new TileMixers(mwindow));
	windowmenu->add_item(new TileWindows(mwindow,_("Tile left"),0));
	windowmenu->add_item(new TileWindows(mwindow,_("Tile right"),1));
	windowmenu->add_item(new BC_MenuItem("-"));

	windowmenu->add_item(new TileWindows(mwindow,_("Default positions"),-1,_("Ctrl-P"),'p'));
	windowmenu->add_item(load_layout = new LoadLayout(mwindow, _("Load layout..."),LAYOUT_LOAD));
	load_layout->create_objects();
	windowmenu->add_item(save_layout = new LoadLayout(mwindow, _("Save layout..."),LAYOUT_SAVE));
	save_layout->create_objects();
}

int MainMenu::load_defaults(BC_Hash *defaults)
{
	init_loads(defaults);
	init_aeffects(defaults);
	init_veffects(defaults);
	return 0;
}

void MainMenu::update_toggles(int use_lock)
{
	if(use_lock) lock_window("MainMenu::update_toggles");
	labels_follow_edits->set_checked(mwindow->edl->session->labels_follow_edits);
	plugins_follow_edits->set_checked(mwindow->edl->session->plugins_follow_edits);
	keyframes_follow_edits->set_checked(mwindow->edl->session->autos_follow_edits);
	typeless_keyframes->set_checked(mwindow->edl->session->typeless_keyframes);
	cursor_on_frames->set_checked(mwindow->edl->session->cursor_on_frames);
	loop_playback->set_checked(mwindow->edl->local_session->loop_playback);

	show_assets->set_checked(mwindow->edl->session->show_assets);
	show_titles->set_checked(mwindow->edl->session->show_titles);
	show_transitions->set_checked(mwindow->edl->session->auto_conf->transitions);
	fade_automation->update_toggle();
	mute_automation->update_toggle();
	pan_automation->update_toggle();
	camera_x->update_toggle();
	camera_y->update_toggle();
	camera_z->update_toggle();
	project_x->update_toggle();
	project_y->update_toggle();
	project_z->update_toggle();
	plugin_automation->set_checked(mwindow->edl->session->auto_conf->plugins);
	mode_automation->update_toggle();
	mask_automation->update_toggle();
	speed_automation->update_toggle();
	split_x->set_checked(mwindow->gui->pane[TOP_RIGHT_PANE] != 0);
	split_y->set_checked(mwindow->gui->pane[BOTTOM_LEFT_PANE] != 0);

	if(use_lock) mwindow->gui->unlock_window();
}

int MainMenu::save_defaults(BC_Hash *defaults)
{
	save_loads(defaults);
	save_aeffects(defaults);
	save_veffects(defaults);
	return 0;
}





int MainMenu::quit()
{
	quit_program->handle_event();
	return 0;
}





// ================================== load most recent

int MainMenu::init_aeffects(BC_Hash *defaults)
{
	total_aeffects = defaults->get((char*)"TOTAL_AEFFECTS", 0);

	char string[BCTEXTLEN], title[BCTEXTLEN];
	if(total_aeffects) audiomenu->add_item(new BC_MenuItem("-"));

	for(int i = 0; i < total_aeffects; i++)
	{
		sprintf(string, "AEFFECTRECENT%d", i);
		defaults->get(string, title);
		audiomenu->add_item(aeffect[i] = new MenuAEffectItem(aeffects, title));
	}
	return 0;
}

int MainMenu::init_veffects(BC_Hash *defaults)
{
	total_veffects = defaults->get((char*)"TOTAL_VEFFECTS", 0);

	char string[BCTEXTLEN], title[BCTEXTLEN];
	if(total_veffects) videomenu->add_item(new BC_MenuItem("-"));

	for(int i = 0; i < total_veffects; i++)
	{
		sprintf(string, "VEFFECTRECENT%d", i);
		defaults->get(string, title);
		videomenu->add_item(veffect[i] = new MenuVEffectItem(veffects, title));
	}
	return 0;
}

void MainMenu::init_loads(BC_Hash *defaults)
{
	total_loads = defaults->get((char*)"TOTAL_LOADS", 0);
	if( !total_loads ) return;
	filemenu->add_item(new BC_MenuItem("-"));

	char string[BCTEXTLEN], path[BCTEXTLEN], filename[BCTEXTLEN];
	FileSystem dir;
//printf("MainMenu::init_loads 2\n");

	for(int i = 0; i < total_loads; i++) {
		sprintf(string, "LOADPREVIOUS%d", i);
//printf("MainMenu::init_loads 3\n");
		defaults->get(string, path);
		filemenu->add_item(load[i] = new LoadPrevious(mwindow, load_file));
		dir.extract_name(filename, path, 0);
		load[i]->set_text(filename);
		load[i]->set_path(path);
	}
}

// ============================ save most recent

int MainMenu::save_aeffects(BC_Hash *defaults)
{
	defaults->update((char*)"TOTAL_AEFFECTS", total_aeffects);
	char string[BCTEXTLEN];
	for(int i = 0; i < total_aeffects; i++)
	{
		sprintf(string, "AEFFECTRECENT%d", i);
		defaults->update(string, aeffect[i]->get_text());
	}
	return 0;
}

int MainMenu::save_veffects(BC_Hash *defaults)
{
	defaults->update((char*)"TOTAL_VEFFECTS", total_veffects);
	char string[BCTEXTLEN];
	for(int i = 0; i < total_veffects; i++)
	{
		sprintf(string, "VEFFECTRECENT%d", i);
		defaults->update(string, veffect[i]->get_text());
	}
	return 0;
}

int MainMenu::save_loads(BC_Hash *defaults)
{
	defaults->update((char*)"TOTAL_LOADS", total_loads);
	char string[BCTEXTLEN];
	for(int i = 0; i < total_loads; i++)
	{
		sprintf(string, "LOADPREVIOUS%d", i);
		defaults->update(string, load[i]->path);
	}
	return 0;
}

// =================================== add most recent

int MainMenu::add_aeffect(char *title)
{
// add bar for first effect
	if(total_aeffects == 0)
	{
		audiomenu->add_item(new BC_MenuItem("-"));
	}

// test for existing copy of effect
	for(int i = 0; i < total_aeffects; i++)
	{
		if(!strcmp(aeffect[i]->get_text(), title))     // already exists
		{                                // swap for top effect
			for(int j = i; j > 0; j--)   // move preceeding effects down
			{
				aeffect[j]->set_text(aeffect[j - 1]->get_text());
			}
			aeffect[0]->set_text(title);
			return 1;
		}
	}

// add another blank effect
	if(total_aeffects < TOTAL_EFFECTS)
	{
		audiomenu->add_item(
			aeffect[total_aeffects] = new MenuAEffectItem(aeffects, (char*)""));
		total_aeffects++;
	}

// cycle effect down
	for(int i = total_aeffects - 1; i > 0; i--)
	{
	// set menu item text
		aeffect[i]->set_text(aeffect[i - 1]->get_text());
	}

// set up the new effect
	aeffect[0]->set_text(title);
	return 0;
}

int MainMenu::add_veffect(char *title)
{
// add bar for first effect
	if(total_veffects == 0)
	{
		videomenu->add_item(new BC_MenuItem("-"));
	}

// test for existing copy of effect
	for(int i = 0; i < total_veffects; i++)
	{
		if(!strcmp(veffect[i]->get_text(), title))     // already exists
		{                                // swap for top effect
			for(int j = i; j > 0; j--)   // move preceeding effects down
			{
				veffect[j]->set_text(veffect[j - 1]->get_text());
			}
			veffect[0]->set_text(title);
			return 1;
		}
	}

// add another blank effect
	if(total_veffects < TOTAL_EFFECTS)
	{
		videomenu->add_item(veffect[total_veffects] =
			new MenuVEffectItem(veffects, (char*)""));
		total_veffects++;
	}

// cycle effect down
	for(int i = total_veffects - 1; i > 0; i--)
	{
// set menu item text
		veffect[i]->set_text(veffect[i - 1]->get_text());
	}

// set up the new effect
	veffect[0]->set_text(title);
	return 0;
}

int MainMenu::add_load(char *path)
{
	if(total_loads == 0)
{
		filemenu->add_item(new BC_MenuItem("-"));
	}

// test for existing copy
	FileSystem fs;
	char text[BCTEXTLEN], new_path[BCTEXTLEN];      // get text and path
	fs.extract_name(text, path);
	strcpy(new_path, path);

	for(int i = 0; i < total_loads; i++)
	{
		if(!strcmp(load[i]->get_text(), text))     // already exists
		{                                // swap for top load
			for(int j = i; j > 0; j--)   // move preceeding loads down
			{
				load[j]->set_text(load[j - 1]->get_text());
				load[j]->set_path(load[j - 1]->path);
	}
			load[0]->set_text(text);
			load[0]->set_path(new_path);

			return 1;
		}
	}

// add another load
	if(total_loads < TOTAL_LOADS)
	{
		filemenu->add_item(load[total_loads] = new LoadPrevious(mwindow, load_file));
		total_loads++;
	}

// cycle loads down
	for(int i = total_loads - 1; i > 0; i--)
	{
	// set menu item text
		load[i]->set_text(load[i - 1]->get_text());
	// set filename
		load[i]->set_path(load[i - 1]->path);
	}

// set up the new load
	load[0]->set_text(text);
	load[0]->set_path(new_path);
	return 0;
}








// ================================== menu items


DumpCICache::DumpCICache(MWindow *mwindow)
 : BC_MenuItem(_("Dump CICache"))
{ this->mwindow = mwindow; }

int DumpCICache::handle_event()
{
//	mwindow->cache->dump();
	return 1;
}

DumpEDL::DumpEDL(MWindow *mwindow)
 : BC_MenuItem(_("Dump EDL"))
{
	this->mwindow = mwindow;
}

int DumpEDL::handle_event()
{
//printf("DumpEDL::handle_event 1\n");
	mwindow->dump_edl();
//printf("DumpEDL::handle_event 2\n");
	return 1;
}

DumpPlugins::DumpPlugins(MWindow *mwindow)
 : BC_MenuItem(_("Dump Plugins"))
{
	this->mwindow = mwindow;
}

int DumpPlugins::handle_event()
{
//printf("DumpEDL::handle_event 1\n");
	mwindow->dump_plugins();
//printf("DumpEDL::handle_event 2\n");
	return 1;
}


DumpAssets::DumpAssets(MWindow *mwindow)
 : BC_MenuItem(_("Dump Assets"))
{ this->mwindow = mwindow; }

int DumpAssets::handle_event()
{
	mwindow->assets->dump();
	return 1;
}

// ================================================= edit

Undo::Undo(MWindow *mwindow) : BC_MenuItem(_("Undo"), "z", 'z')
{
	this->mwindow = mwindow;
}
int Undo::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->undo_entry(mwindow->gui);
	return 1;
}
int Undo::update_caption(const char *new_caption)
{
	char string[BCTEXTLEN];
	sprintf(string, _("Undo %s"), new_caption);
	set_text(string);
	return 0;
}


Redo::Redo(MWindow *mwindow) : BC_MenuItem(_("Redo"), _("Shift-Z"), 'Z')
{
	set_shift(1);
	this->mwindow = mwindow;
}

int Redo::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->redo_entry(mwindow->gui);
	return 1;
}
int Redo::update_caption(const char *new_caption)
{
	char string[BCTEXTLEN];
	sprintf(string, _("Redo %s"), new_caption);
	set_text(string);
	return 0;
}

CutKeyframes::CutKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Cut keyframes"), _("Shift-X"), 'X')
{
	set_shift();
	this->mwindow = mwindow;
}

int CutKeyframes::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->cut_automation();
	return 1;
}

CopyKeyframes::CopyKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Copy keyframes"), _("Shift-C"), 'C')
{
	set_shift();
	this->mwindow = mwindow;
}

int CopyKeyframes::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->copy_automation();
	return 1;
}

PasteKeyframes::PasteKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Paste keyframes"), _("Shift-V"), 'V')
{
	set_shift();
	this->mwindow = mwindow;
}

int PasteKeyframes::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->paste_automation();
	return 1;
}

ClearKeyframes::ClearKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Clear keyframes"), _("Shift-Del"), DELETE)
{
	set_shift();
	this->mwindow = mwindow;
}

int ClearKeyframes::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->clear_automation();
	return 1;
}



StraightenKeyframes::StraightenKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Change to linear"))
{
	this->mwindow = mwindow;
}

int StraightenKeyframes::handle_event()
{
	mwindow->set_automation_mode(FloatAuto::LINEAR);
	return 1;
}




BendKeyframes::BendKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Change to smooth"))
{
	this->mwindow = mwindow;
}

int BendKeyframes::handle_event()
{
	mwindow->set_automation_mode(FloatAuto::SMOOTH);
	return 1;
}



KeyframeCurveType::KeyframeCurveType(MWindow *mwindow)
 : BC_MenuItem(_("Create curve type..."))
{
	this->mwindow = mwindow;
	this->curve_menu = 0;
}
KeyframeCurveType::~KeyframeCurveType()
{
}

void KeyframeCurveType::create_objects()
{
	add_submenu(curve_menu = new KeyframeCurveTypeMenu(this));
	for( int i=FloatAuto::SMOOTH; i<=FloatAuto::FREE; ++i ) {
		KeyframeCurveTypeItem *curve_type_item = new KeyframeCurveTypeItem(i, this);
		curve_menu->add_submenuitem(curve_type_item);
	}
}

void KeyframeCurveType::update(int curve_type)
{
	for( int i=0; i<curve_menu->total_items(); ++i ) {
		KeyframeCurveTypeItem *curve_type_item = (KeyframeCurveTypeItem *)curve_menu->get_item(i);
		curve_type_item->set_checked(curve_type_item->type == curve_type);
	}
}

int KeyframeCurveType::handle_event()
{
	return 1;
}

KeyframeCurveTypeMenu::KeyframeCurveTypeMenu(KeyframeCurveType *menu_item)
 : BC_SubMenu()
{
	this->menu_item = menu_item;
}
KeyframeCurveTypeMenu::~KeyframeCurveTypeMenu()
{
}

KeyframeCurveTypeItem::KeyframeCurveTypeItem(int type, KeyframeCurveType *main_item)
 : BC_MenuItem(FloatAuto::curve_name(type))
{
	this->type = type;
	this->main_item = main_item;
}
KeyframeCurveTypeItem::~KeyframeCurveTypeItem()
{
}

int KeyframeCurveTypeItem::handle_event()
{
	main_item->update(type);
	main_item->mwindow->set_keyframe_type(type);
	return 1;
}


CutDefaultKeyframe::CutDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem(_("Cut default keyframe"), _("Alt-x"), 'x')
{
	set_alt();
	this->mwindow = mwindow;
}

int CutDefaultKeyframe::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->cut_default_keyframe();
	return 1;
}

CopyDefaultKeyframe::CopyDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem(_("Copy default keyframe"), _("Alt-c"), 'c')
{
	set_alt();
	this->mwindow = mwindow;
}

int CopyDefaultKeyframe::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->copy_default_keyframe();
	return 1;
}

PasteDefaultKeyframe::PasteDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem(_("Paste default keyframe"), _("Alt-v"), 'v')
{
	set_alt();
	this->mwindow = mwindow;
}

int PasteDefaultKeyframe::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->paste_default_keyframe();
	return 1;
}

ClearDefaultKeyframe::ClearDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem(_("Clear default keyframe"), _("Alt-Del"), DELETE)
{
	set_alt();
	this->mwindow = mwindow;
}

int ClearDefaultKeyframe::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->clear_default_keyframe();
	return 1;
}

Cut::Cut(MWindow *mwindow)
 : BC_MenuItem(_("Split | Cut"), "x", 'x')
{
	this->mwindow = mwindow;
}

int Cut::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->cut();
	return 1;
}

Copy::Copy(MWindow *mwindow)
 : BC_MenuItem(_("Copy"), "c", 'c')
{
	this->mwindow = mwindow;
}

int Copy::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->copy();
	return 1;
}

Paste::Paste(MWindow *mwindow)
 : BC_MenuItem(_("Paste"), "v", 'v')
{
	this->mwindow = mwindow;
}

int Paste::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->paste();
	return 1;
}

Clear::Clear(MWindow *mwindow)
 : BC_MenuItem(_("Clear"), _("Del"), DELETE)
{
	this->mwindow = mwindow;
}

int Clear::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		mwindow->cwindow->gui->lock_window("Clear::handle_event");
		mwindow->clear_entry();
		mwindow->cwindow->gui->unlock_window();
	}
	return 1;
}

PasteSilence::PasteSilence(MWindow *mwindow)
 : BC_MenuItem(_("Paste silence"), _("Shift-Space"), ' ')
{
	this->mwindow = mwindow;
	set_shift();
}

int PasteSilence::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->paste_silence();
	return 1;
}

SelectAll::SelectAll(MWindow *mwindow)
 : BC_MenuItem(_("Select All"), "a", 'a')
{
	this->mwindow = mwindow;
}

int SelectAll::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->select_all();
	return 1;
}

ClearLabels::ClearLabels(MWindow *mwindow) : BC_MenuItem(_("Clear labels"))
{
	this->mwindow = mwindow;
}

int ClearLabels::handle_event()
{
	mwindow->clear_labels();
	return 1;
}

CutCommercials::CutCommercials(MWindow *mwindow) : BC_MenuItem(_("Cut ads"))
{
	this->mwindow = mwindow;
}

int CutCommercials::handle_event()
{
	mwindow->cut_commercials();
	return 1;
}

DetachTransitions::DetachTransitions(MWindow *mwindow)
 : BC_MenuItem(_("Detach transitions"))
{
	this->mwindow = mwindow;
}

int DetachTransitions::handle_event()
{
	mwindow->detach_transitions();
	return 1;
}

MuteSelection::MuteSelection(MWindow *mwindow)
 : BC_MenuItem(_("Mute Region"), "m", 'm')
{
	this->mwindow = mwindow;
}

int MuteSelection::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->mute_selection();
	return 1;
}


TrimSelection::TrimSelection(MWindow *mwindow)
 : BC_MenuItem(_("Trim Selection"))
{
	this->mwindow = mwindow;
}

int TrimSelection::handle_event()
{
	mwindow->trim_selection();
	return 1;
}












// ============================================= audio

AddAudioTrack::AddAudioTrack(MWindow *mwindow)
 : BC_MenuItem(_("Add track"), "t", 't')
{
	this->mwindow = mwindow;
}

int AddAudioTrack::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->add_audio_track_entry(0, 0);
	return 1;
}

DeleteAudioTrack::DeleteAudioTrack(MWindow *mwindow)
 : BC_MenuItem(_("Delete track"))
{
	this->mwindow = mwindow;
}

int DeleteAudioTrack::handle_event()
{
	return 1;
}

DefaultATransition::DefaultATransition(MWindow *mwindow)
 : BC_MenuItem(_("Default Transition"), "u", 'u')
{
	this->mwindow = mwindow;
}

int DefaultATransition::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->paste_audio_transition();
	return 1;
}


MapAudio1::MapAudio1(MWindow *mwindow)
 : BC_MenuItem(_("Map 1:1"))
{
	this->mwindow = mwindow;
}

int MapAudio1::handle_event()
{
	mwindow->map_audio(MWindow::AUDIO_1_TO_1);
	return 1;
}

MapAudio2::MapAudio2(MWindow *mwindow)
 : BC_MenuItem(_("Map 5.1:2"))
{
	this->mwindow = mwindow;
}

int MapAudio2::handle_event()
{
	mwindow->map_audio(MWindow::AUDIO_5_1_TO_2);
	return 1;
}




// ============================================= video


AddVideoTrack::AddVideoTrack(MWindow *mwindow)
 : BC_MenuItem(_("Add track"), _("Shift-T"), 'T')
{
	set_shift();
	this->mwindow = mwindow;
}

int AddVideoTrack::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->add_video_track_entry();
	return 1;
}


DeleteVideoTrack::DeleteVideoTrack(MWindow *mwindow)
 : BC_MenuItem(_("Delete track"))
{
	this->mwindow = mwindow;
}

int DeleteVideoTrack::handle_event()
{
	return 1;
}



ResetTranslation::ResetTranslation(MWindow *mwindow)
 : BC_MenuItem(_("Reset Translation"))
{
	this->mwindow = mwindow;
}

int ResetTranslation::handle_event()
{
	return 1;
}



DefaultVTransition::DefaultVTransition(MWindow *mwindow)
 : BC_MenuItem(_("Default Transition"), _("Shift-U"), 'U')
{
	set_shift();
	this->mwindow = mwindow;
}

int DefaultVTransition::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->paste_video_transition();
	return 1;
}














// ============================================ settings

DeleteTracks::DeleteTracks(MWindow *mwindow)
 : BC_MenuItem(_("Delete tracks"))
{
	this->mwindow = mwindow;
}

int DeleteTracks::handle_event()
{
	mwindow->delete_tracks();
	return 1;
}

DeleteTrack::DeleteTrack(MWindow *mwindow)
 : BC_MenuItem(_("Delete last track"), "d", 'd')
{
	this->mwindow = mwindow;
}

int DeleteTrack::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->delete_track();
	return 1;
}

MoveTracksUp::MoveTracksUp(MWindow *mwindow)
 : BC_MenuItem(_("Move tracks up"), _("Shift-Up"), UP)
{
	set_shift(); this->mwindow = mwindow;
}

int MoveTracksUp::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->move_tracks_up();
	return 1;
}

MoveTracksDown::MoveTracksDown(MWindow *mwindow)
 : BC_MenuItem(_("Move tracks down"), _("Shift-Down"), DOWN)
{
	set_shift(); this->mwindow = mwindow;
}

int MoveTracksDown::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->move_tracks_down();
	return 1;
}




ConcatenateTracks::ConcatenateTracks(MWindow *mwindow)
 : BC_MenuItem(_("Concatenate tracks"))
{
	set_shift();
	this->mwindow = mwindow;
}

int ConcatenateTracks::handle_event()
{
	mwindow->concatenate_tracks();
	return 1;
}





LoopPlayback::LoopPlayback(MWindow *mwindow)
 : BC_MenuItem(_("Loop Playback"), _("Shift-L"), 'L')
{
	this->mwindow = mwindow;
	set_checked(mwindow->edl->local_session->loop_playback);
	set_shift();
}

int LoopPlayback::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		mwindow->toggle_loop_playback();
		set_checked(mwindow->edl->local_session->loop_playback);
	}
	return 1;
}



// ============================================= subtitle


AddSubttlTrack::AddSubttlTrack(MWindow *mwindow)
 : BC_MenuItem(_("Add subttl"), _("Shift-Y"), 'Y')
{
	set_shift();
	this->mwindow = mwindow;
}

int AddSubttlTrack::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->add_subttl_track_entry();
	return 1;
}

PasteSubttl::PasteSubttl(MWindow *mwindow)
 : BC_MenuItem(_("paste subttl"), "y", 'y')
{
	this->mwindow = mwindow;
}

int PasteSubttl::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->gui->swindow->paste_subttl();
	return 1;
}


SetBRenderActive::SetBRenderActive(MWindow *mwindow)
 : BC_MenuItem(_("Toggle background rendering"),_("Shift-G"),'G')
{
	this->mwindow = mwindow;
	set_shift(1);
}

int SetBRenderActive::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		int v = mwindow->brender_active ? 0 : 1;
		set_checked(v);
		mwindow->set_brender_active(v);
	}
	return 1;
}


LabelsFollowEdits::LabelsFollowEdits(MWindow *mwindow)
 : BC_MenuItem(_("Edit labels"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->edl->session->labels_follow_edits);
}

int LabelsFollowEdits::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->set_labels_follow_edits(get_checked());
	return 1;
}




PluginsFollowEdits::PluginsFollowEdits(MWindow *mwindow)
 : BC_MenuItem(_("Edit effects"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->edl->session->plugins_follow_edits);
}

int PluginsFollowEdits::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->plugins_follow_edits = get_checked();
	return 1;
}




KeyframesFollowEdits::KeyframesFollowEdits(MWindow *mwindow)
 : BC_MenuItem(_("Keyframes follow edits"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->edl->session->autos_follow_edits);
}

int KeyframesFollowEdits::handle_event()
{
	mwindow->edl->session->autos_follow_edits ^= 1;
	set_checked(!get_checked());
	return 1;
}


CursorOnFrames::CursorOnFrames(MWindow *mwindow)
 : BC_MenuItem(_("Align cursor on frames"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->edl->session->cursor_on_frames);
}

int CursorOnFrames::handle_event()
{
	mwindow->edl->session->cursor_on_frames = !mwindow->edl->session->cursor_on_frames;
	set_checked(mwindow->edl->session->cursor_on_frames);
	return 1;
}


TypelessKeyframes::TypelessKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Typeless keyframes"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->edl->session->typeless_keyframes);
}

int TypelessKeyframes::handle_event()
{
	mwindow->edl->session->typeless_keyframes = !mwindow->edl->session->typeless_keyframes;
	set_checked(mwindow->edl->session->typeless_keyframes);
	return 1;
}


ScrubSpeed::ScrubSpeed(MWindow *mwindow) : BC_MenuItem(_("Slow Shuttle"))
{
	this->mwindow = mwindow;
}

int ScrubSpeed::handle_event()
{
	if(mwindow->edl->session->scrub_speed == .5)
	{
		mwindow->edl->session->scrub_speed = 2;
		set_text(_("Slow Shuttle"));
	}
	else
	{
		mwindow->edl->session->scrub_speed = .5;
		set_text(_("Fast Shuttle"));
	}
	return 1;
}

SaveSettingsNow::SaveSettingsNow(MWindow *mwindow) : BC_MenuItem(_("Save settings now"))
{
	this->mwindow = mwindow;
}

int SaveSettingsNow::handle_event()
{
	mwindow->save_defaults();
	mwindow->save_backup();
	mwindow->gui->show_message(_("Saved settings."));
	return 1;
}



// ============================================ window





ShowVWindow::ShowVWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Viewer"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_vwindow);
}
int ShowVWindow::handle_event()
{
	mwindow->show_vwindow();
	return 1;
}

ShowAWindow::ShowAWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Resources"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_awindow);
}
int ShowAWindow::handle_event()
{
	mwindow->show_awindow();
	return 1;
}

ShowCWindow::ShowCWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Compositor"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_cwindow);
}
int ShowCWindow::handle_event()
{
	mwindow->show_cwindow();
	return 1;
}


ShowGWindow::ShowGWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Overlays"), _("Ctrl-0"), '0')
{
	this->mwindow = mwindow;
	set_ctrl(1);
	set_checked(mwindow->session->show_gwindow);
}
int ShowGWindow::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		if( !mwindow->session->show_gwindow )
			mwindow->show_gwindow();
		else
			mwindow->hide_gwindow();
		set_checked(mwindow->session->show_gwindow);
	}
	return 1;
}


ShowLWindow::ShowLWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Levels"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_lwindow);
}
int ShowLWindow::handle_event()
{
	mwindow->show_lwindow();
	return 1;
}

TileWindows::TileWindows(MWindow *mwindow, const char *item_title, int config,
		const char *hot_keytext, int hot_key)
 : BC_MenuItem(item_title, hot_keytext, hot_key)
{
	this->mwindow = mwindow;
	this->config = config;
	if( hot_key ) set_ctrl(1);
}
int TileWindows::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		int window_config = config >= 0 ? config :
			mwindow->session->window_config;
		if( mwindow->tile_windows(window_config) ) {
			mwindow->restart_status = 1;
			mwindow->gui->set_done(0);
		}
	}
	return 1;
}

SplitX::SplitX(MWindow *mwindow)
 : BC_MenuItem(_("Split X pane"), _("Ctrl-1"), '1')
{
	this->mwindow = mwindow;
	set_ctrl(1);
	set_checked(mwindow->gui->pane[TOP_RIGHT_PANE] != 0);
}
int SplitX::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->split_x();
	return 1;
}

SplitY::SplitY(MWindow *mwindow)
 : BC_MenuItem(_("Split Y pane"), _("Ctrl-2"), '2')
{
	this->mwindow = mwindow;
	set_ctrl(1);
	set_checked(mwindow->gui->pane[BOTTOM_LEFT_PANE] != 0);
}
int SplitY::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->split_y();
	return 1;
}


MixerViewer::MixerViewer(MWindow *mwindow)
 : BC_MenuItem(_("Mixer Viewer"), _("Shift-M"), 'M')
{
	this->mwindow = mwindow;
	set_shift(1);
}

int MixerViewer::handle_event()
{
	mwindow->start_mixer();
	return 1;
}

TileMixers::TileMixers(MWindow *mwindow)
 : BC_MenuItem(_("Tile mixers"), "Alt-t", 't')
{
	this->mwindow = mwindow;
	set_alt();
}

int TileMixers::handle_event()
{
	mwindow->tile_mixers();
	return 1;
}


LoadLayoutItem::LoadLayoutItem(LoadLayout *load_layout, const char *text, int no, int hotkey)
 : BC_MenuItem(text, "", hotkey)
{
	this->no = no;
	this->load_layout = load_layout;
	if( hotkey ) {
		char hot_txt[BCSTRLEN];
		sprintf(hot_txt, _("CtlSft+F%d"), hotkey-KEY_F1+1);
		set_ctrl();  set_shift();
		set_hotkey_text(hot_txt);
	}
}

int LoadLayoutItem::handle_event()
{
	MWindow *mwindow = load_layout->mwindow;
	switch( load_layout->action ) {
	case LAYOUT_LOAD:
		mwindow->load_layout(no);
		break;
	case LAYOUT_SAVE:
		mwindow->save_layout(no);
		break;
	}
	return 1;
}

LoadLayout::LoadLayout(MWindow *mwindow, const char *text, int action)
 : BC_MenuItem(text)
{
	this->mwindow = mwindow;
	this->action = action;
}

void LoadLayout::create_objects()
{
	BC_SubMenu *layout_submenu = new BC_SubMenu();
	add_submenu(layout_submenu);
	for( int i=1; i<=4; ++i ) {
		char text[BCSTRLEN];
		sprintf(text, _("Layout %d"), i);
		LoadLayoutItem *load_layout_item =
			new LoadLayoutItem(this, text, i,
				action==LAYOUT_LOAD ? KEY_F1-1+i : 0);
		layout_submenu->add_submenuitem(load_layout_item);
	}
}

