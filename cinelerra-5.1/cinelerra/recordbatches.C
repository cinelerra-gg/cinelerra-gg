#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "asset.h"
#include "batch.h"
#include "bchash.h"
#include "bclistbox.h"
#include "channel.h"
#include "channeldb.h"
#include "filesystem.h"
#include "mwindow.h"
#include "record.h"
#include "recordbatches.h"
#include "timeentry.h"

const char * RecordBatches::
default_batch_titles[] = {
	N_("On"), N_("Path"), N_("News"), N_("Start time"),
	N_("Duration"), N_("Source"), N_("Mode")
};

const int RecordBatches::
default_columnwidth[] = {
	30, 200, 100, 100, 100, 100, 70
};

void RecordBatches::
load_defaults(ChannelDB * channeldb, Record * record)
{
	char string[BCTEXTLEN];
	BC_Hash *defaults = mwindow->defaults;
	defaults->get("RECORD_DEFAULT_DIRECTORY", default_directory);
	early_margin = defaults->get("RECORD_EARLY_MARGIN", early_margin);
	late_margin = defaults->get("RECORD_LATE_MARGIN", late_margin);
	for(int i = 0; i < BATCH_COLUMNS; i++) {
		batch_titles[i] = _(default_batch_titles[i]);
		sprintf(string, "BATCH_COLUMNWIDTH_%d", i);
		column_widths[i] = defaults->get(string, default_columnwidth[i]);
	}
	int total_batches = defaults->get("TOTAL_BATCHES", 1);
	if(total_batches < 1) total_batches = 1;
	for(int i = 0; i < total_batches; ++i) {
		Batch *batch = new Batch(mwindow, record);
		batch->create_objects();
		batches.append(batch);
		Asset *asset = batch->asset;
		sprintf(string, "RECORD_PATH_%d", i);
		defaults->get(string, asset->path);
		sprintf(string, "RECORD_CHANNEL_%d", i);
		int chan_no = channeldb->number_of(batch->channel);
		chan_no = defaults->get(string, chan_no);
		batch->channel = channeldb->get(chan_no);
		sprintf(string, "RECORD_STARTDAY_%d", i);
		batch->start_day = defaults->get(string, batch->start_day);
		sprintf(string, "RECORD_STARTTIME_%d", i);
		batch->start_time = defaults->get(string, batch->start_time);
		sprintf(string, "RECORD_DURATION_%d", i);
		batch->duration = defaults->get(string, batch->duration);
		sprintf(string, "RECORD_MODE_%d", i);
		batch->record_mode = defaults->get(string, batch->record_mode);
		sprintf(string, "BATCH_ENABLED_%d", i);
		batch->enabled = defaults->get(string, batch->enabled);
		batch->update_times();
	}

	current_item = editing_item = 0;
}

void RecordBatches::
save_defaults(ChannelDB * channeldb)
{
	char string[BCTEXTLEN];
	BC_Hash *defaults = mwindow->defaults;
	defaults->update("RECORD_DEFAULT_DIRECTORY", default_directory);
	defaults->update("RECORD_EARLY_MARGIN", early_margin);
	defaults->update("RECORD_LATE_MARGIN", late_margin);
	for(int i = 0; i < BATCH_COLUMNS; i++) {
		sprintf(string, "BATCH_COLUMNWIDTH_%d", i);
		defaults->update(string, column_widths[i]);
	}

	defaults->update("TOTAL_BATCHES", batches.total);
	for(int i = 0; i < batches.total; ++i) {
		Batch *batch = batches.values[i];
		Asset *asset = batch->asset;
		sprintf(string, "RECORD_PATH_%d", i);
		defaults->update(string, asset->path);
		sprintf(string, "RECORD_CHANNEL_%d", i);
		int chan_no = channeldb->number_of(batch->channel);
		defaults->update(string, chan_no);
		sprintf(string, "RECORD_STARTDAY_%d", i);
		defaults->update(string, batch->start_day);
		sprintf(string, "RECORD_STARTTIME_%d", i);
		defaults->update(string, batch->start_time);
		sprintf(string, "RECORD_DURATION_%d", i);
		defaults->update(string, batch->duration);
		sprintf(string, "RECORD_MODE_%d", i);
		defaults->update(string, batch->record_mode);
		sprintf(string, "BATCH_ENABLED_%d", i);
		defaults->update(string, batch->enabled);
	}
}

