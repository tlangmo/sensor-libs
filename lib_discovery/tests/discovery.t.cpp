// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#include "../../unittest/catch.hpp"
#include "discovery.h"
#include <iostream>

using namespace motesque;

class UdpTransportMock
{
public:
	UdpTransportMock()
    {

    }
	~UdpTransportMock() 
    {

    }
    int create_socket(uint16_t port) {return 0;}
	int close_socket()  {return 0;}
	int send_datagram(const uint8_t* buf, uint16_t buf_size, const uint32_t& dest_ip, uint16_t dest_port)
    {
      //  std::cout << buf << std::endl;
        const char* expected = "{\"instance_id\": 756027456, \"ip\": \"255.0.0.0\", \"product_version\": \"1999.1.1\", \"product_name\": \"motesque-mpu-gen3\", \"battery_soc\": 77.199997, \"battery_charging\": 1, \"owner_mac_addr\": \"aa:bb:cc:dd:ee:ff\", \"device_id\": \"00aabbccddeeff\", \"config_hash\": 11234567000000000001}";
        // std::cout << expected << std::endl;
        REQUIRE(strcmp((const char*)buf, expected) == 0);
        return 0;
    }
};

typedef DiscoveryT<UdpTransportMock> Discovery;

void fill_attributes(Discovery::AttributeMap* data)
{
    Discovery::Attribute a;
     a.u32 = 756027456;
    (*data)["instance_id"] = a;
    a.u32 = 0xff000000;
    (*data)["ip"] = a;
    snprintf(a.s,sizeof(a.s),"1999.1.1");
    (*data)["product_version"] = a;
    snprintf(a.s,sizeof(a.s),"motesque-mpu-gen3");
    (*data)["product_name"] = a;
    a.f = 77.2;
    (*data)["battery_soc"] = a;
     a.u32 = 1;
    (*data)["battery_charging"] = a;
     snprintf(a.s,sizeof(a.s),"aa:bb:cc:dd:ee:ff");
    (*data)["owner_mac_addr"] = a;
     snprintf(a.s,sizeof(a.s),"00aabbccddeeff");
    (*data)["device_id"] = a;
    a.u64 = 11234567000000000001ULL;
    (*data)["config_hash"] = a;

}

TEST_CASE( "discovery ctor") 
{
    Discovery discovery(nullptr);
    REQUIRE(discovery.start(0,0) == -1);
}

TEST_CASE( "attributes get filled by callback") 
{
    Discovery discovery(fill_attributes);
    discovery.send_discovery_message();
}
