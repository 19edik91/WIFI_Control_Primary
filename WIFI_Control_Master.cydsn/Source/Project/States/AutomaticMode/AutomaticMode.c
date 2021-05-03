//********************************************************************************
/*!
\author     Kraemer E.
\date       21.08.2020

\file       AutomaticMode.c
\brief      State machine for different automatic modes

***********************************************************************************/
#include "AutomaticMode.h"
#include <project.h>
#include "Aom_Regulation.h"
#include "Aom_System.h"
#include "Measure.h"
#include "Watchdog.h"
#include "EventManager.h"
#include "Serial.h"
#include "Actors.h"

/***************************** defines / macros ******************************/

/************************ local data type definitions ************************/
typedef bool (*pbFunction)(void);

typedef struct _tCStateDefinition
{
    pbFunction   pFctAutomaticMode1;   /**< Pointer to the Entry function. This function will be called if the state is entered */
    pbFunction   pFctAutomaticMode2;   /**< Pointer to the Entry function. This function will be called if the state is entered */
    pbFunction   pFctAutomaticMode3;   /**< Pointer to the Execution function. This function will be called for every message if the state is active or a child of this state is active. */
}tCStateDefinition;

typedef struct
{
    teAutomaticState eCurrentState;
    teAutomaticState eReqState;
    pbFunction pFctState;
    bool bStateReached;
}tsAutomaticState;


/************************* local function prototypes *************************/
static bool StateAutomaticMode_1(void);
static bool StateAutomaticMode_2(void);
static bool StateAutomaticMode_3(void);


/************************* local data (const and var) ************************/
tsAutomaticState sAutomaticState = 
{
    .eCurrentState = eStateDisabled,
    .eReqState = eStateDisabled,
    .pFctState = NULL,
    .bStateReached = false
};

const tCStateDefinition sAutomaticStateFn = 
{
    .pFctAutomaticMode1 = StateAutomaticMode_1, 
    .pFctAutomaticMode2 = StateAutomaticMode_2,
    .pFctAutomaticMode3 = StateAutomaticMode_3
};

/****************************** local functions ******************************/
//********************************************************************************
/*!
\author  KraemerE
\date    19.02.2019

\fn      CheckForNextState

\brief   Checks the next available state according to the requested state. 

\param   none

\return  none
***********************************************************************************/
static void CheckForNextState(void)
{    
    /* When the requested state wasn't reached yet swtich the function pointer according the next state */
    if(sAutomaticState.eReqState != sAutomaticState.eCurrentState
        && sAutomaticState.bStateReached == true)
    {
        /* Change callback for dependent state */
        switch(sAutomaticState.eReqState)
        {
            case eStateDisabled:
            {
                sAutomaticState.pFctState = NULL;
                break;
            }
            
            case eStateAutomaticMode_1:
            {
                sAutomaticState.pFctState = sAutomaticStateFn.pFctAutomaticMode1;
                break;
            }
            
            case eStateAutomaticMode_2:
            {
                sAutomaticState.pFctState = sAutomaticStateFn.pFctAutomaticMode2;
                break;
            }
            
            case eStateAutomaticMode_3:
            {
                sAutomaticState.pFctState = sAutomaticStateFn.pFctAutomaticMode3;
                break;
            }
            
            
            default:
                break;
        } 
        
        /* Clear bit for next state */
        sAutomaticState.bStateReached = false;
    }
}



//********************************************************************************
/*!
\author  KraemerE
\date    21.08.2020

\fn      StateAutomaticMode_1

\brief   Automatic mode 1 - Simply switching the light on and off during the
                            user defined time slots.

\param   none

\return  none
***********************************************************************************/
static bool StateAutomaticMode_1(void)
{
    bool bEnableLight = OFF;
    
    /* Change actual state */
    sAutomaticState.eCurrentState = eStateAutomaticMode_1;

    const tsAutomaticModeValues* psAutoValues = Aom_System_GetAutomaticModeValuesStruct();    
    bEnableLight = psAutoValues->bInUserTimerSlot;
    
    sAutomaticState.bStateReached = true;
    
    return bEnableLight;
}




//********************************************************************************
/*!
\author  KraemerE
\date    21.08.2020

\fn      StateAutomaticMode_2

\brief   Automatic mode 2 - Light is only switched on during the given time slots
                            and when a motion was detected during these slots.
                            Light will burn for the set burning period after the last
                            motion was detected. A new detected motion restarts the
                            burning timeout.
\param   none

\return  none
***********************************************************************************/
static bool StateAutomaticMode_2(void)
{    
    bool bEnableLight = OFF;
    
    /* Change actual state */
    sAutomaticState.eCurrentState = eStateAutomaticMode_2;

    const tsAutomaticModeValues* psAutoValues = Aom_System_GetAutomaticModeValuesStruct();  
    
    if(psAutoValues->bMotionDetected && psAutoValues->bInUserTimerSlot)
    {
        bEnableLight = true;
    }
    
    sAutomaticState.bStateReached = true;
    
    return bEnableLight;
}


