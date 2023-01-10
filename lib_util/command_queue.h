#pragma once

#include <algorithm>
#include <functional>
#include <cstring>
#include <assert.h>
#include <atomic>
#include <array>
#ifdef __linux__
    #define wiced_rtos_thread_yield()
    #define WICED_NEVER_TIMEOUT 0xffffffff
#else
    #include <wiced.h>
#endif


namespace motesque {

// interesting article in this context: https://embeddedartistry.com/blog/2018/1/18/implementing-an-asynchronous-dispatch-queue-with-threadx

extern void delayMs(uint32_t ms);
extern uint32_t nowMs();

typedef std::function<void(void)> FuncType;

enum CommandID
{
    Unknown,
    WifiUp,
    WifiDown,
    WifiSetCredentials,
    FrameStreamStart,
    FrameStreamStop,
    SendDiscoveryMessage
};

struct IntStatus {
    IntStatus() : status(0) {}

    void set() {
        status = 1;
    }
    void clear() {
        status = 0;
    }
    int wait_for(uint32_t timeout_ms) {
        while (status == 0) {
            wiced_rtos_thread_yield();
        }
        return 0;
    }
    std::atomic<int> status;
};

struct RefCounter {
    RefCounter() :
        ref_count(0) {

    }

    void inc_ref_count() {
        ref_count.fetch_add(1);
    }

    void dec_ref_count()
    {
        ref_count.fetch_sub(1);
    }
    std::atomic<int>  ref_count;
};



template<typename STATUS>
struct FuncContextT : public RefCounter {
    FuncContextT() :  func(), status(), cancelled(0), scheduled_ms(), cmd_id(-1),interval_ms(-1) {
    }
    virtual ~FuncContextT()
    {

    }
    void reset() {
        status.clear();
        scheduled_ms = 0;
        cmd_id = -1;
        interval_ms = -1;
        cancelled = 0;
    }
    FuncType  func;
    STATUS    status;
    std::atomic<int>  cancelled;
    int       scheduled_ms;
    int       cmd_id;
    int       interval_ms;
};

/**
 * A queue which contains std::function pointers for cross thread execution.
 * It is especially useful to execute lambda functions in a different thread context. It also supports the
 * delayed scheduling of  functions
 * It assumes that the provided queue types are threadsafe, ideally lock free.
 *
 */
template<template<typename> class QueueT, int SIZE, typename STATUS>
class CommandQueue
{
public:
    typedef FuncContextT<STATUS> FuncContext;

    CommandQueue() :
        m_commands_later(SIZE),
        m_commands(SIZE),
        m_func_contexts()
    {
        // create n contexts
        for (size_t i=0; i < m_func_contexts.size(); i++) {
            FuncContext* p = new FuncContext();
            m_func_contexts[i] = p;
        }
    }

    virtual ~CommandQueue() {
        for (size_t i=0; i < m_func_contexts.size(); i++) {
            delete m_func_contexts[i];
        }
    }

    int cancel(int id) {
        auto it = std::find_if(m_func_contexts.begin(), m_func_contexts.end(), [id](const FuncContext* ctx) -> bool {
            return ctx->cmd_id == id && ctx->ref_count > 0;
        });
        if (it != m_func_contexts.end()) {
            // need to avoid that it is executed
            (*it)->cancelled = 1; 
        }
        return 0;
    }

    int find_available_func_context(size_t* idx) {
        auto it = std::find_if(m_func_contexts.begin(), m_func_contexts.end(), [](const FuncContext* ctx) -> bool {
            return ctx->ref_count == 0;
        });
        if (it == m_func_contexts.end()) {
            // no available func context to execute command
            return -1;
        }
        *idx = std::distance(m_func_contexts.begin(), it);
        return 0;
    }

    /**
     * Put a function type on the queue;
     * Make it waitable as well. This way we can do interleaving
     */
    template<typename FuncT>
    int execute_async(FuncT func, bool wait=false) {
        size_t func_context_idx = 0;
        if (find_available_func_context(&func_context_idx) < 0) {
            return -1;
        }
        FuncContext* func_context = m_func_contexts[func_context_idx];
        func_context->reset();
        // wrap lambda in std::function to be easy assignable
        func_context->func = FuncType(func);
        func_context->scheduled_ms = 0;
        func_context->status.clear();
        func_context->inc_ref_count();
        int rc = -1;
        if ((rc = m_commands.put(func_context_idx, 100)) != 0 ) {
            // cannot schedule at this time, abort
            func_context->dec_ref_count();
            return -1;
        }
        if (wait) {
            wiced_rtos_thread_yield();
            func_context->status.wait_for(WICED_NEVER_TIMEOUT); 
        }
        return 0;
    }

