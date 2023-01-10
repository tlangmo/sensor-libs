#include "assert.h"
#include <queue>
#include "../../unittest/catch.hpp"
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>
#include "../sequential_buffer.h"
#include <unistd.h>
#include <mutex>

static uint32_t clock_counter = 0;

struct SeqTestIntStatus {
    SeqTestIntStatus() : status(0) {}

    void set() {
        status = 1;
    }
    void clear() {
        status = 0;
    }
    int wait_for(uint32_t timeout_ms) {
        clock_counter = 0;
        while (status == 0 && clock_counter < timeout_ms) {
            clock_counter++;
        }
        return clock_counter == timeout_ms ? -1 : 0;
    }
    volatile std::atomic<int> status;
};

typedef motesque::SequentialBufferT<std::mutex, SeqTestIntStatus> SequentialBuffer;

TEST_CASE( "ctor") {
    SequentialBuffer sqb(1000);
}

TEST_CASE( "read_empty") {
    SequentialBuffer sqb(1000);
    const uint8_t* dst;
    size_t  request_size = 0;
    REQUIRE(-1 == sqb.request_read(&dst, &request_size, 0));
    REQUIRE(request_size == 0);
}
//
TEST_CASE( "simple writes") {
    SequentialBuffer sqb(200);
    uint8_t data[201];
    // it is to large
    REQUIRE(-1 == sqb.write(data, 201));
    // it should fit
    REQUIRE(0 == sqb.write(data, 200));
    REQUIRE(200 == sqb.size());
    // it is now full
    REQUIRE(-1 == sqb.write(data, 1));
}


TEST_CASE( "wrapping writes") {
    SequentialBuffer sqb(100);
    uint8_t data[100];
    // it is to large
    REQUIRE(0 == sqb.write(data, 50));
    REQUIRE(0 == sqb.write(data, 50));
    REQUIRE(100 == sqb.size());
    REQUIRE(0 == sqb.free());
    REQUIRE(-1 == sqb.write(data, 50));
    const uint8_t* read_ptr;
    size_t available;
    REQUIRE(0 == sqb.request_read(&read_ptr, &available, 0));
    REQUIRE(100 == available);
    REQUIRE(0 == sqb.commit_read(50));
    REQUIRE(0 == sqb.commit_read(50));
    REQUIRE(-1 == sqb.request_read(&read_ptr, &available, 0));
    REQUIRE(0 == available);
}

TEST_CASE( "wrapping writes shift") {
    SequentialBuffer sqb(100);
    uint8_t data[100];
    // write 20 bytes
    REQUIRE(0 == sqb.write(data, 20));
    // -----------------
    // |  |
    // r  w
    const uint8_t* read_ptr;
    size_t available;
    REQUIRE(0 == sqb.request_read(&read_ptr, &available, 0));
    REQUIRE(20 == available);
    REQUIRE(0 == sqb.commit_read(10));
    REQUIRE(0 == sqb.commit_read(10));
    // -----------------
    //    |
    //   w/r
    REQUIRE(0 == sqb.write(data, 100));
    // -----------------
    //    |
    //   w/r
    // should read until the end
    REQUIRE(0 == sqb.request_read(&read_ptr, &available, 0));
    REQUIRE(80 == available);
    REQUIRE(0 == sqb.commit_read(80));
    // should read the rest up front
    REQUIRE(0 == sqb.request_read(&read_ptr, &available, 0));
    REQUIRE(20 == available);
    // indemptotent
    REQUIRE(0 == sqb.request_read(&read_ptr, &available, 0));
    REQUIRE(20 == available);
    REQUIRE(0 == sqb.commit_read(20));
    REQUIRE(-1 == sqb.request_read(&read_ptr, &available, 0));
    REQUIRE(0 == available);
}

TEST_CASE( "random write read")
{
    SequentialBuffer sqb(501);
    uint8_t data[100];
    srand(100);

    for (int i=0 ; i < 100000; ++i) {
        size_t to_write = rand() % 100;
        //printf("to_write=%lu\n",to_write);
        bool should_read = rand() % 5 == 1;
        const uint8_t* read_ptr;
        size_t available = 0;
        if (0 == sqb.write(data, to_write)) {
            if (should_read) {
                size_t available;
                // since we just wrote some, there is always something to read
                REQUIRE(0 == sqb.request_read(&read_ptr, &available, 0));
                REQUIRE(available > 0);
                REQUIRE(0 == sqb.commit_read(available));
               // printf("read=%lu\n",available);
            }
        }
        else  {
            //write failed, try to read
            if (sqb.request_read(&read_ptr, &available, 0) == 0) {
                REQUIRE(available > 0);
                REQUIRE(0 == sqb.commit_read(available));
               // printf("read=%lu\n",available);
            }
        }
    }
}


TEST_CASE( "write_commit_read_with_timeout")
{
    SequentialBuffer sqb(1000);
    const uint8_t* data;
    size_t available = 0;
    REQUIRE(-1 == sqb.request_read(&data, &available, 1000));
    REQUIRE(0 == available);
    // the read should have timed out and the clock have  advanced to 1000 ticks
    REQUIRE(1000 == clock_counter);
    uint8_t buf[100];
    REQUIRE(0 == sqb.write(buf, sizeof(buf)));
    REQUIRE(0 == sqb.request_read(&data, &available, 1000));
    REQUIRE(100 == available);
    REQUIRE(clock_counter == 0);
}



