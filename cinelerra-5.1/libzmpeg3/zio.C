#include "libzmpeg3.h"

#ifdef USE_FUTEX
typedef zmpeg3_t::zloc_t zzloc_t;

int zzloc_t::
zyield()
{
  return syscall(SYS_sched_yield);
}

int zzloc_t::
zgettid()
{
  return syscall(SYS_gettid);
}

int zzloc_t::
zwake(int nwakeups)
{
  int ret;
  while( (ret=zfutex(FUTEX_WAKE, nwakeups)) < 0 );
  return ret;
}

int zzloc_t::
zwait(int val)
{
  return zfutex(FUTEX_WAIT, val);
}

int zzlock_t::
zemsg1()
{
  fprintf(stderr,"unlocking and not locked\n");
  return -1;
}

int zzlock_t::
zlock(int v)
{
  if( v || zxchg(1,loc) >= 0 ) do {
    zwait(1);
  } while ( zxchg(1,loc) >= 0 );
  return 1;
}

int zzlock_t::
zunlock(int nwakeups)
{
  loc = -1;
  return zwake(1);
}

void zzrwlock_t::
zenter()
{
  zdecr(loc); lk.lock();
  zincr(loc); lk.unlock();
}

void zzrwlock_t::
zleave()
{
  if( lk.loc >= 0 )
    zwake(1);
}

void zzrwlock_t::
zwrite_enter(int r)
{
  lk.lock();
  if( r < 0 ) zdecr(loc);
  int v;  while( (v=loc) >= 0 ) zwait(v);
}

void zzrwlock_t::
zwrite_leave(int r)
{
  if( r < 0 ) zincr(loc);
  lk.unlock();
}

#else

void zzrwlock_t::
enter()
{
  lock();
  while( blocking ) { unlock(); lk.lock(); lk.unlock(); lock(); }
  ++users;
  unlock();
}

void zzrwlock_t::
leave()
{
  lock();
  if( !--users && blocking ) wake();
  unlock();
}

void zzrwlock_t::
zwrite_enter(int r)
{
  lk.lock();
  blocking = pthread_self();
  lock();
  if( r < 0 ) --users;
  while( users ) { unlock(); wait(); lock(); }
  unlock();
}

void zzrwlock_t::
zwrite_leave(int r)
{
  if( r < 0 ) ++users;
  blocking = 0; lk.unlock();
}

#endif

zbuffer_t::
buffer_t(zmpeg3_t *zsrc, int access)
{
  src = zsrc;
  access_type = access;
  alloc = !(access_type & io_SEQUENTIAL) ? IO_SIZE : SEQ_IO_SIZE;
  reader_done = -1;
  writer_done = -1;
  ref_count = 1;
  fd = -1;
  reset();
  owner = pthread_self();
}

zbuffer_t::
~buffer_t()
{
  if( (access_type & io_THREADED) )
    stop_reader();
  if( data ) delete [] data;
}

void zbuffer_t::
unblock()
{
  io_lock.unlock();
  io_block.unblock();
  ::sched_yield();
  io_lock.lock();
}

void zbuffer_t::
block()
{
  io_block.block();
}

void zbuffer_t::
reader()
{
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,0);
  lock();
  while( !reader_done ) {
    if( do_restart ) {
      do_restart = 0;
      restart(0);
      restarted = 1;
    }
    if( size+MAX_IO_SIZE > alloc )
      size = alloc - MAX_IO_SIZE;
    unlock();
    int64_t count = read_in(zmpeg3_t::MAX_IO_SIZE);
    lock();
    if( count > 0 ) {
      file_pos += count;
      fin = in;
      if( (size+=count) > alloc )
        size = alloc;
      if( src->recd_fd >= 0 && the_writer )
        write_lock.unblock();
    }
    unblock();
  }
  unlock();
  reader_done = -1;
}

void *zbuffer_t::
reader(void *the_buffer)
{
  buffer_t *b = (buffer_t *)the_buffer;
  b->reader();
  return 0;
}

