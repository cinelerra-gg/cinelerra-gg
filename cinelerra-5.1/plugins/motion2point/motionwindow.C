
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

#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clip.h"
#include "language.h"
#include "motion.h"
#include "motionscan-hv.h"
#include "motionwindow.h"










MotionWindow::MotionWindow(MotionMain2 *plugin)
 : PluginClientWindow(plugin, 680, 660, 680, 660, 0)
{
	this->plugin = plugin;
}

MotionWindow::~MotionWindow()
{
}

void MotionWindow::create_objects()
{
	int x1[] = { 10, get_w()/2 };
	int x = 10, y = 10, y1 = 10;
	BC_Title *title;


	for(int i = 0; i < TOTAL_POINTS; i++)
	{
		y = y1;
		char *global_title[] =
		{
			_("Track Point 1"),
			_("Track Point 2"),
		};
		add_subwindow(global[i] = new MotionGlobal(plugin,
			this,
			x1[i],
			y,
			&plugin->config.global[i],
			global_title[i]));
		y += 50;

		add_subwindow(title = new BC_Title(x1[i],
			y,
			_("Translation search radius:\n(W/H Percent of image)")));
		add_subwindow(global_range_w[i] = new MotionPot(plugin,
			x1[i] + title->get_w() + 10,
			y,
			&plugin->config.global_range_w[i],
			MIN_RADIUS,
			MAX_RADIUS));
		add_subwindow(global_range_h[i] = new MotionPot(plugin,
			x1[i] + title->get_w() + 10 + global_range_w[i]->get_w(),
			y,
			&plugin->config.global_range_h[i],
			MIN_RADIUS,
			MAX_RADIUS));

		y += 50;

		add_subwindow(title = new BC_Title(x1[i],
			y,
			_("Translation search offset:\n(X/Y Percent of image)")));
		add_subwindow(global_origin_x[i] = new MotionPot(plugin,
			x1[i] + title->get_w() + 10,
			y,
			&plugin->config.global_origin_x[i],
			MIN_ORIGIN,
			MAX_ORIGIN));
		add_subwindow(global_origin_y[i] = new MotionPot(plugin,
			x1[i] + title->get_w() + 10 + global_origin_x[i]->get_w(),
			y,
			&plugin->config.global_origin_y[i],
			MIN_ORIGIN,
			MAX_ORIGIN));

		y += 50;
		add_subwindow(title = new BC_Title(x1[i],
			y,
			_("Translation block size:\n(W/H Percent of image)")));
		add_subwindow(global_block_w[i] = new MotionPot(plugin,
			x1[i] + title->get_w() + 10,
			y,
			&plugin->config.global_block_w[i],
			MIN_BLOCK,
			MAX_BLOCK));
		add_subwindow(global_block_h[i] = new MotionPot(plugin,
			x1[i] + title->get_w() + 10 + global_block_w[i]->get_w(),
			y,
			&plugin->config.global_block_h[i],
			MIN_BLOCK,
			MAX_BLOCK));


		y += 40;
		add_subwindow(title = new BC_Title(x1[i], y + 10, _("Block X:")));
		add_subwindow(block_x[i] = new MotionBlockX(plugin,
			this,
			x1[i] + title->get_w() + 10,
			y,
			i));
		add_subwindow(block_x_text[i] = new MotionBlockXText(plugin,
			this,
			x1[i] + title->get_w() + 10 + block_x[i]->get_w() + 10,
			y + 10,
			i));

		y += 40;
		add_subwindow(title = new BC_Title(x1[i], y + 10, _("Block Y:")));
		add_subwindow(block_y[i] = new MotionBlockY(plugin,
			this,
			x1[i] + title->get_w() + 10,
			y,
			i));
		add_subwindow(block_y_text[i] = new MotionBlockYText(plugin,
			this,
			x1[i] + title->get_w() + 10 + block_y[i]->get_w() + 10,
			y + 10,
			i));


		y += 40;
		add_subwindow(vectors[i] = new MotionDrawVectors(plugin,
			this,
			x1[i],
			y,
			i));
	}


	y += 30;
	add_subwindow(new BC_Bar(x, y, get_w() - x * 2));
	y += 10;
	add_subwindow(title = new BC_Title(x, y, _("Search steps:")));
	add_subwindow(global_search_positions = new GlobalSearchPositions(plugin,
		x + title->get_w() + 10,
		y,
		80));
	global_search_positions->create_objects();

	y += 30;
	add_subwindow(title = new BC_Title(x, y, _("Search directions:")));
	add_subwindow(tracking_direction = new TrackingDirection(plugin,
		this,
		x + title->get_w() + 10,
		y));
	tracking_direction->create_objects();

	y += 30;
	add_subwindow(title = new BC_Title(x, y + 10, _("Maximum absolute offset:")));
	add_subwindow(magnitude = new MotionMagnitude(plugin,
		x + title->get_w() + 10,
		y));

	y += 40;
	add_subwindow(title = new BC_Title(x, y + 10, _("Settling speed:")));
	add_subwindow(return_speed = new MotionReturnSpeed(plugin,
		x + title->get_w() + 10,
		y));


	y += 40;
	add_subwindow(track_single = new TrackSingleFrame(plugin,
		this,
		x,
		y));
	add_subwindow(title = new BC_Title(x + track_single->get_w() + 20,
		y,
		_("Frame number:")));
	add_subwindow(track_frame_number = new TrackFrameNumber(plugin,
		this,
		x + track_single->get_w() + title->get_w() + 20,
		y));
	if(plugin->config.tracking_object != MotionScan::TRACK_SINGLE)
		track_frame_number->disable();

	y += 20;
	add_subwindow(track_previous = new TrackPreviousFrame(plugin,
		this,
		x,
		y));

	y += 20;
	add_subwindow(previous_same = new PreviousFrameSameBlock(plugin,
		this,
		x,
		y));

	y += 40;
	y1 = y;
	add_subwindow(title = new BC_Title(x, y, _("Master layer:")));
	add_subwindow(master_layer = new MasterLayer(plugin,
		this,
		x + title->get_w() + 10,
		y));
	master_layer->create_objects();
	y += 30;


	add_subwindow(title = new BC_Title(x, y, _("Action:")));
	add_subwindow(action = new Action(plugin,
		this,
		x + title->get_w() + 10,
		y));
	action->create_objects();
	y += 30;




	add_subwindow(title = new BC_Title(x, y, _("Calculation:")));
	add_subwindow(calculation = new Calculation(plugin,
		this,
		x + title->get_w() + 10,
		y));
	calculation->create_objects();



	show_window();
	flush();
}

