#include "thread.h"
#include <stdio.h>
#include <signal.h>

void *
Thread::ThreadProc (void *param)
{
  Thread *thread = (Thread *) param;
  int r = mutexLock(thread->mutex);
  if (r < 0) {
    thread->running = false;
    perror ("ThreadProc:mutexLock");
    return 0;
  }
  thread->Proc ();
  thread->running = false;

  // this unsuspends a waiting "Kill" below
  r = mutexUnlock (thread->mutex);
  if (r < 0) {
    perror("ThreadProc:mutexUnlock");
  }
  return 0;
}

void
Thread::Run ()
{
  if (!running) {
    running = true;
    int r = threadCreate(p,ThreadProc,this);
    if (r != 0) {
      running = false;
      perror ("Thread::Run:threadCreate");
    }
  }
}

void
Thread::Kill ()
{
  if (running) {
    running = false;		// signals thread to stop
    // now wait for clean thread exit (mutex unlock)
    mutexLock(mutex);
    mutexUnlock(mutex);
  }
}

static void sigquit(int sig) {}
void
Thread::Kill1()
{
  if (running) {
    void (*ohr)(int sig) = signal(SIGQUIT,sigquit);
    running = false;		// signals thread to stop
    // now wait for clean thread exit (mutex unlock)
    threadKill(p);    // give it a push
    mutexLock(mutex);
    mutexUnlock(mutex);
    signal(SIGQUIT,ohr);
  }
}

