#include <stdio.h>
#include <signal.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "bcsignals.h"
#include "pluginlv2client.h"
#include "pluginlv2ui.h"

int main(int ac, char **av)
{
// to grab this task in the debugger
const char *cp = getenv("BUG");
static int zbug = !cp ? 0 : atoi(cp);  volatile int bug = zbug;
while( bug ) usleep(10000);
	BC_Signals signals;
	if( getenv("BC_TRAP_LV2_SEGV") ) {
		signals.initialize("/tmp/lv2ui_%d.dmp");
		BC_Signals::set_catch_segv(1);
	}
	return PluginLV2ChildUI().run(ac, av);
}

int PluginLV2ChildUI::run(int ac, char **av)
{
	this->ac = ac;
	this->av = av;

	if( ac > 3 ) {
		signal(SIGINT, SIG_IGN);
		ForkBase::child_fd = atoi(av[1]);
		ForkBase::parent_fd = atoi(av[2]);
		ForkBase::ppid = atoi(av[3]);
	}
	else {
		int sample_rate = samplerate, bfrsz = block_length;
		if( ac > 2 ) sample_rate = atoi(av[2]);
		if( init_ui(av[1], sample_rate, bfrsz) ) {
			fprintf(stderr," init_ui failed\n");
			return 1;
		}
		start_gui();
	}
	return run_ui();
}

