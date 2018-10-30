#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void hist_eq(unsigned char *data, int len, int n)
{
  while( --n >= 0 ) {
    int hist[256];
    for( int i=0; i<256; ++i ) hist[i] = 0;
    unsigned char *bp = data;
    for( int i=len; --i>=0; ++bp ) ++hist[*bp];
    for( int t=0, i=0; i<256; ++i ) { t += hist[i];  hist[i] = t; }

    int mn = 0;    while( mn<255 && !hist[mn] ) ++mn;
    int mx = 256;  while( mx>mn && hist[mx-1]==len ) --mx;
    double r = (double)(mx - mn) / (256-1);
    int map[256];  map[0] = 0;  map[255] = 255;
    double dv = (len-1) / 256., v = dv;
    for( int i=1; i<255; ++i, v+=dv ) {
      int j = i * r + mn;
      map[i] = i * (1-(v - hist[j])/len);
    }
//for( int i=0; i<256; ++i ) fprintf(stderr,"%d\n",map[i]);
    bp = data;
    for( int i=len; --i>=0; ++bp ) *bp = map[*bp];
  }
}

int main(int ac, char **av)
{
  FILE *ifp = !strcmp(av[1],"-") ? stdin : fopen(av[1],"r");
  char line[120];
  fgets(line,sizeof(line),ifp);
  fgets(line,sizeof(line),ifp);
  int w, h;  if( sscanf(line,"%d %d\n",&w,&h) != 2 ) exit(1);
  fgets(line,sizeof(line),ifp);
  int len = w*h;
  unsigned char data[len], *bp = data;
  for( int ch, i=len; --i>=0 && (ch=getc(ifp)) >= 0; ++bp ) *bp = ch;

  int hist[256]; for( int i=0; i<256; ++i ) hist[i] = 0;
  bp = data;     for( int i=len; --i>=0; ++bp ) ++hist[*bp];
  for( int t=0, i=0; i<256; ++i ) { t += hist[i];  hist[i] = t; }
  FILE *ofp = popen("gnuplot -e \"set terminal png; plot \\\"-\\\" with lines\" | xv - &","w");
  for( int i=0; i<256; ++i ) fprintf(ofp,"%d\n",hist[i]);
  fclose(ofp);
  if( ifp != stdin ) fclose(ifp);
  return 0;
}

