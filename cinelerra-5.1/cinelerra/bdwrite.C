//#include "bd.h"
//This program was created by inverting libbluray, without any docs,
//  so it is probably got problems.  Still, works on my Samsung player.
//  thanks to: Petri Hintukainen, William Hahne, John Stebbinsm, et.al.
//
//Usage:
// ./bd <tgt_dir_path> <playlist-0> <sep> <playlistp1> <sep> ... <sep> <playlist-n>
//    <sep> == -<pgm_pid> | --<pgm_pid> | ---<pgm_pid>
//    <pgm_pid> may be empty string, or a numeric pgm_pid for curr title clip
//    <pgm_pid> defaults to first pgm probed.
//    <playlist-x> == <clip-0.m2ts> <clip-1.m2ts> ... <clip-n.m2ts>
// eg:
// ./brwrite /tmp/dir /path/menu.m2ts --- /path/clip0.m2ts /path/clip1.m2ts -- /path/clip2.m2ts
//
// one title is built for each playlist
// playlist-0 is used as first-play item
//
// the basic idea is to use playlist-0 as a menu / directions to
// use the bluray player remote-control to select the desired title
// and start the play, avoiding the need for a menu system.
//
// if the first play item is the main title, that is ok also.
// ./brwrite /tmp/dir /path/title.m2ts
//
//
//To use a bluray bd-re rewriteable: (such as for /dev/sr1)
// dvd+rw-format /dev/sr1  (only done once to init the media)
// mkudffs /dev/sr1 $((`cat /sys/block/sr1/size`*512/2048-1))
// mount /dev/sr1 /mnt1
// cp -av <tgd_dir_path>/BDMV /mnt1/.
// umount /mnt1
// eject sr1
//
#ifndef __BD_H__
#define __BD_H__
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <limits.h>
#include <sys/stat.h>
// work arounds (centos)
#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif
#ifndef INT64_MIN
#define INT64_MIN (-INT64_MAX-1)
#endif

#include "arraylist.h"
#include "cstrdup.h"
#define BCTEXTLEN 1024
#define BLURAY_TS_PKTSZ 192L

static const int bd_sig = 2;

