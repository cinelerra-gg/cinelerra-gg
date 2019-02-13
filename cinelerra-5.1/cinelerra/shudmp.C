#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <signal.h>

#include <libusb-1.0/libusb.h>

#define SHUTTLE_INTERFACE 0

libusb_device_handle *devsh = 0;
int claimed = -1;

void usb_done()
{
	if( devsh ) {
		if( claimed > 0 ) {
			int sh_iface = SHUTTLE_INTERFACE;
			libusb_release_interface(devsh, sh_iface);
			libusb_attach_kernel_driver(devsh, sh_iface);
			claimed = 0;
		}
		libusb_close(devsh);
		devsh = 0;
	}
	if( claimed >= 0 ) {
		libusb_exit(0);
		claimed = -1;
	}
}

void usb_probe()
{
	int ret = libusb_init(0);
	if( ret < 0 ) return;
	claimed = 0;
	devsh = libusb_open_device_with_vid_pid(0, 0x0b33, 0x0030);
	if( devsh ) {
		int sh_iface = SHUTTLE_INTERFACE;
		libusb_detach_kernel_driver(devsh, sh_iface);
		ret = libusb_claim_interface(devsh, sh_iface);
		if( ret >= 0 ) claimed = 1;
	}
	if( !claimed )
		usb_done();
}

int done = 0;
void sigint(int n) { done = 1; }

int main(int ac, char **av)
{
	setbuf(stdout, 0);
	usb_probe();
	if( claimed > 0 ) {
		signal(SIGINT, sigint);
		while( !done ) {
			int len = 0;
			static const int IN_ENDPOINT = 0x81;
			unsigned char dat[64];
			int ret = libusb_interrupt_transfer(devsh,
					IN_ENDPOINT, dat, sizeof(dat), &len, 100);
			if( ret != 0 ) {
				if( ret == LIBUSB_ERROR_TIMEOUT ) continue;
				printf("error: %s\n", libusb_strerror((libusb_error)ret));
				sleep(1);  continue;
			}
			printf("shuttle: ");
			for( int i=0; i<len; ++i ) printf(" %02x", dat[i]);
			printf("\n");
		}
	}
	usb_done();
	return 0;
}

