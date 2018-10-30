#include<stdio.h>
#include<stdarg.h>
#include<signal.h>
#include<time.h>

#include "tdb.h"
#include "s.C"

int main(int ac, char **av)
{
  setbuf(stdout,0);
  theDb db;
  if( db.open(av[1], 34543) ) return 1;
  printf("clip_sets %ld\n", db.clip_set.index(0)->Count());
  printf("timelines %ld\n", db.timeline.index(1)->Count());
  printf("sequences %ld\n", db.timeline.index(2)->Count());
  printf("frames %ld\n", db.video_frame.index(0)->Count());
  db.close();
  return 0;
}

