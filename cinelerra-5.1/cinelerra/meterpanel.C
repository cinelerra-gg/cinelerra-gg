
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "preferences.h"
#include "theme.h"
#include "vframe.h"



MeterPanel::MeterPanel(MWindow *mwindow, BC_WindowBase *subwindow,
	int x, int y, int w, int h, int meter_count,
	int visible, int use_headroom, int use_titles)
{
	this->subwindow = subwindow;
	this->mwindow = mwindow;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	this->meter_count = meter_count;
	this->visible = visible;
	this->use_headroom = use_headroom;
	this->use_titles = use_titles;
}


MeterPanel::~MeterPanel()
{
	meters.remove_all_objects();
	meter_titles.remove_all_objects();
}

void MeterPanel::create_objects()
{
	set_meters(meter_count, visible);
}

int MeterPanel::set_meters(int meter_count, int visible)
{
	if(meter_count != meters.total || visible != this->visible)
	{
// Delete old meters
		if(meter_count != meters.total)
		{
			meters.remove_all_objects();
			meter_titles.remove_all_objects();
			meter_peaks.remove_all();
		}

		this->meter_count = meter_count;
		this->visible = visible;
//		if(!visible) this->meter_count = 0;

		if(meter_count && !meters.size())
		{
			int x1 = this->x;
			int y1 = this->y;
			int h1 = get_meter_h();
			int border = mwindow->theme->widget_border;
			for(int i = 0; i < meter_count; i++)
			{
//printf("MeterPanel::set_meters %d %d %d %d\n", __LINE__, i, x1, get_meter_w(i));
				y1 = this->y;

				if(use_titles)
				{
					BC_Title *new_title;
					subwindow->add_subwindow(new_title = new BC_Title(
						get_title_x(i, "0"),
						y1,
						"0",
						SMALLFONT));
					y1 += new_title->get_h() + border;
					meter_titles.append(new_title);
				}

				MeterMeter *new_meter;
				subwindow->add_subwindow(new_meter = new MeterMeter(mwindow,
					this,
					x1,
					y1,
					this->w > 0 ? get_meter_w(i) : -1,
					h1,
					(i == 0)));
				meters.append(new_meter);
        		x1 += get_meter_w(i) + border;

				meter_peaks.append(0);
			}
		}
	}

	return 0;
}

void MeterPanel::reposition_window(int x, int y, int w, int h)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;

//printf("MeterPanel::reposition_window 0 %d\n", meter_count);

	int border = mwindow->theme->widget_border;
	int x1 = x;
	for(int i = 0; i < meter_count; i++)
	{
		if(use_titles)
		{
			meter_titles.get(i)->reposition_window(
				get_title_x(i, meter_titles.get(i)->get_text()),
				meter_titles.get(i)->get_y());
		}
//printf("MeterPanel::reposition_window %d %d %d %d\n", __LINE__, i, x1, get_meter_w(i));
		meters.get(i)->reposition_window(x1,
			meters.get(i)->get_y(),
			get_meter_w(i),
			get_meter_h());
		x1 += get_meter_w(i) + border;
	}
}

int MeterPanel::change_status_event(int new_status)
{
//printf("MeterPanel::change_status_event\n");
	return 1;
}

int MeterPanel::get_reset_x()
{
	return x +
		get_meters_width(mwindow->theme, meter_count, visible) -
		mwindow->theme->over_button[0]->get_w();
}

int MeterPanel::get_reset_y()
{
	return y + h - mwindow->theme->over_button[0]->get_h();
}

int MeterPanel::get_title_x(int number, const char *text)
{
	int border = mwindow->theme->widget_border;
	int x1 = x;

	for(int i = 0; i < meter_count; i++)
	{
		if(i == number)
		{
			int text_w = subwindow->get_text_width(SMALLFONT, text);
			int title_x = x1;
			int meter_w = (i == 0) ?
				get_meter_w(i) - BC_Meter::get_title_w() :
				get_meter_w(i);
			title_x += (i == 0) ? BC_Meter::get_title_w() : 0;
			title_x += meter_w / 2 -
				text_w / 2;
			title_x = MAX(title_x, x1);
			if(i == 0) title_x = MAX(title_x, x1 + BC_Meter::get_title_w());
			return title_x;
		}


		x1 += get_meter_w(i) + border;
	}
	return -1;
}

int MeterPanel::get_meters_width(Theme *theme, int meter_count, int visible)
{
//printf("MeterPanel::get_meters_width %d %d\n", BC_Meter::get_title_w(), BC_Meter::get_meter_w());
	return visible ?
		(BC_Meter::get_title_w() +
			theme->widget_border +
			BC_Meter::get_meter_w() * meter_count +
			theme->widget_border * (meter_count - 1)) :
		0;
}


