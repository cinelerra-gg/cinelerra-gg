#ifndef __FILEFFMPEG_H__
#define __FILEFFMPEG_H__

#include "asset.inc"
#include "bcdialog.h"
#include "bcwindowbase.inc"
#include "bitspopup.inc"
#include "edl.inc"
#include "ffmpeg.h"
#include "filebase.h"
#include "fileffmpeg.inc"
#include "indexfile.inc"
#include "mainprogress.inc"
#include "mutex.h"
#include "thread.h"
#include "vframe.inc"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


class FileFFMPEG : public FileBase
{
public:
	FFMPEG *ff;

        FileFFMPEG(Asset *asset, File *file);
        ~FileFFMPEG();
	static void ff_lock(const char *cp=0);
	static void ff_unlock();

	static void set_options(char *cp, int len, const char *bp);
	static void get_parameters(BC_WindowBase *parent_window,
		Asset *asset, BC_WindowBase *&format_window,
		int audio_options,int video_options, EDL *edl);
	static int check_sig(Asset *asset);
	int get_best_colormodel(int driver, int vstream);
	int get_video_info(int track, int &pid, double &framerate,
		int &width, int &height, char *title=0);
	int get_audio_for_video(int vstream, int astream, int64_t &channel_mask);
	static void get_info(char *path,char *text,int len);
	int open_file(int rd,int wr);
	int get_index(IndexFile *index_file, MainProgressBar *progress_bar);
	int close_file(void);
	int write_samples(double **buffer,int64_t len);
	int write_frames(VFrame ***frames,int len);
	int read_samples(double *buffer,int64_t len);
	int read_frame(VFrame *frame);
	int can_scale_input() { return 1; }
	int64_t get_memory_usage(void);
	int colormodel_supported(int colormodel);
	static int get_best_colormodel(Asset *asset, int driver);
	int select_video_stream(Asset *asset, int vstream);
	int select_audio_stream(Asset *asset, int astream);
};

class FFMpegConfigNum : public BC_TumbleTextBox
{
public:
        FFMpegConfigNum(BC_Window *window, int x, int y,
                char *title_text, int *output);
        ~FFMpegConfigNum();

        void create_objects();
	int update_param(const char *param, const char *opts);
        int handle_event();
        int *output;
        BC_Window *window;
        BC_Title *title;
        char *title_text;
        int x, y;
};

class FFMpegAudioNum : public FFMpegConfigNum
{
public:
        FFMpegAudioNum(BC_Window *window, int x, int y, char *title_text, int *output);
        ~FFMpegAudioNum() {}

        FFMPEGConfigAudio *window() { return (FFMPEGConfigAudio *)FFMpegConfigNum::window; }
};

class FFMpegAudioBitrate : public FFMpegAudioNum
{
public:
        FFMpegAudioBitrate(BC_Window *window, int x, int y, char *title_text, int *output)
          : FFMpegAudioNum(window, x, y, title_text, output) {}
	int handle_event();
};

class FFMpegAudioQuality : public FFMpegAudioNum
{
public:
        FFMpegAudioQuality(BC_Window *window, int x, int y, char *title_text, int *output)
          : FFMpegAudioNum(window, x, y, title_text, output) {}
	int handle_event();
};

class FFMpegVideoNum : public FFMpegConfigNum
{
public:
        FFMpegVideoNum(BC_Window *window, int x, int y, char *title_text, int *output);
        ~FFMpegVideoNum() {}

        FFMPEGConfigVideo *window() { return (FFMPEGConfigVideo *)FFMpegConfigNum::window; }
};

class FFMpegVideoBitrate : public FFMpegVideoNum
{
public:
        FFMpegVideoBitrate(BC_Window *window, int x, int y, char *title_text, int *output)
          : FFMpegVideoNum(window, x, y, title_text, output) {}
	int handle_event();
};

class FFMpegVideoQuality : public FFMpegVideoNum
{
public:
        FFMpegVideoQuality(BC_Window *window, int x, int y, char *title_text, int *output)
          : FFMpegVideoNum(window, x, y, title_text, output) {}
	int handle_event();
};

class FFMpegPixFmtItems : public ArrayList<BC_ListBoxItem*>
{
public:
	FFMpegPixFmtItems() {}
	~FFMpegPixFmtItems() { remove_all_objects(); }
};

class FFMpegPixelFormat : public BC_PopupTextBox
{
public:
	FFMpegPixelFormat(FFMPEGConfigVideo *vid_config, int x, int y, int w, int list_h);

        FFMPEGConfigVideo *vid_config;
	FFMpegPixFmtItems pixfmts;

	int handle_event();
	void update_formats();
};

