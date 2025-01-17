/*
 * Pylontech_CAN.h
 *
 * Definitions for the CAN frames to send as Pylon protocol.
 * TODO The generated output does not correspond to the logs below and
 * published at https://www.setfirelabs.com/green-energy/pylontech-can-reading-can-replication
 *
 *  Copyright (C) 2023  Armin Joachimsmeyer
 *  Email: armin.joachimsmeyer@gmail.com
 *
 *  This file is part of ArduinoUtils https://github.com/ArminJo/PVUtils.
 *
 *  Arduino-Utils is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#ifndef _PYLONTECH_CAN_H
#define _PYLONTECH_CAN_H

#include <Arduino.h>
#include "JK-BMS.h"
#include "LongUnion.h"

/* LOG:
 7 35E 00 00 00 00 00 00 00 00 08 08
 6 35C 00 00 00 00 00 00 00 00 08 08
 5 356 00 00 00 00 0A 50 4E 00 07 07
 4 355 14 02 74 0E 74 0E CC 01 08 08
 3 351 0E 00 64 00 00 00 00 00 04 04
 2 359 02 13 00 00 4A 01 00 00 06 06
 1 305 C0 00 00 00 00 00 00 00 02 02
 0 305 50 59 4C 4F 4E 20 20 20 08 08 // ("PYLON")
 */

/*
 * Frame ID's
 */
#define PYLON_CAN_NETWORK_ALIVE_MSG_FRAME_ID            0x305
#define PYLON_CAN_BATTERY_LIMITS_FRAME_ID               0x351 // Battery voltage + current limits
#define PYLON_CAN_BATTERY_SOC_SOH_FRAME_ID              0x355 // State of Health (SOH) / State of Charge (SOC)
#define PYLON_CAN_BATTERY_CURRENT_VALUES_U_I_T_FRAME_ID 0x356 // Voltage / Current / Temperature
#define PYLON_CAN_BATTERY_ERROR_WARNINGS_FRAME_ID       0x359 // Protection & Alarm flags
#define PYLON_CAN_BATTERY_CHARGE_REQUEST_FRAME_ID       0x35C // Battery charge request flags
#define PYLON_CAN_BATTERY_MANUFACTURER_FRAME_ID         0x35E // Manufacturer name ("PYLON")
#define PYLON_CAN_BATTERY_SPECIFICATIONS_FRAME_ID       0x35F // Chemistry and Capacity for SMA Sunny Island inverters
#define PYLON_CAN_BATTERY_LUXPOWER_CAPACITY_FRAME_ID    0x379 // Capacity for Luxpower SNA inverters
#define PYLON_CAN_BATTERY_CELL_INFO_FRAME_ID            0x373 // Cell infor

extern struct PylontechCANBatteryLimitsFrameStruct PylontechCANBatteryLimitsFrame;
extern struct PylontechCANSohSocFrameStruct PylontechCANSohSocFrame;
extern struct PylontechCANCurrentValuesFrameStruct PylontechCANCurrentValuesFrame;
extern struct PylontechCANManufacturerFrameStruct PylontechCANManufacturerFrame;
extern struct PylontechCANBatteryRequesFrameStruct PylontechCANBatteryRequestFrame;
extern struct PylontechCANAliveFrameStruct PylontechCANAliveFrameStruct;
extern struct PylontechCANErrorsWarningsFrameStruct PylontechCANErrorsWarningsFrame;
extern struct PylontechCANSpecificationsFrameStruct PylontechCANSpecificationsFrame;
// added by Ngoc
extern struct PylontechCANSMACapacityFrameStruct PylontechCANSMACapacityFrame;
extern struct PylontechCANLuxpowerCapacityFrameStruct PylontechCANLuxpowerCapacityFrame;
extern struct PylontechCANCellInfoFrameStruct PylontechCANCellInfoFrame;

void fillPylontechCANBatteryLimitsFrame(struct JKReplyStruct *aJKFAllReply);
void fillPylontechCANBatterySohSocFrame(struct JKReplyStruct *aJKFAllReply);
void fillPylontechCANManufactureFrame(struct JKReplyStruct *aJKFAllReply);
void fillPylontechCANBatteryRequesFrame(struct JKReplyStruct *aJKFAllReply);
void fillPylontechCANErrors_WarningsFrame(struct JKReplyStruct *aJKFAllReply);
void fillPylontechCANCurrentValuesFrame(struct JKReplyStruct *aJKFAllReply);

