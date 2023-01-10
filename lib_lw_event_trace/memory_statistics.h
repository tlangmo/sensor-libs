#ifndef LW_EVENT_TRACE_MEMORY_STATS_H_
#define LW_EVENT_TRACE_MEMORY_STATS_H_
#include <stdint.h>
namespace lw_event_trace
{
class MemoryStatistics
{
private:
    MemoryStatistics();
    virtual ~MemoryStatistics();
public:
    static MemoryStatistics* getInstance();
    void capture();
public:
    uint32_t min_free_bytes;
    uint32_t max_free_bytes;
    uint32_t cma_free_bytes;
    uint32_t free_bytes;
    uint64_t capture_count;
};
}; // end ns
#endif