    /**
     * Put a function type on the queue with delay;
     */
    template<typename FuncT>
    int execute_async_later(FuncT func, uint32_t delay_ms, int id = -1) {
        size_t func_context_idx = 0;
        if (find_available_func_context(&func_context_idx) < 0) {
            return -1;
        }
        FuncContext* func_context = m_func_contexts[func_context_idx];
        func_context->reset();
        // wrap lambda in std::function to be easy assignable
        func_context->func = FuncType(func);
        func_context->scheduled_ms = nowMs() + delay_ms;
        func_context->status.clear();
        func_context->inc_ref_count();
        func_context->cmd_id = id;
        int rc = -1;
        if ((rc = m_commands_later.put(func_context_idx, 100)) != 0 ) {
            // cannot schedule at this time, abort
            func_context->dec_ref_count();
            return -1;
        }
        return 0;
    }

    template<typename FuncT>
    int execute_async_interval(FuncT func, uint32_t interval_ms, int id = -1)
    {
        size_t func_context_idx = 0;
        if (find_available_func_context(&func_context_idx) < 0) {
            return -1;
        }
        FuncContext* func_context = m_func_contexts[func_context_idx];
        func_context->reset();
        func_context->func = FuncType(func);
        func_context->scheduled_ms = nowMs() + interval_ms;
        func_context->status.clear();
        func_context->inc_ref_count();
        func_context->cmd_id = id;
        func_context->interval_ms = interval_ms;
        int rc = -1;
        if ((rc = m_commands_later.put(func_context_idx, 100)) != 0 ) {
            // cannot schedule at this time, abort
            func_context->dec_ref_count();
            return -1;
        }
        return 0;
    }


    void process(uint32_t timeout_ms) {
        enqueue_delayed_commands();
        size_t func_context_idx = 0;
        if (0 == m_commands.pop(&func_context_idx, timeout_ms)) {
            FuncContext* func_context = m_func_contexts[func_context_idx];
            timeout_ms = 0;
            if (func_context->cancelled) {
                func_context->dec_ref_count();
                return;
            }
            // call the function
            func_context->func();
            if (func_context->interval_ms > 0) {
               // execute after interval time
               func_context->scheduled_ms = nowMs() + func_context->interval_ms;
               if (m_commands_later.put(func_context_idx, 100) != 0 ) {
                   // cannot schedule at this time, abort execution of this interval function
                   func_context->dec_ref_count();
               }    
            }
            else {
                // set the status to wake up waiting threads
                func_context->status.set();
                func_context->dec_ref_count();
            }
        }
    }

private:
        void enqueue_delayed_commands() {
            FuncContext* func_context = NULL;
            FuncContext* func_context_start = NULL;
            uint32_t now = nowMs();
            int rc;
            size_t func_context_idx = 0;
            // iterate through the pending commands and and put other queue
            while ( (rc = m_commands_later.pop(&func_context_idx, 0)) == 0) {
                func_context = m_func_contexts[func_context_idx];
                if (now >= (uint32_t)func_context->scheduled_ms) {
                    // should run
                    // note that we do not have increase the ref count here because we already did though for the later queue
                    if ((rc = m_commands.put(func_context_idx, 0)) != 0 ) {
                        // cannot schedule, cleanup and fail silently.
                        func_context->dec_ref_count();
                    }
                    // exit here since we messed up the func_context_start loop detection.
                    // This is really bad design and needs to be reimplemented 
                    break;
                }
                else {
                    // reenter the queue
                    m_commands_later.put(func_context_idx, 0);
                }
                // check whether we looped around
                if (func_context == func_context_start) {
                    return;
                }
                // save the start for loop detection
                if (func_context_start == NULL) {
                    func_context_start = func_context;
                }
            }
        }

private:
    QueueT<size_t>   m_commands_later;
    QueueT<size_t>   m_commands;
    std::array<FuncContext*, SIZE> m_func_contexts;

};


} // end ns


