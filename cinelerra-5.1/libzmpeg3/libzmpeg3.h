#ifndef LIBMPEG3_H
#define LIBMPEG3_H
#ifdef HAVE_LIBZMPEG

/* for quicktime build */
#define MAXFRAMESAMPLES 65536
#define ZDVB
#define USE_FUTEX

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdio.h>
#include <stdint.h>

typedef int (*zthumbnail_cb)(void *p, int trk);
typedef int (*zcc_text_cb)(int sid, int id, int sfrm, int efrm, const char *txt);

#ifdef __cplusplus
#include <unistd.h>
#include <stdlib.h>
#include <byteswap.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <endian.h>
#include <signal.h>
#include <pthread.h>
#include <mntent.h>
#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/syscall.h>
#ifdef USE_FUTEX
#include <linux/futex.h>
#endif

extern "C" {
#include "a52.h"
}

#define ZMPEG3_MAJOR   2
#define ZMPEG3_MINOR   0
#define ZMPEG3_RELEASE 0
#define ZRENDERFARM_FS_PREFIX "vfs://"

#define ZIO_SINGLE_ACCESS zfs_t::io_SINGLE_ACCESS
#define ZIO_UNBUFFERED zfs_t::io_UNBUFFERED
#define ZIO_NONBLOCK zfs_t::io_NONBLOCK
#define ZIO_SEQUENTIAL zfs_t::io_SEQUENTIAL
#define ZIO_THREADED zfs_t::io_THREADED
#define ZIO_ERRFAIL zfs_t::io_ERRFAIL
#define ZIO_RETRIES 1

#define TOC_SAMPLE_OFFSETS zmpeg3_t::show_toc_SAMPLE_OFFSETS
#define TOC_AUDIO_INDEX zmpeg3_t::show_toc_AUDIO_INDEX
#define TOC_FRAME_OFFSETS zmpeg3_t::show_toc_FRAME_OFFSETS

#define ZMPEG3_PROC_CPUINFO "/proc/cpuinfo"
#if BYTE_ORDER == LITTLE_ENDIAN
#define ZMPEG3_LITTLE_ENDIAN 1
#else
#define ZMPEG3_LITTLE_ENDIAN 0
#endif

// Combine the pid and the stream id into one unit
#define CUSTOM_ID(pid, stream_id) (((pid << 8) | stream_id) & 0xffff)
#define CUSTOM_ID_PID(id) (id >> 8)
#define CUSTOM_ID_STREAMID(id) (id & 0xff)

#ifndef ZMAX
#define ZMAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef ZMIN
#define ZMIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef isizeof
#define isizeof(x) ((int)sizeof(x))
#endif
#ifndef lengthof
#define lengthof(x) ((int)(sizeof(x)/sizeof(x[0])))
#endif

#define zlikely(x)   __builtin_expect((x),1)
#define zunlikely(x) __builtin_expect((x),0)

#define bcd(n) ((((n)>>4)&0x0f)*10+((n)&0x0f))

#define new_memset(s) \
  void *operator new(size_t n) { \
    void * volatile t = (void*) new char[n]; \
    memset(t,s,n); \
    return t; \
  } \
  void operator delete(void *t,size_t n) { \
    delete[](char*)t; \
  } \
  void *operator new[](size_t n) { \
    void * volatile t = (void*) new char[n]; \
    memset(t,s,n); \
    return t; \
  } \
  void operator delete[](void *t,size_t n) { \
    delete[](char*)t; \
  }

#define znew(T,n) ((T*)memset(new T[n],0,(n)*sizeof(T)))
#define zmsg(fmt) printf("%s: " fmt, __func__)
#define zmsgs(fmt,args...) printf("%s: " fmt, __func__, args)
#define zerr(fmt) fprintf(stderr,"%s:err " fmt, __func__)
#define zerrs(fmt,args...) fprintf(stderr,"%s:err " fmt, __func__, args)
#define perr(fmt) do { char msg[512]; \
 snprintf(&msg[0],sizeof(msg), "%s: " fmt,__func__); perror(&msg[0]); \
} while(0)
#define perrs(fmt,args...) do { char msg[512]; \
 snprintf(&msg[0],sizeof(msg), "%s: " fmt,__func__,args); perror(&msg[0]); \
} while(0)

class zmpeg3_t;

class zmpeg3_t {
public:
  new_memset(0);
  class zlock_t;
  class zblock_t;
  class zrwlock_t;
  class fs_t;
  class bits_t;
  class index_t;
  class bitfont_t;
  class timecode_t;
  class audio_t;
  class slice_buffer_t;
  class video_t;
  class slice_decoder_t;
  class subtitle_t;
  class demuxer_t;
  class cacheframe_t;
  class cache_t;
  class atrack_t;
  class vtrack_t;
  class strack_t;
#ifdef ZDVB
  class dvb_t;
#endif

  enum {
    // Error codes for the error_return variable
    ERR_UNDEFINED_ERROR              = 1,
    ERR_INVALID_TOC_VERSION          = 2,
    ERR_TOC_DATE_MISMATCH            = 3,

    TOC_PREFIX                       = 0x544f4320,
    // This decreases with every new version
    TOC_VERSION                      = 0x000000f9,
    ID3_PREFIX                       = 0x494433,
    IFO_PREFIX                       = 0x44564456,
    // First byte to read when opening a file
    START_BYTE                       = 0x0,
    // Bytes read by mpeg3io at a time
    MAX_IO_SIZE                      = 0x10000,
    IO_SIZE                          = 0x80000,
    SEQ_IO_SIZE                      = 0xc00000,
    MAX_TS_PROBE                     = 0x800000,
    MAX_PGM_PROBE                    = 0x800000,
    // Largest possible packet
    RAW_SIZE                         = 0x400000,
    RIFF_CODE                        = 0x52494646,
    BD_PACKET_SIZE                   = 192,
    TS_PACKET_SIZE                   = 188,
    DVD_PACKET_SIZE                  = 0x800,
    ERR_PACKET_SIZE                  = 0x800,
    IO_ERR_LIMIT                     = 2,
    SYNC_BYTE                        = 0x47,
    PROGRAM_END_CODE                 = 0x000001b9,
    PACK_START_CODE                  = 0x000001ba,
    SEQUENCE_START_CODE              = 0x000001b3,
    SEQUENCE_END_CODE                = 0x000001b7,
    SYSTEM_START_CODE                = 0x000001bb,
    STRLEN                           = 1024,
    // Maximum number of PIDs in one stream
    PIDMAX                          = 256,
    PROGRAM_ASSOCIATION_TABLE       = 0x00,
    CONDITIONAL_ACCESS_TABLE        = 0x01,
    PACKET_START_CODE_PREFIX        = 0x000001,
    // NAV/PRIVATE_STREAM_2 are the same id
    PRIVATE_STREAM_2                = 0xbf,
    NAV_PACKET_STREAM               = 0xbf,
    NAV_PCI_BYTES                   = 0x3d3,
    NAV_DSI_BYTES                   = 0x3f9,
    NAV_PCI_SSID                    = 0x00,
    NAV_DSI_SSID                    = 0x01,
    NAV_SRI_END_OF_CELL             = 0x3fffffff,
    PADDING_STREAM                  = 0xbe,
    GOP_START_CODE                  = 0x000001b8,
    PICTURE_START_CODE              = 0x00000100,
    EXT_START_CODE                  = 0x000001b5,
    USER_START_CODE                 = 0x000001b2,
    SLICE_MIN_START                 = 0x00000101,
    SLICE_MAX_START                 = 0x000001af,
    AC3_START_CODE                  = 0x0b77,
    PCM_START_CODE                  = 0x7f7f807f,
    MAX_CPUS                        = 256,
    MAX_STREAMZ                     = 0x10000,
    MAX_PACKSIZE                    = 262144,
    MAX_CACHE_FRAMES                = 128,
    // Maximum number of complete subtitles to buffer in a subtitle track
    // or number of incomplete subtitles to buffer in demuxer.
    MAX_SUBTITLES                   = 16,
    // Positive difference before declaring timecodes discontinuous
    CONTIGUOUS_THRESHOLD            = 10  ,
    // Minimum number of seconds before interleaving programs
    PROGRAM_THRESHOLD               = 5   ,
    // Number of frames difference before absolute seeking
    SEEK_THRESHOLD                  = 16  ,
    // Size of chunk of audio in table of contents
    AUDIO_CHUNKSIZE                 = 0x10000 ,
    // Minimum amount of data required to read a video header in streaming mode.
    VIDEO_STREAM_SIZE               = 0x100,
    // Number of samples in audio history
    AUDIO_HISTORY                   = 0x100000 ,
    // max number of samples returned by decoder per input frame
    AUDIO_MAX_DECODE                = 0x800 ,
    // Range to scan for pts after byte seek
    PTS_RANGE                       = 0x100000,
    // stream type
    FT_PROGRAM_STREAM               = 0x0001,
    FT_TRANSPORT_STREAM             = 0x0002,
    FT_AUDIO_STREAM                 = 0x0004,
    FT_VIDEO_STREAM                 = 0x0008,
    FT_IFO_FILE                     = 0x0010,
    FT_BD_FILE                      = 0x0020,
    SUBTITLE_HOLD_TIME              = 3,
    CCAPTION_HOLD_TIME              = 15,
  };

  enum color_model {
    // All color models supported for read_frame
    cmdl_BGR888=0,
    cmdl_BGRA8888=1,
    cmdl_RGB565=2,
    cmdl_RGB888=3,
    cmdl_RGBA8888=4,
    cmdl_RGBA16161616=5,
    // Color models for the 601 to RGB conversion
    // 601 not implemented for scalar code
    cmdl_601_BGR888=7,
    cmdl_601_BGRA8888=8,
    cmdl_601_RGB888=9,
    cmdl_601_RGBA8888=10,
    cmdl_601_RGB565=11,
    // next 2 are supported color models for read_yuvframe
    //  also YVU420P/YVU422P with uv output_row ptrs reversed for read_frame
    cmdl_YUV420P=12,
    cmdl_YUV422P=13,
    // the rest are supported color models for read_frame
    cmdl_601_YUV420P=14,
    cmdl_601_YUV422P=15,
    cmdl_UYVY=16,
    cmdl_YUYV=17,
    cmdl_601_UYVY=18,
    cmdl_601_YUYV=19,
    cmdl_YUV888=20,
    cmdl_YUVA8888=21,
    cmdl_601_YUV888=22,
    cmdl_601_YUVA8888=23,
  };

  // Values for audio input/output formats
  enum audio_format {
    afmt_IGNORE=-1,
    afmt_UNKNOWN=0,
    afmt_MPEG=1,
    afmt_AC3=2,
    afmt_PCM=3,
    afmt_AAC=4,
    afmt_JESUS=5,

    atyp_NONE=0,
    atyp_DOUBLE=1,
    atyp_FLOAT=2,
    atyp_INT=3,
    atyp_SHORT=4,
  };

  // Table of contents sections
  enum toc_section_type {
    toc_FILE_TYPE_PROGRAM=0,
    toc_FILE_TYPE_TRANSPORT=1,
    toc_FILE_TYPE_AUDIO=2,
    toc_FILE_TYPE_VIDEO=3,
    toc_STREAM_AUDIO=4,
    toc_STREAM_VIDEO=5,
    toc_STREAM_SUBTITLE=6,
    toc_OFFSETS_AUDIO=7,
    toc_OFFSETS_VIDEO=8,
    toc_ATRACK_COUNT=9,
    toc_VTRACK_COUNT=10,
    toc_STRACK_COUNT=11,
    toc_TITLE_PATH=12,
    toc_IFO_PALETTE=13,
    toc_FILE_INFO=14,
    toc_IFO_PLAYINFO=15,
    toc_SRC_PROGRAM=16,
  };


#ifdef USE_FUTEX
#define ZMPEG3_ZLOCK_INIT zzlock_t()
  class zloc_t {
  protected:
    volatile int loc;
    int zfutex(int op, int val, timespec *time=0) {
      return syscall(SYS_futex,&loc,op,val,time,0,0);
    }
    int zyield();
    int zgettid();
  public:
    int zwake(int nwakeups);
    int zwait(int val);
    int zwait() { return zwait(loc); }
    static void zincr(volatile int &v) { /* atomic(++v) */
      asm ( " lock incl %1\n" : "+m" (v) :: );
    }
    static void zdecr(volatile int &v) { /* atomic(--v) */
      asm ( " lock decl %1\n" : "+m" (v) :: );
    }
    static char tdecr(volatile int &v) {
      char ret; /* ret = atomic(--loc >= 0 ? 1 : 0) */
      asm ( " lock decl %1\n setge %0\n" : "=r" (ret), "+m" (v) :: );
      return ret;
    }
    static char tincr(volatile int &v) {
      char ret; /* ret = atomic(++loc > 0 ? 1 : 0) */
      asm ( " lock incl %1\n setg %0\n" : "=r" (ret), "+m" (v) :: );
      return ret;
    }
    static int zcmpxchg(int old, int val, volatile int &v) {
      int ret = old;
      asm volatile( " lock\n cmpxchgl %2,%1\n"
        : "+a" (ret), "+m" (v) :  "r" (val) : "memory" );
      return ret;
    }
    static int zxchg(int val, volatile int &v) {
      asm volatile( " xchgl %0,%1\n"
        : "+r" (val), "+m" (v) :: "memory" );
      return val;
    }
    static int zadd(int n, volatile int &v) {
      int old, mod, val;
      do { val = (old=v)+n; mod = zcmpxchg(old,val,v);
      } while( mod != old );
      return val;
    }
    static void zmfence() {
      asm volatile ( " mfence\n" ::: "memory" );
    }

    zloc_t() : loc(-1) {}
    ~zloc_t() {}
  };

  class zlock_t : zloc_t {
  protected:
    friend class zblock_t;
    friend class zrwlock_t;
    int zlock(int v);
    int zunlock(int nwakeups=1);
    static int zemsg1();
  public:
    int lock() {
      int v, ret = zunlikely( (v=zcmpxchg(-1,0,loc)) >= 0 ) ? zlock(v) : 0;
      return ret;
    }
    int unlock() {
      if( zunlikely(loc < 0) ) { return zemsg1(); }
      int v, ret = zunlikely( (v=zcmpxchg(0,-1,loc)) != 0 ) ? zunlock() : 0;
      return ret;
    }
    zlock_t() {}
    ~zlock_t() {}
  };

  class zblock_t : zlock_t {
  public:
    void block() { loc = 0; zwait(0); }
    void unblock() { if( !loc ) { loc = -1; zwake(INT_MAX); } }
    zblock_t() {}
    ~zblock_t() {}
  };

