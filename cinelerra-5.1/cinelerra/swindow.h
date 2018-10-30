#ifndef __SWINDOW_H__
#define __SWINDOW_H__

#include <stdio.h>
#include <stdint.h>

#include "arraylist.h"
#include "guicast.h"
#include "mwindow.inc"
#include "swindow.inc"

class SWindow : public Thread
{
public:
	MWindow *mwindow;
	Mutex *window_lock;
	Condition *swin_lock;
	SWindowGUI *gui;

        void start();
        void stop();
        void run();
	void run_swin();
	void paste_subttl();
	int update_selection();
	int done, gui_done;

	SWindow(MWindow *mwindow);
	~SWindow();
};


class SWindowOK : public BC_OKButton
{
public:
	SWindowGUI *gui;

	int button_press_event();
	int keypress_event();

	SWindowOK(SWindowGUI *gui, int x, int y);
	~SWindowOK();
};

class SWindowCancel : public BC_CancelButton
{
public:
	SWindowGUI *gui;

	int button_press_event();

	SWindowCancel(SWindowGUI *gui, int x, int y);
	~SWindowCancel();
};

class SWindowLoadPath : public BC_TextBox
{
public:
	SWindowGUI *sw_gui;

	SWindowLoadPath(SWindowGUI *gui, int x, int y, char *path);
	~SWindowLoadPath();

	int handle_event();
};

class SWindowLoadFile : public BC_GenericButton
{
public:
	SWindowGUI *sw_gui;

        int handle_event();

	SWindowLoadFile(SWindowGUI *gui, int x, int y);
	~SWindowLoadFile();
};

class SWindowSaveFile : public BC_GenericButton
{
public:
	SWindowGUI *sw_gui;

        int handle_event();

	SWindowSaveFile(SWindowGUI *gui, int x, int y);
	~SWindowSaveFile();
};


class ScriptLines
{
	int allocated, used;
public:
	int lines;
	char *text;
	void append(char *cp);
	int size() { return used; }
	int break_lines();
	int get_text_rows();
	char *get_text_row(int n);

	ScriptLines();
	~ScriptLines();
};

class ScriptScroll : public BC_ScrollBar
{
public:
	SWindowGUI *sw_gui;

	int handle_event();

	ScriptScroll(SWindowGUI *gui, int x, int y, int w);
	~ScriptScroll();
};

class ScriptPosition : public BC_TumbleTextBox
{
public:
	SWindowGUI *sw_gui;

	int handle_event();

	ScriptPosition(SWindowGUI *gui, int x, int y, int w, int v=0, int mn=0, int mx=0);
	~ScriptPosition();
};

class ScriptEntry : public BC_ScrollTextBox
{
public:
	SWindowGUI *sw_gui;

	char *ttext;
	void set_text(char *text, int isz=-1);
	int handle_event();

	ScriptEntry(SWindowGUI *gui, int x, int y, int w, int rows, char *text);
	ScriptEntry(SWindowGUI *gui, int x, int y, int w, int rows);
	~ScriptEntry();
};

class ScriptPrev : public BC_GenericButton
{
public:
	SWindowGUI *sw_gui;

	int handle_event();
	ScriptPrev(SWindowGUI *gui, int x, int y);
	~ScriptPrev();
};

class ScriptNext : public BC_GenericButton
{
public:
	SWindowGUI *sw_gui;

	int handle_event();
	ScriptNext(SWindowGUI *gui, int x, int y);
	~ScriptNext();
};

class ScriptPaste : public BC_GenericButton
{
public:
	SWindowGUI *sw_gui;

	int handle_event();
	ScriptPaste(SWindowGUI *gui, int x, int y);
	~ScriptPaste();
};

class ScriptClear : public BC_GenericButton
{
public:
	SWindowGUI *sw_gui;

	int handle_event();
	ScriptClear(SWindowGUI *gui, int x, int y);
	~ScriptClear();
};

class SWindowGUI : public BC_Window
{
	static int max(int a,int b) { return a>b ? a : b; }
public:
	SWindow *swindow;
	BC_OKButton *ok;
	BC_CancelButton *cancel;
	SWindowLoadPath *load_path;
	SWindowLoadFile *load_file;
	SWindowSaveFile *save_file;
	BC_Title *script_filesz;
	BC_Title *script_lines;
	BC_Title *script_entries;
	BC_Title *script_texts;
	BC_Title *script_title;
	BC_Title *line_title;
	ScriptPrev *prev_script;
	ScriptNext *next_script;
	ScriptPaste *paste_script;
	ScriptClear *clear_script;
	ScriptPosition *script_position;
	ScriptEntry *script_entry;
	ScriptEntry *line_entry;
	ScriptScroll *script_scroll;
	int pad;
	char *blank_line;

	char script_path[BCTEXTLEN];
	ArrayList<ScriptLines *> script;

	void create_objects();
	void load();
	void stop(int v);
	int translation_event();
	int resize_event(int w, int h);
	void load_defaults();
	void save_defaults();
	void load_script();
	void load_script(FILE *fp);
	int load_script_line(FILE *fp);
	void set_script_pos(int64_t entry_no, int text_no=0);
	int load_selection();
	int load_selection(int pos, int row);
	int load_prev_selection();
	int load_next_selection();
	int update_selection();
	int paste_text(const char *text, double start, double end);
	int paste_selection();
	int clear_selection();
	void save_spumux_data();

	int ok_x, ok_y, ok_w, ok_h;
	int cancel_x, cancel_y, cancel_w, cancel_h;
	int64_t script_entry_no, script_text_no;
	int64_t script_line_no, script_text_lines;
	int text_font, text_rowsz;

	SWindowGUI(SWindow *swindow, int x, int y, int w, int h);
	~SWindowGUI();

};


class SubttlSWin : public BC_MenuItem
{
public:
        MWindow *mwindow;
        int handle_event();

        SubttlSWin(MWindow *mwindow);
        ~SubttlSWin();
};


#endif