void MotionWindow::update_mode()
{
	for(int i = 0; i < TOTAL_POINTS; i++)
	{
		global_range_w[i]->update(plugin->config.global_range_w[i],
	 		MIN_RADIUS,
	 		MAX_RADIUS);
		global_range_h[i]->update(plugin->config.global_range_h[i],
	 		MIN_RADIUS,
	 		MAX_RADIUS);
		vectors[i]->update(plugin->config.draw_vectors[i]);
			global[i]->update(plugin->config.global[i]);
	}
}













MotionGlobal::MotionGlobal(MotionMain2 *plugin,
	MotionWindow *gui,
	int x,
	int y,
	int *value,
	char *text)
 : BC_CheckBox(x,
 	y,
	*value,
	text)
{
	this->plugin = plugin;
	this->gui = gui;
	this->value = value;
}

int MotionGlobal::handle_event()
{
	*value = get_value();
	plugin->send_configure_change();
	return 1;
}


MotionPot::MotionPot(MotionMain2 *plugin,
	int x,
	int y,
	int *value,
	int min,
	int max)
 : BC_IPot(x,
		y,
		(int64_t)*value,
		(int64_t)min,
		(int64_t)max)
{
	this->plugin = plugin;
	this->value = value;
}


int MotionPot::handle_event()
{
	*value = (int)get_value();
	plugin->send_configure_change();
	return 1;
}












GlobalSearchPositions::GlobalSearchPositions(MotionMain2 *plugin,
	int x,
	int y,
	int w)
 : BC_PopupMenu(x,
 	y,
	w,
	"",
	1)
{
	this->plugin = plugin;
}
void GlobalSearchPositions::create_objects()
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
	add_item(new BC_MenuItem("65536"));
	add_item(new BC_MenuItem("131072"));
	char string[BCTEXTLEN];
	sprintf(string, "%d", plugin->config.global_positions);
	set_text(string);
}

