//********************************************************************************
/*!
\author     Kraemer E 
\date       18.02.2019

\file       Regulation.h
\brief      Regulation handler

***********************************************************************************/
#ifndef _REGULATION_H_
#define _REGULATION_H_
    
#ifdef __cplusplus
extern "C"
{
#endif   

    
/********************************* includes **********************************/    
#include "BaseTypes.h"
#include "Aom.h"
#include "Regulation_Data.h"

/***************************** defines / macros ******************************/
#define ADC_LIMITS                   4      //Just test it..
#define PWM_ISR_ENABLE               0

/****************************** type definitions *****************************/    
typedef struct
{
    uint16_t uiPeriodValue;     //The period value of the chosen PWM module
    uint16_t uiCompareValue;    //The compare value of the chosen PWM module
    bool     bStatus;           //The running status of the chosen PWM module
}tsPwmData;

/***************************** global variables ******************************/

/************************ externally visible functions ***********************/
void    DR_Regulation_Init(void);
void    DR_Regulation_ChangeState(teRegulationState eRequestedState, u8 ucOutputIdx);

bool    DR_Regulation_GetEspResetStatus(void);
void    DR_Regulation_SetEspResetStatus(bool bReset);

u8      DR_Regulation_Handler(u16 uiMilliSecElapsed);
teRegulationState DR_Regulation_GetActualState(u8 ucOutputIdx);
teRegulationState DR_Regulation_GetRequestedState(u8 ucOutputIdx);
bool DR_Regulation_GetHardwareEnabledStatus(u8 ucOutputIdx);

void DR_Regulation_ModulesSleep(void);
void DR_Regulation_ModulesWakeup(void);
void DR_Regulation_SetWakeupInterrupts(void);
void DR_Regulation_DeleteWakeupInterrupts(void);
void DR_Regulation_EnterDeepSleepMode(void);
void DR_Regulation_RxInterruptOnSleep(void);

void DR_Regulation_GetPWMData(uint8_t ucOutputIdx, tsPwmData* psPwmData);

#ifdef __cplusplus
}
#endif    

#endif //_REGULATION_H_