void zbuffer_t::
writer()
{
  const int mx_blksz = 0x100000;

  while( !writer_done ) {
    write_record(mx_blksz,0xfff);
    if( file_pos - write_pos < mx_blksz )
      write_lock.block();
    else
      sched_yield();
  }

  write_record(INT_MAX, 0);
  writer_done = -1;
}

void *zbuffer_t::
writer(void *the_buffer)
{
  buffer_t *b = (buffer_t *)the_buffer;
  b->writer();
  return 0;
}

int zbuffer_t::
open_file(char *path)
{
//zmsgs("1 %s\n", path);
  if( !open_count ) {
    if( (access_type & io_THREADED) )
      access_type |= io_SINGLE_ACCESS + io_SEQUENTIAL;
    if( !(access_type & io_UNBUFFERED) ) {
      if( fp == 0 ) {
        if( !(fp=::fopen(path, "rb")) ) {
          perrs("%s",path);
          return 1;
        }
      }
    }
    else {
      if( fd < 0 ) {
        int mode = (access_type & io_NONBLOCK) ? O_RDONLY+O_NONBLOCK : O_RDONLY;
        if( (fd=::open(path, mode)) < 0 ) {
          perrs("%s",path);
          return 1;
        }
      }
    }
    if( (access_type & io_THREADED) )
      start_reader();
    else
      restart();
  }
  ++open_count;
  return 0;
}

void zbuffer_t::
reset()
{
  in = fin = out = size = 0;
  file_pos = out_pos = file_nudge = 0;
}

void zbuffer_t::
restart(int lk)
{
  if( lk ) lock();
  reset();
  if( !(access_type & io_SEQUENTIAL) ) {
    if( !(access_type & io_UNBUFFERED) ) {
      if( ::fseek(fp,0,SEEK_SET) < 0 )
        perrs("fseek %jd", file_pos);
    }
    else {
      if( ::lseek(fd,0,SEEK_SET) < 0 )
        perrs("lseek %jd", file_pos);
    }
  }
  if( lk ) unlock();
}

void zbuffer_t::
start_reader()
{
  restart();
  reader_done = 0;
  pthread_create(&the_reader,0,reader,this);
}

void zbuffer_t::
stop_reader()
{
  if( reader_done || !the_reader ) return;
  reader_done = 1;
  io_block.unblock();
  int tmo = 10;
  while( reader_done >= 0 && --tmo >= 0 ) usleep(100000);
  if( tmo < 0 ) pthread_cancel(the_reader);
  the_reader = 0;
  stop_record();
}

int zbuffer_t::
start_record()
{
  writer_done = 0;
  pthread_create(&the_writer,0,writer,this);
  return 0;
}

int zbuffer_t::
stop_record()
{
  if( writer_done || !the_writer ) return 1;
  writer_done = 1;
  write_lock.unblock();
  pthread_join(the_writer,0);
  int tmo = 10;
  while( writer_done >= 0 && --tmo >= 0 ) usleep(100000);
  if( tmo < 0 ) pthread_cancel(the_writer);
  the_writer = 0;
  return 0;
}

int zbuffer_t::
write_align(int sz)
{
  int isz = 2*src->packet_size;
  if( sz > isz ) isz = sz;
  uint8_t *pat = 0;
  int psz = 0;
  if( src->is_program_stream() ) {
    static uint8_t pack_start[4] = { 0x00, 0x00, 0x01, 0xba };
    pat = pack_start;  psz = sizeof(pack_start);
  }
  else if( src->is_transport_stream() ) {
    static uint8_t sync_start[1] = { 0x47 };
    pat = sync_start;  psz = sizeof(sync_start);
  }
  if( isz > out_pos ) isz = out_pos;
  int64_t bsz = file_pos - out_pos;
  int i = bsz>alloc ? 0 : alloc-bsz;
  if( isz > i ) isz = i;
  i = out - isz;
  if( i < 0 ) i += alloc;
  uint8_t *bfr = &data[i];
  uint8_t *lmt = &data[alloc];
  int64_t len = isz + bsz;
  i = 0;
  if( pat ) {
    while( --len >= 0 ) {
      if( *bfr++ != pat[i] ) { i = 0; }
      else if( ++i >= psz ) break;
      if( bfr >= lmt ) bfr = &data[0];
    }
  }
  if( len < 0 ) return 1;
  len += psz;
  if( (bfr-=psz) < &data[0] ) bfr += alloc;
  wout = bfr - data;
  write_pos = file_pos - len;
  return 0;
}

