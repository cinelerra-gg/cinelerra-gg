#ifndef __BC_TRACE_H__
#define __BC_TRACE_H__

#include "arraylist.h"
#include "linklist.h"
#include "bctrace.inc"
#include "bcwindowbase.inc"
#include "cstrdup.h"
#include <pthread.h>

#ifdef BOOBY
#define BT if( top_level->display_lock_owner != pthread_self() ) booby();
void booby();
#else
#define BT
#endif

class BC_Trace
{
public:
	BC_Trace();
	~BC_Trace();

	static BC_Trace *global_trace;
        static void reset_locks();

        static void delete_temps();
        static void set_temp(char *string);
        static void unset_temp(char *string);

	static void enable_locks();
	static void disable_locks();
	static int set_lock(const char *title, const char *location, trace_info *info);
	static void set_lock2(int table_id, trace_info *info);
	static void unset_lock2(int table_id, trace_info *info);
	static void unset_lock(trace_info *info);
// Used in lock destructors so takes away all references
	static void unset_all_locks(trace_info *info);
	static void clear_locks_tid(pthread_t tid);

	static void new_trace(const char *text);
	static void new_trace(const char *file, const char *function, int line);
	static void delete_traces();

	static void enable_memory();
	static void disable_memory();
	static void set_buffer(int size, void *ptr, const char* location);
// This one returns 1 if the buffer wasn't found.
	static int unset_buffer(void *ptr);
	static void lock_locks(const char *s);
	static void unlock_locks();

	static void dump_traces(FILE *fp=stdout);
	static void dump_locks(FILE *fp=stdout);
	static void dump_buffers(FILE *fp=stdout);
	static void dump_threads(FILE *fp=stdout);

	static void dump_shm_stat(const char *fn, FILE *fp=stdout);
	static void dump_shm_stats(FILE *fp=stdout);
};

class bc_trace_list : public List<trace_item> {
public:
	void clear() { while( last ) remove(last); }
	bc_trace_list() {}
	~bc_trace_list() { clear(); }
};

class bc_trace_t : public bc_trace_list {
public:
	int size;
	bc_trace_t() : size(0) {}
	~bc_trace_t() {}
};

class bc_trace_spin : public bc_trace_t {
	pthread_spinlock_t spin;
public:
	void *operator new(size_t n) { return (void*) malloc(n); }
	void operator delete(void *t, size_t n) { free(t); }

	void lock() { pthread_spin_lock(&spin); }
	void unlock() { pthread_spin_unlock(&spin); }
	bc_trace_spin() { pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE); }
	~bc_trace_spin() { pthread_spin_destroy(&spin); }
};

class bc_trace_mutex : public bc_trace_t {
	pthread_mutex_t mutex;
public:
	void *operator new(size_t n) { return (void*) malloc(n); }
	void operator delete(void *t, size_t n) { free(t); }

	void lock() { pthread_mutex_lock(&mutex); }
	void unlock() { pthread_mutex_unlock(&mutex); }
	bc_trace_mutex() { pthread_mutex_init(&mutex, 0); }
	~bc_trace_mutex() { pthread_mutex_destroy(&mutex); }
};

extern bc_trace_mutex execution_table;
extern bc_trace_mutex memory_table;
extern bc_trace_mutex lock_table;
extern bc_trace_mutex file_table;
extern "C" void dump();

class trace_item : public ListItem<trace_item> {
public:
	bc_trace_t &table;
	trace_item(bc_trace_t &t);
	~trace_item();
};

class execution_item : public trace_item {
public:
	void *operator new(size_t n) { return (void*) malloc(n); }
	void operator delete(void *t, size_t n) { free(t); }

	const char *value;
	void clear() { delete [] value;  value = 0; }
	void set(const char *v) { delete [] value;  value = cstrdup(v); }

	execution_item() : trace_item(execution_table) { value = 0; }
	~execution_item() { clear(); }
};

class lock_item : public trace_item {
	static int table_id;
public:
	void *operator new(size_t n) { return (void*) malloc(n); }
	void operator delete(void *t, size_t n) { free(t); }

        trace_info *info;
        const char *title;
        const char *loc;
        int is_owner;
        int id;
        pthread_t tid;
	void set(trace_info *info, const char *title, const char *loc) {
		this->info = info;  this->title = title;
		this->loc = loc;  this->is_owner = 0;
		this->id = table_id++;  this->tid = pthread_self();
	}
	void clear() {
		this->info = 0;  this->title = 0; this->loc = 0;
		this->is_owner = 0;  this->id = -1;  this->tid = 0;
	}

	lock_item() : trace_item(lock_table) { clear(); }
	~lock_item() {}
};

class memory_item : public trace_item {
public:
	void *operator new(size_t n) { return (void*) malloc(n); }
	void operator delete(void *t, size_t n) { free(t); }

	int size;
	void *ptr;
	const char *loc;

	memory_item(int size, void *ptr, const char *loc)
	 : trace_item(memory_table) {
		this->size = size; this->ptr = ptr; this->loc = loc;
	}
	~memory_item() {}
};

class file_item : public trace_item {
public:
	void *operator new(size_t n) { return (void*) malloc(n); }
	void operator delete(void *t, size_t n) { free(t); }

	const char *value;
	void clear() { delete [] value;  value = 0; }
	void set(const char *v) { delete [] value;  value = cstrdup(v); }

	file_item() : trace_item(file_table) { value = 0; }
	~file_item() { clear(); }
};

// track unjoined threads at termination
#ifdef TRACE_THREADS

class TheLock {
public:
	pthread_mutex_t the_lock;

	void lock() { pthread_mutex_lock(&the_lock); }
	void unlock() { pthread_mutex_unlock(&the_lock); }

	void init() {
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutex_init(&the_lock, &attr);
	}
	void finit() {
		pthread_mutex_destroy(&the_lock);
	}
	void reset() { finit();  init(); }

	TheLock()  { init();  }
	~TheLock() { finit(); }
};

class TheLocker {
public:
	static TheLock the_lock;
	static void reset() { the_lock.reset(); }

	TheLocker() { the_lock.lock(); }
	~TheLocker() { the_lock.unlock(); }
};

class TheDbg {
public:
	pthread_t tid, owner;  const char *name;
	TheDbg(pthread_t t, pthread_t o, const char *nm) { tid = t; owner = o; name = nm; }
	~TheDbg() {}
};


class TheList : public ArrayList<TheDbg *> {
public:
	static TheList the_list;
	static void dump_threads(FILE *fp);
	static void dbg_add(pthread_t tid, pthread_t owner, const char *nm);
	static void dbg_del(pthread_t tid);
	static void reset() { the_list.remove_all_objects(); TheLocker::reset(); }
	void check() {
		int i = the_list.size();
		if( !i ) return;
		printf("unjoined tids / owner %d\n", i);
		while( --i >= 0 ) printf("  %016lx / %016lx %s\n",
			(unsigned long)the_list[i]->tid,
			(unsigned long)the_list[i]->owner,
			the_list[i]->name);
	}
	 TheList() {}
	~TheList() { check(); reset(); }
};

#endif

#endif
