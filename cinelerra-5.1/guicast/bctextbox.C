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

#include "bcclipboard.h"
#include "bclistboxitem.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bctextbox.h"
#include "clip.h"
#include "bccolors.h"
#include <ctype.h>
#include "cursors.h"
#include "filesystem.h"
#include "keys.h"
#include "language.h"
#include <math.h>
#include "bctimer.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#define VERTICAL_MARGIN 2
#define VERTICAL_MARGIN_NOBORDER 0
#define HORIZONTAL_MARGIN 4
#define HORIZONTAL_MARGIN_NOBORDER 2

BC_TextBox::BC_TextBox(int x, int y, int w, int rows,
	int size, char *text, int has_border, int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	is_utf8 = 1;
	skip_cursor = 0;
	reset_parameters(rows, has_border, font, size);
	if( size >= 0 )
		tstrcpy(text);
	else	// reference shared directly
		this->text = text;
}

BC_TextBox::BC_TextBox(int x, int y, int w, int rows,
	int size, wchar_t *wtext, int has_border, int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	is_utf8 = 1;
	skip_cursor = 0;
	reset_parameters(rows, has_border, font, size);
	wdemand(wcslen(wtext));
	wcsncpy(this->wtext, wtext, wsize);
}

BC_TextBox::BC_TextBox(int x, int y, int w, int rows,
	const char *text, int has_border, int font, int is_utf8)
 : BC_SubWindow(x, y, w, 0, -1)
{
	this->is_utf8 = is_utf8;
	skip_cursor = 0;
	reset_parameters(rows, has_border, font, BCTEXTLEN);
	tstrcpy(text);
}

BC_TextBox::BC_TextBox(int x, int y, int w, int rows,
	const wchar_t *wtext, int has_border, int font, int is_utf8)
 : BC_SubWindow(x, y, w, 0, -1)
{
	this->is_utf8 = is_utf8;
	skip_cursor = 0;
	reset_parameters(rows, has_border, font, BCTEXTLEN);
	wsize = BCTEXTLEN;
	wtext = new wchar_t[wsize+1];
	wcsncpy(this->wtext, wtext, wsize);
	this->wtext[wsize] = 0;
}

BC_TextBox::BC_TextBox(int x, int y, int w, int rows,
	int64_t text, int has_border, int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	is_utf8 = 1;
	skip_cursor = 0;
	reset_parameters(rows, has_border, font, BCSTRLEN);
	snprintf(this->text, this->tsize, "%jd", text);
	dirty = 1;  wtext_update();
}

BC_TextBox::BC_TextBox(int x, int y, int w, int rows,
	float text, int has_border, int font, int precision)
 : BC_SubWindow(x, y, w, 0, -1)
{
	is_utf8 = 1;
	skip_cursor = 0;
	reset_parameters(rows, has_border, font, BCSTRLEN);
	this->precision = precision;
	snprintf(this->text, this->tsize, "%0.*f", precision, text);
	dirty = 1;  wtext_update();
}

BC_TextBox::BC_TextBox(int x, int y, int w, int rows,
	int text, int has_border, int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	is_utf8 = 1;
	skip_cursor = 0;
	reset_parameters(rows, has_border, font, BCSTRLEN);
	snprintf(this->text, this->tsize, "%d", text);
	dirty = 1;  wtext_update();
}

BC_TextBox::~BC_TextBox()
{
	if(skip_cursor) delete skip_cursor;
	delete suggestions_popup;
	suggestions->remove_all_objects();
	delete suggestions;
	delete menu;
	delete [] wtext;
	if( size >= 0 )
		delete [] text;
}

int BC_TextBox::reset_parameters(int rows, int has_border, int font, int size)
{
	suggestions = new ArrayList<BC_ListBoxItem*>;
	suggestions_popup = 0;
	suggestion_column = 0;

	this->rows = rows;
	this->has_border = has_border;
	this->font = font;
// size > 0: fixed buffer, size == 0: dynamic buffer
// size < 0: fixed shared buffer via set_text
	this->size = size;
	this->tsize = size >= 0 ? size : -size;
	this->text = size > 0 ? new char[this->tsize+1] : 0;
	if( this->text ) this->text[0] = 0;
	text_start = 0;
	text_end = 0;
	highlight_letter1 = highlight_letter2 = 0;
	highlight_letter3 = highlight_letter4 = 0;
	ibeam_letter = 0;
	active = 0;
	text_selected = 0;
	word_selected = 0;
	line_selected = 0;
	unicode_active = -1;
	text_x = 0;
	enabled = 1;
	highlighted = 0;
	back_color = -1;
	high_color = -1;
	precision = 4;
	if (!skip_cursor)
		skip_cursor = new Timer;
	wtext = 0;
	wsize = 0;
	wlen = 0;
	keypress_draw = 1;
	last_keypress = 0;
	separators = 0;
	xscroll = 0;
	yscroll = 0;
	menu = 0;
	dirty = 1;
	selection_active = 0;
	return 0;
}

int BC_TextBox::tstrlen()
{
	return !text ? 0 : strnlen(text, tsize);
}

int BC_TextBox::tstrcmp(const char *cp)
{
	const char *tp = get_text();
	return strncmp(tp, cp, tsize);
}

char *BC_TextBox::tstrcpy(const char *cp)
{
	dirty = 1;
	if( cp ) {
		if( !size ) tdemand(strlen(cp));
		strncpy(text, cp, tsize);
	}
	else if( text )
		text[0] = 0;
	return text;
}

char *BC_TextBox::tstrcat(const char *cp)
{
	dirty = 1;
	if( !size ) tdemand(tstrlen() + strlen(cp));
	char *result = strncat(text, cp, tsize);
	return result;
}

int BC_TextBox::wtext_update()
{
	if( dirty ) {
		const char *src_enc = is_utf8 ? "UTF8" : BC_Resources::encoding;
		const char *dst_enc = BC_Resources::wide_encoding;
		int tlen = tstrlen();
		int nsize = tsize > 0 ? tsize : tlen + BCTEXTLEN;
		wdemand(nsize);
		wlen = BC_Resources::encode(src_enc, dst_enc, text, tlen,
			(char*)wtext, wsize*sizeof(wchar_t)) / sizeof(wchar_t);
		dirty = 0;
	}
	wtext[wlen] = 0;
	return wlen;
}

int BC_TextBox::text_update(const wchar_t *wcp, int wsz, char *tcp, int tsz)
{
	const char *src_enc = BC_Resources::wide_encoding;
	const char *dst_enc = BC_Resources::encoding;
	if( wsz < 0 ) wsz = wcslen(wcp);
	int len = BC_Resources::encode(src_enc, dst_enc,
		(char*)wcp, wsz*sizeof(wchar_t), tcp, tsz);
	tcp[len] = 0;
	return len;
}

int BC_TextBox::initialize()
{

	if (!skip_cursor)
		skip_cursor = new Timer;
	skip_cursor->update();
// Get dimensions
	text_ascent = get_text_ascent(font) + 1;
	text_descent = get_text_descent(font) + 1;
	text_height = text_ascent + text_descent;
	ibeam_letter = wtext_update();
	if(has_border)
	{
		left_margin = right_margin = HORIZONTAL_MARGIN;
		top_margin = bottom_margin = VERTICAL_MARGIN;
	}
	else
	{
		left_margin = right_margin = HORIZONTAL_MARGIN_NOBORDER;
		top_margin = bottom_margin = VERTICAL_MARGIN_NOBORDER;
	}
	h = get_row_h(rows);
	text_x = left_margin;
	text_y = top_margin;
	find_ibeam(0);

// Create the subwindow
	BC_SubWindow::initialize();

	BC_Resources *resources = get_resources();
	if(has_border)
	{
		if( back_color < 0 ) back_color = resources->text_background;
		if( high_color < 0 ) high_color = resources->text_background_hi;
	}
	else
	{
		if( back_color < 0 ) back_color = bg_color;
		if( high_color < 0 ) high_color = resources->text_background_noborder_hi;
	}

	draw(0);
	set_cursor(IBEAM_CURSOR, 0, 0);
	show_window(0);

	add_subwindow(menu = new BC_TextMenu(this));
	menu->create_objects();

	return 0;
}

int BC_TextBox::calculate_h(BC_WindowBase *gui,
	int font,
	int has_border,
	int rows)
{
	return rows * (gui->get_text_ascent(font) + 1 +
		gui->get_text_descent(font) + 1) +
		2 * (has_border ? VERTICAL_MARGIN : VERTICAL_MARGIN_NOBORDER);
}

void BC_TextBox::set_precision(int precision)
{
	this->precision = precision;
}

// Compute suggestions for a path
int BC_TextBox::calculate_suggestions(ArrayList<BC_ListBoxItem*> *entries, const char *filter)
{
// Let user delete suggestion
	if(get_last_keypress() != BACKSPACE) {
// Compute suggestions
		FileSystem fs;
		ArrayList<char*> suggestions;
		const char *current_text = get_text();
		int suggestion_column = 0;
		char dirname[BCTEXTLEN];  dirname[0] = 0;
		if( current_text[0] == '/' || current_text[0] == '~' )
			strncpy(dirname, current_text, sizeof(dirname));
		else if( !entries )
			getcwd(dirname, sizeof(dirname));
// If directory, tabulate it
		if( dirname[0] ) {
			if( filter ) fs.set_filter(filter);
			char *prefix, *cp;
			strncpy(dirname, current_text, sizeof(dirname));
			if( (cp=strrchr(dirname, '/')) ||
			    (cp=strrchr(dirname, '~')) ) *++cp = 0;
			fs.parse_tildas(dirname);
			fs.update(dirname);
			cp = (char *)current_text;
			if( (prefix=strrchr(cp, '/')) ||
			    (prefix=strrchr(cp, '~')) ) ++prefix;
			suggestion_column = !prefix ? 0 : prefix - cp;
			int prefix_len = prefix ? strlen(prefix) : 0;
// only include items where the filename matches the basename prefix
			for(int i = 0; i < fs.total_files(); i++) {
				char *current_name = fs.get_entry(i)->name;
				if( prefix_len>0 && strncmp(prefix, current_name, prefix_len) ) continue;
				suggestions.append(current_name);
			}
		}
		else if(entries) {
			char *prefix = (char *)current_text;
			int prefix_len = strlen(prefix);
			for(int i = 0; i < entries->size(); i++) {
				char *current_name = entries->get(i)->get_text();
				if( prefix_len>0 && strncmp(prefix, current_name, prefix_len) ) continue;
				suggestions.append(current_name);
			}
		}
		set_suggestions(&suggestions, suggestion_column);
	}

	return 1;
}

