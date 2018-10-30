#include<stdio.h>
#include<stdarg.h>
#include<time.h>

#include "tdb.h"
#include "s.C"


int main(int ac, char **av)
{
  int ret;  setbuf(stdout,0);
  theDb db;
  db.open(av[1]);
  //db.access(av[1], 34543, 0);
  if( !db.opened() || db.error() ) exit(1);

  if( !(ret=db.clip_set.FindId(atoi(av[2]))) ) {
    printf("clip_id: %d\n", db.clip_set.id());
    printf("title: %s\n",db.clip_set._Title());
    printf("path: %s\n",db.clip_set._Asset_path());
    printf("position: %f\n",db.clip_set.Position());
    printf("framerate: %f\n",db.clip_set.Framerate());
    printf("average_weight: %f\n",db.clip_set.Average_weight());
    printf("frames: %u",db.clip_set.Frames());
    printf(" (%f)\n",db.clip_set.Frames()/db.clip_set.Framerate());
    printf("pre/suffix: %u/%u\n",db.clip_set.Prefix_size(),db.clip_set.Suffix_size());
    time_t st = (time_t)db.clip_set.System_time();
    printf("system time: %s",ctime(&st));
    time_t ct = (time_t)db.clip_set.Creation_time();
    printf("creation time: %s",ctime(&ct));
  }

  db.close();
  return 0;
}

