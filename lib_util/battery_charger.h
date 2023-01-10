#pragma once
#include "wiced.h"
#include "battery_defs.h"
#include "pin_config.h"


static int read_device(uint8_t i2c_address, uint8_t read_register, uint8_t* reg_value, uint16_t read_size)
{
    wiced_i2c_device_t dev;

    memset(&dev, 0, sizeof(wiced_i2c_device_t));
    dev.port = WICED_I2C_1;  //I2C_1
    dev.address = i2c_address;
    dev.address_width = I2C_ADDRESS_WIDTH_7BIT;
    dev.speed_mode = I2C_STANDARD_SPEED_MODE;

    wiced_result_t rc = wiced_i2c_init(&dev);

    uint8_t wbuff[2];
    memset(wbuff, 0, 2);

    wbuff[0] = read_register;
    rc = wiced_i2c_write( &dev, WICED_I2C_START_FLAG, wbuff, 1);
    rc = wiced_i2c_read(&dev, WICED_I2C_REPEATED_START_FLAG | WICED_I2C_STOP_FLAG, reg_value, read_size);
    
    if (WICED_SUCCESS != rc) {
        return WICED_ERROR;
    }

    wiced_i2c_deinit(&dev);

    return 0;
}


static int write_device(uint8_t i2c_address, uint8_t* write_register, uint16_t write_reg_size_bytes)
{
    wiced_i2c_device_t dev;

    memset(&dev, 0, sizeof(wiced_i2c_device_t));
    dev.port = WICED_I2C_1;  //I2C_1
    dev.address = i2c_address;
    dev.address_width = I2C_ADDRESS_WIDTH_7BIT;
    dev.speed_mode = I2C_STANDARD_SPEED_MODE;

    wiced_result_t rc = wiced_i2c_init(&dev);

    rc = wiced_i2c_write( &dev, WICED_I2C_START_FLAG | WICED_I2C_STOP_FLAG, write_register, sizeof(write_register));
    if (WICED_SUCCESS != rc) {
        return WICED_ERROR;
    }

    wiced_i2c_deinit(&dev);

    return 0;
}





class FuelGauge {
public:
    FuelGauge() :
        m_device_address(MAX77818_FUEL_GAUGE_ADDR) {}

    int get_temperature(float* gauge_temperature) {
        // MAX77818 documentation:
        // When using AIN for temperature (Tex = 0), config-
        // ure TGAIN and TOFF to adjust the AIN measure-
        // ment to provide units degrees in the high-byte of
        // Temp. When TGAIN and TOFF are configured
        // properly for the selected thermistor, the LSB is
        // 0.0039 ̊C and the upper byte has units 1°C. Temp
        // is a signed register. To configure the BT07 to re-
        // ceive temperature information from the I2C, set Tex
        // = 1 and periodically write the Temp register with the
        // appropriate temperature.

        uint8_t temp[2];
        int rc = read_device(m_device_address, MAX_FG_TEMP_REG, temp, 2);
        if (0 != rc) {
            return rc;
        }

        float lower_temp = (float) temp[0] * 0.0039f;
        float upper_temp = (float) temp[1] * 1.0f;

        *gauge_temperature = (float) upper_temp + lower_temp;

        return 0;
    }
    int get_voltage(float* voltage)
    {
        uint8_t volt[2];
        int rc = read_device(m_device_address, MAX_FG_VCELL_REG, volt, 2);
        if (0 != rc) {
            return rc;
        }

        uint16_t vcell_voltage = volt[1] << 8;
        vcell_voltage |= volt[0];
        float v_factor = 78.125e-6;  // uV per LSB
        *voltage = (float) vcell_voltage * v_factor;

        return 0;
    }