extern "C" {
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

class stream;

enum {
  obj_hdmv = 1,
  obj_bdj = 2,
  title_len = 20,
  pb_typ_movie = 0,
  pb_typ_iactv = 1,
  pb_len = 16,

  BLURAY_APP_TYPE_MAIN_MOVIE = 1,
  BLURAY_APP_TYPE_MAIN_TIMED_SLIDE_SHOW = 2,
  BLURAY_APP_TYPE_MAIN_BROWSED_SLIDE_SHOW = 3,
  BLURAY_APP_TYPE_SUBPATH_BROWSED_SLIDE_SHOW = 4,
  BLURAY_APP_TYPE_SUBPATH_IG_MENU = 5,
  BLURAY_APP_TYPE_SUBPATH_TEXT_SUBTITLE = 6,
  BLURAY_APP_TYPE_SUBPATH_ELEMENTARY_STREAM = 7,

  BLURAY_STREAM_TYPE_VIDEO_MPEG1 = 0x01,
  BLURAY_STREAM_TYPE_VIDEO_MPEG2 = 0x02,
  BLURAY_STREAM_TYPE_AUDIO_MPEG1 = 0x03,
  BLURAY_STREAM_TYPE_AUDIO_MPEG2 = 0x04,
  BLURAY_STREAM_TYPE_AUDIO_LPCM = 0x80,
  BLURAY_STREAM_TYPE_AUDIO_AC3 = 0x81,
  BLURAY_STREAM_TYPE_AUDIO_DTS = 0x82,
  BLURAY_STREAM_TYPE_AUDIO_TRUHD = 0x83,
  BLURAY_STREAM_TYPE_AUDIO_AC3PLUS = 0x84,
  BLURAY_STREAM_TYPE_AUDIO_DTSHD = 0x85,
  BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER = 0x86,
  BLURAY_STREAM_TYPE_VIDEO_VC1 = 0xea,
  BLURAY_STREAM_TYPE_VIDEO_H264 = 0x1b,
  BLURAY_STREAM_TYPE_VIDEO_H264_MVC = 0x20,
  BLURAY_STREAM_TYPE_SUB_PG = 0x90,
  BLURAY_STREAM_TYPE_SUB_IG = 0x91,
  BLURAY_STREAM_TYPE_SUB_TEXT = 0x92,
  BLURAY_STREAM_TYPE_AUDIO_AC3PLUS_SECONDARY = 0xa1,
  BLURAY_STREAM_TYPE_AUDIO_DTSHD_SECONDARY = 0xa2,

  BLURAY_MARK_TYPE_ENTRY = 0x01,
  BLURAY_MARK_TYPE_LINK  = 0x02,

  BLURAY_PLAYBACK_TYPE_SEQUENTIAL = 1,
  BLURAY_PLAYBACK_TYPE_RANDOM = 2,
  BLURAY_PLAYBACK_TYPE_SHUFFLE = 3,

  BLURAY_VIDEO_FORMAT_480I = 1,  // ITU-R BT.601-5
  BLURAY_VIDEO_FORMAT_576I = 2,  // ITU-R BT.601-4
  BLURAY_VIDEO_FORMAT_480P = 3,  // SMPTE 293M
  BLURAY_VIDEO_FORMAT_1080I = 4, // SMPTE 274M
  BLURAY_VIDEO_FORMAT_720P = 5,  // SMPTE 296M
  BLURAY_VIDEO_FORMAT_1080P = 6, // SMPTE 274M
  BLURAY_VIDEO_FORMAT_576P = 7,  // ITU-R BT.1358

  BLURAY_VIDEO_RATE_24000_1001 = 1, // 23.976
  BLURAY_VIDEO_RATE_24 = 2,
  BLURAY_VIDEO_RATE_25 = 3,
  BLURAY_VIDEO_RATE_30000_1001 = 4, // 29.97
  BLURAY_VIDEO_RATE_50 = 6,
  BLURAY_VIDEO_RATE_60000_1001 = 7, // 59.94

  BLURAY_ASPECT_RATIO_4_3 = 2,
  BLURAY_ASPECT_RATIO_16_9 = 3,

  BLURAY_AUDIO_FORMAT_MONO = 1,
  BLURAY_AUDIO_FORMAT_STEREO = 3,
  BLURAY_AUDIO_FORMAT_MULTI_CHAN = 6,
  BLURAY_AUDIO_FORMAT_COMBO = 12,   // Stereo ac3/dts,

  BLURAY_AUDIO_RATE_48 = 1,
  BLURAY_AUDIO_RATE_96 = 4,
  BLURAY_AUDIO_RATE_192 = 5,
  BLURAY_AUDIO_RATE_192_COMBO = 12, // 48 or 96 ac3/dts, 192 mpl/dts-hd
  BLURAY_AUDIO_RATE_96_COMBO = 14,  // 48 ac3/dts, 96 mpl/dts-hd
  BLURAY_TEXT_CHAR_CODE_UTF8 = 0x01,
  BLURAY_TEXT_CHAR_CODE_UTF16BE = 0x02,
  BLURAY_TEXT_CHAR_CODE_SHIFT_JIS = 0x03,
  BLURAY_TEXT_CHAR_CODE_EUC_KR = 0x04,
  BLURAY_TEXT_CHAR_CODE_GB18030_20001 = 0x05,
  BLURAY_TEXT_CHAR_CODE_CN_GB = 0x06,
  BLURAY_TEXT_CHAR_CODE_BIG5 = 0x07,

  BLURAY_STILL_NONE = 0x00,
  BLURAY_STILL_TIME = 0x01,
  BLURAY_STILL_INFINITE = 0x02,

  BLURAY_PG_TEXTST_STREAM = 0x01,
  BLURAY_SECONDARY_VIDEO_STREAM = 0x02,
  BLURAY_SECONDARY_AUDIO_STREAM = 0x03,
};

class bs_file {
  FILE *fp;
  uint32_t reg, len;
  int64_t fpos, fsz;
public:
  bs_file() { fp = 0; }
  ~bs_file() {}

  void init();
  int  open(const char *fn);
  void close();
  void write(uint32_t v, int n);
  void pad(int n);
  void padb(int n) { pad(n*8); }
  int64_t posb() { return fpos; }
  void posb(int64_t n);
  int64_t pos() { return posb()*8 + len; }
  void writeb(uint8_t * bp, int n);
  void writeb(const char *cp, int n) {
    writeb((uint8_t *) cp, n);
  }
};

class bs_length {
  int64_t fpos, len;
public:
  bs_length() { fpos = len = 0; }
  int64_t bs_posb(bs_file &bs) { return bs.posb() - fpos; }
  void bs_len(bs_file &bs, int n) {
    bs.write(len, n);  fpos = bs.posb();
  }
  void bs_end(bs_file &bs) {
    len = bs_posb(bs);
  }
  void bs_ofs(bs_file &bs, int n) {
    bs.write(fpos-n/8, n);
  }
  void bs_zofs(bs_file &bs, int n) {
    bs.write(!len ? 0 : fpos-n/8, n);
  }
};

class _bd_stream_info {
public:
  uint8_t coding_type;
  uint8_t format;
  uint8_t rate;
  uint8_t char_code;
  uint8_t lang[4];
  uint16_t pid;
  uint8_t aspect;
  uint8_t subpath_id;

  _bd_stream_info() { memset(this, 0, sizeof(*this)); }
  ~_bd_stream_info() {}
};

class bd_stream_info : public _bd_stream_info {
public:
  bd_stream_info() {}
  ~bd_stream_info() {}
};

class _bd_clip_info {
public:
  uint32_t pkt_count;
  uint8_t still_mode;
  uint16_t still_time;          /* seconds */
  uint64_t start_time;
  uint64_t in_time;
  uint64_t out_time;

  _bd_clip_info() { memset(this, 0, sizeof(*this)); }
  ~_bd_clip_info() {}
};

class bd_clip_info : public _bd_clip_info {
public:
  ArrayList<bd_stream_info *> video_streams;
  ArrayList<bd_stream_info *> audio_streams;
  ArrayList<bd_stream_info *> pg_streams;
  ArrayList<bd_stream_info *> ig_streams;
  ArrayList<bd_stream_info *> sec_audio_streams;
  ArrayList<bd_stream_info *> sec_video_streams;

  bd_clip_info() {}
  ~bd_clip_info() {
    video_streams.remove_all_objects();
    audio_streams.remove_all_objects();
    pg_streams.remove_all_objects();
    ig_streams.remove_all_objects();
    sec_audio_streams.remove_all_objects();
    sec_video_streams.remove_all_objects();
  }
};

class _bd_title_chapter {
public:
  uint32_t idx;
  uint64_t start;
  uint64_t duration;
  uint64_t offset;
  unsigned clip_ref;

  _bd_title_chapter() { memset(this, 0, sizeof(*this)); }
  ~_bd_title_chapter() {}
};

class bd_title_chapter : public _bd_title_chapter {
public:
  bd_title_chapter() {}
  ~bd_title_chapter() {}
};

class _bd_title_mark {
public:
  uint32_t idx;
  int type;
  uint64_t start;
  uint64_t duration;
  uint64_t offset;
  unsigned clip_ref;

  _bd_title_mark() { memset(this, 0, sizeof(*this)); }
  ~_bd_title_mark() {}
};

class bd_title_mark : public _bd_title_mark {
public:
  bd_title_mark() {}
  ~bd_title_mark() {}
};

class _bd_title_info {
public:
  uint32_t idx;
  uint32_t playlist;
  uint64_t duration;
  uint8_t angle_count;

  _bd_title_info() { memset(this, 0, sizeof(*this)); }
  ~_bd_title_info() {}
};

class bd_title_info : public _bd_title_info {
public:
  ArrayList<bd_clip_info *> clips;
  ArrayList<bd_title_chapter *> chapters;
  ArrayList<bd_title_mark *> marks;

  bd_title_info() {}
  ~bd_title_info() {
    clips.remove_all_objects();
    chapters.remove_all_objects();
    marks.remove_all_objects();
  }
};

class _clpi_stc_seq {
public:
  uint16_t pcr_pid;
  uint32_t spn_stc_start;
  uint32_t presentation_start_time;
  uint32_t presentation_end_time;

  _clpi_stc_seq() { memset(this, 0, sizeof(*this)); }
  ~_clpi_stc_seq() {}
};

class clpi_stc_seq : public _clpi_stc_seq {
public:
  int write();

  clpi_stc_seq() {}
  ~clpi_stc_seq() {}
};

class _clpi_atc_seq {
public:
  uint32_t spn_atc_start;
  uint8_t offset_stc_id;

  _clpi_atc_seq() { memset(this, 0, sizeof(*this)); }
  ~_clpi_atc_seq() {}
};

class clpi_atc_seq : public _clpi_atc_seq {
public:
  ArrayList<clpi_stc_seq *> stc_seq;
  int write();

  clpi_atc_seq() {}
  ~clpi_atc_seq() {
    stc_seq.remove_all_objects();
  }
};

class clpi_sequences : public bs_length,
			public ArrayList<clpi_atc_seq *> {
public:
  int write();

  clpi_sequences() {}
  ~clpi_sequences() { remove_all_objects(); }
};

class _clpi_ts_type {
public:
  uint8_t validity;
  char format_id[5];

  _clpi_ts_type() { memset(this, 0, sizeof(*this)); }
  ~_clpi_ts_type() {}
};

class clpi_ts_type : public _clpi_ts_type {
public:
  clpi_ts_type() {}
  ~clpi_ts_type() {}
};

class _clpi_atc_delta {
public:
  uint32_t delta;
  char file_id[6];
  char file_code[5];

  _clpi_atc_delta() { memset(this, 0, sizeof(*this)); }
  ~_clpi_atc_delta() {}
};

class clpi_atc_delta : public _clpi_atc_delta {
public:
  int write();

  clpi_atc_delta() {}
  ~clpi_atc_delta() {}
};

class _clpi_font {
public:
  char file_id[6];

  _clpi_font() { memset(this, 0, sizeof(*this)); }
  ~_clpi_font() {}
};

class clpi_font : public _clpi_font {
public:
  clpi_font() {}
  ~clpi_font() {}
};

class clpi_font_info {
public:
  ArrayList<clpi_font *> font;

  clpi_font_info() {}
  ~clpi_font_info() {
    font.remove_all_objects();
  }
};

class _clpi_clip_info {
public:
  uint8_t clip_stream_type;
  uint8_t application_type;
  uint8_t is_atc_delta;
  uint32_t ts_recording_rate;
  uint32_t num_source_packets;

  _clpi_clip_info() { memset(this, 0, sizeof(*this)); }
  ~_clpi_clip_info() {}
};

class clpi_clip_info : public bs_length, public _clpi_clip_info {
public:
  clpi_ts_type ts_type_info;
  clpi_font_info font_info;
  ArrayList<clpi_atc_delta *> atc_delta;
  int write();

  clpi_clip_info() {}
  ~clpi_clip_info() {
    atc_delta.remove_all_objects();
  }
};

class _clpi_prog_stream {
public:
  uint16_t pid;
  uint8_t coding_type;
  uint8_t format;
  uint8_t rate;
  uint8_t aspect;
  uint8_t oc_flag;
  uint8_t char_code;
  char lang[4];

  _clpi_prog_stream() { memset(this, 0, sizeof(*this)); }
  ~_clpi_prog_stream() {}
};

class clpi_prog_stream : public bs_length, public _clpi_prog_stream {
public:
  int write();

  clpi_prog_stream() {}
  ~clpi_prog_stream() {}
};

class _clpi_ep_coarse {
public:
  int ref_ep_fine_id;
  int pts_ep;
  uint32_t spn_ep;

  _clpi_ep_coarse() { memset(this, 0, sizeof(*this)); }
  ~_clpi_ep_coarse() {}
};

class clpi_ep_coarse : public _clpi_ep_coarse {
public:
  int write();

  clpi_ep_coarse() {}
  ~clpi_ep_coarse() {}
};

class _clpi_ep_fine {
public:
  uint8_t is_angle_change_point;
  uint8_t i_end_position_offset;
  int pts_ep;
  int spn_ep;

  _clpi_ep_fine() { memset(this, 0, sizeof(*this)); }
  ~_clpi_ep_fine() {}
};

class clpi_ep_fine : public _clpi_ep_fine {
public:
  int write();

  clpi_ep_fine() {}
  ~clpi_ep_fine() {}
};

class _clpi_ep_map_entry {
public:
  uint8_t ep_stream_type;
  uint32_t ep_map_stream_start_addr;

  _clpi_ep_map_entry() { memset(this, 0, sizeof(*this)); }
  ~_clpi_ep_map_entry() {}
};

class clpi_ep_map_entry : public _clpi_ep_map_entry {
public:
  uint32_t fine_start;
  uint16_t pid;
  ArrayList<clpi_ep_coarse *> coarse;
  ArrayList<clpi_ep_fine *> fine;
  int write(uint32_t ep_map_pos);
  int write_map();

  clpi_ep_map_entry(int id) { fine_start = 0;  pid = id; }
  ~clpi_ep_map_entry() {
    coarse.remove_all_objects();
    fine.remove_all_objects();
  }
};

class _clpi_prog {
public:
  uint32_t spn_program_sequence_start;
  uint8_t num_groups;
  _clpi_prog() { memset(this, 0, sizeof(*this)); }
  ~_clpi_prog() {}
};

class clpi_prog : public _clpi_prog {
public:
  ArrayList<clpi_prog_stream *> streams;
  uint16_t program_map_pid;

  int write();
  clpi_prog(int pmt_pid) { program_map_pid = pmt_pid; }
  ~clpi_prog() { streams.remove_all_objects(); }
};

class clpi_programs : public bs_length,
			public ArrayList<clpi_prog *> {
public:
  int write();

  clpi_programs() {}
  ~clpi_programs() { remove_all_objects(); }
};

class clpi_extents : public bs_length,
			public ArrayList<uint32_t> {
public:
  int write();

  clpi_extents() {}
  ~clpi_extents() {}
};
class _clpi_cpi {
public:
  uint8_t type;

  _clpi_cpi() { type = 0; }
  ~_clpi_cpi() {}
};

class clpi_cpi : public bs_length, public _clpi_cpi,
			public ArrayList<clpi_ep_map_entry *> {
public:
  int write();

  clpi_cpi() {}
  ~clpi_cpi() { remove_all_objects(); }
};

class clpi_cmrk : public bs_length {
public:
  int write();

  clpi_cmrk() {}
  ~clpi_cmrk() {}
};


class bd_uo_mask {
public:
  unsigned int menu_call : 1;
  unsigned int title_search : 1;
  unsigned int chapter_search : 1;
  unsigned int time_search : 1;
  unsigned int skip_to_next_point : 1;
  unsigned int skip_to_prev_point : 1;
  unsigned int play_firstplay : 1;
  unsigned int stop : 1;
  unsigned int pause_on : 1;
  unsigned int pause_off : 1;
  unsigned int still_off : 1;
  unsigned int forward : 1;
  unsigned int backward : 1;
  unsigned int resume : 1;
  unsigned int move_up : 1;
  unsigned int move_down : 1;
  unsigned int move_left : 1;
  unsigned int move_right : 1;
  unsigned int select : 1;
  unsigned int activate : 1;
  unsigned int select_and_activate : 1;
  unsigned int primary_audio_change : 1;
  unsigned int reserved0 : 1;
  unsigned int angle_change : 1;
  unsigned int popup_on : 1;
  unsigned int popup_off : 1;
  unsigned int pg_enable_disable : 1;
  unsigned int pg_change : 1;
  unsigned int secondary_video_enable_disable : 1;
  unsigned int secondary_video_change : 1;
  unsigned int secondary_audio_enable_disable : 1;
  unsigned int secondary_audio_change : 1;
  unsigned int reserved1 : 1;
  unsigned int pip_pg_change : 1;

  int write();
  bd_uo_mask() {
    memset(this, 0, sizeof(*this));
  }
  ~bd_uo_mask() {
  }
};

class _mpls_stream {
public:
  uint8_t stream_type;
  uint8_t coding_type;
  uint16_t pid;
  uint8_t subpath_id;
  uint8_t subclip_id;
  uint8_t format;
  uint8_t rate;
  uint8_t char_code;
  char lang[4];

  _mpls_stream() { memset(this, 0, sizeof(*this)); }
  ~_mpls_stream() {}
};

class mpls_stream : public _mpls_stream {
public:
  bs_length strm, code;
  int write();

  mpls_stream() {}
  ~mpls_stream() {}
};

class _mpls_stn {
public:
  uint8_t num_pip_pg;
  _mpls_stn() {
    num_pip_pg = 0;
  }
  ~_mpls_stn() {}
};

class mpls_stn : public bs_length, public _mpls_stn {
public:
  ArrayList<mpls_stream *> video;
  ArrayList<mpls_stream *> audio;
  ArrayList<mpls_stream *> pg;
  ArrayList<mpls_stream *> ig;
  ArrayList<mpls_stream *> secondary_audio;
  ArrayList<mpls_stream *> secondary_video;
  // Secondary audio specific fields
  ArrayList<uint8_t> sa_primary_audio_ref;
  // Secondary video specific fields
  ArrayList<uint8_t> sv_secondary_audio_ref;
  ArrayList<uint8_t> sv_pip_pg_ref;

  int write();
  mpls_stn() {}
  ~mpls_stn() {
    video.remove_all_objects();
    audio.remove_all_objects();
    pg.remove_all_objects();
    ig.remove_all_objects();
    secondary_audio.remove_all_objects();
    secondary_video.remove_all_objects();
  }
};

class _mpls_clip {
public:
  char clip_id[6];
  char codec_id[5];
  uint8_t stc_id;

  _mpls_clip() { memset(this, 0, sizeof(*this)); }
  ~_mpls_clip() {}
};

class mpls_clip : public _mpls_clip {
public:
  mpls_clip() { strcpy(codec_id, "M2TS"); }
  ~mpls_clip() {}
};

class _mpls_pi {
public:
  uint8_t is_multi_angle;
  uint8_t connection_condition;
  uint32_t in_time;
  uint32_t out_time;
  uint8_t random_access_flag;
  uint8_t still_mode;
  uint16_t still_time;
  uint8_t is_different_audio;
  uint8_t is_seamless_angle;

  _mpls_pi() { memset(this, 0, sizeof(*this)); }
  ~_mpls_pi() {}
};

class mpls_pi : public bs_length, public _mpls_pi {
public:
  bd_uo_mask uo_mask;
  ArrayList<mpls_clip *> clip;
  mpls_stn stn;

  int write();

  mpls_pi() {}
  ~mpls_pi() {
    clip.remove_all_objects();
  }
};

class _mpls_plm {
public:
  uint8_t mark_id;
  uint8_t mark_type;
  uint16_t play_item_ref;
  uint32_t time;
  uint16_t entry_es_pid;
  uint32_t duration;

  _mpls_plm() { memset(this, 0, sizeof(*this)); }
  ~_mpls_plm() {}
};

class mpls_plm : public _mpls_plm {
public:
  int write();

  mpls_plm() {}
  ~mpls_plm() {}
};

class _mpls_ai {
public:
  uint8_t playback_type;
  uint16_t playback_count;
  uint8_t random_access_flag;
  uint8_t audio_mix_flag;
  uint8_t lossless_bypass_flag;

  _mpls_ai() { memset(this, 0, sizeof(*this)); }
  ~_mpls_ai() {}
};

class mpls_ai : public bs_length, public _mpls_ai {
public:
  bd_uo_mask uo_mask;
  int write();

  mpls_ai() {}
  ~mpls_ai() {}
};

class _mpls_sub_pi {
public:
  uint8_t connection_condition;
  uint8_t is_multi_clip;
  uint32_t in_time;
  uint32_t out_time;
  uint16_t sync_play_item_id;
  uint32_t sync_pts;

  _mpls_sub_pi() { memset(this, 0, sizeof(*this)); }
  ~_mpls_sub_pi() {}
};

class mpls_sub_pi : public bs_length, public _mpls_sub_pi {
public:
  ArrayList<mpls_clip *> clip;
  int write();

  mpls_sub_pi() {}
  ~mpls_sub_pi() {
    clip.remove_all_objects();
  }
};

class _mpls_sub {
public:
  uint8_t type;
  uint8_t is_repeat;

  _mpls_sub() { memset(this, 0, sizeof(*this)); }
  ~_mpls_sub() {}
};

class mpls_sub : public bs_length, public _mpls_sub {
public:
  ArrayList<mpls_sub_pi *> sub_play_item;

  int write();
  mpls_sub() {}
  ~mpls_sub() {
   sub_play_item.remove_all_objects();
  }
};

class _mpls_pip_data {
public:
  uint32_t time;
  uint16_t xpos;
  uint16_t ypos;
  uint8_t scale_factor;

  _mpls_pip_data() { memset(this, 0, sizeof(*this)); }
  ~_mpls_pip_data() {}
};

class mpls_pip_data : public _mpls_pip_data {
public:
  int write();

  mpls_pip_data() {}
  ~mpls_pip_data() {}
};

class _mpls_pip_metadata {
public:
  uint16_t clip_ref;
  uint8_t secondary_video_ref;
  uint8_t timeline_type;
  uint8_t luma_key_flag;
  uint8_t upper_limit_luma_key;
  uint8_t trick_play_flag;

  _mpls_pip_metadata() { memset(this, 0, sizeof(*this)); }
  ~_mpls_pip_metadata() {}
};

class mpls_pip_metadata : public _mpls_pip_metadata {
public:
  ArrayList<mpls_pip_data *> data;

  int write(uint32_t start_address);
  mpls_pip_metadata() {}
  ~mpls_pip_metadata() {
    data.remove_all_objects();
  }
};

class _mpls_pl {
public:
  uint32_t list_pos;
  uint32_t mark_pos;
  uint32_t ext_pos;

  _mpls_pl() { memset(this, 0, sizeof(*this)); }
  ~_mpls_pl() {}
};

class mpls_pl : public _mpls_pl {
public:
  int sig;
  bs_length play, mark, subx, pipm;
  mpls_ai app_info;
  ArrayList<mpls_pi *> play_item;
  ArrayList<mpls_sub *> sub_path;
  ArrayList<mpls_plm *> play_mark;
  // extension data (profile 5, version 2.4)
  ArrayList<mpls_sub *> ext_sub_path;
  // extension data (Picture-In-Picture metadata)
  ArrayList<mpls_pip_metadata *> ext_pip_data;

  int write_header();
  int write_playlist();
  int write_playlistmark();
  int write_subpath_extension();
  int write_pip_metadata_extension();
  int write();

  mpls_pl() { sig = bd_sig; }
  ~mpls_pl() {
    play_item.remove_all_objects();
    sub_path.remove_all_objects();
    play_mark.remove_all_objects();
    ext_sub_path.remove_all_objects();
    ext_pip_data.remove_all_objects();
  }
};

class _clpi_cl {
public:
  uint32_t sequence_info_start_addr;
  uint32_t program_info_start_addr;
  uint32_t cpi_start_addr;
  uint32_t clip_mark_start_addr;
  uint32_t ext_data_start_addr;

  _clpi_cl() { memset(this, 0, sizeof(*this)); }
  ~_clpi_cl() {}
};

class clpi_cl : public _clpi_cl {
public:
  int sig;
  clpi_clip_info clip;
  clpi_sequences sequences;
  clpi_programs programs;
  clpi_cpi cpi;
  clpi_extents extents;
  clpi_programs programs_ss;
  clpi_cpi cpi_ss;
  clpi_cmrk cmrk;

  int write_header();
  int write();

  int write_clpi_extension(int id1, int id2, void *handle);
  int write_mpls_extension(int id1, int id2, void *handle);

  clpi_cl() { sig = bd_sig; }
  ~clpi_cl() {}
};

class command_obj {
public:
  union {
    uint32_t cmd;
    struct {
      uint32_t sub_grp:3;       /* command sub-group */
      uint32_t grp:2;           /* command group */
      uint32_t op_cnt:3;        /* operand count */
      uint32_t branch_opt:4;    /* branch option */
      uint32_t rsvd1:2;
      uint32_t imm_op2:1;       /* I-flag for operand 2 */
      uint32_t imm_op1:1;       /* I-flag for operand 1 */
      uint32_t cmp_opt:4;       /* compare option */
      uint32_t rsvd2:4;
      uint32_t set_opt:5;       /* set option */
      uint32_t rsvd3:3;
    };
  };
  uint32_t dst;
  uint32_t src;

  command_obj() { cmd = dst = src = 0; }
  ~command_obj() {}
};

class _movie_obj {
public:
  int resume_intention_flag;
  int menu_call_mask;
  int title_search_mask;

  _movie_obj() { memset(this, 0, sizeof(*this)); }
  ~_movie_obj() {}
};

class movie_obj : public _movie_obj {
public:
  ArrayList<command_obj *> cmds;

  movie_obj() {}
  ~movie_obj() {
    cmds.remove_all_objects();
  }
};

class movie_file : public bs_length {
public:
  int sig;
  ArrayList<movie_obj *> movies;

  movie_file() { sig = bd_sig; }
  ~movie_file() {
    movies.remove_all_objects();
  }

  int write();
};

class _pb_obj {
public:
  int obj_typ;
  int pb_typ;
  int hdmv_id;

  _pb_obj() { memset(this, 0, sizeof(*this)); }
  ~_pb_obj() {}
};

class pb_obj : public _pb_obj {
public:
  int last;
  char *bdj_name;
  void write_obj();

  pb_obj() {
    last = 0;
    bdj_name = 0;
  }
  ~pb_obj() {
    delete [] bdj_name;
  }

  void set_hdmv(int id, int pt);
  void set_bdj(char *nm, int pt);
  void write_hdmv_obj(int id_ref);
  void write_bdj_obj(char *name);
};

class _title_obj {
public:
  int acc_typ;
  _title_obj() { acc_typ = 0; }
  ~_title_obj() {}
};

class title_obj : public pb_obj, public _title_obj {
public:
  title_obj() {}
  ~title_obj() {}

  void write_obj();
};

class _index_file {
public:
  int sig;
  int initial_output_mode_preference;
  int content_exist_flag;
  int video_format;
  int frame_rate;

  _index_file() { memset(this, 0, sizeof(*this)); }
  ~_index_file() {}
};

class index_file : public bs_length, public _index_file {
public:
  ArrayList<title_obj *> titles;
  pb_obj first_play;
  pb_obj top_menu;
  char user_data[32];
  bs_length exten, appinf;

  void write_hdmv_obj(int hdmv_typ, int id_ref);
  void write_bdj_obj(int bdj_typ, char *name);
  void write_pb_obj(pb_obj * pb);
  int write();

  index_file() {
    sig = bd_sig;
    memset(user_data, 0, sizeof(user_data));
  }
  ~index_file() {
    titles.remove_all_objects();
  }
};

class _bdid {
public:
  uint32_t data_start;
  uint32_t extension_data_start;
  char org_id[4];
  char disc_id[16];

  _bdid() { memset(this, 0, sizeof(*this)); }
  ~_bdid() {}
};

class bdid : public _bdid {
public:
  int sig;
  int write();

  bdid() { sig = bd_sig; }
  ~bdid() {}
};

class stream : public bd_stream_info {
public:
  int av_idx;
  AVMediaType type;
  AVCodecID codec_id;
  AVCodecContext *ctx;
  int64_t start_pts;
  int64_t end_pts;
  int64_t last_pts;
  int64_t duration;

  stream(AVMediaType ty, int i) {
    type = ty;  av_idx = i;  ctx = 0;
    start_pts = INT64_MAX; end_pts = INT64_MIN;
    last_pts = -1;
  }
  ~stream() { if( ctx ) avcodec_free_context(&ctx); }
};

class mark {
public:
  static int cmpr(mark **, mark **);
  int64_t sample, pos, pts;
  uint32_t pkt, pid;
  mark(int64_t s, int64_t p, int64_t ts, uint32_t pk, int id) {
    sample = s;  pos = p;  pts = ts; pkt = pk;  pid = id;
  }
};

int mark::cmpr(mark **a, mark **b)
{
  mark *ap = *(mark **)a,  *bp = *(mark **)b;
  return ap->pts > bp->pts ? 1 : ap->pts < bp->pts ? -1 : 0;
}

class _program {
public:
  int pmt_pid, pcr_pid, ep_pid;
  int64_t start_time, end_time;
  int64_t duration;
  _program() {
    memset(this, 0, sizeof(*this));
    start_time = INT64_MAX; end_time = INT64_MIN;
  }
  ~_program() {}
};

class program : public _program {
public:
  int idx;
  uint16_t pid;
  ArrayList<int> strm_idx;
  ArrayList<mark*> marks;
  int64_t curr_pos;
  int build_toc(clpi_ep_map_entry *map);
  void add_label(uint32_t pk, int64_t p, int64_t ts, int id) {
    marks.append(new mark(curr_pos, p, ts, pk, id));
  }

  program(int i, int id) { idx = i;  pid = id;  curr_pos = 0; }
  ~program() { marks.remove_all_objects(); }
};

class _media_info {
public:
  int bit_rate;
  int64_t file_size;

  _media_info() {
    memset(this, 0, sizeof(*this));
  }
  ~_media_info() {}
};

class media_info : public _media_info {
public:
  static const AVRational clk45k;
  char filename[BCTEXTLEN];
  int pgm_pid;
  int brk, pidx, still;
  ArrayList<stream *> streams;
  ArrayList<program *> programs;
  program *prog()  { return programs[pidx]; }

  media_info(const char *fn) {
    strcpy(filename, fn);
    brk = 0;  pidx = 0;  pgm_pid = -1;  still = 0;
  }
  ~media_info() {
    streams.remove_all_objects();
    programs.remove_all_objects();
  }

  int scan();
  int scan(AVFormatContext *fmt_ctx);
};

class Media : public ArrayList<media_info *> {
public:
  char *path;
  char filename[BCTEXTLEN];
  int bd_path(const char *bp, const char *fmt, va_list ap);
  int bd_open(const char *fmt, ...);
  int bd_backup(const char *fmt, ...);
  int bd_copy(const char *ifn, const char *fmt, ...);

  index_file idx;
  movie_file mov;
  ArrayList<clpi_cl *> cl;
  ArrayList<mpls_pl *> pl;

  void add_movie(uint32_t *ops, int n);
  int compose();
  int write(char *fn);

  Media() { path = 0;  filename[0] = 0; }
  ~Media() {
    remove_all_objects();
    cl.remove_all_objects();
    pl.remove_all_objects();
  }
};

#endif

void bs_file::init()
{
  fp = 0;
  reg = len = 0;
  fpos = fsz = 0;
}

int
bs_file::open(const char *fn)
{
  fp = fopen(fn,"w");
  if( !fp ) perror(fn);
  return fp != 0 ? 0 : 1;
}

void
bs_file::close()
{
  if( fp ) { fclose(fp); fp = 0; }
}

void
bs_file::write(uint32_t v, int n)
{
  uint64_t vv = reg;
  vv <<= n;  vv |= v;
  reg = vv;  len += n;
  while( len >= 8 ) {
    len -= 8;
    if( fp ) fputc((vv >> len), fp);
    if( ++fpos > fsz ) fsz = fpos;
  }
}

void
bs_file::writeb(uint8_t * bp, int n)
{
  while( --n >= 0 ) write(*bp++, 8);
}

void
bs_file::pad(int n)
{
  while( n > 32 ) { write(0, 32);  n -= 32; }
  if( n > 0 ) write(0, n);
}

void
bs_file::posb(int64_t n)
{
  if( fpos == fsz && n > fpos ) {
    padb(n-fpos);
    return;
  }
  fpos = n;
  if( fp ) fseek(fp, fpos, SEEK_SET);
}

static bs_file bs;


int
movie_file::write()
{
  // output header
  bs.writeb("MOBJ", 4);
  bs.writeb(sig == 1 ? "0100" : "0200", 4);
  int movie_start = 0x0028;
  bs.posb(movie_start);
  bs_len(bs, 32);
  bs.write(0, 32);
  bs.write(movies.size(), 16);

  for (int i = 0; i < movies.size(); ++i) {
    movie_obj *mov = movies[i];
    bs.write(mov->resume_intention_flag, 1);
    bs.write(mov->menu_call_mask, 1);
    bs.write(mov->title_search_mask, 1);
    bs.write(0, 13);
    ArrayList<command_obj *> &cmds = mov->cmds;
    int num_cmds = cmds.size();
    bs.write(num_cmds, 16);
    for (int j = 0; j < num_cmds; ++j) {
      command_obj *cmd = cmds[j];
      bs.write(cmd->op_cnt, 3);
      bs.write(cmd->grp, 2);
      bs.write(cmd->sub_grp, 3);
      bs.write(cmd->imm_op1, 1);
      bs.write(cmd->imm_op2, 1);
      bs.write(0, 2);
      bs.write(cmd->branch_opt, 4);
      bs.write(0, 4);
      bs.write(cmd->cmp_opt, 4);
      bs.write(0, 3);
      bs.write(cmd->set_opt, 5);
      //bs.write(cmd->cmd, 32);
      bs.write(cmd->dst, 32);
      bs.write(cmd->src, 32);
    }
  }
//in sony AVCHD
//  bs.write('l', 8);
//  bs.writebcd(year, 16);
//  bs.writebcd(month, 8);
//  bs.writebcd(day, 8);
//  bs.writebcd(hour, 8);
//  bs.writebcd(min, 8);
//  bs.writebcd(sec, 8);
  bs_end(bs);
  return 0;
}

void
pb_obj::set_hdmv(int id, int pt)
{
  obj_typ = obj_hdmv;
  pb_typ = pt;
  delete [] bdj_name;
  bdj_name = 0;
  hdmv_id = id;
}

void
pb_obj::set_bdj(char *nm, int pt)
{
  obj_typ = obj_bdj;
  pb_typ = pt;
  delete [] bdj_name;
  bdj_name = 0;
  bdj_name = cstrdup(nm);
}

void
pb_obj::write_hdmv_obj(int id_ref)
{
  bs.write(pb_typ, 2);
  bs.write(0, 14);
  bs.write(id_ref, 16);
  bs.write(0, 32);
}

void
pb_obj::write_bdj_obj(char *name)
{
  bs.write(pb_typ, 2);
  bs.write(0, 14);
  bs.writeb(name, 5);
  bs.write(0, 8);
}

void
pb_obj::write_obj()
{
  switch (obj_typ) {
  case obj_bdj:
    write_bdj_obj(bdj_name);
    break;
  case obj_hdmv:
  default:
    write_hdmv_obj(hdmv_id);
    break;
  }
}

void
title_obj::write_obj()
{
  bs.write(obj_typ, 2);
  bs.write(acc_typ, 2);
  bs.write(0, 28);
  pb_obj::write_obj();
}


void
index_file::write_pb_obj(pb_obj * pb)
{
  bs.write(pb->obj_typ, 2);
  bs.write(0, 30);
  pb->write_obj();
}

int
index_file::write()
{
  // output header
  bs.writeb("INDX", 4);
  bs.writeb(sig == 1 ? "0100" : "0200", 4);
  bs_ofs(bs, 32);
  exten.bs_zofs(bs, 32);
  int appinfo_start = 0x28;
  bs.posb(appinfo_start);
  appinf.bs_len(bs, 32);
  bs.write(1, 1);
  bs.write(initial_output_mode_preference, 1);
  bs.write(content_exist_flag, 1);
  bs.write(0, 5);
  bs.write(video_format, 4);
  bs.write(frame_rate, 4);
  bs.writeb(user_data, 32);
  appinf.bs_end(bs);

  // output index
  bs_len(bs, 32);
  write_pb_obj(&first_play);
  write_pb_obj(&top_menu);
  bs.write(titles.size(), 16);

  for (int i = 0; i < titles.size(); ++i)
    titles[i]->write_obj();
  bs_end(bs);
  exten.bs_len(bs,32);
  exten.bs_end(bs);
  return 0;
}



int
bdid::write()
{
  bs.writeb("BDID",4);
  bs.writeb(sig == 1 ? "0100" : "0200", 4);
  bs.write(data_start, 32);
  bs.write(extension_data_start, 32);
  bs.posb(40 - 16);
  bs.writeb(org_id, sizeof(org_id));
  bs.writeb(disc_id, sizeof(disc_id));
  return 0;
}

// XXX - not current referenced
#if 0
int
bdmv_write_extension_data(int start_address,
	int (*handler) (int, int, void *), void *handle)
{
  int64_t length;
  int num_entries, n;

  bs.write(length, 32);   /* length of extension data block */
  if( length < 1 )
    return 1;

  bs.pad(32);             /* relative start address of extension data */
  bs.pad(24);             /* padding */
  bs.write(num_entries, 8);

  for (n = 0; n < num_entries; n++) {
    bs.write(id1, 16);
    bs.write(id2, 16);
    bs.write(ext_start, 32);
    bs.write(ext_len, 32);
    saved_pos = bs.pos() >> 3;
    bs.posb(start_address + ext_start);
    handler(id1, id2, handle);
    bs.posb(saved_pos);
  }

  return 0;
}
#endif

int
clpi_prog_stream::write()
{
  bs.write(pid, 16);
  bs_len(bs, 8);

  bs.write(coding_type, 8);
  switch (coding_type) {
  case BLURAY_STREAM_TYPE_VIDEO_MPEG1:
  case BLURAY_STREAM_TYPE_VIDEO_MPEG2:
  case BLURAY_STREAM_TYPE_VIDEO_VC1:
  case BLURAY_STREAM_TYPE_VIDEO_H264:
  case 0x20:
    bs.write(format, 4);
    bs.write(rate, 4);
    bs.write(aspect, 4);
    bs.pad(2);
    bs.write(oc_flag, 1);
    bs.pad(1);
    break;

  case BLURAY_STREAM_TYPE_AUDIO_MPEG1:
  case BLURAY_STREAM_TYPE_AUDIO_MPEG2:
  case BLURAY_STREAM_TYPE_AUDIO_LPCM:
  case BLURAY_STREAM_TYPE_AUDIO_AC3:
  case BLURAY_STREAM_TYPE_AUDIO_DTS:
  case BLURAY_STREAM_TYPE_AUDIO_TRUHD:
  case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS:
  case BLURAY_STREAM_TYPE_AUDIO_DTSHD:
  case BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER:
  case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS_SECONDARY:
  case BLURAY_STREAM_TYPE_AUDIO_DTSHD_SECONDARY:
    bs.write(format, 4);
    bs.write(rate, 4);
    bs.writeb(lang, 3);
    break;

  case BLURAY_STREAM_TYPE_SUB_PG:
  case BLURAY_STREAM_TYPE_SUB_IG:
  case 0xa0:
    bs.writeb(lang, 3);
    break;

  case BLURAY_STREAM_TYPE_SUB_TEXT:
    bs.write(char_code, 8);
    bs.writeb(lang, 3);
    break;

  default:
    fprintf(stderr, "clpi_prog_stream: unrecognized coding type %02x\n",
            coding_type);
    return 1;
  };

  bs.padb(0x15 - bs_posb(bs));
  bs_end(bs);
  return 0;
}

int
clpi_cl::write_header()
{
  bs.writeb("HDMV", 4);
  bs.writeb(sig == 1 ? "0100" : "0200", 4);
  bs.write(sequence_info_start_addr, 32);
  bs.write(program_info_start_addr, 32);
  bs.write(cpi_start_addr, 32);
  bs.write(clip_mark_start_addr, 32);
  bs.write(ext_data_start_addr, 32);
  return 0;
}

int
clpi_atc_delta::write()
{
  bs.write(delta, 32);
  bs.writeb(file_id, 5);
  bs.writeb(file_code, 4);
  bs.pad(8);
  return 0;
}

int
clpi_clip_info::write()
{
  bs.posb(40);
  bs_len(bs, 32);

  bs.pad(16);                   // reserved
  bs.write(clip_stream_type, 8);
  bs.write(application_type, 8);
  bs.pad(31);                   // skip reserved 31 bits
  bs.write(is_atc_delta, 1);
  bs.write(ts_recording_rate, 32);
  bs.write(num_source_packets, 32);

  bs.padb(128);                 // Skip reserved 128 bytes

  // ts type info block
  int ts_len = 30;
  bs.write(ts_len, 16);
  int64_t pos = bs.posb();
  if( ts_len ) {
    bs.write(ts_type_info.validity, 8);
    bs.writeb(ts_type_info.format_id, 4);
    // pad the stuff we don't know anything about
    bs.padb(ts_len - (bs.posb() - pos));
  }

  if( is_atc_delta ) {
    bs.pad(8);                  // Skip reserved byte
    bs.write(atc_delta.size(), 8);
    for( int ii=0; ii < atc_delta.size(); ++ii )
      if( atc_delta[ii]->write() ) return 1;
  }

  // font info
  if( application_type == 6 /* Sub TS for a sub-path of Text subtitle */  ) {
    bs.pad(8);
    bs.write(font_info.font.size(), 8);
    if( font_info.font.size() ) {
      for( int ii=0; ii < font_info.font.size(); ++ii ) {
        bs.writeb(font_info.font[ii]->file_id, 5);
        bs.pad(8);
      }
    }
  }

  bs_end(bs);
  return 0;
}

int
clpi_stc_seq::write()
{
  bs.write(pcr_pid, 16);
  bs.write(spn_stc_start, 32);
  bs.write(presentation_start_time, 32);
  bs.write(presentation_end_time, 32);
  return 0;
}

int
clpi_atc_seq::write()
{
  bs.write(spn_atc_start, 32);
  bs.write(stc_seq.size(), 8);
  bs.write(offset_stc_id, 8);

  for( int ii=0; ii < stc_seq.size(); ++ii )
    if( stc_seq[ii]->write() ) return 1;
  return 0;
}

int
clpi_sequences::write()
{
  bs_len(bs, 32);
  bs.padb(1);                   // reserved byte
  bs.write(size(), 8);
  for( int ii=0; ii < size(); ++ii )
    if( get(ii)->write() ) return 1;
  bs_end(bs);
  return 0;
}

int
clpi_prog::write()
{
  bs.write(spn_program_sequence_start, 32);
  bs.write(program_map_pid, 16);
  bs.write(streams.size(), 8);
  bs.write(num_groups, 8);
  for( int ii=0; ii < streams.size(); ++ii )
    if( streams[ii]->write() ) return 1;
  return 0;
}

int
clpi_programs::write()
{
  bs_len(bs, 32);
  bs.padb(1);                   // reserved byte
  bs.write(size(), 8);
  for( int ii=0; ii < size(); ++ii )
    if( get(ii)->write() ) return 1;
  bs_end(bs);
  return 0;
}

int
clpi_ep_coarse::write()
{
  bs.write(ref_ep_fine_id, 18);
  bs.write(pts_ep, 14);
  bs.write(spn_ep, 32);
  return 0;
}

int
clpi_ep_fine::write()
{
  bs.write(is_angle_change_point, 1);
  bs.write(i_end_position_offset, 3);
  bs.write(pts_ep, 11);
  bs.write(spn_ep, 17);
  return 0;
}

int
clpi_ep_map_entry::write(uint32_t ep_map_pos)
{
  bs.write(pid, 16);
  bs.pad(10);
  bs.write(ep_stream_type, 4);
  bs.write(coarse.size(), 16);
  bs.write(fine.size(), 18);
  bs.write(ep_map_stream_start_addr - ep_map_pos, 32);
  return 0;
}

int
clpi_ep_map_entry::write_map()
{
  ep_map_stream_start_addr = bs.posb();
  bs.write(fine_start, 32);

  for( int ii=0; ii < coarse.size(); ++ii )
    if( coarse[ii]->write() ) return 1;

  fine_start = bs.posb() - ep_map_stream_start_addr;
  for( int ii=0; ii < fine.size(); ++ii )
    if( fine[ii]->write() ) return 1;

  return 0;
}

int
clpi_cpi::write()
{
  bs_len(bs, 32);
  bs.pad(12);
  bs.write(type, 4);
  uint32_t ep_map_pos = bs.posb();

  // EP Map starts here
  bs.pad(8);
  bs.write(size(), 8);

  for( int ii=0; ii < size(); ++ii )
    if( get(ii)->write(ep_map_pos) ) return 1;

  for( int ii=0; ii < size(); ++ii )
    if( get(ii)->write_map() ) return 1;

  bs_end(bs);
  return 0;
}

int
clpi_cmrk::write()
{
  bs_len(bs, 32);
  bs_end(bs);
  return 0;
}

int
clpi_extents::write()
{
  bs_len(bs, 32);
  bs.write(size(), 32);
  for( int ii=0; ii < size(); ++ii )
    bs.write(get(ii), 32);
  return 0;
}

int
clpi_cl::write_clpi_extension(int id1, int id2, void *handle)
{
  clpi_cl *cl = (clpi_cl *) handle;

  if( id1 == 1 ) {
    if( id2 == 2 ) {
      // LPCM down mix coefficient
      //write_lpcm_down_mix_coeff(&cl->lpcm_down_mix_coeff);
      return 0;
    }
  }

  if( id1 == 2 ) {
    if( id2 == 4 ) {
      // Extent start point
      return cl->extents.write();
    }
    if( id2 == 5 ) {
      // ProgramInfo SS
      return cl->programs_ss.write();
    }
    if( id2 == 6 ) {
      // CPI SS
      return cl->cpi_ss.write();
    }
  }

  fprintf(stderr, "write_clpi_extension(): unhandled extension %d.%d\n", id1, id2);
  return 1;
}

int
clpi_cl::write()
{
  if( write_header() ) return 1;
  if( clip.write() ) return 1;
  sequence_info_start_addr = bs.posb();
  if( sequences.write() ) return 1;
  program_info_start_addr = bs.posb();
  if( programs.write() ) return 1;
  cpi_start_addr = bs.posb();
  if( cpi.write() ) return 1;
  clip_mark_start_addr = bs.posb();
  if( cmrk.write() ) return 1;
//if( has_ext_data ) {
//  ext_data_start_addr = bs.pos();
//  bdmv_write_extension_data(write_clpi_extension, this);
//}
  return 0;
}

int
bd_uo_mask::write()
{
  bs.write(menu_call, 1);
  bs.write(title_search, 1);
  bs.write(chapter_search, 1);
  bs.write(time_search, 1);
  bs.write(skip_to_next_point, 1);
  bs.write(skip_to_prev_point, 1);
  bs.write(play_firstplay, 1);
  bs.write(stop, 1);
  bs.write(pause_on, 1);
  bs.write(pause_off, 1);
  bs.write(still_off, 1);
  bs.write(forward, 1);
  bs.write(backward, 1);
  bs.write(resume, 1);
  bs.write(move_up, 1);
  bs.write(move_down, 1);
  bs.write(move_left, 1);
  bs.write(move_right, 1);
  bs.write(select, 1);
  bs.write(activate, 1);
  bs.write(select_and_activate, 1);
  bs.write(primary_audio_change, 1);
  bs.pad(1);
  bs.write(angle_change, 1);
  bs.write(popup_on, 1);
  bs.write(popup_off, 1);
  bs.write(pg_enable_disable, 1);
  bs.write(pg_change, 1);
  bs.write(secondary_video_enable_disable, 1);
  bs.write(secondary_video_change, 1);
  bs.write(secondary_audio_enable_disable, 1);
  bs.write(secondary_audio_change, 1);
  bs.pad(1);
  bs.write(pip_pg_change, 1);
  bs.pad(30);
  return 0;
}

int
mpls_ai::write()
{
  bs_len(bs, 32);
  bs.pad(8);              // Reserved
  bs.write(playback_type, 8);
  if (playback_type == BLURAY_PLAYBACK_TYPE_RANDOM ||
      playback_type == BLURAY_PLAYBACK_TYPE_SHUFFLE ) {
    bs.write(playback_count, 16);
  }
  else {
    bs.pad(16);           // Reserved
  }
  uo_mask.write();
  bs.write(random_access_flag, 1);
  bs.write(audio_mix_flag, 1);
  bs.write(lossless_bypass_flag, 1);
  bs.pad(13);             // Reserved
  bs_end(bs);
  return 0;
}

int
mpls_pl::write_header()
{
  bs.writeb("MPLS", 4);
  bs.writeb(sig == 1 ? "0100" : "0200", 4);
  bs.write(list_pos, 32);
  bs.write(mark_pos, 32);
  bs.write(ext_pos, 32);
  bs.pad(160);            // Skip 160 reserved bits
  app_info.write();
  return 0;
}

int mpls_stream::
write()
{
  strm.bs_len(bs, 8);

  bs.write(stream_type, 8);
  switch (stream_type) {
  case 0x01:
    bs.write(pid, 16);
    break;

  case 0x02:
  case 0x04:
    bs.write(subpath_id, 8);
    bs.write(subclip_id, 8);
    bs.write(pid, 16);
    break;

  case 0x03:
    bs.write(subpath_id, 8);
    bs.write(pid, 16);
    break;

  default:
    fprintf(stderr, "unrecognized stream type %02x\n", stream_type);
    break;
  };
  bs.padb(9 - strm.bs_posb(bs));
  strm.bs_end(bs);

  code.bs_len(bs, 8);
  bs.write(coding_type, 8);
  switch (coding_type) {
  case BLURAY_STREAM_TYPE_VIDEO_MPEG1:
  case BLURAY_STREAM_TYPE_VIDEO_MPEG2:
  case BLURAY_STREAM_TYPE_VIDEO_VC1:
  case BLURAY_STREAM_TYPE_VIDEO_H264:
    bs.write(format, 4);
    bs.write(rate, 4);
    break;

  case BLURAY_STREAM_TYPE_AUDIO_MPEG1:
  case BLURAY_STREAM_TYPE_AUDIO_MPEG2:
  case BLURAY_STREAM_TYPE_AUDIO_LPCM:
  case BLURAY_STREAM_TYPE_AUDIO_AC3:
  case BLURAY_STREAM_TYPE_AUDIO_DTS:
  case BLURAY_STREAM_TYPE_AUDIO_TRUHD:
  case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS:
  case BLURAY_STREAM_TYPE_AUDIO_DTSHD:
  case BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER:
  case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS_SECONDARY:
  case BLURAY_STREAM_TYPE_AUDIO_DTSHD_SECONDARY:
    bs.write(format, 4);
    bs.write(rate, 4);
    bs.writeb(lang, 3);
    break;

  case BLURAY_STREAM_TYPE_SUB_PG:
  case BLURAY_STREAM_TYPE_SUB_IG:
    bs.writeb(lang, 3);
    break;

  case BLURAY_STREAM_TYPE_SUB_TEXT:
    bs.write(char_code, 8);
    bs.writeb(lang, 3);
    break;

  default:
    fprintf(stderr, "mpls_stream: unrecognized coding type %02x\n", coding_type);
    break;
  };
  bs.padb(5 - code.bs_posb(bs));
  code.bs_end(bs);
  return 0;
}

int
mpls_stn::write()
{
  bs_len(bs, 16);
  bs.pad(16);             // Skip 2 reserved bytes

  bs.write(video.size(), 8);
  bs.write(audio.size(), 8);
  bs.write(pg.size() - num_pip_pg, 8);
  bs.write(ig.size(), 8);
  bs.write(secondary_audio.size(), 8);
  bs.write(secondary_video.size(), 8);
  bs.write(num_pip_pg, 8);

  // 5 reserve bytes
  bs.pad(5 * 8);

  // Primary Video Streams
  for( int ii=0; ii < video.size(); ++ii )
    if( video[ii]->write() ) return 1;

  // Primary Audio Streams
  for( int ii=0; ii < audio.size(); ++ii )
    if( audio[ii]->write() ) return 1;

  // Presentation Graphic Streams
  for( int ii=0; ii < pg.size(); ++ii )
    if( pg[ii]->write() ) return 1;

  // Interactive Graphic Streams
  for( int ii=0; ii < ig.size(); ++ii )
    if( ig[ii]->write() ) return 1;

  // Secondary Audio Streams
  for( int ii=0; ii < secondary_audio.size(); ++ii ) {
    if( secondary_audio[ii]->write() ) return 1;
    // Read Secondary Audio Extra Attributes
    bs.write(sa_primary_audio_ref.size(), 8);
    bs.pad(8);
    for( int jj=0; jj < sa_primary_audio_ref.size(); ++jj )
      bs.write(sa_primary_audio_ref[jj], 8);
    if( sa_primary_audio_ref.size() % 2) bs.pad(8 );
  }

  // Secondary Video Streams
  for( int ii=0; ii < secondary_video.size(); ++ii ) {
    if( secondary_video[ii]->write() ) return 1;
    // Read Secondary Video Extra Attributes
    bs.write(sv_secondary_audio_ref.size(), 8);
    bs.pad(8);
    for( int jj=0; jj < sv_secondary_audio_ref.size(); ++jj )
      bs.write(sv_secondary_audio_ref[jj], 8);
    if( sv_secondary_audio_ref.size() % 2) bs.pad(8 );
    bs.write(sv_pip_pg_ref.size(), 8);
    bs.pad(8);
    for( int jj=0; jj < sv_pip_pg_ref.size(); ++jj )
      bs.write(sv_pip_pg_ref[jj], 8);
    if( sv_pip_pg_ref.size() % 2) bs.pad(8 );
  }

  bs_end(bs);
  return 0;
}

int
mpls_pi::write()
{
  bs_len(bs, 16);
  // Primary Clip identifer
  bs.writeb(clip[0]->clip_id, 5);
  bs.writeb(clip[0]->codec_id, 4); // "M2TS"
  bs.pad(11); // Skip reserved 11 bits

  bs.write(is_multi_angle, 1);
  bs.write(connection_condition, 4);  // 0x01, 0x05, 0x06

  bs.write(clip[0]->stc_id, 8);
  bs.write(in_time, 32);
  bs.write(out_time, 32);

  uo_mask.write();
  bs.write(random_access_flag, 1);
  bs.pad(7);
  bs.write(still_mode, 8);
  if( still_mode == 0x01 ) {
    bs.write(still_time, 16);
  }
  else {
    bs.pad(16);
  }

  if( is_multi_angle ) {
    bs.write(clip.size(), 8);
    bs.pad(6);
    bs.write(is_different_audio, 1);
    bs.write(is_seamless_angle, 1);
  }

  for( int ii=1; ii < clip.size(); ++ii ) {
    bs.writeb(clip[ii]->clip_id, 5);
    bs.writeb(clip[ii]->codec_id, 4); // "M2TS"
    bs.write(clip[ii]->stc_id, 8);
  }

  if( stn.write() ) return 1;

  bs_end(bs);
  return 0;
}

int
mpls_sub_pi::write()
{
  bs_len(bs, 16);
  // Primary Clip identifer
  bs.writeb(clip[0]->clip_id, 5);
  bs.writeb(clip[0]->codec_id, 4); // "M2TS"
  bs.pad(27);

  bs.write(connection_condition, 4); // 0x01, 0x05, 0x06

  bs.write(is_multi_clip, 1);
  bs.write(clip[0]->stc_id, 8);
  bs.write(in_time, 32);
  bs.write(out_time, 32);
  bs.write(sync_play_item_id, 16);
  bs.write(sync_pts, 32);

  if( is_multi_clip )
    bs.write(clip.size(), 8);

  for( int ii=1; ii < clip.size(); ++ii ) {
    // Primary Clip identifer
    bs.writeb(clip[ii]->clip_id, 5);
    bs.writeb(clip[ii]->codec_id, 4); // "M2TS"
    bs.write(clip[ii]->stc_id, 8);
  }

  bs_end(bs);
  return 0;
}

int
mpls_sub::write()
{
  bs_len(bs, 32);
  bs.pad(8);
  bs.write(type, 8);
  bs.pad(15);
  bs.write(is_repeat, 1);
  bs.pad(8);
  bs.write(sub_play_item.size(), 8);

  for( int ii=0; ii < sub_play_item.size(); ++ii )
    if( sub_play_item[ii]->write() ) return 1;

  bs_end(bs);
  return 0;
}

int
mpls_plm::write()
{
  bs.write(mark_id, 8);
  bs.write(mark_type, 8);
  bs.write(play_item_ref, 16);
  bs.write(time, 32);
  bs.write(entry_es_pid, 16);
  bs.write(duration, 32);
  return 0;
}

int
mpls_pl::write_playlistmark()
{
  mark.bs_len(bs, 32);
  // Then get the number of marks
  bs.write(play_mark.size(), 16);

  for( int ii=0; ii < play_mark.size(); ++ii )
    if( play_mark[ii]->write() ) return 1;

  mark.bs_end(bs);
  return 0;
}

int
mpls_pl::write_playlist()
{
  play.bs_len(bs,32);

  // Skip reserved bytes
  bs.pad(16);

  bs.write(play_item.size(), 16);
  bs.write(sub_path.size(), 16);

  for( int ii=0; ii < play_item.size(); ++ii )
    if( play_item[ii]->write() ) return 1;

  for( int ii=0; ii < sub_path.size(); ++ii )
    if( sub_path[ii]->write() ) return 1;

  play.bs_end(bs);
  return 0;
}

int
mpls_pip_data::write()
{
  bs.write(time, 32);
  bs.write(xpos, 12);
  bs.write(ypos, 12);
  bs.write(scale_factor, 4);
  bs.pad(4);
  return 0;
}

int
mpls_pip_metadata::write(uint32_t start_address)
{

  bs.write(clip_ref, 16);
  bs.write(secondary_video_ref, 8);
  bs.pad(8);
  bs.write(timeline_type, 4);
  bs.write(luma_key_flag, 1);
  bs.write(trick_play_flag, 1);
  bs.pad(10);
  if( luma_key_flag ) {
    bs.pad(8);
    bs.write(upper_limit_luma_key, 8);
  }
  else {
    bs.pad(16);
  }
  bs.pad(16);

  uint32_t data_address = 0;  // XXX
  bs.write(data_address, 32);

  int64_t pos = bs.pos() / 8;
  bs.posb(start_address + data_address);

  bs.write(data.size(), 16);
  if( data.size() < 1 ) return 1;

  for( int ii=0; ii < data.size(); ++ii )
    if( data[ii]->write() ) return 1;

  bs.posb(pos);
  return 0;
}

int
mpls_pl::write_pip_metadata_extension()
{
  uint32_t pos = bs.posb();
  pipm.bs_len(bs, 32);

  bs.write(ext_pip_data.size(), 16);
  for( int ii=0; ii < ext_pip_data.size(); ++ii )
    if( ext_pip_data[ii]->write(pos) ) return 1;

  pipm.bs_end(bs);
  return 0;
}

int
mpls_pl::write_subpath_extension()
{
  subx.bs_len(bs, 32);

  bs.write(ext_sub_path.size(), 16);
  for( int ii=0; ii < ext_sub_path.size(); ++ii )
    if( ext_sub_path[ii]->write() ) return 1;

  subx.bs_end(bs);
  return 0;
}

int
clpi_cl::write_mpls_extension(int id1, int id2, void *handle)
{
  mpls_pl *pl = (mpls_pl *) handle;

  if( id1 == 1 ) {
    if( id2 == 1 ) {
      // PiP metadata extension
      return pl->write_pip_metadata_extension();
    }
  }

  if( id1 == 2 ) {
    if( id2 == 1 ) {
      return 0;
    }
    if( id2 == 2 ) {
      // SubPath entries extension
      return pl->write_subpath_extension();
    }
  }

  return 0;
}

int mpls_pl::
write()
{
  int ret = write_header();
  list_pos = bs.posb();;
  if( !ret ) ret = write_playlist();
  mark_pos = bs.posb();
  if( !ret ) ret = write_playlistmark();
//if( has_pls_extension ) {
//  ext_pos = bs.posb();
//  bdmv_write_extension_data(write_mpls_extension, pl);
  return ret;
}

static int
mk_dir(char *path)
{
  if( !mkdir(path, 0777) )
    return 0;
  perror(path);
  return 1;
}

static int
mk_bdmv_dir(char *bdmv_path)
{
  if( mk_dir(bdmv_path) )
    return 1;
  char bdjo_path[BCTEXTLEN];
  sprintf(bdjo_path, "%s/BDJO", bdmv_path);
  if( mk_dir(bdjo_path) )
    return 1;
  char clipinf_path[BCTEXTLEN];
  sprintf(clipinf_path, "%s/CLIPINF", bdmv_path);
  if( mk_dir(clipinf_path) )
    return 1;
  char jar_path[BCTEXTLEN];
  sprintf(jar_path, "%s/JAR", bdmv_path);
  if( mk_dir(jar_path) )
    return 1;
  char playlist_path[BCTEXTLEN];
  sprintf(playlist_path, "%s/PLAYLIST", bdmv_path);
  if( mk_dir(playlist_path) )
    return 1;
  return 0;
}

static int
mkbdmv(char *path)
{
  char bdmv_path[BCTEXTLEN];
  sprintf(bdmv_path, "%s/BDMV", path);
  if( mk_bdmv_dir(bdmv_path) ) return 1;
  char cert_path[BCTEXTLEN];
  sprintf(cert_path, "%s/CERTIFICATE", path);
  if( mk_bdmv_dir(cert_path) ) return 1;
  char cert_backup[BCTEXTLEN];
  sprintf(cert_backup, "%s/BACKUP", cert_path);
  if( mk_bdmv_dir(cert_backup) ) return 1;
  char stream_path[BCTEXTLEN];
  sprintf(stream_path, "%s/STREAM", bdmv_path);
  if( mk_dir(stream_path) ) return 1;
  char auxdata_path[BCTEXTLEN];
  sprintf(auxdata_path, "%s/AUXDATA", bdmv_path);
  if( mk_dir(auxdata_path) ) return 1;
  char meta_path[BCTEXTLEN];
  sprintf(meta_path, "%s/META", bdmv_path);
  if( mk_dir(meta_path) ) return 1;
  char backup_path[BCTEXTLEN];
  sprintf(backup_path, "%s/BACKUP", bdmv_path);
  if( mk_bdmv_dir(backup_path) ) return 1;
  return 0;
}

int program::
build_toc(clpi_ep_map_entry *map)
{
  clpi_ep_coarse *cp = 0;
  marks.sort(mark::cmpr);
  uint16_t ep_pid = map->pid;
  int64_t last_pts = -1, last_pkt = -1;
  for( int ii=0; ii<marks.size(); ++ii ) {
    mark *mp = marks[ii];
    if( mp->pid != ep_pid ) continue;
    int64_t pts = mp->pts;
    if( last_pts >= pts ) continue;
    last_pts = pts;
    uint32_t pkt = mp->pos / BLURAY_TS_PKTSZ;
    if( last_pkt >= pkt ) continue;
    last_pkt = pkt;
    int64_t coarse_pts = (pts >> 18); // & ~0x01;
    int64_t fine_pts = (pts & 0x7ffff) >> 8;
    uint32_t mpkt = pkt & ~0x1ffff;
    if( !cp || cp->pts_ep != coarse_pts || mpkt > cp->spn_ep ) {
      cp = new clpi_ep_coarse();
      map->coarse.append(cp);
      cp->ref_ep_fine_id = map->fine.size();
      cp->pts_ep = coarse_pts;
      cp->spn_ep = pkt;
    }
    clpi_ep_fine *fp = new clpi_ep_fine();
    map->fine.append(fp);
    fp->is_angle_change_point = 0;
// XXX - dont know what this is
    fp->i_end_position_offset = 1;
    fp->pts_ep = fine_pts;
    fp->spn_ep = pkt & 0x1ffff;
  }
  return 0;
}

const AVRational media_info::clk45k = { 1, 45000 };

static int bd_stream_type(AVCodecID codec_id)
{
  int stream_type = 0;
  switch (codec_id) {
  case AV_CODEC_ID_MPEG1VIDEO:
    stream_type = BLURAY_STREAM_TYPE_VIDEO_MPEG1;
    break;
  case AV_CODEC_ID_MPEG2VIDEO:
    stream_type = BLURAY_STREAM_TYPE_VIDEO_MPEG2;
    break;
  case AV_CODEC_ID_H264:
    stream_type = BLURAY_STREAM_TYPE_VIDEO_H264;
    break;
  case AV_CODEC_ID_MP2:
    stream_type = BLURAY_STREAM_TYPE_AUDIO_MPEG1;
    break;
  case AV_CODEC_ID_MP3:
    stream_type = BLURAY_STREAM_TYPE_AUDIO_MPEG2;
    break;
  case AV_CODEC_ID_AC3:
    stream_type = BLURAY_STREAM_TYPE_AUDIO_AC3;
    break;
  case AV_CODEC_ID_EAC3:
    stream_type = BLURAY_STREAM_TYPE_AUDIO_AC3PLUS;
    break;
  case AV_CODEC_ID_DTS:
    stream_type = BLURAY_STREAM_TYPE_AUDIO_DTS;
    break;
  case AV_CODEC_ID_TRUEHD:
    stream_type = BLURAY_STREAM_TYPE_AUDIO_TRUHD;
    break;
  case AV_CODEC_ID_HDMV_PGS_SUBTITLE:
    stream_type = BLURAY_STREAM_TYPE_SUB_PG;
    break;
  default:
    fprintf(stderr, "unknown bluray stream type %s\n", avcodec_get_name(codec_id));
    exit(1);
  }
  return stream_type;
}

static int bd_audio_format(int channels)
{
  int audio_format = 0;
  switch( channels ) {
  case 1:
    audio_format = BLURAY_AUDIO_FORMAT_MONO;
    break;
  case 2:
    audio_format = BLURAY_AUDIO_FORMAT_STEREO;
    break;
  case 6:
    audio_format = BLURAY_AUDIO_FORMAT_MULTI_CHAN;
    break;
  default:
    fprintf(stderr, "unknown bluray audio format %d ch\n", channels);
    exit(1);
  }
  return audio_format;
}

static int bd_audio_rate(int rate)
{
  int audio_rate = 0;
  switch( rate ) {
  case 48000:  audio_rate = BLURAY_AUDIO_RATE_48;  break;
  case 96000:  audio_rate = BLURAY_AUDIO_RATE_96;  break;
  case 192000: audio_rate = BLURAY_AUDIO_RATE_192; break;
  default:
    fprintf(stderr, "unknown bluray audio rate %d\n", rate);
    exit(1);
  }
  return audio_rate;
}

static int bd_video_format(int w, int h, int ilace)
{
  if( w ==  720 && h ==  480    &&  ilace   ) return BLURAY_VIDEO_FORMAT_480I;
  if( w ==  720 && h ==  576    &&  ilace   ) return BLURAY_VIDEO_FORMAT_576I;
  if( w ==  720 && h ==  480    && !ilace   ) return BLURAY_VIDEO_FORMAT_480P;
  if( w ==  720 && h ==  576    && !ilace   ) return BLURAY_VIDEO_FORMAT_576P;
// this seems to be overly restrictive
  if( w == 1280 && h ==  720 /* && !ilace*/ ) return BLURAY_VIDEO_FORMAT_720P;
  if( w == 1440 && h == 1080 /* &&  ilace*/ ) return BLURAY_VIDEO_FORMAT_1080I;
  if( w == 1920 && h == 1080 /* && !ilace*/ ) return BLURAY_VIDEO_FORMAT_1080P;
  fprintf(stderr, "unknown bluray video format %dx%d %silace\n",
    w, h, !ilace ? "not " : "");
  exit(1);
}

static int bd_video_rate(double rate)
{
  if( fabs(rate-23.976) < 0.01 ) return BLURAY_VIDEO_RATE_24000_1001;
  if( fabs(rate-24.000) < 0.01 ) return BLURAY_VIDEO_RATE_24;
  if( fabs(rate-25.000) < 0.01 ) return BLURAY_VIDEO_RATE_25;
  if( fabs(rate-29.970) < 0.01 ) return BLURAY_VIDEO_RATE_30000_1001;
  if( fabs(rate-50.000) < 0.01 ) return BLURAY_VIDEO_RATE_50;
  if( fabs(rate-59.940) < 0.01 ) return BLURAY_VIDEO_RATE_60000_1001;
  fprintf(stderr, "unknown bluray video framerate %5.2f\n",rate);
  exit(1);
}

static int bd_aspect_ratio(int w, int h, double ratio)
{
  double aspect = (w * ratio) / h;
  if( fabs(aspect-1.333) < 0.01 ) return BLURAY_ASPECT_RATIO_4_3;
  if( fabs(aspect-1.777) < 0.01 ) return BLURAY_ASPECT_RATIO_16_9;
  return w == 720 ? BLURAY_ASPECT_RATIO_4_3 : BLURAY_ASPECT_RATIO_16_9;
  fprintf(stderr, "unknown bluray aspect ratio %5.3f\n",aspect);
  exit(1);
}

static int field_probe(AVFormatContext *fmt_ctx, AVStream *st)
{
  AVDictionary *copts = 0;
  //av_dict_copy(&copts, opts, 0);
  AVCodecID codec_id = st->codecpar->codec_id;
  AVCodec *decoder = avcodec_find_decoder(codec_id);
  AVCodecContext *ctx = avcodec_alloc_context3(decoder);
  if( !ctx ) {
    fprintf(stderr,"codec alloc failed\n");
    return -1;
  }
  avcodec_parameters_to_context(ctx, st->codecpar);
  if( avcodec_open2(ctx, decoder, &copts) < 0 ) {
    fprintf(stderr,"codec open failed\n");
    return -1;
  }
  av_dict_free(&copts);

  AVFrame *ipic = av_frame_alloc();
  AVPacket ipkt;
  av_init_packet(&ipkt);
  int ilaced = -1;
  for( int retrys=100; --retrys>=0 && ilaced<0; ) {
    av_packet_unref(&ipkt);
    int ret = av_read_frame(fmt_ctx, &ipkt);
    if( ret == AVERROR_EOF ) break;
    if( ret != 0 ) continue;
    if( ipkt.stream_index != st->index ) continue;
    if( !ipkt.data || !ipkt.size ) continue;
    ret = avcodec_send_packet(ctx, &ipkt);
    if( ret < 0 ) {
      fprintf(stderr, "avcodec_send_packet failed\n");
      break;
    }
    ret = avcodec_receive_frame(ctx, ipic);
    if( ret >= 0 ) {
      ilaced = ipic->interlaced_frame ? 1 : 0;
      break;
    }
    if( ret != AVERROR(EAGAIN) )
      fprintf(stderr, "avcodec_receive_frame failed %d\n", ret);
  }
  av_packet_unref(&ipkt);
  av_frame_free(&ipic);
  avcodec_free_context(&ctx);
  return ilaced;
}

int media_info::scan()
{
  struct stat st;
  if( stat(filename, &st) ) return 1;
  file_size = st.st_size;

  AVFormatContext *fmt_ctx = 0;
  AVDictionary *fopts = 0;
  av_dict_set(&fopts, "formatprobesize", "5000000", 0);
  av_dict_set(&fopts, "scan_all_pmts", "1", 0);
  av_dict_set(&fopts, "threads", "auto", 0);
  int ret = avformat_open_input(&fmt_ctx, filename, NULL, &fopts);
  av_dict_free(&fopts);
  if( ret < 0 ) return ret;
  ret = avformat_find_stream_info(fmt_ctx, NULL);

  bit_rate = fmt_ctx->bit_rate;

  int ep_pid = -1;
  for( int i=0; ret>=0 && i<(int)fmt_ctx->nb_streams; ++i ) {
    AVStream *st = fmt_ctx->streams[i];
    AVMediaType type = st->codecpar->codec_type;
    switch( type ) {
    case AVMEDIA_TYPE_VIDEO: break;
    case AVMEDIA_TYPE_AUDIO: break;
    case AVMEDIA_TYPE_SUBTITLE: break;
    default: continue;
    }
    stream *s = new stream(type, i);
    s->pid = st->id;
    AVCodecID codec_id = st->codecpar->codec_id;
    AVCodec *decoder = avcodec_find_decoder(codec_id);
    s->ctx = avcodec_alloc_context3(decoder);
    if( !s->ctx ) {
      fprintf(stderr, "avcodec_alloc_context failed\n");
      continue;
    }
    switch( type ) {
    case AVMEDIA_TYPE_VIDEO: {
      if( ep_pid < 0 ) ep_pid = st->id;
      s->coding_type = bd_stream_type(codec_id);
      int ilace = field_probe(fmt_ctx, st);
      if( ilace < 0 ) {
        fprintf(stderr, "interlace probe failed\n");
        exit(1);
      }
      s->format = bd_video_format(st->codecpar->width, st->codecpar->height, ilace);
      AVRational framerate = av_guess_frame_rate(fmt_ctx, st, 0);
      s->rate = bd_video_rate(!framerate.den ? 0 : (double)framerate.num / framerate.den);
      s->aspect = bd_aspect_ratio(st->codecpar->width, st->codecpar->height,
		!st->sample_aspect_ratio.num || !st->sample_aspect_ratio.den ? 1. :
		 (double)st->sample_aspect_ratio.num / st->sample_aspect_ratio.den);
      break; }
    case AVMEDIA_TYPE_AUDIO: {
      s->coding_type = bd_stream_type(codec_id);
      s->format = bd_audio_format(st->codecpar->channels);
      s->rate = bd_audio_rate(st->codecpar->sample_rate);
      strcpy((char*)s->lang, "eng");
      break; }
    case AVMEDIA_TYPE_SUBTITLE: {
      s->coding_type = bd_stream_type(codec_id);
      AVDictionaryEntry *lang = av_dict_get(st->metadata, "language", 0, 0);
      strncpy((char*)s->lang, lang ? lang->value : "und", sizeof(s->lang));
      break; }
    default:
      break;
    }
    if( bit_rate > 0 && st->duration == AV_NOPTS_VALUE &&
        st->time_base.num < INT64_MAX / bit_rate ) {
      st->duration = av_rescale(8*file_size, st->time_base.den,
          bit_rate * (int64_t) st->time_base.num);
    }
    s->duration = av_rescale_q(st->duration, st->time_base, clk45k);
    streams.append(s);

    AVDictionary *copts = 0;
    ret = avcodec_open2(s->ctx, decoder, &copts);
  }
  if( ep_pid < 0 )
    ep_pid = fmt_ctx->nb_streams > 0 ? fmt_ctx->streams[0]->id : 0;

  int npgm = fmt_ctx->nb_programs;
  if( npgm < 1 ) {
    program *pgm = new program(-1, 1);
    pgm->ep_pid = ep_pid;
    pgm->pmt_pid = 0x1000;
    pgm->pcr_pid = 0x1001;
    pgm->duration = 0;
    for( int jj=0; jj<streams.size(); ++jj ) {
      AVStream *st = fmt_ctx->streams[jj];
      AVMediaType type = st->codecpar->codec_type;
      switch( type ) {
      case AVMEDIA_TYPE_VIDEO:
      case AVMEDIA_TYPE_AUDIO:
        break;
      default:
        continue;
      }
      pgm->strm_idx.append(jj);
      if( !pgm->duration || st->duration < pgm->duration )
        pgm->duration = av_rescale_q(st->duration, st->time_base, clk45k);
    }
    programs.append(pgm);
  }

  for( int ii=0; ii<npgm; ++ii ) {
    AVProgram *pgrm = fmt_ctx->programs[ii];
    program *pgm = new program(ii, pgrm->id);
    pgm->pmt_pid = pgrm->pmt_pid;
    pgm->pcr_pid = pgrm->pcr_pid;
    pgm->duration = 0;
    ep_pid = -1;
    for( int jj=0; jj<(int)pgrm->nb_stream_indexes; ++jj ) {
      int av_idx = pgrm->stream_index[jj];
      AVStream *st = fmt_ctx->streams[av_idx];
      AVMediaType type = st->codecpar->codec_type;
      switch( type ) {
      case AVMEDIA_TYPE_VIDEO:
        if( ep_pid < 0 ) ep_pid = st->id;
        break;
      case AVMEDIA_TYPE_AUDIO:
      case AVMEDIA_TYPE_SUBTITLE:
        break;
      default:
        continue;
      }
      int sidx = streams.size();
      while( --sidx>=0 && streams[sidx]->av_idx!=av_idx );
      if( sidx < 0 ) {
        fprintf(stderr, "bad stream idx %d in pgm %d\n",av_idx, ii);
        continue;
      }
      if( !pgm->duration || st->duration < pgm->duration )
        pgm->duration = av_rescale_q(st->duration, st->time_base, clk45k);
      pgm->strm_idx.append(sidx);
    }
    if( ep_pid < 0 ) {
      AVProgram *pgrm = fmt_ctx->programs[0];
      ep_pid = pgrm->nb_stream_indexes > 0 ?
          fmt_ctx->streams[pgrm->stream_index[0]]->id : 0;
    }
    pgm->ep_pid = ep_pid;
    programs.append(pgm);
  }

  if( ret >= 0 )
    ret = scan(fmt_ctx);

  for( int i=0; i<(int)streams.size(); ++i )
    avcodec_close(streams[i]->ctx);
  avformat_close_input(&fmt_ctx);

  return ret;
}

int media_info::scan(AVFormatContext *fmt_ctx)
{
  int ret = 0;
  AVPacket ipkt;
  av_init_packet(&ipkt);
#if 1
// zero pts at pos zero
  for( int i=0; i<programs.size(); ++i ) {
    program *p = programs[i];
    for( int j=0; j<p->strm_idx.size(); ++j ) {
      stream *sp = streams[p->strm_idx[j]];
      sp->last_pts = 0;
      AVStream *st = fmt_ctx->streams[sp->av_idx];
      p->add_label(0, 0, 0, st->id);
    }
  }
#endif
  for( int64_t count=0; ret>=0; ++count ) {
    av_packet_unref(&ipkt);
    ipkt.data = 0; ipkt.size = 0;

    ret = av_read_frame(fmt_ctx, &ipkt);
    if( ret < 0 ) {
      if( ret == AVERROR_EOF ) break;
      ret = 0;
      continue;
    }
    if( !ipkt.data ) continue;
    if( (ipkt.flags & AV_PKT_FLAG_CORRUPT) ) continue;
    if( ipkt.pts == AV_NOPTS_VALUE ) continue;
    int i = ipkt.stream_index;
    if( i < 0 || i >= streams.size() ) continue;

    stream *sp = 0;
    program *pgm = 0;
    for( int ii=0; !pgm && ii<programs.size(); ++ii ) {
      program *p = programs[ii];
      for( int jj=0; jj<p->strm_idx.size(); ++jj ) {
        sp = streams[p->strm_idx[jj]];
        if( sp->av_idx == i ) { pgm = p;  break; }
      }
    }
    if( !pgm ) continue;
    AVStream *st = fmt_ctx->streams[i];
    if( pgm->ep_pid != st->id ) continue;
    int64_t pts45k = av_rescale_q(ipkt.pts, st->time_base, clk45k);
    if( sp->start_pts > pts45k ) sp->start_pts = pts45k;
    if( sp->end_pts < pts45k ) sp->end_pts = pts45k;
    if( pgm->start_time > pts45k ) pgm->start_time = pts45k;
    if( pgm->end_time < pts45k ) pgm->end_time = pts45k;

    if( !(ipkt.flags & AV_PKT_FLAG_KEY) ) continue;

    if( sp->last_pts != pts45k ) {
      sp->last_pts = pts45k;
      pgm->add_label(count, ipkt.pos, pts45k, st->id);
    }
  }

  for( int ii=0; ii<programs.size(); ++ii ) {
    program *pgm = programs[ii];
    if( pgm->end_time > pgm->start_time )
      pgm->duration = pgm->end_time - pgm->start_time;
  }

  return ret != AVERROR_EOF ? -1 : 0;
}

void
Media::add_movie(uint32_t *ops, int n)
{
  movie_obj *mp = new movie_obj();
  mp->resume_intention_flag = 1;
  uint32_t *eop = ops + n/sizeof(*ops);
  while( ops < eop ) {
    command_obj *cmd = new command_obj();
    cmd->cmd = htobe32(*ops++);
    cmd->dst = *ops++;
    cmd->src = *ops++;
    mp->cmds.append(cmd);
  }
  mov.movies.append(mp);
}

int
Media::compose()
{
// movie
  bs.init();

// top menu
  int top_menu_obj = mov.movies.size();
  movie_obj *mp = new movie_obj();
  mp->resume_intention_flag = 1;
  command_obj *cmd = new command_obj();
  cmd->cmd = htobe32(0x21810000); cmd->dst = 1; cmd->src = 0;
  mp->cmds.append(cmd);  // JUMP_TITLE 1
  mov.movies.append(mp);

// titles
  for( int ii=0; ii<size(); ++ii ) {
    mp = new movie_obj();
    mp->resume_intention_flag = 1;
    cmd = new command_obj();
    cmd->cmd = htobe32(0x22800000); cmd->dst = ii; cmd->src = 0;
    mp->cmds.append(cmd);  // PLAY_PL   ii
    cmd = new command_obj();
    cmd->cmd = htobe32(0x00020000); cmd->dst = 0; cmd->src = 0;
    mp->cmds.append(cmd);
    mov.movies.append(mp); // BREAK
  }

// first play
  int first_play_obj = mov.movies.size();
  mp = new movie_obj();
  mp->resume_intention_flag = 1;
  cmd = new command_obj();
  cmd->cmd = htobe32(0x21810000); cmd->dst = 0; cmd->src = 0;
  mp->cmds.append(cmd);  // JUMP_TITLE 0 ; top menu
  mov.movies.append(mp);

// index
  bs.init();
  idx.first_play.set_hdmv(first_play_obj, pb_typ_iactv);
  idx.top_menu.set_hdmv(top_menu_obj, pb_typ_iactv);

  title_obj *tp = 0;
// clips
  for( int ii=0; ii<size(); ++ii ) {
    if( !tp ) {
      tp = new title_obj();
      tp->set_hdmv(idx.titles.size()+1, pb_typ_movie);
      idx.titles.append(tp);
    }
    bs.init();
    media_info *ip = get(ii);
// clip program, if specified
    int pidx = ip->programs.size();
    while( --pidx>=0 && ip->programs[pidx]->pid != ip->pgm_pid );
    if( pidx < 0 ) pidx = 0;
    ip->pidx = pidx;
    ip->pgm_pid = ip->prog()->pid;

    clpi_cl *cp = new clpi_cl();
    cp->clip.clip_stream_type = 1;
    cp->clip.application_type = BLURAY_APP_TYPE_MAIN_MOVIE;
    cp->clip.ts_recording_rate = ip->bit_rate;
    uint32_t ts_pkt_count = ip->file_size / BLURAY_TS_PKTSZ;
    cp->clip.num_source_packets = ts_pkt_count;
    cp->clip.ts_type_info.validity = 0x80;
    strcpy(cp->clip.ts_type_info.format_id, "HDMV");
    cp->cpi.type = 1;

    for( int jj=0; jj<ip->programs.size(); ++jj ) {
      program *pgm = ip->programs[jj];
      clpi_prog *p = new clpi_prog(pgm->pmt_pid);
      for( int kk=0; kk<pgm->strm_idx.size(); ++kk ) {
        int k = pgm->strm_idx[kk];
        stream *sp = ip->streams[k];
        clpi_prog_stream *s = new clpi_prog_stream();
        s->pid = sp->pid;
        s->coding_type = sp->coding_type;
        s->format = sp->format;
//use unspecified (0)
//      if( !idx.video_format ) idx.video_format = s->format;
        s->rate = sp->rate;
//      if( !idx.frame_rate ) idx.frame_rate = s->rate;
        switch( sp->type ) {
        case AVMEDIA_TYPE_VIDEO:
          s->aspect = sp->aspect;
          break;
        case AVMEDIA_TYPE_AUDIO:
        case AVMEDIA_TYPE_SUBTITLE:
          memcpy(s->lang,sp->lang,sizeof(s->lang));
          break;
        default:
          break;
        }
        p->streams.append(s);
      }
      clpi_ep_map_entry *map = new clpi_ep_map_entry(pgm->ep_pid);
      map->ep_stream_type = 1;
      pgm->build_toc(map);
      cp->cpi.append(map);
      cp->programs.append(p);

      clpi_atc_seq *atc_seq = new clpi_atc_seq();
      clpi_stc_seq *stc_seq = new clpi_stc_seq();
      stc_seq->pcr_pid = pgm->pcr_pid;
      stc_seq->presentation_start_time = pgm->start_time;
      stc_seq->presentation_end_time = pgm->end_time;
      atc_seq->stc_seq.append(stc_seq);
      cp->sequences.append(atc_seq);
    }

    cl.append(cp);
    tp->last = ii;
    if( ip->brk ) tp = 0;
  }

// playlists, one per title
//   one playitem per media clip
  int clip_id = 0;
  for( int ii=0; ii<idx.titles.size(); ++ii ) {
    bs.init();
    media_info *ip = get(clip_id);
    program *pgm = ip->prog();
    mpls_pl *pp = new mpls_pl();
    pp->app_info.playback_type = BLURAY_PLAYBACK_TYPE_SEQUENTIAL;
//  pp->app_info.uo_mask.xxx = 1;
    int last = idx.titles[ii]->last;
    for( ; clip_id<=last; ++clip_id ) {
      ip = get(clip_id);
      pgm = ip->prog();
      mpls_pi *pi = new mpls_pi();
      pi->connection_condition = 1; // seamless
//    pi->uo_mask.xxx = 1;
      pi->in_time = pgm->start_time;
      pi->out_time = pgm->end_time;
      if( ip->still )
        pi->still_mode = BLURAY_STILL_INFINITE;
      int64_t end_time = pgm->start_time + pgm->duration;
      if( pi->out_time < end_time ) pi->out_time = end_time;
      mpls_clip *cp = new mpls_clip();
      sprintf(cp->clip_id,"%05d", clip_id);
      pi->clip.append(cp);
      for( int kk=0; kk<ip->streams.size(); ++kk ) {
        stream *sp = ip->streams[kk];
        switch( sp->type ) {
        case AVMEDIA_TYPE_VIDEO: break;
        case AVMEDIA_TYPE_AUDIO: break;
        case AVMEDIA_TYPE_SUBTITLE: break;
        default: continue;
        }
        mpls_stream *ps = new mpls_stream();
        ps->pid = sp->pid;
        ps->stream_type = BLURAY_PG_TEXTST_STREAM;
        ps->coding_type = sp->coding_type;
        ps->format = sp->format;
        ps->rate = sp->rate;
        switch( sp->type ) {
        case AVMEDIA_TYPE_VIDEO:
          pi->stn.video.append(ps);
          break;
        case AVMEDIA_TYPE_AUDIO:
          memcpy(ps->lang, sp->lang, sizeof(ps->lang));
          pi->stn.audio.append(ps);
          break;
        case AVMEDIA_TYPE_SUBTITLE:
          memcpy(ps->lang, sp->lang, sizeof(ps->lang));
          pi->stn.pg.append(ps);
          break;
        default:
          break;
        }
      }
      pp->play_item.append(pi);
    }
// chapter marks every ch_duration ticks
    int64_t ch_duration = 45000 * 60*5;
    int64_t mrktm = ch_duration;
    int64_t plytm = 0;
    int pmark = 0, pitem = 0;
    mpls_pi *pi = pp->play_item[pitem];
    mpls_plm *pm = new mpls_plm();
    pm->mark_id = pmark++;
    pm->mark_type = BLURAY_MARK_TYPE_ENTRY;
    pm->play_item_ref = pitem;
    pm->time = pi->in_time;
    pm->entry_es_pid = 0;
    pp->play_mark.append(pm);
    for( int jj=0; jj < pp->play_item.size(); ++jj ) {
      pitem = jj;
      pi = pp->play_item[pitem];
      int64_t pi_duration = pi->out_time - pi->in_time;
      int64_t endtm = plytm + pi_duration;
      while( mrktm < endtm ) {
        pm = new mpls_plm();
        pm->mark_id = pmark++;
        pm->mark_type = BLURAY_MARK_TYPE_ENTRY;
        pm->play_item_ref = pitem;
        pm->time = pi->in_time + mrktm - plytm;
        pm->entry_es_pid = 0;
        pp->play_mark.append(pm);
        mrktm += ch_duration;
      }
      plytm = endtm;
    }
    pm = new mpls_plm();
    pm->mark_id = pmark;
    pm->mark_type = BLURAY_MARK_TYPE_ENTRY;
    pm->play_item_ref = pitem;
    pm->time = pi->out_time;
    pm->entry_es_pid = 0;
    pp->play_mark.append(pm);

    pl.append(pp);
  }
  return 0;
}

int Media::
bd_path(const char *bp, const char *fmt, va_list ap)
{
  int n = sizeof(filename)-1;
  char *cp = filename;
  const char *pp = path;
  while( n>0 && (*cp=*pp)!=0 ) { --n;  ++cp;  ++pp; }
  while( n>0 && (*cp=*bp)!=0 ) { --n;  ++cp;  ++bp; }
  n -= vsnprintf(cp, n, fmt, ap);
  va_end(ap);
  return n > 0 ? 0 : 1;
}

int Media::
bd_copy(const char *ifn, const char *fmt, ...)
{
  int bfrsz = 0x40000, ret = 1;
  char bfr[bfrsz];
  FILE *ifp = fopen(ifn,"r");
  if( ifp ) {
    va_list ap;
    va_start(ap, fmt);
    if( bd_path("/BDMV/", fmt, ap) ) return 1;
    va_end(ap);
    FILE *ofp = fopen(filename,"w");
    if( ofp ) {
      setvbuf(ifp, 0, _IOFBF, 0x80000);
      setvbuf(ofp, 0, _IOFBF, 0x80000);
      ret = 0;
      int n = bfrsz;
      while( !ret && n >= bfrsz ) {
	n = fread(bfr,1,bfrsz,ifp);
        if( n > 0 && (int)fwrite(bfr,1,n,ofp) != n ) {
          fprintf(stderr, "cant write: %s\n",filename);
          ret = 1;
	}
      }
      if( ferror(ifp) ) {
        fprintf(stderr, "read error: %s = %m\n",ifn);
        ret = 1;
      }
      if( ferror(ofp) ) {
        fprintf(stderr, "write error: %s = %m\n",filename);
        ret = 1;
      }
      if( fclose(ofp) ) {
        fprintf(stderr, "close error: %s = %m\n",filename);
        ret = 1;
      }
    }
    fclose(ifp);
  }
  if( ret )
    fprintf(stderr, "cant copy clip %s\n",ifn);
  return ret;
}

int Media::
bd_open(const char *fmt, ...)
{
  bs.init();
  if( !path ) return 0;
  va_list ap;
  va_start(ap, fmt);
  if( bd_path("/BDMV/", fmt, ap) ) return 1;
  va_end(ap);
  if( bs.open(filename) ) {
    fprintf(stderr, "cant open file %s\n",filename);
    return 1;
  }
  return 0;
}

int Media::
bd_backup(const char *fmt, ...)
{
  bs.close();
  if( !path ) return 0;
  int n, ret = 1;
  char bfr[0x10000];
  FILE *ifp = fopen(filename,"r");
  if( ifp ) {
    va_list ap;
    va_start(ap, fmt);
    if( bd_path("/BDMV/BACKUP/", fmt, ap) ) return 1;
    va_end(ap);
    FILE *ofp = fopen(filename,"w");
    if( ofp ) {
      while( (n=fread(bfr,1,sizeof(bfr),ifp)) > 0 ) fwrite(bfr,1,n,ofp);
      fclose(ofp);
      ret = 0;
    }
    fclose(ifp);
  }
  if( ret )
    fprintf(stderr, "cant backup %s\n",filename);
  return ret;
}

int Media::write(char *fn)
{
  this->path = fn;
// index
  if( bd_open("index.bdmv") ) return 1;
  if( idx.write() ) return 1;
  if( bd_backup("index.bdmv") ) return 1;
// movie
  if( bd_open("MovieObject.bdmv") ) return 1;
  if( mov.write() ) return 1;
  if( bd_backup("MovieObject.bdmv") ) return 1;
// clips
  for( int ii=0; ii<cl.size(); ++ii ) {
    if( bd_open("CLIPINF/%05d.clpi", ii) ) return 1;
    if( cl[ii]->write() ) return 1;
    if( bd_backup("CLIPINF/%05d.clpi", ii) ) return 1;
  }
// playlists
  for( int ii=0; ii<pl.size(); ++ii ) {
    if( bd_open("PLAYLIST/%05d.mpls", ii) ) return 1;
    if( pl[ii]->write() ) return 1;
    if( bd_backup("PLAYLIST/%05d.mpls", ii) ) return 1;
  }
  return 0;
}

int
main(int ac, char **av)
{
  char *path = av[1];
  if( mkbdmv(path) ) return 1;
  av_log_set_level(AV_LOG_FATAL);
  //av_log_set_level(AV_LOG_VERBOSE);
  //av_log_set_level(AV_LOG_DEBUG);
  Media media;
  media_info *mp = 0;

  for( int ii=2; ii<ac; ++ii ) {
    char *ap = av[ii];
    // any dash seq followed by number sets curr title pgm_pid
    // single dash only sets title pgm_pid
    // double dash ends curr title, starts a new title
    // triple dash ends curr title as infinite still
    if( *ap == '-' ) {
      if( !mp ) continue;
      if( *++ap == '-' ) {
        mp->brk = 1;
        if( *++ap == '-' ) { ++ap;  mp->still = 1; }
      }
      if( *ap >= '0' && *ap <= '9' )
        mp->pgm_pid = strtoul(ap,&ap,0);
      if( mp->brk ) mp = 0;
      if( !ap || *ap ) {
        fprintf(stderr, "err arg %d: %s\n",ii,av[ii]);
        return 1;
      }
      continue;
    }
    mp = new media_info(av[ii]);
    media.append(mp);
    if( mp->scan() ) {
      fprintf(stderr, "cant scan media: %s\n", av[ii]);
      return 1;
    }
  }

  if( mp ) mp->brk = 1;

  if( media.compose() ) {
    fprintf(stderr, "cant compose media\n");
    return 1;
  }
  if( media.write(0) ) {
    fprintf(stderr, "cant prepare media\n");
    return 1;
  }
  if( media.write(path) ) {
    fprintf(stderr, "cant write media\n");
    return 1;
  }

  for( int ii=0; ii<media.size(); ++ii )
    if( media.bd_copy(media[ii]->filename, "STREAM/%05d.m2ts", ii) )
      return 1;

  return 0;
}