void fillAllCANData(struct JKReplyStruct *aJKFAllReply);
void sendPylontechAllCANFrames(bool aDebugModeActive);
void modifyAllCanDataToInactive();

// added by Ngoc
// checking limit for charging and apply charging control
uint8_t ReachChargeLimit();
void ControlChargeScheme();
void resetCharge();

struct PylontechCANFrameInfoStruct {
    // Both values will be statically initialized in each instance
    uint16_t CANId;
    uint8_t FrameLength;
};

struct PylontechCANFrameStruct {
    struct PylontechCANFrameInfoStruct PylontechCANFrameInfo;
    LongLongUnion FrameData; // 8 byte
};

struct PylontechCANAliveFrameStruct {
    struct PylontechCANFrameInfoStruct PylontechCANFrameInfo = { PYLON_CAN_NETWORK_ALIVE_MSG_FRAME_ID, 8 }; // 0x305
    struct {
        uint8_t AlivePacketArray[8] = { 33 };
    } FrameData;
};

struct PylontechCANBatteryLimitsFrameStruct {
    struct PylontechCANFrameInfoStruct PylontechCANFrameInfo = { PYLON_CAN_BATTERY_LIMITS_FRAME_ID, 8 }; // 0x351
    struct {
        int16_t BatteryChargeOvervoltage100Millivolt;       // 0 to 750
        int16_t BatteryChargeCurrentLimit100Milliampere;    // 0 to 5000
        int16_t BatteryDischargeCurrentLimit100Milliampere; // -5000 to 0
        int16_t BatteryDischarge100Millivolt;               // 0 to 65535 // not in documentation
    } FrameData;
    void fillFrame(struct JKReplyStruct *aJKFAllReply) {
        FrameData.BatteryChargeOvervoltage100Millivolt = JKComputedData.BatteryFullVoltage10Millivolt / 10;
        FrameData.BatteryChargeOvervoltage100Millivolt = JKComputedData.BatteryFullVoltage10Millivolt / 10;
        if (Charge_Current_100_milliAmp > 0){
          FrameData.BatteryChargeCurrentLimit100Milliampere = Charge_Current_100_milliAmp;
        }else {
          FrameData.BatteryChargeCurrentLimit100Milliampere = swap(aJKFAllReply->ChargeOvercurrentProtectionAmpere) * 10;
        }
        //FrameData.BatteryChargeCurrentLimit100Milliampere = swap(aJKFAllReply->ChargeOvercurrentProtectionAmpere) * 10;
        FrameData.BatteryDischargeCurrentLimit100Milliampere = swap(aJKFAllReply->DischargeOvercurrentProtectionAmpere) * 10;
        FrameData.BatteryDischarge100Millivolt = swap(aJKFAllReply->BatteryUndervoltageProtection10Millivolt) / 10;
    }
};

struct PylontechCANSohSocFrameStruct {
    struct PylontechCANFrameInfoStruct PylontechCANFrameInfo = { PYLON_CAN_BATTERY_SOC_SOH_FRAME_ID, 4 }; // 0x355
    struct {
        uint16_t SOCPercent;
        uint16_t SOHPercent = 100; // fixed 100
#if defined(SMA_EXTENSIONS)
//        uint16_t SOCHighDefinition100PPM = 10000; // for SMA Sunny Island inverters
#endif
    } FrameData;
    void fillFrame(struct JKReplyStruct *aJKFAllReply) {
        //FrameData.SOCPercent = JKComputedData.SOCPercent; //Temporary taken out for testing with real SOC        
        if (JKComputedData.BatteryType == 0) {
          FrameData.SOHPercent = round(((JKComputedData.Cycles/MAX_CYCLES_LFP)-1)*-100);
        }else if (JKComputedData.BatteryType == 1) {
          FrameData.SOHPercent = round(((JKComputedData.Cycles/MAX_CYCLES_LF)-1)*-100);
        }
        FrameData.SOHPercent = JKComputedData.Cycles; 
    }
};

