


/********************************* includes **********************************/
#include "OS_State_Reset.h"
#include "OS_StateManager.h"
#include "OS_SoftwareTimer.h"
#include "OS_Serial_UART.h"
#include "OS_ErrorDebouncer.h"
#include "OS_RealTimeClock.h"

#include <project.h>

#include "DR_ErrorDetection.h"
#include "DR_Measure.h"
#include "DR_Regulation.h"
#include "Aom_Flash.h"
#include "MessageHandler.h"

#include "Aom_System.h"
#include "Aom_Regulation.h"
#include "Aom_Measure.h"
#include "Aom_Time.h"

#warning Includes pruefen!
#include "HAL_IO.h"
#include "AutomaticMode.h"
#include "Standby.h"

/***************************** defines / macros ******************************/
#define NIGHT_MODE_START        22
#define NIGHT_MODE_STOP         5

#define RESET_CTRL_TIMEOUT      3000   //3 sec for reset timeout

/************************ local data type definitions ************************/

/************************* local function prototypes *************************/

/************************* local data (const and var) ************************/
static u8 ucActiveOutputs = 0;
static bool bModulesInit = false;

static u8 ucSW_Timer_2ms = 0;
static u8 ucSW_Timer_10ms = 0;
static u8 ucSW_Timer_FlashWrite = 0;
static u8 ucSW_Timer_EnterStandby = 0;

/* Communication timeout variables */
static u16 uiResetCtrlTimeout = 0;


/************************ export data (const and var) ************************/


/****************************** local functions ******************************/
//***************************************************************************
/*!
\author     KraemerE
\date       04.05.2021
\brief      Timeout callback for async timer. Saves the current user settings
            into the flash.
\return     none
\param      none
******************************************************************************/
void TimeoutFlashUserSettings(void)
{
    Aom_Flash_WriteUserSettingsInFlash();
}


//***************************************************************************
/*!
\author     KraemerE
\date       04.05.2021
\brief      Requests a state change to the standby state.
\return     none
\param      none
******************************************************************************/
void RequestStandbyState(void)
{
    OS_EVT_PostEvent(eEvtState_Request, eSM_State_Standby, 0);
}


