#include<stdio.h>
#include<stdarg.h>
#include<time.h>
#include<math.h>
#include "tdb.h"
#include "s.C"

#include "../cinelerra/mediadb.inc"

char *diff_path = 0;
int curr_id = -1;
int next_id = -1;

static inline int clip(int v, int mn, int mx)
{
  return v<mn ? mn : v>mx ? mx : v;
}

void write_pbm(uint8_t *tp, int w, int h, const char *fmt, ...)
{
  va_list ap;    va_start(ap, fmt);
  char fn[256];  vsnprintf(fn, sizeof(fn), fmt, ap);
  va_end(ap);
  FILE *fp = !strcmp(fn,"-") ? stdout : fopen(fn,"w");
  if( fp ) {
    fprintf(fp,"P5\n%d %d\n255\n",w,h);
    fwrite(tp,w,h,fp);
    fclose(fp);
  }
}

void pbm_diff(uint8_t *ap, uint8_t *bp, int w, int h, const char *fmt, ...)
{
  va_list va;    va_start(va, fmt);
  char fn[256];  vsnprintf(fn, sizeof(fn), fmt, va);
  va_end(va);
  FILE *fp = !strcmp(fn,"-") ? stdout : fopen(fn,"w");
  if( fp ) {
    int sz = w*h;
    uint8_t dat[sz], *dp = dat;
    for( int i=sz; --i>=0; ++dp,++ap,++bp ) *dp = clip(*ap-*bp+128, 0 ,255);
    fprintf(fp,"P5\n%d %d\n255\n",w,h);
    fwrite(dat,w,h,fp);
    fclose(fp);
  }
}

int verify_clips(uint8_t *ap,int asz, double afr,
		 uint8_t *bp, int bsz, double bfr,
                 double &ofs, double &error)
{
  double atm = asz/afr, btm = bsz/bfr, lmt = fmin(atm, btm) / 2;
  if( lmt > 8 ) lmt = 8;
  if( fabs(atm - btm) > lmt ) return 0;
  double arate, brate, fr; int margin;
  if( afr < bfr ) {
    arate = 1;  brate = bfr / afr;
    fr = afr;   margin = afr + 0.5;
  }
  else {
    arate = afr / bfr;  brate = 1;
    fr = bfr;   margin = bfr + 0.5;
  }
  double *awt = (double *)ap, *bwt = (double *)bp;
  double best = 1e100;  int ii = -1;
  for( int i=-margin; i<=margin; ++i ) {
    int aofs = i < 0 ? 0 : i*arate + 0.5;
    int bofs = i < 0 ? -i*brate + 0.5 : 0;
    double err = 0;  int n = 0;
    for( ;; ++n ) {
      int ai = aofs + n*arate + 0.5;
      if( ai >= asz ) break;
      int bi = bofs + n*brate + 0.5;
      if( bi >= bsz ) break;
      err += fabs(awt[ai] - bwt[bi]);
    }
    err /= n;
    if( err < best ) { best = err;  ii = i; }
  }
  ofs = ii / fr;  error = best;
  if( best >= MEDIA_WEIGHT_ERRLMT ) return 0;
  return 1;
}

int *load_frames(theDb &db, int clip_id, int no, int len)
{
  int *fp = new int[len], first = no;
  for( int i=len; --i>=0; ) fp[i] = -1;
  int ret = TimelineLoc::ikey_Sequences(db.timeline,clip_id,no).Locate();
  if( !ret ) {
    for( int i=0; i<len; ++i,++no ) {
      if( (int)db.timeline.Clip_id() != clip_id ) break;
      int next = db.timeline.Sequence_no();
      if( next != no ) {
//        printf("seqno skipped in clip %d from %d to %d\n",clip_id,no,next);
        no = next;
      }
      int k = no-first;
      if( k >= 0 && k < len ) fp[k] = db.timeline.Frame_id();
      if( TimelineLoc::rkey_Sequences(db.timeline).Next() ) break;
    }
  }
  else
    printf("cant access timeline of clip %d\n",clip_id);
  return fp;
}

