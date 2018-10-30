
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

#ifndef NEWFOLDER_H
#define NEWFOLDER_H

#include "arraylist.h"
#include "awindowgui.inc"
#include "binfolder.inc"
#include "datatype.h"
#include "edl.inc"
#include "filesystem.h"
#include "guicast.h"
#include "indexable.h"
#include "mutex.h"
#include "mwindow.inc"

enum {
	FOLDER_COLUMN_ENABLE,
	FOLDER_COLUMN_TARGET,
	FOLDER_COLUMN_OP,
	FOLDER_COLUMN_VALUE,
	FOLDER_COLUMNS
};

enum {
	FOLDER_ENABLED_OFF,
	FOLDER_ENABLED_AND,
	FOLDER_ENABLED_OR,
	FOLDER_ENABLED_AND_NOT,
	FOLDER_ENABLED_OR_NOT,
};
enum {
	FOLDER_TARGET_PATTERNS,
	FOLDER_TARGET_FILE_SIZE,
	FOLDER_TARGET_MOD_TIME,
	FOLDER_TARGET_TRACK_TYPE,
	FOLDER_TARGET_WIDTH,
	FOLDER_TARGET_HEIGHT,
	FOLDER_TARGET_FRAMERATE,
	FOLDER_TARGET_SAMPLERATE,
	FOLDER_TARGET_CHANNELS,
	FOLDER_TARGET_DURATION,
};
enum {
	FOLDER_OP_AROUND,
	FOLDER_OP_EQ, FOLDER_OP_GE, FOLDER_OP_GT,
	FOLDER_OP_NE, FOLDER_OP_LE, FOLDER_OP_LT,
	FOLDER_OP_MATCHES,
};

class BinFolderFilters : public ArrayList<BinFolderFilter *>
{
public:
	BinFolderFilters() {}
	~BinFolderFilters() { remove_all_objects(); }
	void copy_from(BinFolderFilters *filters);
	void clear() { remove_all_objects(); }
};

class BinFolder
{
public:
	BinFolder(int awindow_folder, int is_clips, const char *title);
	BinFolder(BinFolder &that);
	~BinFolder();

	BinFolderFilters filters;
	void save_xml(FileXML *file);
	int load_xml(FileXML *file);
	double matches_indexable(Indexable *idxbl);
	void copy_from(BinFolder *that);
	const char *get_idxbl_title(Indexable *idxbl);
	int add_patterns(ArrayList<Indexable*> *drag_assets, int use_basename);

	char title[BCSTRLEN];
	int awindow_folder;
	int is_clips;
};

class BinFolders : public ArrayList<BinFolder *>
{
public:
	BinFolders() {}
	~BinFolders() {}
	void save_xml(FileXML *file);
	int load_xml(FileXML *file);
	double matches_indexable(int folder, Indexable *idxbl);
	void copy_from(BinFolders *that);
	void clear() { remove_all_objects(); }
};

class BinFolderFilter
{
public:
	BinFolderFilter();
	~BinFolderFilter();

	void update_enabled(int type);
	void update_target(int type);
	void update_op(int type);
	void update_value(const char *text);
	void save_xml(FileXML *file);
	int load_xml(FileXML *file);

	BinFolderEnabled *enabled;
	BinFolderTarget *target;
	BinFolderOp *op;
	BinFolderValue *value;
};

class BinFolderEnabled : public BC_ListBoxItem
{
public:
	BinFolderEnabled(BinFolderFilter *filter, int type);
	virtual ~BinFolderEnabled();
	void update(int type);

	BinFolderFilter *filter;
	int type;
	static const char *types[];
};

class BinFolderEnabledType : public BC_MenuItem
{
public:
	BinFolderEnabledType(int no);
	~BinFolderEnabledType();
	int no;
	int handle_event();
};
class BinFolderEnabledPopup : public BC_PopupMenu
{
public:
	BinFolderEnabledPopup(BinFolderList *folder_list);
	void create_objects();
	void activate_menu(BC_ListBoxItem *item);

	BinFolderList *folder_list;
	BinFolderEnabled *enabled;
};

