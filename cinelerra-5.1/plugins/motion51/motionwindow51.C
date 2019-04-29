
/*
 * CINELERRA
 * Copyright (C) 2012 Adam Williams <broadcast at earthling dot net>
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
#include "cstrdup.h"
#include "edl.h"
#include "fonts.h"
#include "edlsession.h"
#include "language.h"
#include "motion51.h"
#include "motionwindow51.h"
#include "mwindow.h"
#include "pluginserver.h"

Motion51Window::Motion51Window(Motion51Main *plugin)
 : PluginClientWindow(plugin, 600, 400, 600, 400, 0)
{
	this->plugin = plugin;
}

Motion51Window::~Motion51Window()
{
}

void Motion51Window::create_objects()
{
	int x = 10, y = 20;
	int x0 = x, x1 = get_w()/2;
	add_subwindow(sample_steps = new Motion51SampleSteps(plugin, x0=x, y, 120));
	BC_Title *title = new BC_Title(x0+=sample_steps->get_w()+10, y, _("Samples"));
	add_subwindow(title);
	sample_steps->create_objects();
	sample_r = new Motion51Limits(plugin, this, x1,y, _("Sample Radius%"),
		&plugin->config.sample_r, 0.f,100.f, 72);
	sample_r->create_objects();
	y += sample_r->get_h() + 20;

	block_x = new Motion51Limits(plugin, this, x0=x,y, _("Center X%"),
		&plugin->config.block_x, 0.f, 100.f, 72);
	block_x->create_objects();
	block_y = new Motion51Limits(plugin, this, x1,y, _("Center Y%"),
		&plugin->config.block_y, 0.f, 100.f, 72);
	block_y->create_objects();
	y += block_x->get_h() + 10;
	block_w = new Motion51Limits(plugin, this, x0,y, _("Search W%"),
		&plugin->config.block_w, 0.f,100.f, 72);
	block_w->create_objects();
	block_h = new Motion51Limits(plugin, this, x1,y, _("Search H%"),
		&plugin->config.block_h, 0.f,100.f, 72);
	block_h->create_objects();
	y += block_w->get_h() + 10;

	horiz_limit = new Motion51Limits(plugin, this, x0=x,y, _("Horiz shake limit%"),
		&plugin->config.horiz_limit, 0.f, 75.f, 72);
	horiz_limit->create_objects();
	shake_fade = new Motion51Limits(plugin, this, x1,y, _("Shake fade%"),
		&plugin->config.shake_fade, 0.f, 75.f, 72);
	shake_fade->create_objects();
	y += horiz_limit->get_h() + 10;
	vert_limit = new Motion51Limits(plugin, this, x0,y, _("Vert shake limit%"),
		&plugin->config.vert_limit, 0.f, 75.f, 72);
	vert_limit->create_objects();
	y += vert_limit->get_h() + 10;
	twist_limit = new Motion51Limits(plugin, this, x0,y, _("Twist limit%"),
		&plugin->config.twist_limit, 0.f, 75.f, 72);
	twist_limit->create_objects();
	twist_fade = new Motion51Limits(plugin, this, x1,y, _("Twist fade%"),
		&plugin->config.twist_fade, 0.f, 75.f, 72);
	twist_fade->create_objects();
	y += twist_fade->get_h() + 20;

	add_subwindow(draw_vectors = new Motion51DrawVectors(plugin, this, x, y));
	add_subwindow(title = new BC_Title(x1, y, _("Tracking file:")));
	y += draw_vectors->get_h() + 5;
	add_subwindow(enable_tracking = new Motion51EnableTracking(plugin, this, x, y));
	add_subwindow(tracking_file = new Motion51TrackingFile(plugin,
		plugin->config.tracking_file, this, x1, y, get_w()-30-x1));
	y += enable_tracking->get_h() + 20;

	add_subwindow(reset_config = new Motion51ResetConfig(plugin, this, x0=x, y));
	add_subwindow(reset_tracking = new Motion51ResetTracking(plugin, this, x1, y));
	y += reset_config->get_h()+20;

	int pef = client->server->mwindow->edl->session->video_every_frame;
	add_subwindow(pef_title = new BC_Title(x, y,
		!pef ?	_("For best results\n"
				" Set: Play every frame\n"
				" Preferences-> Playback-> Video Out") :
			_("Currently using: Play every frame"), MEDIUMFONT,
		!pef ? RED : GREEN));

	show_window(1);
}

void Motion51Window::update_gui()
{
	Motion51Config &config = plugin->config;
	horiz_limit->update(config.horiz_limit);
	vert_limit->update(config.vert_limit);
	twist_limit->update(config.twist_limit);
	shake_fade->update(config.shake_fade);
	twist_fade->update(config.twist_fade);

	sample_r->update(config.sample_r);
	char string[BCTEXTLEN];
	sprintf(string, "%d", config.sample_steps);
	sample_steps->set_text(string);

	block_w->update(config.block_w);
	block_h->update(config.block_h);
	block_x->update(config.block_x);
	block_y->update(config.block_y);

	draw_vectors->update(config.draw_vectors);
	tracking_file->update(config.tracking_file);
	enable_tracking->update(config.tracking);
}

Motion51Limits::Motion51Limits(Motion51Main *plugin, Motion51Window *gui, int x, int y,
	const char *ttext, float *value, float min, float max, int ttbox_w)
 : BC_TumbleTextBox(gui, *value, min, max, x, y, ttbox_w)
{
	this->plugin = plugin;
	this->gui = gui;
	this->ttext = cstrdup(ttext);
	this->value = value;
	title = 0;
}

Motion51Limits::~Motion51Limits()
{
	delete [] ttext;
}

void Motion51Limits::create_objects()
{
	BC_TumbleTextBox::create_objects();
	int tx = BC_TumbleTextBox::get_x() + BC_TumbleTextBox::get_w() + 5;
	int ty = BC_TumbleTextBox::get_y();
	gui->add_subwindow(title = new BC_Title(tx,ty,ttext));
}

int Motion51Limits::handle_event()
{
	*value = atof(get_text());
	plugin->send_configure_change();
	return 1;
}

Motion51TrackingFile::Motion51TrackingFile(Motion51Main *plugin,
	const char *filename, Motion51Window *gui, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, filename)
{
	this->plugin = plugin;
	this->gui = gui;
};

int Motion51TrackingFile::handle_event()
{
	strcpy(plugin->config.tracking_file, get_text());
	plugin->send_configure_change();
	return 1;
}


Motion51SampleSteps::Motion51SampleSteps(Motion51Main *plugin,
	int x, int y, int w)
 : BC_PopupMenu(x, y, w, "", 1)
{
	this->plugin = plugin;
}

void Motion51SampleSteps::create_objects()
{
	add_item(new BC_MenuItem("16"));
	add_item(new BC_MenuItem("32"));
	add_item(new BC_MenuItem("64"));
	add_item(new BC_MenuItem("128"));
	add_item(new BC_MenuItem("256"));
	add_item(new BC_MenuItem("512"));
	add_item(new BC_MenuItem("1024"));
	add_item(new BC_MenuItem("2048"));
	add_item(new BC_MenuItem("4096"));
	add_item(new BC_MenuItem("8192"));
	add_item(new BC_MenuItem("16384"));
	add_item(new BC_MenuItem("32768"));
	char string[BCTEXTLEN];
	sprintf(string, "%d", plugin->config.sample_steps);
	set_text(string);
}

int Motion51SampleSteps::handle_event()
{
	plugin->config.sample_steps = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}



Motion51DrawVectors::Motion51DrawVectors(Motion51Main *plugin,
	Motion51Window *gui, int x, int y)
 : BC_CheckBox(x, y, plugin->config.draw_vectors, _("Draw vectors"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int Motion51DrawVectors::handle_event()
{
	plugin->config.draw_vectors = get_value();
	plugin->send_configure_change();
	return 1;
}


Motion51ResetConfig::Motion51ResetConfig(Motion51Main *plugin, Motion51Window *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset defaults"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int Motion51ResetConfig::handle_event()
{
	plugin->config.init();
	gui->update_gui();
	plugin->send_configure_change();
	return 1;
}

Motion51ResetTracking::Motion51ResetTracking(Motion51Main *plugin, Motion51Window *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset Tracking"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int Motion51ResetTracking::handle_event()
{
	plugin->set_tracking_path();
	gui->tracking_file->update(plugin->config.tracking_file);
	plugin->config.tracking = 0;
	plugin->send_configure_change();
	gui->enable_tracking->update(0);
	::remove(plugin->config.tracking_file);
	return 1;
}

Motion51EnableTracking::Motion51EnableTracking(Motion51Main *plugin, Motion51Window *gui, int x, int y)
 : BC_CheckBox(x, y, plugin->config.tracking,_("Enable Tracking"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int Motion51EnableTracking::handle_event()
{
	plugin->config.tracking = get_value();
	plugin->send_configure_change();
	return 1;
}

