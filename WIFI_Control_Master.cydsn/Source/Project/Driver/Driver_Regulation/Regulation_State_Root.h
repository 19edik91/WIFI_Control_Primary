//********************************************************************************
/*!
\author     Kraemer E 
\date       18.02.2019

\file       Regulation_State.h
\brief      Regulation state machine handler

***********************************************************************************/
#ifndef _REGULATION_STATE_ROOT_H_
#define _REGULATION_STATE_ROOT_H_
    
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
/****************************** type definitions *****************************/    
/***************************** global variables ******************************/

/************************ externally visible functions ***********************/
tCStateDefinition* Regulation_State_RootLink(tsRegulationHandler* psRegulationHandler, u8 ucOutputIdx);




#ifdef __cplusplus
}
#endif    

#endif //_REGULATION_STATE_ROOT_H_