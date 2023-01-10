/*
	This file defines platform specific items that are required by event_trace
*/
#ifndef LW_EVENT_TRACE_PLATFORM_H
#define LW_EVENT_TRACE_PLATFORM_H


#include <pthread.h>
#include <functional>


namespace lw_event_trace
{

class LockImpl;
class Lock
{

public:
	Lock();
	~Lock();
	void acquire();
	void release();
private:
	LockImpl* m_pimpl;
};

class LwScopedLock 
{
public:
	LwScopedLock(Lock* lock):m_lock(lock) {
        m_lock->acquire();
    };
   ~LwScopedLock(){ 
    if(m_lock) {
        m_lock->release();
        m_lock = nullptr;
    };
   };
private:
	Lock* m_lock;
};

uint32_t get_current_thread_id();

void query_counter(uint64_t* timestamp);


}; // end ns
#endif
