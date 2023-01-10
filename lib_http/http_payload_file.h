#pragma once
#include <functional>
#include "http_payload.h"
#include "http_result.h"
#include <vector>
namespace motesque
{

template<typename BufferT, typename FileT>
class PayloadFile : public Payload
{
public:
    typedef std::function< int (FileT* file, BufferT* sink) >    FileIoOpen;
    typedef std::function< int (FileT* file) >    FileIoClose;
    PayloadFile(FileIoOpen open, FileIoClose close);
    virtual ~PayloadFile();
    // get pointer to the underlying data of this payload
    int get_read_ptr(const char** outData, size_t* outDataSize);
    // advance the cursor
    HttpResult commit_read(size_t data_size);
    size_t size() const;
    BufferT* get_buffer();
    const std::vector<KeyValuePair>& get_header_fields() const {
        return m_headers;
    }


private:
    BufferT          m_buffer;
    FileT            m_file;
    size_t           m_transferred_bytes;
    FileIoOpen       m_file_io_open;
    FileIoClose      m_file_io_close;
    std::vector<KeyValuePair> m_headers;

};

template<typename BufferT, typename FileT>
PayloadFile<BufferT, FileT>::PayloadFile(FileIoOpen open, FileIoClose close) :
        m_buffer(512*4),
        m_file(),
        m_transferred_bytes(0),
        m_file_io_open(open),
        m_file_io_close(close)
{
    m_file_io_open(&m_file, &m_buffer);
}

template<typename BufferT, typename FileT>
PayloadFile<BufferT, FileT>::~PayloadFile()
{

    m_file_io_close(&m_file);
}

template<typename BufferT, typename FileT>
size_t PayloadFile<BufferT, FileT>::size() const
{
    return m_file.size();
}

template<typename BufferT, typename FileT>
int PayloadFile<BufferT, FileT>::get_read_ptr(const char** data, size_t* available_data)
{
    // read from our buffer, wait a max of 1000ms
    int rc =  m_buffer.request_read((const uint8_t**)data, available_data, 0);
    return rc;
}

template<typename BufferT, typename FileT>
HttpResult PayloadFile<BufferT, FileT>::commit_read(size_t data_size)
{
    m_buffer.commit_read(data_size);
    m_transferred_bytes += data_size;
    // are we done? We are done when the file is finished and we transferred all the bytes...
    return m_file.size() == m_transferred_bytes ? HttpResult_Complete : HttpResult_Incomplete;
}


}
