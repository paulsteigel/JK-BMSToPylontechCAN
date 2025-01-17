/*
 * JK-BMS.cpp
 *
 * Functions to read, convert and print JK-BMS data
 *
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

#ifndef _JK_BMS_HPP
#define _JK_BMS_HPP

#include <Arduino.h>

#include "JK-BMS.h"

JKReplyStruct lastJKReply;

#if defined(DEBUG)
#define LOCAL_DEBUG
#else
//#define LOCAL_DEBUG // This enables debug output only for this file - only for development
#endif

// see JK Communication protocol.pdf http://www.jk-bms.com/Upload/2022-05-19/1621104621.pdf
uint8_t JKRequestStatusFrame[21] = { 0x4E, 0x57 /*4E 57 = StartOfFrame*/, 0x00, 0x13 /*0x13 | 19 = LengthOfFrame*/, 0x00, 0x00,
        0x00, 0x00/*BMS ID, highest byte is default 00*/, 0x06/*Function 1=Activate, 3=ReadIdentifier, 6=ReadAllData*/,
        0x03/*Frame source 0=BMS, 1=Bluetooth, 2=GPRS, 3=PC*/, 0x00 /*TransportType 0=Request, 1=Response, 2=BMSActiveUpload*/,
        0x00/*0=ReadAllData or commandToken*/, 0x00, 0x00, 0x00,
        0x00/*RecordNumber High byte is random code, low 3 bytes is record number*/, JK_FRAME_END_BYTE/*0x68 = EndIdentifier*/,
        0x00, 0x00, 0x01, 0x29 /*Checksum, high 2 bytes for checksum not yet enabled -> 0, low 2 Byte for checksum*/};
uint8_t JKrequestStatusFrameOld[] = { 0xDD, 0xA5, 0x03, 0x00, 0xFF, 0xFD, 0x77 };

uint16_t sReplyFrameBufferIndex = 0;        // Index of next byte to write to array, except for last byte received. Starting with 0.
uint16_t sReplyFrameLength;                 // Received length of frame
uint8_t JKReplyFrameBuffer[350];            // The raw big endian data as received from JK BMS
bool sJKBMSFrameHasTimeout;                 // If true, timeout message or CAN Info page is displayed.

JKConvertedCellInfoStruct JKConvertedCellInfo;  // The converted little endian cell voltage data
JKComputedDataStruct JKComputedData;            // All derived converted and computed data useful for display
JKComputedDataStruct lastJKComputedData;        // For detecting changes
char sUpTimeString[12];                         // "9999D23H12M" is 11 bytes long
char sBalancingTimeString[11] = { ' ', ' ', '0', 'D', '0', '0', 'H', '0', '0', 'M', '\0' };    // "999D23H12M" is 10 bytes long
bool sUpTimeStringMinuteHasChanged;
bool sUpTimeStringTenthOfMinuteHasChanged;
char sLastUpTimeTenthOfMinuteCharacter;     // For detecting changes in string and setting sUpTimeStringTenthOfMinuteHasChanged

/*
 * The JKFrameAllDataStruct starts behind the header + cell data header 0x79 + CellInfoSize + the variable length cell data (CellInfoSize is contained in JKReplyFrameBuffer[12])
 */
JKReplyStruct *sJKFAllReplyPointer;

const char lowCapacity[] PROGMEM = "Low capacity";                          // Byte 0.0,
const char MosFetOvertemperature[] PROGMEM = "Power MosFet overtemperature"; // Byte 0.1;
const char chargingOvervoltage[] PROGMEM = "Battery is full";               // Byte 0.2,  // Charging overvoltage
const char dischargingUndervoltage[] PROGMEM = "Discharging undervoltage";  // Byte 0.3,
const char Sensor2Overtemperature[] PROGMEM = "Sensor1_2 overtemperature";  // Byte 0.4,
const char chargingOvercurrent[] PROGMEM = "Charging overcurrent";          // Byte 0.5,
const char dischargingOvercurrent[] PROGMEM = "Discharging overcurrent";    // Byte 0.6,
const char CellVoltageDifference[] PROGMEM = "Cell voltage difference";     // Byte 0.7,
const char Sensor1Overtemperature[] PROGMEM = "Sensor2 overtemperature";    // Byte 1.0,
const char Sensor2LowLemperature[] PROGMEM = "Sensor1_2 low temperature";   // Byte 1.1,
const char CellOvervoltage[] PROGMEM = "Cell overvoltage";                  // Byte 1.2,
const char CellUndervoltage[] PROGMEM = "Cell undervoltage";                // Byte 1.3,
const char _309AProtection[] PROGMEM = "309_A protection";                  // Byte 1.4,
const char _309BProtection[] PROGMEM = "309_B protection";                  // Byte 1.5,

//added by Ngoc for fake map from volt to SOC
#define LFP_LEN 15
#define LION_LEN 21

uint8_t MapSOCLFP[LFP_LEN] = {0, 1, 5, 10, 14, 20, 30, 40, 50, 60, 70, 80, 90, 99, 100}; // array to keep SOC level
uint8_t MapSOCLion[LION_LEN] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100};
uint16_t MapVoltLFP[LFP_LEN] = {2500, 2538, 2800, 3000, 3150, 3200, 3225, 3250, 3263, 3275, 3300, 3325, 3350, 3375, 3450}; // Keep Cell milivolt for LFP mapping
uint16_t MapVoltLion[LION_LEN] = {3000, 3062, 3123, 3177, 3238, 3300, 3362, 3423, 3477, 3538, 3600, 3662, 3723, 3777, 3838, 3900, 3962, 4023, 4077, 4138, 4200};


const char *const JK_BMSErrorStringsArray[NUMBER_OF_DEFINED_ALARM_BITS] PROGMEM = { lowCapacity, MosFetOvertemperature,
        chargingOvervoltage, dischargingUndervoltage, Sensor2Overtemperature, chargingOvercurrent, dischargingOvercurrent,
        CellVoltageDifference, Sensor1Overtemperature, Sensor2LowLemperature, CellOvervoltage, CellUndervoltage, _309AProtection,
        _309BProtection };
