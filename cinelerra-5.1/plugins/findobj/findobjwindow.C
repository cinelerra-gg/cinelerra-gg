
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "dragcheckbox.h"
#include "language.h"
#include "findobj.h"
#include "findobjwindow.h"
#include "plugin.h"
#include "pluginserver.h"
#include "theme.h"
#include "track.h"


FindObjWindow::FindObjWindow(FindObjMain *plugin)
 : PluginClientWindow(plugin, 500, 700, 500, 700, 0)
{
	this->plugin = plugin;
}

FindObjWindow::~FindObjWindow()
{
}

void FindObjWindow::create_objects()
{
	int x = 10, y = 10, x1 = x, x2 = get_w()*1/3, x3 = get_w()*2/3;
	plugin->load_configuration();

	BC_Title *title;
	add_subwindow(title = new BC_Title(x1, y, _("Mode:")));
	add_subwindow(mode = new FindObjMode(plugin, this,
		x1 + 100, y));
	add_subwindow(reset = new FindObjReset(plugin, this, get_w()-15, y));
	mode->create_objects();
	y += mode->get_h() + 10;
	int y0 = y;
	add_subwindow(title = new BC_Title(x1, y, _("Algorithm:")));
	add_subwindow(algorithm = new FindObjAlgorithm(plugin, this,
		x1 + 100, y));
	algorithm->create_objects();
	y += algorithm->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(use_flann = new FindObjUseFlann(plugin, this, x, y));
	y += use_flann->get_h() + 10;

	int y1 = y;  y = y0;
	add_subwindow(replace_object = new FindObjReplace(plugin, this,x3, y));
	y += replace_object->get_h() + 10;
	add_subwindow(draw_match = new FindObjDrawMatch(plugin, this, x3, y));
	y += draw_match->get_h() + 10;
	add_subwindow(aspect = new FindObjAspect(plugin, this, x3, y));
	y += aspect->get_h() + 10;
	add_subwindow(scale = new FindObjScale(plugin, this, x3, y));
	y += scale->get_h() + 10;
	add_subwindow(rotate = new FindObjRotate(plugin, this, x3, y));
	y += rotate->get_h() + 10;
	add_subwindow(translate = new FindObjTranslate(plugin, this, x3, y));

	int x0 = x + 200;  y = y1 + 10;
	add_subwindow(title = new BC_Title(x, y, _("Output/scene layer:")));
	scene_layer = new FindObjLayer(plugin, this, x0, y,
		&plugin->config.scene_layer);
	scene_layer->create_objects();
	y += scene_layer->get_h() + plugin->get_theme()->widget_border;

	add_subwindow(title = new BC_Title(x, y, _("Object layer:")));
	object_layer = new FindObjLayer(plugin, this, x0, y,
		&plugin->config.object_layer);
	object_layer->create_objects();
	y += object_layer->get_h() + plugin->get_theme()->widget_border;

	add_subwindow(title = new BC_Title(x, y, _("Replacement object layer:")));
	replace_layer = new FindObjLayer(plugin, this, x0, y,
		&plugin->config.replace_layer);
	replace_layer->create_objects();
	y += replace_layer->get_h() + plugin->get_theme()->widget_border + 10;

	y += 10;
	add_subwindow(title = new BC_Title(x+15, y, _("Units: 0 to 100 percent")));
	y += title->get_h();

	y1 = y;
	add_subwindow(title = new BC_Title(x1, y + 10, _("Scene X:")));
	Track *track = plugin->server->plugin->track;
	int trk_w = track->track_w, trk_h = track->track_h;
	float drag_w = trk_w * plugin->config.scene_w / 100.;
	float drag_h = trk_h * plugin->config.scene_h / 100.;
	float ctr_x  = trk_w * plugin->config.scene_x / 100.;
	float ctr_y  = trk_h * plugin->config.scene_y / 100.;
	float drag_x = ctr_x - drag_w/2, drag_y = ctr_y - drag_h/2;
	drag_scene = new FindObjDragScene(plugin, this, x1+title->get_w()+10, y+5,
		drag_x, drag_y, drag_w, drag_h);
	add_subwindow(drag_scene);
	drag_scene->create_objects();
	y += title->get_h() + 15;

	add_subwindow(scene_x = new FindObjScanFloat(plugin, this,
		x1, y, &plugin->config.scene_x));
	add_subwindow(scene_x_text = new FindObjScanFloatText(plugin, this,
		x1 + scene_x->get_w() + 10, y + 10, &plugin->config.scene_x));
	scene_x->center_text = scene_x_text;
	scene_x_text->center = scene_x;

	y += 40;
	add_subwindow(title = new BC_Title(x1, y + 10, _("Scene Y:")));
	y += title->get_h() + 15;
	add_subwindow(scene_y = new FindObjScanFloat(plugin, this,
		x1, y, &plugin->config.scene_y));
	add_subwindow(scene_y_text = new FindObjScanFloatText(plugin, this,
		x1 + scene_y->get_w() + 10, y + 10, &plugin->config.scene_y));
	scene_y->center_text = scene_y_text;
	scene_y_text->center = scene_y;

	y += 40;
	add_subwindow(new BC_Title(x1, y + 10, _("Scene W:")));
	y += title->get_h() + 15;
	add_subwindow(scene_w = new FindObjScanFloat(plugin, this,
		x1, y, &plugin->config.scene_w));
	add_subwindow(scene_w_text = new FindObjScanFloatText(plugin, this,
		x1 + scene_w->get_w() + 10, y + 10, &plugin->config.scene_w));
	scene_w->center_text = scene_w_text;
	scene_w_text->center = scene_w;

	y += 40;
	add_subwindow(title = new BC_Title(x1, y + 10, _("Scene H:")));
	y += title->get_h() + 15;
	add_subwindow(scene_h = new FindObjScanFloat(plugin, this,
		x1, y, &plugin->config.scene_h));
	add_subwindow(scene_h_text = new FindObjScanFloatText(plugin, this,
		x1 + scene_h->get_w() + 10, y + 10,
		&plugin->config.scene_h));
	scene_h->center_text = scene_h_text;
	scene_h_text->center = scene_h;

	y = y1;
	add_subwindow(title = new BC_Title(x2, y + 10, _("Object X:")));
	drag_w = trk_w * plugin->config.object_w / 100.;
	drag_h = trk_h * plugin->config.object_h / 100.;
	ctr_x  = trk_w * plugin->config.object_x / 100.,
	ctr_y  = trk_h * plugin->config.object_y / 100.;
	drag_x = ctr_x - drag_w/2;  drag_y = ctr_y - drag_h/2;
	drag_object = new FindObjDragObject(plugin, this, x2+title->get_w()+10, y+5,
		drag_x, drag_y, drag_w, drag_h);
	add_subwindow(drag_object);
	drag_object->create_objects();
	y += title->get_h() + 15;

	add_subwindow(object_x = new FindObjScanFloat(plugin, this,
		x2, y, &plugin->config.object_x));
	add_subwindow(object_x_text = new FindObjScanFloatText(plugin, this,
		x2 + object_x->get_w() + 10, y + 10, &plugin->config.object_x));
	object_x->center_text = object_x_text;
	object_x_text->center = object_x;

	y += 40;
	add_subwindow(title = new BC_Title(x2, y + 10, _("Object Y:")));
	y += title->get_h() + 15;
	add_subwindow(object_y = new FindObjScanFloat(plugin, this,
		x2, y, &plugin->config.object_y));
	add_subwindow(object_y_text = new FindObjScanFloatText(plugin, this,
		x2 + object_y->get_w() + 10, y + 10, &plugin->config.object_y));
	object_y->center_text = object_y_text;
	object_y_text->center = object_y;

	y += 40;
	add_subwindow(new BC_Title(x2, y + 10, _("Object W:")));
	y += title->get_h() + 15;
	add_subwindow(object_w = new FindObjScanFloat(plugin, this,
		x2, y, &plugin->config.object_w));
	add_subwindow(object_w_text = new FindObjScanFloatText(plugin, this,
		x2 + object_w->get_w() + 10, y + 10, &plugin->config.object_w));
	object_w->center_text = object_w_text;
	object_w_text->center = object_w;

	y += 40;
	add_subwindow(title = new BC_Title(x2, y + 10, _("Object H:")));
	y += title->get_h() + 15;
	add_subwindow(object_h = new FindObjScanFloat(plugin, this,
		x2, y, &plugin->config.object_h));
	add_subwindow(object_h_text = new FindObjScanFloatText(plugin, this,
		x2 + object_h->get_w() + 10, y + 10,
		&plugin->config.object_h));
	object_h->center_text = object_h_text;
	object_h_text->center = object_h;

	y = y1;
	add_subwindow(title = new BC_Title(x3, y + 10, _("Replace X:")));
	drag_w = trk_w * plugin->config.replace_w / 100.;
	drag_h = trk_h * plugin->config.replace_h / 100.;
	ctr_x  = trk_w * plugin->config.replace_x / 100.,
	ctr_y  = trk_h * plugin->config.replace_y / 100.;
	drag_x = ctr_x - drag_w/2;  drag_y = ctr_y - drag_h/2;
	drag_replace = new FindObjDragReplace(plugin, this, x3+title->get_w()+10, y+5,
		drag_x, drag_y, drag_w, drag_h);
	add_subwindow(drag_replace);
	drag_replace->create_objects();
	y += title->get_h() + 15;

	add_subwindow(replace_x = new FindObjScanFloat(plugin, this,
		x3, y, &plugin->config.replace_x));
	add_subwindow(replace_x_text = new FindObjScanFloatText(plugin, this,
		x3 + replace_x->get_w() + 10, y + 10, &plugin->config.replace_x));
	replace_x->center_text = replace_x_text;
	replace_x_text->center = replace_x;

	y += 40;
	add_subwindow(title = new BC_Title(x3, y + 10, _("Replace Y:")));
	y += title->get_h() + 15;
	add_subwindow(replace_y = new FindObjScanFloat(plugin, this,
		x3, y, &plugin->config.replace_y));
	add_subwindow(replace_y_text = new FindObjScanFloatText(plugin, this,
		x3 + replace_y->get_w() + 10, y + 10, &plugin->config.replace_y));
	replace_y->center_text = replace_y_text;
	replace_y_text->center = replace_y;

	y += 40;
	add_subwindow(new BC_Title(x3, y + 10, _("Replace W:")));
	y += title->get_h() + 15;
	add_subwindow(replace_w = new FindObjScanFloat(plugin, this,
		x3, y, &plugin->config.replace_w));
	add_subwindow(replace_w_text = new FindObjScanFloatText(plugin, this,
		x3 + replace_w->get_w() + 10, y + 10, &plugin->config.replace_w));
	replace_w->center_text = replace_w_text;
	replace_w_text->center = replace_w;

	y += 40;
	add_subwindow(title = new BC_Title(x3, y + 10, _("Replace H:")));
	y += title->get_h() + 15;
	add_subwindow(replace_h = new FindObjScanFloat(plugin, this,
		x3, y, &plugin->config.replace_h));
	add_subwindow(replace_h_text = new FindObjScanFloatText(plugin, this,
		x3 + replace_h->get_w() + 10, y + 10,
		&plugin->config.replace_h));
	replace_h->center_text = replace_h_text;
	replace_h_text->center = replace_h;

	y += 40;  int y2 = y;
	add_subwindow(title = new BC_Title(x3, y + 10, _("Replace DX:")));
	y += title->get_h() + 15;
	add_subwindow(replace_dx = new FindObjScanFloat(plugin, this,
		x3, y, &plugin->config.replace_dx, -100.f, 100.f));
	add_subwindow(replace_dx_text = new FindObjScanFloatText(plugin, this,
		x3 + replace_dx->get_w() + 10, y + 10, &plugin->config.replace_dx));
	replace_dx->center_text = replace_dx_text;
	replace_dx_text->center = replace_dx;

	y += 40;
	add_subwindow(title = new BC_Title(x3, y + 10, _("Replace DY:")));
	y += title->get_h() + 15;
	add_subwindow(replace_dy = new FindObjScanFloat(plugin, this,
		x3, y, &plugin->config.replace_dy, -100.f, 100.f));
	add_subwindow(replace_dy_text = new FindObjScanFloatText(plugin, this,
		x3 + replace_dy->get_w() + 10, y + 10, &plugin->config.replace_dy));
	replace_dy->center_text = replace_dy_text;
	replace_dy_text->center = replace_dy;

	y = y2 + 15;
	add_subwindow(draw_keypoints = new FindObjDrawKeypoints(plugin, this, x, y));
	y += draw_keypoints->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(draw_scene_border = new FindObjDrawSceneBorder(plugin, this, x, y));
	y += draw_scene_border->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(draw_object_border = new FindObjDrawObjectBorder(plugin, this, x, y));
	y += draw_object_border->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(draw_replace_border = new FindObjDrawReplaceBorder(plugin, this, x, y));
	y += draw_object_border->get_h() + plugin->get_theme()->widget_border;

	add_subwindow(title = new BC_Title(x, y + 10, _("Object blend amount:")));
	add_subwindow(blend = new FindObjBlend(plugin,
		x + title->get_w() + plugin->get_theme()->widget_border, y,
		&plugin->config.blend));
	y += blend->get_h();

	show_window(1);
}