    int get_inst_current(float* current)
    {
        // Sense resistor is in between CSP and CSN. Is not on schematic (maybe BAT_SP, BAT_SN)
        uint8_t curr[2];
        int rc = read_device(m_device_address, MAX_FG_CURRENT_REG, curr, 2);
        if (0 != rc) {
            return rc;
        }

        int16_t inst_curr = curr[1] << 8;
        inst_curr |= curr[0];
        WPRINT_APP_INFO(("inst current 0x%x, 0x%x\n", curr[1], curr[0]));
        float sense_resistor = 330.0;
        *current = (float) inst_curr / sense_resistor;

        return 0;
    }

    int get_avg_current(float* current)
    {
        // This is the 0.7s to 6.4hr (configurable) IIR av- erage of the current.
        // This register represents the upper 16 bits of the 32-bit shift register that 
        // filters current. The average should be set equal to Current upon startup

        uint8_t curr[2];
        int rc = read_device(m_device_address, MAX_FG_AVG_CURRENT_REG, curr, 2);
        if (0 != rc) {
            return rc;
        }

        uint16_t avg_curr = curr[1] << 8;
        avg_curr |= curr[0];
        float i_factor = 1.5625;  // uA / Rsns
  
        *current = (float) avg_curr * i_factor * 1e-6;

        return 0;
    }


    int status(uint8_t* status)
    {
        uint8_t status_reg_val[2];
        int rc = read_device(m_device_address, MAX_FG_STATUS, status_reg_val, 2);
        if (0 != rc) {
            return rc;
        }

        return 0;

    }

    int set_shutdown_timer(uint8_t timer)
    {
        uint8_t read_reg[2];
        int rc = read_device(m_device_address, MAX_FG_SD_TIMER, read_reg, 2);
        if (0 != rc) {
            WPRINT_APP_INFO(("error: set_shutdown_timer, cannot set register %d\n", MAX_FG_SD_TIMER));
            return rc;
        }
        uint8_t msb = read_reg[1] & 0b00011111;
        uint8_t write_reg[3] = {MAX_FG_SD_TIMER, read_reg[0], msb};
        rc = write_device(m_device_address, write_reg, 2);
        if (0 != rc) {
            return rc;
        }

        return 0;
    }

    uint8_t get_shutdown_threshold()
    {
        uint8_t read_reg[2];
        int rc = read_device(m_device_address, MAX_FG_SD_TIMER, read_reg, 2);
        if (0 != rc) {
            WPRINT_APP_INFO(("error: get_shutdown_threshold, cannot read register %d\n", MAX_FG_SD_TIMER));
            return rc;
        }

        return read_reg[1] >> 5;
    }

    uint16_t get_shutdown_counter()
    {
        uint8_t read_reg[2];
        int rc = read_device(m_device_address, MAX_FG_SD_TIMER, read_reg, 2);
        if (0 != rc) {
            WPRINT_APP_INFO(("error: get_shutdown_counter, cannot read register %d\n", MAX_FG_SD_TIMER));
            return rc;
        }
        uint16_t counter = 0;
        counter = (read_reg[1] & 0b00011111) << 8;
        counter |= read_reg[0];

        return counter;
    }

    int device_shutdown()
    {
        uint8_t read_reg[2];
        int rc = read_device(m_device_address, MAX_FG_CONFIG, read_reg, 2);
        if (0 != rc) {
            WPRINT_APP_INFO(("error: cannot read register %d\n", MAX_FG_CONFIG));
            return rc;
        }
        uint8_t sd_reg = 0b10000000 | read_reg[0];

        uint8_t sd_cmd[3]= {MAX_FG_CONFIG, sd_reg, read_reg[1]};
        rc = write_device(m_device_address, sd_cmd, 2);
        if (0 != rc) {
            WPRINT_APP_INFO(("error: cannot write register %d\n", MAX_FG_CONFIG));
            return rc;
        }
        uint8_t status_reg_val[2];
        rc = read_device(m_device_address, MAX_FG_CONFIG, status_reg_val, 3);
        if (0 != rc) {
            WPRINT_APP_INFO(("error: cannot read register %d\n", MAX_FG_CONFIG));
            return rc;
        }

        return 0;
    }


private:
    uint8_t m_device_address;
};

