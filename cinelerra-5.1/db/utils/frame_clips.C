
#include<stdio.h>
#include<stdarg.h>
#include<time.h>

#include "tdb.h"
#include "s.C"

int main(int ac, char **av)
{
  setbuf(stdout,0);
  theDb db;
  db.open(av[1]);
  //db.access(av[1], 34543, 0);
  if( !db.opened() || db.error() ) exit(1);
  if( ac > 2 ) {
    unsigned int fid = atoi(av[2]);
    if( TimelineLoc::ikey_Timelines(db.timeline, fid).Locate() ) {
      printf(" find timeline frame_id(%u) failed\n", fid);
      return 1;
    }

    while( db.timeline.Frame_id()==fid ) {
      int cid = db.timeline.Clip_id();
      printf(" %d",cid);
      if( TimelineLoc::rkey_Timelines(db.timeline).Next() ) break;
    }
    printf("\n");
  }
  else {
    if( !db.video_frame.FirstId() ) do {
      unsigned int fid = db.video_frame.id();
      printf("%u",fid);
      if( TimelineLoc::ikey_Timelines(db.timeline, fid).Locate() ) {
        printf(" find timeline frame_id(%u) failed\n", fid);
        continue;
      }
      while( db.timeline.Frame_id()==fid ) {
        int cid = db.timeline.Clip_id();
        printf(" %d",cid);
        if( TimelineLoc::rkey_Timelines(db.timeline).Next() ) break;
      }
      printf("\n");
    } while( !db.video_frame.NextId() );
  }

  db.close();
  return 0;
}

