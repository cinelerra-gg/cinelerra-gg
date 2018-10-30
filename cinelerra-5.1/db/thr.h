#ifndef __THR_H__
#define __THR_H__
#include <cstdio>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

class Mutex;
class Condition;

class Mutex {
  pthread_mutex_t mutex;
public:
  friend class Condition;
  Mutex() { pthread_mutex_init(&mutex, 0); }
  ~Mutex() { pthread_mutex_destroy(&mutex); }
  int lock() { return pthread_mutex_lock(&mutex); }
  int trylock() { return pthread_mutex_trylock(&mutex); }
  int unlock() { return pthread_mutex_unlock(&mutex); }
  void reset() { pthread_mutex_destroy(&mutex); pthread_mutex_init(&mutex,0); }
};

class Condition : public Mutex {
  pthread_cond_t cond;
  int init_v;
  volatile int v;
public:
  int lock() {
    Mutex::lock();  while(v <= 0) pthread_cond_wait(&cond, &mutex);
    --v;  Mutex::unlock(); return 0;
  }
  void unlock() { Mutex::lock(); ++v; pthread_cond_signal(&cond); Mutex::unlock(); }
  Condition(int iv=1) : init_v(iv) {
    v = init_v;  pthread_cond_init(&cond, NULL);
  }
  ~Condition() { pthread_cond_destroy(&cond); }
  void reset() {
    pthread_cond_destroy(&cond); pthread_cond_init(&cond, NULL);
    Mutex::reset();  v = 1;
  }
};

class Thread {
private:
  int started, active;
  pthread_t owner_tid, tid;
public:
  virtual void run() = 0;
  static void *proc(void *t) { ((Thread*)t)->run(); return 0; }
  pthread_t owner() { return owner_tid; }
  pthread_t self() { return tid; }
  int running() { return active; }
  int cancel() {
    if( started ) { started = 0; pthread_cancel(tid); }
    return 0;
  }
  void start() {
    started = active = 1;
    pthread_create(&tid, 0, proc,(void*)this);
  }
  void join() { if( active ) { pthread_join(tid, 0); active = 0; } }
  void pause() { pthread_kill(tid, SIGSTOP); }
  void resume() { pthread_kill(tid, SIGCONT); }
  Thread() { started = active = 0; owner_tid = pthread_self(); }
  virtual ~Thread() {}

};

// old stuff

class thread : public Thread {
  int done;
  Condition ready;
public:
  thread() { done = 0;  ready.lock();  start(); }
  ~thread() { Kill(); }

  virtual void Proc () = 0;
  void run() { while( !done ) { ready.lock(); if( !done ) Proc(); } }
  void Run() { ready.unlock(); }
  void Kill() { done = 1; Run(); join(); }
};

class Lock : public Mutex {};

static inline void yield() { pthread_yield(); }

#endif
