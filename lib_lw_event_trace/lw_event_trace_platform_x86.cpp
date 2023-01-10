#include "lw_event_trace_platform_x86.h"
#include <functional>
#include <mutex>
#include <thread>

namespace lw_event_trace
{


class LockImpl
{
public:
	LockImpl(): m_mutex() {
	}
	~LockImpl() {
	}
	
	void acquire(){
		m_mutex.lock();
	}

	void release(){
		m_mutex.unlock();
	}
private:
	std::mutex m_mutex;
};


Lock::Lock() :m_pimpl(new LockImpl()) 
	{

}
Lock::~Lock() {
	delete m_pimpl;
	m_pimpl = NULL;
}
void Lock::acquire() {
	m_pimpl->acquire();
}
void Lock::release() {
	m_pimpl->release();
}


uint32_t get_current_thread_id() {
    return std::hash<std::thread::id>{}(std::this_thread::get_id());
}

void query_counter(uint64_t* timestamp) {
    *timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::
                  now().time_since_epoch()).count();

}


}; // end ns

