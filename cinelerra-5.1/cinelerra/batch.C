
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

#include "asset.h"
#include "batch.h"
#include "channel.h"
#include "channeldb.h"
#include "edl.h"
#include "filesystem.h"
#include "record.h"
#include "recordlabel.h"
#include <string.h>

Batch::Batch(MWindow *mwindow, Record *record)
{
	this->mwindow = mwindow;
	this->record = record;
	asset = new Asset;
	labels = new RecordLabels;
	record_mode = RECORD_UNTIMED;
	recorded = 0;
	channel = 0;
	enabled = 1;
	file_exists = 0;
	start_time = last_start_time = 0.;
	duration = record_duration = 0.;
	start_day = 0; last_start_day = -1;
	time_start = 0;
	time_end = 0;
	notice = 0;
	news[0] = 0;
}

Batch::~Batch()
{
	asset->Garbage::remove_user();
	delete labels;
}

void Batch::create_objects()
{
}

void Batch::clear_labels()
{
	while(labels->last) delete labels->last;
}

void Batch::start_over()
{
	clear_labels();
}

void Batch::copy_from(Batch *batch)
{
	record_mode = batch->record_mode;
	channel = batch->channel;
	enabled = batch->enabled;
	start_time = batch->start_time;
	duration = batch->duration;
	record_duration = batch->record_duration;
	start_day = batch->start_day;
	last_start_day = -1;
}


void Batch::calculate_news()
{
	if( notice ) sprintf(news, "%s", notice);
	else if( record && record->get_current_batch() == this ) {
		FileSystem fs;  char text[BCTEXTLEN];
		int64_t bytes = fs.get_size(asset->path);
		if( bytes >= 0 ) {
			Units::size_totext(bytes, text);
			const char *stat = record->file ?  _("Open") :
				!this->enabled ? _("Done") : _("Ok");
			sprintf(news,"%s %s", text, stat);
		}
		else
			sprintf(news,"%s", _("New file"));
	}
	else {
		sprintf(news, "%s", !access(asset->path, F_OK) ?
			 _("Exists") : _("New file"));
	}
}

void Batch::create_default_path()
{
	char string[BCTEXTLEN];
	strcpy(string, record->default_asset->path);
	char *path = asset->path;
	strcpy(path, record->default_asset->path);
	int i = 0, k = 0;
	while( path[i] ) {
		int ch = path[i++];
		if( ch == '/' ) k = i;
	}
	i = k;
	while( path[i] && (path[i]<'0' || path[i]>'9') ) ++i;
	int j = i;
	while( path[i] && (path[i]>='0' || path[i]<='9') ) ++i;
	int l = i;

	sprintf(&path[j], "%d", record->record_batches.total());
	strcat(path, &string[l]);
}


int Batch::text_to_mode(const char *text)
{
	if(!strcasecmp(mode_to_text(RECORD_UNTIMED), text)) return RECORD_UNTIMED;
	if(!strcasecmp(mode_to_text(RECORD_TIMED), text)) return RECORD_TIMED;
	return RECORD_UNTIMED;
}

const char* Batch::mode_to_text(int record_mode)
{
	switch( record_mode ) {
	case RECORD_UNTIMED: return _("Untimed");
	case RECORD_TIMED:   return _("Timed");
	}
	return _("Unknown");
}

Channel* Batch::get_current_channel()
{
	return channel;
}


const char* Batch::get_source_text()
{
	Channel *channel = get_current_channel();
	return channel ? channel->title : "";
}

void Batch::toggle_label(double position)
{
	labels->toggle_label(position);
}

void Batch::update_times()
{
	if( start_time != last_start_time ||
		start_day != last_start_day ) {
		last_start_day = start_day;
		last_start_time = start_time;
		int64_t seconds = start_time;
		int hour = seconds/3600;
		int minute = seconds/60 - hour*60;
		int second = seconds - (hour*3600 + minute*60);
		struct timeval tv;  struct timezone tz;
		gettimeofday(&tv, &tz);  time_t t = tv.tv_sec;
		if( !strcmp(tzname[0],"UTC") ) t -= tz.tz_minuteswest*60;
		struct tm tm;  localtime_r(&t, &tm);
		if( start_day < 7 && start_day != tm.tm_wday ) {
			int days = start_day - tm.tm_wday;
			if( days < 0 ) days += 7;
			t += days * 7*24*3600;
			localtime_r(&t, &tm);
		}
		tm.tm_hour = hour;
		tm.tm_min = minute;
		tm.tm_sec = second;
		time_start = mktime(&tm);
	}
	time_end = time_start + (int)duration;
}

