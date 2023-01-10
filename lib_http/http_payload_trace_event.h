#pragma once
#include "http_payload.h"
#include "http_result.h"
#include "lw_event_trace.h"
#include <vector>
#include <numeric>
#ifndef __linux__
#include "wiced.h"
#else

#endif

namespace motesque
{

class PayloadTraceEvent : public Payload
{
public:
	PayloadTraceEvent(const std::vector<lw_event_trace::TraceEvent>& events) : 
		m_events(events), 
		m_cur_event_json(),
		m_cur_event_json_cursor(0),
		m_event_index(0)
	{
		m_headers.push_back(KeyValuePair("Content-Type", "application/json"));
		char buf[64];
		memset(buf,0,sizeof(buf));
		snprintf(buf, sizeof(buf),"%d", (int)size());
		m_headers.push_back(KeyValuePair("Content-Length", buf));
		m_headers.push_back(KeyValuePair("Connection", "close"));
	}

	virtual  ~PayloadTraceEvent() {

	}

	const std::vector<KeyValuePair>& get_header_fields() const {
        return m_headers;
    } 

	static size_t to_json_size(int a, const lw_event_trace::TraceEvent& b) {
		static std::string tmp;
		lw_event_trace::to_json(b, &tmp);
		return tmp.size() + (size_t)a;
	}

	size_t size() const {
	//	WPRINT_APP_INFO(("payload_size %d\n",(int)m_events.size()));
		size_t payload_size=  (size_t)std::accumulate(m_events.begin(),m_events.end(), 0, to_json_size)+1; // accunt for the  list start character
		//WPRINT_APP_INFO(("payload_size %d\n",(int)payload_size));
		return payload_size;
	}

 	int get_read_ptr(const char** outData, size_t* outDataSize) {
		if (m_cur_event_json_cursor == m_cur_event_json.size() && m_event_index < m_events.size()) {
			lw_event_trace::to_json(m_events[m_event_index], &m_cur_event_json);
			if (m_event_index == 0) {
				m_cur_event_json = '['+m_cur_event_json;
			}
			if (m_event_index == m_events.size()-1) {
				m_cur_event_json[m_cur_event_json.size()-1] = ']'; // convert the last trailing comma to ']'
			}
			m_cur_event_json_cursor = 0;
			m_event_index++;
		}
	 	*outData = m_cur_event_json.c_str() + m_cur_event_json_cursor;
		*outDataSize = m_cur_event_json.size() - m_cur_event_json_cursor;
		return 0;
	}

	HttpResult commit_read(size_t dataSize)
    {
        m_cur_event_json_cursor += dataSize;
		return (m_cur_event_json_cursor == m_cur_event_json.size() && m_event_index >= m_events.size()) ? 
			HttpResult_Complete: HttpResult_Incomplete;
    }

private:
	const std::vector<lw_event_trace::TraceEvent>& m_events;
	std::string m_cur_event_json;
	size_t 	   m_cur_event_json_cursor;
	size_t 	   m_event_index;
	std::vector<KeyValuePair> m_headers;
};



} // end ns
