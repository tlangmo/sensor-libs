// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================

#include <assert.h>
#include <cstring>
#include <string>
#include <cstdlib>
#include <algorithm>
#include "http_payload.h"
#include "http_result.h"
#include "http_utils.h"


namespace motesque
{

int parse_range_header(const std::string& str, size_t* start)
{
    char ptn_bytes[]="bytes=";
    int pos_s = -1;
    int pos_dash = -1;
    for (size_t i=0; i < str.length(); i++) {
        if (i < strlen(ptn_bytes)) {
            if (str[i] != ptn_bytes[i]) {
                return -1;
            }
        }
        else {
            // first range
            if (pos_s < 0) {
                pos_s = i;
            }
            if (str[i] == '-') {
                pos_dash = i;
            }
        }
    }
    if ((int)str.length() > pos_dash+1){
        // we only support open ranges
        return -1;
    }
    // we need to make sure that stoul will work. Otherwise an exception is thrown.
    if (isdigit(str[pos_s])) {
        *start = strtol(str.substr(pos_s, pos_dash-pos_s).c_str(), nullptr, 10);
        return 0;
    }
    return -1;
}



PayloadEmpty::PayloadEmpty()
{
    m_headers.push_back(KeyValuePair("Content-Length", "0"));
    m_headers.push_back(KeyValuePair("Connection", "close"));
}


PayloadEmpty::~PayloadEmpty()
{
}

int PayloadEmpty::get_read_ptr(const char** out_data, size_t* out_data_size)
{
    *out_data = nullptr;
    *out_data_size = 0;
    return 0;
}
HttpResult PayloadEmpty::commit_read(size_t data_size)
{
    return HttpResult_Complete;
}


PayloadJson::PayloadJson(const std::string& txt)
: m_text(txt),
  m_text_ptr(&m_text),
  m_text_pos(0)
{
   init_default_headers();
}

PayloadJson::PayloadJson(const char* txt)
: m_text(txt),
  m_text_ptr(&m_text),
  m_text_pos(0)
{
      init_default_headers();
}

PayloadJson::PayloadJson(const std::string* txt)
: m_text(),
  m_text_ptr(txt),
  m_text_pos(0)
{
    init_default_headers();
}

void PayloadJson::init_default_headers()
{
    std::string content_size;
    content_size << size();
    m_headers.push_back(KeyValuePair("Content-Length", content_size));
    m_headers.push_back(KeyValuePair("Content-Type", "application/json"));
    m_headers.push_back(KeyValuePair("Connection", "close"));
}

PayloadJson::~PayloadJson()
{

}

int PayloadJson::get_read_ptr(const char** out_data, size_t* out_data_size)
{
    *out_data = m_text_ptr->c_str() + m_text_pos;
    *out_data_size =  m_text_ptr->size() - m_text_pos;
    return 0;
}

HttpResult PayloadJson::commit_read(size_t data_size)
{
    m_text_pos += data_size;
    return m_text_pos ==  m_text_ptr->size() ? HttpResult_Complete : HttpResult_Incomplete;
}

size_t PayloadJson::size() const {
    return (int) m_text_ptr->size();
}


PayloadTest::PayloadTest(size_t size) : m_size(size), m_size_pos(0) {
    memset(m_domain, 0xAC, sizeof(m_domain));

    std::string content_size;
    content_size << m_size;
    m_headers.push_back(KeyValuePair("Content-Length", content_size));
    m_headers.push_back(KeyValuePair("Content-Type", "application/octet-stream"));
    m_headers.push_back(KeyValuePair("Connection", "close"));
}

PayloadTest::~PayloadTest()
{

}

int PayloadTest::get_read_ptr(const char** outData, size_t* outDataSize)
{
    *outData = m_domain;
    //size_t avail = m_size-m_size_pos;
    *outDataSize = std::min<size_t>(m_size-m_size_pos, sizeof(m_domain));
    return 0;
}

HttpResult PayloadTest::commit_read(size_t dataSize)
{
    m_size_pos += dataSize;
    return m_size_pos == m_size ? HttpResult_Complete : HttpResult_Incomplete;
}

size_t PayloadTest::size() const
{
    return m_size;
}

} // end ns