struct PylontechCANCurrentValuesFrameStruct {
    struct PylontechCANFrameInfoStruct PylontechCANFrameInfo = { PYLON_CAN_BATTERY_CURRENT_VALUES_U_I_T_FRAME_ID, 6 }; // 0x356
    struct {
        int16_t Voltage10Millivolt;        // 0 to 32767
        int16_t Current100Milliampere;      // -2500 to 2500
        int16_t Temperature100Millicelsius; // -500 to 750
    } FrameData;
    void fillFrame(struct JKReplyStruct *aJKFAllReply) {
        (void) aJKFAllReply; // To avoid [-Wunused-parameter] warning
        FrameData.Voltage10Millivolt = JKComputedData.BatteryVoltage10Millivolt;
        FrameData.Current100Milliampere = JKComputedData.Battery10MilliAmpere / 10;
        FrameData.Temperature100Millicelsius = JKComputedData.TemperatureMaximum * 10;
    }
};

struct PylontechCANErrorsWarningsFrameStruct {
    struct PylontechCANFrameInfoStruct PylontechCANFrameInfo = { PYLON_CAN_BATTERY_ERROR_WARNINGS_FRAME_ID, 7 }; // 0x359
    struct FrameDataStruct {
        // 0=off 1=on
        // Byte 0
        bool :1; // unused
        bool CellOvervoltageError :1;           // 0x02
        bool CellUndervoltageError :1;          // 0x04
        bool CellOvertemperatureError :1;       // 0x08
        bool CellUndertemperatureError :1;      // 0x10
        bool :2; // unused
        bool DischargeOvercurrentError :1;      // 0x80

        // Byte 1
//    bool :0;
        bool ChargeOvercurrentError :1;         // 0x01
        bool :6; // unused
        bool SystemError :1;                    // 0x80

        // Byte 2
//    bool :0;
        bool :1; // unused
        bool CellHighVoltageWarning :1;         // 0x02
        bool CellLowVoltageWarning :1;          // 0x04
        bool CellHighTemperatureWarning :1;     // 0x08
        bool CellLowTemperatureWarning :1;      // 0x10
        bool :2; // unused
        bool DischargeHighCurrentWarning :1;    // 0x80

        // Byte 3
//    bool :0;
        bool ChargeHighCurrentWarning :1;       // 0x01
        // found in documentation
//        bool :2; // unused
//        bool InternalCommunicationFail :1;
//        bool :4; // unused

        // Found in software
        bool :6; // unused
        bool SystemWarning :1;                  // 0x80

        // Byte 4 to 6
//    bool :0;
        uint8_t ModuleNumber = 1;           // 0 to 255. 1 is displayed on Deye as "LiBms01"
        uint8_t Token1 = 0x50;              // 'P'
        uint8_t Token2 = 0x4E;              // 'N'
    } FrameData;
    void fillFrame(struct JKReplyStruct *aJKFAllReply) {
        /*
         * Pylon has no battery over voltage alarm but cell over voltage warning and error
         * We (mis)use the battery alarms as cell warnings
         */
        // Byte 0
        FrameData.CellOvervoltageError = aJKFAllReply->AlarmUnion.AlarmBits.CellOvervoltageAlarm;
        FrameData.CellUndervoltageError = aJKFAllReply->AlarmUnion.AlarmBits.CellUndervoltageAlarm;
        FrameData.CellOvertemperatureError = aJKFAllReply->AlarmUnion.AlarmBits.PowerMosFetOvertemperatureAlarm
                || aJKFAllReply->AlarmUnion.AlarmBits.Sensor1Or2OvertemperatureAlarm;
        FrameData.CellUndertemperatureError = aJKFAllReply->AlarmUnion.AlarmBits.Sensor1Or2UndertemperatureAlarm;
        FrameData.DischargeOvercurrentError = aJKFAllReply->AlarmUnion.AlarmBits.DischargeOvercurrentAlarm;

        // Byte 1
        FrameData.ChargeOvercurrentError = aJKFAllReply->AlarmUnion.AlarmBits.ChargeOvercurrentAlarm;
        FrameData.SystemError = aJKFAllReply->BMSStatus.StatusBits.BatteryDown;
//        if (aJKFAllReply->SOCPercent < 5) {
//            FrameData.SystemError = 1;
//        }

        // Byte 2
        // (mis)use the battery alarms as cell warnings for Pylon
        FrameData.CellHighVoltageWarning = aJKFAllReply->AlarmUnion.AlarmBits.ChargeOvervoltageAlarm;
        FrameData.CellLowVoltageWarning = aJKFAllReply->AlarmUnion.AlarmBits.DischargeUndervoltageAlarm;
        // Use the same values as for error here
        FrameData.CellHighTemperatureWarning = aJKFAllReply->AlarmUnion.AlarmBits.PowerMosFetOvertemperatureAlarm
                || aJKFAllReply->AlarmUnion.AlarmBits.Sensor1Or2OvertemperatureAlarm;
        FrameData.CellLowTemperatureWarning = aJKFAllReply->AlarmUnion.AlarmBits.Sensor1Or2UndertemperatureAlarm;
        FrameData.DischargeHighCurrentWarning = aJKFAllReply->AlarmUnion.AlarmBits.DischargeOvercurrentAlarm;

        // Byte 3
        // Use the same values as for error here
        FrameData.ChargeHighCurrentWarning = aJKFAllReply->AlarmUnion.AlarmBits.ChargeOvercurrentAlarm;
        FrameData.SystemError = aJKFAllReply->BMSStatus.StatusBits.BatteryDown;

//        if (aJKFAllReply->SOCPercent < 10) {
//            FrameData.SystemWarning = 1;
//        }

    }
};