const char *sErrorStringForLCD; // store of the error string of the highest error bit, NULL otherwise
bool sErrorStatusJustChanged = false; // True -> display overview page. Is set by handleAndPrintAlarmInfo(), if error flags changed, and reset on switching to overview page.
bool sErrorStatusIsError = false; // True if status is error and beep should be started. False e.g. for "Battery full".

/*
 * Helper macro for getting a macro definition as string
 */
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

/*
 * 1.85 ms
 */
void requestJK_BMSStatusFrame(SoftwareSerialTX *aSerial, bool aDebugModeActive) {
    if (aDebugModeActive) {
        Serial.println();
        Serial.println(F("Send requestFrame with TxToJKBMS"));
        for (uint8_t i = 0; i < sizeof(JKRequestStatusFrame); ++i) {
            Serial.print(F(" 0x"));
            Serial.print(JKRequestStatusFrame[i], HEX);
        }
        Serial.println();
    }
    Serial.flush();

    for (uint8_t i = 0; i < sizeof(JKRequestStatusFrame); ++i) {
        aSerial->write(JKRequestStatusFrame[i]);
    }
}

void initJKReplyFrameBuffer() {
    sReplyFrameBufferIndex = 0;
}

/*
 * Prints formatted reply buffer raw content
 */
void printJKReplyFrameBuffer() {
    for (uint16_t i = 0; i < (sReplyFrameBufferIndex + 1); ++i) {
        /*
         * Insert newline and address after header (11 byte), before and after cell data before trailer (9 byte) and after each 16 byte
         */
        if (i == JK_BMS_FRAME_HEADER_LENGTH || i == JK_BMS_FRAME_HEADER_LENGTH + 2
                || i
                        == (uint16_t) (JK_BMS_FRAME_INDEX_OF_CELL_INFO_LENGTH + 1
                                + JKReplyFrameBuffer[JK_BMS_FRAME_INDEX_OF_CELL_INFO_LENGTH])
                || i == ((sReplyFrameBufferIndex + 1) - JK_BMS_FRAME_TRAILER_LENGTH)
                || (i < ((sReplyFrameBufferIndex + 1) - JK_BMS_FRAME_TRAILER_LENGTH) && i % 16 == 0) /* no 16 byte newline in trailer*/
                ) {
            if (i != 0) {
                Serial.println();
            }
            Serial.print(F("0x"));
            if (i < 0x10) {
                Serial.print('0'); // padding with zero
            }
            Serial.print(i, HEX);
            Serial.print(F("  "));
        }

        Serial.print(F("0x"));
        if (JKReplyFrameBuffer[i] < 0x10) {
            Serial.print('0'); // padding with zero
        }
        Serial.print(JKReplyFrameBuffer[i], HEX);
        Serial.print(' ');

    }
    Serial.println();
}

#define JK_BMS_RECEIVE_OK           0
#define JK_BMS_RECEIVE_FINISHED     1
#define JK_BMS_RECEIVE_ERROR        2
/*
 * Is assumed to be called if Serial.available() is true
 * @return JK_BMS_RECEIVE_OK, if still receiving; JK_BMS_RECEIVE_FINISHED, if complete frame was successfully read
 *          JK_BMS_RECEIVE_ERROR, if frame has errors.
 * Reply starts 0.18 ms to 0.45 ms after request was received
 */
uint8_t readJK_BMSStatusFrameByte() {
    uint8_t tReceivedByte = Serial.read();
    JKReplyFrameBuffer[sReplyFrameBufferIndex] = tReceivedByte;

    /*
     * Plausi check and get length of frame
     */
    if (sReplyFrameBufferIndex == 0) {
        // start byte 1
        if (tReceivedByte != JK_FRAME_START_BYTE_0) {
            Serial.println(F("Error start frame token != 0x4E"));
            return JK_BMS_RECEIVE_ERROR;
        }
    } else if (sReplyFrameBufferIndex == 1) {
        if (tReceivedByte != JK_FRAME_START_BYTE_1) {
            // Error
            return JK_BMS_RECEIVE_ERROR;
        }

    } else if (sReplyFrameBufferIndex == 3) {
        // length of frame
        sReplyFrameLength = (JKReplyFrameBuffer[2] << 8) + tReceivedByte;

    } else if (sReplyFrameLength > MINIMAL_JK_BMS_FRAME_LENGTH && sReplyFrameBufferIndex == sReplyFrameLength - 3) {
        // Check end token 0x68
        if (tReceivedByte != JK_FRAME_END_BYTE) {
            Serial.print(F("Error end frame token 0x"));
            Serial.print(tReceivedByte, HEX);
            Serial.print(F(" at index"));
            Serial.print(sReplyFrameBufferIndex);
            Serial.print(F(" is != 0x68. sReplyFrameLength= "));
            Serial.print(sReplyFrameLength);
            Serial.print(F(" | 0x"));
            Serial.println(sReplyFrameLength, HEX);
            return JK_BMS_RECEIVE_ERROR;
        }

    } else if (sReplyFrameLength > MINIMAL_JK_BMS_FRAME_LENGTH && sReplyFrameBufferIndex == sReplyFrameLength + 1) {
        /*
         * Frame received completely, perform checksum check
         */
        uint16_t tComputedChecksum = 0;
        for (uint16_t i = 0; i < sReplyFrameLength - 2; i++) {
            tComputedChecksum = tComputedChecksum + JKReplyFrameBuffer[i];
        }
        uint16_t tReceivedChecksum = (JKReplyFrameBuffer[sReplyFrameLength] << 8) + tReceivedByte;
        if (tComputedChecksum != tReceivedChecksum) {
            Serial.print(F("Checksum error, computed checksum=0x"));
            Serial.print(tComputedChecksum, HEX);
            Serial.print(F(", received checksum=0x"));
            Serial.println(tReceivedChecksum, HEX);

            return JK_BMS_RECEIVE_ERROR;
        } else {
            return JK_BMS_RECEIVE_FINISHED;
        }
    }
    sReplyFrameBufferIndex++;
    return JK_BMS_RECEIVE_OK;
}

/*
 * Charge is positive, discharge is negative
 */
