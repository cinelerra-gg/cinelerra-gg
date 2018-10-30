#include <cstdio>
#include <cstring>
#include <new>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "tdb.h"

#if 0
#include <execinfo.h>
extern "C" void traceback(const char *fmt,...)
{
  FILE *fp = fopen("/tmp/x","a");
  if( !fp ) return;
  va_list ap;  va_start(ap, fmt);
  vfprintf(fp, fmt, ap);
  va_end(ap);
  int nptrs;  void *buffer[100];
  nptrs = backtrace(buffer, lengthof(buffer));
  struct timeval now; gettimeofday(&now,0);
  fprintf(fp,"@%ld.03%ld %5d: ",now.tv_sec,now.tv_usec/1000,getpid());
  fprintf(fp,"*** stack %d\n",nptrs);
  char **strings = backtrace_symbols(buffer, nptrs);
  if( !strings ) return;
  for( int i=0; i<nptrs; ++i ) fprintf(fp,"%s\n", strings[i]);
  free(strings);
  fclose(fp);
}
#endif

// spif mods
//  use repeated string byte move for decompress operation.
//  page table design should be changed to hierachical bit
//    field, so that only the trace of page table blocks in
//    the update path are written at commit time.  Writing
//    the entire page table on commit can be expensive.

// local memory allocator
void *Db::get_mem8_t(int id)
{
  return 0;
}

void *Db::new_mem8_t(int size, int &id)
{
  return new uint8_t[size];
}

int Db::del_mem8_t(const void *vp, int id)
{
  delete [] (uint8_t*)vp;
  return 0;
}

volatile int gdb_wait;

void wait_gdb()
{
  gdb_wait = 1;
  while( gdb_wait )
    sleep(1);
}

// shared memory allocator
void *Db::
get_shm8_t(int id)
{
  void *vp = shmat(id, NULL, 0);
  if( vp == (void*)-1 ) { perror("shmat"); wait_gdb(); vp = 0; }
  return (uint8_t *)vp;
}

void *Db::
new_shm8_t(int size, int &id)
{
  id = shmget(IPC_PRIVATE, size, IPC_CREAT+0666);
  if( id < 0 ) { perror("shmget"); return 0; }
  uint8_t *addr = (uint8_t *)get_shm8_t(id);
  if( addr ) shmctl(id, IPC_RMID, 0);
  return addr;
}

int Db::
del_shm8_t(const void *vp, int id)
{
  int ret = 0;
  if( id >= 0 ) {
    struct shmid_ds ds;
    if( !shmctl(id, IPC_STAT, &ds) ) ret = ds.shm_nattch;
    else { perror("shmctl"); wait_gdb(); ret = errIoStat; }
  }
  if( vp ) shmdt(vp);
  return ret;
}


// memory allocator
uint8_t *Db::
get_uint8_t(int id, int pg)
{
  uint8_t *bp = (uint8_t *) get_mem(id);
  return bp;
}

uint8_t *Db::
new_uint8_t(int size, int &id, int pg)
{
  void *vp = new_mem(size, id);
  return (uint8_t *)vp;
}

int Db::
del_uint8_t(const void *vp, int id, int pg)
{
  return del_mem(vp, id);
}


void Db::
error(int v)
{
  if( !err_no ) {
    err_no = v;
    if( err_callback ) err_callback(this, v);
  }
}

//int Db::debug = DBBUG_ERR + DBBUG_FAIL;
int Db::debug = DBBUG_ERR;

#ifdef DEBUG

void dmp(unsigned char *bp, int len)
{
  unsigned char ch[16], lch[16];
  int llen = lengthof(lch)+1;
  int dups = 0;
  unsigned char *adr = 0;
  int i, n;

  do {
    if( !adr ) adr = bp;
    for( n=0; n<lengthof(ch) && --len>=0; ch[n++]=*bp++ );
    if( (i=n) >= llen ) // check for a duplicate
      while( --i>=0 && ch[i]==lch[i] );
    if( i >= 0 || len < 0 ) { /* not a duplicate */
      if( dups > 0 ) fprintf(stderr," ...%d\n",dups);
      dups = 0;
      fprintf(stderr,"%p:",adr);
      for( i=0; i<n; ++i ) fprintf(stderr," %02x",lch[i]=ch[i]);
      for( i=n; i<lengthof(ch); ++i ) fprintf(stderr,"   ");
      fprintf(stderr," ");
      for( i=0; i<n; ++i )
        fprintf(stderr,"%c", ch[i]>=' ' && ch[i]<='~' ? ch[i] : '.');
      fprintf(stderr,"\n");
      adr += n;
    }
    else {
      dups += n;
      adr = 0;
    }
    llen = n;
  } while( len > 0 );
}

const char *Db::
errMsgs[] = {
    "NoCmprFn", "NotFound", "Duplicate", "NoPage", "NoMemory",
    "IoRead", "IoWrite", "IoSeek", "IoStat", "BadMagic", "Corrupt",
    "Invalid", "Previous", "ObjEntity", "Limit", "User",
};

void Db::
dmsg(int msk, const char *msg,...)
{
  if( !(msk & debug) ) return;
#ifdef DEBUG_TIMESTAMPS
  struct timeval now; gettimeofday(&now,0);
  printf("@%ld.03%ld: ",now.tv_sec,now.tv_usec/1000);
#endif
  va_list ap;
  va_start(ap, msg);
  vprintf(msg, ap);
  va_end(ap);
FILE *fp = fopen("/tmp/x","a"); if( !fp ) return;
fprintf(fp,"@%ld.03%ld %5d: ",now.tv_sec,now.tv_usec/1000,getpid());
va_start(ap, msg); vfprintf(fp, msg, ap); va_end(ap);
fclose(fp);
}

int Db::
_err_(int v,const char *fn,int ln)
{
  error(v);
  dmsg(DBBUG_ERR,"%s:%d errored %d (%s)\n",fn,ln,v,errMsgs[-v]);
  wait_gdb();
  return v;
}

int Db::
_fail_(int v,const char *fn,int ln)
{
  dmsg(DBBUG_FAIL,"%s:%d failed %d (%s)\n",fn,ln,v,errMsgs[-v]);
  return v;
}

void Db::
Error(int v,const char *msg)
{
  error(v);
  dmsg(DBBUG_ERR,"%s\n",msg);
}

void Db::
dmp()
{
  tdmp();  pdmp();
  printf("\n");
}

void Db::
tdmp()
{
  printf("dmp  root_info->file_size %016lx\n",
    root_info->file_size);
  printf(" rootInfo root_info->transaction_id %d\n",
    root_info->transaction_id);
  printf("   root_info->root_info_addr/size %016lx/%08x\n",
    root_info->root_info_addr,root_info->root_info_size);
  printf("   root_info->last_info_addr/size %016lx/%08x\n",
    root_info->last_info_addr,root_info->last_info_size);
  printf("   root_info->indeciesUsed %d\n",
    root_info->indeciesUsed);
  printf("   alloc_cache: "); alloc_cache.dmp();
  for( int idx=0; idx<root_info->indeciesUsed; ++idx ) {
    IndexBase *ib = indecies[idx];
    if( !ib ) continue;
    printf("     idx %d. %-24s %s%c pop %5ld"
      "   root %-5d rhs %-5d ky/Dt %2d/%-2d ",
      idx, &ib->st->name[0], ib->st->type==idxBin ? "bin" :
      ib->st->type==idxStr ? "str" : "???",
      ib->st->key_type >= ktyBin && ib->st->key_type <= ktyDir ?
        " *="[ib->st->key_type] : '?', ib->st->count,
      ib->st->rootPageId, ib->st->rightHandSide,
      ib->st->keySz, ib->st->dataSz);
    printf("   free %d/",ib->st->freeBlocks);
    int n = 0;
    for( pageId id=ib->st->freeBlocks; id>=0; ++n ) {
      pgRef loc; loc.id = id;  loc.offset = 0;
      keyBlock *kbb;  addrRead_(loc,kbb);
      if( (id=kbb->right_link()) < 0 ) break;
      //printf(", %d",id);
    }
    printf("%d pages\n",n);
  }

  printf("   Entities,  count %ld\n", entityIdIndex->Count());
  Entity e(this);  EntityLoc &ent = e.ent;  int eid;
  if( !entityIdIndex->First(&eid,&ent.obj) ) do {
    int nidxs = ent->nidxs;
    printf("     id %d. %s  maxId %d, recdSz %d, count %d, nidxs %d:",
      eid, &ent->name[0], ent->maxId, ent->recdSz, ent->count, nidxs);
    printf("   alloc_cache: "); ent->alloc_cache.dmp();
    for( int i=0; i<nidxs; ++i ) {
      int idx = ent->indexs[i];
      printf(" %d(%s),", idx, idx < 0 ? "" :
         !indecies[idx] ? "(err)" : &indecies[idx]->st->name[0]);
    }
    printf("\n");
  } while( !entityIdIndex->Next(&eid,&ent.obj) );
}

void Db::
pdmp()
{
  printf("   root_info->pageTableUsed %d\n",root_info->pageTableUsed);
  for( int pid=0; pid<root_info->pageTableUsed; ++pid ) {
    Page &pg = *get_page(pid);
    if( !pg.addr && !pg->io_allocated && pg->io_addr==NIL &&
        pg->trans_id < 0 && pg->shmid < 0 && !pg->flags &&
        !pg->wr_allocated && pg->wr_io_addr==NIL ) continue;
    printf("     pid %d.  used %d, type %d, link %d, flags %04x addr %p\n",
      pid, pg->used, pg->type, pg->link, pg->flags, pg.addr);
    printf("     allocated %d io_alloc/io_addr %d/%016lx, wr_alloc/io_addr %d/%016lx\n",
      pg->allocated, pg->io_allocated, pg->io_addr,
      pg->wr_allocated, pg->wr_io_addr);
  }
  printf("   root_info->freePages %d",root_info->freePages);
  int n = 0;
  for( pageId id=root_info->freePages; id>=0; ++n ) id = (*get_page(id))->link;
    // printf(" %d\n",(id=(*get_page(id))->link));
  printf(",  pages = %d\n",n);
}

void Db::
fdmp()
{
  freeStoreRecord free;
  if( !freeStoreIndex->First(&free,0) ) do {
      printf("free=%04lx/%012lx\n", free.size,free.io_addr);
  } while( !freeStoreIndex->Next(&free,0) );
}

void Db::
admp()
{
  addrStoreRecord addr;
  if( !addrStoreIndex->First(&addr,0) ) do {
      printf("addr=%012lx/%04lx\n", addr.io_addr,addr.size);
  } while( !addrStoreIndex->Next(&addr,0) );
}

void Db::
cdmp()
{
  Entity e(this);  EntityLoc &ent = e.ent;  int ret, eid;
  if( !(ret=entityIdIndex->First(&eid,&ent.obj)) ) do {
    printf(" %d. %-32s  %5d/%-5d  %d\n",eid,ent->name,
      ent->alloc_cache.loc.id, ent->alloc_cache.loc.offset,
      ent->alloc_cache.avail);
  } while( !(ret=entityIdIndex->Next(&eid,&ent.obj)) );
}

void Db::
achk()
{
  if( !indecies ) return;  addrStoreRecord addr;
  addrStoreRecord last;  last.io_addr = 0; last.size = 0;
  if( !addrStoreIndex->First(&addr,0) ) do {
      if( last.io_addr > addr.io_addr ||
         (last.io_addr == addr.io_addr && last.size >= addr.size ) ) {
        printf("last=%012lx/%04lx, addr=%012lx/%04lx\n",
           addr.io_addr, addr.size, last.io_addr, last.size);
        wait_gdb();
      }
      last = addr;
  } while( !addrStoreIndex->Next(&addr,0) );
}

void Db::
fchk()
{
  if( !indecies ) return;  freeStoreRecord free;
  freeStoreRecord last;  last.size = 0; last.io_addr = 0;
  if( !freeStoreIndex->First(&free,0) ) do {
      if( last.size > free.size ||
         (last.size == free.size && last.io_addr >= free.io_addr ) ) {
        printf("last=%04lx/%012lx, free=%04lx/%012lx\n",
           free.size, free.io_addr, last.size, last.io_addr);
        wait_gdb();
      }
      last = free;
  } while( !freeStoreIndex->Next(&free,0) );
}

void Db::
edmp(AllocCache &cache)
{
  freeSpaceRecord free;
  if( !freeSpaceIndex->First(&free,0) ) do {
      printf("free=%04lx %d/%d\n", free.size,free.id,free.offset);
  } while( !freeSpaceIndex->Next(&free,0) );
}

void Db::
bdmp(AllocCache &cache)
{
  addrSpaceRecord addr;
  if( !addrSpaceIndex->First(&addr,0) ) do {
    printf("addr=%d/%d %04lx\n", addr.id,addr.offset,addr.size);
  } while( !addrSpaceIndex->Next(&addr,0) );
}

void Db::
stats()
{
  long store_allocated=0, store_used=0;
  long loaded_allocated=0, loaded_used=0;
  long written_allocated=0, written_used=0;
  int pages_written=0, pages_loaded=0, pages_deleted=0;
  for( int id=0,n=root_info->pageTableUsed; --n>=0; ++id ) {
    Page &pg = *get_page(id);
    store_allocated += pg->allocated;
    store_used += pg->used;
    if( pg.addr ) {
      ++pages_loaded;
      loaded_allocated += pg->allocated;
      loaded_used += pg->used;
    }
    if( pg->chk_flags(fl_wr) ) {
      ++pages_written;
      written_allocated += pg->allocated;
      written_used += pg->used;
      if( !pg.addr ) ++pages_deleted;
    }
  }
#define percent(a,b) (!(a) || !(b) ? 0.0 : (100.0*(a)) / (b))
  long read_allocated = loaded_allocated - written_allocated;
  long read_used = loaded_used - written_used;
  int pages_read = pages_loaded - pages_written;
  printf("\ncommit %d\n", root_info->transaction_id);
  printf("    pages  %8d/del %-4d  alloc:%-12ld  used:%-12ld  %7.3f%%\n",
    root_info->pageTableUsed, pages_deleted, store_allocated, store_used,
    percent(store_used,store_allocated));
  printf("    loaded %8d/%-7.3f%%  alloc:%-12ld  used:%-12ld  %7.3f%%\n",
    pages_loaded, percent(pages_loaded, root_info->pageTableUsed),
    loaded_allocated, loaded_used, percent(loaded_used, loaded_allocated));
  printf("    read   %8d/%-7.3f%%  alloc:%-12ld  used:%-12ld  %7.3f%%\n",
    pages_read, percent(pages_read, root_info->pageTableUsed),
    read_allocated, read_used, percent(read_used, read_allocated));
  printf("    write  %8d/%-7.3f%%  alloc:%-12ld  used:%-12ld  %7.3f%%\n",
    pages_written, percent(pages_written, root_info->pageTableUsed),
    written_allocated, written_used, percent(written_used, written_allocated));
#undef percent
}

#endif


#ifdef ZFUTEX

int Db::zloc_t::
zyield()
{
  return syscall(SYS_sched_yield);
}

int Db::zloc_t::
zgettid()
{
  return syscall(SYS_gettid);
}

int Db::zloc_t::
zwake(int nwakeups)
{
  int ret;
  while( (ret=zfutex(FUTEX_WAKE, nwakeups)) < 0 );
  return ret;
}

int Db::zloc_t::
zwait(int val, timespec *ts)
{
  return zfutex(FUTEX_WAIT, val, ts);
}

int Db::zlock_t::
zemsg1()
{
  fprintf(stderr,"unlocking and not locked\n");
  return -1;
}

int Db::zlock_t::
zlock(int v)
{
  if( v || zxchg(1,loc) >= 0 ) do {
    zwait(1);
  } while ( zxchg(1,loc) >= 0 );
  return 1;
}

int Db::zlock_t::
zunlock(int nwakeups)
{
  loc = -1;
  return zwake(1);
}

void Db::zrwlock_t::
zenter()
{
  zdecr(loc); lk.lock(); lk.unlock(); zincr(loc);
}

void Db::zrwlock_t::
zleave()
{
  if( lk.loc >= 0 ) zwake(1);
}

void Db::zrwlock_t::
write_enter()
{
  lk.lock();  timespec ts = { 1, 0 };
  int v;  while( (v=loc) >= 0 ) zwait(v, &ts);
}

void Db::zrwlock_t::
write_leave()
{
  lk.unlock();
}

#endif

int Db::attach_wr()
{
  if( !db_info ) return -1;
  if( is_locked() > 0 && db_info->owner == my_pid ) return 0;
  write_enter();
//traceback(" attach_wr %d/%d/%d\n",my_pid,db_info->owner,db_info->last_owner);
  db_info->last_owner = db_info->owner;
  db_info->owner = my_pid;      
  return 1;
}

int Db::attach_rd()
{
  if( db_info ) {
    enter();
//traceback(" attach_rd %d/%d\n",my_pid,db_info->owner);
  }
  return 1;
}

void Db::detach_rw()
{
  if( !db_info ) return;
  int v = is_locked();
  if( v < 0 ) return;
//fchk();  achk();
  if( v > 0 )
    write_leave();
  else
    leave();
//traceback(" detach_rw %d\n",my_pid);
}

// persistent pageTable element initial constructor
void Db::PageStorage::
init()
{
  used = 0;
  allocated = 0;
  flags = 0;
  type = pg_unknown;
  io_allocated = 0;
  wr_allocated = 0;
  shmid = -1;
  link = NIL;
  trans_id = -1;
  io_addr = NIL;
  wr_io_addr = NIL;
}

// non-persistent pageTable element initial constructor
void Db::Page::
init()
{
  addr = 0;
  shm_id = -1;
}

void Db::Page::
reset_to(Page *pp)
{
  addr = pp->addr;
  shm_id = pp->shm_id;
  pp->init();
  *st = *pp->st;
  pp->st->init();
}

// deletes storage next start_transaction
int Db::Page::
release()
{
  st->used = 0;  st->set_flags(fl_wr);
  return 0;
}

// locked pageTable access
Db::Page *Db::
get_page(pageId pid)
{
  read_locked by(db_info->pgTblLk);
  return get_Page(pid);
}

/***
 *  Db::alloc_pageTable -- alloc page table
 *
 *  parameters
 *    sz      int        input         page table size
 *  returns error code
 */
int Db::
alloc_pageTable(int sz)
{
  write_blocked by(db_info->pgTblLk);
  int n = pageTableHunks(sz) * pageTableHunkSize;
  Page **pt = new Page*[n];
  if( !pt ) Err(errNoMemory);
  int info_id, info_sz = n*sizeof(PageStorage);
  PageStorage *new_page_info = (PageStorage *)new_uint8_t(info_sz, info_id);
  if( !new_page_info ) { delete pt;  Err(errNoMemory); }
  int i = 0;
  if( page_info ) {
    for( ; i<root_info->pageTableUsed; ++i ) {
      pt[i] = get_Page(i);
      new_page_info[i] = *pt[i]->st;
      pt[i]->st = &new_page_info[i];
    }
  }
  for( ; i<n; ++i ) pt[i] = 0;
  db_info->page_info_id = page_info_id = info_id;            
  del_uint8_t(page_info);  page_info = new_page_info;
  delete [] pageTable;  pageTable = pt;
  pageTableAllocated = n;
  return 0;
}

/***
 *  Db::new_page -- access/construct new page
 *
 *  parameters: none
 *  returns page id on success (>=zero), error code otherwise(< 0)
 */

Db::pageId Db::
new_page()
{
  locked by(db_info->pgAlLk);
  pageId id = root_info->freePages;
  if( id < 0 ) {
    if( root_info->pageTableUsed >= pageTableAllocated ) {
      int sz = root_info->pageTableUsed + root_info->pageTableUsed/2 + 1;
      if_err( alloc_pageTable(sz) );
    }
    id = root_info->pageTableUsed;
    Page * pp = new Page(*new(&page_info[id]) PageStorage());
    if( !pp ) Err(errNoMemory);
    set_Page(id, pp);
    page_table_sz = ++root_info->pageTableUsed;
  }
  else {
    Page &pg = *get_page(id);
    root_info->freePages = pg->link;
    new(&pg) Page(*new(&page_info[id]) PageStorage());
  }
  return id;
}

void Db::
del_page(pageId id)
{
  Page *pp = get_Page(id);
  pageDealloc(*pp);
  delete pp;
  set_Page(id, 0);
}

/***
 *  Db::free_page -- link to root_info->freePages
 *
 *  parameters
 *    pid     pageId     input         page id
 */

void Db::
free_page_(int pid)
{
  Page &pg = *get_Page(pid);
  pageDealloc(pg);
  pg->allocated = 0;
  pg->used = 0;
  pg->clr_flags(fl_wr | fl_rd);
  pg->set_flags(fl_free);
  pageId id = root_info->freePages;
#if 0
  Page *lpp = 0;  // use sorted order
  while( id >= 0 && id < pid ) {
    lpp = get_Page(id);
    id = (*lpp)->link;
  }
  if( lpp )
    (*lpp)->link = pid;
  else
#endif
    root_info->freePages = pid;
  pg->link = id;
}

Db::pageId Db::
lower_page(pageId mid)
{
  locked by(db_info->pgAlLk);
  pageId id = root_info->freePages;
  pageId lid = mid;
  Page *pp = 0, *lpp = 0;
  while( id >= 0 ) {
    if( id < lid ) { lid = id;  lpp = pp; }
    pp = get_Page(id);
    id = (*pp)->link;
  }
  if( lid < mid ) {
    Page &pg = *get_Page(lid);
    if( lpp )
      (*lpp)->link = pg->link;
    else
      root_info->freePages = pg->link;
    lpp = get_Page(mid);
    pg.reset_to(lpp);
    free_page_(mid);
  }
  return lid;
}

int Db::
get_index(const char *nm, CmprFn cmpr)
{
  int idx = root_info->indeciesUsed;
  while( --idx >= 0  ) {
    IndexBase *ib = indecies[idx];
    if( !ib ) continue;
    if( !strncmp(&ib->st->name[0],nm,nmSz) ) break;
  }
  if( idx >= 0 && cmpr && indecies[idx]->st->type == idxBin ) {
    IndexBinary *bidx = (IndexBinary *)indecies[idx];
    bidx->compare = cmpr;
    bidx->bst->cmprId = findCmprFn(cmpr);
  }
  return idx;
}

long Db::
get_count(int r)
{
  if( r < 0 || r >= root_info->indeciesUsed ) Fail(errInvalid);
  if( !indecies[r] ) Fail(errNotFound);
  return indecies[r]->Count();
}

int Db::
alloc_indecies(int n)
{
  IndexBase **it = new IndexBase*[n];
  if( !it ) Err(errNoMemory);
  int info_id;
  IndexInfo *info = (IndexInfo *)new_uint8_t(n*sizeof(*info), info_id);
  if( !info ) { delete it; Err(errNoMemory); }
  memcpy(info, index_info, indeciesAllocated*sizeof(*info));
  int i = 0;
  for( ; i<root_info->indeciesUsed; ++i ) {
    if( !(it[i] = indecies[i]) ) continue;
    switch( it[i]->st->type ) {
    case idxBin: {
      IndexBinary *bidx = (IndexBinary *)it[i];
      bidx->st = info[i].bin;
      bidx->bst = info[i].bin;
      break; }
    case idxStr: {
      IndexString *sidx = (IndexString *)it[i];
      sidx->st = info[i].str;
      sidx->sst = info[i].str;
      break; }
    default:
      it[i]->st = 0;
      break;
    }
  }
  for( ; i<n; ++i ) it[i] = 0;
  db_info->index_info_id = index_info_id = info_id;  
  del_uint8_t(index_info);  index_info = info;
  delete [] indecies;  indecies = it;
  indeciesAllocated = n;
  return 0;
}

int Db::
new_index()
{
  int idx = 0;
  while( idx < root_info->indeciesUsed && indecies[idx] )
    ++idx;
  if( idx >= indeciesAllocated ) {
    int n = indeciesAllocated + indexTableHunkSize;
    if( alloc_indecies(n) ) Err(errNoMemory);
  }
  if( idx >= root_info->indeciesUsed ) {
    if( idx >= max_index_type ) Err(errLimit);
    root_info->indeciesUsed = idx+1;
    indecies_sz = root_info->indeciesUsed;
  }
  indecies[idx] = 0;
  return idx;
}

void Db::
del_index(int idx)
{
  delete indecies[idx];
  indecies[idx] = 0;
  for( idx=root_info->indeciesUsed; idx>0 && indecies[idx-1]==0; --idx );
  indecies_sz = root_info->indeciesUsed = idx;
}

int Db::
new_index(IndexBase *&ibp, IndexBaseInfo *b, IndexBinaryInfo *bi)
{
  IndexBaseStorage *st = new(b) IndexBaseStorage();
  IndexBinaryStorage *bst = new(bi) IndexBinaryStorage();
  IndexBinary *bidx = new IndexBinary(this, st, bst);
  if( !bidx || !bidx->ikey() || !bidx->tkey() ) {
    if( bidx ) delete bidx;
    Err(errNoMemory);
  }
  ibp = bidx;
  return 0;
}

