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

#ifndef _MEASURE_H_
#define _MEASURE_H_

#ifdef __cplusplus
extern "C"
{
#endif   


#include "BaseTypes.h"
#include "Aom.h"

#define BUFFER_LENGTH            2

//Use of X-Macros for defining AD-MUX-Channels
/*      Channel name   |  MeasureValues |   Measure_Type        |   Output index    */
#define AD_MUX_LIST \
   A_CH(   eA_CH_0     ,    {0}         ,    eMeasureChVoltage  ,       0x00   )\
   A_CH(   eA_CH_1     ,    {0}         ,    eMeasureChVoltage  ,       0x01   )\
   A_CH(   eA_CH_2     ,    {0}         ,    eMeasureChVoltage  ,       0x02   )\
   A_CH(   eA_CH_3     ,    {0}         ,    eMeasureChCurrent  ,       0x00   )\
   A_CH(   eA_CH_4     ,    {0}         ,    eMeasureChCurrent  ,       0x01   )\
   A_CH(   eA_CH_5     ,    {0}         ,    eMeasureChCurrent  ,       0x02   )\
   A_CH(   eA_CH_6     ,    {0}         ,    eMeasureChTemp     ,       0x03   )\
   A_CH(   eA_CH_INV   ,    {0}         ,    eMeasureChInvalid  ,       0xFF   )


// Generate an enum list for the error list
typedef enum 
{
    #define A_CH(ChannelName, MeasureVal, MeasureType, OutputIndex) ChannelName,
        AD_MUX_LIST
    #undef A_CH
}teAdMuxList;

typedef struct
{
    s16  siAdcBuffer[BUFFER_LENGTH];
    s16  siAdcSum;
    s16  siActualVoltageValue;
    u8   ucBufferIndex;
}tsMeasureValues;


// Create typedef structure for MUX list
typedef struct
{
    tsMeasureValues       sMeasureValue;
    const teMeasureType   eMeasureType;
    const u8              ucOutputIndex;    
}tsAdMuxList;


void DR_Measure_Init(void);
void DR_Measure_Tick(void);
u32  DR_Measure_CalculateVoltageFromPercent(u8 ucPercentValue, bool bUseDefaultLimit, u8 ucOutputIdx);
u16  DR_Measure_CalculateAdcValue(u32 ulVoltage, u16 uiCurrent);
u32  DR_Measure_CalculateVoltageValue(u16 uiAdcValue);
u16  DR_Measure_CalculateCurrentValue(u16 uiAdcValue);
u16  DR_Measure_CalculateTemperatureValue(u16 uiAdcValue);
void DR_Measure_Start(void);
void DR_Measure_Stop(void);
void DR_Measure_SetNewVoltageLimits(u32 ulMinLimit, u32 ulMaxLimit, u8 ucOutputIdx);
u32  DR_Measure_GetSystemVoltage(void);
void DR_Measure_SetSystemVoltage(u32 ulSystemVoltage);
u16  DR_Measure_GetAveragedAdcValue(teAdMuxList eAdcChannel);
#ifdef __cplusplus
}
#endif    

#endif //_MEASURE_H_
