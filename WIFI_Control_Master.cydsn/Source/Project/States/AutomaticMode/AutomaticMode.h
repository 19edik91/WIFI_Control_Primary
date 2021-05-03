//********************************************************************************
/*!
\author     Kraemer E (Ego)
\date       21.08.2020

\file       AutomaticMode.h
\brief      Automatic Mode handler

***********************************************************************************/

#ifndef _AUTOMATICMODE_H_
#define _AUTOMATICMODE_H_
    
    
/********************************* includes **********************************/    
#include "BaseTypes.h"
#include "Measure.h"
#include "Messages.h"

/***************************** defines / macros ******************************/

/****************************** type definitions *****************************/    
typedef enum
{
    eStateDisabled,           /**<  */
    eStateAutomaticMode_1,    /**<  */
    eStateAutomaticMode_2,    /**<  */
    eStateAutomaticMode_3,    /**<  */
}teAutomaticState;

/***************************** global variables ******************************/

/************************ externally visible functions ***********************/
void AutomaticMode_ChangeState(teAutomaticState eRequestedState);
bool AutomaticMode_Handler(void);
teAutomaticState AutomaticMode_GetAutomaticState(void);
void AutomaticMode_Tick(u16 uiMsTick);
void AutomaticMode_ResetBurningTimeout(void);

#endif // _AUTOMATICMODE_H_