int Db::
new_binary_index(const char *nm, int ksz, int dsz, CmprFn cmpr)
{
  if( get_index(nm) >= 0 ) Err(errDuplicate);
  int idx = new_index();
  if( idx < 0 ) Err(errNoMemory);
  IndexBinary *bidx = new IndexBinary(this, idx, ksz, dsz, cmpr);
  if( !bidx || !bidx->ikey() || !bidx->tkey() ) {
    del_index(idx);
    Err(errNoMemory);
  }
  if( nm ) strncpy(&bidx->st->name[0],nm,sizeof(bidx->st->name));
  indecies[idx] = bidx;
  return idx;
}

int Db::
new_index(IndexBase *&ibp, IndexBaseInfo *b, IndexStringInfo *si)
{
  IndexBaseStorage *st = new(b) IndexBaseStorage();
  IndexStringStorage *sst = new(si) IndexStringStorage();
  IndexString *sidx = new IndexString(this, st, sst);
  if( !sidx ) Err(errNoMemory);
  ibp = sidx;
  return 0;
}

int Db::
new_string_index(const char *nm, int dsz)
{
  if( get_index(nm) >= 0 ) Err(errDuplicate);
  int idx = new_index();
  if( idx < 0 ) Err(errNoMemory);
  IndexString *sidx = new IndexString(this, idx, dsz);
  if( !sidx ) {
    del_index(idx);
    Err(errNoMemory);
  }
  if( nm ) strncpy(&sidx->st->name[0],nm,sizeof(sidx->st->name));
  indecies[idx] = sidx;
  return idx;
}

/***
 *  Db::IndexBinary::keyMap - map function on index pages
 *
 *  parameters
 *      s      pageId     input        current id
 *
 *  returns 0 on success,
 *          errNotFound if index is empty
 *          error code otherwise
 */

int Db::IndexBinary::
keyMap(pageId s, int(IndexBase::*fn)(pageId id))
{
  pageId r;
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,0,sbb,spp,sn) );
  if( (r=sbb->right_link()) >= 0 ) {
    int lkdSz = kdSz + sizeof(pageId);
    int n = spp->iused() / lkdSz;
    for( int i=0; i<n; ++i ) {
      pageId l = readPageId(sn);
      if_ret( keyMap(l,fn) );
      sn += lkdSz;
    }
    if_ret( keyMap(r,fn) );
  }
  if_err( (this->*fn)(s) );
  return 0;
}

/***
 *  Db::IndexBinary::setLastKey -- conditionally update lastAccess
 *
 *  parameters
 *     s       pageId     input        page of last insert
 *     u       pageId     input        rightLink of page
 *     k       int        input        insert offset in block
 */

void Db::IndexBinary::
setLastKey(pageId s, pageId u, int k)
{
  if( keyInterior < 0 ) {
    if( u >= 0 ) {
      keyInterior = 1;
      k += sizeof(pageId);
    }
    else
      keyInterior = 0;
    lastAccess.id = s;
    lastAccess.offset = sizeof(keyBlock) + k;
  }
}

/***
 *  Db::IndexBinary::keyLocate -- generalized database key retreival
 *
 *  parameters
 *      s      pageId     input        current id
 *   cmpr      CmprFn     input        key compare function
 *
 * returns 0 on success
 *         errNotFound if not found,
 *         error code otherwise
 */

int Db::IndexBinary::
keyLocate(pgRef &last, pageId s, int op,void *ky,CmprFn cmpr)
{
  int ret = errNotFound;
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,0,sbb,spp,sn) );
  int len = spp->iused();
  int lkdSz = kdSz;
  if( sbb->right_link() >= 0 )
     lkdSz += sizeof(pageId);

  int l = -1;
  int r = len / lkdSz;
  // binary search block for key
  while( (r - l) > 1 ) {
    int i = (l+r) >> 1;
    int k = i * lkdSz;
    if( sbb->right_link() >= 0 )
       k += sizeof(pageId);
    char *kn = sn + k;
    int n = cmpr((char*)ky,kn);
    if( n == 0 ) {
      if( op >= keyLE && op <= keyGE ) {
        last.id = s;
        last.offset = sizeof(keyBlock) + k;
        ret = 0;
      }
      if( op == keyLE || op == keyGT ) n = 1;
    }
    if( n > 0 ) l = i; else r = i;
  }

  r *= lkdSz;
  int k = op < keyEQ ? l*lkdSz : (r < len ? r : -1);
  if( op != keyEQ && k >= 0 ) {
    if( sbb->right_link() >= 0 )
      k += sizeof(pageId);
    last.id = s;
    last.offset = sizeof(keyBlock) + k;
    ret = 0;
  }

  if( (s = sbb->right_link()) >= 0 ) {
    if( r < len ) s = readPageId(sn+r);
    k = ret;
    ret = keyLocate(last,s,op,ky,cmpr);
    if( k == 0 ) ret = 0;
  }

  return ret;
}

/***
 *  Db::IndexBinary::Locate -- interface to generalized database key retreival
 *
 *  parameters
 * op          int             input        key relation in keyLT..keyGT
 * key         void *          input        retreival key image
 * cmpr        CmprFn          input        retreival key image
 * rtnKey      void *          output       resulting key value
 * rtnData     void *          output       resulting recd value
 *
 * returns 0 on success
 *         errNotFound on not found,
 *         error code otherwise
 */

int Db::IndexBinary::
refLocate(pgRef &loc, int op, void *key, CmprFn cmpr)
{
  if( st->rootPageId == NIL )
    Fail(errNotFound);
  if( op == keyEQ ) op = keyLE;
  if( !cmpr ) cmpr = compare;
  if_fail( keyLocate(loc,st->rootPageId,op, key,cmpr) );
{ locked by(idxLk);
  chkLastFind(loc); }
  return 0;
}

int Db::IndexBinary::
Locate(int op, void *key, CmprFn cmpr, void *rtnKey, void *rtnData)
{
  pgRef last;
  if_fail( refLocate(last, op, key, cmpr) );
  char *kp = 0;
  if_err( db->addrRead_(last,kp) );
  if( rtnKey && st->keySz > 0 )
    wr_key(kp, (char*)rtnKey,st->keySz);
  if( rtnData && st->dataSz > 0 )
    memmove(rtnData,kp+st->keySz,st->dataSz);
  return 0;
}

/***
 *  Db::IndexBinary::chkFind - check lastAccess block for key
 *
 *  parameters
 *    key      char *          input        key image
 *    last     pgRef *         input        last position loc
 *
 *  returns 0 if not found
 *          error code otherwise
 */

int Db::IndexBinary::
chkFind(pgRef &loc, char *key)
{
  pageId s = loc.id;
  if( s < 0 ) return 0;                         // must be valid block
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,0,sbb,spp,sn) );
  if( sbb->right_link() >= 0 ) return 0;        // must be leaf
  int slen = spp->iused();
  int k = loc.offset - sizeof(keyBlock);
  if( k < 0 || k > slen ) return 0;             // must be inside/end of block
  int cmpr0 = k>=slen ? -1 : compare(key,sn+k); // compare last access
  if( cmpr0 ) {                                 // not found here
    int l = k;
    if( cmpr0 < 0 ? (k-=kdSz) < 0 : (k+=kdSz) >= slen ) return 0;
    int cmpr1 = compare(key,sn+k);
    if( !cmpr1 ) goto xit;                        // found here
    if( cmpr1 > 0 ) {
      if( l >= slen ) return 0;                   // no curr
      if( cmpr0 < 0 ) Fail(errNotFound);          // btwn prev/curr
      k = slen - kdSz;
      cmpr1 = compare(key,sn+k);
      if( !cmpr1 ) goto xit;                      // found here
      if( cmpr1 > 0 ) return 0;                   // key after last in block
    }
    else {
      if( cmpr0 > 0 ) Fail(errNotFound);          // btwn curr/next
      k = 0;
      cmpr1 = compare(key,sn);
      if( !cmpr1 ) goto xit;                      // found here
      if( cmpr1 < 0 ) return 0;                   // key before first in block
    }
    return errNotFound;                           // key in block range, but not found
  }
xit:
  loc.offset = sizeof(keyBlock) + k;
  return 1;
}

/***
 *  Db::IndexBinary::keyFind -- database unique key retreival
 *
 *  parameters
 *      s      pageId     input        current id
 *
 * returns 0 on success
 *         errNotFound on not found,
 *         error code otherwise
 */

int Db::IndexBinary::
keyFind(pgRef &loc, void *ky, pageId s)
{
  for(;;) {
    keyBlock *sbb;  Page *spp;  char *sn;
    if_err( db->indexRead(s,0,sbb,spp,sn) );
    int lkdSz = kdSz;
    if( sbb->right_link() >= 0 )
      lkdSz += sizeof(pageId);
    int len = spp->iused();

    int l = -1;
    int r = len/lkdSz;
    // binary search block for key
    while( (r - l) > 1 ) {
      int i = (l+r) >> 1;
      int k = i*lkdSz;
      if( sbb->right_link() >= 0 )
        k += sizeof(pageId);
      char *kn = sn + k;
      int n = compare((char*)ky,kn);
      if( n == 0 ) {
        loc.id = s;
        loc.offset = sizeof(keyBlock) + k;
        return 0;
      }
      if( n > 0 ) l = i; else r = i;
    }

    if( (s = sbb->right_link()) < 0 ) break;
    if( (r*=lkdSz) < len ) s = readPageId(sn+r);
  }

  Fail(errNotFound);
}

/***
 *  Db::IndexBinary::Find -- interface to database unique key retreival
 *
 *  parameters
 * key         void *          input        retreival key image
 * rtnData     void *          output       resulting recd value
 *
 * returns 0 on success
 *         errNotFound on not found,
 *         error code otherwise
 */

int Db::IndexBinary::
refFind(pgRef &loc, void *ky)
{
  if( st->rootPageId == NIL )
    Fail(errNotFound);
  pageId r = st->rootPageId;
  int ret = 0;
{ locked by(idxLk);
  loc = lastFind;
  if( CHK cFindCount > 2 ) ret = 1; }
  if( ret ) {                                   // try the easy way
    ret = chkFind(loc, (char *)ky);
    if( ret == errNotFound ) {
      r = loc.id;  ret = 0;
    }
  }
  if_err( ret );
  if( !ret ) {                                  // try the hard way
    if_fail( keyFind(loc,ky,r) );
  }
{ locked by(idxLk);
  chkLastFind(loc); }
  return 0;
}

int Db::IndexBinary::
Find(void *ky, void *rtnData)
{
  pgRef last;
  if_fail( refFind(last, ky) );
  char *kp = 0;
  if_err( db->addrRead_(last,kp) );
  if( rtnData )
    memmove(rtnData,kp+st->keySz,st->dataSz);
  return 0;
}


int Db::IndexBinary::
chkInsert(void *key, void *data)
{
  int rhs = 0;
  char *ky = (char *)key;
  pageId s = lastInsert.id;
  if( s < 0 || cInsCount < 2 ) return 0;        /* >= 2 in a row */
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,1,sbb,spp,sn) );
  if( sbb->right_link() >= 0 ) return 0;        /* must be exterior */
  int slen = spp->iused();
  int k = lastInsert.offset - sizeof(keyBlock);
  char *kp = sn + k;
  char *kn = kp + kdSz;
  char *rp = sn + slen;
  int n = compare(ky,kp);
  if( n == 0 ) Fail(errDuplicate);
  if( n > 0 ) {                                 /* after last one */
    if( kn >= rp ) {                            /* no next one */
      if( st->rightHandSide == s )
        rhs = 1;                                /* rhs */
    }
    else {
      n = compare(ky,kn);
      if( n == 0 ) Fail(errDuplicate);
      if( n < 0 )
        rhs = 1;                                /* before next one */
    }
  }
  if( !rhs ) return 0;                          /* not a hit */
  if( spp->iallocated()-slen < kdSz ) return 0; /* doesnt fit */
  if( rp > kn ) memmove(kn+kdSz,kn,rp-kn);      /* move data up */
  if( key && st->keySz > 0 )
    wr_key(key, kn,st->keySz);
  if( data && st->dataSz > 0 )
    memmove(kn+st->keySz,data,st->dataSz);        /* add new key/data */
  spp->iused(slen + kdSz);
  keyInterior = 0;
  lastAccess.id = s;
  lastAccess.offset = sizeof(keyBlock) + kn-sn;
  return 1;
}

/***
 *  Db::IndexBinary::keyInsert - add unique key to database
 *
 *  parameters
 *      s      pageId     input        current id
 *  uses:
 *     iky     char *     input        assembled insertion key
 *
 *  returns 0 on success,
 *          errDuplicate if key already exists in database
 *          error code otherwise
 */

int Db::IndexBinary::
keyInsert(pageId s, pageId &t)
{
  char *kn;
/* mark no continued insertion, and check end of search */
/*  if not end, read current pageId, search for key */
/*  return error if found.  */
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,1,sbb,spp,sn) );
  int lkdSz = kdSz;
  pageId u = sbb->right_link();
  if( u >= 0 )
    lkdSz += sizeof(pageId);
  int slen = spp->iused();
  int keyCount = slen / lkdSz;
  int maxKeysInBlock = spp->iallocated() / lkdSz;
  int l = -1;
  int r = keyCount;

/* binary search block for key */
  while( (r - l) > 1 ) {
    int i = (l+r) >> 1;
    int k = i*lkdSz;
    if( sbb->right_link() >= 0 )
      k += sizeof(pageId);
    kn = sn + k;
    int n = compare(this->akey,kn);
    if( n == 0 ) {
      lastAccess.id = s;
      lastAccess.offset = sizeof(keyBlock) + k;
      Fail(errDuplicate);
    }
    if( n > 0 ) l = i; else r = i;
  }

/* not found in this pageId, continue search at lower levels. */
/*  process insertion if lower level splits ( t>=0 ).  */
  int i = sizeof(pageId);
  int k = r * lkdSz;
  if( u >= 0 ) {
    if( k < slen )
      u = readPageId(sn+k);
    if_ret( keyInsert(u, t) );
    if( t < 0 ) return 0;
    if( k < slen )
      writePageId(sn+k,t);
    else
      sbb->right_link(t);
    i = 0;
  }

/* if current pageId is not full, insert into this pageId. */
  if( keyCount < maxKeysInBlock ) {
    t = NIL;
    kn = sn + k;
    memmove(kn+lkdSz,kn,slen-k);
    spp->iused(slen + lkdSz);
    memmove(kn,&iky[i],lkdSz);
    setLastKey(s,u,k);                  /*  save insert loc */
    return 0;
  }

/* current pageId is full, split pageId and promote new parent key */
  keyBlock *tbb;  Page *tpp;  char *tn;
  if_err( blockAllocate(t,tbb,tpp,tn) );
/* split point is middle, or end if inserting consecutive on rhs */
  int promoted = maxKeysInBlock/2;
  if( cInsCount > 2 && s == st->rightHandSide )
    promoted = maxKeysInBlock-1;
  promoted *= lkdSz;
  int tlen = slen - promoted;
  if( st->rightHandSide == s ) st->rightHandSide = t;

  if( k <= promoted ) {                 /*  at or left of promoted key */
    if( k != promoted ) {               /*  not promoting current key */
      kn = sn+promoted-lkdSz;
      memmove(&tky[0],kn,lkdSz);         /*  save promoted key */
      kn = sn+k;
      memmove(kn+lkdSz,kn,promoted-lkdSz-k);
      memmove(kn,&iky[i],lkdSz);         /*  insert new key */
      memmove(&iky[i],&tky[0],lkdSz);    /*  promote key */
      setLastKey(s,u,k);                /*  save insert loc */
    }
    memmove(tn,sn+promoted,tlen);
  }
  else {                                /*  key is > center */
    kn = sn+promoted;
    memmove(&tky[0],kn,lkdSz);           /*  save promoted key */
    int j = k - promoted - lkdSz;
    memmove(tn,kn+=lkdSz,j);
    memmove(kn=tn+j,&iky[i],lkdSz);      /*  insert new key */
    setLastKey(t,u,j);                  /*  save insert loc */
    memmove(kn+=lkdSz,sn+k,slen-k);
    memmove(&iky[i],&tky[0],lkdSz);      /*  promote key */
  }

/* set newly split blocks half full, and set new links. */
  slen = promoted;
  spp->iused(slen);
  tpp->iused(tlen);
  tbb->right_link(sbb->right_link());
  sbb->right_link(readPageId(&iky[0]));
  writePageId(&iky[0],s);
/* if root is splitting, create new left */
  if( s == st->rootPageId ) {
    keyBlock *ubb;  Page *upp;  char *un;
    if_err( blockAllocate(u,ubb,upp,un) );
    memmove(un,sn,slen);
    upp->iused(slen);
    ubb->right_link(sbb->right_link());
    writePageId(&iky[0],u);
    k = st->keySz + st->dataSz + sizeof(pageId);
    memmove(sn,&iky[0],k);
    spp->iused(k);
    sbb->right_link(t);
    setLastKey(s,t,sizeof(pageId));
  }
/* t >= 0 indicates continued insertion, return success */
  return 0;
}

void Db::IndexBinary::
makeKey(char *cp,char *key,int l,char *recd,int n)
{
  writePageId(cp,NIL);  cp += sizeof(pageId);
  if( l > 0 ) { wr_key(key, cp,l);  cp += l; }
  if( n > 0 && recd ) memmove(cp,recd,n);
}

/***
 *  Db::Insert - interface to database unique key insertion
 *
 *  parameters
 * key         void *          input        key image
 * data        void *          input        recd value
 *
 *  returns 0 on success,
 *          errDuplicate if key already exists in database
 *          error code otherwise
 */

int Db::IndexBinary::
Insert(void *key, void *data)
{
  if_err( MakeRoot() );
  keyInterior = -1;
  int ret = 0;
  if( CHK cInsCount > 2 ) {                     // try the easy way
    ret = chkInsert(key,data);
    if_ret( ret );
  }
  if( !ret ) {                                  // try the hard way
    makeKey(&iky[0],this->akey=(char *)key,st->keySz,(char *)data,st->dataSz);
    pageId t = NIL;  lastAccess.id = NIL;
    if_ret( keyInsert(st->rootPageId, t) );
  }
  if( keyInterior > 0 ) lastAccess.id = NIL;
  chkLastInsert();
  ++st->count;
  return 0;
}

/***
 *  Db::keyBlockUnderflow -- balences/merges blocks after a deletion
 *
 *  parameters
 *      t      int           output       continuation flag
 *    lbb      keyBlock *    input        left sibling keyBlock
 *      p      pageId        input        parent keyBlock id
 *    pbb      keyBlock *    input        parent keyBlock
 *     pi      int           input        deletion key offset parent
 *
 *
 *  returns 0 on success, errorcode otherwise
 */

int Db::IndexBinary::
keyBlockUnderflow(int &t,keyBlock *lbb,pageId p,keyBlock *pbb,int pi)
{
  int i, k;
  char *kn;
  pageId l, r;
  keyBlock *rbb;
  int psiz = kdSz + sizeof(pageId);
/*
 *  if deletion was at right end of block
 *    back up one key otherwise use this key
 */
  char *pn = (char *)(pbb+1);
  Page *ppp = db->get_page(p);
  int plen = ppp->iused();
  if( pi >= plen ) {                            /* read lt side */
    r = pbb->right_link();
    rbb = lbb;
    pi -= psiz;
    l = readPageId(pn+pi);
    if_err( db->indexRead(l,1,lbb) );
  }
  else {                                        /* read rt side */
    l = readPageId(pn+pi);
    i = pi + psiz;
    r = i >= plen ? pbb->right_link() : readPageId(pn+i);
    if_err( db->indexRead(r,1,rbb) );
  }

  int lsz = lbb->right_link() >= 0 ? sizeof(pageId) : 0;
  int lkdSz = kdSz + lsz;
  char *ln = (char *)(lbb+1);
  Page *lpp = db->get_page(l);
  int llen = lpp->iused();
  char *rn = (char *)(rbb+1);
  Page *rpp = db->get_page(r);
  int rlen = rpp->iused();

/*
 * if both lt&rt blocks+parent key all fit in one block, merge them
 */
  int n = llen + rlen;
  if( n <= rpp->iallocated()-lkdSz ) {          /* merge */
    writePageId(pn+pi,lbb->right_link());       /* reset parent key left ptr */
    memmove(ln+llen,pn+pi+sizeof(pageId)-lsz,lkdSz);
    llen += lkdSz;
    memmove(ln+llen,rn,rlen);                    /* move right to left */
    llen += rlen;
    lbb->right_link(rbb->right_link());
    i = pi + psiz;
    memmove(pn+pi,pn+i,plen-i);                 /* remove parent key */
    plen -= psiz;
    if( plen == 0 && p == st->rootPageId ) {        /* totally mashed root */
      if( st->rightHandSide == r ) st->rightHandSide = p;
      if_err( blockFree(r) );
      memmove(pn,ln,llen);                       /* copy to parent page */
      pbb->right_link(lbb->right_link());
      ppp->iused(llen);
      if_err( blockFree(l) );
    }
    else {
      if( r != pbb->right_link() )              /* not rightLink */
        writePageId(pn+pi,l);                   /* must be next key */
      else
        pbb->right_link(l);
      if( st->rightHandSide == r ) st->rightHandSide = l;
      if_err( blockFree(r) );
      lpp->iused(llen);
      ppp->iused(plen);
    }
    lastAccess.id = NIL;
    return 0;                                   /* continue underflow */
  }

  int tsiz = n / 6;
  if( tsiz < lkdSz ) tsiz = lkdSz;              /* slosh threshold */
  if( plen > psiz && plen > tsiz ) t = 0;       /* discontinue underflow */
  if( (i=(n/=2)-llen) >= tsiz || !llen ) {      /* slosh left */
    writePageId(pn+pi,lbb->right_link());       /* reset parent key left ptr */
    k = pi+sizeof(pageId);
    memmove(kn=ln+llen,pn+k-lsz,lkdSz);          /* move parent to left */
    i = (i/lkdSz)-1;
    memmove(kn+=lkdSz,rn,i*=lkdSz);              /* migrate keys left */
    llen += i+lkdSz;  kn = rn+i;
    if( lbb->right_link() >=0 )
      lbb->right_link(readPageId(kn));
    writePageId(pn+pi,l);                       /* change parent key */
    memmove(pn+k,kn+lsz,kdSz);
    kn += lkdSz;  i += lkdSz;
    memmove(rn,kn,rlen-=i);                      /* migrate keys left */
  }
  else if( (i=n-rlen) >= tsiz || !rlen ) {      /* slosh right */
    i /= lkdSz; i *= lkdSz;
    memmove(kn=rn+i,rn,rlen);
    rlen += i;                                  /* migrate keys right */
    writePageId(pn+pi,lbb->right_link());
    k = pi+sizeof(pageId);
    memmove(kn-=lkdSz,pn+k-lsz,lkdSz);           /* move parent key */
    i -= lkdSz;  n = llen-i;
    memmove(rn,kn=ln+n,i);
    kn -= lkdSz;                                /* migrate keys right */
    if( lbb->right_link() >=0 )
      lbb->right_link(readPageId(kn));
    memmove(pn+k,kn+lsz,kdSz);                   /* change parent key */
    writePageId(pn+pi,l);
    llen = n - lkdSz;
  }
  else
    return 0;
  lastAccess.id = NIL;
  lpp->iused(llen);                             /* update lengths */
  rpp->iused(rlen);
  ppp->iused(plen);
  return 0;
}

/***
 *  Db::IndexBinary::keyDelete - remove unique key from database
 *
 *  parameters
 *      t      int        input/output check underflow flag
 *     kp      void *     input        key image to be removed
 *      s      pageId     input        current id
 *      p      pageId     input        parent id
 *    pbb      keyBlock * input        parent
 *     pi      int        input        deletion key offset parent
 *
 *  returns 0 on success,
 *          errNotFound if key does not exists in database
 *          error code otherwise
 */

int Db::IndexBinary::
keyDelete(int &t,void *kp,pageId s,pageId p,keyBlock *pbb,int pi)
{
  pageId u;
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,1,sbb,spp,sn) );
  int slen = spp->iused();
  t = 1;                                        /* check underflow */
  if( idf == 0 ) {                              /* not interior deletion */
    int lkdSz = kdSz;
    if( sbb->right_link() >= 0 )
      lkdSz += sizeof(pageId);
    int l = -1;
    int r = slen / lkdSz;
    while( (r - l) > 1 ) {                      /* binary search for key */
      int i = (l+r) >> 1;
      int k = i * lkdSz;
      if( sbb->right_link() >= 0 )
        k += sizeof(pageId);
      char *kn = sn + k;
      int n = compare(this->akey,kn);
      if( n == 0 ) {
        if( sbb->right_link() < 0 ) {           /* terminal key */
          slen -= lkdSz;
          memmove(kn,kn+lkdSz,slen-k);
          spp->iused(slen);                     /* delete key */
          lastAccess.id = s;                    /* lastAccess = key */
          lastAccess.offset = sizeof(keyBlock) + k;
        }
        else {                                  /* interior key */
          k -= sizeof(pageId);
          kn = sn + k;
          u = readPageId(kn);
          idf = 1;                              /* interior delete */
          if_ret( keyDelete(t,(void *)kn,u,s,sbb,k) );
        }
        goto xit;
      }
      if( n > 0 ) l = i; else r = i;
    }
    if( (u=sbb->right_link()) < 0 ) {           /* could not find it */
      t = 0;
      Fail(errNotFound);
    }
    if( (r *= lkdSz) < slen )
      u = readPageId(sn+r);
    if_ret( keyDelete(t,kp,u,s,sbb,r) );        /* continue search */
  }
  else {                                        /* interior deletion */
    if( (u=sbb->right_link()) < 0 ) {           /* equivalent exterior key */
      int i = slen - kdSz;
      char *kn = sn + i;                        /* translate to interior */
      memmove((char *)kp+sizeof(pageId),kn,kdSz);
      spp->iused(i);                            /* remove key */
    }
    else {                                      /* continue decending */
      if_ret( keyDelete(t,kp,u,s,sbb,slen) );
    }
  }
