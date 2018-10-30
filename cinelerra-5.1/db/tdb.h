#ifndef __DB_H__
#define __DB_H__
#include <cstring>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>

#define entityIdIndex indecies[entityIdIdx]
#define entityNmIndex indecies[entityNmIdx]
#define freeStoreIndex indecies[freeStoreIdx]
#define addrStoreIndex indecies[addrStoreIdx]
#define freeSpaceIndex indecies[cache.freeIdx]
#define addrSpaceIndex indecies[cache.addrIdx]

#define noThrow std::nothrow
#ifndef likely
#define likely(x)   (__builtin_constant_p(x) ? !!(x) : __builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_constant_p(x) ? !!(x) : __builtin_expect(!!(x), 0))
#endif
#ifndef lengthof
#define lengthof(x) ((int)(sizeof(x)/sizeof(x[0])))
#endif

#if 0
inline void *operator new(size_t n) { void *vp = malloc(n); bzero(vp,n); return vp; }
inline void operator delete(void *t) { free(t); }
inline void operator delete(void *t,size_t n) { free(t); }
inline void *operator new[](size_t n) { void *vp = malloc(n); bzero(vp,n); return vp; }
inline void operator delete[](void *t) { free(t); }
inline void operator delete[](void *t,size_t n) { free(t); }
#endif

#define ZMEDIA
#define ZFUTEX
#ifdef ZFUTEX
#include <unistd.h>
#include <endian.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#else
#include <pthread.h>
#endif

#if 1
//entity definitions
#define DbObj(nm) \
  class nm##Obj : public Db::Obj { public:

#define DbLoc(nm) \
  class nm##Loc : public Db::ObjectLoc { public: \
    nm##Obj *operator ->() { return (nm##Obj *)addr(); } \
    nm##Loc(Db::Entity *ep) : Db::ObjectLoc(ep) {} \
    nm##Loc(Db::Entity &e) : Db::ObjectLoc(&e) {}

//basic definitions
#define basic_def(ty,n) class t_##n { public: ty v; t_##n() {} \
 t_##n(const ty &i) : v(i) {} \
 t_##n(const t_##n &i) : v(i.v) {} \
 t_##n &operator =(const t_##n &i) { v = i.v; return *this; } \
 ty &operator =(const ty &i) { return v = i; } \
 ty *addr() { return &v; } int size() { return sizeof(v); } \
 } v_##n \

//array definitions
#define array_def(ty,n,l) class t_##n { public: ty v[l]; t_##n() {} \
 t_##n(const t_##n &i) { memcpy(&v,&i.v,sizeof(v)); } \
 t_##n(ty *i) { memcpy(&v,i,sizeof(v)); } \
 t_##n(ty(*i)[l]) { memcpy(&v,i,sizeof(v)); } \
 ty *operator =(const ty *i) { memcpy(&v,i,sizeof(v)); return &v[0]; } \
 ty *addr() { return &v[0]; } int size() { return sizeof(v); } \
 } v_##n \

// variable array definitions
#define varray_def(ty,n) \
 class t_##n { public: char *v; int l; t_##n() {} \
 t_##n(const char *i, int sz) { v = (char *)i; l = sz; } \
 t_##n(const unsigned char *i, int sz) { v = (char *)i; l = sz; } \
 ty *addr() { return (ty *)v; } int size() { return l; } \
 }; Db::varObj v_##n \

// string array definitions
#define sarray_def(ty,n) \
 class t_##n { public: char *v; int l; t_##n() {} \
 t_##n(const char *i, int sz) { v = (char *)i; l = sz; } \
 t_##n(const unsigned char *i, int sz) { v = (char *)i; l = sz; } \
 t_##n(const char *i) { t_##n(i,strlen(i)+1); } \
 t_##n(const unsigned char *i) { t_##n(i,strlen(v)+1); } \
 ty *addr() { return (ty *)v; } int size() { return l; } \
 }; Db::varObj v_##n \