FindObjReset::FindObjReset(FindObjMain *plugin, FindObjWindow *gui, int x, int y)
 : BC_GenericButton(x - BC_GenericButton::calculate_w(gui, _("Reset")), y, _("Reset"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FindObjReset::handle_event()
{
	plugin->config.reset();
	gui->drag_scene->drag_deactivate();
	gui->drag_object->drag_deactivate();
	gui->drag_replace->drag_deactivate();
	gui->update_gui();
	plugin->send_configure_change();
	return 1;
}

void FindObjWindow::update_drag()
{
	Track *track = drag_scene->get_drag_track();
	int trk_w = track->track_w, trk_h = track->track_h;
	drag_scene->drag_w = trk_w * plugin->config.scene_w/100.;
	drag_scene->drag_h = trk_h * plugin->config.scene_h/100.;
	drag_scene->drag_x = trk_w * plugin->config.scene_x/100. - drag_scene->drag_w/2;
	drag_scene->drag_y = trk_h * plugin->config.scene_y/100. - drag_scene->drag_h/2;
	track = drag_object->get_drag_track();
	trk_w = track->track_w, trk_h = track->track_h;
	drag_object->drag_w = trk_w * plugin->config.object_w/100.;
	drag_object->drag_h = trk_h * plugin->config.object_h/100.;
	drag_object->drag_x = trk_w * plugin->config.object_x/100. - drag_object->drag_w/2;
	drag_object->drag_y = trk_h * plugin->config.object_y/100. - drag_object->drag_h/2;
	track = drag_replace->get_drag_track();
	trk_w = track->track_w, trk_h = track->track_h;
	drag_replace->drag_w = trk_w * plugin->config.replace_w/100.;
	drag_replace->drag_h = trk_h * plugin->config.replace_h/100.;
	drag_replace->drag_x = trk_w * plugin->config.replace_x/100. - drag_replace->drag_w/2;
	drag_replace->drag_y = trk_h * plugin->config.replace_y/100. - drag_replace->drag_h/2;
}

void FindObjWindow::update_gui()
{
	update_drag();
	FindObjConfig &conf = plugin->config;
	algorithm->update(conf.algorithm);
	use_flann->update(conf.use_flann);
	draw_match->update(conf.draw_match);
	aspect->update(conf.aspect);
	scale->update(conf.scale);
	rotate->update(conf.rotate);
	translate->update(conf.translate);
	drag_object->update(conf.drag_object);
	object_x->update(conf.object_x);
	object_x_text->update((float)conf.object_x);
	object_y->update(conf.object_y);
	object_y_text->update((float)conf.object_y);
	object_w->update(conf.object_w);
	object_w_text->update((float)conf.object_w);
	object_h->update(conf.object_h);
	object_h_text->update((float)conf.object_h);
	drag_scene->update(conf.drag_scene);
	scene_x->update(conf.scene_x);
	scene_x_text->update((float)conf.scene_x);
	scene_y->update(conf.scene_y);
	scene_y_text->update((float)conf.scene_y);
	scene_w->update(conf.scene_w);
	scene_w_text->update((float)conf.scene_w);
	scene_h->update(conf.scene_h);
	scene_h_text->update((float)conf.scene_h);
	drag_replace->update(conf.drag_replace);
	replace_x->update(conf.replace_x);
	replace_x_text->update((float)conf.replace_x);
	replace_y->update(conf.replace_y);
	replace_y_text->update((float)conf.replace_y);
	replace_w->update(conf.replace_w);
	replace_w_text->update((float)conf.replace_w);
	replace_h->update(conf.replace_h);
	replace_h_text->update((float)conf.replace_h);
	replace_dx->update(conf.replace_dx);
	replace_dx_text->update((float)conf.replace_dx);
	replace_dx->update(conf.replace_dx);
	replace_dx_text->update((float)conf.replace_dx);
	draw_keypoints->update(conf.draw_keypoints);
	draw_scene_border->update(conf.draw_scene_border);
	replace_object->update(conf.replace_object);
	draw_object_border->update(conf.draw_object_border);
	draw_replace_border->update(conf.draw_replace_border);
	object_layer->update( (int64_t)conf.object_layer);
	replace_layer->update( (int64_t)conf.replace_layer);
	scene_layer->update( (int64_t)conf.scene_layer);
	blend->update( (int64_t)conf.blend);
}

FindObjScanFloat::FindObjScanFloat(FindObjMain *plugin, FindObjWindow *gui,
	int x, int y, float *value, float min, float max)
 : BC_FPot(x, y, *value, min, max)
{
	this->plugin = plugin;
	this->gui = gui;
	this->value = value;
	this->center_text = 0;
	set_precision(0.1);
}

int FindObjScanFloat::handle_event()
{
	*value = get_value();
	center_text->update(*value);
	gui->update_drag();
	plugin->send_configure_change();
	return 1;
}

void FindObjScanFloat::update(float v)
{
	BC_FPot::update(*value = v);
	center_text->update(v);
	plugin->send_configure_change();
}


FindObjScanFloatText::FindObjScanFloatText(FindObjMain *plugin, FindObjWindow *gui,
	int x, int y, float *value)
 : BC_TextBox(x, y, 75, 1, *value)
{
	this->plugin = plugin;
	this->gui = gui;
	this->value = value;
	this->center = 0;
	set_precision(1);
}

int FindObjScanFloatText::handle_event()
{
	*value = atof(get_text());
	center->update(*value);
	gui->update_drag();
	plugin->send_configure_change();
	return 1;
}


FindObjDrawSceneBorder::FindObjDrawSceneBorder(FindObjMain *plugin, FindObjWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.draw_scene_border, _("Draw scene border"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjDrawSceneBorder::handle_event()
{
	plugin->config.draw_scene_border = get_value();
	plugin->send_configure_change();
	return 1;
}

FindObjDrawObjectBorder::FindObjDrawObjectBorder(FindObjMain *plugin, FindObjWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.draw_object_border, _("Draw object border"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjDrawObjectBorder::handle_event()
{
	plugin->config.draw_object_border = get_value();
	plugin->send_configure_change();
	return 1;
}

FindObjDrawReplaceBorder::FindObjDrawReplaceBorder(FindObjMain *plugin, FindObjWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.draw_replace_border, _("Draw replace border"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjDrawReplaceBorder::handle_event()
{
	plugin->config.draw_replace_border = get_value();
	plugin->send_configure_change();
	return 1;
}


FindObjDrawKeypoints::FindObjDrawKeypoints(FindObjMain *plugin, FindObjWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.draw_keypoints, _("Draw keypoints"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjDrawKeypoints::handle_event()
{
	plugin->config.draw_keypoints = get_value();
	plugin->send_configure_change();
	return 1;
}


FindObjReplace::FindObjReplace(FindObjMain *plugin, FindObjWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.replace_object, _("Replace object"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjReplace::handle_event()
{
	plugin->config.replace_object = get_value();
	plugin->send_configure_change();
	return 1;
}


FindObjDragScene::FindObjDragScene(FindObjMain *plugin, FindObjWindow *gui, int x, int y,
		float drag_x, float drag_y, float drag_w, float drag_h)
 : DragCheckBox(plugin->server->mwindow, x, y, _("Drag"), &plugin->config.drag_scene,
		drag_x, drag_y, drag_w, drag_h)
{
	this->plugin = plugin;
	this->gui = gui;
}

FindObjDragScene::~FindObjDragScene()
{
}

int FindObjDragScene::handle_event()
{
	int ret = DragCheckBox::handle_event();
	plugin->send_configure_change();
	return ret;
}
Track *FindObjDragScene::get_drag_track()
{
	return plugin->server->plugin->track;
}
int64_t FindObjDragScene::get_drag_position()
{
	return plugin->get_source_position();
}
void FindObjDragScene::update_gui()
{
	bound();
	Track *track = get_drag_track();
	int trk_w = track->track_w, trk_h = track->track_h;
	float ctr_x = drag_x + drag_w/2, ctr_y = drag_y + drag_h/2;
	gui->scene_x->update( plugin->config.scene_x = 100. * ctr_x / trk_w );
	gui->scene_y->update( plugin->config.scene_y = 100. * ctr_y / trk_h );
	gui->scene_w->update( plugin->config.scene_w = 100. * drag_w / trk_w );
	gui->scene_h->update( plugin->config.scene_h = 100. * drag_h / trk_h );
	plugin->send_configure_change();
}

FindObjDragObject::FindObjDragObject(FindObjMain *plugin, FindObjWindow *gui, int x, int y,
		float drag_x, float drag_y, float drag_w, float drag_h)
 : DragCheckBox(plugin->server->mwindow, x, y, _("Drag"), &plugin->config.drag_object,
		drag_x, drag_y, drag_w, drag_h)
{
	this->plugin = plugin;
	this->gui = gui;
}

FindObjDragObject::~FindObjDragObject()
{
}

int FindObjDragObject::handle_event()
{
	int ret = DragCheckBox::handle_event();
	plugin->send_configure_change();
	return ret;
}
Track *FindObjDragObject::get_drag_track()
{
	return plugin->server->plugin->track;
}
int64_t FindObjDragObject::get_drag_position()
{
	return plugin->get_source_position();
}
void FindObjDragObject::update_gui()
{
	bound();
	Track *track = get_drag_track();
	int trk_w = track->track_w, trk_h = track->track_h;
	float ctr_x = drag_x + drag_w/2, ctr_y = drag_y + drag_h/2;
	gui->object_x->update( plugin->config.object_x = 100. * ctr_x / trk_w );
	gui->object_y->update( plugin->config.object_y = 100. * ctr_y / trk_h );
	gui->object_w->update( plugin->config.object_w = 100. * drag_w / trk_w );
	gui->object_h->update( plugin->config.object_h = 100. * drag_h / trk_h );
	plugin->send_configure_change();
}

FindObjDragReplace::FindObjDragReplace(FindObjMain *plugin, FindObjWindow *gui, int x, int y,
		float drag_x, float drag_y, float drag_w, float drag_h)
 : DragCheckBox(plugin->server->mwindow, x, y, _("Drag"), &plugin->config.drag_replace,
		drag_x, drag_y, drag_w, drag_h)
{
	this->plugin = plugin;
	this->gui = gui;
}

FindObjDragReplace::~FindObjDragReplace()
{
}

int FindObjDragReplace::handle_event()
{
	int ret = DragCheckBox::handle_event();
	plugin->send_configure_change();
	return ret;
}
Track *FindObjDragReplace::get_drag_track()
{
	return plugin->server->plugin->track;
}
int64_t FindObjDragReplace::get_drag_position()
{
	return plugin->get_source_position();
}
void FindObjDragReplace::update_gui()
{
	bound();
	Track *track = get_drag_track();
	int trk_w = track->track_w, trk_h = track->track_h;
	float ctr_x = drag_x + drag_w/2, ctr_y = drag_y + drag_h/2;
	gui->replace_x->update( plugin->config.replace_x = 100. * ctr_x / trk_w );
	gui->replace_y->update( plugin->config.replace_y = 100. * ctr_y / trk_h );
	gui->replace_w->update( plugin->config.replace_w = 100. * drag_w / trk_w );
	gui->replace_h->update( plugin->config.replace_h = 100. * drag_h / trk_h );
	plugin->send_configure_change();
}



FindObjAlgorithm::FindObjAlgorithm(FindObjMain *plugin, FindObjWindow *gui, int x, int y)
 : BC_PopupMenu(x, y, calculate_w(gui), to_text(plugin->config.algorithm))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FindObjAlgorithm::handle_event()
{
	plugin->config.algorithm = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}

void FindObjAlgorithm::create_objects()
{
	add_item(new BC_MenuItem(to_text(NO_ALGORITHM)));
#ifdef _SIFT
	add_item(new BC_MenuItem(to_text(ALGORITHM_SIFT)));
#endif
#ifdef _SURF
	add_item(new BC_MenuItem(to_text(ALGORITHM_SURF)));
#endif
#ifdef _ORB
	add_item(new BC_MenuItem(to_text(ALGORITHM_ORB)));
#endif
#ifdef _AKAZE
	add_item(new BC_MenuItem(to_text(ALGORITHM_AKAZE)));
#endif
#ifdef _BRISK
	add_item(new BC_MenuItem(to_text(ALGORITHM_BRISK)));
#endif
}

int FindObjAlgorithm::from_text(char *text)
{
#ifdef _SIFT
	if(!strcmp(text, _("SIFT"))) return ALGORITHM_SIFT;
#endif
#ifdef _SURF
	if(!strcmp(text, _("SURF"))) return ALGORITHM_SURF;
#endif
#ifdef _ORB
	if(!strcmp(text, _("ORB"))) return ALGORITHM_ORB;
#endif
#ifdef _AKAZE
	if(!strcmp(text, _("AKAZE"))) return ALGORITHM_AKAZE;
#endif
#ifdef _BRISK
	if(!strcmp(text, _("BRISK"))) return ALGORITHM_BRISK;
#endif
	return NO_ALGORITHM;
}

void FindObjAlgorithm::update(int mode)
{
	set_text(to_text(mode));
}

char* FindObjAlgorithm::to_text(int mode)
{
	switch( mode ) {
#ifdef _SIFT
	case ALGORITHM_SIFT:	return _("SIFT");
#endif
#ifdef _SURF
	case ALGORITHM_SURF:	return _("SURF");
#endif
#ifdef _ORB
	case ALGORITHM_ORB:	return _("ORB");
#endif
#ifdef _AKAZE
	case ALGORITHM_AKAZE:	return _("AKAZE");
#endif
#ifdef _BRISK
	case ALGORITHM_BRISK:	return _("BRISK");
#endif
	}
	return _("Don't Calculate");
}

int FindObjAlgorithm::calculate_w(FindObjWindow *gui)
{
	int result = 0;
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(NO_ALGORITHM)));
#ifdef _SIFT
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(ALGORITHM_SIFT)));
#endif
#ifdef _SURF
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(ALGORITHM_SURF)));
#endif
#ifdef _ORB
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(ALGORITHM_ORB)));
#endif
#ifdef _AKAZE
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(ALGORITHM_AKAZE)));
#endif
#ifdef _BRISK
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(ALGORITHM_BRISK)));
#endif
	return result + 50;
}


