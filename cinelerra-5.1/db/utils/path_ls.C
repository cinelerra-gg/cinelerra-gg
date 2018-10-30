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

  Db::pgRef clip;
  Clip_setLoc::rkey_Clip_path_pos rkey(db.clip_set);
  if( !(ret=rkey.First(clip)) ) do {
    if( Clip_viewsLoc::ikey_Clip_access(db.clip_views,db.clip_set.id()).Find() ) {
      printf("clip %d, missed\n", db.clip_set.id()); continue;
    }
    time_t t = (time_t) db.clip_views.Access_time();
    printf("clip %u, %s (%5.2f) %d=%d+%d, %f+%f, wt%f %d %s", db.clip_set.id(),
	db.clip_set._Asset_path(), db.clip_set.Framerate(),
	db.clip_set.Frames(), db.clip_set.Prefix_size(), db.clip_set.Suffix_size(),
	db.clip_set.Position(), db.clip_set.Frames()/db.clip_set.Framerate(),
        db.clip_set.Average_weight(), db.clip_views.Access_count() ,ctime(&t));
  } while( !(ret=rkey.Next(clip)) );

  db.close();
  return 0;
}

