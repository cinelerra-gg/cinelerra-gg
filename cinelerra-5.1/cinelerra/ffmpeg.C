
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>

// work arounds (centos)
#include <lzma.h>
#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif
#define MAX_RETRY 1000
// max pts/curr_pos drift allowed before correction (in seconds)
#define AUDIO_PTS_TOLERANCE 0.04

#include "asset.h"
#include "bccmodels.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "fileffmpeg.h"
#include "filesystem.h"
#include "ffmpeg.h"
#include "indexfile.h"
#include "interlacemodes.h"
#include "libdv.h"
#include "libmjpeg.h"
#include "mainerror.h"
#include "mwindow.h"
#include "vframe.h"

#ifdef FFMPEG3
#define url filename
#else
#define av_register_all(s)
#define avfilter_register_all(s)
#endif

#define VIDEO_INBUF_SIZE 0x10000
#define AUDIO_INBUF_SIZE 0x10000
#define VIDEO_REFILL_THRESH 0
#define AUDIO_REFILL_THRESH 0x1000
#define AUDIO_MIN_FRAME_SZ 128

#define FF_ESTM_TIMES 0x0001
#define FF_BAD_TIMES  0x0002

Mutex FFMPEG::fflock("FFMPEG::fflock");

static void ff_err(int ret, const char *fmt, ...)
{
	char msg[BCTEXTLEN];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	char errmsg[BCSTRLEN];
	av_strerror(ret, errmsg, sizeof(errmsg));
	fprintf(stderr,_("%s  err: %s\n"),msg, errmsg);
}

void FFPacket::init()
{
	av_init_packet(&pkt);
	pkt.data = 0; pkt.size = 0;
}
void FFPacket::finit()
{
	av_packet_unref(&pkt);
}

FFrame::FFrame(FFStream *fst)
{
	this->fst = fst;
	frm = av_frame_alloc();
	init = fst->init_frame(frm);
}

FFrame::~FFrame()
{
	av_frame_free(&frm);
}

void FFrame::queue(int64_t pos)
{
	position = pos;
	fst->queue(this);
}

void FFrame::dequeue()
{
	fst->dequeue(this);
}

int FFAudioStream::read(float *fp, long len)
{
	long n = len * nch;
	float *op = outp;
	while( n > 0 ) {
		int k = lmt - op;
		if( k > n ) k = n;
		n -= k;
		while( --k >= 0 ) *fp++ = *op++;
		if( op >= lmt ) op = bfr;
	}
	return len;
}

void FFAudioStream::realloc(long nsz, int nch, long len)
{
	long bsz = nsz * nch;
	float *np = new float[bsz];
	inp = np + read(np, len) * nch;
	outp = np;
	lmt = np + bsz;
	this->nch = nch;
	sz = nsz;
	delete [] bfr;  bfr = np;
}

void FFAudioStream::realloc(long nsz, int nch)
{
	if( nsz > sz || this->nch != nch ) {
		long len = this->nch != nch ? 0 : hpos;
		if( len > sz ) len = sz;
		iseek(len);
		realloc(nsz, nch, len);
	}
}

void FFAudioStream::reserve(long nsz, int nch)
{
	long len = (inp - outp) / nch;
	nsz += len;
	if( nsz > sz || this->nch != nch ) {
		if( this->nch != nch ) len = 0;
		realloc(nsz, nch, len);
		return;
	}
	if( (len*=nch) > 0 && bfr != outp )
		memmove(bfr, outp, len*sizeof(*bfr));
	outp = bfr;
	inp = bfr + len;
}

long FFAudioStream::used()
{
	long len = inp>=outp ? inp-outp : inp-bfr + lmt-outp;
	return len / nch;
}
long FFAudioStream::avail()
{
	float *in1 = inp+1;
	if( in1 >= lmt ) in1 = bfr;
	long len = outp >= in1 ? outp-in1 : outp-bfr + lmt-in1;
	return len / nch;
}
void FFAudioStream::reset_history()
{
	inp = outp = bfr;
	hpos = 0;
	memset(bfr, 0, lmt-bfr);
}

void FFAudioStream::iseek(int64_t ofs)
{
	if( ofs > hpos ) ofs = hpos;
	if( ofs > sz ) ofs = sz;
	outp = inp - ofs*nch;
	if( outp < bfr ) outp += sz*nch;
}

float *FFAudioStream::get_outp(int ofs)
{
	float *ret = outp;
	outp += ofs*nch;
	return ret;
}

int64_t FFAudioStream::put_inp(int ofs)
{
	inp += ofs*nch;
	return (inp-outp) / nch;
}

int FFAudioStream::write(const float *fp, long len)
{
	long n = len * nch;
	float *ip = inp;
	while( n > 0 ) {
		int k = lmt - ip;
		if( k > n ) k = n;
		n -= k;
		while( --k >= 0 ) *ip++ = *fp++;
		if( ip >= lmt ) ip = bfr;
	}
	inp = ip;
	hpos += len;
	return len;
}

int FFAudioStream::zero(long len)
{
	long n = len * nch;
	float *ip = inp;
	while( n > 0 ) {
		int k = lmt - ip;
		if( k > n ) k = n;
		n -= k;
		while( --k >= 0 ) *ip++ = 0;
		if( ip >= lmt ) ip = bfr;
	}
	inp = ip;
	hpos += len;
	return len;
}

// does not advance outp
int FFAudioStream::read(double *dp, long len, int ch)
{
	long n = len;
	float *op = outp + ch;
	float *lmt1 = lmt + nch-1;
	while( n > 0 ) {
		int k = (lmt1 - op) / nch;
		if( k > n ) k = n;
		n -= k;
		while( --k >= 0 ) { *dp++ = *op;  op += nch; }
		if( op >= lmt ) op -= sz*nch;
	}
	return len;
}

// load linear buffer, no wrapping allowed, does not advance inp
int FFAudioStream::write(const double *dp, long len, int ch)
{
	long n = len;
	float *ip = inp + ch;
	while( --n >= 0 ) { *ip = *dp++;  ip += nch; }
	return len;
}


FFStream::FFStream(FFMPEG *ffmpeg, AVStream *st, int fidx)
{
	this->ffmpeg = ffmpeg;
	this->st = st;
	this->fidx = fidx;
	frm_lock = new Mutex("FFStream::frm_lock");
	fmt_ctx = 0;
	avctx = 0;
	filter_graph = 0;
	buffersrc_ctx = 0;
	buffersink_ctx = 0;
	frm_count = 0;
	nudge = AV_NOPTS_VALUE;
	seek_pos = curr_pos = 0;
	seeked = 1;  eof = 0;
	reading = writing = 0;
	flushed = 0;
	need_packet = 1;
	frame = fframe = 0;
	bsfc = 0;
	stats_fp = 0;
	stats_filename = 0;
	stats_in = 0;
	pass = 0;
}

FFStream::~FFStream()
{
	if( reading > 0 || writing > 0 ) avcodec_close(avctx);
	if( avctx ) avcodec_free_context(&avctx);
	if( fmt_ctx ) avformat_close_input(&fmt_ctx);
	if( bsfc ) av_bsf_free(&bsfc);
	while( frms.first ) frms.remove(frms.first);
	if( filter_graph ) avfilter_graph_free(&filter_graph);
	if( frame ) av_frame_free(&frame);
	if( fframe ) av_frame_free(&fframe);
	delete frm_lock;
	if( stats_fp ) fclose(stats_fp);
	if( stats_in ) av_freep(&stats_in);
	delete [] stats_filename;
}

void FFStream::ff_lock(const char *cp)
{
	FFMPEG::fflock.lock(cp);
}

void FFStream::ff_unlock()
{
	FFMPEG::fflock.unlock();
}

void FFStream::queue(FFrame *frm)
{
 	frm_lock->lock("FFStream::queue");
	frms.append(frm);
	++frm_count;
	frm_lock->unlock();
	ffmpeg->mux_lock->unlock();
}

void FFStream::dequeue(FFrame *frm)
{
	frm_lock->lock("FFStream::dequeue");
	--frm_count;
	frms.remove_pointer(frm);
	frm_lock->unlock();
}

int FFStream::encode_activate()
{
	if( writing < 0 )
		writing = ffmpeg->encode_activate();
	return writing;
}

int FFStream::decode_activate()
{
	if( reading < 0 && (reading=ffmpeg->decode_activate()) > 0 ) {
		ff_lock("FFStream::decode_activate");
		reading = 0;
		AVDictionary *copts = 0;
		av_dict_copy(&copts, ffmpeg->opts, 0);
		int ret = 0;
		// this should be avformat_copy_context(), but no copy avail
		ret = avformat_open_input(&fmt_ctx,
			ffmpeg->fmt_ctx->url, ffmpeg->fmt_ctx->iformat, &copts);
		if( ret >= 0 ) {
			ret = avformat_find_stream_info(fmt_ctx, 0);
			st = fmt_ctx->streams[fidx];
			load_markers();
		}
		if( ret >= 0 && st != 0 ) {
			AVCodecID codec_id = st->codecpar->codec_id;
			AVCodec *decoder = avcodec_find_decoder(codec_id);
			avctx = avcodec_alloc_context3(decoder);
			if( !avctx ) {
				eprintf(_("cant allocate codec context\n"));
				ret = AVERROR(ENOMEM);
			}
			if( ret >= 0 ) {
				avcodec_parameters_to_context(avctx, st->codecpar);
				if( !av_dict_get(copts, "threads", NULL, 0) )
					avctx->thread_count = ffmpeg->ff_cpus();
				ret = avcodec_open2(avctx, decoder, &copts);
			}
			if( ret >= 0 ) {
				reading = 1;
			}
			else
				eprintf(_("open decoder failed\n"));
		}
		else
			eprintf(_("can't clone input file\n"));
		av_dict_free(&copts);
		ff_unlock();
	}
	return reading;
}

int FFStream::read_packet()
{
	av_packet_unref(ipkt);
	int ret = av_read_frame(fmt_ctx, ipkt);
	if( ret < 0 ) {
		st_eof(1);
		if( ret == AVERROR_EOF ) return 0;
		ff_err(ret, "FFStream::read_packet: av_read_frame failed\n");
		flushed = 1;
		return -1;
	}
	return 1;
}

int FFStream::decode(AVFrame *frame)
{
	int ret = 0;
	int retries = MAX_RETRY;

	while( ret >= 0 && !flushed && --retries >= 0 ) {
		if( need_packet ) {
			if( (ret=read_packet()) < 0 ) break;
			AVPacket *pkt = ret > 0 ? (AVPacket*)ipkt : 0;
			if( pkt ) {
				if( pkt->stream_index != st->index ) continue;
				if( !pkt->data | !pkt->size ) continue;
			}
			if( (ret=avcodec_send_packet(avctx, pkt)) < 0 ) {
				ff_err(ret, "FFStream::decode: avcodec_send_packet failed\n");
				break;
			}
			need_packet = 0;
			retries = MAX_RETRY;
		}
		if( (ret=decode_frame(frame)) > 0 ) break;
		if( !ret ) {
			need_packet = 1;
			flushed = st_eof();
		}
	}

	if( retries < 0 ) {
		fprintf(stderr, "FFStream::decode: Retry limit\n");
		ret = 0;
	}
	if( ret < 0 )
		fprintf(stderr, "FFStream::decode: failed\n");
	return ret;
}

int FFStream::load_filter(AVFrame *frame)
{
	int ret = av_buffersrc_add_frame_flags(buffersrc_ctx, frame, 0);
	if( ret < 0 )
		eprintf(_("av_buffersrc_add_frame_flags failed\n"));
	return ret;
}

int FFStream::read_filter(AVFrame *frame)
{
	int ret = av_buffersink_get_frame(buffersink_ctx, frame);
	if( ret < 0 ) {
		if( ret == AVERROR(EAGAIN) ) return 0;
		if( ret == AVERROR_EOF ) { st_eof(1); return -1; }
		ff_err(ret, "FFStream::read_filter: av_buffersink_get_frame failed\n");
		return ret;
	}
	return 1;
}

int FFStream::read_frame(AVFrame *frame)
{
	av_frame_unref(frame);
	if( !filter_graph || !buffersrc_ctx || !buffersink_ctx )
		return decode(frame);
	if( !fframe && !(fframe=av_frame_alloc()) ) {
		fprintf(stderr, "FFStream::read_frame: av_frame_alloc failed\n");
		return -1;
	}
	int ret = -1;
	while( !flushed && !(ret=read_filter(frame)) ) {
		if( (ret=decode(fframe)) < 0 ) break;
		if( ret > 0 && (ret=load_filter(fframe)) < 0 ) break;
	}
	return ret;
}

int FFStream::write_packet(FFPacket &pkt)
{
	int ret = 0;
	if( !bsfc ) {
		av_packet_rescale_ts(pkt, avctx->time_base, st->time_base);
		pkt->stream_index = st->index;
		ret = av_interleaved_write_frame(ffmpeg->fmt_ctx, pkt);
	}
	else {
		ret = av_bsf_send_packet(bsfc, pkt);
		while( ret >= 0 ) {
			FFPacket bs;
			if( (ret=av_bsf_receive_packet(bsfc, bs)) < 0 ) {
				if( ret == AVERROR(EAGAIN) ) return 0;
				if( ret == AVERROR_EOF ) return -1;
				break;
			}
			av_packet_rescale_ts(bs, avctx->time_base, st->time_base);
			bs->stream_index = st->index;
			ret = av_interleaved_write_frame(ffmpeg->fmt_ctx, bs);
		}
	}
	if( ret < 0 )
		ff_err(ret, "FFStream::write_packet: write packet failed\n");
	return ret;
}

int FFStream::encode_frame(AVFrame *frame)
{
	int pkts = 0, ret = 0;
	for( int retry=MAX_RETRY; --retry>=0; ) {
		if( frame || !pkts )
			ret = avcodec_send_frame(avctx, frame);
		if( !ret && frame ) return pkts;
		if( ret < 0 && ret != AVERROR(EAGAIN) ) break;
		FFPacket opkt;
		ret = avcodec_receive_packet(avctx, opkt);
		if( !frame && ret == AVERROR_EOF ) return pkts;
		if( ret < 0 ) break;
		ret = write_packet(opkt);
		if( ret < 0 ) break;
		++pkts;
		if( frame && stats_fp ) {
			ret = write_stats_file();
			if( ret < 0 ) break;
		}
	}
	ff_err(ret, "FFStream::encode_frame: encode failed\n");
	return -1;
}

int FFStream::flush()
{
	if( writing < 0 )
		return -1;
	int ret = encode_frame(0);
	if( ret >= 0 && stats_fp ) {
		ret = write_stats_file();
		close_stats_file();
	}
	if( ret < 0 )
		ff_err(ret, "FFStream::flush");
	return ret >= 0 ? 0 : 1;
}


int FFStream::open_stats_file()
{
	stats_fp = fopen(stats_filename,"w");
	return stats_fp ? 0 : AVERROR(errno);
}

int FFStream::close_stats_file()
{
	if( stats_fp ) {
		fclose(stats_fp);  stats_fp = 0;
	}
	return 0;
}

