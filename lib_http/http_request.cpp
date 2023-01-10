// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#include <string.h>

#include "http_request.h"

#ifndef MOTESQUE_PLATFORM_X86
#include "wwd_debug.h"
#else
#define WPRINT_APP_DEBUG(args) printf args
#define WPRINT_APP_INFO(args) printf args
/* Suppress unused variable warning */
#ifndef UNUSED_VARIABLE
#define UNUSED_VARIABLE(x) /*@-noeffect@*/ ( (void)(x) ) /*@+noeffect@*/
#endif
#endif

namespace motesque
{

bool operator<(const MethodPath& lhs, const MethodPath& rhs) {
    return (lhs.method < rhs.method) || ((lhs.method == rhs.method) && (lhs.path < rhs.path));
}


int on_header_field(http_parser* parser, const char *at, size_t length) {
    HttpRequest* req = static_cast<HttpRequest*>(parser->data);
    // decide whether this is part of an existing field name or a new one.
    // a new one is either the first one, or already has a value part
    if (req->m_header_fields.empty() || req->m_header_fields.back().second.size() != 0) {
        req->m_header_fields.push_back(KeyValuePair(KeyValuePair::first_type(at, length), KeyValuePair::second_type()));
    }
    else {
        // append on existing one
        req->m_header_fields.back().first.append(at, length);
    }
    return 0;
}

int on_header_value(http_parser* parser, const char *at, size_t length) {
    HttpRequest* req = static_cast<HttpRequest*>(parser->data);
    req->m_header_fields.back().second.append(at, length);
    return 0;
}

int on_url(http_parser* parser, const char *at, size_t length) {
    HttpRequest* req = static_cast<HttpRequest*>(parser->data);
    req->m_url.append(at, length);
    return 0;
}

int on_body(http_parser* parser, const char *at, size_t length) {
    HttpRequest* req = static_cast<HttpRequest*>(parser->data);
    req->m_body.append(at, length);
    return 0;
}

int on_headers_complete(http_parser* parser) {
    HttpRequest* req = static_cast<HttpRequest*>(parser->data);
    // now that the headers are complete we can extract all query fields
    http_parser_url urlParser;
    http_parser_url_init(&urlParser);

    const char* at = req->m_url.c_str();
    int rc = http_parser_parse_url(req->m_url.c_str(), req->m_url.size(), 0, &urlParser);
    req->m_path.assign(at+urlParser.field_data[UF_PATH].off, urlParser.field_data[UF_PATH].len);
    // parse query fields if present
    if (rc == 0 && (urlParser.field_set & 1 << UF_QUERY)) {
        const char* pos = at + urlParser.field_data[UF_QUERY].off;
        const char* end = pos + urlParser.field_data[UF_QUERY].len;
        const char* fieldCursor = pos;
        KeyValuePair  kv;
        //a=one&b=two&c=three
        for (; pos < end; pos++) {
            if (*pos == '=') {
                // everything before '=' must be field name
                kv.first.assign(fieldCursor, pos - fieldCursor);
                // skip the '='
                fieldCursor = pos + 1;
            }
            if (*pos == '&') {
                // everything before '&' must be field value
                kv.second.assign(fieldCursor, pos - fieldCursor);
                fieldCursor = pos + 1;
                decode_uri_escape_sequences(kv.first,&kv.first);
                decode_uri_escape_sequences(kv.second,&kv.second);
                req->m_query_parameters.push_back(kv);
            }
        }
        // assign the last value now since it does not have a trailing '&'
        kv.second.assign(fieldCursor, pos - fieldCursor);
        decode_uri_escape_sequences(kv.first,&kv.first);
        decode_uri_escape_sequences(kv.second,&kv.second);
        req->m_query_parameters.push_back(kv);
    }
    return 0;
}

int on_message_complete(http_parser* parser) {
    HttpRequest* req = static_cast<HttpRequest*>(parser->data);
    req->m_state = HttpResult_Complete;
    return 0;
}

HttpRequest::HttpRequest()
: m_settings(),
  m_parser(),
  m_query_parameters(),
  m_header_fields(),
  m_body(),
  m_path(),
  m_state(HttpResult_Incomplete),
  m_remote_ip(0) {
    memset(&m_settings,0,sizeof(m_settings));
    memset(&m_parser, 0, sizeof(m_parser));
    http_parser_settings_init(&m_settings);
    // setup all callbacks.
    m_settings.on_url = on_url;
    m_settings.on_header_field = on_header_field;
    m_settings.on_header_value = on_header_value;
    m_settings.on_body = on_body;
    m_settings.on_headers_complete = on_headers_complete;
    m_settings.on_message_complete = on_message_complete;
    memset(&m_parser, 0, sizeof(http_parser));
    http_parser_init(&m_parser, HTTP_REQUEST);
    m_parser.data = this; // hook to request object
}

HttpRequest::~HttpRequest() {

}


void HttpRequest::set_remote_ip(uint32_t ip_addr)
{
    m_remote_ip = ip_addr;
}

uint32_t HttpRequest::get_remote_ip() const
{
    return m_remote_ip;
}

HttpMethod HttpRequest::method() const {
    return (HttpMethod)m_parser.method;
}

const std::string& HttpRequest::path() const {
    return m_path;
}

const std::string& HttpRequest::body() const {
    return m_body;
}

const std::string& HttpRequest::url() const {
    return m_url;
}

int HttpRequest::query_param(const char* name, const std::string** value) const {
    for (auto it = m_query_parameters.begin(); it!= m_query_parameters.end(); it++) {
        const KeyValuePair& kv = *it;
        if (strcmp(kv.first.c_str(),name) == 0) {
            *value = &kv.second;
            return 0;
        }
    }
    return -1;
}

int HttpRequest::header_field(const char* name, const std::string** value) const {
    for (auto it = m_header_fields.begin(); it!= m_header_fields.end(); it++) {
        const KeyValuePair& kv = *it;
        if (strcmp(kv.first.c_str(), name) == 0) {
            *value = &kv.second;
            return 0;
        }
    }
    return -1;
}

HttpResult HttpRequest::read_from(const char* data, size_t data_size) {
    //WPRINT_APP_INFO(("read_from %d, %s", data_size, std::string(data, data_size).c_str()));
    if (m_state != HttpResult_Incomplete) {
        // do nothing if the request was already processed completely
        return m_state;
    }
    size_t nparsed = http_parser_execute(&m_parser, &m_settings, data, data_size);
    //WPRINT_APP_INFO(("stats parsed=%d, dataSize=%d, http_errno=%d, http_errno_desc=\"%s\"\r\n", nparsed, data_size,m_parser.http_errno,
    //            http_errno_description((http_errno)m_parser.http_errno)));
    if (nparsed != data_size) {
        //WPRINT_APP_DEBUG(("WARNING parsed=%d, dataSize=%d, http_errno=%d, http_errno_desc=\"%s\"\r\n", nparsed, data_size,m_parser.http_errno,
       //         http_errno_description((http_errno)m_parser.http_errno)));
        return HttpResult_Error;
    }
    return m_state;
}

int decode_uri_escape_sequences(std::string& src, std::string* dst) {
    size_t nLength;
    if (src.size() > dst->size()) {
        return -1;
    }
    char *sSource = (char*)src.c_str();
    char *sDest = (char*)dst->c_str();
    for (nLength = 0; sSource < (src.c_str()+src.size()); nLength++) {
        if (*sSource == '%' && sSource[1] && sSource[2] && isxdigit(sSource[1]) && isxdigit(sSource[2])) {
            sSource[1] -= sSource[1] <= '9' ? '0' : (sSource[1] <= 'F' ? 'A' : 'a')-10;
            sSource[2] -= sSource[2] <= '9' ? '0' : (sSource[2] <= 'F' ? 'A' : 'a')-10;
            sDest[nLength] = 16 * sSource[1] + sSource[2];
            sSource += 3;
            continue;
        }
        sDest[nLength] = *(sSource++);
    }
    dst->erase(dst->begin()+nLength, dst->end());
    return 0;
}

int find_http_handler(const HttpHandlerMap& routes,
        HttpMethod method,
        const std::string& path,
        bool is_tls,
        HttpHandler* handler)
{
    auto itCb = std::find_if(routes.begin(), routes.end(), [&](const HttpHandlerMap::value_type& value) -> bool {
        const MethodPath& mp = value.first;
        // check for tls first
        if ( (mp.tls_match == MethodPath::TLS_MATCH_NONE && is_tls) || (mp.tls_match == MethodPath::TLS_MATCH_ONLY && !is_tls)) {
            return false;
        }

        if (mp.path.length() > path.length() || mp.method != method) {
            // that never matches
            return false;
        }
        else {
            auto matchPair = std::mismatch(mp.path.begin(),mp.path.end(),
                    path.begin());
            // we have a match if the cb url has an * at the first mismatch, or if both paths are the same
            return *matchPair.first == *matchPair.second || *matchPair.first == '*';
        }
        return false;
    });
    if (itCb != routes.end()) {
        *handler = itCb->second;
        return 0;
    }
    return -1;
}

}; // end ns
