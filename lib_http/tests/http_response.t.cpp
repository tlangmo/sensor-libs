// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#include <array>
#include "../../unittest/catch.hpp"
#include "http_response.h"
#include "http_payload.h"

using namespace motesque;


TEST_CASE( "response ctor") {
    HttpResponse resp;
    REQUIRE(resp.status_code() == 0);
}

TEST_CASE( "response write empty body") {
    HttpResponse resp;
    std::string message;

    SECTION( "write empty body 1 pass" ) {
        resp.set_status_code(200);
        const char* bufC;
        size_t bufSize;
        int rc = resp.get_read_ptr(&bufC,&bufSize);
        REQUIRE(rc == 0);
        REQUIRE(std::string(bufC,bufSize) == "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");

        // getReadPtr is indempodent
        rc = resp.get_read_ptr(&bufC,&bufSize);
        REQUIRE(rc == 0);
        REQUIRE(std::string(bufC,bufSize) == "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");

        // commit
        HttpResult http_res = resp.commit_read(bufSize);
        REQUIRE(http_res == HttpResult_Incomplete);

        // payload part
        rc = resp.get_read_ptr(&bufC, &bufSize);
        REQUIRE(rc == 0);
        REQUIRE(bufSize == 0);
        http_res = resp.commit_read(bufSize);
        REQUIRE(http_res == HttpResult_Complete);
    }

    SECTION( "write empty body in small pieces" ) {
       resp.set_status_code(200);
       const char* bufC ;
       size_t bufSize;
       resp.get_read_ptr(&bufC, &bufSize);
       message.append(bufC, 10);
       resp.commit_read(10);
       resp.get_read_ptr(&bufC, &bufSize);
       message.append(bufC, 10);
       resp.commit_read(10);

       resp.get_read_ptr(&bufC, &bufSize);
       message.append(bufC, bufSize);
       HttpResult http_res = resp.commit_read(bufSize);
       REQUIRE(http_res == HttpResult_Incomplete);
       resp.get_read_ptr(&bufC, &bufSize);
       http_res =  resp.commit_read(bufSize);
       REQUIRE(http_res == HttpResult_Complete);
       REQUIRE(message == "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
   }
}

TEST_CASE( "response write text body") {
    HttpResponse resp;
    resp.set_payload(std::make_shared<PayloadJson>("this is so exiting"));
    std::string message;
    SECTION( "write text body 1 pass" ) {
        resp.set_status_code(200);
        const char* bufC ;
        size_t bufSize;
        resp.get_read_ptr(&bufC, &bufSize);
        message.append(bufC, bufSize);
        REQUIRE(resp.commit_read(bufSize) == HttpResult_Incomplete);
        resp.get_read_ptr(&bufC, &bufSize);
        message.append(bufC, bufSize);
        REQUIRE(resp.commit_read(bufSize) == HttpResult_Complete);
        REQUIRE(message == "HTTP/1.1 200 OK\r\nContent-Length: 18\r\nContent-Type: application/json\r\nConnection: close\r\n\r\nthis is so exiting");
    }
}