int FFStream::read_stats_file()
{
	int64_t len = 0;  struct stat stats_st;
	int fd = open(stats_filename, O_RDONLY);
	int ret = fd >= 0 ? 0: ENOENT;
	if( !ret && fstat(fd, &stats_st) )
		ret = EINVAL;
	if( !ret ) {
		len = stats_st.st_size;
		stats_in = (char *)av_malloc(len+1);
		if( !stats_in )
			ret = ENOMEM;
	}
	if( !ret && read(fd, stats_in, len+1) != len )
		ret = EIO;
	if( !ret ) {
		stats_in[len] = 0;
		avctx->stats_in = stats_in;
	}
	if( fd >= 0 )
		close(fd);
	return !ret ? 0 : AVERROR(ret);
}

int FFStream::write_stats_file()
{
	int ret = 0;
	if( avctx->stats_out && (ret=strlen(avctx->stats_out)) > 0 ) {
		int len = fwrite(avctx->stats_out, 1, ret, stats_fp);
		if( ret != len )
			ff_err(ret = AVERROR(errno), "FFStream::write_stats_file");
	}
	return ret;
}

int FFStream::init_stats_file()
{
	int ret = 0;
	if( (pass & 2) && (ret = read_stats_file()) < 0 )
		ff_err(ret, "stat file read: %s", stats_filename);
	if( (pass & 1) && (ret=open_stats_file()) < 0 )
		ff_err(ret, "stat file open: %s", stats_filename);
	return ret >= 0 ? 0 : ret;
}

int FFStream::seek(int64_t no, double rate)
{
// default ffmpeg native seek
	int npkts = 1;
	int64_t pos = no, pkt_pos = -1;
	IndexMarks *index_markers = get_markers();
	if( index_markers && index_markers->size() > 1 ) {
		IndexMarks &marks = *index_markers;
		int i = marks.find(pos);
		int64_t n = i < 0 ? (i=0) : marks[i].no;
// if indexed seek point not too far away (<30 secs), use index
		if( no-n < 30*rate ) {
			if( n < 0 ) n = 0;
			pos = n;
			if( i < marks.size() ) pkt_pos = marks[i].pos;
			npkts = MAX_RETRY;
		}
	}
	if( pos == curr_pos ) return 0;
	double secs = pos < 0 ? 0. : pos / rate;
	AVRational time_base = st->time_base;
	int64_t tstmp = time_base.num > 0 ? secs * time_base.den/time_base.num : 0;
	if( !tstmp ) {
		if( st->nb_index_entries > 0 ) tstmp = st->index_entries[0].timestamp;
		else if( st->start_time != AV_NOPTS_VALUE ) tstmp = st->start_time;
		else if( st->first_dts != AV_NOPTS_VALUE ) tstmp = st->first_dts;
		else tstmp = INT64_MIN+1;
	}
	else if( nudge != AV_NOPTS_VALUE ) tstmp += nudge;
	int idx = st->index;
#if 0
// seek all streams using the default timebase.
//   this is how ffmpeg and ffplay work.  stream seeks are less tested.
	tstmp = av_rescale_q(tstmp, time_base, AV_TIME_BASE_Q);
	idx = -1;
#endif

	avcodec_flush_buffers(avctx);
	avformat_flush(fmt_ctx);
#if 0
	int64_t seek = tstmp;
	int flags = AVSEEK_FLAG_ANY;
	if( !(fmt_ctx->iformat->flags & AVFMT_NO_BYTE_SEEK) && pkt_pos >= 0 ) {
		seek = pkt_pos;
		flags = AVSEEK_FLAG_BYTE;
	}
	int ret = avformat_seek_file(fmt_ctx, st->index, -INT64_MAX, seek, INT64_MAX, flags);
#else
// finds the first index frame below the target time
	int flags = AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY;
	int ret = av_seek_frame(fmt_ctx, idx, tstmp, flags);
#endif
	int retry = MAX_RETRY;
	while( ret >= 0 ) {
		need_packet = 0;  flushed = 0;
		seeked = 1;  st_eof(0);
// read up to retry packets, limited to npkts in stream, and not pkt.pos past pkt_pos
		while( --retry >= 0 ) {
			if( read_packet() <= 0 ) { ret = -1;  break; }
			if( ipkt->stream_index != st->index ) continue;
			if( !ipkt->data || !ipkt->size ) continue;
			if( pkt_pos >= 0 && ipkt->pos >= pkt_pos ) break;
			if( --npkts <= 0 ) break;
			int64_t pkt_ts = ipkt->dts != AV_NOPTS_VALUE ? ipkt->dts : ipkt->pts;
			if( pkt_ts == AV_NOPTS_VALUE ) continue;
			if( pkt_ts >= tstmp ) break;
		}
		if( retry < 0 ) {
			fprintf(stderr,"FFStream::seek: retry limit, pos=%jd tstmp=%jd\n",pos,tstmp);
			ret = -1;
		}
		if( ret < 0 ) break;
		ret = avcodec_send_packet(avctx, ipkt);
		if( !ret ) break;
//some codecs need more than one pkt to resync
		if( ret == AVERROR_INVALIDDATA ) ret = 0;
		if( ret < 0 ) {
			ff_err(ret, "FFStream::avcodec_send_packet failed\n");
			break;
		}
	}
	if( ret < 0 ) {
printf("** seek fail %jd, %jd\n", pos, tstmp);
		seeked = need_packet = 0;
		st_eof(flushed=1);
		return -1;
	}
//printf("seeked pos = %ld, %ld\n", pos, tstmp);
	seek_pos = curr_pos = pos;
	return 0;
}

FFAudioStream::FFAudioStream(FFMPEG *ffmpeg, AVStream *strm, int idx, int fidx)
 : FFStream(ffmpeg, strm, fidx)
{
	this->idx = idx;
	channel0 = channels = 0;
	sample_rate = 0;
	mbsz = 0;
	frame_sz = AUDIO_MIN_FRAME_SZ;
	length = 0;
	resample_context = 0;
	swr_ichs = swr_ifmt = swr_irate = 0;

	aud_bfr_sz = 0;
	aud_bfr = 0;

// history buffer
	nch = 2;
	sz = 0x10000;
	long bsz = sz * nch;
	bfr = new float[bsz];
	lmt = bfr + bsz;
	reset_history();
}

FFAudioStream::~FFAudioStream()
{
	if( resample_context ) swr_free(&resample_context);
	delete [] aud_bfr;
	delete [] bfr;
}

void FFAudioStream::init_swr(int ichs, int ifmt, int irate)
{
	if( resample_context ) {
		if( swr_ichs == ichs && swr_ifmt == ifmt && swr_irate == irate )
			return;
		swr_free(&resample_context);
	}
	swr_ichs = ichs;  swr_ifmt = ifmt;  swr_irate = irate;
	if( ichs == channels && ifmt == AV_SAMPLE_FMT_FLT && irate == sample_rate )
		return;
	uint64_t ilayout = av_get_default_channel_layout(ichs);
	if( !ilayout ) ilayout = ((uint64_t)1<<ichs) - 1;
	uint64_t olayout = av_get_default_channel_layout(channels);
	if( !olayout ) olayout = ((uint64_t)1<<channels) - 1;
	resample_context = swr_alloc_set_opts(NULL,
		olayout, AV_SAMPLE_FMT_FLT, sample_rate,
		ilayout, (AVSampleFormat)ifmt, irate,
		0, NULL);
	if( resample_context )
		swr_init(resample_context);
}

int FFAudioStream::get_samples(float *&samples, uint8_t **data, int len)
{
	samples = *(float **)data;
	if( resample_context ) {
		if( len > aud_bfr_sz ) {
			delete [] aud_bfr;
			aud_bfr = 0;
		}
		if( !aud_bfr ) {
			aud_bfr_sz = len;
			aud_bfr = new float[aud_bfr_sz*channels];
		}
		int ret = swr_convert(resample_context,
			(uint8_t**)&aud_bfr, aud_bfr_sz, (const uint8_t**)data, len);
		if( ret < 0 ) {
			ff_err(ret, "FFAudioStream::get_samples: swr_convert failed\n");
			return -1;
		}
		samples = aud_bfr;
		len = ret;
	}
	return len;
}

int FFAudioStream::load_history(uint8_t **data, int len)
{
	float *samples;
	len = get_samples(samples, data, len);
	if( len > 0 ) {
		// biggest user bfr since seek + frame
		realloc(mbsz + len + 1, channels);
		write(samples, len);
	}
	return len;
}

int FFAudioStream::decode_frame(AVFrame *frame)
{
	int first_frame = seeked;  seeked = 0;
	frame->best_effort_timestamp = AV_NOPTS_VALUE;
	int ret = avcodec_receive_frame(avctx, frame);
	if( ret < 0 ) {
		if( first_frame || ret == AVERROR(EAGAIN) ) return 0;
		if( ret == AVERROR_EOF ) { st_eof(1); return 0; }
		ff_err(ret, "FFAudioStream::decode_frame: Could not read audio frame\n");
		return -1;
	}
	int64_t pkt_ts = frame->best_effort_timestamp;
	if( pkt_ts != AV_NOPTS_VALUE ) {
		double ts = ffmpeg->to_secs(pkt_ts - nudge, st->time_base);
		double t = (double)curr_pos / sample_rate;
// some time_base clocks are very grainy, too grainy for audio (clicks, pops)
		if( fabs(ts - t) > AUDIO_PTS_TOLERANCE )
			curr_pos = ts * sample_rate + 0.5;
	}
	return 1;
}

int FFAudioStream::encode_activate()
{
	if( writing >= 0 ) return writing;
	if( !avctx->codec ) return writing = 0;
	frame_sz = avctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE ?
		10000 : avctx->frame_size;
	return FFStream::encode_activate();
}

int64_t FFAudioStream::load_buffer(double ** const sp, int len)
{
	reserve(len+1, st->codecpar->channels);
	for( int ch=0; ch<nch; ++ch )
		write(sp[ch], len, ch);
	return put_inp(len);
}

int FFAudioStream::in_history(int64_t pos)
{
	if( pos > curr_pos ) return 0;
	int64_t len = hpos;
	if( len > sz ) len = sz;
	if( pos < curr_pos - len ) return 0;
	return 1;
}


int FFAudioStream::init_frame(AVFrame *frame)
{
	frame->nb_samples = frame_sz;
	frame->format = avctx->sample_fmt;
	frame->channel_layout = avctx->channel_layout;
	frame->sample_rate = avctx->sample_rate;
	int ret = av_frame_get_buffer(frame, 0);
	if (ret < 0)
		ff_err(ret, "FFAudioStream::init_frame: av_frame_get_buffer failed\n");
	return ret;
}

int FFAudioStream::load(int64_t pos, int len)
{
	if( audio_seek(pos) < 0 ) return -1;
	if( !frame && !(frame=av_frame_alloc()) ) {
		fprintf(stderr, "FFAudioStream::load: av_frame_alloc failed\n");
		return -1;
	}
	if( mbsz < len ) mbsz = len;
	int64_t end_pos = pos + len;
	int ret = 0, i = len / frame_sz + MAX_RETRY;
	while( ret>=0 && !flushed && curr_pos<end_pos && --i>=0 ) {
		ret = read_frame(frame);
		if( ret > 0 && frame->nb_samples > 0 ) {
			init_swr(frame->channels, frame->format, frame->sample_rate);
			load_history(&frame->extended_data[0], frame->nb_samples);
			curr_pos += frame->nb_samples;
		}
	}
	if( end_pos > curr_pos ) {
		zero(end_pos - curr_pos);
		curr_pos = end_pos;
	}
	len = curr_pos - pos;
	iseek(len);
	return len;
}

int FFAudioStream::audio_seek(int64_t pos)
{
	if( decode_activate() <= 0 ) return -1;
	if( !st->codecpar ) return -1;
	if( in_history(pos) ) return 0;
	if( pos == curr_pos ) return 0;
	reset_history();  mbsz = 0;
// guarentee preload > 1sec samples
	if( (pos-=sample_rate) < 0 ) pos = 0;
	if( seek(pos, sample_rate) < 0 ) return -1;
	return 1;
}

int FFAudioStream::encode(double **samples, int len)
{
	if( encode_activate() <= 0 ) return -1;
	ffmpeg->flow_ctl();
	int ret = 0;
	int64_t count = samples ? load_buffer(samples, len) : used();
	int frame_sz1 = samples ? frame_sz-1 : 0;
	FFrame *frm = 0;

	while( ret >= 0 && count > frame_sz1 ) {
		frm = new FFrame(this);
		if( (ret=frm->initted()) < 0 ) break;
		AVFrame *frame = *frm;
		len = count >= frame_sz ? frame_sz : count;
		float *bfrp = get_outp(len);
		ret =  swr_convert(resample_context,
			(uint8_t **)frame->extended_data, len,
			(const uint8_t **)&bfrp, len);
		if( ret < 0 ) {
			ff_err(ret, "FFAudioStream::encode: swr_convert failed\n");
			break;
		}
		frame->nb_samples = len;
		frm->queue(curr_pos);
		frm = 0;
		curr_pos += len;
		count -= len;
	}

	delete frm;
	return ret >= 0 ? 0 : 1;
}

int FFAudioStream::drain()
{
	return encode(0,0);
}

int FFAudioStream::encode_frame(AVFrame *frame)
{
	return FFStream::encode_frame(frame);
}

int FFAudioStream::write_packet(FFPacket &pkt)
{
	return FFStream::write_packet(pkt);
}

void FFAudioStream::load_markers()
{
	IndexState *index_state = ffmpeg->file_base->asset->index_state;
	if( !index_state || idx >= index_state->audio_markers.size() ) return;
	if( index_state->marker_status == MARKERS_NOTTESTED ) return;
	FFStream::load_markers(*index_state->audio_markers[idx], sample_rate);
}

IndexMarks *FFAudioStream::get_markers()
{
	IndexState *index_state = ffmpeg->file_base->asset->index_state;
	if( !index_state || idx >= index_state->audio_markers.size() ) return 0;
	return index_state->audio_markers[idx];
}

FFVideoStream::FFVideoStream(FFMPEG *ffmpeg, AVStream *strm, int idx, int fidx)
 : FFStream(ffmpeg, strm, fidx)
{
	this->idx = idx;
	width = height = 0;
	frame_rate = 0;
	aspect_ratio = 0;
	length = 0;
	interlaced = 0;
	top_field_first = 0;
}

FFVideoStream::~FFVideoStream()
{
}

int FFVideoStream::decode_frame(AVFrame *frame)
{
	int first_frame = seeked;  seeked = 0;
	int ret = avcodec_receive_frame(avctx, frame);
	if( ret < 0 ) {
		if( first_frame || ret == AVERROR(EAGAIN) ) return 0;
		if( ret == AVERROR(EAGAIN) ) return 0;
		if( ret == AVERROR_EOF ) { st_eof(1); return 0; }
		ff_err(ret, "FFVideoStream::decode_frame: Could not read video frame\n");
		return -1;
	}
	int64_t pkt_ts = frame->best_effort_timestamp;
	if( pkt_ts != AV_NOPTS_VALUE )
		curr_pos = ffmpeg->to_secs(pkt_ts - nudge, st->time_base) * frame_rate + 0.5;
	return 1;
}

