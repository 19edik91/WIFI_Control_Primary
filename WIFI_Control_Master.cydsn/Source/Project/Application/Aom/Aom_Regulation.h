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

#ifndef _AOM_REGULATION_H_
#define _AOM_REGULATION_H_

#ifdef __cplusplus
extern "C"
{
#endif   

#include "BaseTypes.h"
#include "Aom.h"

const tRegulationValues* Aom_Regulation_GetRegulationValuesPointer(void);


void Aom_Regulation_ChangeValueRelative(s8 scChangeValue, u8 ucOutputIdx);
void Aom_Regulation_ChangeValueAbsolute(u8 ucNewValue, u8 ucOutputIdx);
void Aom_Regulation_CheckRequestValues(u8 ucBrightnessValue, bool bLedStatus, bool bInitMenuActive, u8 ucOutputIdx);
bool Aom_Regulation_CompareCustomValue(u8 ucBrightnessValue, bool bLedStatus, u8 ucOutputIdx);
void Aom_Regulation_SetCalculatedVoltageValue(tRegulationValues* psRegulationValues);
void Aom_Regulation_GetRegulationValues(tRegulationValues* psRegulationValues);
              
void Aom_Regulation_SetMinSystemSettings(u16 uiCurrentAdc, u16 uiVoltageAdc, u8 ucOutputIdx);
void Aom_Regulation_SetMaxSystemSettings(u16 uiCurrentAdc, u16 uiVoltageAdc, u8 ucOutputIdx);
              
void Aom_Regulation_SetAutomaticModeStatus(bool bAutomaticModeStatus);
void Aom_Regulation_SetNightModeStatus(bool bNightModeOnOff);
void Aom_Regulation_SetMotionDectionStatus(bool bMotionDetectionOnOff, u8 ucBurnTime);

#ifdef __cplusplus
}
#endif    

#endif //_AOM_REGULATION_H_