void RecordBatches::
save_default_channel(ChannelDB * channeldb)
{
	BC_Hash *defaults = mwindow->defaults;
	Batch *batch = get_editing_batch();
	if( !batch ) return;
	int chan_no = channeldb->number_of(batch->channel);
	if( chan_no < 0 ) return;
	defaults->update("RECORD_CURRENT_CHANNEL", chan_no);
}

RecordBatches::
RecordBatches(MWindow * mwindow)
{
	this->mwindow = mwindow;
	editing_item = current_item = -1;
	batch_active = 0;
	early_margin = late_margin = 0.;
	default_directory[0] = 0;
}

RecordBatches::
~RecordBatches()
{
}

void RecordBatches::
remove(Batch *batch)
{
	batches.remove(batch);
	int total_batches = total();
	if( current_item == editing_item ) {
		if(current_item >= total_batches)
			current_item = total_batches - 1;
		editing_item = current_item;
	}
	else {
		if(current_item > editing_item)
			--current_item;
		if( editing_item >= total_batches )
			editing_item = total_batches - 1;
	}
}

void RecordBatches::
clear()
{
	batches.remove_all_objects();
	for(int j = 0; j < BATCH_COLUMNS; j++) {
		data[j].remove_all_objects();
	}
	current_item = editing_item = -1;
}

RecordBatchesGUI::
RecordBatchesGUI(RecordBatches &batches,
	int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT,		// Display text list or icons
			batches.data,		// Each column has an ArrayList of BC_ListBoxItems.
			batches.batch_titles,	// Titles for columns.	Set to 0 for no titles
			batches.column_widths,	// width of each column
			BATCH_COLUMNS,		// Total columns.
			0,			// Pixel of top of window.
			0,			// If this listbox is a popup window
			LISTBOX_SINGLE,		// Select one item or multiple items
			ICON_LEFT,		// Position of icon relative to text of each item
			1),			// Allow dragging
   batches(batches)
{
	dragging_item = 0;
}

// Do nothing for double clicks to protect active batch
int RecordBatchesGUI::
handle_event()
{
	return 1;
}

int RecordBatchesGUI::
update(int highlighted_item, int recalc_positions)
{
	return BC_ListBox::update(batches.data, batches.batch_titles,
			batches.column_widths, BATCH_COLUMNS,
			get_xposition(), get_yposition(),
			highlighted_item, recalc_positions);
}

int RecordBatchesGUI::
column_resize_event()
{
	for(int i = 0; i < BATCH_COLUMNS; i++) {
		batches.column_widths[i] = get_column_width(i);
	}
	return 1;
}

int RecordBatchesGUI::
drag_start_event()
{
	if(!BC_ListBox::drag_start_event()) return 0;
	dragging_item = 1;
	return 1;
}

int RecordBatchesGUI::
drag_motion_event()
{
	if(!BC_ListBox::drag_motion_event()) return 0;
	return 1;
}

int RecordBatchesGUI::
selection_changed()
{
	int n = get_selection_number(0, 0);
	if(n >= 0) {
		set_editing_batch(n);
		if(get_cursor_x() < batches.column_widths[0]) {
			Batch *batch = batches[n];
			batch->enabled = !batch->enabled;
			update_batches(n);
		}
	}
	return 1;
}

int RecordBatchesGUI::
drag_stop_event()
{
	if(dragging_item) {
		int total_batches = batches.total();
		int src_item = editing_batch();
		Batch *src_batch = batches[src_item];
		int dst_item = get_highlighted_item();
		if(dst_item < 0) dst_item = total_batches;
		if(dst_item > src_item) --dst_item;

		for(int i = src_item; i < total_batches - 1; i++)
			batches[i] = batches[i + 1];
		for(int i = total_batches - 1; i > dst_item; --i)
			batches[i] = batches[i - 1];
		batches[dst_item] = src_batch;

		BC_ListBox::drag_stop_event();
		dragging_item = 0;
		set_editing_batch(dst_item);
		update_batches(dst_item);
	}
	return 0;
}