int16_t getCurrent(uint16_t aJKRAWCurrent) {
    uint16_t tCurrent = swap(aJKRAWCurrent);
    if (tCurrent == 0 || (tCurrent & 0x8000) == 0x8000) {
        // Charge
        return (tCurrent & 0x7FFF);
    }
    // discharge
    return tCurrent * -1;

}

int16_t getJKTemperature(uint16_t aJKRAWTemperature) {
    uint16_t tTemperature = swap(aJKRAWTemperature);
    if (tTemperature <= 100) {
        return tTemperature;
    }
    return 100 - tTemperature;
}

// Identity function to avoid swapping if accidentally called
uint8_t swap(uint8_t aByte) {
    return (aByte);
}

uint16_t swap(uint16_t aWordToSwapBytes) {
    return ((aWordToSwapBytes << 8) | (aWordToSwapBytes >> 8));
}

uint32_t swap(uint32_t aLongToSwapBytes) {
    return ((aLongToSwapBytes << 24) | ((aLongToSwapBytes & 0xFF00) << 8) | ((aLongToSwapBytes >> 8) & 0xFF00)
            | (aLongToSwapBytes >> 24));
}

void myPrintln(const __FlashStringHelper *aPGMString, uint8_t a8BitValue) {
    Serial.print(aPGMString);
    Serial.println(a8BitValue);
}

void myPrint(const __FlashStringHelper *aPGMString, uint8_t a8BitValue) {
    Serial.print(aPGMString);
    Serial.print(a8BitValue);
}

void myPrintln(const __FlashStringHelper *aPGMString, uint16_t a16BitValue) {
    Serial.print(aPGMString);
    Serial.println(a16BitValue);
}

void myPrint(const __FlashStringHelper *aPGMString, uint16_t a16BitValue) {
    Serial.print(aPGMString);
    Serial.print(a16BitValue);
}

void myPrintln(const __FlashStringHelper *aPGMString, int16_t a16BitValue) {
    Serial.print(aPGMString);
    Serial.println(a16BitValue);
}

void myPrint(const __FlashStringHelper *aPGMString, int16_t a16BitValue) {
    Serial.print(aPGMString);
    Serial.print(a16BitValue);
}

void myPrintlnSwap(const __FlashStringHelper *aPGMString, uint16_t a16BitValue) {
    Serial.print(aPGMString);
    Serial.println(swap(a16BitValue));
}

void myPrintlnSwap(const __FlashStringHelper *aPGMString, int16_t a16BitValue) {
    Serial.print(aPGMString);
    Serial.println((int16_t) swap((uint16_t) a16BitValue));
}

void myPrintSwap(const __FlashStringHelper *aPGMString, int16_t a16BitValue) {
    Serial.print(aPGMString);
    Serial.print((int16_t) swap((uint16_t) a16BitValue));
}

void myPrintlnSwap(const __FlashStringHelper *aPGMString, uint32_t a32BitValue) {
    Serial.print(aPGMString);
    Serial.println(swap(a32BitValue));
}

/*
 * Due to wrong SOC calculation of JKBMS that caused inverter to work incorrectly, I make this mapping of SOC based on voltage
 * As this is not good at all but it would be helpful for helping user.
 * Sets SOCPercent to the mapped one 
 */
uint16_t getMappedSOC(uint16_t AverageRefVoltage){
  uint16_t RefVoltage = 0;
  uint16_t *pMapVol;
  uint8_t *pMapSOC;
  uint8_t fakeSoc;
  uint8_t arrLength;    

  if (JKComputedData.BatteryType == 0) {
    //Serial.println(F("<<<LFP battery>>>"));
    arrLength=LFP_LEN;
    pMapVol = MapVoltLFP;
    pMapSOC = MapSOCLFP;
    
  }else if (JKComputedData.BatteryType == 1) {
    //Serial.println(F("<<<Lion battery>>>"));
    arrLength=LION_LEN;
    pMapVol = MapVoltLion;
    pMapSOC = MapSOCLion;    
  }
  if (JKComputedData.BatteryLoadCurrentFloat > 0){
    // While charging, charged voltage is about greater than opened circuit, minimum voltage should be referred, even the difference is not much
    RefVoltage = AverageRefVoltage;//MinRefVoltage;
  }else{
    RefVoltage = AverageRefVoltage;
  }
  for (uint8_t ptr = 3; ptr < arrLength; ptr++){ //Stop at 80%    
    if(RefVoltage >= *(pMapVol + ptr) && RefVoltage <= *(pMapVol + ptr +1)){
      fakeSoc = map(RefVoltage, *(pMapVol + ptr), *(pMapVol + ptr + 1), *(pMapSOC +ptr), *(pMapSOC + ptr + 1));
      /*
      Serial.println(F("Battery Type:"));
      Serial.print(JKComputedData.BatteryType,HEX);
      Serial.println(F("Voltage"));
      Serial.print(AverageRefVoltage);
      Serial.print(F("/"));
      Serial.print(*(pMapVol + ptr));
      Serial.print(F("->"));
      Serial.print(*(pMapVol + ptr + 1));
      Serial.print(F("||"));
      Serial.print(F("Fake SOC:"));
      Serial.println(fakeSoc);     
      */
      return fakeSoc;
    }
  }
  Serial.println(F("No map returned!!"));
  return 20;
}

/*
 * Convert the big endian cell voltage data from JKReplyFrameBuffer to little endian data in JKConvertedCellInfo
 * and compute minimum, maximum, delta, and average
 */
