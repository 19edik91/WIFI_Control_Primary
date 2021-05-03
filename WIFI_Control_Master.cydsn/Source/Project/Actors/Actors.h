/* ========================================
 *
 * Copyright Eduard Kraemer, 2019
 *
 * ========================================
*/

#ifndef _ACTORS_H_
#define _ACTORS_H_

#ifdef __cplusplus
extern "C"
{
#endif   

#include "BaseTypes.h"
#include "Aom.h"


/****************************************** Defines ******************************************************/
// Macro derived form PsoC generated source code
#define CY_PIN_MAPPING(name, check) {name ## __PS, name ## __DR, name ## __PC, name ## __SHIFT, name ## __PORT, check}
//Definitions: PS - PortPinStatusRegister | DR - PortOutputDataRegister | PC - PortConfigurationRegister

/* PWM_Definition                   Start_Fn          |     Stop_Fn            |       WriteCmp_Fn             |   ReadCmp_Fn                 |     ReadPeriod_Fn eriod value   |   WritePeriod_Fn                 */
#define CY_PWM_MAPPING(index) {PWM_ ## index ## _Start,  PWM_ ## index ## _Stop, PWM_ ## index ## _WriteCompare, PWM_ ## index ## _ReadCompare,     PWM_ ## index ## _ReadPeriod,   PWM_ ## index ## _WritePeriod   }
//    P_MAP(    ePWM_0    ,   PWM_0_Start      ,  PWM_0_Stop      ,       PWM_0_WriteCompare  ,   PWM_0_ReadCompare   ,     PWM_0_Init)

#define COUNT_OF_SENSELINES     eInvalidSense
#define COUNT_OF_OUTPUTS        eInvalidOutput
#define COUNT_OF_PWM            eInvalidPWM

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
    O_MAP(    ePwmOut_0         ,   CY_PIN_MAPPING(Pin_PwmOut_0__0,         false))\
    O_MAP(    ePwmOut_1         ,   CY_PIN_MAPPING(Pin_PwmOut_1__0,         false))\
    O_MAP(    ePwmOut_2         ,   CY_PIN_MAPPING(Pin_PwmOut_2__0,         false))\
    O_MAP(    ePwmOut_3         ,   CY_PIN_MAPPING(Pin_PwmOut_3__0,         false))\
    O_MAP(    eLedGreen         ,   CY_PIN_MAPPING(Pin_LED_G__0,            true) )\
    O_MAP(    eLedRed           ,   CY_PIN_MAPPING(Pin_LED_R__0,            true) )\
    O_MAP(    eEspResetPin      ,   CY_PIN_MAPPING(Pin_ESP_Reset__0,        false))

    /* PWM-X-Macro table*/    
#define PWM_MAP\
    P_MAP(ePWM_0,   CY_PWM_MAPPING(0))\
    P_MAP(ePWM_1,   CY_PWM_MAPPING(1))\
    P_MAP(ePWM_2,   CY_PWM_MAPPING(2))\
    P_MAP(ePWM_3,   CY_PWM_MAPPING(3))


typedef enum
{
    #define P_MAP(PwmName, PwmFn) PwmName,
        PWM_MAP
    #undef P_MAP
    eInvalidPWM    
}tePWM;

//Enum for the Senselines
typedef enum
{
    #define S_MAP( Senseline, PinMapping) Senseline,
        SENSE_MAP
    #undef S_MAP
    eInvalidSense
}teSenseLine;

//Enum for the outputs
typedef enum
{
    #define O_MAP( Senseline, PinMapping) Senseline,
        OUTPUT_MAP
    #undef O_MAP
    eInvalidOutput
}teOutput;



// struct for invalid ports and pins
typedef struct
{
    u16 uiInvalidOutputs;
    u8  ucPinFaults[NUMBER_OF_PORTS];
}tFaultOutputPortValues;

// struct with registers for a GPIO pin
typedef struct _pin_mapping
{
    u32  ulPortStateRegister;
    u32  ulPortOutputDataRegister;
    u32  ulPortConfigRegister;
    u8   ucBitShift;
    u8   ucPort;
    bool bCheckIO;
} tPinMapping;

typedef struct _pwm_fn_mapping
{
    pFunction pfnStart;
    pFunction pfnStop;
    pFunctionParamU32 pfnWriteCompare;
    pulFunction pfnReadCompare;
    pulFunction pfnReadPeriod;
    pFunctionParamU32 pfnWritePeriod;
}tsPwmFnMapping;

extern const tPinMapping sOutputMap[COUNT_OF_OUTPUTS];
extern const tPinMapping sSenseMap[COUNT_OF_SENSELINES];
extern const tsPwmFnMapping sPwmMap[COUNT_OF_PWM];



/****************************************** External visible functiones **********************************/

void Actors_Init(void);
bool Actors_ReadDigitalSense(teSenseLine eSenseLine);
bool Actors_ReadOutputStatus(teOutput eOutput);
void Actors_SetOutputStatus(teOutput eOutput, bool bStatus);
u8 Actors_ReadPortPinStateRegister(u8 ucPort);
u8 Actors_ReadPortDataRegister(u8 ucPort);
bool Actors_ValidateOutputPorts(tFaultOutputPortValues* psFaultOutputPortValues);
bool Actors_GetPortPinMaskStatus(void);
void Actors_Sleep(void);
void Actors_Wakeup(void);
bool Actors_GetPwmStatus(u8 ucOutputIdx);

#ifdef __cplusplus
}
#endif    

#endif //_ACTORS_H_