xit:
  if( t != 0 ) {                                /* underflow possible */
    if( !pbb )
      t = 0;                                    /* no parent, root */
    else
      if_err( keyBlockUnderflow(t,sbb,p,pbb,pi) );
  }
  return 0;
}

int Db::IndexBinary::
chkDelete(pgRef &loc, void *kp)
{
  int ret = 0;
  loc = lastDelete;
  ret = chkFind(loc, (char*)kp);                // try last delete
  if( !ret && lastFind.id != loc.id ) {
    loc = lastFind;
    ret = chkFind(loc, (char*)kp);              // try last find
  }
  if( !ret ) return 0;
  if( ret == errNotFound ) ret = 0;
  if_err( ret );
  pageId s = loc.id;
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,1,sbb,spp,sn) );
  int dlen = spp->iused() - kdSz;
  if( dlen < kdSz ) return 0;                   // at least 1 key must remain
  if( !ret ) return errNotFound;
  spp->iused(dlen);                             // delete
  int k = loc.offset - sizeof(keyBlock);
  if( dlen > k ) {
    char *kp = sn + k;
    memmove(kp,kp+kdSz,dlen-k);
  }
  return 1;
}

/***
 *  Db::IndexBinary::Delete - interface to remove unique key
 *
 *  parameters
 *    key      void *          input        key image to be removed
 *
 *  returns 0 on success,
 *          errNotFound key does not exists in database
 *          error code otherwise
 */

int Db::IndexBinary::
Delete(void *key)
{
  if( st->rootPageId == NIL ) Fail(errNotFound);
  this->akey = (char *)key;
  this->idf = 0;
  pageId r = st->rootPageId;
  int ret = 0;
  if( CHK cDelCount > 2 ) {                     // try the easy way
    pgRef loc;
    ret = chkDelete(loc, key);
    if( ret == errNotFound ) {                  // in exterior block
      r = loc.id;  ret = 0;
    }
  }
  if( !ret ) {                                  // try the hard way
    makeKey(&iky[0],this->akey=(char *)key,st->keySz,0,0);
    lastAccess.id = NIL;  int t = 1;
    (void)r; // use full search, r works but is not traditional
    if_fail( keyDelete(t,(void *)&iky[0],/*r*/st->rootPageId,0,0,0) );
    if_err( UnmakeRoot() );
  }
  chkLastDelete();
  --st->count;
  return 0;
}

/***
 *  Db::IndexBinary::keyFirst - access leftmost key in tree
 *
 *  parameters
 *      s      pageId     input        current id
 *
 *  returns 0 on success,
 *          errNotFound if index is empty
 *          error code otherwise
 */

int Db::IndexBinary::
keyFirst(pgRef &loc, pageId s)
{
  for(;;) {
    keyBlock *sbb;
    if_err( db->indexRead(s,0,sbb) );
    if( sbb->right_link() < 0 ) break;
    char *sn = (char *)(sbb+1);
    s = readPageId(sn);
  }

  loc.id = s;
  loc.offset = sizeof(keyBlock);
  return 0;
}

/***
 *  Db::IndexBinary::First -- interface to database access leftmost key in tree
 *
 *  parameters
 * rtnKey      void *          output       resulting key value
 * rtnData     void *          output       resulting recd value
 *
 * returns 0 on success
 *         errNotFound on not found,
 *         error code otherwise
 */

int Db::IndexBinary::
First(void *rtnKey,void *rtnData)
{
  if( st->rootPageId == NIL ) Fail(errNotFound);
  pgRef first;
  if_fail( keyFirst(first, st->rootPageId) );
  char *kp = 0;
  if_err( db->addrRead_(first,kp) );
  if( rtnKey && st->keySz > 0 )
    wr_key(kp, (char*)rtnKey,st->keySz);
  if( rtnData && st->dataSz > 0 )
    memmove(rtnData,kp+st->keySz,st->dataSz);
{ locked by(idxLk);
  lastNext = lastAccess = first; }
  return 0;
}

/***
 *  Db::IndexBinary::keyLast - access rightmost key in tree
 *
 *  parameters
 *      s      pageId     input        current id
 *
 *  returns 0 on success,
 *          errNotFound if index is empty
 *          error code otherwise
 */

int Db::IndexBinary::
keyLast(pgRef &loc, pageId s)
{
  for(;;) {
    keyBlock *sbb;
    if_err( db->indexRead(s,0,sbb) );
    pageId u = sbb->right_link();
    if( u < 0 ) break;
    s = u;
  }

  Page *spp = db->get_page(s);
  loc.id = s;
  int k = spp->iused() - kdSz;
  loc.offset = sizeof(keyBlock) + k;
  return 0;
}

/***
 *  Db::IndexBinary::Last -- interface to database access rightmost key in tree
 *
 *  parameters
 * rtnKey      void *          output       resulting key value
 * rtnData     void *          output       resulting recd value
 *
 * returns 0 on success
 *         errNotFound on not found,
 *         error code otherwise
 */

int Db::IndexBinary::
Last(void *rtnKey,void *rtnData)
{
  if( st->rootPageId == NIL ) Fail(errNotFound);
  pgRef last;
  if_fail( keyLast(last, st->rootPageId) );
  char *kp = 0;
  if_err( db->addrRead_(last,kp) );
  if( rtnKey && st->keySz > 0 )
    wr_key(kp, (char*)rtnKey,st->keySz);
  if( rtnData && st->dataSz > 0 )
    memmove(rtnData,kp+st->keySz,st->dataSz);
{ locked by(idxLk);
  lastNext = lastAccess = last; }
  return 0;
}

/***
 *  Db::IndexBinary::Modify -- interface to database record modify
 *
 *  parameters
 *  keyDef      index         input        key definition block
 *  key         void *          input        retreival key image
 *  recd        void *          input        new recd value
 *
 * returns 0 on success
 *         errNotFound on not found,
 *         error code otherwise
 */

int Db::IndexBinary::
Modify(void *key,void *recd)
{
  if_fail( Find(key,0) );
  char *bp = 0;
  if_err( db->addrWrite_(lastAccess,bp) );
  memmove(bp+st->keySz,recd,st->dataSz);
  return 0;
}

int Db::IndexBinary::
chkNext(pgRef &loc, char *&kp)
{
  pageId s = loc.id;
  if( s < 0 ) return 0;                         // must be valid
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,0,sbb,spp,sn) );
  if( sbb->right_link() >= 0 ) return 0;        // must be leaf
  int k = loc.offset - sizeof(keyBlock);
  if( k < 0 || k >= spp->iused() ) return 0;    // curr must be in block
  if( (k+=kdSz) >= spp->iused() ) return 0;     // next must be in block
  kp = sn + k;
  loc.offset = sizeof(keyBlock) + k;
  return 1;
}

int Db::IndexBinary::
keyNext(pgRef &loc, char *kp)
{
  if_fail( keyLocate(loc,st->rootPageId, keyGT,kp,compare) );
  return 0;
}

/***
 *  Db::IndexBinary::Next -- interface to sequential database key retreival
 *
 *  parameters
 * loc         pgRef &         input        last known retreival key
 * rtnKey      void *          output       resulting key value
 * rtnData     void *          output       resulting recd value
 *
 * returns 0 on success
 *         error code otherwise
 */

int Db::IndexBinary::
Next(pgRef &loc,void *rtnKey,void *rtnData)
{
  if( st->rootPageId == NIL ) Fail(errNotFound);
  char *kp;
  int ret = CHK chkNext(loc,kp);                // try the easy way
  if_ret( ret );
  if( !ret ) {
    char *ky = 0;
    switch( st->key_type ) {
    case ktyInd:
      ky = (char *)rtnKey;
      break;
    case ktyBin: case ktyDir:
      if_err( db->addrRead_(loc,ky) );
      break;
    }
    if_ret( keyNext(loc, ky) );                 // try the hard way
  }
  if_err( db->addrRead_(loc,kp) );
  if( rtnKey && st->keySz > 0 )
    wr_key(kp, (char*)rtnKey,st->keySz);
  if( rtnData && st->dataSz > 0 )
    memmove(rtnData,kp+st->keySz,st->dataSz);
{ locked by(idxLk);
  lastAccess = loc; }
  return 0;
}

void Db::IndexBinary::
init()
{
  keyInterior = 0;
  idf = 0;  akey = 0;
}

Db::IndexBinary::
IndexBinary(Db *zdb, int zidx, int ksz, int dsz, CmprFn cmpr)
 : IndexBase(zdb, idxBin, zidx, ksz, dsz)
{
  compare = cmpr;
  bst = new(st+1) IndexBinaryStorage(zdb->findCmprFn(compare));
  iky = new char[st->blockSize/2+1];
  tky = new char[st->blockSize/2+1];
  init();
}

Db::IndexBinary::
IndexBinary(Db *zdb, IndexBaseStorage *b, IndexBinaryStorage *d)
 : IndexBase(zdb, *b)
{
  bst = new(d) IndexBinaryStorage();
  compare = cmprFns[bst->cmprId];
  iky = new char[st->blockSize/2+1];
  tky = new char[st->blockSize/2+1];
  init();
}

Db::IndexBinary::
IndexBinary(IndexBase *ib, IndexBaseStorage *b, IndexBinaryStorage *d)
 : IndexBase(ib->db, *b)
{
  bst = new(d) IndexBinaryStorage();
  compare = cmprFns[bst->cmprId];
  init();
}

Db::IndexBinary::
~IndexBinary()
{
  delete [] iky;
  delete [] tky;
}


int Db::IndexString::
keyMap(pageId s, int(IndexBase::*fn)(pageId id))
{
  pageId r;
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,0,sbb,spp,sn) );
  unsigned char *lp = (unsigned char *)sn;
  unsigned char *rp = lp + spp->iused();
  if( (r=sbb->right_link()) >= 0 ) {
    while( lp < rp ) {
      pageId l = getPageId(lp);
      lp += st->dataSz;  ++lp;
      while( *lp++ != 0 );
      if_ret( keyMap(l,fn) );
    }
    if_ret( keyMap(r,fn) );
  }
  if_err( (this->*fn)(s) );
  return 0;
}

/***
 *  Db::IndexString::kpress -- compresses kp with prefix
 *
 *  parameters
 * kp     unsigned char *    input        key string to compress
 * lp     unsigned char *    input        left prefix
 * cp     unsigned char *    output       compressed string
 *
 * returns length of compressed string cp
 *   safe to kpress buffer in place over cp or lp
 */

int Db::IndexString::
kpress(unsigned char *kp, unsigned char *lp, unsigned char *cp)
{
  unsigned char ch;
  int n = 0, len = 0;
  while( *kp == *lp ) {
    ++n; ++lp; ++kp;
  }
  do {
    ch = *kp++;  *cp++ = n;
    ++len;       n = ch;
  } while( n != 0);
  *cp = 0;
  return ++len;
}

/*
 *  divide tbfr length n into sectors l and s with rlink r
 *   if i >= 0 then the insert point is i
 *   if l<0 allocate l
 *  promoted key in tky, return left sector
 */

int Db::IndexString::
split(int n, int i, pageId s, pageId &l, pageId r)
{
  unsigned char lky[keysz];
  unsigned char *bp = &tbfr[0];
  unsigned char *lp = bp;
  unsigned char *rp = bp + n;
  unsigned char *kp = lp;
  unsigned char *tp = 0;
  pageId t = NIL, u = NIL;
  keyBlock *lbb;  Page *lpp;  char *ln;
  if_err( l >= 0 ?
    db->indexRead(l,1,lbb,lpp,ln) :
    blockAllocate(l,lbb,lpp,ln) );
  int rlen = n;
  int blen = lpp->iallocated()-1 - st->dataSz;
  if( r >= 0 ) blen -= sizeof(pageId);
  if( n > blen ) n = blen;
/* split point is middle, or end if inserting consecutive on rhs */
  int promoted = n/2;
  if( i >= 0 && cInsCount > 2 && s == st->rightHandSide )
    promoted = n-1;
  unsigned char *mp = lp + promoted;
  // get promoted key in lky
  i = 0;  lky[0] = 0;
  while( lp < rp ) {
    // expand key to lky
    if( r >= 0 ) u = getPageId(lp);
    tp = lp;  lp += st->dataSz;
    for( i=*lp++; (lky[i++]=*lp++) != 0; );
    // stop copy if past mid pt and
    //   remaining bytes + expanded key bytes fit in block
    rlen = rp - lp;
    if( kp != &tbfr[0] && lp >= mp && rlen+i <= blen )
      break;
    // copy promoted data/key
    unsigned char *tkp = tky;
    umemmove(tkp, tp, st->dataSz);
    ustrcpy(tkp, lky);
    // save end of left block, move to next key
    kp = bp;  bp = lp;  t = u;
  }
  // store left block
  n = kp - &tbfr[0];
  // must be at least 3 keys in tbfr (left,parent,right)
  if( !n || bp >= rp ) Err(errCorrupt);
  memmove(ln,&tbfr[0],n);
  lpp->iused(n);
  lbb->right_link(t);
  // add first key in next block, order of moves important
  //  memmove may be move up since key may expand past split
  mp = &tbfr[0];
  if( r >= 0 ) putPageId(mp,u);
  umemmove(mp, tp, st->dataSz);  // data recd
  *mp++ = 0;                // first prefix is zero length
  memmove(mp+i, lp, rlen);  // rest of block
  memmove(mp, &lky[0], i);   // expanded key
  mp += rlen + i;
  int slen = mp - &tbfr[0];
/*
 *  if root is splitting, allocate new right sibling
 *  and set root to contain only new parent key.
 */
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,1,sbb,spp,sn) );
  if( s == st->rootPageId ) {
    keyBlock *rbb;  Page *rpp;  char *rn;
    if_err( blockAllocate(u,rbb,rpp,rn) );
    rpp->iused(slen);
    rbb->right_link(r);
    memmove(rn,&tbfr[0],slen);
    r = u;
    if( st->rightHandSide == s ) st->rightHandSide = r;
    mp = (unsigned char *)sn;
    putPageId(mp,l);
    umemmove(mp, kp=&tky[0], st->dataSz);
    kp += st->dataSz;
    for( *mp++=0; (*mp++=*kp++)!=0; );
    slen = mp - (unsigned char *)sn;
  }
  else
    memmove(sn, &tbfr[0], slen);
  sbb->right_link(r);
  spp->iused(slen);
  return 0;
}

int Db::IndexString::
chkInsert(void *key, void *data)
{
  unsigned char *ky = (unsigned char *)key;
  pageId s = lastInsert.id;
  if( s < 0 ) return 0;                         // must be valid
  int n = ustrcmp(&lastInsKey[0],ky);
  if( !n ) Fail(errDuplicate);
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,0,sbb,spp,sn) );
  if( sbb->right_link() >= 0 ) return 0;        // must be leaf
  int slen = spp->iused();
  int k = lastInsert.offset - sizeof(keyBlock);
  if( k < 0 || k >= slen ) return 0;            // must be inside block
  unsigned char *bp = (unsigned char *)sn;
  unsigned char *lp = bp;
  unsigned char *rp = bp + k;                   // beginning to current
  unsigned char *tp = bp + slen;
  unsigned char nky[keysz];  nky[0] = 0;
  if( n < 0 ) {                                 // past here
    ustrcpy(&nky[0],&lastInsKey[0]);
    lp = rp;  rp = tp;
  }
  else {                                        // before here
    n = ustrcmp(bp+st->dataSz+1,ky);
    if( !n ) Fail(errDuplicate);
    if( n > 0 ) return 0;                       // before first
    unsigned char rb;  rp += st->dataSz;        // move past curr data
    for( int i=*rp++; (rb=*rp++) == lastInsKey[i] && rb != 0; ++i );
    if( rb ) return 0;                          // must match curr
  }
  unsigned char lky[keysz];  lky[0] = 0;
  unsigned char *kp;  n = 0;
  while( (kp=lp) < rp ) {
    lp += st->dataSz;                           // scan next
    for( int i=*lp++; (nky[i]=*lp++) != 0; ++i );
    n = ustrcmp(ky,&nky[0]);
    if( !n ) Fail(errDuplicate);
    if( n < 0 ) break;                          // btwn last,next
    ustrcpy(&lky[0],&nky[0]);
  }
  if( lp >= tp && s != st->rightHandSide ) return 0;// not found, not rhs
  // recompress and compute storage demand
  int llen = kp - (unsigned char *)sn;          // left
  int lsz = kpress(ky, &lky[0], &lky[0]);       // lky = kpress new key
  int mlen = st->dataSz + lsz;
  int klen = mlen;                              // new key demand
  int nsz = 0;
  if( kp < rp ) {                               // if next key exists
    nsz = kpress(&nky[0], ky, &nky[0]);
    mlen += st->dataSz + nsz;                   // new+next key demand
  }
  int rlen = tp - lp;                           // right
  int blen = llen + mlen + rlen;                // left + demand + right

  if( blen > spp->iallocated() ) return 0;      // if insertion wont fit
  Page &spg = *spp;
  spg->set_flags(fl_wr);
  if( kp < rp ) {                               // insert not at end of block
    memmove(kp+mlen, lp, rlen);                 // move right up
    memmove(kp+klen, kp, st->dataSz);           // move next data up
    memmove(kp+klen+st->dataSz,&nky[0],nsz);    // kpress next key
  }
  k = kp - (unsigned char *)sn;
  lastAccess.id = lastInsert.id;
  lastAccess.offset = sizeof(keyBlock) + k;
  ustrcpy(&lastAccKey[0],ky);
  umemmove(kp,(unsigned char *)data,st->dataSz); // insert new key
  umemmove(kp,&lky[0],lsz);
  spp->iused(blen);
  return 1;
}

/*
 *      insert - add node to compressed b-tree.
 *      entry - tky - data/key to add
 *              s  - root of tree/subtree
 *              t  - NIL/discontinue or tky->left_link
 */
int Db::IndexString::
keyInsert(pageId &t, pageId s)
{
  unsigned char lky[keysz];         // last key
  unsigned char nky[keysz];         // next key
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,1,sbb,spp,sn) );
  t = NIL;  // set discontinue insertion
  pageId l = NIL;
  pageId r = sbb->right_link();
  lky[0] = 0;
  int n = spp->iused();
  unsigned char *lp = (unsigned char *)sn;      // rest of block
  unsigned char *rp = lp + n;                   // end of block
  unsigned char *kp = 0;                        // insertion point
  unsigned char *tkp = &tky[st->dataSz];            // tky = data/key

  while( (kp=lp) < rp ) {                       // search
    if( r >= 0 ) l = getPageId(lp);
    lp += st->dataSz;
    for( int i=*lp++; (nky[i]=*lp++) != 0; ++i );
    n = ustrcmp(&tkp[0], &nky[0]);
    if( n < 0 ) break;
    if( n == 0 ) Fail(errDuplicate);
    ustrcpy(lky, nky);
  }
  // if non-terminal block, continue search at lower levels
  // if lower level splits( t>=0 ), process insertion
  if( r >= 0 ) {
    if_ret( keyInsert(t, kp>=rp ? r : l) );
    if( t < 0 ) return 0;                       // discontinue
  }
  
  // recompress and compute storage demand
  int llen = kp - (unsigned char *)sn;          // left
  int lsz = kpress(tkp, &lky[0], &lky[0]);      // lky = kpress new key
  int dsz = st->dataSz;
  if( r >= 0 ) dsz += sizeof(pageId);           // link+data size
  int mlen = dsz + lsz;
  int klen = mlen;                              // new key demand
  int nsz = 0;
  if( kp < rp ) {                               // if next key exists
    nsz = kpress(&nky[0], &tkp[0], &nky[0]);
    mlen += dsz + nsz;                          // new+next key demand
  }
  int rlen = rp - lp;                           // right
  int blen = llen + mlen + rlen;                // left + demand + right

  if( blen <= spp->iallocated() ) {             // if insertion will fit
    if( kp < rp ) {                             // insert not at end of block
      memmove(kp+mlen, lp, rlen);               // move right up
      memmove(kp+klen, kp, dsz);                // move next link/data up
      memmove(kp+klen+dsz,&nky[0],nsz);         // kpress next key
    }
    if( r >= 0 ) putPageId(kp, t);
    if( lastAccess.id < 0 ) {                   // update lastAccess
      lastAccess.id = s;
      int k = kp - (unsigned char *)sn;
      lastAccess.offset = sizeof(keyBlock) + k;
      ustrcpy(&lastAccKey[0],&tkp[0]);
    }
    umemmove(kp,&tky[0],st->dataSz);            // insert new key
    memmove(kp,&lky[0],lsz);
    t = NIL;
    spp->iused(blen);
  }
  else {
    unsigned char *bp = &tbfr[0];               // overflowed, insert to tbfr
    umemmove(bp,(unsigned char *)sn,llen);
    if( r >= 0 ) putPageId(bp, t);
    umemmove(bp,&tky[0],st->dataSz);            // insert new key
    umemmove(bp,&lky[0],lsz);
    if( kp < rp ) {                             // insert not at end of block
      umemmove(bp,kp,dsz);
      umemmove(bp,&nky[0],nsz);                 // kpress next key
      umemmove(bp,lp,rlen);                     // copy rest of block
    }
    t = NIL;
    if_err( split(blen,llen,s,t,r) );           // split block, and continue
  }
  return 0;
}

int Db::IndexString::
Insert(void *key,void *data)
{
  if_err( MakeRoot() );
  int ret = 0;
  if( CHK cInsCount > 2 ) {                     // try the easy way
    ret = chkInsert(key,data);
    if_ret( ret );
  }
  if( !ret ) {                                  // try the hard way
    unsigned char *tp = &tky[0];
    umemmove(tp,(unsigned char *)data,st->dataSz);
    ustrcpy(tp,(unsigned char*)key);
    pageId t = NIL;  lastAccess.id = NIL;
    if_ret( keyInsert(t, st->rootPageId) );
  }
  ustrcpy(&lastInsKey[0],&lastAccKey[0]);
  chkLastInsert();
  ++st->count;
  return 0;
}


int Db::IndexString::
keyFirst(pageId s)
{
  keyBlock *sbb;
  if_err( db->indexRead(s,0,sbb) );
  char *sn = (char *)(sbb+1);

  while( sbb->right_link() >= 0 ) {
    s = readPageId(sn);
    if_err( db->indexRead(s,0,sbb) );
    sn = (char *)(sbb+1);
  }

  lastAccess.id = s;
  lastAccess.offset = sizeof(keyBlock);
  unsigned char *kp = (unsigned char *)sn;
  ustrcpy(&lastAccKey[0],kp+st->dataSz+1);
  return 0;
}

int Db::IndexString::
First(void *rtnKey,void *rtnData)
{
  if( st->rootPageId == NIL ) Fail(errNotFound);
  if_ret( keyFirst(st->rootPageId) );
  if( rtnKey )
    ustrcpy((unsigned char *)rtnKey,&lastAccKey[0]);
  if( rtnData ) {
    char *kp = 0;
    if_err( db->addrRead_(lastAccess,kp) );
    memmove(rtnData,kp,st->dataSz);
  }
  ustrcpy(&lastNxtKey[0],&lastAccKey[0]);
  lastNext = lastAccess;
  return 0;
}

int Db::IndexString::
keyLast(pageId s)
{
  keyBlock *sbb;
  if_err( db->indexRead(s,0,sbb) );
  pageId u;

  while( (u=sbb->right_link()) >= 0 ) {
    if_err( db->indexRead(s=u,0,sbb) );
  }

  unsigned char lky[keysz];
  Page *spp = db->get_page(s);
  char *sn = (char *)(sbb+1);
  unsigned char *lp = (unsigned char *)sn;
  unsigned char *rp = lp + spp->iused();
  unsigned char *kp = 0;
  lky[0] = 0;

  while( lp < rp ) {
    kp = lp;  lp += st->dataSz;
    for( int i=*lp++; (lky[i]=*lp++) != 0; ++i );
  }

  if( !kp ) Fail(errNotFound);
  int k = kp - (unsigned char *)sn;
  lastAccess.id = s;
  lastAccess.offset = sizeof(keyBlock) + k;
  ustrcpy(&lastAccKey[0],&lky[0]);
  return 0;
}

int Db::IndexString::
Last(void *rtnKey,void *rtnData)
{
  if( st->rootPageId == NIL ) Fail(errNotFound);
  if_ret( keyLast(st->rootPageId) );
  if( rtnKey )
    ustrcpy((unsigned char *)rtnKey,&lastAccKey[0]);
  if( rtnData ) {
    char *kp = 0;
    if_err( db->addrRead_(lastAccess,kp) );
    memmove(rtnData,kp,st->dataSz);
  }
  ustrcpy(&lastNxtKey[0],&lastAccKey[0]);
  lastNext = lastAccess;
  return 0;
}

