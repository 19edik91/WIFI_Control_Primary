


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

/***************************** defines / macros ******************************/

/************************ local data type definitions ************************/

/************************* local function prototypes *************************/

/************************* local data (const and var) ************************/
static u8 ucActiveOutputs = 0;


/************************ export data (const and var) ************************/


/****************************** local functions ******************************/


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
        case eEvtEnterResetState:
        {
            /* Initialize software timer */
            OS_SW_Timer_Init();

            /* Initialize UART Module */
            OS_Serial_UART_Init();

            /* Initialize Error debouncer */
            OS_ErrorDebouncer_Initialize();

            /* Initialize RTC */
            OS_RealTimeClock_Init();

            /* Create necessary software timer */
            OS_EVT_PostEvent(eEvtSoftwareTimerCreate, TMF_PERIODIC, SW_TIMER_2MS);
            OS_EVT_PostEvent(eEvtSoftwareTimerCreate, TMF_PERIODIC, SW_TIMER_10MS);

            /* Switch state to reset state */
            OS_StateManager_CurrentStateReached();
            break;
        }

        default:
            ucReturn = EVT_NOT_PROCESSED;
            break;
    }

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
            if(ulParam2 == EVT_SW_TIMER_2MS)
            {
                DR_Measure_Tick();
                //ucActiveOutputs = 
            }
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
    
    OS_StateManager_CurrentStateReached();

    return ucReturn;
}


