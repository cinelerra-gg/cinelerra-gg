
/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "bcsignals.h"
#include "bcwindowbase.h"
#include "bccmodels.h"
#include "bckeyboard.h"
#include "bcresources.h"
#include "cstrdup.h"

#include <ctype.h>
#include <dirent.h>
#include <execinfo.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/types.h>

BC_Signals* BC_Signals::global_signals = 0;
static int signal_done = 0;

static struct sigaction old_segv = {0, }, old_intr = {0, };
static void handle_dump(int n, siginfo_t * info, void *sc);

const char *BC_Signals::trap_path = 0;
void *BC_Signals::trap_data = 0;
void (*BC_Signals::trap_hook)(FILE *fp, void *data) = 0;
bool BC_Signals::trap_sigsegv = false;
bool BC_Signals::trap_sigintr = false;

static void uncatch_sig(int sig, struct sigaction &old)
{
	struct sigaction act;
	sigaction(sig, &old, &act);
	old.sa_handler = 0;
}

static void catch_sig(int sig, struct sigaction &old)
{
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = handle_dump;
	act.sa_flags = SA_SIGINFO;
	sigaction(sig, &act, (!old.sa_handler ? &old : 0));
}

static void uncatch_intr() { uncatch_sig(SIGINT, old_intr); }
static void catch_intr() { catch_sig(SIGINT, old_intr); }
static void uncatch_segv() { uncatch_sig(SIGSEGV, old_segv); }
static void catch_segv() { catch_sig(SIGSEGV, old_segv); }

void BC_Signals::set_trap_path(const char *path)
{
	trap_path = path;
}

void BC_Signals::set_trap_hook(void (*hook)(FILE *fp, void *vp), void *data)
{
	trap_data = data;
	trap_hook = hook;
}

void BC_Signals::set_catch_segv(bool v) {
	if( v == trap_sigsegv ) return;
	if( v ) catch_segv();
	else uncatch_segv();
	trap_sigsegv = v;
}

void BC_Signals::set_catch_intr(bool v) {
	if( v == trap_sigintr ) return;
	if( v ) catch_intr();
	else uncatch_intr();
	trap_sigintr = v;
}

static void bc_copy_textfile(int lines, FILE *ofp, const char *fmt,...)
{
	va_list ap;    va_start(ap, fmt);
	char bfr[BCTEXTLEN];  vsnprintf(bfr, sizeof(bfr), fmt, ap);
	va_end(ap);
	FILE *ifp = fopen(bfr,"r");
	if( !ifp ) return;
	while( --lines >= 0 && fgets(bfr,sizeof(bfr),ifp) ) fputs(bfr,ofp);
	fclose(ifp);
}

static void bc_list_openfiles(int lines, FILE *ofp, const char *fmt,...)
{
	va_list ap;    va_start(ap, fmt);
	char bfr[BCTEXTLEN];  vsnprintf(bfr, sizeof(bfr), fmt, ap);
	va_end(ap);
	DIR *dir  = opendir(bfr);
	if( !dir ) return;
	struct dirent64 *dent;
	while( --lines >= 0 && (dent = readdir64(dir)) ) {
		const char *fn = dent->d_name;
		fprintf(ofp, "%s", fn);
		char path[BCTEXTLEN], link[BCTEXTLEN];
		struct stat st;
		snprintf(path, sizeof(path), "%s/%s", bfr, fn);
		if( !stat(path,&st) ) {
			int typ = 0;
			if( S_ISREG(st.st_mode) )       typ = ' ';
			else if( S_ISDIR(st.st_mode) )  typ = 'd';
			else if( S_ISBLK(st.st_mode) )  typ = 'b';
			else if( S_ISCHR(st.st_mode) )  typ = 'c';
			else if( S_ISFIFO(st.st_mode) ) typ = 'f';
			else if( S_ISLNK(st.st_mode) )  typ = 'l';
			else if( S_ISSOCK(st.st_mode) ) typ = 's';
			if( typ ) fprintf(ofp, "\t%c", typ);
			fprintf(ofp, "\tsize %jd", st.st_size);
			int len = readlink(path, link, sizeof(link)-1);
			if( len > 0 ) {
				link[len] = 0;
				fprintf(ofp, "\t-> %s", link);
			}
		}
		snprintf(path, sizeof(path), "%sinfo/%s", bfr, fn);
		FILE *fp = fopen(path,"r");  int64_t pos;
		if( fp ) {
			while( fgets(link, sizeof(link), fp) ) {
				if( sscanf(link, "pos:%jd", &pos) == 1 ) {
					fprintf(ofp, "\tpos: %jd", pos);
					break;
				}
			}
			fclose(fp);
		}
		fprintf(ofp, "\n");
	}
	closedir(dir);
}

