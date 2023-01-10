#pragma once
#include <stdint.h>
#include <array>
#include <cstring>
//#include <stdio.h>

namespace motesque {



// automatically filled message header.
struct FrameMessageHeader
{
    uint16_t message_size;  // the total size in byte of this message, including FrameMessageHeader
    uint16_t feature_mask;  // feature bits indicating the message content
};

struct SensorMetaData
{
    enum  { feature_id = 1};
    uint32_t semantic;          // semantic code of this sensor
    uint32_t sensor_id;         
};  

struct TimestampData
{
    enum  { feature_id = 1 << 1};     
    uint64_t timestamp_us;      // 64bt timestamp in microseconds
};  


// Motion data from the Icm20488. Lower range, typically +-16g, but less signal noise
struct LowNoiseImuData 
{
    enum  { feature_id = 1 << 2 };
    float acc_g[3];
    float gyr_rps[3];
};

// Motion data from the Icm20649. High range, typically +-32g, but more noise
struct HighRangeImuData {
    enum  { feature_id = 1 << 3};
    float acc_g[3];
    float gyr_rps[3];
};

// acc data for the ADXL372
struct ExtremeRangeAccData {
    enum  { feature_id = 1 << 4};
    float acc_g[3];
};

// pressure
struct BarometerData 
{
    enum  { feature_id = 1 << 5};
    float pressure_pa;
    float temperature_celsius;
};

// Compass 
struct MagnetometerData 
{
    enum  { feature_id = 1 << 6};
    float mag_gauss[3];
};

// Orientation data via data fusion, e.g. Kalman or Complementary
struct FusionData 
{
    enum  { feature_id = 1 << 7};
    float orientation_quat[4];
};

struct BatteryData 
{
    enum  { feature_id = 1 << 8};
    float state_of_charge;  // 0..1
};


// Recursive template patterns to compute the offsets for each sensor -data type at compile time
// The order is obviously important and follows the order of the feature_id
template <typename DataType>
struct FrameMessageOffset 
{
};

template <>
struct FrameMessageOffset<SensorMetaData> 
{
    enum { value = sizeof(FrameMessageHeader)};  
};

template <>
struct FrameMessageOffset<TimestampData> 
{
    enum { value = sizeof(SensorMetaData)  + FrameMessageOffset<SensorMetaData>::value};
};


template <>
struct FrameMessageOffset<LowNoiseImuData> 
{
    enum { value = sizeof(TimestampData)  + FrameMessageOffset<TimestampData>::value};
};

template <>
struct FrameMessageOffset<HighRangeImuData>
 {
    enum { value = sizeof(LowNoiseImuData) + FrameMessageOffset<LowNoiseImuData>::value};
};

template <>
struct FrameMessageOffset<ExtremeRangeAccData>
 {
    enum { value = sizeof(HighRangeImuData) + FrameMessageOffset<HighRangeImuData>::value};
};

template <>
struct FrameMessageOffset<BarometerData> 
{
    enum { value = sizeof(ExtremeRangeAccData) + FrameMessageOffset<ExtremeRangeAccData>::value};
};

template <>
struct FrameMessageOffset<MagnetometerData>
{
    enum { value = sizeof(BarometerData) + FrameMessageOffset<BarometerData>::value};
};

template <>
struct FrameMessageOffset<FusionData>
{
     enum { value = sizeof(MagnetometerData) + FrameMessageOffset<MagnetometerData>::value};
};

template <>
struct FrameMessageOffset<BatteryData>
{
     enum { value = sizeof(FusionData) + FrameMessageOffset<FusionData>::value};
};

// for convenience, hold a list of of ordered ids and respective sizes
typedef std::array<uint32_t,9> FeatureArray;
const FeatureArray& all_feature_ids();
const FeatureArray& all_feature_sizes();


// obtain the message size of this buffer by extracting the FrameMessageHeader
//uint16_t get_frame_message_size(uint8_t* msg);

template<typename DataTypeT>
int get_frame_message_data(const uint8_t* buffer, uint32_t buffer_size, DataTypeT* dt) {
    const FrameMessageHeader* hdr = (const FrameMessageHeader*)(buffer);
    auto&  features = all_feature_ids();
    auto& feature_sizes = all_feature_sizes();
    const uint8_t* buffer_ptr = buffer + sizeof(FrameMessageHeader);
    const uint8_t* buffer_end = buffer + buffer_size;
    for (size_t i=0; i < features.size(); i++) {
        // prevent reading past the end
        if (buffer_ptr >= buffer_end) {
            return -1;
        }
        if (hdr->feature_mask & features[i]) {  
            // we have the feature in the message
            if (features[i] == DataTypeT::feature_id) {
                 *dt = *(DataTypeT*)buffer_ptr;
                 return 0;
            }
            buffer_ptr += feature_sizes[i];
        }
    }
    return -1;
}


// Build a FrameMessage by adding data structs (order does not matter) and then calling finish. This last op "compacts" the buffer to 
// not having to send optional fields. 
// When extending the data model do the following:
// 1. Check wether the new field is best placed in an existing struct, e.g. SensorMetaData. If yes, you are done for the C++ part
// or 
// 1. Add a new struct and assign it a free |feature_mask bit| as feature_id 
// 2. Create FrameMessageOffset<> specialization template to compute the correct offset. Easiest here would be to append the struct at the end. 
// 3. Update 'all_feature_ids()' and 'all_feature_sizes' and increase the array datatype size  const std::array<uint32_t,N>& all_feature_ids();

// Whatever changes you make, keep python/pymotesque_mpu_sdk/pymotesque_mpu_sdk/messages/py_message_frame.py in sync!

class FrameMessageBuilder {
public:
 
