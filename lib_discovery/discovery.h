#pragma once
#include "stdint.h"
#include <map>
#include <string>
#include <functional>
#include "json_frozen.h"
#include "ip_address.h"
namespace motesque {

/** Device discovery service. Sends out periodic beacons with current device information
*/

template<typename TRANSPORT_T>
class DiscoveryT
{
public:
    union Attribute
    {
        uint16_t u16;
        uint32_t u32;
        int32_t  i32;
        float f;
        uint64_t u64;
        char s[32];
    };   
    typedef std::map<std::string, Attribute> AttributeMap;
    typedef std::function<void (AttributeMap* data)> AttributeProviderCb;
public:
    DiscoveryT(const AttributeProviderCb& cb);
    virtual ~DiscoveryT();
    int start(uint32_t destination_ip, uint16_t destination_port);
    int stop();
    int send_discovery_message();

private:
    uint32_t m_destination_ip;
    uint16_t m_destination_port;
    AttributeProviderCb m_attributeProviderCb;
    AttributeMap m_attributeMap;
    TRANSPORT_T   m_transport;
};

template<typename TRANSPORT_T>
DiscoveryT<TRANSPORT_T>::DiscoveryT(const AttributeProviderCb& cb):
    m_destination_ip(0),
    m_destination_port(0),
    m_attributeProviderCb(cb),
    m_attributeMap(),
    m_transport()

{

}
template<typename TRANSPORT_T>
DiscoveryT<TRANSPORT_T>::~DiscoveryT()
{

}
template<typename TRANSPORT_T>
int DiscoveryT<TRANSPORT_T>::start(uint32_t destination_ip, uint16_t destination_port)
{
    if (!m_attributeProviderCb) {
        return -1;
    }
    m_destination_port = destination_port;
    m_destination_ip = destination_ip;
    return m_transport.create_socket(8999);
}   

template<typename TRANSPORT_T>
int DiscoveryT<TRANSPORT_T>::stop()
{
    return m_transport.close_socket();
}

template<typename TRANSPORT_T>
int DiscoveryT<TRANSPORT_T>::send_discovery_message()
{
    m_attributeProviderCb(&m_attributeMap);
    char ip4_buf[32];
    memset(ip4_buf, 0, sizeof(ip4_buf));
    motesque::ip4_to_string(m_attributeMap["ip"].u32, ip4_buf, sizeof(ip4_buf));

    char* msg = json_asprintf("{"
                                 "instance_id: %lu, "
                                 "ip: %Q, "
                                 "product_version: %Q, "
                                 "product_name: %Q, "
                                 "battery_soc: %f, "
                                 "battery_charging: %d, "
                                 "owner_mac_addr: %Q, "
                                 "device_id: %Q, "
                                 "config_hash: %llu"
                                 "}",
                                 m_attributeMap["instance_id"].u32, 
                                 ip4_buf,
                                 m_attributeMap["product_version"].s,
                                 m_attributeMap["product_name"].s,
                                 m_attributeMap["battery_soc"].f,
                                 m_attributeMap["battery_charging"].u16,
                                 m_attributeMap["owner_mac_addr"].s,
                                 m_attributeMap["device_id"].s,   
                                 m_attributeMap["config_hash"].u64  
                                 );

    int rc = m_transport.send_datagram((const uint8_t*)msg, strlen(msg), m_destination_ip, m_destination_port);
    free(msg);
    return rc;
}
  
}