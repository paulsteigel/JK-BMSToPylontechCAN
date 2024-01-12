/*
 * Pylontech_CAN.hpp
 *
 * Functions to fill and send CAN data defined in Pylontech_CAN.h
 *
 * Useful links:
 * https://www.setfirelabs.com/green-energy/pylontech-can-reading-can-replication
 * https://www.skpang.co.uk/products/teensy-4-1-triple-can-board-with-240x240-ips-lcd
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

#ifndef _PYLONTECH_CAN_HPP
#define _PYLONTECH_CAN_HPP

#include <Arduino.h>

#if defined(DEBUG)
#define LOCAL_DEBUG
#else
//#define LOCAL_DEBUG // This enables debug output only for this file - only for development
#endif

#include "MCP2515_TX.h" // my reduced driver
#include "Pylontech_CAN.h"

struct PylontechCANBatteryLimitsFrameStruct PylontechCANBatteryLimitsFrame;
struct PylontechCANSohSocFrameStruct PylontechCANSohSocFrame;
struct PylontechCANCurrentValuesFrameStruct PylontechCANCurrentValuesFrame;
struct PylontechCANBatteryRequesFrameStruct PylontechCANBatteryRequestFrame;
struct PylontechCANErrorsWarningsFrameStruct PylontechCANErrorsWarningsFrame;
struct PylontechCANSpecificationsFrameStruct PylontechCANSpecificationsFrame;
// Frames with fixed data
struct PylontechCANManufacturerFrameStruct PylontechCANManufacturerFrame;
struct PylontechCANAliveFrameStruct PylontechCANAliveFrame;

void fillAllCANData(struct JKReplyStruct *aJKFAllReply) {
    PylontechCANBatteryLimitsFrame.fillFrame(aJKFAllReply);
    PylontechCANSohSocFrame.fillFrame(aJKFAllReply);
    PylontechCANBatteryRequestFrame.fillFrame(aJKFAllReply);
    PylontechCANErrorsWarningsFrame.fillFrame(aJKFAllReply);
    PylontechCANCurrentValuesFrame.fillFrame(aJKFAllReply);
    PylontechCANSpecificationsFrame.fillFrame(aJKFAllReply);
}

void sendPylontechCANFrame(struct PylontechCANFrameStruct *aPylontechCANFrame) {
    sendCANMessage(aPylontechCANFrame->PylontechCANFrameInfo.CANId, aPylontechCANFrame->PylontechCANFrameInfo.FrameLength,
            aPylontechCANFrame->FrameData.UBytes);
}

/*
 * Called in case of BMS communication timeout
 */
void modifyAllCanDataToInactive() {
    PylontechCANCurrentValuesFrame.FrameData.Current100Milliampere = 0;
    // Clear all requests in case of timeout / BMS switched off, before sending
    reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANBatteryRequestFrame)->FrameData.UWords[0] = 0;
    reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANErrorsWarningsFrame)->FrameData.ULong.LowLong = 0;
}

void printPylontechCANFrame(struct PylontechCANFrameStruct *aPylontechCANFrame) {
    Serial.print(F("CANId=0x"));
    Serial.print(aPylontechCANFrame->PylontechCANFrameInfo.CANId, HEX);
    Serial.print(F(", FrameLength="));
    Serial.print(aPylontechCANFrame->PylontechCANFrameInfo.FrameLength);
    Serial.print(F(", Data=0x"));
    for (uint_fast8_t i = 0; i < aPylontechCANFrame->PylontechCANFrameInfo.FrameLength; ++i) {
        if (i != 0) {
            Serial.print(F(", 0x"));
        }
        Serial.print(aPylontechCANFrame->FrameData.UBytes[i], HEX);
    }
    Serial.println();
}

/*
 * Inverter reply every second: 0x305: 00-00-00-00-00-00-00-00
 * If no CAN receiver is attached, every frame is retransmitted once, because of the NACK error.
 * Or use CAN.writeRegister(REG_CANCTRL, 0x08); // One Shot Mode
 */
void sendPylontechAllCANFrames(bool aDebugModeActive) {
    if (aDebugModeActive) {
        ControlChargeScheme(); //modify charge scheme
        printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANBatteryLimitsFrame));
        printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANSohSocFrame));
        printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANCurrentValuesFrame));
        printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANManufacturerFrame));
        printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANBatteryRequestFrame));
        printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANAliveFrame));
        printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANErrorsWarningsFrame));
#if defined(SMA_EXTENSIONS)
        printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANSpecificationsFrame));