void RecordBatchesGUI::
update_batch_news(int item)
{
	Batch *batch = batches[item];
	batch->calculate_news();
	batches.data[2].values[item]->set_text(batch->news);
	update();  flush();
}

void RecordBatchesGUI::
update_batches(int selection_item)
{
	time_t t;  time(&t);
	char string[BCTEXTLEN], string2[BCTEXTLEN];

	for(int j = 0; j < BATCH_COLUMNS; j++) {
		batches.data[j].remove_all_objects();
	}
	int total_batches = batches.total();
	for(int i = 0; i < total_batches; i++) {
		Batch *batch = batches[i];
		batch->update_times();
		batch->calculate_news();

		int color = LTGREEN;
		if( i != batches.current_item ) {
			if( batch->time_start < t )
				color = RED;
			else if( i > 0 && batches[i-1]->time_end > batch->time_start )
				color = RED;
			else if( batch->time_start - t > 2*24*3600 )
				color = YELLOW;
		}
		else
			color = LTYELLOW;

		batches.data[0].append(
			new BC_ListBoxItem((char *)(batch->enabled ? "X" : " "), color));
		batches.data[1].append(
			new BC_ListBoxItem(batch->asset->path, color));
		batches.data[2].append(
			new BC_ListBoxItem(batch->news, RED));
		Units::totext(string2, batch->start_time, TIME_HMS3);
		sprintf(string, "%s %s", TimeEntry::day_table[batch->start_day], string2);
		batches.data[3].append(
			new BC_ListBoxItem(string, color));
		Units::totext(string, batch->duration, TIME_HMS3);
		batches.data[4].append(
			new BC_ListBoxItem(string, color));
		sprintf(string, "%s", batch->channel ? batch->channel->title : _("None"));
		batches.data[5].append(
			new BC_ListBoxItem(string, color));
		sprintf(string, "%s", Batch::mode_to_text(batch->record_mode));
		batches.data[6].append(
			new BC_ListBoxItem(string, color));

		if(i == selection_item) {
			for(int j = 0; j < BATCH_COLUMNS; j++) {
				batches.data[j].values[i]->set_selected(1);
			}
		}
	}
	update(batches.editing_item, 1);
	flush();
}

void RecordBatchesGUI::
set_row_color(int row, int color)
{
	for(int i = 0; i < BATCH_COLUMNS; i++) {
		BC_ListBoxItem *batch = batches.data[i].values[row];
		batch->set_color(color);
	}
}


RecordBatchesGUI::Dir::
Dir(RecordBatches &batches, const char *dir, int x, int y)
 : BC_TextBox(x, y, 200, 1, dir),
   batches(batches),
   directory(batches.default_directory)
{
	strncpy(directory, dir, sizeof(directory));
	dir_entries = new ArrayList<BC_ListBoxItem*>;
	entries_dir[0] = 0;
	load_dirs(dir);
}

RecordBatchesGUI::Dir::
~Dir()
{
	dir_entries->remove_all_objects();
	delete dir_entries;
}

int RecordBatchesGUI::Dir::
handle_event()
{
	char *path = FileSystem::basepath(directory);
	load_dirs(path);
	calculate_suggestions(dir_entries);
	delete [] path;
	return 1;
}

void RecordBatchesGUI::Dir::
load_dirs(const char *dir)
{
	if( !strncmp(dir, entries_dir, sizeof(entries_dir)) ) return;
	strncpy(entries_dir, dir, sizeof(entries_dir));
	dir_entries->remove_all_objects();
	FileSystem fs;  fs.update(dir);
	int total_files = fs.total_files();
	for(int i = 0; i < total_files; i++) {
		if( !fs.get_entry(i)->get_is_dir() ) continue;
		const char *name = fs.get_entry(i)->get_name();
		dir_entries->append(new BC_ListBoxItem(name));
	}
}

RecordBatchesGUI::Path::
Path(RecordBatches &batches, int x, int y)
 : BC_TextBox(x, y, 200, 1, batches.get_editing_batch()->asset->path),
   batches(batches)
{
}

