#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "libzmpeg3.h"

const char *prog = 0;
int vtrk = 0;
int verbose = 0;
int wdw_mask = -1;
int cc_service = 1;
int done = 0;
long fpos = 0;
char dfrm[1000];
int dlen = 0;
long dsofs = 0, deofs = 0;
char *xfn = 0, *ofn = 0;
FILE *xfp = 0, *ofp = 0;

class Edit {
  long start, length;
  long offset;
public:
  static int cmpr(const void *ap, const void *bp);
  Edit(long st, long len, long ofs) : start(st), length(len), offset(ofs) {}
  long sfrm() { return start; }
  long efrm() { return start+length-1; }
  long ofrm(long frm) { return frm-start+offset; }
  ~Edit() {}
};

int Edit::cmpr(const void *ap, const void *bp)
{
  Edit *a = *(Edit **)ap;
  Edit *b = *(Edit **)bp;
  int n = a->sfrm() - b->sfrm();
  if( !n ) n = a->length - b->length;
  return n;
}

class Edits {
  int count, allocated;
  Edit **list;
  long pos;
public:
  Edits() : count(0), allocated(0), list(0), pos(0) {}
  ~Edits() { for( int i=0; i<count; ++i ) delete list[i]; }
  void append(long st, long len);
  bool finished(long frm) { return count>0 && frm>=list[count-1]->efrm(); }
  Edit *active(long frm);
  void sort() { qsort(list, count, sizeof(Edit*), Edit::cmpr); }
  int len() { return count; }
} edits;

void Edits::append(long st, long len)
{
  if( count >= allocated ) {
    int sz = 2*allocated + 16;
    Edit **new_list = new Edit*[sz];
    for( int i=0; i<count; ++i ) new_list[i] = list[i];
    delete list;  list = new_list;  allocated = sz;
  }
  list[count++] = new Edit(st, len, pos);
  pos += len;
}

Edit *Edits::active(long frm)
{
  Edit *mp = 0;
  int l=-1, h=count;
  while( h-l > 1 ) {
    int m = (l+h) >> 1;
    mp = list[m];
    if( frm < mp->sfrm() ) l = m;
    else if( frm > mp->sfrm() ) h = m;
    else break;
  }
  if( !mp ) return 0;
  if( frm < mp->sfrm() ) return 0;
  if( frm > mp->efrm() ) return 0;
  return mp;
}


void usage()
{
  fprintf(stderr,"usage: %s ",prog);
  fprintf(stderr," [-c cc_service ] ");
  fprintf(stderr," [-s start:length, ...] ");
  fprintf(stderr," [ -t track ] ");
  fprintf(stderr," [-v verbose ]\n");
  fprintf(stderr,"            ");
  fprintf(stderr," [-w wdw mask ] ");
  fprintf(stderr," [-x file.xml ] ");
  fprintf(stderr," [-o file.udvd ] ");
  fprintf(stderr," file.ts\n");
  fprintf(stderr,"  cc_service   = closed caption service id\n");
  fprintf(stderr,"  start:length = start:length frames (comma seperated list)\n");
  fprintf(stderr,"  track        = video track number\n");
  fprintf(stderr,"  verbose      = verbose level\n");
  fprintf(stderr,"  wdw mask     = bit mask for windows (-1 = all)\n");
  fprintf(stderr,"  file.xml     = filename for edl xml format subtitle data\n");
  fprintf(stderr,"  file.udvd    = filename for udvd format subtitle data\n");
  fprintf(stderr,"  file.ts      = filename for transport stream\n");
  exit(1);
}

void edl_header()
{
  fprintf(xfp,"<?xml version=\"1.0\"?>\n");
  fprintf(xfp,"<EDL VERSION=4.4 PATH=%s>\n", xfn);
  fprintf(xfp," <TRACK RECORD=1 NUDGE=0 PLAY=1 GANG=1 DRAW=1 EXPAND=0");
  fprintf(xfp," TRACK_W=0 TRACK_H=0 TYPE=SUBTTL>\n");
  fprintf(xfp,"<TITLE>Subttl 1</TITLE>\n");
  fprintf(xfp,"<EDITS>\n");
}

void edit_data(const char *sp, int n, int len)
{
  const char *cp, *ep = sp + n;
  fprintf(xfp,"<EDIT STARTSOURCE=0 CHANNEL=0 LENGTH=%d TEXT=\"", len);
  while( sp < ep ) {
    int ch = *sp++;
    switch( ch ) {
    case '<':  cp = "&lt;";    break;
    case '>':  cp = "&gt;";    break;
    case '&':  cp = "&amp;";   break;
    case '"':  cp = "&quot;";  break;
    default:  fputc(ch, xfp);  continue;
    }
    while( *cp != 0 ) fputc(*cp++, xfp);
  }
  fprintf(xfp,"\"></EDIT>\n");
}


void edit_write()
{
  if( !xfp ) return;
  long df;
  if( (df=dsofs-fpos) > 0 ) edit_data("", 0, df);
  if( (df=deofs-dsofs) > 0 ) edit_data(dfrm, dlen, df);
  fpos = deofs;
}

void edl_trailer()
{
  if( dlen > 0 ) edit_write();
  fprintf(xfp,"</EDITS>\n");
  fprintf(xfp,"</TRACK>\n");
  fprintf(xfp,"</EDL>\n");
}

