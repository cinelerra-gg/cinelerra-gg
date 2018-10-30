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

  TimelineLoc::rkey_Sequences key(db.timeline);
  if( !(ret=key.First()) ) {
    int clip_id = db.timeline.Clip_id();
    int n = 1;
    while( !(ret=key.Next()) ) {
printf("(%d,%d,%d),",db.timeline.Clip_id(),db.timeline.Sequence_no(),db.timeline.Frame_id());
      if( clip_id != (int)db.timeline.Clip_id() ) {
        if( db.clip_set.FindId(clip_id) ) {
          printf(" broken clip %d\n",clip_id);  exit(1);
        }
        printf("clip %d has %d frames\n",clip_id,n);
        n = 0;
        clip_id = db.timeline.Clip_id();
      }
      ++n;
    }
    if( db.clip_set.FindId(clip_id) ) {
      printf(" broken last %d\n",clip_id);  exit(1);
    }
    printf("last %d has %d frames\n",clip_id,n);
  }

  db.close();
  return 0;
}