RecordBatchesGUI::Path::
~Path()
{
}

int RecordBatchesGUI::Path::
handle_event()
{
	calculate_suggestions();
	Batch *batch = batches.gui->get_editing_batch();
	strcpy(batch->asset->path, get_text());
	batches.gui->update_batches();
	return 1;
}

RecordBatchesGUI::StartTime::
StartTime(BC_Window *win, RecordBatches &batches, int x, int y, int w)
 : TimeEntry(win, x, y,
		 &batches.get_editing_batch()->start_day,
		 &batches.get_editing_batch()->start_time, TIME_HMS3, w),
   batches(batches)
{
}

int RecordBatchesGUI::StartTime::
handle_event()
{
	batches.gui->update_batches();
	return 1;
}


RecordBatchesGUI::Duration::
Duration(BC_Window *win, RecordBatches &batches, int x, int y, int w)
 : TimeEntry(win, x, y, 0,
		 &batches.get_editing_batch()->duration, TIME_HMS2, w),
   batches(batches)
{
}

int RecordBatchesGUI::Duration::
handle_event()
{
	batches.gui->update_batches();
	return 1;
}


RecordBatchesGUI::Source::
Source(BC_Window *win, RecordBatches &batches, int x, int y)
 : BC_PopupTextBox(win, &sources,
		 batches.get_editing_batch()->get_source_text(),
		 x, y, 200, 200),
   batches(batches)
{
}

int RecordBatchesGUI::Source::
handle_event()
{
	batches.gui->update_batches();
	return 1;
}


RecordBatchesGUI::News::
News(RecordBatches &batches, int x, int y)
 : BC_TextBox(x, y, 200, 1, batches.get_editing_batch()->news),
   batches(batches)
{
}

int RecordBatchesGUI::News::
handle_event()
{
	return 1;
}


RecordBatchesGUI::NewBatch::
NewBatch(RecordBatches &batches, int x, int y)
 : BC_GenericButton(x, y, _("New")),
   batches(batches)
{
	set_tooltip(_("Create new clip."));
}

int RecordBatchesGUI::NewBatch::
handle_event()
{
	int new_item = batches.total()-1;
	batches.editing_item = new_item;
	batches.gui->update_batches(new_item);
	return 1;
}


RecordBatchesGUI::DeleteBatch::
DeleteBatch(RecordBatches &batches, int x, int y)
 : BC_GenericButton(x, y, _("Delete")),
   batches(batches)
{
	set_tooltip(_("Delete clip."));
}

int RecordBatchesGUI::DeleteBatch::
handle_event()
{
	batches.gui->update_batches();
	return 1;
}


RecordBatchesGUI::StartBatches::
StartBatches(RecordBatches &batches, int x, int y)
 : BC_GenericButton(x, y, _("Start")), batches(batches)
{
	set_tooltip(_("Start batch recording\nfrom the current position."));
}

int RecordBatchesGUI::StartBatches::
handle_event()
{
	batches.batch_active = 1;
	return 1;
}


RecordBatchesGUI::StopBatches::
StopBatches(RecordBatches &batches, int x, int y)
 : BC_GenericButton(x, y, _("Stop")), batches(batches)
{
}

int RecordBatchesGUI::StopBatches::
handle_event()
{
	batches.batch_active = 0;
	return 1;
}


RecordBatchesGUI::ActivateBatch::
ActivateBatch(RecordBatches &batches, int x, int y)
 : BC_GenericButton(x, y, _("Activate")), batches(batches)
{
	set_tooltip(_("Make the highlighted\nclip active."));
}

int RecordBatchesGUI::ActivateBatch::
handle_event()
{
	return 1;
}


RecordBatchesGUI::ClearBatch::
ClearBatch(RecordBatches &batches, int x, int y)
 : BC_GenericButton(x, y, _("Clear")), batches(batches)
{
	set_tooltip(_("Delete all clips."));
}

int RecordBatchesGUI::ClearBatch::
handle_event()
{
	batches.gui->update_batches();
	return 1;
}

