
#include "keys.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "androidcontrol.h"
#include "preferences.h"


int AndroidControl::open(unsigned short port)
{
	sockfd = socket(PF_INET,SOCK_DGRAM,0);
	if( sockfd < 0 ) { perror("socket:"); return -1; }

	struct linger lgr;
	lgr.l_onoff = 0;
	lgr.l_linger = 0;

	if( setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &lgr, sizeof(lgr)) < 0 ) {
		perror("setlinger:");
		return -1;
	}

	struct sockaddr_in sadr;
	memset(&sadr,0,sizeof(sadr));
	sadr.sin_family = AF_INET;
	sadr.sin_port = htons(port);
	sadr.sin_addr.s_addr = 0;

	if( bind(sockfd,(struct sockaddr *)&sadr,sizeof(sadr)) < 0 ) {
		perror("bind:");
		return -1;
	}

	return 0;
}

void AndroidControl::close()
{
	if( sockfd >= 0 ) {
		::close(sockfd);
		sockfd = -1;
	}
}

AndroidControl::AndroidControl(MWindowGUI *mwindow_gui)
 : Thread(1, 0, 0)
{
	this->mwindow_gui = mwindow_gui;
	Thread::start();
}

bool AndroidControl::is_msg(const char *cp)
{
	if( msg_len != (int)strlen(cp) ) return false;
	if( strncmp(cp, msg, msg_len) ) return false;
	return true;
}

void AndroidControl::press(int key)
{
// printf("press 0x%04x\n",key);
	if( mwindow_gui->key_listener(key) ) return;
	mwindow_gui->remote_control->remote_key(key);
}

void AndroidControl::run()
{
	unsigned short port = mwindow_gui->mwindow->preferences->android_port;
	done = open(port) >= 0 ? 0 : 1;

	while( !done ) {
		struct sockaddr sadr;
		socklen_t sadr_len;
		enable_cancel();
		int ret = recvfrom(sockfd, buf, sizeof(buf), 0, &sadr, &sadr_len);
		if( ret < 0 ) {
			perror("AndroidControl::run: recvfrom");
			sleep(1);
		}
		disable_cancel();
		// validate message
		msg = buf;  msg_len = ret;
		// cinelerra android remote magic number
		if( msg[0] != 'C' || msg[1] != 'A' || msg[2] != 'R' ) continue;
		// version number
		if( msg[3] != 1 ) continue;
		msg += 4;  msg_len -= 4;
		// pin
		const char *pin = mwindow_gui->mwindow->preferences->android_pin;
		int len = sizeof(mwindow_gui->mwindow->preferences->android_pin);
		while( len > 0 && msg_len > 0 && *pin != 0 && *msg != 0 ) {
			if( *pin != *msg ) break;
			++pin;  --len;
			++msg;  --msg_len;
		}
		if( !len || !msg_len || *pin != *msg ) continue;
		++msg; --msg_len;
		if( msg_len <= 0 ) continue;
		if( is_msg("stop") ) press(KPSTOP);
		else if( is_msg("play") ) press(KPPLAY);
		else if( is_msg("rplay") ) press(KPREV);
		else if( is_msg("pause") ) press(' ');
       		else if( is_msg("fast_lt") ) press(KPBACK);
		else if( is_msg("media_up") ) press(UP);
		else if( is_msg("fast_rt") ) press(KPFWRD);
		else if( is_msg("menu") ) press(KPMENU);
		else if( is_msg("media_lt") ) press(LEFT);
		else if( is_msg("media_rt") ) press(RIGHT);
		else if( is_msg("slow_lt") ) press(KPRECD);
		else if( is_msg("media_dn") ) press(DOWN);
		else if( is_msg("slow_rt") ) press(KPAUSE);
		else if( is_msg("key 0") ) press('0');
		else if( is_msg("key 1") ) press('1');
		else if( is_msg("key 2") ) press('2');
		else if( is_msg("key 3") ) press('3');
		else if( is_msg("key 4") ) press('4');
		else if( is_msg("key 5") ) press('5');
		else if( is_msg("key 6") ) press('6');
		else if( is_msg("key 7") ) press('7');
		else if( is_msg("key 8") ) press('8');
		else if( is_msg("key 9") ) press('9');
		else if( is_msg("key A") ) press('a');
		else if( is_msg("key B") ) press('b');
		else if( is_msg("key C") ) press('c');
		else if( is_msg("key D") ) press('d');
		else if( is_msg("key E") ) press('e');
		else if( is_msg("key F") ) press('f');
		else if( is_msg("suspend") ) {
			system("sync; sleep 1; pm-suspend");
		}
		else if( is_msg("power") ) {
			system("sync; sleep 1; poweroff");
		}
		else {
			printf("AndroidControl::run: unkn msg: %s\n", msg);
			sleep(1);
		}
	}
}

AndroidControl::~AndroidControl()
{
	if( Thread::running() ) {
		done = 1;
		Thread::cancel();
	}
	Thread::join();
}

