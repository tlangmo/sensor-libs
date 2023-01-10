#include <cstring>
#include "lw_event_trace.h"
#ifdef __unix__
#include "lw_event_trace_platform_x86.h"
#else
#include "lw_event_trace_platform.h"
#endif
#include <algorithm>
using namespace std;
namespace lw_event_trace
{
 

void to_json(const lw_event_trace::TraceEvent& evt, std::string* out) {
    char buf[128];
    memset(buf, 0, 128);
    int a = sprintf(buf, "{\"cat\": \"%s\", \"pid\":1, \"tid\": %.0f, \"ts\": %.0f, \"ph\":\"%c\", \"name\": \"%s\", \"args\":{}},", 
        evt.category, (double)evt.thread_id, (double)evt.timestamp, evt.phase == 0 ? 'B' : 'E', evt.name);
    out->assign(buf, a);
}

 LwEventTraceLogger::LwEventTraceLogger() : 
    m_events(), 
    m_lock(new Lock()), 
    m_enabled(false)
{
}

void LwEventTraceLogger::enable(size_t reservedBytes)
{
    LwScopedLock lock(m_lock);
    // pre-allocate to avoid any stalls
    m_events.reserve(reservedBytes/sizeof(TraceEvent));
    m_events.clear();
    m_enabled = true;
   
}
void LwEventTraceLogger::disable()
{
    LwScopedLock lock(m_lock);
    m_enabled = false;
}

void LwEventTraceLogger::addEventTrace(const char* category, const char* name, uint32_t thread_id, uint64_t timestamp, char phase) {
    LwScopedLock lock(m_lock);
    if (!m_enabled) {
        return;
    }
    TraceEvent evt;
    memset(&evt, 0, sizeof(evt));
    evt.category = category;
    evt.name = name;
    evt.thread_id = thread_id % 255;
    evt.timestamp = timestamp;
    evt.phase = phase == 'B' ? 0 : 1;
    if (m_events.size() < m_events.capacity()-1) {
        m_events.push_back(evt);
    }
   
}

ScopedEvent::ScopedEvent(const char* category, const char* name) : m_category(category), m_name(name)
{
    uint32_t thread_id = get_current_thread_id();
    uint64_t timestamp = 0;
    query_counter(&timestamp);
    LwEventTraceLogger::get_instance().addEventTrace(category, name, thread_id, timestamp, 'B');
}
ScopedEvent::~ScopedEvent()
{
    uint32_t thread_id = get_current_thread_id();
    uint64_t timestamp = 0;
    query_counter(&timestamp);
    LwEventTraceLogger::get_instance().addEventTrace(m_category, m_name, thread_id, timestamp, 'E');
}
 
}; // end ns