int Db::IndexString::
chkFind(pgRef &loc, char *key, unsigned char *lkey, unsigned char *lkp)
{
  pageId s = loc.id;
  if( s < 0 ) return 0;                         // must be valid block
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,0,sbb,spp,sn) );
  if( sbb->right_link() >= 0 ) return 0;        // must be leaf
  int slen = spp->iused();
  int k = loc.offset - sizeof(keyBlock);
  if( k < 0 || k >= slen ) return 0;            // must be inside/end of block
  unsigned char *ky = (unsigned char *)key;
  unsigned char *bp = (unsigned char *)sn;
  unsigned char *kp = bp + k;
  unsigned char *rp = bp + slen;
  unsigned char *lp = kp;  kp += st->dataSz;
  unsigned char *ip = &lkey[*kp++];
  while( kp<rp && *kp!=0 && *ip==*kp ) { ++ip; ++kp; }
  if( *ip || *kp++ ) return 0;                  // must match curr
  int n = ustrcmp(&lkey[0], ky);
  if( !n && !lkp ) goto xit;                    // found here, and no last key
  unsigned char lky[keysz];
  ip = lkey;
  if( n > 0 ) {                                 // before here
    rp = lp;  lp = kp = bp;
    ip = lp + st->dataSz;
    if( *ip++ ) Err(errCorrupt);
    if( (n=ustrcmp(ip, ky)) > 0 ) return 0;     // before first
    if( !n ) { lky[0] = 0;  goto xit; }         // found here, first
  }
  ustrcpy(&lky[0], ip);
  while( kp < rp ) {
    lp = kp;  kp += st->dataSz;
    unsigned char nky[keysz];
    ustrcpy(&nky[0], &lky[0]);
    for( int i=*lp++; (nky[i]=*lp++) != 0; ++i );
    n = ustrcmp(ky, &nky[0]);
    if( !n ) goto xit;                          // found here
    if( n < 0 ) Fail(errNotFound);              // btwn prev,next
    ustrcpy(&lky[0], &nky[0]);
  }
  return 0;                                     // not in block
xit:
  if( lkp ) ustrcpy(lkp, &lky[0]);
  k = lp - bp;
  loc.offset = sizeof(keyBlock) + k;
  return 1;
}

int Db::IndexString::
keyFind(pgRef &loc,unsigned char *ky)
{
  unsigned char nky[keysz];

  for( pageId s=st->rootPageId; s>=0; ) {
    keyBlock *sbb;  Page *spp;  char *sn;
    if_err( db->indexRead(s,0,sbb,spp,sn) );
    unsigned char *lp = (unsigned char *)sn;
    unsigned char *rp = lp + spp->iused();
    pageId l = NIL;
    pageId r = sbb->right_link();
    while( lp < rp ) {
      if( r >= 0 ) l = getPageId(lp);
      unsigned char *kp = lp;
      lp += st->dataSz;
      for( int i=*lp++; (nky[i]=*lp++) != 0; ++i );
      int n = ustrcmp(ky, &nky[0]);
      if( n == 0 ) {
        loc.id = s;
        int k = kp - (unsigned char *)sn;
        loc.offset = sizeof(keyBlock) + k;
        return 0;
      }
      if( n < 0 ) { r = l; break; }
    }
    s = r;
  }
  Fail(errNotFound);
}


int Db::IndexString::
refFind(pgRef &loc, void *key)
{
  if( st->rootPageId == NIL ) Fail(errNotFound);
  int ret = 0;
{ locked by(idxLk);
  loc = lastFind;
  if( CHK cFindCount > 2 ) ret = 1; }
  if( ret ) {                   // try the easy way
    ret = chkFind(loc,(char *)key,&lastFndKey[0]);
    if_ret( ret );
  }
  if( !ret ) {                  // try the hard way
    if_fail( keyFind(loc, (unsigned char *)key) );
  }
{ locked by(idxLk);
  chkLastFind(loc);
  ustrcpy(&lastAccKey[0],(unsigned char *)key);
  ustrcpy(&lastFndKey[0],&lastAccKey[0]);
  ustrcpy(&lastNxtKey[0],&lastAccKey[0]); }
  return 0;
}

int Db::IndexString::
Find(void *key, void *rtnData)
{
  pgRef last;
  if_fail( refFind(last, key) );
  char *kp = 0;
  if( !db->addrRead_(last,kp) ) {
    if( rtnData )
      memmove(rtnData,kp,st->dataSz);
  }
  return 0;
}


/*
 *  remap sectors on underflow
 *    s - parent sector, t - pageId if overflowed
 *    k - parent key offset
 *    updates tky with parent data/key
 */

int Db::IndexString::
keyUnderflow(pageId s, pageId &t, int k)
{
  unsigned char lky[keysz], nky[keysz];
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,1,sbb,spp,sn) );
  unsigned char *lp = (unsigned char *)sn;
  unsigned char *rp = lp + spp->iused();
  unsigned char *kp = lp + k;
  unsigned char *mp = &tbfr[0];
  // mark continue underlow
  t = NIL;
  pageId r = sbb->right_link();
  lky[0] = nky[0] = 0;
  //  if underflow, copy to parent key offset
  while( lp < kp ) {
    putPageId(mp,getPageId(lp));
    umemmove(mp,lp,st->dataSz);  lp += st->dataSz;
    for( int i=*mp++=*lp++; (lky[i]=*mp++=*lp++) != 0; ++i );
  }
  // read l, key, r --  remove key from parent block
  unsigned char *tp = &tky[0];
  pageId l = getPageId(lp);
  umemmove(tp,lp,st->dataSz);  lp += st->dataSz;
  ustrcpy(tp,&lky[0]);
  for( int i=*lp++; (tp[i]=*lp++) != 0; ++i );
  if( lp < rp ) {
    putPageId(mp, r=getPageId(lp));
    umemmove(mp,lp,st->dataSz);  lp += st->dataSz;
    ustrcpy(&nky[0],tp);
    for( int i=*lp++; (nky[i]=*lp++) != 0; ++i );
    mp += kpress(&nky[0],&lky[0],mp);
    umemmove(mp,lp,rp-lp);
  }
  int n = mp-&tbfr[0];
  memmove(sn,&tbfr[0],n);
  spp->iused(n);
  // copy l to tbfr
  keyBlock *lbb;  Page *lpp;  char *ln;
  if_err( db->indexRead(l,1,lbb,lpp,ln) );
  lp = (unsigned char *)ln;
  rp = lp + lpp->iused();
  pageId m = lbb->right_link();
  mp = &tbfr[0];  nky[0] = 0;
  while( lp < rp ) {
    if( m >= 0 ) putPageId(mp,getPageId(lp));
    umemmove(mp,lp,st->dataSz);  lp += st->dataSz;
    for( int i=*mp++=*lp++; (nky[i]=*mp++=*lp++) != 0; ++i );
  }
  // parent key to tbfr
  if( m >= 0 ) putPageId(mp,m);
  umemmove(mp,&tky[0],st->dataSz);
  mp += kpress(tp,&nky[0],mp);
  // copy r to tbfr
  keyBlock *rbb;  Page *rpp;  char *rn;
  if_err( db->indexRead(r,1,rbb,rpp,rn) );
  lp = (unsigned char *)rn;
  rp = lp + rpp->iused();
  m = rbb->right_link();
  if( lp < rp ) {
    if( m >= 0 ) putPageId(mp,getPageId(lp));
    umemmove(mp,lp,st->dataSz);  lp += st->dataSz;
    for( int i=*lp++; (nky[i]=*lp++) != 0; ++i );
    mp += kpress(&nky[0],tp,mp);
    umemmove(mp,lp,rp-lp);
  }
  n = mp - &tbfr[0];
  if( n > rpp->iallocated() ) {
    // tbfr too big for r
    if_err( split(n, -1, r, l, m) );
    t = l;   // start overflow
  }
  else {
    if( s == st->rootPageId && !spp->iused() ) {
      // if root, delete r
      memmove(sn,&tbfr[0],n);
      spp->iused(n);
      sbb->right_link(m);
      if_err( blockFree(r) );
      st->rightHandSide = s;
    }
    else {
      // update r with tbfr
      memmove(rn,&tbfr[0],n);
      rpp->iused(n);
      rbb->right_link(m);
    }
    if_err( blockFree(l) );
  }
  return 0;
}

/*
 *  remap sectors on overflow
 *    s - parent sector, k - parent key offset, o - insertion offset
 *    t parent link on input,
 *    t == DDONE on output, end overflow 
 *    tky with parent data/key
 */
int Db::IndexString::
keyOverflow(pageId s, pageId &t, int k, int o)
{
  unsigned char lky[keysz], nky[keysz];
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,1,sbb,spp,sn) );
  unsigned char *lp = (unsigned char *)sn;
  unsigned char *rp = lp + spp->iused();
  unsigned char *kp = lp + k;
  unsigned char *mp = &tbfr[0];
  pageId r = sbb->right_link();
  lky[0] = nky[0] = 0;
  // copy parent up to parent key to tbfr
  while( lp < kp ) {
    putPageId(mp,getPageId(lp));
    umemmove(mp,lp,st->dataSz);  lp += st->dataSz;
    for( int i=*mp++=*lp++; (lky[i]=*mp++=*lp++) != 0; ++i );
  }
  // if at insertion point, add new key
  if( o <= k ) {
    putPageId(mp,t);
    unsigned char *tp = &tky[0];
    umemmove(mp,tp,st->dataSz);  tp += st->dataSz;
    mp += kpress(kp=tp, &lky[0], mp);
  }
  else
    kp = &lky[0];
  // copy parent key, if insertion - add tky, copy rest
  if( lp < rp ) {
    ustrcpy(&nky[0],kp);
    putPageId(mp,getPageId(lp));
    umemmove(mp,lp,st->dataSz);  lp += st->dataSz;
    for( int i=*lp++; (lky[i]=*lp++) != 0; ++i );
    mp += kpress(&lky[0],&nky[0],mp);
    if( o > k ) {
      putPageId(mp,t);
      unsigned char *tp = &tky[0];
      umemmove(mp,tp,st->dataSz);  tp += st->dataSz;
      mp += kpress(tp, &lky[0], mp);
      ustrcpy(&lky[0],tp);
    }
    if( lp < rp ) {
      putPageId(mp,getPageId(lp));
      umemmove(mp,lp,st->dataSz);  lp += st->dataSz;
      ustrcpy(&nky[0],&lky[0]);
      for( int i=*lp++; (lky[i]=*lp++) != 0; ++i );
      mp += kpress(&lky[0],&nky[0],mp);
    }
    umemmove(mp,lp,rp-lp);
  }
  int n = mp - &tbfr[0];
  // if overflow, split sector and promote new parent key
  if( n > spp->iallocated() ) {
    t = NIL;  lastAccess.id = NIL;
    if_ret( split(n, -1, s, t, r) );
  }
  else {
    t = DDONE;
    spp->iused(n);
    sbb->right_link(r);
    memmove(sn,&tbfr[0],n);
  }
  return 0;
}

int Db::IndexString::
keyRemap(pageId s, pageId &t, int k, int o)
{
  if( t < 0 ) {
    if_err( keyUnderflow(s,t,k) );
    o = k;
  }
  if( t >= 0 )
    if_err( keyOverflow(s,t,k,o) );
  return 0;
}

/*
 *  delete or replace key
 *   s  - starting sector, nky - replacement key if != 0
 *   t - returned state -2/done,-1/underflow,pageid/overflow
 */

int Db::IndexString::
keyDelete(pageId s, pageId &t)
{
  unsigned char lky[keysz], nky[keysz];
  unsigned char *ky = &akey[0];

  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,1,sbb,spp,sn) );
  unsigned char *bp = (unsigned char *)sn;
  unsigned char *lp = bp;
  unsigned char *rp = lp + spp->iused();
  unsigned char *kp = lp;
  unsigned char *mp = 0;
  int n = -1;
  pageId l = NIL;
  pageId r = sbb->right_link();
  lky[0] = nky[0] = 0;
  // mark delete done and search
  t = DDONE;
  while( (kp=lp) < rp ) {
    mp = lp;
    if( r >= 0 ) l = getPageId(lp);
    lp += st->dataSz;
    for( int i=*lp++; (nky[i]=*lp++) != 0; ++i );
    if( idf <= 0 && (n=ustrcmp(ky,&nky[0])) <= 0)
      break;
    ustrcpy(&lky[0],&nky[0]);
  }
  if( n != 0 ) {
    if( r < 0 ) {
      if( !idf ) Fail(errNotFound);
      // found greatest node less than ky, delete and return with underflow
      // deleted key must be on rt side of block to be next to interior key
      unsigned char *dp = &dky[0];
      umemmove(dp,mp,st->dataSz);
      ustrcpy(dp,&nky[0]);
      spp->iused(mp-bp);
      t = NIL;  // start underflow
    }
    else {
      // not found in block, try next level
      int status = keyDelete(kp<rp ? l : r,t);
      if_ret( status );
      if( t != DDONE )
        if_ret( keyRemap(s,t,mp-bp,kp-bp) );
    }
  }
  else if( r < 0 || idf < 0 ) {                 // found here
    int llen = kp - bp;
    int mlen = 0;
    int dsz = st->dataSz;
    if( r >= 0 ) dsz += sizeof(pageId);
    unsigned char *tp = &lky[0];
    int lsz = 0;
    if( idf < 0 ) {                             // translating data/key
      lsz = kpress(&dky[st->dataSz],tp,tp);     // kpress dky to lky
      mlen += dsz + lsz;
      tp = &dky[st->dataSz];
    }
    int nsz = 0;
    unsigned char *np = lp;
    lastAccKey[0] = 0;
    if( lp < rp ) {
      lp += dsz;                                // get next key
      for( int i=*lp++; (nky[i]=*lp++) != 0; ++i );
      if( r < 0 ) ustrcpy(&lastAccKey[0],&nky[0]);// new curr key
      nsz = kpress(&nky[0],tp,&nky[0]);
      mlen += dsz + nsz;
    }
    int rlen = rp - lp;
    int slen = llen + mlen + rlen;
    unsigned char *sp = &tbfr[0];
    mp = sp;
    if( (llen > 0 || rlen > 0) &&               // at least 3 data/keys
        slen <= spp->iallocated() ) {           // and fits in block
      sp = bp;  mp = kp;                        // edit in place
    }
    else
      umemmove(mp,bp,llen);                      // use tbfr, copy left
    if( np < rp ) {                             // next exists
      tp = mp + mlen;
      unsigned char *ip = mp;
      if( idf < 0 ) ip += dsz + lsz;
      if( sp == bp && tp > lp ) {
        memmove(tp,lp,rlen);                    // up copy/move right
        memmove(ip,np,dsz);  ip += dsz;         // next link/data/key
        memmove(ip,&nky[0],nsz);
      }
      else {
        memmove(ip,np,dsz);  ip += dsz;         // next link/data/key
        memmove(ip,&nky[0],nsz);
        if( tp != lp )
          memmove(tp,lp,rlen);                  // down copy/move right
      }
    }
    if( idf < 0 ) {                             // move exterior key
      if(r >= 0) putPageId(mp,getPageId(kp));
      umemmove(mp,&dky[0],st->dataSz);          // add dky data/key 
      umemmove(mp,&lky[0],lsz);
    }
    if( sp == bp ) {                            // in place
      t = DDONE;
      spp->iused(slen);
      if( r < 0 ) {
        lastAccess.id = s;                      // position to curr
        lastAccess.offset = sizeof(keyBlock) + llen;
        ustrcpy(&lastAccKey[0],ky);
      }
    }
    else {
      t = NIL;
      if( slen > spp->iallocated() ) {
        if_ret( split(slen, -1, s, t, r) );     // continue with overflow
      }
      else {
        memmove(sn,&tbfr[0],slen);               // continue with underflow
        spp->iused(slen);
      }
    }
  }
  else {
    // deleting nonterminal key, translate to greatest node less than ky
    idf = 1;
    int status = keyDelete(l,t);
    if_ret( status );
     if( t != DDONE )
      if_ret( keyRemap(s,t,mp-bp,kp-bp) );
  }
  return 0;
}


int Db::IndexString::
Delete(void *key)
{
  if( st->rootPageId == NIL ) Fail(errNotFound);
  unsigned char lky[keysz];  lky[0] = 0;
  pgRef *last = &lastDelete;
  unsigned char *lkey = &lastDelKey[0];
  int ret = 0;
  if( CHK cDelCount > 2 ) {                     // try the easy way
    if( lastFind.id >= 0 ) {                    // chk find/delete
      if( !ustrcmp((unsigned char *)key,&lastFndKey[0]) ) {
        last = &lastFind;  lkey = &lastFndKey[0];
      }
    }
    ret = chkFind(*last,(char *)key,lkey,&lky[0]);
    if_ret( ret );
    if( ret > 0 ) {
      ret = 0;
      pageId s = lastAccess.id;
      keyBlock *sbb;  Page *spp;  char *sn;
      if_err( db->indexRead(s,1,sbb,spp,sn) );
      unsigned char *bp = (unsigned char *)sn;
      int slen = spp->iused();
      int llen = lastAccess.offset - sizeof(keyBlock);
      unsigned char *kp = bp + llen;            // curr data/key
      unsigned char *lp = kp;
      unsigned char *rp = bp + slen;
      if( lp < rp ) {
        lp += st->dataSz;  ++lp;  while( *lp++ ); // skip curr
      }
      int mlen = 0;
      int nsz = 0;
      unsigned char *np = lp;
      unsigned char nky[keysz]; nky[0] = 0;
      if( lp < rp ) {
        lp += st->dataSz;                       // get next key
        ustrcpy(&nky[0],(unsigned char *)key);
        for( int i=*lp++; (nky[i]=*lp++) != 0; ++i );
        nsz = kpress(&nky[0],&lky[0],&lky[0]);
        mlen += st->dataSz + nsz;
      }
      int rlen = rp - lp;
      slen = llen + mlen + rlen;
      if( (llen > 0 || rlen > 0 ) &&            // at least 3 data/keys
          slen <= spp->iallocated() ) {         // and fits in block
        if( np < rp ) {                         // right exists
          unsigned char *tp = kp + mlen;
          umemmove(kp,np,st->dataSz);           // next data/key
          memmove(kp,&lky[0],nsz);
          if( tp != lp && rlen > 0 )
            memmove(tp,lp,rlen);                // move right down
        }
        spp->iused(slen);
        ustrcpy(&lastAccKey[0],&nky[0]);        // new curr key
        ret = 1;
      }
    }
  }
  if( !ret ) {                                  // try the hard way
    ustrcpy(&this->akey[0],(unsigned char*)key);
    lastAccess.id = NIL;
    pageId t = NIL;  idf = 0;
    if_ret( keyDelete(st->rootPageId,t) );
    if( idf ) {
      idf = -1;
      if_ret( keyDelete(st->rootPageId,t) );
    }
    if_err( UnmakeRoot() );
  }
  ustrcpy(&lastDelKey[0],&lastAccKey[0]);
  chkLastDelete();
  --st->count;
  return 0;
}

int Db::IndexString::
keyLocate(pgRef &last,pageId s, int &t, int op,
    unsigned char *ky, CmprFn cmpr, unsigned char *rky)
{
  unsigned char lky[keysz], nky[keysz];
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,0,sbb,spp,sn) );
  pageId l = NIL;
  pageId r = sbb->right_link();
  unsigned char *lp = (unsigned char *)sn;
  unsigned char *rp = lp + spp->iused();
  unsigned char *kp = 0, *mp = 0;
  lky[0] = nky[0] = 0;
  t = 0;

  while( (kp=lp) < rp ) {
    if( r >= 0 ) l = getPageId(lp);
    kp = lp;  lp += st->dataSz;
    for( int i=*lp++; (nky[i]=*lp++) != 0; ++i );
    int n = cmpr((char *)ky,(char *)&nky[0]);
    if( op <= keyEQ ? n <= 0 : n < 0 ) break;
    ustrcpy(&lky[0],&nky[0]);
    mp = kp;
  }

  if( r >= 0 ) {
    int status = keyLocate(last, (kp<rp ? l : r), t, op, ky, cmpr, rky);
    if( !t && status == errNotFound ) status = 0;
    if_ret( status );
  }

  if( !t ) {
    if( op == keyLT || op == keyGE ) {
      if( !mp ) Fail(errNotFound);
      kp = mp;
      ustrcpy(rky,&lky[0]);
    }
    else {
      if( kp >= rp ) Fail(errNotFound);
      ustrcpy(rky,&nky[0]);
    }
    last.id = s;
    int k = kp - (unsigned char *)sn;
    last.offset = sizeof(keyBlock) + k;
    t = 1;
  }
  return 0;
}


int Db::IndexString::
refLocate(pgRef &loc, int op,void *key,CmprFn cmpr, unsigned char *rkey)
{
  if( st->rootPageId == NIL ) Fail(errNotFound);
  if( op == keyEQ ) op = keyLE;
  if( !cmpr ) cmpr = cmprStr;
  int t = 0;
  if_fail( keyLocate(loc,st->rootPageId,t,op,(unsigned char*)key,cmpr, rkey) );
{ locked by(idxLk);
  chkLastFind(loc);
  ustrcpy(&lastAccKey[0], rkey);
  ustrcpy(&lastNxtKey[0],&lastAccKey[0]); }
  return 0;
}

int Db::IndexString::
Locate(int op,void *key,CmprFn cmpr,void *rtnKey,void *rtnData)
{
  pgRef last;  uint8_t rkey[keysz];
  if_fail( refLocate(last, op, key, cmpr, rkey) );
  if( rtnKey )
    ustrcpy((unsigned char *)rtnKey, rkey);
  if( rtnData ) {
    char *kp = 0;
    if_err( db->addrRead_(last,kp) );
    memmove(rtnData,kp,st->dataSz);
  }
  return 0;
}


int Db::IndexString::
Modify(void *key,void *recd)
{
  if_fail( Find(key,0) );
  char *bp = 0;
  if_err( db->addrWrite_(lastAccess,bp) );
  memmove(bp,recd,st->dataSz);
  return 0;
}

int Db::IndexString::
keyNext(pgRef &loc, unsigned char *rky)
{
  unsigned char lky[keysz];  lky[0] = 0;
  pageId s = loc.id;
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,0,sbb,spp,sn) );
  unsigned char *bp = (unsigned char *)sn;
  int k = loc.offset - sizeof(keyBlock);
  unsigned char *lp = bp;
  unsigned char *kp = bp + k;
  unsigned char *rp = bp + spp->iused();
  if( kp >= rp ) Err(errInvalid);
  pageId r = sbb->right_link();
  if( &loc != &lastNext ) {                     // find last key
    while( lp <= kp ) {                         // until past last
      if( r >= 0 ) lp += sizeof(pageId);
      bp = lp;  lp += st->dataSz;
      for( int i=*lp++; (lky[i]=*lp++) != 0; ++i );
    }
  }
  else {
    ustrcpy(&lky[0],&lastNxtKey[0]);
    lp = kp;  lp += st->dataSz;                 // verify lastNext
    unsigned char *ip = &lky[*lp++];
    while( lp<rp && *lp!=0 && *ip==*lp ) { ++ip; ++lp; }
    if( *ip || *lp++ ) Fail(errInvalid);        // bad lastNxtKey
  }
  if( r < 0 && lp < rp ) {                      // exterior and more data
    ustrcpy(rky, lky);
    bp = lp;  lp += st->dataSz;
    for( int i=*lp++; lp<rp && (rky[i]=*lp) != 0; ++i,++lp );
    if( *lp ) Err(errCorrupt);
    loc.offset = (bp-(unsigned char *)sn) + sizeof(keyBlock);
    return 0;
  }
  int t = 0;
  if_fail( keyLocate(loc,st->rootPageId,t,keyGT,lky,cmprStr, rky) );
  return 0;
}

int Db::IndexString::
Next(pgRef &loc,void *rtnKey,void *rtnData)
{
  if( st->rootPageId == NIL ) Fail(errNotFound);
  unsigned char rky[keysz];
  if_ret( keyNext(loc, rky) );
{ locked by(idxLk);
  ustrcpy(&lastAccKey[0], rky);
  if( rtnKey )
    ustrcpy((unsigned char *)rtnKey,&lastAccKey[0]);
  if( rtnData ) {
    char *kp = 0;
    if_err( db->addrRead_(loc,kp) );
    memmove(rtnData,kp,st->dataSz);
  }
  if( &loc == &lastNext )
    ustrcpy(&lastNxtKey[0],&lastAccKey[0]);
  lastAccess = lastNext = loc; }
  return 0;
}

void Db::IndexString::
init()
{
}

Db::IndexString::
IndexString(Db *zdb, int zidx, int dsz)
 : IndexBase(zdb, idxStr, zidx, 0, dsz)
{
  sst = new(st+1) IndexStringStorage();
  dky = new unsigned char[st->dataSz+keysz+1];
  tky = new unsigned char[st->dataSz+keysz+1];
  tbfr = new unsigned char[3*st->blockSize];
  init();
}

Db::IndexString::
IndexString(Db *zdb, IndexBaseStorage *b, IndexStringStorage *d)
 : IndexBase(zdb, *b)
{
  sst = d;
  dky = new unsigned char[st->dataSz+keysz+1];
  tky = new unsigned char[st->dataSz+keysz+1];
  tbfr = new unsigned char[3*st->blockSize];
  init();
}

Db::IndexString::
IndexString(IndexBase *ib, IndexBaseStorage *b, IndexStringStorage *d)
 : IndexBase(ib->db, *b)
{
  sst = new(d) IndexStringStorage();
  init();
}

