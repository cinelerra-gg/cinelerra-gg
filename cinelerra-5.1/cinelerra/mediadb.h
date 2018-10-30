#ifndef _MEDIA_DB_H_
#define _MEDIA_DB_H_

#include "arraylist.h"
#include "mediadb.inc"
#include "filexml.inc"
#include "bcwindowbase.inc"
#include "file.inc"
#include "linklist.h"
#include "../db/tdb.h"

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>


void write_pbm(uint8_t *tp, int w, int h, const char *fmt, ...);
void write_pgm(uint8_t *tp, int w, int h, const char *fmt, ...);
void pbm_diff(uint8_t *ap, uint8_t *bp, int w, int h, const char *fmt, ...);


class Scale {
  int rx, ry, rw, rh, iw, ih;
  int ix0, ix1, iy0, iy1, rsh;
  double xf0, xf1, yf0, yf1, pw;
  uint8_t *idat, *odat;
  uint8_t *row(int y) { return idat + (iw<<rsh) * (y>>rsh); }

  inline double scalex(uint8_t *rp, double v, double yw) {
    int x = ix0;  rp += x;
    double vv = xf0 * *rp++;
    int iv = 0;  while( ++x < ix1 ) iv += *rp++;
    vv += iv + xf1 * *rp;
    return v += vv * yw;
  }

  inline double scalexy() {
    double yw, v = 0;
    int y = iy0;
    if( (yw = pw * yf0) > 0 ) v = scalex(row(y), v, yw);
    while( ++y < iy1 ) v = scalex(row(y), v, pw);
    if( (yw = pw * yf1) > 0 ) v = scalex(row(y), v, yw);
    return v;
  }
public:
  Scale(uint8_t *idata, int type, int iw, int ih, int rx, int ry, int rw, int rh) {
    rsh = type ? 1 : 0;
    this->idat = idata + ry*iw + rx;
    this->iw = iw;  this->ih = ih;
    this->rx = rx;  this->ry = ry;
    this->rw = rw;  this->rh = rh;
  }
  void scale(uint8_t *odata, int ow, int oh, int sx, int sy, int sw, int sh);
};


enum {
	 DEL_START,	// always the first mark
	 DEL_MARK,	// boundary between cur/next media deletion
	 DEL_SKIP,	// skip media in current deletion region
	 DEL_OOPS,	// ignore previous mark
};

// definition:
//   dele: n. A sign indicating that something is to be removed from <media>

class Dele : public ListItem<Dele>
{
public:
        double time;
	int action;

	Dele(double t, int a) : time(t), action(a) {}
	~Dele() {}
};

class Deletions: public List<Dele>
{
	int save(FileXML &xml);
	int load(FileXML &xml);
public:
	int pid;
	char filepath[BCTEXTLEN];
	const char *file_path() { return filepath; }
	bool empty() { return !first; }

	int write_dels(const char *filename);
	static Deletions *read_dels(const char *filename);

	Deletions(int pid, const char *fn);
        ~Deletions();
};


class Clip : public ListItem<Clip> {
public:
	int clip_id;
	int votes, index, groups, cycle, muted;
	double start, end, best;

	Clip(int clip_id, double start, double end, int index);
	~Clip();
};

class Clips : public List<Clip> {
public:
	int pid, count, cycle;
	Clip *current;
	Clip *locate(int id, double start, double end);
	int get_clip(int idx, int &id, double &start, double &end);
	virtual Clip *new_clip(int clip_id, double start, double end, int index);
	int check(int clip_id, double start, double end, int group, double err);
	int save(FileXML &xml);
	int load(FileXML &xml);
	Clips(int pid=0);
	~Clips();
};

class Snip : public Clip {
public:
	ArrayList<double> weights;
	ArrayList<double> positions;

	Snip(int clip_id, double start, double end, int index);
	~Snip();
};

class Snips : public Clips {
public:
	Clip *new_clip(int clip_id, double start, double end, int index);
	Snips();
	~Snips();
};


