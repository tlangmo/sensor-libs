// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================

#pragma once
#include "http_result.h"
//#include "lw_event_trace.h"
#include <cstdio>
#include <vector>
#include <array>
#include <string.h>
namespace motesque
{
int parse_range_header(const std::string& str, size_t* start);

// The Payload represents the various content sources for an HttpRespsonse
// It either owns the complete response text or builds it iteratively
class Payload
{
public:
    // get a read-only pointer to the underlying data. This call is indempodent since
    // subsequent tcp writes can fail. Commit the read to advance the data source
    virtual int get_read_ptr(const char** outData, size_t* outDataSize) = 0 ;
    // Advance the underlying data pointer. Used after the data was transmitted successfully
    virtual HttpResult commit_read(size_t dataSize) = 0;
    // Returns the total content size.
    virtual size_t size() const = 0;
    virtual const std::vector<KeyValuePair>& get_header_fields() const = 0;
};

// The simplest payload, no content
class PayloadEmpty : public Payload
{
public:
    PayloadEmpty();
    virtual ~PayloadEmpty();
    int get_read_ptr(const char** outData, size_t* outDataSize) ;
    HttpResult commit_read(size_t dataSize);
    size_t size() const {
        return 0;
    }
    const std::vector<KeyValuePair>& get_header_fields() const {
        return m_headers;
    }
private:
    std::vector<KeyValuePair> m_headers;
};

// A simple text payload, non-chunked. Can be json, ascii etc.
class PayloadJson : public Payload
{
public:
    PayloadJson(const std::string& txt);
    PayloadJson(const std::string* txt);
    PayloadJson(const char* txt);
    virtual ~PayloadJson();
    int get_read_ptr(const char** outData, size_t* outDataSize);
    HttpResult commit_read(size_t dataSize);
    size_t size() const;

    const std::vector<KeyValuePair>& get_header_fields() const {
        return m_headers;
    }
    void init_default_headers();
private:
    std::string  m_text;
    const std::string* m_text_ptr;
    // keeps track of how much is written already
    size_t m_text_pos;
    std::vector<KeyValuePair> m_headers;
};



// A simple text payload, non-chunked. Can be json, ascii etc.
template<size_t SIZE>
class PayloadBlob : public Payload
{
public:
    PayloadBlob(const std::array<uint8_t, SIZE>& data) :
        m_data(data), m_pos(0)
    {
        init_default_headers();
    }
    virtual ~PayloadBlob()
    {

    }

    int get_read_ptr(const char** out_data, size_t* out_data_size)
    {
        *out_data = (const char*)m_data.data() + m_pos;
        *out_data_size =  m_data.size() - m_pos;
        return 0;
    }

    HttpResult commit_read(size_t data_size)
    {
        m_pos += data_size;
        return m_pos == m_data.size() ? HttpResult_Complete : HttpResult_Incomplete;
    }

    size_t size() const {
        return m_data.size();
    }

    const std::vector<KeyValuePair>& get_header_fields() const {
        return m_headers;
    }

    void init_default_headers() {
         m_headers.push_back(KeyValuePair("Content-Type", "application/octet-stream"));
         char buf[64];
		 memset(buf,0,sizeof(buf));
		 snprintf(buf, sizeof(buf),"%d", (int)size());
		 m_headers.push_back(KeyValuePair("Content-Length", buf));
         m_headers.push_back(KeyValuePair("Connection", "close"));
    }
private:
    std::array<uint8_t, SIZE> m_data;
    size_t m_pos;
    std::vector<KeyValuePair> m_headers;
};


// A simple test payload, sends continously the same data
class PayloadTest : public Payload
{
public:
    PayloadTest(size_t size);
    virtual ~PayloadTest();
    int get_read_ptr(const char** outData, size_t* outDataSize) ;
    HttpResult commit_read(size_t dataSize);
    size_t size() const;
    const std::vector<KeyValuePair>& get_header_fields() const {
        return m_headers;
    }
private:
    char m_domain[1024];
    // keeps track of how much is written already
    size_t m_size;
    size_t m_size_pos;
    std::vector<KeyValuePair> m_headers;
};

} // end ns
