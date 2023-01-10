// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "http_result.h"

namespace motesque
{

class Payload;

// Encapsulates a HTTP 1 response. A response has a payload and a status code.
class HttpResponse {
public:
    HttpResponse();
    virtual ~HttpResponse();
    int status_code() const;
    void set_status_code(int status);
    std::shared_ptr<Payload> payload() const;
    void set_payload(std::shared_ptr<Payload> payload);
    // add a header field to the response, in addition to default ones
    void add_header_field(const KeyValuePair& kv);
    // get a read-only pointer to the underlying data. This call is indempotent since
    // subsequent tcp writes can fail. Commit the read to advance the data source
    int get_read_ptr(const char** outData, size_t* outDataSize);
    // Advance the underlying data pointer. Used after the data was transmitted successfully
    HttpResult commit_read(size_t dataSize);
private:
    // serializes all header fields to a string
    int build_header_string(const std::vector<KeyValuePair>& headerFields, std::string* header );
private:
    // the HTTP status code
    int m_status_code;
    // the payload if any
    std::shared_ptr<Payload> m_payload;
    // the complete header string. built from header fields
    std::string m_header;
    // the header fields of the response
    std::vector<KeyValuePair> m_header_fields;
    // keeps track of how much is written already
    size_t      m_header_pos;
};

}; // end ns
