//********************************************************************************
/*!
\author     Kraemer E 
\date       18.02.2019

\file       Regulation.h
\brief      Regulation handler

***********************************************************************************/
#ifndef _REGULATION_DATA_H_
#define _REGULATION_DATA_H_
    
#ifdef __cplusplus
extern "C"
{
#endif   

    
/********************************* includes **********************************/    
#include "BaseTypes.h"
#include "Aom.h"
#include "Messages.h"

/***************************** defines / macros ******************************/
/****************************** type definitions *****************************/    
typedef enum
{
    eStateOff,      /**< In this state all peripherals are active. Standby isn't active */
    eStateEntry,    /**< Standby on system is entered. All sytem relevant peripherals are switched off */
    eStateActiveR,  /**< Standby is active. Should only wait for RS485 and sleep */
    eStateExit      /**< Standby is leaved completely here. Next state is "eStateActive" */
}teRegulationState;

typedef enum
{
    eStateMachine_Root, /**< Root state machine for normal regulation */
    eStateMachine_Init  /**< Initialization state machine for init the hardware */
}teRegulationStateMachine;

typedef struct _tCStateDefinition
{
    pFunctionParamU8   pFctOff;     /**< Pointer to the Off function. This function will be called if the state is entered */
    pFunctionParamU8   pFctEntry;   /**< Pointer to the Entry function. This function will be called if the state is entered */
    pFunctionParamU8   pFctActive;  /**< Pointer to the Execution function. This function will be called for every message if the state is active or a child of this state is active. */
    pFunctionParamU8   pFctExit;    /**< Pointer to the first Exit function of the state. This function is called if the state machine changes to an other state. */
}tCStateDefinition;

typedef struct
{
    teRegulationState eRegulationState;
    teRegulationState eNextState;
    teRegulationState eReqState;
    pFunctionParamU8 pFctState;
    bool bStateReached;
}tsRegulationState;

typedef struct
{
    u16 uiOldReqValue;
    u16 uiReqValue;
    u16 uiIsValue;
    bool bReached; 
    bool bCantReach;
    bool bInitialized;
}tsRegAdcVal;

typedef struct
{
    tsRegAdcVal         sRegAdcVal;
    tsRegulationState   sRegState;
    bool                bHardwareEnabled;
}tsRegulationHandler;


typedef enum
{
    eSubMinInit,
    eSubMaxInit,
    eSubDone
}teSubState;


#ifdef __cplusplus
}
#endif    

#endif //_REGULATION_H_