int64_t media_weight(uint8_t *dat, int sz)
{
  int64_t wt = 0;
  for( int i=sz; --i>=0; ++dat ) wt += *dat;
  return wt;
}

int64_t media_cmpr(uint8_t *ap, uint8_t *bp, int sz)
{
  int64_t v = 0;
  for( int i=sz; --i>=0; ++ap,++bp ) v += fabs(*ap-*bp);
  return v;
}

int fix_err(theDb &db, double &err, double ofs,
            int aid, int *afp, int asz, double afr,
            int bid, int *bfp, int bsz, double bfr)
{
  double arate, brate;
  if( afr < bfr ) {
    arate = 1;  brate = bfr / afr;
  }
  else {
    arate = afr / bfr;  brate = 1;
  }
  int aofs = ofs < 0 ? 0 : ofs*afr + 0.5;
  int bofs = ofs < 0 ? -ofs*bfr + 0.5 : 0;
  int n = 0, fno = 0;  int votes = 0;
  for( ;; ++fno ) {
    int ai = aofs + fno*arate + 0.5;
    if( ai >= asz ) break;
    int bi = bofs + fno*brate + 0.5;
    if( bi >= bsz ) break;
    Video_frameLoc aframe(db.video_frame);
    Video_frameLoc bframe(db.video_frame);
    if( afp[ai] < 0 || bfp[bi] < 0 ) {
//      printf("missing prefix frame %d %d/%d in clip %d/%d\n",
//        n, afp[ai], bfp[bi], aid, bid);
      continue;
    }
    if( aframe.FindId(afp[ai]) ) {
      printf("cant access prefix frame %d (id %d) in clip %d\n",
        ai, afp[ai], aid);  continue;
    }
    if( bframe.FindId(bfp[bi]) ) {
      printf("cant access prefix frame %d (id %d) in clip %d\n",
        bi, bfp[bi], bid);  continue;
    }
    int w=80, h=45, sfrm_sz = w*h;
    int64_t v = media_cmpr(aframe._Frame_data(), bframe._Frame_data(), sfrm_sz);
    if( v/3600 < MEDIA_SEARCH_ERRLMT ) ++votes;
//printf("%d %ld(%d,%d)\n", n, v, afp[ai], bfp[bi]);
    if( diff_path ) {
      uint8_t *adat = aframe._Frame_data(), *bdat = bframe._Frame_data();
      write_pbm(adat,w,h,"%s/a%05d.pbm",diff_path,afp[ai]);
      write_pbm(bdat,w,h,"%s/b%05d.pbm",diff_path,bfp[bi]);
      pbm_diff(adat,bdat,80,45,"%s/da%05d-b%05d.pbm",diff_path,afp[ai],bfp[bi]);
    }
    err += v;  ++n;
  }
  printf("v%d",votes);
  return n;
}

int verify_frames(theDb &db, int aid, int bid, double ofs)
{
  int *apfx=0, *asfx=0, *bpfx=0, *bsfx=0;
  int apsz=0, assz=0, bpsz=0, bssz=0, alen=0, blen=0, afrm=0, bfrm=0;
  double afr = 1, bfr = 1;
  if( !db.clip_set.FindId(aid) ) {
    apsz = db.clip_set.Prefix_size();
    assz = db.clip_set.Suffix_size();
    alen = db.clip_set.Frames();
    apfx = load_frames(db, aid, 0, apsz);
    asfx = load_frames(db, aid, alen-assz, assz);
    afr = db.clip_set.Framerate();
    afrm = db.clip_set.Frames();
  }
  else {
    printf("cant open -a- clip %d\n",aid);
    return 0;
  }
  if( !db.clip_set.FindId(bid) ) {
    bpsz = db.clip_set.Prefix_size();
    bssz = db.clip_set.Suffix_size();
    blen = db.clip_set.Frames();
    bpfx = load_frames(db, bid, 0, bpsz);
    bsfx = load_frames(db, bid, blen-bssz, bssz);
    bfr = db.clip_set.Framerate();
    bfrm = db.clip_set.Frames();
  }
  else {
    printf("cant open -b- clip %d\n",bid);
    return 0;
  }
//printf("afrm %d, bfrm %d ",afrm, bfrm);
  double perr = 0;
  int pn = fix_err(db, perr, ofs, aid, apfx, apsz, afr, bid, bpfx, bpsz, bfr);
  double serr = 0;
  printf("/");
  ofs -= afrm/afr-bfrm/bfr;
  int sn = fix_err(db, serr, ofs, aid, asfx, assz, afr, bid, bsfx, bssz, bfr);
  printf(" err %0.3f/%0.3f #%d+%d=%d\n", perr/(pn*80*45),serr/(sn*80*45), pn,sn,pn+sn);

  delete [] bsfx;
  delete [] bpfx;
  delete [] asfx;
  delete [] apfx;
  return 0;
}

