#ifndef __BDCREATE_H__
#define __BDCREATE_H__

#include "arraylist.h"
#include "batchrender.h"
#include "bcwindowbase.h"
#include "bcbutton.h"
#include "bcdialog.h"
#include "bclistboxitem.inc"
#include "bcmenuitem.h"
#include "bctextbox.h"
#include "browsebutton.h"
#include "mwindow.h"
#include "rescale.h"

#include "bdcreate.inc"


class CreateBD_MenuItem : public BC_MenuItem
{
public:
	CreateBD_MenuItem(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};


class CreateBD_Thread : public BC_DialogThread
{
	static const int64_t BD_SIZE;
	static const int BD_STREAMS, BD_WIDTH, BD_HEIGHT;
	static const double BD_ASPECT_WIDTH, BD_ASPECT_HEIGHT;
	static const double BD_WIDE_ASPECT_WIDTH, BD_WIDE_ASPECT_HEIGHT;
	static const int BD_MAX_BITRATE, BD_CHANNELS, BD_WIDE_CHANNELS;
	static const double BD_FRAMERATE, BD_SAMPLERATE, BD_KAUDIO_RATE;
	static const int BD_INTERLACE_MODE;
	static int get_udfs_mount(char *udfs, char *mopts, char *mntpt);
public:
	CreateBD_Thread(MWindow *mwindow);
	~CreateBD_Thread();
	void handle_close_event(int result);
	BC_Window* new_gui();
	int option_presets();
	int create_bd_jobs(ArrayList<BatchRenderJob*> *jobs, const char *asset_dir);
	int insert_video_plugin(const char *title, KeyFrame *default_keyframe);
	int resize_tracks();

	MWindow *mwindow;
	CreateBD_GUI *gui;
	char asset_title[BCTEXTLEN];
	char tmp_path[BCTEXTLEN];
	int use_deinterlace, use_inverse_telecine;
	int use_scale, use_resize_tracks;
	int use_wide_audio;
	int use_histogram, use_label_chapters;
	int use_standard;

	int64_t bd_size;
	int bd_width;
	int bd_height;
	double bd_aspect_width;
	double bd_aspect_height;
	double bd_framerate;
	int bd_samplerate;
	int bd_max_bitrate;
	double bd_kaudio_rate;
	int bd_interlace_mode;
	int max_w, max_h;
};

class CreateBD_OK : public BC_OKButton
{
public:
	CreateBD_OK(CreateBD_GUI *gui, int x, int y);
	~CreateBD_OK();
	int button_press_event();
	int keypress_event();

	CreateBD_GUI *gui;
};

class CreateBD_Cancel : public BC_CancelButton
{
public:
	CreateBD_Cancel(CreateBD_GUI *gui, int x, int y);
	~CreateBD_Cancel();
	int button_press_event();

	CreateBD_GUI *gui;
};


class CreateBD_DiskSpace : public BC_Title
{
public:
	CreateBD_DiskSpace(CreateBD_GUI *gui, int x, int y);
	~CreateBD_DiskSpace();
	int64_t tmp_path_space();
	void update();

	CreateBD_GUI *gui;
};

class CreateBD_TmpPath : public BC_TextBox
{
public:
	CreateBD_TmpPath(CreateBD_GUI *gui, int x, int y, int w);
	~CreateBD_TmpPath();
	int handle_event();

	CreateBD_GUI *gui;
};


class CreateBD_AssetTitle : public BC_TextBox
{
public:
	CreateBD_AssetTitle(CreateBD_GUI *gui, int x, int y, int w);
	~CreateBD_AssetTitle();
	int handle_event();

	CreateBD_GUI *gui;
};

class CreateBD_Deinterlace : public BC_CheckBox
{
public:
	CreateBD_Deinterlace(CreateBD_GUI *gui, int x, int y);
	~CreateBD_Deinterlace();
	int handle_event();