Db::IndexString::
~IndexString()
{
  delete [] tbfr;
  delete [] tky;
  delete [] dky;
}



/***
 *  Db::IndexBase::Clear - clear index
 *
 *  parameters: none
 *
 *  returns 0 on success,
 *          error code otherwise
 */

int Db::IndexBase::
Clear()
{
  while( st->freeBlocks >= 0 ) {
    keyBlock *kbb;  pgRef loc;
    pageId id = st->freeBlocks;
    loc.id = id; loc.offset = 0;
    if_err( db->addrWrite_(loc,kbb) );
    st->freeBlocks = kbb->right_link();
    blockRelease(id);
  }
  if( st->rootPageId >= 0 ) {
    if_err( keyMap(st->rootPageId, &Db::IndexBase::blockRelease) );
    st->rootPageId = st->rightHandSide = NIL;
  }
  init(db);
  return 0;
}

int Db::IndexBase::
MakeRoot()
{
  if( st->rootPageId == NIL ) {
    pageId u;  keyBlock *ubb;  Page *upp;  char *un;
    if_err( blockAllocate(u,ubb,upp,un) );
    upp->iused(0);
    ubb->right_link(NIL);
    st->rootPageId = st->rightHandSide = u;
  }
  return 0;
}

int Db::IndexBase::
UnmakeRoot()
{
  pageId u = st->rootPageId;
  Page *upp = db->get_page(u);
  if( !upp->iused() ) {
    if_err( blockFree(u) );
    st->rootPageId = st->rightHandSide = NIL;
  }
  return 0;
}

void Db::IndexBase::
chkLastReset()
{
  lastOp = opFind;
  lastAccess.id = lastDelete.id = lastInsert.id =
    lastFind.id = lastNext.id = NIL;
  lastAccess.offset = lastDelete.offset = lastInsert.offset =
    lastFind.offset = lastNext.offset = 0;
  cFindCount = cDelCount = cInsCount = 0;
}

void Db::IndexBase::
chkLastInsert()
{
  if( lastAccess.id >= 0 && lastAccess.id == lastInsert.id )
    ++cInsCount;
  else
    cInsCount = 0;
  lastInsert = lastAccess;
  cDelCount = 0;  lastDelete.id = NIL;
  cFindCount = 0; lastFind.id = NIL;
  lastOp = opInsert; lastNext.id = NIL;
}

void Db::IndexBase::
chkLastDelete()
{
  if( lastAccess.id >= 0 && lastAccess.id == lastDelete.id )
    ++cDelCount;
  else
    cDelCount = 0;
  lastDelete = lastAccess;
  cInsCount = 0;  lastInsert.id = NIL;
  cFindCount = 0; lastFind.id = NIL;
  lastOp = opDelete;  lastNext.id = NIL;
}

void Db::IndexBase::
chkLastFind(pgRef &last)
{
  if( last.id >= 0 && last.id == lastFind.id )
    ++cFindCount;
  else
    cFindCount = 0;
  lastAccess = last;
  lastFind = last;
  lastNext = last;
  lastOp = opFind;
}

Db::IndexBaseType::
IndexBaseType(int typ)
{
  magic = idx_magic;
  memset(&name[0],0,sizeof(name));
  type = typ;
}

int Db::
defaultBlockSizes[] = {
  0,                       // idxNil
  defaultBinaryBlockSize,  // idxBin
  defaultStringBlockSize,  // idxStr
};
  
Db::IndexBaseRecd::
IndexBaseRecd(int typ, int zidx, int ksz, int dsz)
{
  idx = zidx;
  keySz = ksz;
  dataSz = dsz;
  rootPageId = NIL;
  rightHandSide = NIL;
  freeBlocks = NIL;
  count = 0;  pad1 = 0;
  blockSize = defaultBlockSizes[typ];
}

Db::IndexBaseStorage::
IndexBaseStorage(int typ, int zidx, int ksz, int dsz)
{
  new((IndexTypeInfo *)this) IndexBaseType(typ);
  new((IndexRecdInfo *)this) IndexBaseRecd(typ, zidx, ksz, dsz);
}

void Db::IndexBase::
init(Db *zdb)
{
  db = zdb;
  kdSz = st->keySz + st->dataSz;
  lastAccess.id = lastDelete.id = lastInsert.id =
    lastFind.id = lastNext.id = NIL;
  lastAccess.offset = lastDelete.offset = lastInsert.offset =
    lastFind.offset = lastNext.offset = 0;
  cFindCount = cDelCount = cInsCount = 0;
}

Db::IndexBase::
IndexBase(Db *zdb, IndexBaseStorage &d)
{
  st = &d;
  init(zdb);
}

Db::IndexBase::
IndexBase(Db *db, int typ, int zidx, int ksz, int dsz)
{
  st = new(&db->index_info[zidx]) IndexBaseStorage(typ, zidx, ksz, dsz);
  init(db);
}

Db::IndexBase::
~IndexBase()
{
}



/*** int Db::objectHeapInsert(int sz,int pg,int off)
 *
 * insert a free space record into the entity heap
 *
 * Input
 *   sz     int      record size
 *   id     int      record id
 *  offset  int      record offset
 *
 * returns zero on success, error code on failure
 */

int Db::
objectHeapInsert(int sz,int id,int offset,AllocCache &cache)
{
  freeSpaceRecord free;
  free.size = sz;  free.id = id;  free.offset = offset;
  if_err( freeSpaceIndex->Insert(&free,0) );
  addrSpaceRecord addr;
  addr.id = id;  addr.offset = offset;  addr.size = sz;
  if_err( addrSpaceIndex->Insert(&addr,0) );
  return 0;
}

/*** int Db::objectHeapDelete(int sz,int pg,int off)
 *
 * delete a free space record from the entity heap
 *
 * Input
 *   sz     int      record size
 *   id     int      record id
 *  offset  int      record offset
 *
 * returns zero on success, error code on failure
 */

int Db::
objectHeapDelete(int sz,int id,int offset,AllocCache &cache)
{
  freeSpaceRecord free;
  free.size = sz;  free.id = id;  free.offset = offset;
  if_err( freeSpaceIndex->Delete(&free) );
  addrSpaceRecord addr;
  addr.id = id;  addr.offset = offset;  addr.size = sz;
  if_err( addrSpaceIndex->Delete(&addr) );
  return 0;
}

/*** int Db::pgRefGet(int &size, pgRef &loc, AllocCache &cache)
 *
 *  find existing persistent free storage.
 *
 *  parameters
 *   size    int        input/output requested/allocated size
 *   loc     pgRef &    output       locator returned for new storage
 *   cache   AllocCache& output      allocator cache to operate
 *
 * returns: zero on success, error code (< 0) on failure, (> 0) no avail
 */

int Db::
pgRefGet(int &size, pgRef &loc, AllocCache &cache)
{
  freeSpaceRecord look, find;
  look.size = size; look.id = 0; look.offset = 0;
  int status = freeSpaceIndex->Locate(keyGE, &look, 0, &find, 0);
  if( status == errNotFound ) return 1;
  if_err( status );
  // if record is at offset 0, need extra space for prefix
  while( !find.offset && look.size+(int)sizeof(pagePrefix) > find.size ) {
    status = freeSpaceIndex->Next(&find, 0);
    if( status == errNotFound ) return 1;
    if_err( status );
  }
  if_err( objectHeapDelete(find.size,find.id,find.offset,cache) );
  loc.id = find.id;
  loc.offset = find.offset ? find.offset : sizeof(pagePrefix);
  Page &pg = *get_page(loc.id);
  unsigned int ofs = loc.offset + look.size;
  if( ofs > pg->used ) pg->used = ofs;
  int sz = find.offset+find.size - ofs;
  if( sz >= min_heap_allocation ) {
    //if_err( objectHeapInsert(sz,find.id,ofs,cache) );
    if_err( cache.Load(this, find.id, ofs, sz) );
  }
  else
    size = look.size + sz;
  return 0;
}

/*** int Db::pgRefNew(int &size, pgRef &loc, AllocCache &cache)
 *
 *  allocate new persistent free storage.
 *
 *  parameters
 *   size    int        input/output requested/allocated size
 *   loc     pgRef &    output       locator returned for new storage
 *   cache   AllocCache& output      allocator cache to operate
 *
 * returns: zero on success, error code.
 */

int Db::
pgRefNew(int &size, pgRef &loc, AllocCache &cache)
{
  pageId id = new_page();
  if_err( id );
  unsigned int sz = size + sizeof(pagePrefix);
  sz = entityPages(sz) * entityPageSize;
  Page &pg = *get_page(id);
  if( pg->allocated != sz ) {
    pageDealloc(pg);
    pg->allocated = sz;
  }
  pg->used = 0;
  loc.id = id;
  loc.offset = sizeof(pagePrefix);
  int used = loc.offset + size;
  int free = pg->allocated - used;
  if( free >= min_heap_allocation ) {
    //if_err( objectHeapInsert(free,id,used,cache) );
    if_err( cache.Load(this, id, used, free) );
  }
  else
    size = pg->allocated - loc.offset;
  return 0;
}

/*** int Db::pgRefAllocate(int &size, pgRef &loc)
 *
 *  find persistent free storage. existing/new
 *    does not allocate virtual memory storage
 *
 *  parameters
 *   size    int        input/output requested/allocated size
 *   loc     pgRef &    output       locator returned for new storage
 *   cache   AllocCache& output      allocator cache to operate
 *
 * returns: zero on success, error code (< 0) on failure, (> 0) no avail
 */

int Db::
pgRefAllocate(int &size, pgRef &loc, AllocCache &cache)
{
  int status = pgRefGet(size, loc, cache);
  if_err( status );
  if( status )
    if_err( pgRefNew(size, loc, cache) );
  return 0;
}

/*** int Db::objectAllocate(int typ, int &size, pgRef &loc)
 *
 *  find persistent free storage. existing/new
 *    does allocate virtual memory storage
 *    inits pagePrefix, inits allocPrefix
 *
 *  parameters
 *   type    int        input        entity type id
 *   size    int        input/output requested/allocated size
 *   loc     pgRef &    output       locator returned for new storage
 *   cache   AllocCache& output      allocator cache to operate
 *
 * returns: zero on success, error code (< 0) on failure, (> 0) no avail
 */

int Db::
objectAllocate(int typ, int &size, pgRef &loc, AllocCache &cache)
{
  int status = cache.Get(this, size, loc);
  if_err( status );
  if( !status ) {
    if_err( pgRefAllocate(size, loc, cache) );
    Page &pg = *get_page(loc.id);
    if( !pg->used ) {
      pagePrefix *bpp;  pgRef ref;
      ref.id = loc.id;  ref.offset = 0;
      if_err( addrWrite_(ref,bpp) );
      bpp->magic = page_magic;
      bpp->type = pg_entity + typ;
      pg->type = bpp->type;
    }
    unsigned int sz = loc.offset + size;
    if( pg->used < sz )
      pg->used = sz;
  }
  return 0; 
}

/*** int Db::objectFree(pgRef &loc)
 *
 * Purpose: Immediate release of entity memory
 *
 * Input
 *   loc   pgRef &    Page/offset
 *
 * returns zero on success, error code on failure
 */

int Db::objectFree(pgRef &loc,AllocCache &cache)
{
  allocPrefix *mp;
  if_err( addrRead_(loc,mp) );
  if( mp->magic != entity_magic ) Err(errBadMagic);
  addrSpaceRecord addr, prev, next;
  /* get prev, next addrSpace free heap items near this addr */
  addr.size = mp->size;
  addr.id = loc.id;
  addr.offset = loc.offset;
  int status = addrSpaceIndex->Locate(keyLT,&addr,0,&prev,0);
  if( status == errNotFound ) {
    prev.id = NIL;
    status = addrSpaceIndex->Locate(keyGT,&addr,0,&next,0);
  }
  else if( !status )
    status = addrSpaceIndex->Next(&next,0);
  if( status == errNotFound ) {
    next.id = NIL;
    status = 0;
  }
  if_err( status );
  /* merge with prev if possible */
  if( prev.id == addr.id &&
    prev.offset + prev.size == addr.offset ) {
    if_err( objectHeapDelete(prev.size,prev.id,prev.offset,cache) );
    addr.offset = prev.offset;
    addr.size += prev.size;
  }
  /* merge with next if possible */
  if( addr.id == next.id &&
      addr.offset + addr.size == next.offset ) {
    if_err( objectHeapDelete(next.size,next.id,next.offset,cache) );
    addr.size += next.size;
  }
  /* reduce used block bytes if possible */
  Page &pg = *get_page(loc.id);
  if( pg->used <= addr.offset + addr.size )
    pg->used = addr.offset;
  // if only page prefix, release page, otherwise save new fragment
  if( pg->used == sizeof(pagePrefix) )
    pg.release();
  else
    if_err( objectHeapInsert(addr.size,addr.id,addr.offset,cache) );
  return 0;
}

/*** int Db::blockAllocate(pageId *pid, keyBlock *&pkbb)
 *
 *  allocate persistent storage.
 *
 * Output
 *   pageId &     pid       page allocated
 *   keyBlock *&  pkbb      keyblock* pointer
 */

int Db::IndexBase::
blockAllocate(pageId &pid, keyBlock *&pkbb)
{
  locked by(db->db_info->blkAlLk);
  pageId id;  Page *kpp;
  pgRef loc;  keyBlock *kbb;
  if( st->freeBlocks != NIL ) {
    pgRef loc;
    id = st->freeBlocks;
    loc.id = id; loc.offset = 0;
    if_err( db->addrWrite_(loc,kbb) );
    st->freeBlocks = kbb->right_link();
    Page &kpg = *db->get_page(id);
    if( kpg->allocated != st->blockSize ) {
      db->pageDealloc(kpg);
      kpg->allocated = st->blockSize;
      if_err( db->addrWrite_(loc, kbb) );
    }
    kpp = &kpg;
  }
  else {
    id = db->new_page();
    if_err( id );
    loc.id = id;  loc.offset = 0;
    Page &kpg = *db->get_page(id);
    kpg->allocated = st->blockSize;
    if_err( db->addrWrite_(loc, kbb) );
    kpp = &kpg;
  }
  kbb->magic = page_magic;
  kbb->used = 0;
  kbb->type = pg_index + st->idx;
  kpp->iused(0);
  Page &kpg = *kpp;
  kpg->type = kbb->type;
  pkbb = kbb;
  pid = id;
  return 0;
}

/*** int Db::blockFree(pageId pid)
 *
 * Purpose: delayed release database memory
 *
 * Input
 *   pid        int     input     Page
 *
 * returns zero on success, error code on failure
 */

int Db::IndexBase::
blockFree(pageId pid)
{
  locked by(db->db_info->blkAlLk);
  keyBlock *kbb = 0;
  pageId id = st->freeBlocks;  pgRef loc;
  loc.id = NIL;  loc.offset = 0;
#if 0
  // use sorted order
  while( id >= 0 && id < pid ) {
    loc.id = id;
    if_err( db->addrRead_(loc,kbb) );
    id = kbb->right_link();
  }
#endif
  if( loc.id >= 0 ) {
    if_err( db->addrWrite_(loc,kbb) );
    kbb->right_link(pid);
  }
  else
    st->freeBlocks = pid;
  loc.id = pid;
  if_err( db->addrWrite_(loc,kbb) );
  kbb->right_link(id);
  Page *kpp = db->get_page(pid);
  kpp->iused(0);
  return 0;
}

int Db::IndexBase::
blockRelease(pageId pid)
{
  Page *pp = db->get_page(pid);
  return pp->release();
}

int Db::IndexBase::
blockLoad(pageId pid)
{
  pgRef loc;  char *op = 0;
  loc.id = pid;  loc.offset = 0;
  return db->addrRead(loc, op);
}

/*** int Db::deleteFreeBlock()
 *
 * Purpose: release database memory/storage
 *
 * returns zero on success, error code on failure
 */

int Db::IndexBase::
deleteFreeBlocks()
{
  pageId id;
  while( (id=st->freeBlocks) != NIL ) {
    keyBlock *kbb;  pgRef loc;
    loc.id = id;  loc.offset = 0;
    if_err( db->addrRead_(loc,kbb) );
    st->freeBlocks = kbb->right_link();
    Page &pg = *db->get_Page(id);
    if_err( db->storeFree(pg->wr_allocated, pg->wr_io_addr) );
    pg->wr_allocated = 0;  pg->wr_io_addr = NIL;
    if_err( db->storeFree(pg->io_allocated, pg->io_addr) );
    pg->io_allocated = 0;  pg->io_addr = NIL;
    db->free_page(id);
  }
  return 0;
}

/*** int Db::storeInsert(int sz, ioAddr io_addr)
 *
 * insert a storage record into the storage heap
 *
 * Input
 *   sz      int      blob size
 *   io_addr ioAddr   blob addr
 *
 * returns zero on success, error code on failure
 */

int Db::
storeInsert(long sz, ioAddr io_addr)
{
//dmsg(-1, "storeInsert %08lx/%012lx\n",sz,io_addr);
  freeStoreRecord free;
  free.size = sz;  free.io_addr = io_addr;
  if_err( freeStoreIndex->Insert(&free,0) );
//fchk();
  addrStoreRecord addr;
  addr.io_addr = io_addr;  addr.size = sz;
  if_err( addrStoreIndex->Insert(&addr,0) );
//achk();
  return 0;
}

/*** int Db::storeDelete(int sz, ioAddr io_addr)
 *
 * delete a storage record from the storage heap
 *
 * Input
 *   sz      int      blob size
 *   io_addr ioAddr   blob addr
 *
 * returns zero on success, error code on failure
 */

int Db::
storeDelete(long sz, ioAddr io_addr)
{
//dmsg(-1, "storeDelete %08lx/%012lx\n",sz,io_addr);
  freeStoreRecord free;
  free.size = sz;  free.io_addr = io_addr;
  if_err( freeStoreIndex->Delete(&free) );
//fchk();
  addrStoreRecord addr;
  addr.io_addr = io_addr;  addr.size = sz;
  if_err( addrStoreIndex->Delete(&addr) );
//achk();
  return 0;
}

/*** int Db::storeGet(int &size, ioAddr &io_addr)
 *
 * allocate storage record from the storage heap
 *
 *   size    int        input/output requested/allocated blob size
 *   io_addr ioAddr     output       blob addr
 *
 * returns: zero on success, error code (< 0) on failure, (> 0) no avail
 */

int Db::
storeGet(int &size, ioAddr &io_addr)
{
  freeStoreRecord look, find;
  look.size = size;  look.io_addr = 0;
  int status = freeStoreIndex->Locate(keyGE, &look,0, &find,0);
  if( status == errNotFound ) return 1;
  if_err( status );
  if_err( storeDelete(find.size,find.io_addr) );
  io_addr = find.io_addr;
  long sz = find.size - look.size;
  if( sz >= min_heap_allocation ) {
    /* put back extra */
    find.size = sz;  find.io_addr += look.size;
    if_err( storeInsert(find.size,find.io_addr) );
  }
  else
    look.size = find.size;
  size = look.size;
  return 0;
}

/*** int Db::storeNew(int &size, ioAddr &io_addr)
 *
 * allocate storage by extending file
 *
 *   size    int        input/output requested/allocated blob size
 *   io_addr ioAddr     output       blob addr
 *
 * returns: zero on success, error code (< 0) on failure, (> 0) no avail
 */

int Db::
storeNew(int &size, ioAddr &io_addr)
{
  if_err( seek_data(root_info->file_size) );
  io_addr = root_info->file_size;
  size = storeBlocks(size) * storeBlockSize;
  /* make sure file storage exists */
  if_err( write_padding(root_info->file_size + size) ); 
  return 0;
}

/*** int Db::storeAllocate(int &size, ioAddr &io_addr)
 *
 *  find persistent free storage. existing/new
 *    does not allocate virtual memory storage
 *
 *  parameters
 *   size    int        input/output requested/allocated size
 *   io_addr ioAddr &   output       returned storage address
 *
 * returns: zero on success, error code (< 0) on failure, (> 0) no avail
 */

int Db::
storeAllocate(int &size, ioAddr &io_addr)
{
  int status = storeGet(size, io_addr);
  if( status > 0 )
    status = storeNew(size, io_addr);
  if_err( status );
  return 0;
}

/*** int Db::storeFree(int size, ioAddr io_addr)
 *
 * Immediate release of store heap
 *
 *   size    int        input        blob size
 *   io_addr ioAddr     input        blob addr
 *
 * returns zero on success, error code on failure
 */

int Db::
storeFree(int size, ioAddr io_addr)
{
  if( io_addr < 0 || size == 0 ) return 0;
  /* get prev, next addrStore heap items near this io_addr */
  addrStoreRecord addr, prev, next;
  addr.io_addr = io_addr;  addr.size = size;
  int status = addrStoreIndex->Locate(keyLT,&addr,0, &prev,0);
  if( status == errNotFound ) {
    prev.io_addr = -1L;
    status = addrStoreIndex->Locate(keyGT,&addr,0, &next,0);
  }
  else if( !status )
    status = addrStoreIndex->Next(&next,0);
  if( status == errNotFound ) {
    next.io_addr = -1L;
    status = 0;
  }
  if_err( status );
  /* merge with prev if possible */
  if( prev.io_addr >= 0 && prev.io_addr + prev.size == addr.io_addr ) {
    if_err( storeDelete(prev.size,prev.io_addr) );
    addr.io_addr = prev.io_addr;
    addr.size += prev.size;
  }
  /* merge with next if possible */
  if( next.io_addr >= 0 && addr.io_addr + addr.size == next.io_addr ) {
    if_err( storeDelete(next.size,next.io_addr) );
    addr.size += next.size;
  }

  return storeInsert(addr.size,addr.io_addr);
}

/*** int Db::pageLoad(pageId id)
 *
 * loads a db page
 *
 * Input
 *   id         pageId      pageTable element to load
 *
 */

int Db::
pageLoad(pageId id, Page &pg)
{
  locked by(pg.pgLk);
  uint8_t *bp = (uint8_t*)pg.addr;
  int rd = pg->chk_flags(fl_rd);
  int shmid = pg->shmid, used = pg->used;
  if( !bp ) {
    if( no_shm || shmid < 0 ) {
      bp = new_uint8_t(pg->allocated, shmid, id);
      if( used ) rd = fl_rd;
    }
    else {
      bp = get_uint8_t(shmid, id);
      rd = 0;
    }
    if( !bp ) Err(errNoMemory);
  }
  if( likely(rd && used > 0) ) {
    if_err( pageRead(pg->io_addr, bp, used) );
  }
  pg.addr = (pagePrefix*)bp;
  pg.shm_id = shmid;
  pg->clr_flags(fl_rd);
  return 0;
}

/*** int Db::pageRead(ioAddr io_adr, uint8_t *bp, int len)
 *
 * read data from the database file
 *
 * Input
 *   pg    Page    input    Page to read
 *
 */

int Db::
pageRead(ioAddr io_adr, uint8_t *bp, int len)
{
  //locked by(db_info->pgLdLk);
  //if_err( seek_data(io_adr) );
  //if_err( read_data((char*)bp, len) );
  long sz = pread(fd, bp, len, io_adr);
  if( len != sz ) Err(errIoRead);
  file_position = io_adr+len;
  pagePrefix *bpp = (pagePrefix *)bp;
  if( bpp->magic != page_magic ) Err(errBadMagic);
  return 0;
}

/*** int Db::pageWrite(Page *pp)
 *
 * writes data to the database file
 *
 * Input
 *   pg    Page    input    Page to write
 *
 */

int Db::
pageWrite(Page &pg)
{
  pagePrefix *bpp = pg.addr;
//  when io is block aligned and not disk block sized, but
//    the allocated areas exceed disk block size, increase
//    write to whole blocks to prevent read/modify/write.
  int len = bpp->used = pg->used;
  unsigned int blen = (len+0xfff) & ~0xfff;
  if( blen > pg->allocated ) blen = pg->allocated;
  if_err( seek_data(pg->wr_io_addr) );
  if_err( write_data((char *)bpp, blen) );
  pg->trans_id = active_transaction;
  return 0;
}

// deallocate page data buffer
//  mode<0: delete without shm deallocate
//  mode=0: delete and deallocate written page
//  mode>0: delete and deallocate page (default)

void Db::
pageDealloc(Page &pg, int mode)
{
  uint8_t *bp = (uint8_t *)pg.addr;
//int pg=page_table_sz; while( --pg>=0 && pp!=pageTable[pg] );
  if( del_uint8_t(bp, pg.shm_id /*, pg*/) <= 1 ||
      mode > 0 || (!mode && pg->chk_flags(fl_wr)) )
    pg->shmid = -1;
  pg.init();
}


int Db::AllocCache::
cacheFlush(Db *db)
{
  if( loc.id >= 0 ) {
    if_ret( db->objectHeapInsert(avail,loc.id,loc.offset,*this) );
    loc.id = NIL;
  }
  return 0;
}

int Db::AllocCache::
Get(Db *db,int &size, pgRef &ref)
{
  if( loc.id >= 0 ) {
    int isz = avail - size;
    if( isz >= 0 ) {
      unsigned int sz = isz;
      if( sz < min_heap_allocation ) {
        size += sz;  sz = 0;
      }
      ref = loc;  avail = sz;
      if( sz == 0 ) loc.id = NIL;
      sz = (loc.offset += size);
      Page &pg = *db->get_page(ref.id);
      if( pg->used < sz ) pg->used = sz;
      return 1;
    }
    if_ret( cacheFlush(db) );
  }
  return 0;
}