int GlobalSearchPositions::handle_event()
{
	plugin->config.global_positions = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}












MotionMagnitude::MotionMagnitude(MotionMain2 *plugin,
	int x,
	int y)
 : BC_IPot(x,
		y,
		(int64_t)plugin->config.magnitude,
		(int64_t)0,
		(int64_t)100)
{
	this->plugin = plugin;
}

int MotionMagnitude::handle_event()
{
	plugin->config.magnitude = (int)get_value();
	plugin->send_configure_change();
	return 1;
}


MotionReturnSpeed::MotionReturnSpeed(MotionMain2 *plugin,
	int x,
	int y)
 : BC_IPot(x,
		y,
		(int64_t)plugin->config.return_speed,
		(int64_t)0,
		(int64_t)100)
{
	this->plugin = plugin;
}

int MotionReturnSpeed::handle_event()
{
	plugin->config.return_speed = (int)get_value();
	plugin->send_configure_change();
	return 1;
}





MotionBlockX::MotionBlockX(MotionMain2 *plugin,
	MotionWindow *gui,
	int x,
	int y,
	int number)
 : BC_FPot(x,
 	y,
	(float)plugin->config.block_x[number],
	(float)0,
	(float)100)
{
	this->plugin = plugin;
	this->gui = gui;
	this->number = number;
}

int MotionBlockX::handle_event()
{
	plugin->config.block_x[number] = get_value();
	gui->block_x_text[number]->update((float)plugin->config.block_x[number]);
	plugin->send_configure_change();
	return 1;
}




MotionBlockY::MotionBlockY(MotionMain2 *plugin,
	MotionWindow *gui,
	int x,
	int y,
	int number)
 : BC_FPot(x,
 	y,
	(float)plugin->config.block_y[number],
	(float)0,
	(float)100)
{
	this->plugin = plugin;
	this->gui = gui;
	this->number = number;
}

int MotionBlockY::handle_event()
{
	plugin->config.block_y[number] = get_value();
	gui->block_y_text[number]->update((float)plugin->config.block_y[number]);
	plugin->send_configure_change();
	return 1;
}

MotionBlockXText::MotionBlockXText(MotionMain2 *plugin,
	MotionWindow *gui,
	int x,
	int y,
	int number)
 : BC_TextBox(x,
 	y,
	75,
	1,
	(float)plugin->config.block_x[number])
{
	this->plugin = plugin;
	this->gui = gui;
	this->number = number;
	set_precision(4);
}

int MotionBlockXText::handle_event()
{
	plugin->config.block_x[number] = atof(get_text());
	gui->block_x[number]->update(plugin->config.block_x[number]);
	plugin->send_configure_change();
	return 1;
}




MotionBlockYText::MotionBlockYText(MotionMain2 *plugin,
	MotionWindow *gui,
	int x,
	int y,
	int number)
 : BC_TextBox(x,
 	y,
	75,
	1,
	(float)plugin->config.block_y[number])
{
	this->plugin = plugin;
	this->gui = gui;
	this->number = number;
	set_precision(4);
}

int MotionBlockYText::handle_event()
{
	plugin->config.block_y[number] = atof(get_text());
	gui->block_y[number]->update(plugin->config.block_y[number]);
	plugin->send_configure_change();
	return 1;
}
















MotionDrawVectors::MotionDrawVectors(MotionMain2 *plugin,
	MotionWindow *gui,
	int x,
	int y,
	int number)
 : BC_CheckBox(x,
 	y,
	plugin->config.draw_vectors[number],
	_("Draw vectors"))
{
	this->gui = gui;
	this->plugin = plugin;
	this->number = number;
}

int MotionDrawVectors::handle_event()
{
	plugin->config.draw_vectors[number] = get_value();
	plugin->send_configure_change();
	return 1;
}








