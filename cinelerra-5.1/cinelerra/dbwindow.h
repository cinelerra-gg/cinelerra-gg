#ifndef _DBWINDOW_H_
#define _DBWINDOW_H_

#include "canvas.h"
#include "condition.h"
#include "garbage.h"
#include "linklist.h"
#include "mutex.h"
#include "commercials.inc"
#include "dbwindow.inc"
#include "guicast.h"
#include "mediadb.h"
#include "mwindow.inc"
#include "vicon.h"

enum {
	col_vicon, col_id, col_length, col_source,
	col_title, col_start_time, col_access_time,
	col_access_count, sizeof_col
};

class DbSearchItem : public BC_ListBoxItem {
public:
	DbWindowVIcon *vicon;

	DbSearchItem(const char *text, int color=-1);
	~DbSearchItem();
};

class DbWindow : public Thread
{
public:
	MWindow *mwindow;
	DbWindowGUI *gui;
	Mutex *window_lock;
	Mutex *db_lock;
	class MDb : public Garbage, public MediaDb {
		DbWindow *dwin;
	public:
		int attach_rd() { dwin->db_lock->lock();  attachDb(0);  return 0; }
		int attach_wr() { dwin->db_lock->lock();  attachDb(1);  return 0; }
		int detach() {  dwin->db_lock->unlock();  detachDb();   return 0; }

		MDb(DbWindow *d);
		~MDb();
	} *mdb;

	void start();
	void stop();
	void run();

	DbWindow(MWindow *mwindow);
	~DbWindow();
};

class DbWindowGUI : public BC_Window
{
public:
	DbWindow *dwindow;

	DbWindowText *search_text;
	DbWindowTitleText *title_text;
	DbWindowInfoText *info_text;
	DbWindowMatchCase *match_case;
	DbWindowStart *search_start;
	DbWindowDeleteItems *del_items;
	DbWindowCancel *cancel;
	DbWindowList *search_list;
	DbWindowCanvas *canvas;
	DbWindowVIconThread *vicon_thread;

	int title_text_enable;
	int info_text_enable;
	int match_case_enable;

	int search_x, search_y, text_x, text_y;
	int del_items_x, del_items_y;
	int cancel_x, cancel_y, cancel_w, cancel_h;
	int canvas_x, canvas_y, canvas_w, canvas_h;
	int list_x, list_y, list_w, list_h;
	int sort_column, sort_order;

	const char *search_column_titles[sizeof_col];
	int search_column_widths[sizeof_col];
	int search_columns[sizeof_col];
	ArrayList<DbSearchItem*> search_items[sizeof_col];
	ArrayList<DbWindowItem*> search_results;

	void create_objects();
	void search(int n, const char *text);
	void delete_items();
	int close_event();
	int resize_event(int x, int y);
	int stop_drawing();
	int start_drawing(int update=1);
	void update();
	static int cmpr_id_dn(const void *a, const void *b);
	static int cmpr_id_up(const void *a, const void *b);
	static int cmpr_length_dn(const void *a, const void *b);
	static int cmpr_length_up(const void *a, const void *b);
	static int cmpr_source_dn(const void *a, const void *b);
	static int cmpr_source_up(const void *a, const void *b);
	static int cmpr_Source_dn(const void *a, const void *b);
	static int cmpr_Source_up(const void *a, const void *b);
	static int cmpr_Title_dn(const void *a, const void *b);
	static int cmpr_Title_up(const void *a, const void *b);
	static int cmpr_title_dn(const void *a, const void *b);
	static int cmpr_title_up(const void *a, const void *b);
	static int cmpr_start_time_dn(const void *a, const void *b);
	static int cmpr_start_time_up(const void *a, const void *b);
	static int cmpr_access_time_dn(const void *a, const void *b);
	static int cmpr_access_time_up(const void *a, const void *b);
	static int cmpr_access_count_dn(const void *a, const void *b);
	static int cmpr_access_count_up(const void *a, const void *b);
	void sort_events(int column, int order);
	void move_column(int src, int dst);

	DbWindowGUI(DbWindow *dwindow);
	~DbWindowGUI();
private:
	int search_string(const char *text, const char *sp);
	void search_clips(MediaDb *mdb, int n, const char *text);
	int delete_selection(MediaDb *mdb);
};