  class zrwlock_t : zloc_t {
    zlock_t lk;
    void zenter();
    void zleave();
    void zwrite_enter(int r); // r==0: do nothing, r>0: write_lock
    void zwrite_leave(int r); // r<0: write_lock while read_lock
  public:
    void enter() { zincr(loc); if( zunlikely( lk.loc >= 0 ) ) zenter(); }
    void leave() { if( zunlikely( !tdecr(loc) ) ) zleave(); }
    void write_enter(int r=1) { if( r ) zwrite_enter(r); }
    void write_leave(int r=1) { if( r ) zwrite_leave(r); }
    zrwlock_t() {}
    ~zrwlock_t() {}
  };

#else
#define ZMPEG3_ZLOCK_INIT { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER }

  class zlock_t {
  protected:
    pthread_mutex_t zlock;
  public:
    void lock() { pthread_mutex_lock(&zlock); }
    void unlock() { pthread_mutex_unlock(&zlock); }
    zlock_t() { pthread_mutex_init(&zlock, 0); }
    ~zlock_t() { pthread_mutex_destroy(&zlock); }
  };

  class zblock_t {
    pthread_mutex_t zblock;
    pthread_cond_t cond;
  public:
    zblock_t() {
      pthread_mutex_init(&zblock, 0);
      pthread_cond_init(&cond, 0);
    }
    ~zblock_t() {
      pthread_mutex_destroy(&zblock);
      pthread_cond_destroy(&cond);
    }
    void block() {
      pthread_mutex_lock(&zblock);
      pthread_cond_wait(&cond, &zblock);
      pthread_mutex_unlock(&zblock);
    }
    void unblock() { pthread_cond_broadcast(&cond); }
  };

  class zrwlock_t : zlock_t {
    volatile pthread_t blocking;
    volatile int users;
    zlock_t lk;
    pthread_cond_t cond;
    void wait() { pthread_cond_wait(&cond, &zlock); }
    void wake() { pthread_cond_signal(&cond); }
    void zwrite_enter(int r);
    void zwrite_leave(int r);
  public:
    void enter();
    void leave();
    void write_enter(int r=1) { if( r ) zwrite_enter(r); }
    void write_leave(int r=1) { if( r ) zwrite_leave(r); }
    int count() { return users; }

    zrwlock_t() { pthread_cond_init(&cond, 0); }
    ~zrwlock_t() { pthread_cond_destroy(&cond); }
  };

#endif

  // I/O

  // Filesystem structure
  // We buffer in IO_SIZE buffers.  Stream IO would require back
  // buffering a buffer since the stream must be rewound for packet headers,
  // sequence start codes, format parsing, decryption, and mpeg3cat.

  class fs_t {
    int sync() {
      return buffer->sync(current_byte);
    }
    int enter() {
      buffer->lock();
      return sync();
    }
    void leave() {
      buffer->unlock();
    }
  public:
    new_memset(0);
    class css_t;

    class css_t { // Encryption object
    public:
      class playkey_t;
      class keytext_t;

      typedef uint8_t key_t[5];
      typedef uint8_t blk_t[5];
      class playkey_t {
      public:
        int offset;
        key_t key;
      };
      class keytext_t {
      public:
        keytext_t *lt, *rt;
        uint8_t ktxt[4], ctxt[10];
        keytext_t(const uint8_t *kp, const uint8_t *cp) {
          lt = rt = 0;
          memcpy(&ktxt[0],kp,sizeof(ktxt));
          memcpy(&ctxt[0],cp,sizeof(ctxt));
        }
        ~keytext_t() {}
      } *key_root;
      void add_key_text(const uint8_t *kp, const uint8_t *cp);
      uint8_t *find_key_text(const uint8_t *kp);
      int attack_pattern(uint8_t const *sec, uint8_t *key);
      void del_keytexts(keytext_t *p);
    private:
      void css_engine(int v,uint8_t const *input, blk_t &output);
      void crypt_key1(int v, blk_t &key);
      void crypt_key2(blk_t &key);
      void crypt_bus_key(blk_t &key);
      int get_asf();
      int authenticate_drive(key_t &key);
      int hostauth(dvd_authinfo *ai);
      int get_title_key(key_t &key);
      int get_disk_key(key_t &key);
      int validate(int do_title);
      int validate_path(int do_title);
      int decrypt_title_key(uint8_t *dkey, uint8_t *tkey);
    public:
      key_t key1;
      key_t key2;
      key_t keycheck;
      key_t title_key;
      int lba, fsz, agid, encrypted;
      char device_path[STRLEN];    // Device where the file is located
      uint8_t disk_key[DVD_PACKET_SIZE];
      char challenge[10];
      int varient;
      int fd;
      char path[STRLEN];

      css_t();
      ~css_t();

      int get_keys(char *fpath);
      int decrypt_packet(uint8_t *sector, int offset);
      int crack_title_key(key_t &ckey, key_t &tkey);
    } css;

    enum access_strategy {
      io_SINGLE_ACCESS=1,
      io_UNBUFFERED=2,
      io_NONBLOCK=4,
      io_SEQUENTIAL=8,
      io_THREADED=16,
      io_ERRFAIL=32,
    };

    class buffer_t {
      void unblock();
      void block();
      void reader();
      static void *reader(void *the_buffer);
      void writer();
      static void *writer(void *the_buffer);
      int64_t read_in(int64_t len);
      int read_fin(int64_t len);
      int wait_in(int64_t pos);
      int seek_in(int64_t pos);
      int seek_to(int64_t pos, int64_t len);
      int read_to(int64_t pos);
      zmpeg3_t *src;
      zlock_t io_lock;          // buffer_t access lock
      zblock_t io_block;        // buffer_t access block
      int errs;                 // number of consecutive errors
      uint8_t *data;            // Readahead buffer
      int alloc;                // buffer byte allocate
      int size;                 // defined bytes in buffer
      int in;                   // buffer byte in, owned by read_in
      int out;                  // buffer byte out
      int64_t out_pos;          // file offset next byte from last read
      int64_t file_pos;         // file position of next readable byte
      int64_t file_nudge;       // sync position offset
      int fin;                  // locked version of in, synced with file_pos
      pthread_t the_reader;     // reader thread if io_THREADED
      pthread_t the_writer;     // writer thread if io_THREADED and recording
      pthread_t owner;          // for debugging
      int restarted;            // reader restarted
      int reader_done;          // reader thread termination flag
      int writer_done;          // writer thread termination flag
      int wout;                 // write record file buffer byte out
      int64_t write_pos;        // file_pos written to recd_fd
      zblock_t write_lock;      // write record block
    public:
      new_memset(0);
      buffer_t(zmpeg3_t *zsrc, int access);
      ~buffer_t();
      int access_type;          // io_ flags
      int paused;               // reader discard data
      int do_restart;           // reader restart buffer
      int ref_count;            // number of fs_t referencing this
      int open_count;           // number of is_open fs_t
      int fd;                   // unbuffered file descriptor
      FILE *fp;                 // buffered file handle
      int open_file(char *path);
      void close_file();
      void reset();
      void restart(int lk=1);
      int64_t tell() { return out_pos; }
      int64_t file_tell() { return file_pos; }
      int sync(int64_t pos);
      int errors() { return errs; }
      uint32_t next_byte() {
        return data[out];
      }
      uint32_t get_byte() {
        uint32_t result = next_byte();
        if( ++out >= alloc ) out = 0;
        ++out_pos;
        return result;
      }
      int read_out(uint8_t *bfr,int len);
      int get_fd() {
        return (access_type & io_UNBUFFERED) ? fd : fileno(fp);
      }
      void lock() {
        if( (access_type & io_SINGLE_ACCESS) )
          io_lock.lock();
        if( !data ) data = new uint8_t[alloc];
      }
      void unlock() {
        if( (access_type & io_SINGLE_ACCESS) )
          io_lock.unlock();
      }
      void start_reader();
      void stop_reader();
      int start_record();
      int stop_record();
      int write_align(int sz);
      void write_record(int sz, int mask);
    } *buffer;

    zmpeg3_t *src;
    char path[STRLEN];
    int64_t current_byte;       // Hypothetical position of file pointer
    int64_t total_bytes;
    int is_open;

    fs_t(zmpeg3_t *zsrc, const char *fpath, int access=io_UNBUFFERED+io_SINGLE_ACCESS);
    fs_t(zmpeg3_t *zsrc, int fd, int access=io_UNBUFFERED+io_SINGLE_ACCESS);
    fs_t(zmpeg3_t *zsrc, FILE *fp, int access=io_SINGLE_ACCESS);
    fs_t(fs_t &fs);
    ~fs_t();
    int open_file();
    void close_file();
    int errors() { return buffer ? buffer->errors() : 0; }
    int get_fd() { return buffer ? buffer->get_fd() : -1; }
    int read_data(uint8_t *bfr, int64_t len);
    uint8_t next_char();
    uint8_t read_char();
    uint32_t read_uint16();
    uint32_t read_uint24();
    uint32_t read_uint32();
    uint64_t read_uint64();
    int seek(int64_t byte);
    int seek_relative(int64_t bytes);
    int64_t tell() { return current_byte; }
    int64_t eof() { return current_byte >= total_bytes; }
    int64_t bof() { return current_byte < 0; }
    int64_t get_total_bytes();
    int64_t ztotal_bytes() { return total_bytes; }
    void chk_next() {
      if( ++current_byte >= buffer->file_tell() ) sync();
    }
    void restart();
    int pause_reader(int v);
    int access() { return !buffer ? 0 : buffer->access_type; }
    void sequential() { buffer->access_type |= io_SEQUENTIAL; }
    int start_record(int bsz);
    int stop_record();
    void attach(fs_t *zfs);
    void detach();
    void close_buffer();
  };


  // Bitstream                                      
  //    next bit in forward direction               
  //  next bit in reverse direction |               
  //                              v v               
  // ... | | | | | | | | |1|1|1|1|1|1|              
  //                     ^         ^                
  //                     |         bit_number = 1   
  //                     bfr_size = 6               

  class bits_t {
  public:
    new_memset(0);
    zmpeg3_t *src;
    uint32_t bfr;   // bfr = buffer for bits
    int bit_number; // position of pointer in bfr
    int bfr_size;   // number of bits in bfr.  Should always be a multiple of 8
    demuxer_t *demuxer; // Mpeg2 demuxer
    uint8_t *input_ptr; // when input_ptr!=0, data is read from it instead of the demuxer.

    bits_t(zmpeg3_t *zsrc, demuxer_t *demux);
    ~bits_t();
    int read_buffer(uint8_t *buffer, int byte);
    void use_ptr(uint8_t *buffer);
    int eos(uint8_t *ptr) {
      return input_ptr - (bit_number+7)/8 >= ptr;
    }
    int eos(uint8_t *ptr,int bits) {
      int n = bits - bit_number;
      if( n <= 0 ) return 0;
      return input_ptr + (n+7)/8 > ptr;
    }
    void use_demuxer();
    void start_reverse();
    void start_forward();
    int refill_ptr();
    int refill_noptr();
    int refill() { return input_ptr ? refill_ptr() : refill_noptr(); }
    int refill_reverse();
    int open_title(int title);
    int seek_byte(int64_t position);
    void reset() { bfr_size = bit_number = 0; }
    void reset(uint32_t v) { bfr = v; bfr_size = bit_number = 32; }
    int get_bit_offset() { return bit_number & 7; }
    void prev_byte_align() { bit_number = (bit_number+7) & ~7; }
    void next_byte_align() { bit_number &= ~7; }
    int bof() { return demuxer->zdata.bof() && demuxer->bof(); }
    int eof() { return demuxer->zdata.eof() && demuxer->eof(); }
    int error() { return demuxer->error(); }
    int64_t tell();
    int next_code(uint32_t zcode);
    int prev_code(uint32_t zcode);
    int next_start_code();
    void pack_bits(uint32_t v, int n) {
      bfr = (bfr << n) | v;
      if( (bfr_size+=n) > (int)(8*sizeof(bfr)) )
         bfr_size = 8*sizeof(bfr);
      bit_number += n;
    }
    void pack_8bits(uint32_t v) { pack_bits(v, 8); }
    void fill_bits_ptr(int bits) {
      while( bit_number < bits ) { pack_8bits(*input_ptr++); }
    }
    void fill_bits_noptr(int bits) {
      while( bit_number < bits ) { pack_8bits(demuxer->read_char()); }
    }
    void fill_bits(int bits) {
      return input_ptr ? fill_bits_ptr(bits) : fill_bits_noptr(bits);
    }
    uint32_t show_bits_noptr(int bits) {
      if( bits <= 0 ) return 0;
      fill_bits_noptr(bits);
      return (bfr >> (bit_number-bits)) & (0xffffffff >> (32-bits));
    }
    uint32_t show_bit_noptr(int bits) { return show_bits_noptr(1); }
    uint32_t show_byte_noptr() { return show_bits_noptr(8); }
    uint32_t show_bits24_noptr() { return show_bits_noptr(24); }
    uint32_t show_bits32_noptr() { fill_bits_noptr(32); return bfr; }
    int show_bits(int bits) {
      if( bits <= 0 ) return 0;
      fill_bits(bits);
      return (bfr >> (bit_number-bits)) & (0xffffffff >> (32-bits));
    }
    uint32_t get_bits_noptr(int bits) {
      uint32_t result = show_bits_noptr(bits);
      bit_number -= bits;
      return result;
    }
    uint32_t get_bit_noptr() { return get_bits_noptr(1); }
    int get_bits(int bits) {
      uint32_t result = show_bits(bits);
      bit_number -= bits;
      return result;
    }
    uint32_t get_byte_noptr() { return get_bits_noptr(8); }
    uint32_t get_bits24_noptr() { return get_bits_noptr(24); }
    void fill_bits_reverse(int bits) {
      int n = (bit_number & ~7);
      if( n ) { bfr >>= n;  bfr_size -= n;  bit_number -= n; }
      n = bit_number + bits;
      while( bfr_size < n ) {
        uint32_t prev_byte = input_ptr ?
          *--input_ptr : demuxer->read_prev_char();
        bfr |= prev_byte << bfr_size;
        bfr_size += 8;
      }
    }
    int show_bits_reverse(int bits) {
      fill_bits_reverse(bits);
      return (bfr >> bit_number) & (0xffffffff >> (32 - bits));
    }
    int get_bits_reverse(int bits) {
      uint32_t result = show_bits_reverse(bits);
      bit_number += bits;
      return result;
    }
  };


