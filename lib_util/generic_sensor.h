#pragma once
#include <cstring>
#include <array>
#include <vector>
#include <algorithm>
#include <cassert>

namespace motesque {


template<typename ENUM>
struct EnumValuesTrait
{

};

enum PresPowerMode {
    PRES_SLEEP_MODE = 0,
    PRES_FORCED_MODE = 1,
    PRES_NORMAL_MODE = 3,
    PRES_MODE_INVALID = -1
};
template<>
struct EnumValuesTrait<PresPowerMode>
{
    static const std::array<PresPowerMode, 3>& get() {
        static std::array<PresPowerMode, 3> valid = {PRES_SLEEP_MODE, PRES_FORCED_MODE, PRES_NORMAL_MODE};
        return valid;
    }
};

enum AccOperatingMode {
    ACC_OP_MODE_STANDBY = 0,
    ACC_OP_MODE_WAKE_UP = 1,
	ACC_OP_MODE_INSTANT_ON = 2,
	ACC_OP_MODE_FULL_BW_MEASUREMENT = 3,
    ACC_OP_MODE_INVALID = -1
};
template<>
struct EnumValuesTrait<AccOperatingMode>
{
    static const std::array<AccOperatingMode, 4>& get() {
        static std::array<AccOperatingMode, 4> valid = {ACC_OP_MODE_STANDBY, ACC_OP_MODE_WAKE_UP,
                        ACC_OP_MODE_INSTANT_ON, ACC_OP_MODE_FULL_BW_MEASUREMENT};
        return valid;
    }
};

enum AccBandwidth {
    ACC_BW_200HZ = 0,
	ACC_BW_400HZ = 1,
	ACC_BW_800HZ = 2,
	ACC_BW_1600HZ = 3,
	ACC_BW_3200HZ = 4,
    ACC_BW_INVALID = -1
};
template<>
struct EnumValuesTrait<AccBandwidth>
{
    static const std::array<AccBandwidth, 5>& get() {
        static std::array<AccBandwidth, 5> valid = {ACC_BW_200HZ, ACC_BW_400HZ, ACC_BW_800HZ, ACC_BW_1600HZ, ACC_BW_3200HZ};
        return valid;
    }
};

enum AccInstantOnThMode{
    ACC_INSTANT_ON_LOW_TH = 0,
	ACC_INSTANT_ON_HIGH_TH = 1,
    ACC_INSTANT_ON_INVALID = -1
};
template<>
struct EnumValuesTrait<AccInstantOnThMode>
{
    static const std::array<AccInstantOnThMode, 2>& get() {
        static std::array<AccInstantOnThMode, 2> valid = {ACC_INSTANT_ON_LOW_TH, ACC_INSTANT_ON_HIGH_TH};
        return valid;
    }
};

enum AccODR {
    ACC_ODR_400HZ = 0,
	ACC_ODR_800HZ = 1,
	ACC_ODR_1600HZ = 2,
	ACC_ODR_3200HZ = 3,
	ACC_ODR_6400HZ = 4,
    ACC_ODR_INVALID = -1
};
template<>
struct EnumValuesTrait<AccODR>
{
    static const std::array<AccODR, 5>& get() {
        static std::array<AccODR, 5> valid = {ACC_ODR_400HZ, ACC_ODR_800HZ, ACC_ODR_1600HZ, ACC_ODR_3200HZ, ACC_ODR_6400HZ};
        return valid;
    }
};

enum AccLrODR {
    
