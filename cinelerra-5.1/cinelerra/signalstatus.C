#ifdef HAVE_DVB

#include "bctitle.h"
#include "bccolors.h"
#include "devicedvbinput.h"
#include "fonts.h"
#include "signalstatus.h"

#include <unistd.h>


SignalStatus::SignalStatus(BC_WindowBase *wdw, int x, int y)
 : BC_SubWindow(x, y, 100, 35)
{
	this->wdw = wdw;
	dvb_input = 0;

	channel_title = 0;
	lck_x = lck_y = 0;
	crr_x = crr_y = 0;
	pwr_x = pwr_y = 0;
	snr_x = snr_y = 0;
	ber_x = ber_y = 0;
	unc_x = unc_y = 0;
}

SignalStatus::~SignalStatus()
{
	if( dvb_input )
		dvb_input->set_signal_status(0);
}


void SignalStatus::create_objects()
{
	clear_box(0,0,get_w(),get_h());
	int x = 0, y = 0;
	channel_title = new BC_Title(x, y, " ", SMALLFONT, GREEN);
	add_subwindow(channel_title);
	y += channel_title->get_h();
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, "lk:", SMALLFONT, YELLOW));
	lck_x = x + title->get_w() + pad0;
	lck_y = y + pad0;
	int x1 = lck_x + lck_w + pad1;
	int y1 = y + title->get_h();
	add_subwindow(title = new BC_Title(x, y1, "cr:", SMALLFONT, YELLOW));
	crr_x = lck_x;
	crr_y = y1 + pad0;
	add_subwindow(title = new BC_Title(x1, y, "pwr", SMALLFONT, YELLOW));
	pwr_x = x1 + title->get_w() + pad0;
	pwr_y = y + pad0;
	snr_x = pwr_x;
	snr_y = pwr_y + pwr_h;
	add_subwindow(title = new BC_Title(x1, y1, "err", SMALLFONT, YELLOW));
	ber_x = pwr_x;
	ber_y = y1 + pad0;
	unc_x = ber_x;
	unc_y = ber_y + ber_h;
}

int SignalStatus::calculate_w(BC_WindowBase *wdw)
{
	return BC_Title::calculate_w(wdw, "lk:", SMALLFONT) + pad0 + lck_w + pad1 +
		BC_Title::calculate_w(wdw, "pwr", SMALLFONT) + pad0 + pwr_w;
}

int SignalStatus::calculate_h(BC_WindowBase *wdw)
{
	return 3*BC_Title::calculate_h(wdw, "lk:", SMALLFONT);
}

void SignalStatus::update_lck(int v)
{
	set_color(v>0 ? GREEN : RED);
	draw_box(lck_x, lck_y, lck_w, lck_h);
}

void SignalStatus::update_crr(int v)
{
	set_color(v>0 ? GREEN : RED);
	draw_box(crr_x, crr_y, crr_w, crr_h);
}

void SignalStatus::update_pwr(int v)
{
	if( v < 0 ) v = 0;
	int w0 = (v*pwr_w) / 65535, w1 = pwr_w-w0;
	if( w0 > 0 ) { set_color(GREEN); draw_box(pwr_x, pwr_y, w0, pwr_h); }
	if( w1 > 0 ) clear_box(pwr_x+w0, pwr_y, w1, pwr_h);
}

void SignalStatus::update_snr(int v)
{
	if( v < 0 ) v = 0;
	int w0 = (v*snr_w) / 65535, w1 = pwr_w-w0;
	if( w0 > 0 ) { set_color(BLUE); draw_box(snr_x, snr_y, w0, snr_h); }
	if( w1 > 0 ) clear_box(snr_x+w0, snr_y, w1, snr_h);
}

void SignalStatus::update_ber(int v)
{
	if( v < 0 ) v = 0;
	int b = 0;
	while( v > 0 ) { ++b; v>>=1; }
	int w0 = (ber_w*b)/16, w1 = ber_w-w0;
	if( w0 > 0 ) { set_color(YELLOW); draw_box(ber_x, ber_y, w0, ber_h); }
	if( w1 > 0 ) clear_box(ber_x+w0, ber_y, w1, ber_h);
}

void SignalStatus::update_unc(int v)
{
	if( v < 0 ) v = 0;
	int b = 0;
	while( v > 0 ) { ++b; v>>=1; }
	int w0 = (unc_w*b)/16, w1 = unc_w-w0;
	if( w0 > 0 ) { set_color(RED); draw_box(unc_x, unc_y, w0, unc_h); }
	if( w1 > 0 ) clear_box(unc_x+w0, unc_y, w1, unc_h);
}


void SignalStatus::update()
{
	channel_title->update(dvb_input->channel_title());
	update_lck(dvb_input->signal_lck);  update_crr(dvb_input->signal_crr);
	update_pwr(dvb_input->signal_pwr);  update_snr(dvb_input->signal_snr);
	update_ber(dvb_input->signal_ber);  update_unc(dvb_input->signal_unc);
	flash();
}

#endif
