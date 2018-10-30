#ifndef _included_thread
#define _included_thread

#ifdef TCL_THREADS
#include <tcl.h>
#define ThreadId Tcl_ThreadId
#define tMutex Tcl_Mutex
#define mutexInit(m) m = NULL
#define mutexDestroy(m) Tcl_MutexFinalize(&m)
#define mutexLock(m) (Tcl_MutexLock(&m), 0)
#define mutexUnlock(m) (Tcl_MutexUnlock(&m), 0)
#define threadCreate(p,fn,dt) Tcl_CreateThread(&p,v##fn,dt,TCL_THREAD_STACK_DEFAULT,0)
#define threadKill(p)
#else
#include <pthread.h>
#define ThreadId pthread_t
#define tMutex pthread_mutex_t
#define mutexInit(m) pthread_mutex_init(&m,0)
#define mutexDestroy(m) pthread_mutex_destroy(&m)
#define mutexLock(m) pthread_mutex_lock(&m)
#define mutexUnlock(m) pthread_mutex_unlock(&m)
#define threadCreate(p,fn,dt) pthread_create(&p,0,fn,dt)
#define threadKill(p) pthread_kill(p,SIGQUIT)
#endif

class Thread
{
private:
  bool running;
  ThreadId p;
  tMutex mutex;
  static void *ThreadProc (void *);
  friend void vThreadProc (void *param) { ThreadProc(param); }
public:  Thread ():running (false)
  {
    mutexInit(mutex);
  }
  virtual ~ Thread ()
  {
    Kill ();
    mutexDestroy(mutex);
  }
  void Run ();
  void PostKill ()
  {
    running = false;
  }
  void Kill ();
  void Kill1();
  bool Running ()
  {
    return running;
  }
  virtual void Proc () = 0;
  void Lock() { mutexLock(mutex); }
  void Unlock() { mutexUnlock(mutex); }
};

#endif /*  */