  class index_t {
  public:
    new_memset(0);
    float **index_data; // Buffer of frames for index.  A frame is a high/low pair.
    int index_allocated; // Number of frames allocated in each index channel.
    int index_channels; // Number of index channels allocated
    int index_size; // Number of high/low pairs in index channel
    int index_zoom; // Downsampling of index buffers when constructing index

    index_t();
    ~index_t();
  };


  class bitfont_t {
  public:
    class bitchar_t;
    static bitfont_t *fonts[];
    static int total_fonts;
    static int font_refs;
    static void init_fonts();
    static void destroy_fonts();
    class static_init_t {
    public:
      static_init_t();
      ~static_init_t();
    };
    class bitchar_t {
    public:
      uint16_t ch;
      int16_t left, right;
      int16_t ascent, decent;
      uint8_t *bitmap;
    } *chrs;
    int16_t nch, idx, idy, imy;
  };
  static bitfont_t *bitfont(int style,int pen_size,int italics,int size);

  class timecode_t {
  public:
    new_memset(0);
    int hour;
    int minute;
    int second;
    int frame;

    timecode_t() {}
    ~timecode_t() {}
  };


  class audio_t {
  public:
    new_memset(0);
    class imdct_complex_t;
    class imdct_al_table_t;
    class audio_decoder_layer_t;
    class audio_decoder_ac3_t;
    class audio_decoder_pcm_t;
    class ac3audblk_t; /*???*/

    enum {
      AC3_N                          = 512,
      MAXFRAMESIZE                   = 4096,
      MAXFRAMESAMPLEZ                = MAXFRAMESAMPLES,
      HDRCMPMASK                     = 0xfffffd00,
      SBLIMIT                        = 32,
      SSLIMIT                        = 18,
      SCALE_BLOCK                    = 12,
      MPEG3AUDIO_PADDING             = 1024,
      MAX_AC3_FRAMESIZE              = 1920 * 2 + 512,
    };

    enum exponent_type { // Exponent strategy constants
      exp_REUSE=0,
      exp_D15=1,
      exp_D25=2,
      exp_D45=3,
    };

    enum delta_type { // Delta bit allocation constants
      bit_REUSE=0,
      bit_NEW=1,
      bit_NONE=2,
      bit_RESERVED=3,
    };

    enum mpg_md { // Values for mode
      md_STEREO=0,
      md_JOINT_STEREO=1,
      md_DUAL_CHANNEL=2,
      md_MONO=3,
    };

    enum { PCM_HEADERSIZE = 20, };

    enum id3_state {
      id3_IDLE=0, // No ID3 tag found
      id3_HEADER=1, // Reading header
      id3_SKIP=2, // Skipping ID3 tag
    };

    // IMDCT variables
    class imdct_complex_t {
    public:
      float real;
      float imag;
    };

    class imdct_al_table_t {
    public:
      short bits;
      short d;
    };

    zmpeg3_t *src;
    atrack_t *track;

    class l3_info_t {
    public:
      int scfsi;
      uint32_t part2_3_length;
      uint32_t big_values;
      uint32_t scalefac_compress;
      uint32_t block_type;
      uint32_t mixed_block_flag;
      uint32_t table_select[3];
      uint32_t subblock_gain[3];
      uint32_t maxband[3];
      uint32_t maxbandl;
      uint32_t maxb;
      uint32_t region1start;
      uint32_t region2start;
      uint32_t preflag;
      uint32_t scalefac_scale;
      uint32_t count1table_select;
      float *full_gain[3];
      float *pow2gain;
    };

    class l3_sideinfo_t {
    public:
      uint32_t main_data_begin;
      uint32_t private_bits;
      class { public: l3_info_t gr[2]; } ch[2];
    };

    class audio_decoder_layer_t {
      static int tabsel_123[2][3][16];
      static long freqs[9];
      static float decwin[512 + 32];
      static float cos64[16], cos32[8], cos16[4], cos8[2], cos4[1];
      static float *pnts[5];
      static int grp_3tab[32 * 3];   /* used: 27 */
      static int grp_5tab[128 * 3];  /* used: 125 */
      static int grp_9tab[1024 * 3]; /* used: 729 */
      static float muls[27][64];     /* also used by layer 1 */
      static float gainpow2[256 + 118 + 4];
      static long intwinbase[257];
      static float ispow[8207];
      static float aa_ca[8], aa_cs[8];
      static float win[4][36];
      static float win1[4][36];
      static float COS1[12][6];
      static float COS9[9];
      static float COS6_1, COS6_2;
      static float tfcos36[9];
      static float tfcos12[3];
      static float cos9[3], cos18[3];
      static float tan1_1[16], tan2_1[16], tan1_2[16], tan2_2[16];
      static float pow1_1[2][16], pow2_1[2][16];
      static float pow1_2[2][16], pow2_2[2][16];
      static int longLimit[9][23];
      static int shortLimit[9][14];
      static class bandInfoStruct {
      public:
        int longIdx[23], longDiff[22];
        int shortIdx[14], shortDiff[13];
      } bandInfo[9];
      static int mapbuf0[9][152];
      static int mapbuf1[9][156];
      static int mapbuf2[9][44];
      static int *map[9][3];
      static int *mapend[9][3];
      static uint32_t n_slen2[512]; /* MPEG 2.0 slen for 'normal' mode */
      static uint32_t i_slen2[256]; /* MPEG 2.0 slen for intensity stereo */

      static short tab0 [  1], tab1 [  7], tab2  [17],  tab3[ 17];
      static short tab5 [ 31], tab6 [ 31], tab7  [71],  tab8[ 71];
      static short tab9 [ 71], tab10[127], tab11[127], tab12[127];
      static short tab13[511], tab15[511], tab16[511], tab24[511];
      static short tab_c0[31], tab_c1[31];
      static class huffman_t {
      public:
        unsigned int linbits;
        short *table;
      } ht[32], htc[2];

      static int dct64_1(float *out0,float *out1,float *b1,float *b2,
        float *samples);
      static int dct64(float *a,float *b,float *c);
      static int dct36(float *inbuf,float *o1,float *o2,float *wintab,
        float *tsbuf);
      static int dct12(float *in,float *rawout1,float *rawout2,float *wi,
        float *ts);

      int get_scale_factors_1(int *scf,l3_info_t *l3_info,int ch,int gr);
      int get_scale_factors_2(int *scf,l3_info_t *l3_info,int i_stereo);
      int dequantize_sample(float xr[SBLIMIT][SSLIMIT],int *scf,
         l3_info_t *l3_info,int sfreq,int part2bits);
      int get_side_info(l3_sideinfo_t *si,int channels,int ms_stereo,
         long sfreq,int single,int lsf);
      int hybrid(float fsIn[SBLIMIT][SSLIMIT],
         float tsOut[SSLIMIT][SBLIMIT],int ch,l3_info_t *l3_info);
      int antialias(float xr[SBLIMIT][SSLIMIT],l3_info_t *l3_info);
      int calc_i_stereo(float xr_buf[2][SBLIMIT][SSLIMIT],int *scalefac,
         l3_info_t *l3_info,int sfreq,int ms_stereo,int lsf);
      int synth_stereo(float *bandPtr,int channel,float *out, int *pnt);
      int init_audio(zmpeg3_t *zsrc, atrack_t *ztrack, int zformat);
      int synths_reset();
      int select_table();
      int step_one(uint8_t *bit_alloc, int *scale);
      int step_two(uint8_t *bit_alloc, float fraction[2][4][SBLIMIT],
         int *scale, int x1);
    public:
      new_memset(0);
      bits_t *stream;
      // Layer 3
      uint8_t *bsbuf, *prev_bsbuf;
      uint8_t bsspace[2][MAXFRAMESIZE + 512]; // MAXFRAMESIZE
      int bsnum;
      // For mp3 current framesize without header.  For AC3 current framesize with header.
      long framesize;
      long prev_framesize, past_framesize;
      int channels;
      int samplerate;
      int single;
      int sampling_frequency_code;
      int error_protection;
      int mode;
      int mode_ext;
      int lsf;
      long ssize;
      int mpeg35;
      int padding;
      int layer;
      int extension;
      int copyright;
      int original;
      int emphasis;
      int bitrate;
      // Static variable in synthesizer
      int bo;                      
      // Ignore first frame after a seek
      int first_frame;
      float synth_stereo_buffs[2][2][0x110];
      float synth_mono_buff[64];
      float mp3_block[2][2][SBLIMIT * SSLIMIT];
      int mp3_blc[2];

      // State of ID3 parsing
      int id3_state;
      int id3_current_byte;
      int id3_size;
      // Layer 2
      int bitrate_index;
      imdct_al_table_t *alloc;
      int jsbound;
      int II_sblimit;
      uint8_t layer2_scfsi_buf[64];

      void layer_reset();
      static int layer_check(uint8_t *data);
      static int id3_check(uint8_t *data);
      int layer3_header(uint8_t *data);
      audio_decoder_layer_t();
      ~audio_decoder_layer_t();
      int do_layer2(uint8_t *zframe,int zframe_size,float **zoutput,int render);
      int do_layer3(uint8_t *zframe,int zframe_size,float **zoutput,int render);
      int init_layer2();
      int init_layer3();
      int init_decode_tables();
    } *layer_decoder;

    class audio_decoder_ac3_t {
    public:
      new_memset(0);
      static int ac3_samplerates[3];

      bits_t *stream;
      int samplerate;
      int bitrate;
      int flags;
      int channels;
      a52_state_t *state;
      sample_t *output;
      int framesize;
      static int ac3_check(uint8_t *data);
      int ac3_header(uint8_t *data);
      int do_ac3(uint8_t *zframe, int zframe_size, float **zoutput, int render);
      audio_decoder_ac3_t();
      ~audio_decoder_ac3_t();
    } *ac3_decoder;

    class audio_decoder_pcm_t {
    public:
      new_memset(0);
      int samplerate;
      int bits;
      int channels;
      int framesize;
      int pcm_header(uint8_t *data);
      int do_pcm(uint8_t *zframe, int zframe_size, float **zoutput, int render);
      audio_decoder_pcm_t() {}
      ~audio_decoder_pcm_t() {}
    } *pcm_decoder;

    int channels;            // channels from read_header
    int samplerate;          // samplerate from read_header
    int framenum;            // Number of current frame being decoded
    int framesize;           // Size of frame including header
    int64_t start_byte;      // First byte of audio data in the file
    int output_channels;     // number of defined output buffers
    float **output;          // Output from synthesizer in linear floats
    int output_size;         // Number of pcm samples in the buffer
    int output_allocated;    // Allocated number of samples in output
    int64_t output_position; // Sample position in file of start of output buffer
    int64_t sample_seek;     // Perform a seek to the sample
    int64_t byte_seek;       // Perform a seek to the absolute byte
    // +/- number of samples of difference between audio and video
    int seek_correction;
    // Buffer containing current packet
    uint8_t packet_buffer[MAXFRAMESIZE];
    // Position in packet buffer of next byte to read
    int packet_position;

    audio_t() {}
    int init_audio(zmpeg3_t *zsrc, atrack_t *ztrack, int zformat);
    ~audio_t();

    int rewind_audio();
    int read_header();
    void update_channels();
    int audio_pts_padding();
    int audio_pts_skipping(int samples);
    void update_audio_history();
    int read_frame(int render);
    int get_length();
    int seek();
    int seek_byte(int64_t byte);
    int seek_sample(long sample);
    int read_raw(uint8_t *output,long *size,long max_size);
    void shift_audio(int64_t diff);
    int decode_audio(void *output_v,int atyp,int channel,int len);
    int64_t audio_position() { return output_position + output_size; }
  };

  class VLCtab_t {
  public:
    int8_t val, len;
  };

  class DCTtab_t {
  public:
    int8_t run, level;
    int16_t len;
  };

  class slice_buffer_t {
    static VLCtab_t DClumtab0[32];
    static VLCtab_t DClumtab1[16];
    static VLCtab_t DCchromtab0[32];
    static VLCtab_t DCchromtab1[32];
  
  public:
    slice_buffer_t *next;    // avail list link
    uint8_t *data;           // Buffer for holding the slice data
    video_t *video;          // decoder
    int buffer_size;         // Size of buffer
    int buffer_allocation;   // Space allocated for buffer 
    int buffer_position;     // Position in buffer
    uint32_t bits;
    int bits_size;

    int eob() { return buffer_position >= buffer_size; }
    uint32_t get_data_byte() { return data[buffer_position++]; }
    void fill_bits(int nbits) {
      while( bits_size < nbits ) {
        bits <<= 8;
        if( !eob() ) bits |= get_data_byte();
        bits_size += 8;
      }
    }
    void flush_bits(int nbits) { fill_bits(nbits); bits_size -= nbits; }
    void flush_bit() { flush_bits(1); }
    uint32_t show_bits(int nbits) {
      fill_bits(nbits);
      return (bits >> (bits_size-nbits)) & (0xffffffff >> (32-nbits));
    }
    uint32_t get_bits(int nbits) {
      uint32_t result = show_bits(nbits);
      bits_size -= nbits;
      return result;
    }
    uint32_t get_bit() { return get_bits(1); }
    int get_dc_lum();
    int get_dc_chrom();
    uint8_t *expand_buffer(int bfrsz);
    void fill_buffer(bits_t *vstream);
    int ext_bit_info();

    slice_buffer_t();
    ~slice_buffer_t();
  };


  class video_t {
    static uint8_t zig_zag_scan_nommx[64]; // zig-zag scan
    static uint8_t alternate_scan_nommx[64]; // alternate scan
    // default intra quantization matrix
    static uint8_t default_intra_quantizer_matrix[64];
    // Frame rate table must agree with the one in the encoder
    static double frame_rate_table[16];

    inline void recon_comp(uint8_t *s, uint8_t *d,
           int lx, int lx2, int h, int type);
    void recon(uint8_t *src[], int sfield,
           uint8_t *dst[], int dfield, int lx, int lx2,
           int w, int h, int x, int y, int dx, int dy, int addflag);
    int delete_decoder();
    void init_scantables();
    int read_frame_backend(int skip_bframes);
    int *get_scaletable(int input_w, int output_w);
    int get_gop_header();
    long gop_to_frame(timecode_t *gop_timecode);
    int seek();

  public:
    new_memset(0);
    class cc_t;

    enum chroma_format {
      cfmt_420=1,
      cfmt_422=2,
      cfmt_444=3,
    };