int FFVideoStream::load(VFrame *vframe, int64_t pos)
{
	int ret = video_seek(pos);
	if( ret < 0 ) return -1;
	if( !frame && !(frame=av_frame_alloc()) ) {
		fprintf(stderr, "FFVideoStream::load: av_frame_alloc failed\n");
		return -1;
	}
	int i = MAX_RETRY + pos - curr_pos;
	while( ret>=0 && !flushed && curr_pos<=pos && --i>=0 ) {
		ret = read_frame(frame);
		if( ret > 0 ) ++curr_pos;
	}
	if( frame->format == AV_PIX_FMT_NONE || frame->width <= 0 || frame->height <= 0 )
		ret = -1;
	if( ret >= 0 ) {
		ret = convert_cmodel(vframe, frame);
	}
	ret = ret > 0 ? 1 : ret < 0 ? -1 : 0;
	return ret;
}

int FFVideoStream::video_seek(int64_t pos)
{
	if( decode_activate() <= 0 ) return -1;
	if( !st->codecpar ) return -1;
	if( pos == curr_pos-1 && !seeked ) return 0;
// if close enough, just read up to current
	int gop = avctx->gop_size;
	if( gop < 4 ) gop = 4;
	if( gop > 64 ) gop = 64;
	int read_limit = curr_pos + 3*gop;
	if( pos >= curr_pos && pos <= read_limit ) return 0;
// guarentee preload more than 2*gop frames
	if( seek(pos - 3*gop, frame_rate) < 0 ) return -1;
	return 1;
}

int FFVideoStream::init_frame(AVFrame *picture)
{
	picture->format = avctx->pix_fmt;
	picture->width  = avctx->width;
	picture->height = avctx->height;
	int ret = av_frame_get_buffer(picture, 32);
	return ret;
}

int FFVideoStream::encode(VFrame *vframe)
{
	if( encode_activate() <= 0 ) return -1;
	ffmpeg->flow_ctl();
	FFrame *picture = new FFrame(this);
	int ret = picture->initted();
	if( ret >= 0 ) {
		AVFrame *frame = *picture;
		frame->pts = curr_pos;
		ret = convert_pixfmt(vframe, frame);
	}
	if( ret >= 0 ) {
		picture->queue(curr_pos);
		++curr_pos;
	}
	else {
		fprintf(stderr, "FFVideoStream::encode: encode failed\n");
		delete picture;
	}
	return ret >= 0 ? 0 : 1;
}

int FFVideoStream::drain()
{
	return 0;
}

int FFVideoStream::encode_frame(AVFrame *frame)
{
	if( frame ) {
		frame->interlaced_frame = interlaced;
		frame->top_field_first = top_field_first;
	}
	return FFStream::encode_frame(frame);
}

int FFVideoStream::write_packet(FFPacket &pkt)
{
	if( !(ffmpeg->fmt_ctx->oformat->flags & AVFMT_VARIABLE_FPS) )
		pkt->duration = 1;
	return FFStream::write_packet(pkt);
}

AVPixelFormat FFVideoConvert::color_model_to_pix_fmt(int color_model)
{
	switch( color_model ) {
	case BC_YUV422:		return AV_PIX_FMT_YUYV422;
	case BC_RGB888:		return AV_PIX_FMT_RGB24;
	case BC_RGBA8888:	return AV_PIX_FMT_RGBA;
	case BC_BGR8888:	return AV_PIX_FMT_BGR0;
	case BC_BGR888:		return AV_PIX_FMT_BGR24;
	case BC_ARGB8888:	return AV_PIX_FMT_ARGB;
	case BC_ABGR8888:	return AV_PIX_FMT_ABGR;
	case BC_RGB8:		return AV_PIX_FMT_RGB8;
	case BC_YUV420P:	return AV_PIX_FMT_YUV420P;
	case BC_YUV422P:	return AV_PIX_FMT_YUV422P;
	case BC_YUV444P:	return AV_PIX_FMT_YUV444P;
	case BC_YUV411P:	return AV_PIX_FMT_YUV411P;
	case BC_RGB565:		return AV_PIX_FMT_RGB565;
	case BC_RGB161616:      return AV_PIX_FMT_RGB48LE;
	case BC_RGBA16161616:   return AV_PIX_FMT_RGBA64LE;
	case BC_AYUV16161616:	return AV_PIX_FMT_AYUV64LE;
	case BC_GBRP:		return AV_PIX_FMT_GBRP;
	default: break;
	}

	return AV_PIX_FMT_NB;
}

int FFVideoConvert::pix_fmt_to_color_model(AVPixelFormat pix_fmt)
{
	switch (pix_fmt) {
	case AV_PIX_FMT_YUYV422:	return BC_YUV422;
	case AV_PIX_FMT_RGB24:		return BC_RGB888;
	case AV_PIX_FMT_RGBA:		return BC_RGBA8888;
	case AV_PIX_FMT_BGR0:		return BC_BGR8888;
	case AV_PIX_FMT_BGR24:		return BC_BGR888;
	case AV_PIX_FMT_ARGB:		return BC_ARGB8888;
	case AV_PIX_FMT_ABGR:		return BC_ABGR8888;
	case AV_PIX_FMT_RGB8:		return BC_RGB8;
	case AV_PIX_FMT_YUV420P:	return BC_YUV420P;
	case AV_PIX_FMT_YUV422P:	return BC_YUV422P;
	case AV_PIX_FMT_YUV444P:	return BC_YUV444P;
	case AV_PIX_FMT_YUV411P:	return BC_YUV411P;
	case AV_PIX_FMT_RGB565:		return BC_RGB565;
	case AV_PIX_FMT_RGB48LE:	return BC_RGB161616;
	case AV_PIX_FMT_RGBA64LE:	return BC_RGBA16161616;
	case AV_PIX_FMT_AYUV64LE:	return BC_AYUV16161616;
	case AV_PIX_FMT_GBRP:		return BC_GBRP;
	default: break;
	}

	return -1;
}

int FFVideoConvert::convert_picture_vframe(VFrame *frame, AVFrame *ip)
{
	AVFrame *ipic = av_frame_alloc();
	int ret = convert_picture_vframe(frame, ip, ipic);
	av_frame_free(&ipic);
	return ret;
}

int FFVideoConvert::convert_picture_vframe(VFrame *frame, AVFrame *ip, AVFrame *ipic)
{
	int cmodel = frame->get_color_model();
	AVPixelFormat ofmt = color_model_to_pix_fmt(cmodel);
	if( ofmt == AV_PIX_FMT_NB ) return -1;
	int size = av_image_fill_arrays(ipic->data, ipic->linesize,
		frame->get_data(), ofmt, frame->get_w(), frame->get_h(), 1);
	if( size < 0 ) return -1;

	int bpp = BC_CModels::calculate_pixelsize(cmodel);
	int ysz = bpp * frame->get_w(), usz = ysz;
	switch( cmodel ) {
	case BC_YUV410P:
	case BC_YUV411P:
		usz /= 2;
	case BC_YUV420P:
	case BC_YUV422P:
		usz /= 2;
	case BC_YUV444P:
	case BC_GBRP:
		// override av_image_fill_arrays() for planar types
		ipic->data[0] = frame->get_y();  ipic->linesize[0] = ysz;
		ipic->data[1] = frame->get_u();  ipic->linesize[1] = usz;
		ipic->data[2] = frame->get_v();  ipic->linesize[2] = usz;
		break;
	default:
		ipic->data[0] = frame->get_data();
		ipic->linesize[0] = frame->get_bytes_per_line();
		break;
	}

	AVPixelFormat pix_fmt = (AVPixelFormat)ip->format;
	convert_ctx = sws_getCachedContext(convert_ctx, ip->width, ip->height, pix_fmt,
		frame->get_w(), frame->get_h(), ofmt, SWS_POINT, NULL, NULL, NULL);
	if( !convert_ctx ) {
		fprintf(stderr, "FFVideoConvert::convert_picture_frame:"
				" sws_getCachedContext() failed\n");
		return -1;
	}
	int ret = sws_scale(convert_ctx, ip->data, ip->linesize, 0, ip->height,
	    ipic->data, ipic->linesize);
	if( ret < 0 ) {
		ff_err(ret, "FFVideoConvert::convert_picture_frame: sws_scale() failed\n");
		return -1;
	}
	return 0;
}

int FFVideoConvert::convert_cmodel(VFrame *frame, AVFrame *ip)
{
	// try direct transfer
	if( !convert_picture_vframe(frame, ip) ) return 1;
	// use indirect transfer
	AVPixelFormat ifmt = (AVPixelFormat)ip->format;
	const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(ifmt);
	int max_bits = 0;
	for( int i = 0; i <desc->nb_components; ++i ) {
		int bits = desc->comp[i].depth;
		if( bits > max_bits ) max_bits = bits;
	}
	int imodel = pix_fmt_to_color_model(ifmt);
	int imodel_is_yuv = BC_CModels::is_yuv(imodel);
	int cmodel = frame->get_color_model();
	int cmodel_is_yuv = BC_CModels::is_yuv(cmodel);
	if( imodel < 0 || imodel_is_yuv != cmodel_is_yuv ) {
		imodel = cmodel_is_yuv ?
		    (BC_CModels::has_alpha(cmodel) ?
			BC_AYUV16161616 :
			(max_bits > 8 ? BC_AYUV16161616 : BC_YUV444P)) :
		    (BC_CModels::has_alpha(cmodel) ?
			(max_bits > 8 ? BC_RGBA16161616 : BC_RGBA8888) :
			(max_bits > 8 ? BC_RGB161616 : BC_RGB888)) ;
	}
	VFrame vframe(ip->width, ip->height, imodel);
	if( convert_picture_vframe(&vframe, ip) ) return -1;
	frame->transfer_from(&vframe);
	return 1;
}

int FFVideoConvert::transfer_cmodel(VFrame *frame, AVFrame *ifp)
{
	int ret = convert_cmodel(frame, ifp);
	if( ret > 0 ) {
		const AVDictionary *src = ifp->metadata;
		AVDictionaryEntry *t = NULL;
		BC_Hash *hp = frame->get_params();
		//hp->clear();
		while( (t=av_dict_get(src, "", t, AV_DICT_IGNORE_SUFFIX)) )
			hp->update(t->key, t->value);
	}
	return ret;
}

int FFVideoConvert::convert_vframe_picture(VFrame *frame, AVFrame *op)
{
	AVFrame *opic = av_frame_alloc();
	int ret = convert_vframe_picture(frame, op, opic);
	av_frame_free(&opic);
	return ret;
}

int FFVideoConvert::convert_vframe_picture(VFrame *frame, AVFrame *op, AVFrame *opic)
{
	int cmodel = frame->get_color_model();
	AVPixelFormat ifmt = color_model_to_pix_fmt(cmodel);
	if( ifmt == AV_PIX_FMT_NB ) return -1;
	int size = av_image_fill_arrays(opic->data, opic->linesize,
		 frame->get_data(), ifmt, frame->get_w(), frame->get_h(), 1);
	if( size < 0 ) return -1;

	int bpp = BC_CModels::calculate_pixelsize(cmodel);
	int ysz = bpp * frame->get_w(), usz = ysz;
	switch( cmodel ) {
	case BC_YUV410P:
	case BC_YUV411P:
		usz /= 2;
	case BC_YUV420P:
	case BC_YUV422P:
		usz /= 2;
	case BC_YUV444P:
	case BC_GBRP:
		// override av_image_fill_arrays() for planar types
		opic->data[0] = frame->get_y();  opic->linesize[0] = ysz;
		opic->data[1] = frame->get_u();  opic->linesize[1] = usz;
		opic->data[2] = frame->get_v();  opic->linesize[2] = usz;
		break;
	default:
		opic->data[0] = frame->get_data();
		opic->linesize[0] = frame->get_bytes_per_line();
		break;
	}

	AVPixelFormat ofmt = (AVPixelFormat)op->format;
	convert_ctx = sws_getCachedContext(convert_ctx, frame->get_w(), frame->get_h(),
		ifmt, op->width, op->height, ofmt, SWS_POINT, NULL, NULL, NULL);
	if( !convert_ctx ) {
		fprintf(stderr, "FFVideoConvert::convert_frame_picture:"
				" sws_getCachedContext() failed\n");
		return -1;
	}
	int ret = sws_scale(convert_ctx, opic->data, opic->linesize, 0, frame->get_h(),
			op->data, op->linesize);
	if( ret < 0 ) {
		ff_err(ret, "FFVideoConvert::convert_frame_picture: sws_scale() failed\n");
		return -1;
	}
	return 0;
}

int FFVideoConvert::convert_pixfmt(VFrame *frame, AVFrame *op)
{
	// try direct transfer
	if( !convert_vframe_picture(frame, op) ) return 1;
	// use indirect transfer
	int cmodel = frame->get_color_model();
	int max_bits = BC_CModels::calculate_pixelsize(cmodel) * 8;
	max_bits /= BC_CModels::components(cmodel);
	AVPixelFormat ofmt = (AVPixelFormat)op->format;
	int imodel = pix_fmt_to_color_model(ofmt);
	int imodel_is_yuv = BC_CModels::is_yuv(imodel);
	int cmodel_is_yuv = BC_CModels::is_yuv(cmodel);
	if( imodel < 0 || imodel_is_yuv != cmodel_is_yuv ) {
		imodel = cmodel_is_yuv ?
		    (BC_CModels::has_alpha(cmodel) ?
			BC_AYUV16161616 :
			(max_bits > 8 ? BC_AYUV16161616 : BC_YUV444P)) :
		    (BC_CModels::has_alpha(cmodel) ?
			(max_bits > 8 ? BC_RGBA16161616 : BC_RGBA8888) :
			(max_bits > 8 ? BC_RGB161616 : BC_RGB888)) ;
	}
	VFrame vframe(frame->get_w(), frame->get_h(), imodel);
	vframe.transfer_from(frame);
	if( !convert_vframe_picture(&vframe, op) ) return 1;
	return -1;
}

int FFVideoConvert::transfer_pixfmt(VFrame *frame, AVFrame *ofp)
{
	int ret = convert_pixfmt(frame, ofp);
	if( ret > 0 ) {
		BC_Hash *hp = frame->get_params();
		AVDictionary **dict = &ofp->metadata;
		//av_dict_free(dict);
		for( int i=0; i<hp->size(); ++i ) {
			char *key = hp->get_key(i), *val = hp->get_value(i);
			av_dict_set(dict, key, val, 0);
		}
	}
	return ret;
}

void FFVideoStream::load_markers()
{
	IndexState *index_state = ffmpeg->file_base->asset->index_state;
	if( !index_state || idx >= index_state->video_markers.size() ) return;
	FFStream::load_markers(*index_state->video_markers[idx], frame_rate);
}

IndexMarks *FFVideoStream::get_markers()
{
	IndexState *index_state = ffmpeg->file_base->asset->index_state;
	if( !index_state || idx >= index_state->video_markers.size() ) return 0;
	return !index_state ? 0 : index_state->video_markers[idx];
}


