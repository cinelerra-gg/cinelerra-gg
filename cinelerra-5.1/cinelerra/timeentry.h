
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

#ifndef TIMEENTRY_H
#define TIMEENTRY_H


class DayText;
class DayTumbler;
class SecTumbler;
class TimeTextBox;
#define TOTAL_DAYS 8


#include "guicast.h"
#include "timeentry.inc"
#include "units.h"

// 3 part entry widget.
// part 1: day of the week
// part 2: day tumbler
// part 3: time of day
// Used by the Record GUI, Batch Rendering.

class TimeEntry
{
public:
	TimeEntry(BC_WindowBase *gui, int x, int y,
		int *output_day, double *output_time, int time_format,
		int time_w=DEFAULT_TIMEW);
	virtual ~TimeEntry();

	void create_objects();
	void time_to_hours(char *result, double time);
	void time_to_minutes(char *result, double time);
	void time_to_seconds(char *result, double time);
	virtual int handle_event();
	static int day_to_int(const char *day);
	void update(int *day, double *time);
	void update_time(double time);
	void update_day(int day);
	void reposition_window(int x, int y);
	void enable();
	void disable();
	int get_h();
	int get_w();
	double get_time() { return *output_time; }
	int get_day() { return *output_day; }

	BC_WindowBase *gui;
	int enabled;
	int x, y;
	int time_w;
	DayText *day_text;
	DayTumbler *day_tumbler;
	TimeTextBox *time_text;
	static const char *day_table[TOTAL_DAYS];
	int *output_day;
	double *output_time;
	int time_format;
};

class DayText : public BC_TextBox
{
public:
	DayText(TimeEntry *timeentry, int x, int y, int w,
		const char **table, int table_items, const char *text);
	int handle_event();

	const char **table;
	TimeEntry *timeentry;
	int table_items;
	int current_item;
};

class DayTumbler : public BC_Tumbler
{
public:
	DayTumbler(TimeEntry *timeentry, DayText *text, int x, int y);

	int handle_up_event();
	int handle_down_event();

	TimeEntry *timeentry;
	DayText *text;
};


class TimeEntryTumbler : public TimeEntry
{
public:
	int time_w;
	int incr;
	SecTumbler *sec_tumbler;

	void create_objects();
	void reposition_window(int x, int y);
	virtual int handle_up_event();
	virtual int handle_down_event();
	int get_h();
	int get_w();
	int get_incr() { return incr; }
	void update_incr(int i) { incr = i; }

	TimeEntryTumbler(BC_WindowBase *gui, int x, int y,
		int *output_day, double *output_time, int incr=1,
		int time_format=TIME_MS2, int time_w=DEFAULT_TIMEW);
	~TimeEntryTumbler();
};

class SecTumbler : public BC_Tumbler
{
public:
	SecTumbler(TimeEntryTumbler *timetumbler, int x, int y);

	int handle_up_event();
	int handle_down_event();

	TimeEntryTumbler *timetumbler;
};


class TimeTextBox : public BC_TextBox
{
public:
	TimeTextBox(TimeEntry *timeentry,
		int x, int y, int w, char *default_text);
	int handle_event();
	TimeEntry *timeentry;
};


#endif
