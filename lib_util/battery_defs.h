#pragma once

// CHG_DETAILS_01 Register 0xB4
// MAX77818 page 63
#define MAX_CHG_DETAILS_01 0xB4
enum ChargerModes {
    LowBattPreQual = 0x00,
    FastChargeConstCurrent = 0x01,
    FastChargeConstVoltage = 0x02,
    TopOff = 0x03,
    Done = 0x04,
    TimerFault = 0x06,
    HighSuspend = 0x07,
    Off = 0x08,
    OffHighJunctionTemp = 0x0A,
    OffWatchdogTimer = 0x0B
};

enum BatteryModes {
    NoBattery = 0x00,
    DeadBattPreQual = 0x01,
    SlowChargeFault = 0x02,
    OkayGoodVoltage = 0x03,
    OkayLowVoltage = 0x04,
    OverVoltage = 0x05,
    OverCurrent = 0x06,
};

// MAX77818 DEFINITIONS
#define MAX77818_BATT_CGR_ADDR (0xd2 >> 1) // 0xd3
#define MAX77818_BATT_CGR_TOP_LVL_ADDR (0xCC >> 1)
#define MAX77818_FUEL_GAUGE_ADDR (0x6c >> 1)  // 0x6d
// Clogic, GTEST and Safeout LDOs: 0xCCh/0xCDh
#define MAX_PMIC_ID_REG 0x20
#define MAX_PMIC_REV_REG 0x33
#define MAX_BATT_STATUS_REG 0x00
//#define 

#define MAX_FG_STATUS 0x00
#define MAX_FG_AGE_REG 0x07
#define MAX_FG_TEMP_REG 0x08
#define MAX_FG_VCELL_REG 0x09
#define MAX_FG_CURRENT_REG 0x0A
#define MAX_FG_AVG_CURRENT_REG 0x0B

#define MAX_FG_CHG_CNFG_03 0xBA   // bits: 2:0=TOP-OFF CURRENT - 5:3=TOP-OFF TIME - 7:6=PEAK CURRENT LIMIT (see datasheet for details)
#define MAX_FG_CONFIG 0x1D
#define MAX_FG_SD_TIMER 0x3F
#define MAX_CHG_CNFG_00_REG 0xB7  // part of BATT_CGR, not FG
#define MAX_MBAT_TO_SYSFET_BIT 6

// TPS63810 DEFINITIONS
#define TPS63810_ADDR 0x75 //0b1110101
#define TPS_CTRL_REG 0x01
#define TPS_STATUS_REG 0x02
#define TPS_DEVID_REG 0x03
#define TPS_VOUT1_REG 0x04
#define TPS_VOUT2_REG 0x05