	CreateBD_GUI *gui;
};

class CreateBD_InverseTelecine : public BC_CheckBox
{
public:
	CreateBD_InverseTelecine(CreateBD_GUI *gui, int x, int y);
	~CreateBD_InverseTelecine();
	int handle_event();

	CreateBD_GUI *gui;
};

class CreateBD_ResizeTracks : public BC_CheckBox
{
public:
	CreateBD_ResizeTracks(CreateBD_GUI *gui, int x, int y);
	~CreateBD_ResizeTracks();

	CreateBD_GUI *gui;
};

class CreateBD_Histogram : public BC_CheckBox
{
public:
	CreateBD_Histogram(CreateBD_GUI *gui, int x, int y);
	~CreateBD_Histogram();

	CreateBD_GUI *gui;
};

class CreateBD_LabelChapters : public BC_CheckBox
{
public:
	CreateBD_LabelChapters(CreateBD_GUI *gui, int x, int y);
	~CreateBD_LabelChapters();

	CreateBD_GUI *gui;
};

class CreateBD_WideAudio : public BC_CheckBox
{
public:
	CreateBD_WideAudio(CreateBD_GUI *gui, int x, int y);
	~CreateBD_WideAudio();

	CreateBD_GUI *gui;
};

class CreateBD_GUI : public BC_Window
{
public:
	CreateBD_GUI(CreateBD_Thread *thread,
		int x, int y, int w, int h);
	~CreateBD_GUI();

	void create_objects();
	int resize_event(int w, int h);
	int translation_event();
	int close_event();
	void update();

	CreateBD_Thread *thread;
	int at_x, at_y;
	CreateBD_AssetTitle *asset_title;
	int tmp_x, tmp_y;
	CreateBD_TmpPath *tmp_path;
	BrowseButton *btmp_path;
	CreateBD_DiskSpace *disk_space;
	CreateBD_Format *standard;
	CreateBD_Scale *scale;
	ArrayList<BC_ListBoxItem *> media_sizes;
	CreateBD_MediaSize *media_size;
	CreateBD_Deinterlace *need_deinterlace;
	CreateBD_InverseTelecine *need_inverse_telecine;
	CreateBD_ResizeTracks *need_resize_tracks;
	CreateBD_Histogram *need_histogram;
	BC_Title *non_standard;
	CreateBD_WideAudio *need_wide_audio;
	CreateBD_LabelChapters *need_label_chapters;
	int ok_x, ok_y, ok_w, ok_h;
	CreateBD_OK *ok;
	int cancel_x, cancel_y, cancel_w, cancel_h;
	CreateBD_Cancel *cancel;
};

class CreateBD_FormatItem : public BC_MenuItem
{
public:
	int handle_event();
	CreateBD_FormatItem(CreateBD_Format *popup, int standard, const char *name);
	~CreateBD_FormatItem();

	CreateBD_Format *popup;
	int standard;
};

class CreateBD_Format : public BC_PopupMenu
{
public:
	void create_objects();
	int handle_event();
	CreateBD_Format(CreateBD_GUI *gui, int x, int y);
	~CreateBD_Format();
	void set_value(int v) { set_text(get_item(v)->get_text()); }

	CreateBD_GUI *gui;
};

class CreateBD_ScaleItem : public BC_MenuItem
{
public:
	int handle_event();
	CreateBD_ScaleItem(CreateBD_Scale *popup, int scale, const char *text);
	~CreateBD_ScaleItem();

	CreateBD_Scale *popup;
	int scale;
};

class CreateBD_Scale : public BC_PopupMenu
{
public:
	void create_objects();
	int handle_event();
	CreateBD_Scale(CreateBD_GUI *gui, int x, int y);
	~CreateBD_Scale();

	CreateBD_GUI *gui;
	void set_value(int v) { set_text(Rescale::scale_types[v]); }
};

class CreateBD_MediaSize : public BC_PopupTextBox
{
public:
	CreateBD_MediaSize(CreateBD_GUI *gui, int x, int y);
	~CreateBD_MediaSize();
	int handle_event();

	CreateBD_GUI *gui;
};

#endif