    enum ext_start_code {
      ext_id_SEQ=1,
      ext_id_DISP=2,
      ext_id_QUANT=3,
      ext_id_SEQSCAL=5,
      ext_id_PANSCAN=7,
      ext_id_CODING=8,
      ext_id_SPATSCAL=9,
      ext_id_TEMPSCAL=10,
    };

    enum picture_type {
      pic_type_I=1,
      pic_type_P=2,
      pic_type_B=3,
      pic_type_D=4,
    };

    enum picture_structure {
      pics_TOP_FIELD=1,
      pics_BOTTOM_FIELD=2,
      pics_FRAME_PICTURE=3,
    };

    slice_buffer_t *slice_buffers, *avail_slice_buffers;
    int total_slice_buffers;
    void allocate_slice_buffers();
    void delete_slice_buffers();
    void reallocate_slice_buffers();
    zlock_t slice_lock, slice_wait, slice_active;
    int slice_wait_locked, slice_active_locked;

    zmpeg3_t *src;
    vtrack_t *track;

    // ================================= Seeking variables =========================
    bits_t *vstream;
    int decoder_initted;
    uint8_t **output_rows;     // Output frame buffer supplied by user
    int in_x, in_y, in_w, in_h, out_w, out_h; // Output dimensions
    int row_span;
    int *x_table, *y_table;          // Location of every output pixel in the input
    int color_model;
    int want_yvu;                    // Want to return a YUV frame
    char *y_output, *u_output, *v_output; // Output pointers for a YUV frame

    int blockreadsize;
    int maxframe;          // Max value of frame num to read
    int64_t byte_seek;     // Perform absolute byte seek before the next frame is read
    int frame_seek;        // Perform a frame seek before the next frame is read
    int framenum;          // Number of the next frame to be decoded
    int last_number;       // Last framenum rendered
    int ref_frames;        // ref_frames since last seek
    int found_seqhdr;
    int bitrate;
    timecode_t gop_timecode;     // Timecode for the last GOP header read.
    int has_gops; // Some streams have no GOPs so try sequence start codes instead

    class cc_t {        // close captioning (atsc only)
      video_t *video;
      uint8_t data[3][128];   // reorder buffers
      int frs_history, font_size;
      void decode(int idx);
    public:
      class svc_t;
  
      enum {
        MX_SVC = 64, MX_WIN=8,MX_CX=48,MX_CY=16,
        /* frame struct history */
        frs_tf1 = 0x01, /* top field in data[1] */
        frs_bf2 = 0x02, /* bottom field in data[2] */
        frs_frm = 0x04, /* frame in data[1] */
        frs_prc = 0x08, /* frame processed */
        st_unmap=-1, st_hidden=0, st_visible=1, /* state */
        dir_rt=0, dir_lt=1, dir_up=2, dir_dn=3, /* print/scroll/effect dir */
        jfy_left=0, jfy_right=1, jfy_center=2, jfy_full=3, /* justify */
        eft_snap = 0, eft_fade = 1, eft_wipe = 2, /* effect */
        oty_solid=0, oty_flash=1, oty_translucent=2,
        oty_transparent=3, bdr_width=5, /* opacity/border width */
        edg_none=0, edg_raised=1, edg_depressed=2, edg_uniform=3,
        edg_shadow_left=4, edg_shadow_right=5, /* edge type */
        psz_small=0, psz_standard=1, psz_large=2, /* pen size */
        fst_default=0, fst_mono_serif=1, fst_prop_serif=2,
        fst_mono_sans=3, fst_prop_sans=4, fst_casual=5,
        fst_cursive=6, fst_small_caps=7, /* font style */
        fsz_small=0, fsz_normal=1, fsz_large=2, /* font size */
        tag_dialog=0, tag_source_id=1, tag_device=2,
        tag_dialog_2=3, tag_voiceover=4, tag_audible_transl=5,
        tag_subtitle_transl=6, tag_voice_descr=7, tag_lyrics=8,
        tag_effect_descr=9, tag_score_descr=10, tag_expletive=11,
        tag_not_displayable=15, /* text tag */
        ofs_subscript=0, ofs_normal=1, ofs_superscript=2, /* offset */
      };
      /* color RGB 2:2:2 (lsb = B). */
      class svc_t {
        cc_t *cc;
        int sid, ctrk, size;
        uint8_t *bp, *ep;
        uint8_t data[128];
      public:
        class chr_t {
        public:
          uint16_t glyph;
          uint8_t fg_opacity : 2;
          uint8_t fg_color : 6;
          uint8_t bg_opacity : 2;
          uint8_t bg_color : 6;
          uint8_t pen_size : 2;
          uint8_t edge_color : 6;
          uint8_t italics : 1;
          uint8_t edge_type : 3;
          uint8_t underline : 1;
          uint8_t font_style : 3; 
          uint8_t font_size : 2; 
          uint8_t offset : 2; 
          uint8_t text_tag : 4;
          uint8_t rsvd2;
        } *bfrs;
        chr_t *get_bfr();
        void bfr_put(chr_t *&bfr);

        class win_t;
        class win_t {
        public:
          class chr_attr_t;
          int8_t id, st, pdir, sdir;
          uint8_t cx, cy, cw, ch; /* pen/dim in bytes */
          int ax, ay, x, y, bw; /* anchor/pen/border in pixels */
          uint32_t fg_yuva, bg_yuva, edge_yuva; /* pen color */
          uint32_t fill_yuva, border_yuva; /* window color */
          int start_frame, end_frame;
          svc_t *svc;
          subtitle_t *subtitle;
          uint8_t anchor_point, anchor_horizontal, anchor_vertical;
          uint8_t anchor_relative, row_lock, col_lock, dirty;
          uint8_t priority, window_style, pen_style;
          /* pen style properties */
          uint8_t pen_size, font_style, offset, italics, underline;
          uint8_t fg_color, fg_opacity, bg_color, bg_opacity;
          uint8_t text_tag, edge_color, edge_type, scale;
          /* window style properties */
          uint8_t justify, wordwrap, fill_opacity, fill_color;
          uint8_t display_effect, effect_speed, effect_direction;
          uint8_t border_type, border_color, border_width;
          chr_t *bfr;

          win_t();
          ~win_t();
          void reset(int i,svc_t *svc);
          int BS();   int FF();   int CR();   int HCR();
          int init(int ncw, int nch);
          int resize(int ncw, int nch);
          static int is_breakable(int b);
          int wrap();
          int ovfl();
          int store(int ch);
          void set_window_style(int i);
          void set_pen_style(int i);
          void default_window();
          void print_buffer();
          void hbar(int x, int y, int ww, int hh, int lm, int rm, int clr);
          void vbar(int x, int y, int ww, int hh, int tm, int bm, int clr);
          void border(int ilt, int olt, int irt, int ort,
                      int iup, int oup, int idn, int odn);
          void put_chr(chr_t *chr);
          static bitfont_t *chr_font(chr_t *bp, int scale);
          void render();
          void output_text();
          void set_state(int8_t st);
        } win[MX_WIN], *curw;    
        svc_t(cc_t *cc);
        ~svc_t();
        int id() { return sid; }
        int trk() { return ctrk; }
        void append(uint8_t *bp, int len);
        void decode();
        int render();
        void reset(int sid,cc_t *cc);
        int CWx(int id);  int DFz(int id);
        int DFx(int id);  int RSVD();   int ETX();
        int BS();   int FF();   int CR();   int HCR();
        int RST();  int DLC();  int DLW();  int DLY();
        int CLW();  int DSW();  int HDW();  int TGW();
        int SPA();  int SPC();  int SPL();  int SWA();
        int command(int cmd);
        static uint32_t pen_yuva(int color, int opacity);
      } svc;
  
      cc_t(video_t *video);
      ~cc_t();
      void reset();
      void get_atsc_data(bits_t *v);
      void decode();
      void reorder();
      zcc_text_cb text_cb;
    } *cc;
    cc_t *get_cc();
    int subtitle_track; // Subtitle track to composite if >= 0
    int show_subtitle(int strk);

    // These are only available from elementary streams.
    int frames_per_gop;       // Frames per GOP after the first GOP.
    int first_gop_frames;     // Frames in the first GOP.
    int first_frame;     // Number of first frame stored in timecode
    int last_frame;      // Last frame in file

    // ================================= Compression variables =====================
    // Malloced frame buffers.  2 refframes are swapped in and out.
    // while only 1 auxframe is used.
    uint8_t *yuv_buffer[5];  // Make YVU buffers contiguous for all frames
    uint8_t *oldrefframe[3], *refframe[3], *auxframe[3];
    uint8_t *llframe0[3], *llframe1[3];
    uint8_t *zigzag_scan_table;
    uint8_t *alternate_scan_table;
    // Source for the next frame presentation
    uint8_t *output_src[3];
    // Pointers to frame buffers.
    uint8_t *newframe[3];
    uint8_t *tdat;
    int horizontal_size, vertical_size, mb_width, mb_height;
    int coded_picture_width,  coded_picture_height;
    int chroma_format, chrom_width, chrom_height, blk_cnt;
    int8_t pict_type, pict_struct, field_sequence, intravlc;
    int8_t forw_r_size, back_r_size, full_forw, full_back;
    int8_t prog_seq, prog_frame, repeatfirst, secondfield;
    int8_t h_forw_r_size, v_forw_r_size, h_back_r_size, v_back_r_size;
    int8_t dc_prec, topfirst, frame_pred_dct, conceal_mv;
    union {
      struct { int8_t got_top, got_bottom, repeat_fields, current_field; };
      int32_t repeat_data;
    };
    int skip_bframes;
    int frame_time, seek_time;
    int stwc_table_index, llw, llh, hm, hn, vm, vn;
    int lltempref, llx0, lly0, llprog_frame, llfieldsel;
    int matrix_coefficients;
    int framerate_code;
    double frame_rate;
    int *cr_to_r, *crb_to_g, *cb_to_b;
    int intra_quantizer_matrix[64], non_intra_quantizer_matrix[64];
    int chroma_intra_quantizer_matrix[64], chroma_non_intra_quantizer_matrix[64];
    int mpeg2;
    int skim;                     // skip reconstruction/motion
    int thumb, ithumb;            // thumbnail scan/ thumbnail frame ready 
    int qscale_type, altscan;     // picture coding extension
    int pict_scal;                // picture spatial scalable extension
    int scalable_mode;            // sequence scalable extension

    // Subtitling frame
    uint8_t *subtitle_frame[3];

    video_t(zmpeg3_t *zsrc, vtrack_t *ztrack);
    ~video_t();
    void init_video();
    int seek_video() { return seek(); }
    void reset_subtitles();
    int eof();

    int set_cpus(int cpus);
    int set_mmx(int use_mmx);
    void cache_frame();
    int drop_frames(long frames, int cache_it);
    int colormodel();
    int read_frame(uint8_t **output_rows,
      int in_x, int in_y, int in_w, int in_h,
      int out_w, int out_h, int color_model);
    int read_yuvframe(char *y_output, char *u_output, char *v_output,
      int in_x, int in_y, int in_w, int in_h);
    int read_yuvframe_ptr(char **y_output, char **u_output, char **v_output);
    int read_raw(uint8_t *output, long *size, long max_size);
    int seek_frame(long frame);
    int seek_byte(int64_t byte);
    int rewind_video(int preload=1);
    int previous_frame();
    int init_decoder();
    void new_output();
    int reconstruct( int bx, int by, int mb_type, int motion_type,
           int PMV[2][2][2], int mv_field_sel[2][2],
           int dmvector[2], int stwtype);
    void dump();
    slice_buffer_t *get_slice_buffer();
    void put_slice_buffer(slice_buffer_t* buffer);
    int get_macroblocks();
    int display_second_field();
    int video_pts_padding();
    int video_pts_skipping();
    int read_picture();
    int get_picture();
    int get_seq_hdr();
    int sequence_extension();
    int sequence_display_extension();
    int quant_matrix_extension();
    int sequence_scalable_extension();
    int picture_display_extension();
    int picture_coding_extension();
    int picture_spatial_scalable_extension();
    int picture_temporal_scalable_extension();
    int ext_user_data();
    int get_picture_hdr();
    int find_header();
    int get_header();
    void overlay_subtitle(subtitle_t *subtitle);
    void decode_subtitle();
    int init_output();
    int dither_frame(uint8_t *yy, uint8_t *uu, uint8_t *vv,uint8_t **output_rows);
    int dither_frame444(uint8_t *yy, uint8_t *uu, uint8_t *vv);
    int dithertop(uint8_t *yy, uint8_t *uu, uint8_t *vv);
    int dithertop444(uint8_t *yy, uint8_t *uu, uint8_t *vv);
    int ditherbot(uint8_t *yy, uint8_t *uu, uint8_t *vv);
    int ditherbot444(uint8_t *yy, uint8_t *uu, uint8_t *vv);
    int present_frame(uint8_t *yy, uint8_t *uu, uint8_t *vv);
    void calc_dmv(int DMV[][2],int *dmvector,int mvx,int mvy);
    int is_refframe() { return pict_type == pic_type_B ? 0 :
      pict_struct == pics_FRAME_PICTURE ? 1 : secondfield;
    }
  };


  class slice_decoder_t {
  public:
    static VLCtab_t PMBtab0[8];
    static VLCtab_t PMBtab1[8];
    static VLCtab_t BMBtab0[16];
    static VLCtab_t BMBtab1[8];
    static VLCtab_t spIMBtab[16];
    static VLCtab_t spPMBtab0[16];
    static VLCtab_t spPMBtab1[16];
    static VLCtab_t spBMBtab0[14];
    static VLCtab_t spBMBtab1[12];
    static VLCtab_t spBMBtab2[8];
    static VLCtab_t SNRMBtab[8];
    static VLCtab_t MVtab0[8];
    static VLCtab_t MVtab1[8];
    static VLCtab_t MVtab2[12];
    static VLCtab_t CBPtab0[32];
    static VLCtab_t CBPtab1[64];
    static VLCtab_t CBPtab2[8];
    static VLCtab_t MBAtab1[16];
    static VLCtab_t MBAtab2[104];

    static DCTtab_t DCTtabfirst[12];
    static DCTtab_t DCTtabnext[12];
    static DCTtab_t DCTtab0[60];
    static DCTtab_t DCTtab0a[252];
    static DCTtab_t DCTtab1[8];
    static DCTtab_t DCTtab1a[8];
    static DCTtab_t DCTtab2[16];
    static DCTtab_t DCTtab3[16];
    static DCTtab_t DCTtab4[16];
    static DCTtab_t DCTtab5[16];
    static DCTtab_t DCTtab6[16];

    // non-linear quantization coefficient table
    static uint8_t non_linear_mquant_table[32];

