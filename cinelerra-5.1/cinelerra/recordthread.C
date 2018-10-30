
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "asset.h"
#include "batch.h"
#include "channel.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "question.h"
#include "record.h"
#include "recordgui.h"
#include "recordthread.h"


RecordThread::RecordThread(MWindow *mwindow, Record *record)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->record = record;
	batch_timed_lock = new Condition(0,"RecordThread::batch_timed_lock");

	done = 0;
	cron_active = 0;
}

RecordThread::~RecordThread()
{
	if( Thread::running() ) {
		done = 1;
		batch_timed_lock->unlock();
		Thread::cancel();
	}
	Thread::join();
	delete batch_timed_lock;
}

int RecordThread::cron(Batch *batch)
{
	while( !done ) {
		batch->update_times();
		time_t t;  time(&t);
		int delay = (int)(batch->time_start - t);
		int duration = (int)(batch->time_end - t);
		if( delay <= 1 ) {
			if( duration < 1 ) return -1;
			batch->record_duration = duration+1;
			return 1;
		}
		if( delay < 0 || delay > 60 ) delay = 60;
		else if( (delay -= 3) < 0 ) delay = 1;
		delay *= 1000000;
		Thread::enable_cancel();
		batch_timed_lock->timed_lock(delay, "RecordThread::cron");
		Thread::disable_cancel();
	}
	return 0;
}

void RecordThread::run()
{
	int batch_no = record->get_next_batch(0);
	done = batch_no >= 0 ? 0 : 1;
	if( done ) {
		QuestionWindow *qwindow = new QuestionWindow(mwindow);
		qwindow->create_objects(_("Re-enable batches and restart?"),1);
		int result = qwindow->run_window();
		delete qwindow;
		if( result == 2 ) {
			int n = record->record_batches.total();
			for( int i=0; i<n; ++i )
				record->record_batches[i]->enabled = 1;
			record->activate_batch(batch_no = 0);
			record->record_gui->update_batches();
			done = 0;
		}
	}

	while( !done ) {
		batch_timed_lock->reset();
		Batch *batch = record->get_current_batch();
		Channel *current_channel = record->get_current_channel();
		Channel *batch_channel = batch->get_current_channel();
//printf("RecordThread::current channel %s\n",current_channel ? current_channel->title : "None");
//printf("RecordThread::batch channel %s\n",batch_channel ? batch_channel->title : "None");
		if( *current_channel != *batch_channel ) {
			if( batch_channel ) {
				current_channel = batch_channel;
//printf("RecordThread::set channel %s\n",batch_channel->title);
				record->set_channel(batch_channel);
				record->has_signal();
			}
		}
		int result = cron(batch);
		if( result > 0 ) {
			cron_active = 1;
			batch->recorded = 0;
			record->pause_input_threads();
			record->record_gui->reset_audio();
			record->record_gui->reset_video();
			record->update_position();
			record->start_writing_file();
			record->resume_input_threads();
printf("RecordThread::Started\n");
			batch_timed_lock->lock("RecordThread::run");
printf("RecordThread::Done\n");
			record->stop_writing();
			cron_active = 0;
		}
		if( result ) {
			batch_no = record->get_next_batch();
			if( batch_no < 0 ) { done = -1; batch_no = 0; }
			record->activate_batch(batch_no);
		}
	}

	record->record_gui->update_cron_status(_("Done"));
	record->record_gui->enable_batch_buttons();
	if( record->power_off && done < 0 )
	{
		mwindow->save_defaults();
		sleep(2);  sync();
		pid_t pid = vfork();
		if( pid == 0 ) {
			const char poweroff[] = "poweroff";
			char *const argv[] = { (char*)poweroff, 0 };
			execvp(poweroff ,&argv[0]);
			perror(_("execvp poweroff failed"));
			exit(1);
		}
		if( pid > 0 )
			fprintf(stderr,_("poweroff imminent!!!\n"));
		else
			perror(_("cant vfork poweroff process"));
	}
	cron_active = -1;
}


