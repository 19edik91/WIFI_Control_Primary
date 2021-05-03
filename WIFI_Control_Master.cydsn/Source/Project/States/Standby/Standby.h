//********************************************************************************
/*!
\author     Kraemer E (Ego)
\date       18.02.2019

\file       Standby.h
\brief      Standby handler

***********************************************************************************/

#ifndef _STANDBY_H_
#define _STANDBY_H_
    
    
/********************************* includes **********************************/    
#include "BaseTypes.h"
#include "Measure.h"
#include "Messages.h"

/***************************** defines / macros ******************************/

/****************************** type definitions *****************************/    
typedef enum
{
    eStateActive,           /**< In this state all peripherals are active. Standby isn't active */
    eStateEntrySystem,      /**< Standby on system is entered. All sytem relevant peripherals are switched off */
    eStateEntryLowVoltage,  /**< Standby on LV is entered. All peripherals on LV side are switched off */
    eStateStandby,          /**< Standby is active. Should only wait for RS485 and sleep */
    eStateExitLowVoltage,   /**< Standby on LV is leaved. While standby is requested this state shall activate Measurements */
    eStateExitSystem        /**< Standby is leaved completely here. Next state is "eStateActive" */
}teStandbyState;

/***************************** global variables ******************************/

/************************ externally visible functions ***********************/
void Standby_ChangeState(teStandbyState eRequestedState);
teStandbyState Standby_Handler(void);
teStandbyState Standby_GetStandbyState(void);


#endif // _STANDBY_H_