	ACC_ODR_1_5625HZ = 0,
	ACC_ODR_3_125HZ = 1,
	ACC_ODR_6_25HZ = 2,
	ACC_ODR_12_5HZ = 3,
    ACC_ODR_25HZ = 4,
    ACC_ODR_50HZ = 5,
    ACC_ODR_100HZ = 6,
    ACC_ODR_200HZ = 7,
    ACC_ODR_500HZ = 8,
    ACC_ODR_1KHZ = 9,
    ACC_ODR_2KHZ = 10,
    ACC_ODR_4KHZ = 11,
    ACC_ODR_8KHZ = 12,
    ACC_ODR_16KHZ = 13,
    ACC_ODR_32KHZ = 14,
    ACC_ODR_LR_INVALID = -1
};
template<>
struct EnumValuesTrait<AccLrODR>
{
    static const std::array<AccLrODR, 15>& get() {
        static std::array<AccLrODR, 15> valid = {ACC_ODR_1_5625HZ, ACC_ODR_3_125HZ, ACC_ODR_6_25HZ, ACC_ODR_12_5HZ, ACC_ODR_25HZ,
                                                 ACC_ODR_50HZ, ACC_ODR_100HZ, ACC_ODR_200HZ, ACC_ODR_500HZ, ACC_ODR_1KHZ, ACC_ODR_2KHZ,
                                                 ACC_ODR_4KHZ, ACC_ODR_8KHZ, ACC_ODR_16KHZ, ACC_ODR_32KHZ};
        return valid;
    }
};

enum GyrLrODR {
    