class MediaDb
{
	theDb *db;
	int opened;

	uint8_t frm[SFRM_SZ];
	double position, error_limit;
	int lid, nid, distance, group_mask;
	double fmean, fsd, fcx, fcy, fmoment;
	int64_t lerr, pixlmt;
	Db::pgRef clip_id_loc;

	int get_key(Clips *clips, Db::rKey &prev_rkey, Db::rKey &next_rkey);
	int64_t key_compare(Clips *clips, Db::ObjectLoc &loc);
	static double mean(uint8_t *dat, int n, int stride=1);
	static double variance(uint8_t *dat, double mn, int n, int stride=1);
	static double std_dev(uint8_t *dat, double mn, int n, int stride=1) {
		return sqrt(variance(dat, mn, n, stride));
	}
	static double centroid(uint8_t *dat, int n, int stride=1);
	static void centroid(uint8_t *dat, int w, int h, double &xx, double &yy);
	static void deviation(uint8_t *a, uint8_t *b, int sz, int margin,
		double &dev, int &ofs, int stride=1);
	static int64_t cmpr(uint8_t *ap, uint8_t *bp, int sz);
	static int64_t diff(uint8_t *a, uint8_t *b, int w, int h, int dx, int dy, int bias);
	static void fit(int *dat, int n, double &a, double &b);
	static void eq(uint8_t *sp, uint8_t *dp, int len);
	static void eq(uint8_t *p, int len) { eq(p, p, len); }
public:
	int newDb();
	int openDb(int rw=0);
	int resetDb(int rw=0);
	void closeDb();
	void commitDb();
	void undoDb();
	int attachDb(int rw=0);
	int detachDb();
	int is_open();
	int transaction();

	static void hist_eq(uint8_t *dp, int len);
	static void scale(uint8_t *idata,int iw,int ih,int rx,int ry,int rw,int rh,
			  uint8_t *odata,int ow,int oh,int sx,int sy,int sw,int sh);
	int new_clip_set(const char *title, const char *asset_path, double position,
		double framerate, int frames, int prefix_size, int suffix_size,
		int64_t creation_time, int64_t system_time);
	int new_clip_view(int clip_id, int64_t access_time);
	int access_clip(int clip_id, int64_t access_time=0);
	int del_clip_set(int clip_id);
	int new_frame(int clip_id, uint8_t *dat, int no, int group, double offset);
	int get_timeline_by_frame_id(int fid);
	int get_timelines(int frame_id);
	int next_timelines();
	int get_sequences(int clip_id, int seq_no=0);
	int next_sequences();
	int get_image(int id, uint8_t *dat, int &w, int &h);
	int get_frame_key(uint8_t *dat, int &fid,
		Clips *clips=0, double pos=0, int mask=~0);
	int get_media_key(uint8_t *key, int &fid, int dist, double errlmt,
		Clips *clips=0, double pos=0, int mask=~0);
	int add_timelines(Clips *clips, int fid, double err);
	// clip_set accessors
	int clip_id(int id);
	int clip_id();
	int clips_first_id();
	int clips_next_id();
	const char *clip_title();
	const char *clip_path();
	double clip_position();
	double clip_framerate();
	double clip_average_weight();
	void clip_average_weight(double avg_wt);
	double clip_length();
	int clip_frames();
	int clip_prefix_size();
	int clip_suffix_size();
	int clip_size();
	double *clip_weights();
	int64_t clip_creation_time();
	int64_t clip_system_time();

	// timeline accessors
	int timeline_id(int id);
	int timeline_id();
	int timeline_clip_id();
	int timeline_sequence_no();
	int timeline_frame_id();
	int timeline_group();
	double timeline_offset();

	// clip_views accessors
	int views_id(int id);
	int views_id();
	int views_clip_id(int id);
	int views_clip_id();
	int64_t views_access_time();
	int views_access_count();

	MediaDb();
	~MediaDb();
};

#endif