TrackSingleFrame::TrackSingleFrame(MotionMain2 *plugin,
	MotionWindow *gui,
	int x,
	int y)
 : BC_Radial(x,
 	y,
	plugin->config.tracking_object == MotionScan::TRACK_SINGLE,
 	_("Track single frame"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int TrackSingleFrame::handle_event()
{
	plugin->config.tracking_object = MotionScan::TRACK_SINGLE;
	gui->track_previous->update(0);
	gui->previous_same->update(0);
	gui->track_frame_number->enable();
	plugin->send_configure_change();
	return 1;
}








TrackFrameNumber::TrackFrameNumber(MotionMain2 *plugin,
	MotionWindow *gui,
	int x,
	int y)
 : BC_TextBox(x, y, 100, 1, plugin->config.track_frame)
{
	this->plugin = plugin;
	this->gui = gui;
}

int TrackFrameNumber::handle_event()
{
	plugin->config.track_frame = atol(get_text());
	plugin->send_configure_change();
	return 1;
}







TrackPreviousFrame::TrackPreviousFrame(MotionMain2 *plugin,
	MotionWindow *gui,
	int x,
	int y)
 : BC_Radial(x,
 	y,
	plugin->config.tracking_object == MotionScan::TRACK_PREVIOUS,
	_("Track previous frame"))
{
	this->plugin = plugin;
	this->gui = gui;
}
int TrackPreviousFrame::handle_event()
{
	plugin->config.tracking_object = MotionScan::TRACK_PREVIOUS;
	gui->track_single->update(0);
	gui->previous_same->update(0);
	gui->track_frame_number->disable();
	plugin->send_configure_change();
	return 1;
}








PreviousFrameSameBlock::PreviousFrameSameBlock(MotionMain2 *plugin,
	MotionWindow *gui,
	int x,
	int y)
 : BC_Radial(x,
 	y,
	plugin->config.tracking_object == MotionScan::PREVIOUS_SAME_BLOCK,
	_("Previous frame same block"))
{
	this->plugin = plugin;
	this->gui = gui;
}
int PreviousFrameSameBlock::handle_event()
{
	plugin->config.tracking_object = MotionScan::PREVIOUS_SAME_BLOCK;
	gui->track_single->update(0);
	gui->track_previous->update(0);
	gui->track_frame_number->disable();
	plugin->send_configure_change();
	return 1;
}








MasterLayer::MasterLayer(MotionMain2 *plugin, MotionWindow *gui, int x, int y)
 : BC_PopupMenu(x,
 	y,
	calculate_w(gui),
	to_text(plugin->config.bottom_is_master))
{
	this->plugin = plugin;
	this->gui = gui;
}

int MasterLayer::handle_event()
{
	plugin->config.bottom_is_master = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}

void MasterLayer::create_objects()
{
	add_item(new BC_MenuItem(to_text(0)));
	add_item(new BC_MenuItem(to_text(1)));
}

int MasterLayer::from_text(char *text)
{
	if(!strcmp(text, _("Top"))) return 0;
	return 1;
}

char* MasterLayer::to_text(int mode)
{
	return mode ? _("Bottom") : _("Top");
}

int MasterLayer::calculate_w(MotionWindow *gui)
{
	int result = 0;
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(0)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(1)));
	return result + 50;
}








Action::Action(MotionMain2 *plugin, MotionWindow *gui, int x, int y)
 : BC_PopupMenu(x,
 	y,
	calculate_w(gui),
	to_text(plugin->config.action))
{
	this->plugin = plugin;
	this->gui = gui;
}

int Action::handle_event()
{
	plugin->config.action = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}

void Action::create_objects()
{
	add_item(new BC_MenuItem(to_text(MotionScan::TRACK)));
	add_item(new BC_MenuItem(to_text(MotionScan::TRACK_PIXEL)));
	add_item(new BC_MenuItem(to_text(MotionScan::STABILIZE)));
	add_item(new BC_MenuItem(to_text(MotionScan::STABILIZE_PIXEL)));
	add_item(new BC_MenuItem(to_text(MotionScan::NOTHING)));
}

int Action::from_text(char *text)
{
	if(!strcmp(text, _("Track"))) return MotionScan::TRACK;
	if(!strcmp(text, _("Track Pixel"))) return MotionScan::TRACK_PIXEL;
	if(!strcmp(text, _("Stabilize"))) return MotionScan::STABILIZE;
	if(!strcmp(text, _("Stabilize Pixel"))) return MotionScan::STABILIZE_PIXEL;
	if(!strcmp(text, _("Do Nothing"))) return MotionScan::NOTHING;
	return 0;
}

