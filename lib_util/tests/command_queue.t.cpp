#include "assert.h"
#include <queue>
#include "../../unittest/catch.hpp"
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>
#include "../command_queue.h"
#include <unistd.h>
using namespace motesque;

namespace motesque {

void delayMs(uint32_t ms) {
    usleep(ms * 1000);
}

static uint32_t now = 0;

uint32_t nowMs() {
    uint32_t ticks = now;
    now++;
    return ticks;
}

};

template<typename T>
struct QueueTest
{
    QueueTest(size_t size)
    : queueSize(size){

    }
    int put(const T& cmd, int waitMs) {
        std::unique_lock<std::mutex> lck(mutex);
        if (queue.size() < queueSize) {
            queue.push(cmd);
            return 0;
        }
        return -1;
    }
    int pop(T* cmd, int waitMs) {
        std::unique_lock<std::mutex> lck(mutex);
        if (queue.empty()) {
            return -1;
        }
        *cmd = queue.front();
        queue.pop();
        return 0;
    }
    std::queue<T> queue;
    size_t queueSize;
    std::mutex mutex;
};



TEST_CASE( "command queue execute_async with reference") {
    typedef CommandQueue<QueueTest, 1, IntStatus> TestCommandQueue;
    TestCommandQueue cmdQueue;
    int counter = 0;
    auto f = [&counter]() {
        // global obj do sth.
        counter++;
    };
    size_t id = 0;

    int rc = cmdQueue.execute_async(f);
    REQUIRE( rc == 0);
    // only one command can be in queue
    rc = cmdQueue.execute_async(f);
    REQUIRE( rc == -1);
    // it should call the function
    cmdQueue.process(100);
    REQUIRE( counter == 1);
    // it should succeed since the function was called already
    rc = cmdQueue.execute_async(f);
    REQUIRE( rc == 0);
    cmdQueue.process(100);
    REQUIRE( counter == 2);
}
//
TEST_CASE( "command queue execute_async no reference") {
    typedef CommandQueue<QueueTest, 1, IntStatus> TestCommandQueue;
    TestCommandQueue cmdQueue;
    int counter = 0;
    auto f = [&counter]() {
        // global obj do sth.
        counter++;
    };
    size_t id = 0;
    cmdQueue.execute_async(f);
    cmdQueue.process(100);
    REQUIRE( counter == 1);
    cmdQueue.execute_async(f);
    cmdQueue.process(100);
    REQUIRE( counter == 2);
}


TEST_CASE( "command queue execute_async_later") {
    now = 0;
    typedef CommandQueue<QueueTest, 1, IntStatus> TestCommandQueue;
    TestCommandQueue cmdQueue;
    int counter = 0;
    auto f = [&counter]() {
        // global obj do sth.
        counter++;
    };
    {
        int rc = cmdQueue.execute_async_later(f,2);
        REQUIRE( rc == 0);
    }
    // it should not execute the function yet at tick 0
    cmdQueue.process(0);
    REQUIRE( counter == 0);
    // it should execute the function yet at tick 1
    cmdQueue.process(0);
    REQUIRE( counter == 1);

    cmdQueue.process(0);
    REQUIRE( counter == 1);
}

typedef CommandQueue<QueueTest, 3, IntStatus> TestCommandQueue;
volatile bool command_queue_should_process = true;
void command_queue_process(TestCommandQueue& cmdQueue) {
    while (command_queue_should_process) {
        cmdQueue.process(0);
    }
}

TEST_CASE( "command queue execute with wait") {
    TestCommandQueue cmdQueue;
    int counter = 0;
    auto f = [&counter]() {
        // global obj do sth.
        counter++;
    };
    // it shoud execute synchroniously
    const int kNumTries = 100000;
    command_queue_should_process = true;
    std::thread t(command_queue_process, std::ref(cmdQueue));
    auto start = std::chrono::steady_clock::now();
    for (int i=0; i < kNumTries; i++) {
        int rc = cmdQueue.execute_async(f, true);
        REQUIRE( rc  == 0);
    }
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
    command_queue_should_process = false;
    t.join();
    std::cout << "Command Queue Execute elapsed: " << duration << " us, tries: " << kNumTries << std::endl;
    REQUIRE( counter == kNumTries);
}

TEST_CASE( "command queue cancel" ) {
    TestCommandQueue cmdQueue;
    int counter = 0;
    auto f = [&counter]() {
        // global obj do sth.
        counter++;
    };

    cmdQueue.execute_async_later(f, 1);
    cmdQueue.process(0);
    REQUIRE(counter == 1);

    int command_id = 1;
    cmdQueue.execute_async_later(f, 1, command_id);
    cmdQueue.cancel(command_id);
    cmdQueue.process(0);
    cmdQueue.process(0);
    REQUIRE(counter == 1);
}

TEST_CASE( "command queue interval" ) {
    TestCommandQueue cmdQueue;
    int counter = 0;
    auto f = [&counter]() {
        // global obj do sth.
        counter++;
    };

    cmdQueue.execute_async_interval(f, 10, 77);
    for (int i=0; i < 102; i++) {
        cmdQueue.process(0);
    }
    REQUIRE(counter == 10);

    cmdQueue.cancel(77);
    for (int i=0; i < 100; i++) {
        cmdQueue.process(0);
    }
    REQUIRE(counter == 10);

}