class BinFolderTarget : public BC_ListBoxItem
{
public:
	BinFolderTarget(BinFolderFilter *filter, int type);
	virtual ~BinFolderTarget();

	virtual void save_xml(FileXML *file) = 0;
	virtual void load_xml(FileXML *file) = 0;
	virtual void copy_from(BinFolderTarget *that) = 0;
	virtual BC_Window *new_gui(ModifyTargetThread *thread) = 0;

	BinFolderFilter *filter;
	int type;
	double around;
	static const char *types[];
};

class BinFolderTargetPatterns : public BinFolderTarget
{
public:
	BinFolderTargetPatterns(BinFolderFilter *filter);
	~BinFolderTargetPatterns();

	void save_xml(FileXML *file);
	void load_xml(FileXML *file);
	void copy_from(BinFolderTarget *that);
	BC_Window *new_gui(ModifyTargetThread *thread);
	void update(const char *text);

	char *text;
};

class BinFolderTargetFileSize : public BinFolderTarget
{
public:
	BinFolderTargetFileSize(BinFolderFilter *filter);
	~BinFolderTargetFileSize();

	void save_xml(FileXML *file);
	void load_xml(FileXML *file);
	void copy_from(BinFolderTarget *that);
	BC_Window *new_gui(ModifyTargetThread *thread);
	void update(int64_t file_size, double around);

	int64_t file_size;
};

class BinFolderTargetTime : public BinFolderTarget
{
public:
	BinFolderTargetTime(BinFolderFilter *filter);
	~BinFolderTargetTime();

	void save_xml(FileXML *file);
	void load_xml(FileXML *file);
	void copy_from(BinFolderTarget *that);
	BC_Window *new_gui(ModifyTargetThread *thread);
	void update(int64_t mtime, double around);

	int64_t mtime;
};

class BinFolderTargetTrackType : public BinFolderTarget
{
public:
	BinFolderTargetTrackType(BinFolderFilter *filter);
	~BinFolderTargetTrackType();

	void save_xml(FileXML *file);
	void load_xml(FileXML *file);
	void copy_from(BinFolderTarget *that);
	BC_Window *new_gui(ModifyTargetThread *thread);
	void update(int data_types);

	int data_types;
};

class BinFolderTargetWidth : public BinFolderTarget
{
public:
	BinFolderTargetWidth(BinFolderFilter *filter);
	~BinFolderTargetWidth();

	void save_xml(FileXML *file);
	void load_xml(FileXML *file);
	void copy_from(BinFolderTarget *that);
	BC_Window *new_gui(ModifyTargetThread *thread);
	void update(int width, double around);

	int width;
};

class BinFolderTargetHeight : public BinFolderTarget
{
public:
	BinFolderTargetHeight(BinFolderFilter *filter);
	~BinFolderTargetHeight();

	void save_xml(FileXML *file);
	void load_xml(FileXML *file);
	void copy_from(BinFolderTarget *that);
	BC_Window *new_gui(ModifyTargetThread *thread);
	void update(int height, double around);

	int height;
};

class BinFolderTargetFramerate : public BinFolderTarget
{
public:
	BinFolderTargetFramerate(BinFolderFilter *filter);
	~BinFolderTargetFramerate();

	void save_xml(FileXML *file);
	void load_xml(FileXML *file);
	void copy_from(BinFolderTarget *that);
	BC_Window *new_gui(ModifyTargetThread *thread);
	void update(double framerate, double around);

	double framerate;
};

class BinFolderTargetSamplerate : public BinFolderTarget
{
public:
	BinFolderTargetSamplerate(BinFolderFilter *filter);
	~BinFolderTargetSamplerate();

	void save_xml(FileXML *file);
	void load_xml(FileXML *file);
	void copy_from(BinFolderTarget *that);
	BC_Window *new_gui(ModifyTargetThread *thread);
	void update(int samplerate, double around);

	int samplerate;
};

class BinFolderTargetChannels : public BinFolderTarget
{
public:
	BinFolderTargetChannels(BinFolderFilter *filter);
	~BinFolderTargetChannels();

