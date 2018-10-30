#ifndef RECORDBATCHES_H
#define RECORDBATCHES_H

#include "arraylist.h"
#include "batch.h"
#include "bclistbox.h"
#include "bclistboxitem.inc"
#include "channeldb.inc"
#include "guicast.h"
#include "record.inc"
#include "recordbatches.inc"
#include "timeentry.h"

#include <string.h>

class RecordBatchesGUI;

class RecordBatches
{
public:
	RecordBatchesGUI *gui;
	MWindow *mwindow;
//  Don't want to interrupt recording to edit a different batch.
	int current_item; // Current batch being recorded.
	int editing_item; // Current batch being edited.
	int batch_active;
	static const char* default_batch_titles[];
	const char *batch_titles[BATCH_COLUMNS];
	static const int default_columnwidth[];
	int column_widths[BATCH_COLUMNS];
	ArrayList<BC_ListBoxItem*> data[BATCH_COLUMNS];
	ArrayList<Batch*> batches;
	double early_margin, late_margin;
	char default_directory[BCTEXTLEN];

	void load_defaults(ChannelDB *channeldb, Record *record=0);
	void save_defaults(ChannelDB *channeldb);
	void save_default_channel(ChannelDB *channeldb);
	Batch *&operator [](int i) { return batches.values[i]; }
	Batch* get_current_batch() { return batches.values[current_item]; }
	Batch* get_editing_batch() { return batches.values[editing_item]; }
	double *get_early_margin() { return &early_margin; }
	double *get_late_margin() { return &late_margin; }
	char *get_default_directory() { return default_directory; }
	int total() { return batches.total; }
	void append(Batch *batch) { batches.append(batch); }
	void remove(Batch *batch);
	void clear();
	RecordBatches(MWindow *mwindow);
	~RecordBatches();
};

class RecordBatchesGUI : public BC_ListBox
{
public:
	RecordBatches &batches;
	int dragging_item;

	int handle_event();
	virtual int selection_changed() = 0;
	int update(int highlighted_item, int recalc_positions);
	int update() { return update(get_highlighted_item(), 0); }
	int column_resize_event();
	int drag_start_event();
	int drag_motion_event();
	int drag_stop_event();
	void set_row_color(int i, int color);
	void calculate_batches(int selection_number);
	void update_batch_news(int item);
	void update_batches(int selection_item);
	void update_batches() { return update_batches(get_selection_number(0, 0)); }
	int editing_batch() { return batches.editing_item; }
	int set_editing_batch(int i) { return batches.editing_item=i; }
	int current_batch() { return batches.current_item; }
	int set_current_batch(int i) { return batches.current_item=i; }
	int count() { return batches.data[0].total; }
	Batch* get_current_batch() { return batches.get_current_batch(); }
	Batch* get_editing_batch() { return batches.get_editing_batch(); }
	RecordBatchesGUI(RecordBatches &batches, int x, int y, int w, int h);

	class Dir : public BC_TextBox {
	public:
		RecordBatches &batches;
		char (&directory)[BCTEXTLEN];
		ArrayList<BC_ListBoxItem*> *dir_entries;
		char entries_dir[BCTEXTLEN];

		int handle_event();
		void load_dirs(const char *path);
		char *get_directory() { return directory; }

		Dir(RecordBatches &batches, const char *dir, int x, int y);
		~Dir();
	};

	class Path : public BC_TextBox {
	public:
		RecordBatches &batches;

		int handle_event();

		Path(RecordBatches &batches, int x, int y);
		~Path();
	};

	class StartTime : public TimeEntry {
	public:
		RecordBatches &batches;
		int handle_event();
		StartTime(BC_Window *win, RecordBatches &batches,
			int x, int y, int w=DEFAULT_TIMEW);
	};

	class Duration : public TimeEntry
	{
	public:
		RecordBatches &batches;
		int handle_event();
		Duration(BC_Window *win, RecordBatches &batches,
			int x, int y, int w=DEFAULT_TIMEW);
	};

	class Margin : public TimeEntry
	{
		RecordBatches &batches;
		int handle_event();
		Margin(BC_Window *win, RecordBatches &batches, int x, int y);
	};

	class Sources
	{
	public:
       		ArrayList<BC_ListBoxItem*> sources;
	};

	class Source : protected Sources, public BC_PopupTextBox {
	public:
		RecordBatches &batches;
		int handle_event();
		Source(BC_Window *win, RecordBatches &batches, int x, int y);
	};

	class News : public BC_TextBox {
	public:
		RecordBatches &batches;
		int handle_event();
		News(RecordBatches &batches, int x, int y);
	};

	class NewBatch : public BC_GenericButton {
	public:
		RecordBatches &batches;
		int handle_event();
		NewBatch(RecordBatches &batches, int x, int y);
	};

	class DeleteBatch : public BC_GenericButton {
	public:
		RecordBatches &batches;
		int handle_event();
		DeleteBatch(RecordBatches &batches, int x, int y);
	};

	class StartBatches : public BC_GenericButton {
	public:
		RecordBatches &batches;
		int handle_event();
		StartBatches(RecordBatches &batches, int x, int y);
	};

	class StopBatches : public BC_GenericButton {
	public:
		RecordBatches &batches;
		int handle_event();
		StopBatches(RecordBatches &batches, int x, int y);
	};

	class ActivateBatch : public BC_GenericButton {
	public:
		RecordBatches &batches;
		int handle_event();
		ActivateBatch(RecordBatches &batches, int x, int y);
	};

	class ClearBatch : public BC_GenericButton {
	public:
		RecordBatches &batches;
		int handle_event();
		ClearBatch(RecordBatches &batches, int x, int y);
	};
};

#endif
