#include "vicon.h"

#include "bctimer.h"
#include "bcwindow.h"
#include "bccolors.h"
#include "keys.h"
#include "mutex.h"
#include "condition.h"

VIcon::
VIcon(int vw, int vh, double rate)
{
	this->vw = vw;
	this->vh = vh;
	this->frame_rate = rate;

	cycle_start = 0;
	age = 0;
	seq_no = 0;
	in_use = 1;
	hidden = 0;
	audio_data = 0;
	audio_size = 0;
	playing_audio = 0;
}

VIcon::
~VIcon()
{
	clear_images();
	delete [] audio_data;
}

void VIcon::
add_image(VFrame *frm, int ww, int hh, int vcmdl)
{
	VIFrame *vifrm = new VIFrame(ww, hh, vcmdl);
	VFrame *img = *vifrm;
	img->transfer_from(frm);
	images.append(vifrm);
}

void VIcon::
draw_vframe(VIconThread *vt, BC_WindowBase *wdw, int x, int y)
{
	VFrame *vfrm = frame();
	if( !vfrm ) return;
	int sx0 = 0, sx1 = sx0 + vt->view_w;
	int sy0 = 0, sy1 = sy0 + vt->view_h;
	int dx0 = x, dx1 = dx0 + vw;
	int dy0 = y, dy1 = dy0 + vh;
	if( (x=vt->draw_x0-dx0) > 0 ) { sx0 += (x*vt->view_w)/vw;  dx0 = vt->draw_x0; }
	if( (x=dx1-vt->draw_x1) > 0 ) { sx1 -= (x*vt->view_w)/vw;  dx1 = vt->draw_x1; }
	if( (y=vt->draw_y0-dy0) > 0 ) { sy0 += (y*vt->view_h)/vh;  dy0 = vt->draw_y0; }
	if( (y=dy1-vt->draw_y1) > 0 ) { sy1 -= (y*vt->view_h)/vh;  dy1 = vt->draw_y1; }
	int sw = sx1 - sx0, sh = sy1 - sy0;
	int dw = dx1 - dx0, dh = dy1 - dy0;
	if( dw > 0 && dh > 0 && sw > 0 && sh > 0 )
		wdw->draw_vframe(vfrm, dx0,dy0, dw,dh, sx0,sy0, sw,sh);
}

void VIconThread::
set_drawing_area(int x0, int y0, int x1, int y1)
{
	draw_x0 = x0;  draw_y0 = y0;
	draw_x1 = x1;  draw_y1 = y1;
}

VIcon *VIconThread::low_vicon()
{
	if( !t_heap.size() ) return 0;
	VIcon *vip = t_heap[0];
	remove_vicon(0);
	return vip;
}

void VIconThread::remove_vicon(int i)
{
	int sz = t_heap.size();
	for( int k; (k=2*(i+1)) < sz; i=k ) {
		if( t_heap[k]->age > t_heap[k-1]->age ) --k;
		t_heap[i] = t_heap[k];
	}
	VIcon *last = t_heap[--sz];
	t_heap.remove_number(sz);
	double age = last->age;
	for( int k; i>0 && age<t_heap[k=(i-1)/2]->age; i=k )
		t_heap[i] = t_heap[k];
	t_heap[i] = last;
}


VIconThread::
VIconThread(BC_WindowBase *wdw, int vw, int vh)
 : Thread(1, 0, 0)
{
	this->wdw = wdw;
	this->view_win = 0;  this->vicon = 0;
	this->view_w = vw;   this->view_h = vh;
	this->viewing = 0;
	this->draw_x0 = 0;   this->draw_x1 = wdw->get_w();
	this->draw_y0 = 0;   this->draw_y1 = wdw->get_h();
	draw_lock = new Condition(0, "VIconThread::draw_lock", 1);
	timer = new Timer();
	this->refresh_rate = VICON_RATE;
	done = 0;
	interrupted = -1;
	stop_age = 0;
}

VIconThread::
~VIconThread()
{
	stop_drawing();
	done = 1;
	draw_lock->unlock();
	if( Thread::running() ) {
		Thread::cancel();
	}
	Thread::join();
	t_heap.remove_all_objects();
	delete timer;
	delete draw_lock;
}

