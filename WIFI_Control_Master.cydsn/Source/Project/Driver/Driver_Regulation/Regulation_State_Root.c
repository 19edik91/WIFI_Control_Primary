//********************************************************************************
/*!
\author     Kraemer E.
\date       05.02.2019

\file       Regulation.c
\brief      Functionality for the regulation

***********************************************************************************/
#include "OS_EventManager.h"
#include "OS_ErrorDebouncer.h"

#include "DR_Regulation.h"
#include "DR_ErrorDetection.h"

#include "HAL_IO.h"

#include "Aom_Regulation.h"
#include "Aom_Flash.h"
#include "Aom_Measure.h"
#include "Regulation_Data.h"
#include "Regulation_State_Root.h"
/****************************************** Defines ******************************************************/

/****************************************** Function prototypes ******************************************/
static void StateEntry(u8 ucOutputIdx);
static void StateActive(u8 ucOutputIdx);
static void StateExit(u8 ucOutputIdx);
static void StateOff(u8 ucOutputIdx);

/****************************************** Variables ****************************************************/
/* Pointer to the regulation handler */
static tsRegulationHandler* psRegHandler[DRIVE_OUTPUTS] = {NULL, NULL, NULL};

static tCStateDefinition sRegulationStateFn = 
{
    .pFctOff = StateOff,
    .pFctEntry = StateEntry,
    .pFctActive = StateActive,
    .pFctExit = StateExit
};

/****************************************** local functions *********************************************/
//********************************************************************************
/*!
\author  KraemerE
\date    06.11.2020
\brief   Enter function of the standby process. In this module all necessary
         system related peripherals should be switched off.
\param   none
\return  none
***********************************************************************************/
static void StateEntry(u8 ucOutputIdx)
{
    /* Change actual state */
    psRegHandler[ucOutputIdx]->sRegState.eRegulationState = eStateEntry;

    bool bErrorFound = false;
    bool bSystemVoltageFound = false;
    
    /***** Enable regulation modules *****/      
    if(HAL_IO_GetPwmStatus(ucOutputIdx) == false)
    {       
        //TODO: Enable voltage output
        
        /* Check if system voltage was already calculated. Only valid when PWM is disabled */
        bSystemVoltageFound = Aom_Measure_SystemVoltageCalculated();
        
        if(bSystemVoltageFound)
        {
            /* Start PWM module */
            HAL_IO_PWM_Start(ucOutputIdx);
            
            /* Check PWM output for a fault */
            bErrorFound = DR_ErrorDetection_CheckPwmOutput(ucOutputIdx);
            
            if(bErrorFound == false)
            {
                /* Enable PWM module with lowest brightness level */
                //sPwmMap[ucOutputIdx].pfnWriteCompare(sPwmMap[ucOutputIdx].pfnReadPeriod());
                HAL_IO_PWM_WriteCompare(ucOutputIdx, 10);
                
                //Enable delay for smoother dimming
                CyDelay(10);
            }
        }
    }
    
    /* Set the regulation values for this state */
    psRegHandler[ucOutputIdx]->sRegAdcVal.uiReqValue = 0;

    psRegHandler[ucOutputIdx]->sRegAdcVal.uiIsValue = Aom_Measure_GetAdcIsValue(eMeasureChVoltage, ucOutputIdx);
    
    if(bErrorFound == false && bSystemVoltageFound == true)
        psRegHandler[ucOutputIdx]->sRegState.bStateReached = true;
}


//********************************************************************************
/*!
\author  KraemerE
\date    18.02.2019
\brief   In this state the CPU shall only wait for a message from the UIM,
         do some self-tests and maybe go to sleep.
\param   none
\return  none
***********************************************************************************/
static void StateActive(u8 ucOutputIdx)
{
    /* Change actual state */
    psRegHandler[ucOutputIdx]->sRegState.eRegulationState = eStateActiveR;
       
    /* Set the regulation values for this state */
    psRegHandler[ucOutputIdx]->sRegAdcVal.uiReqValue = Aom_Measure_GetAdcRequestedValue(ucOutputIdx);
    psRegHandler[ucOutputIdx]->sRegAdcVal.uiIsValue = Aom_Measure_GetAdcIsValue(eMeasureChVoltage, ucOutputIdx);
    
    psRegHandler[ucOutputIdx]->sRegState.bStateReached = true;
}

//********************************************************************************
/*!
\author  KraemerE
\date    18.02.2019
\brief   This state re-enables the low voltage peripherals to measure the temperature.
         After some measurements the state is leaved and entered via "StateEntryLowVoltage"
         or when the standby should be left the "StateExitSystem".
\param   none
\return  none
***********************************************************************************/
static void StateExit(u8 ucOutputIdx)
{    
    /* Change actual state */
    psRegHandler[ucOutputIdx]->sRegState.eRegulationState = eStateExit;
    
    /* Set the regulation values for this state */
    psRegHandler[ucOutputIdx]->sRegAdcVal.uiReqValue = 0x00;   
    psRegHandler[ucOutputIdx]->sRegAdcVal.uiIsValue = Aom_Measure_GetAdcIsValue(eMeasureChVoltage,ucOutputIdx);    
    
    //Dimm down until lowest possible value is reached
    if((psRegHandler[ucOutputIdx]->sRegAdcVal.uiReqValue == psRegHandler[ucOutputIdx]->sRegAdcVal.uiIsValue)
        || psRegHandler[ucOutputIdx]->sRegAdcVal.bCantReach)
    {
        psRegHandler[ucOutputIdx]->sRegState.bStateReached = true;
    }
}


//********************************************************************************
/*!
\author  KraemerE
\date    18.02.2019
\brief   This state is called when the PCM temperature is above the set limit or
         the UIM requested to leave the stand by mode.
\param   none
\return  none
***********************************************************************************/
static void StateOff(u8 ucOutputIdx)
{
    /* Change actual state */
    psRegHandler[ucOutputIdx]->sRegState.eRegulationState = eStateOff;
    
    /***** Disable regulation module ****/    
    if(HAL_IO_GetPwmStatus(ucOutputIdx) == true)
    {       
        /* Wait a short time */
        CyDelay(10);
        
        /* Stops PWM Timer and PWM modlue */
        HAL_IO_PWM_Stop(ucOutputIdx);
    }
    
    psRegHandler[ucOutputIdx]->sRegState.bStateReached = true;
}

/****************************************** External visible functiones **********************************/

//********************************************************************************
/*!
\author  KraemerE
\date    06.11.2020
\brief   Links the regulation module with this state module together
\param   psRegulationHandler - Pointer to the regulation handler structure of the
                               regulation module
\return  tCStateDefinition* - Either the address of this regulation function module or null
***********************************************************************************/
tCStateDefinition* Regulation_State_RootLink(tsRegulationHandler* psRegulationHandler, u8 ucOutputIdx)
{    
    /* Check for valid pointer */
    if(psRegulationHandler)
    {
        /* Link the regulation handler together */
        psRegHandler[ucOutputIdx] = psRegulationHandler;
        
        /* Return the correct state function pointers */
        return &sRegulationStateFn;
    }
    else
    {
        return NULL;
    }
}