// Can't use Mutex because it would be recursive
static pthread_mutex_t *handler_lock = 0;


static const char* signal_titles[] =
{
	"NULL",
	"SIGHUP",
	"SIGINT",
	"SIGQUIT",
	"SIGILL",
	"SIGTRAP",
	"SIGABRT",
	"SIGBUS",
	"SIGFPE",
	"SIGKILL",
	"SIGUSR1",
	"SIGSEGV",
	"SIGUSR2",
	"SIGPIPE",
	"SIGALRM",
	"SIGTERM"
};

void BC_Signals::dump_stack(FILE *fp)
{
	void *buffer[256];
	int total = backtrace (buffer, 256);
	char **result = backtrace_symbols (buffer, total);
	fprintf(fp, "BC_Signals::dump_stack\n");
	for(int i = 0; i < total; i++)
	{
		fprintf(fp, "%s\n", result[i]);
	}
}

// Kill subprocesses
void BC_Signals::kill_subs()
{
// List /proc directory
	struct dirent64 *new_filename;
	struct stat ostat;
	char path[BCTEXTLEN], string[BCTEXTLEN];
	DIR *dirstream = opendir("/proc");
	if( !dirstream ) return;
	pid_t ppid = getpid();

	while( (new_filename = readdir64(dirstream)) != 0 ) {
		char *ptr = new_filename->d_name;
		while( *ptr && *ptr != '.' && !isalpha(*ptr) ) ++ptr;
// All digits are numbers
		if( *ptr ) continue;

// Must be a directory
		sprintf(path, "/proc/%s", new_filename->d_name);
		if( stat(path, &ostat) ) continue;
		if( !S_ISDIR(ostat.st_mode) ) continue;
		strcat(path, "/stat");
		FILE *fd = fopen(path, "r");
		if( !fd ) continue;
		while( !feof(fd) && fgetc(fd)!=')' );
//printf("kill_subs %d %d\n", __LINE__, c);
		for( int sp=2; !feof(fd) && sp>0; )
			if( fgetc(fd) == ' ' ) --sp;
// Read in parent process
		for( ptr=string; !feof(fd) && (*ptr=fgetc(fd))!=' '; ++ptr );
		if( (*ptr=fgetc(fd)) == ' ' ) break;
		*ptr = 0;

// printf("kill_subs %d process=%d getpid=%d parent_process=%d\n",
// __LINE__, atoi(new_filename->d_name), getpid(), atoi(string));
		int parent_process = atoi(string);
// Kill if we're the parent
		if( ppid == parent_process ) {
			int child_process = atoi(new_filename->d_name);
//printf("kill_subs %d: process=%d\n", __LINE__, atoi(new_filename->d_name));
			kill(child_process, SIGKILL);
		}
		fclose(fd);
	}
}

static void signal_entry(int signum)
{
	signal(signum, SIG_DFL);

	pthread_mutex_lock(handler_lock);
	int done = signal_done;
	signal_done = 1;
	pthread_mutex_unlock(handler_lock);
	if( done ) exit(0);

	printf("signal_entry: got %s my pid=%d execution table size=%d:\n",
		signal_titles[signum], getpid(), execution_table.size);

	BC_Signals::kill_subs();
	BC_Trace::dump_traces();
	BC_Trace::dump_locks();
	BC_Trace::dump_buffers();
	BC_Trace::delete_temps();

// Call user defined signal handler
	BC_Signals::global_signals->signal_handler(signum);

	abort();
}

static void signal_entry_recoverable(int signum)
{
	printf("signal_entry_recoverable: got %s my pid=%d\n",
		signal_titles[signum],
		getpid());
}

// used to terminate child processes when program terminates
static void handle_exit(int signum)
{
//printf("child %d exit\n", getpid());
	exit(0);
}