	GYR_ODR_12_5HZ = 0,
    GYR_ODR_25HZ = 1,
    GYR_ODR_50HZ = 2,
    GYR_ODR_100HZ = 3,
    GYR_ODR_200HZ = 4,
    GYR_ODR_500HZ = 5,
    GYR_ODR_1KHZ = 6,
    GYR_ODR_2KHZ = 7,
    GYR_ODR_4KHZ = 8,
    GYR_ODR_8KHZ = 9,
    GYR_ODR_16KHZ = 10,
    GYR_ODR_32KHZ = 11,
    GYR_ODR_LR_INVALID = -1
};
template<>
struct EnumValuesTrait<GyrLrODR>
{
    static const std::array<GyrLrODR, 12>& get() {
        static std::array<GyrLrODR, 12> valid = {GYR_ODR_12_5HZ, GYR_ODR_25HZ, GYR_ODR_50HZ, GYR_ODR_100HZ, GYR_ODR_200HZ, GYR_ODR_500HZ, GYR_ODR_1KHZ, GYR_ODR_2KHZ,
                                                 GYR_ODR_4KHZ, GYR_ODR_8KHZ, GYR_ODR_16KHZ, GYR_ODR_32KHZ};
        return valid;
    }
};

enum AccActivityThreshold{
	ACC_TH_ACTIVITY = 0,
	ACC_TH_ACTIVITY2 = 1,
	ACC_TH_INACTIVITY = 2,
    ACC_TH_INVALID = -1
};
template<>
struct EnumValuesTrait<AccActivityThreshold>
{
    static const std::array<AccActivityThreshold, 3>& get() {
        static std::array<AccActivityThreshold, 3> valid = {ACC_TH_ACTIVITY, ACC_TH_ACTIVITY2, ACC_TH_INACTIVITY};
        return valid;
    }
};

enum AccActProcMode{
	ACC_ACT_PROC_MODE_DEFAULT = 0,
	ACC_ACT_PROC_MODE_LINKED = 1,
	ACC_ACT_PROC_MODE_LOOPED = 2,
    ACC_ACT_PROC_MODE_INVALID = -1
};
template<>
struct EnumValuesTrait<AccActProcMode>
{
    static const std::array<AccActProcMode, 3>& get() {
        static std::array<AccActProcMode, 3> valid = {ACC_ACT_PROC_MODE_DEFAULT, ACC_ACT_PROC_MODE_LINKED, ACC_ACT_PROC_MODE_LOOPED};
        return valid;
    }
};

enum AccWakeUpRate {
    ACC_WUR_52ms = 0,
	ACC_WUR_104ms = 1,
	ACC_WUR_208ms = 2,
	ACC_WUR_512ms = 3,
	ACC_WUR_2048ms = 4,
	ACC_WUR_4096ms = 5,
	ACC_WUR_8192ms = 6,
	ACC_WUR_24576ms = 7,
    ACC_WUR_INVALID = -1
};
template<>
struct EnumValuesTrait<AccWakeUpRate>
{
    static const std::array<AccWakeUpRate, 8>& get() {
        static std::array<AccWakeUpRate, 8> valid = {ACC_WUR_52ms, ACC_WUR_104ms, ACC_WUR_208ms, ACC_WUR_512ms, ACC_WUR_2048ms,
                            ACC_WUR_4096ms, ACC_WUR_8192ms, ACC_WUR_24576ms};
        return valid;
    }
};

enum MagOperatingMode {
    MAG_OP_MODE_CONTINUOUS = 0,
    MAG_OP_MODE_SINGLE_TRIGGER = 1,
    MAG_OP_MODE_POWER_DOWN = 2,
    MAG_OP_MODE_INVALID = -1
};
template<>
struct EnumValuesTrait<MagOperatingMode>
{
    static const std::array<MagOperatingMode, 3>& get() {
        static std::array<MagOperatingMode, 3> valid = {MAG_OP_MODE_CONTINUOUS, MAG_OP_MODE_SINGLE_TRIGGER, MAG_OP_MODE_POWER_DOWN};
        return valid;
    }
};

enum MagScaleRange {
    MAG_SCALE_RANGE_4_GAUSS   = 4,
    MAG_SCALE_RANGE_8_GAUSS   = 8,
    MAG_SCALE_RANGE_12_GAUSS  = 12,
    MAG_SCALE_RANGE_16_GAUSS  = 16,
    MAG_SCALE_RANGE_INVALID  = -1
};
template<>
struct EnumValuesTrait<MagScaleRange>
{
    static const std::array<MagScaleRange, 4>& get() {
        static std::array<MagScaleRange, 4> valid = {MAG_SCALE_RANGE_4_GAUSS, MAG_SCALE_RANGE_8_GAUSS, MAG_SCALE_RANGE_12_GAUSS,
                                                        MAG_SCALE_RANGE_16_GAUSS};
        return valid;
    }
};

enum AccScaleRange {
    ACC_SCALE_RANGE_2G = 2,
    ACC_SCALE_RANGE_4G = 4,
    ACC_SCALE_RANGE_8G = 8,
    ACC_SCALE_RANGE_16G = 16,
    ACC_SCALE_RANGE_30G = 30,
    ACC_SCALE_RANGE_32G = 32,
    ACC_SCALE_RANGE_INVALID = -1
};
template<>
struct EnumValuesTrait<AccScaleRange>
{
    static const std::array<AccScaleRange, 6>& get() {
        static std::array<AccScaleRange, 6> valid = {ACC_SCALE_RANGE_2G,ACC_SCALE_RANGE_4G,ACC_SCALE_RANGE_8G,
                                                     ACC_SCALE_RANGE_16G, ACC_SCALE_RANGE_30G,ACC_SCALE_RANGE_32G};
        return valid;
    }
};

enum GyrScaleRange {
    GYR_SCALE_RANGE_16DPS = 16,
    GYR_SCALE_RANGE_31DPS = 31,
    GYR_SCALE_RANGE_62DPS = 62,
    GYR_SCALE_RANGE_125DPS = 125,
    GYR_SCALE_RANGE_250DPS = 250,
    GYR_SCALE_RANGE_500DPS = 500,
    GYR_SCALE_RANGE_1000DPS = 1000,
    GYR_SCALE_RANGE_2000DPS = 2000,
    GYR_SCALE_RANGE_4000DPS = 4000,
    GYR_SCALE_RANGE_INVALID = -1
};
template<>
struct EnumValuesTrait<GyrScaleRange>
{
    static const std::array<GyrScaleRange, 5>& get() {
        static std::array<GyrScaleRange, 5> valid = {GYR_SCALE_RANGE_250DPS,GYR_SCALE_RANGE_500DPS,GYR_SCALE_RANGE_1000DPS,
                                                     GYR_SCALE_RANGE_2000DPS, GYR_SCALE_RANGE_4000DPS};
        return valid;
    }
};


enum LowPassFilter
{
   // not known yet
};

/**
 * The Sample Rate options are only a rough estimate. Not every chip
 * can hit the number exactly, e.g. the icm20649 with its 1125Hz base rate
 */
enum SampleRate {
    SAMPLE_RATE_10HZ = 10,
    SAMPLE_RATE_100HZ = 100,
    SAMPLE_RATE_200HZ = 200,
    SAMPLE_RATE_500HZ = 500,
    SAMPLE_RATE_1000HZ = 1000,
    SAMPLE_RATE_INVALID = -1
};

template<>
struct EnumValuesTrait<SampleRate>
{
    static const std::array<SampleRate, 5>& get() {
        static std::array<SampleRate, 5> valid = {SAMPLE_RATE_10HZ, SAMPLE_RATE_100HZ,SAMPLE_RATE_200HZ,SAMPLE_RATE_500HZ,
                                                  SAMPLE_RATE_1000HZ};
        return valid;
    }
};

/**
 * Utility function to check wether a given integer value matches an enum.
 */
template<typename ENUM>
bool is_valid_enum_value(int value) {
    auto it = std::find_if(EnumValuesTrait<ENUM>::get().begin(), EnumValuesTrait<ENUM>::get().end(), [value](const ENUM& e_val) {
        return value == static_cast<int>(e_val);
    });
    return it != EnumValuesTrait<ENUM>::get().end();
}


/*
 * A host class to combine the different aspects of a harware sensor.
 *  - The device itself, typically contains handle or pointer
 *  - The settings, used to configure the sensor
 *  - The data processing, reads the values from the IC and scales them to SI units
 *  - The powersave mechanism. Used to control the power save features, if any
 *
 *  By using the policy pattern, we can hide the implementation details while gaining flexiblity
 *  to mix and match certain parts for testing or experimentation. E.g. we can change the data policy
 *  to use interrupts, fifo, triggered reads etc.
 */
template <typename DevicePolicy, typename SettingsPolicy,
          typename DataPolicy, typename PowersavePolicy>
class GenericSensor
{
public:
    typedef typename DataPolicy::Packet PacketType;
    typedef typename DataPolicy::PacketList PacketListType;
    GenericSensor() :
        m_device_policy(),
        m_settings_policy(),
        m_data_policy(),
        m_powersave_policy(),
        m_active(true)
    {

    }