void BC_TextBox::no_suggestions()
{
	if( suggestions_popup ) {
		delete suggestions_popup;
		suggestions_popup = 0;
		activate();
	}
}

void BC_TextBox::set_suggestions(ArrayList<char*> *suggestions, int column)
{
// Copy the array
	this->suggestions->remove_all_objects();
	this->suggestion_column = column;

	if(suggestions) {
		for(int i = 0; i < suggestions->size(); i++) {
			this->suggestions->append(new BC_ListBoxItem(suggestions->get(i)));
		}

// Show the popup without taking focus
		if( suggestions->size() > 1 ) {
			if( !suggestions_popup ) {
				suggestions_popup = new BC_TextBoxSuggestions(this, x, y);
				get_parent()->add_subwindow(suggestions_popup);
				suggestions_popup->set_is_suggestions(1);
			}
			else {
				suggestions_popup->update(this->suggestions, 0, 0, 1);
			}
			suggestions_popup->activate(0);
		}
		else
// Show the highlighted text
		if( suggestions->size() == 1 ) {
			highlight_letter1 = wtext_update();
			int len = text_update(wtext,wlen, text,tsize);
			char *current_suggestion = suggestions->get(0);
			int col = len - suggestion_column;
			if( col < 0 ) col = 0;
			char *cur = current_suggestion + col;
			tstrcat(cur);
			highlight_letter2 = wtext_update();
//printf("BC_TextBox::set_suggestions %d %d\n", __LINE__, suggestion_column);

			draw(1);
			no_suggestions();
		}
	}

// Clear the popup
	if( !suggestions || !this->suggestions->size() )
		no_suggestions();
}

void BC_TextBox::wset_selection(int char1, int char2, int ibeam)
{
	highlight_letter1 = char1;
	highlight_letter2 = char2;
	ibeam_letter = ibeam;
	draw(1);
}

// count utf8 chars in text which occur before cp
int BC_TextBox::wcpos(const char *text, const char *cp)
{
	int len = 0;
	const unsigned char *bp = (const unsigned char *)text;
	const unsigned char *ep = (const unsigned char *)cp;
	while( bp < ep && *bp != 0 ) {
		++len;
		int ch = *bp++;
		if( ch < 0x80 ) continue;
		int i = ch - 0x0c0;
		int n = i<0? 0 : i<32? 1 : i<48? 2 : i<56? 3 : i<60? 4 : 5;
		for( i=n; bp < ep && --i>=0 && (*bp&0xc0) == 0x80; ++bp );
	}
	return len;
}

void BC_TextBox::set_selection(int char1, int char2, int ibeam)
{
	const char *cp = get_text();
	wset_selection(wcpos(cp, cp+char1), wcpos(cp, cp+char2), wcpos(cp, cp+ibeam));
}

int BC_TextBox::update(const char *text)
{
//printf("BC_TextBox::update 1 %d %s %s\n", tstrcmp(text), text, this->text);
// Don't update if contents are the same
	int bg_color = has_border || !highlighted ? back_color : high_color;
	if( bg_color == background_color && !tstrcmp(text)) return 0;
	tstrcpy(text);
	int wtext_len = wtext_update();
	if(highlight_letter1 > wtext_len) highlight_letter1 = wtext_len;
	if(highlight_letter2 > wtext_len) highlight_letter2 = wtext_len;
	if(ibeam_letter > wtext_len) ibeam_letter = wtext_len;
	draw(1);
	return 0;
}

int BC_TextBox::update(const wchar_t *wtext)
{
	int wtext_len = wcslen(wtext);
	wdemand(wtext_len);
	wcsncpy(this->wtext, wtext, wsize);
	this->wlen = wtext_len;
	if(highlight_letter1 > wtext_len) highlight_letter1 = wtext_len;
	if(highlight_letter2 > wtext_len) highlight_letter2 = wtext_len;
	if(ibeam_letter > wtext_len) ibeam_letter = wtext_len;
	draw(1);
	return 0;
}

int BC_TextBox::update(int64_t value)
{
	char string[BCTEXTLEN];
	sprintf(string, "%jd", value);
	update(string);
	return 0;
}

int BC_TextBox::update(float value)
{
	char string[BCTEXTLEN];
	sprintf(string, "%0.*f", precision, value);
	update(string);
	return 0;
}

void BC_TextBox::disable()
{
	if(enabled) {
		enabled = 0;
		deactivate();
		draw(1);
	}
}

void BC_TextBox::enable()
{
	if(!enabled) {
		enabled = 1;
		if(top_level) {
			draw(1);
		}
	}
}

int BC_TextBox::get_enabled() { return enabled; }
int BC_TextBox::get_text_x() { return text_x; }
int BC_TextBox::get_text_y() { return text_y; }
void BC_TextBox::set_text_x(int v) { text_x = v; }
void BC_TextBox::set_text_y(int v) { text_y = v; }
int BC_TextBox::get_back_color() { return back_color; }
void BC_TextBox::set_back_color(int v) { back_color = v; }

int BC_TextBox::pixels_to_rows(BC_WindowBase *window, int font, int pixels)
{
	return (pixels - 4) /
		(window->get_text_ascent(font) + 1 +
			window->get_text_descent(font) + 1);
}

int BC_TextBox::calculate_row_h(int rows,
	BC_WindowBase *parent_window,
	int has_border,
	int font)
{
	return rows *
		(parent_window->get_text_ascent(font) + 1 +
		parent_window->get_text_descent(font) + 1) +
		(has_border ? 4 : 0);
}

const char* BC_TextBox::get_text()
{
	int wtext_len = wtext_update();
	text_update(wtext,wtext_len, text,tsize);
	return text;
}

const wchar_t* BC_TextBox::get_wtext()
{
	wtext_update();
	return wtext;
}

void BC_TextBox::set_text(char *text, int isz)
{
	if( size < 0 && isz > 0 ) {
		this->text = text;
		tsize = isz;
		size = -isz;
		dirty = 1;
		wtext_update();
		draw(1);
	}
}

int BC_TextBox::get_text_rows()
{
	int wtext_len = wtext_update();
	int result = 1;
	for(int i = 0; i < wtext_len; i++) {
		if(wtext[i] == '\n') result++;
	}
	return result;
}


int BC_TextBox::get_row_h(int rows)
{
	return rows * text_height + top_margin + bottom_margin;
}

int BC_TextBox::reposition_window(int x, int y, int w, int rows)
{
	int new_h = get_h();
	if(w < 0) w = get_w();
	if(rows != -1)
	{
		new_h = get_row_h(rows);
		this->rows = rows;
	}

	if(x != get_x() ||
		y != get_y() ||
		w != get_w() ||
		new_h != get_h())
	{
// printf("BC_TextBox::reposition_window 1 %d %d %d %d %d %d %d %d\n",
// x, get_x(), y, get_y(), w, get_w(), new_h, get_h());
		BC_WindowBase::reposition_window(x, y, w, new_h);
		draw(0);
	}
	return 0;
}

void BC_TextBox::draw_border()
{
	BC_Resources *resources = get_resources();
// Clear margins
	set_color(background_color);
	draw_box(0, 0, left_margin, get_h());
	draw_box(get_w() - right_margin, 0, right_margin, get_h());

	if(has_border)
	{
		if(highlighted)
			draw_3d_border(0, 0, w, h,
				resources->text_border1,
				resources->text_border2_hi,
				resources->text_border3_hi,
				resources->text_border4);
		else
			draw_3d_border(0, 0, w, h,
				resources->text_border1,
				resources->text_border2,
				resources->text_border3,
				resources->text_border4);
	}
}

void BC_TextBox::draw_cursor()
{
//	set_color(background_color);

	if(ibeam_x >= 0 &&
		ibeam_y >= 0)
	{
	set_color(WHITE);
	set_inverse();

	draw_box(ibeam_x + text_x,
		ibeam_y + text_y,
		BCCURSORW,
		text_height);
	set_opaque();
	}
}


void BC_TextBox::draw(int flush)
{
	int i, k;
	int row_begin, row_end;
	int highlight_x1, highlight_x2;
	int need_ibeam = 1;
	BC_Resources *resources = get_resources();

//printf("BC_TextBox::draw %d %s\n", __LINE__, text);
// Background
	background_color = has_border || !highlighted ? back_color : high_color;
	set_color(background_color);
	draw_box(0, 0, w, h);

	int wtext_len = wtext_update();

// Draw text with selection
	set_font(font);

	for(i=0, k=text_y; i < wtext_len && k < get_h(); k += text_height) {
// Draw row of text
		row_begin = i;
		wchar_t *wtext_row = &wtext[i];
		for( ; i<wtext_len && wtext[i]!='\n'; ++i );
		if( (row_end=i) < wtext_len ) ++i;

		if(k > top_margin-text_height && k < get_h()-bottom_margin) {
// Draw highlighted region of row
			if( highlight_letter2 > highlight_letter1 &&
			    highlight_letter2 > row_begin &&
			    highlight_letter1 <= row_end ) {
				int color = active && enabled && get_has_focus() ?
					resources->text_highlight :
				    selection_active ?
					resources->text_selected_highlight :
					resources->text_inactive_highlight;
				if( unicode_active >= 0 )
					color ^= LTBLUE;
				set_color(color);
				if(highlight_letter1 >= row_begin &&
					highlight_letter1 <= row_end)
					highlight_x1 = get_x_position(highlight_letter1, row_begin);
				else
					highlight_x1 = 0;

				if(highlight_letter2 > row_begin &&
					highlight_letter2 <= row_end)
					highlight_x2 = get_x_position(highlight_letter2, row_begin);
				else
					highlight_x2 = get_w();

				draw_box(highlight_x1 + text_x, k,
					highlight_x2 - highlight_x1, text_height);
			}

// Draw text over highlight
			int len = row_end - row_begin;
			if( len > 0 ) {
				set_color(enabled ? resources->text_default : DMGREY);
				draw_single_text(1, font, text_x, k + text_ascent, wtext_row, len);
			}

// Get ibeam location
			if(ibeam_letter >= row_begin && ibeam_letter <= row_end) {
				need_ibeam = 0;
				ibeam_y = k - text_y;
				ibeam_x = get_x_position(ibeam_letter, row_begin);
			}
		}
	}

//printf("BC_TextBox::draw 3 %d\n", ibeam_y);
	if(need_ibeam) {
//		ibeam_x = ibeam_y = !wtext_len ? 0 : -1;
		ibeam_x = 0;  ibeam_y = k - text_y;
	}

//printf("BC_TextBox::draw 4 %d\n", ibeam_y);
// Draw solid cursor
	if (active)
		draw_cursor();

// Border
	draw_border();
	flash(flush);
}

