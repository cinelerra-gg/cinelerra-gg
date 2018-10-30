#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include "../db/tdb.h"
#include "../db/s.C"

#include "linklist.h"
#include "mediadb.h"
#include "filexml.h"



void write_pgm(uint8_t *tp, int w, int h, const char *fmt, ...)
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

void write_ppm(uint8_t *tp, int w, int h, const char *fmt, ...)
{
  va_list ap;    va_start(ap, fmt);
  char fn[256];  vsnprintf(fn, sizeof(fn), fmt, ap);
  va_end(ap);
  FILE *fp = !strcmp(fn,"-") ? stdout : fopen(fn,"w");
  if( fp ) {
    fprintf(fp,"P6\n%d %d\n255\n",w,h);
    fwrite(tp,3*w,h,fp);
    fclose(fp);
  }
}

static inline int clip(int v, int mn, int mx)
{
  return v<mn ? mn : v>mx ? mx : v;
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


void Scale::
scale(uint8_t *odata, int ow, int oh, int sx, int sy, int sw, int sh)
{
  pw = (double)(ow * oh) / (rw * rh);
  odat = odata + sy*ow + sx;
  iy1 = 0;  yf1 = 0;
  double r = 0.5;

  for( int dy=1; dy<=sh; ++dy, odat+=ow ) {
    iy0 = iy1;  yf0 = 1.0 - yf1;
    double ny = (double)(dy * rh) / oh;
    iy1 = (int) ny;  yf1 = ny-iy1;
    uint8_t *bp = odat;
    ix1 = 0;  xf1 = 0;

    for( int dx=1; dx<=sw; ++dx ) {
      ix0 = ix1;  xf0 = 1.0 - xf1;
      double nx = (double)(dx*rw) / ow;
      ix1 = (int)nx;  xf1 = nx-ix1;
      double px = scalexy();
      int ipx = px + r;
      r += px - ipx;
      *bp++ = ipx;
    }
  }
}


Deletions::
Deletions(int pid, const char *fn)
{
	this->pid = pid;
	strcpy(this->filepath,fn);
}

Deletions::
~Deletions()
{
}


int Deletions::
load(FileXML &xml)
{
        for(;;) {
                if( xml.read_tag() != 0 ) return 1;
                if( !xml.tag.title_is("DEL") ) break;
                double time = xml.tag.get_property("TIME", (double)0.0);
                int action = xml.tag.get_property("ACTION", (int)0);
                append(new Dele(time, action));
        }
        return 0;
}

Deletions *Deletions::
read_dels(const char *filename)
{
        FileXML xml;
        if( xml.read_from_file(filename, 1) ) return 0;
        do {
                if( xml.read_tag() ) return 0;
        } while( !xml.tag.title_is("DELS") );

	int pid = xml.tag.get_property("PID", (int)-1);
	const char *file = xml.tag.get_property("FILE");
	Deletions *dels = new Deletions(pid, file);
	if( dels->load(xml) || !xml.tag.title_is("/DELS") ) {
		 delete dels; dels = 0;
	}
	return dels;
}

int Deletions::
save(FileXML &xml)
{
	xml.tag.set_title("DELS");
	xml.tag.set_property("PID", pid);
	xml.tag.set_property("FILE", filepath);
	xml.append_tag();
	xml.append_newline();

	for( Dele *del=first; del; del=del->next ) {
		xml.tag.set_title("DEL");
		xml.tag.set_property("TIME", del->time);
		xml.tag.set_property("ACTION", del->action);
		xml.append_tag();
		xml.tag.set_title("/DEL");
		xml.append_tag();
		xml.append_newline();
	}

	xml.tag.set_title("/DELS");
	xml.append_tag();
	xml.append_newline();
	return 0;
}

int Deletions::
write_dels(const char *filename)
{
	FILE *fp = fopen(filename, "wb");
	if( !fp != 0 ) return 1;
	FileXML xml;
	save(xml);
	xml.terminate_string();
	xml.write_to_file(fp);
	fclose(fp);
	return 0;
}


Clip::
Clip(int clip_id, double start, double end, int index)
{
	this->clip_id = clip_id;
	this->start = start;
	this->end = end;
	this->index = index;
	this->votes = 0;
	this->groups = 0;
	this->cycle = 0;
	this->muted = 0;
	this->best = 1.e99;
}

Clip::
~Clip()
{
	Clips *clips = (Clips *)list;
	if( clips && clips->current == this ) clips->current = 0;
}

Clip *Clips::locate(int id, double start, double end)
{
	if( current && current->clip_id == id &&
	    end >= current->start && start < current->end )
		return current;
	for( current=first; current!=0; current=current->next ) {
		if( current->clip_id != id ) continue;
		if( current->start <= end && current->end > start ) break;
	}
	return current;
}

Clips::
Clips(int pid)
{
	this->pid = pid;
	this->current = 0;
	this->count = 0;
	this->cycle = 0;
}

Clips::
~Clips()
{
}

Clip *Clips::
new_clip(int clip_id, double start, double end, int index)
{
	return append(new Clip(clip_id, start, end, index));
}

int Clips::
get_clip(int idx, int &id, double &start, double &end)
{
	int votes = -1, cid = -1;
	double st = 0, et = 0;

	for( Clip *clip=first; clip!=0; clip=clip->next ) {
		if( clip->index != idx ) continue;
		// must have prefix+suffix frames
 		if( clip->groups != 3 ) continue;
		if( clip->votes < votes ) continue;
		if( clip->votes > votes ) {
			// first/better match
			votes = clip->votes;
			cid = clip->clip_id;
			st = clip->start;
			et = clip->end;
			continue;
		}
		// reject if not overlapping
		if( clip->start > et ) continue;
		if( clip->end < st ) continue;
		// expand range on overlap
		if( clip->start < st ) st = clip->start;
		if( clip->end > et ) et = clip->end;
	}

	if( votes > 2 ) {
		start = st;  end = et;  id = cid;
		return 1;
	}

	return 0;
}

int Clips::
check(int clip_id, double start, double end, int group, double err)
{
	Clip *clip = locate(clip_id, start, end);
	if( clip ) {
		if( err < clip->best ) {
			clip->best = err;
			if( start < clip->start ) clip->start = start;
			if( end > clip->end ) clip->end = end;
		}
	}
        else
		clip = new_clip(clip_id, start, end, count++);
	clip->groups |= group;
	if( cycle != clip->cycle ) {
		 clip->cycle = cycle;
		++clip->votes;
//printf("clip %d votes %d\n",clip_id,clip->votes);
	}
	return 0;
}

int Clips::
save(FileXML &xml)
{
	xml.tag.set_title("CLIPS");
	xml.tag.set_property("PID", pid);
	xml.append_tag();
	xml.append_newline();

	for( int idx=0; idx<count; ++idx ) {
		int id;  double start, end;
		if( !get_clip(idx, id, start, end) ) continue;
		xml.tag.set_title("CLIP");
		xml.tag.set_property("ID", id);
		xml.tag.set_property("START", start);
		xml.tag.set_property("END", end);
		xml.append_tag();
		xml.tag.set_title("/CLIP");
		xml.append_tag();
		xml.append_newline();
	}

	xml.tag.set_title("/CLIPS");
	xml.append_tag();
	xml.append_newline();
	return 0;
}

int Clips::
load(FileXML &xml)
{
	int i = 0;
	for(;;) {
		if( xml.read_tag() != 0 ) return 1;
		if( !xml.tag.title_is("CLIP") ) break;
		int clip_id = xml.tag.get_property("ID", (int)0);
		double start = xml.tag.get_property("START", (double)0.0);
		double end = xml.tag.get_property("END", (double)0.0);
		append(new Clip(clip_id, start, end, i++));
	}
	count = i;
	return 0;
}


Snip::
Snip(int clip_id, double start, double end, int index)
 : Clip(clip_id, start, end, index)
{
}

Snip::
~Snip()
{
}

Snips::
Snips()
{
}

Snips::
~Snips()
{
}

Clip *Snips::
new_clip(int clip_id, double start, double end, int index)
{
	return append(new Snip(clip_id, start, end, index));
}


MediaDb::
MediaDb()
{
	opened = 0;
	db = new theDb();
}

MediaDb::
~MediaDb()
{
	if( opened ) closeDb();
	delete db;
}

int MediaDb::is_open() { return opened > 0 ? opened : 0; }
int MediaDb::transaction() { return db->transaction(); }

// clip_set accessors
int MediaDb::clip_id(int id) { return db->clip_set.FindId(id); }
int MediaDb::clip_id() { return db->clip_set.id(); }
int MediaDb::clips_first_id() { return db->clip_set.FirstId(clip_id_loc); }
int MediaDb::clips_next_id() { return db->clip_set.NextId(clip_id_loc); }
const char *MediaDb::clip_title() { return db->clip_set.Title(); }
const char *MediaDb::clip_path() { return db->clip_set.Asset_path(); }
double MediaDb::clip_position() { return db->clip_set.Position(); }
double MediaDb::clip_framerate() { return db->clip_set.Framerate(); }
double MediaDb::clip_average_weight() { return db->clip_set.Average_weight(); }
void MediaDb::clip_average_weight(double avg_wt) { return db->clip_set.Average_weight(avg_wt); }
int MediaDb::clip_frames() { return db->clip_set.Frames(); }
double MediaDb::clip_length() {
	return clip_framerate()>0 ? clip_frames() / clip_framerate() : 0;
}
int MediaDb::clip_prefix_size() { return db->clip_set.Prefix_size(); }
int MediaDb::clip_suffix_size() { return db->clip_set.Suffix_size(); }
int MediaDb::clip_size() { return clip_prefix_size()+clip_suffix_size(); }
double *MediaDb::clip_weights() { return (double *)db->clip_set._Weights(); }
int64_t MediaDb::clip_creation_time() { return db->clip_set.Creation_time(); }
int64_t MediaDb::clip_system_time() { return db->clip_set.System_time(); }

// MediaDb::timeline accessors
int MediaDb::timeline_id(int id) { return db->timeline.FindId(id); }
int MediaDb::timeline_id() { return db->timeline.id(); }
int MediaDb::timeline_clip_id() { return db->timeline.Clip_id(); }
int MediaDb::timeline_sequence_no() { return db->timeline.Sequence_no(); }
int MediaDb::timeline_frame_id() { return db->timeline.Frame_id(); }
int MediaDb::timeline_group() { return db->timeline.Group(); }
double MediaDb::timeline_offset() { return db->timeline.Time_offset(); }

// MediaDb::clip_view accessors
int MediaDb::views_id(int id) { return db->clip_views.FindId(id); }
int MediaDb::views_id() { return db->clip_views.id(); }
int MediaDb::views_clip_id(int id) {
	return Clip_viewsLoc::ikey_Clip_access(db->clip_views,id).Find();
}
int MediaDb::views_clip_id() { return db->clip_views.Access_clip_id(); }
int64_t MediaDb::views_access_time() { return db->clip_views.Access_time(); }
int MediaDb::views_access_count() { return db->clip_views.Access_count(); }


double MediaDb::
mean(uint8_t *dat, int n, int stride)
{
  int s = 0;
  for( int i=0; i<n; ++i, dat+=stride ) s += *dat;
  return (double)s / n;
}

double MediaDb::
variance(uint8_t *dat, double m, int n, int stride)
{
  double ss = 0, s = 0;
  for( int i=0; i<n; ++i, dat+=stride ) {
    double dx = *dat-m;
    s += dx;  ss += dx*dx;
  }
  return (ss - s*s / n) / (n-1);
}

double MediaDb::
centroid(uint8_t *dat, int n, int stride)
{
  int s = 0, ss = 0;
  for( int i=0; i<n; dat+=stride ) { s += *dat;  ss += ++i * *dat; }
  return s > 0 ? (double)ss / s : n / 2.;
}

void MediaDb::
centroid(uint8_t *dat, int w, int h, double &xx, double &yy)
{
	double x = 0, y = 0;
	uint8_t *dp = dat;
	for( int i=h; --i>=0; dp+=w ) x += centroid(dp, w);
	for( int i=w; --i>=0; ) y += centroid(dat+i, h, w);
	xx = x / h;  yy = y / w;
}

void MediaDb::
deviation(uint8_t *a, uint8_t *b, int sz, int margin,
		double &dev, int &ofs, int stride)
{
  double best = 1e100;  int ii = -1;
  for( int i=-margin; i<=margin; ++i ) {
    int aofs = i < 0 ? 0 : i*stride;
    int bofs = i < 0 ? -i*stride : 0;
    uint8_t *ap = a + aofs, *bp = b + bofs;
    int ierr = 0; int n = sz - abs(i);
    for( int j=n; --j>=0; ap+=stride,bp+=stride ) ierr += abs(*ap - *bp);
    double err = (double)ierr / n;
    if( err < best ) { best = err;  ii = i; }
  }
  dev = best;
  ofs = ii;
}

int64_t MediaDb::
cmpr(uint8_t *ap, uint8_t *bp, int sz)
{
  int64_t v = 0;
  for( int i=sz; --i>=0; ++ap,++bp ) v += abs(*ap-*bp);
  return v;
}

int64_t MediaDb::
diff(uint8_t *a, uint8_t *b, int w, int h, int dx, int dy, int bias)
{
  int axofs = dx < 0 ? 0 : dx;
  int ayofs = w * (dy < 0 ? 0 : dy);
  int aofs = ayofs + axofs;
  int bxofs = dx < 0 ? -dx : 0;
  int byofs = w * (dy < 0 ? -dy : 0);
  int bofs = byofs + bxofs;
  uint8_t *ap = a + aofs, *bp = b + bofs;
  int ierr = 0, ww = w-abs(dx), hh = h-abs(dy);
  for( int y=hh; --y>=0; ap+=w, bp+=w ) {
    a = ap;  b = bp;
    for( int x=ww; --x>=0; ++a, ++b ) { ierr += abs(*a-bias - *b); }
  }
  return ierr;
}

void MediaDb::
fit(int *dat, int n, double &a, double &b)
{
	double st2 = 0, sb = 0;
	int64_t sy = 0;
	for( int i=0; i<n; ++i ) sy += dat[i];
	double mx = (n-1)/2., my = (double)sy / n;
	for( int i=0; i<n; ++i ) {
		double t = i - mx;
		st2 += t * t;
		sb += t * dat[i];
	}
	b = sb / st2;
	a = my - mx*b;
}

void MediaDb::
eq(uint8_t *sp, uint8_t *dp, int len)
{
#if 0
	int hist[256], wts[256], map[256];
	for( int i=0; i<256; ++i ) hist[i] = 0;
	uint8_t *bp = sp;
	for( int i=len; --i>=0; ++bp ) ++hist[*bp];
	int t = 0;
	for( int i=0; i<256; ++i ) { t += hist[i];  wts[i] = t; }
	int lmn = len/20, lmx = len-lmn;
	int mn =   0;  while( mn < 256 && wts[mn] < lmn ) ++mn;
	int mx = 255;  while( mx > mn && wts[mx] > lmx ) --mx;
	double a, b;
	fit(&wts[mn], mx-mn, a, b);
	double r = 256./len;
	double a1 = (a - b*mn) * r, b1 = b * r;
	for( int i=0 ; i<256;  ++i ) map[i] = clip(a1 + i*b1, 0, 255);
	for( int i=len; --i>=0; ++sp,++dp ) *dp = map[*sp];
#else
	if( sp != dp ) memcpy(dp,sp,len);
#endif
}

// record constructors
int MediaDb::
new_frame(int clip_id, uint8_t *dat, int no, int group, double offset)
{
//printf("add %d at %f\n",no,tm);
	int fid, ret = get_media_key(dat, fid, MEDIA_FRAME_DIST, MEDIA_FRAME_ERRLMT);
	if( ret < 0 ) return -1;
	if( ret > 0 ) {
		db->video_frame.Allocate();
		uint8_t *fp = db->video_frame._Frame_data(SFRM_SZ);
		eq(dat, fp, SFRM_SZ);
		double mn = mean(dat, SFRM_SZ);
		db->video_frame.Frame_mean(mn);
		double sd = std_dev(dat, mn, SFRM_SZ);
		db->video_frame.Frame_std_dev(sd);
		double cx, cy;  centroid(dat, SWIDTH, SHEIGHT, cx, cy);
		db->video_frame.Frame_cx(cx);
		db->video_frame.Frame_cy(cy);
		double moment = cx + cy;
		db->video_frame.Frame_moment(moment);
		db->video_frame.Construct();
		fid = db->video_frame.id();
	}
	db->timeline.Allocate();
	db->timeline.Clip_id(clip_id);
	db->timeline.Sequence_no(no);
	db->timeline.Frame_id(fid);
	db->timeline.Group(group);
	db->timeline.Time_offset(offset);
	db->timeline.Construct();
	return 0;
}

int MediaDb::
new_clip_set(const char *title, const char *asset_path, double position,
	double framerate, int frames, int prefix_size, int suffix_size,
	int64_t creation_time, int64_t system_time)
{
	db->clip_set.Allocate();
	db->clip_set.Title(title);
	db->clip_set.Asset_path(asset_path);
	db->clip_set.Position(position);
	db->clip_set.Framerate(framerate);
	db->clip_set.Frames(frames);
	db->clip_set.Prefix_size(prefix_size);
	db->clip_set.Suffix_size(suffix_size);
	db->clip_set._Weights(frames*sizeof(double));
	db->clip_set.Creation_time(creation_time);
	db->clip_set.System_time(system_time);
	db->clip_set.Construct();

	return new_clip_view(db->clip_set.id(), creation_time);
}

int MediaDb::
new_clip_view(int clip_id, int64_t access_time)
{
	db->clip_views.Allocate();
	db->clip_views.Access_clip_id(clip_id);
	db->clip_views.Access_time(access_time);
	db->clip_views.Access_count(1);
	db->clip_views.Construct();
	return 0;
}

int MediaDb::
access_clip(int clip_id, int64_t access_time)
{
	if( Clip_viewsLoc::ikey_Clip_access(db->clip_views,clip_id).Find() ) return 1;
	db->clip_views.Destruct();
	if( !access_time ) { time_t at;  ::time(&at);  access_time = (int64_t)at; }
	db->clip_views.Access_time(access_time);
	int access_count = db->clip_views.Access_count();
	db->clip_views.Access_count(access_count+1);
	db->clip_views.Construct();
	return 0;
}

int MediaDb::
del_clip_set(int clip_id)
{
	if( db->clip_set.FindId(clip_id) ) return 1;
	// delete clip_set
	db->clip_set.Destruct();
	db->clip_set.Deallocate();
	// delete clip_views
	if( Clip_viewsLoc::ikey_Clip_access(db->clip_views,clip_id).Find() ) return 1;
	db->clip_views.Destruct();
	db->clip_views.Deallocate();
	// delete timeline
	while( !TimelineLoc::ikey_Sequences(db->timeline,clip_id,0).Locate() ) {
		if( clip_id != (int)db->timeline.Clip_id() ) break;
		int frame_id = db->timeline.Frame_id();
		db->timeline.Destruct();
		db->timeline.Deallocate();
		// check for more timeline refs
		if( !TimelineLoc::ikey_Timelines(db->timeline, frame_id).Locate() &&
			frame_id == (int)db->timeline.Frame_id() ) continue;
		if( db->video_frame.FindId(frame_id) ) continue;
		// delete frame
		db->video_frame.Destruct();
		db->video_frame.Deallocate();
	}
	return 0;
}


// db file operations
int MediaDb::
newDb()
{
	if( opened > 0 || db->create(MEDIA_DB) ) {
		printf("MediaDb::newDb failed\n");
		return opened = -1;
	}
	return opened = 0;
}

int MediaDb::
openDb(int rw)
{
	if( opened > 0 ) return 0;
	if( db->access(MEDIA_DB, MEDIA_SHM_KEY, rw) ) {
		printf("MediaDb::openDb failed\n");
		return opened = -1;
	}
	opened = 1;
	return 0;
}

int MediaDb::
resetDb(int rw)
{
	int result = 0;
	if( access(MEDIA_DB, F_OK) ) result = newDb();
	if( !result ) result = openDb(rw);
	return result;
}

void MediaDb::
closeDb()
{
	if( opened > 0 ) { detachDb();  db->close(); }
	opened = 0;
}

void MediaDb::
commitDb()
{
	if( opened > 0 ) db->commit();
}

void MediaDb::
undoDb()
{
	if( opened > 0 ) db->undo();
}

int MediaDb::
attachDb(int rw)
{
	return opened > 0 ? db->attach(rw) : -1;
}

int MediaDb::
detachDb()
{
	return opened > 0 ? db->detach() : -1;
}


int MediaDb::
get_timelines(int frame_id)
{
	return TimelineLoc::ikey_Timelines(db->timeline, frame_id).Locate();
}

int MediaDb::
next_timelines()
{
	return TimelineLoc::rkey_Timelines(db->timeline).Next();
}

int MediaDb::
get_sequences(int clip_id, int seq_no)
{
	int ret = TimelineLoc::ikey_Sequences(db->timeline,clip_id,seq_no).Locate();
	if( !ret && clip_id != (int)db->timeline.Clip_id() ) ret = 1;
	return ret;
}

int MediaDb::
next_sequences()
{
	return TimelineLoc::rkey_Sequences(db->timeline).Next();
}

// frame accessors
int MediaDb::
get_image(int id, uint8_t *dat, int &w, int &h)
{
	int result = 1;
	if( !db->video_frame.FindId(id) ) {
		memcpy(dat, db->video_frame.Frame_data(), SFRM_SZ);
		w = SWIDTH;  h = SHEIGHT;
		result = 0;
	}
	return result;
}

int MediaDb::
get_frame_key(uint8_t *dat, int &fid,
		Clips *clips, double pos, int mask)
{
	return get_media_key(dat, fid,
			MEDIA_SEARCH_DIST, MEDIA_SEARCH_ERRLMT,
			clips, pos, mask);
}

int MediaDb::
get_media_key(uint8_t *dat, int &fid, int dist, double errlmt,
		Clips *clips, double pos, int mask)
{
	fmean = mean(dat, SFRM_SZ);
	if( fmean < 17 ) return -1; // black, forbidden
	if( fmean > 239 ) return -1; // white, forbidden
	distance = dist;
	pixlmt = SFRM_SZ*errlmt;
	position = pos;
	group_mask = mask;
	fsd = std_dev(dat, fmean, SFRM_SZ);
	centroid(dat, SWIDTH, SHEIGHT, fcx, fcy);
	fmoment = fcx + fcy;
	eq(dat, frm, SFRM_SZ);
	if( clips ) ++clips->cycle;
	lid = nid = -1;
	lerr = LONG_MAX;
	int result = 1;

	if( !Video_frameLoc::ikey_Frame_center(db->video_frame, fmoment).Locate() ) {
		Video_frameLoc prev(db->video_frame), next(db->video_frame);
		Video_frameLoc::rkey_Frame_center prev_ctr(prev), next_ctr(next);
		if( !get_key(clips, prev_ctr, next_ctr) ) result = 0;
	}
	if( !Video_frameLoc::ikey_Frame_weight(db->video_frame, fmean).Locate() ) {
		Video_frameLoc prev(db->video_frame), next(db->video_frame);
		Video_frameLoc::rkey_Frame_weight prev_wt(prev), next_wt(next);
		if( !get_key(clips, prev_wt, next_wt) ) result = 0;
	}

	fid = lid;
	return result;
}

int MediaDb::
get_key(Clips *clips, Db::rKey &prev_rkey, Db::rKey &next_rkey)
{
	key_compare(clips, next_rkey.loc);
	// search outward from search target for frame with least error
	Db::pgRef next_loc;  next_rkey.NextLoc(next_loc);

	for( int i=1; i<=distance; ++i ) {
		if( !prev_rkey.Locate(Db::keyLT) )
			key_compare(clips,prev_rkey.loc);
		if( !next_rkey.Next(next_loc) )
			key_compare(clips,next_rkey.loc);
	}

	return lerr < pixlmt ? 0 : 1;
}

int64_t MediaDb::
key_compare(Clips *clips, Db::ObjectLoc &loc)
{
	int id = loc.id();
	if( nid == id ) return 1;
	nid = id;
	Video_frameLoc &frame = *(Video_frameLoc*)&loc;
	double dm = frame.Frame_mean()-fmean;
	if( fabs(dm) > MEDIA_MEAN_ERRLMT ) return 1;
	double ds = frame.Frame_std_dev()-fsd;
	if( fabs(ds) > MEDIA_STDDEV_ERRLMT ) return 1;
	double dx = frame.Frame_cx()-fcx;
	if( fabs(dx) > MEDIA_XCENTER_ERRLMT ) return 1;
	double dy = frame.Frame_cy()-fcy;
	if( fabs(dy) > MEDIA_YCENTER_ERRLMT ) return 1;
	uint8_t *dat = frame._Frame_data();
	//int64_t err = cmpr(frm, dat, SFRM_SZ);
	int64_t err = diff(frm, dat, SWIDTH, SHEIGHT, lround(dx), lround(dy), lround(dm));
	if( lerr > err ) { lerr = err;  lid = id; }
	if( clips && err < pixlmt ) add_timelines(clips, id, err);
	return 0;
}

int MediaDb::
add_timelines(Clips *clips, int fid, double err)
{
	// found frame, find timelines
	if( get_timelines(fid) ) {
		printf(_(" find timeline frame_id(%d) failed\n"), fid);
		return 1;
	}

	// process Timelines which contain Frame_id
	int cid = -1;
	for( int ret=0; !ret && timeline_frame_id()==fid; ret=next_timelines() ) {
		int tid = timeline_clip_id();
		if( tid != cid && clip_id(cid=tid) ) break;
		double start_time = position - timeline_offset();
		if( start_time < 0 ) continue;
		int group = timeline_group();
		if( (group & group_mask) == 0 ) continue;
		double length = clip_length();
		double end_time = start_time + length;
//printf("clip=%d, pos=%f, time=%f-%f, %s\n",
//  cid, position, start_time, end_time, clip_title());
		clips->check(cid, start_time, end_time, group, err);
	}

	return 0;
}

