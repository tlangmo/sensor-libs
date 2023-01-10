// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#include "../../unittest/catch.hpp"
#include "message_frame.h"


TEST_CASE( "all_feature_ids") {
    using namespace motesque;
    auto it = all_feature_ids();
}

TEST_CASE( "FrameMessageOffset") {
    using namespace motesque;
    FrameMessageBuilder builder;
    REQUIRE(FrameMessageOffset<SensorMetaData>::value  ==  sizeof(FrameMessageHeader));
}

TEST_CASE( "FrameMessageBuilder_add") {
    using namespace motesque;
    FrameMessageBuilder builder;
    LowNoiseImuData data;
    builder.add(data);
    REQUIRE(builder.get_size() == sizeof(FrameMessageHeader) + sizeof(LowNoiseImuData));
    builder.add(SensorMetaData());
    REQUIRE(builder.get_size() ==  sizeof(FrameMessageHeader) + sizeof(LowNoiseImuData) + sizeof(SensorMetaData)); 
}


TEST_CASE( "FrameMessageBuilder_finish compacts buffer with one gap") {
    using namespace motesque;
    FrameMessageBuilder builder;
    LowNoiseImuData data;
    data.acc_g[0] = 1.0f;
    data.acc_g[1] = 2.0f;
    data.acc_g[2] = 3.0f;

    REQUIRE( builder.add(data) == 0);
    
    const uint8_t* buffer = builder.get_buffer_pointer() + sizeof(FrameMessageHeader);
    REQUIRE( builder.finish() == 0);
    REQUIRE( ((LowNoiseImuData*)buffer)->acc_g[0] == 1.0f);
    REQUIRE( ((LowNoiseImuData*)buffer)->acc_g[1] == 2.0f);

    // cannot add more data after finish
    REQUIRE( builder.add(data) < 0);
}

TEST_CASE( "FrameMessageBuilder_finish compacts buffer with multiple gaps") {
    using namespace motesque;
    FrameMessageBuilder builder;
    HighRangeImuData data;
    data.acc_g[0]  =  1.0f;
    data.acc_g[1]  =  2.0f;
    data.acc_g[2]  =  3.0f;
    REQUIRE(0 == builder.add(data));
    REQUIRE(0 == builder.finish());
    const uint8_t* buffer = builder.get_buffer_pointer() + sizeof(FrameMessageHeader);
    REQUIRE(1.0f == *(const float*)buffer);
    REQUIRE(28 == builder.get_size());

}


TEST_CASE( "read data type from message return -1 on not found") {
    using namespace motesque;
    FrameMessageBuilder builder;
    HighRangeImuData data;
    data.acc_g[0]  =  1.0f;
    data.acc_g[1]  =  2.0f;
    data.acc_g[2]  =  3.0f;
    REQUIRE(0 == builder.add(data));
    REQUIRE(0 == builder.finish());
    HighRangeImuData data_copy;
    REQUIRE( 0 == get_frame_message_data( builder.get_buffer_pointer(), builder.get_size(), &data_copy));
    MagnetometerData lis3data;
    REQUIRE( -1 == get_frame_message_data( builder.get_buffer_pointer(), builder.get_size(), &lis3data));
}


TEST_CASE("read data type from message")
{
    using namespace motesque;
    FrameMessageBuilder builder;
    TimestampData time_data;
    memset(&time_data,0,sizeof(TimestampData));
    time_data.timestamp_us = 12344ULL;
    builder.add(time_data);
    
    HighRangeImuData icm20649_data;
    memset(&icm20649_data,0,sizeof(HighRangeImuData));
    float acc_g[] = {1.0f,2.0f,3.0f};
    memcpy(icm20649_data.acc_g, acc_g, sizeof(icm20649_data.acc_g));
    builder.add(icm20649_data);
    builder.finish();
    
    TimestampData a;
    REQUIRE( 0 ==  get_frame_message_data( builder.get_buffer_pointer(), builder.get_size(), &a));
    REQUIRE( 12344ULL ==  a.timestamp_us);
    HighRangeImuData b;
    REQUIRE( 0 ==  get_frame_message_data( builder.get_buffer_pointer(), builder.get_size(), &b));
    REQUIRE( 1.0f ==  b.acc_g[0]);
    REQUIRE( 2.0f ==  b.acc_g[1]);
    REQUIRE( 3.0f ==  b.acc_g[2]);

}

TEST_CASE("battery data type")
{
    using namespace motesque;
    FrameMessageBuilder builder;
    HighRangeImuData icm20649_data;
    memset(&icm20649_data,0,sizeof(HighRangeImuData));
    float acc_g[] = {1.0f,2.0f,3.0f};
    memcpy(icm20649_data.acc_g, acc_g, sizeof(icm20649_data.acc_g));
    builder.add(icm20649_data);
    
    BatteryData battery_data;
    battery_data.state_of_charge = 89.1f;
    builder.add(battery_data);
    
    builder.finish();
    
    BatteryData a;
    REQUIRE( 0 ==  get_frame_message_data( builder.get_buffer_pointer(), builder.get_size(), &a));
    REQUIRE( 89.1f ==  a.state_of_charge);
    HighRangeImuData b;
    REQUIRE( 0 ==  get_frame_message_data( builder.get_buffer_pointer(), builder.get_size(), &b));
    REQUIRE( 1.0f ==  b.acc_g[0]);
    REQUIRE( 2.0f ==  b.acc_g[1]);
    REQUIRE( 3.0f ==  b.acc_g[2]);

}
