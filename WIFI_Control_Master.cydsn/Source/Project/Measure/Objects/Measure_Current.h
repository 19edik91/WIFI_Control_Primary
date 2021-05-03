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

#ifndef _MEASURE_CURRENT_H_
#define _MEASURE_CURRENT_H_

#ifdef __cplusplus
extern "C"
{
#endif   


#include "BaseTypes.h"

u16 Measure_Current_CalculateCurrentValue(u16 uiAdcValue);
u16 Measure_Current_CalculateAdcValue(u16 uiMilliCurrent);


#ifdef __cplusplus
}
#endif    

#endif //_MEASURE_CURRENT_H_
