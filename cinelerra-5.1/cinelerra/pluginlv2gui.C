
/*
 * CINELERRA
 * Copyright (C) 2018 GG
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
#ifdef HAVE_LV2

#include "clip.h"
#include "cstrdup.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "mwindow.h"
#include "pluginlv2client.h"
#include "pluginlv2config.h"
#include "pluginlv2gui.h"
#include "pluginserver.h"
#include "preferences.h"
#include "samples.h"

#include <ctype.h>
#include <string.h>


PluginLV2ClientUI::PluginLV2ClientUI(PluginLV2ClientWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("UI"))
{
	this->gui = gui;
}

PluginLV2ClientUI::~PluginLV2ClientUI()
{
}

int PluginLV2ClientUI::handle_event()
{
	PluginLV2ParentUI *ui = gui->get_ui();
	if( ui->show() )
		flicker(8, 64);
	return 1;
}

PluginLV2ClientReset::
PluginLV2ClientReset(PluginLV2ClientWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->gui = gui;
}

PluginLV2ClientReset::
~PluginLV2ClientReset()
{
}

int PluginLV2ClientReset::handle_event()
{
	PluginLV2Client *client = gui->client;
	client->config.init_lv2(client->lilv, client);
	client->config.update();
	client->update_lv2(LV2_LOAD);
	gui->update(0);
	client->send_configure_change();
	return 1;
}

PluginLV2ClientText::PluginLV2ClientText(PluginLV2ClientWindow *gui, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, (char *)"")
{
	this->gui = gui;
}

PluginLV2ClientText::~PluginLV2ClientText()
{
}

int PluginLV2ClientText::handle_event()
{
	return 0;
}


PluginLV2ClientApply::PluginLV2ClientApply(PluginLV2ClientWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Apply"))
{
	this->gui = gui;
}

PluginLV2ClientApply::~PluginLV2ClientApply()
{
}

int PluginLV2ClientApply::handle_event()
{
	const char *text = gui->text->get_text();
	if( text && gui->selected ) {
		gui->selected->update(atof(text));
		gui->update_selected();
		gui->client->send_configure_change();
	}
	return 1;
}


PluginLV2Client_OptPanel::PluginLV2Client_OptPanel(PluginLV2ClientWindow *gui,
		int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT), opts(items[0]), vals(items[1])
{
	this->gui = gui;
	update();  // init col/wid/columns
}

PluginLV2Client_OptPanel::~PluginLV2Client_OptPanel()
{
}

int PluginLV2Client_OptPanel::selection_changed()
{
	PluginLV2Client_Opt *opt = 0;
	BC_ListBoxItem *item = get_selection(0, 0);
	if( item ) {
		PluginLV2Client_OptName *opt_name = (PluginLV2Client_OptName *)item;
		opt = opt_name->opt;
	}
	gui->update(opt);
	return 1;
}

void PluginLV2Client_OptPanel::update()
{
	opts.remove_all();
	vals.remove_all();
	PluginLV2ClientConfig &conf = gui->client->config;
	for( int i=0; i<conf.size(); ++i ) {
		PluginLV2Client_Opt *opt = conf[i];
		opts.append(opt->item_name);
		vals.append(opt->item_value);
	}
	const char *cols[] = { "option", "value", };
	const int col1_w = 150;
	int wids[] = { col1_w, get_w()-col1_w };
	BC_ListBox::update(&items[0], &cols[0], &wids[0], sizeof(items)/sizeof(items[0]),
		get_xposition(), get_yposition(), get_highlighted_item());
}

PluginLV2ClientPot::PluginLV2ClientPot(PluginLV2ClientWindow *gui, int x, int y)
 : BC_FPot(x, y, 0.f, 0.f, 0.f)
{
	this->gui = gui;
}

int PluginLV2ClientPot::handle_event()
{
	if( gui->selected ) {
		gui->selected->update(get_value());
		gui->update_selected();
		gui->client->send_configure_change();
	}
	return 1;
}

PluginLV2ClientSlider::PluginLV2ClientSlider(PluginLV2ClientWindow *gui, int x, int y)
 : BC_FSlider(x, y, 0, gui->get_w()-x-20, gui->get_w()-x-20, 0.f, 0.f, 0.f)
{
	this->gui = gui;
}

int PluginLV2ClientSlider::handle_event()
{
	if( gui->selected ) {
		gui->selected->update(get_value());
		gui->update_selected();
		gui->client->send_configure_change();
	}
	return 1;
}

PluginLV2ClientWindow::PluginLV2ClientWindow(PluginLV2Client *client)
 : PluginClientWindow(client, 500, 300, 500, 300, 1)
{
	this->client = client;
	selected = 0;
}

void PluginLV2ClientWindow::done_event(int result)
{
	PluginLV2ParentUI *ui = PluginLV2ParentUI::plugin_lv2.del_ui(this);
	if( ui ) ui->hide();
}

PluginLV2ClientWindow::~PluginLV2ClientWindow()
{
	done_event(0);
}


void PluginLV2ClientWindow::create_objects()
{
	BC_Title *title;
	int x = 10, y = 10, x1;
	add_subwindow(title = new BC_Title(x, y, client->title));
	x1 = get_w() - BC_GenericButton::calculate_w(this, _("UI")) - 8;
	add_subwindow(client_ui = new PluginLV2ClientUI(this, x1, y));
	y += title->get_h() + 10;
	add_subwindow(varbl = new BC_Title(x, y, ""));
	add_subwindow(range = new BC_Title(x+160, y, ""));
	x1 = get_w() - BC_GenericButton::calculate_w(this, _("Reset")) - 8;
	add_subwindow(reset = new PluginLV2ClientReset(this, x1, y));
	y += title->get_h() + 10;
	x1 = get_w() - BC_GenericButton::calculate_w(this, _("Apply")) - 8;
	add_subwindow(apply = new PluginLV2ClientApply(this, x1, y));
	add_subwindow(text = new PluginLV2ClientText(this, x, y, x1-x - 8));
	y += title->get_h() + 10;
	add_subwindow(pot = new PluginLV2ClientPot(this, x, y));
	x1 = x + pot->get_w() + 10;
	add_subwindow(slider = new PluginLV2ClientSlider(this, x1, y+10));
	y += pot->get_h() + 10;

	client->load_configuration();
	client->config.update();

	int panel_x = x, panel_y = y;
	int panel_w = get_w()-10 - panel_x;
	int panel_h = get_h()-10 - panel_y;
	panel = new PluginLV2Client_OptPanel(this, panel_x, panel_y, panel_w, panel_h);
	add_subwindow(panel);
	panel->update();
	show_window(1);

	if( client->server->mwindow->preferences->autostart_lv2ui ) {
		client_ui->disable();
		PluginLV2ParentUI *ui = get_ui();
		ui->show();
	}
}

int PluginLV2ClientWindow::resize_event(int w, int h)
{
	int x1;
	x1 = w - client_ui->get_w() - 8;
	client_ui->reposition_window(x1, client_ui->get_y());
	x1 = w - reset->get_w() - 8;
	reset->reposition_window(x1, reset->get_y());
	x1 = w - apply->get_w() - 8;
	apply->reposition_window(x1, apply->get_y());
	text->reposition_window(text->get_x(), text->get_y(), x1-text->get_x() - 8);
	x1 = pot->get_x() + pot->get_w() + 10;
	int w1 = w - slider->get_x() - 20;
	slider->set_pointer_motion_range(w1);
	slider->reposition_window(x1, slider->get_y(), w1, slider->get_h());
	int panel_x = panel->get_x(), panel_y = panel->get_y();
	panel->reposition_window(panel_x, panel_y, w-10-panel_x, h-10-panel_y);
	return 1;
}

void PluginLV2ClientWindow::update_selected()
{
	update(selected);
	if( !selected ) return;
	PluginLV2ParentUI *ui = find_ui();
	if( !ui ) return;
	PluginLV2ClientConfig &conf = client->config;
	ui->send_child(LV2_UPDATE, conf.ctls, sizeof(float)*conf.nb_ports);
}

int PluginLV2ClientWindow::scalar(float f, char *rp)
{
	const char *cp = 0;
	     if( f == FLT_MAX ) cp = "FLT_MAX";
	else if( f == FLT_MIN ) cp = "FLT_MIN";
	else if( f == -FLT_MAX ) cp = "-FLT_MAX";
	else if( f == -FLT_MIN ) cp = "-FLT_MIN";
	else if( f == 0 ) cp = signbit(f) ? "-0" : "0";
	else if( isnan(f) ) cp = signbit(f) ? "-NAN" : "NAN";
	else if( isinf(f) ) cp = signbit(f) ? "-INF" : "INF";
	else return sprintf(rp, "%g", f);
	return sprintf(rp, "%s", cp);
}

void PluginLV2ClientWindow::update(PluginLV2Client_Opt *opt)
{
	if( selected != opt ) {
		if( selected ) selected->item_name->set_selected(0);
		selected = opt;
		if( selected ) selected->item_name->set_selected(1);
	}
	char var[BCSTRLEN];  var[0] = 0;
	char val[BCSTRLEN];  val[0] = 0;
	char rng[BCTEXTLEN]; rng[0] = 0;
	if( opt ) {
		sprintf(var,"%s:", opt->conf->names[opt->idx]);
		char *cp = rng;
		cp += sprintf(cp,"( ");
		float min = opt->conf->mins[opt->idx];
		cp += scalar(min, cp);
		cp += sprintf(cp, " .. ");
		float max = opt->conf->maxs[opt->idx];
		cp += scalar(max, cp);
		cp += sprintf(cp, " )");
		float v = opt->get_value();
		sprintf(val, "%f", v);
		float p = (max-min) / slider->get_w();
		slider->set_precision(p);
		slider->update(slider->get_w(), v, min, max);
		pot->set_precision(p);
		pot->update(v, min, max);
	}
	else {
		slider->update(slider->get_w(), 0.f, 0.f, 0.f);
		pot->update(0.f, 0.f, 0.f);
	}
	varbl->update(var);
	range->update(rng);
	text->update(val);
	panel->update();
}

void PluginLV2ClientWindow::lv2_update()
{
	lock_window("PluginLV2ClientWindow::lv2_update");
	PluginLV2ClientConfig &conf = client->config;
	int ret = conf.update();
	if( ret > 0 ) update(0);
	unlock_window();
	if( ret > 0 )
		client->send_configure_change();
}

void PluginLV2ClientWindow::lv2_update(float *vals)
{
	PluginLV2ClientConfig &conf = client->config;
	int nb_ports = conf.nb_ports;
	float *ctls = conf.ctls;
	for( int i=0; i<nb_ports; ++i ) *ctls++ = *vals++;
	lv2_update();
}

void PluginLV2ClientWindow::lv2_set(int idx, float val)
{
	PluginLV2ClientConfig &conf = client->config;
	conf[idx]->set_value(val);
	lv2_update();
}

void PluginLV2ClientWindow::lv2_ui_enable()
{
	lock_window("PluginLV2ClientWindow::lv2_update");
	client_ui->enable();
	unlock_window();
}

#endif