int BC_TextBox::focus_in_event()
{
	draw(1);
	return 1;
}

int BC_TextBox::focus_out_event()
{
	draw(1);
	return 1;
}

int BC_TextBox::cursor_enter_event()
{
	if( top_level->event_win == win && enabled &&
	    !(top_level->get_resources()->textbox_focus_policy & CLICK_ACTIVATE) )
	{
		if( !active ) {
			top_level->deactivate();
			activate();
		}
		if(!highlighted)
		{
			highlighted = 1;
			draw_border();
			flash(1);
		}
	}
	return 0;
}

int BC_TextBox::cursor_leave_event()
{
	if(highlighted)
	{
		highlighted = 0;
		hide_tooltip();
		draw_border();
		flash(1);
	}
	if( !suggestions_popup && !get_button_down() &&
	    !(top_level->get_resources()->textbox_focus_policy & CLICK_DEACTIVATE) )
		deactivate();
	return 0;
}

int BC_TextBox::button_press_event()
{
	const int debug = 0;

	if(!enabled) return 0;
// 	if(get_buttonpress() != WHEEL_UP &&
// 		get_buttonpress() != WHEEL_DOWN &&
// 		get_buttonpress() != LEFT_BUTTON &&
// 		get_buttonpress() != MIDDLE_BUTTON) return 0;

	if(debug) printf("BC_TextBox::button_press_event %d\n", __LINE__);

	int cursor_letter = 0;
	int wtext_len = wtext_update();
	int update_scroll = 0;


	if(top_level->event_win == win)
	{
		if(!active)
		{
			hide_tooltip();
			top_level->deactivate();
			activate();
		}


		if(get_buttonpress() == WHEEL_UP)
		{
			text_y += text_height;
			text_y = MIN(text_y, top_margin);
			update_scroll = 1;
		}
		else
		if(get_buttonpress() == WHEEL_DOWN)
		{
			int min_y = -(get_text_rows() *
				text_height -
				get_h() +
				bottom_margin);
			text_y -= text_height;
			text_y = MAX(text_y, min_y);
			text_y = MIN(text_y, top_margin);
			update_scroll = 1;
		}
		else
		if(get_buttonpress() == RIGHT_BUTTON)
		{
			menu->activate_menu();
		}
		else
		{

		cursor_letter = get_cursor_letter(top_level->cursor_x, top_level->cursor_y);

	//printf("BC_TextBox::button_press_event %d %d\n", __LINE__, cursor_letter);


		if(get_triple_click())
		{
	//printf("BC_TextBox::button_press_event %d\n", __LINE__);
			line_selected = 1;
			select_line(highlight_letter1, highlight_letter2, cursor_letter);
			highlight_letter3 = highlight_letter1;
			highlight_letter4 = highlight_letter2;
			ibeam_letter = highlight_letter2;
			copy_selection(PRIMARY_SELECTION);
		}
		else
		if(get_double_click())
		{
			word_selected = 1;
			select_word(highlight_letter1, highlight_letter2, cursor_letter);
			highlight_letter3 = highlight_letter1;
			highlight_letter4 = highlight_letter2;
			ibeam_letter = highlight_letter2;
			copy_selection(PRIMARY_SELECTION);
		}
		else
		if(get_buttonpress() == MIDDLE_BUTTON)
		{
			highlight_letter3 = highlight_letter4 =
				ibeam_letter = highlight_letter1 =
				highlight_letter2 = cursor_letter;
			paste_selection(PRIMARY_SELECTION);
		}
		else
		{
			text_selected = 1;
			highlight_letter3 = highlight_letter4 =
				ibeam_letter = highlight_letter1 =
				highlight_letter2 = cursor_letter;
		}


	// Handle scrolling by highlighting text
		if(text_selected || word_selected || line_selected)
		{
			set_repeat(top_level->get_resources()->scroll_repeat);
		}

		if(ibeam_letter < 0) ibeam_letter = 0;
		if(ibeam_letter > wtext_len) ibeam_letter = wtext_len;
		}

		draw(1);
		if(update_scroll && yscroll)
		{
			yscroll->update_length(get_text_rows(),
				get_text_row(),
				yscroll->get_handlelength(),
				1);
		}
		handle_event();
		return 1;
	}
	else
	if( active ) {
		if( suggestions_popup && (!yscroll || !yscroll->is_event_win())) {
			if( suggestions_popup->button_press_event() )
				return suggestions_popup->handle_event();
		}
		else if( (top_level->get_resources()->textbox_focus_policy & CLICK_DEACTIVATE) )
			deactivate();
	}

	return 0;
}

int BC_TextBox::button_release_event()
{
	if(active)
	{
		hide_tooltip();
		if(text_selected || word_selected || line_selected)
		{
			text_selected = 0;
			word_selected = 0;
			line_selected = 0;

// Stop scrolling by highlighting text
			unset_repeat(top_level->get_resources()->scroll_repeat);
		}
	}
	return 0;
}

int BC_TextBox::cursor_motion_event()
{
	int cursor_letter, letter1, letter2;
	if(active)
	{
		if(text_selected || word_selected || line_selected)
		{
			cursor_letter = get_cursor_letter(top_level->cursor_x,
				top_level->cursor_y);

//printf("BC_TextBox::cursor_motion_event %d cursor_letter=%d\n", __LINE__, cursor_letter);

			if(line_selected)
			{
				select_line(letter1, letter2, cursor_letter);
			}
			else
			if(word_selected)
			{
				select_word(letter1, letter2, cursor_letter);
			}
			else
			if(text_selected)
			{
				letter1 = letter2 = cursor_letter;
			}

			if(letter1 <= highlight_letter3)
			{
				highlight_letter1 = letter1;
				highlight_letter2 = highlight_letter4;
				ibeam_letter = letter1;
			}
			else
			if(letter2 >= highlight_letter4)
			{
				highlight_letter2 = letter2;
				highlight_letter1 = highlight_letter3;
				ibeam_letter = letter2;
			}

			copy_selection(PRIMARY_SELECTION);




			draw(1);
			return 1;
		}
	}

	return 0;
}

int BC_TextBox::activate()
{
	if( !active ) {
		active = 1;
		top_level->set_active_subwindow(this);
		top_level->set_repeat(top_level->get_resources()->blink_rate);
	}
	draw(1);
	return 0;
}

int BC_TextBox::deactivate()
{
	if( active ) {
		active = 0;
		text_selected = word_selected = line_selected = 0;
		top_level->set_active_subwindow(0);
		top_level->unset_repeat(top_level->get_resources()->blink_rate);
	}
	draw(1);
	return 0;
}

int BC_TextBox::repeat_event(int64_t duration)
{
	int result = 0;
	int cursor_y = get_cursor_y();
	//int cursor_x = get_cursor_x();

	if(duration == top_level->get_resources()->tooltip_delay &&
		tooltip_text && tooltip_text[0] != 0 && highlighted)
	{
		show_tooltip();
		result = 1;
	}

	if(duration == top_level->get_resources()->blink_rate &&
		active &&
		get_has_focus())
	{
// don't flash if keypress
		if(skip_cursor->get_difference() < 500)
		{
// printf("BC_TextBox::repeat_event 1 %lld %lld\n",
// skip_cursor->get_difference(),
// duration);
			result = 1;
		}
		else
		{
			if(!(text_selected || word_selected || line_selected))
			{
				draw_cursor();
				flash(1);
			}
			result = 1;
		}
	}

	if(duration == top_level->get_resources()->scroll_repeat &&
		(text_selected || word_selected || line_selected))
	{
		int difference = 0;
		if(get_cursor_y() < top_margin)
		{
			difference = get_cursor_y() - top_margin ;
		}
		else
		if(get_cursor_y() > get_h() - bottom_margin)
		{
			difference = get_cursor_y() -
				(get_h() - bottom_margin);
		}
		if(difference != 0) {
			int min_y = -(get_text_rows() * text_height -
				get_h() + bottom_margin);

			text_y -= difference;
// printf("BC_TextBox::repeat_event %d %d %d\n",
// __LINE__,
// text_y,
// min_y);
			text_y = MAX(min_y, text_y);
			text_y = MIN(text_y, top_margin);

			draw(1);
			motion_event();
			result = 1;
		}

		if(get_cursor_x() < left_margin)
		{
			int difference = left_margin - get_cursor_x();

			text_x += difference;
			text_x = MIN(text_x, left_margin);
			draw(1);
			result = 1;
		}
		else if(get_cursor_x() > get_w() - right_margin)
		{
			int difference = get_cursor_x() - (get_w() - right_margin);
			int new_text_x = text_x - difference;

// Get width of current row
			int min_x = 0;
			int row_width = 0;
			int wtext_len = wtext_update();
			int row_begin = 0;
			int row_end = 0;
			for(int i = 0, k = text_y; i < wtext_len; k += text_height)
			{
				row_begin = i;
				while(wtext[i] != '\n' && i < wtext_len) {
					i++;
				}
				row_end = i;
				if(wtext[i] == '\n') i++;

				if(cursor_y >= k && cursor_y < k + text_height) {
					row_width = get_text_width(font,
						wtext + row_begin,
						row_end - row_begin);

					break;
				}
			}

			min_x = -row_width + get_w() - left_margin - BCCURSORW;
			new_text_x = MAX(new_text_x, min_x);
			new_text_x = MIN(new_text_x, left_margin);

			if(new_text_x < text_x) text_x = new_text_x;
			draw(1);
			result = 1;
		}
	}

	return result;
}

