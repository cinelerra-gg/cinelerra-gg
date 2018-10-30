
/*
 * CINELERRA
 * Copyright (C) 1997-2017 Adam Williams <broadcast at earthling dot net>
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

#include "bcdelete.h"
#include "bcfilebox.h"
#include "bclistboxitem.h"
#include "bcnewfolder.h"
#include "bcpixmap.h"
#include "bcrename.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bctitle.h"
#include "clip.h"
#include "condition.h"
#include "filesystem.h"
#include "keys.h"
#include "language.h"
#include "mutex.h"
#include <string.h>
#include <sys/stat.h>







BC_FileBoxRecent::BC_FileBoxRecent(BC_FileBox *filebox, int x, int y)
 : BC_ListBox(x, y, 250,
		filebox->get_text_height(MEDIUMFONT) * FILEBOX_HISTORY_SIZE +
		BC_ScrollBar::get_span(SCROLL_HORIZ) +
		LISTBOX_MARGIN * 2, LISTBOX_TEXT, &filebox->recent_dirs,
		0, 0, 1, 0, 1)
{
	this->filebox = filebox;
}

int BC_FileBoxRecent::handle_event()
{
	BC_ListBoxItem *selection = get_selection(0, 0);
	if( selection != 0 ) {
		char *path = selection->get_text();
		filebox->submit_dir(path);
	}
	return 1;
}











BC_FileBoxListBox::BC_FileBoxListBox(int x, int y, BC_FileBox *filebox)
 : BC_ListBox(x, y, filebox->get_listbox_w(), filebox->get_listbox_h(y),
		filebox->get_display_mode(), filebox->list_column,
		filebox->column_titles, filebox->column_width,
		filebox->columns, 0, 0,
		filebox->select_multiple ? LISTBOX_MULTIPLE : LISTBOX_SINGLE,
		ICON_LEFT, 0)
{
	this->filebox = filebox;
	set_sort_column(filebox->sort_column);
	set_sort_order(filebox->sort_order);
	set_allow_drag_column(1);
}

BC_FileBoxListBox::~BC_FileBoxListBox()
{
}

int BC_FileBoxListBox::handle_event()
{
	filebox->submit_file(filebox->textbox->get_text());
	return 1;
}

int BC_FileBoxListBox::selection_changed()
{
	BC_ListBoxItem *item = get_selection(
		filebox->column_of_type(FILEBOX_NAME), 0);

	if(item) {
		char path[BCTEXTLEN];
		strcpy(path, item->get_text());
		filebox->textbox->update(path);
		filebox->fs->extract_dir(filebox->directory, path);
		filebox->fs->extract_name(filebox->filename, path);
		filebox->fs->complete_path(path);
		strcpy(filebox->current_path, path);
		strcpy(filebox->submitted_path, path);
	}
	return 1;
}

int BC_FileBoxListBox::column_resize_event()
{
	for(int i = 0; i < filebox->columns; i++)
		BC_WindowBase::get_resources()->filebox_columnwidth[i] =
			filebox->column_width[i] =
			get_column_width(i);
	return 1;
}

int BC_FileBoxListBox::sort_order_event()
{
	get_resources()->filebox_sortcolumn = filebox->sort_column = get_sort_column();
	get_resources()->filebox_sortorder = filebox->sort_order = get_sort_order();
	filebox->refresh(-1);
	return 1;
}

int BC_FileBoxListBox::move_column_event()
{
	filebox->move_column(get_from_column(), get_to_column());
	return 1;
}

int BC_FileBoxListBox::evaluate_query(char *string)
{
// Search name column
	ArrayList<BC_ListBoxItem*> *column =
		&filebox->list_column[filebox->column_of_type(FILEBOX_NAME)];
// Get current selection
	int current_selection = get_selection_number(0, 0);

// Get best score in remaining items
	int lowest_score = 0x7fffffff;
	int best_item = -1;
	if(current_selection < 0) current_selection = 0;
//	for(int i = current_selection + 1; i < column->size(); i++)
	for(int i = current_selection; i < column->size(); i++)
	{
		int len1 = strlen(string);
		int len2 = strlen(column->get(i)->get_text());
		int current_score = strncasecmp(string,
			column->get(i)->get_text(),
			MIN(len1, len2));
//printf(" %d i=%d %d %s %s\n", __LINE__, i, current_score, string, column->get(i)->get_text());

		if(abs(current_score) < lowest_score)
		{
			lowest_score = abs(current_score);
			best_item = i;
		}
	}


	return best_item;
}


BC_FileBoxTextBox::BC_FileBoxTextBox(int x, int y, BC_FileBox *filebox)
 : BC_TextBox(x, y, filebox->get_w() - 20, 1,
	filebox->want_directory ?  filebox->directory : filebox->filename)
{
	this->filebox = filebox;
}

BC_FileBoxTextBox::~BC_FileBoxTextBox()
{
}

int BC_FileBoxTextBox::handle_event()
{
	int result = 0;
	if(get_keypress() != RETURN)
	{
		result = calculate_suggestions(&filebox->list_column[0]);
	}
	return result;
}


BC_FileBoxDirectoryText::BC_FileBoxDirectoryText(int x, int y, BC_FileBox *filebox)
 : BC_TextBox(x, y, filebox->get_w() - 40, 1, filebox->fs->get_current_dir())
{
	this->filebox = filebox;
}

int BC_FileBoxDirectoryText::handle_event()
{
	const char *path = get_text();
	if( !filebox->fs->is_dir(path) ) return 0;
	const char *cp = path;
	while( *cp ) ++cp;
	if( cp > path && *--cp != '/' ) return 0;
	char *file_path = FileSystem::basepath(path);
	char *dir_path = FileSystem::basepath(filebox->directory);
	int ret = !strcmp(file_path, dir_path) ? 0 : 1;
	if( ret ) {
		strcpy(filebox->directory, file_path);
		filebox->update_history();
		filebox->fs->change_dir(file_path);
		filebox->refresh(1);
	}
	delete [] dir_path;
	delete [] file_path;
	return ret;
}


BC_FileBoxSearchText::BC_FileBoxSearchText(int x, int y, BC_FileBox *filebox)
 : BC_TextBox(x, y, filebox->get_w() - x - 40, 1, "")
{
	this->filebox = filebox;
}

int BC_FileBoxSearchText::handle_event()
{
	filebox->refresh();
	return 1;
}


BC_FileBoxFilterText::BC_FileBoxFilterText(int x, int y, BC_FileBox *filebox)
 : BC_TextBox(x, y, filebox->get_w() - x - 50, 1, filebox->get_resources()->filebox_filter)
{
	this->filebox = filebox;
}

int BC_FileBoxFilterText::handle_event()
{
	filebox->update_filter(get_text());
	return 1;
}


BC_FileBoxFilterMenu::BC_FileBoxFilterMenu(int x, int y, BC_FileBox *filebox)
 : BC_ListBox(x, y, filebox->get_w() - 30, 120,
	LISTBOX_TEXT, &filebox->filter_list, 0, 0, 1, 0, 1)
{
	this->filebox = filebox;
	set_tooltip(_("Change the filter"));
}

int BC_FileBoxFilterMenu::handle_event()
{
	filebox->filter_text->update(
		get_selection(filebox->column_of_type(FILEBOX_NAME), 0)->get_text());
	filebox->update_filter(
		get_selection(filebox->column_of_type(FILEBOX_NAME), 0)->get_text());
	return 1;
}


BC_FileBoxSizeFormat::BC_FileBoxSizeFormat(int x, int y, BC_FileBox *file_box)
 : BC_Button(x, y, &BC_WindowBase::get_resources()->
		filebox_szfmt_images[3*file_box->size_format])
{
	this->file_box = file_box;
	set_tooltip(_("Size numeric format"));
}
BC_FileBoxSizeFormat::~BC_FileBoxSizeFormat()
{
}

int BC_FileBoxSizeFormat::handle_event()
{
	if( ++file_box->size_format > FILEBOX_SIZE_THOU )
		file_box->size_format = FILEBOX_SIZE_RAW;
	BC_WindowBase::get_resources()->filebox_size_format = file_box->size_format;
	set_images(&BC_WindowBase::get_resources()->
		filebox_szfmt_images[3*file_box->size_format]);
	draw_face(0);
	file_box->refresh(0);
	return 1;
}


BC_FileBoxUseThis::BC_FileBoxUseThis(BC_FileBox *filebox)
 : BC_Button(filebox->get_w() / 2 -
 		BC_WindowBase::get_resources()->usethis_button_images[0]->get_w() / 2,
 	filebox->ok_button->get_y(),
	BC_WindowBase::get_resources()->usethis_button_images)
{
	this->filebox = filebox;
	set_tooltip(_("Submit the directory"));
}

BC_FileBoxUseThis::~BC_FileBoxUseThis()
{
}

int BC_FileBoxUseThis::handle_event()
{
// printf("BC_FileBoxUseThis::handle_event %d '%s'\n",
// __LINE__,
// filebox->textbox->get_text());
	filebox->submit_file(filebox->textbox->get_text(), 1);
	return 1;
}


BC_FileBoxOK::BC_FileBoxOK(BC_FileBox *filebox)
 : BC_OKButton(filebox,
 	!filebox->want_directory ?
		BC_WindowBase::get_resources()->ok_images :
		BC_WindowBase::get_resources()->filebox_descend_images)
{
	this->filebox = filebox;
	if(filebox->want_directory)
		set_tooltip(_("Descend directory"));
	else
		set_tooltip(_("Submit the file"));
}

BC_FileBoxOK::~BC_FileBoxOK()
{
}

int BC_FileBoxOK::handle_event()
{
//printf("BC_FileBoxOK::handle_event %d\n", __LINE__);
	const char *path = filebox->textbox->get_text();
	if( *path ) filebox->submit_file(path);
	return 1;
}

BC_FileBoxCancel::BC_FileBoxCancel(BC_FileBox *filebox)
 : BC_CancelButton(filebox)
{
	this->filebox = filebox;
	set_tooltip(_("Cancel the operation"));
}

BC_FileBoxCancel::~BC_FileBoxCancel()
{
}

int BC_FileBoxCancel::handle_event()
{
//	filebox->submit_file(filebox->textbox->get_text());
	filebox->newfolder_thread->interrupt();
	filebox->set_done(1);
	return 1;
}



BC_FileBoxText::BC_FileBoxText(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_text_images)
{
	this->filebox = filebox;
	set_tooltip(_("Display text"));
}
int BC_FileBoxText::handle_event()
{
	filebox->create_listbox(filebox->listbox->get_x(), filebox->listbox->get_y(), LISTBOX_TEXT);
	filebox->listbox->show_window(1);
	return 1;
}


BC_FileBoxIcons::BC_FileBoxIcons(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_icons_images)
{
	this->filebox = filebox;
	set_tooltip(_("Display icons"));
}
int BC_FileBoxIcons::handle_event()
{
	filebox->create_listbox(filebox->listbox->get_x(), filebox->listbox->get_y(), LISTBOX_ICONS);
	filebox->listbox->show_window(1);
	return 1;
}


BC_FileBoxNewfolder::BC_FileBoxNewfolder(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_newfolder_images)
{
	this->filebox = filebox;
	set_tooltip(_("Create new folder"));
}
int BC_FileBoxNewfolder::handle_event()
{
	filebox->newfolder_thread->start_new_folder();
	return 1;
}


BC_FileBoxRename::BC_FileBoxRename(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_rename_images)
{
	this->filebox = filebox;
	set_tooltip(_("Rename file"));
}
int BC_FileBoxRename::handle_event()
{
	filebox->rename_thread->start_rename();
	return 1;
}

BC_FileBoxUpdir::BC_FileBoxUpdir(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_updir_images)
{
	this->filebox = filebox;
	set_tooltip(_("Up a directory"));
}
int BC_FileBoxUpdir::handle_event()
{
// Need a temp so submit_file can expand it
	sprintf(string, "..");
	filebox->submit_file(string);
	return 1;
}

BC_FileBoxDelete::BC_FileBoxDelete(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_delete_images)
{
	this->filebox = filebox;
	set_tooltip(_("Delete files"));
}
int BC_FileBoxDelete::handle_event()
{
	filebox->unlock_window();
	filebox->delete_thread->start();
	filebox->lock_window("BC_FileBoxDelete::handle_event");
	return 1;
}

BC_FileBoxReload::BC_FileBoxReload(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_reload_images)
{
	this->filebox = filebox;
	set_tooltip(_("Refresh"));
}
int BC_FileBoxReload::handle_event()
{
	filebox->refresh();
	return 1;
}



BC_FileBox::BC_FileBox(int x, int y, const char *init_path,
		const char *title, const char *caption, int show_all_files,
		int want_directory, int multiple_files, int h_padding)
 : BC_Window(title, x, y,
 	BC_WindowBase::get_resources()->filebox_w,
	BC_WindowBase::get_resources()->filebox_h,
	400, 300, 1, 0, 1)
{
	fs = new FileSystem;
// 	if(want_directory)
// 	{
// 		fs->set_want_directory();
// 		columns = DIRBOX_COLUMNS;
// 		columns = FILEBOX_COLUMNS;
// 	}
// 	else
	{
		columns = FILEBOX_COLUMNS;
	}

	list_column = new ArrayList<BC_ListBoxItem*>[columns];
	column_type = new int[columns];
	column_width = new int[columns];

	filter_title = 0;
	filter_text = 0;
	filter_popup = 0;
	usethis_button = 0;
	size_format = BC_WindowBase::get_resources()->filebox_size_format;

	strcpy(this->caption, caption);
	strcpy(this->current_path, init_path);
	strcpy(this->submitted_path, init_path);
	select_multiple = multiple_files;
	this->want_directory = want_directory;
	if(show_all_files) fs->set_show_all();
	fs->complete_path(this->current_path);
	strcpy(this->submitted_path, this->current_path);
	fs->extract_dir(directory, this->current_path);
	fs->extract_name(filename, this->current_path);

// printf("BC_FileBox::BC_FileBox %d '%s' '%s' '%s'\n",
// __LINE__,
// this->submitted_path,
// directory,
// filename);

// 	if(want_directory)
// 	{
// 		for(int i = 0; i < columns; i++)
// 		{
// 			column_type[i] = get_resources()->dirbox_columntype[i];
// 			column_width[i] = get_resources()->dirbox_columnwidth[i];
// 			column_titles[i] = BC_FileBox::columntype_to_text(column_type[i]);
// 		}
// 		sort_column = get_resources()->dirbox_sortcolumn;
// 		sort_order = get_resources()->dirbox_sortorder;
// 	}
// 	else
	{
		for(int i = 0; i < columns; i++)
		{
			column_type[i] = get_resources()->filebox_columntype[i];
			column_width[i] = get_resources()->filebox_columnwidth[i];
			column_titles[i] = (char*)BC_FileBox::columntype_to_text(column_type[i]);
		}
		sort_column = get_resources()->filebox_sortcolumn;
		sort_order = get_resources()->filebox_sortorder;
	}



// Test if current directory exists
	if(!fs->is_dir(directory))
	{
		sprintf(directory, "~");
		fs->complete_path(directory);
		strcpy(current_path,directory);
		filename[0] = 0;
	}
	fs->set_current_dir(directory);

	if(h_padding == -1)
	{
		h_padding = BC_WindowBase::get_resources()->ok_images[0]->get_h() -
			20;
	}
	this->h_padding = h_padding;
	delete_thread = new BC_DeleteThread(this);
	rename_thread = 0;
}

BC_FileBox::~BC_FileBox()
{
// this has to be destroyed before tables, because it can call for an update!
	delete newfolder_thread;
	delete fs;
	delete_tables();
	for(int i = 0; i < TOTAL_ICONS; i++)
		delete icons[i];
	filter_list.remove_all_objects();
	delete [] list_column;
	delete [] column_type;
	delete [] column_width;
	delete delete_thread;
	delete rename_thread;
	recent_dirs.remove_all_objects();
}

void BC_FileBox::create_objects()
{
	lock_window("BC_FileBox::create_objects");
	int x = 10, y = 10;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int directory_title_margin = MAX(20,
		resources->filebox_text_images[0]->get_h());

// Directories aren't filtered in FileSystem so skip this
	if(!want_directory)
	{
		filter_list.append(new BC_ListBoxItem("*"));
		filter_list.append(new BC_ListBoxItem("[*.mkv][*.webm]"));
		filter_list.append(new BC_ListBoxItem("[*.mp4][*.MP4]"));
		filter_list.append(new BC_ListBoxItem("[*.mp2][*.mp3][*.wav]"));
		filter_list.append(new BC_ListBoxItem("[*.avi][*.mpg][*.m2v][*.m1v][*.mov]"));
		filter_list.append(new BC_ListBoxItem("[*.jpg][*.JPG][*.png][*.gif][*.tiff]"));
		filter_list.append(new BC_ListBoxItem("*.xml"));
		fs->set_filter(get_resources()->filebox_filter);
	}

	create_icons();
	create_tables(0);

	add_subwindow(ok_button = new BC_FileBoxOK(this));
	if(want_directory)
		add_subwindow(usethis_button = new BC_FileBoxUseThis(this));
	add_subwindow(cancel_button = new BC_FileBoxCancel(this));

	add_subwindow(new BC_Title(x, y, caption));

	x = get_w() - resources->filebox_icons_images[0]->get_w() - 10;

	add_subwindow(icon_button = new BC_FileBoxIcons(x, y, this));
	x -= resources->filebox_text_images[0]->get_w() + 5;

	add_subwindow(text_button = new BC_FileBoxText(x, y, this));
	x -= resources->filebox_newfolder_images[0]->get_w() + 5;

	add_subwindow(folder_button = new BC_FileBoxNewfolder(x, y, this));
	x -= resources->filebox_delete_images[0]->get_w() + 5;

	add_subwindow(rename_button = new BC_FileBoxRename(x, y, this));
	x -= resources->filebox_delete_images[0]->get_w() + 5;

	add_subwindow(delete_button = new BC_FileBoxDelete(x, y, this));
	x -= resources->filebox_reload_images[0]->get_w() + 5;

	add_subwindow(reload_button = new BC_FileBoxReload(x, y, this));
	x -= resources->filebox_updir_images[0]->get_w() + 5;

	add_subwindow(updir_button = new BC_FileBoxUpdir(x, y, this));
	x -= resources->filebox_szfmt_images[0]->get_w() + 5;

	add_subwindow(szfmt_button = new BC_FileBoxSizeFormat(x, y, this));

	x = 10;
	y += directory_title_margin + 3;

	add_subwindow(recent_popup = new BC_FileBoxRecent(this, x, y));
	add_subwindow(directory_title = new BC_FileBoxDirectoryText(x, y, this));
	directory_title->reposition_window(x, y, get_w() - recent_popup->get_w() -  20, 1);
	x += directory_title->get_w() + 8;
	recent_popup->reposition_window(x, y, directory_title->get_w(), 200);

	x = 10;
	y += directory_title->get_h() + 5;

	BC_Title *search_title;
	add_subwindow(search_title = new BC_Title(x, y, _("Search:")));
	x += search_title->get_w() + 10;
	add_subwindow(search_text = new BC_FileBoxSearchText(x, y, this));

	x = 10;
	y += search_text->get_h() + 5;

	int newest_id = 0, newest = -1;
	for(int i = 0; i < FILEBOX_HISTORY_SIZE; i++) {
		if( !resources->filebox_history[i].path[0] ) continue;
		if( resources->filebox_history[i].id > newest_id ) {
			newest_id = resources->filebox_history[i].id;
			newest = i;
		}
	}
	if( newest >= 0 ) {
		strcpy(directory, resources->filebox_history[newest].path);
		fs->change_dir(directory, 0);
		strcpy(directory, fs->get_current_dir());
		directory_title->update(fs->get_current_dir());
	}
	fs->set_sort_order(sort_order);
	fs->set_sort_field(column_type[sort_column]);
	fs->update(directory);

// Create recent dir list
	create_history();
	update_history();

	listbox = 0;
	create_listbox(x, y, get_display_mode());
	y += listbox->get_h() + 10;
	add_subwindow(textbox = new BC_FileBoxTextBox(x, y, this));
	y += textbox->get_h() + 10;

	if(!want_directory) {
		add_subwindow(filter_title = new BC_Title(x, y, _("Specify filter:")));
		int x1 = x + filter_title->get_w() + 10;
		add_subwindow(filter_text = new BC_FileBoxFilterText(x1, y, this));
		add_subwindow(filter_popup =
			new BC_FileBoxFilterMenu(x1 + filter_text->get_w(), y, this));
	}

// listbox has to be active because refresh might be called from newfolder_thread
 	listbox->activate();
	newfolder_thread = new BC_NewFolderThread(this);

	rename_thread = new BC_RenameThread(this);

	refresh();
	show_window();
	unlock_window();
}

int BC_FileBox::get_listbox_w()
{
	return get_w() - 20;
}

int BC_FileBox::get_listbox_h(int y)
{
	int result = get_h() - y - h_padding - 10;
	if(want_directory)
		result -= BC_WindowBase::get_resources()->dirbox_margin;
	else
		result -= BC_WindowBase::get_resources()->filebox_margin;

	return result;
}

int BC_FileBox::create_icons()
{
	for(int i = 0; i < TOTAL_ICONS; i++)
	{
		icons[i] = new BC_Pixmap(this,
			BC_WindowBase::get_resources()->type_to_icon[i],
			PIXMAP_ALPHA);
	}
	return 0;
}

int BC_FileBox::resize_event(int w, int h)
{
	draw_background(0, 0, w, h);
	flash(0);

// OK button handles resize event itself
// 	ok_button->reposition_window(ok_button->get_x(),
// 		h - (get_h() - ok_button->get_y()));
// 	cancel_button->reposition_window(w - (get_w() - cancel_button->get_x()),
// 		h - (get_h() - cancel_button->get_y()));
	if(usethis_button)
		usethis_button->reposition_window(w / 2 - 50,
			h - (get_h() - usethis_button->get_y()));


	if(filter_popup) filter_popup->reposition_window(w - (get_w() - filter_popup->get_x()),
		h - (get_h() - filter_popup->get_y()),
		w - 30,
		0);

	if(filter_title) filter_title->reposition_window(filter_title->get_x(),
		h - (get_h() - filter_title->get_y()));
	if(filter_text) filter_text->reposition_window(filter_text->get_x(),
		h - (get_h() - filter_text->get_y()),
		w - (get_w() - filter_text->get_w()),
		1);
	directory_title->reposition_window(
		directory_title->get_x(),
		directory_title->get_y(),
		get_w() - recent_popup->get_w() -  20,
		1);
	recent_popup->reposition_window(
		directory_title->get_x() + directory_title->get_w() + 8,
		directory_title->get_y(),
		directory_title->get_w() + recent_popup->get_w(),
		recent_popup->get_h());
	search_text->reposition_window(
		search_text->get_x(),
		search_text->get_y(),
		get_w() - search_text->get_x() -  40,
		1);
	textbox->reposition_window(textbox->get_x(),
		h - (get_h() - textbox->get_y()),
		w - (get_w() - textbox->get_w()),
		1);
	listbox->reposition_window(listbox->get_x(),
		listbox->get_y(),
		w - (get_w() - listbox->get_w()),
		h - (get_h() - listbox->get_h()),
		0);
	int dx = w - get_w();
	icon_button->reposition_window(icon_button->get_x()+dx, icon_button->get_y());
	text_button->reposition_window(text_button->get_x()+dx, text_button->get_y());
	folder_button->reposition_window(folder_button->get_x()+dx, folder_button->get_y());
	rename_button->reposition_window(rename_button->get_x()+dx, rename_button->get_y());
	reload_button->reposition_window(reload_button->get_x()+dx, reload_button->get_y());
	delete_button->reposition_window(delete_button->get_x()+dx, delete_button->get_y());
	updir_button->reposition_window(updir_button->get_x()+dx, updir_button->get_y());
	szfmt_button->reposition_window(szfmt_button->get_x()+dx, szfmt_button->get_y());
	set_w(w);  set_h(h);
	get_resources()->filebox_w = get_w();
	get_resources()->filebox_h = get_h();
	flush();
	return 1;
}

int BC_FileBox::keypress_event()
{
	switch(get_keypress()) {
	case 'a':
		if( !ctrl_down() ) break;
		refresh(0, 1);
		return 1;
	case 'z':
		if( !ctrl_down() ) break;
		refresh(0, 0);
		return 1;
	case 'w':
		if( !ctrl_down() ) break;
		set_done(1);
		return 1;
	}
	return 0;
}

int BC_FileBox::close_event()
{
	set_done(1);
	return 1;
}

int BC_FileBox::handle_event()
{
	return 0;
}

int BC_FileBox::extract_extension(char *out, const char *in)
{
	*out = 0;
	int i = strlen(in);
	while( --i>=0 && in[i]!='.' );
	if( i >= 0 ) strcpy(out, &in[++i]);
	return 0;
}

static inline int64_t ipow(int m, int n)
{
	int64_t v = 1;
	for( int64_t vv=m; n>0; vv*=vv,n>>=1 ) if( n & 1 ) v *= vv;
	return v;
}
static inline int ilen(int64_t v)
{
        int len = 1;
        while( len<16 && (v/=10)>0 ) ++len;
        return len;
}

int BC_FileBox::create_tables(int select_all)
{
	int preload_textbox = select_all;
	delete_tables();
	char string[BCTEXTLEN];
	BC_ListBoxItem *new_item;

	fs->set_sort_order(sort_order);
	fs->set_sort_field(column_type[sort_column]);

	for(int i = 0; i < fs->total_files(); i++)
	{
		FileItem *file_item = fs->get_entry(i);
		const char *text = search_text->get_text();
		if( text && text[0] && !bstrcasestr(file_item->name, text) )
			continue;
		int is_dir = file_item->is_dir;
		BC_Pixmap* icon = get_icon(file_item->name, is_dir);

// Name entry
		new_item = new BC_ListBoxItem(file_item->name,
			icon,
			is_dir ? get_resources()->directory_color : get_resources()->file_color);
		if(is_dir) new_item->set_searchable(0);
		list_column[column_of_type(FILEBOX_NAME)].append(new_item);

// Size entry
// 		if(!want_directory)
// 		{
			if(!is_dir)
			{
				if( preload_textbox ) {
					preload_textbox = 0;
					textbox->update(new_item->get_text());
				}
				int64_t size = file_item->size;
				if( (size_format == FILEBOX_SIZE_1000 && size >= 1000) ||
				    (size_format == FILEBOX_SIZE_1024 && size >= 1024) ) {
					static const char *suffix[] = { "", "K", "M", "G", "T", "P" };
					if( size_format == FILEBOX_SIZE_1024 ) {
						static const long double kk = logl(1000.)/logl(1024.);
				                size = expl(kk*logl((long double)size)) + 0.5;
					}
					int len = ilen(size), drop = len-3, round = 1;
					if( round && drop > 0 ) { //round
						size += ipow(10,drop)/2;
						len = ilen(size);  drop = len-3;
					}
					size /= ipow(10,drop);
					int sfx = (len-1)/3;
					int digits = (sfx+1)*3 - len;
					int64_t frac = ipow(10,digits);
					int mant  = size / frac;
					int fraction = size - mant*frac;
					sfx = *suffix[sfx];
					if( sfx && size_format == FILEBOX_SIZE_1000 ) sfx += 'a'-'A';
					if( digits )
						sprintf(string, "%d.%0*d%c", mant, digits, fraction, sfx);
					else
						sprintf(string, "%d%c", mant, sfx);
				}
				else {
					sprintf(string, "%jd", size);
					if( size_format == FILEBOX_SIZE_THOU )
						Units::punctuate(string);
				}
				new_item = new BC_ListBoxItem(string, get_resources()->file_color);
			}
			else
			{
				new_item = new BC_ListBoxItem("", get_resources()->directory_color);
			}

	 		list_column[column_of_type(FILEBOX_SIZE)].append(new_item);
//		}

// Date entry
		if(!is_dir || 1)
		{
			struct tm mod_time;
			localtime_r(&file_item->mtime, &mod_time);
			sprintf(string, "%04d.%02d.%02d  %02d:%02d:%02d",
				mod_time.tm_year+1900, mod_time.tm_mon+1, mod_time.tm_mday,
				mod_time.tm_hour, mod_time.tm_min, mod_time.tm_sec);
			new_item = new BC_ListBoxItem(string, get_resources()->file_color);
		}
		else
		{
			new_item = new BC_ListBoxItem("", get_resources()->directory_color);
		}

		list_column[column_of_type(FILEBOX_DATE)].append(new_item);

// Extension entry
//		if(!want_directory)
//		{
			if(!is_dir) {
				extract_extension(string, file_item->name);
				new_item = new BC_ListBoxItem(string, get_resources()->file_color);
			}
			else {
				new_item = new BC_ListBoxItem("", get_resources()->directory_color);
			}
			list_column[column_of_type(FILEBOX_EXTENSION)].append(new_item);
//		}

		if( !is_dir && select_all ) {
			int k = list_column[0].size()-1;
			for( int j=0; j<columns; ++j )
				list_column[j][k]->set_selected(1);
		}
	}

	return 0;
}

int BC_FileBox::delete_tables()
{
	for(int j = 0; j < columns; j++)
	{
		list_column[j].remove_all_objects();
	}
	return 0;
}

BC_Pixmap* BC_FileBox::get_icon(char *path, int is_dir)
{
	if( is_dir ) return icons[ICON_FOLDER];
	int icon_type = ICON_UNKNOWN;
	char *suffix = strrchr(path, '.');
	if( suffix && *++suffix ) {
		suffix_to_type_t *stp = &BC_WindowBase::get_resources()->suffix_to_type[0];
		while( stp->suffix && strcasecmp(stp->suffix, suffix) ) ++stp;
		if( stp->icon_type ) icon_type = stp->icon_type;
	}

	return icons[icon_type];
}

const char* BC_FileBox::columntype_to_text(int type)
{
	switch(type) {
		case FILEBOX_NAME: return FILEBOX_NAME_TEXT;
		case FILEBOX_SIZE: return FILEBOX_SIZE_TEXT;
		case FILEBOX_DATE: return FILEBOX_DATE_TEXT;
		case FILEBOX_EXTENSION: return FILEBOX_EXTENSION_TEXT;
	}
	return "";
}

int BC_FileBox::column_of_type(int type)
{
	for(int i = 0; i < columns; i++)
		if(column_type[i] == type) return i;
	return 0;
}



int BC_FileBox::refresh(int reset, int select_all)
{
	fs->set_sort_order(sort_order);
	fs->set_sort_field(column_type[sort_column]);
	if( reset >= 0 )
		fs->update(0);
	else
		fs->update_sort();
	create_tables(select_all);
	listbox->set_master_column(column_of_type(FILEBOX_NAME), 0);
	listbox->update(list_column, column_titles, column_width, columns,
		reset>0 ? 0 : listbox->get_xposition(),
		reset>0 ? 0 : listbox->get_yposition(),
		-1, 1);
	return 0;
}

int BC_FileBox::update_filter(const char *filter)
{
	fs->set_filter(filter);
//	fs->update(0);
	refresh();
	strcpy(get_resources()->filebox_filter, filter);

	return 0;
}


void BC_FileBox::move_column(int src, int dst)
{
	if(src != dst)
	{
		ArrayList<BC_ListBoxItem*> *new_columns =
			new ArrayList<BC_ListBoxItem*>[columns];
		int *new_types = new int[columns];
		int *new_widths = new int[columns];

	// Fill in remaining columns with consecutive data
		for(int out_column = 0, in_column = 0;
			out_column < columns;
			out_column++,
			in_column++)
		{
	// Copy destination column from src column
			if(out_column == dst)
			{
				for(int i = 0; i < list_column[src].total; i++)
				{
					new_columns[out_column].append(list_column[src].values[i]);
				}
				new_types[out_column] = column_type[src];
				new_widths[out_column] = column_width[src];
				in_column--;
			}
			else
			{
	// Skip source column
				if(in_column == src) in_column++;
				for(int i = 0; i < list_column[src].total; i++)
				{
					new_columns[out_column].append(list_column[in_column].values[i]);
				}
				new_types[out_column] = column_type[in_column];
				new_widths[out_column] = column_width[in_column];
			}
		}

	// Swap tables
		delete [] list_column;
		delete [] column_type;
		delete [] column_width;
		list_column = new_columns;
		column_type = new_types;
		column_width = new_widths;

		for(int i = 0; i < columns; i++)
		{
			get_resources()->filebox_columntype[i] = column_type[i];
			get_resources()->filebox_columnwidth[i] = column_width[i];
			column_titles[i] = (char*)BC_FileBox::columntype_to_text(column_type[i]);
		}
	}

	refresh();
}


int BC_FileBox::submit_dir(char *dir)
{
	strcpy(directory, dir);
	fs->join_names(current_path, directory, filename);

// printf("BC_FileBox::submit_dir %d '%s' '%s' '%s'\n",
// __LINE__,
// current_path,
// directory,
// filename);
	strcpy(submitted_path, current_path);
	fs->change_dir(dir, 0);
	refresh(1);
	directory_title->update(fs->get_current_dir());
	if(want_directory)
		textbox->update(fs->get_current_dir());
	else
		textbox->update(filename);
	listbox->reset_query();
	return 0;
}

int BC_FileBox::submit_file(const char *path, int use_this)
{
	char path1[BCTEXTLEN];
	strcpy(path1, path);
	char *cp = strchr(path1,'\n');
	if( cp ) *cp = 0;

// Deactivate textbox to hide suggestions
	textbox->deactivate();

// If file wanted, take the current directory as the desired file.
// If directory wanted, ignore it.
	if(!path1[0] && !want_directory)
	{
// save complete path
		strcpy(this->current_path, directory);
// save complete path
		strcpy(this->submitted_path, directory);
		update_history();
// Zero out filename
		filename[0] = 0;
		set_done(0);
		return 0;
	}

// is a directory, change directories
	if(fs->is_dir(path1) && !use_this)
	{
		fs->change_dir(path1, 0);
		refresh(1);
		directory_title->update(fs->get_current_dir());
		strcpy(this->current_path, fs->get_current_dir());
		strcpy(this->submitted_path, fs->get_current_dir());
		strcpy(this->directory, fs->get_current_dir());
		update_history();
		filename[0] = 0;
		if(want_directory)
			textbox->update(fs->get_current_dir());
		else
			textbox->update("");
		listbox->reset_query();
		return 1;
	}
	else
// Is a file or desired directory.  Quit the operation.
	{
		char path2[BCTEXTLEN];
		strcpy(path2, path1);

// save directory for defaults
		fs->extract_dir(directory, path2);

// Just take the directory
		if(want_directory)
		{
			filename[0] = 0;
			strcpy(path2, directory);
		}
		else
// Take the complete path
		{
			fs->extract_name(filename, path2);     // save filename
		}

		fs->complete_path(path2);
		strcpy(this->current_path, path2);          // save complete path
		strcpy(this->submitted_path, path2);          // save complete path
		update_history();
		newfolder_thread->interrupt();
		set_done(0);
		return 0;
	}
	return 0;
}

void BC_FileBox::update_history()
{
// Look for path already in history
	BC_Resources *resources = get_resources();
	char path[BCTEXTLEN];
	strcpy(path, directory);
// enfore one trailing slash
	char *cp = path;
	while( *cp && *cp != '\n' ) ++cp;
	while( cp > path && *(cp-1) == '/' ) --cp;
	*cp++ = '/';  *cp = 0;

//	int new_slot = FILEBOX_HISTORY_SIZE - 1;

	for(int i = FILEBOX_HISTORY_SIZE - 1; i >= 0; i--)
	{
		if(resources->filebox_history[i].path[0] &&
			!strcmp(resources->filebox_history[i].path, path))
		{
// Got matching path.
// Update ID.
			resources->filebox_history[i].id = resources->get_filebox_id();
			return;
		}
// // Shift down from this point.
// 			while(i > 0)
// 			{
// 				strcpy(resources->filebox_history[i],
// 					resources->filebox_history[i - 1]);
// 				if(resources->filebox_history[i][0]) new_slot--;
// 				i--;
// 			}
// 			break;
// 		}
// 		else
// 			if(resources->filebox_history[i][0])
// 				new_slot--;
// 		else
// 			break;
	}

// Remove oldest entry if full
	if(resources->filebox_history[FILEBOX_HISTORY_SIZE - 1].path[0])
	{
		int oldest_id = 0x7fffffff;
		int oldest = 0;
		for(int i = 0; i < FILEBOX_HISTORY_SIZE; i++)
		{
			if(resources->filebox_history[i].path[0] &&
				resources->filebox_history[i].id < oldest_id)
			{
				oldest_id = resources->filebox_history[i].id;
				oldest = i;
			}
		}

		for(int i = oldest; i < FILEBOX_HISTORY_SIZE - 1; i++)
		{
			strcpy(resources->filebox_history[i].path,
				resources->filebox_history[i + 1].path);
			resources->filebox_history[i].id =
				resources->filebox_history[i + 1].id;
		}
	}

// Create new entry
	strcpy(resources->filebox_history[FILEBOX_HISTORY_SIZE - 1].path, path);
	resources->filebox_history[FILEBOX_HISTORY_SIZE - 1].id = resources->get_filebox_id();

// Alphabetize
	int done = 0;
	while(!done)
	{
		done = 1;
		for(int i = 1; i < FILEBOX_HISTORY_SIZE; i++)
		{
			if( (resources->filebox_history[i - 1].path[0] &&
				resources->filebox_history[i].path[0] &&
				strcasecmp(resources->filebox_history[i - 1].path,
					resources->filebox_history[i].path) > 0) ||
				(resources->filebox_history[i - 1].path[0] == 0 &&
					resources->filebox_history[i].path[0]) )
			{
				done = 0;
				char temp[BCTEXTLEN];
				int id_temp;
				strcpy(temp, resources->filebox_history[i - 1].path);
				id_temp = resources->filebox_history[i - 1].id;
				strcpy(resources->filebox_history[i - 1].path,
					resources->filebox_history[i].path);
				resources->filebox_history[i - 1].id =
					resources->filebox_history[i].id;
				strcpy(resources->filebox_history[i].path, temp);
				resources->filebox_history[i].id = id_temp;
			}
		}
	}

// 	if(new_slot < 0)
// 	{
// 		for(int i = FILEBOX_HISTORY_SIZE - 1; i > 0; i--)
// 		{
// 			strcpy(resources->filebox_history[i],
// 					resources->filebox_history[i - 1]);
// 		}
// 		new_slot = 0;
// 	}
//
// 	strcpy(resources->filebox_history[new_slot], path);

	create_history();
	recent_popup->update(&recent_dirs, 0, 0, 1);
}

void BC_FileBox::create_history()
{
	BC_Resources *resources = get_resources();
	recent_dirs.remove_all_objects();
	for(int i = 0; i < FILEBOX_HISTORY_SIZE; i++) {
		if(resources->filebox_history[i].path[0]) {
			recent_dirs.append(new BC_ListBoxItem(resources->filebox_history[i].path));
		}
	}
}


int BC_FileBox::get_display_mode()
{
	return top_level->get_resources()->filebox_mode;
}

void BC_FileBox::create_listbox(int x, int y, int mode)
{
	if(listbox && listbox->get_display_mode() != mode)
	{
		delete listbox;
		listbox = 0;
		top_level->get_resources()->filebox_mode = mode;
	}

	if(!listbox)
		add_subwindow(listbox = new BC_FileBoxListBox(x, y, this));
}

char* BC_FileBox::get_path(int selection)
{
	if(selection == 0)
	{
		return get_submitted_path();
	}
	else
	{
		BC_ListBoxItem *item = listbox->get_selection(
			column_of_type(FILEBOX_NAME), selection - 1);
		if(item)
		{
			fs->join_names(string, directory, item->get_text());
			return string;
		}
	}
	return 0;
}

char* BC_FileBox::get_submitted_path()
{
	return submitted_path;
}

char* BC_FileBox::get_current_path()
{
//printf("BC_FileBox::get_current_path 1 %s\n", current_path);
	return current_path;
}

char* BC_FileBox::get_newfolder_title()
{
	char *letter2 = strchr(title, ':');
	new_folder_title[0] = 0;
	if(letter2)
	{
		memcpy(new_folder_title, title, letter2 - title);
		new_folder_title[letter2 - title] = 0;
	}

	strcat(new_folder_title, _(": New folder"));

	return new_folder_title;
}

char* BC_FileBox::get_rename_title()
{
	char *letter2 = strchr(title, ':');
	new_folder_title[0] = 0;
	if(letter2)
	{
		memcpy(new_folder_title, title, letter2 - title);
		new_folder_title[letter2 - title] = 0;
	}

	strcat(new_folder_title, _(": Rename"));

	return new_folder_title;
}

char* BC_FileBox::get_delete_title()
{
	char *letter2 = strchr(title, ':');
	new_folder_title[0] = 0;
	if(letter2)
	{
		memcpy(new_folder_title, title, letter2 - title);
		new_folder_title[letter2 - title] = 0;
	}

	strcat(new_folder_title, _(": Delete"));

	return new_folder_title;
}

void BC_FileBox::delete_files()
{
// Starting at 1 causes it to ignore what's in the textbox.
	int i = 1;
	char *path;
	FileSystem fs;
	while((path = get_path(i)))
	{
// Not directory.  Remove it.
		if(!fs.is_dir(path))
		{
//printf("BC_FileBox::delete_files: removing \"%s\"\n", path);
			remove(path);
		}
		i++;
	}
	refresh();
}

BC_Button* BC_FileBox::get_ok_button()
{
	return ok_button;
}

BC_Button* BC_FileBox::get_cancel_button()
{
	return cancel_button;
}

