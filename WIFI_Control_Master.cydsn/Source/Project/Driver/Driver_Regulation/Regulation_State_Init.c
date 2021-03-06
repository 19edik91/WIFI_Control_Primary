//********************************************************************************
/*!
\author     Kraemer E.
\date       05.02.2019

\file       Regulation.c
\brief      Functionality for the regulation

***********************************************************************************/
#include "DR_Regulation.h"
#include "DR_ErrorDetection.h"
#include "OS_EventManager.h"
#include "OS_ErrorDebouncer.h"
#include "HAL_IO.h"
#include "ErrorHandler.h"
#include "Aom_Regulation.h"
#include "Aom_Flash.h"
#include "Aom_Measure.h"
#include "Regulation_Data.h"
#include "Regulation_State_Init.h"

#if 0
/****************************************** Defines ******************************************************/
/****************************************** Function prototypes ******************************************/
static void StateEntry(u8 ucOutputIdx);
static void StateActive(u8 ucOutputIdx);
static void StateExit(u8 ucOutputIdx);
static void StateOff(u8 ucOutputIdx);

/****************************************** Variables ****************************************************/
static u16 uiOldVoltageValue[DRIVE_OUTPUTS];
static u8 ucVoltageChangedCnt[DRIVE_OUTPUTS] = {3, 3, 3};

static tsRegulationHandler* psRegHandler[DRIVE_OUTPUTS] = {NULL, NULL, NULL};

static teSubState eSubState[DRIVE_OUTPUTS];

static tCStateDefinition sRegulationStateFn = 
{
    .pFctOff = StateOff,
    .pFctEntry = StateEntry,
    .pFctActive = StateActive,
    .pFctExit = StateExit
};
/****************************************** loacl functiones *********************************************/

//********************************************************************************
/*!
\author  KraemerE
\date    18.02.2019
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
    
    /***** Enable regulation modules *****/      
    if(HAL_IO_GetPwmStatus(ucOutputIdx) == false)
    {       
        /* Start PWM module */
        sPwmMap[ucOutputIdx].pfnStart();
        
        /* Check PWM output for a fault */
        bErrorFound = ErrorDetection_CheckPwmOutput(ucOutputIdx);
        
        if(bErrorFound == false)
        {
            /* Enable PWM module with lowest brightness level */
            sPwmMap[ucOutputIdx].pfnWriteCompare(sPwmMap[ucOutputIdx].pfnReadPeriod());
            
            //Enable delay for smoother dimming
            CyDelay(10);
        }
    }
    
    /* Set the regulation values for this state */
    psRegHandler[ucOutputIdx]->sRegAdcVal.uiReqValue = 0;

    psRegHandler[ucOutputIdx]->sRegAdcVal.uiIsValue = Aom_Measure_GetAdcIsValue(eMeasureChVoltage, ucOutputIdx);
    
    if(bErrorFound == false)
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
           
    u16 uiCurrentAdc = Aom_Measure_GetAdcIsValue(eMeasureChCurrent, ucOutputIdx);
    u16 uiVoltageAdc = Aom_Measure_GetAdcIsValue(eMeasureChVoltage, ucOutputIdx);
    
    switch(eSubState[ucOutputIdx])
    {
        case eSubMinInit:
        {
            /* The first current flow is the low limit */
            if(uiCurrentAdc)
            {
                /* Save settings in AOM */
                Aom_Regulation_SetMinSystemSettings(uiCurrentAdc, uiVoltageAdc, sPwmMap[ucOutputIdx].pfnReadCompare(), ucOutputIdx);
                
                /* Change to next state */
                eSubState[ucOutputIdx] = eSubMaxInit;
                
                /* Clear old voltage value */
                uiOldVoltageValue[ucOutputIdx] = 0;
            }
            else
            {
                /* While the max voltage ADC value isn't reached, increment the requested value */
                if(psRegHandler[ucOutputIdx]->sRegAdcVal.uiReqValue < ADC_MAX_VAL)
                {
                    psRegHandler[ucOutputIdx]->sRegAdcVal.uiReqValue += Aom_Measure_GetAdcVoltageStepValue();
                }
                else
                {
                    /* Limit reached which means no load available */
                    ErrorDebouncer_PutErrorInQueue(eLoadMissingFault_0 + ucOutputIdx);
                }                
            }
            break;
        }
        
        case eSubMaxInit:
        {
            /* When the new voltage value doesn't really changes anymore a saturation is reached */
            if((uiOldVoltageValue[ucOutputIdx] + Aom_Measure_GetAdcVoltageStepValue()) <= uiVoltageAdc)
            {
                if(ucVoltageChangedCnt[ucOutputIdx] < 3)
                    ++ucVoltageChangedCnt[ucOutputIdx];
                
                /* Save new value for next comparison */
                uiOldVoltageValue[ucOutputIdx] = uiVoltageAdc;
            }
            else
            {
                --ucVoltageChangedCnt[ucOutputIdx];
            }
            
            
            /* Increment the requested value until the current limit or the voltage limit is reached */
            if(uiCurrentAdc > Aom_Measure_GetAdcCurrentLimitValue() 
                || ucVoltageChangedCnt[ucOutputIdx] == 0)
            {
                /* limit reached */
                Aom_Regulation_SetMaxSystemSettings(uiCurrentAdc, uiVoltageAdc, sPwmMap[ucOutputIdx].pfnReadCompare(), ucOutputIdx);
                
                /* Change to next state */
                eSubState[ucOutputIdx] = eSubDone;
            }
            else
            {
                psRegHandler[ucOutputIdx]->sRegAdcVal.uiReqValue += Aom_Measure_GetAdcVoltageStepValue();
            }            
            break;   
        }
        
        case eSubDone:
        {
            /* Wait for all outputs to be done */
            u8 ucDoneCnt = 0;
            
            u8 ucIdx;
            for(ucIdx = 0; ucIdx < DRIVE_OUTPUTS; ucIdx++)
            {
                if(eSubState[ucOutputIdx] == eSubDone)
                {
                    ++ucDoneCnt;
                }
            }
            
            if(ucDoneCnt == DRIVE_OUTPUTS)
            {
                /* Save values in flash */
                Aom_Flash_WriteSystemSettingsInFlash();
                
                /* Post event to inform GUI */
                EVT_PostEvent(eEvtInitRegulationValue, eEvtParam_InitRegulationDone, 0);
            }
            
            /* When done set to user requested value */
            #warning what to do?
            //psRegHandler->sRegAdcVal.uiReqValue = Aom_Measure_GetAdcRequestedValue(ucOutputIdx);
            
            psRegHandler[ucOutputIdx]->sRegState.bStateReached = true;
            psRegHandler[ucOutputIdx]->sRegAdcVal.bInitialized = true;
            break;
        }
    }
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
    if(Actors_GetPwmStatus(ucOutputIdx) == true)
    {       
        /* Wait a short time */
        CyDelay(10);

        sPwmMap[ucOutputIdx].pfnStop();
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
tCStateDefinition* Regulation_State_InitLink(tsRegulationHandler* psRegulationHandler, u8 ucOutputIdx)
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
#endif