void BC_TextBox::default_keypress(int &dispatch_event, int &result)
{
	int key = top_level->get_keypress(), len;
	wchar_t *wkeys = top_level->get_wkeystring(&len);
	switch( key ) {
	case KPENTER:	key = '\n';	goto kpchr;
	case KPMINUS:	key = '-';	goto kpchr;
	case KPPLUS:	key = '+';	goto kpchr;
	case KPDEL:	key = '.';	goto kpchr;
	case RETURN:	key = '\n';	goto kpchr;
	case KPINS:	key = '0';	goto kpchr;
	case KP1: case KP2: case KP3: case KP4: case KP5:
	case KP6: case KP7: case KP8: case KP9:
		key = key - KP1 + '1';
	kpchr: {
		wkeys[0] = key;  wkeys[1] = 0;  len = 1;
		break; }
	default:
		if( key < 32 || key > 255 ) return;
	}
	insert_text(wkeys, len);
	find_ibeam(1);
	draw(1);
	dispatch_event = 1;
	result = 1;
}

int BC_TextBox::keypress_event()
{
// Result == 2 contents changed
// Result == 1 trapped keypress
// Result == 0 nothing
	int result = 0;
	int dispatch_event = 0;

	if(!active || !enabled) return 0;

	int wtext_len = wtext_update();
	last_keypress = get_keypress();

	if( unicode_active >= 0 ) {
		wchar_t wch = 0;
		int wlen =  -1;
		switch( last_keypress ) {
//unicode active acitons
		case RETURN: {
			for( int i=highlight_letter1+1; i<highlight_letter2; ++i ) {
				int ch = nib(wtext[i]);
				if( ch < 0 ) return 1;
				wch = (wch<<4) + ch;
			}
			if( wch ) {
				dispatch_event = 1;
				unicode_active = -1;
				result = 1;
				wlen = 1;
				break;
			} } // fall through
		case ESC: {
			unicode_active = -1;
			result = 1;
			wlen = 0;
			break; }
		case BACKSPACE: {
			if(ibeam_letter > 0) {
				delete_selection(ibeam_letter - 1, ibeam_letter, wtext_len);
				highlight_letter2 = --ibeam_letter;
				if( highlight_letter1 >= highlight_letter2 )
					unicode_active = -1;
			}
			result = 1;
			wlen = 0;
			break; }
		case KPINS: last_keypress = KP1-'1'+'0'; // fall thru
		case KP1: case KP2: case KP3: case KP4: case KP5:
		case KP6: case KP7: case KP8: case KP9:
			last_keypress = last_keypress-KP1 + '1';
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': {
			int n = nib(last_keypress);
			wch = n < 10 ? '0'+n : 'A'+n-10;
			result = 1;
			wlen = 1;
			break; }
//normal actions
		case TAB:
		case LEFTTAB:
			break;
//ignored actions
		default:
			result = 1;
			break;
		}
		if( wlen >= 0 ) {
			insert_text(&wch, wlen);
			find_ibeam(1);
			if( unicode_active >= 0 )
				highlight_letter2 = ibeam_letter;
			else
				highlight_letter1 = highlight_letter2 = 0;
			draw(1);
		}
	}

	if( !result ) {
//printf("BC_TextBox::keypress_event %d %x\n", __LINE__, last_keypress)
		switch(last_keypress) {
		case ESC: {
// Deactivate the suggestions
			if( suggestions_popup ) {
				no_suggestions();
				result = 1;
			}
			else {
				top_level->deactivate();
				result = 0;
			}
			break; }

		case RETURN: {
			if( rows == 1 ) {
				highlight_letter1 = highlight_letter2 = 0;
				ibeam_letter = wtext_update();
				top_level->deactivate();
				dispatch_event = 1;
				result = 0;
			}
			else {
				default_keypress(dispatch_event, result);
			}
			break; }


// Handle like a default keypress
		case TAB: {
			top_level->cycle_textboxes(1);
			result = 1;
			break; }

		case LEFTTAB: {
			top_level->cycle_textboxes(-1);
			result = 1;
			break; }

		case LEFT: {
			if(ibeam_letter > 0) {
				int old_ibeam_letter = ibeam_letter;
// Single character
				if(!ctrl_down()) {
					ibeam_letter--;
				}
				else {
// Word
					ibeam_letter--;
					while(ibeam_letter > 0 && isalnum(wtext[ibeam_letter - 1]))
						ibeam_letter--;
				}

// Extend selection
				if(top_level->shift_down()) {
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2) {
						highlight_letter1 = ibeam_letter;
						highlight_letter2 = old_ibeam_letter;
					}
					else if(highlight_letter1 == old_ibeam_letter) {
// Extend left highlight
						highlight_letter1 = ibeam_letter;
					}
					else if(highlight_letter2 == old_ibeam_letter) {
// Shrink right highlight
						highlight_letter2 = ibeam_letter;
					}
				}
				else {
					highlight_letter1 = highlight_letter2 = ibeam_letter;
				}


				find_ibeam(1);
				if(keypress_draw) draw(1);
			}
			result = 1;
			break; }

		case RIGHT: {
			if(ibeam_letter < wtext_len) {
				int old_ibeam_letter = ibeam_letter;
// Single character
				if(!ctrl_down()) {
					ibeam_letter++;
				}
				else {
// Word
					while(ibeam_letter < wtext_len && isalnum(wtext[ibeam_letter++]));
				}



// Extend selection
				if(top_level->shift_down()) {
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2) {
						highlight_letter1 = old_ibeam_letter;
						highlight_letter2 = ibeam_letter;
					}
					else if(highlight_letter1 == old_ibeam_letter) {
// Shrink left highlight
						highlight_letter1 = ibeam_letter;
					}
					else if(highlight_letter2 == old_ibeam_letter) {
// Expand right highlight
						highlight_letter2 = ibeam_letter;
					}
				}
				else {
					highlight_letter1 = highlight_letter2 = ibeam_letter;
				}

				find_ibeam(1);
				if(keypress_draw) draw(1);
			}
			result = 1;
			break; }

		case UP: {
			if( suggestions && suggestions_popup ) {
// Pass to suggestions popup
//printf("BC_TextBox::keypress_event %d\n", __LINE__);
				suggestions_popup->activate(1);
				suggestions_popup->keypress_event();
				result = 1;
			}
			else if(ibeam_letter > 0) {
//printf("BC_TextBox::keypress_event 1 %d %d %d\n", ibeam_x, ibeam_y, ibeam_letter);
				int new_letter = get_cursor_letter2(ibeam_x + text_x,
					ibeam_y + text_y - text_height);
//printf("BC_TextBox::keypress_event 2 %d %d %d\n", ibeam_x, ibeam_y, new_letter);

// Extend selection
				if(top_level->shift_down()) {
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2) {
						highlight_letter1 = new_letter;
						highlight_letter2 = ibeam_letter;
					}
					else if(highlight_letter1 == ibeam_letter) {
// Expand left highlight
						highlight_letter1 = new_letter;
					}
					else if(highlight_letter2 == ibeam_letter) {
// Shrink right highlight
						highlight_letter2 = new_letter;
					}
				}
				else
					highlight_letter1 = highlight_letter2 = new_letter;

				if(highlight_letter1 > highlight_letter2) {
					int temp = highlight_letter1;
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = temp;
				}
				ibeam_letter = new_letter;

				find_ibeam(1);
				if(keypress_draw) draw(1);
			}
			result = 1;
			break; }

		case PGUP: {
			if(ibeam_letter > 0) {
				int new_letter = get_cursor_letter2(ibeam_x + text_x,
					ibeam_y + text_y - get_h());

// Extend selection
				if(top_level->shift_down()) {
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2) {
						highlight_letter1 = new_letter;
						highlight_letter2 = ibeam_letter;
					}
					else if(highlight_letter1 == ibeam_letter) {
// Expand left highlight
						highlight_letter1 = new_letter;
					}
					else if(highlight_letter2 == ibeam_letter) {
// Shrink right highlight
						highlight_letter2 = new_letter;
					}
				}
				else
					highlight_letter1 = highlight_letter2 = new_letter;

				if(highlight_letter1 > highlight_letter2) {
					int temp = highlight_letter1;
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = temp;
				}
				ibeam_letter = new_letter;

				find_ibeam(1);
				if(keypress_draw) draw(1);
			}
			result = 1;
			break; }

		case DOWN: {
// printf("BC_TextBox::keypress_event %d %p %p\n",
// __LINE__,
// suggestions,
// suggestions_popup);
			if( suggestions && suggestions_popup ) {
// Pass to suggestions popup
				suggestions_popup->activate(1);
				suggestions_popup->keypress_event();
				result = 1;
			}
			else
//			if(ibeam_letter > 0)
			{
// Extend selection
				int new_letter = get_cursor_letter2(ibeam_x + text_x,
					ibeam_y + text_y + text_height);
//printf("BC_TextBox::keypress_event 10 %d\n", new_letter);

				if(top_level->shift_down()) {
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2) {
						highlight_letter1 = new_letter;
						highlight_letter2 = ibeam_letter;
					}
					else if(highlight_letter1 == ibeam_letter) {
// Shrink left highlight
						highlight_letter1 = new_letter;
					}
					else if(highlight_letter2 == ibeam_letter) {
// Expand right highlight
						highlight_letter2 = new_letter;
					}
				}
				else
					highlight_letter1 = highlight_letter2 = new_letter;

				if(highlight_letter1 > highlight_letter2) {
					int temp = highlight_letter1;
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = temp;
				}
				ibeam_letter = new_letter;

				find_ibeam(1);
				if(keypress_draw) draw(1);

//printf("BC_TextBox::keypress_event 20 %d\n", ibeam_letter);
			}
			result = 1;
			break; }

		case PGDN: {
// Extend selection
			int new_letter = get_cursor_letter2(ibeam_x + text_x,
				ibeam_y + text_y + get_h());
//printf("BC_TextBox::keypress_event 10 %d\n", new_letter);

			if(top_level->shift_down()) {
// Initialize highlighting
				if(highlight_letter1 == highlight_letter2) {
					highlight_letter1 = new_letter;
					highlight_letter2 = ibeam_letter;
				}
				else if(highlight_letter1 == ibeam_letter) {
// Shrink left highlight
					highlight_letter1 = new_letter;
				}
				else if(highlight_letter2 == ibeam_letter) {
// Expand right highlight
					highlight_letter2 = new_letter;
				}
			}
			else
				highlight_letter1 = highlight_letter2 = new_letter;

			if(highlight_letter1 > highlight_letter2) {
				int temp = highlight_letter1;
				highlight_letter1 = highlight_letter2;
				highlight_letter2 = temp;
			}
			ibeam_letter = new_letter;

			find_ibeam(1);
			if(keypress_draw) draw(1);

//printf("BC_TextBox::keypress_event 20 %d\n", ibeam_letter);
			result = 1;
			break; }

		case END: {
			no_suggestions();

			int old_ibeam_letter = ibeam_letter;

			while(ibeam_letter < wtext_len && wtext[ibeam_letter] != '\n')
				ibeam_letter++;

			if(top_level->shift_down()) {
// Begin selection
				if(highlight_letter1 == highlight_letter2) {
					highlight_letter2 = ibeam_letter;
					highlight_letter1 = old_ibeam_letter;
				}
				else if(highlight_letter1 == old_ibeam_letter) {
// Shrink selection
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = ibeam_letter;
				}
				else if(highlight_letter2 == old_ibeam_letter) {
// Extend selection
					highlight_letter2 = ibeam_letter;
				}
			}
			else
				highlight_letter1 = highlight_letter2 = ibeam_letter;

			find_ibeam(1);
			if(keypress_draw) draw(1);
			result = 1;
			break; }

		case HOME: {
			no_suggestions();

			int old_ibeam_letter = ibeam_letter;

			while(ibeam_letter > 0 && wtext[ibeam_letter - 1] != '\n')
				ibeam_letter--;

			if(top_level->shift_down())
			{
// Begin selection
				if(highlight_letter1 == highlight_letter2)
				{
					highlight_letter2 = old_ibeam_letter;
					highlight_letter1 = ibeam_letter;
				}
				else
// Extend selection
				if(highlight_letter1 == old_ibeam_letter)
				{
					highlight_letter1 = ibeam_letter;
				}
				else
// Shrink selection
				if(highlight_letter2 == old_ibeam_letter)
				{
					highlight_letter2 = highlight_letter1;
					highlight_letter1 = ibeam_letter;
				}
			}
			else
				highlight_letter1 = highlight_letter2 = ibeam_letter;

			find_ibeam(1);
			if(keypress_draw) draw(1);
			result = 1;
			break; }

		case BACKSPACE: {
			no_suggestions();

			if(highlight_letter1 == highlight_letter2) {
				if(ibeam_letter > 0) {
					delete_selection(ibeam_letter - 1, ibeam_letter, wtext_len);
					ibeam_letter--;
				}
			}
			else {
				delete_selection(highlight_letter1, highlight_letter2, wtext_len);
				highlight_letter2 = ibeam_letter = highlight_letter1;
			}

			find_ibeam(1);
			if(keypress_draw) draw(1);
			dispatch_event = 1;
			result = 1;
			break; }

		case DELETE: {
//printf("BC_TextBox::keypress_event %d\n", __LINE__);
			if(highlight_letter1 == highlight_letter2) {
				if(ibeam_letter < wtext_len) {
					delete_selection(ibeam_letter, ibeam_letter + 1, wtext_len);
				}
			}
			else {
				delete_selection(highlight_letter1, highlight_letter2, wtext_len);
				highlight_letter2 = ibeam_letter = highlight_letter1;
			}

			find_ibeam(1);
			if(keypress_draw) draw(1);
			dispatch_event = 1;
			result = 1;
			break; }

		default: {
			if( ctrl_down() ) {
				switch( last_keypress ) {
				case 'c': case 'C': {
					result = copy(0);
					break; }
				case 'v': case 'V': {
					result = paste(0);
					dispatch_event = 1;
					break; }
				case 'x': case 'X': {
					result = cut(0);
					dispatch_event = 1;
					break; }
				case 'u': case 'U': {
					if( shift_down() ) {
						unicode_active = ibeam_letter;
						wchar_t wkey = 'U';
						insert_text(&wkey, 1);
						find_ibeam(1);
						highlight_letter1 = unicode_active;
						highlight_letter2 = ibeam_letter;
						draw(1);
						result = 1;
					}
					break; }
				}
				break;
			}

			default_keypress(dispatch_event, result);
			break; }
		}
	}

	if(result) skip_cursor->update();
	if(dispatch_event && handle_event())
		result = 1;

	return result;
}


