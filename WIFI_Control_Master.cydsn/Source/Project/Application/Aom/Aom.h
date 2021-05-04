/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#ifndef _AOM_H_
#define _AOM_H_

#ifdef __cplusplus
extern "C"
{
#endif   

#include "BaseTypes.h"
#include "Project_Config.h"

typedef struct
{
    u16 uiReqVoltageAdc;
    u16 uiIsVoltageAdc;    
    u16 uiIsCurrentAdc;
    u8  ucPercentValue;
    bool bStatus;
}tLedValue;

typedef struct
{
    u8 ucHourSet;
    u8 ucMinSet;
    u8 ucHourClear;
    u8 ucMinClear;
}tsTimeFormat;

typedef struct
{
    u32 ulTicks;
    u8 ucHours;
    u8 ucMinutes;
}tsCurrentTime;

typedef struct
{
    tsTimeFormat sTimer[USER_TIMER_AMOUNT];
    u8 ucSetTimerBinary;
    u8 ucBurningTime;
    bool bAutomaticModeActive;
    bool bMotionDetectOnOff;
}tsUserTimeSettings;

typedef struct
{
    tLedValue sLedValue[DRIVE_OUTPUTS];
    tsUserTimeSettings sUserTimerSettings;
    u16  uiNtcAdcValue;
    bool bNightModeOnOff;
}tRegulationValues;

typedef struct
{
    s32  slBurningTimeMs;       //Variable which is decremented in the automatic mode
    bool bInNightModeTimeSlot;
    bool bInUserTimerSlot;
    bool bMotionDetected;
}tsAutomaticModeValues;

typedef struct
{
    u16 uiMinAdcCurrent;
    u16 uiMaxAdcCurrent;
    u16 uiMinAdcVoltage;
    u16 uiMaxAdcVoltage;
    u16 uiMinCompVal;
    u16 uiMaxCompVal;    
}tsSystemSettings;

typedef struct
{
    struct Output
    {
        u32 ulMilliVolt;
        u16 uiMilliAmp;
    }sOutput[DRIVE_OUTPUTS];
    u16 uiTemp;
}tsConvertedMeasurement;

typedef enum
{
    eMeasureChVoltage,
    eMeasureChCurrent,
    eMeasureChTemp,
    eMeasureChInvalid
}teMeasureType;



tsSystemSettings*       Aom_GetSystemSettingsEntry(u8 ucDriveOutputIdx);
tRegulationValues*      Aom_GetRegulationSettings(void);
tLedValue*              Aom_GetOutputsSettingsEntry(u8 ucDriveOutputIdx);
tsCurrentTime*          Aom_GetCurrentTimePointer(void);
tsAutomaticModeValues*  Aom_GetAutomaticModeSettingsPointer(void);
tsConvertedMeasurement* Aom_GetConvertedMeasurementPointer(void);
#ifdef __cplusplus
}
#endif    

#endif //_AOM_H_