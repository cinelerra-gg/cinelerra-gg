/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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

#include "aboutprefs.h"
#include "arraylist.h"
#include "batchrender.h"
#include "bcsignals.h"
#include "edl.h"
#include "file.h"
#include "filexml.h"
#include "filesystem.h"
#include "language.h"
#include "langinfo.h"
#include "loadfile.inc"
#include "mainmenu.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "pluginserver.h"
#include "preferences.h"
#include "renderfarmclient.h"
#include "units.h"
#include "versioninfo.h"

#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifdef LEAKER
#define STRC(v) printf("==new %p from %p sz %jd\n", v, __builtin_return_address(0), n)
#define STRD(v) printf("==del %p from %p\n", v, __builtin_return_address(0))
void *operator new(size_t n) { void *vp = malloc(n); STRC(vp); bzero(vp,n); return vp; }
void operator delete(void *t) { STRD(t); free(t); }
void operator delete(void *t,size_t n) { STRD(t); free(t); }
void *operator new[](size_t n) { void *vp = malloc(n); STRC(vp); bzero(vp,n); return vp; }
void operator delete[](void *t) { STRD(t); free(t); }
void operator delete[](void *t,size_t n) { STRD(t); free(t); }
#endif

enum
{
	DO_GUI,
	DO_DEAMON,
	DO_DEAMON_FG,
	DO_BRENDER,
	DO_USAGE,
	DO_BATCHRENDER
};

#include "thread.h"


class CommandLineThread : public Thread
{
public:
	CommandLineThread(ArrayList<char*> *filenames,
		MWindow *mwindow)
	{
		this->filenames = filenames;
		this->mwindow = mwindow;
	}


	~CommandLineThread()
	{
	}

	void run()
	{
//PRINT_TRACE
		mwindow->gui->lock_window("main");
//PRINT_TRACE
		mwindow->load_filenames(filenames, LOADMODE_REPLACE);
//PRINT_TRACE
		if( filenames->size() == 1 )
			mwindow->gui->mainmenu->add_load(filenames->get(0));
//PRINT_TRACE
		mwindow->gui->unlock_window();
//PRINT_TRACE
	}

	MWindow *mwindow;
	ArrayList<char*> *filenames;
};

long cin_timezone;

