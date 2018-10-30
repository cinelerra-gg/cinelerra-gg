#ifndef __BCTRACE_INC__
#define __BCTRACE_INC__

//#define ENABLE_TRACE
#define TRACE_LOCKS
//#define TRACE_MEMORY
#define TRACE_THREADS
#define TRACE_LOCKS

class BC_Trace;
class trace_item;
class execution_item;
class lock_item;
class memory_item;
class file_item;
class bc_trace_t;
class bc_trace_spin;
class bc_trace_mutex;

class TheLock;
class TheLocker;
class TheDbg;
class TheList;
class TheChk;

class trace_info {
public:
	void *trace;
	trace_info() { trace = 0; }
	~trace_info() {}
};

#ifdef NO_GUICAST

#undef ENABLE_TRACE
#undef TRACE_LOCKS
#undef TRACE_MEMORY
#undef TRACE_THREADS

#endif


#ifdef ENABLE_TRACE
// Add a trace
#define TRACE(text) BC_Trace::new_trace(text);
#define SET_TRACE BC_Trace::new_trace(__FILE__, __FUNCTION__, __LINE__);
#define PRINT_TRACE { printf("%s: %d\n", __FILE__, __LINE__); fflush(stdout); }
// Delete all traces
#define UNTRACE BC_Trace::delete_traces();

#else

#define TRACE(text) ;
#define UNTRACE ;
#define PRINT_TRACE { printf("%s: %d\n", __FILE__, __LINE__); fflush(stdout); }
//#define PRINT_TRACE ;
#define SET_TRACE ;

#endif


#ifdef TRACE_LOCKS
// Before user acquires
#define SET_LOCK(info, title, location) \
 int table_id = BC_Trace::set_lock(title, location, info);
// After successful acquisition of a mutex, the table is flagged
#define SET_LOCK2 BC_Trace::set_lock2(table_id, this);
// After successful acquisition of a condition, the table is removed because
// the user never unlocks a condition after locking it.
// Release current lock table after failing to acquire
#define UNSET_LOCK2 BC_Trace::unset_lock2(table_id, this);

// Release current owner of lock
#define UNSET_LOCK(info) BC_Trace::unset_lock(info);
// Delete a lock
#define UNSET_ALL_LOCKS(info) BC_Trace::unset_all_locks(info);

#define LOCK_LOCKS(s) BC_Trace::lock_locks(s);
#define UNLOCK_LOCKS BC_Trace::unlock_locks();
#define CLEAR_LOCKS_TID(tid) BC_Trace::clear_locks_tid(tid);

#else

#define SET_LOCK(title, location) ;
#define SET_LOCK2 ;
#define SET_LOCK2_CONDITION ;
#define UNSET_LOCK(info) ;
#define UNSET_LOCK2 ;
#define UNSET_ALL_LOCKS(info) ;

#define LOCK_LOCKS(s) ;
#define UNLOCK_LOCKS ;
#define CLEAR_LOCKS_TID(tid) ;
#endif


#ifdef TRACE_MEMORY

#define ENABLE_BUFFER BC_Trace::enable_memory();
#define DISABLE_BUFFER BC_Trace::disable_memory();
// Note the size, pointer, and location of an allocation
#define BUFFER(size, ptr, location) BC_Trace::set_buffer(size, ptr, location);
// Note the pointer and location of an allocation
#define BUFFER2(ptr, location) BC_Trace::set_buffer(0, ptr, location);
// Remove a pointer from the allocation table
#define UNBUFFER(ptr) BC_Trace::unset_buffer(ptr);

#else

#define ENABLE_BUFFER ;
#define DISABLE_BUFFER ;
#define BUFFER(size, ptr, location);
#define UNBUFFER(ptr);

#endif

// Handling of temporary files in crash
#define SET_TEMP BC_Trace::set_temp
#define UNSET_TEMP BC_Trace::unset_temp

#endif