    enum macroblock_type {
      mb_INTRA=1,
      mb_PATTERN=2,
      mb_BACKWARD=4,
      mb_FORWARD=8,
      mb_QUANT=16,
      mb_WEIGHT=32,
      mb_CLASS4=64,
    };

    enum motion_type {
      mc_FIELD=1,
      mc_FRAME=2,
      mc_16X8=2,
      mc_DMV=3,
    };

    enum mv_format {
      mv_FIELD=0,
      mv_FRAME=1,
    };

    enum scalable_mode {
      sc_NONE=0,
      sc_DP=1,
      sc_SPAT=2,
      sc_SNR=3,
      sc_TEMP=4,
    };

    slice_decoder_t *next;
    zmpeg3_t *src;
    video_t *video;
    pthread_t owner;        // for debugging
    slice_buffer_t *slice_buffer;
    int fault;
    int done;
    int quant_scale;
    int pri_brk;            // slice/macroblock
    short block[12][64];
    int sparse[12];
    pthread_t tid;          // ID of thread
    zlock_t input_lock;

    int val, sign, idx;
    int get_cbp();
    int clear_block(int comp, int size);
    static uint16_t *DCTlutab[3];
    static uint16_t lu_pack(DCTtab_t *lp) {
      int run = lp->run>=64 ? lp->run-32 : lp->run;
      return run | ((lp->len-1)<<6) | (lp->level<<10);
    }
    static int lu_run(uint16_t lu) { return lu & 0x3f; }
    static int lu_level(uint16_t lu) { return lu >> 10; }
    static int lu_len(uint16_t lu) { return ((lu>>6) & 0x0f) + 1; }
    static void init_lut(uint16_t *&lutbl, DCTtab_t *tabn, DCTtab_t *tab0, DCTtab_t *tab1);
    static void init_tables();
    int get_coef(uint16_t *lut);
    int get_mpg2_coef(uint16_t *lut);
    void get_intra_block(int comp, int dc_dct_pred[]);
    void get_mpg2_intra_block(int comp, int dc_dct_pred[]);
    void get_inter_block(int comp);
    void get_mpg2_inter_block(int comp);
    int get_slice_hdr();
    int decode_slice();
    void slice_loop();
    static void *the_slice_loop(void *the_slice_decoder);
    int get_mv();
    int get_dmv();
    void motion_vector(int *PMV, int *dmvector, int h_r_size, int v_r_size,
      int dmv, int mvscale, int full_pel_vector);
    int motion_vectors(int PMV[2][2][2], int dmvector[2], int mv_field_sel[2][2],
      int s, int mv_count, int mv_format, int h_r_size, int v_r_size,
      int dmv, int mvscale);
    int get_macroblock_address();
    inline int getsp_imb_type();
    inline int getsp_pmb_type();
    inline int getsp_bmb_type();
    inline int get_imb_type();
    inline int get_pmb_type();
    inline int get_bmb_type();
    inline int get_dmb_type();
    inline int get_snrmb_type();
    int get_mb_type();
    int macroblock_modes(int *pmb_type,int *pstwtype, int *pstwclass,
       int *pmotion_type,int *pmv_count,int *pmv_format,int *pdmv,
       int *pmvscale,int *pdct_type);
    int add_block(int comp,int bx,int by,int dct_type,int addflag);
    static void idct_conversion(short* block);
    int get_active_slice_buffer();

    slice_decoder_t();
    ~slice_decoder_t();
  } *slice_decoders, *avail_slice_decoders;

  int total_slice_decoders;
  slice_buffer_t *active_slice_buffers;
  zlock_t decoder_lock, decoder_active;
  int decoder_active_locked;
  void allocate_slice_decoders();
  void delete_slice_decoders();
  void reallocate_slice_decoders();
  void decode_slice(slice_buffer_t* buffer);
  

  class subtitle_t {
  public:
    new_memset(0);
    uint8_t *data;   // Raw data of subtitle
    int data_allocated, data_used;
    int id;         // Number of stream starting at 0x20
    int done;       // -1 avail, 0 read in, 1 decode
    int active;     // 1 subtitle is contructed, -1 subtitle is close caption
    int draw;       // -1 never draw, 0 draw based on frame time, 1 always draw
    int64_t offset; // Program offset of start of subtitle
    int force;      // 1 Force display, -1 force delete
    // image in YUV444
    int w, h, sz;
    uint8_t *image_y, *image_u, *image_v, *image_a;
    int x1, x2, y1, y2;
    int start_time; // Time after detection of subtitle to display it in msec
    int stop_time;  // Time after detection of subtitle to hide it in msec
    int palette[4]; // Indexes in the main palette
    int alpha[4];
    int start_frame, stop_frame; // framenum start/stop drawing
    int frame_time; // presentation frame_time at creation

    subtitle_t();
    subtitle_t(int nid, int ww, int hh);
    ~subtitle_t();

    void realloc_data(int count);
    int decompress_subtitle(zmpeg3_t *zsrc);
    void set_image_size(int isz);
    void set_image_size(int ww, int hh);
    int decode(video_t *video);
  };

  class demuxer_t {
    friend class bits_t;

    uint8_t packet_next_char() { return raw_data[raw_offset]; }
    uint8_t packet_read_char() { return raw_data[raw_offset++]; }
    uint32_t packet_read_int16() {
      uint32_t a = raw_data[raw_offset++];
      uint32_t b = raw_data[raw_offset++];
      return (a << 8) | b;
    }
    uint32_t packet_next_int24() {
      uint32_t a = raw_data[raw_offset+0];
      uint32_t b = raw_data[raw_offset+1];
      uint32_t c = raw_data[raw_offset+2];
      return (a << 16) | (b << 8) | c;
    }
    uint32_t packet_read_int24() {
      uint32_t a = raw_data[raw_offset++];
      uint32_t b = raw_data[raw_offset++];
      uint32_t c = raw_data[raw_offset++];
      return (a << 16) | (b << 8) | c;
    }
    uint32_t packet_read_int32() {
      uint32_t a = raw_data[raw_offset++];
      uint32_t b = raw_data[raw_offset++];
      uint32_t c = raw_data[raw_offset++];
      uint32_t d = raw_data[raw_offset++];
      return (a << 24) | (b << 16) | (c << 8) | d;
    }
    void packet_skip(int length) { raw_offset += length; }
    int get_adaptation_field();
    int get_program_association_table();
    int get_transport_payload(int is_audio, int is_video);
    int get_pes_packet_header(uint64_t *pts, uint64_t *dts);
    int get_unknown_data();
    int get_transport_pes_packet();
    int get_pes_packet();
    int get_payload();
    int read_transport();
    int get_system_header();
    uint64_t get_timestamp();
    int get_pack_header();
    int get_program_payload(int bytes, int is_audio, int is_video);
    int handle_scrambling(int decryption_offset);
    void del_subtitle(int idx);
    subtitle_t* get_subtitle(int id,int64_t offset);
    void handle_subtitle(zmpeg3_t *src, int stream_id, int bytes);
    int handle_pcm(int bytes);
    int get_program_pes_packet(uint32_t header);
    int previous_code(uint32_t zcode);

  public:
    new_memset(0);
    class zstream_t;
    class title_t;
    class nav_t {
    public:
#include "nav.h"
      new_memset(0);
    } *nav;

    zmpeg3_t* src;
    uint8_t *raw_data; // One unparsed packet.
    int raw_offset; // Offset in raw_data of read pointer
    int raw_size; // Amount loaded in last raw_data

    // Elementary stream data when only one stream is to be read.
    // Erased in every call to read a packet.
    class zstream_t {
    public:
      new_memset(0);
      uint8_t *buffer;
      int allocated; // Allocation of data_buffer
      int size; // Position in data_buffer of write pointer
      int position; // Position in data_buffer of read pointer
      int start; // Start of the next pes packet
      int length() { return size - position; }
      bool eof() { return position >= size; }
      bool bof() { return position == 0; }
    } zdata;
    // Elementary stream data when all streams are to be read.  There is no
    // read pointer since data is expected to be copied directly to a track.
    // Some packets contain audio and video.  Further division into
    // stream ID may be needed.
    zstream_t zaudio;
    zstream_t zvideo;

    subtitle_t *subtitles[MAX_SUBTITLES];
    int total_subtitles;

    // What type of data to read, which track is accessing
    atrack_t *do_audio;
    vtrack_t *do_video;
    int read_all;
    // Direction of reads
    int reverse;
    // Set to 1 when eof or attempt to read before beginning
    int error_flag;
    int discontinuity;
    // Temp variables for returning
    uint8_t next_char;
    // Info for mpeg3cat
    int64_t last_packet_start;
    int64_t last_packet_end;
    int64_t last_packet_decryption;

  // Table of contents
    class title_t { // Titles
      void extend_cell_table();
    public:
      new_memset(0);
      class cell_t;

      zmpeg3_t *src;
      fs_t *fs;
      int64_t total_bytes; // Total bytes in title file.
      int64_t start_byte; // Absolute starting byte of the title in the stream
      int64_t end_byte; // Absolute ending byte of the title in the stream + 1
      // May get rid of time values and rename to a cell offset table.
      // May also get rid of end byte.

      class cell_t {
      public:
        new_memset(0);
        int64_t title_start; // Starting byte of cell in the title (start_byte)
        int64_t title_end;  // Ending byte of cell in the title (end_byte)
        int64_t program_start; // Starting byte of the cell in the program
        int64_t program_end; // Ending byte of the cell in the program
        double cell_time; // play time at end of cell, -1 if unknown
        int cell_no; // cell in original cell table
        int discontinuity;

        cell_t() {}
        ~cell_t() {}
      } *cell_table; // Timecode table
      int cell_table_size; // Number of entries
      int cell_table_allocation; // Number of available slots

      title_t(zmpeg3_t *zsrc, char *fpath);
      title_t(zmpeg3_t *zsrc);
      ~title_t();
      title_t(title_t &title);
      void new_cell(int cell_no,
        int64_t title_start, int64_t title_end,
        int64_t program_start, int64_t program_end,
        int discontinuity);
      void new_cell(int64_t byte_end) { // default cell
        new_cell(0, 0, byte_end, 0, byte_end, 0);
      }
      int dump_title();
      int print_cells(FILE *output);
    } *titles[MAX_STREAMZ];
    int total_titles;
    int current_title; // Title currently being used

    // Tables of every stream ID encountered
    int8_t astream_table[MAX_STREAMZ];  // afmt index
    int8_t vstream_table[MAX_STREAMZ];  // 1 if video
    int8_t sstream_table[MAX_STREAMZ];  // 1 if subtitle

    // Cell in the current title currently used
    int title_cell;
    int64_t nav_cell_next_vobu;
    int64_t nav_cell_end_byte;

    // Byte position in current program.
    int64_t program_byte;
    // Total bytes in all titles
    int64_t total_bytes;
    // The end of the current stream in the current program
    int64_t stream_end;

    int transport_error_indicator;
    int payload_unit_start_indicator;
    int pid; // PID of last packet
    int stream_id; // Stream ID of last packet
    int custom_id; // Custom ID of last packet
    int transport_scrambling_control;
    int adaptation_field_control;
    int continuity_counter;
    int is_padding;
    int pid_table[PIDMAX];
    int continuity_counters[PIDMAX];
    int total_pids;
    int adaptation_fields;
    double time;           // Time in seconds
    uint32_t last_code;
    int audio_pid;
    int video_pid;
    int subtitle_pid;
    int got_audio;
    int got_video;
    // if subtitle object was created in last packet
    int got_subtitle;
    // When only one stream is to be read, these store the stream IDs
    // Audio stream ID being decoded.  -1 = select first ID in stream
    int astream;     
    // Video stream ID being decoded.  -1 = select first ID in stream
    int vstream;
    // Multiplexed streams have the audio type
    // Format of the audio derived from multiplexing codes
    int aformat;      
    int program_association_tables;
    int table_id;
    int section_length;
    int transport_stream_id;
    int pes_packets;
    double pes_audio_time;  // Presentation Time stamps
    double pes_video_time;
    int pes_audio_pid;      // custom_id of timestamp
    int pes_video_pid;
    // Cause the stream parameters to be dumped in human readable format
    int dump;

    demuxer_t(zmpeg3_t *zsrc,atrack_t *do_aud,vtrack_t *do_vid,int cust_id);
    ~demuxer_t();

    int create_title(int full_scan=0);
    int64_t tell_byte() { return program_byte; }
    int64_t absolute_position() {
      return titles[current_title]->fs->tell() + titles[current_title]->start_byte;
    }
    bool eof();
    bool bof();
    bool error() { return error_flag; }
    int64_t movie_size();
    double get_time() { return time; }
    int get_cell(int no, title_t::cell_t *&v);
    void start_reverse();
    void start_forward();
    int seek_byte(int64_t byte);
    double scan_pts();
    int goto_pts(double pts);
    int seek_phys();
    int read_program();
    int64_t prog2abs_fwd(int64_t byte, int64_t *nbyte, int *ntitle, int *ncell);
    int64_t prog2abs_rev(int64_t byte, int64_t *nbyte, int *ntitle, int *ncell);
    int64_t program_to_absolute(int64_t byte, int64_t *nbyte=0, int *ntitle=0, int *ncell=0);
    int64_t absolute_to_program(int64_t byte);
    int64_t title_bytes();
    void append_data(uint8_t *data, int bytes);
    void shift_data();
    int open_title(int title_number);
    int copy_titles(demuxer_t *dst);
    void end_title(int64_t end_byte);
    int next_code(uint32_t zcode);
    int prev_code(uint32_t zcode);
    uint8_t read_char_packet();
    uint8_t read_prev_char_packet();
    uint8_t read_char() {
      if( zunlikely(zdata.eof()) ) return read_char_packet();
      return zdata.buffer[zdata.position++];
    }
    uint8_t read_prev_char();
    int read_data(uint8_t *output,int size);
    int read_next_packet();
    int read_prev_packet();
    void skip_video_frame();
    int64_t next_cell();
    int64_t playinfo_next_cell();
    double video_pts() { double pts = pes_video_time; pes_video_time = -1.; return pts; }
    double audio_pts() { double pts = pes_audio_time; pes_audio_time = -1.; return pts; }
    void reset_pts();
    void set_audio_pts(uint64_t pts, const double denom);
    void set_video_pts(uint64_t pts, const double denom);
    int current_cell_no();
  };

  class cacheframe_t {
  public:
    new_memset(0);
    uint8_t *y, *u, *v;
    int y_alloc, u_alloc, v_alloc;
    int y_size, u_size, v_size;
    int64_t frame_number;
    uint32_t age;