int Db::AllocCache::
Load(Db *db, pageId id, int ofs, int sz)
{
  if( loc.id >= 0 ) {
    if( avail > sz ) {
      if_ret( db->objectHeapInsert(sz,id,ofs,*this) );
      return 0;
    }
    cacheFlush(db);
  }
  init(id, ofs, sz);
  return 0;
}

void Db::
cacheDelete(AllocCache &cache)
{
  freeSpaceRecord free;
  if( !freeSpaceIndex->First(&free,0) ) do {
//    printf("free=%04lx %d/%d\n", free.size,free.id,free.offset);
    objectHeapInsert(free.size, free.id, free.offset,alloc_cache);
  } while( !freeSpaceIndex->Next(&free,0) );
  indecies[cache.freeIdx]->Clear();
  indecies[cache.addrIdx]->Clear();
  del_index(cache.freeIdx);
  del_index(cache.addrIdx);
  cache.freeIdx = -1;
  cache.addrIdx = -1;
}

int Db::cache_all_flush()
{
  Entity e(this);  EntityLoc &ent = e.ent;  int ret;
  if( !(ret=entityIdIndex->First(0,&ent.obj)) ) do {
    if_err( ent.cacheFlush() );
  } while( !(ret=entityIdIndex->Next(0,&ent.obj)) );
  if( ret == errNotFound ) ret = 0;
  if_err( ret );
  if_err( cacheFlush() );
  return 0;
}

int Db::
allocate(int typ, int size, pgRef &loc, AllocCache &cache)
{
  locked by(db_info->objAlLk);
  // add allocPrefix
  int sz = size + sizeof(allocPrefix);
  // granularity is sizeof pagePrefix
  sz = pagePrefixHunks(sz) * sizeof(pagePrefix);
  if_err( objectAllocate(typ, sz, loc, cache) );
  allocPrefix *mp;
  if_err( addrWrite_(loc,mp) );
  mp->magic = entity_magic;
  mp->type = typ;
  mp->size = sz;
  return 0;
}

int Db::
deallocate(pgRef &loc, AllocCache &cache)
{
  locked by(db_info->objAlLk);
  cache.cacheFlush(this);
  if( loc.id < 0 ) return 0;
  if_fail( objectFree(loc, cache) );
  loc.id = NIL;  loc.offset = 0;
  return 0;
}

int Db::
reallocate(int size, pgRef &loc, AllocCache &cache)
{
  int sz = size + sizeof(allocPrefix);
  sz = pagePrefixHunks(sz) * sizeof(pagePrefix);
  int typ = 0, msz = 0;
  if( loc.id >= 0 ) {
    allocPrefix *mp;
    if_err( addrRead_(loc,mp) );
    typ = mp->type;
    msz = mp->size;
  }
  if( msz != sz ) {
    pgRef ref;
    ref.id = NIL; ref.offset = 0;
    if( size > 0 ) {
      if_err( allocate(typ, size, ref, cache) );
      if( (msz-=sizeof(allocPrefix)) > 0 ) {
        char *bp = 0, *cp = 0;
        if_err( addrRead(loc,bp) );
        if_err( addrWrite(ref,cp) );
        if( msz > size ) msz = size;
        memmove(cp,bp,msz);
      }
    }
    if_err( deallocate(loc, cache) );
    loc = ref;
  }
  return 0;
}

#ifdef ZMEDIA

int Db::pack::aligned = 0;

void Db::pack::put(uint64_t v, int n)
{
  if( (n & (n-1)) == 0 && (idx & (n-1)) == 0 ) {
    int i = idx/8;
    if( n < 8 ) {
      int k = 8-n-(idx&7), m = (1<<n)-1;
      bits[i] = (bits[i] & ~(m<<k)) | (v<<k);
    }
    else {
      switch( n ) {
      case 8:  *((uint8_t *)&bits[i]) = v;           break;
      case 16: *((uint16_t*)&bits[i]) = htobe16(v);  break;
      case 32: *((uint32_t*)&bits[i]) = htobe32(v);  break;
      case 64: *((uint64_t*)&bits[i]) = htobe64(v);  break;
      }
    }
    idx += n;
    return;
  }

  if( !aligned && n <= 32 ) {
    int i = idx/32, k = 64-n-idx%32;
    uint64_t *bp = (uint64_t *)&bits[4*i], m = ((uint64_t)1<<n)-1;
    *bp = htobe64(((be64toh(*bp) & ~(m<<k)) | (v<<k)));
    idx += n;
    return;
  }

  while( n > 0 ) {
    int i = idx/64, k = 64-(idx&(64-1)), l = k - n;
    uint64_t *bp = (uint64_t*)&bits[8*i];
    uint64_t b = be64toh(*bp), m = ((uint64_t)1<<n)-1;  v &= m;
    b = l>=0 ? (b & ~(m<<l)) | (v<<l) : (b & ~(m>>-l)) | (v>>-l);
    *bp = htobe64(b);
    if( n < k ) k = n;
    idx += k;  n -= k;
  }
}


uint64_t Db::pack::get(int n)
{
  uint64_t v = 0;
  if( (n & (n-1)) == 0 && (idx & (n-1)) == 0 ) {
    int i = idx>>3;
    if( n < 8 ) {
      int k = 8-n-(idx&7), m = (1<<n)-1;
      v = (bits[i]>>k) & m;
    }
    else {
      switch( n ) {
      case 8:  v = *((uint8_t *)&bits[i]);         break;
      case 16: v = be16toh(*(uint16_t*)&bits[i]);  break;
      case 32: v = be32toh(*(uint32_t*)&bits[i]);  break;
      case 64: v = be64toh(*(uint64_t*)&bits[i]);  break;
      }
    }
    idx += n;
    return v;
  }

  if( !aligned && n <= 32 ) {
    int i = idx/32, k = 64-n-idx%32;
    uint64_t *bp = (uint64_t *)&bits[4*i], m = ((uint64_t)1<<n)-1;
    v = (be64toh(*bp) >> k) & m;
    idx += n;
    return v;
  }

  while( n > 0 ) {
    int i = idx/64, k = 64-(idx&(64-1)), l = k - n;
    uint64_t *bp = (uint64_t*)&bits[8*i];
    uint64_t b = be64toh(*bp), m = ((uint64_t)1<<n)-1;
    uint64_t vv = l>=0 ? (b>>l) & m : b & (m>>-l);
    if( n < k ) k = n;
    v <<= k;  v |= vv;
    idx += k;  n -= k;
  }
  return v;
}


int64_t Db::mediaKey::bit_size(int len, int w)
{
  int64_t sz = 0;
  for( int i=0, m=len; m>1; ++i ) {
    m = (m+1)/2;
    int b = m * (i+w+1);
    b = (b+pack::alignBits-1) & ~(pack::alignBits-1);
    sz += b;
  }
  return sz;
}

int64_t Db::mediaKey::byte_size(int len, int w)
{
  int64_t sz = bit_size(len, w);
  sz = (sz+(8*sizeof(uint64_t)-1)) & ~(8*sizeof(uint64_t)-1);
  return sz/8 + 2*sizeof(int)+sizeof(int64_t);
}

int64_t Db::mediaKey::count1(uint8_t *kp)
{
  pack pk(kp);
  int w = pk.iget(), len = pk.iget();
  pk.seek(pk.pos()+sizeof(int64_t)*8);
  int k = high_bit_no(len-1);
  int sz = k+w;
  int64_t z = (uint64_t)1<<sz++;
  return pk.get(sz) - z;
}

Db::mediaKey::
mediaKey(uint8_t *kp, uint8_t *bp, int w, int len)
{
  pack pk(kp);
  pk.iput(this->w = w);
  pk.iput(this->len = len);
  if( !len ) return;
  pack pb(bp);
  if( len == 1 ) {
    pk.lput(this->cnt = pb.get(w));
    return;
  }

  // allocate memory, every level length m requires (m+1)/2 differences
  //  need an extra one when length is power of 2
  int k = high_bit_no(len-1) + 1;
  struct { int64_t cnt, sz, *wgt; } lt[k+1], rt[k+1];
  for( int i=0, m=len; i<=k; ++i ) {
    m = (m+1)/2;
    lt[i].wgt = new int64_t[m];
    rt[i].wgt = new int64_t[m];
    lt[i].sz = rt[i].sz = 0;
    lt[i].cnt = rt[i].cnt = 0;
  }    

  int64_t n = 0;
  for( int i=0,l=0; ++i<=len; l=i ) {
    n += pb.get(w);
    int m = i&1 ? 0 : high_bit_no(i ^ l); // highest flipped bit
    rt[m].cnt = n;                        // 0->1, begin right
    for( int j=0; j<m; ++j ) {
      rt[j].wgt[rt[j].sz++] = n-rt[j].cnt;// 1->0, end right
      lt[j].cnt = n;                      // 1->0, begin left
    }
    lt[m].wgt[lt[m].sz++] = n-lt[m].cnt;  // 0->1, end left
  }

  for( int i=0, m=len; m>1; ++i ) {
    m = (m+1)/2;
    if( lt[i].sz < m ) {                  // finish left
      lt[i].wgt[lt[i].sz++] = n-lt[i].cnt;
      rt[i].cnt = n;
    }
    if( lt[i].sz != rt[i].sz )            // finish right
      rt[i].wgt[rt[i].sz++] = n-rt[i].cnt;
  }

  pk.lput(cnt=n);

  for( int i=k; --i>=0; ) {
    n = (uint64_t)1<<(i+w);               // offset for signed difference
    int sz = lt[i].sz;
    for( int j=0; j<sz; ++j ) {
      uint64_t v = (lt[i].wgt[j]-rt[i].wgt[j]) + n;
      pk.put(v, i+w+1);                    // output pair differences
    }
    if( (n=pk.pos()%pack::alignBits) != 0 )      // align next level
      pk.put(0, pack::alignBits-n);
  }

  for( int i=0; i<=k; ++i ) {
    delete [] lt[i].wgt;
    delete [] rt[i].wgt;
  }
}

Db::mediaKey::
~mediaKey()
{
}


Db::mediaLoad::
mediaLoad(uint8_t *bp)
{
  pb.init(bp);
  w = pb.iget(); len = pb.iget(); cnt = pb.lget();
  spos = pb.pos();
  psz = high_bit_no(len-1)+1;
  if( psz < 1 ) psz = 1;
  dsz = dsize(len);
}

Db::mediaLoad::
~mediaLoad()
{
}

void Db::mediaLoad::
get_fields(int n, int k)
{
  int m = (n+1)/2;
  if( m > 1 ) {
    dp[k] = dat;  dat += m;
    get_fields(m, k+1);
  }
  int64_t *ap = dp[k];
  int sz = k+w;
  int64_t z = (uint64_t)1<<sz++;
  if( k > 0 ) {
    int64_t *avp = dp[k-1];
    for( int j=n/2; --j>=0; ) {
      int64_t av = pb.get(sz)-z, a = *ap++;
      *avp++ = (a+av)/2;  *avp++ = (a-av)/2;
    }
    if( (n&1) != 0 ) {
      int64_t av = pb.get(sz)-z, a = *ap;
      *avp++ = (a+av)/2;
    }
  }
  else {
    int64_t *ap = dp[0];
    for( int j=n/2; --j>=0; ) {
      int64_t av = pb.get(sz)-z, a = *ap++;
      pk.putc((a+av)/2, w);  pk.putc((a-av)/2, w);
    }
    if( (n&1) != 0 ) {
      int64_t av = pb.get(sz)-z, a = *ap;
      pk.putc((a+av)/2, w);
    }
  }
  pb.align();
}

void Db::mediaLoad::
load(uint8_t *kp)
{
  pb.seek(spos);  pk.init(kp);
  pk.set_max((1<<w)-1);
  int64_t d[dsz];  dat = d;
  int64_t *p[psz]; dp = p;
  dp[psz-1] = &cnt;
  for( int i=psz-1; --i>=0; ) dp[i] = 0;
  if( len > 1 )
    get_fields(len, 0);
  else
    pk.put(cnt, w);
}


Db::mediaCmpr::
mediaCmpr(uint8_t *bp)
{
  this->err = 0;  this->lmt = ~0;
  pb.init(bp);  w = pb.iget();  len = pb.iget();
  spos = pb.pos();
  psz = high_bit_no(len-1)+1;
  if( psz < 1 ) psz = 1;
  dsz = dsize(len);
}

Db::mediaCmpr::
~mediaCmpr()
{
}

uint64_t Db::mediaCmpr::
chk_fields(int n, int k)
{
  int m = (n+1)/2;
  if( m > 1 ) {
    adp[k] = adat;  adat += m;
    bdp[k] = bdat;  bdat += m;
    if( chk_fields(m, k+1) > lmt ) return err;
  }
  int64_t *ap = adp[k], *bp = bdp[k];
  //uint64_t serr = 0;
  err = 0;  int sz = k+w;
  int64_t z = (uint64_t)1<<sz++;
  if( k > 0 ) {
    int64_t *avp = adp[k-1], *bvp = bdp[k-1];
    for( int j=n/2; --j>=0; ) {
      int64_t a = *ap++, b = *bp++;
      int64_t av = pb.get(sz)-z, bv = pk.get(sz)-z;
      int64_t alt = (a+av)/2, art = (a-av)/2;
      int64_t blt = (b+bv)/2, brt = (b-bv)/2;
      //serr += sqr(alt-blt) + sqr(art-brt);
      *avp++ = alt;  *avp++ = art;
      *bvp++ = blt;  *bvp++ = brt;
      err += labs(alt-blt) + labs(art-brt);
    }
    if( (n&1) != 0 ) {
      int64_t av = pb.get(sz)-z, bv = pk.get(sz)-z;
      int64_t a = *ap, b = *bp;
      int64_t alt = (a+av)/2, blt = (b+bv)/2;
      //serr += sqr(alt-blt);
      *avp++ = alt;  *bvp++ = blt;
      err += labs(alt-blt);
    }
  }
  else {
    int64_t *ap = adp[0], *bp = bdp[0];
    for( int j=n/2; --j>=0; ) {
      int64_t av = pb.get(sz)-z, bv = pk.get(sz)-z;
      int64_t a = *ap++, b = *bp++;
      int64_t v = a-b, dv = av-bv;
      //serr += sqr(v) + sqr(dv);
      err += labs(v+dv)/2 + labs(v-dv)/2;
    }
    //serr /= 2;
    if( (n&1) != 0 ) {
      int64_t av = pb.get(sz)-z, bv = pk.get(sz)-z;
      int64_t a = *ap, b = *bp;
      int64_t v = a-b, dv = av-bv, d = v + dv;
      //serr += sqr(d/2);
      err += labs(d)/2;
    }
  }
  pb.align();  pk.align();
  //printf("n=%d,k=%d,lerr=%lu,err=%lu\n",n,k,lerr,(int64_t)sqrt((double)serr));
  return err;
}

uint64_t Db::mediaCmpr::
chk(uint8_t *kp, uint64_t lmt)
{
  pb.seek(spos);  pk.init(kp);  err = 0;
  if( pk.iget() != w || len != pk.iget() ) return ~0;
  acnt = pb.lget();  bcnt = pk.lget();
  //if( (err=sqr(acnt-bcnt)) > (this->lmt=lmt) ) return err;
  if( (err=labs(acnt-bcnt)) > (this->lmt=lmt) ) return err;
  int64_t a[dsz], b[dsz];      adat = a;  bdat = b;
  int64_t *ap[psz], *bp[psz];  adp = ap;  bdp = bp;
  adp[psz-1] = &acnt;  bdp[psz-1] = &bcnt;
  for( int i=psz-1; --i>=0; ) adp[i] = bdp[i] = 0;
  return len > 1 ? chk_fields(len, 0) : err;
}

int Db::mediaCmpr::
cmpr(uint8_t *kp, uint64_t lmt)
{
  pb.seek(spos);  pk.init(kp);
  if( pk.iget() != w || len != pk.iget() ) return ~0;
  acnt = pb.lget();  bcnt = pk.lget();
  if( acnt < bcnt ) return -1;
  if( acnt > bcnt ) return 1;
  if( len == 1 ) return 0;
  int bsz = Db::mediaKey::bit_size(len, w);
  int i = bsz / (8*sizeof(uint64_t));
  uint64_t *ap = (uint64_t *)pb.addr(), *bp = (uint64_t *)pk.addr();
  while( i > 0 ) {
    if( *ap != *bp ) return be64toh(*ap)<be64toh(*bp) ? -1 : 1;
    ++ap;  ++bp;  --i;
  }
  if( (i=bsz % (8*sizeof(uint64_t))) > 0 ) {
    int l = (8*sizeof(uint64_t)) - i;
    uint64_t av = be64toh(*ap)>>l, bv = be64toh(*bp)>>l;
    if( av != bv ) return av<bv ? -1 : 1;
  }
  return 0;
}

#endif


int Db::
start_transaction(int undo_save)
{
//traceback("  start_transaction %d\n",my_pid);
  pageId id;
  // activate written pages
  for( id=0; id<root_info->pageTableUsed; ++id ) {
    Page &pg = *get_Page(id);
    if( !pg.addr && pg->used ) pg->set_flags(fl_rd);
    if( !pg->chk_flags(fl_new) ) continue;
    int io_allocated = pg->io_allocated;
    pg->io_allocated = pg->wr_allocated;
    pg->wr_allocated = io_allocated;
    ioAddr io_addr = pg->io_addr;
    pg->io_addr = pg->wr_io_addr;
    pg->wr_io_addr = io_addr;
  }
  // release inactivate page storage
  for( id=0; id<root_info->pageTableUsed; ++id ) {
    Page &pg = *get_Page(id);
    if( !pg->chk_flags(fl_new) ) continue;
    pg->clr_flags(fl_new);
    if_err( storeFree(pg->wr_allocated, pg->wr_io_addr) );
    pg->wr_allocated = 0;  pg->wr_io_addr = NIL;
    if( pg->used ) continue;
    if_err( storeFree(pg->io_allocated, pg->io_addr) );
    pg->io_allocated = 0;  pg->io_addr = NIL;
    free_page(id);
  }

  // truncate storage
  addrStoreRecord addr;
  int status = addrStoreIndex->Last(&addr,0);
  if( !status ) {
    if( addr.io_addr+addr.size == root_info->file_size ) {
      if_err( storeDelete(addr.size, addr.io_addr) );
      ioAddr fsz = storeBlocks(addr.io_addr) * storeBlockSize;
      if( root_info->file_size > fsz ) {
        status = ftruncate(fd, fsz);
        if( status ) Err(errIoWrite);
        addr.size = fsz - addr.io_addr;
        if( addr.size > 0 )
          if_err( storeInsert(addr.size, addr.io_addr) );
        root_info->file_size = fsz;
      }
    }
  }
  else
    if( status != errNotFound ) Err(status);

  for( int idx=0; idx<root_info->indeciesUsed; ++idx )
    if( indecies[idx] ) indecies[idx]->deleteFreeBlocks();

  // move root pages lower if possible
  for( int idx=0; idx<root_info->indeciesUsed; ++idx ) {
    IndexBase *bip = indecies[idx];
    if( !bip ) continue;
    bip->chkLastReset();
    pageId r = bip->st->rootPageId;
    if( r < 0 ) continue;
    if( r != bip->st->rightHandSide ) continue;
    bip->st->rootPageId = bip->st->rightHandSide = lower_page(r);
  }

  // truncate pageTable
  for( id=root_info->pageTableUsed; id>0; --id ) {
    Page &pg = *get_Page(id-1);
    if( pg->used ) break;
  }
  page_table_sz = root_info->pageTableUsed = id;

  // remove released pages from free list
  Page *prev = 0;
  id = root_info->freePages;
  while( id >= 0 ) {
    Page &pg = *get_Page(id);
    pageId next_id = pg->link;
    if( id < root_info->pageTableUsed ) {
      if( prev ) prev->st->link = id;
      else root_info->freePages = id;
      prev = &pg;
    }
    else
      del_page(id);
    id = next_id;
  }
  if( prev ) prev->st->link = NIL;
  else root_info->freePages = NIL;

  if( pageTableHunks(root_info->pageTableUsed)*pageTableHunkSize < pageTableAllocated )
    alloc_pageTable(root_info->pageTableUsed);

  if( undo_save ) undata.save(this);
  active_transaction = ++root_info->transaction_id;
  return 0;
}


#if 1
#define bfr_sz 0x40000

int Db::
open_bfr()
{
  bfr = inp = new char[bfr_sz];
  memset(bfr, 0, bfr_sz);
  lmt = bfr + bfr_sz;
  return 0;
}

int Db::
close_bfr()
{
  int ret = flush_bfr();
  delete [] bfr;
  bfr = inp = lmt = 0;
  return ret;
}

int Db::
flush_bfr()
{
  int n = inp - bfr;
  inp = bfr;
  return n > 0 ? write_data(bfr, n) : 0;
}

int Db::
write_bfr(char *dp, int sz)
{
  while( sz > 0 ) {
    int n = lmt - inp;
    if( n > sz ) n = sz;
    memcpy(inp, dp, n);
    if( (inp+=n) >= lmt && flush_bfr() < 0 )
      return -1;
    dp += n;  sz -= n;
  }
  return 0;
}
#endif


int Db::
commit(int force)
{
//traceback(" commit %d\n",my_pid);
  int v = attach_wr();
  int ret = icommit(force);
  if( !ret && force < 0 ) ret = icommit(1);
  if( ret ) iundo();
  if( v ) detach_rw();
  if_err( ret );
  return 0;
}

int Db::
icommit(int force)
{
//traceback("  icommit %d\n",my_pid);
  int n, id;

  if( force < 0 )
    cache_all_flush();

  if( !force ) {
    // check for written pages
    for( id=0,n=root_info->pageTableUsed; --n>=0; ++id ) {
      Page &pg = *get_Page(id);
      if( pg->chk_flags(fl_wr) ) break;
    }
    // if no pages are written
    if( n < 0 ) return 0;
  }

#if 1
  // release last root info, allows storage to move down
  if_err( storeFree(root_info->last_info_size, root_info->last_info_addr) );
  root_info->last_info_addr = NIL;  root_info->last_info_size = 0;
#endif

  ioAddr ri_addr = root_info->last_info_addr;
  int ri_size = root_info->last_info_size;
  int k = root_info_extra_pages;

  for(;;) {
    // allocate new storage
    do {
      n = 0;
      for( id=0; id<root_info->pageTableUsed; ++id ) {
        Page &pg = *get_Page(id);
        if( !pg->chk_flags(fl_wr) ) continue;
        if( pg->chk_flags(fl_new) ) continue;
        pg->set_flags(fl_new);
        if( pg->used ) {
          pg->wr_allocated = pg->allocated;
          if_err( storeAllocate(pg->wr_allocated, pg->wr_io_addr) );
          ++n;
        }
      }
    } while( n > 0 );
    // compute rootInfo size
    int rsz = writeRootInfo(&Db::size_data);
    int rsz0 = entityPages(rsz) * entityPageSize;
    int rsz1 = rsz0 + k * entityPageSize;
    // retry while no storage, too little, or too big
    if( !(ri_addr < 0 || ri_size < rsz0 || ri_size > rsz1) ) break;
    // delete/allocate can change pageTable;
    if_err( storeFree(ri_size, ri_addr) );
    ri_addr = NIL;  ri_size = 0;
    rsz1 = rsz0 + k/2 * entityPageSize;
    if_err( storeAllocate(rsz1, ri_addr) );
    ri_size = rsz1;  k += 2;
  }

  // delete free index pages
  for( int idx=0; idx<root_info->indeciesUsed; ++idx ) {
    IndexBase *ib = indecies[idx];
    if( !ib ) continue;
    while( (id=ib->st->freeBlocks) != NIL ) {
      keyBlock *kbb;  pgRef loc;
      loc.id = id;  loc.offset = 0;
      if_err( addrWrite_(loc,kbb) );
      ib->st->freeBlocks = kbb->right_link();
      Page &pg = *get_Page(id);
      pg->used = 0;  pg->set_flags(fl_new);
    }
  }

  root_info->last_info_addr = root_info->root_info_addr;
  root_info->last_info_size = root_info->root_info_size;
  root_info->root_info_addr = ri_addr;
  root_info->root_info_size = ri_size;

  int npages = root_info->pageTableUsed;
  pageId *pages = new pageId[npages];
  for( int i=0 ; i<npages; ++i ) pages[i] = i;
  qsort_r(pages, npages, sizeof(*pages), ioCmpr, this);

  // write page storage
  for( id=0; id<npages; ++id ) {
    Page &pg = *get_Page(pages[id]);
    if( pg->chk_flags(fl_wr) ) {
      pg->clr_flags(fl_wr);
      if( pg->used )
        if_err( pageWrite(pg) );
    }
    if( force ) {
      if( force < 0 || pg->type < pg_index || pg->type > max_index_type ) {
        pageDealloc(pg);
        pg->set_flags(fl_rd);
      }
    }
  }

  delete [] pages;

  // write rootInfo storage
  if_err( seek_data(ri_addr) );
#if 1
  if_err( open_bfr() );
  if_err( writeRootInfo(&Db::write_bfr) );
  if_err( close_bfr() );
#else
  if_err( writeRootInfo(&Db::write_data) );
#endif
  if_err( write_zeros(ri_addr+ri_size) );

  // update rootInfo pointer
  if_err( seek_data(root_info_offset) );
  if_err( write_data((char*)&ri_addr,sizeof(ri_addr)) );

  // commit is finished
  return start_transaction();
}

