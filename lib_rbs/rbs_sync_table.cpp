#include "rbs_sync_table.h"
#include <math.h>
#include <atomic>
#include "device_info.h"


namespace motesque {

// Atomic pointer to the active sync table. 
// A sync table update is a compartivlely slow operation. To avoid race conditions, we use 
// double buffering with the buffer pointed at by `active_sync_table` always being valid
std::atomic<SyncTable*> active_sync_table;

int calculate_synced_time(uint64_t time_us_local, uint64_t* time_us_synced)
{
	double m = 1.0;
	double t = 0.0;
	
	if (active_sync_table.load()->get_coeff(DeviceInfo::sensor_id(), &m, &t) == 0) {
		*time_us_synced = (uint64_t)ceil(m*(double)time_us_local + t);
		return 0;
	}

	return -1;
}

}