void fillJKConvertedCellInfo() {
//    uint8_t *tJKCellInfoReplyPointer = &TestJKReplyStatusFrame[11];
    uint8_t *tJKCellInfoReplyPointer = &JKReplyFrameBuffer[JK_BMS_FRAME_INDEX_OF_CELL_INFO_LENGTH];

    uint8_t tNumberOfCellInfo = (*tJKCellInfoReplyPointer++) / 3;
    JKConvertedCellInfo.ActualNumberOfCellInfoEntries = tNumberOfCellInfo;
    if (tNumberOfCellInfo > MAXIMUM_NUMBER_OF_CELLS) {
        Serial.print(F("Error: Program compiled with \"MAXIMUM_NUMBER_OF_CELLS=" STR(MAXIMUM_NUMBER_OF_CELLS) "\", but "));
        Serial.print(tNumberOfCellInfo);
        Serial.println(F(" cell info were sent"));
        return;
    }

    uint16_t tVoltage;
    uint32_t tMillivoltSum = 0;
    uint8_t tNumberOfNonNullCellInfo = 0;
    uint16_t tMinimumMillivolt = 0xFFFF;
    uint16_t tMaximumMillivolt = 0;

    for (uint8_t i = 0; i < tNumberOfCellInfo; ++i) {
        tJKCellInfoReplyPointer++;                                  // Skip Cell number
        uint8_t tHighByte = *tJKCellInfoReplyPointer++;             // Copy CellMillivolt
        tVoltage = tHighByte << 8 | *tJKCellInfoReplyPointer++;
        JKConvertedCellInfo.CellInfoStructArray[i].CellMillivolt = tVoltage;
        if (tVoltage > 0) {
            tNumberOfNonNullCellInfo++;
            tMillivoltSum += tVoltage;
            if (tMinimumMillivolt > tVoltage) {
                tMinimumMillivolt = tVoltage;
            }
            if (tMaximumMillivolt < tVoltage) {
                tMaximumMillivolt = tVoltage;
            }
        }
    }
    JKConvertedCellInfo.MinimumCellMillivolt = tMinimumMillivolt;
    JKConvertedCellInfo.MaximumCellMillivolt = tMaximumMillivolt;
    JKConvertedCellInfo.DeltaCellMillivolt = tMaximumMillivolt - tMinimumMillivolt;
    JKConvertedCellInfo.AverageCellMillivolt = tMillivoltSum / tNumberOfNonNullCellInfo;

    /*
     * Mark and count minimum and maximum cell voltages
     */
    for (uint8_t i = 0; i < tNumberOfCellInfo; ++i) {
        if (JKConvertedCellInfo.CellInfoStructArray[i].CellMillivolt == tMinimumMillivolt) {
            JKConvertedCellInfo.CellInfoStructArray[i].VoltageIsMinMaxOrBetween = VOLTAGE_IS_MINIMUM;
            if (sJKFAllReplyPointer->BMSStatus.StatusBits.BalancerActive) {
                CellMinimumArray[i]++; // count for statistics
            }
        } else if (JKConvertedCellInfo.CellInfoStructArray[i].CellMillivolt == tMaximumMillivolt) {
            JKConvertedCellInfo.CellInfoStructArray[i].VoltageIsMinMaxOrBetween = VOLTAGE_IS_MAXIMUM;
            if (sJKFAllReplyPointer->BMSStatus.StatusBits.BalancerActive) {
                CellMaximumArray[i]++;
            }
        } else {
            JKConvertedCellInfo.CellInfoStructArray[i].VoltageIsMinMaxOrBetween = VOLTAGE_IS_BETWEEN_MINIMUM_AND_MAXIMUM;
        }
    }

    /*
     * Process minimum statistics
     */
    uint32_t tCellStatisticsSum = 0;
    bool tDoDaylyScaling = false;
    for (uint8_t i = 0; i < tNumberOfCellInfo; ++i) {
        /*
         * After 43200 counts (a whole day being the minimum / maximum) we do scaling
         */
        uint16_t tCellStatisticsCount = CellMinimumArray[i];
        tCellStatisticsSum += tCellStatisticsCount;
        if (tCellStatisticsCount > (60UL * 60UL * 24UL * 1000UL / MILLISECONDS_BETWEEN_JK_DATA_FRAME_REQUESTS)) {
            /*
             * After 43200 counts (a whole day being the minimum / maximum) we do scaling
             */
            tDoDaylyScaling = true;
        }
    }

    // Here, we demand 2 minutes of balancing as minimum
    if (tCellStatisticsSum > 60) {
        for (uint8_t i = 0; i < tNumberOfCellInfo; ++i) {
            CellMinimumPercentageArray[i] = ((uint32_t) (CellMinimumArray[i] * 100UL)) / tCellStatisticsSum;
        }
    }

    if (tDoDaylyScaling) {
        /*
         * Do scaling by dividing all values by 2 resulting in an Exponential Moving Average filter for values
         */
        Serial.println(F("Do scaling of minimum counts"));
        for (uint8_t i = 0; i < tNumberOfCellInfo; ++i) {
            CellMinimumArray[i] = CellMinimumArray[i] / 2;
        }
    }

    /*
     * Process maximum statistics
     */
    tCellStatisticsSum = 0;
    tDoDaylyScaling = false;
    for (uint8_t i = 0; i < tNumberOfCellInfo; ++i) {
        /*
         * After 43200 counts (a whole day being the minimum / maximum) we do scaling
         */
        uint16_t tCellStatisticsCount = CellMaximumArray[i];
        tCellStatisticsSum += tCellStatisticsCount;
        if (tCellStatisticsCount > (60UL * 60UL * 24UL * 1000UL / MILLISECONDS_BETWEEN_JK_DATA_FRAME_REQUESTS)) {
            /*
             * After 43200 counts (a whole day being the minimum / maximum) we do scaling
             */
            tDoDaylyScaling = true;
        }
    }

    // Here, we demand 2 minutes of balancing as minimum
    if (tCellStatisticsSum > 60) {
        for (uint8_t i = 0; i < tNumberOfCellInfo; ++i) {
            CellMaximumPercentageArray[i] = ((uint32_t) (CellMaximumArray[i] * 100UL)) / tCellStatisticsSum;
        }
    }
    if (tDoDaylyScaling) {
        /*
         * Do scaling by dividing all values by 2 resulting in an Exponential Moving Average filter for values
         */
        Serial.println(F("Do scaling of maximum counts"));
        for (uint8_t i = 0; i < tNumberOfCellInfo; ++i) {
            CellMaximumArray[i] = CellMaximumArray[i] / 2;
        }
    }

#if defined(LOCAL_DEBUG)
    Serial.print(tNumberOfCellInfo);
    Serial.println(F(" cell voltages processed"));
#endif
    // During JK-BMS startup all cell voltages are sent as zero for around 6 seconds
    if (tNumberOfNonNullCellInfo < tNumberOfCellInfo && !JKComputedData.BMSIsStarting) {
        Serial.print(F("Problem: "));
        Serial.print(tNumberOfCellInfo);
        Serial.print(F(" cells configured in BMS, but only "));
        Serial.print(tNumberOfNonNullCellInfo);
        Serial.println(F(" cells seems to be connected"));
    }
}

