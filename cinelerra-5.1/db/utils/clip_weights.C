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
    char fn[1024];  sprintf(fn,"%s/w%05d.txt",av[2],db.clip_set.id());
    FILE *fp = fopen(fn,"w");
    double *weights = (double *)db.clip_set._Weights();
    int frames = db.clip_set.Frames();
    for( int i=0; i<frames; ++i )
      fprintf(fp,"%f\n",weights[i]);
    fclose(fp);
  } while( !(ret=db.clip_set.NextId()) );

  db.close();
  return 0;
}

