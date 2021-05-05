


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

/***************************** defines / macros ******************************/

/************************ local data type definitions ************************/

/************************* local function prototypes *************************/

/************************* local data (const and var) ************************/
static u8 ucActiveOutputs = 0;
static bool bModulesInit = false;

static u8 ucSW_Timer_2ms = 0;
static u8 ucSW_Timer_10ms = 0;
static u8 ucSW_Timer_FlashWrite = 0;
static u8 ucSW_Timer_EnterStandby = 0;

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
    OS_EVT_PostEvent(eEvtSoftwareTimerCreate, TMF_PERIODIC, SW_TIMER_2MS);
    OS_EVT_PostEvent(eEvtSoftwareTimerCreate, TMF_PERIODIC, SW_TIMER_10MS);

    /* Create async timer */           
    OS_EVT_PostEvent(eEvtSoftwareTimerAsyncCreate, SAVE_IN_FLASH_TIMEOUT, (ulEventParam2)TimeoutFlashUserSettings);
    OS_EVT_PostEvent(eEvtSoftwareTimerAsyncCreate, TIMEOUT_ENTER_STANDBY, (ulEventParam2)RequestStandbyState);
    
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
            //2ms-Tick
            if(ulParam2 == EVT_SW_TIMER_2MS)
            {
                DR_Measure_Tick();
                ucActiveOutputs = DR_Regulation_Handler();
            }
            else if(ulParam2 == EVT_SW_TIMER_10MS)
            {
                /* Get standby-timer state */
                u8 ucTimerState = OS_SW_Timer_GetTimerState(
            }
            
            //51Ms-Tick
            else if(ulParam2 == EVT_SW_TIMER_51MS)
            {
                /* Check for over-current faults */
                DR_ErrorDetection_CheckCurrentValue();
                
                /* Check for over-temperature faults */
                DR_ErrorDetection_CheckAmbientTemperature();
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