int main(int argc, char *argv[])
{
// handle command line arguments first
	srand(time(0));
	ArrayList<char*> filenames;
	FileSystem fs;

	time_t st; time(&st);
	struct tm ltm, gtm;
	localtime_r(&st, &ltm);
	gmtime_r(&st, &gtm);
	int tzofs = ltm.tm_hour - gtm.tm_hour;
	cin_timezone = tzofs * 60*60;

	int operation = DO_GUI;
	int deamon_port = DEAMON_PORT;
	char deamon_path[BCTEXTLEN];
	char config_path[BCTEXTLEN];
	char batch_path[BCTEXTLEN];
	int nice_value = 20;
	int load_perpetual = 1;
	config_path[0] = 0;
	batch_path[0] = 0;
	deamon_path[0] = 0;
	Units::init();

	File::init_cin_path();
	const char *locale_path = File::get_locale_path();
	const char *cin = File::get_cin();

	bindtextdomain(cin, locale_path);
	textdomain(cin);
	setlocale(LC_MESSAGES, "");

#ifdef X_HAVE_UTF8_STRING
	char *loc = setlocale(LC_CTYPE, "");
	if( loc ) {
		strcpy(BC_Resources::encoding, nl_langinfo(CODESET));
		BC_Resources::locale_utf8 = !strcmp(BC_Resources::encoding, "UTF-8");

		// Extract from locale language & region
		char locbuf[32], *p;
		locbuf[0] = 0;
		if( (p = strchr(loc, '.')) != 0 && (p - loc) < (int)sizeof(locbuf)-1 ) {
			strncpy(locbuf, loc, p - loc);
			locbuf[p - loc] = 0;
		}
		else if( strlen(loc) < sizeof(locbuf)-1 )
			strcpy(locbuf, loc);

                // Locale 'C' does not give useful language info - assume en
		if( !locbuf[0] || locbuf[0] == 'C' )
			strcpy(locbuf, "en");

		if( (p = strchr(locbuf, '_')) && p - locbuf < LEN_LANG ) {
			*p++ = 0;
			strcpy(BC_Resources::language, locbuf);
			if( strlen(p) < LEN_LANG )
				strcpy(BC_Resources::region, p);
		}
		else if( strlen(locbuf) < LEN_LANG )
			strcpy(BC_Resources::language, locbuf);
	}
	else
		printf(_(PROGRAM_NAME ": Could not set locale.\n"));
#else
        setlocale(LC_CTYPE, "");
#endif
	tzset();

	int load_backup = 0;
	int start_remote_control = 0;

	for( int i = 1; i < argc; i++ ) {
		if( !strcmp(argv[i], "-h") ) {
			operation = DO_USAGE;
		}
		else if( !strcmp(argv[i], "-z") ) {
			start_remote_control = 1;
		}
		else if( !strcmp(argv[i], "-r") ) {
			operation = DO_BATCHRENDER;
			if( argc > i + 1 ) {
				if( argv[i + 1][0] != '-' ) {
					strcpy(batch_path, argv[i + 1]);
					i++;
				}
			}
		}
		else if( !strcmp(argv[i], "-c") ) {
			if( argc > i + 1 ) {
				strcpy(config_path, argv[i + 1]);
				i++;
			}
			else {
				fprintf(stderr, _("%s: -c needs a filename.\n"), argv[0]);
			}
		}
		else if( !strcmp(argv[i], "-d") || !strcmp(argv[i], "-f") ) {
			operation = !strcmp(argv[i], "-d") ? DO_DEAMON : DO_DEAMON_FG;
			if( argc > i + 1 ) {
				if( atol(argv[i + 1]) > 0 ) {
					deamon_port = atol(argv[i + 1]);
					i++;
				}
			}
		}
		else if( !strcmp(argv[i], "-b") ) {
			operation = DO_BRENDER;
			if( i > argc - 2 ) {
				fprintf(stderr, _("-b may not be used by the user.\n"));
				exit(1);
			}
			else
				strcpy(deamon_path, argv[i + 1]);
		}
		else if( !strcmp(argv[i], "-n") ) {
			if( argc > i + 1 ) {
				nice_value = atol(argv[i + 1]);
				i++;
			}
		}
		else if( !strcmp(argv[i], "-x") ) {
			load_backup = 1;
		}
		else if( !strcmp(argv[i], "-S") ) {
			load_perpetual = 0;
		}
		else {
			char *new_filename;
			new_filename = new char[BCTEXTLEN];
			strcpy(new_filename, argv[i]);
			fs.complete_path(new_filename);
			filenames.append(new_filename);
		}
	}



	if( operation == DO_GUI ||
	    operation == DO_DEAMON || operation == DO_DEAMON_FG ||
	    operation == DO_USAGE  || operation == DO_BATCHRENDER) {

#ifndef REPOMAINTXT
#define REPOMAINTXT ""
#endif
#ifndef COPYRIGHTTEXT1
#define COPYRIGHTTEXT1 ""
#endif
#ifndef COPYRIGHTTEXT2
#define COPYRIGHTTEXT2 ""
#endif
		fprintf(stderr, "%s %s - %s\n%s",
			PROGRAM_NAME,CINELERRA_VERSION, AboutPrefs::build_timestamp,
			REPOMAINTXT COPYRIGHTTEXT1 COPYRIGHTTEXT2);
		fprintf(stderr, "%s is free software, covered by the GNU General Public License,\n"
			"and you are welcome to change it and/or distribute copies of it under\n"
			"certain conditions. There is absolutely no warranty for %s.\n\n",
			PROGRAM_NAME, PROGRAM_NAME);
	}

	switch( operation ) {
		case DO_USAGE:
			printf(_("\nUsage:\n"));
			printf(_("%s [-f] [-c configuration] [-d port] [-n nice] [-r batch file] [filenames]\n\n"), argv[0]);
			printf(_("-d = Run in the background as renderfarm client.  The port (400) is optional.\n"));
			printf(_("-f = Run in the foreground as renderfarm client.  Substitute for -d.\n"));
			printf(_("-n = Nice value if running as renderfarm client. (19)\n"));
			printf(_("-c = Configuration file to use instead of %s/%s.\n"),
				File::get_config_path(), CONFIG_FILE);
			printf(_("-r = batch render the contents of the batch file (%s/%s) with no GUI.  batch file is optional.\n"),
				File::get_config_path(), BATCH_PATH);
			printf(_("-S = do not reload perpetual session\n"));
			printf(_("-x = reload from backup\n"));
			printf(_("filenames = files to load\n\n\n"));
			exit(0);
			break;

		case DO_DEAMON:
		case DO_DEAMON_FG: {
			if( operation == DO_DEAMON ) {
				int pid = fork();

				if( pid ) {
// Redhat 9 requires _exit instead of exit here.
					_exit(0);
				}
			}

			RenderFarmClient client(deamon_port,
				0,
				nice_value,
				config_path);
			client.main_loop();
			break; }

// Same thing without detachment
		case DO_BRENDER: {
			RenderFarmClient client(0,
				deamon_path,
				20,
				config_path);
			client.main_loop();
			break; }

		case DO_BATCHRENDER: {
			BatchRenderThread *thread = new BatchRenderThread(0);
			thread->start_rendering(config_path,
				batch_path);
			break; }

		case DO_GUI: {
			int restart = 0, done = 0;
			while( !done ) {
				BC_WindowBase::get_resources()->vframe_shm = 0;
				MWindow mwindow;
				mwindow.create_objects(1, !filenames.total, config_path);
				CommandLineThread *thread  = 0;
				if( mwindow.preferences->perpetual_session && load_perpetual )
					mwindow.load_undo_data();
//SET_TRACE
// load the initial files on seperate tracks
// use a new thread so it doesn't block the GUI
				if( filenames.total ) {
					thread  = new CommandLineThread(&filenames, &mwindow);
					thread->start();
//PRINT_TRACE
				}
				if( load_backup ) {
					load_backup = 0;
					mwindow.gui->lock_window("main");
					mwindow.load_backup();
					mwindow.gui->unlock_window();
				}
				if( start_remote_control ) {
					start_remote_control = 0;
					mwindow.gui->remote_control->activate();
				}
// run the program
//PRINT_TRACE
				mwindow.start();
				mwindow.run();
//PRINT_TRACE
				restart = mwindow.restart();
				if( restart ) {
					mwindow.save_backup();
					load_backup = 1;
					start_remote_control =
						mwindow.gui->remote_control->is_active();
				}
				if( restart <= 0 )
					done = 1;

				mwindow.save_defaults();
				if( mwindow.preferences->perpetual_session )
					mwindow.save_undo_data();
//PRINT_TRACE
				filenames.remove_all_objects();
				delete thread;
			}

			if( restart < 0 ) {
				char exe_path[BCTEXTLEN];
				int len = readlink("/proc/self/exe", exe_path, sizeof(exe_path)-1);
				if( len < 0 ) break;
				exe_path[len] = 0;
				char *av[4] = { 0, };  int ac = 0;
				av[ac++] = exe_path;
				if( load_backup ) av[ac++] = (char*) "-x";
				if( start_remote_control ) av[ac++] = (char*) "-z";
				av[ac++] = 0;
				execv(exe_path, av);
			}
//SET_TRACE
DISABLE_BUFFER
		break; }
	}

	filenames.remove_all_objects();
	Units::finit();

	time_t et; time(&et);
	long dt = et - st;
	printf("Session time: %ld:%02ld:%02ld\n", dt/3600, dt%3600/60, dt%60);
	struct rusage ru;
	getrusage(RUSAGE_SELF, &ru);
	long usr_ms = ru.ru_utime.tv_sec*1000 + ru.ru_utime.tv_usec/1000;
	long us = usr_ms/1000;  int ums = usr_ms%1000;
	long sys_ms = ru.ru_stime.tv_sec*1000 + ru.ru_stime.tv_usec/1000;
	long ss = sys_ms/1000;  int sms = sys_ms%1000;
	printf("Cpu time: user: %ld:%02ld:%02ld.%03d sys: %ld:%02ld:%02ld.%03d\n",
		us/3600, us%3600/60, us%60, ums, ss/3600, ss%3600/60, ss%60, sms);
	return 0;
}