FindObjUseFlann::FindObjUseFlann(FindObjMain *plugin, FindObjWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.use_flann, _("Use FLANN"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjUseFlann::handle_event()
{
	plugin->config.use_flann = get_value();
	plugin->send_configure_change();
	return 1;
}

FindObjDrawMatch::FindObjDrawMatch(FindObjMain *plugin, FindObjWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.draw_match, _("Draw match"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjDrawMatch::handle_event()
{
	plugin->config.draw_match = get_value();
	plugin->send_configure_change();
	return 1;
}

FindObjAspect::FindObjAspect(FindObjMain *plugin, FindObjWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.aspect, _("Aspect"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjAspect::handle_event()
{
	plugin->config.aspect = get_value();
	plugin->send_configure_change();
	return 1;
}

FindObjScale::FindObjScale(FindObjMain *plugin, FindObjWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.scale, _("Scale"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjScale::handle_event()
{
	plugin->config.scale = get_value();
	plugin->send_configure_change();
	return 1;
}

FindObjRotate::FindObjRotate(FindObjMain *plugin, FindObjWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.rotate, _("Rotate"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjRotate::handle_event()
{
	plugin->config.rotate = get_value();
	plugin->send_configure_change();
	return 1;
}

FindObjTranslate::FindObjTranslate(FindObjMain *plugin, FindObjWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.translate, _("Translate"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjTranslate::handle_event()
{
	plugin->config.translate = get_value();
	plugin->send_configure_change();
	return 1;
}

FindObjMode::FindObjMode(FindObjMain *plugin, FindObjWindow *gui, int x, int y)
 : BC_PopupMenu(x, y, calculate_w(gui), to_text(plugin->config.mode))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FindObjMode::handle_event()
{
	plugin->config.mode = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}

void FindObjMode::create_objects()
{
	add_item(new BC_MenuItem(to_text(MODE_SQUARE)));
	add_item(new BC_MenuItem(to_text(MODE_RHOMBUS)));
	add_item(new BC_MenuItem(to_text(MODE_RECTANGLE)));
	add_item(new BC_MenuItem(to_text(MODE_PARALLELOGRAM)));
	add_item(new BC_MenuItem(to_text(MODE_QUADRILATERAL)));
}
int FindObjMode::from_text(char *text)
{
	if(!strcmp(text, _("Square"))) return MODE_SQUARE;
	if(!strcmp(text, _("Rhombus"))) return MODE_RHOMBUS;
	if(!strcmp(text, _("Rectangle"))) return MODE_RECTANGLE;
	if(!strcmp(text, _("Parallelogram"))) return MODE_PARALLELOGRAM;
	if(!strcmp(text, _("Quadrilateral"))) return MODE_QUADRILATERAL;
	return MODE_NONE;
}

void FindObjMode::update(int mode)
{
	set_text(to_text(mode));
}

char* FindObjMode::to_text(int mode)
{
	switch( mode ) {
	case MODE_SQUARE: return _("Square");
	case MODE_RHOMBUS: return _("Rhombus");
	case MODE_RECTANGLE: return _("Rectangle");
	case MODE_PARALLELOGRAM: return _("Parallelogram");
	case MODE_QUADRILATERAL: return _("Quadrilateral");
	}
	return _("None");
}

int FindObjMode::calculate_w(FindObjWindow *gui)
{
	int result = 0;
	for( int mode=MODE_NONE; mode<MODE_MAX; ++mode )
		result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(mode)));
	return result + 50;
}


FindObjLayer::FindObjLayer(FindObjMain *plugin, FindObjWindow *gui,
	int x, int y, int *value)
 : BC_TumbleTextBox(gui, *value, MIN_LAYER, MAX_LAYER, x, y, calculate_w(gui))
{
	this->plugin = plugin;
	this->gui = gui;
	this->value = value;
}

int FindObjLayer::handle_event()
{
	*value = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}

int FindObjLayer::calculate_w(FindObjWindow *gui)
{
	int result = 0;
	result = gui->get_text_width(MEDIUMFONT, "000");
	return result + 50;
}


FindObjBlend::FindObjBlend(FindObjMain *plugin,
	int x,
	int y,
	int *value)
 : BC_IPot(x,
		y,
		(int64_t)*value,
		(int64_t)MIN_BLEND,
		(int64_t)MAX_BLEND)
{
	this->plugin = plugin;
	this->value = value;
}

int FindObjBlend::handle_event()
{
	*value = (int)get_value();
	plugin->send_configure_change();
	return 1;
}

