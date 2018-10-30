#ifndef __RECORDTHREAD_H__
#define __RECORDTHREAD_H__

#include "batch.inc"
#include "condition.inc"
#include "mwindow.inc"
#include "record.inc"

class RecordThread : public Thread
{
public:
	RecordThread(MWindow *mwindow, Record *record);
	~RecordThread();
	int cron(Batch *batch);
	void run();

	MWindow *mwindow;
	Record *record;
	Condition *batch_timed_lock;

	int done;
	int cron_active;
};

#endif