void fillJKComputedData() {
    JKComputedData.TemperaturePowerMosFet = getJKTemperature(sJKFAllReplyPointer->TemperaturePowerMosFet);
    int16_t tMaxTemperature = JKComputedData.TemperaturePowerMosFet;

    JKComputedData.TemperatureSensor1 = getJKTemperature(sJKFAllReplyPointer->TemperatureSensor1);
    if (tMaxTemperature < JKComputedData.TemperatureSensor1) {
        tMaxTemperature = JKComputedData.TemperatureSensor1;
    }

    JKComputedData.TemperatureSensor2 = getJKTemperature(sJKFAllReplyPointer->TemperatureSensor2);
    if (tMaxTemperature < JKComputedData.TemperatureSensor2) {
        tMaxTemperature = JKComputedData.TemperatureSensor2;
    }
    JKComputedData.TemperatureMaximum = tMaxTemperature;

    JKComputedData.TotalCapacityAmpereHour = swap(sJKFAllReplyPointer->TotalCapacityAmpereHour);
    // 16 bit multiplication gives overflow at 640 Ah
    JKComputedData.RemainingCapacityAmpereHour = ((uint32_t) JKComputedData.TotalCapacityAmpereHour
            * sJKFAllReplyPointer->SOCPercent) / 100;

    // Two values which are zero during JK-BMS startup for around 16 seconds
    JKComputedData.BMSIsStarting = (sJKFAllReplyPointer->SOCPercent == 0 && sJKFAllReplyPointer->Cycles == 0);

    JKComputedData.BatteryFullVoltage10Millivolt = swap(sJKFAllReplyPointer->BatteryOvervoltageProtection10Millivolt);
    JKComputedData.BatteryVoltage10Millivolt = swap(sJKFAllReplyPointer->Battery10Millivolt);
    JKComputedData.BatteryVoltageFloat = JKComputedData.BatteryVoltage10Millivolt;
    JKComputedData.BatteryVoltageFloat /= 100;

    JKComputedData.Battery10MilliAmpere = getCurrent(sJKFAllReplyPointer->Battery10MilliAmpere);
    JKComputedData.BatteryLoadCurrentFloat = JKComputedData.Battery10MilliAmpere;
    JKComputedData.BatteryLoadCurrentFloat /= 100;

//    Serial.print("Battery10MilliAmpere=0x");
//    Serial.print(sJKFAllReplyPointer->Battery10MilliAmpere, HEX);
//    Serial.print(" Battery10MilliAmpere swapped=0x");
//    Serial.println(swap(sJKFAllReplyPointer->Battery10MilliAmpere), HEX);
//    Serial.print(" Battery10MilliAmpere=");
//    Serial.print(JKComputedData.Battery10MilliAmpere);
//    Serial.print(" BatteryLoadCurrent=");
//    Serial.println(JKComputedData.BatteryLoadCurrentFloat);

    JKComputedData.BatteryLoadPower = JKComputedData.BatteryVoltageFloat * JKComputedData.BatteryLoadCurrentFloat;

// added by Ngoc for sending cell max/min volt to Luxpower
    JKComputedData.BatteryType = sJKFAllReplyPointer->BatteryType; 
    JKComputedData.MinimumCellMillivolt = JKConvertedCellInfo.MinimumCellMillivolt;
    JKComputedData.MaximumCellMillivolt = JKConvertedCellInfo.MaximumCellMillivolt;
// For calculating Fake SOC
    JKComputedData.AverageCellMillivolt = JKConvertedCellInfo.AverageCellMillivolt;
    JKComputedData.ActualNumberOfCellInfoEntries = JKConvertedCellInfo.ActualNumberOfCellInfoEntries;
    JKComputedData.SOCPercent = getMappedSOC(JKConvertedCellInfo.AverageCellMillivolt);
        

    if (sJKFAllReplyPointer->BMSStatus.StatusBits.BalancerActive) {
        sBalancingCount++;
        sprintf_P(sBalancingTimeString, PSTR("%3uD%02uH%02uM"), (uint16_t) (sBalancingCount / (60 * 24 * 30UL)),
                (uint16_t) ((sBalancingCount / (60 * 30)) % 24), (uint16_t) (sBalancingCount / 30) % 60);
    }
}

/*
 * Print formatted cell info on Serial
 */
void printJKCellStatisticsInfo() {
    uint8_t tNumberOfCellInfo = JKConvertedCellInfo.ActualNumberOfCellInfoEntries;

    /*
     * Cell statistics
     */
    char tStringBuffer[18]; // "12=12 % |  4042, "

    Serial.println(F("Cell Minimum percentages"));
    for (uint8_t i = 0; i < tNumberOfCellInfo; ++i) {
        if (i != 0 && (i % 8) == 0) {
            Serial.println();
        }
        sprintf_P(tStringBuffer, PSTR("%2u=%2u %% |%5u, "), i + 1, CellMinimumPercentageArray[i], CellMinimumArray[i]);
        Serial.print(tStringBuffer);
    }
    Serial.println();

    Serial.println(F("Cell Maximum percentages"));
    for (uint8_t i = 0; i < tNumberOfCellInfo; ++i) {
        if (i != 0 && (i % 8) == 0) {
            Serial.println();
        }
        sprintf_P(tStringBuffer, PSTR("%2u=%2u %% |%5u, "), i + 1, CellMaximumPercentageArray[i], CellMaximumArray[i]);
        Serial.print(tStringBuffer);
    }
    Serial.println();

    Serial.println();
}
/*
 * Print formatted cell info on Serial
 */
