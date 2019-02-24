
/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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

#ifndef BCTEXTBOX_H
#define BCTEXTBOX_H

#include "bclistbox.h"
#include "bcmenuitem.h"
#include "bcpopupmenu.h"
#include "bcsubwindow.h"
#include "bctumble.h"
#include "fonts.h"
#include "bctextbox.inc"
#include "bctimer.inc"

#define BCCURSORW 2




class BC_TextBoxSuggestions;

class BC_ScrollTextBoxXScroll;
class BC_ScrollTextBoxYScroll;
class BC_ScrollTextBoxText;
class BC_TextMenu;

class BC_TextBox : public BC_SubWindow
{
	static inline int nib(int ch) {
		if( ch >= '0' && ch <= '9' ) ch -= '0';
		else if( ch >= 'A' && ch <= 'F' ) ch -= 'A'-10;
		else if( ch >= 'a' && ch <= 'f' ) ch -= 'a'-10;
		else ch = -1;
		return ch;
	}
// may be local fixed/dynamic, or shared fixed ref via set_text
	char *text;
// always local dynamic
	wchar_t *wtext;
public:
// size > 0: fixed buffer, size == 0: dynamic buffer
// size < 0: fixed shared buffer via set_text
	BC_TextBox(int x, int y, int w, int rows, int size, char *text,
		int has_border=1, int font=MEDIUMFONT);
	BC_TextBox(int x, int y, int w, int rows, int size, wchar_t *wtext,
		int has_border=1, int font=MEDIUMFONT);
	BC_TextBox(int x, int y, int w, int rows, const char *text,
		int has_border=1, int font=MEDIUMFONT, int is_utf8=1);
	BC_TextBox(int x, int y, int w, int rows, const wchar_t *wtext,
		int has_border=1, int font=MEDIUMFONT, int is_utf8=1);
	BC_TextBox(int x, int y, int w, int rows, int64_t text,
		int has_border=1, int font=MEDIUMFONT);
	BC_TextBox(int x, int y, int w, int rows, float text,
		int has_border=1, int font=MEDIUMFONT, int precision=4);
	BC_TextBox(int x, int y, int w, int rows,
		int text, int has_border=1, int font=MEDIUMFONT);
	virtual ~BC_TextBox();


	friend class BC_TextBoxSuggestions;
	friend class BC_ScrollTextBox;


// Whenever the contents of the text change
	virtual int handle_event() { return 0; };
// Whenever the position of the text changes
	virtual int motion_event() { return 0; };
	void set_selection(int char1, int char2, int ibeam);
	void wset_selection(int char1, int char2, int ibeam);
	int update(const char *text);
	int update(const wchar_t *wtext);
	int update(int64_t value);
	int update(float value);
	void disable();
	void enable();
	int get_enabled();
	int get_rows();

	int initialize();

	int focus_in_event();
	int focus_out_event();
	int cursor_enter_event();
	int cursor_leave_event();
	int cursor_motion_event();
	virtual int button_press_event();
	int button_release_event();
	int repeat_event(int64_t repeat_id);
	int keypress_event();
	int selection_clear_event();
	int activate();
	int deactivate();
	const char* get_text();
	const wchar_t* get_wtext();
	void set_text(char *text, int isz);
	int wdemand(int len);
	int tdemand(int len);
	int get_text_rows();
// Set top left of text view
	void set_text_row(int row);
	int get_text_row();
	int get_text_x();
	int get_text_y();
	void set_text_x(int v);
	void set_text_y(int v);
	int get_back_color();
	void set_back_color(int v);

	int reposition_window(int x, int y, int w = -1, int rows = -1);
	int uses_text();
	int cut(int do_update);
	int copy(int do_update);
	int paste(int do_update);
	
	static int calculate_h(BC_WindowBase *gui, int font, int has_border, int rows);
	static int calculate_row_h(int rows, BC_WindowBase *parent_window, int has_border = 1, int font = MEDIUMFONT);
	static int pixels_to_rows(BC_WindowBase *window, int font, int pixels);
	static int wcpos(const char *text, const char *ep);

