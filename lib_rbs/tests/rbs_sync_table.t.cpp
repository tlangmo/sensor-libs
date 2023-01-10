// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#include "../../unittest/catch.hpp"
#include "rbs_sync_table.h"
#include "device_info.h"
#include <atomic>
using namespace motesque;
namespace motesque {

extern std::atomic<SyncTable*> active_sync_table;

};

TEST_CASE( "synctable ctor")  {
   SyncTable tbl;
}

TEST_CASE( "calculate_synced_time return -1 if no table exists")  {
   SyncTable tbl;
   DeviceInfo::init(0xaabbccdd, 1000);

   active_sync_table = &tbl;
   uint64_t time_us_local=0;
   uint64_t time_us_synced = 0;
   REQUIRE(calculate_synced_time(time_us_local, &time_us_synced) != 0);

}

TEST_CASE( "calculate_synced_time transforms time to synced time")  {
   SyncTable tbl;
   DeviceInfo::init(0xaabbccdd, 1000);
   tbl.rows[0].sensor_id = 0xaabbccdd;
   tbl.rows[0].sensor_instance_id = 1000;
   tbl.rows[0].m = 1.0;
   tbl.rows[0].t = 77;
   active_sync_table = &tbl;
   uint64_t time_us_local= 10;
   uint64_t time_us_synced = 0;
   REQUIRE(calculate_synced_time(time_us_local, &time_us_synced) == 0);
   REQUIRE(time_us_synced == 87);
}