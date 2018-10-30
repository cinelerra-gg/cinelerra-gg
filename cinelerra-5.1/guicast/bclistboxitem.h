
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

#ifndef BCLISTBOXITEM_H
#define BCLISTBOXITEM_H

#include "arraylist.h"
#include "bcpixmap.inc"
#include "bccolors.h"
#include "vframe.h"



// Every item in a list inherits this
class BC_ListBoxItem
{
public:
	BC_ListBoxItem();
// New items
	BC_ListBoxItem(const char *text,
		int color = -1);
	BC_ListBoxItem(const char *text,
		BC_Pixmap *icon,
		int color = -1);



// autoplace is always 1 in initialization.
// Positions set with the set_ commands cancel autoplacement.
// Final positions are calculated in the next draw_items.

	virtual ~BC_ListBoxItem();

	friend class BC_ListBox;

	BC_Pixmap* get_icon();
	int get_icon_x() { return icon_x; }
	int get_icon_y() { return icon_y; }
	int get_icon_w();
	int get_icon_h();
	int get_text_x() { return text_x; }
	int get_text_y() { return text_y; }
	int get_text_w() { return text_w; }
	int get_text_h() { return text_h; }
	int get_baseline() { return baseline; }
	int get_in_view() { return in_view; }
	void set_autoplace_icon(int value) { autoplace_icon = value; }
	void set_autoplace_text(int value) { autoplace_text = value; }
	void set_icon_x(int x) { icon_x = x; autoplace_icon = 0; }
	void set_icon_y(int y) { icon_y = y; autoplace_icon = 0; }
	void set_searchable(int value) { searchable = value; }
	int get_selected() { return selected; }
	void set_selected(int value) { selected = value; }
	void set_selectable(int value) { selectable = value; }
	int get_selectable() { return selectable; }
	void set_text_x(int x) { text_x = x; autoplace_text = 0; }
	void set_text_y(int y) { text_y = y; autoplace_text = 0; }
	void set_text_w(int w) { text_w = w; }
	void set_text_h(int h) { text_h = h; }
	void set_baseline(int b) { baseline = b; }
	char* get_text() { return text; }
	void set_icon(BC_Pixmap *p) { icon = p; }
	void set_icon_vframe(VFrame *p) { icon_vframe = p; }
	void set_color(int v) { color = v; }
	int get_color() { return color; }
	virtual VFrame *get_vicon_frame() { return 0; }

	void copy_from(BC_ListBoxItem *item);
	BC_ListBoxItem& operator=(BC_ListBoxItem& item) {
		copy_from(&item);
		return *this;
	}
	void set_text(const char *new_text);

// The item with the sublist must be in column 0.  Only this is searched by
// BC_ListBox.
	int sublist_active() { return sublist && expand ? 1 : 0; }
// Mind you, sublists are ignored in icon mode.
	ArrayList<BC_ListBoxItem*>* new_sublist(int columns);
	ArrayList<BC_ListBoxItem*>* get_sublist() { return sublist; }
// Return the number of columns in the sublist
	int get_columns() { return columns; }
// Return if it's expanded
	int get_expand() { return expand; }
	void set_expand(int value) { expand = value; }
// alpha sort on text
	static void sort_items(ArrayList<BC_ListBoxItem*> &items);

private:
	int initialize();
	void set_in_view(int v) { in_view = v; }
	static int compare_item_text(const void *a, const void *b);

	BC_Pixmap *icon;
	VFrame *icon_vframe;
// x and y position in listbox relative to top left
// Different positions for icon mode and text mode are maintained
	int icon_x, icon_y;
	int text_x, text_y, text_w, text_h;
	int baseline, in_view;
// If autoplacement should be performed in the next draw
	int autoplace_icon, autoplace_text;
	char *text;
	int color;
// 1 - currently selected
// 2 - previously selected and now adding selections with shift
	int selected;
// Allow this item to be included in queries.  Directories are not
// included in queries.
	int searchable;

// Array of one list of pointers for each column for a sublist.
// It's an array so we can pass the sublist directly to another listbox.
// Sublists were used on an obsolete DVD robot interface & never again.
	ArrayList<BC_ListBoxItem*> *sublist;
// Columns in sublist
	int columns;
// Sublist is visible
	int expand;
// Item is selectable
	int selectable;
};



#endif