    cacheframe_t() {}
    ~cacheframe_t() {}
  };

  class cache_t {
    int extend_cache();
  public:
    new_memset(0);
    cacheframe_t *frames;
    int total;
    int allocation;
    uint32_t seq;

    cache_t() {}
    ~cache_t();
    void clear();
    void reset() { total = 0; }
    void put_frame(int64_t zframe_number,
      uint8_t *zy, uint8_t *zu, uint8_t *zv,
      int zy_size, int zu_size, int zv_size);
    int get_frame( int64_t frame_number,
        uint8_t **zy, uint8_t **zu, uint8_t **zv);
    int has_frame(int64_t frame_number);
    int64_t memory_usage();
  };

  fs_t *fs; // Store entry path here
  demuxer_t *demuxer; // Master title tables copied to all tracks
  int iopened; // file opened by user (before init)

// Media specific

  int total_atracks;
  class atrack_t {
    void extend_sample_offsets();
  public:
    new_memset(0);
    int channels;
    int sample_rate;
    demuxer_t *demuxer;
    audio_t *audio;
    int64_t current_position;
    int64_t total_samples;
    int format;               // format of audio
    int number, pid;
    long nudge;
    // If we got the header information yet.  Used in streaming mode.
    int got_header;
    // Pointer to master table of contents when the TOC is read.
    // Pointer to private table when the TOC is being created
    // Stores the absolute byte of each audio chunk
    int64_t *sample_offsets;
    int total_sample_offsets;
    int sample_offsets_allocated;
    // If this sample offset table must be deleted by the track
    int private_offsets;
    // End of stream in table of contents construction
    int64_t audio_eof;
    // Starting byte of previous/current packet for making TOC
    int64_t prev_offset, curr_offset;
    double audio_time;     // audio pts based time
    int askip;             // skipping
    int64_t pts_position;  // sample position at pts
    double pts_origin;
    double pts_starttime;  // demuxer first pts
    double pts_offset;     // pts offset due to discontinuity
    double frame_pts;
    void reset_pts();
    double last_pts() { return pts_starttime+audio_time-pts_offset; }

    atrack_t(zmpeg3_t *zsrc, int custom_id, int format, demuxer_t *demux, int no);
    ~atrack_t();

    int calculate_format(zmpeg3_t *src);
    int handle_audio_data(int track_number);
    int handle_audio(int track_number);
    int64_t track_position() { return current_position+nudge; }
    void append_samples(int64_t offset);
    void update_frame_pts() { if( frame_pts < 0 ) frame_pts = demuxer->audio_pts(); }
    void update_audio_time();
    double get_audio_time();
    double pts_audio_time(double pts) {
      return pts_starttime<0. ? pts_starttime : pts-pts_starttime+pts_offset;
    }
    int64_t apparent_position();
  } *atrack[MAX_STREAMZ];

  int total_vtracks;
  class vtrack_t {
    void extend_frame_offsets();
    void extend_keyframe_numbers();
  public:
    new_memset(0);
    int width, height;
    uint32_t tcode;
    double frame_rate;
    float aspect_ratio;
    demuxer_t *demuxer;
    video_t *video; // Video decoding object
    long current_position;  // Number of next frame to be played
    long total_frames;      // Total frames in the file
    int number, pid;
    slice_buffer_t *slice;  // toc slice data
    uint8_t *sbp;
    int sb_size;
    // Pointer to master table of contents when the TOC is read.
    // Pointer to private table when the TOC is being created
    // Stores the absolute byte of each frame
    int64_t *frame_offsets;
    int total_frame_offsets;
    int frame_offsets_allocated;
    int *keyframe_numbers;
    int total_keyframe_numbers;
    int keyframe_numbers_allocated;
    // Starting byte of previous/current packet for making TOC
    int64_t prev_frame_offset;
    int64_t prev_offset, curr_offset;
    int vskip;             // skipping
    double video_time;     // reshuffled pts
    // End of stream in table of contents construction
    int64_t video_eof;
    int got_top;
    int got_keyframe;
    cache_t *frame_cache;
    // If these tables must be deleted by the track
    int private_offsets;
    int pts_position;      // framenum at pts
    double pts_origin;
    double pts_starttime;  // first pts
    double pts_offset;     // pts offset due to discontinuity
    double refframe_pts;   // last refframe pts
    double frame_pts;
    double pts_video_time(double pts) {
      return pts_starttime<0. ? pts_starttime : pts-pts_starttime+pts_offset;
    }
    void reset_pts();
    double last_pts() { return pts_starttime+video_time-pts_offset; }

    vtrack_t(zmpeg3_t *zsrc, int custom_id, demuxer_t *demux, int no);
    ~vtrack_t();

    int handle_video_data(int track_number, int prev_data_size);
    int handle_video(int track_number);
    int find_keyframe_index(int64_t frame);
    void append_frame(int64_t offset, int is_keyframe);
    void update_frame_pts() { if( frame_pts < 0 ) frame_pts = demuxer->video_pts(); }
    void update_video_time();
    double get_video_time();
    int apparent_position();
  } *vtrack[MAX_STREAMZ];

  // Subtitle track
  // Stores the program offsets of subtitle images.
  // Only used for seeking off of table of contents for editing.
  // Doesn't have its own demuxer but hangs off the video demuxer.
  int total_stracks;
  class strack_t {
    void extend_offsets();
    void extend_subtitles();
  public:
    new_memset(0);
    zrwlock_t rwlock;
    int id; // 0x2X
    video_t *video;   // if non-zero, decoder owns track
    int64_t *offsets; // Offsets in program of subtitle packets
    int total_offsets;
    int allocated_offsets;
    subtitle_t *subtitles[MAX_SUBTITLES]; // Last subtitle objects found in stream.
    int total_subtitles;

    strack_t(int zid, video_t *vid=0);
    strack_t(strack_t &strack);
    ~strack_t();

    int append_subtitle(subtitle_t *subtitle, int lock=1);
    void append_subtitle_offset(int64_t program_offset);
    void del_subtitle(subtitle_t *subtitle, int lock=0);
    void del_subtitle(int i, int lock=0);
    void del_all_subtitles();
  } *strack[MAX_STREAMZ];

  static uint64_t get8bytes(uint8_t *buf) {
    return bswap_64(*((uint64_t *)buf));
  }
  static uint32_t get4bytes(uint8_t *buf) {
    return bswap_32(*((uint32_t *)buf));
  }
  static uint32_t get2bytes(uint8_t *buf) {
    return bswap_16(*((uint16_t *)buf));
  }

  class icell_t {
  public:
    new_memset(0);
    int64_t start_byte, end_byte;  // Bytes relative to start of stream.
    short vob_id, cell_id;         // cell address vob/cell id
    uint32_t inlv;                 // bit vector of allowed interleaves
    short angle, discon;           // angle interleave no, discontinuity flag

    int has_inlv(int i) { return (inlv >> i) & 1; }
  };

  class icell_table_t {
  public:
    icell_t *cells;
    long total_cells;
    long cells_allocated;
    icell_table_t() { cells = 0;  total_cells = cells_allocated = 0; }
    ~icell_table_t();
    icell_t *append_cell();
  };

  class ifo_t {
  public:
    new_memset(0);
    enum ifo_id_type {
      id_NUM_MENU_VOBS          = 0,
      id_NUM_TITLE_VOBS         = 1,
      id_MAT                    = 0,
      id_PTT                    = 1,
      id_TSP                    = 1,
      id_TITLE_PGCI             = 2,
      id_MENU_PGCI              = 3,
      id_TMT                    = 4,
      id_MENU_CELL_ADDR         = 5,
      id_MENU_VOBU_ADDR_MAP     = 6,
      id_TITLE_CELL_ADDR        = 7,
      id_TITLE_VOBU_ADDR_MAP    = 8,
    };

    uint32_t num_menu_vobs;
    uint32_t vob_start;
    uint32_t empirical;               // scan for audio/subtitle stream data
    uint8_t *data[10];
    int fd;                           // file descriptor
    int64_t pos;                      // offset of ifo file on device 
    int current_vob_id;
    int max_inlv;
    int64_t current_byte;
#include "ifo.h"

    int type_vts() {
      return !strncmp((char*)data[id_MAT], "DVDVIDEO-VTS", 12) ? 0 : -1;
    }
    int type_vmg() {
      return !strncmp((char*)data[id_MAT], "DVDVIDEO-VMG", 12) ? 0 : -1;
    }

    void get_playlist(zmpeg3_t *zsrc);
    int ifo_read(long pos, long count, uint8_t *data);
    int read_mat();

    ifo_t(int zfd, long zpos);
    ~ifo_t();
    int ifo_close();
    int init_tables();
    int get_table(int64_t offset,unsigned long tbl_id);
    void get_palette(zmpeg3_t *zsrc);
    void get_header(demuxer_t *demux);
    void get_playinfo(zmpeg3_t *zsrc, icell_table_t *icell_addrs);
    void icell_addresses(icell_table_t *cell_addrs);
    int64_t ifo_chapter_cell_time(zmpeg3_t *zsrc, int chapter);
    void get_ititle(zmpeg3_t *zsrc, int chapter=0);
    void icell_map(zmpeg3_t *zsrc, icell_table_t *icell_addrs);
    int chk(int vts_title, int chapter, int inlv, int angle,
      icell_table_t *icell_addrs, int &sectors, int &pcells, int &max_angle);
  };

  // Table of contents storage
  int64_t **frame_offsets;
  int64_t **sample_offsets;
  int **keyframe_numbers;
  int64_t *video_eof;
  int64_t *audio_eof;
  int *total_frame_offsets;
  int *total_sample_offsets;
  int64_t *total_samples;
  int *total_keyframe_numbers;
  // Handles changes in channel count after the start of a stream
  int *channel_counts, *nudging;
  // Indexes for audio tracks
  index_t **indexes;
  int total_indexes;
  double cell_time; // time at start of last cell
  // toc/skim thumbnail data callback
  zthumbnail_cb thumbnail_fn;
  void *thumbnail_priv;

  // Number of bytes to devote to the index of a single track
  // in the index building process.
  int64_t index_bytes;
  int file_type, log_errs;
  int err_logging(int v=-1) { int lv = log_errs; if( v>=0 ) log_errs=v; return lv; }
  // Only one of these is set to 1 to specify what kind of stream we have.
  int is_transport_stream() { return file_type & FT_TRANSPORT_STREAM; }
  int is_program_stream() { return file_type & FT_PROGRAM_STREAM; }
  int is_audio_stream() { return file_type & FT_AUDIO_STREAM; } // Elemental stream
  int is_video_stream() { return file_type & FT_VIDEO_STREAM; } // Elemental stream
  int is_ifo_file() { return file_type & FT_IFO_FILE; } // dvd ifo file
  // Special kind of transport stream for BD or AVC-HD
  int is_bd() { return 0; /* file_type & FT_BD_FILE; */ }
  int packet_size; // > 0 if known otherwise determine empirically for every packet
  // Type and stream for getting current absolute byte
  int last_type_read;  // 1 - audio   2 - video
  int last_stream_read;
  int vts_title;  // video titleset title number (counting from 0)
  int total_vts_titles;
  int interleave, angle; // active program sector interleave/angle index
  int total_interleaves;
  int cpus; // number of decode threads
  int seekable; // Filesystem is seekable.  Also means the file isn't a stream.
  int last_cell_no; // last known cell no in TOC build
  int pts_padding; // use pts data to add padding to sync damaged data
  FILE *toc_fp; // For building TOC, the output file.
  int recd_fd; // record file
  int64_t recd_pos; // record file size
  /* The first color palette in the IFO file.  Used for subtitles.
   * Byte order: YUVX * 16 */
  int have_palette;
  uint8_t palette[16 * 4];
  // Date of source file index was created from.
  // Used to compare DVD source file to table of contents source.
  int64_t source_date;
  // playback cell table
  icell_table_t *playinfo;

  int calculate_packet_size();
  int get_file_type(int *toc_atracks, int *toc_vtracks, const char *title_path);
  int read_toc(int *toc_atracks, int *toc_vtracks, const char *title_path);
  enum { show_toc_SAMPLE_OFFSETS=1, show_toc_AUDIO_INDEX=2, show_toc_FRAME_OFFSETS=4 };
  int show_toc(int flags);
  int handle_nudging();
  void divide_index(int track_number);
  int update_index(int track_number,int flush);
  static ifo_t *ifo_open(int fd,long pos);
  int read_ifo();
  atrack_t *new_atrack_t(int custom_id, int format,
    demuxer_t *demux, int number);
  vtrack_t *new_vtrack_t(int custom_id, demuxer_t *demux, int number);
  audio_t *new_audio_t(atrack_t *ztrack, int zformat);
  video_t *new_video_t(vtrack_t *ztrack);
  strack_t* get_strack_id(int id, video_t *vid);
  strack_t* get_strack(int number);
  strack_t* create_strack(int id, video_t *vid=0);
  int display_subtitle(int stream, int sid, int id,
    uint8_t *yp, uint8_t *up, uint8_t *vp, uint8_t *ap,
    int x, int y, int w, int h, double start_msecs, double stop_msecs);
  int delete_subtitle(int stream, int sid, int id);
  static int clip(int v, int mn, int mx) {
    if( v > mx ) return mx;
    if( v < mn ) return mn;
    return v;
  }
  static uint8_t clip(int32_t v) {
    return ((uint8_t)v) == v ? v : clip(v,0,255);
  }
  void handle_subtitle();
  void handle_cell(int this_cell_no);

#ifdef ZDVB
  class dvb_t {
    uint8_t *xbfr, *eob;
    uint32_t get8bits()  { return xbfr < eob ? *xbfr++ : 0; }
    uint32_t get16bits() { uint32_t v = get8bits();  return (v<<8) | get8bits(); }
    uint32_t get24bits() { uint32_t v = get16bits(); return (v<<8) | get8bits(); }
    uint32_t get32bits() { uint32_t v = get24bits(); return (v<<8) | get8bits(); }
  public:
    class mgt_t;  // master guide table
    class vct_t;  // virtual channel table
    class rrt_t;  // rating region table
    class eit_t;  // event information table
    class ett_t;  // extended text table
    class stt_t;  // system time table

    class mgt_t {
    public:
      new_memset(0);