void VIconThread::
start_drawing()
{
	wdw->lock_window("VIconThread::start_drawing");
	if( view_win )
		wdw->set_active_subwindow(view_win);
        if( interrupted < 0 )
		draw_lock->unlock();
	timer->update();
	timer->subtract(-stop_age);
	interrupted = 0;
	wdw->unlock_window();
}

void VIconThread::
stop_drawing()
{
	wdw->lock_window("VIconThread::stop_drawing");
	close_view_popup();
	if( !interrupted )
		interrupted = 1;
	stop_age = timer->get_difference();
	wdw->unlock_window();
}

int VIconThread::keypress_event(int key)
{
	if( key != ESC ) return 0;
	close_view_popup();
	return 1;
}

bool VIconThread::
visible(VIcon *vicon, int x, int y)
{
	if( vicon->hidden ) return false;
	if( y+vicon->vh <= draw_y0 ) return false;
	if( y >= draw_y1 ) return false;
	if( x+vicon->vw <= draw_x0 ) return false;
	if( x >= draw_x1 ) return false;
	return true;
}

int ViewPopup::keypress_event()
{
	int key = get_keypress();
	return vt->keypress_event(key);
}

int ViewPopup::button_press_event()
{
	return !vt->vicon ? 0 :
		vt->vicon->popup_button_press(get_cursor_x(), get_cursor_y());
}
int ViewPopup::cursor_motion_event()
{
	return !vt->vicon ? 0 :
		vt->vicon->popup_cursor_motion(get_cursor_x(), get_cursor_y());
}


ViewPopup::ViewPopup(VIconThread *vt, VFrame *frame, int x, int y, int w, int h)
 : BC_Popup(vt->wdw, x, y, w, h, BLACK)
{
	this->vt = vt;
}

ViewPopup::~ViewPopup()
{
	vt->wdw->set_active_subwindow(0);
}

ViewPopup *VIconThread::new_view_window(VFrame *frame)
{
	BC_WindowBase *parent = wdw->get_parent();
	XineramaScreenInfo *info = parent->get_xinerama_info(-1);
	int cx = info ? info->x_org + info->width/2 : parent->get_root_w(0)/2;
	int cy = info ? info->y_org + info->height/2 : parent->get_root_h(0)/2;
	int vx = viewing->get_vx(), rx = 0;
	int vy = viewing->get_vy(), ry = 0;
	wdw->get_root_coordinates(vx, vy, &rx, &ry);
	rx += (rx >= cx ? -view_w : viewing->vw);
	ry += (ry >= cy ? -view_h : viewing->vh);
	ViewPopup *vwin = new ViewPopup(this, frame, rx, ry, view_w, view_h);
	wdw->set_active_subwindow(vwin);
	return vwin;
}

void VIconThread::
reset_images()
{
	for( int i=t_heap.size(); --i>=0; ) t_heap[i]->reset();
	timer->update();
	img_dirty = win_dirty = 0;
}

void VIconThread::add_vicon(VIcon *vip)
{
	double age = vip->age;
	int i = t_heap.size();  t_heap.append(vip);
	for( int k; i>0 && age<t_heap[(k=(i-1)/2)]->age; i=k )
		t_heap[i] = t_heap[k];
	t_heap[i] = vip;
}

int VIconThread::del_vicon(VIcon *vicon)
{
	int i = t_heap.size();
	while( --i >= 0 && t_heap[i] != vicon );
	if( i < 0 ) return 0;
	remove_vicon(i);
	return 1;
}

void VIconThread::draw_vframe(BC_WindowBase *wdw, VFrame *frame)
{
	if( !wdw || !frame ) return;
	wdw->draw_vframe(frame, 0,0, wdw->get_w(),wdw->get_h());
}

void VIconThread::set_view_popup(VIcon *vicon, VIconDrawVFrame *draw_vfrm)
{
	this->vicon = vicon;
	this->draw_vfrm = vicon && !draw_vfrm ? VIconThread::draw_vframe : draw_vfrm;
}

void VIconThread::close_view_popup()
{
	set_view_popup(0);
}

