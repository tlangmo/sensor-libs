// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#include "../../unittest/catch.hpp"
#include "generic_sensor.h"
#include "../sync_packets.h"
#include <vector>
using namespace motesque;

struct FakeDevice
{
    int id;
};
struct DevicePolicyMock {

    int initialize()  {
        dev.id = 88;
        return 0;
    }

    FakeDevice* get() {
        return &dev;
    }
    FakeDevice dev;
};

struct PowerSavePolicyMock {
    int initialize(FakeDevice* dev)  {
        return 0;
    }
};
struct DataPipelinePolicyMock {
    struct Packet {

    };
    typedef std::vector<Packet> PacketList;
    int initialize(FakeDevice* dev)  {
       return 0;
   }
};
struct SettingsPolicyMock {
    int initialize(FakeDevice* dev)  {
       return 0;
   }
};

struct SettingsPolicyNoopMock {
    int initialize(FakeDevice* dev)  {
          return 0;
    }
    int seventy_seven() {
        return 77;
    }
};


TEST_CASE( "is_valid_enum_value") {
    REQUIRE(false == is_valid_enum_value<AccScaleRange>(0));
    REQUIRE(false == is_valid_enum_value<AccScaleRange>(-1));
    REQUIRE(true == is_valid_enum_value<AccScaleRange>(16));

    REQUIRE(false == is_valid_enum_value<GyrScaleRange>(0));
    REQUIRE(true == is_valid_enum_value<GyrScaleRange>(2000));
    REQUIRE(false == is_valid_enum_value<GyrScaleRange>(-1));

    REQUIRE(true == is_valid_enum_value<SampleRate>(100));
    REQUIRE(false == is_valid_enum_value<SampleRate>(-1));
    REQUIRE(true == is_valid_enum_value<SampleRate>(1000));
    REQUIRE(false == is_valid_enum_value<SampleRate>(1001));

}

struct Packet1 {
    Packet1():timestamp_us(0) {}
    Packet1(const Packet1& rhs) {
        timestamp_us = rhs.timestamp_us;
    }
    Packet1(uint64_t ts): timestamp_us(ts) {}
    uint64_t timestamp_us;
};
struct Packet2 {
    Packet2():timestamp_us(0) {}
    Packet2(const Packet2& rhs) {
           timestamp_us = rhs.timestamp_us;
       }
    Packet2(uint64_t ts): timestamp_us(ts) {}
    uint64_t timestamp_us;
};
struct Packet3 {
    Packet3():timestamp_us(0) {}
    Packet3(const Packet3& rhs) {
           timestamp_us = rhs.timestamp_us;
       }
    Packet3(uint64_t ts): timestamp_us(ts) {}
    uint64_t timestamp_us;
};
struct Packet4 {
    Packet4():timestamp_us(0) {}
    Packet4(const Packet4& rhs) {
           timestamp_us = rhs.timestamp_us;
       }
    Packet4(uint64_t ts): timestamp_us(ts) {}
    uint64_t timestamp_us;
};
struct Packet5 {
    Packet5():timestamp_us(0) {}
    Packet5(const Packet5& rhs) {
           timestamp_us = rhs.timestamp_us;
       }
    Packet5(uint64_t ts): timestamp_us(ts) {}
    uint64_t timestamp_us;
};

TEST_CASE( "PacketSyncClosest not enough packages")
{
    PacketSyncClosest<Packet1, Packet2, Packet3, Packet4, Packet5> sync;
    Packet1 p1_synced;
    Packet2 p2_synced;
    Packet3 p3_synced;
    Packet4 p4_synced;
    Packet5 p5_synced;
    // not all 'streams' have a packet yet
    REQUIRE( -1 == sync.pop(&p1_synced, &p2_synced, &p3_synced, &p4_synced, &p5_synced));
}