FFMPEG::FFMPEG(FileBase *file_base)
{
	fmt_ctx = 0;
	this->file_base = file_base;
	memset(file_format,0,sizeof(file_format));
	mux_lock = new Condition(0,"FFMPEG::mux_lock",0);
	flow_lock = new Condition(1,"FFStream::flow_lock",0);
	done = -1;
	flow = 1;
	decoding = encoding = 0;
	has_audio = has_video = 0;
	opts = 0;
	opt_duration = -1;
	opt_video_filter = 0;
	opt_audio_filter = 0;
	fflags = 0;
	char option_path[BCTEXTLEN];
	set_option_path(option_path, "%s", "ffmpeg.opts");
	read_options(option_path, opts);
}

FFMPEG::~FFMPEG()
{
	ff_lock("FFMPEG::~FFMPEG()");
	close_encoder();
	ffaudio.remove_all_objects();
	ffvideo.remove_all_objects();
	if( fmt_ctx ) avformat_close_input(&fmt_ctx);
	ff_unlock();
	delete flow_lock;
	delete mux_lock;
	av_dict_free(&opts);
	delete [] opt_video_filter;
	delete [] opt_audio_filter;
}

int FFMPEG::check_sample_rate(AVCodec *codec, int sample_rate)
{
	const int *p = codec->supported_samplerates;
	if( !p ) return sample_rate;
	while( *p != 0 ) {
		if( *p == sample_rate ) return *p;
		++p;
	}
	return 0;
}

static inline AVRational std_frame_rate(int i)
{
	static const int m1 = 1001*12, m2 = 1000*12;
	static const int freqs[] = {
		40*m1, 48*m1, 50*m1, 60*m1, 80*m1,120*m1, 240*m1,
		24*m2, 30*m2, 60*m2, 12*m2, 15*m2, 48*m2, 0,
	};
	int freq = i<30*12 ? (i+1)*1001 : freqs[i-30*12];
	return (AVRational) { freq, 1001*12 };
}

AVRational FFMPEG::check_frame_rate(AVCodec *codec, double frame_rate)
{
	const AVRational *p = codec->supported_framerates;
	AVRational rate, best_rate = (AVRational) { 0, 0 };
	double max_err = 1.;  int i = 0;
	while( ((p ? (rate=*p++) : (rate=std_frame_rate(i++))), rate.num) != 0 ) {
		double framerate = (double) rate.num / rate.den;
		double err = fabs(frame_rate/framerate - 1.);
		if( err >= max_err ) continue;
		max_err = err;
		best_rate = rate;
	}
	return max_err < 0.0001 ? best_rate : (AVRational) { 0, 0 };
}

AVRational FFMPEG::to_sample_aspect_ratio(Asset *asset)
{
#if 1
	double display_aspect = asset->width / (double)asset->height;
	double sample_aspect = display_aspect / asset->aspect_ratio;
	int width = 1000000, height = width * sample_aspect + 0.5;
	float w, h;
	MWindow::create_aspect_ratio(w, h, width, height);
	return (AVRational){(int)w, (int)h};
#else
// square pixels
	return (AVRational){1, 1};
#endif
}

AVRational FFMPEG::to_time_base(int sample_rate)
{
	return (AVRational){1, sample_rate};
}

int FFMPEG::get_fmt_score(AVSampleFormat dst_fmt, AVSampleFormat src_fmt)
{
	int score = 0;
	int dst_planar = av_sample_fmt_is_planar(dst_fmt);
	int src_planar = av_sample_fmt_is_planar(src_fmt);
	if( dst_planar != src_planar ) ++score;
	int dst_bytes = av_get_bytes_per_sample(dst_fmt);
	int src_bytes = av_get_bytes_per_sample(src_fmt);
	score += (src_bytes > dst_bytes ? 100 : -10) * (src_bytes - dst_bytes);
	int src_packed = av_get_packed_sample_fmt(src_fmt);
	int dst_packed = av_get_packed_sample_fmt(dst_fmt);
	if( dst_packed == AV_SAMPLE_FMT_S32 && src_packed == AV_SAMPLE_FMT_FLT ) score += 20;
	if( dst_packed == AV_SAMPLE_FMT_FLT && src_packed == AV_SAMPLE_FMT_S32 ) score += 2;
	return score;
}

AVSampleFormat FFMPEG::find_best_sample_fmt_of_list(
		const AVSampleFormat *sample_fmts, AVSampleFormat src_fmt)
{
	AVSampleFormat best = AV_SAMPLE_FMT_NONE;
	int best_score = get_fmt_score(best, src_fmt);
	for( int i=0; sample_fmts[i] >= 0; ++i ) {
		AVSampleFormat sample_fmt = sample_fmts[i];
		int score = get_fmt_score(sample_fmt, src_fmt);
		if( score >= best_score ) continue;
		best = sample_fmt;  best_score = score;
	}
	return best;
}


void FFMPEG::set_option_path(char *path, const char *fmt, ...)
{
	char *ep = path + BCTEXTLEN-1;
	strncpy(path, File::get_cindat_path(), ep-path);
	strncat(path, "/ffmpeg/", ep-path);
	path += strlen(path);
	va_list ap;
	va_start(ap, fmt);
	path += vsnprintf(path, ep-path, fmt, ap);
	va_end(ap);
	*path = 0;
}

void FFMPEG::get_option_path(char *path, const char *type, const char *spec)
{
	if( *spec == '/' )
		strcpy(path, spec);
	else
		set_option_path(path, "%s/%s", type, spec);
}

int FFMPEG::get_format(char *format, const char *path, const char *spec)
{
	char option_path[BCTEXTLEN], line[BCTEXTLEN], codec[BCTEXTLEN];
	get_option_path(option_path, path, spec);
	FILE *fp = fopen(option_path,"r");
	if( !fp ) return 1;
	int ret = 0;
	if( !fgets(line, sizeof(line), fp) ) ret = 1;
	if( !ret ) {
		line[sizeof(line)-1] = 0;
		ret = scan_option_line(line, format, codec);
	}
	fclose(fp);
	return ret;
}

int FFMPEG::get_codec(char *codec, const char *path, const char *spec)
{
	char option_path[BCTEXTLEN], line[BCTEXTLEN], format[BCTEXTLEN];
	get_option_path(option_path, path, spec);
	FILE *fp = fopen(option_path,"r");
	if( !fp ) return 1;
	int ret = 0;
	if( !fgets(line, sizeof(line), fp) ) ret = 1;
	fclose(fp);
	if( !ret ) {
		line[sizeof(line)-1] = 0;
		ret = scan_option_line(line, format, codec);
	}
	if( !ret ) {
		char *vp = codec, *ep = vp+BCTEXTLEN-1;
		while( vp < ep && *vp && *vp != '|' ) ++vp;
		if( *vp == '|' ) --vp;
		while( vp > codec && (*vp==' ' || *vp=='\t') ) *vp-- = 0;
	}
	return ret;
}

int FFMPEG::get_file_format()
{
	char audio_muxer[BCSTRLEN], video_muxer[BCSTRLEN];
	char audio_format[BCSTRLEN], video_format[BCSTRLEN];
	audio_muxer[0] = audio_format[0] = 0;
	video_muxer[0] = video_format[0] = 0;
	Asset *asset = file_base->asset;
	int ret = asset ? 0 : 1;
	if( !ret && asset->audio_data ) {
		if( !(ret=get_format(audio_format, "audio", asset->acodec)) ) {
			if( get_format(audio_muxer, "format", audio_format) ) {
				strcpy(audio_muxer, audio_format);
				audio_format[0] = 0;
			}
		}
	}
	if( !ret && asset->video_data ) {
		if( !(ret=get_format(video_format, "video", asset->vcodec)) ) {
			if( get_format(video_muxer, "format", video_format) ) {
				strcpy(video_muxer, video_format);
				video_format[0] = 0;
			}
		}
	}
	if( !ret && !audio_muxer[0] && !video_muxer[0] )
		ret = 1;
	if( !ret && audio_muxer[0] && video_muxer[0] &&
	    strcmp(audio_muxer, video_muxer) ) ret = -1;
	if( !ret && audio_format[0] && video_format[0] &&
	    strcmp(audio_format, video_format) ) ret = -1;
	if( !ret )
		strcpy(file_format, !audio_format[0] && !video_format[0] ?
			(audio_muxer[0] ? audio_muxer : video_muxer) :
			(audio_format[0] ? audio_format : video_format));
	return ret;
}

int FFMPEG::scan_option_line(const char *cp, char *tag, char *val)
{
	while( *cp == ' ' || *cp == '\t' ) ++cp;
	const char *bp = cp;
	while( *cp && *cp != ' ' && *cp != '\t' && *cp != '=' && *cp != '\n' ) ++cp;
	int len = cp - bp;
	if( !len || len > BCSTRLEN-1 ) return 1;
	while( bp < cp ) *tag++ = *bp++;
	*tag = 0;
	while( *cp == ' ' || *cp == '\t' ) ++cp;
	if( *cp == '=' ) ++cp;
	while( *cp == ' ' || *cp == '\t' ) ++cp;
	bp = cp;
	while( *cp && *cp != '\n' ) ++cp;
	len = cp - bp;
	if( len > BCTEXTLEN-1 ) return 1;
	while( bp < cp ) *val++ = *bp++;
	*val = 0;
	return 0;
}

int FFMPEG::can_render(const char *fformat, const char *type)
{
	FileSystem fs;
	char option_path[BCTEXTLEN];
	FFMPEG::set_option_path(option_path, type);
	fs.update(option_path);
	int total_files = fs.total_files();
	for( int i=0; i<total_files; ++i ) {
		const char *name = fs.get_entry(i)->get_name();
		const char *ext = strrchr(name,'.');
		if( !ext ) continue;
		if( !strcmp(fformat, ++ext) ) return 1;
	}
	return 0;
}

int FFMPEG::get_ff_option(const char *nm, const char *options, char *value)
{
        for( const char *cp=options; *cp!=0; ) {
                char line[BCTEXTLEN], *bp = line, *ep = bp+sizeof(line)-1;
                while( bp < ep && *cp && *cp!='\n' ) *bp++ = *cp++;
                if( *cp ) ++cp;
                *bp = 0;
                if( !line[0] || line[0] == '#' || line[0] == ';' ) continue;
                char key[BCSTRLEN], val[BCTEXTLEN];
                if( FFMPEG::scan_option_line(line, key, val) ) continue;
                if( !strcmp(key, nm) ) {
                        strncpy(value, val, BCSTRLEN);
                        return 0;
                }
        }
        return 1;
}

void FFMPEG::scan_audio_options(Asset *asset, EDL *edl)
{
	char cin_sample_fmt[BCSTRLEN];
	int cin_fmt = AV_SAMPLE_FMT_NONE;
	const char *options = asset->ff_audio_options;
	if( !get_ff_option("cin_sample_fmt", options, cin_sample_fmt) )
		cin_fmt = (int)av_get_sample_fmt(cin_sample_fmt);
	if( cin_fmt < 0 ) {
		char audio_codec[BCSTRLEN]; audio_codec[0] = 0;
		AVCodec *av_codec = !FFMPEG::get_codec(audio_codec, "audio", asset->acodec) ?
			avcodec_find_encoder_by_name(audio_codec) : 0;
		if( av_codec && av_codec->sample_fmts )
			cin_fmt = find_best_sample_fmt_of_list(av_codec->sample_fmts, AV_SAMPLE_FMT_FLT);
	}
	if( cin_fmt < 0 ) cin_fmt = AV_SAMPLE_FMT_S16;
	const char *name = av_get_sample_fmt_name((AVSampleFormat)cin_fmt);
	if( !name ) name = _("None");
	strcpy(asset->ff_sample_format, name);

	char value[BCSTRLEN];
	if( !get_ff_option("cin_bitrate", options, value) )
		asset->ff_audio_bitrate = atoi(value);
	if( !get_ff_option("cin_quality", options, value) )
		asset->ff_audio_quality = atoi(value);
}

void FFMPEG::load_audio_options(Asset *asset, EDL *edl)
{
	char options_path[BCTEXTLEN];
	set_option_path(options_path, "audio/%s", asset->acodec);
        if( !load_options(options_path,
			asset->ff_audio_options,
			sizeof(asset->ff_audio_options)) )
		scan_audio_options(asset, edl);
}

void FFMPEG::scan_video_options(Asset *asset, EDL *edl)
{
	char cin_pix_fmt[BCSTRLEN];
	int cin_fmt = AV_PIX_FMT_NONE;
	const char *options = asset->ff_video_options;
	if( !get_ff_option("cin_pix_fmt", options, cin_pix_fmt) )
			cin_fmt = (int)av_get_pix_fmt(cin_pix_fmt);
	if( cin_fmt < 0 ) {
		char video_codec[BCSTRLEN];  video_codec[0] = 0;
		AVCodec *av_codec = !get_codec(video_codec, "video", asset->vcodec) ?
			avcodec_find_encoder_by_name(video_codec) : 0;
		if( av_codec && av_codec->pix_fmts ) {
			if( 0 && edl ) { // frequently picks a bad answer
				int color_model = edl->session->color_model;
				int max_bits = BC_CModels::calculate_pixelsize(color_model) * 8;
				max_bits /= BC_CModels::components(color_model);
				cin_fmt = avcodec_find_best_pix_fmt_of_list(av_codec->pix_fmts,
					(BC_CModels::is_yuv(color_model) ?
						(max_bits > 8 ? AV_PIX_FMT_AYUV64LE : AV_PIX_FMT_YUV444P) :
						(max_bits > 8 ? AV_PIX_FMT_RGB48LE : AV_PIX_FMT_RGB24)), 0, 0);
			}
			else
				cin_fmt = av_codec->pix_fmts[0];
		}
	}
	if( cin_fmt < 0 ) cin_fmt = AV_PIX_FMT_YUV420P;
	const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get((AVPixelFormat)cin_fmt);
	const char *name = desc ? desc->name : _("None");
	strcpy(asset->ff_pixel_format, name);

	char value[BCSTRLEN];
	if( !get_ff_option("cin_bitrate", options, value) )
		asset->ff_video_bitrate = atoi(value);
	if( !get_ff_option("cin_quality", options, value) )
		asset->ff_video_quality = atoi(value);
}

void FFMPEG::load_video_options(Asset *asset, EDL *edl)
{
	char options_path[BCTEXTLEN];
	set_option_path(options_path, "video/%s", asset->vcodec);
        if( !load_options(options_path,
			asset->ff_video_options,
			sizeof(asset->ff_video_options)) )
		scan_video_options(asset, edl);
}

int FFMPEG::load_defaults(const char *path, const char *type,
		 char *codec, char *codec_options, int len)
{
	char default_file[BCTEXTLEN];
	set_option_path(default_file, "%s/%s.dfl", path, type);
	FILE *fp = fopen(default_file,"r");
	if( !fp ) return 1;
	fgets(codec, BCSTRLEN, fp);
	char *cp = codec;
	while( *cp && *cp!='\n' ) ++cp;
	*cp = 0;
	while( len > 0 && fgets(codec_options, len, fp) ) {
		int n = strlen(codec_options);
		codec_options += n;  len -= n;
	}
	fclose(fp);
	set_option_path(default_file, "%s/%s", path, codec);
	return load_options(default_file, codec_options, len);
}