	void set_precision(int precision);
// Whether to draw every time there is a keypress or rely on user to
// follow up every keypress with an update().
	void set_keypress_draw(int value);
	int get_ibeam_letter();
	void set_ibeam_letter(int number, int redraw = 1);
// Used for custom formatting text boxes
	int get_last_keypress();
// Table of separators to skip.  Used by time textboxes
// The separator format is "0000:0000".  Things not alnum are considered
// separators.  The alnums are replaced by user text.
	void set_separators(const char *separators);

// 1 - selects text, -1 - deselects, 0 - do nothing
// in all cases it returns text_selected after the operation
	int select_whole_text(int select);

// Compute suggestions for a path
// If entries is null, just search absolute paths
	int calculate_suggestions(ArrayList<BC_ListBoxItem*> *entries=0, const char *filter=0);


// User computes suggestions after handle_event.
// The array is copied to a local variable.
// A highlighted extension is added if 1 suggestion or a popup appears
// if multiple suggestions.
// column - starting column to replace
	void no_suggestions();
	void set_suggestions(ArrayList<char*> *suggestions, int column);

	BC_ScrollTextBoxXScroll *xscroll;
	BC_ScrollTextBoxYScroll *yscroll;
private:
	int reset_parameters(int rows, int has_border, int font, int size);
	BC_TextMenu *menu;
	void draw(int flush);
	void draw_border();
	void draw_cursor();
	void copy_selection(int clipboard_num);
	void paste_selection(int clipboard_num);
	void delete_selection(int letter1, int letter2, int text_len);
	void insert_text(const wchar_t *wcp, int len=-1);
// Reformat text according to separators.
// ibeam_left causes the ibeam to move left.
	int is_separator(const char *txt, int i);
	void do_separators(int ibeam_left);
	int get_x_position(int i, int start=0);
	void get_ibeam_position(int &x, int &y);
	void find_ibeam(int dispatch_event);
	void select_word(int &letter1, int &letter2, int ibeam_letter);
	void select_line(int &letter1, int &letter2, int ibeam_letter);
	int get_cursor_letter(int cursor_x, int cursor_y);
	int get_cursor_letter2(int cursor_x, int cursor_y);
	int get_row_h(int rows);
	void default_keypress(int &dispatch_event, int &result);
	int tstrlen();
	int tstrcmp(const char *cp);
	char *tstrcpy(const char *cp);
	char *tstrcat(const char *cp);
	int text_update(const wchar_t *wcp, int wsz, char *tcp, int tsz);
	int wtext_update();

// Top left of text relative to window
	int text_x, text_y;
// Top left of cursor relative to text
	int ibeam_x, ibeam_y;

	int ibeam_letter, unicode_active;
	int highlight_letter1, highlight_letter2;
	int highlight_letter3, highlight_letter4;
	int text_x1, text_start, text_end;
	int text_selected, word_selected, line_selected;
	int text_ascent, text_descent, text_height;
	int left_margin, right_margin, top_margin, bottom_margin;
	int has_border;
	int font;
	int rows;
	int highlighted;
	int high_color, back_color;
	int background_color;
	int size, tsize, dirty;
	int wlen, wsize;
	char temp_string[KEYPRESSLEN];
	int is_utf8;
	int active;
	int enabled;
	int precision;
	int keypress_draw;
// Cause the repeater to skip a cursor refresh if a certain event happened
// within a certain time of the last repeat event
	Timer *skip_cursor;
// Used for custom formatting text boxes
	int last_keypress;
	const char *separators;
	ArrayList<BC_ListBoxItem*> *suggestions;
	BC_TextBoxSuggestions *suggestions_popup;
	int suggestion_column;
	int selection_active;
};



class BC_TextBoxSuggestions : public BC_ListBox
{
public:
	BC_TextBoxSuggestions(BC_TextBox *text_box, int x, int y);
	virtual ~BC_TextBoxSuggestions();

	int handle_event();

	BC_TextBox *text_box;
};


class BC_ScrollTextBox
{
	BC_ScrollTextBoxText *text;
	BC_ScrollTextBoxXScroll *xscroll;
	BC_ScrollTextBoxYScroll *yscroll;
	BC_WindowBase *parent_window;
	const char *default_text;
	const wchar_t *default_wtext;
	int default_size;
	int x, y, w, rows;