// write blocks of (mask+1) bytes of data at data+wout, update wout
//   only write full blocks, fragments cause disk thrashing
void zbuffer_t::
write_record(int sz, int mask)
{
  int isz = file_pos - write_pos;
  if( isz > sz ) isz = sz;
  if( !(isz &= ~mask) ) return;
//zmsgs(" isz=%d, file_pos=%ld, write_pos=%ld\n", isz, file_pos, write_pos);
  write_pos += isz;
  int len = isz;
  int n = alloc - wout;
  if( n > len ) n = len;
  if( n > 0 ) {
    src->write_record(&data[wout],n);
    len -= n;
  }
  if( len > 0 )
    src->write_record(&data[0],wout=len);
  else
    wout += n;
}

void zbuffer_t::
close_file()
{
  if( open_count > 0 && --open_count == 0 ) {
    if( (access_type & io_THREADED) )
      stop_reader();
    if( !src || !src->iopened ) {
      if( !(access_type & io_UNBUFFERED) ) {
        if( fp != 0 ) { fclose(fp); fp = 0; }
      }
      else {
        if( fd >= 0 ) { close(fd); fd = -1; }
      }
    }
  }
}

/* sole user of in ptr (except restart) */
int64_t zbuffer_t::
read_in(int64_t len)
{
  int64_t count = 0;
  while( count < len ) {
    int xfr = len - count;
    int avl = alloc - in;
    if( avl < xfr ) xfr = avl;
    if( access_type & io_UNBUFFERED ) {
      if( access_type & io_NONBLOCK ) {
        avl = -1;
        for( int i=10; avl<0 && --i>=0; ) { // 10 retries, 2 seconds
          struct timeval tv;  tv.tv_sec = 0;  tv.tv_usec = 200000;
          fd_set rd_fd;  FD_ZERO(&rd_fd);  FD_SET(fd, &rd_fd);
          int ret = select(fd+1, &rd_fd, 0, 0, &tv);
          if( !ret ) continue;
          if( ret < 0 ) break;
          if( !FD_ISSET(fd, &rd_fd) ) continue;
          if( (ret=::read(fd, &data[in], xfr)) > 0 ) avl = ret;
        }
      }
      else
        avl = ::read(fd, &data[in], xfr);
    }
    else {
      avl = ::fread(&data[in], 1, xfr, fp);
      if( !avl && ferror(fp) ) avl = -1;
    }
    if( avl < 0 ) {
      if( errno == EOVERFLOW ) {
	zerr("Overflow\n");
	continue;
      }
      ++errs;
      if( !(access_type & io_ERRFAIL) ) {
        if( errs < IO_ERR_LIMIT && xfr > ERR_PACKET_SIZE )
          xfr = ERR_PACKET_SIZE;
        memset(&data[in],0,xfr);
        if( !(access_type & io_THREADED) ) {
          int64_t pos = file_pos + count + xfr;
          if( (access_type & io_UNBUFFERED) ) {
            if( ::lseek(fd,pos,SEEK_SET) < 0 )
              perrs("lseek pos %jx",pos);
          }
          else {
            if( ::fseek(fp,pos,SEEK_SET) < 0 )
              perrs("fseek pos %jx",pos);
          }
        }
        avl = xfr;
      }
      else {
        perrs("read pos %jx",file_pos + count);
        avl = 0;
      }
    }
    else
      errs = 0;
    if( avl == 0 ) break;
    if( paused ) continue;
    count += avl;
    in += avl;
    if( (avl=in-alloc) >= 0 ) in = avl;
  }
  return count;
}

