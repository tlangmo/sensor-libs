// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#include <array>
#include "../../unittest/catch.hpp"
#include "http_payload_file.h"
#include "sequential_buffer.h"
#include <mutex>
using namespace motesque;

static uint32_t clock_counter = 0;
struct IntStatus {
    IntStatus() : status(0) {}

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
    std::atomic<int> status;
};

struct TestFile {

};

//TEST_CASE( "payloadfile") {
//    typedef SequentialBufferT<std::mutex, IntStatus> SequentialBufferTest;
//    size_t kFileSize = 1000;
//    size_t file_bytes_read = 0;
//    auto io_read = [&](TestFile* file, uint8_t* data, uint64_t data_size,  size_t* bytes_read)  -> int{
//        *bytes_read = std::min<size_t>(data_size, kFileSize-file_bytes_read);
//        file_bytes_read += *bytes_read;
//        memset(data,0xfd,*bytes_read);
//        return 0;
//    };
//    auto io_open = [](TestFile* file) -> int {
//        return 0;
//    };
//    auto io_close = [](TestFile* file) -> int {
//        return 0;
//    };
//    auto io_size = [&](const TestFile* file) -> size_t {
//        return kFileSize;
//    };
//
//    PayloadFile<SequentialBufferTest, TestFile> pl(io_read,io_open,io_close,io_size);
//    REQUIRE(pl.size() == 1000);
//    const char* data;
//    size_t      available_data;
//    REQUIRE( 0 == pl.get_read_ptr(&data, &available_data));
//    REQUIRE(available_data == kFileSize);
//    REQUIRE( HttpResult_Incomplete == pl.commit_read(available_data));
//    REQUIRE( -1 == pl.get_read_ptr(&data, &available_data));
//}