/*
 * ForceChargeRequestI / bit 5 is designed for inverter allows battery to shut down, and able to wake battery up to charge it.
 * ForceChargeRequestII / bit 4 is designed for inverter doesn`t want battery to shut down, able to charge battery before shut down to avoid low energy.
 * 2 bytes
 */
struct PylontechCANBatteryRequesFrameStruct {
    struct PylontechCANFrameInfoStruct PylontechCANFrameInfo = { PYLON_CAN_BATTERY_CHARGE_REQUEST_FRAME_ID, 2 }; // 0x35C
    struct {
        bool :3; // unused
        // 0=off 1=Request
        /*
         * From Pylontech manual: Depending on the soc level, there will be a regularly (3 month) fully charge requesting during continuous operation as well.
         * It will be handled automatically by the communication between BESS and external device.
         * This is to stop SOC calculations drifting too far from reality when the battery has not had a full charge for n days.
         */
        bool FullChargeRequest :1; // sent as always 0

        // Force to charge battery even from the grid.
        // From Battery-Communications-Integration-Guide-V2.5-1.pdf: Command sent by the BMS telling the inverter to charge the battery from any available power source regardless of inverter settings.
        bool ForceChargeRequestII :1;
        bool ForceChargeRequestI :1;
        bool DischargeEnable :1;
        bool ChargeEnable :1;
        uint8_t Filler = 0;
    } FrameData;
    void fillFrame(struct JKReplyStruct *aJKFAllReply) {
        FrameData.FullChargeRequest = 0;

        if (aJKFAllReply->SOCPercent < sSOCThresholdForForceCharge) {
            // ForceChargeRequestI forces the inverter to charge the battery from any available power source regardless of inverter settings
            FrameData.ForceChargeRequestI = 1;
        } else {
            FrameData.ForceChargeRequestI = 0;
        }
        // If battery drops below lower voltage. See https://powerforum.co.za/topic/13587-battery-anomaly-on-synsynk-hybrid-inverter/
        if (swap(aJKFAllReply->Battery10Millivolt) < swap(aJKFAllReply->BatteryUndervoltageProtection10Millivolt)) {
            FrameData.ForceChargeRequestII = 1;
        } else {
            FrameData.ForceChargeRequestII = 0;
        }
        FrameData.DischargeEnable = aJKFAllReply->BMSStatus.StatusBits.ChargeMosFetActive;
        FrameData.ChargeEnable = aJKFAllReply->BMSStatus.StatusBits.DischargeMosFetActive;
    }
};

/*
 * Character array DIYPYLON is not recognized by Deye, array PYLONDIY is recognized as PYLON
 */