      dvb_t *dvb;
      uint32_t ver;
      int items;
      class mitem_t {
      public:
        new_memset(0);
        uint8_t *bfr;
        int bfr_size, bfr_alloc;
        int src_id, tbl_id, tbl_len;
        uint16_t type;
        uint16_t pid;
        uint32_t version;
        uint32_t size;
        void init(int len);
        void clear();
        void extract(uint8_t *dat, int len);
        int bfr_len() { return  tbl_len - bfr_size; }
        mitem_t() {}
        mitem_t(int zpid) : pid(zpid) { bfr=0; bfr_size=bfr_alloc=0; }
        ~mitem_t() { clear(); }
      } *mitems;
      void clear();
      void extract();
      mitem_t *search(uint16_t pid);
      mgt_t(dvb_t *p) : dvb(p) {}
      ~mgt_t() { clear(); }
    } *mgt;

    class vct_t {
    public:
      new_memset(0);
      class vitem_t;

      dvb_t *dvb;
      uint32_t version;
      uint32_t transport_stream_id;
      int items, items_allocated;

      class vitem_t {
      public:
        new_memset(0);
        class ch_elts_t;

        uint16_t short_name[7+1];
        uint32_t major_channel_number;
        uint32_t minor_channel_number;
        uint32_t modulation_mode;
        uint32_t carrier_frequency;
        uint32_t channel_TSID;
        uint32_t program_number;
        uint32_t etm_location;
        uint8_t access_controlled;
        uint8_t hidden;
        uint8_t path_select;
        uint8_t out_of_band;
        uint32_t service_type;
        uint32_t source_id;
        uint32_t pcr_pid;
        int num_ch_elts;
        class ch_elts_t {
        public:
          uint32_t stream_id;
          uint32_t pes_pid;
          uint8_t  code_639[3];
          void extract(dvb_t *dvb);
        } elts[10];
        void extract(dvb_t *dvb);
      } **vct_tbl;

      vct_t(dvb_t *p) : dvb(p) {}
      ~vct_t() { clear(); }
      void extract();
      void clear();
      int search(vitem_t &new_item);
      void append(vitem_t &new_item);
    } *tvct, *cvct;

    class rrt_t {
    public:
      new_memset(0);
      class ritem_t;

      dvb_t *dvb;
      uint32_t version;
      uint8_t region_nlen;
      uint8_t region_name[2*32+1];
      int items;

      class ritem_t {
      public:
        new_memset(0);
        class rating_vt;

        uint8_t dim_nlen;
        uint8_t dim_name[2*20+1];
        uint8_t graduated_scale;
        uint8_t num_values;
        class rating_vt {
        public:
          uint8_t rating_nlen;
          uint8_t rating_name[2*8+1];
          uint8_t rating_tlen;
          uint8_t rating_text[2*150+1];
          void extract(dvb_t *dvb);
        } *abrevs;
        void extract(dvb_t *dvb);
        void clear();
        ritem_t() {}
        ~ritem_t() { clear(); }
      } *ratings;

      rrt_t(dvb_t *p) : dvb(p) {}
      ~rrt_t() { clear(); }
      void extract();
      void clear();
    } *rrt;

    class eit_t {
    public:
      new_memset(0);
      class einfo_t;

      dvb_t *dvb;
      eit_t *next;
      int id;
      uint32_t version;
      uint32_t source_id;
      int items, nitems;

      class einfo_t {
      public:
        new_memset(0);
        uint16_t event_id;
        uint16_t location;
        uint32_t start_time;
        uint32_t seconds;
        uint8_t title_tlen;
        uint8_t *title_text;
        void extract(dvb_t *dvb);
        void clear();
        einfo_t() {}
        ~einfo_t() { clear(); }
      } *infos;

      eit_t(dvb_t *p, int id) : dvb(p), id(id) {}
      ~eit_t() { clear(); }
      void extract();
      int search(uint16_t evt_id);
      void clear();
    } *eit;

    class ett_t {
    public:
      new_memset(0);
      class etext_t;

      dvb_t *dvb;
      ett_t *next;
      int id;

      class etext_t {
      public:
        new_memset(0);

        etext_t *next;
        int id;
        uint32_t version;
        uint16_t table_id;
        uint32_t etm_id;
        uint16_t msg_tlen;
        uint8_t *msg_text;
        void extract(dvb_t *dvb, uint32_t eid);
        void clear();
        etext_t(int id) : id(id) {}
        ~etext_t() { clear(); }
      } *texts;

      ett_t(dvb_t *p, int id) : dvb(p), id(id) {}
      ~ett_t() { clear(); }
      void extract();
      void clear();
    } *ett;

    class stt_t {
      dvb_t *dvb;
    public:
      new_memset(0);

      uint32_t version;
      uint16_t table_id;
      uint32_t system_time;
      uint8_t utc_offset;
      uint16_t daylight_saving;

      stt_t(dvb_t *p) : dvb(p) {}
      ~stt_t() { clear(); }
      void extract();
      void clear();
    } *stt;

    uint32_t stt_start_time;
    int64_t stt_offset() {
      return (365*10+7)*24*3600L - (stt ? stt->utc_offset : 0);
    }

    int64_t get_system_time() {
      return stt && stt->system_time ? stt->system_time + stt_offset() : -1;
    }
    zmpeg3_t *src;
    int sect_len;
    int stream_id;
    uint32_t version;
    int cur_next;
    int sect_num;
    int proto_ver;
    int empirical;

    int bfr_pos()  { return xbfr - active->bfr; }
    int bfr_len()  { return active->bfr_size - bfr_pos(); }
    void skip_bfr(int n);
    void skp_bfr(int n) { if( n > 0 ) skip_bfr(n); }

    int text_length, text_allocated;
    char *text;
    int get_text(uint8_t *dat, int bytes);
    void append_text(char *dat, int bytes);
    char *mstring(uint8_t *bp, int len);

    dvb_t(zmpeg3_t *zsrc);
    ~dvb_t();
    void reset();
    void extract();

    mgt_t::mitem_t *active;
    mgt_t::mitem_t *atsc_pid(int pid);
    mgt_t::mitem_t base_pid;

    int atsc_tables(demuxer_t *demux, int pid);
    void skip_descr(int descr_len);

    int channel_count();
    int signal_time();
    int get_channel(int n, int &major, int &minor);
    int get_station_id(int n, char *name);
    int total_astreams(int n, int &count);
    int astream_number(int n, int ord, int &stream, char *enc=0);
    int total_vstreams(int n, int &count);
    int vstream_number(int n, int ord, int &stream);
    int read_dvb(demuxer_t *demux);
    int get_chan_info(int n, int ord, int i, char *txt, int len);
  } dvb;

#endif

  // Interface

  // Get version information
  static int zmajor()   { return ZMPEG3_MAJOR; };
  static int zminor()   { return ZMPEG3_MINOR; };
  static int zrelease() { return ZMPEG3_RELEASE; };

  // Check for file compatibility.  Return 1 if compatible.
  static int check_sig(char *path);

  // Open the MPEG stream.
  // An error code is put into *ret if it fails and error_return is nonzero.
  zmpeg3_t(const char *path);
  zmpeg3_t(const char *path, int &ret, int access=fs_t::io_UNBUFFERED, const char *title_path=0);
  zmpeg3_t(int fd, int &ret, int access=0);
  zmpeg3_t(FILE *fp, int &ret, int access=0);
  int init(const char *title_path=0);
  ~zmpeg3_t();

  // Performance
  int set_cpus(int cpus);
  int set_pts_padding(int v);

  // Query the MPEG3 stream about audio.
  int has_audio();
  int total_astreams();
  int audio_channels(int stream);
  double sample_rate(int stream);
  const char *audio_format(int stream);

  // Total length obtained from the timecode.
  // For DVD files, this is unreliable.
  long audio_nudge(int stream); 
  long audio_samples(int stream); 
  int set_sample(long sample, int stream);
  long get_sample(int stream);

  // Stream defines the number of the multiplexed stream to read.
  // If output argument is null the audio is not rendered.
  // Read a PCM buffer of audio from 1 channel and advance the position.
  int read_audio(void *output_v, int type, int channel, long samples, int stream);
  int read_audio(double *output_d, int channel, long samples, int stream);
  int read_audio(float *output_f, int channel, long samples, int stream);
  int read_audio(int *output_i, int channel, long samples, int stream);
  int read_audio(short *output_s, int channel, long samples, int stream);
  // Read a PCM buffer of audio from 1 channel without advancing the position.
  int reread_audio(void *output_v, int type, int channel, long samples, int stream);
  int reread_audio(double *output_d, int channel, long samples, int stream);
  int reread_audio(float *output_f, int channel, long samples, int stream);
  int reread_audio(int *output_i, int channel, long samples, int stream);
  int reread_audio(short *output_s, int channel, long samples, int stream);

  // Read the next compressed audio chunk.  Store the size in size and return a 
  // Stream defines the number of the multiplexed stream to read.
  int read_audio_chunk(uint8_t *output, long *size, long max_size, int stream);

  // Query the stream about video.
  int has_video();
  int total_vstreams();
  int video_width(int stream);
  int video_height(int stream);
  int coded_width(int stream);
  int coded_height(int stream);
  int video_pid(int stream);
  float aspect_ratio(int stream); // aspect ratio.  0 if none
  double frame_rate(int stream);  // Frames/sec

  long video_frames(int stream);  // total length - for TOC files only.
  int set_frame(long frame, int stream); // Seek to a frame
  // int skip_frames(); /*???*/
  long get_frame(int stream);            // Tell current position

  // Total bytes.  Used for absolute byte seeking.
  int64_t get_bytes();

  // Seek all the tracks to the absolute byte in the 
  // file.  This eliminates the need for tocs but doesn't 
  // give frame accuracy.
  int seek_byte(int64_t byte);
  int64_t tell_byte();

  int previous_frame(int stream);
  int end_of_audio(int stream);
  int end_of_video(int stream);

  // Give the seconds time in the last packet read
  double get_time();
  double get_audio_time(int stream);
  double get_video_time(int stream);
  int get_cell_time(int no, double &time);

  // Read input frame and scale to the output frame
  // The last row of **output_rows must contain 4 extra bytes for scratch work.
  int read_frame(uint8_t **output_rows, // row start pointers
    int in_x, int in_y, int in_w, int in_h, // Location in input frame
    int out_w, int out_h, // Dimensions of output_rows
    int color_model, int stream);

  // Get the colormodel being used natively by the stream
  int colormodel(int stream);
  // Set the row stride to be used in read_yuvframe
  int set_rowspan(int bytes, int stream);

  // Read a frame in the native color model used by the stream. 
  // The Y, U, and V planes are copied into the y, u, and v
  // BUFFERS Provided.
  // The input is cropped to the dimensions given but not scaled.
  int read_yuvframe(char *y_output, char *u_output, char *v_output,
    int in_x, int in_y, int in_w, int in_h, int stream);

  // Read a frame in the native color model used by the stream. 
  // The Y, U, and V planes are not copied but the _output pointers
  // are redirected to the frame buffer.
  int read_yuvframe_ptr(char **y_output, char **u_output, char **v_output,
    int stream);

  // Drop frames number of frames
  int drop_frames(long frames, int stream);

  // Read the next compressed frame including headers.
  // Store the size in size and return a 1 if error.
  // Stream defines the number of the multiplexed stream to read.
  int read_video_chunk(uint8_t *output, long *size, long max_size,
    int stream);

  // Master control
  int get_total_vts_titles();
  int set_vts_title(int title=-1);
  int get_total_interleaves();
  int set_interleave(int inlv=-1);
  int set_angle(int a=-1);
  int set_program(int no); // legacy

  // Memory used by video caches.
  int64_t memory_usage();
  // write incoming datastream on file descr fd
  //   buffer access must be io_THREADED.  useful to record dvb device data
  //   bsz records up to bsz bytes of past buffer data in write_align
  int start_record(int fd, int bsz=0);
  int stop_record();
  void write_record(uint8_t *data, int len);
  // limit position for start of last full packet
  int64_t record_position() { return recd_pos - packet_size + 1; }
  // restart streams, discard device buffer and reinit file positions
  void restart();
  // enable/disable data xfr from reader thread
  int pause_reader(int v);

  // subtitle functions
  // get number of subtitle tracks
  int subtitle_tracks() { return total_stracks; }
  // Enable overlay of a subtitle track.
  // track - the number of the subtitle track starting from 0
  // The same subtitle track is overlayed for all video tracks.
  // track=-1 to disable subtitles.
  int show_subtitle(int stream, int strk);

  // Table of contents generation
  // Begin constructing table of contents
  static zmpeg3_t *start_toc(const char *path, const char *toc_path,
    int program=0, int64_t *total_bytes=0);
  // Set the maximum number of bytes per index track
  void set_index_bytes(int64_t bytes) { index_bytes = bytes; }
  // Process one packet
  int do_toc(int64_t *bytes_processed);
  // Write table of contents
  void stop_toc();
  int set_thumbnail_callback(int trk, int skim, int thumb, zthumbnail_cb fn, void *p);
  int get_thumbnail(int trk, int64_t &frn, uint8_t *&t, int &w, int &h);
  int set_cc_text_callback(int trk, zcc_text_cb fn);

  // Get modification date of source file from table of contents.
  // Used to compare DVD source file to table of contents source.
  int64_t get_source_date() { return source_date; }
  // Get modification date of source file from source file.
  static int64_t calculate_source_date(char *path);
  static int64_t calculate_source_date(int fd);
  // Table of contents queries
  int index_tracks();
  int index_channels(int track);
  int index_zoom();
  int index_size(int track);
  float* index_data(int track, int channel);
  // Returns 1 if the file has a table of contents
  int has_toc() {
    return frame_offsets || sample_offsets ? 1 : 0;
  }
  // Return the path of the title number or 0 if no more titles.
  char* title_path(int number) {
    return number>=0 && number<demuxer->total_titles ?
      demuxer->titles[number]->fs->path : 0;
  }

  static inline void complete_path(char *full_path, char *path) {
    char dir[STRLEN];
    if( path[0] != '/' && getcwd(dir, sizeof(dir)) )
      snprintf(full_path, STRLEN, "%s/%s", dir, path);
    else
      strcpy(full_path, path);
  }

  static inline void get_directory(char *directory, char *path) {
    char *ptr = strrchr(path, '/');
    int i = 0;
    if( ptr ) {
      int n = ptr-path;
      while( i < n ) {
        directory[i] = path[i];
        ++i;
      }
    }
    if( i == 0 ) directory[i++] = '.';
    directory[i] = 0;
  }

  static inline void get_filename(char *filename, char *path) {
    char *ptr = strrchr(path, '/');
    if( ptr ) ++ptr;
    else ptr = path;
    strcpy(filename, ptr);
  }

