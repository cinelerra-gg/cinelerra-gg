#ifndef __DVDCREATE_H__
#define __DVDCREATE_H__

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

#include "dvdcreate.inc"

class CreateDVD_MenuItem : public BC_MenuItem
{
public:
	CreateDVD_MenuItem(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class DVD_BatchRenderJob : public BatchRenderJob
{
	int chapter;
	FILE *fp;
	EDL *edl;
public:
	DVD_BatchRenderJob(Preferences *preferences,
		int labeled, int farmed, int standard, int muxed);
	void copy_from(DVD_BatchRenderJob *src);
	DVD_BatchRenderJob *copy();
	void load(FileXML *file);
	void save(FileXML *file);
	char *create_script(EDL *edl, ArrayList<Indexable *> *idxbls);
	void create_chapter(double pos);

	int standard;
	int muxed;
};

class CreateDVD_Thread : public BC_DialogThread
{
	static const int64_t DVD_SIZE;
	static const int DVD_STREAMS, DVD_WIDTH, DVD_HEIGHT;
	static const double DVD_ASPECT_WIDTH, DVD_ASPECT_HEIGHT;
	static const double DVD_WIDE_ASPECT_WIDTH, DVD_WIDE_ASPECT_HEIGHT;
	static const int DVD_MAX_BITRATE, DVD_CHANNELS, DVD_WIDE_CHANNELS;
	static const double DVD_FRAMERATE, DVD_SAMPLERATE, DVD_KAUDIO_RATE;
public:
	CreateDVD_Thread(MWindow *mwindow);
	~CreateDVD_Thread();
	void handle_close_event(int result);
	BC_Window* new_gui();
	int option_presets();
	void create_chapter(FILE *fp, double pos);
	static int create_dvd_script(BatchRenderJob *job);
	int create_dvd_jobs(ArrayList<BatchRenderJob*> *jobs, const char *asset_path);
	int insert_video_plugin(const char *title, KeyFrame *default_keyframe);
	int resize_tracks();

	MWindow *mwindow;
	CreateDVD_GUI *gui;
	char asset_title[BCTEXTLEN];
	char tmp_path[BCTEXTLEN];
	int use_deinterlace, use_inverse_telecine;
	int use_scale, use_resize_tracks;
	int use_wide_audio, use_farmed;
	int use_histogram, use_labeled;
	int use_ffmpeg, use_standard;

	int64_t dvd_size;
	int dvd_width;
	int dvd_height;
	double dvd_aspect_width;
	double dvd_aspect_height;
	double dvd_framerate;
	int dvd_samplerate;
	int dvd_max_bitrate;
	double dvd_kaudio_rate;
	int max_w, max_h;
};

class CreateDVD_OK : public BC_OKButton
{
public:
	CreateDVD_OK(CreateDVD_GUI *gui, int x, int y);
	~CreateDVD_OK();
	int button_press_event();
	int keypress_event();

	CreateDVD_GUI *gui;
};

class CreateDVD_Cancel : public BC_CancelButton
{
public:
	CreateDVD_Cancel(CreateDVD_GUI *gui, int x, int y);
	~CreateDVD_Cancel();
	int button_press_event();

	CreateDVD_GUI *gui;
};


class CreateDVD_DiskSpace : public BC_Title
{
public:
	CreateDVD_DiskSpace(CreateDVD_GUI *gui, int x, int y);
	~CreateDVD_DiskSpace();
	int64_t tmp_path_space();
	void update();

	CreateDVD_GUI *gui;
};

class CreateDVD_TmpPath : public BC_TextBox
{
public:
	CreateDVD_TmpPath(CreateDVD_GUI *gui, int x, int y, int w);
	~CreateDVD_TmpPath();
	int handle_event();

	CreateDVD_GUI *gui;
};


class CreateDVD_AssetTitle : public BC_TextBox
{
public:
	CreateDVD_AssetTitle(CreateDVD_GUI *gui, int x, int y, int w);
	~CreateDVD_AssetTitle();
	int handle_event();

	CreateDVD_GUI *gui;
};

class CreateDVD_Deinterlace : public BC_CheckBox
{
public:
	CreateDVD_Deinterlace(CreateDVD_GUI *gui, int x, int y);
	~CreateDVD_Deinterlace();
	int handle_event();

	CreateDVD_GUI *gui;
};

class CreateDVD_InverseTelecine : public BC_CheckBox
{
public:
	CreateDVD_InverseTelecine(CreateDVD_GUI *gui, int x, int y);
	~CreateDVD_InverseTelecine();
	int handle_event();

	CreateDVD_GUI *gui;
};

class CreateDVD_ResizeTracks : public BC_CheckBox
{
public:
	CreateDVD_ResizeTracks(CreateDVD_GUI *gui, int x, int y);
	~CreateDVD_ResizeTracks();

	CreateDVD_GUI *gui;
};

class CreateDVD_Histogram : public BC_CheckBox
{
public:
	CreateDVD_Histogram(CreateDVD_GUI *gui, int x, int y);
	~CreateDVD_Histogram();

	CreateDVD_GUI *gui;
};

class CreateDVD_LabelChapters : public BC_CheckBox
{
public:
	CreateDVD_LabelChapters(CreateDVD_GUI *gui, int x, int y);
	~CreateDVD_LabelChapters();

	CreateDVD_GUI *gui;
};

class CreateDVD_UseRenderFarm : public BC_CheckBox
{
public:
	CreateDVD_UseRenderFarm(CreateDVD_GUI *gui, int x, int y);
	~CreateDVD_UseRenderFarm();

	CreateDVD_GUI *gui;
};

class CreateDVD_WideAudio : public BC_CheckBox
{
public:
	CreateDVD_WideAudio(CreateDVD_GUI *gui, int x, int y);
	~CreateDVD_WideAudio();

	CreateDVD_GUI *gui;
};

class CreateDVD_UseFFMpeg : public BC_CheckBox
{
public:
	CreateDVD_UseFFMpeg(CreateDVD_GUI *gui, int x, int y);
	~CreateDVD_UseFFMpeg();

	CreateDVD_GUI *gui;
};

class CreateDVD_GUI : public BC_Window
{
public:
	CreateDVD_GUI(CreateDVD_Thread *thread,
		int x, int y, int w, int h);
	~CreateDVD_GUI();

	void create_objects();
	int resize_event(int w, int h);
	int translation_event();
	int close_event();
	void update();

	CreateDVD_Thread *thread;
	int at_x, at_y;
	CreateDVD_AssetTitle *asset_title;
	int tmp_x, tmp_y;
	CreateDVD_TmpPath *tmp_path;
	BrowseButton *btmp_path;
	CreateDVD_DiskSpace *disk_space;
	CreateDVD_Format *standard;
	CreateDVD_Scale *scale;
	ArrayList<BC_ListBoxItem *> media_sizes;
	CreateDVD_MediaSize *media_size;
	CreateDVD_Deinterlace *need_deinterlace;
	CreateDVD_InverseTelecine *need_inverse_telecine;
	CreateDVD_UseFFMpeg *need_use_ffmpeg;
	CreateDVD_ResizeTracks *need_resize_tracks;
	CreateDVD_Histogram *need_histogram;
	CreateDVD_WideAudio *need_wide_audio;
	CreateDVD_LabelChapters *need_labeled;
	CreateDVD_UseRenderFarm *need_farmed;
	int ok_x, ok_y, ok_w, ok_h;
	CreateDVD_OK *ok;
	int cancel_x, cancel_y, cancel_w, cancel_h;
	CreateDVD_Cancel *cancel;
};

class CreateDVD_FormatItem : public BC_MenuItem
{
public:
	int handle_event();
	CreateDVD_FormatItem(CreateDVD_Format *popup, int standard, const char *text);
	~CreateDVD_FormatItem();

	CreateDVD_Format *popup;
	int standard;
};

class CreateDVD_Format : public BC_PopupMenu
{
public:
	void create_objects();
	int handle_event();
	CreateDVD_Format(CreateDVD_GUI *gui, int x, int y);
	~CreateDVD_Format();
	void set_value(int v) { set_text(get_item(v)->get_text()); }

	CreateDVD_GUI *gui;
};

class CreateDVD_ScaleItem : public BC_MenuItem
{
public:
	int handle_event();
	CreateDVD_ScaleItem(CreateDVD_Scale *popup, int scale, const char *text);
	~CreateDVD_ScaleItem();

	CreateDVD_Scale *popup;
	int scale;
};

class CreateDVD_Scale : public BC_PopupMenu
{
public:
	void create_objects();
	int handle_event();
	CreateDVD_Scale(CreateDVD_GUI *gui, int x, int y);
	~CreateDVD_Scale();

	CreateDVD_GUI *gui;
	void set_value(int v) { set_text(Rescale::scale_types[v]); }
};

class CreateDVD_MediaSize : public BC_PopupTextBox
{
public:
	CreateDVD_MediaSize(CreateDVD_GUI *gui, int x, int y);
	~CreateDVD_MediaSize();
	int handle_event();

	CreateDVD_GUI *gui;
};

#endif