	friend class BC_ScrollTextBoxText;
	friend class BC_ScrollTextBoxXScroll;
	friend class BC_ScrollTextBoxYScroll;
public:
	BC_ScrollTextBox(BC_WindowBase *parent_window,
		int x, int y, int w, int rows,
		const char *default_text, int default_size=BCTEXTLEN);
        BC_ScrollTextBox(BC_WindowBase *parent_window,
                int x, int y, int w, int rows,
                const wchar_t *default_wtext, int default_size=BCTEXTLEN);
	virtual ~BC_ScrollTextBox();
	void create_objects();

	virtual int handle_event() { return 1; }
	virtual int button_press_event();
	virtual int button_release_event();

	void set_text(char *text, int isz);
	int set_text_row(int n);
	void update(const char *text);
	void update(const wchar_t *wtext);
	void reposition_window(int x, int y, int w, int rows);
	void update_scrollbars();
// accessors
	int get_x() { return x; }
	int get_y() { return y; }
	int get_w() { return w; }
// Visible rows for resizing
	int get_rows() { return rows; }
	int get_x_pos();
	void set_x_pos(int x);
// forward functions
	int get_h();
	const char *get_text();
	const wchar_t *get_wtext();
	int get_buttonpress();
	void wset_selection(int char1, int char2, int ibeam);
	void set_selection(int char1, int char2, int ibeam);
	int get_ibeam_letter();
};

class BC_ScrollTextBoxText : public BC_TextBox
{
public:
	BC_ScrollTextBoxText(BC_ScrollTextBox *gui, const char *text);
	BC_ScrollTextBoxText(BC_ScrollTextBox *gui, const wchar_t *wtext);
	virtual ~BC_ScrollTextBoxText();
	int handle_event();
	int motion_event();
	BC_ScrollTextBox *gui;
	int button_press_event() { return gui->button_press_event(); }
	int button_release_event() { return gui->button_release_event(); }
};

class BC_ScrollTextBoxXScroll : public BC_ScrollBar
{
public:
	BC_ScrollTextBoxXScroll(BC_ScrollTextBox *gui);
	virtual ~BC_ScrollTextBoxXScroll();
	int handle_event();
	BC_ScrollTextBox *gui;
};

class BC_ScrollTextBoxYScroll : public BC_ScrollBar
{
public:
	BC_ScrollTextBoxYScroll(BC_ScrollTextBox *gui);
	virtual ~BC_ScrollTextBoxYScroll();
	int handle_event();
	BC_ScrollTextBox *gui;
};




class BC_PopupTextBoxText;
class BC_PopupTextBoxList;

class BC_PopupTextBox
{
public:
	BC_PopupTextBox(BC_WindowBase *parent_window,
		ArrayList<BC_ListBoxItem*> *list_items,
		const char *default_text, int x, int y,
		int text_w, int list_h, int list_format = LISTBOX_TEXT);
	virtual ~BC_PopupTextBox();
	int create_objects();
	virtual int handle_event();
	const char* get_text();
	const wchar_t* get_wtext();
	int get_number();
	int get_x();
	int get_y();
	int get_w();
	int get_h();
	int get_show_query();
	void set_show_query(int v);

	void update(const char *text);
	void update_list(ArrayList<BC_ListBoxItem*> *data);
	void reposition_window(int x, int y);

	friend class BC_PopupTextBoxText;
	friend class BC_PopupTextBoxList;

private:
	int x, y, text_w, list_h;
	int list_format;
	const char *default_text;
	const wchar_t *default_wtext;
	ArrayList<BC_ListBoxItem*> *list_items;
	BC_PopupTextBoxText *textbox;
	BC_PopupTextBoxList *listbox;
	BC_WindowBase *parent_window;
};

class BC_PopupTextBoxText : public BC_TextBox
{
public:
	BC_PopupTextBoxText(BC_PopupTextBox *popup, int x, int y, const char *text);
	BC_PopupTextBoxText(BC_PopupTextBox *popup, int x, int y, const wchar_t *wtext);
	virtual ~BC_PopupTextBoxText();
	int handle_event();
	BC_PopupTextBox *popup;
};

class BC_PopupTextBoxList : public BC_ListBox
{
public:
	BC_PopupTextBoxList(BC_PopupTextBox *popup, int x, int y);
	int handle_event();
	BC_PopupTextBox *popup;
};


