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
#include "Messages.h"
#include "Regulation_Data.h"

/***************************** defines / macros ******************************/
#define ADC_LIMITS                   2      //Just test it..
#define PWM_ISR_ENABLE               0

/****************************** type definitions *****************************/    
/***************************** global variables ******************************/

/************************ externally visible functions ***********************/
void Regulation_Init(void);
void Regulation_ChangeState(teRegulationState eRequestedState, u8 ucOutputIdx);
u8 Regulation_Handler(void);
teRegulationState Regulation_GetActualState(u8 ucOutputIdx);
teRegulationState Regulation_GetRequestedState(u8 ucOutputIdx);
bool Regulation_GetHardwareEnabledStatus(u8 ucOutputIdx);






#ifdef __cplusplus
}
#endif    

#endif //_REGULATION_H_