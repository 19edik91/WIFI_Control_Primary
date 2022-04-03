/*
 ============================================================================
 Name        : BasicOS.c
 Author      : Eduard Kraemer
 Version     :
 Copyright   : No Copyrights
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include "project.h"
#include <stdio.h>
#include <stdlib.h>
#include "OS_Watchdog.h"
#include "OS_SelfTest.h"
#include "OS_EventManager.h"
#include "OS_StateManager.h"
#include "OS_State_Main.h"
#include "OS_SoftwareTimer.h"
#include "OS_ErrorDebouncer.h"

#include "HAL_Timer.h"

#define LOG_NOT_PROCESSED_EVTS  true

static tsEventMsg sEvt = {eEvtNone, eEvtParam_None, eEvtParam_None};
static tsEventMsg sNotProcessedEvents[10];
static u8 ucNotProcessedEvtIndex = 0;

void CyBoot_Start_c_Callback(void)
{
    OS_SelfTest_StartCallback();
}

static void ClearEventStruct(tsEventMsg* psEvt)
{
    psEvt->eEventID = eEvtNone;
    psEvt->param1 = eEvtParam_None;
    psEvt->param2 = eEvtParam_None;
}

static void Main_Task(void)
{   
    /* Check if event was handled */
    if(sEvt.eEventID == eEvtNone)
    {
        /* Get new event */
        OS_EVT_GetEvent(&sEvt);
    }

    /* Check if new or old event is available */
    if(sEvt.eEventID != eEvtNone)
    {
        /* Call main state first. Afterwards the events can be used by
         * the other states. */
        u8 ucMainStateProcessed = OS_State_Main(sEvt.eEventID, sEvt.param1, sEvt.param2);
        u8 ucStateProcessed = OS_StateManager_Handle(sEvt.eEventID, sEvt.param1, sEvt.param2);

        /* Use state machine handler */
        if(ucMainStateProcessed == EVT_PROCESSED || ucStateProcessed == EVT_PROCESSED)
        {
            ClearEventStruct(&sEvt);
        }
        else
        {
            #if LOG_NOT_PROCESSED_EVTS
                if(ucNotProcessedEvtIndex < 10)
                {
                    ucNotProcessedEvtIndex++;
                }
                else
                {
                    ucNotProcessedEvtIndex = 0;
                }
                
                memcpy(&sNotProcessedEvents[ucNotProcessedEvtIndex], &sEvt, sizeof(tsEventMsg));
                
                ClearEventStruct(&sEvt);
            #else
                /* Event has not been processed from main-state nor from other states */
                OS_ErrorDebouncer_PutErrorInQueue(eEventError_NotProcessed);
            #endif
        }

    }
    else
    {
        /* Currently no event. use Self-test routine */
        OS_SelfTest_Cyclic_Run();
    }

    /* Clear watchdog counter */
    OS_WDT_ClearWatchdogCounter();
}

/* Main is called after start-up and therefore after the start-up
 * self test.
 */
int main(void)
{
    /* Initialize self-tests */
    OS_SelfTest_InitCyclic();

    /* Initialize state manager */
    OS_StateManager_Init();
    
    /* Initialize the Watchdog with 2 second intervall */
    OS_WDT_InitWatchdog(2000);

    /* Start with first event */
    OS_EVT_PostEvent(eEvtEnterResetState, 0, 0);
    
    /* Enable global interrupts. */
    CyGlobalIntEnable; 

    /* Do the main-task forever */
    for(;;)
    {
        Main_Task();
    }

    //This shall never be reached
    return EXIT_FAILURE;
}
