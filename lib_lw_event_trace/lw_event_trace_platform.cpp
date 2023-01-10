#include "wiced.h"
#include "lw_event_trace_platform.h"
#include "micro_clock.h"
#include "tx_api.h"
#include <functional>


namespace lw_event_trace
{


class LockImpl
{
public:
	LockImpl(): m_mutex() {
		wiced_rtos_init_mutex(&m_mutex); 
	}
	~LockImpl() {
		wiced_rtos_deinit_mutex(&m_mutex);
	}
	
	void acquire(){
		wiced_rtos_lock_mutex(&m_mutex);
	}
	void release(){
		wiced_rtos_unlock_mutex(&m_mutex);
	}
private:
	wiced_mutex_t m_mutex;
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
	TX_THREAD* tx_thread = tx_thread_identify();

    return (uint32_t)std::hash<uint32_t>()(tx_thread->tx_thread_id);
}

void query_counter(uint64_t* timestamp) {
    *timestamp = get_time_micros();
}


}; // end ns