int VIconThread::
update_view()
{
	if( viewing ) viewing->stop_audio();
	delete view_win;  view_win = 0;
	if( (viewing=vicon) != 0 ) {
		VFrame *frame = viewing->frame();
		view_win = new_view_window(frame);
		view_win->show_window();
		vicon->start_audio();
	}
	wdw->set_active_subwindow(view_win);
	return 1;
}


void VIconThread::
draw_images()
{
	for( int i=0; i<t_heap.size(); ++i )
		draw(t_heap[i]);
}

void VIconThread::
flash()
{
	if( !img_dirty && !win_dirty ) return;
	if( img_dirty ) wdw->flash();
	if( win_dirty && view_win ) view_win->flash();
	win_dirty = img_dirty = 0;
}

int VIconThread::
draw(VIcon *vicon)
{
	int x = vicon->get_vx(), y = vicon->get_vy();
	int draw_img = visible(vicon, x, y);
	int draw_win = view_win && viewing == vicon ? 1 : 0;
	if( !draw_img && !draw_win ) return 0;
	if( !vicon->frame() ) return 0;
	if( draw_img ) {
		vicon->draw_vframe(this, wdw, x, y);
		img_dirty = 1;
	}
	if( draw_win ) {
		draw_vfrm(view_win, vicon->frame());
		win_dirty = 1;
	}
	return 1;
}

void VIconThread::hide_vicons(int v)
{
	for( int i=0; i<t_heap.size(); ++i ) {
		t_heap[i]->hidden = v;
		t_heap[i]->age = 0;
	}
}

void VIconThread::
run()
{
	while(!done) {
		draw_lock->lock("VIconThread::run 0");
		if( done ) break;
		wdw->lock_window("BC_WindowBase::run 1");
		drawing_started();
		reset_images();
		int64_t seq_no = 0, now = 0;
		int64_t draw_flash = 1000 / refresh_rate;
		while( !interrupted ) {
			if( viewing != vicon )
				update_view();
			VIcon *next = low_vicon();
			while( next && next->age < draw_flash ) {
				now = timer->get_difference();
				if( now >= draw_flash ) break;
				draw(next);
				if( !next->seq_no ) {
					next->cycle_start = now;
					if( next->playing_audio > 0 )
						next->start_audio();
				}
				int64_t ref_no = (now - next->cycle_start) / 1000. * refresh_rate;
				int count = ref_no - next->seq_no;
				if( count < 1 ) count = 1;
				ref_no = next->seq_no + count;
				next->age =  next->cycle_start + 1000. * ref_no / refresh_rate;
				if( !next->set_seq_no(ref_no) )
					next->age = now + 1000.;
				add_vicon(next);
				next = low_vicon();
			}
			if( !next ) break;
			add_vicon(next);
			if( draw_flash < now+1 )
				draw_flash = now+1;
			wdw->unlock_window();
			while( !interrupted ) {
				now = timer->get_difference();
				int64_t ms = draw_flash - now;
				if( ms <= 0 ) break;
				if( ms > 100 ) ms = 100;
				Timer::delay(ms);
			}
			wdw->lock_window("BC_WindowBase::run 2");
			now = timer->get_difference();
			int64_t late = now - draw_flash;
			if( late < 1000 ) flash();
			int64_t ref_no = now / 1000. * refresh_rate;
			int64_t count = ref_no - seq_no;
			if( count < 1 ) count = 1;
			seq_no += count;
			draw_flash = seq_no * 1000. / refresh_rate;
		}
		if( viewing != vicon )
			update_view();
		drawing_stopped();
		interrupted = -1;
		wdw->unlock_window();
	}
}


void VIcon::init_audio(int audio_size)
{
	this->audio_size = audio_size;
	audio_data = new uint8_t[audio_size];
	memset(audio_data, 0, audio_size);
}

void VIcon::dump(const char *dir)
{
	mkdir(dir,0777);
	for( int i=0; i<images.size(); ++i ) {
		char fn[1024];  sprintf(fn,"%s/img%05d.png",dir,i);
		printf("\r%s",fn);
		VFrame *img = *images[i];
		img->write_png(fn);
	}
	printf("\n");
}

