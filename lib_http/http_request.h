// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <algorithm>
#include "http_parser.h"
#include "http_result.h"

namespace motesque
{
int decode_uri_escape_sequences(std::string& src, std::string* dst);
typedef http_method HttpMethod;

// Encapsulates a HTTP 1 request. A request is build iteratively by calling HttpRequest::readFrom() with whatever
// data chunk was read from the stream.
// Since it is a client facing object, the read function is only accessible by using a subclass.
class HttpRequest {
public:
    // parsing functions which neeed to  access all members
    friend int on_header_field(http_parser* parser, const char *at, size_t length);
    friend int on_header_value(http_parser* parser, const char *at, size_t length);
    friend int on_url(http_parser* parser, const char *at, size_t length);
    friend int on_body(http_parser* parser, const char *at, size_t length);
    friend int on_headers_complete(http_parser* parser);
    friend int on_message_complete(http_parser* parser);

    HttpRequest();
    virtual ~HttpRequest();

    // Get a query parameter value by name
    // Returns: 0 on success, else  -1
    int query_param(const char* name, const std::string** value) const;
    const std::vector<KeyValuePair>& query_params() const;

    // Get a field value value by name
    // Returns: 0 on success, else  -1
    int header_field(const char* name, const std::string** value) const;
    const std::vector<KeyValuePair>& header_fields() const;

    const std::string& body() const;
    const std::string& url() const;
    const std::string& path() const;
    HttpMethod method() const;
    void set_remote_ip(uint32_t ip_addr);
    uint32_t get_remote_ip() const;

    // progressively read the Http Request from data stream.
    // Call repeatedly with new data until it return HttpState_Complete or HttpState_Error
    HttpResult read_from(const char* data, size_t data_size);

private:
    http_parser_settings      m_settings;
    http_parser               m_parser;
    // query params decoded from url
    std::vector<KeyValuePair> m_query_parameters;
    // header fields
    std::vector<KeyValuePair> m_header_fields;
    // the raw body
    std::string               m_body;
    // the complete url
    std::string               m_url;
    // just the path component of url
    std::string               m_path;
    // keeps track of the current state of the request, incomplete, error etc
    HttpResult                m_state;
    uint32_t                  m_remote_ip;
};

class HttpResponse;
typedef std::function<void(const HttpRequest& request, HttpResponse* response)> HttpHandler;
struct MethodPath
{
    MethodPath(int _method, const std::string& _path, int _tls_match) :
        method(_method),path(_path),tls_match(_tls_match)
    {

    }

    MethodPath(int _method, const std::string& _path) :
           method(_method), path(_path), tls_match(TLS_MATCH_ONLY)
    {
    }
    enum  {
        TLS_MATCH_NONE = 0, // only http
        TLS_MATCH_ONLY = 1, // only tls
        TLS_MATCH_BOTH = 2  // both
    };
    int method;
    std::string path;
    int tls_match;
};

bool operator<(const MethodPath& lhs, const MethodPath& rhs);

//typedef std::pair<int, std::string> MethodPath;
// associates a Method/Path pair with a handler function
typedef std::map<MethodPath, HttpHandler> HttpHandlerMap;

// searches the map for the correct handler;
int find_http_handler(const HttpHandlerMap& routes, HttpMethod method,
        const std::string& path, bool is_tls, HttpHandler* handler);

}; // end ns