void printJKCellInfo() {
    uint8_t tNumberOfCellInfo = JKConvertedCellInfo.ActualNumberOfCellInfoEntries;

    /*
     * Summary
     */
    Serial.print(tNumberOfCellInfo);
    myPrint(F(" Cells, Minimum="), JKConvertedCellInfo.MinimumCellMillivolt);
    myPrint(F(" mV, Maximum="), JKConvertedCellInfo.MaximumCellMillivolt);
    myPrint(F("mV, Delta="), JKConvertedCellInfo.DeltaCellMillivolt);
    myPrint(F(" mV, Average="), JKConvertedCellInfo.AverageCellMillivolt);
    Serial.println(F(" mV"));

    /*
     * Individual cell voltages
     */
    for (uint8_t i = 0; i < tNumberOfCellInfo; ++i) {
        if (i != 0 && (i % 8) == 0) {
            Serial.println();
        }
        if (i < 10) {
            Serial.print(' ');
        }
        Serial.print(i + 1);
        Serial.print(F("="));
        Serial.print(JKConvertedCellInfo.CellInfoStructArray[i].CellMillivolt);
#if defined(LOCAL_TRACE)
     Serial.print(F("|0x"));
     Serial.print(JKConvertedCellInfo.CellInfoStructArray[i].CellMillivolt, HEX);
#endif
        Serial.print(F(" mV, "));
    }
    Serial.println();

    Serial.println();
}

void printVoltageProtectionInfo() {
    JKReplyStruct *tJKFAllReply = sJKFAllReplyPointer;
    /*
     * Voltage protection
     */
    myPrint(F("Battery Overvoltage Protection[mV]="), JKComputedData.BatteryFullVoltage10Millivolt * 10);
    myPrintln(F(", Undervoltage="), swap(tJKFAllReply->BatteryUndervoltageProtection10Millivolt) * 10);
    myPrintSwap(F("Cell Overvoltage Protection[mV]="), tJKFAllReply->CellOvervoltageProtectionMillivolt);
    myPrintSwap(F(", Recovery="), tJKFAllReply->CellOvervoltageRecoveryMillivolt);
    myPrintlnSwap(F(", Delay[s]="), tJKFAllReply->CellOvervoltageDelaySeconds);
    myPrintSwap(F("Cell Undervoltage Protection[mV]="), tJKFAllReply->CellUndervoltageProtectionMillivolt);
    myPrintSwap(F(", Recovery="), tJKFAllReply->CellUndervoltageRecoveryMillivolt);
    myPrintlnSwap(F(", Delay[s]="), tJKFAllReply->CellUndervoltageDelaySeconds);
    myPrintlnSwap(F("Cell Voltage Difference Protection[mV]="), tJKFAllReply->VoltageDifferenceProtectionMillivolt);
    myPrintSwap(F("Discharging Overcurrent Protection[A]="), tJKFAllReply->DischargeOvercurrentProtectionAmpere);
    myPrintlnSwap(F(", Delay[s]="), tJKFAllReply->DischargeOvercurrentDelaySeconds);
    myPrintSwap(F("Charging Overcurrent Protection[A]="), tJKFAllReply->ChargeOvercurrentProtectionAmpere);
    myPrintlnSwap(F(", Delay[s]="), tJKFAllReply->ChargeOvercurrentDelaySeconds);
    Serial.println();
}

void printTemperatureProtectionInfo() {
    JKReplyStruct *tJKFAllReply = sJKFAllReplyPointer;
    /*
     * Temperature protection
     */
    myPrintSwap(F("Power MosFet Temperature Protection="), tJKFAllReply->PowerMosFetTemperatureProtection);
    myPrintlnSwap(F(", Recovery="), tJKFAllReply->PowerMosFetRecoveryTemperature);
    myPrintSwap(F("Sensor1 Temperature Protection="), tJKFAllReply->Sensor1TemperatureProtection);
    myPrintlnSwap(F(", Recovery="), tJKFAllReply->Sensor1RecoveryTemperature);
    myPrintlnSwap(F("Sensor1 to Sensor2 Temperature Difference Protection="), tJKFAllReply->BatteryDifferenceTemperatureProtection);
    myPrintSwap(F("Charge Overtemperature Protection="), tJKFAllReply->ChargeOvertemperatureProtection);
    myPrintlnSwap(F(", Discharge="), tJKFAllReply->DischargeOvertemperatureProtection);
    myPrintSwap(F("Charge Undertemperature Protection="), tJKFAllReply->ChargeUndertemperatureProtection);
    myPrintlnSwap(F(", Recovery="), tJKFAllReply->ChargeRecoveryUndertemperature);
    myPrintSwap(F("Discharge Undertemperature Protection="), tJKFAllReply->DischargeUndertemperatureProtection);
    myPrintlnSwap(F(", Recovery="), tJKFAllReply->DischargeRecoveryUndertemperature);
    Serial.println();
}

void printBatteryInfo() {
    JKReplyStruct *tJKFAllReply = sJKFAllReplyPointer;

    Serial.print(F("Manufacturer Date="));
    tJKFAllReply->TokenSystemWorkingMinutes = '\0'; // Set end of string token
    Serial.println(tJKFAllReply->ManufacturerDate);

    Serial.print(F("Manufacturer Id="));   // First 8 characters of the manufacturer id entered in the app field "User Private Data"
    tJKFAllReply->TokenProtocolVersionNumber = '\0'; // Set end of string token
    Serial.println(tJKFAllReply->ManufacturerId);
    Serial.print(F("Device ID String="));           // First 8 characters of ManufacturerId
    tJKFAllReply->TokenManufacturerDate = '\0';     // Set end of string token
    Serial.println(tJKFAllReply->DeviceIdString);

    myPrintln(F("Device Address="), tJKFAllReply->BoardAddress);
    myPrint(F("Total Battery Capacity[Ah]="), JKComputedData.TotalCapacityAmpereHour); // 0xAA
    myPrintln(F(", Low Capacity Alarm Percent="), tJKFAllReply->LowCapacityAlarmPercent); // 0xB1
    myPrintlnSwap(F("Charging Cycles="), tJKFAllReply->Cycles);
    myPrintlnSwap(F("Total Charging Cycle Capacity="), tJKFAllReply->TotalBatteryCycleCapacity);
    myPrintSwap(F("# Battery Cells="), tJKFAllReply->NumberOfBatteryCells); // 0x8A Total number of battery strings
    myPrintln(F(", Cell Count="), tJKFAllReply->BatteryCellCount); // 0xA9 Battery string count settings
    Serial.println();
}