double avg_wt(uint8_t *wp, int sz)
{
  double *wt = (double *)wp;
  double v = 0;
  if( sz > 0 ) {
    for( int i=sz; --i>=0; ++wt ) v += *wt;
     v /= sz;
  }
  return v;
}

int main(int ac, char **av)
{
  int ret;  setbuf(stdout,0);
  theDb db;
  db.open(av[1]);
  //db.access(av[1], 34543, 0);
  if( !db.opened() || db.error() ) exit(1);
  if( ac > 2 ) curr_id = atoi(av[2]);
  if( ac > 3 ) next_id = atoi(av[3]);
  if( ac > 4 ) diff_path = av[4];
  if( curr_id >= 0 && next_id >= 0 ) {
    Clip_setLoc curr(db.clip_set);
    if( (ret=curr.FindId(curr_id)) ) {
      printf("cant access clip %d\n", curr_id);
      exit(1);
    }
    Clip_setLoc next(db.clip_set);
    if( (ret=next.FindId(next_id)) ) {
      printf("cant access clip %d\n", next_id);
      exit(1);
    }
    double awt = curr.Average_weight();
    double bwt = next.Average_weight();
    double error=-1, ofs=-1;
    int bias = awt - bwt;
    verify_clips(curr._Weights(),curr.Frames(),curr.Framerate(),
      next._Weights(), next.Frames(), next.Framerate(), ofs, error);
    printf("dupl a=%4d(%0.2f)-%0.2f b=%4d(%0.2f)-%0.2f %s%d err=%0.3f ofs=%0.5f ",
      curr.id(), curr.Framerate(),curr.Frames()/curr.Framerate(),
      next.id(), next.Framerate(), next.Frames()/next.Framerate(),
      bias >= 0 ? "+" : "", bias, error, ofs);
    verify_frames(db, curr.id(), next.id(), ofs);
    exit(0);
  }
  Clip_setLoc curr(db.clip_set);
  Db::pgRef curr_loc;
  if( !(ret=curr.FirstId(curr_loc)) ) {
    do {
      Clip_setLoc next(curr);
      Db::pgRef next_loc = curr_loc;
      double awt = curr.Average_weight();
      while( !next.NextId(next_loc) ) {
        double error=-1, ofs =-1;
        double bwt = next.Average_weight();
        int bias = awt - bwt;
        if( fabs(bias) > 10 ) continue;
        if( verify_clips(curr._Weights(),curr.Frames(),curr.Framerate(),
              next._Weights(), next.Frames(), next.Framerate(), ofs, error) ) {
          printf("dupl a=%4d(%0.2f)-%0.2f b=%4d(%0.2f)-%0.2f %s%d err=%0.3f ofs=%0.3f ",
            curr.id(), curr.Framerate(),curr.Frames()/curr.Framerate(),
            next.id(), next.Framerate(), next.Frames()/next.Framerate(),
            bias >= 0 ? "+" : "", bias, error, ofs);
          verify_frames(db, curr.id(), next.id(), ofs);
          break;
        }
      }
    } while( !(ret=curr.NextId(curr_loc)) );
  }
  db.close();
  return 0;
}

