// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#include <algorithm>
#include <cstring>
#include "http_response.h"
#include "http_payload.h"
#include "http_utils.h"

namespace motesque
{

HttpResponse::HttpResponse()
: m_status_code(0),
  // an empty payload is standard
  m_payload(),
  m_header(),
  m_header_fields(),
  m_header_pos(0) {
    set_payload(std::make_shared<PayloadEmpty>());
}

HttpResponse::~HttpResponse() {

}

int HttpResponse::build_header_string(const std::vector<KeyValuePair>& header_fields, std::string* header) {
    // reserve some space to avoid reallocations later.
    // the 'stream' is not efficient, but it is easier to read and safer than snprintf...
    header->reserve(128);
    // if we have an invalid status, return
    if (StatusCodes::codes.find(m_status_code) == StatusCodes::codes.end()) {
        return -1;
    }
    (*header) << "HTTP/1.1 " << m_status_code << " " << StatusCodes::codes[m_status_code] << "\r\n";
    std::for_each(header_fields.begin(),header_fields.end(), [&](const KeyValuePair& kv) {
        (*header) << kv.first << ": " << kv.second << "\r\n";
    });
    // end of headers
    (*header) << "\r\n";
    return 0;
}

int HttpResponse::get_read_ptr(const char** outData, size_t* outDataSize) {
    if (m_header.empty()) {
        // build header string first time
        if (build_header_string(m_header_fields, &m_header)) {
            // error case. could happen for invalid status code for example
            return -1;
        }
        m_header_pos = 0;
    }
    if (m_header_pos < m_header.size()) {
        // process header
        *outData = m_header.c_str()    + m_header_pos;
        *outDataSize = m_header.size() - m_header_pos;
    }
    else {
        // process payload
        return m_payload->get_read_ptr(outData, outDataSize);
    }
    return 0;
}

HttpResult HttpResponse::commit_read(size_t dataSize) {
    if (m_header_pos == m_header.size()) {
        return m_payload->commit_read(dataSize);
    }
    m_header_pos += dataSize;
    // when we still process the header, it can never be complete
    return HttpResult_Incomplete;
}

int HttpResponse::status_code() const {
    return m_status_code;
}

void HttpResponse::set_status_code(int status) {
    m_status_code = status;
}

std::shared_ptr<Payload> HttpResponse::payload() const {
    return m_payload;
}

void HttpResponse::set_payload(std::shared_ptr<Payload> payload) {
    std::string content_size;
    content_size << payload->size();
    m_header_fields.clear();
    //add_header_field(KeyValuePair("Content-Length", content_size));
    for (auto it = payload->get_header_fields().begin(); it != payload->get_header_fields().end(); it++) {
        add_header_field(*it);
    }
    m_payload = payload;
}

void HttpResponse::add_header_field(const KeyValuePair& kv) {
    m_header_fields.push_back(kv);
}

}; // end ns
