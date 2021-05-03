//********************************************************************************
/*!
\author     Kraemer E 
\date       18.02.2019

\file       Regulation.h
\brief      Regulation handler

***********************************************************************************/
#ifndef _REGULATION_STATE_INIT_H_
#define _REGULATION_STATE_INIT_H_
    
#ifdef __cplusplus
extern "C"
{
#endif   

    
/********************************* includes **********************************/    
#include "BaseTypes.h"
#include "Aom.h"
#include "Messages.h"
#include "Regulation.h"
#include "Regulation_Data.h"

/***************************** defines / macros ******************************/

/****************************** type definitions *****************************/    


/***************************** global variables ******************************/

/************************ externally visible functions ***********************/
tCStateDefinition* Regulation_State_InitLink(tsRegulationHandler* psRegulationHandler, u8 ucOutputIdx);






#ifdef __cplusplus
}
#endif    

#endif //_REGULATION_STATE_INIT_H_