int Db::
flush()
{
  // evict unwritten page storage
  for( int id=0; id<root_info->pageTableUsed; ++id ) {
    Page &pg = *get_page(id);
    if( !pg.addr ) continue;
    if( pg->chk_flags(fl_wr) ) continue;
    pageDealloc(pg);
    pg->set_flags(fl_rd);
  }
  return 0;
}

int Db::
seek_data(ioAddr io_addr)
{
  if( io_addr < 0 ) Err(errIoSeek);
  if( file_position != io_addr ) {
    long offset = lseek(fd,io_addr,SEEK_SET);
    if( offset != io_addr ) Err(errIoSeek);
    file_position = io_addr;
  }
  return 0;
}

int Db::
size_data(char *dp, int sz)
{
  return sz > 0 ? sz : 0;
}

int Db::
read_data(char *dp, int sz)
{
  if( sz > 0 ) {
    long len = read(fd,dp,sz);
    if( len != sz ) Err(errIoRead);
    file_position += sz;
  }
  return 0;
}

int Db::
write_data(char *dp, int sz)
{
  if( sz > 0 ) {
    long len = write(fd,dp,sz);
    if( len != sz ) Err(errIoWrite);
    file_position += sz;
    if( file_position > root_info->file_size )
      root_info->file_size = file_position;
  }
  return 0;
}

int Db::
write_zeros(ioAddr io_addr)
{
  ioAddr len = io_addr - file_position;
  if( len < 0 ) Err(errIoWrite);
  int sz = len > entityPageSize ? entityPageSize : len;
  char bfr[sz];  memset(&bfr[0],0,sz);
  while( len > 0 ) {
    sz = len > entityPageSize ? entityPageSize : len;
    if_err( write_data(&bfr[0], sz) );
    len = io_addr - file_position;
  }
  return 0;
}

int Db::
write_padding(ioAddr io_addr)
{
  if( root_info->file_size > io_addr ) Err(errIoWrite);
#if 0
  if_err( write_zeros(io_addr) );
#else
  int status = ftruncate(fd, io_addr);
  if( status ) Err(errIoSeek);
  root_info->file_size = io_addr;
#endif
  return 0;
}

#define Read(n) do { \
    if( (count+=sizeof(n)) > root_info->root_info_size ) Err(errCorrupt); \
    if_err( (this->*fn)((char*)&(n),sizeof(n)) ); \
  } while(0)

int Db::
readRootInfo(int(Db::*fn)(char *dp,int sz))
{
  int id, sz;
  int count = 0;

  // rootInfo data
  root_info->root_info_size = sizeof(RootInfo);
  Read(*root_info);
  if( root_info->root_magic != root_magic ) Err(errBadMagic);

  // indecies data
  sz = indeciesHunks(root_info->indeciesUsed) * indexTableHunkSize;
  indecies = new IndexBase*[sz];
  if( !indecies ) Err(errNoMemory);
  index_info = (IndexInfo *)new_uint8_t(sz*sizeof(IndexInfo),id);
  if( !index_info ) Err(errNoMemory);
  db_info->index_info_id = index_info_id = id;    
  indeciesAllocated = sz;

  indecies_sz = root_info->indeciesUsed;
  for( int idx=0; idx<indecies_sz; ++idx ) {
    IndexBaseInfo *b = &index_info[idx].base;
    Read(*(IndexTypeInfo*)b);
    if( b->magic != idx_magic ) Err(errBadMagic);
    if( b->type != idxNil )
      Read(*(IndexRecdInfo*)b);
    switch( b->type ) {
    case idxBin: {
      IndexBinaryInfo *bi = &index_info[idx].bin;
      Read(*bi);
      if_err( new_index(indecies[idx], b, bi) );
      break; }
    case idxStr: {
      IndexStringInfo *si = &index_info[idx].str;
      Read(*si);
      if_err( new_index(indecies[idx], b, si) );
      break; }
    case idxNil: {
      indecies[idx] = 0;
      break; }
    default:
      Err(errCorrupt);
    }
  }

  // allocator
  int fidx = get_index(".free");
  if( fidx < 0 ) Err(errCorrupt);
  int aidx = get_index(".addr");
  if( aidx < 0 ) Err(errCorrupt);
  alloc_cache.freeIdx = fidx;
  alloc_cache.addrIdx = aidx;

  // pageTable data
  page_table_sz = root_info->pageTableUsed;
  sz = pageTableHunks(page_table_sz) * pageTableHunkSize;
  pageTable = (Page **)new Page*[sz];
  if( !pageTable ) Err(errNoMemory);
  pageTableAllocated = sz;
  sz = pageTableAllocated*sizeof(PageStorage);
  page_info = (PageStorage *)new_uint8_t(sz, id);
  if( !page_info ) Err(errNoMemory);
  db_info->page_info_id = page_info_id = id;    
  for( id=0; id<root_info->pageTableUsed; ++id ) {
    Read(page_info[id]);
    Page *pp = new Page(page_info[id]);
    if( !pp ) Err(errNoMemory);
    set_Page(id, pp);
    Page &pg = *pp;
    if( !pg->chk_flags(fl_free) ) pg->shmid = -1;
    if( pg->used ) pg->set_flags(fl_rd);
  }
  while( id < pageTableAllocated )
    set_Page(id++, 0);

  return count;
}

#undef Read

#define Write(n) do { \
    if_err( (sz=(this->*fn)((char*)&(n),sizeof(n))) ); \
    count+=sz; \
  } while(0)

int Db::
writeRootInfo(int(Db::*fn)(char *data, int sz))
{
  int id, sz;
  int count = 0;

  // rootInfo data
  Write(*root_info);

  // indecies data
  for( id=0; id<root_info->indeciesUsed; ++id ) {
    IndexBase *ib = indecies[id];
    if( ib ) {
      switch( ib->st->type ) {
      case idxBin: {
        IndexBinary *b = (IndexBinary*)ib;
        Write(*b->st);
        Write(*b->bst);
        break; }
      case idxStr: {
        IndexString *b = (IndexString*)ib;
        Write(*b->st);
        Write(*b->sst);
        break; }
      }
    }
    else {
      IndexBaseType b(idxNil);
      Write(b);
    }
  }

  // pageTable data
  for( id=0; id<root_info->pageTableUsed; ++id ) {
    Page *pp = get_Page(id);
    Write(*pp->st);
  }

  return count;
}

#undef Write

int Db::undoData::
save(Db *db)
{
  int n = db->indeciesAllocated - usrIdx + 1;
  if( cfnAllocated != n ) {
    delete [] cfn;
    cfn = new cfnData[n];
    cfnAllocated = n;
  }
  cfnData *cdp = &cfn[0];
  cfnUsed = db->root_info->indeciesUsed;
  for( int idx=usrIdx; idx<cfnUsed; ++idx,++cdp ) {
    IndexBase *ib = db->indecies[idx];
    if( ib ) {
      switch( ib->st->type ) {
      case idxBin: {
        IndexBinary *bidx = (IndexBinary *)ib;
        cdp->cmprId = bidx->bst->cmprId;
        cdp->compare = bidx->compare;
        break; }
      default:
        break;
      }
    }
  }
  return 0;
}

int Db::undoData::
restore(Db *db)
{
  cfnData *cdp = &cfn[0];
  int n = db->root_info->indeciesUsed<cfnUsed ? db->root_info->indeciesUsed : cfnUsed;
  for( int idx=usrIdx; idx<n; ++idx,++cdp ) {
    IndexBase *ib = db->indecies[idx];
    if( ib ) {
      switch( ib->st->type ) {
      case idxBin: {
        IndexBinary *bidx = (IndexBinary *)ib;
        int cid = cdp->cmprId;
        bidx->bst->cmprId = cid;
        bidx->compare = cid>0 ? db->cmprFns[cid] : cdp->compare;
        break; }
      default:
        break;
      }
    }
  }
  return 0;
}

int Db::
undo()
{
  int v = attach_wr();
  int ret = iundo();
  if( ret ) iclose();
  if( v ) detach_rw();
  if_err( ret );
  return 0;
}

int Db::
iundo()
{
  deinit();  init();
  if_err( iopen(0) );
  undata.restore(this);
  file_position = -1;
  alloc_cache.init();
  return 0;
}

Db::DbInfo::
DbInfo(int pid, int key, int id)
{
  magic = info_magic;
  owner = pid;
  last_owner = -1;
  info_key = key;
  info_id = id;
  index_info_id = -1;
  page_info_id = -1;
  root_info_id = -1;
}

int Db::
new_info(int key)
{
  int id = -1, flk = 0, ret = 0;
  void *vp = 0;
  if( !no_shm ) {
    if( !shm_init ) init_shm();
    get_mem = &get_shm8_t;
    new_mem = &new_shm8_t;
    del_mem = &del_shm8_t;
    if( flock(fd, LOCK_EX) ) ret = errInvalid;
    if( !ret ) {
      flk = 1;
      id = shmget(key, sizeof(DbInfo), IPC_CREAT+IPC_EXCL+0666);
      if( id < 0 ) ret = errno==EEXIST ? errDuplicate : errNoMemory;
    }
    if( !ret ) {
      vp = shmat(id, NULL, 0);
      if( vp == (void*)-1 ) ret = errNoMemory;
    }
  }
  else {
    get_mem = &get_mem8_t;
    new_mem = &new_mem8_t;
    del_mem = &del_mem8_t;
    vp = (void *)new uint8_t[sizeof(DbInfo)];
    if( !vp ) Err(errNoMemory);
  }
  if( !ret ) {
    db_info = new(vp) DbInfo(my_pid, key, id);
    attach_wr();
    this->key = key;
  }
  if( flk ) flock(fd, LOCK_UN);
//traceback("new_info %s\n",ret ? "failed" : "success");
  if_err( ret );
  return 0;
}

int Db::
get_info(int key)
{
  if( no_shm ) Err(errInvalid);
  get_mem = &get_shm8_t;
  new_mem = &new_shm8_t;
  del_mem = &del_shm8_t;
  int id = shmget(key, sizeof(DbInfo), 0666);
  if( id < 0 ) Fail(errNotFound);
  void *vp = shmat(id, NULL, 0);
  if( vp == (void*)-1 ) Err(errNoMemory);
  if( del_uint8_t(0, id) <= 1 ) {
    shmctl(id, IPC_RMID, 0);
    shmdt(vp);
//traceback("get_info failed\n");
    Fail(errNotFound);
  }
  DbInfo *dip = (DbInfo *)vp;
  if( dip->magic != info_magic ) Err(errBadMagic);
  if( dip->info_key != key || dip->info_id != id ) Err(errCorrupt);
  db_info = dip;
//traceback("get_info success\n");
  return 0;
}

int Db::
del_info()
{
  if( db_info ) {
    db_info->index_info_id = -1;
    db_info->page_info_id = -1;
    db_info->root_info_id = -1;
    db_info->owner = -1;
    db_info->last_owner = my_pid;
    detach_rw();
    int flk = !flock(fd, LOCK_EX) ? 1 : 0;
    if( !no_shm ) {
      int ret = del_uint8_t(0, db_info->info_id);
      if( ret <= 1 )
        shmctl(db_info->info_id, IPC_RMID, 0);
      shmdt(db_info);
    }
    else
      delete [] (uint8_t*)db_info;
    if( flk ) flock(fd, LOCK_UN);
//traceback("del_info %d, refs=%d\n",my_pid,ret);
    db_info = 0;
  }
  return 0;
}

int Db::
open(int zfd, int zkey)
{
//traceback("open %d\n",my_pid);
  if( zfd >= 0 ) {
    if( fd >= 0 ) Err(errDuplicate);
    fd = zfd;
  }
  if( fd < 0 ) Err(errNotFound);
  struct stat st;
  if( fstat(fd,&st) < 0 ) Err(errIoStat);
  if( zkey >= 0 ) use_shm(1);
  deinit();  init();
  if_err( new_info(zkey) );
  int ret = iopen();
  detach_rw();
  if_err( ret );
  return 0;
}

int Db::
iopen(int undo_save)
{
//traceback(" iopen %d\n",my_pid);
  int info_id;
  root_info = (RootInfo *)new_uint8_t(sizeof(*root_info), info_id);
  int ret = !root_info ? errNoMemory : 0;
  if( !ret ) {
    root_info->init();
    db_info->root_info_id = root_info_id = info_id;     
  }
  int magic;
  if( !ret ) ret = seek_data(db_magic_offset);
  if( !ret ) ret = read_data((char*)&magic,sizeof(magic));
  if( !ret && magic != db_magic ) ret = errBadMagic;
  if( !ret ) ret = seek_data(root_info_offset);
  if( !ret ) ret = read_data((char*)&root_info->root_info_addr,sizeof(root_info->root_info_addr));
  if( !ret ) ret = seek_data(root_info->root_info_addr);
  if( !ret ) ret = readRootInfo(&Db::read_data) >= 0 ? 0 : ret;
  if( !ret ) ret = start_transaction(undo_save);
  if( !ret ) {
    active_transaction = root_info->transaction_id;
  }
  if( ret ) iclose();
  return ret;
}

int Db::
close()
{
//traceback("close %d\n",my_pid);
  if( !db_info || fd < 0 ) return 0;
  return iclose();
}

int Db::
iclose()
{
  attach_wr();
//traceback(" iclose %d\n",my_pid);
  deinit();
  del_info();
  reset();
  return 0;
}

int Db::
ireopen()
{
//traceback(" ireopen %d\n",my_pid);
  Page **old_page_table = pageTable;  pageTable = 0;
  PageStorage *old_page_info = page_info;  page_info = 0;
  int old_page_table_sz = page_table_sz;  page_table_sz = 0;
  int ret = iopen();
  if( !ret ) {
    for( pageId id=0; id<old_page_table_sz; ++id ) {
      Page *opp = old_page_table[id], &opg = *opp;
      if( id < page_table_sz && opg.addr && !opg->chk_flags(fl_wr | fl_rd) ) { 
        Page &pg = *get_Page(id);
        if( !pg.addr && pg->shmid < 0 && opg.shm_id == opg->shmid &&
            pg->io_addr == opg->io_addr && pg->trans_id == opg->trans_id ) { 
          pg.addr = opg.addr;  pg->shmid = pg.shm_id = opg.shm_id;
          pg->clr_flags(fl_rd);
        }
        else
          pageDealloc(opg);
      }
      delete opp;
    }
  }
  delete [] old_page_table;
  del_uint8_t(old_page_info);
  return ret;
}

int Db::
iattach()
{
//traceback(" iattach %d\n",my_pid);
  RootInfo *new_root_info = 0;
  IndexInfo *new_index_info = 0;
  PageStorage *new_page_info = 0;
  int new_indecies_alloc = 0;
  int new_indecies_sz = 0;
  IndexBase **new_indecies = 0;
  int new_page_table_alloc = 0;
  int new_page_table_sz = 0;
  Page **new_page_table = 0;
  int ret = 0;
  // new root info
  if( !ret && root_info_id != db_info->root_info_id ) {
    new_root_info = (RootInfo *)get_uint8_t(db_info->root_info_id);
    if( !new_root_info ) ret = errCorrupt;
  }
  else {
    new_root_info = root_info;
    root_info = 0;
  }
  // new indecies
  if( !ret ) { 
     new_indecies_sz = new_root_info->indeciesUsed;
     new_indecies_alloc = indeciesHunks(new_indecies_sz) * indexTableHunkSize;
     new_indecies = new IndexBase*[new_indecies_alloc];
     if( !new_indecies ) ret = errNoMemory;
  }
  // new index info
  if( !ret ) {
   if( index_info_id != db_info->index_info_id ) {
      new_index_info = (IndexInfo *)get_uint8_t(db_info->index_info_id);
      if( !new_index_info ) ret = errCorrupt;
    }
    else {
      new_index_info = index_info;
      index_info = 0;
    }
  }

  for( int idx=0; !ret && idx<new_indecies_sz; ++idx ) {
    IndexBaseInfo *b = &new_index_info[idx].base;
    if( b->magic == idx_magic ) {
      IndexBase *ib = idx<indecies_sz ? indecies[idx] : 0;
      if( ib && ib->st->type != b->type ) ib = 0;
      switch( b->type ) {
      case idxBin: {
        if( ib ) {
          new_indecies[idx] = new(ib) IndexBinary(ib,*b,new_index_info[idx].bin);
          indecies[idx] = 0;
          break;
        }
        ret = new_index(new_indecies[idx], b, &new_index_info[idx].bin);
        break; }
      case idxStr: {
        if( ib ) {
          new_indecies[idx] = new(ib) IndexString(ib,*b,new_index_info[idx].str);
          indecies[idx] = 0;
          break;
        }
        ret = new_index(new_indecies[idx], b, &new_index_info[idx].str);
        break; }
      case idxNil: {
        new_indecies[idx] = 0;
        break; }
      default:
        ret = errCorrupt;
        break;
      }
    }
    else
      ret = errBadMagic;
  }
  for( int idx=0; idx<indecies_sz; ++idx ) {
    if( !indecies[idx] ) continue;
    delete indecies[idx];  indecies[idx] = 0;
  }

  // new page table
  if( !ret ) {
    new_page_table_sz = new_root_info->pageTableUsed;
    new_page_table_alloc = pageTableHunks(new_page_table_sz) * pageTableHunkSize;
    new_page_table = (Page **)new Page*[new_page_table_alloc];
    if( !new_page_table ) ret = errNoMemory;
  }
  // new page info
  if( !ret ) {
    if( page_info_id != db_info->page_info_id ) {
      new_page_info = (PageStorage *)get_uint8_t(db_info->page_info_id);
      if( !new_page_info ) ret = errCorrupt;
    }
    else {
      new_page_info = page_info;
      page_info = 0;
    }
  }

  pageId id;
  for( id=0; !ret && id<new_page_table_sz; ++id ) {
    Page *pp = id<page_table_sz ? pageTable[id] : 0;
    PageStorage *st = &new_page_info[id];
    if( pp ) {
      pageTable[id] = 0;  pp->st = st;  Page &pg = *pp;
      if( pg->chk_flags(fl_rd | fl_free) )
        pageDealloc(pg);
      else if( pg->shmid >= 0 && pp->shm_id != pg->shmid ) {
        del_uint8_t(pp->addr);
        pp->addr = (pagePrefix *)get_uint8_t(pp->shm_id=pg->shmid);
      }
    }
    else {
      pp = new Page(*st);
      if( !pp ) ret = errNoMemory;
    }
    new_page_table[id] = pp;
  }
  while( id<page_table_sz ) del_page(id++);

  if( ret ) {
    delete [] new_indecies;
    delete [] new_page_table;
    if( !root_info ) root_info = new_root_info;
    else del_uint8_t(new_root_info);
    if( !index_info ) index_info = new_index_info;
    else del_uint8_t(new_index_info);
    if( !page_info ) page_info = new_page_info;
    else del_uint8_t(new_page_info);
    iclose();
    Err(ret);
  }

  delete [] indecies;
  indecies = new_indecies;
  indeciesAllocated = new_indecies_alloc;
  indecies_sz = new_indecies_sz;
  delete [] pageTable;
  pageTable = new_page_table;
  pageTableAllocated = new_page_table_alloc;
  page_table_sz = new_page_table_sz;
  root_info_id = db_info->root_info_id;
  del_uint8_t(root_info);
  root_info = new_root_info;
  index_info_id = db_info->index_info_id;
  del_uint8_t(index_info);
  index_info = new_index_info;
  page_info_id = db_info->page_info_id;
  del_uint8_t(page_info);
  page_info = new_page_info;
  active_transaction = root_info->transaction_id;
  return 0;
}

int Db::
attach(int zfd, int zkey, int zrw)
{
//traceback("attach %s %d\n",zrw ? "wr" : "rd", my_pid);
  if( !db_info ) {
    if_ret( get_info(zkey) );
    fd = zfd;  key = zkey;
    init();
  }
  else if( zfd != fd || zkey != key )
    Err(errInvalid);
  attach_rw(zrw);
  if( no_shm ||
    ( root_info && active_transaction == root_info->transaction_id &&
      db_info->root_info_id >= 0 && root_info_id == db_info->root_info_id &&
      db_info->index_info_id >= 0 && index_info_id == db_info->index_info_id &&
      db_info->page_info_id >= 0 && page_info_id == db_info->page_info_id ) )
    return 0;
  int ret = db_info->root_info_id < 0 ? ireopen() : iattach();
//fchk();  achk();
  if( ret ) iclose();
  if_err( ret );
  return 0;
}

int Db::
detach()
{
  detach_rw();
  return 0;
}


int Db::
make(int zfd)
{
  if( fd >= 0 ) Err(errDuplicate);
  fd = zfd;
  init();
  no_shm = 1;
  if( new_info(key) ) Err(errNotFound);
  int info_id;
  root_info = (RootInfo *)new_uint8_t(sizeof(*root_info), info_id);
  if( !root_info ) { Err(errNoMemory); }
  root_info->init();
  db_info->root_info_id = root_info_id = info_id;      
  if_err( init_idx() );
  if_err( seek_data(db_magic_offset) );
  int magic = db_magic;
  if_err( write_data((char *)&magic,sizeof(magic)) );
  write_zeros(entityPageSize);
  return commit(1);
}


// in-order traversal copying IndexBinary
int Db::IndexBinary::
keyCopy(pageId s, IndexBase *ib)
{
  IndexBinary *idx = (IndexBinary *)ib;
  pageId r;
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,0,sbb,spp,sn) );
  if( (r=sbb->right_link()) >= 0 ) {
    int lkdSz = kdSz + sizeof(pageId);
    int n = spp->iused() / lkdSz;
    for( int i=0; i<n; ++i ) {
      pageId l = readPageId(sn);
      sn += sizeof(pageId);
      if_ret( keyCopy(l,idx) );
      if_ret( idx->Insert(sn,sn+st->keySz) );
      sn += kdSz;
    }
    if_ret( keyCopy(r,idx) );
  }
  else {
    int n = spp->iused() / kdSz;
    for( int i=0; i<n; ++i ) {
      if_ret( idx->Insert(sn,sn+st->keySz) );
      sn += kdSz;
    }
  }
  return 0;
}

// in-order traversal copying IndexString
int Db::IndexString::
keyCopy(pageId s, IndexBase *ib)
{
  IndexString *idx = (IndexString *)ib;
  pageId r;  unsigned char lky[keysz];
  keyBlock *sbb;  Page *spp;  char *sn;
  if_err( db->indexRead(s,0,sbb,spp,sn) );
  unsigned char *lp = (unsigned char *)sn;
  unsigned char *rp = lp + spp->iused();
  lky[0] = 0;
  if( (r=sbb->right_link()) >= 0 ) {
    while( lp < rp ) {
      pageId l = getPageId(lp);
      if_ret( keyCopy(l,idx) );
      char *dp = (char *)lp;  lp += st->dataSz;
      for( int i=*lp++; (lky[i++]=*lp++) != 0; );
      if_ret( idx->Insert(&lky[0],dp) );
    }
    if_ret( keyCopy(r,idx) );
  }
  else {
    while( lp < rp ) {
      char *dp = (char *)lp;  lp += st->dataSz;
      for( int i=*lp++; (lky[i++]=*lp++) != 0; );
      if_ret( idx->Insert(&lky[0],dp) );
    }
  }
  return 0;
}

Db::EntityObj::
EntityObj(EntityObj &eobj, int eid)
{
  id = eid;
  memmove(&name[0],&eobj.name[0],nmSz);
  recdSz = eobj.recdSz;
  nidxs = eobj.nidxs;
  indexs[idxId] = eobj.indexs[idxId];
  for( int i=idxId+1; i<nidxs; ++i )
    indexs[i] = eobj.indexs[i];
  maxId = count = 0;
  alloc_cache.init();
}

int Db::ObjectLoc::
copy(ObjectLoc &dobj)
{
  // allocate object
  Obj *dp = dobj.addr();
  if( !dp ) Err(errNoMemory);
  allocPrefix *mp = ((allocPrefix *)dp) - 1;
  int sz = mp->size - sizeof(*mp);
  if_err( allocate(sz - dobj.entity->ent->recdSz) );
  Obj *np = addr();
  if( !np ) Err(errNoMemory);
  memcpy(np, dp, sz);
  // copy varObj data using ref data, if any
  for( varObjs vp=dobj.entity->vobjs; vp!=0; vp=vp->next ) {
    vRef ref = vp->ref;
    varObj &vd = dp->*ref;
    varObj &vn = np->*ref;
    vn.init();
    if( (sz=vd.size()) > 0 ) {
      size(vn, sz);
      memcpy(addr(vn),dobj.addr(vd), sz);
    }
  }
  return 0;
}

