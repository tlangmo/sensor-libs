// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#include "../../unittest/catch.hpp"
#include "http_request.h"
#include <cstring>
using namespace motesque;

TEST_CASE( "request ctor") {
    HttpRequest req;
    const std::string* value;
    REQUIRE(req.query_param("name",&value) < 0);
    REQUIRE(req.header_field("name",&value) < 0);
    REQUIRE(req.header_field("content",&value) < 0);
}

TEST_CASE( "request read") {
    HttpRequest req;
    SECTION( "valid GET minimal" ) {
        char data[] = "GET / HTTP/1.1\r\n\r\n";
        REQUIRE(req.read_from(data,strlen(data)) == HttpResult_Complete);
        REQUIRE(req.read_from(data,0) == HttpResult_Complete);
    }
    SECTION( "invalid GET minimal" ) {
        char data[] = "GET / HTTP-1.1\r\n\r\n";
        REQUIRE(req.read_from(data,strlen(data)) == HttpResult_Error);
    }
    SECTION( "valid GET with headers split" ) {
        char data[] = "GET / HTTP/1.1\r\nHost: www.googe.com\r\nAccept: */*\r\n\r\n";
        REQUIRE(req.read_from(data,5) == HttpResult_Incomplete);
        REQUIRE(req.read_from(data+5,strlen(data)-5) == HttpResult_Complete);
        REQUIRE(req.method() == HTTP_GET);
        const std::string* value;
        REQUIRE(req.header_field("Host",&value) == 0);
        REQUIRE(*value == std::string("www.googe.com"));
        REQUIRE(req.header_field("Accept",&value) == 0);
        REQUIRE(*value == std::string("*/*"));
    }
    SECTION( "valid GET with path and headers" ) {
        char data[] = "GET /this/is/a/path HTTP/1.1\r\nHost: www.googe.com\r\nAccept: */*\r\n\r\n";
        REQUIRE(req.read_from(data,strlen(data)) == HttpResult_Complete);
        REQUIRE(req.method() == HTTP_GET);
        const std::string* value;
        REQUIRE(req.header_field("Host",&value) == 0);
        REQUIRE(*value == std::string("www.googe.com"));
        REQUIRE(req.header_field("Accept",&value) == 0);
        REQUIRE(*value == std::string("*/*"));
    }
    SECTION( "valid GET with one query param" ) {
        char data[] = "GET /this/is/a/path?payload=nothing HTTP/1.1\r\n\r\n";
        REQUIRE(req.read_from(data,strlen(data)) == HttpResult_Complete);
        const std::string* value;
        REQUIRE(req.query_param("payload",&value) == 0);
        REQUIRE(*value == std::string("nothing"));
    }
    SECTION( "valid GET with two escaped query params" ) {
        //https://www.w3schools.com/tags/ref_urlencode.asp
        char data[] = "GET /this/is/a/path?payload=nothing&message=%C3%BCmlauts%20are%20great%3C%3E HTTP/1.1\r\n\r\n";
        REQUIRE(req.read_from(data,12) == HttpResult_Incomplete);
        REQUIRE(req.read_from(data+12,strlen(data)-12) == HttpResult_Complete);
        const std::string* value;
        REQUIRE(req.path() == std::string("/this/is/a/path"));
        REQUIRE(req.query_param("payload",&value) == 0);
        REQUIRE(*value == std::string("nothing"));
        REQUIRE(req.query_param("message",&value) == 0);
        REQUIRE(*value == std::string("ümlauts are great<>"));
    }
    SECTION( "invalid GET with two  query params" ) {
        //https://www.w3schools.com/tags/ref_urlencode.asp
        char data[] = "GET /this/is/a/path?paylo ad=nothing HTTP/1.1\r\n\r\n";
        REQUIRE(req.read_from(data,12) == HttpResult_Incomplete);
        REQUIRE(req.read_from(data+12,strlen(data)-12) == HttpResult_Error);
        const std::string* value;
        // the request failed in the header part, should have no valid path then
        REQUIRE(req.path() == "");
        REQUIRE(req.query_param("payload",&value) < 0);
    }
    SECTION( "valid PUT with body" ) {
        //https://www.w3schools.com/tags/ref_urlencode.asp
        char data[] = "PUT /this/is/a/path HTTP/1.1\r\nContent-Length: 20\r\n\r\nAn important message\r\n";
        REQUIRE(req.read_from(data,3) == HttpResult_Incomplete);
        REQUIRE(req.read_from(data+3,strlen(data)-3) == HttpResult_Complete);
        const std::string* value;
        // the request failed in the header part, should have no valid path then
        REQUIRE(req.method() == HTTP_PUT);
        REQUIRE(req.path() == "/this/is/a/path");
        REQUIRE(req.body() == "An important message");
        REQUIRE(req.header_field("Content-Length",&value) == 0);
        REQUIRE(*value == std::string("20"));
    }
    SECTION( "invalid PUT with body" ) {
        // message is missing a \r\n
        char data[] = "PUT /this/is/a/path HTTP/1.1\r\nContent-Length: 20\r\nAn important message\r\n";
        REQUIRE(req.read_from(data,3) == HttpResult_Incomplete);
        REQUIRE(req.read_from(data+3,strlen(data)-3) == HttpResult_Error);
    }
}

static void reqA(const HttpRequest& request, HttpResponse* response)
{
    //printf("reqA\n");
}
static void reqB(const HttpRequest& request, HttpResponse* response)
{
    //printf("reqB\n");
}

TEST_CASE( "find_http_handler") {
    HttpHandlerMap routes;
    SECTION( "get simple" ) {
        routes[MethodPath(HTTP_GET, "/about")]= reqA;
        HttpHandler handler;
        REQUIRE( -1 == find_http_handler(routes,HTTP_GET,"/ab",   false, &handler));
        REQUIRE( 0 == find_http_handler(routes,HTTP_GET,"/about", true, &handler));
        REQUIRE( -1 == find_http_handler(routes,HTTP_GET,"about", false, &handler));
    }
    SECTION( "multiple methods" ) {
       routes[MethodPath(HTTP_GET, "/about", MethodPath::TLS_MATCH_BOTH)] = reqA;
       routes[MethodPath(HTTP_POST, "/about")]= reqB;
       routes[MethodPath(HTTP_GET, "/volume/*", MethodPath::TLS_MATCH_NONE)]= reqB;
       REQUIRE(routes.size() == 3);
       HttpHandler handler;
       REQUIRE( -1 == find_http_handler(routes,HTTP_POST,"/ab", false, &handler));

       REQUIRE( 0 == find_http_handler(routes,HTTP_POST,"/about",true,&handler));
       REQUIRE(handler);
       handler(HttpRequest(), nullptr);
       REQUIRE( 0 == find_http_handler(routes,HTTP_GET,"/about",false, &handler));
       REQUIRE(handler);
       handler(HttpRequest(), nullptr);
       REQUIRE( 0 == find_http_handler(routes, HTTP_GET,"/volume/*",false, &handler));
   }
}