int BC_TextBox::cut(int do_update)
{
	if( highlight_letter1 != highlight_letter2 ) {
		int wtext_len = wtext_update();
		copy_selection(SECONDARY_SELECTION);
		delete_selection(highlight_letter1, highlight_letter2, wtext_len);
		highlight_letter2 = ibeam_letter = highlight_letter1;
	}

	find_ibeam(1);
	if( keypress_draw )
		draw(1);
	
	if( do_update ) {
		skip_cursor->update();
		handle_event();
	}
	return 1;
}

int BC_TextBox::copy(int do_update)
{
	int result = 0;
	if( highlight_letter1 != highlight_letter2 ) {
		copy_selection(SECONDARY_SELECTION);
		result = 1;
		if( do_update ) {
			skip_cursor->update();
		}
	}
	return result;
}

int BC_TextBox::paste(int do_update)
{
	paste_selection(SECONDARY_SELECTION);
	find_ibeam(1);
	if( keypress_draw )
		draw(1);
	if( do_update ) {
		skip_cursor->update();
		handle_event();
	}
	return 1;
}


int BC_TextBox::uses_text()
{
	return 1;
}


void BC_TextBox::delete_selection(int letter1, int letter2, int wtext_len)
{
	int i, j;
	for(i=letter1, j=letter2; j<wtext_len; i++, j++) {
		wtext[i] = wtext[j];
	}
	wtext[i] = 0;
	wlen = i;

	do_separators(1);
}

int BC_TextBox::wdemand(int len)
{
	if( wtext && wsize >= len ) return 0;
	int nsize = len + wlen/2 + BCTEXTLEN;
	wchar_t *ntext = new wchar_t[nsize+1];
	ntext[nsize] = 0;
	memcpy(ntext, wtext, wsize*sizeof(wtext[0]));
	delete [] wtext;  wtext = ntext;  wsize = nsize;
	return 1;
}

int BC_TextBox::tdemand(int len)
{
	if( text && tsize >= len ) return 0;
	int tlen = !text ? 0 : strlen(text);
	int nsize = len + tlen/2 + BCTEXTLEN;
	char *ntext = new char[nsize+1];
	ntext[nsize] = 0;
	memcpy(ntext, text, tsize*sizeof(text[0]));
	delete [] text;  text = ntext;  tsize = nsize;
	return 1;
}

void BC_TextBox::insert_text(const wchar_t *wcp, int len)
{
	if( len < 0 ) len = wcslen(wcp);
	int wtext_len = wtext_update();
	wdemand(wtext_len + len + 1);
	if( unicode_active < 0 && highlight_letter1 < highlight_letter2 ) {
		delete_selection(highlight_letter1, highlight_letter2, wtext_len);
		highlight_letter2 = ibeam_letter = highlight_letter1;
		wtext_len = wtext_update();
	}

	int i, j;
	for( i=wtext_len, j=wtext_len+len; --i>=ibeam_letter; )
		if( --j < wsize ) wtext[j] = wtext[i];
	for( i=ibeam_letter,j=0; j<len; ++i, ++j )
		if( i < wsize ) wtext[i] = wcp[j];

	if( (wlen+=len) > wsize ) wlen = wsize;
	if( (ibeam_letter+=len) > wsize ) ibeam_letter = wsize;
	wtext[wlen] = 0;  // wtext allocated wsize+1
	do_separators(0);
}

int BC_TextBox::is_separator(const char *txt, int i)
{
	if( i != 0 || separators[0] != '+' ) return !isalnum(txt[i]);
	return txt[0] != '+' && txt[0] != '-' && !isalnum(txt[0]);
}

// used for time entry
void BC_TextBox::do_separators(int ibeam_left)
{
	if(separators)
	{
// Remove separators from text
		int wtext_len = wtext_update();
		for(int i = 0; i < wtext_len; ) {
			if( !iswalnum(wtext[i]) ) {
				for(int j = i; j < wtext_len - 1; j++)
					wtext[j] = wtext[j + 1];
				if(!ibeam_left && i < ibeam_letter) ibeam_letter--;
				wlen = --wtext_len;
				continue;
			}
			++i;
		}
		wtext[wtext_len] = 0;





// Insert separators into text
		int separator_len = strlen(separators);
		for(int i = 0; i < separator_len; i++) {
			if(i < wtext_len) {
// Insert a separator
				if( is_separator(separators,i) ) {
					for(int j = wtext_len; j >= i; j--) {
						wtext[j + 1] = wtext[j];
					}
					if(!ibeam_left && i < ibeam_letter) ibeam_letter++;
					++wtext_len;
					wtext[i] = separators[i];
				}
			}
			else {
				wtext[i] = separators[i];
			}
		}

// Truncate text
		wtext[separator_len] = 0;
		wlen = separator_len;
	}
}

int BC_TextBox::get_x_position(int i, int start)
{
	return get_text_width(font, &wtext[start], i - start);
}

void BC_TextBox::get_ibeam_position(int &x, int &y)
{
	int i, row_begin, row_end;
	int wtext_len = wtext_update();
	x = y = 0;

	for( i=0; i<wtext_len; ) {
		row_begin = i;
		for(; i<wtext_len && wtext[i]!='\n'; i++);
		row_end = i;

		if( ibeam_letter >= row_begin && ibeam_letter <= row_end ) {
			x = get_x_position(ibeam_letter, row_begin);
//printf("BC_TextBox::get_ibeam_position %d %d %d %d %d\n", ibeam_letter, row_begin, row_end, x, y);
			return;
		}

		if( i < wtext_len && wtext[i] == '\n' ) {
			i++;
			y += text_height;
		}
	}
//printf("BC_TextBox::get_ibeam_position 10 %d %d\n", x, y);

	x = 0;
	return;
}