int text_cb(int sid, int id, int sfrm, int efrm, const char *txt)
{
  long sofs = sfrm, eofs = efrm;
  if( ((1<<id) & wdw_mask) == 0 ) return 0;
  if( edits.finished(sfrm) ) { done = 1; return 0; }
  // check for edit range specs
  if( edits.len() > 0 ) {
    Edit *edit = edits.active(sfrm);
    if( !edit ) return 0;
    long frm;
    if( sfrm < (frm=edit->sfrm()) ) sfrm = frm;
    if( efrm > (frm=edit->efrm()) ) efrm = frm;
    sofs = edit->ofrm(sfrm);
    eofs = edit->ofrm(efrm);
  }
  // ltrim, rtrim data
  const char *bp, *cp;
  for( bp=txt; *bp; ++bp )
    if( !isblank(*bp) && *bp != '|' ) break;
  if( !*bp ) return 0;
  const char *ep = bp;
  for( cp=bp; *cp; ++cp )
    if( !isblank(*cp) && *cp != '|' ) ep = cp;
  int n = ep - bp + 1;
  // logging
  if( verbose ) {
    if( verbose > 2 ) fprintf(stderr,"\r");
    fprintf(stderr,"%d",sfrm);
    if( verbose > 1 ) fprintf(stderr,": %*.*s", n, n, bp);
    fprintf(stderr,"\n");
  }
  // output txt directly
  if( ofp && sofs < eofs ) {
    fprintf(ofp, "{%ld}{%ld}%*.*s\n", sofs, eofs-1, n, n, bp);
  }
  // last dfrm expired, output edit_data + reset dfrm
  if( sofs >= deofs ) {
    if( dlen > 0 ) edit_write();
    dlen = 0;  dsofs = sofs;
  }
  // append overlapping timeline data
  if( dlen > 0 && dlen < (int)sizeof(dfrm) ) dfrm[dlen++] = '|';
  while( bp <= ep && dlen < (int)sizeof(dfrm) ) dfrm[dlen++] = *bp++;
  if( eofs > deofs ) deofs = eofs;
  return 0;
}

int main(int ac, char *av[])
{
  char *cp = 0, *fn = 0;
  prog = *av++;

  while( --ac > 0 ) {
    fn = *av++;
    if( *fn == '-' ) {
      switch( fn[1] ) {
      case 'c':
        if( --ac <= 0 ) usage();
        cc_service = atoi(*av++);
        break;
      case 's':
        if( --ac <= 0 ) usage();
        for( cp=*av++; *cp; ++cp ) {
          int st = strtol(cp,&cp,0);
          int len = *cp == ':' ? strtol(cp+1,&cp,0) : INT_MAX;
          edits.append(st,len);
          if( *cp != ',' ) break;
        }
        if( *cp ) { fprintf(stderr,"edit list error at: %s\n",cp); usage(); }
        edits.sort();
        break;
      case 't':
        if( --ac <= 0 ) usage();
        vtrk = atoi(*av++);
        break;
      case 'v':
        if( --ac <= 0 ) usage();
        verbose = atoi(*av++);
        break;
      case 'o':
        if( --ac <= 0 ) usage();
        ofn = *av++;
        break;
      case 'x':
        if( --ac <= 0 ) usage();
        xfn = *av++;
        break;
      case 'w':
        if( --ac <= 0 ) usage();
        wdw_mask = strtoul(*av++,0,0);
        if( !wdw_mask ) {
          fprintf(stderr,"no windows in bitmask\n");
          exit(1);
        }
        break;
      default:
        usage();
      }
      continue;
    }
    if( ac > 1 ) usage();
    if( !access(fn,R_OK) ) break;
    fprintf(stderr, "no file %s\n", fn);
    exit(0);
  }

  if( !fn ) usage();
  setbuf(stderr, 0);

  if( !xfn && !ofn ) {
    fprintf(stderr,"no output file (-x,-o) in command line arguments\n");
    usage();
  }
  if( xfn && ofn && !strcmp(xfn,ofn) ) {
    fprintf(stderr,"output files (-x,-o) overlap command line arguments\n");
    usage();
  }
  if( ofn ) {
    if( !strcmp(ofn,"-") ) ofp = stdout;
    else if( !(ofp=fopen(ofn,"w")) ) {
      perror(ofn);  usage();
    }
  }
  if( xfn ) {
    if( !strcmp(xfn,"-") ) xfp = stdout;
    else if( !(xfp=fopen(xfn,"w")) ) {
      perror(xfn);  usage();
    }
  }

  zmpeg3_t *in;
  int error = 0;
  if( !(in = mpeg3_open(fn, &error)) ) {
    fprintf(stderr, "unable to open %s\n", fn);
    exit(1);
  }
  if( !in->is_transport_stream() ) {
    fprintf(stderr, "not transport stream %s\n", fn);
    exit(1);
  }
  if( !mpeg3_has_video(in) ) {
    fprintf(stderr, "no video %s\n", fn);
    exit(1);
  }

  mpeg3_set_cpus(in, 8);
  int ctrk = cc_service-1;
  if( ctrk < 0 ) ctrk = 63;
  mpeg3_show_subtitle(in, vtrk, ctrk);
  if( mpeg3_set_cc_text_callback(in, vtrk, text_cb) ) {
    fprintf(stderr, "no video track %d on %s\n", vtrk, fn);
    exit(1);
  }
  if( xfp ) edl_header();

  char *yp, *up, *vp;
  while( !done && !mpeg3_read_yuvframe_ptr(in, &yp, &up, &vp, vtrk) )
    if( verbose > 2 ) fprintf(stderr," \r%ld",  mpeg3_get_frame(in, vtrk));

  if( xfp ) edl_trailer();
  mpeg3_close(in);
  exit(0);
}

