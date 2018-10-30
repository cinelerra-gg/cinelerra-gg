
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

#include "timeentry.h"
#include "language.h"
#include <string.h>

TimeEntry::TimeEntry(BC_WindowBase *gui, int x, int y,
		int *output_day, double *output_time,
		int time_format, int time_w)
{
	this->gui = gui;
	this->x = x;
	this->y = y;
	this->output_day = output_day;
	this->output_time = output_time;
	this->time_format = time_format;
	this->time_w = time_w;
	enabled = 1;
	day_text = 0;
	day_tumbler = 0;
	time_text = 0;
}

TimeEntry::~TimeEntry()
{
	delete day_text;
	delete day_tumbler;
	delete time_text;
}
const char* TimeEntry::day_table[] =
{
	N_("Sun"), N_("Mon"), N_("Tue"), N_("Wed"), N_("Thu"), N_("Fri"), N_("Sat"), "*"
};

int TimeEntry::day_to_int(const char *day)
{
	for(int i = 0; i < TOTAL_DAYS; i++)
		if(!strcasecmp(day, _(day_table[i]))) return i;
	return 0;
}


void TimeEntry::time_to_hours(char *result, double time)
{
	int hour = (int)(time / 3600);
	sprintf(result, "%d", hour);
}

void TimeEntry::time_to_minutes(char *result, double time)
{
	int hour = (int)(time / 3600);
	int minute = (int)(time / 60 - hour * 60);
	sprintf(result, "%d", minute);
}

void TimeEntry::time_to_seconds(char *result, double time)
{
	int hour = (int)(time / 3600);
	int minute = (int)(time / 60 - hour * 60);
	float second = (float)time - (int64_t)hour * 3600 - (int64_t)minute * 60;
	sprintf(result, "%.3f", second);
}

void TimeEntry::create_objects()
{
	int x = this->x;
	int y = this->y;
	int time_w = this->time_w;
	char string[BCTEXTLEN];

	if(output_day) {
		day_text = new DayText(this, x, y, 50,
			day_table, TOTAL_DAYS, day_table[*output_day]);
		gui->add_subwindow(day_text);
		x += day_text->get_w();
		time_w -= day_text->get_w();
		day_tumbler = new DayTumbler(this, day_text, x, y);
		gui->add_subwindow(day_tumbler);
		x += day_tumbler->get_w();
		time_w -= day_tumbler->get_w();
	}

	time_text = new TimeTextBox(this, x, y, time_w,
		Units::totext(string, *output_time, time_format));
	gui->add_subwindow(time_text);

	time_text->set_separators(Units::format_to_separators(time_format));
}

void TimeEntry::enable()
{
	enabled = 1;
	if( day_text ) day_text->enable();
	time_text->enable();
}

void TimeEntry::disable()
{
	enabled = 0;
	if( day_text ) day_text->disable();
	time_text->disable();
}

int TimeEntry::get_h()
{
	return time_text->get_h();
}

int TimeEntry::get_w()
{
	int w = 0;
	if(day_text) w += day_text->get_w();
	if(day_tumbler) w += day_tumbler->get_w();
	w += time_text->get_w();
	return w;
}

void TimeEntry::reposition_window(int x, int y)
{
	int time_w = this->time_w;
	this->x = x;
	this->y = y;

	if(day_text) {
		day_text->reposition_window(x, y);
		x += day_text->get_w();
		time_w -= day_text->get_w();
	}
	if(day_tumbler) {
		day_tumbler->reposition_window(x, y);
		x += day_tumbler->get_w();
		time_w -= day_tumbler->get_w();
	}

	time_text->reposition_window(x, y, time_w);
}

void TimeEntry::update_day(int day)
{
	if( !output_day ) return;
	*output_day = day;
	day_text->update(day_table[day]);
}

void TimeEntry::update_time(double time)
{
	*output_time = time;
	char text[BCTEXTLEN];
	Units::totext(text, time, time_format);
	time_text->update(text);
}