class BC_TumbleTextBox;
class BC_TumbleTextBoxText;
class BC_FTextTumbler;
class BC_ITextTumbler;


class BC_TumbleTextBox
{
public:
	BC_TumbleTextBox(BC_WindowBase *parent_window,
		int64_t default_value, int64_t min, int64_t max,
		int x, int y, int text_w);
	BC_TumbleTextBox(BC_WindowBase *parent_window,
		int default_value, int min, int max,
		int x, int y, int text_w);
	BC_TumbleTextBox(BC_WindowBase *parent_window,
		float default_value, float min, float max,
		int x, int y, int text_w, int precision=4);
	virtual ~BC_TumbleTextBox();

	int create_objects();
	void reset();
	virtual int handle_event();
	virtual int handle_up_event();
	virtual int handle_down_event();
	const char* get_text();
	const wchar_t* get_wtext();
	BC_TextBox* get_textbox();
	int update(const char *value);
	int update(int64_t value);
	int update(float value);
	int get_x();
	int get_y();
	int get_w();
	int get_h();
	void disable(int hide_text=0);
	void enable();
	int get_enabled();
	void reposition_window(int x, int y);
	void set_boundaries(int64_t min, int64_t max);
	void set_boundaries(float min, float max);
	void set_precision(int precision);
	void set_increment(float value);
	void set_log_floatincrement(int value);
	void set_tooltip(const char *text);

	friend class BC_TumbleTextBoxText;
	friend class BC_FTextTumbler;
	friend class BC_ITextTumbler;

private:
	int x, y, text_w;
	int64_t default_value, min, max;
	float default_value_f, min_f, max_f;
	int use_float;
	int precision;
	float increment;
	int log_floatincrement;
	BC_TumbleTextBoxText *textbox;
	BC_Tumbler *tumbler;
	BC_WindowBase *parent_window;
};

class BC_TumbleTextBoxText : public BC_TextBox
{
public:
	BC_TumbleTextBoxText(BC_TumbleTextBox *popup, int64_t default_value, int x, int y);
	BC_TumbleTextBoxText(BC_TumbleTextBox *popup, float default_value, int x, int y, int precision);
	virtual ~BC_TumbleTextBoxText();
	int handle_event();
	int button_press_event();
	BC_TumbleTextBox *popup;
};

class BC_FTextTumbler : public BC_FTumbler
{
public:
	BC_FTextTumbler(BC_TumbleTextBox *tumble_text,
			float min_f, float max_f, int x, int y)
	 : BC_FTumbler(tumble_text->textbox, min_f, max_f, x, y) {
		this->tumble_text = tumble_text;
	}
	~BC_FTextTumbler() {}
	int handle_up_event() { return tumble_text->handle_up_event(); }
	int handle_down_event() { return tumble_text->handle_down_event(); }

	BC_TumbleTextBox *tumble_text;
};

class BC_ITextTumbler : public BC_ITumbler
{
public:
	BC_ITextTumbler(BC_TumbleTextBox *tumble_text,
			int64_t min, int64_t max, int x, int y)
	 : BC_ITumbler(tumble_text->textbox, min, max, x, y) {
		this->tumble_text = tumble_text;
	}
	~BC_ITextTumbler() {}
	int handle_up_event() { return tumble_text->handle_up_event(); }
	int handle_down_event() { return tumble_text->handle_down_event(); }

	BC_TumbleTextBox *tumble_text;
};


class BC_TextMenu : public BC_PopupMenu
{
public:
	BC_TextMenu(BC_TextBox *textbox);
	~BC_TextMenu();
	
	void create_objects();
	
	BC_TextBox *textbox;
};

class BC_TextMenuCut : public BC_MenuItem
{
public:
	BC_TextMenuCut(BC_TextMenu *menu);
	int handle_event();
	BC_TextMenu *menu;
};

class BC_TextMenuCopy : public BC_MenuItem
{
public:
	BC_TextMenuCopy(BC_TextMenu *menu);
	int handle_event();
	BC_TextMenu *menu;
};

class BC_TextMenuPaste : public BC_MenuItem
{
public:
	BC_TextMenuPaste(BC_TextMenu *menu);
	int handle_event();
	BC_TextMenu *menu;
};


#endif
