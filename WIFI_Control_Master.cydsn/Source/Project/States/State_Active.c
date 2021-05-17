


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

#include "AutomaticMode.h"


/***************************** defines / macros ******************************/
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
static u8 ucSW_Timer_EspReset = 0;

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
\author     Kraemer E.
\date       26.10.2020
\brief      Resets slave after a communication fault.
\param      bReset - true for activating reset otherwise false
\return     none
***********************************************************************************/
static void ResetSlaveByTimeout(bool bReset)
{
    if(bReset == true)
    {
        //Start timeout
        OS_SW_Timer_SetTimerState(ucSW_Timer_EspReset, TM_RUNNING);
    }
    
    DR_Regulation_SetEspResetStatus(bReset);
    MessageHandler_ClearAllTimeouts();
    Aom_System_SetSlaveResetState(bReset);
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       06.05.2021
\brief      Timeout callback function which enables the slave again.
\param      none
\return     none
***********************************************************************************/
static void EnableSlaveTimeout(void)
{
    ResetSlaveByTimeout(false);
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
    ucSW_Timer_EspReset = OS_SW_Timer_CreateAsyncTimer(RESET_CTRL_TIMEOUT, TMF_CREATESUSPENDED, EnableSlaveTimeout);
    
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
    u8 ucReturn = EVT_PROCESSED;
    
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
                    if(TM_SUSPENDED == ucTimerState && Aom_System_StandbyAllowed())
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
                
                /* Handle message in the retry buffer */
                MessageHandler_Tick(SW_TIMER_51MS);
            }
            
            /******* 251ms-Tick **********/
            else if(ulParam2 == EVT_SW_TIMER_251MS)
            {
                DR_Regulation_CheckSensorForMotion();
            }
            
            /******* 1001ms-Tick **********/
            else if(ulParam2 == EVT_SW_TIMER_1001MS)
            {                        
                AutomaticMode_Tick(SW_TIMER_1001MS);
                                
                /* Toggle LED to show a living CPU */
                DR_Regulation_ToggleHeartBeatLED();
                
                /* Toggle error LED when an error is in timeout */
                DR_Regulation_ToggleErrorLED();

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
            const tsCurrentTime* psTime = Aom_Time_GetCurrentTime();
            
            /* Update RTC time with the received epoch time from NTP-Client */                        
            if(uiParam1 == eEvtParam_TimeFromNtpClient)
            {
                OS_RealTimeClock_SetTime(psTime->ulTicks);
            }
            
            /* Check for handling in automatic mode */
            AutomaticMode_TimeUpdated();
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
            if(DR_Regulation_GetEspResetStatus() == false)
            {
                ResetSlaveByTimeout(true);
                Aom_System_SetSystemStarted(false);
            }
            break;
        }
        
        
        case eEvtAutomaticMode_ResetBurningTimeout:
        {
            AutomaticMode_ResetBurningTimeout();
            break;
        }
        
        default:
            ucReturn = EVT_NOT_PROCESSED;
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
    OS_SW_Timer_DeleteTimer(ucSW_Timer_2ms);
    OS_SW_Timer_DeleteTimer(ucSW_Timer_10ms);
    OS_SW_Timer_DeleteTimer(ucSW_Timer_FlashWrite);
    OS_SW_Timer_DeleteTimer(ucSW_Timer_EnterStandby);
    OS_SW_Timer_DeleteTimer(ucSW_Timer_EspReset);
    
    /* Switch state to root state */
    OS_StateManager_CurrentStateReached();

    return ucReturn;
}