void BC_Signals::set_sighup_exit(int enable)
{
	if( enable ) {
// causes SIGHUP to be generated when parent dies
		signal(SIGHUP, handle_exit);
		prctl(PR_SET_PDEATHSIG, SIGHUP, 0,0,0);
// prevents ^C from signalling child when attached to gdb
		setpgid(0, 0);
		if( isatty(0) ) ioctl(0, TIOCNOTTY, 0);
	}
	else {
		signal(SIGHUP, signal_entry);
		prctl(PR_SET_PDEATHSIG, 0,0,0,0);
	}
}

BC_Signals::BC_Signals()
{
}
BC_Signals::~BC_Signals()
{
  BC_CModels::bcxfer_stop_slicers();
}


int BC_Signals::x_error_handler(Display *display, XErrorEvent *event)
{
	char string[1024];
	XGetErrorText(event->display, event->error_code, string, 1024);
	fprintf(stderr, "BC_Signals::x_error_handler: error_code=%d opcode=%d,%d id=0x%jx %s\n",
		event->error_code, event->request_code, event->minor_code,
		(int64_t)event->resourceid, string);
	return 0;
}


void BC_Signals::initialize(const char *trap_path)
{
	BC_Signals::global_signals = this;
	BC_Trace::global_trace = this;
	set_trap_path(trap_path);
	handler_lock = (pthread_mutex_t*)calloc(1, sizeof(pthread_mutex_t));
	pthread_mutex_init(handler_lock, 0);
	old_err_handler = XSetErrorHandler(x_error_handler);
	initialize2();
}

void BC_Signals::terminate()
{
	BC_Signals::global_signals = 0;
	BC_Trace::global_trace = 0;
	uncatch_segv();  uncatch_intr();
	signal(SIGHUP, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGFPE, SIG_DFL);
	signal(SIGPIPE, SIG_DFL);
	signal(SIGUSR2, SIG_DFL);
	XSetErrorHandler(old_err_handler);
}

// kill SIGUSR2
void BC_Signals::signal_dump(int signum)
{
	BC_KeyboardHandler::kill_grabs();
	dump();
	signal(SIGUSR2, signal_dump);
}






void BC_Signals::initialize2()
{
	signal(SIGHUP, signal_entry);
	signal(SIGINT, signal_entry);
	signal(SIGQUIT, signal_entry);
	// SIGKILL cannot be stopped
	// signal(SIGKILL, signal_entry);
	catch_segv();
	signal(SIGTERM, signal_entry);
	signal(SIGFPE, signal_entry);
	signal(SIGPIPE, signal_entry_recoverable);
	signal(SIGUSR2, signal_dump);
}


void BC_Signals::signal_handler(int signum)
{
printf("BC_Signals::signal_handler\n");
//	exit(0);
}

const char* BC_Signals::sig_to_str(int number)
{
	return signal_titles[number];
}


#include <ucontext.h>
#include <sys/wait.h>
#include "thread.h"

#if __i386__
#define IP eip
#endif
#if __x86_64__
#define IP rip
#endif
#ifndef IP
#error gotta have IP
#endif