int zbuffer_t::
wait_in(int64_t pos)
{
  int result = 0;
  if( (access_type & io_THREADED) && !restarted ) {
    while( !reader_done && !restarted && file_pos <= pos ) {
      io_lock.unlock();
      io_block.block();
      io_lock.lock();
    }
  }
  if( reader_done || restarted ) result = 1;
  restarted = 0;
  return result;
}

int zbuffer_t::
seek_in(int64_t pos)
{
  int result = 0;
  if( file_pos != pos ) {
    if( !(access_type & io_SEQUENTIAL) ) {
      result = (access_type & io_UNBUFFERED) ?
        (lseek64(fd, pos, SEEK_SET) >= 0 ? 0 : 1) :
        (fseeko(fp, pos, SEEK_SET) == 0 ? 0 : 1) ;
    }
    else if( pos != 0 ) {
      if( (access_type & io_THREADED) )
        src->restart();
      zerrs("seek on sequential from %jd to %jd\n", file_pos, pos);
      result = 1;
    }
    else {
      restart(0);
      result = 1;
    }
  }
  return result;
}

int zbuffer_t::
read_fin(int64_t len)
{
  int64_t count = read_in(len);
  /* must already be locked */
  fin = in;
  file_pos += count;
  if( (size+=count) > alloc )
    size = alloc;
  return len && count ? 0 : 1;
}

int zbuffer_t::
seek_to(int64_t pos, int64_t len)
{
//zmsgs(" pos=%ld, len=%ld\n", pos, len);
  int result = seek_in(pos);
  if( !result ) {
    file_pos = out_pos = pos;
    in = size = 0;
    result = read_fin(len);
  }
  return result;
}

int zbuffer_t::
read_to(int64_t pos)
{
//zmsgs(" pos=%ld\n", pos);
  int result = 0;
  int64_t len = pos - file_pos;
  if( len < 0 ) {
    zerrs("reversed seq read (%jd < %jd)\n", pos, file_pos);
    result = 1;
  }
  else
    result = read_fin(len);
  return result;
}

int zbuffer_t::
sync(int64_t pos)
{
  int result = 1;
  pos += file_nudge;
  int64_t start_pos = file_pos - size;
  if( pos < start_pos ) { /* before buffer */
    if( (access_type & io_THREADED) ) {
      if( pos ) {
        zerrs("threaded sync before buffer %jd < %jd\n", pos, start_pos);
        int64_t mid_pos = start_pos + size/2;
        file_nudge += mid_pos - pos;
        pos = mid_pos;
      }
      else {
        restart(0);  /* allow fake seek to start */
        result = -1;
      }
    }
    else
      result = -1;
  }
  else
    result = pos < file_pos ? 0 : /* in buffer */
      (access_type & io_THREADED) ?
        wait_in(pos) : -1; /* after buffer */
  if( result < 0 ) { /* out of buffer */
//zmsgs("out of buffer pos=%ld, start_pos=%ld, file_pos=%ld, out_pos=%ld\n",
//  pos, start_pos, file_pos, out_pos);
    int64_t seek_pos = pos - alloc/2;
    int64_t end_pos = seek_pos + alloc;
    if( seek_pos < 0 ) seek_pos = 0;
    result = (access_type & io_SEQUENTIAL) ||
        (seek_pos < file_pos && end_pos >= file_pos) ?
      read_to(end_pos) : seek_to(seek_pos, alloc);
  }
  if( !result ) {
    int64_t offset = file_pos - pos;
    if( offset >= 0 && offset <= size ) {
      if( (offset=fin-offset) < 0 ) offset += alloc;
      out = offset;  out_pos = pos;
    }
    else
      result = 1;
  }
  return result;
}

int zbuffer_t::
read_out(uint8_t *bfr,int len)
{
  int count = 0;
  while( count < len ) {
    int avail = file_pos - out_pos;
    if( avail <= 0 ) break;
    int fragment_size = alloc - out;
    if( fragment_size > avail ) fragment_size = avail;
    avail = len - count;
    if( fragment_size > avail ) fragment_size = avail;
    memcpy(bfr, &data[out], fragment_size);
    bfr += fragment_size;
    out += fragment_size;
    if( out >= alloc ) out = 0;
    out_pos += fragment_size;
    count += fragment_size;
  }
  return count;
}