	void save_xml(FileXML *file);
	void load_xml(FileXML *file);
	void copy_from(BinFolderTarget *that);
	BC_Window *new_gui(ModifyTargetThread *thread);
	void update(int channels, double around);

	int channels;
};

class BinFolderTargetDuration : public BinFolderTarget
{
public:
	BinFolderTargetDuration(BinFolderFilter *filter);
	~BinFolderTargetDuration();

	void save_xml(FileXML *file);
	void load_xml(FileXML *file);
	void copy_from(BinFolderTarget *that);
	BC_Window *new_gui(ModifyTargetThread *thread);
	void update(int64_t duration, double around);

	int64_t duration;
};

class BinFolderTargetType : public BC_MenuItem
{
public:
	BinFolderTargetType(int no);
	~BinFolderTargetType();
	int no;
	int handle_event();
};

class BinFolderTargetPopup : public BC_PopupMenu
{
public:
	BinFolderTargetPopup(BinFolderList *folder_list);
	void create_objects();
	void activate_menu(BC_ListBoxItem *item);

	BinFolderList *folder_list;
	BinFolderTarget *target;
};


class BinFolderOp : public BC_ListBoxItem
{
public:
	BinFolderOp(BinFolderFilter *filter, int type);
	virtual ~BinFolderOp();

	double compare(BinFolderTarget *target, Indexable *idxbl);
	virtual double test(BinFolderTarget *target, Indexable *idxbl);
	double around(double v, double a);
	double around(const char *ap, const char *bp);

	void copy_from(BinFolderOp *that);

	BinFolderFilter *filter;
	int type;
	static const char *types[];
};

class BinFolderOpType : public BC_MenuItem
{
public:
	BinFolderOpType(int no);
	~BinFolderOpType();
	int no;
	int handle_event();
};

class BinFolderOpPopup : public BC_PopupMenu
{
public:
	BinFolderOpPopup(BinFolderList *folder_list);
	void create_objects();
	void activate_menu(BC_ListBoxItem *item);

	BinFolderList *folder_list;
	BinFolderOp *op;
};


class BinFolderOpAround : public BinFolderOp
{
public:
	BinFolderOpAround(BinFolderFilter *filter)
	 : BinFolderOp(filter, FOLDER_OP_AROUND) {}
	~BinFolderOpAround() {}
	double test(BinFolderTarget *target, Indexable *idxbl);
};

class BinFolderOpEQ : public BinFolderOp
{
public:
	BinFolderOpEQ(BinFolderFilter *filter)
	 : BinFolderOp(filter, FOLDER_OP_EQ) {}
	~BinFolderOpEQ() {}
	double test(BinFolderTarget *target, Indexable *idxbl);
};

class BinFolderOpGT : public BinFolderOp
{
public:
	BinFolderOpGT(BinFolderFilter *filter)
	 : BinFolderOp(filter, FOLDER_OP_GT) {}
	~BinFolderOpGT() {}
	double test(BinFolderTarget *target, Indexable *idxbl);
};

class BinFolderOpGE : public BinFolderOp
{
public:
	BinFolderOpGE(BinFolderFilter *filter)
	 : BinFolderOp(filter, FOLDER_OP_GE) {}
	~BinFolderOpGE() {}
	double test(BinFolderTarget *target, Indexable *idxbl);
};

class BinFolderOpNE : public BinFolderOp
{
public:
	BinFolderOpNE(BinFolderFilter *filter)
	 : BinFolderOp(filter, FOLDER_OP_NE) {}
	~BinFolderOpNE() {}
	double test(BinFolderTarget *target, Indexable *idxbl);
};

class BinFolderOpLT : public BinFolderOp
{
public:
	BinFolderOpLT(BinFolderFilter *filter)
	 : BinFolderOp(filter, FOLDER_OP_LT) {}
	~BinFolderOpLT() {}
	double test(BinFolderTarget *target, Indexable *idxbl);
};

class BinFolderOpLE : public BinFolderOp
{
public:
	BinFolderOpLE(BinFolderFilter *filter)
	 : BinFolderOp(filter, FOLDER_OP_LE) {}
	~BinFolderOpLE() {}
	double test(BinFolderTarget *target, Indexable *idxbl);
};