void FFMPEG::set_asset_format(Asset *asset, EDL *edl, const char *text)
{
	if( asset->format != FILE_FFMPEG ) return;
	if( text != asset->fformat )
		strcpy(asset->fformat, text);
	if( asset->audio_data && !asset->ff_audio_options[0] ) {
		if( !load_defaults("audio", text, asset->acodec,
				asset->ff_audio_options, sizeof(asset->ff_audio_options)) )
			scan_audio_options(asset, edl);
		else
			asset->audio_data = 0;
	}
	if( asset->video_data && !asset->ff_video_options[0] ) {
		if( !load_defaults("video", text, asset->vcodec,
				asset->ff_video_options, sizeof(asset->ff_video_options)) )
			scan_video_options(asset, edl);
		else
			asset->video_data = 0;
	}
}

int FFMPEG::get_encoder(const char *options,
		char *format, char *codec, char *bsfilter)
{
	FILE *fp = fopen(options,"r");
	if( !fp ) {
		eprintf(_("options open failed %s\n"),options);
		return 1;
	}
	char line[BCTEXTLEN];
	if( !fgets(line, sizeof(line), fp) ||
	    scan_encoder(line, format, codec, bsfilter) )
		eprintf(_("format/codec not found %s\n"), options);
	fclose(fp);
	return 0;
}

int FFMPEG::scan_encoder(const char *line,
		char *format, char *codec, char *bsfilter)
{
	format[0] = codec[0] = bsfilter[0] = 0;
	if( scan_option_line(line, format, codec) ) return 1;
	char *cp = codec;
	while( *cp && *cp != '|' ) ++cp;
	if( !*cp ) return 0;
	char *bp = cp;
	do { *bp-- = 0; } while( bp>=codec && (*bp==' ' || *bp == '\t' ) );
	while( *++cp && (*cp==' ' || *cp == '\t') );
	bp = bsfilter;
	for( int i=BCTEXTLEN; --i>0 && *cp; ) *bp++ = *cp++;
	*bp = 0;
	return 0;
}

int FFMPEG::read_options(const char *options, AVDictionary *&opts, int skip)
{
	FILE *fp = fopen(options,"r");
	if( !fp ) return 1;
	int ret = 0;
	while( !ret && --skip >= 0 ) {
		int ch = getc(fp);
		while( ch >= 0 && ch != '\n' ) ch = getc(fp);
		if( ch < 0 ) ret = 1;
	}
	if( !ret )
		ret = read_options(fp, options, opts);
	fclose(fp);
	return ret;
}

int FFMPEG::scan_options(const char *options, AVDictionary *&opts, AVStream *st)
{
	FILE *fp = fmemopen((void *)options,strlen(options),"r");
	if( !fp ) return 0;
	int ret = read_options(fp, options, opts);
	fclose(fp);
	AVDictionaryEntry *tag = av_dict_get(opts, "id", NULL, 0);
	if( tag ) st->id = strtol(tag->value,0,0);
	return ret;
}

int FFMPEG::read_options(FILE *fp, const char *options, AVDictionary *&opts)
{
	int ret = 0, no = 0;
	char line[BCTEXTLEN];
	while( !ret && fgets(line, sizeof(line), fp) ) {
		line[sizeof(line)-1] = 0;
		if( line[0] == '#' ) continue;
		if( line[0] == '\n' ) continue;
		char key[BCSTRLEN], val[BCTEXTLEN];
		if( scan_option_line(line, key, val) ) {
			eprintf(_("err reading %s: line %d\n"), options, no);
			ret = 1;
		}
		if( !ret ) {
			if( !strcmp(key, "duration") )
				opt_duration = strtod(val, 0);
			else if( !strcmp(key, "video_filter") )
				opt_video_filter = cstrdup(val);
			else if( !strcmp(key, "audio_filter") )
				opt_audio_filter = cstrdup(val);
			else if( !strcmp(key, "loglevel") )
				set_loglevel(val);
			else
				av_dict_set(&opts, key, val, 0);
		}
	}
	return ret;
}

int FFMPEG::load_options(const char *options, AVDictionary *&opts)
{
	char option_path[BCTEXTLEN];
	set_option_path(option_path, "%s", options);
	return read_options(option_path, opts);
}

int FFMPEG::load_options(const char *path, char *bfr, int len)
{
	*bfr = 0;
	FILE *fp = fopen(path, "r");
	if( !fp ) return 1;
	fgets(bfr, len, fp); // skip hdr
	len = fread(bfr, 1, len-1, fp);
	if( len < 0 ) len = 0;
	bfr[len] = 0;
	fclose(fp);
	return 0;
}

void FFMPEG::set_loglevel(const char *ap)
{
	if( !ap || !*ap ) return;
	const struct {
		const char *name;
		int level;
	} log_levels[] = {
		{ "quiet"  , AV_LOG_QUIET   },
		{ "panic"  , AV_LOG_PANIC   },
		{ "fatal"  , AV_LOG_FATAL   },
		{ "error"  , AV_LOG_ERROR   },
		{ "warning", AV_LOG_WARNING },
		{ "info"   , AV_LOG_INFO    },
		{ "verbose", AV_LOG_VERBOSE },
		{ "debug"  , AV_LOG_DEBUG   },
	};
	for( int i=0; i<(int)(sizeof(log_levels)/sizeof(log_levels[0])); ++i ) {
		if( !strcmp(log_levels[i].name, ap) ) {
			av_log_set_level(log_levels[i].level);
			return;
		}
	}
	av_log_set_level(atoi(ap));
}

double FFMPEG::to_secs(int64_t time, AVRational time_base)
{
	double base_time = time == AV_NOPTS_VALUE ? 0 :
		av_rescale_q(time, time_base, AV_TIME_BASE_Q);
	return base_time / AV_TIME_BASE;
}

int FFMPEG::info(char *text, int len)
{
	if( len <= 0 ) return 0;
	decode_activate();
#define report(s...) do { int n = snprintf(cp,len,s); cp += n;  len -= n; } while(0)
	char *cp = text;
	report("format: %s\n",fmt_ctx->iformat->name);
	if( ffvideo.size() > 0 )
		report("\n%d video stream%s\n",ffvideo.size(), ffvideo.size()!=1 ? "s" : "");
	for( int vidx=0; vidx<ffvideo.size(); ++vidx ) {
		FFVideoStream *vid = ffvideo[vidx];
		AVStream *st = vid->st;
		AVCodecID codec_id = st->codecpar->codec_id;
		report(_("vid%d (%d),  id 0x%06x:\n"), vid->idx, vid->fidx, codec_id);
		const AVCodecDescriptor *desc = avcodec_descriptor_get(codec_id);
		report("  video%d %s", vidx+1, desc ? desc->name : " (unkn)");
		report(" %dx%d %5.2f", vid->width, vid->height, vid->frame_rate);
		AVPixelFormat pix_fmt = (AVPixelFormat)st->codecpar->format;
		const char *pfn = av_get_pix_fmt_name(pix_fmt);
		report(" pix %s\n", pfn ? pfn : "(unkn)");
		double secs = to_secs(st->duration, st->time_base);
		int64_t length = secs * vid->frame_rate + 0.5;
		double ofs = to_secs((vid->nudge - st->start_time), st->time_base);
		int64_t nudge = ofs * vid->frame_rate;
		int ch = nudge >= 0 ? '+' : (nudge=-nudge, '-');
		report("    %jd%c%jd frms %0.2f secs", length,ch,nudge, secs);
		int hrs = secs/3600;  secs -= hrs*3600;
		int mins = secs/60;  secs -= mins*60;
		report("  %d:%02d:%05.2f\n", hrs, mins, secs);
	}
	if( ffaudio.size() > 0 )
		report("\n%d audio stream%s\n",ffaudio.size(), ffaudio.size()!=1 ? "s" : "");
	for( int aidx=0; aidx<ffaudio.size(); ++aidx ) {
		FFAudioStream *aud = ffaudio[aidx];
		AVStream *st = aud->st;
		AVCodecID codec_id = st->codecpar->codec_id;
		report(_("aud%d (%d),  id 0x%06x:\n"), aud->idx, aud->fidx, codec_id);
		const AVCodecDescriptor *desc = avcodec_descriptor_get(codec_id);
		int nch = aud->channels, ch0 = aud->channel0+1;
		report("  audio%d-%d %s", ch0, ch0+nch-1, desc ? desc->name : " (unkn)");
		AVSampleFormat sample_fmt = (AVSampleFormat)st->codecpar->format;
		const char *fmt = av_get_sample_fmt_name(sample_fmt);
		report(" %s %d", fmt, aud->sample_rate);
		int sample_bits = av_get_bits_per_sample(codec_id);
		report(" %dbits\n", sample_bits);
		double secs = to_secs(st->duration, st->time_base);
		int64_t length = secs * aud->sample_rate + 0.5;
		double ofs = to_secs((aud->nudge - st->start_time), st->time_base);
		int64_t nudge = ofs * aud->sample_rate;
		int ch = nudge >= 0 ? '+' : (nudge=-nudge, '-');
		report("    %jd%c%jd smpl %0.2f secs", length,ch,nudge, secs);
		int hrs = secs/3600;  secs -= hrs*3600;
		int mins = secs/60;  secs -= mins*60;
		report("  %d:%02d:%05.2f\n", hrs, mins, secs);
	}
	if( fmt_ctx->nb_programs > 0 )
		report("\n%d program%s\n",fmt_ctx->nb_programs, fmt_ctx->nb_programs!=1 ? "s" : "");
	for( int i=0; i<(int)fmt_ctx->nb_programs; ++i ) {
		report("program %d", i+1);
		AVProgram *pgrm = fmt_ctx->programs[i];
		for( int j=0; j<(int)pgrm->nb_stream_indexes; ++j ) {
			int idx = pgrm->stream_index[j];
			int vidx = ffvideo.size();
			while( --vidx>=0 && ffvideo[vidx]->fidx != idx );
			if( vidx >= 0 ) {
				report(", vid%d", vidx);
				continue;
			}
			int aidx = ffaudio.size();
			while( --aidx>=0 && ffaudio[aidx]->fidx != idx );
			if( aidx >= 0 ) {
				report(", aud%d", aidx);
				continue;
			}
			report(", (%d)", pgrm->stream_index[j]);
		}
		report("\n");
	}
	report("\n");
	AVDictionaryEntry *tag = 0;
	while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
		report("%s=%s\n", tag->key, tag->value);

	if( !len ) --cp;
	*cp = 0;
	return cp - text;
#undef report
}


int FFMPEG::init_decoder(const char *filename)
{
	ff_lock("FFMPEG::init_decoder");
	av_register_all();
	char file_opts[BCTEXTLEN];
	char *bp = strrchr(strcpy(file_opts, filename), '/');
	char *sp = strrchr(!bp ? file_opts : bp, '.');
	if( !sp ) sp = bp + strlen(bp);
	FILE *fp = 0;
	AVInputFormat *ifmt = 0;
	if( sp ) {
		strcpy(sp, ".opts");
		fp = fopen(file_opts, "r");
	}
	if( fp ) {
		read_options(fp, file_opts, opts);
		fclose(fp);
		AVDictionaryEntry *tag;
		if( (tag=av_dict_get(opts, "format", NULL, 0)) != 0 ) {
			ifmt = av_find_input_format(tag->value);
		}
	}
	else
		load_options("decode.opts", opts);
	AVDictionary *fopts = 0;
	av_dict_copy(&fopts, opts, 0);
	int ret = avformat_open_input(&fmt_ctx, filename, ifmt, &fopts);
	av_dict_free(&fopts);
	if( ret >= 0 )
		ret = avformat_find_stream_info(fmt_ctx, NULL);
	if( !ret ) {
		decoding = -1;
	}
	ff_unlock();
	return !ret ? 0 : 1;
}

int FFMPEG::open_decoder()
{
	struct stat st;
	if( stat(fmt_ctx->url, &st) < 0 ) {
		eprintf(_("can't stat file: %s\n"), fmt_ctx->url);
		return 1;
	}

	int64_t file_bits = 8 * st.st_size;
	if( !fmt_ctx->bit_rate && opt_duration > 0 )
		fmt_ctx->bit_rate = file_bits / opt_duration;

	int estimated = 0;
	if( fmt_ctx->bit_rate > 0 ) {
		for( int i=0; i<(int)fmt_ctx->nb_streams; ++i ) {
			AVStream *st = fmt_ctx->streams[i];
			if( st->duration != AV_NOPTS_VALUE ) continue;
			if( st->time_base.num > INT64_MAX / fmt_ctx->bit_rate ) continue;
			st->duration = av_rescale(file_bits, st->time_base.den,
				fmt_ctx->bit_rate * (int64_t) st->time_base.num);
			estimated = 1;
		}
	}
	if( estimated && !(fflags & FF_ESTM_TIMES) ) {
		fflags |= FF_ESTM_TIMES;
		printf("FFMPEG::open_decoder: some stream times estimated: %s\n",
			fmt_ctx->url);
	}

	ff_lock("FFMPEG::open_decoder");
	int ret = 0, bad_time = 0;
	for( int i=0; !ret && i<(int)fmt_ctx->nb_streams; ++i ) {
		AVStream *st = fmt_ctx->streams[i];
		if( st->duration == AV_NOPTS_VALUE ) bad_time = 1;
		AVCodecParameters *avpar = st->codecpar;
		const AVCodecDescriptor *codec_desc = avcodec_descriptor_get(avpar->codec_id);
		if( !codec_desc ) continue;
		switch( avpar->codec_type ) {
		case AVMEDIA_TYPE_VIDEO: {
			if( avpar->width < 1 ) continue;
			if( avpar->height < 1 ) continue;
			AVRational framerate = av_guess_frame_rate(fmt_ctx, st, 0);
			if( framerate.num < 1 ) continue;
			has_video = 1;
			int vidx = ffvideo.size();
			FFVideoStream *vid = new FFVideoStream(this, st, vidx, i);
			vstrm_index.append(ffidx(vidx, 0));
			ffvideo.append(vid);
			vid->width = avpar->width;
			vid->height = avpar->height;
			vid->frame_rate = !framerate.den ? 0 : (double)framerate.num / framerate.den;
			double secs = to_secs(st->duration, st->time_base);
			vid->length = secs * vid->frame_rate;
			vid->aspect_ratio = (double)st->sample_aspect_ratio.num / st->sample_aspect_ratio.den;
			vid->nudge = st->start_time;
			vid->reading = -1;
			if( opt_video_filter )
				ret = vid->create_filter(opt_video_filter, avpar);
			break; }
		case AVMEDIA_TYPE_AUDIO: {
			if( avpar->channels < 1 ) continue;
			if( avpar->sample_rate < 1 ) continue;
			has_audio = 1;
			int aidx = ffaudio.size();
			FFAudioStream *aud = new FFAudioStream(this, st, aidx, i);
			ffaudio.append(aud);
			aud->channel0 = astrm_index.size();
			aud->channels = avpar->channels;
			for( int ch=0; ch<aud->channels; ++ch )
				astrm_index.append(ffidx(aidx, ch));
			aud->sample_rate = avpar->sample_rate;
			double secs = to_secs(st->duration, st->time_base);
			aud->length = secs * aud->sample_rate;
			aud->init_swr(aud->channels, avpar->format, aud->sample_rate);
			aud->nudge = st->start_time;
			aud->reading = -1;
			if( opt_audio_filter )
				ret = aud->create_filter(opt_audio_filter, avpar);
			break; }
		default: break;
		}
	}
	if( bad_time && !(fflags & FF_BAD_TIMES) ) {
		fflags |= FF_BAD_TIMES;
		printf("FFMPEG::open_decoder: some stream have bad times: %s\n",
			fmt_ctx->url);
	}
	ff_unlock();
	return ret < 0 ? -1 : 0;
}