char* Action::to_text(int mode)
{
	switch(mode)
	{
		case MotionScan::TRACK:
			return _("Track");
			break;
		case MotionScan::TRACK_PIXEL:
			return _("Track Pixel");
			break;
		case MotionScan::STABILIZE:
			return _("Stabilize");
			break;
		case MotionScan::STABILIZE_PIXEL:
			return _("Stabilize Pixel");
			break;
		case MotionScan::NOTHING:
			return _("Do Nothing");
			break;
		default:
			return _("Unknown");
			break;
	}
}

int Action::calculate_w(MotionWindow *gui)
{
	int result = 0;
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionScan::TRACK)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionScan::TRACK_PIXEL)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionScan::STABILIZE)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionScan::STABILIZE_PIXEL)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionScan::NOTHING)));
	return result + 50;
}





Calculation::Calculation(MotionMain2 *plugin, MotionWindow *gui, int x, int y)
 : BC_PopupMenu(x,
 	y,
	calculate_w(gui),
	to_text(plugin->config.calculation))
{
	this->plugin = plugin;
	this->gui = gui;
}

int Calculation::handle_event()
{
	plugin->config.calculation = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}

void Calculation::create_objects()
{
	add_item(new BC_MenuItem(to_text(MotionScan::NO_CALCULATE)));
	add_item(new BC_MenuItem(to_text(MotionScan::CALCULATE)));
	add_item(new BC_MenuItem(to_text(MotionScan::SAVE)));
	add_item(new BC_MenuItem(to_text(MotionScan::LOAD)));
}

int Calculation::from_text(char *text)
{
	if(!strcmp(text, _("Don't Calculate"))) return MotionScan::NO_CALCULATE;
	if(!strcmp(text, _("Recalculate"))) return MotionScan::CALCULATE;
	if(!strcmp(text, _("Save coords to /tmp"))) return MotionScan::SAVE;
	if(!strcmp(text, _("Load coords from /tmp"))) return MotionScan::LOAD;
	return 0;
}

char* Calculation::to_text(int mode)
{
	switch(mode)
	{
		case MotionScan::NO_CALCULATE:
			return _("Don't Calculate");
			break;
		case MotionScan::CALCULATE:
			return _("Recalculate");
			break;
		case MotionScan::SAVE:
			return _("Save coords to /tmp");
			break;
		case MotionScan::LOAD:
			return _("Load coords from /tmp");
			break;
		default:
			return _("Unknown");
			break;
	}
}

int Calculation::calculate_w(MotionWindow *gui)
{
	int result = 0;
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionScan::NO_CALCULATE)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionScan::CALCULATE)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionScan::SAVE)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionScan::LOAD)));
	return result + 50;
}










TrackingDirection::TrackingDirection(MotionMain2 *plugin, MotionWindow *gui, int x, int y)
 : BC_PopupMenu(x,
 	y,
	calculate_w(gui),
	to_text(plugin->config.horizontal_only, plugin->config.vertical_only))
{
	this->plugin = plugin;
	this->gui = gui;
}

int TrackingDirection::handle_event()
{
	from_text(&plugin->config.horizontal_only, &plugin->config.vertical_only, get_text());
	plugin->send_configure_change();
	return 1;
}

void TrackingDirection::create_objects()
{
	add_item(new BC_MenuItem(to_text(1, 0)));
	add_item(new BC_MenuItem(to_text(0, 1)));
	add_item(new BC_MenuItem(to_text(0, 0)));
}

void TrackingDirection::from_text(int *horizontal_only, int *vertical_only, char *text)
{
	*horizontal_only = 0;
	*vertical_only = 0;
	if(!strcmp(text, to_text(1, 0))) *horizontal_only = 1;
	if(!strcmp(text, to_text(0, 1))) *vertical_only = 1;
}

char* TrackingDirection::to_text(int horizontal_only, int vertical_only)
{
	if(horizontal_only) return _("Horizontal only");
	if(vertical_only) return _("Vertical only");
	return _("Both");
}

int TrackingDirection::calculate_w(MotionWindow *gui)
{
	int result = 0;
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(1, 0)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(0, 1)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(0, 0)));
	return result + 50;
}