class BinFolderOpMatches : public BinFolderOp
{
public:
	BinFolderOpMatches(BinFolderFilter *filter)
	 : BinFolderOp(filter, FOLDER_OP_MATCHES) {}
	~BinFolderOpMatches() {}
	double test(BinFolderTarget *target, Indexable *idxbl);
};

class BinFolderValue : public BC_ListBoxItem
{
public:
	BinFolderValue(BinFolderFilter *filter, const char *text);
	virtual ~BinFolderValue();

	void update(const char *text);

	BinFolderFilter *filter;
};


class BinFolderList : public BC_ListBox
{
public:
	BinFolderList(BinFolder *folder, MWindow *mwindow,
		ModifyFolderGUI *window, int x, int y, int w, int h);
	~BinFolderList();
	void create_objects();

	int handle_event();
	int handle_event(int column);
	int selection_changed();
	int column_resize_event();
	void create_list();
	void save_defaults(BC_Hash *defaults);
	void load_defaults(BC_Hash *defaults);
	void move_filter(int src, int dst);

	int drag_start_event();
	int drag_motion_event();
	int drag_stop_event();
	int dragging_item;

	BinFolder *folder;
	MWindow *mwindow;
	ModifyFolderGUI *window;
	ArrayList<BC_ListBoxItem *>list_items[FOLDER_COLUMNS];
	const char *list_titles[FOLDER_COLUMNS];
	int list_width[FOLDER_COLUMNS];

	BinFolderEnabledPopup *enabled_popup;
	BinFolderOpPopup *op_popup;
	BinFolderTargetPopup *target_popup;

	ModifyTargetThread *modify_target;
};

class BinFolderAddFilter : public BC_GenericButton
{
public:
	BinFolderAddFilter(BinFolderList *folder_list, int x, int y);
	~BinFolderAddFilter();
	int handle_event();
	BinFolderList *folder_list;
};

class BinFolderDelFilter : public BC_GenericButton
{
public:
	BinFolderDelFilter(BinFolderList *folder_list, int x, int y);
	~BinFolderDelFilter();
	int handle_event();
	BinFolderList *folder_list;
};

class BinFolderApplyFilter : public BC_GenericButton
{
public:
	BinFolderApplyFilter(BinFolderList *folder_list, int x, int y);
	~BinFolderApplyFilter();
	int handle_event();
	BinFolderList *folder_list;
};

class NewFolderGUI : public BC_Window
{
public:
	NewFolderGUI(NewFolderThread *thread, int x, int y, int w, int h);
	~NewFolderGUI();
	void create_objects();
	const char *get_text();

	NewFolderThread *thread;
	BinFolderList *folder_list;
	BC_TextBox *text_box;
};

class NewFolderThread : public BC_DialogThread
{
public:
	NewFolderThread(AWindowGUI *agui);
	~NewFolderThread();
	BC_Window *new_gui();
	void handle_done_event(int result);
	void handle_close_event(int result);
	void start(int x, int y, int w, int h, int is_clips);

	int wx, wy, ww, wh;
	int is_clips;
	AWindowGUI *agui;
	NewFolderGUI *window;
};


class ModifyFolderGUI : public BC_Window
{
public:
	ModifyFolderGUI(ModifyFolderThread *thread, int x, int y, int w, int h);
	~ModifyFolderGUI();
	void create_objects();
	int resize_event(int w, int h);
	const char *get_text();
	int receive_custom_xatoms(xatom_event *event);
	void async_update_filters();
	void update_filters();

	ModifyFolderThread *thread;
	BinFolderList *folder_list;
	BinFolderAddFilter *add_filter;
	BinFolderDelFilter *del_filter;
	BinFolderApplyFilter *apply_filter;
	BC_Title *text_title;
	BC_TextBox *text_box;
	Atom modify_folder_xatom;
	BC_OKButton *ok_button;
	BC_CancelButton *cancel_button;
};

class ModifyFolderThread : public BC_DialogThread
{
public:
	ModifyFolderThread(AWindowGUI *agui);
	~ModifyFolderThread();
	BC_Window *new_gui();
	void handle_done_event(int result);
	void handle_close_event(int result);
	void start(BinFolder *folder, int x, int y, int w, int h);