int FFMPEG::init_encoder(const char *filename)
{
// try access first for named pipes
	int ret = access(filename, W_OK);
	if( ret ) {
		int fd = ::open(filename,O_WRONLY);
		if( fd < 0 ) fd = open(filename,O_WRONLY+O_CREAT,0666);
		if( fd >= 0 ) { close(fd);  ret = 0; }
	}
	if( ret ) {
		eprintf(_("bad file path: %s\n"), filename);
		return 1;
	}
	ret = get_file_format();
	if( ret > 0 ) {
		eprintf(_("bad file format: %s\n"), filename);
		return 1;
	}
	if( ret < 0 ) {
		eprintf(_("mismatch audio/video file format: %s\n"), filename);
		return 1;
	}
	ff_lock("FFMPEG::init_encoder");
	av_register_all();
	char format[BCSTRLEN];
	if( get_format(format, "format", file_format) )
		strcpy(format, file_format);
	avformat_alloc_output_context2(&fmt_ctx, 0, format, filename);
	if( !fmt_ctx ) {
		eprintf(_("failed: %s\n"), filename);
		ret = 1;
	}
	if( !ret ) {
		encoding = -1;
		load_options("encode.opts", opts);
	}
	ff_unlock();
	return ret;
}

int FFMPEG::open_encoder(const char *type, const char *spec)
{

	Asset *asset = file_base->asset;
	char *filename = asset->path;
	AVDictionary *sopts = 0;
	av_dict_copy(&sopts, opts, 0);
	char option_path[BCTEXTLEN];
	set_option_path(option_path, "%s/%s.opts", type, type);
	read_options(option_path, sopts);
	get_option_path(option_path, type, spec);
	char format_name[BCSTRLEN], codec_name[BCTEXTLEN], bsfilter[BCTEXTLEN];
	if( get_encoder(option_path, format_name, codec_name, bsfilter) ) {
		eprintf(_("get_encoder failed %s:%s\n"), option_path, filename);
		return 1;
	}

#ifdef HAVE_DV
	if( !strcmp(codec_name, CODEC_TAG_DVSD) ) strcpy(codec_name, "dv");
#endif
	else if( !strcmp(codec_name, CODEC_TAG_MJPEG) ) strcpy(codec_name, "mjpeg");
	else if( !strcmp(codec_name, CODEC_TAG_JPEG) ) strcpy(codec_name, "jpeg");

	int ret = 0;
	ff_lock("FFMPEG::open_encoder");
	FFStream *fst = 0;
	AVStream *st = 0;
	AVCodecContext *ctx = 0;

	const AVCodecDescriptor *codec_desc = 0;
	AVCodec *codec = avcodec_find_encoder_by_name(codec_name);
	if( !codec ) {
		eprintf(_("cant find codec %s:%s\n"), codec_name, filename);
		ret = 1;
	}
	if( !ret ) {
		codec_desc = avcodec_descriptor_get(codec->id);
		if( !codec_desc ) {
			eprintf(_("unknown codec %s:%s\n"), codec_name, filename);
			ret = 1;
		}
	}
	if( !ret ) {
		st = avformat_new_stream(fmt_ctx, 0);
		if( !st ) {
			eprintf(_("cant create stream %s:%s\n"), codec_name, filename);
			ret = 1;
		}
	}
	if( !ret ) {
		switch( codec_desc->type ) {
		case AVMEDIA_TYPE_AUDIO: {
			if( has_audio ) {
				eprintf(_("duplicate audio %s:%s\n"), codec_name, filename);
				ret = 1;
				break;
			}
			if( scan_options(asset->ff_audio_options, sopts, st) ) {
				eprintf(_("bad audio options %s:%s\n"), codec_name, filename);
				ret = 1;
				break;
			}
			has_audio = 1;
			ctx = avcodec_alloc_context3(codec);
			if( asset->ff_audio_bitrate > 0 ) {
				ctx->bit_rate = asset->ff_audio_bitrate;
				char arg[BCSTRLEN];
				sprintf(arg, "%d", asset->ff_audio_bitrate);
				av_dict_set(&sopts, "b", arg, 0);
			}
			else if( asset->ff_audio_quality >= 0 ) {
				ctx->global_quality = asset->ff_audio_quality * FF_QP2LAMBDA;
				ctx->qmin    = ctx->qmax =  asset->ff_audio_quality;
				ctx->mb_lmin = ctx->qmin * FF_QP2LAMBDA;
				ctx->mb_lmax = ctx->qmax * FF_QP2LAMBDA;
				ctx->flags |= AV_CODEC_FLAG_QSCALE;
				char arg[BCSTRLEN];
				av_dict_set(&sopts, "flags", "+qscale", 0);
				sprintf(arg, "%d", asset->ff_audio_quality);
				av_dict_set(&sopts, "qscale", arg, 0);
				sprintf(arg, "%d", ctx->global_quality);
				av_dict_set(&sopts, "global_quality", arg, 0);
			}
			int aidx = ffaudio.size();
			int fidx = aidx + ffvideo.size();
			FFAudioStream *aud = new FFAudioStream(this, st, aidx, fidx);
			aud->avctx = ctx;  ffaudio.append(aud);  fst = aud;
			aud->sample_rate = asset->sample_rate;
			ctx->channels = aud->channels = asset->channels;
			for( int ch=0; ch<aud->channels; ++ch )
				astrm_index.append(ffidx(aidx, ch));
			ctx->channel_layout =  av_get_default_channel_layout(ctx->channels);
			ctx->sample_rate = check_sample_rate(codec, asset->sample_rate);
			if( !ctx->sample_rate ) {
				eprintf(_("check_sample_rate failed %s\n"), filename);
				ret = 1;
				break;
			}
			ctx->time_base = st->time_base = (AVRational){1, aud->sample_rate};
			AVSampleFormat sample_fmt = av_get_sample_fmt(asset->ff_sample_format);
			if( sample_fmt == AV_SAMPLE_FMT_NONE )
				sample_fmt = codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_S16;
			ctx->sample_fmt = sample_fmt;
			uint64_t layout = av_get_default_channel_layout(ctx->channels);
			aud->resample_context = swr_alloc_set_opts(NULL,
				layout, ctx->sample_fmt, aud->sample_rate,
				layout, AV_SAMPLE_FMT_FLT, ctx->sample_rate,
				0, NULL);
			swr_init(aud->resample_context);
			aud->writing = -1;
			break; }
		case AVMEDIA_TYPE_VIDEO: {
			if( has_video ) {
				eprintf(_("duplicate video %s:%s\n"), codec_name, filename);
				ret = 1;
				break;
			}
			if( scan_options(asset->ff_video_options, sopts, st) ) {
				eprintf(_("bad video options %s:%s\n"), codec_name, filename);
				ret = 1;
				break;
			}
			has_video = 1;
			ctx = avcodec_alloc_context3(codec);
			if( asset->ff_video_bitrate > 0 ) {
				ctx->bit_rate = asset->ff_video_bitrate;
				char arg[BCSTRLEN];
				sprintf(arg, "%d", asset->ff_video_bitrate);
				av_dict_set(&sopts, "b", arg, 0);
			}
			else if( asset->ff_video_quality >= 0 ) {
				ctx->global_quality = asset->ff_video_quality * FF_QP2LAMBDA;
				ctx->qmin    = ctx->qmax =  asset->ff_video_quality;
				ctx->mb_lmin = ctx->qmin * FF_QP2LAMBDA;
				ctx->mb_lmax = ctx->qmax * FF_QP2LAMBDA;
				ctx->flags |= AV_CODEC_FLAG_QSCALE;
				char arg[BCSTRLEN];
				av_dict_set(&sopts, "flags", "+qscale", 0);
				sprintf(arg, "%d", asset->ff_video_quality);
				av_dict_set(&sopts, "qscale", arg, 0);
				sprintf(arg, "%d", ctx->global_quality);
				av_dict_set(&sopts, "global_quality", arg, 0);
			}
			int vidx = ffvideo.size();
			int fidx = vidx + ffaudio.size();
			FFVideoStream *vid = new FFVideoStream(this, st, vidx, fidx);
			vstrm_index.append(ffidx(vidx, 0));
			vid->avctx = ctx;  ffvideo.append(vid);  fst = vid;
			vid->width = asset->width;
			vid->height = asset->height;
			vid->frame_rate = asset->frame_rate;

			AVPixelFormat pix_fmt = av_get_pix_fmt(asset->ff_pixel_format);
			if( pix_fmt == AV_PIX_FMT_NONE )
				pix_fmt = codec->pix_fmts ? codec->pix_fmts[0] : AV_PIX_FMT_YUV420P;
			ctx->pix_fmt = pix_fmt;
			const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
			int mask_w = (1<<desc->log2_chroma_w)-1;
			ctx->width = (vid->width+mask_w) & ~mask_w;
			int mask_h = (1<<desc->log2_chroma_h)-1;
			ctx->height = (vid->height+mask_h) & ~mask_h;
			ctx->sample_aspect_ratio = to_sample_aspect_ratio(asset);
			AVRational frame_rate = check_frame_rate(codec, vid->frame_rate);
			if( !frame_rate.num || !frame_rate.den ) {
				eprintf(_("check_frame_rate failed %s\n"), filename);
				ret = 1;
				break;
			}
			av_reduce(&frame_rate.num, &frame_rate.den,
				frame_rate.num, frame_rate.den, INT_MAX);
			ctx->framerate = (AVRational) { frame_rate.num, frame_rate.den };
			ctx->time_base = (AVRational) { frame_rate.den, frame_rate.num };
			st->avg_frame_rate = frame_rate;
			st->time_base = ctx->time_base;
			vid->writing = -1;
			vid->interlaced = asset->interlace_mode == ILACE_MODE_TOP_FIRST ||
				asset->interlace_mode == ILACE_MODE_BOTTOM_FIRST ? 1 : 0;
			vid->top_field_first = asset->interlace_mode == ILACE_MODE_TOP_FIRST ? 1 : 0;
			break; }
		default:
			eprintf(_("not audio/video, %s:%s\n"), codec_name, filename);
			ret = 1;
		}

		if( ctx ) {
			AVDictionaryEntry *tag;
			if( (tag=av_dict_get(sopts, "cin_stats_filename", NULL, 0)) != 0 ) {
				char suffix[BCSTRLEN];  sprintf(suffix,"-%d.log",fst->fidx);
				fst->stats_filename = cstrcat(2, tag->value, suffix);
			}
			if( (tag=av_dict_get(sopts, "flags", NULL, 0)) != 0 ) {
				int pass = fst->pass;
				char *cp = tag->value;
				while( *cp ) {
					int ch = *cp++, pfx = ch=='-' ? -1 : ch=='+' ? 1 : 0;
					if( !isalnum(!pfx ? ch : (ch=*cp++)) ) continue;
					char id[BCSTRLEN], *bp = id, *ep = bp+sizeof(id)-1;
					for( *bp++=ch; isalnum(ch=*cp); ++cp )
						if( bp < ep ) *bp++ = ch;
					*bp = 0;
					if( !strcmp(id, "pass1") ) {
						pass = pfx<0 ? (pass&~1) : pfx>0 ? (pass|1) : 1;
					}
					else if( !strcmp(id, "pass2") ) {
						pass = pfx<0 ? (pass&~2) : pfx>0 ? (pass|2) : 2;
					}
				}
				if( (fst->pass=pass) ) {
					if( pass & 1 ) ctx->flags |= AV_CODEC_FLAG_PASS1;
					if( pass & 2 ) ctx->flags |= AV_CODEC_FLAG_PASS2;
				}
			}
		}
	}
	if( !ret ) {
		if( fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER )
			ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		if( fst->stats_filename && (ret=fst->init_stats_file()) )
			eprintf(_("error: stats file = %s\n"), fst->stats_filename);
	}
	if( !ret ) {
		av_dict_set(&sopts, "cin_bitrate", 0, 0);
		av_dict_set(&sopts, "cin_quality", 0, 0);

		if( !av_dict_get(sopts, "threads", NULL, 0) )
			ctx->thread_count = ff_cpus();
		ret = avcodec_open2(ctx, codec, &sopts);
		if( ret >= 0 ) {
			ret = avcodec_parameters_from_context(st->codecpar, ctx);
			if( ret < 0 )
				fprintf(stderr, "Could not copy the stream parameters\n");
		}
		if( ret >= 0 ) {
_Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
			ret = avcodec_copy_context(st->codec, ctx);
_Pragma("GCC diagnostic warning \"-Wdeprecated-declarations\"")
			if( ret < 0 )
				fprintf(stderr, "Could not copy the stream context\n");
		}
		if( ret < 0 ) {
			ff_err(ret,"FFMPEG::open_encoder");
			eprintf(_("open failed %s:%s\n"), codec_name, filename);
			ret = 1;
		}
		else
			ret = 0;
	}
	if( !ret && fst && bsfilter[0] ) {
		ret = av_bsf_list_parse_str(bsfilter, &fst->bsfc);
		if( ret < 0 ) {
			ff_err(ret,"FFMPEG::open_encoder");
			eprintf(_("bitstream filter failed %s:\n%s\n"), filename, bsfilter);
			ret = 1;
		}
		else
			ret = 0;
	}

	if( !ret )
		start_muxer();

	ff_unlock();
	av_dict_free(&sopts);
	return ret;
}

int FFMPEG::close_encoder()
{
	stop_muxer();
	if( encoding > 0 ) {
		av_write_trailer(fmt_ctx);
		if( !(fmt_ctx->flags & AVFMT_NOFILE) )
			avio_closep(&fmt_ctx->pb);
	}
	encoding = 0;
	return 0;
}

