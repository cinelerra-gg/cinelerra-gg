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

  unsigned int id = atoi(av[2]);
  if( (ret=db.video_frame.FindId(id)) ) {
printf("id(%d) not found, ret = %d\n", id, ret);
    return 1;
  }
  printf("%6d %f %f %f %f %f\n", db.video_frame.id(),
    db.video_frame.Frame_mean(), db.video_frame.Frame_std_dev(),
    db.video_frame.Frame_cx(), db.video_frame.Frame_cy(),
    db.video_frame.Frame_moment());

  db.close();
  return 0;
}

