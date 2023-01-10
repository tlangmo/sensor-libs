#pragma once
#include <algorithm>
#include <cassert>
#include <vector>
#include <cmath>
#include <iostream>

namespace motesque {

template<typename PacketT1, typename PacketT2, typename PacketT3, typename PacketT4, typename PacketT5>
class PacketSyncClosest
{
public:
    int push(const PacketT1& packet) {
        // check for monotonic increasing timestamp
        if (0 != check_monotonic_timestamp(packet.timestamp_us, m_packets_t1)) {
            return -1;
        }
        m_packets_t1.emplace_back(packet);
        return 0;
    }
    int push(const PacketT2& packet) {
         // check for monotonic increasing timestamp
         if ( 0 != check_monotonic_timestamp(packet.timestamp_us, m_packets_t2)) {
            return -1;
        }
         m_packets_t2.emplace_back(packet);
         return 0;
    }
    int push(const PacketT3& packet) {
         // check for monotonic increasing timestamp
         if ( 0 != check_monotonic_timestamp(packet.timestamp_us, m_packets_t3)) {
            return -1;
        }
         m_packets_t3.emplace_back(packet);
         return 0;
    }
    int push(const PacketT4& packet) {
         // check for monotonic increasing timestamp
         if ( 0 != check_monotonic_timestamp(packet.timestamp_us, m_packets_t4)) {
            return -1;
        }
         m_packets_t4.emplace_back(packet);
         return 0;
    }
    int push(const PacketT5& packet) {
         // check for monotonic increasing timestamp
         if ( 0 != check_monotonic_timestamp(packet.timestamp_us, m_packets_t5)) {
            return -1;
        }
         m_packets_t5.emplace_back(packet);
         return 0;
    }
    void clear() {
        m_packets_t1.clear();
        m_packets_t2.clear();
        m_packets_t3.clear();
        m_packets_t4.clear();
        m_packets_t5.clear();
    }
// && !m_packets_t2.empty() && !m_packets_t3.empty()  && !m_packets_t4.empty()  && !m_packets_t5.empty())
    int pop(PacketT1* packet_t1, PacketT2* packet_t2, PacketT3* packet_t3, PacketT4* packet_t4, PacketT5* packet_t5)
    {
        if (m_packets_t1.empty()){
            return -1;
        }

        *packet_t1 = *(m_packets_t1.begin());
        m_packets_t1.erase(m_packets_t1.begin());

        if (!m_packets_t2.empty()) {
            auto it_t2 = find_closest_packet(packet_t1->timestamp_us, m_packets_t2);
            *packet_t2 = *it_t2;
            // remove all packets prior
            if (it_t2 > m_packets_t2.begin()) {
                m_packets_t2.erase(m_packets_t2.begin(), it_t2);
            }
        }

        if (!m_packets_t3.empty()) {
            auto it_t3 = find_closest_packet(packet_t1->timestamp_us, m_packets_t3);
            *packet_t3 = *it_t3;
            if (it_t3 > m_packets_t3.begin()) {
                m_packets_t3.erase(m_packets_t3.begin(), it_t3);
            }
        }

        if (!m_packets_t4.empty()) {
            auto it_t4 = find_closest_packet(packet_t1->timestamp_us, m_packets_t4);
            *packet_t4 = *it_t4;
            if (it_t4 > m_packets_t4.begin()) {
                m_packets_t4.erase(m_packets_t4.begin(), it_t4);
            }
        }

        if (!m_packets_t5.empty()) {
            auto it_t5 = find_closest_packet(packet_t1->timestamp_us, m_packets_t5);
            *packet_t5 = *it_t5;
            if (it_t5 > m_packets_t5.begin()) {
                m_packets_t5.erase(m_packets_t5.begin(), it_t5);
            }
        }
        
        return 0;
        
    }
public:
    template<typename T>
    int check_monotonic_timestamp(uint64_t ts, const T& packets) {
        if (!packets.empty() && packets.back().timestamp_us > ts) {
            return -1;
        }
        return 0;
    }

    template<typename T>
    typename std::vector<T>::const_iterator find_closest_packet(uint64_t ts, const std::vector<T>& packets ) const
    {
        typename std::vector<T>::const_iterator it_min = packets.end();

        auto lower_it = std::lower_bound(packets.begin(), packets.end(), ts,
            [](const T& pkt, uint64_t ts) { return pkt.timestamp_us < ts; });

        if (std::labs(std::prev(lower_it)->timestamp_us - ts) < (std::labs(
            lower_it->timestamp_us - ts))) {
            it_min = std::prev(lower_it);
        }
        else {
            it_min = lower_it;
        }

        assert(it_min != packets.end());
        return it_min;
    }
private:
    std::vector<PacketT1> m_packets_t1;
    std::vector<PacketT2> m_packets_t2;
    std::vector<PacketT3> m_packets_t3;
    std::vector<PacketT4> m_packets_t4;
    std::vector<PacketT5> m_packets_t5;
};

};// end ns