int Db::
copy(Db *db, Objects objs)
{
  int id, n = db->root_info->indeciesUsed;
  for( id=usrIdx; id<n; ++id ) {
    IndexBase *ib = db->indecies[id];
    if( !ib ) continue;
    int ret = 0;
    switch( ib->st->type ) {
    // copy binary index
    case idxBin: {
      IndexBinary *bidx = (IndexBinary *)ib;
      int idx = get_index(&bidx->st->name[0]);
      if( idx < 0 ) {
        int kySz = bidx->st->keySz, dtSz = bidx->st->dataSz;
        idx = new_binary_index(&bidx->st->name[0], kySz, dtSz, bidx->compare);
        if_err( idx );
      }
      IndexBase *bib = indecies[idx];
      bib->st->key_type = ib->st->key_type;
      // ignore empty indecies
      if( bidx->st->rootPageId < 0 ) break;
      // ignore allocator indecies
      if( bidx->compare == Db::cmprFrSp ) break;
      if( bidx->compare == Db::cmprAdSp ) break;
      // entity id indecies are processed below
      if( !db->entityNmIndex->Find(&ib->st->name[0],0) ) break;
      IndexBinary *bip = (IndexBinary *)bib;
      // use cmprLast since index is in-order. Avoids using
      //   user defined class key cmprs and compare functions.
      bip->compare = cmprLast;
      bib->st->key_type = ktyBin;
      ret = bidx->keyCopy(bidx->st->rootPageId, bib);
      bip->compare = cmprFns[bip->bst->cmprId];
      bib->st->key_type = ib->st->key_type;
      break; }
    // copy string index
    case idxStr: {
      IndexString *sidx = (IndexString *)ib;
      int idx = get_index(&sidx->st->name[0]);
      if( idx < 0 ) {
        int dtSz = sidx->st->dataSz;
        idx = new_string_index(&sidx->st->name[0], dtSz);
        if_err( idx );
      }
      IndexBase *bib = indecies[idx];
      bib->st->key_type = ib->st->key_type;
      if( sidx->st->rootPageId < 0 ) break;
      // copy key/data
      ret = sidx->keyCopy(sidx->st->rootPageId, bib);
      break; }
    }
    if_err( ret );
    if_err( db->flush() );
    if_err( commit(-1) );
  }
  // copy entity indecies/data
  IndexBinary *eidx = (IndexBinary *)db->entityIdIndex;
  int eid, status;  pgRef loc;
  if( !(status=eidx->First(&eid,&loc)) ) do {
    Objects op = objs;
    while( op ) { 
      Entity *ep = op->obj->entity;
      if( ep->ent->id  == eid ) break;
      op = op->next;
    }
    if( !op ) continue;
    Entity db_ent(db);
    EntityLoc &dent = db_ent.ent;
    dent.obj = loc;
    ObjectLoc objd(&db_ent), *dobj = &objd;
    // if old db copy fn, use ObjectLoc::copy
    if( op->obj->entity->db == db ) dobj = op->obj;
    char name[nmSz];  memset(name,0,sizeof(name));
    strncpy(&name[0],&dent->name[0],sizeof(name));
    // new entity
    Entity entity(this);
    EntityLoc &nent = entity.ent;
    int nid;
    if( entityNmIndex->Find(&name[0],&nid) != 0 ) {
      int nidx1 = dent->nidxs-1;
      int sz = sizeof(EntityObj) + sizeof(dent->indexs[0])*nidx1;
      // allocate entity
      if_err( allocate(dent->id, sz, nent.obj, alloc_cache) );
      if( !nent.addr_wr() ) Err(errNoMemory);
      // construct entity
      new((EntityObj *)nent.addr())
        EntityObj(*(EntityObj*)dent.addr(),eid);
      if_err( entityIdIndex->Insert(&eid,&nent.obj) );
      if_err( entityNmIndex->Insert(&name[0],&eid) );
      // connect entity allocator
      char idxNm[nmSz];  memset(idxNm,0,sizeof(idxNm));
      strncpy(idxNm,name,sizeof(idxNm)-1);
      strncat(idxNm,".free",sizeof(idxNm)-1);
      int fidx = get_index(idxNm);
      if( fidx < 0 ) Err(errCorrupt);
      memset(idxNm,0,sizeof(idxNm));
      strncpy(idxNm,name,sizeof(idxNm)-1);
      strncat(idxNm,".addr",sizeof(idxNm)-1);
      int aidx = get_index(idxNm);
      if( aidx < 0 ) Err(errCorrupt);
      nent->alloc_cache.freeIdx = fidx;
      nent->alloc_cache.addrIdx = aidx;
    }
    else if( nid == eid )
      if_err( entityIdIndex->Find(&eid,&nent.obj) );
    else
      Err(errInvalid);
    ObjectLoc objn(&entity), *nobj = &objn;
    // if new db copy fn, use it instead of ObjectLoc::copy
    if( op->obj->entity->db == this ) nobj = op->obj;
    pgRef cur;
    if( !(status = dobj->FirstId(cur)) ) do {
      // copy object data
      if_err( nobj->copy(*dobj) );
      // construct object
      if_err( entity.construct_(*nobj, dobj->id()) );
    } while( !(status=dobj->NextId(cur)) );
    if( status == errNotFound ) status = 0;
    if_err( status );
    if_err( db->flush() );
    if_err( commit(-1) );
    // next entity
  } while( !(status=eidx->Next(&eid,&loc)) );
  if( status == errNotFound ) status = 0;
  if_err( status );
  if_err( db->flush() );
  if_err( commit(-1) );
  return 0;
}

int Db::
cmprFrSt(char *a, char *b)
{
  freeStoreRecord *ap = (freeStoreRecord *)a;
  freeStoreRecord *bp = (freeStoreRecord *)b;
  if( ap->size > bp->size ) return 1;
  if( ap->size < bp->size ) return -1;
  if( ap->io_addr > bp->io_addr ) return 1;
  if( ap->io_addr < bp->io_addr ) return -1;
  return 0;
}

int Db::
cmprAdSt(char *a, char *b)
{
  addrStoreRecord *ap = (addrStoreRecord *)a;
  addrStoreRecord *bp = (addrStoreRecord *)b;
  if( ap->io_addr > bp->io_addr ) return 1;
  if( ap->io_addr < bp->io_addr ) return -1;
  if( ap->size > bp->size ) return 1;
  if( ap->size < bp->size ) return -1;
  return 0;
}

int Db::
cmprFrSp(char *a, char *b)
{
  freeSpaceRecord *ap = (freeSpaceRecord *)a;
  freeSpaceRecord *bp = (freeSpaceRecord *)b;
  if( ap->size > bp->size ) return 1;
  if( ap->size < bp->size ) return -1;
  if( ap->id > bp->id ) return 1;
  if( ap->id < bp->id ) return -1;
  if( ap->offset > bp->offset ) return 1;
  if( ap->offset < bp->offset ) return -1;
  return 0;
}

int Db::
cmprAdSp(char *a, char *b)
{
  addrSpaceRecord *ap = (addrSpaceRecord *)a;
  addrSpaceRecord *bp = (addrSpaceRecord *)b;
  if( ap->id > bp->id ) return 1;
  if( ap->id < bp->id ) return -1;
  if( ap->offset > bp->offset ) return 1;
  if( ap->offset < bp->offset ) return -1;
  if( ap->size > bp->size ) return 1;
  if( ap->size < bp->size ) return -1;
  return 0;
}

int Db::
cmprOIds(char *a, char *b)
{
  Obj *ap = (Obj *)a;
  Obj *bp = (Obj *)b;
  if( ap->id > bp->id ) return 1;
  if( ap->id < bp->id ) return -1;
  return 0;
}

int Db::
cmprStr(char *a, char *b)
{
  return strncmp(a,b,keysz);
}

int Db::
cmprKey(char *a, char *b)
{
  Key *kp = (Key *)a;
  return kp->cmpr(a,b);
}

int Db::
cmprLast(char *a, char *b)
{
  return 1;
}

Db::CmprFn Db::cmprFns[] = {
  0,        cmprFrSt,
  cmprAdSt, cmprFrSp,
  cmprAdSp, cmprOIds,
  cmprKey,  cmprStr,
  cmprLast,
};

int Db::
findCmprFn(CmprFn fn)
{
  int i;
  for( i=lengthof(cmprFns); --i>0; )
    if( fn == cmprFns[i] ) return i;
  return 0;
}

int Db::AllocCache::
init_idx(Db *db,const char *nm)
{
  char idxNm[nmSz];
  memset(idxNm,0,sizeof(idxNm));
  snprintf(idxNm,sizeof(idxNm),"%s.free",nm);
  int fidx = db->new_binary_index(idxNm, sizeof(freeSpaceRecord), 0, cmprFrSp);
  if_ret( fidx );
  memset(idxNm,0,sizeof(idxNm));
  snprintf(idxNm,sizeof(idxNm),"%s.addr",nm);
  int aidx = db->new_binary_index(idxNm, sizeof(addrSpaceRecord), 0, cmprAdSp);
  if( aidx < 0 ) db->del_index(fidx);
  if_ret( aidx );
  freeIdx = fidx;  addrIdx = aidx;
  loc.id = NIL;    loc.offset = 0;
  avail = 0;
  return 0;
}

int Db::
init_idx()
{
  if_err( new_binary_index("entityIdIndex", sizeof(int), sizeof(pgRef), cmprOIds) );
  if_err( new_binary_index("entityNmIndex", sizeof(char[nmSz]), sizeof(int), cmprStr) );
  if_err( new_binary_index("freeStoreIndex", sizeof(freeStoreRecord), 0, cmprFrSt) );
  if_err( new_binary_index("addrStoreIndex", sizeof(addrStoreRecord), 0, cmprAdSt) );
  if_err( alloc_cache.init_idx(this,"") );
  return 0;
}


void Db::
init_shm()
{
  shm_init = 1;
  FILE *fp = fopen("/proc/sys/kernel/shmall","w");
  if( fp ) { fprintf(fp,"%d",0x7fffffff); fclose(fp); }
  fp = fopen("/proc/sys/kernel/shmmax","w");
  if( fp ) { fprintf(fp,"%d",0x7fffffff); fclose(fp); }
  fp = fopen("/proc/sys/kernel/shmmni","w");
  if( fp ) { fprintf(fp,"%d",0x7fffffff); fclose(fp); }
}

int Db::RootInfo::
init()
{
  root_magic = Db::root_magic;
  root_info_size = sizeof(RootInfo);
  last_info_size = 0;
  file_size = 0;
  root_info_addr = NIL;
  last_info_addr = NIL;
  transaction_id = 1;
  entity_max_id = 0;
  entity_count = 0;
  freePages = NIL;
  indeciesUsed = 0;
  pageTableUsed = 0;
  return 0;
}

void Db::
init()
{
  err_no = 0;
  err_callback = 0;

  storeBlockSize = defaultStoreBlockSize;
  entityPageSize = defaultEntityPageSize;
  pageTableHunkSize = defaultPageTableHunkSize;
  indexTableHunkSize = defaultIndexTableHunkSize;

  root_info_id = -1;   root_info = 0;
  index_info_id = -1;  index_info = 0;
  indecies = 0;        indeciesAllocated = 0;   indecies_sz = 0;
  page_info_id = -1;   page_info = 0;
  pageTable = 0;       pageTableAllocated = 0;  page_table_sz = 0;

  file_position = -1;
  alloc_cache.init();
  bfr = lmt = inp = 0;
  active_transaction = -1;
}

void Db::
deinit()
{
  if( indecies ) {
    for( int idx=indecies_sz; --idx>=0; ) delete indecies[idx];
    delete [] indecies;  indecies = 0;
  }
  del_uint8_t(index_info);  index_info = 0;
  indeciesAllocated = 0;    indecies_sz = 0;
  index_info_id = -1;

  if( pageTable ) {
    for( pageId id=0; id<page_table_sz; ++id ) {
      Page *pp = get_Page(id);  pageDealloc(*pp);  delete pp;
    }
    delete [] pageTable;  pageTable = 0;
  }
  del_uint8_t(page_info);  page_info = 0;
  pageTableAllocated = 0;  page_table_sz = 0;
  page_info_id = -1;

  del_uint8_t(root_info);  root_info = 0;
  root_info_id = -1;
}

void Db::
reset()
{
  no_shm = defaultNoShm;
  get_mem = !no_shm ? &get_shm8_t : &get_mem8_t;
  new_mem = !no_shm ? &new_shm8_t : &new_mem8_t;
  del_mem = !no_shm ? &del_shm8_t : &del_mem8_t;
  root_info_id = index_info_id = page_info_id = -1;
  db_info = 0;
  shm_init = 0;
  fd = key = -1;
  init();
}

Db::
Db()
{
  my_pid = getpid();
  reset();
}

Db::
~Db()
{
  close();
}


#define Run(r,fn) \
  if( error() < 0 ) Fail(errPrevious); \
  if( r < 0 || r >= root_info->indeciesUsed || !indecies[r] ) Err(errInvalid); \
  return indecies[r]->fn;

int Db::
ins(int r, void *key, void *data)
{
  Run(r,Insert(key,data));
}

int Db::
del(int r, void *key)
{
  Run(r,Delete(key));
}

int Db::
find(int r, void *key, void *rtnData)
{
  Run(r,Find(key,rtnData));
}

int Db::
locate(int r, int op, void *key, CmprFn cmpr, void *rtnKey, void *rtnData)
{
  Run(r,Locate(op,key,cmpr,rtnKey,rtnData));
}

int Db::
first(int r, void *key, void *rtnData)
{
  Run(r,First(key,rtnData));
}

int Db::
last(int r, void *key, void *rtnData)
{
  Run(r,Last(key,rtnData));
}

int Db::
next(int r, void *key, void *rtnData)
{
  Run(r,Next(key,rtnData));
}

int Db::
nextloc(int r, pgRef &loc)
{
  Run(r,NextLoc(loc));
}




int Db::Entity::
allocate(ObjectLoc &loc, int sz)
{
  if( loc.entity != this ) Err(errObjEntity);
  int id = ent->id;
  int n = ent->recdSz + sz;
  if_err( db->allocate(id, n, loc.obj, ent->alloc_cache) );
  Obj *op = loc.addr();
  if( op ) {
    ent._wr();  loc._wr();
    memset(op, 0, n);
    op->id = ent->maxId;
  }
  return 0;
}

int Db::Entity::
construct_(ObjectLoc &loc, int id)
{
  int idx = ent->indexs[idxId];
  loc._wr();  loc->id = id;
  if_err( db->indecies[idx]->Insert(&id,&loc.obj) );
  ent._wr();  ++ent->count;
  if( id >= ent->maxId ) ent->maxId = id+1;
  return 0;
}


int Db::Entity::
destruct_(ObjectLoc &loc, int id)
{
  if_err( index(idxId)->Delete(&id) );
  ent._wr();  --ent->count;
  if( id+1 == ent->maxId ) {
    if( ent->count > 0 ) {
      if_err( index(idxId)->Last(&id,0) );
      ++id;
    }
    else
      id = 0;
    ent->maxId = id;
  }
  return 0;
}


int Db::
new_entity_(Entity &entity, const char *nm, int sz)
{
  // construct Entity
  EntityLoc &ent = entity.ent;
  // construct EntityObj
  if( root_info->entity_max_id >= max_entity_type ) Err(errLimit);
  if_err( allocate(root_info->entity_max_id+1,
      sizeof(EntityObj), ent.obj, alloc_cache) );
  int id = root_info->entity_max_id;
  ent._wr();  ent->id = id;
  char name[nmSz];  memset(&name[0],0,sizeof(name));
  strncpy(name,nm,sizeof(name)-1);
  memmove(&ent->name[0],name,sizeof(name));
  if_err( ent->alloc_cache.init_idx(this,name) );
  ent->maxId = 0;
  ent->recdSz = sz;
  ent->count = 0;
  // add to entity indecies
  if_err( entityIdIndex->Insert(&id,&ent.obj) );
  if_err( entityNmIndex->Insert(&name[0],&id) );
  // create entity id/loc
  int idx = new_binary_index(nm, sizeof(int),sizeof(pgRef),cmprOIds);
  if_err( idx );
  ent->indexs[idxId] = idx;
  ent->nidxs = 1;
  ++root_info->entity_count;
  ++root_info->entity_max_id;
  entity.rw_lock = &db_info->rw_locks[id];
  return 0;
}

int Db::
del_entity(Entity &entity)
{
  EntityLoc &ent = entity.ent;
  if( ent.obj.id >= 0 ) {
    ent.cacheFlush();
    ObjectLoc loc(&entity);
    int status = loc.FirstId();
    if( !status ) do {
      loc.v_del();
      entity.deallocate(loc.obj);
    } while( !(status=loc.NextId()) );
    if( status != errNotFound )
      if_err( status );
    cacheDelete(ent->alloc_cache);
    for( int i=ent->nidxs; --i>=0; ) entity.del_index_(i);
    int id = ent->id;
    entityIdIndex->Delete(&id);
    entityNmIndex->Delete(&ent->name[0]);
    if_err( deallocate(ent.obj, alloc_cache) );
    ent.obj.id = NIL;
    --root_info->entity_count;
    if( id+1 == root_info->entity_max_id ) {
      if( root_info->entity_count > 0 ) {
        if_err( entityIdIndex->Last(&id,0) );
        ++id;
      }
      else
        id = 0;
      root_info->entity_max_id = id;
    }
  }
  return 0;
}

int Db::
new_entity(Entity &entity, const char *nm, int sz)
{
  int ret = new_entity_(entity, nm, sz);
  if( ret ) del_entity(entity);
  return ret;
}

int Db::
get_entity(Entity &entity, const char *nm)
{
  EntityLoc &ent = entity.ent;
  char name[nmSz];  memset(&name[0],0,sizeof(name));
  strncpy(name,nm,sizeof(name)-1);  int id;
  if_fail( entityNmIndex->Find(&name[0], &id) );
  if_err( entityIdIndex->Find(&id, &ent.obj) );
  entity.rw_lock = &db_info->rw_locks[id];
  return 0;
}

int Db::Entity::
get_index(const char *nm, CmprFn cmpr)
{
  int idx = ent->nidxs;
  while( --idx >= 0  ) {
    int i = ent->indexs[idx];
    if( i < 0 ) continue;
    IndexBase *ib = db->indecies[i];
    if( ib && !strncmp(&ib->st->name[0],nm,nmSz) ) {
      if( cmpr && ib->st->type == idxBin ) {
        IndexBinary *bidx = (IndexBinary *)ib;
        bidx->compare = cmpr;
        bidx->bst->cmprId = db->findCmprFn(cmpr);
      }
      break;
    }
  }
  if( idx < 0 ) Fail(errNotFound);
  return idx;
}

int Db::Entity::
add_index(int idx, int kty)
{
  EntityLoc nent(this);
  // construct EntityObj
  int nidx = ent->nidxs;
  int sz = sizeof(EntityObj) + sizeof(ent->indexs[0])*nidx;
  if_err( db->allocate(ent->id, sz, nent.obj, db->alloc_cache) );
  nent._wr();  nent->id = ent->id;
  memmove(&nent->name[0],&ent->name[0],nmSz);
  nent->alloc_cache = ent->alloc_cache;
  nent->maxId = ent->maxId;
  nent->recdSz = ent->recdSz;
  nent->count = ent->count;
  // add to entity indecies
  if_err( db->entityIdIndex->Modify(&ent->id,&nent.obj) );
  for( int i=0; i<nidx; ++i )
    nent->indexs[i] = ent->indexs[i];
  // add new index
  nent->indexs[nidx] = idx;
  nent->nidxs = nidx+1;
  if_err( db->deallocate(ent.obj, db->alloc_cache) );
  ent.obj = nent.obj;
  IndexBase *ib = db->indecies[idx];
  ib->st->key_type = kty;
  return 0;
}

int Db::Entity::
del_index(const char *nm)
{
  int idx;  if_ret( idx = get_index(nm) );
  return del_index(idx);
}

int Db::Entity::
del_index(int i)
{
  if( i <= idxId ) Fail(errInvalid);
  if( i >= ent->nidxs ) Fail(errInvalid);
  return del_index_(i);
}

int Db::Entity::
del_index_(int i)
{
  int idx = ent->indexs[i];
  if( idx < 0 ) Fail(errNotFound);
  if( idx >= db->root_info->indeciesUsed ) Fail(errNotFound);
  if( !db->indecies[idx] ) Fail(errNotFound);
  ent._wr();  ent->indexs[i] = -1;
  db->indecies[idx]->Clear();
  db->del_index(idx);
  return 0;
}

void Db::
finit(Objects objects)
{
  while( objects ) {
    Objects op = objects;  objects = op->next;
    Entity *ep = op->obj->entity;
    for( varObjs nxt=0, vp=ep->vobjs; vp!=0; vp=nxt ) {
       nxt = vp->next;  delete vp;
    }
    delete op;
  }
}

void Db::ObjectLoc::
v_init()
{
  for( varObjs vp = entity->vobjs; vp!=0; vp=vp->next ) {
    Obj *op = addr();
    (op->*(vp->ref)).init();
  }
}

void Db::ObjectLoc::
v_del()
{
  for( varObjs vp = entity->vobjs; vp!=0; vp=vp->next ) {
    Obj *op = addr();
    (op->*(vp->ref)).del(entity);
  }
}

// resize varObj
int Db::ObjectLoc::
size(varObj &vobj, int sz)
{
  if( vobj.len != sz ) {
    AllocCache &cache = entity->ent->alloc_cache;
    if_ret( entity->db->reallocate(sz,vobj.loc,cache) );
    vobj.len = sz;  entity->ent._wr();  _wr();
  }
  return 0;
}


int Db::ObjectLoc::
last(Index idx, ObjectLoc &last_loc)
{
  int id = -1;
  if_ret( idx->Last(0,&id) );
  if_err( last_loc.FindId(id) );
  return 0;
}

// get last index id on member accessed with ip
int Db::ObjectLoc::
last(const char *nm,int (ObjectLoc::*ip)())
{
  Index idx = entity->index(nm);
  if( !idx ) Err(errInvalid);
  ObjectLoc last_loc(*this);
  int ret = last(idx, last_loc);
  if( ret == errNotFound ) return 0;
  if_err( ret );
  return (last_loc.*ip)();
}

unsigned int Db::ObjectLoc::
last(const char *nm,unsigned int (ObjectLoc::*ip)())
{
  Index idx = entity->index(nm);
  if( !idx ) Err(errInvalid);
  ObjectLoc last_loc(*this);
  int ret = last(idx, last_loc);
  if( ret == errNotFound ) return 0;
  if_err( ret );
  return (last_loc.*ip)();
}


#define cmpr_type(nm,ty) int Db::ObjectLoc:: \
nm(const ty *ap, int asz, const ty *bp, int bsz) { \
  int n = asz < bsz ? asz : bsz; \
  while( n>0 ) { \
    if( *ap > *bp ) return 1; \
    if( *ap < *bp ) return -1; \
    ++ap;  ++bp;  n -= sizeof(ty); \
  } \
  if( asz > bsz ) return 1; \
  if( asz < bsz ) return -1; \
  return 0; \
}

cmpr_type(cmpr_char, char)
cmpr_type(cmpr_uchar, unsigned char)
cmpr_type(cmpr_short, short)
cmpr_type(cmpr_ushort, unsigned short)
cmpr_type(cmpr_int, int)
cmpr_type(cmpr_uint, unsigned int)
cmpr_type(cmpr_long, long)
cmpr_type(cmpr_ulong, unsigned long)
cmpr_type(cmpr_float, float)
cmpr_type(cmpr_double, double)

#ifdef ZMEDIA
int Db::ObjectLoc::
cmpr_media(const unsigned char *ap, int asz, const unsigned char *bp, int bsz)
{
  return mediaCmpr((uint8_t *)ap).cmpr((uint8_t *)bp);
}
#endif

#define KeyFn(fn) { \
  int id = -1; \
  if_fail( idx->fn ); \
  if_err( loc.FindId(id) ); \
  return 0; \
}

int Db::iKey::Find() KeyFn(Find(*this, &id))
int Db::iKey::Locate(int op) KeyFn(Locate(op, *this,0, 0,&id))
int Db::rKey::First() KeyFn(First(0,&id))
int Db::rKey::Last() KeyFn(Last(0,&id))
int Db::rKey::Next() KeyFn(Next(this,&id))
int Db::rKey::First(pgRef &pos) KeyFn(First(pos,0,&id))
int Db::rKey::Next(pgRef &pos) KeyFn(Next(pos,this,&id))
int Db::rKey::Locate(int op) KeyFn(Locate(op, *this,0, 0,&id))

int Db::ioCmpr(const void *a, const void *b, void *c)
{
  Db *db = (Db *)c;
  Page &pa = *db->get_page(*(pageId*)a);
  Page &pb = *db->get_page(*(pageId*)b);
  int64_t v = pa->io_addr - pb->io_addr;
  return v < 0 ? -1 : v > 0 ? 1 : 0;
}

int Db::load()
{
  int npages = root_info->pageTableUsed;
  pageId *pages = new pageId[npages];
  for( int i=0 ; i<npages; ++i ) pages[i] = i;
  qsort_r(pages, npages, sizeof(*pages), ioCmpr, this);
  for( int i=0 ; i<npages; ++i ) {
    pgRef loc;  char *op = 0;
    loc.id = pages[i];  loc.offset = 0;
    if_err( addrRead(loc, op) );
  }
  delete [] pages;
  return 0;
}

int Db::load_indecies()
{
  for( int i=0 ; i<indecies_sz; ++i ) {
    Index idx = indecies[i];
    if( !idx || idx->st->rootPageId < 0 ) continue;
    if_err( idx->keyMap(idx->st->rootPageId, &Db::IndexBase::blockLoad) );
  }
  return 0;
}

