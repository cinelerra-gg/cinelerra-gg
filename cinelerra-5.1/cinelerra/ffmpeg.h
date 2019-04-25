#ifndef FFMPEG_H
#define FFMPEG_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "arraylist.h"
#include "asset.inc"
#include "bccmodels.h"
#include "bcwindowbase.inc"
#include "condition.h"
#include "cstrdup.h"
#include "edl.inc"
#include "linklist.h"
#include "ffmpeg.inc"
#include "filebase.inc"
#include "fileffmpeg.inc"
#include "indexstate.inc"
#include "mutex.h"
#include "thread.h"
#include "vframe.inc"

extern "C" {
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

class FFPacket  {
	AVPacket pkt;
public:
	operator AVPacket*() { return &pkt; }
	operator AVPacket&() { return pkt; }
	AVPacket *operator ->() { return &pkt; }

	void init();
	void finit();
	FFPacket() { init(); }
	~FFPacket() { finit(); }
};

class FFrame : public ListItem<FFrame> {
	AVFrame *frm;
	int init;
public:
	int64_t position;
	FFStream *fst;

	FFrame(FFStream *fst);
	~FFrame();

	operator AVFrame*() { return frm; }
	operator AVFrame&() { return *frm; }
	AVFrame *operator ->() { return frm; }

	int initted() { return init; }
	void queue(int64_t pos);
	void dequeue();
};

class FFStream {
public:
	FFStream(FFMPEG *ffmpeg, AVStream *st, int fidx);
	~FFStream();
	static void ff_lock(const char *cp=0);
	static void ff_unlock();
	void queue(FFrame *frm);
	void dequeue(FFrame *frm);

	virtual int encode_activate();
	virtual int decode_activate();
	virtual AVHWDeviceType decode_hw_activate();
	virtual void decode_hw_format(AVCodec *decoder, AVHWDeviceType type);
	virtual int write_packet(FFPacket &pkt);
	int read_packet();
	int seek(int64_t no, double rate);
	int flush();
	int decode(AVFrame *frame);
	void load_markers(IndexMarks &marks, double rate);

	virtual int is_audio() = 0;
	virtual int is_video() = 0;
	virtual int decode_frame(AVFrame *frame) = 0;
	virtual int encode_frame(AVFrame *frame) = 0;
	virtual int init_frame(AVFrame *frame) = 0;
	virtual int create_filter(const char *filter_spec, AVCodecParameters *avpar) = 0;
	virtual void load_markers() = 0;
	virtual IndexMarks *get_markers() = 0;
	int create_filter(const char *filter_spec);
	int load_filter(AVFrame *frame);
	int read_filter(AVFrame *frame);
	int read_frame(AVFrame *frame);
	int open_stats_file();
	int close_stats_file();
	int read_stats_file();
	int write_stats_file();
	int init_stats_file();

	FFMPEG *ffmpeg;
	AVStream *st;
	AVFormatContext *fmt_ctx;
	AVCodecContext *avctx;

	AVFilterContext *buffersink_ctx;
	AVFilterContext *buffersrc_ctx;
	AVFilterGraph *filter_graph;
	AVFrame *frame, *fframe;
	AVBSFContext *bsfc;

	FFPacket ipkt;
	int need_packet, flushed;

	int frm_count;
	List<FFrame> frms;
	Mutex *frm_lock;

	int64_t nudge;
	int64_t seek_pos, curr_pos;
	int fidx;
	int reading, writing;
	int seeked, eof;

	const char *hw_dev;
	int hw_pixfmt;
	AVBufferRef *hw_device_ctx;

	FILE *stats_fp;
	char *stats_filename;
	char *stats_in;
	int pass;

	int st_eof() { return eof; }
	void st_eof(int v) { eof = v; }
};

class FFAudioStream : public FFStream {
	float *inp, *outp, *bfr, *lmt;
	int64_t hpos, sz;
	int nch;

	int read(float *fp, long len);
	void realloc(long nsz, int nch, long len);
	void realloc(long nsz, int nch);
	void reserve(long nsz, int nch);
	long used();
	long avail();
	void iseek(int64_t ofs);
	float *get_outp(int len);
	int64_t put_inp(int len);
	int write(const float *fp, long len);
	int zero(long len);
	int write(const double *dp, long len, int ch);
	int write_packet(FFPacket &pkt);
public:
	FFAudioStream(FFMPEG *ffmpeg, AVStream *strm, int idx, int fidx);
	virtual ~FFAudioStream();
	int is_audio() { return 1; }
	int is_video() { return 0; }
	void init_swr(int ichs, int ifmt, int irate);
	int get_samples(float *&samples, uint8_t **data, int len);
	int load_history(uint8_t **data, int len);
	int decode_frame(AVFrame *frame);
	int encode_frame(AVFrame *frame);
	int create_filter(const char *filter_spec, AVCodecParameters *avpar);
	void load_markers();
	IndexMarks *get_markers();

	int encode_activate();
	int64_t load_buffer(double ** const sp, int len);
	int in_history(int64_t pos);
	void reset_history();
	int read(double *dp, long len, int ch);

	int init_frame(AVFrame *frame);
	int load(int64_t pos, int len);
	int audio_seek(int64_t pos);
	int encode(double **samples, int len);
	int drain();

	int idx;
	int channel0, channels;
	int sample_rate;
	int mbsz, frame_sz;
	int64_t length;

	SwrContext *resample_context;
	int swr_ichs, swr_ifmt, swr_irate;
	int aud_bfr_sz;
	float *aud_bfr;
};


class FFVideoConvert {
public:
	struct SwsContext *convert_ctx;
	AVFrame *sw_frame;

	FFVideoConvert() { convert_ctx = 0; sw_frame = 0; }
	~FFVideoConvert() {
		if( convert_ctx ) sws_freeContext(convert_ctx);
		if( sw_frame ) av_frame_free(&sw_frame);
	}

	static AVPixelFormat color_model_to_pix_fmt(int color_model);
	static int pix_fmt_to_color_model(AVPixelFormat pix_fmt);

	int convert_picture_vframe(VFrame *frame, AVFrame *ip);
	int convert_picture_vframe(VFrame *frame, AVFrame *ip, AVFrame *ipic);
	int convert_cmodel(VFrame *frame, AVFrame *ip);
	int transfer_cmodel(VFrame *frame, AVFrame *ifp);
	int convert_vframe_picture(VFrame *frame, AVFrame *op);
	int convert_vframe_picture(VFrame *frame, AVFrame *op, AVFrame *opic);
	int convert_pixfmt(VFrame *frame, AVFrame *op);
	int transfer_pixfmt(VFrame *frame, AVFrame *ofp);
};

class FFVideoStream : public FFStream, public FFVideoConvert {
	int write_packet(FFPacket &pkt);
public:
	FFVideoStream(FFMPEG *ffmpeg, AVStream *strm, int idx, int fidx);
	virtual ~FFVideoStream();
	int is_audio() { return 0; }
	int is_video() { return 1; }
	int decode_frame(AVFrame *frame);
	AVHWDeviceType decode_hw_activate();
	void decode_hw_format(AVCodec *decoder, AVHWDeviceType type);
	int encode_frame(AVFrame *frame);
	int create_filter(const char *filter_spec, AVCodecParameters *avpar);
	void load_markers();
	IndexMarks *get_markers();

	int init_frame(AVFrame *picture);
	int load(VFrame *vframe, int64_t pos);
	int video_seek(int64_t pos);
	int encode(VFrame *vframe);
	int drain();

	int idx;
	double frame_rate;
	int width, height;
	int64_t length;
	float aspect_ratio;

	int interlaced;
	int top_field_first;
};

class FFMPEG : public Thread {
public:
	static Mutex fflock;
	static void ff_lock(const char *cp=0) { fflock.lock(cp); }
	static void ff_unlock() { fflock.unlock(); }

	int check_sample_rate(AVCodec *codec, int sample_rate);
	AVRational check_frame_rate(AVCodec *codec, double frame_rate);
	AVRational to_sample_aspect_ratio(Asset *asset);
	AVRational to_time_base(int sample_rate);
	static int get_fmt_score(AVSampleFormat dst_fmt, AVSampleFormat src_fmt);
	static AVSampleFormat find_best_sample_fmt_of_list(
		const AVSampleFormat *sample_fmts, AVSampleFormat src_fmt);

	static void set_option_path(char *path, const char *fmt, ...);
	static void get_option_path(char *path, const char *type, const char *spec);
	static int get_format(char *format, const char *path, const char *spec);
	static int get_codec(char *codec, const char *path, const char *spec);
	static int scan_option_line(const char *cp,char *tag,char *val);
	static int load_defaults(const char *path, const char *type,
		 char *codec, char *codec_options, int len);
	static int can_render(const char *fformat, const char *type);
	static int renders_audio(const char *fformat) { return can_render(fformat, "audio"); }
	static int renders_video(const char *fformat) { return can_render(fformat, "video"); }
	static int get_ff_option(const char *nm, const char *options, char *value);
	static void scan_audio_options(Asset *asset, EDL *edl);
	static void load_audio_options(Asset *asset, EDL *edl);
	static void scan_video_options(Asset *asset, EDL *edl);
	static void load_video_options(Asset *asset, EDL *edl);
	static void set_asset_format(Asset *asset, EDL *edl, const char *text);
	int get_file_format();
	static int get_encoder(const char *options, char *format, char *codec, char *bsfilter);
	static int scan_encoder(const char *line, char *format, char *codec, char *bsfilter);
	int read_options(const char *options, AVDictionary *&opts, int skip=0);
	int scan_options(const char *options, AVDictionary *&opts, AVStream *st);
	int read_options(FILE *fp, const char *options, AVDictionary *&opts);
	int load_options(const char *options, AVDictionary *&opts);
	static int load_options(const char *path, char *bfr, int len);
	void set_loglevel(const char *ap);
	static double to_secs(int64_t time, AVRational time_base);
	int info(char *text, int len);

	int init_decoder(const char *filename);
	int open_decoder();
	int init_encoder(const char *filename);
	int open_encoder(const char *type, const char *spec);
	int close_encoder();

	int total_audio_channels();
	int total_video_channels();

	int audio_seek(int ch, int64_t pos);
	int video_seek(int layer, int64_t pos);

	int decode(int chn, int64_t pos, double *samples, int len);
	int decode(int layer, int64_t pos, VFrame *frame);
	int decode_activate();
	int encode(int stream, double **samples, int len);
	int encode(int stream, VFrame *frame);
	int encode_activate();

	FileBase *file_base;
	AVFormatContext *fmt_ctx;
	ArrayList<FFAudioStream*> ffaudio;
	ArrayList<FFVideoStream*> ffvideo;
	AVDictionary *opts;
	double opt_duration;
	char *opt_video_filter;
	char *opt_audio_filter;
	char file_format[BCTEXTLEN];
	int fflags;

	class ffidx {
	public:
		uint16_t st_idx, st_ch;
		ffidx() { st_idx = st_ch = 0; }
		ffidx(const ffidx &t) { st_idx = t.st_idx;  st_ch = t.st_ch; }
		ffidx(uint16_t fidx, uint16_t ch) { st_idx = fidx; st_ch = ch; }
	};

	ArrayList<ffidx> astrm_index;
	ArrayList<ffidx> vstrm_index;
	int mux_audio(FFrame *frm);
	int mux_video(FFrame *frm);
	Condition *mux_lock;
	Condition *flow_lock;
	int done, flow;

	void start_muxer();
	void stop_muxer();
	void flow_off();
	void flow_on();
	void flow_ctl();
	void mux();
	void run();

	int decoding, encoding;
	int has_audio, has_video;

	FFMPEG(FileBase *file_base=0);
	~FFMPEG();
	int scan(IndexState *index_state, int64_t *scan_position, int *canceled);

	int ff_audio_stream(int channel) { return astrm_index[channel].st_idx; }
	int ff_video_stream(int layer) { return vstrm_index[layer].st_idx; }

	int ff_total_audio_channels();
	int ff_total_astreams();
	int ff_audio_channels(int stream);
	int ff_sample_rate(int stream);
	const char *ff_audio_format(int stream);
	int ff_audio_pid(int stream);
	int64_t ff_audio_samples(int stream);
	int ff_audio_for_video(int vstream, int astream, int64_t &channels);

	int ff_total_video_layers();
	int ff_total_vstreams();
	int ff_video_width(int stream);
	int ff_video_height(int stream);
	int ff_set_video_width(int stream, int width);
	int ff_set_video_height(int stream, int height);
	int ff_coded_width(int stream);
	int ff_coded_height(int stream);
	float ff_aspect_ratio(int stream);
	double ff_frame_rate(int stream);
	const char *ff_video_format(int stream);
	int64_t ff_video_frames(int stream);
	int ff_video_pid(int stream);
	int ff_video_mpeg_color_range(int stream);

	int ff_cpus();
	void dump_context(AVCodecContext *ctx);
};

#endif /* FFMPEG_H */