  static inline void joinpath(char *path, char *dir, char *filename) {
    snprintf(path, STRLEN, "%s/%s", dir, filename);
  }

  static inline int64_t path_total_bytes(char *path) {
    struct stat64 st;
    return stat64(path, &st )<0 ? 0 : st.st_size;
  }

};

typedef zmpeg3_t::zlock_t                       zzlock_t;
typedef zmpeg3_t::zblock_t                      zzblock_t;
typedef zmpeg3_t::zrwlock_t                     zzrwlock_t;
typedef zmpeg3_t::fs_t                          zfs_t;
typedef   zfs_t::css_t                          zcss_t;
typedef     zcss_t::key_t                       zkey_t;
typedef     zcss_t::blk_t                       zblk_t;
typedef     zcss_t::playkey_t                   zplaykey_t;
typedef   zfs_t::buffer_t                       zbuffer_t;
typedef zmpeg3_t::bits_t                        zbits_t;
typedef zmpeg3_t::index_t                       zindex_t;
typedef zmpeg3_t::bitfont_t                     zbitfont_t;
typedef   zbitfont_t::bitchar_t                 zbitchar_t;
typedef   zbitfont_t::static_init_t             zstatic_init_t;
typedef zmpeg3_t::timecode_t                    ztimecode_t;
typedef zmpeg3_t::audio_t                       zaudio_t;
typedef   zaudio_t::imdct_complex_t             zimdct_complex_t;
typedef   zaudio_t::imdct_al_table_t            zimdct_al_table_t;
typedef   zaudio_t::audio_decoder_layer_t       zaudio_decoder_layer_t;
typedef   zaudio_t::audio_decoder_ac3_t         zaudio_decoder_ac3_t;
typedef   zaudio_t::audio_decoder_pcm_t         zaudio_decoder_pcm_t;
typedef zmpeg3_t::VLCtab_t                      zVLCtab_t;
typedef zmpeg3_t::DCTtab_t                      zDCTtab_t;
typedef zmpeg3_t::slice_buffer_t                zslice_buffer_t;
typedef zmpeg3_t::video_t                       zvideo_t;
typedef   zvideo_t::cc_t                        zcc_t;
typedef     zcc_t::svc_t                        zsvc_t;
typedef       zsvc_t::win_t                     zwin_t;
typedef       zsvc_t::chr_t                     zchr_t;
typedef zmpeg3_t::slice_decoder_t               zslice_decoder_t;
typedef zmpeg3_t::subtitle_t                    zsubtitle_t;
typedef zmpeg3_t::demuxer_t                     zdemuxer_t;
typedef   zdemuxer_t::zstream_t                 zzstream_t;
typedef   zdemuxer_t::title_t                   ztitle_t;
typedef     ztitle_t::cell_t                    zcell_t;
typedef zmpeg3_t::cacheframe_t                  zcacheframe_t;
typedef zmpeg3_t::cache_t                       zcache_t;
typedef zmpeg3_t::atrack_t                      zatrack_t;
typedef zmpeg3_t::vtrack_t                      zvtrack_t;
typedef zmpeg3_t::strack_t                      zstrack_t;
typedef zmpeg3_t::icell_t                       zicell_t;
typedef zmpeg3_t::icell_table_t                 zicell_table_t;
typedef zmpeg3_t::ifo_t                         zifo_t;
#ifdef ZDVB
typedef zmpeg3_t::dvb_t                         zdvb_t;
typedef   zdvb_t::mgt_t                         zmgt_t;
typedef     zmgt_t::mitem_t                     zmitem_t;
typedef   zdvb_t::vct_t                         zvct_t;
typedef     zvct_t::vitem_t                     zvitem_t;
typedef       zvitem_t::ch_elts_t               zch_elts_t;
typedef   zdvb_t::rrt_t                         zrrt_t;
typedef     zrrt_t::ritem_t                     zritem_t;
typedef       zritem_t::rating_vt               zrating_vt;
typedef   zdvb_t::eit_t                         zeit_t;
typedef     zeit_t::einfo_t                     zeinfo_t;
typedef   zdvb_t::ett_t                         zett_t;
typedef     zett_t::etext_t                     zetext_t;
typedef   zdvb_t::stt_t                         zstt_t;
#endif

/* legacy */
extern "C" {
#else
typedef struct {} zmpeg3_t;
#endif /*__cplusplus */
typedef zmpeg3_t mpeg3_t;

int mpeg3_major(void);
int mpeg3_minor(void);
int mpeg3_release(void);
int mpeg3_check_sig(char *path);
int mpeg3_is_program_stream(zmpeg3_t * zsrc);
int mpeg3_is_transport_stream(zmpeg3_t * zsrc);
int mpeg3_is_video_stream(zmpeg3_t * zsrc);
int mpeg3_is_audio_stream(zmpeg3_t * zsrc);
int mpeg3_is_ifo_file(zmpeg3_t * zsrc);
int mpeg3_create_title(zmpeg3_t * zsrc,int full_scan);
zmpeg3_t *mpeg3_open(const char *path,int *error_return);
zmpeg3_t *mpeg3_open_title(const char *title_path,const char *path,int *error_return);
zmpeg3_t *mpeg3_zopen(const char *title_path,const char *path,int *error_return, int access);
zmpeg3_t *mpeg3_open_copy(const char *path,zmpeg3_t *old_src,int *error_return);
int mpeg3_close(zmpeg3_t *zsrc);
int mpeg3_set_pts_padding(zmpeg3_t *zsrc, int v);
int mpeg3_set_cpus(zmpeg3_t *zsrc,int cpus);
int mpeg3_has_audio(zmpeg3_t *zsrc);
int mpeg3_total_astreams(zmpeg3_t *zsrc);
int mpeg3_audio_channels(zmpeg3_t *zsrc,int stream);
int mpeg3_sample_rate(zmpeg3_t *zsrc,int stream);
const char *mpeg3_audio_format(zmpeg3_t *zsrc,int stream);
long mpeg3_get_audio_nudge(zmpeg3_t *zsrc,int stream);
long mpeg3_audio_samples(zmpeg3_t *zsrc,int stream);
int mpeg3_set_sample(zmpeg3_t *zsrc,long sample,int stream);
long mpeg3_get_sample(zmpeg3_t *zsrc,int stream);
int mpeg3_read_audio(zmpeg3_t *zsrc,float *output_f,short *output_i,int channel,
   long samples,int stream);
int mpeg3_read_audio_d(zmpeg3_t *zsrc,double *output_d, int channel,
   long samples, int stream);
int mpeg3_read_audio_f(zmpeg3_t *zsrc,float *output_f, int channel,
   long samples, int stream);
int mpeg3_read_audio_i(zmpeg3_t *zsrc,int *output_i, int channel,
   long samples, int stream);
int mpeg3_read_audio_s(zmpeg3_t *zsrc,short *output_s, int channel,
   long samples, int stream);
int mpeg3_reread_audio_d(zmpeg3_t *zsrc,double *output_d, int channel,
   long samples, int stream);
int mpeg3_reread_audio_f(zmpeg3_t *zsrc,float *output_f, int channel,
   long samples, int stream);
int mpeg3_reread_audio_i(zmpeg3_t *zsrc,int *output_i, int channel,
   long samples, int stream);
int mpeg3_reread_audio_s(zmpeg3_t *zsrc,short *output_s, int channel,
   long samples, int stream);
int mpeg3_reread_audio(zmpeg3_t *zsrc,float *output_f,short *output_i,
   int channel,long samples,int stream);
int mpeg3_read_audio_chunk(zmpeg3_t *zsrc,unsigned char *output,long *size,
   long max_size,int stream);
int mpeg3_has_video(zmpeg3_t *zsrc);
int mpeg3_total_vstreams(zmpeg3_t *zsrc);
int mpeg3_video_width(zmpeg3_t *zsrc,int stream);
int mpeg3_video_height(zmpeg3_t *zsrc,int stream);
int mpeg3_coded_width(zmpeg3_t *zsrc,int stream);
int mpeg3_coded_height(zmpeg3_t *zsrc,int stream);
int mpeg3_video_pid(zmpeg3_t *zsrc,int stream);
float mpeg3_aspect_ratio(zmpeg3_t *zsrc,int stream);
double mpeg3_frame_rate(zmpeg3_t *zsrc,int stream);
long mpeg3_video_frames(zmpeg3_t *zsrc,int stream);
int mpeg3_set_frame(zmpeg3_t *zsrc,long frame,int stream);
//int mpeg3_skip_frames(zmpeg3_t *zsrc);
long mpeg3_get_frame(zmpeg3_t *zsrc,int stream);
int64_t mpeg3_get_bytes(zmpeg3_t *zsrc);
int mpeg3_seek_byte(zmpeg3_t *zsrc,int64_t byte);
int64_t mpeg3_tell_byte(zmpeg3_t *zsrc);
int mpeg3_previous_frame(zmpeg3_t *zsrc,int stream);
int mpeg3_end_of_audio(zmpeg3_t *zsrc,int stream);
int mpeg3_end_of_video(zmpeg3_t *zsrc,int stream);
double mpeg3_get_time(zmpeg3_t *zsrc);
double mpeg3_get_audio_time(zmpeg3_t *zsrc, int stream);
double mpeg3_get_video_time(zmpeg3_t *zsrc, int stream);
double mpeg3_get_cell_time(zmpeg3_t *zsrc, int no, double *time);
int mpeg3_read_frame(zmpeg3_t *zsrc,unsigned char **output_rows,int in_x,
   int in_y,int in_w,int in_h,int out_w,int out_h,int color_model,int stream);
int mpeg3_colormodel(zmpeg3_t *zsrc,int stream);
int mpeg3_set_rowspan(zmpeg3_t *zsrc,int bytes,int stream);
int mpeg3_read_yuvframe(zmpeg3_t *zsrc,char *y_output,char *u_output,
   char *v_output,int in_x,int in_y,int in_w,int in_h,int stream);
int mpeg3_read_yuvframe_ptr(zmpeg3_t *zsrc,char **y_output,char **u_output,
   char **v_output,int stream);
int mpeg3_drop_frames(zmpeg3_t *zsrc,long frames,int stream);
int mpeg3_read_video_chunk(zmpeg3_t *zsrc,unsigned char *output,long *size,
   long max_size,int stream);
int mpeg3_get_total_vts_titles(zmpeg3_t *zsrc);
int mpeg3_set_vts_title(zmpeg3_t *zsrc,int title); // toc build only
int mpeg3_get_total_interleaves(zmpeg3_t *zsrc);
int mpeg3_set_interleave(zmpeg3_t *zsrc,int inlv); // toc build only
int mpeg3_set_angle(zmpeg3_t *zsrc,int a); // toc build only
int mpeg3_set_program(zmpeg3_t *zsrc,int no); // toc build only
int64_t mpeg3_memory_usage(zmpeg3_t *zsrc);
int mpeg3_get_thumbnail(zmpeg3_t *zsrc, int trk,
   int64_t *frn, uint8_t **t, int *w, int *h);
int mpeg3_set_thumbnail_callback(zmpeg3_t *zsrc, int trk,
   int skim, int thumb, zthumbnail_cb fn, void *p);
int mpeg3_set_cc_text_callback(zmpeg3_t *zsrc, int trk, zcc_text_cb fn);
int mpeg3_subtitle_tracks(zmpeg3_t *zsrc);
int mpeg3_show_subtitle(zmpeg3_t *zsrc,int vtrk, int strk);
int mpeg3_display_subtitle(zmpeg3_t *zsrc,int stream,int sid,int id,
   uint8_t *yp,uint8_t *up,uint8_t *vp,uint8_t *ap,
   int x,int y,int w,int h,double start_msecs,double stop_msecs);
int mpeg3_delete_subtitle(zmpeg3_t *zsrc,int stream,int sid,int id);
zmpeg3_t *mpeg3_start_toc(char *path,char *toc_path,int program,int64_t *total_bytes);
void mpeg3_set_index_bytes(zmpeg3_t *zsrc,int64_t bytes);
int mpeg3_do_toc(zmpeg3_t *zsrc,int64_t *bytes_processed);
void mpeg3_stop_toc(zmpeg3_t *zsrc);
int64_t mpeg3_get_source_date(zmpeg3_t *zsrc);
int64_t mpeg3_calculate_source_date(char *path);
int mpeg3_index_tracks(zmpeg3_t *zsrc);
int mpeg3_index_channels(zmpeg3_t *zsrc,int track);
int mpeg3_index_zoom(zmpeg3_t *zsrc);
int mpeg3_index_size(zmpeg3_t *zsrc,int track);
float *mpeg3_index_data(zmpeg3_t *zsrc,int track,int channel);
int mpeg3_has_toc(zmpeg3_t *zsrc);
char *mpeg3_title_path(zmpeg3_t *zsrc,int number);
// hooks needed for quicktime
typedef struct {} mpeg3_layer_t;
int mpeg3audio_dolayer3(mpeg3_layer_t *audio,
   char *frame, int frame_size, float **output, int render);
int mpeg3_layer_header(mpeg3_layer_t *layer_data, unsigned char *data);
void mpeg3_layer_reset(mpeg3_layer_t *zlayer_data);
mpeg3_layer_t* mpeg3_new_layer();
void mpeg3_delete_layer(mpeg3_layer_t *audio);
// hooks needed for mplexlo
void mpeg3_skip_video_frame(mpeg3_t *zsrc, int stream);
int64_t mpeg3_video_tell_byte(mpeg3_t *zsrc, int stream);
int64_t mpeg3_audio_tell_byte(mpeg3_t *zsrc, int stream);
#ifdef ZDVB
int mpeg3_dvb_channel_count(zmpeg3_t *zsrc);
int mpeg3_dvb_get_channel(mpeg3_t *zsrc,int n, int *major, int *minor);
int mpeg3_dvb_get_station_id(mpeg3_t *zsrc,int n, char *name);
int mpeg3_dvb_total_astreams(mpeg3_t *zsrc,int n, int *count);
int mpeg3_dvb_astream_number(mpeg3_t *zsrc,int n, int ord, int *stream, char *enc);
int mpeg3_dvb_total_vstreams(mpeg3_t *zsrc,int n, int *count);
int mpeg3_dvb_vstream_number(mpeg3_t *zsrc,int n, int ord, int *stream);
int mpeg3_dvb_get_chan_info(mpeg3_t *zsrc,int n, int ord, int i, char *cp, int len);
int mpeg3_dvb_get_system_time(mpeg3_t *zsrc, int64_t *tm);
#endif
#ifdef __cplusplus
}
#endif
#endif
#endif
