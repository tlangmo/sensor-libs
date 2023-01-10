#pragma once

#include <deque>
#include <array>
#include <cstring>
namespace motesque {

/*
 * A Record of the timing info of a received reference broadcast. The deamon_id differentiates between several
 * broadcast sources, which we cannot rule out (e.g. multiple devices in one network) 
 */
struct Timestamp {
	Timestamp() : 
		sequence_id(0), 
		timestamp_us(0), 
		daemon_id(0)  
	{}

	uint32_t 	sequence_id;	// the identifier of the broadcasted pulse
	uint64_t	timestamp_us;	// local time at packet reception in micro seconds
	uint32_t 	daemon_id;		// identifies broadcast source
};

// Every reply message contains several past timestamps to accound for missed packets
typedef std::deque<Timestamp> TimestampHistory;


/*
 *	The sync table contains the linear mapping functions for all senors of the same synchronization set.
 *	It is created by a sync daemon process which emits the reference pulses. These references pulses
 *	a simple UDP packets which contain as payload the current table
 */
struct SyncTable {

	struct SyncTableEntry {
		uint32_t sensor_id;
        uint32_t sensor_instance_id;
		double m;				// t_synced = m*t_local+t
		double t;
	};

	SyncTable() : table_id(0), daemon_id(0)
	{
		memset(rows,0,sizeof(rows));
	}	

	int get_coeff(uint32_t sensor_id, double* m, double* t) const
	{
		for (size_t i = 0; i < 32; i++) {
			if (rows[i].sensor_id == sensor_id) {
				*m = rows[i].m;
				*t = rows[i].t;
				return 0;
			}
		}
		return -1;
	}
    bool has_entry(uint32_t sensor_id, uint32_t sensor_instance_id) {
        for (size_t i = 0; i < 32; i++) {
            if (rows[i].sensor_id == sensor_id && rows[i].sensor_instance_id == sensor_instance_id) {
                return true;
            }
        }
        return false;
    }
	uint32_t table_id;			// the identifer that sync table. Whenever a change occured, this id changes
	uint32_t daemon_id;			// the identifer of sync daemon which created that table
	SyncTableEntry rows[32];	// support max of 32 sensors
};

struct TimeStampedUdpPacket {
	void*     packet;
	uint64_t  timestamp_us;
};
/*
 * Convert the time stamp local->synced via linear mapping function for given sensor
 */
int calculate_synced_time(uint64_t time_us_local, uint64_t* time_us_synced);


}; // end ns
