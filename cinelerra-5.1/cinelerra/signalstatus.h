#ifndef _SIGNALSTATUS_H_
#define _SIGNALSTATUS_H_
#ifdef HAVE_DVB

#include "bcwindowbase.h"
#include "bcsubwindow.h"
#include "bctitle.inc"
#include "channel.inc"
#include "devicedvbinput.inc"


class SignalStatus : public BC_SubWindow
{
	friend class DeviceDVBInput;
	enum {  pad0 = 3, pad1 = 8,
		lck_w = 5,  lck_h = 5,
		crr_w = 5,  crr_h = 5,
        	pwr_w = 32, pwr_h = 4,
        	snr_w = 32, snr_h = 4,
		ber_w = 32, ber_h = 4,
		unc_w = 32, unc_h = 4,
	};

	DeviceDVBInput *dvb_input;
	BC_Title *channel_title;
        int lck_x, lck_y;
        int crr_x, crr_y;
        int pwr_x, pwr_y;
        int snr_x, snr_y;
        int ber_x, ber_y;
        int unc_x, unc_y;

	void update_lck(int v);
	void update_crr(int v);
	void update_pwr(int v);
	void update_snr(int v);
	void update_ber(int v);
	void update_unc(int v);
public:
	BC_WindowBase *wdw;

	void create_objects();
	static int calculate_w(BC_WindowBase *wdw);
	static int calculate_h(BC_WindowBase *wdw);
	void disconnect() { dvb_input = 0; }
	void update();

	SignalStatus(BC_WindowBase *wdw, int x, int y);
	~SignalStatus();
};

#endif
#endif
