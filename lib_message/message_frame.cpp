#include "message_frame.h"

namespace motesque {


const FeatureArray& all_feature_ids() {
    static 
    FeatureArray features = {{SensorMetaData::feature_id, TimestampData::feature_id, LowNoiseImuData::feature_id, HighRangeImuData::feature_id,ExtremeRangeAccData::feature_id,
                                        BarometerData::feature_id, MagnetometerData::feature_id, FusionData::feature_id, BatteryData::feature_id}};
    return features;                                      
}

const FeatureArray& all_feature_sizes() {
    static 
    FeatureArray feature_sizes = {{sizeof(SensorMetaData), sizeof(TimestampData), sizeof(LowNoiseImuData), sizeof(HighRangeImuData),sizeof(ExtremeRangeAccData),
                                             sizeof(BarometerData), sizeof(MagnetometerData), sizeof(FusionData),sizeof(BatteryData)}};
    return feature_sizes;                                      
}

// uint16_t get_frame_message_size(uint8_t* msg) {
//     return ((FrameMessageHeader*)msg)->message_size;
// }

};