void BC_TextBox::set_text_row(int row)
{
	text_x = left_margin;
	text_y = -(row * text_height) + top_margin;
	draw(1);
}

int BC_TextBox::get_text_row()
{
	return -(text_y - top_margin) / text_height;
}

void BC_TextBox::find_ibeam(int dispatch_event)
{
	int x, y;
	int old_x = text_x, old_y = text_y;

	get_ibeam_position(x, y);

	if(left_margin + text_x + x >= get_w() - right_margin - BCCURSORW)
	{
		text_x = -(x - (get_w() - get_w() / 4)) + left_margin;
		if(text_x > left_margin) text_x = left_margin;
	}
	else
	if(left_margin + text_x + x < left_margin)
	{
		text_x = -(x - (get_w() / 4)) + left_margin;
		if(text_x > left_margin) text_x = left_margin;
	}

	int text_row = y / text_height;
	if( text_row < rows ) text_y = top_margin;

	int pix_rows = get_h() - bottom_margin - (y + text_y);
	if( pix_rows < text_height )
		text_y -= text_height * ((2*text_height-1-pix_rows) / text_height);

	pix_rows = y + text_y - top_margin;
	if( pix_rows < 0 ) {
		text_y += text_height * ((text_height-1-pix_rows) / text_height);
		if( text_y > top_margin ) text_y = top_margin;
	}

	if(dispatch_event && (old_x != text_x || old_y != text_y)) motion_event();
}

// New algorithm
int BC_TextBox::get_cursor_letter(int cursor_x, int cursor_y)
{
	int i, j, k, row_begin, row_end, result = 0, done = 0;
	int column1, column2;
	int got_visible_row = 0;

// Select complete row if cursor above the window
//printf("BC_TextBox::get_cursor_letter %d %d\n", __LINE__, text_y);
	if(cursor_y < text_y - text_height)
	{
		result = 0;
		done = 1;
	}

	int wtext_len = wtext_update();

	for(i=0, k=text_y; i<wtext_len && k<get_h() && !done; k+=text_height) {
// Simulate drawing of 1 row
		row_begin = i;
		for(j = 0; wtext[i]!='\n' && i<wtext_len; i++);
		row_end = i;

// check visibility
		int first_visible_row = 0;
		int last_visible_row = 0;
		if( k+text_height > top_margin && !got_visible_row) {
			first_visible_row = 1;
			got_visible_row = 1;
		}

		if( (k+text_height >= get_h() - bottom_margin ||
			(row_end >= wtext_len && k < get_h() - bottom_margin &&
				k + text_height > 0)) )
			last_visible_row = 1;

// Cursor is inside vertical range of row
		if((cursor_y >= top_margin &&
			cursor_y < get_h() - bottom_margin &&
			cursor_y >= k && cursor_y < k + text_height) ||
// Cursor is above 1st row
			(cursor_y < k + text_height && first_visible_row) ||
// Cursor is below last row
			(cursor_y >= k && last_visible_row))
		{
			column1 = column2 = 0;
			for(j = row_begin; j<wsize && j<=row_end && !done; j++) {
				column2 = get_text_width(font, &wtext[row_begin], j-row_begin) + text_x;
				if((column2 + column1) / 2 >= cursor_x) {
					result = j - 1;
					done = 1;
// printf("BC_TextBox::get_cursor_letter %d %d %d %d\n",
// __LINE__, result, first_visible_row, last_visible_row);
				}
				column1 = column2;
			}

			if(!done) {
				result = row_end;
				done = 1;
			}
		}

		if(wtext[i] == '\n') i++;

// Select complete row if last visible & cursor is below window
 		if(last_visible_row && cursor_y > k + text_height * 2)
 			result = row_end;

		if(i >= wtext_len && !done) {
			result = wtext_len;
		}
	}


// printf("BC_TextBox::get_cursor_letter %d cursor_y=%d k=%d h=%d %d %d\n",
//  __LINE__, cursor_y, k, get_h(), first_visible_row, last_visible_row);
	if(result < 0) result = 0;
	if(result > wtext_len) {
//printf("BC_TextBox::get_cursor_letter %d\n", __LINE__);
		result = wtext_len;
	}

	return result;
}

// Old algorithm
int BC_TextBox::get_cursor_letter2(int cursor_x, int cursor_y)
{
	int i, j, k, row_begin, row_end, result = 0, done = 0;
	int column1, column2;
	int wtext_len = wtext_update();

	if(cursor_y < text_y) {
		result = 0;
		done = 1;
	}

	for(i = 0, k = text_y; i < wtext_len && !done; k += text_height) {
		row_begin = i;
		for(; wtext[i] != '\n' && i < wtext_len; i++);
		row_end = i;

		if(cursor_y >= k && cursor_y < k + text_height) {
			column1 = column2 = 0;
			for(j = 0; j <= row_end - row_begin && !done; j++) {
				column2 = get_text_width(font, &wtext[row_begin], j) + text_x;
				if((column2 + column1) / 2 >= cursor_x) {
					result = row_begin + j - 1;
					done = 1;
				}
				column1 = column2;
			}
			if(!done)
			{
				result = row_end;
				done = 1;
			}
		}
		if(wtext[i] == '\n') i++;

		if(i >= wtext_len && !done) {
			result = wtext_len;
		}
	}
	if(result < 0) result = 0;
	if(result > wtext_len) result = wtext_len;
	return result;
}


void BC_TextBox::select_word(int &letter1, int &letter2, int ibeam_letter)
{
	int wtext_len = wtext_update();
	letter1 = letter2 = ibeam_letter;
	if( letter1 < 0 ) letter1 = 0;
	if( letter2 < 0 ) letter2 = 0;
	if( letter1 > wtext_len ) letter1 = wtext_len;
	if( letter2 > wtext_len ) letter2 = wtext_len;
	if( !wtext_len ) return;
	for( int i=letter1; i>=0 && iswalnum(wtext[i]); --i ) letter1 = i;
	for( int i=letter2; i<wtext_len && iswalnum(wtext[i]); ) letter2 = ++i;
	if( letter2 < wtext_len && wtext[letter2] == ' ' ) ++letter2;
}


void BC_TextBox::select_line(int &letter1, int &letter2, int ibeam_letter)
{
	int wtext_len = wtext_update();
	letter1 = letter2 = ibeam_letter;
	if( letter1 < 0 ) letter1 = 0;
	if( letter2 < 0 ) letter2 = 0;
	if( letter1 > wtext_len ) letter1 = wtext_len;
	if( letter2 > wtext_len ) letter2 = wtext_len;
	if( !wtext_len ) return;
	for( int i=letter1; i>=0 && wtext[i]!='\n'; --i ) letter1 = i;
	for( int i=letter2; i<wtext_len && wtext[i]!='\n'; ) letter2 = ++i;
}

void BC_TextBox::copy_selection(int clipboard_num)
{
	int wtext_len = wtext_update();
	if(!wtext_len) return;

	if(highlight_letter1 >= wtext_len || highlight_letter2 > wtext_len ||
		highlight_letter1 < 0 || highlight_letter2 < 0 ||
		highlight_letter2 - highlight_letter1 <= 0) return;
	int clip_len = highlight_letter2 - highlight_letter1;
//printf(" BC_TextBox::copy_selection %d %d %d\n",highlight_letter1, highlight_letter2, clip_len);
	char ctext[4*clip_len+4];
	clip_len = text_update(&wtext[highlight_letter1],clip_len, ctext,4*clip_len+4);
	to_clipboard(ctext, clip_len, clipboard_num);
	selection_active = 1;
}

int BC_TextBox::selection_clear_event()
{
	if( !is_event_win() ) return 0;
	selection_active = 0;
	draw(1);
	return 1;
}

void BC_TextBox::paste_selection(int clipboard_num)
{
	int len = clipboard_len(clipboard_num);
	if( len > 0 )
	{
		char cstring[len];  wchar_t wstring[len];
		from_clipboard(cstring, len, clipboard_num);  --len;
//printf("BC_TextBox::paste_selection %d '%*.*s'\n",len,len,len,cstring);
		len = BC_Resources::encode(BC_Resources::encoding, BC_Resources::wide_encoding,
			cstring,len, (char *)wstring,(len+1)*sizeof(wchar_t)) / sizeof(wchar_t);
		insert_text(wstring, len);
		last_keypress = 0;
	}
}

void BC_TextBox::set_keypress_draw(int value)
{
	keypress_draw = value;
}

int BC_TextBox::get_last_keypress()
{
	return last_keypress;
}

int BC_TextBox::get_ibeam_letter()
{
	return ibeam_letter;
}

void BC_TextBox::set_ibeam_letter(int number, int redraw)
{
	this->ibeam_letter = number;
	if(redraw)
	{
		draw(1);
	}
}

void BC_TextBox::set_separators(const char *separators)
{
	this->separators = (char*)separators;
}

int BC_TextBox::get_rows()
{
	return rows;
}







BC_TextBoxSuggestions::BC_TextBoxSuggestions(BC_TextBox *text_box, int x, int y)
 : BC_ListBox(x, y, text_box->get_w(), 200, LISTBOX_TEXT,
	text_box->suggestions, 0, 0, 1, 0, 1)
{
	this->text_box = text_box;
	set_use_button(0);
	set_justify(LISTBOX_LEFT);
}

BC_TextBoxSuggestions::~BC_TextBoxSuggestions()
{
}

int BC_TextBoxSuggestions::handle_event()
{
	char *current_suggestion = 0;
	BC_ListBoxItem *item = get_selection(0, 0);
//printf("BC_TextBoxSuggestions::handle_event %d\n", __LINE__);
	if(item && (current_suggestion=item->get_text()) != 0)
	{
		int col = text_box->suggestion_column;
		int len = BCTEXTLEN-1 - col;
		char *cp = &text_box->text[col];
//printf("BC_TextBoxSuggestions::handle_event %d\n", __LINE__);
		strncpy(cp, current_suggestion, len);
//printf("BC_TextBoxSuggestions::handle_event %d\n", __LINE__);
		if( (col=strlen(current_suggestion)) >= len )
			cp[len-1] = 0;
		text_box->dirty = 1;
	}


//printf("BC_TextBoxSuggestions::handle_event %d\n", __LINE__);
	text_box->highlight_letter1 =
		text_box->highlight_letter2 =
		text_box->ibeam_letter = text_box->tstrlen();
	text_box->wtext_update();
	text_box->draw(1);
	text_box->handle_event();
//printf("BC_TextBoxSuggestions::handle_event %d\n", __LINE__);
	return 1;
}