    int initialize()
    {
        int rc = m_device_policy.initialize();
        rc |= m_settings_policy.initialize(m_device_policy.get());
        rc |= m_data_policy.initialize(m_device_policy.get());
        rc |= m_powersave_policy.initialize(m_device_policy.get());
        return rc;
    }

    int deinitialize()
    {
        int rc = m_powersave_policy.deinitialize();
        rc |= m_data_policy.deinitialize();
        rc |= m_settings_policy.deinitialize();
        rc |= m_device_policy.deinitialize();
        return rc;
    }

    const SettingsPolicy& const_settings()
    {
        return m_settings_policy;
    }
    SettingsPolicy& settings()
    {
        return m_settings_policy;
    }
    const PowersavePolicy& const_powersave()
    {
        return m_powersave_policy;
    }
    PowersavePolicy& powersave()
    {
        return m_powersave_policy;
    }

    DataPolicy& const_data()
    {
        return m_data_policy;
    }
    DataPolicy& data()
    {
        return m_data_policy;
    }

    DevicePolicy& const_device()
     {
         return m_device_policy;
     }
     const DevicePolicy& device()
     {
         return m_device_policy;
     }

     void set_active(bool active)
     {
         m_active = active;
     }
     bool is_active(){
         return m_active;
     }

private:
    DevicePolicy       m_device_policy;
    SettingsPolicy     m_settings_policy;
    DataPolicy         m_data_policy;
    PowersavePolicy    m_powersave_policy;
    bool               m_active;



};
}
