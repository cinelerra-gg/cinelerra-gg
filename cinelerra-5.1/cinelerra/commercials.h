#ifndef _COMMERCIALS_H_
#define _COMMERCIALS_H_

#include "arraylist.h"
#include "asset.inc"
#include "bcprogress.h"
#include "bcwindow.h"
#include "commercials.inc"
#include "condition.inc"
#include "edit.inc"
#include "edits.inc"
#include "file.inc"
#include "garbage.h"
#include "linklist.h"
#include "mediadb.h"
#include "mwindow.inc"
#include "thread.h"
#include "track.inc"
#include "vframe.inc"
#include "filexml.inc"


class Commercials : public Garbage
{
	MediaDb *mdb;
	int armed;
	int clip_id;
	double *clip_weights;
	int64_t frame_start, frame_total;
	double frame_period;
	ArrayList<double> weights;
	ArrayList<double> offsets;
	ArrayList<Clips*> tracks;
	File *scan_file;

	ScanStatus *scan_status;
	int update_cut_info(int trk, double position);
	int update_caption(int trk, int edt, const char *path);
	int update_status(int clip, double start, double end);

	int new_clip(const char *title, int frames, double framerate);
	Clips *find_clips(int pid);
	int put_weight(VFrame *frame, int no);
	int put_frame(VFrame *frame, int no, int group, double offset);
	int skim_weights(int track, double position, double iframerate, int iframes);
	double abs_err(Snip *snip, double *ap, int j, int len, double iframerate);
	double abs_err(double *ap, int len, double iframerate);
	int skim_weight(int track);
	static int skim_weight(void *vp, int track);
public:
	MWindow *mwindow;
	int cancelled, muted;

	int newDb();
	void closeDb();
	int openDb();
	int resetDb();
	void commitDb();
	void undoDb();
	int transaction() { return mdb->transaction(); }
	int attachDb(int rw=0);
	int detachDb();

	int put_clip(File *file, int track, double position, double length);
	int get_frame(File *file, int pid, double position,
		uint8_t *tp, int mw, int mh, int ww, int hh);
	static double frame_weight(uint8_t *tdat, int rowsz, int width, int height);
	int skim_frame(Snips *snips, uint8_t *dat, double position);
	int verify_snip(Snip *snip, double weight, double position);
	int mute_audio(Clip *clip);
	int unmute_audio();
	int test_clip(int clip_id, int track, double position, double &pos);
	int verify_clip(int clip_id, int track, double position, double &pos);
	int write_ads(const char *filename);
	int read_ads(const char *filename);
	void dump_ads();
	int verify_edit(Track *track, Edit *edit, double start, double end);
	Edit * cut_edit(Track *track, Edit *edit, int64_t clip_start, int64_t clip_end);
	int scan_audio(int vstream, double start, double end);
	int scan_media();
	int scan_video();
	int scan_asset(Asset *asset, Track *vtrack, Edit *edit);
	int scan_clips(Track *vtrack, Edit *edit);
	int get_image(int id, uint8_t *dat, int &w, int &h);
	int get_clip_seq_no(int clip_id, int &seq_no, double &offset, int &frame_id);
	MediaDb *media_db() { return mdb; }

	Commercials(MWindow *mwindow);
	~Commercials();
};

class ScanStatusBar : public BC_ProgressBar
{
	int tick, limit;
public:
	int update(int64_t position) {
		if( --tick >= 0 ) return 0;
		tick = limit;
       		return BC_ProgressBar::update(position);
	}
	int update_length(int64_t length) {
		tick = limit = length / 100;
       		return BC_ProgressBar::update_length(length);
	}
	ScanStatusBar(int x, int y, int w, int64_t length, int do_text = 1) :
		BC_ProgressBar(x, y, w, length, do_text) {
		tick = limit = length / 100;
	}
	~ScanStatusBar() {}
};

class ScanStatusGUI : public BC_Window
{
	ScanStatus *sswindow;
	int nbars, nlines;
	BC_Title **texts;
	ScanStatusBar **bars;
	friend class ScanStatus;
public:
	ScanStatusGUI(ScanStatus *sswindow, int x, int y, int nlines, int nbars);
	~ScanStatusGUI();

	void create_objects(const char *text);
};

class ScanStatus : public Thread
{
	int &status;
        void stop();
        void run();
public:
	Commercials *commercials;
        ScanStatusGUI *gui;

	ScanStatus(Commercials *commercials, int x, int y,
		int nlines, int nbars, int &status, const char *text);
        ~ScanStatus();

        int update_length(int i, int64_t length);
	int update_position(int i, int64_t position);
	int update_text(int i, const char *text);
};


class SdbPacket : public ListItem<SdbPacket>
{
public:
	enum sdb_packet_type { sdb_none, sdb_skim_frame, } type;
	SkimDbThread *thread;
	void start();
	virtual void run() = 0;

	SdbPacket(sdb_packet_type ty, SkimDbThread *tp) : type(ty), thread(tp) {}
	~SdbPacket() {}
};

class SdbPacketQueue : public List<SdbPacket>, public Mutex
{
public:
	SdbPacket *get_packet();
	void put_packet(SdbPacket *p);
};

class SdbSkimFrame : public SdbPacket
{
public:
	int pid;
	int64_t framenum;
	double framerate;
	uint8_t dat[SFRM_SZ];

	void load(int pid,int64_t framenum,double framerate,
		uint8_t *idata,int mw,int mh,int iw,int ih);
	void run();

	SdbSkimFrame(SkimDbThread *t) : SdbPacket(sdb_skim_frame, t) {}
	~SdbSkimFrame() {}
};


class SkimDbThread : public Thread
{
	SdbPacketQueue active_packets;
	Condition *input_lock;
	friend class SdbSkimFrame;
public:
	int done;
	SdbPacketQueue skim_frames;
	Commercials *commercials;
	Snips *snips;

	void start(Commercials *commercials);
	void stop();
	void run();
	void put_packet(SdbPacket *p);
	int skim(int pid,int64_t framenum,double framerate,
		uint8_t *idata,int mw,int mh,int iw,int ih);

	SkimDbThread();
	~SkimDbThread();
};


#endif