class DbWindowInfoText : public BC_CheckBox
{
public:
	DbWindowGUI *gui;

	int handle_event();
	void update(int v) { set_value(gui->info_text_enable = v); }

	DbWindowInfoText(DbWindowGUI *gui, int x, int y);
	~DbWindowInfoText();
};

class DbWindowTitleText : public BC_CheckBox
{
public:
	DbWindowGUI *gui;

	int handle_event();
	void update(int v) { set_value(gui->title_text_enable = v); }

	DbWindowTitleText(DbWindowGUI *gui, int x, int y);
	~DbWindowTitleText();
};

class DbWindowMatchCase : public BC_CheckBox
{
public:
	DbWindowGUI *gui;

	int handle_event();

	DbWindowMatchCase(DbWindowGUI *gui, int x, int y);
	~DbWindowMatchCase();
};

class DbWindowText : public BC_TextBox
{
public:
	DbWindowGUI *gui;

	int handle_event();
	int keypress_event();

	DbWindowText(DbWindowGUI *gui, int x, int y, int w);
	~DbWindowText();
};

class DbWindowStart : public BC_GenericButton
{
public:
	DbWindowGUI *gui;

	int handle_event();

	DbWindowStart(DbWindowGUI *gui, int x, int y);
	~DbWindowStart();
};

class DbWindowCancel : public BC_CancelButton
{
public:
	DbWindowGUI *gui;

	int handle_event();

	DbWindowCancel(DbWindowGUI *gui, int x, int y);
	~DbWindowCancel();
};

class DbWindowDeleteItems : public BC_GenericButton
{
public:
	DbWindowGUI *gui;

	int handle_event();

	DbWindowDeleteItems(DbWindowGUI *gui, int x, int y);
	~DbWindowDeleteItems();
};

class DbWindowList : public BC_ListBox
{
public:
	DbWindowGUI *gui;

	int handle_event();
	int sort_order_event();
	int keypress_event();
	int move_column_event();
	int selection_changed();
	void set_view_popup(DbWindowVIcon *vicon);

	int update_images();
	int update();

	DbWindowList(DbWindowGUI *gui, int x, int y, int w, int h);
	~DbWindowList();
};

class DbWindowCanvas : public Canvas
{
public:
	DbWindowGUI *gui;
	int is_fullscreen;

	DbWindowCanvas(DbWindowGUI *gui, int x, int y, int w, int h);
	~DbWindowCanvas();
	void flash_canvas();
	void draw_frame(VFrame *frame, int x, int y, int w, int h);
	int button_press_event() { return 0; }
	int keypress_event() { return 0; }
	int get_fullscreen() { return is_fullscreen; }
	void set_fullscreen(int value) { is_fullscreen = value; }
};

class DbWindowVIcon : public VIcon
{
public:
	DbWindowList *lbox;
	DbSearchItem *item;

	int clip_id, clip_size;
        int frame_id, frames;
        int prefix_size, suffix_offset;

	VFrame *frame();
	int64_t set_seq_no(int64_t no);
	void load_frames(DbWindow::MDb *mdb);
	void read_frames(DbWindow::MDb *mdb);

	int get_vx();
	int get_vy();

	void update_image(DbWindowGUI *gui, int clip_id);
	DbWindowVIcon();
	~DbWindowVIcon();
};

class DbWindowVIconThread : public VIconThread {
public:
	DbWindowGUI *gui;
	int list_update;

	ArrayList <DbWindowVIcon *> vicons;
	DbWindowVIcon *get_vicon(int i, DbSearchItem *item);
	void drawing_started();

	DbWindowVIconThread(DbWindowGUI *gui);
	~DbWindowVIconThread();
};

class DbWindowItem
{
public:
	int no, id, access_count;
	char *source, *title;
	double length, start_time, access_time;

	DbWindowItem(int id, const char *source, const char *title,
		double length, double start_time, double access_time,
		int access_count);
	~DbWindowItem();
};

class DbWindowScan : public BC_MenuItem
{
public:
	MWindow *mwindow;
	int handle_event();

	DbWindowScan(MWindow *mwindow);
	~DbWindowScan();
};


#endif
