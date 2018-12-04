#ifndef __VICON_H__
#define __VICON_H__

#include "arraylist.h"
#include "bccmodels.h"
#include "bcpopup.h"
#include "bcwindowbase.h"
#include "thread.h"
#include "vicon.inc"
#include "vframe.h"

typedef void VIconDrawVFrame(BC_WindowBase *wdw, VFrame *frame);

class ViewPopup : public BC_Popup {
public:
	VIconThread *vt;
	int keypress_event();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();

	ViewPopup(VIconThread *vt, VFrame *frame, int x, int y, int w, int h);
	~ViewPopup();
};

class VIFrame {
	unsigned char *img_data;
	VFrame *vfrm;
public:
	VIFrame(int ww, int hh, int vcmdl) {
		int size = BC_CModels::calculate_datasize(ww, hh, -1, vcmdl);
		img_data = new unsigned char[size];
		vfrm = new VFrame(img_data, -1, ww, hh, vcmdl, -1);
	}
	~VIFrame() { delete vfrm;  delete [] img_data; }

	operator VFrame *() { return vfrm; }
};

class VIcon
{
public:
	int vw, vh, in_use, hidden;
	ArrayList<VIFrame *> images;
        int64_t seq_no;
        double cycle_start, age, frame_rate;
	int audio_size, playing_audio;
	uint8_t *audio_data;

	int64_t vframes() { return images.size(); }
	void reset() { seq_no = 0; cycle_start = 0; age = 0; }
	void reset(double rate) { reset(); frame_rate = rate; }
	void clear_images() { images.remove_all_objects(); }
	void init_audio(int audio_size);

	virtual int64_t set_seq_no(int64_t no) { return seq_no = no; }
	virtual VFrame *frame() { return *images[seq_no]; }
	virtual int get_vx() { return 0; }
	virtual int get_vy() { return 0; }
	virtual void load_audio() {}
	virtual void start_audio() {}
	virtual void stop_audio() {}

	void add_image(VFrame *frm, int ww, int hh, int vcmdl);
	void draw_vframe(VIconThread *vt, BC_WindowBase *wdw, int x, int y);
	void dump(const char *dir);

	VIcon(int vw=VICON_WIDTH, int vh=VICON_HEIGHT, double rate=VICON_RATE);
	virtual ~VIcon();
};

class VIconThread : public Thread
{
public:
	int done, interrupted;
	BC_WindowBase *wdw;
	Timer *timer;
	Condition *draw_lock;
	ViewPopup *view_win;
	VIcon *viewing, *vicon;
	int view_w, view_h;
	int draw_x0, draw_y0;
	int draw_x1, draw_y1;
	int img_dirty, win_dirty;
	double refresh_rate;
	int64_t stop_age;

	ArrayList<VIcon *>t_heap;
	VIcon *low_vicon();
	void add_vicon(VIcon *vicon);
	int del_vicon(VIcon *vicon);
	void run();
	void flash();
	int draw(VIcon *vicon);
	int update_view();
	void draw_images();
	void start_drawing();
	void stop_drawing();
	void reset_images();
	void remove_vicon(int i);
	int keypress_event(int key);
	void set_drawing_area(int x0, int y0, int x1, int y1);
	void set_view_popup(VIcon *vicon, VIconDrawVFrame *draw_vfrm=0);
	void close_view_popup();
	void hide_vicons(int v=1);
	ViewPopup *new_view_window(VFrame *frame);
	virtual int popup_button_press(int x, int y) { return 0; }
	virtual int popup_button_release(int x, int y) { return 0; }
	virtual int popup_cursor_motion(int x, int y) { return 0; }

	virtual bool visible(VIcon *vicon, int x, int y);
	virtual void drawing_started() {}
	virtual void drawing_stopped() {}
	static VIconDrawVFrame draw_vframe;
	VIconDrawVFrame *draw_vfrm;

	VIconThread(BC_WindowBase *wdw, int vw=4*VICON_WIDTH, int vh=4*VICON_HEIGHT);
	~VIconThread();
};

#endif
