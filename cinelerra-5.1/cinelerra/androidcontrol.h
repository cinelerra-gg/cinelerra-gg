#ifndef __ANDROID_CONTROL_H__
#define __ANDROID_CONTROL_H__

#include "androidcontrol.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "mutex.h"
#include "thread.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


class AndroidControl : public Thread {
	static const unsigned short PORT = 23432;
	int done;
	int sockfd;
	char buf[256];
	char *msg;
	int msg_len;
	MWindowGUI *mwindow_gui;

	bool is_msg(const char *cp);
	void press(int key);
public:
	int open(unsigned short port);
	void close();
	void run();

	AndroidControl(MWindowGUI *mwindow_gui);
	~AndroidControl();
};

#endif
