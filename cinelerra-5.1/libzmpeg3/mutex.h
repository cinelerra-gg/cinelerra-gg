#ifndef _included_mutex
#define _included_mutex

#include <pthread.h>
class Mutex
{
private:pthread_mutex_t m;
public:Mutex ()
  {
    pthread_mutex_init (&m, 0);
  }
   ~Mutex ()
  {
    pthread_mutex_destroy (&m);
  }
  operator pthread_mutex_t & ()
  {
    return m;
  }
};

class CriticalSection
{
private:pthread_mutex_t * m;
public:CriticalSection (pthread_mutex_t & _m)
  {
    m = &_m;
    pthread_mutex_lock (m);
  }
   ~CriticalSection ()
  {
    pthread_mutex_unlock (m);
  }
};


#endif /*  */
