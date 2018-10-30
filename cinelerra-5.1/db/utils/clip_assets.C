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
    printf("%s %f %f %f\n", db.clip_set._Asset_path(), db.clip_set.Framerate(),
	db.clip_set.Position(), db.clip_set.Frames()/db.clip_set.Framerate());
  } while( !(ret=db.clip_set.NextId()) );

  db.close();
  return 0;
}

