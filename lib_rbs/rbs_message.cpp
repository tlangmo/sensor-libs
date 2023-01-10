#include <cmath>
#include <cstdlib>
#include <cstring>

#include "rbs_message.h"



namespace motesque {


int write_timestamp_packet(uint32_t sensor_id, uint32_t sensor_instance_id,
                           const TimestampHistory& hist,
                           uint8_t* buffer, size_t buffer_length)
{
	const uint8_t* cur = buffer;
	*(uint32_t*)cur = sensor_id;
	cur+= sizeof(uint32_t);
	*(uint32_t*)cur = sensor_instance_id;
	cur+= sizeof(uint32_t);
	for (size_t i=0; i < hist.size(); i++) {
		*(uint32_t*)cur = hist[i].sequence_id;
		cur+= sizeof(uint32_t);
		*(uint64_t*)cur = hist[i].timestamp_us;
		cur+= sizeof(unsigned long long);
		*(uint32_t*)cur = hist[i].daemon_id;
		cur+= sizeof(uint32_t);
	}
    return cur-buffer;
}


int read_sync_packet(const uint8_t* buffer, size_t buffer_length, uint32_t* sequence_id, SyncTable* sync_table)
{
    memset(sync_table, 0, sizeof(SyncTable));
    if (buffer_length < 4) {
        // invalid packet
        return -1;
    }
    const uint8_t* cur = buffer;
    const uint8_t* bufferEnd = buffer+buffer_length;
    *sequence_id = *(uint32_t*)cur;
    cur += sizeof(uint32_t);
    sync_table->table_id = *(uint32_t*)cur;
    cur += sizeof(uint32_t);
    sync_table->daemon_id = *(uint32_t*)cur;
    cur += sizeof(uint32_t);
    size_t rowIdx = 0;
    while (cur < bufferEnd) {
        sync_table->rows[rowIdx].sensor_id = *(uint32_t*)cur;
        cur += sizeof(uint32_t);
		sync_table->rows[rowIdx].sensor_instance_id = *(uint32_t*)cur;
        cur += sizeof(uint32_t);
        sync_table->rows[rowIdx].m = *(double*)cur;
        cur += sizeof(double);
        sync_table->rows[rowIdx].t = *(double*)cur;
        cur += sizeof(double);
        rowIdx++;
    }
    return 0;
}



}; // end ns
