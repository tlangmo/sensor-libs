#pragma once

#include <deque>
#include <array>
#include "rbs_sync_table.h"
namespace motesque {


/*
 * Fill byte buffer with timestamp information for UDP broadcast
 *    0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9
     +-------------------------------------------------------------------------------+
     |sensorId (32)                                                                  |
     |sensorInstanceId (32)     												     |
     |sequenceId (32)    													         |
     |timeUs (64)    												    	         |
     |timeUs continued                                                               |
     |daemonId (32)  												    	         |
     |sequenceId (32)    													         |
     |timeUs (64)    												    	         |
     |timeUs continued    												    	     |
     |daemonId (32)  												    	         |
     |sequenceId (32)    													         |
     |timeUs (64)    												    	         |
     |timeUs continued    												    	     |
     |daemonId (32)  												    	         |
     +-------------------------------------------------------------------------------+
 */
int write_timestamp_packet(uint32_t sensor_id, uint32_t sensor_instance_id,
                           const TimestampHistory& hist,
                           uint8_t* buffer, size_t buffer_length);

/*
     Parse a sync packet from sync daemon
	  0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9
     +-------------------------------------------------------------------------------+
     |sequenceID (32)                                                                |
     |tableId (32)																	 |
     |daemonId (32)                                                                  |
     |sensorId (32)    																 |
     |sensorInstanceId (32)    														 |
     |m	(64)																     	 |
     |m	continued 																	 |
     |t	(64)																		 |
     |t	continued 																	 |
     |sensorId (32) 																 |
     |sensorInstanceId (32)    														 |
     |m	(64)																		 |
     |m	continued 																	 |
     |t	(64)																		 |
     |t	continued 																	 |
     +-------------------------------------------------------------------------------+
     | SyncTableEntry rows continued ... 										     |
     +-------------------------------------------------------------------------------+
 */
int read_sync_packet(const uint8_t* buffer, size_t buffer_length, uint32_t* sequence_id, SyncTable* sync_table);

//int device_id_string_to_sensor_id(const std::string& device_id);


}; // end ns