static void handle_dump(int n, siginfo_t * info, void *sc)
{
	uncatch_segv();  uncatch_intr();
	signal(SIGSEGV, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	// gotta be root, or the dump is worthless
	int uid = getuid();
// it is not necessary to be root if ptrace is allowed via:
// echo 0 > /proc/sys/kernel/yama/ptrace_scope (usually set to 1)
//	if( uid != 0 ) return;
	ucontext_t *uc = (ucontext_t *)sc;
	int pid = getpid(), tid = gettid();
	struct sigcontext *c = (struct sigcontext *)&uc->uc_mcontext;
	uint8_t *ip = (uint8_t *)c->IP;
	fprintf(stderr,"** %s at %p in pid %d, tid %d\n",
		n==SIGSEGV? "segv" : n==SIGINT? "intr" : "trap",
		(void*)ip, pid, tid);
	FILE *fp = 0;
	char fn[PATH_MAX];
	if( BC_Signals::trap_path ) {
		snprintf(fn, sizeof(fn), BC_Signals::trap_path, pid);
		fp = fopen(fn,"w");
	}
	if( fp ) {
		fprintf(stderr,"writing debug data to %s\n", fn);
		fprintf(fp,"** %s at %p in pid %d, tid %d\n",
			n==SIGSEGV? "segv" : n==SIGINT? "intr" : "trap",
			(void*)c->IP, pid, tid);
	}
	else {
		strcpy(fn, "stdout");
		fp = stdout;
	}
	time_t t;  time(&t);
	fprintf(fp,"created on %s", ctime(&t));
	struct passwd *pw = getpwuid(uid);
	if( pw ) {
		fprintf(fp,"        by %d:%d %s(%s)\n",
			pw->pw_uid, pw->pw_gid, pw->pw_name, pw->pw_gecos);
	}
	fprintf(fp,"\nCPUS: %d\n",   BC_Resources::get_machine_cpus());
	fprintf(fp,"\nCPUINFO:\n");  bc_copy_textfile(32, fp,"/proc/cpuinfo");
	fprintf(fp,"\nTHREADS:\n");  BC_Trace::dump_threads(fp);
	fprintf(fp,"\nTRACES:\n");   BC_Trace::dump_traces(fp);
	fprintf(fp,"\nLOCKS:\n");    BC_Trace::dump_locks(fp);
	fprintf(fp,"\nBUFFERS:\n");  BC_Trace::dump_buffers(fp);
	fprintf(fp,"\nSHMMEM:\n");   BC_Trace::dump_shm_stats(fp);
	if( BC_Signals::trap_hook ) {
		fprintf(fp,"\nMAIN HOOK:\n");
		BC_Signals::trap_hook(fp, BC_Signals::trap_data);
	}
	fprintf(fp,"\nVERSION:\n");  bc_copy_textfile(INT_MAX, fp,"/proc/version");
	fprintf(fp,"\nMEMINFO:\n");  bc_copy_textfile(INT_MAX, fp,"/proc/meminfo");
	fprintf(fp,"\nSTATUS:\n");   bc_copy_textfile(INT_MAX, fp,"/proc/%d/status",pid);
	fprintf(fp,"\nFD:\n");	     bc_list_openfiles(INT_MAX, fp,"/proc/%d/fd", pid);
	fprintf(fp,"\nMAPS:\n");     bc_copy_textfile(INT_MAX, fp,"/proc/%d/maps",pid);
	char proc_mem[64];
	if( tid > 0 && tid != pid )
		sprintf(proc_mem,"/proc/%d/task/%d/mem",pid,tid);
	else
		sprintf(proc_mem,"/proc/%d/mem",pid);
	int pfd = open(proc_mem,O_RDONLY);
	if( pfd >= 0 ) {
		fprintf(fp,"\nCODE:\n");
		for( int i=-32; i<32; ) {
			uint8_t v;  void *vp = (void *)(ip + i);
			if( !(i & 7) ) fprintf(fp,"%p:  ", vp);
			if( pread(pfd,&v,sizeof(v),(off_t)vp) != sizeof(v) ) break;
			fprintf(fp,"%c%02x", !i ? '>' : ' ', v);
			if( !(++i & 7) ) fprintf(fp,"\n");
		}
		fprintf(fp,"\n");
		close(pfd);
	}
	else
		fprintf(fp,"err opening: %s, %m\n", proc_mem);

	fprintf(fp,"\n\n");
	if( fp != stdout ) fclose(fp);
	char cmd[1024], *cp = cmd;
	cp += sprintf(cp, "exec gdb /proc/%d/exe -p %d --batch --quiet "
		"-ex \"thread apply all info registers\" "
		"-ex \"thread apply all bt full\" "
		"-ex \"quit\"", pid, pid);
	if( fp != stdout )
		cp += sprintf(cp," >> \"%s\"", fn);
	cp += sprintf(cp," 2>&1");
//printf("handle_dump:: pid=%d, cmd='%s'  fn='%s'\n",pid,cmd,fn);
        pid = vfork();
        if( pid < 0 ) {
		fprintf(stderr,"** can't start gdb, dump abondoned\n");
		return;
	}
	if( pid > 0 ) {
		waitpid(pid,0,0);
		fprintf(stderr,"** dump complete\n");
		return;
	}
        char *const argv[4] = { (char*) "/bin/sh", (char*) "-c", cmd, 0 };
        execvp(argv[0], &argv[0]);
}

