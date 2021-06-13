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
#ifndef _HAL_IO_CONFIG_H_
#define _HAL_IO_CONFIG_H_
    
#ifdef __cplusplus
extern "C"
{
#endif   

#include "BaseTypes.h"
#include "project.h"

#include "DR_Regulation.h"

/****************************************** Defines ******************************************************/
#define HAL_IO_ERRORCALLBACK        //Define a callback routine for errors in the HAL-IO-Module

#define ADC_MAX_VAL     ADC_INPUT_DEFAULT_HIGH_LIMIT

//Xmacro for sense lines
    /*      Connector name  |       Pin mapping   |                       Check Pin   */
#define SENSE_MAP\
    S_MAP(    eSenseVoltage_0       ,   CY_PIN_MAPPING(Pin_VoltageMeasure__0, false))\
    S_MAP(    eSenseVoltage_1       ,   CY_PIN_MAPPING(Pin_VoltageMeasure__1, false))\
    S_MAP(    eSenseVoltage_2       ,   CY_PIN_MAPPING(Pin_VoltageMeasure__2, false))\
    S_MAP(    eSenseVoltage_3       ,   CY_PIN_MAPPING(Pin_VoltageMeasure__3, false))\
    S_MAP(    eSenseCurrent_0       ,   CY_PIN_MAPPING(Pin_CurrentMeasure__0, false))\
    S_MAP(    eSenseCurrent_1       ,   CY_PIN_MAPPING(Pin_CurrentMeasure__1, false))\
    S_MAP(    eSenseCurrent_2       ,   CY_PIN_MAPPING(Pin_CurrentMeasure__2, false))\
    S_MAP(    eSenseCurrent_3       ,   CY_PIN_MAPPING(Pin_CurrentMeasure__3, false))\
    S_MAP(    eSenseTemperature_0   ,   CY_PIN_MAPPING(Pin_NTC_IN__0,         false))\
    S_MAP(    eSenseTemperature_1   ,   CY_PIN_MAPPING(Pin_NTC_IN__1,         false))\
    S_MAP(    eSenseTemperature_2   ,   CY_PIN_MAPPING(Pin_NTC_IN__2,         false))\
    S_MAP(    eSenseTemperature_3   ,   CY_PIN_MAPPING(Pin_NTC_IN__3,         false))\
    S_MAP(    eSensePIR             ,   CY_PIN_MAPPING(Pin_PIR__0,            false))

#define OUTPUT_MAP\
    O_MAP(    ePin_PwmOut_0         ,   CY_PIN_MAPPING(Pin_PwmOut_0__0,         false))\
    O_MAP(    ePin_PwmOut_1         ,   CY_PIN_MAPPING(Pin_PwmOut_1__0,         false))\
    O_MAP(    ePin_PwmOut_2         ,   CY_PIN_MAPPING(Pin_PwmOut_2__0,         false))\
    O_MAP(    ePin_PwmOut_3         ,   CY_PIN_MAPPING(Pin_PwmOut_3__0,         false))\
    O_MAP(    ePin_VoltEn_0         ,   CY_PIN_MAPPING(Pin_VOLT_EN__0,          true) )\
    O_MAP(    ePin_VoltEn_1         ,   CY_PIN_MAPPING(Pin_VOLT_EN__1,          true) )\
    O_MAP(    ePin_VoltEn_2         ,   CY_PIN_MAPPING(Pin_VOLT_EN__2,          true) )\
    O_MAP(    ePin_VoltEn_3         ,   CY_PIN_MAPPING(Pin_VOLT_EN__3,          true) )\
    O_MAP(    ePin_PwmEn_0          ,   CY_PIN_MAPPING(Pin_PWM_EN__0,           true) )\
    O_MAP(    ePin_PwmEn_1          ,   CY_PIN_MAPPING(Pin_PWM_EN__1,           true) )\
    O_MAP(    ePin_PwmEn_2          ,   CY_PIN_MAPPING(Pin_PWM_EN__2,           true) )\
    O_MAP(    ePin_PwmEn_3          ,   CY_PIN_MAPPING(Pin_PWM_EN__3,           true) )\
    O_MAP(    ePin_LedGreen         ,   CY_PIN_MAPPING(Pin_LED_G__0,            true) )\
    O_MAP(    ePin_LedRed           ,   CY_PIN_MAPPING(Pin_LED_R__0,            true) )\
    O_MAP(    ePin_EspResetPin      ,   CY_PIN_MAPPING(Pin_ESP_Reset__0,        false))\
    O_MAP(    ePin_EspResetCopy     ,   CY_PIN_MAPPING(Pin_ESP_COPY__0,         false))

    /* PWM-X-Macro table*/    
#define PWM_MAP\
    P_MAP(ePWM_0,   CY_PWM_MAPPING(0))\
    P_MAP(ePWM_1,   CY_PWM_MAPPING(1))\
    P_MAP(ePWM_2,   CY_PWM_MAPPING(2))\
    P_MAP(ePWM_3,   CY_PWM_MAPPING(3))

/*              PORT        |   Pin_0-Callback  |   Pin_1-Callback  |   Pin_2-Callback  |   Pin_3-Callback  |   Pin_4-Callback  |   Pin_5-Callback  |   Pin_6-Callback  |   Pin_7-Callback  |*/
#define ISR_IO_MAP\
    ISR_MAP(    ePort_0     ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        )\
    ISR_MAP(    ePort_1     ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        )\
    ISR_MAP(    ePort_2     ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        )\
    ISR_MAP(    ePort_3     ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        )\
    ISR_MAP(    ePort_4     ,       DR_Regulation_RxInterruptOnSleep        ,       NULL        ,       NULL        , DR_Regulation_CheckSensorForMotion  ,       NULL        ,       NULL        ,       NULL        ,       NULL        )\
    ISR_MAP(    ePort_5     ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        )\
    ISR_MAP(    ePort_6     ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        )\
    ISR_MAP(    ePort_7     ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        ,       NULL        )


#ifdef __cplusplus
}
#endif    

#endif //_HAL_IO_CONFIG_H_

/* [] END OF FILE */