#endif
#if defined(LUXPOWER_EXTENSIONS)
    //sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANBatInfoFrame));
    //sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANCellInfoFrame));
    //sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANCellMinVoltageNameFrame));
    //sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANCellMaxVoltageNameFrame));
    sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANLuxpowerCapacityFrame));
    sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANSMACapacityFrame));
    sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANCellInfoFrame));
    //sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANBatteryInfoFrame));
#endif
    }
    ControlChargeScheme(); //modify charge scheme
    sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANBatteryLimitsFrame));
    sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANSohSocFrame));
    sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANCurrentValuesFrame));
    sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANManufacturerFrame));
    sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANBatteryRequestFrame));
    sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANAliveFrame));
    sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANErrorsWarningsFrame));
#if defined(SMA_EXTENSIONS)
    sendPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANSpecificationsFrame));
#endif
#if defined(LUXPOWER_EXTENSIONS)        
    //printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANBatInfoFrame));
    //printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANCellInfoFrame));
    //printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANCellMinVoltageNameFrame));
    //printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANCellMaxVoltageNameFrame));
    printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANLuxpowerCapacityFrame));
    printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANSMACapacityFrame));  
    printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANCellInfoFrame));  
    //printPylontechCANFrame(reinterpret_cast<struct PylontechCANFrameStruct*>(&PylontechCANBatteryInfoFrame));     
#endif  
}

/* This part is for controlling the charge scheme 
 * following CCCV method
 * 1.Warm-up in about 40 minutes; 2. Constant current till certain voltage or SOC;
 * 3.Constant Voltage with reduction of charging curent till SOC 100% or 3.40 V for LFP, 4.15 V for Lion (about 3h)
 * Warmup 40 keep 160  cool-down 45 minutes
*/
 
void ControlChargeScheme(){
  uint8_t ChargeStatusRef = 0;
  uint16_t Local_Charge_Current_100_milliAmp;
  
  if (JKComputedData.BatteryLoadCurrentFloat > 0) {
    ChargeStatusRef = ReachChargeLimit();
    if (StartChargeTime == 0) {
      if (ChargeStatusRef >= 1 && ChargeTryEffort > 2){
          // Too much for now, request charging off        
          ChargeTryEffort++;          
          PylontechCANBatteryRequestFrame.FrameData.ChargeEnable = 0;
          Serial.println(F("Setting charge to OFF:")); 
          return;
      }else{          
          PylontechCANBatteryRequestFrame.FrameData.ChargeEnable = sJKFAllReplyPointer->BMSStatus.StatusBits.DischargeMosFetActive;
      }
      StartChargeTime = millis(); // Store starting time for charge
      // Get the proper charging current: either BMS limit or using 0.3C
      Computed_Current_limits_100mA = min(swap(sJKFAllReplyPointer->ChargeOvercurrentProtectionAmpere) * 10, 
      JKComputedData.TotalCapacityAmpereHour * CHARGING_CURRENT_PER_CAPACITY);
      Serial.print(F("Charging check: >Selected Current:")); Serial.println(Computed_Current_limits_100mA);   
    }
  } else {
    resetCharge();
    return;
  }
  
  // Get into current charging phase  
  if (StartChargeTime == 0) return; // No charging detected
  // Allow this to be triggered once
  if (ChargePhase == 0) {
    if ((millis() - StartChargeTime) < MOMENTARY_CHARGE_DURATION) return;
    // Charge started in more than MOMENTARY_CHARGE_DURATION, charging started, get into phase 1
    //MinuteCount = MOMENTARY_CHARGE_DURATION / (CHARGE_RATIO * 1000L); // Marking the counter for warming up charge
    MinuteCount = map(JKComputedData.BatteryLoadCurrentFloat * 10, 1, Computed_Current_limits_100mA, 0, CHARGE_PHASE_1) + 1;
    ChargePhase = 1;
    Serial.println(F("Enter phase 1:"));
    Serial.print(MinuteCount);
  }

  // Check if end of phase 1, move to phase 2 or next 
  if (ChargePhase == 1) {
    if ((millis() - StartChargeTime) >= ((CHARGE_PHASE_1 + 1) * CHARGE_RATIO * 1000L)) {
      ChargePhase = 2;
      MinuteCount = 0; //reset the minute counter to enter phase 2
      Serial.print(F("Enter phase 2:"));Serial.println(MinuteCount);
    }    
  } else if (ChargePhase == 2) {
    //ChargeStatusRef = ReachChargeLimit();          
    if (ChargeStatusRef == 1) {      
      ChargePhase = 3;  
      MinuteCount = 0; //reset the minute counter during phase 2
    } else if (ChargeStatusRef == 2) {
      // reducing current by 10%
      Charge_Current_100_milliAmp = Charge_Current_100_milliAmp * 0.98;
    }
  }
  
  if ((millis() - LastCheckTime) < CHARGE_STATUS_REFRESH_INTERVAL) return;
  LastCheckTime = millis(); //reset the counter for resuming the check
  switch (ChargePhase) {
  case 1:        
    // Linear mapping of charging current until reaching 0.3C
    Local_Charge_Current_100_milliAmp = map(MinuteCount, 0, CHARGE_PHASE_1, 1, Computed_Current_limits_100mA);    
    Charge_Current_100_milliAmp = (Local_Charge_Current_100_milliAmp > Computed_Current_limits_100mA) ? Computed_Current_limits_100mA : Local_Charge_Current_100_milliAmp;
    PylontechCANBatteryLimitsFrame.FrameData.BatteryChargeCurrentLimit100Milliampere = Charge_Current_100_milliAmp;
    MinuteCount ++;
    Serial.print(F("Charging phase 1: minute count::"));Serial.println(MinuteCount);
    Serial.print(F("Applied charged current::"));
    Serial.print(PylontechCANBatteryLimitsFrame.FrameData.BatteryChargeCurrentLimit100Milliampere);
    Serial.print(F("/"));Serial.println(Computed_Current_limits_100mA); 

    break;
  case 2: 
    // in this phase, do nothing to current at all
    Serial.print(F("Charging phase 2: Minute count::"));Serial.println(MinuteCount);
    Serial.print(F("Applied charged current::"));
    Serial.print(Charge_Current_100_milliAmp);
    Serial.print(F("/"));Serial.println(Computed_Current_limits_100mA);
    MinuteCount ++;    
    break;
  case 3: 
    //Keep charging till full or when the Inverter stop charging, need a new routine for this charging   
    Serial.print(F("Charging phase 3: Minute count::"));Serial.println(MinuteCount);
    MinuteCount ++;     
    if (ChargeStatusRef != 2) Charge_Current_100_milliAmp = map(MinuteCount, 1, CHARGE_PHASE_3, Computed_Current_limits_100mA, 0);
    PylontechCANBatteryLimitsFrame.FrameData.BatteryChargeCurrentLimit100Milliampere = Charge_Current_100_milliAmp;        
    Serial.print(F("Applied charged current::"));
    Serial.print(PylontechCANBatteryLimitsFrame.FrameData.BatteryChargeCurrentLimit100Milliampere);
    Serial.print(F("/"));Serial.println(Computed_Current_limits_100mA); 

    if (Charge_Current_100_milliAmp == 0) {
      // tell the inverter to stop charging or keep it to maintain the charge
      Serial.print(F("End of charge"));
      resetCharge();
    }
    break;
  default:
    // statements
    break;
  }      
}

