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

#ifndef _MEASURE_VOLTAGE_H_
#define _MEASURE_VOLTAGE_H_

#ifdef __cplusplus
extern "C"
{
#endif   


#include "BaseTypes.h"

#define VOLTAGE_DEFAULT_LOWER_LIMIT        0u    //0V
#define VOLTAGE_DEFAULT_UPPER_LIMIT    24000u    //24V


u32 Measure_Voltage_CalculateVoltageFromPercent(u8 ucPercentValue, u8 ucOutputIdx, bool bUseDefault);
u16 Measure_Voltage_CalculateAdcValue(u32 ulVoltage);
u32 Measure_Voltage_CalculateVoltageValue(u16 uiAdcValue);
void Measure_Voltage_SetNewLimits(u32 ulMinLimit, u32 ulMaxLimit, u8 ucOutputIdx);
u32 Measure_Voltage_GetSystemVoltage(void);
u32 Measure_Voltage_CalculateSystemVoltage(u16 uiAdcValue);
void Measure_Voltage_SetSystemVoltage(u32 ulSystemVoltage);

#ifdef __cplusplus
}
#endif    

#endif //_MEASURE_VOLTAGE_H_