//********************************************************************************
/*!
\author     KraemerE   
\date       12.02.2019  
\brief      Just toggles the LED
\return     none
\param      none
***********************************************************************************/
void ToggleLed(void)
{
    bool bActualStatus = HAL_IO_ReadOutputStatus(eLedGreen);
    bActualStatus ^= 0x01;
    HAL_IO_SetOutputStatus(eLedGreen, bActualStatus);
    
    #ifdef Pin_DEBUG_0
    Pin_DEBUG_Write(~Pin_DEBUG_Read());
    #endif
}
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
bool IsCurrentTimeInActiveTimeSlot(u8 ucHours, u8 ucMin)
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
bool IsCurrentTimeInNightModeTimeSlot(u8 ucHours)
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
\author     Kraemer E.
\date       26.10.2020
\fn         ResetSlaveByTimeout()
\brief      Resets slave after a communication fault.
\param      bReset - true for activating reset otherwise false
\return     none
***********************************************************************************/
static void ResetSlaveByTimeout(bool bReset)
{
    /* Allow changes only when reset ctrl timeout is 0 */
    if(uiResetCtrlTimeout == 0)
    {
        HAL_IO_SetOutputStatus(eEspResetPin, !bReset);
        uiResetCtrlTimeout = RESET_CTRL_TIMEOUT;
        MessageHandler_ClearAllTimeouts();
        Aom_System_SetSlaveResetState(bReset);
    }
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       26.10.2020
\fn         CheckResetTimeout
\brief      Checks if the reset ctrl timeout is running and decrements it
            until zero. When the timeout is zero the reset pin is set to default
\param      ucElapsedTime - The elapsed time since the last call
\return     none
***********************************************************************************/
static inline void CheckResetTimeout(u16 uiElapsedTime)
{
    /* Decrement Timeout */
    s16 siDiff = uiResetCtrlTimeout - uiElapsedTime;
    
    if(siDiff > 0)
    {
        uiResetCtrlTimeout -= uiElapsedTime;
    }
    else
    {
        uiResetCtrlTimeout = 0;    
    
        /* When reset pin is in "Reset set" state, put it back */
        if(HAL_IO_ReadOutputStatus(eEspResetPin) == OFF)
        {
            ResetSlaveByTimeout(false);
        }
    }
}
/************************ externally visible functions ***********************/
//***************************************************************************
/*!
\author     KraemerE
\date       30.04.2021
\brief      Entry state of this state.
\return     u8
\param      eEventID - Event which shall be handled while in this state.
\param      uiParam1 - Event parameter from the received event
\param      ulParam2 - Second event parameter from the received event
******************************************************************************/
u8 State_Active_Entry(teEventID eEventID, uiEventParam1 uiParam1, ulEventParam2 ulParam2)
{
    /* No events expected in this state */
    u8 ucReturn = EVT_PROCESSED;

    /* Make compiler happy */
    (void)uiParam1;
    (void)ulParam2;
    
    switch(eEventID)
    {
        default:
            ucReturn = EVT_NOT_PROCESSED;
            break;
    }

    /* Create necessary software timer */      
    ucSW_Timer_2ms = OS_SW_Timer_CreateTimer(SW_TIMER_2MS, TMF_PERIODIC);
    ucSW_Timer_10ms = OS_SW_Timer_CreateTimer(SW_TIMER_10MS, TMF_PERIODIC);
    
    /* Create async timer */
    ucSW_Timer_EnterStandby = OS_SW_Timer_CreateAsyncTimer(TIMEOUT_ENTER_STANDBY, TMF_CREATESUSPENDED, RequestStandbyState);
    ucSW_Timer_FlashWrite = OS_SW_Timer_CreateAsyncTimer(SAVE_IN_FLASH_TIMEOUT, TMF_CREATESUSPENDED, TimeoutFlashUserSettings);
        
    if(bModulesInit == false)
    {
        MessageHandler_Init();
        DR_Measure_Init();
        DR_Regulation_Init();
        bModulesInit = true;
    }
    
    /* Switch state to root state */
    OS_StateManager_CurrentStateReached();

    return ucReturn;
}


//***************************************************************************
/*!
\author     KraemerE
\date       30.04.2021
\brief      Entry state of this state.
\return     u8
\param      eEventID - Event which shall be handled while in this state.
\param      uiParam1 - Event parameter from the received event
\param      ulParam2 - Second event parameter from the received event
******************************************************************************/
u8 State_Active_Root(teEventID eEventID, uiEventParam1 uiParam1, ulEventParam2 ulParam2)
{
    u8 ucReturn = EVT_NOT_PROCESSED;
    
    switch(eEventID)
    {
        case eEvtSoftwareTimer:
        {
            /******* 2ms-Tick **********/
            if(ulParam2 == EVT_SW_TIMER_2MS)
            {
                DR_Measure_Tick();
                ucActiveOutputs = DR_Regulation_Handler();
            }
            
            /******* 10ms-Tick **********/
            else if(ulParam2 == EVT_SW_TIMER_10MS)
            {
                /* Get standby-timer state */
                u8 ucTimerState = OS_SW_Timer_GetTimerState(ucSW_Timer_EnterStandby);
                
                /* Start the timeout for the standby when all regulation states are off */
                if(ucActiveOutputs == 0)
                {
                    if(TM_SUSPENDED == ucTimerState && (Aom_System_IsStandbyActive() == false) && Aom_System_StandbyAllowed())
                    {
                        /* Start the timeout for the standby timeout */
                        OS_SW_Timer_SetTimerState(ucSW_Timer_EnterStandby, TM_RUNNING);
                    }
                }
                else if(TM_RUNNING == ucTimerState)
                {
                    /* Stop standby timeout when its counting */
                    OS_SW_Timer_SetTimerState(ucSW_Timer_EnterStandby, TM_SUSPENDED);
                }
            }
            
            /******* 51ms-Tick **********/
            else if(ulParam2 == EVT_SW_TIMER_51MS)
            {
                /* Check for over-current faults */
                DR_ErrorDetection_CheckCurrentValue();
                
                /* Check for over-temperature faults */
                DR_ErrorDetection_CheckAmbientTemperature();
            }
            
            /******* 251ms-Tick **********/
            else if(ulParam2 == EVT_SW_TIMER_251MS)
            {
                /* Read PIR out for possible motion detection */                        
                const tRegulationValues* psReg = Aom_Regulation_GetRegulationValuesPointer();                        
                if(psReg->sUserTimerSettings.bMotionDetectOnOff)
                {                            
                    /* Check if PIR has detected a change */
                    if(HAL_IO_ReadDigitalSense(eSensePIR))
                    {
                        /* Reset "ON" timeout */
                        AutomaticMode_ResetBurningTimeout();
                    }
                }                
            }
            
            /******* 1001ms-Tick **********/
            else if(ulParam2 == EVT_SW_TIMER_1001MS)
            {                        
                AutomaticMode_Tick(1001);
                
                /* Set ESP-reset pin to open drain state */
                CheckResetTimeout(1001);
                
                /* Toggle LED to show a living CPU */
                ToggleLed();                

                /* Calculate voltage, current and temperature and send them afterwards to the slave */
                Aom_Measure_SetMeasuredValues(true, true, true);
                
                /* Check first if the slave is active before sending a request */
                if(Aom_System_GetSystemStarted())
                {
                    MessageHandler_SendOutputState();
                }
            }            
            break;
        }//eEvtSoftwareTimer
        
        case eEvtNewRegulationValue:
        {
            /* Start the timeout for saving the requested values into flash */
            if(uiParam1 == eEvtParam_RegulationValueStartTimer)
            {
                /* Restart the flash timeout */
                OS_SW_Timer_SetTimerState(ucSW_Timer_FlashWrite, TM_RUNNING);
            }
            else if(uiParam1 == eEvtParam_RegulationStart)
            {
                DR_Regulation_ChangeState(eStateActiveR, (u8)ulParam2);
            }
            else if(uiParam1 == eEvtParam_RegulationStop)
            {
                DR_Regulation_ChangeState(eStateOff, (u8)ulParam2);
            }
            break;
        }
        
        case eEvtTimeReceived:
        {
            /* Get pointer to the regulation structure (read-only) */
            const tRegulationValues* psRegVal = Aom_Regulation_GetRegulationValuesPointer();
            tsAutomaticModeValues* psAutoMode = Aom_System_GetAutomaticModeValuesStruct();
            const tsCurrentTime* psTime = Aom_Time_GetCurrentTime();
            
            /* Update RTC time with the received epoch time from NTP-Client */                        
            if(uiParam1 == eEvtParam_TimeFromNtpClient)
            {
                OS_RealTimeClock_SetTime(psTime->ulTicks);
            }
            
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
            break;
        }
                
        case eEvtSendError:
        {
            /* Send error message to the POD controller */ 
            MessageHandler_SendFaultMessage(uiParam1);
            break;
        }        
        
        case eEvtCommTimeout:
        {
            /* When the reset pin is in high state, put it to low and reset the slave */
            if(HAL_IO_ReadOutputStatus(eEspResetPin) == ON)
            {
                ResetSlaveByTimeout(true);
                Aom_System_SetSystemStarted(false);
            }
            break;
        }
        
        default:
            break;
    }
    
    
    OS_StateManager_CurrentStateReached();

    return ucReturn;
}


//***************************************************************************
/*!
\author     KraemerE
\date       30.04.2021
\brief      Entry state of this state.
\return     u8
\param      eEventID - Event which shall be handled while in this state.
\param      uiParam1 - Event parameter from the received event
\param      ulParam2 - Second event parameter from the received event
******************************************************************************/
u8 State_Active_Exit(teEventID eEventID, uiEventParam1 uiParam1, ulEventParam2 ulParam2)
{
    u8 ucReturn = EVT_NOT_PROCESSED;
    
    /* Make compiler happy */
    (void)eEventID;
    (void)uiParam1;
    (void)ulParam2;
    
    /* Delete software timer which are related to this state */            
    OS_EVT_PostEvent(eEvtSoftwareTimerDelete, 0, SW_TIMER_2MS);
    OS_EVT_PostEvent(eEvtSoftwareTimerDelete, 0, SW_TIMER_10MS);
    OS_EVT_PostEvent(eEvtSoftwareTimerDelete, 0, SAVE_IN_FLASH_TIMEOUT);
    OS_EVT_PostEvent(eEvtSoftwareTimerDelete, 0, TIMEOUT_ENTER_STANDBY);
    
    /* Switch state to root state */
    OS_StateManager_CurrentStateReached();

    return ucReturn;
}