TEST_CASE( "PacketSyncClosest simple")
{
    PacketSyncClosest<Packet1, Packet2, Packet3, Packet4, Packet5> sync;
    Packet1 p1_synced;
    Packet2 p2_synced;
    Packet3 p3_synced;
    Packet4 p4_synced;
    Packet5 p5_synced;
    sync.push(Packet1(1000));
    sync.push(Packet2(1010));
    sync.push(Packet3(1020));
    sync.push(Packet4(1030));
    sync.push(Packet5(1040));
    REQUIRE( 0 == sync.pop(&p1_synced, &p2_synced, &p3_synced, &p4_synced, &p5_synced));
    REQUIRE( 1000 == p1_synced.timestamp_us);
    REQUIRE( 1010 == p2_synced.timestamp_us);
    REQUIRE( 1020 == p3_synced.timestamp_us);
    REQUIRE( 1030 == p4_synced.timestamp_us);
    REQUIRE( 1040 == p5_synced.timestamp_us);
    // no more packages
    REQUIRE( -1 == sync.pop(&p1_synced, &p2_synced, &p3_synced, &p4_synced, &p5_synced));

    sync.clear();
    sync.push(Packet1(1020));
    sync.push(Packet2(1010));
    sync.push(Packet3(1000));
    sync.push(Packet4(1005));
    sync.push(Packet5(1015));
    REQUIRE( 0 == sync.pop(&p1_synced, &p2_synced, &p3_synced, &p4_synced, &p5_synced));
    REQUIRE( 1020 == p1_synced.timestamp_us);
    REQUIRE( 1010 == p2_synced.timestamp_us);
    REQUIRE( 1000 == p3_synced.timestamp_us);
    REQUIRE( 1005 == p4_synced.timestamp_us);
    REQUIRE( 1015 == p5_synced.timestamp_us);
   // no more packages
    REQUIRE( -1 == sync.pop(&p1_synced, &p2_synced, &p3_synced, &p4_synced, &p5_synced));

}

TEST_CASE( "PacketSyncClosest assert")
{
    PacketSyncClosest<Packet1, Packet2, Packet3, Packet4, Packet5> sync;
    REQUIRE( 0 == sync.push(Packet1(1020)));
    REQUIRE( 0 == sync.push(Packet1(1020)));
    REQUIRE( -1 == sync.push(Packet1(1010)));
}

TEST_CASE( "PacketSyncClosest closest")
{
    PacketSyncClosest<Packet1,Packet2, Packet3, Packet4, Packet5> sync;
    Packet1 p1_synced;
    Packet2 p2_synced;
    Packet3 p3_synced;
    Packet4 p4_synced;
    Packet5 p5_synced;
    sync.push(Packet1(1020));
    sync.push(Packet1(1021));
    sync.push(Packet1(1022));

    sync.push(Packet2(1023));
    sync.push(Packet2(1025));

    sync.push(Packet3(1021));
    sync.push(Packet3(1024));

    sync.push(Packet4(1020));
    sync.push(Packet4(1023));

    sync.push(Packet5(1020));
    sync.push(Packet5(1023));

    REQUIRE( 0 == sync.pop(&p1_synced, &p2_synced, &p3_synced, &p4_synced, &p5_synced));
    REQUIRE( 1020 == p1_synced.timestamp_us);
    REQUIRE( 1023 == p2_synced.timestamp_us);
    REQUIRE( 1021 == p3_synced.timestamp_us);
    REQUIRE( 1020 == p4_synced.timestamp_us);
    REQUIRE( 1020 == p5_synced.timestamp_us);

    REQUIRE( 0 == sync.pop(&p1_synced, &p2_synced, &p3_synced, &p4_synced, &p5_synced));
    REQUIRE( 1021 == p1_synced.timestamp_us);
    REQUIRE( 1023 == p2_synced.timestamp_us);
    REQUIRE( 1021 == p3_synced.timestamp_us);
    REQUIRE( 1020 == p4_synced.timestamp_us);
    REQUIRE( 1020 == p5_synced.timestamp_us);

    REQUIRE( 0 == sync.pop(&p1_synced, &p2_synced, &p3_synced, &p4_synced, &p5_synced));
    REQUIRE( 1022 == p1_synced.timestamp_us);
    REQUIRE( 1023 == p2_synced.timestamp_us);
    REQUIRE( 1021 == p3_synced.timestamp_us);
    REQUIRE( 1023 == p4_synced.timestamp_us);
    REQUIRE( 1023 == p5_synced.timestamp_us);

    REQUIRE( -1 == sync.pop(&p1_synced, &p2_synced, &p3_synced, &p4_synced, &p5_synced));

    sync.push(Packet1(1025));
    sync.push(Packet2(1023));
    sync.push(Packet2(1025));
    sync.push(Packet3(1005));
    sync.push(Packet3(1025));
    sync.push(Packet3(1025));
    sync.push(Packet4(1022));
    sync.push(Packet4(1025));
    sync.push(Packet5(1025));
    REQUIRE( 0 == sync.pop(&p1_synced, &p2_synced, &p3_synced, &p4_synced, &p5_synced));
    REQUIRE( 1025 == p1_synced.timestamp_us);
    REQUIRE( 1025 == p2_synced.timestamp_us);
    REQUIRE( 1025 == p3_synced.timestamp_us);
    REQUIRE( 1025 == p4_synced.timestamp_us);
    REQUIRE( 1025 == p5_synced.timestamp_us);
}