	int wx, wy, ww, wh;
	AWindowGUI *agui;
	BinFolder *original, *folder;
	ModifyFolderGUI *window;
	EDL *modify_edl;
};


class ModifyTargetGUI : public BC_Window
{
public:
	ModifyTargetGUI(ModifyTargetThread *thread, int allow_resize=0);
	~ModifyTargetGUI();
	virtual void create_objects() {}
	virtual void update() {}
	virtual int resize_event(int w, int h);
	void create_objects(BC_TextBox *&text_box);

	ModifyTargetThread *thread;
};

class ModifyTargetThread : public BC_DialogThread
{
public:
	ModifyTargetThread(BinFolderList *folder_list);
	~ModifyTargetThread();
	BC_Window *new_gui();
	void handle_done_event(int result);
	void handle_close_event(int result);
	void start(BinFolderTarget *target, int x, int y, int w, int h);

	int wx, wy, ww, wh;
	BinFolderList *folder_list;
	BinFolderTarget *target;
	ModifyTargetGUI *window;
};

class ModifyTargetPatternsGUI : public ModifyTargetGUI
{
public:
	ModifyTargetPatternsGUI(ModifyTargetThread *thread);
	~ModifyTargetPatternsGUI();
	void create_objects();
	int resize_event(int w, int h);
	void update();

	BC_ScrollTextBox *scroll_text_box;
	BC_OKButton *ok_button;
	BC_CancelButton *cancel_button;
	int text_rowsz;
};

class ModifyTargetFileSizeGUI : public ModifyTargetGUI
{
public:
	ModifyTargetFileSizeGUI(ModifyTargetThread *thread);
	~ModifyTargetFileSizeGUI();
	void create_objects();
	void update();

	BC_TextBox *text_box;
};

class ModifyTargetTimeGUI : public ModifyTargetGUI
{
public:
	ModifyTargetTimeGUI(ModifyTargetThread *thread);
	~ModifyTargetTimeGUI();
	void create_objects();
	void update();

	BC_TextBox *text_box;
};

class ModifyTargetTrackTypeGUI : public ModifyTargetGUI
{
public:
	ModifyTargetTrackTypeGUI(ModifyTargetThread *thread);
	~ModifyTargetTrackTypeGUI();
	void create_objects();
	void update();

	BC_TextBox *text_box;
};

class ModifyTargetWidthGUI : public ModifyTargetGUI
{
public:
	ModifyTargetWidthGUI(ModifyTargetThread *thread);
	~ModifyTargetWidthGUI();
	void create_objects();
	void update();

	BC_TextBox *text_box;
};

class ModifyTargetHeightGUI : public ModifyTargetGUI
{
public:
	ModifyTargetHeightGUI(ModifyTargetThread *thread);
	~ModifyTargetHeightGUI();
	void create_objects();
	void update();

	BC_TextBox *text_box;
};

class ModifyTargetFramerateGUI : public ModifyTargetGUI
{
public:
	ModifyTargetFramerateGUI(ModifyTargetThread *thread);
	~ModifyTargetFramerateGUI();
	void create_objects();
	void update();

	BC_TextBox *text_box;
};

class ModifyTargetSamplerateGUI : public ModifyTargetGUI
{
public:
	ModifyTargetSamplerateGUI(ModifyTargetThread *thread);
	~ModifyTargetSamplerateGUI();
	void create_objects();
	void update();

	BC_TextBox *text_box;
};

class ModifyTargetChannelsGUI : public ModifyTargetGUI
{
public:
	ModifyTargetChannelsGUI(ModifyTargetThread *thread);
	~ModifyTargetChannelsGUI();
	void create_objects();
	void update();

	BC_TextBox *text_box;
};

class ModifyTargetDurationGUI : public ModifyTargetGUI
{
public:
	ModifyTargetDurationGUI(ModifyTargetThread *thread);
	~ModifyTargetDurationGUI();
	void create_objects();
	void update();

	BC_TextBox *text_box;
};

#endif