class FFMpegSampleFormat : public BC_PopupTextBox
{
public:
	FFMpegSampleFormat(FFMPEGConfigAudio *aud_config, int x, int y, int w, int list_h);

        FFMPEGConfigAudio *aud_config;
	ArrayList<BC_ListBoxItem*> samplefmts;

	int handle_event();
	void update_formats();
};

class FFMPEGConfigAudio : public BC_Window
{
public:
	FFMPEGConfigAudio(BC_WindowBase *parent_window, Asset *asset, EDL *edl);
	~FFMPEGConfigAudio();

	void create_objects();
	int close_event();
	void load_options();

	FFMpegSampleFormat *sample_format;
	ArrayList<BC_ListBoxItem*> presets;
	FFMPEGConfigAudioPopup *preset_popup;
	FFMpegAudioBitrate *bitrate;
	FFMpegAudioQuality *quality;
	FFAudioOptions *audio_options;
	BC_WindowBase *parent_window;
	Asset *asset;
	EDL *edl;
	FFOptionsDialog *ff_options_dialog;
};

class FFAudioOptions : public BC_ScrollTextBox
{
public:
	FFAudioOptions(FFMPEGConfigAudio *audio_popup,
		int x, int y, int w, int rows, int size, char *text);

	FFMPEGConfigAudio *audio_popup;
};


class FFMPEGConfigAudioPopup : public BC_PopupTextBox
{
public:
	FFMPEGConfigAudioPopup(FFMPEGConfigAudio *popup, int x, int y);
	int handle_event();
	FFMPEGConfigAudio *popup;
};


class FFMPEGConfigAudioToggle : public BC_CheckBox
{
public:
	FFMPEGConfigAudioToggle(FFMPEGConfigAudio *popup,
		char *title_text, int x, int y, int *output);
	int handle_event();
	int *output;
	FFMPEGConfigAudio *popup;
};

class FFMPEGConfigVideo : public BC_Window
{
public:
	FFMPEGConfigVideo(BC_WindowBase *parent_window, Asset *asset, EDL *edl);
	~FFMPEGConfigVideo();

	void create_objects();
	int close_event();
	void load_options();

	FFMpegPixelFormat *pixel_format;
	ArrayList<BC_ListBoxItem*> presets;
	FFMPEGConfigVideoPopup *preset_popup;
	BC_WindowBase *parent_window;
	FFMpegVideoBitrate *bitrate;
	FFMpegVideoQuality *quality;
	FFVideoOptions *video_options;
	Asset *asset;
	EDL *edl;
	FFOptionsDialog *ff_options_dialog;
};

class FFVideoOptions : public BC_ScrollTextBox
{
public:
	FFVideoOptions(FFMPEGConfigVideo *video_popup,
		int x, int y, int w, int rows, int size, char *text);

	FFMPEGConfigVideo *video_popup;
};

class FFMPEGConfigVideoPopup : public BC_PopupTextBox
{
public:
	FFMPEGConfigVideoPopup(FFMPEGConfigVideo *popup, int x, int y);
	int handle_event();
	FFMPEGConfigVideo *popup;
};

class FFMPEGConfigVideoToggle : public BC_CheckBox
{
public:
	FFMPEGConfigVideoToggle(FFMPEGConfigVideo *popup,
		char *title_text, int x, int y, int *output);
	int handle_event();
	int *output;
	FFMPEGConfigVideo *popup;
};

class FFMPEGScanProgress : public Thread
{
public:
	IndexFile *index_file;
	MainProgressBar *progress_bar;
	char progress_title[BCTEXTLEN];
	int64_t length, *position;
	int done, *canceled;

	FFMPEGScanProgress(IndexFile *index_file, MainProgressBar *progress_bar,
		const char *title, int64_t length, int64_t *position, int *canceled);
	~FFMPEGScanProgress();
	void run();
};


class FFOptions_OptName : public BC_ListBoxItem {
public:
	FFOptions_Opt *opt;

	FFOptions_OptName(FFOptions_Opt *opt, const char *nm);
	~FFOptions_OptName();

};

class FFOptions_OptValue : public BC_ListBoxItem {
public:
	FFOptions_Opt *opt;

	void update();
	void update(const char *v);
	FFOptions_OptValue(FFOptions_Opt *opt);
};

class FFOptions_Opt {
public:
	FFOptions *options;
	const AVOption *opt;
	FFOptions_OptName *item_name;
	FFOptions_OptValue *item_value;

	char *get(char *vp, int sz=-1);
	void set(const char *val);
	int types(char *rp);
	int scalar(double d, char *rp);
	int ranges(char *rp);
	int units(ArrayList<const char *> &opts);
	const char *unit_name(int id);
	int units(char *rp);
	const char *tip();

	FFOptions_Opt(FFOptions *options, const AVOption *opt, const char *nm);
	~FFOptions_Opt();
};