BC_ScrollTextBox::BC_ScrollTextBox(BC_WindowBase *parent_window,
	int x, int y, int w, int rows,
	const char *default_text, int default_size)
{
	this->parent_window = parent_window;
	this->x = x;
	this->y = y;
	this->w = w;
	this->rows = rows;
	xscroll = 0;  yscroll = 0;
	this->default_text = default_text;
	this->default_wtext = 0;
	this->default_size = default_size;
}

BC_ScrollTextBox::BC_ScrollTextBox(BC_WindowBase *parent_window,
	int x, int y, int w, int rows,
	const wchar_t *default_wtext, int default_size)
{
	this->parent_window = parent_window;
	this->x = x;
	this->y = y;
	this->w = w;
	this->rows = rows;
	xscroll = 0;  yscroll = 0;
	this->default_text = 0;
	this->default_wtext = default_wtext;
	this->default_size = default_size;
}

BC_ScrollTextBox::~BC_ScrollTextBox()
{
	delete xscroll;
	delete yscroll;
	if(text) {
		text->gui = 0;
		delete text;
	}
}

void BC_ScrollTextBox::create_objects()
{
// Must be created first
	parent_window->add_subwindow(text = default_wtext ?
 		new BC_ScrollTextBoxText(this, default_wtext) :
		new BC_ScrollTextBoxText(this, default_text));
	set_text_row(0);
}

void BC_ScrollTextBox::set_text(char *text, int isz)
{
	this->text->set_text(text, isz);
	update_scrollbars();
}

int BC_ScrollTextBox::set_text_row(int n)
{
	text->set_text_row(n);
	update_scrollbars();
	return 1;
}

void BC_ScrollTextBox::update(const char *text)
{
	this->text->update(text);
	update_scrollbars();
}

void BC_ScrollTextBox::update(const wchar_t *wtext)
{
	this->text->update(wtext);
	update_scrollbars();
}

void BC_ScrollTextBox::reposition_window(int x, int y, int w, int rows)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->rows = rows;
	update_scrollbars();
}

int BC_ScrollTextBox::button_press_event()
{
	return text->BC_TextBox::button_press_event();
}
int BC_ScrollTextBox::button_release_event()
{
	return text->BC_TextBox::button_release_event();
}

int BC_ScrollTextBox::get_h() { return text->get_h(); }
const char *BC_ScrollTextBox::get_text() { return text->get_text(); }
const wchar_t *BC_ScrollTextBox::get_wtext() { return text->get_wtext(); }

int BC_ScrollTextBox::get_buttonpress()
{
	return text->BC_TextBox::get_buttonpress();
}
void BC_ScrollTextBox::wset_selection(int char1, int char2, int ibeam)
{
	text->wset_selection(char1, char2, ibeam);
}
void BC_ScrollTextBox::set_selection(int char1, int char2, int ibeam)
{
	text->set_selection(char1, char2, ibeam);
}
int BC_ScrollTextBox::get_ibeam_letter()
{
	return text->get_ibeam_letter();
}
int BC_ScrollTextBox::get_x_pos()
{
	return text->left_margin - text->get_text_x();
}
void BC_ScrollTextBox::set_x_pos(int x)
{
	text->set_text_x(text->left_margin - x);
	text->draw(1);
}

BC_ScrollTextBoxText::BC_ScrollTextBoxText(BC_ScrollTextBox *gui, const char *text)
 : BC_TextBox(gui->x, gui->y,
	gui->w - get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w(),
	gui->rows, gui->default_size, (char*)text, 1, MEDIUMFONT)
{
	this->gui = gui;
}

BC_ScrollTextBoxText::BC_ScrollTextBoxText(BC_ScrollTextBox *gui, const wchar_t *wtext)
 : BC_TextBox(gui->x, gui->y,
	gui->w - get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w(),
	gui->rows, gui->default_size, (wchar_t*)wtext, 1, MEDIUMFONT)
{
	this->gui = gui;
}

BC_ScrollTextBoxText::~BC_ScrollTextBoxText()
{
	if(gui)
	{
		gui->text = 0;
		delete gui;
	}
}

void BC_ScrollTextBox::update_scrollbars()
{
	int view_w = w, view_rows = rows;
	int view_h = BC_TextBox::calculate_row_h(view_rows, parent_window);
	int text_rows = text->get_text_rows();
	int text_width = parent_window->get_text_width(text->font, text->get_wtext());
	BC_Resources *resources = parent_window->get_resources();
	int need_xscroll = 0, need_yscroll = 0;

// Create scrollbars as needed
	int resize = 1;
	while( resize ) {
		resize = 0;
		if( !need_xscroll && (text->get_text_x() != text->left_margin ||
		      text_width >= view_w - text->left_margin - text->right_margin) ) {
			resize = need_xscroll = 1;
			view_h -= resources->hscroll_data[SCROLL_HANDLE_UP]->get_h();
			view_rows = BC_TextBox::pixels_to_rows(parent_window, text->font, view_h);
		}
		if( !need_yscroll && (text->get_text_y() != text->top_margin ||
		      text_rows > view_rows) ) {
			resize = need_yscroll = 1;
			view_w -= resources->vscroll_data[SCROLL_HANDLE_UP]->get_w();
		}
	}

	if( !need_xscroll && xscroll ) {
		text->yscroll = 0;
		delete xscroll;  xscroll = 0;
	}
	if( !need_yscroll && yscroll ) {
		text->yscroll = 0;
		delete yscroll;  yscroll = 0;
	}

	if( view_rows != text->get_rows() || view_w != text->get_w() ) {
		text->reposition_window(x, y, view_w, view_rows);
	}
	if( need_xscroll && !xscroll ) {
		xscroll = new BC_ScrollTextBoxXScroll(this);
		parent_window->add_subwindow(xscroll);
		text->xscroll = xscroll;
		xscroll->bound_to = text;
		xscroll->show_window();
	}
	if( need_yscroll && !yscroll ) {
		yscroll = new BC_ScrollTextBoxYScroll(this);
		parent_window->add_subwindow(yscroll);
		text->yscroll = yscroll;
		yscroll->bound_to = text;
		yscroll->show_window();
	}
	if( xscroll ) {
		xscroll->reposition_window(x, y + text->get_h(), view_w);
		int xpos = get_x_pos();
		if( xpos != xscroll->get_value() )
			xscroll->update_value(xpos);
		if( text_width != xscroll->get_length() ||
		    view_w != xscroll->get_handlelength() )
			xscroll->update_length(text_width, xpos, view_w, 0);
	}
	if( yscroll ) {
		yscroll->reposition_window(x + w - yscroll->get_span(), y, text->get_h());
		int text_row = text->get_text_row();
		if( text_row != yscroll->get_value() )
			yscroll->update_value(text_row);
		if( text_rows != yscroll->get_length() ||
		    view_rows != yscroll->get_handlelength() )
			yscroll->update_length(text_rows, text_row, view_rows, 0);
	}
}

int BC_ScrollTextBoxText::handle_event()
{
	gui->update_scrollbars();
	return gui->handle_event();
}

int BC_ScrollTextBoxText::motion_event()
{
	gui->update_scrollbars();
	return 1;
}

BC_ScrollTextBoxXScroll::BC_ScrollTextBoxXScroll(BC_ScrollTextBox *gui)
 : BC_ScrollBar(gui->x, gui->y + gui->text->get_h(), SCROLL_HORIZ + SCROLL_STRETCH,
	gui->text->get_w(), gui->text->get_text_width(MEDIUMFONT, gui->get_wtext()),
	0, gui->w)
{
	this->gui = gui;
}

BC_ScrollTextBoxXScroll::~BC_ScrollTextBoxXScroll()
{
}

int BC_ScrollTextBoxXScroll::handle_event()
{
	gui->set_x_pos(get_position());
	return 1;
}

BC_ScrollTextBoxYScroll::BC_ScrollTextBoxYScroll(BC_ScrollTextBox *gui)
 : BC_ScrollBar(gui->x + gui->text->get_w(), gui->y, SCROLL_VERT,
	gui->text->get_h(), gui->text->get_text_rows(), 0, gui->rows)
{
	this->gui = gui;
}

BC_ScrollTextBoxYScroll::~BC_ScrollTextBoxYScroll()
{
}

int BC_ScrollTextBoxYScroll::handle_event()
{
	gui->text->set_text_row(get_position());
	return 1;
}



BC_PopupTextBoxText::BC_PopupTextBoxText(BC_PopupTextBox *popup, int x, int y, const char *text)
 : BC_TextBox(x, y, popup->text_w, 1, text, BCTEXTLEN)
{
	this->popup = popup;
}

BC_PopupTextBoxText::BC_PopupTextBoxText(BC_PopupTextBox *popup, int x, int y, const wchar_t *wtext)
 : BC_TextBox(x, y, popup->text_w, 1, wtext, BCTEXTLEN)
{
	this->popup = popup;
}

BC_PopupTextBoxText::~BC_PopupTextBoxText()
{
	if(popup) {
		popup->textbox = 0;
		delete popup;
		popup = 0;
	}
}


int BC_PopupTextBoxText::handle_event()
{
	popup->handle_event();
	return 1;
}

BC_PopupTextBoxList::BC_PopupTextBoxList(BC_PopupTextBox *popup, int x, int y)
 : BC_ListBox(x, y,
	popup->text_w + BC_WindowBase::get_resources()->listbox_button[0]->get_w(),
	popup->list_h, popup->list_format, popup->list_items, 0, 0, 1, 0, 1)
{
	this->popup = popup;
}
int BC_PopupTextBoxList::handle_event()
{
	BC_ListBoxItem *item = get_selection(0, 0);
	if(item)
	{
		popup->textbox->update(item->get_text());
		popup->textbox->set_text_row(0);
		popup->handle_event();
	}
	return 1;
}