int FFMPEG::decode_activate()
{
	if( decoding < 0 ) {
		decoding = 0;
		for( int vidx=0; vidx<ffvideo.size(); ++vidx )
			ffvideo[vidx]->nudge = AV_NOPTS_VALUE;
		for( int aidx=0; aidx<ffaudio.size(); ++aidx )
			ffaudio[aidx]->nudge = AV_NOPTS_VALUE;
		// set nudges for each program stream set
		const int64_t min_nudge = INT64_MIN+1;
		int npgrms = fmt_ctx->nb_programs;
		for( int i=0; i<npgrms; ++i ) {
			AVProgram *pgrm = fmt_ctx->programs[i];
			// first start time video stream
			int64_t vstart_time = min_nudge, astart_time = min_nudge;
			for( int j=0; j<(int)pgrm->nb_stream_indexes; ++j ) {
				int fidx = pgrm->stream_index[j];
				AVStream *st = fmt_ctx->streams[fidx];
				AVCodecParameters *avpar = st->codecpar;
				if( avpar->codec_type == AVMEDIA_TYPE_VIDEO ) {
					if( st->start_time == AV_NOPTS_VALUE ) continue;
					if( vstart_time < st->start_time )
						vstart_time = st->start_time;
					continue;
				}
				if( avpar->codec_type == AVMEDIA_TYPE_AUDIO ) {
					if( st->start_time == AV_NOPTS_VALUE ) continue;
					if( astart_time < st->start_time )
						astart_time = st->start_time;
					continue;
				}
			}
			//since frame rate is much more grainy than sample rate, it is better to
			// align using video, so that total absolute error is minimized.
			int64_t nudge = vstart_time > min_nudge ? vstart_time :
				astart_time > min_nudge ? astart_time : AV_NOPTS_VALUE;
			for( int j=0; j<(int)pgrm->nb_stream_indexes; ++j ) {
				int fidx = pgrm->stream_index[j];
				AVStream *st = fmt_ctx->streams[fidx];
				AVCodecParameters *avpar = st->codecpar;
				if( avpar->codec_type == AVMEDIA_TYPE_VIDEO ) {
					for( int k=0; k<ffvideo.size(); ++k ) {
						if( ffvideo[k]->fidx != fidx ) continue;
						ffvideo[k]->nudge = nudge;
					}
					continue;
				}
				if( avpar->codec_type == AVMEDIA_TYPE_AUDIO ) {
					for( int k=0; k<ffaudio.size(); ++k ) {
						if( ffaudio[k]->fidx != fidx ) continue;
						ffaudio[k]->nudge = nudge;
					}
					continue;
				}
			}
		}
		// set nudges for any streams not yet set
		int64_t vstart_time = min_nudge, astart_time = min_nudge;
		int nstreams = fmt_ctx->nb_streams;
		for( int i=0; i<nstreams; ++i ) {
			AVStream *st = fmt_ctx->streams[i];
			AVCodecParameters *avpar = st->codecpar;
			switch( avpar->codec_type ) {
			case AVMEDIA_TYPE_VIDEO: {
				if( st->start_time == AV_NOPTS_VALUE ) continue;
				int vidx = ffvideo.size();
				while( --vidx >= 0 && ffvideo[vidx]->fidx != i );
				if( vidx < 0 ) continue;
				if( ffvideo[vidx]->nudge != AV_NOPTS_VALUE ) continue;
				if( vstart_time < st->start_time )
					vstart_time = st->start_time;
				break; }
			case AVMEDIA_TYPE_AUDIO: {
				if( st->start_time == AV_NOPTS_VALUE ) continue;
				int aidx = ffaudio.size();
				while( --aidx >= 0 && ffaudio[aidx]->fidx != i );
				if( aidx < 0 ) continue;
				if( ffaudio[aidx]->frame_sz < avpar->frame_size )
					ffaudio[aidx]->frame_sz = avpar->frame_size;
				if( ffaudio[aidx]->nudge != AV_NOPTS_VALUE ) continue;
				if( astart_time < st->start_time )
					astart_time = st->start_time;
				break; }
			default: break;
			}
		}
		int64_t nudge = vstart_time > min_nudge ? vstart_time :
			astart_time > min_nudge ? astart_time : 0;
		for( int vidx=0; vidx<ffvideo.size(); ++vidx ) {
			if( ffvideo[vidx]->nudge == AV_NOPTS_VALUE )
				ffvideo[vidx]->nudge = nudge;
		}
		for( int aidx=0; aidx<ffaudio.size(); ++aidx ) {
			if( ffaudio[aidx]->nudge == AV_NOPTS_VALUE )
				ffaudio[aidx]->nudge = nudge;
		}
		decoding = 1;
	}
	return decoding;
}

int FFMPEG::encode_activate()
{
	int ret = 0;
	if( encoding < 0 ) {
		encoding = 0;
		if( !(fmt_ctx->flags & AVFMT_NOFILE) &&
		    (ret=avio_open(&fmt_ctx->pb, fmt_ctx->url, AVIO_FLAG_WRITE)) < 0 ) {
			ff_err(ret, "FFMPEG::encode_activate: err opening : %s\n",
				fmt_ctx->url);
			return -1;
		}

		int prog_id = 1;
		AVProgram *prog = av_new_program(fmt_ctx, prog_id);
		for( int i=0; i< ffvideo.size(); ++i )
			av_program_add_stream_index(fmt_ctx, prog_id, ffvideo[i]->fidx);
		for( int i=0; i< ffaudio.size(); ++i )
			av_program_add_stream_index(fmt_ctx, prog_id, ffaudio[i]->fidx);
		int pi = fmt_ctx->nb_programs;
		while(  --pi >= 0 && fmt_ctx->programs[pi]->id != prog_id );
		AVDictionary **meta = &prog->metadata;
		av_dict_set(meta, "service_provider", "cin5", 0);
		const char *path = fmt_ctx->url, *bp = strrchr(path,'/');
		if( bp ) path = bp + 1;
		av_dict_set(meta, "title", path, 0);

		if( ffaudio.size() ) {
			const char *ep = getenv("CIN_AUDIO_LANG"), *lp = 0;
			if( !ep && (lp=getenv("LANG")) ) { // some are guesses
				static struct { const char lc[3], lng[4]; } lcode[] = {
					{ "en", "eng" }, { "de", "ger" }, { "es", "spa" },
					{ "eu", "bas" }, { "fr", "fre" }, { "el", "gre" },
					{ "hi", "hin" }, { "it", "ita" }, { "ja", "jap" },
					{ "ko", "kor" }, { "du", "dut" }, { "pl", "pol" },
					{ "pt", "por" }, { "ru", "rus" }, { "sl", "slv" },
					{ "uk", "ukr" }, { "vi", "vie" }, { "zh", "chi" },
				};
				for( int i=sizeof(lcode)/sizeof(lcode[0]); --i>=0 && !ep; )
					if( !strncmp(lcode[i].lc,lp,2) ) ep = lcode[i].lng;
			}
			if( !ep ) ep = "und";
			char lang[5];
			strncpy(lang,ep,3);  lang[3] = 0;
			AVStream *st = ffaudio[0]->st;
			av_dict_set(&st->metadata,"language",lang,0);
		}

		AVDictionary *fopts = 0;
		char option_path[BCTEXTLEN];
		set_option_path(option_path, "format/%s", file_format);
		read_options(option_path, fopts, 1);
		ret = avformat_write_header(fmt_ctx, &fopts);
		if( ret < 0 ) {
			ff_err(ret, "FFMPEG::encode_activate: write header failed %s\n",
				fmt_ctx->url);
			return -1;
		}
		av_dict_free(&fopts);
		encoding = 1;
	}
	return encoding;
}


int FFMPEG::audio_seek(int stream, int64_t pos)
{
	int aidx = astrm_index[stream].st_idx;
	FFAudioStream *aud = ffaudio[aidx];
	aud->audio_seek(pos);
	return 0;
}

int FFMPEG::video_seek(int stream, int64_t pos)
{
	int vidx = vstrm_index[stream].st_idx;
	FFVideoStream *vid = ffvideo[vidx];
	vid->video_seek(pos);
	return 0;
}


int FFMPEG::decode(int chn, int64_t pos, double *samples, int len)
{
	if( !has_audio || chn >= astrm_index.size() ) return -1;
	int aidx = astrm_index[chn].st_idx;
	FFAudioStream *aud = ffaudio[aidx];
	if( aud->load(pos, len) < len ) return -1;
	int ch = astrm_index[chn].st_ch;
	int ret = aud->read(samples,len,ch);
	return ret;
}

int FFMPEG::decode(int layer, int64_t pos, VFrame *vframe)
{
	if( !has_video || layer >= vstrm_index.size() ) return -1;
	int vidx = vstrm_index[layer].st_idx;
	FFVideoStream *vid = ffvideo[vidx];
	return vid->load(vframe, pos);
}


int FFMPEG::encode(int stream, double **samples, int len)
{
	FFAudioStream *aud = ffaudio[stream];
	return aud->encode(samples, len);
}


int FFMPEG::encode(int stream, VFrame *frame)
{
	FFVideoStream *vid = ffvideo[stream];
	return vid->encode(frame);
}

void FFMPEG::start_muxer()
{
	if( !running() ) {
		done = 0;
		start();
	}
}

void FFMPEG::stop_muxer()
{
	if( running() ) {
		done = 1;
		mux_lock->unlock();
	}
	join();
}

void FFMPEG::flow_off()
{
	if( !flow ) return;
	flow_lock->lock("FFMPEG::flow_off");
	flow = 0;
}

void FFMPEG::flow_on()
{
	if( flow ) return;
	flow = 1;
	flow_lock->unlock();
}

void FFMPEG::flow_ctl()
{
	while( !flow ) {
		flow_lock->lock("FFMPEG::flow_ctl");
		flow_lock->unlock();
	}
}

int FFMPEG::mux_audio(FFrame *frm)
{
	FFStream *fst = frm->fst;
	AVCodecContext *ctx = fst->avctx;
	AVFrame *frame = *frm;
	AVRational tick_rate = {1, ctx->sample_rate};
	frame->pts = av_rescale_q(frm->position, tick_rate, ctx->time_base);
	int ret = fst->encode_frame(frame);
	if( ret < 0 )
		ff_err(ret, "FFMPEG::mux_audio");
	return ret >= 0 ? 0 : 1;
}

int FFMPEG::mux_video(FFrame *frm)
{
	FFStream *fst = frm->fst;
	AVFrame *frame = *frm;
	frame->pts = frm->position;
	int ret = fst->encode_frame(frame);
	if( ret < 0 )
		ff_err(ret, "FFMPEG::mux_video");
	return ret >= 0 ? 0 : 1;
}

void FFMPEG::mux()
{
	for(;;) {
		double atm = -1, vtm = -1;
		FFrame *afrm = 0, *vfrm = 0;
		int demand = 0;
		for( int i=0; i<ffaudio.size(); ++i ) {  // earliest audio
			FFStream *fst = ffaudio[i];
			if( fst->frm_count < 3 ) { demand = 1; flow_on(); }
			FFrame *frm = fst->frms.first;
			if( !frm ) { if( !done ) return; continue; }
			double tm = to_secs(frm->position, fst->avctx->time_base);
			if( atm < 0 || tm < atm ) { atm = tm;  afrm = frm; }
		}
		for( int i=0; i<ffvideo.size(); ++i ) {  // earliest video
			FFStream *fst = ffvideo[i];
			if( fst->frm_count < 2 ) { demand = 1; flow_on(); }
			FFrame *frm = fst->frms.first;
			if( !frm ) { if( !done ) return; continue; }
			double tm = to_secs(frm->position, fst->avctx->time_base);
			if( vtm < 0 || tm < vtm ) { vtm = tm;  vfrm = frm; }
		}
		if( !demand ) flow_off();
		if( !afrm && !vfrm ) break;
		int v = !afrm ? -1 : !vfrm ? 1 : av_compare_ts(
			vfrm->position, vfrm->fst->avctx->time_base,
			afrm->position, afrm->fst->avctx->time_base);
		FFrame *frm = v <= 0 ? vfrm : afrm;
		if( frm == afrm ) mux_audio(frm);
		if( frm == vfrm ) mux_video(frm);
		frm->dequeue();
		delete frm;
	}
}

void FFMPEG::run()
{
	while( !done ) {
		mux_lock->lock("FFMPEG::run");
		if( !done ) mux();
	}
	for( int i=0; i<ffaudio.size(); ++i )
		ffaudio[i]->drain();
	for( int i=0; i<ffvideo.size(); ++i )
		ffvideo[i]->drain();
	mux();
	for( int i=0; i<ffaudio.size(); ++i )
		ffaudio[i]->flush();
	for( int i=0; i<ffvideo.size(); ++i )
		ffvideo[i]->flush();
}


int FFMPEG::ff_total_audio_channels()
{
	return astrm_index.size();
}

int FFMPEG::ff_total_astreams()
{
	return ffaudio.size();
}

int FFMPEG::ff_audio_channels(int stream)
{
	return ffaudio[stream]->channels;
}

int FFMPEG::ff_sample_rate(int stream)
{
	return ffaudio[stream]->sample_rate;
}

const char* FFMPEG::ff_audio_format(int stream)
{
	AVStream *st = ffaudio[stream]->st;
	AVCodecID id = st->codecpar->codec_id;
	const AVCodecDescriptor *desc = avcodec_descriptor_get(id);
	return desc ? desc->name : _("Unknown");
}

int FFMPEG::ff_audio_pid(int stream)
{
	return ffaudio[stream]->st->id;
}

int64_t FFMPEG::ff_audio_samples(int stream)
{
	return ffaudio[stream]->length;
}

// find audio astream/channels with this program,
//   or all program audio channels (astream=-1)
int FFMPEG::ff_audio_for_video(int vstream, int astream, int64_t &channel_mask)
{
	channel_mask = 0;
	int pidx = -1;
	int vidx = ffvideo[vstream]->fidx;
	// find first program with this video stream
	for( int i=0; pidx<0 && i<(int)fmt_ctx->nb_programs; ++i ) {
		AVProgram *pgrm = fmt_ctx->programs[i];
		for( int j=0;  pidx<0 && j<(int)pgrm->nb_stream_indexes; ++j ) {
			int st_idx = pgrm->stream_index[j];
			AVStream *st = fmt_ctx->streams[st_idx];
			if( st->codecpar->codec_type != AVMEDIA_TYPE_VIDEO ) continue;
			if( st_idx == vidx ) pidx = i;
		}
	}
	if( pidx < 0 ) return -1;
	int ret = -1;
	int64_t channels = 0;
	AVProgram *pgrm = fmt_ctx->programs[pidx];
	for( int j=0; j<(int)pgrm->nb_stream_indexes; ++j ) {
		int aidx = pgrm->stream_index[j];
		AVStream *st = fmt_ctx->streams[aidx];
		if( st->codecpar->codec_type != AVMEDIA_TYPE_AUDIO ) continue;
		if( astream > 0 ) { --astream;  continue; }
		int astrm = -1;
		for( int i=0; astrm<0 && i<ffaudio.size(); ++i )
			if( ffaudio[i]->fidx == aidx ) astrm = i;
		if( astrm >= 0 ) {
			if( ret < 0 ) ret = astrm;
			int64_t mask = (1 << ffaudio[astrm]->channels) - 1;
			channels |= mask << ffaudio[astrm]->channel0;
		}
		if( !astream ) break;
	}
	channel_mask = channels;
	return ret;
}


int FFMPEG::ff_total_video_layers()
{
	return vstrm_index.size();
}

int FFMPEG::ff_total_vstreams()
{
	return ffvideo.size();
}

int FFMPEG::ff_video_width(int stream)
{
	return ffvideo[stream]->width;
}

int FFMPEG::ff_video_height(int stream)
{
	return ffvideo[stream]->height;
}