void printBMSInfo() {
    JKReplyStruct *tJKFAllReply = sJKFAllReplyPointer;

    myPrintln(F("Protocol Version Number="), tJKFAllReply->ProtocolVersionNumber);
    Serial.print(F("Software Version Number="));
    tJKFAllReply->TokenStartCurrentCalibration = '\0'; // set end of string token
    Serial.println(tJKFAllReply->SoftwareVersionNumber);
    Serial.print(F("Modify Parameter Password="));
    tJKFAllReply->TokenDedicatedChargerSwitchState = '\0'; // set end of string token
    Serial.println(tJKFAllReply->ModifyParameterPassword);

    myPrintln(F("# External Temperature Sensors="), tJKFAllReply->NumberOfTemperatureSensors); // 0x86

    Serial.println();
}

void printMiscellaneousInfo() {
    JKReplyStruct *tJKFAllReply = sJKFAllReplyPointer;

    myPrintlnSwap(F("Balance Starting Cell Voltage=[mV]"), tJKFAllReply->BalancingStartMillivolt);
    myPrintlnSwap(F("Balance Triggering Voltage Difference[mV]="), tJKFAllReply->BalancingStartDifferentialMillivolt);
    Serial.println();
    myPrintlnSwap(F("Current Calibration[mA]="), tJKFAllReply->CurrentCalibrationMilliampere);
    myPrintlnSwap(F("Sleep Wait Time[s]="), tJKFAllReply->SleepWaitingTimeSeconds);
    Serial.println();
    myPrintln(F("Dedicated Charge Switch Active="), tJKFAllReply->DedicatedChargerSwitchIsActive);
    myPrintln(F("Start Current Calibration State="), tJKFAllReply->StartCurrentCalibration);
    myPrintlnSwap(F("Battery Actual Capacity[Ah]="), tJKFAllReply->ActualBatteryCapacityAmpereHour);
    Serial.println();
}

/*
 * Token 0x8B. Prints info only if errors existent and changed from last value
 * Stores error string for LCD in sErrorStringForLCD
 */
void handleAndPrintAlarmInfo() {
    JKReplyStruct *tJKFAllReply = sJKFAllReplyPointer;

    /*
     * Do it only once per change
     */
    if (tJKFAllReply->AlarmUnion.AlarmsAsWord != lastJKReply.AlarmUnion.AlarmsAsWord) {
        // ChargeOvervoltageAlarm is displayed separately
        if (!tJKFAllReply->AlarmUnion.AlarmBits.ChargeOvervoltageAlarm) {
            sErrorStatusJustChanged = true; // This forces a switch to Overview page
            sErrorStatusIsError = true; //  This forces the beep
        }

        if (tJKFAllReply->AlarmUnion.AlarmsAsWord == 0) {
            sErrorStringForLCD = NULL; // reset error string
            sErrorStatusIsError = false;
            Serial.println(F("All alarms are cleared"));
        } else {
            uint16_t tAlarms = swap(tJKFAllReply->AlarmUnion.AlarmsAsWord);
            Serial.println(F("*** ALARM FLAGS ***"));
            Serial.print(F("Alarm bits=0x"));
            Serial.print(tAlarms, HEX);
            Serial.print(F(" | 0b"));
            Serial.println(tAlarms, BIN);

            uint16_t tAlarmMask = 1;
            for (uint_fast8_t i = 0; i < NUMBER_OF_DEFINED_ALARM_BITS; ++i) {
                if (tAlarms & tAlarmMask) {
                    Serial.print(F("Alarm bit=0b"));
                    Serial.print(tAlarmMask, BIN);
                    Serial.print(F(" -> "));
                    const char *tErrorStringPtr = (char*) (pgm_read_word(&JK_BMSErrorStringsArray[i]));
                    sErrorStringForLCD = tErrorStringPtr;
                    Serial.println(reinterpret_cast<const __FlashStringHelper*>(tErrorStringPtr));
                }
                tAlarmMask <<= 1;
            }
            Serial.println();
        }
    }
}

void printEnabledState(bool aIsEnabled) {
    if (aIsEnabled) {
        Serial.print(F(" enabled"));
    } else {
        Serial.print(F(" disabled"));
    }
}

void printActiveState(bool aIsActive) {
    if (!aIsActive) {
        Serial.print(F(" not"));
    }
    Serial.print(F(" active"));
}

void printJKStaticInfo() {

    Serial.println(F("*** BMS INFO ***"));
    printBMSInfo();

    Serial.println(F("*** BATTERY INFO ***"));
    printBatteryInfo();

    Serial.println(F("*** VOLTAGE PROTECTION INFO ***"));
    printVoltageProtectionInfo();

    Serial.println(F("*** TEMPERATURE PROTECTION INFO ***"));
    printTemperatureProtectionInfo();

    Serial.println(F("*** MISC INFO ***"));
    printMiscellaneousInfo();
}

void computeUpTimeString() {
    if (sJKFAllReplyPointer->SystemWorkingMinutes != lastJKReply.SystemWorkingMinutes) {
        sUpTimeStringMinuteHasChanged = true;

        uint32_t tSystemWorkingMinutes = swap(sJKFAllReplyPointer->SystemWorkingMinutes);
// 1 kByte for sprintf  creates string "1234D23H12M"
        sprintf_P(sUpTimeString, PSTR("%4uD%02uH%02uM"), (uint16_t) (tSystemWorkingMinutes / (60 * 24)),
                (uint16_t) ((tSystemWorkingMinutes / 60) % 24), (uint16_t) tSystemWorkingMinutes % 60);
        if (sLastUpTimeTenthOfMinuteCharacter != sUpTimeString[8]) {
            sLastUpTimeTenthOfMinuteCharacter = sUpTimeString[8];
            sUpTimeStringTenthOfMinuteHasChanged = true;
        }
    }
}

/*
 * Print received data
 * Use converted cell voltage info from JKConvertedCellInfo
 * All other data are used unconverted and are therefore printed by swap() functions.
 */
