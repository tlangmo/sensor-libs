#include "rbs_server.h"
#include "device_info.h"
#include "ip_address.h"
#include "wiced.h"
#include "wwd_network.h"
#include "micro_clock.h"
#include <atomic>


namespace motesque {

// we use double buffering for the table update
extern std::atomic<SyncTable*> active_sync_table;


RbsServer::RbsServer() :
   m_history(),
   m_received_packets(10),
   m_suspended(false),
   m_local_port(21001) // the standard port we use. Can be changes via set_sync_port
{
	memset(&m_udp_socket,0,sizeof(m_udp_socket));
	reset();
}

void RbsServer::reset()
{
	static SyncTable nullTable;
	// temporarily set to null table
	active_sync_table = &nullTable;
	memset(m_sync_table_buffer,0,sizeof(m_sync_table_buffer));
	// set to first buffer
	active_sync_table = &m_sync_table_buffer[0];
	m_history.clear();
}


RbsServer::~RbsServer()
{
	stop();
}

void RbsServer::suspend(bool suspended)
{
	m_suspended = suspended;
}

void RbsServer::flip_sync_tables(const SyncTable&  new_table) 
{
	// we have a table in use, figure out the unused one
	SyncTable* table_not_in_use = (active_sync_table.load() == m_sync_table_buffer ? &m_sync_table_buffer[1] : &m_sync_table_buffer[0]);
	// copy into "back" buffer
	*table_not_in_use = new_table;
	// flip the table atomically
	active_sync_table = table_not_in_use;
}


int RbsServer::send_timestamps(uint32_t dst_ip, uint16_t dst_port)
{
	uint8_t msg_data[128];
	memset(msg_data,0,sizeof(msg_data));
	int bytes_to_send = write_timestamp_packet(DeviceInfo::sensor_id(), DeviceInfo::sensor_instance_id(),
                           				        m_history, msg_data, sizeof(msg_data));
	if (bytes_to_send > 0)
	{
		//WPRINT_APP_INFO(("send_timestamps: %lu, size: %d\n", DeviceInfo::sensor_id(),bytes_to_send));	
		uint8_t* payload;
		wiced_packet_t* packet;
		const wiced_ip_address_t INITIALISER_IPV4_ADDRESS(dest_ip_addr, dst_ip);
		uint16_t available_data_length;
		wiced_result_t rc = wiced_packet_create_udp(&m_udp_socket, bytes_to_send, &packet, (uint8_t**) &payload, &available_data_length );
		if ( rc != WICED_SUCCESS )
		{
			WPRINT_APP_INFO( ("UDP tx packet creation failed\n") );
			return WICED_ERROR;
		}
		memcpy(payload, msg_data, bytes_to_send);
		/* Set the end of the data portion */
		wiced_packet_set_data_end(packet, (uint8_t*) payload + bytes_to_send );
		rc =  wiced_udp_send(&m_udp_socket, &dest_ip_addr, dst_port, packet);
		if (rc != WICED_SUCCESS )
		{
			WPRINT_APP_ERROR( ("UDP packet send failed\n") );
			wiced_packet_delete( packet ); /* Delete packet, since the send failed */
			return WICED_ERROR;
		}			
	}
	return WICED_SUCCESS;
}

int RbsServer::process()
{
	TimeStampedUdpPacket ts_pkt;
	while (m_received_packets.pop(&ts_pkt,0) == 0) 
	{
		wiced_ip_address_t udp_src_ip_addr;
    	uint16_t  udp_src_port;
		uint8_t*  rx_data;
		uint16_t  rx_data_length;
		uint16_t  available_data_length;
		wiced_packet_t* packet = (wiced_packet_t*)ts_pkt.packet;

		wiced_udp_packet_get_info( packet, &udp_src_ip_addr, &udp_src_port );
		wiced_packet_get_data( packet, 0, &rx_data, &rx_data_length, &available_data_length );
		SyncTable received_table;
		uint32_t sequence_id;

		if (read_sync_packet(rx_data, rx_data_length, &sequence_id, &received_table) == 0) {
			//WPRINT_APP_INFO(("processing sync pulse. seq_id=%lu, timestamp_us=%llu\n", sequence_id, ts_pkt.timestamp_us));
			if (active_sync_table.load()->table_id != received_table.table_id) {
				//WPRINT_APP_INFO(("switching sync tables. table_id_old=%lu, table_id_new=%lu\n", active_sync_table.load()->table_id, 
				//																				received_table.table_id));
				if (!m_suspended) {
					flip_sync_tables(received_table);
					uint64_t time_us = get_time_micros();
					uint64_t time_us_synced = 0;
					int rc_s  __attribute__((unused)) = calculate_synced_time(time_us, &time_us_synced);
					
					double m = 1.0;
					double t = 0.0;
					active_sync_table.load()->get_coeff(DeviceInfo::sensor_id(), &m, &t);
					//WPRINT_APP_INFO(("received_table. m=%f, t=%f\n",m,t));
					//WPRINT_APP_INFO(("calculate_synced_time. rc=%d, local=%llu, synced=%llu\n", rc_s, time_us, time_us_synced));
				}
			}
			// keep track of received timestampts to send back to sync server
			Timestamp ts;
			ts.timestamp_us = ts_pkt.timestamp_us;
			ts.daemon_id = received_table.daemon_id;
			ts.sequence_id = sequence_id;
			while (m_history.size() > 2) {
				m_history.pop_front();
			}
			m_history.push_back(ts);
			send_timestamps(udp_src_ip_addr.ip.v4, udp_src_port);
		}
		// important! Delete the packet to give back to network stack
		wiced_packet_delete(packet);
	}

	return 0;
}


wiced_result_t RbsServer::on_receive_packet( wiced_udp_socket_t* socket, void* arg )
{
	RbsServer* srv = (RbsServer*)arg;
	wiced_packet_t*  packet;
	if( wiced_udp_receive(socket, &packet, 0) == WICED_SUCCESS)
    {
		
		//ts_pkt.timestamp_us = get_time_micros();
		
		TimeStampedUdpPacket ts_pkt;
		if (get_packet_timestamp((NX_PACKET*)packet, &ts_pkt.timestamp_us) != 0) {
			//ts_pkt.timestamp_us = get_time_micros();
			ts_pkt.timestamp_us = 0;
		}
		// else {
		// 	uint64_t delta = get_time_micros() - ts_pkt.timestamp_us;
		// 	WPRINT_APP_ERROR(("receiving ts delta  %llu\n",delta));
		// }

		ts_pkt.packet = packet;
		if (srv->m_received_packets.put(ts_pkt,0) != 0) {
			WPRINT_APP_ERROR(("unable to queue pulse message\n"));
			wiced_packet_delete(packet);
		}
    }
	return WICED_SUCCESS;
}

int RbsServer::start()
{
	if (m_udp_socket.socket_magic_number != 0) {
		return WICED_ALREADY_CONNECTED;
	}
	wiced_result_t rc = wiced_udp_create_socket(&m_udp_socket, m_local_port, WICED_STA_INTERFACE);
	if (rc == WICED_SUCCESS) {
		rc = wiced_udp_register_callbacks(&m_udp_socket, on_receive_packet, this);
	}
	return rc;
}

int RbsServer::stop()
{
	// delete any packets which are in our queue
	TimeStampedUdpPacket ts_pkt;
	while (m_received_packets.pop(&ts_pkt,0) == 0) 
	{
		wiced_packet_t* packet = (wiced_packet_t*)ts_pkt.packet;
		wiced_packet_delete(packet);
	}
	wiced_result_t rc = wiced_udp_delete_socket(&m_udp_socket);
	memset(&m_udp_socket,0,sizeof(m_udp_socket));
	return rc;
}

}; // end ns