static int fd_name(int fd,char *nm,int sz)
{
  char pfn[PATH_MAX];
  snprintf(&pfn[0],sizeof(pfn),"/proc/self/fd/%d",fd);
  return readlink(&pfn[0],nm,sz);
}

zfs_t::
fs_t(zmpeg3_t *zsrc, const char *fpath, int access)
{
  src = zsrc;
  strcpy(path, fpath);
  buffer = new buffer_t(src, access);
  if( (access & io_SINGLE_ACCESS) )
    open_file();
}

zfs_t::
fs_t(zmpeg3_t *zsrc, int fd, int access)
{
  src = zsrc;
  access |= io_UNBUFFERED;
  access |= io_SINGLE_ACCESS;
  if( (access & io_THREADED) ) access |= io_SEQUENTIAL;
  buffer = new buffer_t(src, access);
  if( !fd_name(fd,&path[0],sizeof(path)) )
    css.get_keys(&path[0]);
  is_open = 1;
  buffer->fd = fd;
  buffer->open_count = 1;
  get_total_bytes();
  if( (buffer->access_type & io_THREADED) )
    buffer->start_reader();
}

zfs_t::
fs_t(zmpeg3_t *zsrc, FILE *fp, int access)
{
  src = zsrc;
  access &= ~io_UNBUFFERED;
  access |= io_SINGLE_ACCESS;
  if( (access & io_THREADED) ) access |= io_SEQUENTIAL;
  buffer = new buffer_t(src, access);
  int fd = fileno(fp);
  if( !fd_name(fd,&path[0],sizeof(path)) )
    css.get_keys(&path[0]);
  is_open = 1;
  buffer->fp = fp;
  buffer->open_count = 1;
  get_total_bytes();
  if( (buffer->access_type & io_THREADED) )
    buffer->start_reader();
}

zfs_t::
~fs_t()
{
  close_file();
  close_buffer();
}

zfs_t::
fs_t(zfs_t &fs)
{
  src = fs.src;
  strcpy(path, fs.path);
  total_bytes = fs.total_bytes;
  css = fs.css;
  if( !fs.buffer ) return;
  int access = fs.buffer->access_type;
  if( (access & io_SINGLE_ACCESS) ) {
    buffer = fs.buffer;
    ++buffer->ref_count;
    is_open = fs.is_open;
    if( is_open )
      ++buffer->open_count;
  }
  else
    buffer = new buffer_t(src, access);
}

uint8_t zfs_t::
next_char()
{
  enter();
  uint32_t ret = buffer->next_byte();
  leave();
  return ret;
}

uint8_t zfs_t::
read_char()
{
  enter();
  uint32_t result = buffer->get_byte();
  ++current_byte;
  leave();
  return result;
}

uint32_t zfs_t::
read_uint16()
{
  uint32_t a, b;
  enter();
  if( current_byte+2 < buffer->file_tell() ) {
    b = buffer->get_byte();
    a = buffer->get_byte();
    current_byte += 2;
  }
  else {
    b = buffer->get_byte(); chk_next();
    a = buffer->get_byte(); ++current_byte;
  }
  leave();
  uint32_t result = (b << 8) | (a);
  return result;
}

uint32_t zfs_t::
read_uint24()
{
  uint32_t a, b, c;
  enter();
  if( current_byte+3 < buffer->file_tell() ) {
    c = buffer->get_byte();
    b = buffer->get_byte();
    a = buffer->get_byte();
    current_byte += 3;
  }
  else {
    c = buffer->get_byte(); chk_next();
    b = buffer->get_byte(); chk_next();
    a = buffer->get_byte(); ++current_byte;
  }
  leave();
  uint32_t result = (c << 16) | (b << 8) | (a);
  return result;
}