    FrameMessageBuilder()
    : m_feature_mask(0),
        m_size(0),
        m_finished(false)
    {
        clear();

    }
    virtual ~FrameMessageBuilder()
    {

    }
    
    template<typename DataTypeT>
    int add(const DataTypeT& data) {
        if (m_finished) {
            // we must not more data after finished was called
            return -1;
        }
         m_feature_mask |= DataTypeT::feature_id;
         memcpy(m_buffer + FrameMessageOffset<DataTypeT>::value, &data, sizeof(DataTypeT));
         m_size += sizeof(data);
        return 0;
    }

    int finish() 
    {
        if  (m_finished) {
            return 0;
        }
        
        // compact the buffer, for every feature 
        // we go in order, for every feature not present, we more the memory to the left, by sizeof(Feature) bytes
        uint8_t* src_buffer_ptr = m_buffer + sizeof(FrameMessageHeader);
        uint8_t* buffer_ptr_end = m_buffer+sizeof(m_buffer);
        auto&  features = all_feature_ids();
        auto& feature_sizes = all_feature_sizes();
        
        for (size_t i=0; i < features.size(); i++) {
            if ((m_feature_mask & features[i])) {
                // we need this feature
                src_buffer_ptr += feature_sizes[i];
            }
            else {
                uint8_t* ptr_feature_end = src_buffer_ptr + feature_sizes[i];
                memmove(src_buffer_ptr, ptr_feature_end, buffer_ptr_end-ptr_feature_end);
            }
        }
        m_finished = true;

        FrameMessageHeader hdr;
        hdr.feature_mask = m_feature_mask;
        hdr.message_size = m_size;
        memcpy(m_buffer,&hdr,sizeof(FrameMessageHeader));
        return 0;
    }

    const uint8_t* get_buffer_pointer() const
    {
        return m_buffer;
    }

    uint32_t get_size() const
    {
        return m_size ;
    }
    void clear()
    {
        m_feature_mask = 0;
        m_size = sizeof(FrameMessageHeader);
        memset(m_buffer,0,sizeof(m_buffer));
        m_finished = false;
    }


private:
    uint16_t m_feature_mask;
    uint32_t m_size;
    uint8_t  m_buffer[512];
    bool     m_finished;
};




}