BC_PopupTextBox::BC_PopupTextBox(BC_WindowBase *parent_window,
		ArrayList<BC_ListBoxItem*> *list_items,
		const char *default_text, int x, int y,
		int text_w, int list_h, int list_format)
{
	this->x = x;
	this->y = y;
	this->list_h = list_h;
	this->list_format = list_format;
	this->default_text = (char*)default_text;
	this->default_wtext = 0;
	this->text_w = text_w;
	this->parent_window = parent_window;
	this->list_items = list_items;
}

BC_PopupTextBox::~BC_PopupTextBox()
{
	delete listbox;
	if(textbox)
	{
		textbox->popup = 0;
		delete textbox;
	}
}

int BC_PopupTextBox::create_objects()
{
	int x = this->x, y = this->y;
	parent_window->add_subwindow(textbox = default_wtext ?
		 new BC_PopupTextBoxText(this, x, y, default_wtext) :
		 new BC_PopupTextBoxText(this, x, y, default_text));
	x += textbox->get_w();
	parent_window->add_subwindow(listbox = new BC_PopupTextBoxList(this, x, y));
	return 0;
}

void BC_PopupTextBox::update(const char *text)
{
	textbox->update(text);
	textbox->set_text_row(0);
}

void BC_PopupTextBox::update_list(ArrayList<BC_ListBoxItem*> *data)
{
	listbox->update(data, 0, 0, 1);
}


const char* BC_PopupTextBox::get_text()
{
	return textbox->get_text();
}

const wchar_t* BC_PopupTextBox::get_wtext()
{
	return textbox->get_wtext();
}

int BC_PopupTextBox::get_number()
{
	return listbox->get_selection_number(0, 0);
}

int BC_PopupTextBox::get_x()
{
	return x;
}

int BC_PopupTextBox::get_y()
{
	return y;
}

int BC_PopupTextBox::get_w()
{
	return textbox->get_w() + listbox->get_w();
}

int BC_PopupTextBox::get_h()
{
	return textbox->get_h();
}

int BC_PopupTextBox::get_show_query()
{
	return listbox->get_show_query();
}

void BC_PopupTextBox::set_show_query(int v)
{
	listbox->set_show_query(v);
}

int BC_PopupTextBox::handle_event()
{
	return 1;
}

void BC_PopupTextBox::reposition_window(int x, int y)
{
	this->x = x;
	this->y = y;
	int x1 = x, y1 = y;
	textbox->reposition_window(x1,
		y1,
		textbox->get_w(),
		textbox->get_rows());
	x1 += textbox->get_w();
	listbox->reposition_window(x1, y1, -1, -1, 0);
//	if(flush) parent_window->flush();
}














BC_TumbleTextBoxText::BC_TumbleTextBoxText(BC_TumbleTextBox *popup,
	int64_t default_value, int x, int y)
 : BC_TextBox(x, y, popup->text_w, 1, default_value)
{
	this->popup = popup;
}

BC_TumbleTextBoxText::BC_TumbleTextBoxText(BC_TumbleTextBox *popup,
	float default_value, int x, int y, int precision)
 : BC_TextBox(x, y, popup->text_w, 1, default_value, 1, MEDIUMFONT, precision)
{
	this->popup = popup;
}

BC_TumbleTextBoxText::~BC_TumbleTextBoxText()
{
	if(popup)
	{
		popup->textbox = 0;
		delete popup;
		popup = 0;
	}
}



int BC_TumbleTextBoxText::handle_event()
{
	popup->handle_event();
	return 1;
}

int BC_TumbleTextBoxText::button_press_event()
{
	if( get_enabled() && is_event_win() ) {
		if( get_buttonpress() < 4 ) return BC_TextBox::button_press_event();
		if( get_buttonpress() == 4 )      popup->tumbler->handle_up_event();
		else if( get_buttonpress() == 5 ) popup->tumbler->handle_down_event();
		return 1;
	}
	return 0;
}




BC_TumbleTextBox::BC_TumbleTextBox(BC_WindowBase *parent_window,
		int64_t default_value,
		int64_t min,
		int64_t max,
		int x,
		int y,
		int text_w)
{
	reset();
	this->x = x;
	this->y = y;
	this->min = min;
	this->max = max;
	this->default_value = default_value;
	this->text_w = text_w;
	this->parent_window = parent_window;
	use_float = 0;
	precision = 4;
	increment = 1;
}

BC_TumbleTextBox::BC_TumbleTextBox(BC_WindowBase *parent_window,
		int default_value,
		int min,
		int max,
		int x,
		int y,
		int text_w)
{
	reset();
	this->x = x;
	this->y = y;
	this->min = min;
	this->max = max;
	this->default_value = default_value;
	this->text_w = text_w;
	this->parent_window = parent_window;
	use_float = 0;
	precision = 4;
	increment = 1;
}

BC_TumbleTextBox::BC_TumbleTextBox(BC_WindowBase *parent_window,
		float default_value_f,
		float min_f,
		float max_f,
		int x,
		int y,
		int text_w)
{
	reset();
	this->x = x;
	this->y = y;
	this->min_f = min_f;
	this->max_f = max_f;
	this->default_value_f = default_value_f;
	this->text_w = text_w;
	this->parent_window = parent_window;
	use_float = 1;
	precision = 4;
	increment = 1;
}

BC_TumbleTextBox::~BC_TumbleTextBox()
{
// Recursive delete.  Normally ~BC_TumbleTextBox is never called but textbox
// is deleted anyway by the windowbase so textbox deletes this.
	if(tumbler) delete tumbler;
	tumbler = 0;
// Don't delete text here if we were called by ~BC_TumbleTextBoxText
	if(textbox)
	{
		textbox->popup = 0;
		delete textbox;
	}
	textbox = 0;
}

void BC_TumbleTextBox::reset()
{
	textbox = 0;
	tumbler = 0;
	increment = 1.0;
}

void BC_TumbleTextBox::set_precision(int precision)
{
	this->precision = precision;
}

void BC_TumbleTextBox::set_increment(float value)
{
	this->increment = value;
	if(tumbler) tumbler->set_increment(value);
}

void BC_TumbleTextBox::set_log_floatincrement(int value)
{
	this->log_floatincrement = value;
	if(tumbler) tumbler->set_log_floatincrement(value);
}

int BC_TumbleTextBox::create_objects()
{
	int x = this->x, y = this->y;

	textbox = use_float ?
		new BC_TumbleTextBoxText(this, default_value_f, x, y, precision) :
		new BC_TumbleTextBoxText(this, default_value, x, y);

	parent_window->add_subwindow(textbox);
	x += textbox->get_w();

	tumbler = use_float ?
		(BC_Tumbler *)new BC_FTumbler(textbox, min_f, max_f, x, y) :
		(BC_Tumbler *)new BC_ITumbler(textbox, min, max, x, y);
	parent_window->add_subwindow(tumbler);
	tumbler->set_increment(increment);
	return 0;
}

const char* BC_TumbleTextBox::get_text()
{
	return textbox->get_text();
}

const wchar_t* BC_TumbleTextBox::get_wtext()
{
	return textbox->get_wtext();
}

BC_TextBox* BC_TumbleTextBox::get_textbox()
{
	return textbox;
}

int BC_TumbleTextBox::update(const char *value)
{
	textbox->update(value);
	textbox->set_text_row(0);
	return 0;
}

int BC_TumbleTextBox::update(int64_t value)
{
	textbox->update(value);
	textbox->set_text_row(0);
	return 0;
}

int BC_TumbleTextBox::update(float value)
{
	textbox->update(value);
	textbox->set_text_row(0);
	return 0;
}


int BC_TumbleTextBox::get_x()
{
	return x;
}

int BC_TumbleTextBox::get_y()
{
	return y;
}

int BC_TumbleTextBox::get_w()
{
	return textbox->get_w() + tumbler->get_w();
}

int BC_TumbleTextBox::get_h()
{
	return textbox->get_h();
}

void BC_TumbleTextBox::disable(int hide_text)
{
	if( hide_text && !textbox->is_hidden() )
		textbox->hide_window(0);
	if( !tumbler->is_hidden() )
		tumbler->hide_window(0);
	if( !get_enabled() ) return;
	return textbox->disable();
}

void BC_TumbleTextBox::enable()
{
	if( textbox->is_hidden() )
		textbox->show_window(0);
	if( tumbler->is_hidden() )
		tumbler->show_window(0);
	if( get_enabled() ) return;
	return textbox->enable();
}

int BC_TumbleTextBox::get_enabled()
{
	return textbox->get_enabled();
}

int BC_TumbleTextBox::handle_event()
{
	return 1;
}

void BC_TumbleTextBox::reposition_window(int x, int y)
{
	this->x = x;
	this->y = y;

	textbox->reposition_window(x,
 		y,
		text_w,
		1);
	tumbler->reposition_window(x + textbox->get_w(),
		y);
//	if(flush) parent_window->flush();
}


void BC_TumbleTextBox::set_boundaries(int64_t min, int64_t max)
{
	tumbler->set_boundaries(min, max);
}

void BC_TumbleTextBox::set_boundaries(float min, float max)
{
	tumbler->set_boundaries(min, max);
}



BC_TextMenu::BC_TextMenu(BC_TextBox *textbox)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->textbox = textbox;
}

BC_TextMenu::~BC_TextMenu()
{
}

void BC_TextMenu::create_objects()
{
	add_item(new BC_TextMenuCut(this));
	add_item(new BC_TextMenuCopy(this));
	add_item(new BC_TextMenuPaste(this));
}


BC_TextMenuCut::BC_TextMenuCut(BC_TextMenu *menu) 
 : BC_MenuItem(_("Cut"))
{
	this->menu = menu;
}

int BC_TextMenuCut::handle_event()
{
	menu->textbox->cut(1);
	
	return 0;
}


BC_TextMenuCopy::BC_TextMenuCopy(BC_TextMenu *menu) 
 : BC_MenuItem(_("Copy"))
{
	this->menu = menu;
}

int BC_TextMenuCopy::handle_event()
{
	menu->textbox->copy(1);
	return 0;
}


BC_TextMenuPaste::BC_TextMenuPaste(BC_TextMenu *menu) 
 : BC_MenuItem(_("Paste"))
{
	this->menu = menu;
}

int BC_TextMenuPaste::handle_event()
{
	menu->textbox->paste(1);
	return 0;
}


void BC_TumbleTextBox::set_tooltip(const char *text)
{
	textbox->set_tooltip(text);
}