struct PylontechCANManufacturerFrameStruct {
    struct PylontechCANFrameInfoStruct PylontechCANFrameInfo = { PYLON_CAN_BATTERY_MANUFACTURER_FRAME_ID, 8 }; // 0x35E
    struct {
        char ManufacturerName[8] = { 'P', 'Y', 'L', 'O', 'N', ' ', ' ', ' ' };
    } FrameData;
};
// 0x35F should be removed as it would not work with Luxpower or goodwe, i replaced with 0x373 as placed below
struct PylontechCANSpecificationsFrameStruct {
    struct PylontechCANFrameInfoStruct PylontechCANFrameInfo = { PYLON_CAN_BATTERY_SPECIFICATIONS_FRAME_ID, 8 }; // 0x35F
    struct {
        uint16_t CellChemistry;             // 0
        uint8_t HardwareVersionLowByte;     // "0.9"
        uint8_t HardwareVersionHighByte;    // "0.9"
        uint16_t CapacityAmpereHour;        // -2500 to 2500
        uint8_t SoftwareVersionLowByte;     // "0.9"
        uint8_t SoftwareVersionHighByte;    // "0.9"
    } FrameData;
    void fillFrame(struct JKReplyStruct *aJKFAllReply) {
        (void) aJKFAllReply; // To avoid [-Wunused-parameter] warning
        FrameData.CellChemistry = 0;
        FrameData.HardwareVersionLowByte = 0;
        FrameData.HardwareVersionHighByte = 1;
        FrameData.SoftwareVersionLowByte = aJKFAllReply->SoftwareVersionNumber[1];
        FrameData.SoftwareVersionHighByte = aJKFAllReply->SoftwareVersionNumber[0];
        FrameData.CapacityAmpereHour = JKComputedData.TotalCapacityAmpereHour;
    }
};

/*
 * -Static data-
 * Frame for total capacity for Luxpower - SNA inverters
 * Description was found in a discussion under this fork from Peter: https://github.com/Uksa007/esphome-jk-bms-can/blob/deprecated/esp32-example-can.yaml
 */
struct PylontechCANLuxpowerCapacityFrameStruct {
    struct PylontechCANFrameInfoStruct PylontechCANFrameInfo = { PYLON_CAN_BATTERY_LUXPOWER_CAPACITY_FRAME_ID, 8 }; // 0x379
    struct {
        uint16_t CapacityAmpereHour;
        uint16_t Unknown1;
        uint32_t Unknown2;
    } FrameData;
    void fillFrame(struct JKReplyStruct *aJKFAllReply) {
        (void) aJKFAllReply; // To avoid [-Wunused-parameter] warning
        FrameData.CapacityAmpereHour = JKComputedData.TotalCapacityAmpereHour;
    }
};

/* -Static data-PYLON_CAN_BATTERY_PACK_INFO_FRAME_ID
 * Frame for The name of the cell
 * Description was found in: https://github.com/dfch/BydCanProtocol/tree/main
 * These things happended once and need to be updated every 5 minutes. Next time I will try to capture full frame of data from the SVE5000 battery
 * 25112023: recorded
 */
struct PylontechCANCellInfoFrameStruct {
    struct PylontechCANFrameInfoStruct PylontechCANFrameInfo = { PYLON_CAN_BATTERY_CELL_INFO_FRAME_ID, 8 }; // 0x373
    struct {
        uint16_t CellVoltageMinimumMilliVolt;
        uint16_t CellVoltageMaximumMilliVolt;        
        /// @brief Represents the maximum cell temperature in Kelvin.
        uint16_t CellTemperatureMinimumC;
        uint16_t CellTemperatureMaximumC;        
    } FrameData;
    void fillFrame(struct JKReplyStruct *aJKFAllReply) {
        FrameData.CellVoltageMinimumMilliVolt = JKComputedData.MinimumCellMillivolt;
        FrameData.CellVoltageMaximumMilliVolt = JKComputedData.MaximumCellMillivolt;
        FrameData.CellTemperatureMinimumC = 0x6F; //11.1 no data
        FrameData.CellTemperatureMaximumC = 0xDE;//22.2 JKComputedData.TemperatureMaximum; // no data
    }
};
#endif // _PYLONTECH_CAN_H
