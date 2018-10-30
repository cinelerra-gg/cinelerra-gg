#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdarg.h>
#include<time.h>

#include "tdb.h"
namespace old {
#include "../x.C"
};
#include "s.C"

int main(int ac, char **av)
{
  int ret;  setbuf(stdout,0);
  old::theDb db;
  db.open(av[1]);
  //db.access(av[1], 34543, 0);
  if( !db.opened() || db.error() ) exit(1);

  if( (ret=db.Clip_set.add_kindex("Clip_path_pos")) ) {
    printf("add kindex failed %d\n",ret);
    exit(1);
  }
 
  Db::pgRef clip; 
  if( !(ret=db.clip_set.FirstId(clip)) ) do {
    int id = db.clip_set.id();
    Clip_setLoc::rkey_Clip_path_pos rkey(db.clip_set);
    if( (ret=db.Clip_set.index("Clip_path_pos")->Insert(rkey,&id)) ) {
      printf("insert clip_path_pos failed %d, id = %d\n",ret,id);
      exit(1);
    }
  } while( !(ret=db.clip_set.NextId(clip)) );

  db.commit();
  db.close();
  return 0;
}

