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
#include "Aom_Time.h"
#include "DR_Measure.h"
#include "OS_Config.h"

/***************************** defines / macros ******************************/
#define NIGHT_MODE_START        22
#define NIGHT_MODE_STOP         5

#define ENABLE_FAST_STANDBY     true
/************************ local data type definitions ************************/
typedef bool (*pbFunction)(void);

typedef struct
{
    pbFunction pFctState;
    teAutomaticState eCurrentState;    
}tsAutomaticState;


/************************* local function prototypes *************************/
static bool StateAutomaticMode_1(void);
static bool StateAutomaticMode_2(void);
static bool StateAutomaticMode_3(void);


/************************* local data (const and var) ************************/
tsAutomaticState sAutomaticState = 
{
    .eCurrentState = eStateDisabled,
    .pFctState = NULL,
};


/****************************** local functions ******************************/
//********************************************************************************
/*!
\author     Kraemer E
\date       01.12.2019
\fn         IsCurrentTimeInActiveTimeSlot
\brief      Checks if the current time is in the specific automatic on time
\return     bInSlot - Returns true when in time slot
\param      ucHours - The current hour
\param      ucMin - The current minutes
***********************************************************************************/
static bool IsCurrentTimeInActiveTimeSlot(u8 ucHours, u8 ucMin)
{
    bool bInSlot = false;
    u8 ucSlotsFound = 0;
    
    tRegulationValues sRegulationValues;       
    Aom_Regulation_GetRegulationValues(&sRegulationValues);

    for(u8 ucTimerIdx = 0; ucTimerIdx < USER_TIMER_AMOUNT; ucTimerIdx++)
    {
        if(sRegulationValues.sUserTimerSettings.ucSetTimerBinary  & (0x01 << ucTimerIdx))
        {       
            if((ucHours == sRegulationValues.sUserTimerSettings.sTimer[ucTimerIdx].ucHourSet) && (ucMin >= sRegulationValues.sUserTimerSettings.sTimer[ucTimerIdx].ucMinSet))
            {
                ucSlotsFound++;
            }
            else if(ucHours > sRegulationValues.sUserTimerSettings.sTimer[ucTimerIdx].ucHourSet)
            {
                ucSlotsFound++;
            }
        
            if(ucHours == sRegulationValues.sUserTimerSettings.sTimer[ucTimerIdx].ucHourClear && ucMin >= sRegulationValues.sUserTimerSettings.sTimer[ucTimerIdx].ucMinClear)
            {
                ucSlotsFound--;
            }
            else if(ucHours > sRegulationValues.sUserTimerSettings.sTimer[ucTimerIdx].ucHourClear)
            {
                ucSlotsFound--;
            }
        }
    }

    if(ucSlotsFound > 0)
    {
        bInSlot = true;
    }
      
    return bInSlot;
}

//********************************************************************************
/*!
\author     Kraemer E
\date       21.08.2020
\fn         IsCurrentTimeInNightModeTimeSlot
\brief      Checks if the current time is in the specific night mode time slot
\return     bInSlot - Returns true when in time slot
\param      ucHours - The current hour
***********************************************************************************/
static bool IsCurrentTimeInNightModeTimeSlot(u8 ucHours)
{
    bool bInSlot = false;
    u8 ucSlotsFound = 0;
      
    if(ucHours >= NIGHT_MODE_START)
    {
        ucSlotsFound++;
    }

    if(ucHours >= NIGHT_MODE_STOP)
    {
        ucSlotsFound--;
    }

    if(ucSlotsFound > 0)
    {
        bInSlot = true;
    }
      
    return bInSlot;
}


//********************************************************************************
/*!
\author  KraemerE
\date    21.08.2020
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
    
    return bEnableLight;
}

//********************************************************************************
/*!
\author  KraemerE
\date    21.08.2020
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
    
    if((psAutoValues->bMotionDetected || psAutoValues->slBurningTimeMs) && psAutoValues->bInUserTimerSlot)
    {
        bEnableLight = true;
    }
    
    return bEnableLight;
}


//********************************************************************************
/*!
\author  KraemerE
\date    21.08.2020
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
    bEnableLight = (psAutoValues->bMotionDetected || psAutoValues->slBurningTimeMs);

    return bEnableLight;
}



/************************ externally visible functions ***********************/

