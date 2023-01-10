#include <algorithm>
#include <cstring>
#include <malloc.h>

#include "memory_statistics.h"

extern unsigned char _eheap[];
extern unsigned char *sbrk_heap_top;

namespace lw_event_trace
{

MemoryStatistics::MemoryStatistics() :
     min_free_bytes(0xfffffff), 
     max_free_bytes(0),
     cma_free_bytes(0),
     free_bytes(0),
     capture_count(0)
{

}

MemoryStatistics::~MemoryStatistics() {

}

void MemoryStatistics::capture() {

    volatile struct mallinfo info = mallinfo();
    uint32_t free_memory = info.fordblks + (uint32_t)_eheap - (uint32_t)sbrk_heap_top;
    min_free_bytes = std::min<uint32_t>(min_free_bytes, free_memory);
    max_free_bytes = std::max<uint32_t>(max_free_bytes, free_memory);
    cma_free_bytes = (info.fordblks + capture_count * cma_free_bytes) / (capture_count+1);
    free_bytes = free_memory;
    capture_count++;

}

MemoryStatistics* MemoryStatistics::getInstance()
{
    static MemoryStatistics instance;
    return &instance;
}

}