class FFOptions : public ArrayList<FFOptions_Opt *>
{
public:
	FFOptions();
	~FFOptions();
	FFOptionsWindow *win;
	AVCodecContext *avctx;
	const void *obj;

	void initialize(FFOptionsWindow *win, int k);
	static int cmpr(const void *a, const void *b);
	int update();
	void dump(FILE *fp);
};

class FFOptions_OptPanel : public BC_ListBox {
public:
	FFOptions_OptPanel(FFOptionsWindow *fwin, int x, int y, int w, int h);
	~FFOptions_OptPanel();
	void create_objects();
	int cursor_leave_event();

	FFOptionsWindow *fwin;
	ArrayList<BC_ListBoxItem*> items[2];
	ArrayList<BC_ListBoxItem*> &opts;
	ArrayList<BC_ListBoxItem*> &vals;
	char tip_text[BCTEXTLEN];

	int selection_changed();
	int update();
	void show_tip(const char *tip);
};

class FFOptionsKindItem : public BC_MenuItem
{
public:
	FFOptionsKind *kind;
	int idx;
	int handle_event();
	void show_label();

	FFOptionsKindItem(FFOptionsKind *kind, const char *name, int idx);
	~FFOptionsKindItem();
};

class FFOptionsKind : public BC_PopupMenu
{
	static const char *kinds[];
public:
	FFOptionsWindow *fwin;
	int kind;

	void create_objects();
	int handle_event();
	void set(int k);

	FFOptionsKind(FFOptionsWindow *fwin, int x, int y, int w);
	~FFOptionsKind();
};

class FFOptionsUnits : public BC_PopupMenu {
public:
	FFOptionsWindow *fwin;

	FFOptionsUnits(FFOptionsWindow *fwin, int x, int y, int w);
	~FFOptionsUnits();
	int handle_event();
};

class FFOptionsText : public BC_TextBox {
public:
	FFOptionsWindow *fwin;

	FFOptionsText(FFOptionsWindow *fwin, int x, int y, int w);
	~FFOptionsText();
	int handle_event();
};

class FFOptionsApply : public BC_GenericButton {
public:
	FFOptionsWindow *fwin;

	FFOptionsApply(FFOptionsWindow *fwin, int x, int y);
	~FFOptionsApply();
	int handle_event();
};

class FFOptionsWindow : public BC_Window
{
public:
	FFOptionsWindow(FFOptionsDialog *dialog);
	~FFOptionsWindow();

	void create_objects();
	void update(FFOptions_Opt *oip);
	void draw();
	int resize_event(int w, int h);

	FFOptionsDialog *dialog;
	FFOptions options;

	FFOptions_OptPanel *panel;
        int panel_x, panel_y, panel_w, panel_h;
	BC_Title *type, *range, *kind_title;
        FFOptions_Opt *selected;

	FFOptionsKind *kind;
	FFOptionsUnits *units;
	FFOptionsText *text;
	FFOptionsApply *apply;
};

class FFOptionsDialog : public BC_DialogThread
{
public:
	FFOptionsDialog();
	~FFOptionsDialog();
	virtual void update_options(const char *options) = 0;

	void load_options(const char *bp, int len);
	void store_options(char *cp, int len);
	void start(const char *format_name, const char *codec_name,
		AVCodec *codec, const char *options, int len);
	BC_Window* new_gui();
	void handle_done_event(int result);

	FFOptionsWindow *options_window;
	const char *format_name, *codec_name;
	AVCodec *codec;
	AVDictionary *ff_opts;
	int ff_len;
};

class FFOptionsAudioDialog : public FFOptionsDialog
{
public:
	FFMPEGConfigAudio *aud_config;
	void update_options(const char *options);

	FFOptionsAudioDialog(FFMPEGConfigAudio *aud_config);
	~FFOptionsAudioDialog();
};

class FFOptionsVideoDialog : public FFOptionsDialog
{
public:
	FFMPEGConfigVideo *vid_config;
	void update_options(const char *options);

	FFOptionsVideoDialog(FFMPEGConfigVideo *vid_config);
	~FFOptionsVideoDialog();
};

class FFOptionsViewAudio: public BC_GenericButton
{
public:
	FFOptionsViewAudio(FFMPEGConfigAudio *aud_config, int x, int y, const char *text);
	~FFOptionsViewAudio();

	int handle_event();
	FFMPEGConfigAudio *aud_config;
};

class FFOptionsViewVideo : public BC_GenericButton
{
public:
	FFOptionsViewVideo(FFMPEGConfigVideo *vid_config, int x, int y, const char *text);
	~FFOptionsViewVideo();

	int handle_event();
	FFMPEGConfigVideo *vid_config;
};

#endif