void TimeEntry::update(int *day, double *time)
{
	output_day = day;
	output_time = time;

	if( day != 0 )
		day_text->update(day_table[*day]);

	char text[BCTEXTLEN];
	Units::totext(text, *time, time_format);
	time_text->update(text);
}

int TimeEntry::handle_event()
{
	return 1;
}



DayText::DayText(TimeEntry *timeentry,
		int x, int y, int w,
		const char **table, int table_items,
		const char *text)
 : BC_TextBox(x, y, w, 1, _(text))
{
	this->timeentry = timeentry;
	this->table = table;
	this->table_items = table_items;
}

int DayText::handle_event()
{
	*timeentry->output_day = TimeEntry::day_to_int(get_text());
	return timeentry->handle_event();
}


DayTumbler::DayTumbler(TimeEntry *timeentry, DayText *text, int x, int y)
 : BC_Tumbler(x, y)
{
	this->timeentry = timeentry;
	this->text = text;
}


int DayTumbler::handle_up_event()
{
	if( !timeentry->enabled ) return 1;
	*timeentry->output_day += 1;
	if(*timeentry->output_day >= text->table_items)
		*timeentry->output_day = 0;
	text->update(_(text->table[*timeentry->output_day]));
	return timeentry->handle_event();
}

int DayTumbler::handle_down_event()
{
	if( !timeentry->enabled ) return 1;
	*timeentry->output_day -= 1;
	if(*timeentry->output_day < 0)
		*timeentry->output_day = text->table_items - 1;
	text->update(_(text->table[*timeentry->output_day]));
	return timeentry->handle_event();
}



TimeEntryTumbler::TimeEntryTumbler(BC_WindowBase *gui, int x, int y,
		int *output_day, double *output_time, int incr,
		int time_format, int time_w)
 : TimeEntry(gui, x, y, output_day, output_time,
		 time_format, time_w-BC_Tumbler::calculate_w())
{
	this->time_w = time_w;
	this->incr = incr;
	sec_tumbler = 0;
}

TimeEntryTumbler::~TimeEntryTumbler()
{
	delete sec_tumbler;
}

void TimeEntryTumbler::create_objects()
{
	TimeEntry::create_objects();
	sec_tumbler = new SecTumbler(this, x+TimeEntry::time_w, y);
	gui->add_subwindow(sec_tumbler);
}

void TimeEntryTumbler::reposition_window(int x, int y)
{
	TimeEntry::reposition_window(x, y);
	sec_tumbler->reposition_window(x+TimeEntry::time_w, y);
}

int TimeEntryTumbler::get_h()
{
	return TimeEntry::get_h();
}

int TimeEntryTumbler::get_w()
{
	return TimeEntry::get_w() + BC_Tumbler::calculate_w();
}

int TimeEntryTumbler::handle_up_event()
{
	if( !enabled ) return 1;
	if( output_time ) {
		*output_time += incr;
		char text[BCTEXTLEN];
		Units::totext(text, *output_time, time_format);
		time_text->update(text);
	}
	return handle_event();
}

int TimeEntryTumbler::handle_down_event()
{
	if( !enabled ) return 1;
	if( output_time ) {
		*output_time -= incr;
		char text[BCTEXTLEN];
		Units::totext(text, *output_time, time_format);
		time_text->update(text);
	}
	return handle_event();
}


SecTumbler::SecTumbler(TimeEntryTumbler *timetumbler, int x, int y)
 : BC_Tumbler(x, y)
{
	this->timetumbler = timetumbler;
}

int SecTumbler::handle_up_event()
{
	return timetumbler->handle_up_event();
}

int SecTumbler::handle_down_event()
{
	return timetumbler->handle_down_event();
}



TimeTextBox::TimeTextBox(TimeEntry *timeentry,
		int x, int y, int w, char *default_text)
 : BC_TextBox(x, y, w, 1, default_text)
{
	this->timeentry = timeentry;
}

int TimeTextBox::handle_event()
{
	*timeentry->output_time = Units::text_to_seconds(get_text(),
				48000, timeentry->time_format);
	return timeentry->handle_event();
}


