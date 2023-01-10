#ifndef LW_EVENT_TRACE_H_
#define LW_EVENT_TRACE_H_

#include <string>
#include <vector>
#include <functional>


namespace lw_event_trace
{
    class Lock;

    // pack as tightly as possible...
    struct TraceEvent {
        uint64_t timestamp:55; // 8 bytes
        uint64_t thread_id:8;
        uint64_t phase:1;      
        const char* category; // 4 bytes
        const char* name;     // 4 bytes
    };

    void to_json(const lw_event_trace::TraceEvent& evt, std::string* out);

    class LwEventTraceLogger {
    public:
        LwEventTraceLogger();
        static LwEventTraceLogger& get_instance() {
            static LwEventTraceLogger instance;
            return instance;
        } 
        void addEventTrace(const char* category, const char* name, uint32_t thread_id, uint64_t timestamp, char phase);
        
        void enable(size_t reservedBytes);
        void disable();
        const std::vector<TraceEvent>& get_events() const {
            return m_events;
        }
    private: 
        std::vector<TraceEvent> m_events;
        Lock*    m_lock;
        bool m_enabled;
    };

    class ScopedEvent {
    public:
        ScopedEvent(const char* category, const char* name);
        virtual ~ScopedEvent();
    private:
        const char* m_category;
        const char* m_name;
    };
};
#ifdef MOTESQUE_ENABLE_EVENT_TRACE
    #define TRACE_EVENT0(cat, name) lw_event_trace::ScopedEvent evt(cat, name);
#else
    #define TRACE_EVENT0(cat, name)
#endif

#endif

