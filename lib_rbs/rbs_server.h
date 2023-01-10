#pragma once

#include "rbs_message.h"
#include "rbs_sync_table.h"
#include "wiced_network.h"
#include "rtos_queue.h"

namespace motesque 
{


/*
 * Main communication server to handle reference broadcast sync packets.
 */
class RbsServer
{
public:
	RbsServer();
	virtual ~RbsServer();
    // open the udp socket
	int start();
    // close the udp socket
	int stop();
    // call in regular intervals to decode packets and send replies to sync daemon
	int process();
    // reset the sync table
    void reset();
    // temporarily suspend table update. This does not stop the processing and replies for the sync pulses
	void suspend(bool suspended);
    
    uint16_t get_sync_port() const { return m_local_port;}
    void set_sync_port(uint16_t local_port) { m_local_port =  local_port;}
 private:   
    // callback for network stack to notify us of recieved udp packets on this socket
    static wiced_result_t on_receive_packet( wiced_udp_socket_t* socket, void* arg );

private:
    void flip_sync_tables(const SyncTable&  new_table);
    int send_timestamps(uint32_t dst_ip, uint16_t dst_port);
private:
    TimestampHistory m_history;
    RtosQueue<TimeStampedUdpPacket> m_received_packets;
    volatile bool m_suspended;
    wiced_udp_socket_t m_udp_socket;
    SyncTable m_sync_table_buffer[2];
    uint16_t m_local_port;
   
};

}; // end ns

