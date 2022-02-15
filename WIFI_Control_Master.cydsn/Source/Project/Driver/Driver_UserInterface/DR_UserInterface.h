//********************************************************************************
/*!
\author     Kraemer E 
\date       01.02.2022

\file       Driver_UserInterface.h
\brief      Interface for UI dependent handling

***********************************************************************************/
#ifndef _DR_USERINTERFACE_H_
#define _DR_USERINTERFACE_H_
    
#ifdef __cplusplus
extern "C"
{
#endif   

    
/********************************* includes **********************************/    
#include "BaseTypes.h"
#include "Aom.h"

/***************************** defines / macros ******************************/

/****************************** type definitions *****************************/    
/***************************** global variables ******************************/

/************************ externally visible functions ***********************/
void    DR_UI_Init(void);
void    DR_UI_ToggleErrorLED(void);
void    DR_UI_ToggleHeartBeatLED(void);
void    DR_UI_SwitchOffHeartBeatLED(void);

void    DR_UI_CheckSensorForMotion(void);

void    DR_UI_InfraredInputIRQ(void);
void    DR_UI_InfraredCmd(uint8_t uiIrCmd);
#ifdef __cplusplus
}
#endif    

#endif //_DR_USERINTERFACE_H_