uint32_t zfs_t::
read_uint32()
{
  uint32_t a, b, c, d;
  enter();
  if( current_byte+4 < buffer->file_tell() ) {
    d = buffer->get_byte();
    c = buffer->get_byte();
    b = buffer->get_byte();
    a = buffer->get_byte();
    current_byte += 4;
  }
  else {
    d = buffer->get_byte(); chk_next();
    c = buffer->get_byte(); chk_next();
    b = buffer->get_byte(); chk_next();
    a = buffer->get_byte(); ++current_byte;
  }
  leave();
  uint32_t result = (d << 24) | (c << 16) | (b << 8) | (a);
  return result;
}

uint64_t zfs_t::
read_uint64()
{
  uint32_t a, b, c, d;
  uint64_t result;
  enter();
  if( current_byte+8 < buffer->file_tell() ) {
    d = buffer->get_byte();
    c = buffer->get_byte();
    b = buffer->get_byte();
    a = buffer->get_byte();
    result = (d << 24) | (c << 16) | (b << 8) | (a);
    d = buffer->get_byte();
    c = buffer->get_byte();
    b = buffer->get_byte();
    a = buffer->get_byte();
    current_byte += 8;
  }
  else {
    d = buffer->get_byte(); chk_next();
    c = buffer->get_byte(); chk_next();
    b = buffer->get_byte(); chk_next();
    a = buffer->get_byte(); chk_next();
    result = (d << 24) | (c << 16) | (b << 8) | (a);
    d = buffer->get_byte(); chk_next();
    c = buffer->get_byte(); chk_next();
    b = buffer->get_byte(); chk_next();
    a = buffer->get_byte(); ++current_byte;
  }
  result = (result <<32 ) | (d << 24) | (c << 16) | (b << 8) | (a);
  leave();
  return result;
}

int64_t zfs_t::
get_total_bytes()
{
  total_bytes = (buffer->access_type & io_SEQUENTIAL) ?
    INT64_MAX : path_total_bytes(path);
  return total_bytes;
}

int zfs_t::
open_file()
{
  /* Need to perform authentication before reading a single byte. */
//zmsgs("1 %s\n", path);
  if( !is_open ) {
    css.get_keys(path);
    if( buffer->open_file(path) ) return 1; 
    if( !get_total_bytes() ) {
      buffer->close_file();
      return 1;
    }
    current_byte = 0;
    is_open = 1;
  } 
  return 0;
}

void zfs_t::
close_file()
{
  if( is_open ) {
    if( buffer ) 
      buffer->close_file();
    is_open = 0;
  }
}

int zfs_t::
read_data(uint8_t *bfr, int64_t len)
{
  int64_t n, count = 0;
//zmsgs("1 %d\n", len);
  int result = enter();
  if( !result ) do {
    n = buffer->read_out(&bfr[count],len-count);
    current_byte += n;  count += n;
  } while( n > 0 && count < len && !(result=sync()) );
  leave();

//zmsgs("100 %d %d\n", result, count);
  return (result && count < len) ? 1 : 0;
}

int zfs_t::
seek(int64_t byte)
{
//zmsgs("1 %jd\n", byte);
  current_byte = byte;
  int result = (current_byte < 0) || (current_byte > total_bytes);
  return result;
}

int zfs_t::
seek_relative(int64_t bytes)
{
//zmsgs("1 %jd\n", bytes);
  current_byte += bytes;
  int result = (current_byte < 0) || (current_byte > total_bytes);
  return result;
}

void zfs_t::restart()
{
  if( !buffer ) return;
  if( !(buffer->access_type & io_SINGLE_ACCESS) ) return;
  if( (buffer->access_type & io_THREADED) )
    buffer->do_restart = 1;
  else
    buffer->restart();
}

int zfs_t::
pause_reader(int v)
{
  if( !buffer ) return 1;
  buffer->paused = v;
  return 0;
}

int zfs_t::
start_record(int bsz)
{
  if( !buffer ) return -1;
  buffer->write_align(bsz);
  return buffer->start_record();
}

int zfs_t::
stop_record()
{
  return buffer ? buffer->stop_record() : -1;
}

void zfs_t::
close_buffer()
{
  if( !buffer || --buffer->ref_count > 0 ) return;
  delete buffer;  buffer = 0;
}