//********************************************************************************
/*!
\author  KraemerE
\date    21.08.2020

\fn      StateAutomaticMode_3

\brief   Automatic mode 3 - Light is only switched when a motion was detected.
                            Timeslots are disabled. 
\param   none

\return  none
***********************************************************************************/
static bool StateAutomaticMode_3(void)
{    
    bool bEnableLight = OFF;
    
    /* Change actual state */
    sAutomaticState.eCurrentState = eStateAutomaticMode_3;

    const tsAutomaticModeValues* psAutoValues = Aom_System_GetAutomaticModeValuesStruct();    
    bEnableLight = psAutoValues->bMotionDetected;
    
    sAutomaticState.bStateReached = true;
    
    return bEnableLight;
}




/************************ externally visible functions ***********************/

//********************************************************************************
/*!
\author  KraemerE
\date    21.08.2020

\fn      AutomaticMode_Handler

\brief   Calls the linked automatic-function. Checks afterwards for the next state.

\param   none

\return  bLightEnabled - Returns the light on/off state
***********************************************************************************/
bool AutomaticMode_Handler(void)
{    
    bool bLightEnabled = OFF;
    
    /* Call the actual standby function */
    if(sAutomaticState.pFctState)
    {
        bLightEnabled = sAutomaticState.pFctState();
    }
    /* State active reached successfully */
    else
    {
        sAutomaticState.eCurrentState = eStateDisabled;        
        sAutomaticState.bStateReached = true;
    }
    
    /* Get the next state */
    CheckForNextState();
    
    return bLightEnabled;
}


//********************************************************************************
/*!
\author  KraemerE
\date    21.08.2020

\fn      AutomaticMode_ChangeState

\brief   Set a change request for the new automatic mode

\param   eRequestedState - The requested state which should be switched to.

\return  none
***********************************************************************************/
void AutomaticMode_ChangeState(teAutomaticState eRequestedState)
{
    /* Save requested state */
    sAutomaticState.eReqState = eRequestedState;
}


//********************************************************************************
/*!
\author  KraemerE
\date    21.08.2020

\fn      AutomaticMode_GetStandbyState

\brief   Returns the acutal automatic mode state

\param   none

\return  teAutomaticState - Returns the current state 
***********************************************************************************/
teAutomaticState AutomaticMode_GetAutomaticState(void)
{
    return sAutomaticState.eCurrentState;
}


//********************************************************************************
/*!
\author  KraemerE
\date    09.09.2020
\fn         AutomaticMode_Tick
\brief      Handles tick value and decrement the burning time when possible.
            (Overwriting the value is done in critical section)
\param      uiMsTick - The elapsed tick since the last time
\return     none
***********************************************************************************/
void AutomaticMode_Tick(u16 uiMsTick)
{
    tsAutomaticModeValues* psAutomaticModeValues = Aom_System_GetAutomaticModeValuesStruct();
    
    /* Check if burning time shall be decremented */
    if(psAutomaticModeValues->slBurningTimeMs)
    {
        /* Create a local copy of the burning time */
        s32 slBurningTimeCopy = psAutomaticModeValues->slBurningTimeMs;
                
        /* Check if subtraction is still possible */
        if((s32)(slBurningTimeCopy - uiMsTick) >= 0)
        {
            slBurningTimeCopy -= uiMsTick;
        }
        else
        {
            slBurningTimeCopy = 0;
        }        
        
        /* Enter critical section and overwrite the burning time value */
        const u8 ucCriticalSection = CyEnterCriticalSection();
     
        psAutomaticModeValues->slBurningTimeMs = slBurningTimeCopy;        
        psAutomaticModeValues->bMotionDetected = slBurningTimeCopy ? true : false;
        
        CyExitCriticalSection(ucCriticalSection);
    }
}

//********************************************************************************
/*!
\author  KraemerE
\date    09.09.2020
\fn      AutomaticMode_ResetBurningTimeout
\brief   Resets the burning time value from the automatic mode structure
\param   none
\return  none
***********************************************************************************/
void AutomaticMode_ResetBurningTimeout(void)
{
    tsAutomaticModeValues* psAutomaticModeValues = Aom_System_GetAutomaticModeValuesStruct();
    const tRegulationValues* psRegulationValues = Aom_Regulation_GetRegulationValuesPointer();
    
    /* Convert minutes value into milliseconds value (x 60.000 or bitshift by 16 (=65.536) */
    s32 slBurningTimeMs = psRegulationValues->sUserTimerSettings.ucBurningTime << 16;
    slBurningTimeMs -= 5536;
    
    /* Enter critical section and overwrite burning time variable */
    const u8 ucCriticalSection = CyEnterCriticalSection();
    psAutomaticModeValues->slBurningTimeMs = slBurningTimeMs;
    
    CyExitCriticalSection(ucCriticalSection);    
}