//********************************************************************************
/*!
\author  KraemerE
\date    21.08.2020
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
    }
    return bLightEnabled;
}


//********************************************************************************
/*!
\author  KraemerE
\date    21.08.2020
\brief   Set a change request for the new automatic mode
\param   eRequestedState - The requested state which should be switched to.
\return  none
***********************************************************************************/
void AutomaticMode_ChangeState(teAutomaticState eRequestedState)
{
    /* Change callback for dependent state */
    switch(eRequestedState)
    {
        case eStateDisabled:
        {
            sAutomaticState.pFctState = NULL;
            break;
        }
        
        case eStateAutomaticMode_1:
        {
            sAutomaticState.pFctState = StateAutomaticMode_1;
            break;
        }
        
        case eStateAutomaticMode_2:
        {
            sAutomaticState.pFctState = StateAutomaticMode_2;
            break;
        }
        
        case eStateAutomaticMode_3:
        {
            sAutomaticState.pFctState = StateAutomaticMode_3;
            break;
        }
        
        
        default:
            break;
    } 
}


//********************************************************************************
/*!
\author  KraemerE
\date    21.08.2020
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
        const u8 ucCriticalSection = EnterCritical();     
        
        psAutomaticModeValues->slBurningTimeMs = slBurningTimeCopy;     
        
        LeaveCritical(ucCriticalSection);
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
    
    #if ENABLE_FAST_STANDBY
        s32 slBurningTimeMs = 15000;    //15 seconds as burning intervall
    #else
        const tRegulationValues* psRegulationValues = Aom_Regulation_GetRegulationValuesPointer();
        
        /* Convert minutes value into milliseconds value (x 60.000 or bitshift by 16 (=65.536) */
        s32 slBurningTimeMs = psRegulationValues->sUserTimerSettings.ucBurningTime << 16;
        slBurningTimeMs -= 5536;
    #endif
        
    /* Enter critical section and overwrite burning time variable */
    const u8 ucCriticalSection = EnterCritical();
    psAutomaticModeValues->slBurningTimeMs = slBurningTimeMs;
    
    LeaveCritical(ucCriticalSection);    
}


//********************************************************************************
/*!
\author  KraemerE
\date    09.05.2021
\fn      AutomaticMode_LeaveStandbyMode
\brief   Checks if the standby mode can be left according to the currently used 
         automatic mode
\param   none
\return  bLeaveStandbyMode - True when automatic mode requests to leave the standby
                             mode.
***********************************************************************************/
bool AutomaticMode_LeaveStandbyMode(void)
{
    bool bLeaveStandbyMode = false;

    if((sAutomaticState.eCurrentState == eStateAutomaticMode_2)
        || (sAutomaticState.eCurrentState == eStateAutomaticMode_3))
    {
        bLeaveStandbyMode = true;    
    }
    
    return bLeaveStandbyMode;    
}

//********************************************************************************
/*!
\author  KraemerE
\date    11.05.2021
\brief   Checks if an automatic mode is active and checks if the current time
         is in the user defined time slot. Also checks if the night mode shall be
         switched on.
\param   none
\return  none
***********************************************************************************/
void AutomaticMode_TimeUpdated(void)
{
    /* Get pointer to the regulation structure (read-only) */
    const tRegulationValues* psRegVal = Aom_Regulation_GetRegulationValuesPointer();
    tsAutomaticModeValues* psAutoMode = Aom_System_GetAutomaticModeValuesStruct();
    const tsCurrentTime* psTime = Aom_Time_GetCurrentTime();
    
    /* Check if automatic mode is enabled. Otherwise handling isn't relevant */
    if(psRegVal->sUserTimerSettings.bAutomaticModeActive)
    {
        psAutoMode->bInUserTimerSlot = IsCurrentTimeInActiveTimeSlot(psTime->ucHours, psTime->ucMinutes);
        
        /* Check if night mode is active and in night mode time slot */
        if(psRegVal->bNightModeOnOff)
        {
            psAutoMode->bInNightModeTimeSlot = IsCurrentTimeInNightModeTimeSlot(psTime->ucHours);
        }                  
    }
}
