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

  if( !(ret=db.clip_set.FirstId()) ) do {
    int cid = db.clip_set.id();
    printf("clip_set %d, %f secs ", cid, db.clip_set.Frames()/db.clip_set.Framerate());
    if( !Clip_viewsLoc::ikey_Clip_access(db.clip_views,cid).Find() ) {
      time_t t = (time_t) db.clip_views.Access_time();
      printf("  %d %s", db.clip_views.Access_count(), ctime(&t));
    }
    else
      printf("missed clip_view for cid %d\n",cid);
  } while( !(ret=db.clip_set.NextId()) );

  db.close();
  return 0;
}