int FFMPEG::ff_set_video_width(int stream, int width)
{
	int w = ffvideo[stream]->width;
	ffvideo[stream]->width = width;
	return w;
}

int FFMPEG::ff_set_video_height(int stream, int height)
{
	int h = ffvideo[stream]->height;
	ffvideo[stream]->height = height;
	return h;
}

int FFMPEG::ff_coded_width(int stream)
{
	return ffvideo[stream]->avctx->coded_width;
}

int FFMPEG::ff_coded_height(int stream)
{
	return ffvideo[stream]->avctx->coded_height;
}

float FFMPEG::ff_aspect_ratio(int stream)
{
	return ffvideo[stream]->aspect_ratio;
}

const char* FFMPEG::ff_video_format(int stream)
{
	AVStream *st = ffvideo[stream]->st;
	AVCodecID id = st->codecpar->codec_id;
	const AVCodecDescriptor *desc = avcodec_descriptor_get(id);
	return desc ? desc->name : _("Unknown");
}

double FFMPEG::ff_frame_rate(int stream)
{
	return ffvideo[stream]->frame_rate;
}

int64_t FFMPEG::ff_video_frames(int stream)
{
	return ffvideo[stream]->length;
}

int FFMPEG::ff_video_pid(int stream)
{
	return ffvideo[stream]->st->id;
}

int FFMPEG::ff_video_mpeg_color_range(int stream)
{
	return ffvideo[stream]->st->codecpar->color_range == AVCOL_RANGE_MPEG ? 1 : 0;
}

int FFMPEG::ff_cpus()
{
	return file_base->file->cpus;
}

int FFVideoStream::create_filter(const char *filter_spec, AVCodecParameters *avpar)
{
	avfilter_register_all();
	const char *sp = filter_spec;
	char filter_name[BCSTRLEN], *np = filter_name;
	int i = sizeof(filter_name);
	while( --i>=0 && *sp!=0 && !strchr(" \t:=,",*sp) ) *np++ = *sp++;
	*np = 0;
	const AVFilter *filter = !filter_name[0] ? 0 : avfilter_get_by_name(filter_name);
	if( !filter || avfilter_pad_get_type(filter->inputs,0) != AVMEDIA_TYPE_VIDEO ) {
		ff_err(AVERROR(EINVAL), "FFVideoStream::create_filter: %s\n", filter_spec);
		return -1;
	}
	filter_graph = avfilter_graph_alloc();
	const AVFilter *buffersrc = avfilter_get_by_name("buffer");
	const AVFilter *buffersink = avfilter_get_by_name("buffersink");

	int ret = 0;  char args[BCTEXTLEN];
	AVPixelFormat pix_fmt = (AVPixelFormat)avpar->format;
	snprintf(args, sizeof(args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		avpar->width, avpar->height, (int)pix_fmt,
		st->time_base.num, st->time_base.den,
		avpar->sample_aspect_ratio.num, avpar->sample_aspect_ratio.den);
	if( ret >= 0 )
		ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
			args, NULL, filter_graph);
	if( ret >= 0 )
		ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
			NULL, NULL, filter_graph);
	if( ret >= 0 )
		ret = av_opt_set_bin(buffersink_ctx, "pix_fmts",
			(uint8_t*)&pix_fmt, sizeof(pix_fmt),
			AV_OPT_SEARCH_CHILDREN);
	if( ret < 0 )
		ff_err(ret, "FFVideoStream::create_filter");
	else
		ret = FFStream::create_filter(filter_spec);
	return ret >= 0 ? 0 : -1;
}

int FFAudioStream::create_filter(const char *filter_spec, AVCodecParameters *avpar)
{
	avfilter_register_all();
	const char *sp = filter_spec;
	char filter_name[BCSTRLEN], *np = filter_name;
	int i = sizeof(filter_name);
	while( --i>=0 && *sp!=0 && !strchr(" \t:=,",*sp) ) *np++ = *sp++;
	*np = 0;
	const AVFilter *filter = !filter_name[0] ? 0 : avfilter_get_by_name(filter_name);
	if( !filter || avfilter_pad_get_type(filter->inputs,0) != AVMEDIA_TYPE_AUDIO ) {
		ff_err(AVERROR(EINVAL), "FFAudioStream::create_filter: %s\n", filter_spec);
		return -1;
	}
	filter_graph = avfilter_graph_alloc();
	const AVFilter *buffersrc = avfilter_get_by_name("abuffer");
	const AVFilter *buffersink = avfilter_get_by_name("abuffersink");
	int ret = 0;  char args[BCTEXTLEN];
	AVSampleFormat sample_fmt = (AVSampleFormat)avpar->format;
	snprintf(args, sizeof(args),
		"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%jx",
		st->time_base.num, st->time_base.den, avpar->sample_rate,
		av_get_sample_fmt_name(sample_fmt), avpar->channel_layout);
	if( ret >= 0 )
		ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
			args, NULL, filter_graph);
	if( ret >= 0 )
		ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
			NULL, NULL, filter_graph);
	if( ret >= 0 )
		ret = av_opt_set_bin(buffersink_ctx, "sample_fmts",
			(uint8_t*)&sample_fmt, sizeof(sample_fmt),
			AV_OPT_SEARCH_CHILDREN);
	if( ret >= 0 )
		ret = av_opt_set_bin(buffersink_ctx, "channel_layouts",
			(uint8_t*)&avpar->channel_layout,
			sizeof(avpar->channel_layout), AV_OPT_SEARCH_CHILDREN);
	if( ret >= 0 )
		ret = av_opt_set_bin(buffersink_ctx, "sample_rates",
			(uint8_t*)&sample_rate, sizeof(sample_rate),
			AV_OPT_SEARCH_CHILDREN);
	if( ret < 0 )
		ff_err(ret, "FFAudioStream::create_filter");
	else
		ret = FFStream::create_filter(filter_spec);
	return ret >= 0 ? 0 : -1;
}

int FFStream::create_filter(const char *filter_spec)
{
	/* Endpoints for the filter graph. */
	AVFilterInOut *outputs = avfilter_inout_alloc();
	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx;
	outputs->pad_idx = 0;
	outputs->next = 0;

	AVFilterInOut *inputs  = avfilter_inout_alloc();
	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx;
	inputs->pad_idx	= 0;
	inputs->next = 0;

	int ret = !outputs->name || !inputs->name ? -1 : 0;
	if( ret >= 0 )
		ret = avfilter_graph_parse_ptr(filter_graph, filter_spec,
			&inputs, &outputs, NULL);
	if( ret >= 0 )
		ret = avfilter_graph_config(filter_graph, NULL);

	if( ret < 0 ) {
		ff_err(ret, "FFStream::create_filter");
		avfilter_graph_free(&filter_graph);
		filter_graph = 0;
	}
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);
	return ret;
}

int FFMPEG::scan(IndexState *index_state, int64_t *scan_position, int *canceled)
{
	AVPacket pkt;
	av_init_packet(&pkt);
	AVFrame *frame = av_frame_alloc();
	if( !frame ) {
		fprintf(stderr,"FFMPEG::scan: ");
		fprintf(stderr,_("av_frame_alloc failed\n"));
		fprintf(stderr,"FFMPEG::scan:file=%s\n", file_base->asset->path);
		return -1;
	}

	index_state->add_video_markers(ffvideo.size());
	index_state->add_audio_markers(ffaudio.size());

	for( int i=0; i<(int)fmt_ctx->nb_streams; ++i ) {
		int ret = 0;
		AVDictionary *copts = 0;
		av_dict_copy(&copts, opts, 0);
		AVStream *st = fmt_ctx->streams[i];
		AVCodecID codec_id = st->codecpar->codec_id;
		AVCodec *decoder = avcodec_find_decoder(codec_id);
		AVCodecContext *avctx = avcodec_alloc_context3(decoder);
		if( !avctx ) {
			eprintf(_("cant allocate codec context\n"));
			ret = AVERROR(ENOMEM);
		}
		if( ret >= 0 ) {
			avcodec_parameters_to_context(avctx, st->codecpar);
			if( !av_dict_get(copts, "threads", NULL, 0) )
				avctx->thread_count = ff_cpus();
			ret = avcodec_open2(avctx, decoder, &copts);
		}
		av_dict_free(&copts);
		if( ret >= 0 ) {
			AVCodecParameters *avpar = st->codecpar;
			switch( avpar->codec_type ) {
			case AVMEDIA_TYPE_VIDEO: {
				int vidx = ffvideo.size();
				while( --vidx>=0 && ffvideo[vidx]->fidx != i );
				if( vidx < 0 ) break;
				ffvideo[vidx]->avctx = avctx;
				continue; }
			case AVMEDIA_TYPE_AUDIO: {
				int aidx = ffaudio.size();
				while( --aidx>=0 && ffaudio[aidx]->fidx != i );
				if( aidx < 0 ) break;
				ffaudio[aidx]->avctx = avctx;
				continue; }
			default: break;
			}
		}
		fprintf(stderr,"FFMPEG::scan: ");
		fprintf(stderr,_("codec open failed\n"));
		fprintf(stderr,"FFMPEG::scan:file=%s\n", file_base->asset->path);
		avcodec_free_context(&avctx);
	}

	decode_activate();
	for( int i=0; i<(int)fmt_ctx->nb_streams; ++i ) {
		AVStream *st = fmt_ctx->streams[i];
		AVCodecParameters *avpar = st->codecpar;
		if( avpar->codec_type != AVMEDIA_TYPE_AUDIO ) continue;
		int64_t tstmp = st->start_time;
		if( tstmp == AV_NOPTS_VALUE ) continue;
		int aidx = ffaudio.size();
		while( --aidx>=0 && ffaudio[aidx]->fidx != i );
		if( aidx < 0 ) continue;
		FFAudioStream *aud = ffaudio[aidx];
		tstmp -= aud->nudge;
		double secs = to_secs(tstmp, st->time_base);
		aud->curr_pos = secs * aud->sample_rate + 0.5;
	}

	int errs = 0;
	for( int64_t count=0; !*canceled; ++count ) {
		av_packet_unref(&pkt);
		pkt.data = 0; pkt.size = 0;

		int ret = av_read_frame(fmt_ctx, &pkt);
		if( ret < 0 ) {
			if( ret == AVERROR_EOF ) break;
			if( ++errs > 100 ) {
				ff_err(ret,_("over 100 read_frame errs\n"));
				break;
			}
			continue;
		}
		if( !pkt.data ) continue;
		int i = pkt.stream_index;
		if( i < 0 || i >= (int)fmt_ctx->nb_streams ) continue;
		AVStream *st = fmt_ctx->streams[i];
		if( pkt.pos > *scan_position ) *scan_position = pkt.pos;

		AVCodecParameters *avpar = st->codecpar;
		switch( avpar->codec_type ) {
		case AVMEDIA_TYPE_VIDEO: {
			int vidx = ffvideo.size();
			while( --vidx>=0 && ffvideo[vidx]->fidx != i );
			if( vidx < 0 ) break;
			FFVideoStream *vid = ffvideo[vidx];
			if( !vid->avctx ) break;
			int64_t tstmp = pkt.dts;
			if( tstmp == AV_NOPTS_VALUE ) tstmp = pkt.pts;
			if( tstmp != AV_NOPTS_VALUE && (pkt.flags & AV_PKT_FLAG_KEY) && pkt.pos > 0 ) {
				if( vid->nudge != AV_NOPTS_VALUE ) tstmp -= vid->nudge;
				double secs = to_secs(tstmp, st->time_base);
				int64_t frm = secs * vid->frame_rate + 0.5;
				if( frm < 0 ) frm = 0;
				index_state->put_video_mark(vidx, frm, pkt.pos);
			}
#if 0
			ret = avcodec_send_packet(vid->avctx, pkt);
			if( ret < 0 ) break;
			while( (ret=vid->decode_frame(frame)) > 0 ) {}
#endif
			break; }
		case AVMEDIA_TYPE_AUDIO: {
			int aidx = ffaudio.size();
			while( --aidx>=0 && ffaudio[aidx]->fidx != i );
			if( aidx < 0 ) break;
			FFAudioStream *aud = ffaudio[aidx];
			if( !aud->avctx ) break;
			int64_t tstmp = pkt.pts;
			if( tstmp == AV_NOPTS_VALUE ) tstmp = pkt.dts;
			if( tstmp != AV_NOPTS_VALUE && (pkt.flags & AV_PKT_FLAG_KEY) && pkt.pos > 0 ) {
				if( aud->nudge != AV_NOPTS_VALUE ) tstmp -= aud->nudge;
				double secs = to_secs(tstmp, st->time_base);
				int64_t sample = secs * aud->sample_rate + 0.5;
				if( sample >= 0 )
					index_state->put_audio_mark(aidx, sample, pkt.pos);
			}
			ret = avcodec_send_packet(aud->avctx, &pkt);
			if( ret < 0 ) break;
			int ch = aud->channel0,  nch = aud->channels;
			int64_t pos = index_state->pos(ch);
			if( pos != aud->curr_pos ) {
if( abs(pos-aud->curr_pos) > 1 )
printf("audio%d pad %jd %jd (%jd)\n", aud->idx, pos, aud->curr_pos, pos-aud->curr_pos);
				index_state->pad_data(ch, nch, aud->curr_pos);
			}
			while( (ret=aud->decode_frame(frame)) > 0 ) {
				//if( frame->channels != nch ) break;
				aud->init_swr(frame->channels, frame->format, frame->sample_rate);
				float *samples;
				int len = aud->get_samples(samples,
					 &frame->extended_data[0], frame->nb_samples);
				pos = aud->curr_pos;
				if( (aud->curr_pos += len) >= 0 ) {
					if( pos < 0 ) {
						samples += -pos * nch;
						len = aud->curr_pos;
					}
					for( int i=0; i<nch; ++i )
						index_state->put_data(ch+i,nch,samples+i,len);
				}
			}
			break; }
		default: break;
		}
	}
	av_frame_free(&frame);
	return 0;
}

void FFStream::load_markers(IndexMarks &marks, double rate)
{
	int in = 0;
	int64_t sz = marks.size();
	int max_entries = fmt_ctx->max_index_size / sizeof(AVIndexEntry) - 1;
	int nb_ent = st->nb_index_entries;
// some formats already have an index
	if( nb_ent > 0 ) {
		AVIndexEntry *ep = &st->index_entries[nb_ent-1];
		int64_t tstmp = ep->timestamp;
		if( nudge != AV_NOPTS_VALUE ) tstmp -= nudge;
		double secs = ffmpeg->to_secs(tstmp, st->time_base);
		int64_t no = secs * rate;
		while( in < sz && marks[in].no <= no ) ++in;
	}
	int64_t len = sz - in;
	int64_t count = max_entries - nb_ent;
	if( count > len ) count = len;
	for( int i=0; i<count; ++i ) {
		int k = in + i * len / count;
		int64_t no = marks[k].no, pos = marks[k].pos;
		double secs = (double)no / rate;
		int64_t tstmp = secs * st->time_base.den / st->time_base.num;
		if( nudge != AV_NOPTS_VALUE ) tstmp += nudge;
		av_add_index_entry(st, pos, tstmp, 0, 0, AVINDEX_KEYFRAME);
	}
}