void resetCharge(){
  Serial.println(F("Reset charging parameters"));
  StartChargeTime = 0; // reset charge starting time
  LastCheckTime = 0;
  MinuteCount = 0;
  ChargePhase = 0;
  ChargeTryEffort = 0;    
  // recover the charging limits
  if (PylontechCANBatteryLimitsFrame.FrameData.BatteryChargeCurrentLimit100Milliampere != swap(sJKFAllReplyPointer->ChargeOvercurrentProtectionAmpere) * 10) {
    PylontechCANBatteryLimitsFrame.FrameData.BatteryChargeCurrentLimit100Milliampere = swap(sJKFAllReplyPointer->ChargeOvercurrentProtectionAmpere) * 10;
  }
}

uint8_t ReachChargeLimit(){ 
  /* Status return: 
   *  0: nothing; 
   *  1: stop phase; 
   *  2: reducing current 10%; 
  */
  //will do this check every 5 minutes
  uint16_t Charge_MilliVolt_limit = 0;  
  if ((millis() - LastCheckTime) < CHARGE_STATUS_REFRESH_INTERVAL) return 0;
  // first check over voltage
  if (JKComputedData.BatteryType == 0) {//LFP battery  
    Charge_MilliVolt_limit = 3450;
  } else if (JKComputedData.BatteryType == 1){ //Lithium ion       
    Charge_MilliVolt_limit = 4200;  
  }
  // check SOC First
  return (sJKFAllReplyPointer->SOCPercent >= MAX_SOC_BULK_CHARGE_THRESHOLD_PERCENT)? 1 : 0;   
  Serial.print(F("Battery type:")); Serial.println(JKComputedData.BatteryType);
  if ((JKComputedData.MaximumCellMillivolt * 1.02) > Charge_MilliVolt_limit) {
    Serial.print(F("Status check::")); Serial.println((JKComputedData.MaximumCellMillivolt * 1.02) - Charge_MilliVolt_limit);    
    return 2;  
  }       
}

#if defined(LOCAL_DEBUG)
#undef LOCAL_DEBUG
#endif
#endif // _PYLONTECH_CAN_HPP
