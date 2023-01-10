#pragma once
#include <atomic>
#include <cassert>
#include <iterator>     // std::distance
//https://en.cppreference.com/w/cpp/atomic/memory_order
#include <string.h>
namespace motesque
{

/* A thread safe single producer, single consumer byte buffer. The main feature are indempotent reads to work
 * well with tcp / sd card writes and a signalling mechanism to wait for new data. */
template<typename LOCK, typename EVENT_FLAG>
class SequentialBufferT
{
    SequentialBufferT(const SequentialBufferT& rhs);
    SequentialBufferT& operator=(const SequentialBufferT& rhs);

class ScopedLock
{
public:
  ScopedLock(LOCK* lock) : m_lock(lock){
      m_lock->lock();
  }
  ~ScopedLock() {
      m_lock->unlock();
  }
private:
  LOCK* m_lock;

};

public:
    SequentialBufferT(size_t size);
    virtual ~SequentialBufferT();
    // clear the buffer
    void clear();
    // write data to buffer. Fails if not enough space is available to write all the data
    int write(const uint8_t* data, size_t data_size);
    // get a read pointer to the underlying data. Optionally waits for reached watermark timeout milliseconds
    int request_read(const uint8_t** data, size_t* available_size, uint32_t timeout_ms);
    // advance the read cursor
    int commit_read(size_t size);
    // the amount of data available for reads
    size_t free() const;
    // the amount of data stored
    size_t size() const;
    // watermark for signalling mechanism
    int set_watermark(size_t threshold_bytes);
private:
    uint8_t* const m_data;
    uint8_t* const m_data_end;
    uint8_t* m_read_ptr;
    uint8_t* m_write_ptr;
    size_t   m_free;
    LOCK     m_lock;
    EVENT_FLAG m_watermark_event;
    size_t   m_watermark_bytes;
};

template<typename LOCK, typename EVENT_FLAG>
SequentialBufferT<LOCK, EVENT_FLAG>::SequentialBufferT(size_t size)
: m_data(new uint8_t[size]),
  m_data_end(m_data+ size),
  m_read_ptr(m_data),
  m_write_ptr(m_data),
  m_free(0),
  m_lock(),
  m_watermark_bytes(1)
{
    clear();
}

template<typename LOCK, typename EVENT_FLAG>
SequentialBufferT<LOCK, EVENT_FLAG>::~SequentialBufferT()
{
    delete[] m_data;
}


template<typename LOCK, typename EVENT_FLAG>
int SequentialBufferT<LOCK, EVENT_FLAG>::set_watermark(size_t mark_bytes)
{
    m_watermark_bytes =  mark_bytes;
    return 0;
}

template<typename LOCK, typename EVENT_FLAG>
size_t SequentialBufferT<LOCK, EVENT_FLAG>::free() const
{
    return m_free;
}

template<typename LOCK, typename EVENT_FLAG>
size_t SequentialBufferT<LOCK, EVENT_FLAG>::size() const
{
    assert((size_t)std::distance(m_data, m_data_end) >= m_free);
    return (size_t)std::distance(m_data, m_data_end)  - m_free;
}

template<typename LOCK, typename EVENT_FLAG>
void SequentialBufferT<LOCK, EVENT_FLAG>::clear()
{
    ScopedLock sl(&m_lock);
    m_read_ptr  = m_data;
    m_write_ptr = m_data;
    m_free = std::distance(m_data, m_data_end);
}

template<typename LOCK, typename EVENT_FLAG>
int SequentialBufferT<LOCK, EVENT_FLAG>::write(const uint8_t* data, size_t data_size)
{
    if ( data_size == 0 || ( m_free < data_size ) ) {
        // buffer is full. Notify any waiting readers to do their job...
         m_watermark_event.set();
         return -1;
    }
    ScopedLock sl(&m_lock);

    if (m_write_ptr >= m_read_ptr) {
        // ------------------------#
        //   |        |
        //   r        w

        // write the data until the end and maybe wrap around
        size_t append_size = std::min<size_t>(data_size, m_data_end - m_write_ptr );
        memcpy(m_write_ptr, data, append_size);
        m_write_ptr += append_size;
        if (m_write_ptr == m_data_end) {
            m_write_ptr = m_data;
        }
        // write any rest to the front
        size_t prepend_size = data_size - append_size;
        if (prepend_size > 0) {
            memcpy(m_write_ptr, data+append_size, prepend_size);
            m_write_ptr += prepend_size;
        }
        m_free -= data_size;
    }
    else {
        // ------------------------#
        //   |        |
        //   w        r

        // simply copy data
        assert(m_write_ptr + data_size <= m_read_ptr);
        memcpy(m_write_ptr, data, data_size);
        m_free -= data_size;
        m_write_ptr += data_size;
    }
    if (size() >= m_watermark_bytes) {
        // notfiy readers on watermark
        m_watermark_event.set();
    }
    return 0;
}

template<typename LOCK, typename EVENT_FLAG>
int SequentialBufferT<LOCK, EVENT_FLAG>::commit_read(size_t commit_size)
{
    ScopedLock sl(&m_lock);
    m_read_ptr += commit_size;
    assert(m_read_ptr <= m_data_end);
    if (m_read_ptr == m_data_end) {
        m_read_ptr = m_data;
    }
    m_free += commit_size;

    return 0;
}


template<typename LOCK, typename EVENT_FLAG>
int SequentialBufferT<LOCK, EVENT_FLAG>::request_read(const uint8_t** data, size_t* available_size, uint32_t timeout_ms)
{

    // if we use timeouts, we wait until the watermark is reached. If it is not, my might read whatever is present
    if (timeout_ms > 0 && m_watermark_event.wait_for(timeout_ms) != 0) {
        if (size() == 0) {
            return -1; // timed out, no data to read
        }
    }

    ScopedLock sl(&m_lock);
    if (m_read_ptr < m_write_ptr) {
        // ------------------------#
        //   |        |
        //   r        w
        // hand out the read ptr
        *available_size = std::distance(m_read_ptr, m_write_ptr);
    }
    else {
        // ------------------------#
        //   |        |
        //   w        r
        // or
        // ------------------------#
        //   |
        //   w
        //   r
        // we need to distinguish between empty and availble case
        if (size() > 0) {
            // hand out the read pointer until the end
            *available_size = std::distance(m_read_ptr, m_data_end);
        }
        else {
            *available_size = 0;
        }
    }
    *data = m_read_ptr;

    return *available_size > 0 ? 0 : -1;
}




}
