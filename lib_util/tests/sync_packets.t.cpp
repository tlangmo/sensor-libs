#include "assert.h"
#include "../../unittest/catch.hpp"
#include "../sync_packets.h"

#include <iostream>


struct Packet {
    Packet() :
       timestamp_us(0) {}
    Packet(uint64_t timestamp_us) :
       timestamp_us(timestamp_us) {}
    bool operator <(const uint64_t& rhs) const { return this->timestamp_us < rhs;}
    bool operator >(const uint64_t& rhs) const { return this->timestamp_us > rhs;}
    bool operator ==(const uint64_t& rhs) const { return this->timestamp_us == rhs;}
    uint64_t timestamp_us;
};

struct Packet1: Packet {};
struct Packet2: Packet {};
struct Packet3: Packet {};
struct Packet4: Packet {};
struct Packet5: Packet {};
/*
struct Packeta: Packet {
    Packeta() :
       timestamp_us(0) {}
    Packeta(uint64_t timestamp_us) :
       timestamp_us(timestamp_us) {}
    bool operator <(const uint64_t& rhs) const { return this->timestamp_us < rhs;}
    bool operator >(const uint64_t& rhs) const { return this->timestamp_us > rhs;}
    bool operator ==(const uint64_t& rhs) const { return this->timestamp_us == rhs;}

    uint64_t timestamp_us;
};
*/

/*
bool operator <(const uint64_t& rhs, const Packet& lhs) { return lhs.timestamp_us < rhs;}
bool operator >(const uint64_t& rhs, const Packet& lhs) { return lhs.timestamp_us > rhs;}


struct Packetb {
   Packetb() :
       timestamp_us(0) {}

   Packetb(uint64_t timestamp_us) :
       timestamp_us(timestamp_us) {}
    bool operator <(const uint64_t& rhs) const { return this->timestamp_us < rhs;}
    bool operator >(const uint64_t& rhs) const { return this->timestamp_us > rhs;}
    bool operator ==(const uint64_t& rhs) const { return this->timestamp_us == rhs;}
   uint64_t timestamp_us;
};
struct Packetc {
   Packetc() :
       timestamp_us(0) {}

   Packetc(uint64_t timestamp_us) :
       timestamp_us(timestamp_us) {}
    bool operator <(const uint64_t& rhs) const { return this->timestamp_us < rhs;}
    bool operator >(const uint64_t& rhs) const { return this->timestamp_us > rhs;}
    bool operator ==(const uint64_t& rhs) const { return this->timestamp_us == rhs;}
   uint64_t timestamp_us;
};
struct Packetd {
   Packetd() :
       timestamp_us(0) {}

   Packetd(uint64_t timestamp_us) :
       timestamp_us(timestamp_us) {}
    bool operator <(const uint64_t& rhs) const { return this->timestamp_us < rhs;}
    bool operator >(const uint64_t& rhs) const { return this->timestamp_us > rhs;}
    bool operator ==(const uint64_t& rhs) const { return this->timestamp_us == rhs;}
   uint64_t timestamp_us;
};
struct Packete {
   Packete() :
       timestamp_us(0) {}

   Packete(uint64_t timestamp_us) :
       timestamp_us(timestamp_us) {}
    bool operator <(const uint64_t& rhs) const { return this->timestamp_us < rhs;}
    bool operator >(const uint64_t& rhs) const { return this->timestamp_us > rhs;}
    bool operator ==(const uint64_t& rhs) const { return this->timestamp_us == rhs;}
   uint64_t timestamp_us;
};


//typedef Packet Packet1;
//typedef Packetb Packet2;
typedef Packetc Packet3;
typedef Packetd Packet4;
typedef Packete Packet5;
*/
TEST_CASE( "ctor" ) {
    motesque::PacketSyncClosest<Packet1, Packet2, Packet3, Packet4, Packet5> packets;
}


TEST_CASE( "find_closest" ) {

    std::list<Packet> packets;
    std::array<uint64_t, 6> arr {3, 5, 24, 31, 32, 56};
    for (auto i : arr) {
        Packet tmp(i);
        packets.emplace_back(tmp);
    }

    motesque::PacketSyncClosest<Packet1, Packet2, Packet3, Packet4, Packet5> packet_stream;
    uint64_t ref_ts = 30;
    auto res = packet_stream.find_closest_packet(ref_ts, packets);
    REQUIRE( res->timestamp_us == 31 );

    ref_ts = 25;
    res = packet_stream.find_closest_packet(ref_ts, packets);
    REQUIRE( res->timestamp_us == 24 );

    ref_ts = 32;
    res = packet_stream.find_closest_packet(ref_ts, packets);
    REQUIRE( res->timestamp_us == 32 );

    ref_ts = 89;
    res = packet_stream.find_closest_packet(ref_ts, packets);
    REQUIRE( res->timestamp_us == 56 );

    ref_ts = 0;
    res = packet_stream.find_closest_packet(ref_ts, packets);
    REQUIRE( res->timestamp_us == 3 );

}