//basic type ref
#define basic_ref(ty,n) \
 ty *_##n() { return (*this)->v_##n.addr(); } \
 ty n() { return *_##n(); } \
 void n(ty i) { _wr(); *_##n() = i; } \
 int size_##n() { return (*this)->v_##n.size(); } \

//array type ref
#define array_ref(ty,n,l) \
 ty *_##n() { return (*this)->v_##n.addr(); } \
 ty (&n())[l] { return *(ty (*)[l])_##n(); } \
 void n(const ty *i,int m) { _wr(); if( m > 0 ) memcpy(n(),i,m); } \
 void n(const ty *i) { n(i,(*this)->v_##n.size()); } \
 int size_##n() { return (*this)->v_##n.size(); } \

//variable array type ref
#define varray_ref(ty,n) \
 ty *_##n() { return (ty *)addr((*this)->v_##n); } \
 ty *_##n(int sz) { size((*this)->v_##n, sz); \
   return sz > 0 ? (ty *)addr_wr((*this)->v_##n) : 0; } \
 ty (&n())[] { return *(ty (*)[])_##n(); } \
 int n(const ty *v, int sz) { ty *vp=_##n(sz); \
  if( vp && sz > 0 ) memcpy(vp, v, sz); return 0; } \
 int size_##n() { return (*this)->v_##n.size(); } \

//string array type ref
#define sarray_ref(ty,n) \
 ty *_##n() { return (ty *)addr((*this)->v_##n); } \
 ty *_##n(int sz) { size((*this)->v_##n, sz); \
   return sz > 0 ? (ty *)addr_wr((*this)->v_##n) : 0; } \
 ty (&n())[] { return *(ty (*)[])_##n(); } \
 int n(const ty *v, int sz) { ty *vp=_##n(sz); \
  if( vp && sz > 0 ) memcpy(vp, v, sz); return 0; } \
 int n(const char *v) { return n((ty *)v,strlen(v)+1); } \
 int n(const unsigned char *v) { return n((const char *)v); } \
 int size_##n() { return (*this)->v_##n.size(); } \

#endif

#define DEBUG
#define DEBUG_TIMESTAMPS
#define DBBUG_ERR   0x00000001
#define DBBUG_FAIL  0x00000002
#define CHK
//#define CHK 1 ? 0 :

class Db;

class Db {
public:
  typedef int pageId;
  typedef int transId;
  typedef long ioAddr;

private:
  int root_info_id;
  int index_info_id;
  int page_info_id;

  class RootInfo {
  public:
    int root_magic;           // info_magic label
    int root_info_size;       // root_info blob size
    int last_info_size;
    int entity_max_id;
    int entity_count;
    ioAddr root_info_addr;
    ioAddr last_info_addr;
    transId transaction_id;   // current transaction
    ioAddr file_size;         // current file size
    pageId freePages;         // free page table page list
    int indeciesUsed;         // number of active indecies
    int pageTableUsed;        // number of active pages

    int init();
  } *root_info;

  int shm_init, no_shm;

  static void *get_mem8_t(int id);
  static void *new_mem8_t(int size, int &id);
  static int del_mem8_t(const void *vp, int id);
  static void *get_shm8_t(int id);
  static void *new_shm8_t(int size, int &id);
  static int del_shm8_t(const void *vp, int id);
  void *(*get_mem)(int id);
  void *(*new_mem)(int size, int &id);
  int (*del_mem)(const void *vp, int id);

protected:
  uint8_t *get_uint8_t(int id, int pg=-1);
  uint8_t *new_uint8_t(int size, int &id, int pg=-1);
  int del_uint8_t(const void *vp, int id=-1, int pg=-1);

public:
  typedef int (*CmprFn)(char *,char *);
  static int cmprFrSt(char *a, char *b);
  static int cmprAdSt(char *a, char *b);
  static int cmprFrSp(char *a, char *b);
  static int cmprAdSp(char *a, char *b);
  static int cmprOIds(char *a, char *b);
  static int cmprStr(char *a, char *b);
  static int cmprKey(char *a, char *b);
  static int cmprLast(char *a, char *b);
  static CmprFn cmprFns[];
  typedef void (*errCallback)(Db *db, int v);
  static const pageId NIL=-1, DDONE=-2;
  enum {
    errNoCmprFn = 0,
    errNotFound = -1,
    errDuplicate = -2,
    errNoPage = -3,
    errNoMemory = -4,
    errIoRead = -5,
    errIoWrite = -6,
    errIoSeek = -7,
    errIoStat = -8,
    errBadMagic = -9,
    errCorrupt = -10,
    errInvalid = -11,
    errPrevious = -12,
    errObjEntity = -13,
    errLimit = -14,
    errUser = -15,
    idxId = 0, nmSz = 32,
    keysz = 256,
    keyLT=-2, keyLE=-1, keyEQ=0, keyGE=1, keyGT=2,
  };

  class pgRef {
  public:
    pageId id;
    int offset;
  };

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

  class zlock_t;
  class zblock_t;
  class zrwlock_t;

#ifdef ZFUTEX
#define ZLOCK_INIT zzlock_t()
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
    int zwait(int val, timespec *ts=0);
    int zwait() { return zwait(loc); }
    zloc_t() : loc(-1) {}
    ~zloc_t() {}
  };

  class zlock_t : zloc_t {
    void *owner;
    void *self() {
#ifdef __x86_64__
     void *vp; asm ("movq %%fs:%c1,%q0" : "=r" (vp) : "i" (16));
#else
     void *vp; asm ("mov %%fs:%c1,%q0" : "=r" (vp) : "i" (16));
#endif
     return vp;
   }
  protected:
    friend class zblock_t;
    friend class zrwlock_t;
    int zlock(int v);
    int zunlock(int nwakeups=1);
    static int zemsg1();
  public:
    int lock() {
      int v, ret = unlikely( (v=zcmpxchg(-1,0,loc)) >= 0 ) ? zlock(v) : 0;
      owner = self();
      return ret;
    }
    int unlock() {
      if( unlikely(loc < 0) ) { return zemsg1(); }
      owner = 0;
      int v, ret = unlikely( (v=zcmpxchg(0,-1,loc)) != 0 ) ? zunlock() : 0;
      return ret;
    }
    zlock_t() { owner = 0; }
    ~zlock_t() {}
  };

  class zblock_t : zlock_t {
  public:
    void block() { loc = 0; zwait(0); }
    void unblock() { loc = -1; zwake(INT_MAX); }
    zblock_t() {}
    ~zblock_t() {}
  };

  class zrwlock_t : zloc_t {
    zlock_t lk;
    void zenter();
    void zleave();
  public:
    void enter() { zincr(loc); if( unlikely( lk.loc >= 0 ) ) zenter(); }
    void leave() { if( unlikely( !tdecr(loc) ) ) zleave(); }
    void write_enter();
    void write_leave();
    int locked() { return loc >= 0 ? 0 : lk.loc >= 0 ? 1 : -1; }
    int blocked() { return lk.loc >= 0 ? 1 : 0; }
    zrwlock_t() {}
    ~zrwlock_t() {}
  };

#else
#define ZLOCK_INIT { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER }

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
    volatile int blocking, users;
    zlock_t lk;
    pthread_cond_t cond;
    void wait() { pthread_cond_wait(&cond, &zlock); }
    void wake() { pthread_cond_signal(&cond); }
  public:
    void enter() { lock();
      while( blocking ) { unlock(); lk.lock(); lk.unlock(); lock(); }
      ++users;  unlock();
    }
    void leave() { lock();  if( !--users && blocking ) wake();  unlock(); }
    void write_enter() { lk.lock();  blocking = 1;
      lock(); while( users ) wait(); unlock();
    }
    void write_leave() { blocking = 0; lk.unlock(); }
    int count() { return users; }
    int locked() { return users ? 0 : blocking ? 1 : -1; }
    int blocked() { return blocking; }

    zrwlock_t() { pthread_cond_init(&cond, 0); users = 0; blocking = 0; }
    ~zrwlock_t() { pthread_cond_destroy(&cond); }
  };

#endif

  class locked {
    zlock_t &lk;
  public:
    locked(zlock_t &l) : lk(l) { lk.lock(); }
    ~locked() { lk.unlock(); }
  };

  class read_locked {
    zrwlock_t &rwlk;
  public:
    read_locked(zrwlock_t &l) : rwlk(l) { rwlk.enter(); }
    ~read_locked() { rwlk.leave(); }
  };

  class write_blocked {
    zrwlock_t &rwlk;
  public:
    write_blocked(zrwlock_t &l) : rwlk(l) { rwlk.write_enter(); }
    ~write_blocked() { rwlk.write_leave(); }
  };

private:
  enum {
    db_magic_offset=0,
    db_magic=0x00624474,   // tDb
    idx_magic=0x00786469,  // idx
    info_magic=0x6f666e69, // info
    root_magic=0x746f6f72, // root
    page_magic=0x6770,     // pg
    entity_magic = 0x6d65, // em
    root_info_offset=4,
    root_info_extra_pages = 2,
    idxNil=0, idxBin=1, idxStr=2,
    ktyBin=0, ktyInd=1, ktyDir=2,
    opDelete=-1, opFind=0, opInsert=1,
    pg_unknown=0, pg_root_info=1, pg_free=2,
    pg_entity=0x0100, pg_index=0x1000,
    max_entity_type = pg_index-pg_entity-1,
    max_index_type = 0x10000-pg_index-1,
    min_heap_allocation = 32,
    entityIdIdx = 0, entityNmIdx = 1,
    freeStoreIdx = 2, addrStoreIdx = 3,
    freeSpaceIdx = 4, addrSpaceIdx = 5,
    usrIdx = 6,
    fl_wr=1, fl_rd=2, fl_new=4, fl_free=8,
    defaultNoShm = 1,
    defaultStoreBlockSize = 8192,
    defaultPageTableHunkSize = 8192,
    defaultIndexTableHunkSize = 4096,
    defaultBinaryBlockSize = 16384,
    defaultStringBlockSize = 4096,
    defaultEntityPageSize = 65536,
  };

  int key, fd;
  int err_no;
  errCallback err_callback;
  int my_pid;
  transId active_transaction;

#ifdef DEBUG
  static int debug;
  static const char *errMsgs[];
  static void dmsg(int msk, const char *msg,...);
  int _err_(int v,const char *fn,int ln);
  int _fail_(int v,const char *fn,int ln);
#define err_(v) _err_(v,__func__,__LINE__)
#define fail_(v) _fail_(v,__func__,__LINE__)
#else
  static void dmsg(int msk, const char *msg,...) {}
  int _err_(int v) { error(v); return v; }
  int _fail_(int v) { return v; }
#define err_(v) _err_(v)
#define fail_(v) _fail_(v)
#endif

#define Err(v) return err_(v)
#define Fail(v) return fail_(v)

#define if_ret(fn) do{ int _ret; \
 if(unlikely((_ret=(fn))<0)) return _ret; \
}while(0)
#define if_err(fn) do{ int _ret; \
 if(unlikely((_ret=(fn))<0)) Err(_ret); \
}while(0)
#define if_fail(fn) do{ int _ret; \
 if(unlikely((_ret=(fn))<0)) Fail(_ret); \
}while(0)

  class DbInfo {
  public:
    int magic;
    int owner, last_owner;
    int info_key, info_id;
    int root_info_id;
    int index_info_id;
    int page_info_id;
    zlock_t infoLk;   // lock dbinfo up to here
    zrwlock_t dbRwLk; // global lock
    zrwlock_t pgTblLk;// pageTable realloc
    zlock_t pgAlLk;   // new page pagesUsed/pagesAllocated
    zlock_t pgLdLk;   // pageLoad
    zlock_t blkAlLk;  // blockAllocate/Free
    zlock_t objAlLk;  // objectAllocate/Free
    zrwlock_t rw_locks[max_entity_type];

    DbInfo(int pid, int key, int id);
  } *db_info;
  int new_info(int key);
  int get_info(int key);
  int del_info();

  int attach_wr();
  int attach_rd();
  int attach_rw(int zrw) { return zrw ? attach_wr() : attach_rd(); }
  void detach_rw();

  class Page;

  class pagePrefix {
  public:
    unsigned short magic;
    unsigned short type;
    int used;
  };

  Page *get_Page(pageId pid) volatile { return pageTable[pid]; }
  void set_Page(pageId pid, Page *pp) { pageTable[pid] = pp; }
  //Page *get_page(pageId pid) { blocked by(pgTblk); return get_Page(pid); }
  Page *get_page(pageId pid); // locked pageTable access

  static pageId getPageId(unsigned char *&bp) {
    int i = sizeof(pageId);  pageId id;
    for( id = *bp++; --i > 0; id |= *bp++ ) id <<= 8;
    return id;
  }
  static void putPageId(unsigned char *&bp, pageId id) {
    int i = sizeof(pageId) * 8;
    while( (i -= 8) >= 0 ) *bp++ = id >> i;
  }
  static pageId readPageId(char *cp) {
    unsigned char *bp = (unsigned char *)cp;
    return getPageId(bp);
  }
  static void writePageId(char *cp, pageId id) {
    unsigned char *bp = (unsigned char *)cp;
    putPageId(bp,id);
  }

  class keyBlock : public pagePrefix {
    char rightLink[sizeof(pageId)];
  public:
    int right_link() { return readPageId(&rightLink[0]); }
    void right_link(pageId id) { writePageId(&rightLink[0],id); }
  };

  static int defaultBlockSizes[];

  class IndexTypeInfo {
  public:
    int magic;
    short type;                    /* type of index */
    short key_type;
    char name[nmSz];               /* index string identifier */
  };

  class IndexBaseType : public IndexTypeInfo {
  public:
    IndexBaseType(int typ);
  };

  class IndexRecdInfo {
  public:
    int idx;                       /* index in db->indecies[] */
    int keySz, dataSz;             /* sizeof key/data fields in bytes */
    pageId rootPageId;             /* index root page ID */
    pageId rightHandSide;          /* the right hand side of the tree for this index */
    pageId freeBlocks;             /* free index page list */
    unsigned int blockSize;        /* size of new index blocks */
    int pad1;
    long count;                    /* index population count */
  };

  class IndexBaseRecd : public IndexRecdInfo {
  public:
    IndexBaseRecd(int typ, int zidx, int ksz, int dsz);
  };

  class IndexBaseStorage;
  class IndexBaseInfo : public IndexTypeInfo, public IndexRecdInfo {
  public:
    operator IndexBaseStorage *() { return (IndexBaseStorage *)this; }
  };
 
  class IndexBaseStorage : public IndexBaseInfo { 
  public:
    IndexBaseStorage(int typ, int zidx, int ksz, int dsz);
    IndexBaseStorage() {}
    ~IndexBaseStorage() {}
  };

  class IndexBase {
    IndexBaseStorage *st;
    friend class Db;
    Db *db;                        /* owner db */
    zlock_t idxLk;
    pgRef lastAccess, lastFind;    /* last operational access/find location */
    pgRef lastInsert, lastDelete;  /* last operational insert/delete location */
    pgRef lastNext;                /* last operational next location */
    int kdSz;                      /* keySz + dataSz */
    int lastOp;                    /* last operation, delete=-1/find=0/insert=1 */
    int cInsCount;                 /* number of consecutive insertions */
    int cFindCount;                /* number of consecutive finds */
    int cDelCount;                 /* number of consecutive deletions */

    void init(Db *zdb);
    virtual int keyMap(pageId s, int(IndexBase::*fn)(pageId id)) = 0;
    virtual int keyCopy(pageId s, IndexBase *ib) = 0;
    int blockAllocate(pageId &pid, keyBlock *&bp);
    int blockAllocate(pageId &pid, keyBlock *&bp, Page *&pp, char *&cp) {
      pp = 0;  cp = 0;  if_err( blockAllocate(pid, bp) );
      pp = db->get_page(pid);  cp = (char *)(bp + 1);
      return 0;
    }
    int blockFree(pageId pid);
    int blockRelease(pageId pid);
    int blockLoad(pageId pid);
    int deleteFreeBlocks();
    void chkLastReset();
    void chkLastInsert();
    void chkLastDelete();
    void chkLastFind(pgRef &last);
  public:
#ifdef DEBUG
    int _err_(int v,const char *fn,int ln) { return db->_err_(v,fn,ln); }
    int _fail_(int v,const char *fn,int ln) { return db->_fail_(v,fn,ln); }
#else
    int _err_(int v) { return db->_err_(v); }
    int _fail_(int v) { return db->_fail_(v); }
#endif
    virtual int Locate(int op,void *key,CmprFn cmpr,void *rtnKey,void *rtnData) = 0;
    virtual int Find(void *key,void *rtnData) = 0;
    virtual int Insert(void *key,void *data) = 0;
    virtual int Delete(void *key) = 0;
    virtual int First(void *rtnKey,void *rtnData) = 0;
    virtual int Last(void *rtnKey,void *rtnData) = 0;
    virtual int Modify(void *key,void *recd) = 0;
    virtual int Next(pgRef &loc,void *rtnKey,void *rtnData) = 0;
    int First(pgRef &loc,void *rtnKey,void *rtnData) {
      if_fail( First(rtnKey, rtnData) );
      loc = lastNext;
      return 0;
    }
    int Next(void *rtnKey,void *rtnData) {
      if( lastNext.id < 0 ) Fail(errInvalid);
      return Next(lastNext,rtnKey,rtnData);
    }
    long Count() { return st->count; }
    int MakeRoot();
    int UnmakeRoot();
    int Clear();
    int NextLoc(pgRef &loc) { loc = lastNext; return 0; }
    IndexBase(Db *zdb, int typ, int zidx, int ksz, int dsz);
    IndexBase(Db *zdb, IndexBaseStorage &d);
    virtual ~IndexBase();
  } **indecies;

  int indeciesAllocated, indecies_sz;

  class IndexBinaryStorage;
  class IndexBinaryInfo {
  public:
    operator IndexBinaryStorage *() { return (IndexBinaryStorage *)this; }
    int cmprId;
  };

  class IndexBinaryStorage : public IndexBinaryInfo {
  public:
    IndexBinaryStorage(int cmprId) { this->cmprId = cmprId; }
    IndexBinaryStorage() {}
    ~IndexBinaryStorage() {}
  };

  class IndexBinary : public IndexBase {
    IndexBinaryStorage *bst;
    friend class Db;
    CmprFn compare;                /* the key compare function type */
    char *akey;                    /* pointer to key argument */
    int keyInterior;               /* last insert interior/exterior */
    int idf;                       /* interior delete flag */
    char *iky, *tky;               /* search/promoted temp key storage */

    void init();
    int keyMap(pageId s, int(IndexBase::*fn)(pageId id));
    int keyCopy(pageId s, IndexBase *ib);
    int keyBlockUnderflow(int &t,keyBlock *lbb,pageId p,keyBlock *pbb,int pi);
    void makeKey(char *cp,char *key,int l,char *recd,int n);
    void setLastKey(pageId s,pageId u,int k);
    int keyLocate(pgRef &last,pageId s, int op,void *ky, CmprFn cmpr);
    int chkNext(pgRef &loc, char *&kp);
    int keyNext(pgRef &loc, char *kp);
    int chkFind(pgRef &loc, char *key);
    int keyFind(pgRef &loc,void *ky, pageId s);
    int chkInsert(void *key,void *data);
    int keyInsert(pageId s, pageId &t);
    int chkDelete(pgRef &loc, void *kp);
    int keyDelete(int &t,void *kp,pageId s,pageId p,keyBlock *pbb,int pi);
    int keyFirst(pgRef &loc, pageId s);
    int keyLast(pgRef &loc, pageId s);

    int refLocate(pgRef &loc, int op,void *key, CmprFn cmpr);
    int Locate(int op,void *key,CmprFn cmpr,void *rtnKey,void *rtnData);
    int refFind(pgRef &loc, void *key);
    int Find(void *key,void *rtnData);
    int Insert(void *key,void *data);
    int Delete(void *key);
    int First(void *rtnKey,void *rtnData);
    int Last(void *rtnKey,void *rtnData);
    int Modify(void *key,void *recd);
    int Next(pgRef &loc,void *rtnKey,void *rtnData);
    int Next(void *rtnKey,void *rtnData) {
      return IndexBase::Next(rtnKey,rtnData);
    }
    void wr_key(void *kp, char *bp, int sz) {
      switch( st->key_type ) {
      case ktyBin: memcpy(bp,kp,sz); break;
      case ktyDir:
      case ktyInd: ((Key *)kp)->wr_key(bp); break;
      }
    }
    char *ikey() { return iky; }
    char *tkey() { return tky; }
  public:
    IndexBinary(Db *zdb, int zidx, int ksz, int dsz, CmprFn cmpr);
    IndexBinary(Db *zdb, IndexBaseStorage *b, IndexBinaryStorage *d);
    IndexBinary(IndexBase *ib, IndexBaseStorage *b, IndexBinaryStorage *d);
    ~IndexBinary();
  };
  friend class IndexBinary;

  class IndexStringStorage;
  class IndexStringInfo {
    char dummy; // compiler needs this for some reason
  public:
    operator IndexStringStorage *() { return (IndexStringStorage *)this; }
  };

  class IndexStringStorage : public IndexStringInfo {
  public:
    IndexStringStorage() {}
    ~IndexStringStorage() {}
  };

  class IndexString : public IndexBase {
    IndexStringStorage *sst;
    friend class Db;
    static int ustrcmp(unsigned char *a, unsigned char *b) {
      return strncmp((char *)a,(char *)b,keysz);
    }
    static void ustrcpy(unsigned char *a, unsigned char *b) {
      strncpy((char *)a,(char *)b,keysz);
    }
    static void umemmove(unsigned char *&a, unsigned char *b, int n) {
      memmove(a,b,n);  a += n;
    }
    static int kpress(unsigned char *kp, unsigned char *lp, unsigned char *cp);
    int split(int n, int i, pageId s, pageId &l, pageId r);
    void init();
    int keyMap(pageId s, int(IndexBase::*fn)(pageId id));
    int keyCopy(pageId s, IndexBase *ib);
    int chkInsert(void *key,void *data);
    int keyInsert(pageId &t, pageId s);
    int keyFirst(pageId s);
    int keyLast(pageId s);
    int keyLocate(pgRef &last,pageId s,int &t, int op,
        unsigned char *ky,CmprFn cmpr, unsigned char *rky);
    int chkFind(pgRef &loc, char *key, unsigned char *lkey, unsigned char *lky=0);
    int keyFind(pgRef &loc, unsigned char *ky);
    int keyNext(pgRef &loc, unsigned char *rky);
    int keyUnderflow(pageId s, pageId &t, int k);
    int keyOverflow(pageId s, pageId &t, int k, int o);
    int keyRemap(pageId s, pageId &t, int k, int o);
    int keyDelete(pageId s, pageId &t);

    unsigned char lastAccKey[keysz], lastFndKey[keysz];
    unsigned char lastInsKey[keysz], lastDelKey[keysz];
    unsigned char lastNxtKey[keysz];
    unsigned char *tky, *dky;   // dataSz+keysz+1
    unsigned char *tbfr;        // 3*allocated
    unsigned char akey[keysz];  // key in use
    int idf;                    /* interior delete flag */

    int refLocate(pgRef &loc, int op, void *key, CmprFn cmpr, unsigned char *rkey);
    int Locate(int op,void *key,CmprFn cmpr,void *rtnKey,void *rtnData);
    int refFind(pgRef &loc, void *key);
    int Find(void *key,void *rtnData);
    int Insert(void *key,void *data);
    int Delete(void *key);
    int First(void *rtnKey,void *rtnData);
    int Last(void *rtnKey,void *rtnData);
    int Modify(void *key,void *recd);
    int Next(pgRef &loc,void *rtnKey,void *rtnData);
    int Next(void *rtnKey,void *rtnData) {
      return IndexBase::Next(rtnKey,rtnData);
    }

  public:
    IndexString(Db *zdb, int zidx, int dsz);
    IndexString(Db *zdb, IndexBaseStorage *b, IndexStringStorage *d);
    IndexString(IndexBase *ib, IndexBaseStorage *b, IndexStringStorage *d);
    ~IndexString();
  };
  friend class IndexString;

  class IndexBinaryData : public IndexBaseInfo, public IndexBinaryInfo {};
  class IndexStringData : public IndexBaseInfo, public IndexStringInfo {};

  typedef union {
    IndexBaseInfo base;
    IndexBinaryData bin;
    IndexStringData str;
  } IndexInfo;
  IndexInfo *index_info;        /* image for index storage */

  class allocPrefix {
  public:
    unsigned short magic;
    unsigned short type;
    int size;
  };

  class freeStoreRecord {
  public:
    long size;
    ioAddr io_addr;
  };

  class addrStoreRecord {
  public:
    ioAddr io_addr;
    long size;
  };

  class freeSpaceRecord {
  public:
    long size;
    pageId id;
    int offset;
  };

  class addrSpaceRecord {
  public:
    pageId id;
    int offset;
    long size;
  };

  class AllocCache {
    pgRef loc;
    int avail;
    friend class Db;
  public:
    int cacheFlush(Db *db);
    int Get(Db *db,int &size, pgRef &ref);
    int Load(Db *db, pageId id, int ofs, int sz);
    void init() { loc.id = NIL; loc.offset = 0; avail = 0; }
    void init(pageId id, int ofs, int sz) {
      loc.id = id; loc.offset = ofs; avail = sz;
    }
    void dmp() {
      printf("loc: %d/%d  avl: %d\n", loc.id,loc.offset,avail);
    }
    int freeIdx, addrIdx;
    int init_idx(Db *db, const char *nm);
  } alloc_cache;

  int cacheFlush() { return alloc_cache.cacheFlush(this); }
  void cacheDelete(AllocCache &cache);
  int cache_all_flush();

  class PageStorage {
  public:
    unsigned int used;
    unsigned int allocated;
    unsigned short flags;
    unsigned short type;
    int io_allocated;
    int wr_allocated;
    int shmid;
    pageId link;
    transId trans_id;
    ioAddr io_addr;
    ioAddr wr_io_addr;

    void init();
    PageStorage() { init(); }
    ~PageStorage() {}
    int chk_flags(int fl) { return flags & fl; }
    int set_flags(int fl) { return flags |= fl; }
    int clr_flags(int fl) { return flags &= ~fl; }
  } *page_info;

  class Page {
    zlock_t pgLk;
    PageStorage *st;
    pagePrefix *addr;
    int shm_id;
    void init();
  public:
    // PageStorage access
    int iused() { return st->used-sizeof(keyBlock); }
    void iused(int v) { st->used = v+sizeof(keyBlock); }
    int iallocated() { return st->allocated-sizeof(keyBlock); }
    void iallocated(int v) { st->allocated = v+sizeof(keyBlock); }
    PageStorage *operator ->() { return st; }
    int release();
    void reset_to(Page *pp);
    Page(PageStorage &d) { st = &d; init(); }
    ~Page() {}
    friend class Db;
  } **pageTable;
  int pageTableAllocated, page_table_sz;

  int entityPageSize;
  int storeBlockSize;
  int pageTableHunkSize;
  int indexTableHunkSize;

  class undoData {
  public:
    class cfnData {
      public:
      int cmprId;
      CmprFn compare;
    } *cfn;
    int cfnAllocated, cfnUsed;
    undoData() : cfn(0), cfnAllocated(0), cfnUsed(0) {}
    ~undoData() { delete [] cfn; cfnAllocated = cfnUsed = 0; }
    int save(Db *db);
    int restore(Db *db);
  } undata;

  ioAddr file_position;

  char *bfr, *lmt, *inp;
  int open_bfr();
  int close_bfr();
  int flush_bfr();
  int write_bfr(char *dp, int sz);

public:
#ifdef ZMEDIA

  // count of on bits
  inline static unsigned int on_bits(unsigned int n) {
    n = (n & 0x55555555) + ((n >> 1) & 0x55555555);
    n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
    n = (n & 0x0f0f0f0f) + ((n >> 4) & 0x0f0f0f0f);
    n += n >> 8;  n += n >> 16;  //ok, fldsz > 5 bits
    return n & 0x1f;
  }

  // lowest on bit
  inline static unsigned int low_bit(unsigned int n) {
    return n & ~(n-1);
  }

  // bit number of lowest on bit
  inline static unsigned int low_bit_no(unsigned int n) {
    return on_bits(low_bit(n) - 1);
  }

  // highest on bit, and all lower bits set
  inline static unsigned int high_bit_mask(unsigned int n) {
    n |= n >> 1;   n |= n >> 2;
    n |= n >> 4;   n |= n >> 8;
    n |= n >> 16;
    return n;
  }

  // highest on bit
  inline static unsigned int high_bit(unsigned int n) {
    unsigned m = high_bit_mask(n);
    return m & ~(m>>1);
  }

  // bit number of highest on bit
  inline static unsigned int high_bit_no(unsigned int n) {
    return on_bits(high_bit_mask(n)) - 1;
  }

  class pack {
    static inline int cpu_aligned() {
      unsigned long flags;
      asm volatile( "pushf\n" "pop %0\n" : "=rm" (flags) );
      return (flags>>18) & 1;
    }
    static int aligned;
    int idx;
    uint8_t *bits;
    uint64_t vmx;
    uint64_t clip(int64_t v) { return v<0 ? 0 : (uint64_t)v>vmx ? vmx : v; }
  public:
    enum { alignBits = 8, };

    static void init(int v=-1) { aligned = v >= 0 ? v : cpu_aligned(); }
    void put(uint64_t v, int n);
    void putc(uint64_t v, int n) { put(clip(v), n); }
    void iput(int v) { put(v, 8*sizeof(int)); }
    void lput(int64_t v) { put(v, 8*sizeof(int64_t)); }
    uint64_t get(int n);
    int iget() { return get(8*sizeof(int)); }
    int64_t lget() { return get(8*sizeof(int64_t)); }
    int pos() { return idx; }
    void seek(int i) { idx = i; }
    void init(uint8_t *bp) { bits = bp; idx = 0; vmx = 0; }
    uint8_t *bfr() { return this->bits; }
    void align() { idx = (idx+alignBits-1) & ~(alignBits-1); }
    void *addr() { return &bits[idx/8]; }
    void set_max(uint64_t v) { vmx = v; }

    pack() : bits(0) { idx = 0; }
    pack(uint8_t *bp) { init(bp); }
    ~pack() {}
  };

  class mediaKey {
    int w, len;
    int64_t cnt;
    uint8_t *bp;

  public:
    static int64_t bit_size(int len, int w);
    static int64_t byte_size(int len, int w);
    static void build(uint8_t *kp, uint8_t *bp, int w, int len) {
      mediaKey key(kp, bp, w, len);
    }
    static int64_t count(void *kp) {
      mediaKey *mkp = (mediaKey *)kp;
      return be64toh(mkp->cnt);
    }
    static int64_t set_count(void *kp, int64_t v) {
      mediaKey *mkp = (mediaKey *)kp;
      return mkp->cnt = htobe64(v);
    }
    static int64_t incr_count(void *kp, int64_t dv) {
      return set_count(kp, count(kp) + dv);
    }
    static int64_t count1(uint8_t *kp);
    mediaKey(uint8_t *kp, uint8_t *bp, int w, int len);
    ~mediaKey();
  };

  class mediaLoad {
    pack pb, pk;
    int w, len, dsz, psz, spos;
    int64_t cnt, *dat, **dp;
    void get_fields(int n, int k);
    int dsize(int n) { int m = (n+1)/2; return m>1 ? m+dsize(m) : 1; }

  public:
    void load(uint8_t *kp);

    mediaLoad(uint8_t *bp);
    ~mediaLoad();
  };

  class mediaCmpr {
    pack pb, pk;
    int w, len, dsz, psz, spos;
    int64_t acnt, bcnt, *adat, *bdat, **adp, **bdp;
    uint64_t err, lmt;
    uint64_t sqr(int64_t v) { return v*v; }
    int dsize(int n) { int m = (n+1)/2; return m>1 ? m+dsize(m) : 1; }
    uint64_t chk_fields(int m, int k);
    int cmpr_fields(int m, int k);

  public:
    uint64_t chk(uint8_t *kp, uint64_t lmt=~0);
    int cmpr(uint8_t *kp, uint64_t lmt=~0);

    mediaCmpr(uint8_t *bp);
    ~mediaCmpr();
  };
#endif

  class Entity;
  class ObjectLoc;
  typedef IndexBase *Index;
  int new_entity(Entity &entity, const char *nm, int sz);
  int get_entity(Entity &entity, const char *nm);
  int del_entity(Entity &entity);

  class Obj {                    /* per object storage base class */
  public:
    int id;
  };

  class varObj {
  public:
    int len;  pgRef loc;
    int size() { return len; }
    void init() { len = -1;  loc.id = NIL;  loc.offset = 0; }
    void del(Entity *entity) { len = -1;  entity->deallocate(loc); }
  };
  typedef varObj Obj::*vRef;

  class ObjectLoc {
  public:
    Entity *entity;
    pgRef obj;
    Obj *addr(pgRef &loc) { 
      char *op = 0;
      return loc.id < 0 || entity->db->addrRead(loc,op) ? 0 : (Obj *)op;
    }
    Obj *addr_wr(pgRef &loc) { 
      char *op = 0;
      return loc.id < 0 || entity->db->addrWrite(loc,op) ? 0 : (Obj *)op;
    }
    void _wr() { Page &pg = *entity->db->get_page(obj.id); pg->set_flags(fl_wr); }
    Obj *addr() { return addr(obj); }
    Obj *addr_wr() { return addr_wr(obj); }
    void *addr(varObj &vobj) { return addr(vobj.loc); }
    void *addr_wr(varObj &vobj) { return addr_wr(vobj.loc); }
    Obj *operator ->() { return (Obj *)addr(); }
    ObjectLoc(Entity *ep) : entity(ep) { obj.id = NIL;  obj.offset = 0; }
    ~ObjectLoc() {}

    virtual int allocate(int sz=0) { return entity->allocate(*this,sz); }
    virtual int construct() { return entity->construct(*this); }
    virtual int destruct() { return entity->destruct(*this); }
    virtual int deallocate() { return entity->deallocate(*this); }
    virtual int insertCascade() { return 0; }
    virtual int insertProhibit() { return 0; }
    virtual int deleteCascade() { return 0; }
    virtual int deleteProhibit() { return 0; }
    virtual int modifyCascade() { return 0; }
    virtual int modifyProhibit() { return 0; }
    virtual int copy(ObjectLoc &dobj);
    int id() { ObjectLoc &oloc = *this; return oloc->id; }
    const int *_id() { ObjectLoc &oloc = *this; return &oloc->id; }
    int _id_size() { return sizeof(int); }
    int size(varObj &vobj) { return vobj.len; }
    int size(varObj &vobj, int sz);
    Index index(int i) { return entity->index(i); }
    void v_init();
    void v_del();

    int FindId(int id) { return index(idxId)->Find(&id,&obj); }
    int LocateId(int op, int id) { return index(idxId)->Locate(op,&id,0,0,&obj); }
    int FirstId() { return index(idxId)->First(0,&obj); }
    int LastId() { return index(idxId)->Last(0,&obj); }
    int NextId() { return index(idxId)->Next(0,&obj); }
    int FirstId(pgRef &loc) { return index(idxId)->First(loc,0,&obj); }
    int NextId(pgRef &loc) { return index(idxId)->Next(loc,0,&obj); }
    int NextLocId(pgRef &loc) { return index(idxId)->NextLoc(loc); }

    static int cmpr_char(const char *ap, int asz, const char *bp, int bsz);
    static int cmpr_uchar(const unsigned char *ap, int asz, const unsigned char *bp, int bsz);
    static int cmpr_short(const short *ap, int asz, const short *bp, int bsz);
    static int cmpr_ushort(const unsigned short *ap, int asz, const unsigned short *bp, int bsz);
    static int cmpr_int(const int *ap, int asz, const int *bp, int bsz);
    static int cmpr_uint(const unsigned int *ap, int asz, const unsigned int *bp, int bsz);
    static int cmpr_long(const long *ap, int asz, const long *bp, int bsz);
    static int cmpr_ulong(const unsigned long *ap, int asz, const unsigned long *bp, int bsz);
    static int cmpr_float(const float *ap, int asz, const float *bp, int bsz);
    static int cmpr_double(const double *ap, int asz, const double *bp, int bsz);
#ifdef ZMEDIA
    static int cmpr_media(const unsigned char *ap, int asz, const unsigned char *bp, int bsz);
#endif

#ifdef DEBUG
    int _err_(int v,const char *fn,int ln) { return entity->db->_err_(v,fn,ln); }
    int _fail_(int v,const char *fn,int ln) { return entity->db->_fail_(v,fn,ln); }
#else
    int _err_(int v) { return entity->db->_err_(v); }
    int _fail_(int v) { return entity->db->_fail_(v); }
#endif

    int last(Index idx,ObjectLoc &last_loc);
    int last(const char *nm,int (ObjectLoc::*ip)());
    unsigned int last(const char *nm,unsigned int (ObjectLoc::*ip)());
  };

  class Key {
  public:
    ObjectLoc &loc;
    Index idx;
    CmprFn cmpr;
    virtual int wr_key(char *cp) = 0;
    Key(Index i, ObjectLoc &l, CmprFn c) : loc(l), idx(i), cmpr(c) {}
    Key(const char *nm, ObjectLoc &l, CmprFn c) : loc(l), cmpr(c) {
      idx = loc.entity->index(nm);
    }
    operator void *() { return (void *)this; }
#ifdef DEBUG
    int _err_(int v,const char *fn,int ln) { return loc._err_(v,fn,ln); }
    int _fail_(int v,const char *fn,int ln) { return loc._fail_(v,fn,ln); }
#else
    int _err_(int v) { return loc._err_(v); }
    int _fail_(int v) { return loc._fail_(v); }
#endif
  };

  class rKey : public Key {
  public:
    rKey(Index i, ObjectLoc &l, CmprFn c) : Key(i,l,c) {}
    rKey(const char *nm, ObjectLoc &l, CmprFn c) : Key(nm,l,c) {}
    virtual int wr_key(char *cp=0) { return -1; }
    int NextLoc(pgRef &pos) { return idx->NextLoc(pos); }
    int First();  int First(pgRef &pos);
    int Next();   int Next(pgRef &pos);
    int Last();
    int Locate(int op=keyGE);
  };

  class iKey : protected rKey {
  public:
    iKey(Index i, ObjectLoc &l, CmprFn c) : rKey(i,l,c) {}
    iKey(const char *nm, ObjectLoc &l, CmprFn c) : rKey(nm,l,c) {}
    int NextLoc(pgRef &pos) { return idx->NextLoc(pos); }
    int Find();
    int Locate(int op=keyGE);
  };

private:
  class EntityObj : public Obj { /* entity storage */
  public:
    char name[nmSz];             /* string identifier */
    AllocCache alloc_cache;      /* entity allocator cache */
    int maxId;                   /* highest ID value */
    int recdSz;                  /* record size in bytes */
    int count;                   /* number of records */
    int nidxs;                   /* index count */
    int indexs[1];               /* id/loc index */
    EntityObj(EntityObj &eobj, int eid);
  };

  class EntityLoc : public ObjectLoc {
  public:
    EntityObj *operator ->() { return (EntityObj *)addr(); }
    EntityLoc(Entity *ep) : ObjectLoc(ep) {}
    ~EntityLoc() {}
    int cacheFlush() {
      EntityLoc &eloc = *this;  _wr();
      return eloc->alloc_cache.cacheFlush(entity->db);
    }
  };

  class varObjRef;
  typedef varObjRef *varObjs;
  class varObjRef {
  public:
    varObjs next;
    vRef ref;
    varObjRef(varObjs &lp, vRef rp) : next(lp), ref(rp) {}
  };
   
  static int ioCmpr(const void *a, const void *b, void *c);
public:

  class Entity {
  public:
    Db *const db;
    EntityLoc ent;
    varObjs vobjs;
    zrwlock_t *rw_lock;
    operator zrwlock_t&() { return *rw_lock; }

#ifdef DEBUG
    int _err_(int v,const char *fn,int ln) { return db->_err_(v,fn,ln); }
    int _fail_(int v,const char *fn,int ln) { return db->_fail_(v,fn,ln); }
#else
    int _err_(int v) { return db->_err_(v); }
    int _fail_(int v) { return db->_fail_(v); }
#endif
    Entity(Db *const db) : db(db), ent(this), vobjs(0) {}
    ~Entity() {}

    int allocate(ObjectLoc &loc,int sz=0);
    int construct_(ObjectLoc &loc, int id);
    int construct(ObjectLoc &loc) { return construct_(loc,ent->maxId); }
    int destruct_(ObjectLoc &loc, int id);
    int destruct(ObjectLoc &loc) { return destruct_(loc, loc->id); }
    int deallocate(pgRef &obj) { return db->deallocate(obj,ent->alloc_cache); }
    int deallocate(ObjectLoc &loc) { return deallocate(loc.obj); }
    int get_index(const char *nm, CmprFn cmpr=0);
    Index index(int i) { return db->indecies[ent->indexs[i]]; }
    Index index(const char *nm) {
      int idx = get_index(nm);
      return idx >= 0 ? index(idx) : 0;
    }
    int MaxId() { return ent->maxId; }
    int Count() { return ent->count; }
    int add_index(int idx, int kty=ktyBin);
    int add_bindex(const char *nm,int keySz,int dataSz) {
      int idx = db->new_binary_index(nm,keySz,dataSz);
      if_err( idx );  if_err( add_index(idx,ktyBin) );
      return idx;
    }
    int add_ind_index(const char *nm) {
      int idx = db->new_binary_index(nm,0,sizeof(int),Db::cmprKey);
      if_err( idx );  if_err( add_index(idx,ktyInd) );
      return idx;
    }
    int add_dir_index(const char *nm,int keySz) {
      int idx = db->new_binary_index(nm,keySz,sizeof(int),Db::cmprKey);
      if_err( idx );  if_err( add_index(idx,ktyDir) );
      return idx;
    }
    int add_str_index(const char *nm,int dataSz) {
      int idx = db->new_string_index(nm, dataSz);
      if_err( idx );  if_err( add_index(idx,ktyDir) );
      return idx;
    }
    int del_index_(int idx);
    int del_index(int idx);
    int del_index(const char *nm);
    int new_entity(const char *nm, int sz) { return db->new_entity(*this,nm,sz); }
    int get_entity(const char *nm) { return db->get_entity(*this,nm); }
    int del_entity() { return db->del_entity(*this); }
    void add_vref(vRef rp) { vobjs = new varObjRef(vobjs,rp); }
  };

  class ObjectList;
  typedef ObjectList *Objects;
  static void finit(Objects objects);

  class ObjectList {
  public:
    Objects next;
    ObjectLoc *obj;
    ObjectList(Objects op, ObjectLoc &o) : next(op), obj(&o) {}
  };

private:
  int new_entity_(Entity &entity, const char *nm, int sz);

  int findCmprFn(CmprFn fn);
  int pageLoad(pageId id, Page &pg);
  int addrRead_(pgRef &loc, char *&vp, int mpsz=0) {
    Page &pg = *get_page(loc.id);  vp = 0;
    if( unlikely( !pg.addr || pg->chk_flags(fl_rd) ) ) {
      if_err( pageLoad(loc.id, pg) );
    }
    vp = (char *)pg.addr+loc.offset+mpsz;
    return 0;
  }
  int addrRead_(pgRef &loc, keyBlock *&vp, int mpsz=0) {
    return addrRead_(loc,*(char**)&vp, mpsz);
  }
  int addrRead_(pgRef &loc, allocPrefix *&vp, int mpsz=0) {
    return addrRead_(loc,*(char**)&vp, mpsz);
  }
  int addrRead_(pgRef &loc, pagePrefix *&vp, int mpsz=0) {
    return addrRead_(loc,*(char**)&vp, mpsz);
  }
  int addrWrite_(pgRef &loc, char *&vp, int mpsz=0) {
    Page &pg = *get_page(loc.id);  vp = 0;
    if( unlikely( !pg.addr || pg->chk_flags(fl_rd) ) ) {
      if_err( pageLoad(loc.id, pg) );
    }
    pg->set_flags(fl_wr);
    vp = (char *)pg.addr+loc.offset+mpsz;
    return 0;
  }
  int addrWrite_(pgRef &loc, keyBlock *&vp, int mpsz=0) {
    return addrWrite_(loc,*(char**)&vp, mpsz);
  }
  int addrWrite_(pgRef &loc, allocPrefix *&vp, int mpsz=0) {
    return addrWrite_(loc,*(char**)&vp, mpsz);
  }
  int addrWrite_(pgRef &loc, pagePrefix *&vp, int mpsz=0) {
    return addrWrite_(loc,*(char**)&vp, mpsz);
  }
  int addrRead(pgRef &loc, char *&vp) {
    return addrRead_(loc, vp, sizeof(allocPrefix));
  }
  int addrWrite(pgRef &loc, char *&vp) {
    return addrWrite_(loc, vp, sizeof(allocPrefix));
  }

  int objectHeapInsert(int sz,int pg,int off,AllocCache &cache);
  int objectHeapDelete(int sz,int pg,int off,AllocCache &cache);
  int objectAllocate(int typ, int &size, pgRef &loc,AllocCache &cache);
  int objectFree(pgRef &loc,AllocCache &cache);
  int pgRefGet(int &size, pgRef &loc,AllocCache &cache);
  int pgRefNew(int &size, pgRef &lo,AllocCache &cache);
  int pgRefAllocate(int &size, pgRef &lo,AllocCache &cache);

  int storeInsert(long size, ioAddr io_addr);
  int storeDelete(long size, ioAddr io_addr);
  int storeGet(int &size, ioAddr &io_addr);
  int storeNew(int &size, ioAddr &io_addr);
  int storeAllocate(int &size, ioAddr &io_addr);
  int storeFree(int size, ioAddr io_addr);

  int icommit(int force);
  int iundo();
  int iclose();
  int ireopen();
  int iattach();
  int iopen(int undo_save=1);

protected:

  pageId new_page();
  void del_page(int id);
  int alloc_pageTable(int sz);
  void free_page_(int pid);
  void free_page(int pid) { locked by(db_info->pgAlLk); free_page_(pid); }
  pageId lower_page(int mid);
  int alloc_indecies(int n);
  int new_index();
  void del_index(int idx);
  int new_index(IndexBase *&ibp, IndexBaseInfo *b, IndexBinaryInfo *d);
  int new_index(IndexBase *&ibp, IndexBaseInfo *b, IndexStringInfo *d);
  int indexRead(pageId pid, int df, keyBlock *&bp) {
    pgRef pg;  pg.id = pid;  pg.offset = 0;
    return !df ? addrRead_(pg,*(char**)&bp) : addrWrite_(pg,*(char**)&bp);
  }
  int indexRead(pageId pid,int df,keyBlock *&bp, Page *&pp, char *&cp) {
    pp = 0;  cp = 0;  if_err( indexRead(pid, df, bp) );
    pp = get_page(pid);  cp = (char *)(bp + 1);
    return 0;
  }
  void pageDealloc(Page &pg, int mode=1);
  int pageRead(ioAddr io_adr, uint8_t *bp, int len);
  int pageWrite(Page &pg);
  int seek_data(ioAddr io_addr);
  int size_data(char *dp, int sz);
  int read_data(char *dp, int sz);
  int write_data(char *dp, int sz);
  int write_zeros(ioAddr io_addr);
  int write_padding(ioAddr io_addr);
  int readRootInfo(int(Db::*fn)(char *dp,int sz));
  int writeRootInfo(int(Db::*fn)(char *dp,int sz));
  ioAddr storeBlocks(ioAddr sz) { return (sz+storeBlockSize-1)/storeBlockSize; }
  ioAddr entityPages(ioAddr sz) { return (sz+entityPageSize-1)/entityPageSize; }
  int indeciesHunks(int sz) { return (sz+indexTableHunkSize-1)/indexTableHunkSize; }
  int pageTableHunks(int sz) { return (sz+pageTableHunkSize-1)/pageTableHunkSize; }
  int pagePrefixHunks(int sz) { return (sz+sizeof(pagePrefix)-1)/sizeof(pagePrefix); }
  int init_idx();
  void init_shm();
  void init();
  void deinit();
  void reset();
  int start_transaction(int undo_save=1);
  void enter() { db_info->dbRwLk.enter(); }
  void leave() { db_info->dbRwLk.leave(); }
  void write_enter() { db_info->dbRwLk.write_enter(); }
  void write_leave() { db_info->dbRwLk.write_leave(); }

public:
  // 1:wr, 0:rd, -1:unlocked
  int is_locked() { return db_info->dbRwLk.locked(); }
  int is_blocked() { return db_info->dbRwLk.blocked(); }

  int make(int zfd);
  int open(int zfd, int zkey=-1);
  int close();

  int attach(int zrw, int zfd, int zkey);
  int attach(int zrw=0) { return attach(fd, key, zrw); }
  int detach();

  int copy(Db *db, Objects objs);
  int new_binary_index(const char *nm, int ksz, int dsz, CmprFn cmpr=0);
  int new_string_index(const char *nm, int dsz);
  int get_index(const char *nm, CmprFn cmpr=0);
  long get_count(int r);
  int ins   (int r, void *key, void *data);
  int del   (int r, void *key);
  int find  (int r, void *key, void *rtnData=0);
  int locate(int r, int op, void *key, CmprFn cmpr, void *rtnKey, void *rtnData=0);
  int locate(int r, int op, void *key, void *rtnKey, void *rtnData=0) {
    return locate(r,op,key,0,rtnKey,rtnData);
  }
  int first (int r, void *key, void *rtnData=0);
  int last  (int r, void *key, void *rtnData=0);
  int next  (int r, void *key, void *rtnData=0);
  int nextloc(int r, pgRef &loc);
  int allocate(int typ, int size, pgRef &loc, AllocCache &cache);
  int reallocate(int size, pgRef &loc, AllocCache &cache);
  int deallocate(pgRef &loc, AllocCache &cache);
  int commit(int force=0);
  int flush ();
  int undo  ();

  int transaction() { return !root_info ? -1 : root_info->transaction_id; }
  int64_t filesize() { return !root_info ? -1 : root_info->file_size; }
  int opened() { return fd>=0 ? 1 : 0; }
  void use_shm(int v) { no_shm = v ? 0 : 1; }
  int error() { return err_no; }
  void error(int v);
  void Error(int v,const char *msg);
  int load();
  int load_indecies();

  Db();
  ~Db();

#ifdef DEBUG
  void dmp();
  void tdmp();
  void pdmp();
  void fdmp();
  void admp(); void achk(); void fchk();
  void edmp(AllocCache &cache);
  void bdmp(AllocCache &cache);
  void cdmp();
  void stats();
#endif

};

#endif