int MeterPanel::get_meter_w(int number)
{
	int border = mwindow->theme->widget_border;
	int meter_w;
	if( w > 0 ) {
		meter_w = (w - BC_Meter::get_title_w() -
				(meter_count - 1) * border) / meter_count;
		if( meter_w < 3 ) meter_w = 3;
	}
	else
		meter_w = BC_Meter::get_meter_w();
	if( !number )
		meter_w += BC_Meter::get_title_w();
	return meter_w;
}

int MeterPanel::get_meter_h()
{
	int border = mwindow->theme->widget_border;
	if(use_titles)
	{
		return h - border - subwindow->get_text_height(SMALLFONT);
	}
	else
	{
		return h;
	}
}

void MeterPanel::update(double *levels)
{
	if( !visible || subwindow->get_hidden()) return;


	for(int i = 0; i < meter_count; i++)
	{
		meters.values[i]->update(levels[i], levels[i] > 1.0);

		if(use_titles &&
			levels[i] > meter_peaks.get(i))
		{
			update_peak(i, levels[i]);
		}
	}
}

void MeterPanel::init_meters(int dmix)
{
	subwindow->lock_window("MeterPanel::init_meters");
	for(int i = 0; i < meter_count; i++)
	{
		meters.values[i]->reset(dmix);
	}
	subwindow->unlock_window();
}

void MeterPanel::update_peak(int number, float value)
{
	if(use_titles && number < meter_count)
	{
		meter_peaks.set(number, value);

		float db_value = DB::todb(value);
		char string[BCTEXTLEN] = { 0 };
		if(db_value <= mwindow->edl->session->min_meter_db)
		{
			sprintf(string, "oo");
		}
		else
		if(db_value > 0)
		{
			sprintf(string, "+%.1f",
				db_value);
		}
		else
		{
			sprintf(string, "%.1f",
				db_value);
		}

		meter_titles.get(number)->update(string);
		meter_titles.get(number)->reposition_window(get_title_x(number, string),
			meter_titles.get(number)->get_y());
	}
}

void MeterPanel::stop_meters()
{
	for(int i = 0; i < meter_count; i++)
	{
		meters.values[i]->reset();
// Reset peak without drawing
		meter_peaks.set(i, 0);
	}
}


void MeterPanel::reset_meters()
{
	for(int i = 0; i < meters.size(); i++)
		meters.values[i]->reset_over();

	for(int i = 0; i < meter_titles.size(); i++)
	{
		update_peak(i, 0);
	}
}


void MeterPanel::change_format(int mode, int min, int max)
{
	for(int i = 0; i < meters.total; i++)
	{
		if(use_headroom)
			meters.values[i]->change_format(mode, min, 0);
		else
			meters.values[i]->change_format(mode, min, max);
	}
}









MeterReset::MeterReset(MWindow *mwindow, MeterPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->over_button)
{
	this->mwindow = mwindow;
	this->panel = panel;
}

MeterReset::~MeterReset()
{
}

int MeterReset::handle_event()
{
	for(int i = 0; i < panel->meters.total; i++)
		panel->meters.values[i]->reset_over();
	return 1;
}





MeterMeter::MeterMeter(MWindow *mwindow,
	MeterPanel *panel, int x, int y, int w, int h, int titles)
 : BC_Meter(x, y, METER_VERT, h, mwindow->edl->session->min_meter_db,
	panel->use_headroom ? 0 : mwindow->edl->session->max_meter_db,
	mwindow->edl->session->meter_format, titles, w)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_delays(TRACKING_RATE * 10, TRACKING_RATE);
}

MeterMeter::~MeterMeter()
{
}


int MeterMeter::button_press_event()
{
	if(is_event_win() && BC_WindowBase::cursor_inside())
	{
		panel->reset_meters();
		mwindow->reset_meters();
		return 1;
	}

	return 0;
}





MeterShow::MeterShow(MWindow *mwindow, MeterPanel *panel, int x, int y)
 : BC_Toggle(x,
 		y,
		mwindow->theme->get_image_set("meters"),
		panel->visible)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Show meters"));
}


MeterShow::~MeterShow()
{
}


int MeterShow::handle_event()
{
//printf("MeterShow::MeterShow 1 %d\n",panel->visible );
// need to detect a change in visible, outside here
//	panel->visible = get_value();
	panel->change_status_event(get_value());
	return 1;
}