void printJKDynamicInfo() {
    JKReplyStruct *tJKFAllReply = sJKFAllReplyPointer;

    /*
     * Print it every ten minutes
     */
//    if (sUpTimeStringMinuteHasChanged) {
//        sUpTimeStringMinuteHasChanged = false;
    if (sUpTimeStringTenthOfMinuteHasChanged) {
        sUpTimeStringTenthOfMinuteHasChanged = false;

        Serial.print(F("Total Runtime Minutes="));
        Serial.print(swap(sJKFAllReplyPointer->SystemWorkingMinutes));
        Serial.print(F(" -> "));
        Serial.println(sUpTimeString);

        Serial.println(F("*** CELL INFO ***"));
        printJKCellInfo();

        if (sBalancingCount > MINIMUM_BALANCING_COUNT_FOR_DISPLAY) {
            Serial.println(F("*** CELL STATISTICS ***"));
            Serial.print(F("Total balancing time="));

            Serial.print(sBalancingCount * (MILLISECONDS_BETWEEN_JK_DATA_FRAME_REQUESTS / 1000));
            Serial.print(F(" s -> "));
            Serial.print(sBalancingTimeString);
            // Append seconds
            char tString[4]; // "03S" is 3 bytes long
            sprintf_P(tString, PSTR("%02uS"), (uint16_t) (sBalancingCount % 30) * 2);
            Serial.println(tString);
            printJKCellStatisticsInfo();
        }

#if !defined(SUPPRESS_LIFEPO4_PLAUSI_WARNING)
        if (swap(tJKFAllReply->CellOvervoltageProtectionMillivolt) > 3450) {
            // https://www.evworks.com.au/page/technical-information/lifepo4-care-guide-looking-after-your-lithium-batt/
            Serial.print(F("Warning: CellOvervoltageProtectionMillivolt value "));
            Serial.print(swap(tJKFAllReply->CellOvervoltageProtectionMillivolt));
            Serial.println(
                    F(" mV > 3450 mV is not recommended for LiFePO4 chemistry. There is less than 1% extra capacity above 3.5V."));
        }
        if (swap(tJKFAllReply->CellUndervoltageProtectionMillivolt) < 3000) {
            // https://batteryfinds.com/lifepo4-voltage-chart-3-2v-12v-24v-48v/
            Serial.print(F("Warning: CellUndervoltageProtectionMillivolt value "));
            Serial.print(swap(tJKFAllReply->CellUndervoltageProtectionMillivolt));
            Serial.println(F(" mV < 3000 mV is not recommended for LiFePO4 chemistry."));
            Serial.println(F("There is less than 10% capacity below 3.0V and 20% capacity below 3.2V."));
        }
#endif
    }

    /*
     * Temperatures
     * Print only if temperature changed more than 1 degree
     */
#if defined(LOCAL_DEBUG)
    Serial.print(F("TokenTemperaturePowerMosFet=0x"));
    Serial.println(sJKFAllReplyPointer->TokenTemperaturePowerMosFet, HEX);
#endif
    if (abs(JKComputedData.TemperaturePowerMosFet - lastJKComputedData.TemperaturePowerMosFet) > 2
            || abs(JKComputedData.TemperatureSensor1 - lastJKComputedData.TemperatureSensor1) > 2
            || abs(JKComputedData.TemperatureSensor2 - lastJKComputedData.TemperatureSensor2) > 2) {
        myPrint(F("Temperature: Power MosFet="), JKComputedData.TemperaturePowerMosFet);
        myPrint(F(", Sensor 1="), JKComputedData.TemperatureSensor1);
        myPrintln(F(", Sensor 2="), JKComputedData.TemperatureSensor2);
    }

    /*
     * SOC
     */
    if (tJKFAllReply->SOCPercent != lastJKReply.SOCPercent
            || JKComputedData.RemainingCapacityAmpereHour != lastJKComputedData.RemainingCapacityAmpereHour) {
        myPrint(F("SOC[%]="), tJKFAllReply->SOCPercent);
        myPrintln(F(" -> Remaining Capacity[Ah]="), JKComputedData.RemainingCapacityAmpereHour);
    }

    /*
     * Charge and Discharge values
     */
    if (abs(JKComputedData.BatteryVoltageFloat - lastJKComputedData.BatteryVoltageFloat) > 0.02 // Meant is 0.02 but use 15 to avoid strange floating point effects
    || abs(JKComputedData.BatteryLoadPower - lastJKComputedData.BatteryLoadPower) >= 20) {
        Serial.print(F("Battery Voltage[V]="));
        Serial.print(JKComputedData.BatteryVoltageFloat, 2);
        Serial.print(F(", Current[A]="));
        Serial.print(JKComputedData.BatteryLoadCurrentFloat, 2);
        myPrint(F(", Power[W]="), JKComputedData.BatteryLoadPower);
        Serial.print(F(", Difference to full[V]="));
        float tBatteryToFullDifference = JKComputedData.BatteryFullVoltage10Millivolt - JKComputedData.BatteryVoltage10Millivolt;
        Serial.println(tBatteryToFullDifference / 100.0, 1);
    }

    /*
     * Charge, Discharge and Balancer flags
     */
    if (tJKFAllReply->BMSStatus.StatusBits.ChargeMosFetActive != lastJKReply.BMSStatus.StatusBits.ChargeMosFetActive
            || tJKFAllReply->BMSStatus.StatusBits.DischargeMosFetActive != lastJKReply.BMSStatus.StatusBits.DischargeMosFetActive
            || tJKFAllReply->BMSStatus.StatusBits.BalancerActive != lastJKReply.BMSStatus.StatusBits.BalancerActive) {
        Serial.print(F("Charging MosFet"));
        printEnabledState(tJKFAllReply->ChargeIsEnabled);
        Serial.print(',');
        printActiveState(tJKFAllReply->BMSStatus.StatusBits.ChargeMosFetActive);
        Serial.print(F(" | Discharging MosFet"));
        printEnabledState(tJKFAllReply->DischargeIsEnabled);
        Serial.print(',');
        printActiveState(tJKFAllReply->BMSStatus.StatusBits.DischargeMosFetActive);
        Serial.print(F(" | Balancing"));
        printEnabledState(tJKFAllReply->BalancingIsEnabled);
        Serial.print(',');
        printActiveState(tJKFAllReply->BMSStatus.StatusBits.BalancerActive);
        Serial.println(); // printActiveState does no println()
    }
}
#if defined(LOCAL_DEBUG)
#undef LOCAL_DEBUG
#endif
#endif // _JK_BMS_HPP