class BatteryCharger {
public:
    BatteryCharger() :
        m_device_address(MAX77818_BATT_CGR_ADDR) {
        }
    
    
    int top_off_time(uint8_t* top_off_time_min)
    {
        uint8_t ch_reg_value = 0;
        int rc = read_device(m_device_address, MAX_FG_CHG_CNFG_03, &ch_reg_value, 1);
        if (0 != rc) {
            return rc;
        }
        //WPRINT_APP_INFO(("ch reg 0x%x\n", ch_reg_value));
        // we just want bits 5:3
        ch_reg_value >>= 3;
        ch_reg_value &= 0b00000111;

        *top_off_time_min = ch_reg_value;

        return 0;
    }

    int charger_status_as_str(ChargerModes status, std::string& status_str)
    {
        int rc = 0;
        //char charger_status[50];
        switch (status) {
            case LowBattPreQual: status_str = "low_battery_pre_qual"; break;
            case FastChargeConstCurrent : status_str = "fast_charge_constant_current"; break;
            case FastChargeConstVoltage: status_str = "fast_charge_constant_voltage"; break;
            case TopOff: status_str = "top_off"; break;
            case Done: status_str = "done"; break;
            case TimerFault: status_str = "timer_fault"; break;
            case HighSuspend: status_str = "high_suspend"; break;  // detbatb
            case Off: status_str = "off"; break;
            case OffHighJunctionTemp: status_str = "off_high_junction_temperature"; break;
            case OffWatchdogTimer: status_str = "off_watchdog_timer"; break;
            default: status_str = "invalid_status"; rc = -1;
        }

        //memcpy(status_str, charger_status, str_len);

        return rc;
    }

    int battery_status_as_str(BatteryModes status, std::string& status_str)
    {
        switch (status) {
            case NoBattery: status_str = "no_battery"; break;
            case DeadBattPreQual : status_str = "low_battery_pre_qual"; break;
            case SlowChargeFault: status_str = "slow_charge_fault"; break;
            case OkayGoodVoltage: status_str = "ok_good_voltage"; break;
            case OkayLowVoltage: status_str = "ok_low_voltagef"; break;
            case OverVoltage: status_str = "over_voltage"; break;
            case OverCurrent: status_str = "over_current"; break;
            default: status_str = "invalid_status"; return -1;
        }

        return 0;
    }

    int charge_details(uint8_t* charger_status, uint8_t* battery_status)
    {
        // CHG_DETAILS_01 Register 0xB4
        // MAX77818 page 63
        uint8_t ch_reg_value = 0;
        int rc = read_device(m_device_address, MAX_CHG_DETAILS_01, &ch_reg_value, 1);
        if (0 != rc) {
            WPRINT_APP_INFO(("error: cannot read register %d\n", MAX_CHG_DETAILS_01));
            return rc;
        }

        // we want bits 3:0
        *charger_status = ch_reg_value & 0b00001111;
        // we want bits 6:4
        *battery_status = (ch_reg_value >> 4) & 0b00000111;

        return 0;
    }

    int enable_ship_mode()
    {
        uint8_t read_reg = 0;
        int rc = read_device(m_device_address, MAX_CHG_CNFG_00_REG, &read_reg, 1);
        if (0 != rc) {
            WPRINT_APP_INFO(("error: cannot read register %d\n", MAX_CHG_CNFG_00_REG));
            return rc;
        }
        uint8_t sd_reg = (1 << MAX_MBAT_TO_SYSFET_BIT) | read_reg;

        uint8_t sd_cmd[2]= {MAX_CHG_CNFG_00_REG, sd_reg};
        rc = write_device(m_device_address, sd_cmd, 2);
        if (0 != rc) {
            WPRINT_APP_INFO(("error: cannot write \"0x%x\" to register %d\n", sd_reg, MAX_CHG_CNFG_00_REG));
            return rc;
        }

        return 0;
    }


private:
    uint8_t m_device_address;
};
