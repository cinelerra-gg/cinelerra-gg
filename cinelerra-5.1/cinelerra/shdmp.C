#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <signal.h>
#include <linux/input.h>

input_event ev;

int main(int ac, char **av)
{
	setbuf(stdout, 0);
	if( ac < 2 ) { printf("usage: %s /dev/input/by-id/<device?>\n", av[0]); exit(1); }
	int fd = open(av[1], O_RDONLY);
	if( !fd ) { perror(av[1]); exit(1); }

	for(;;) {
		int ret = read(fd, &ev, sizeof(ev));
		if( ret != sizeof(ev) ) {
			if( ret < 0 ) perror("read event");
			printf("bad read: %d\n", ret);
			break;
		}
		printf("event: (%d, %d, 0x%x)\n", ev.type, ev.code, ev.value);
	}

	return 0;
}

