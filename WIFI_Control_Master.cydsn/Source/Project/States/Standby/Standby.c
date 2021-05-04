//********************************************************************************
/*!
\author     Kraemer E.
\date       06.04.2019

\file       Standby.c
\brief      Standby handling

***********************************************************************************/

#include "Standby.h"
#include <project.h>
#include "Aom_Regulation.h"
#include "DR_Measure.h"
#include "OS_Watchdog.h"
#include "OS_EventManager.h"
#include "OS_Serial_UART.h"
#include "HAL_IO.h"

/***************************** defines / macros ******************************/
#define STANDBY_DELAY_CALLS     4

#define ECO_USED    false

//Define for wake up time intervall in milliseconds
#if(CY_IP_SRSSV2)
    #define WAKEUP_TIME                 ((0x01 << WDT2_INTERVAL_USED)*1000)/ILO_FREQ
#else
    #define WAKEUP_TIME                 1024    //Don't know exactly yet
#endif

#define WAKEUP_TIME_START_MEASURE   60000  /* in milliseconds. Wake up time 
                                              when the LV side should be enabled
                                              and measured */
//Define for wake up counter in the standby state
#define MULTI                       2   /* Not sure why the WDT interrupt occurs two times
                                           in series */
#define WAKE_UP_CNT_LIMIT           ((WAKEUP_TIME_START_MEASURE)/(WAKEUP_TIME))*MULTI

#ifdef Pin_DEBUG_0
    #warning Debug standby active!
    #define DEBUG_PIN_ON    Pin_DEBUG_Write(ON);
    #define DEBUG_PIN_OFF   Pin_DEBUG_Write(OFF);
#else
    #define DEBUG_PIN_ON
    #define DEBUG_PIN_OFF
#endif

/************************ local data type definitions ************************/
typedef struct _tCStateDefinition
{
    pFunction   pFctEntrySystem;     /**< Pointer to the Entry function. This function will be called if the state is entered */
    pFunction   pFctEntryLowVoltage; /**< Pointer to the Entry function. This function will be called if the state is entered */
    pFunction   pFctExecute;         /**< Pointer to the Execution function. This function will be called for every message if the state is active or a child of this state is active. */
    pFunction   pFctExitLowVoltage;  /**< Pointer to the first Exit function of the state. This function is called if the state machine changes to an other state. */
    pFunction   pFctExitSystem;      /**< Pointer to the second Exit function of the state. This function is called if the state machine changes to an other state. */
}tCStateDefinition;

typedef struct
{
    teStandbyState eStandbyState;
    teStandbyState eNextState;
    teStandbyState eReqState;
    pFunction pFctState;
    bool bStateReached;
}tsStandbyState;


/************************* local function prototypes *************************/
static void StateEntrySystem(void);
static void StateEntryLowVoltage(void);
static void StateExecute(void);
static void StateExitLowVoltage(void);
static void StateExitSystem(void);



/************************* local data (const and var) ************************/
tsStandbyState sStandbyState = 
{
    .eStandbyState = eStateActive,
    .eReqState = eStateActive,
    .pFctState = NULL,
    .bStateReached = false
};

const tCStateDefinition sStandbyStateFn = 
{
    .pFctEntrySystem = StateEntrySystem, 
    .pFctEntryLowVoltage = StateEntryLowVoltage,
    .pFctExecute = StateExecute,
    .pFctExitLowVoltage = StateExitLowVoltage,
    .pFctExitSystem = StateExitSystem,
};

static u8 ucWakeUpCounter = WAKE_UP_CNT_LIMIT;

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
    /* Check if stand by should be left */
    if(sStandbyState.eReqState == eStateActive)
    {
        switch(sStandbyState.eStandbyState)
        {
            
            //Actual state is system entry
            case eStateEntrySystem:
            {
                //Next state should enable system again
                sStandbyState.eNextState = eStateExitSystem;
                break;
            }
            
            case eStateStandby:
            case eStateEntryLowVoltage:
            {
                //Next state should be to start over again
                sStandbyState.eNextState = eStateExitLowVoltage;
                break;
            }
            
            case eStateExitLowVoltage:
            {
                //Next state should restart the system
                sStandbyState.eNextState = eStateExitSystem;
                break;
            }
            
            case eStateExitSystem:
            {
                //Leave standby complete
                sStandbyState.eNextState = eStateActive;
                break;
            }
            
            case eStateActive:
            default:
                break;
        }
    }
    /* Check if standby doesn't reached the requested state yet */
    else if(sStandbyState.eStandbyState != sStandbyState.eReqState)
    {
        switch(sStandbyState.eStandbyState)
        {
            case eStateActive:
            {
                //Next state should disable the system
                sStandbyState.eNextState = eStateEntrySystem;
                break;
            }
            
            case eStateEntrySystem:
            {
                //Next state should disable low voltage peripherals 
                sStandbyState.eNextState = eStateEntryLowVoltage;
                break;
            }
            
            case eStateEntryLowVoltage:
            {
                //Next state is the standby handling
                sStandbyState.eNextState = eStateStandby;
                break;
            }
            
            case eStateStandby:
            {
                //Looks like a request for the next input measurement. Therefore the
                //LV peripherals needs to be enabled again
                sStandbyState.eNextState = eStateExitLowVoltage;
                break;
            }
            
            
            case eStateExitLowVoltage:
            {                
                //Measurements are done. Disable LV peripherals in the next state.
                sStandbyState.eNextState = eStateEntryLowVoltage;
                break;
            }
            
            //This one isn't necessary because in this case the above if-statement would be used.
            //case eStateExitSystem:            
            default:   
                break;
        }
    }
    
    /* When the requested state wasn't reached yet swtich the function pointer according the next state */
    if(sStandbyState.eReqState != sStandbyState.eStandbyState
        && sStandbyState.bStateReached == true)
    {
        /* Change callback for dependent state */
        switch(sStandbyState.eNextState)
        {
            case eStateActive:
            {
                sStandbyState.pFctState = NULL;
                break;
            }
            
            case eStateEntrySystem:
            {
                sStandbyState.pFctState = sStandbyStateFn.pFctEntrySystem;
                break;
            }
            
            case eStateEntryLowVoltage:
            {
                sStandbyState.pFctState = sStandbyStateFn.pFctEntryLowVoltage;
                break;
            }
            
            case eStateStandby:
            {
                sStandbyState.pFctState = sStandbyStateFn.pFctExecute;
                break;
            }
            
            case eStateExitLowVoltage:
            {
                sStandbyState.pFctState = sStandbyStateFn.pFctExitLowVoltage;
                break;
            }
            
            case eStateExitSystem:
            {
                sStandbyState.pFctState = sStandbyStateFn.pFctExitSystem;
                break;
            }
            
            default:
                break;
        } 
        
        /* Clear bit for next state */
        sStandbyState.bStateReached = false;
    }
}

//********************************************************************************
/*!
\author  KraemerE
\date    19.02.2019

\fn      ModulesSleep

\brief   Puts the modules into sleep prepared mode 

\param   none

\return  none
***********************************************************************************/
static void ModulesSleep(void)
{        
    /* Put into sleep mode */
    ADC_INPUT_Sleep();
    
    //System clock
    System_Timer_Sleep();
    Clock_1_Stop();
    Millisecond_ISR_Disable();
    
    //PWM clock
    //PWM_Sleep();
    PWM_Clock_Stop();
        
    //UART is set to sleep in
    //"Serial_EnableUartWakeupInSleep()"        
}

//********************************************************************************
/*!
\author  KraemerE
\date    19.02.2019

\fn      ModulesWakeup

\brief   Wakes up the modules from sleep mode 

\param   none

\return  none
***********************************************************************************/
static void ModulesWakeup(void)
{        
    /* Wake up from sleep mode */
    ADC_INPUT_Wakeup();
    
    //System clock
    System_Timer_Wakeup();
    Clock_1_Start();
    Millisecond_ISR_Enable();
    
    //PWM clock
    //PWM_Wakeup();
    PWM_Clock_Start();
       
    //UART is set to wakeup in
    //"Serial_DisableUartWakeupInSleep()"
}

//********************************************************************************
/*!
\author  KraemerE
\date    18.02.2019

\fn      StateEntrySystem

\brief   Enter function of the standby process. In this module all necessary
         system related peripherals should be switched off.

\param   none

\return  none
***********************************************************************************/
static void StateEntrySystem(void)
{
    /* Change actual state */
    sStandbyState.eStandbyState = eStateEntrySystem;
    
    sStandbyState.bStateReached = true;
}




//********************************************************************************
/*!
\author  KraemerE
\date    18.02.2019

\fn      StateEntry

\brief   Second enter function of the standby process. In this module all necessary
         low voltage related peripherals, timers and so on should be switched off.

\param   none

\return  none
***********************************************************************************/
static void StateEntryLowVoltage(void)
{    
    /* Change actual state */
    sStandbyState.eStandbyState = eStateEntryLowVoltage;
    
    /* Disable measurement module */
    //Warning when SelfTest_ADC is enabled. This would leave to an stopped CPU.
    DR_Measure_Stop();
    
    /* Disable the LED */
    #warning todo
//    Actors_SetOutputStatus(eLedPinOut, OFF);
    
    /* Change Drivemode-for pins */
// Actors_Sleep();
    
    /* Restart timeout for enabling new measurements */
    ucWakeUpCounter = WAKE_UP_CNT_LIMIT;
    
    sStandbyState.bStateReached = true;
}


//********************************************************************************
/*!
\author  KraemerE
\date    18.02.2019

\fn      StateExecute

\brief   In this state the CPU shall only wait for a message from the UIM,
         do some self-tests and maybe go to sleep.
 
\param   none

\return  none
***********************************************************************************/
static void StateExecute(void)
{    
    /* Change actual state */
    sStandbyState.eStandbyState = eStateStandby;

    if(OS_Serial_UART_TransmitStatus() == true)
    {
        /* Enable deep sleep watchdog counter */
        //WDT_DeepSleepWdtEnable();
        
        /* Enable AllPortIsr */
        if(AllPortIsr_GetState() == OFF)
        {
            AllPortIsr_Enable();
        }
        
        /* Enter critical section */
        u8 ucInterruptStatus = CyEnterCriticalSection();
        
        /* Enable wake up sources from deep sleep or set UART in sleep mode */
        if(OS_Serial_UART_EnableUartWakeupInSleep())
        {                
            /* Put modules like Timers, ADC and so on in sleep mode */
            ModulesSleep();
            
            /* Set interrupt mode for UART-Rx-pin to falling edge */
            UART_rx_SetInterruptMode(UART_rx_0_INTR, UART_rx_INTR_FALLING);
            
            /* Set interrupt mode for PIR-pin to rising edge */
            Pin_PIR_SetInterruptMode(Pin_PIR_0_INTR, Pin_PIR_INTR_RISING);
            
            #if ECO_USED 
                /* Change clock to IMO */        
                CySysClkWriteHfclkDirect(CY_SYS_CLK_HFCLK_IMO);
            #endif

            /* Reset WDT counter */
            //WDT_DeepSleepWdtResetCounter();

            DEBUG_PIN_ON;    //Debug pin
            
            /* Go back to  deep sleep. Sleep is leaved with the next interrupt from a 
               wake up source (Watchdog-Timer or UART changes) */
            CySysPmDeepSleep();
            
            DEBUG_PIN_OFF;   //Debug pin
                        
            #if ECO_USED            
                /* When non zero, than the eco is oscillating correctly */
                while(CySysClkEcoReadStatus() == 0)
                {                
                    /* Start-up time from the data-sheet */
                    CySysClkEcoStart(850);
                }
                
                /* change HF clock source back to ECO */
                while(CySysClkGetSysclkSource() != CY_SYS_CLK_HFCLK_ECO)
                {        
                    CySysClkWriteHfclkDirect(CY_SYS_CLK_HFCLK_ECO);
                }
            #endif        
            
            /* disable interrupts on the UART RX pin. Interrupt request is cleared in the GPIO-Handler in Actors */
            UART_rx_SetInterruptMode(UART_rx_0_INTR, UART_rx_INTR_NONE);
                            
            /* Disable interrupts on PIR pin. Interrupt request is cleared in the GPIO Handler in Actors */
            Pin_PIR_SetInterruptMode(Pin_PIR_0_INTR, Pin_PIR_INTR_NONE);
            
            /* CPU woke up here - Disable wake up sources / Enable UART again */
            OS_Serial_UART_DisableUartWakeupInSleep();
            
            /* Wake up modules like Timers, ADC and so from sleep mode */
            ModulesWakeup(); 
            
            /* Decrement wake up counter until zero */
            if(ucWakeUpCounter)
            {
                --ucWakeUpCounter;
            }
            else
            {
                /* Wake-up counter is reached start with a new measurement */
                OS_EVT_PostEvent(eEvtStandby, eEvtParam_StandbyStateMeasure, 0);
            }
        }
        
        /* Enter critical section */
        CyExitCriticalSection(ucInterruptStatus);
                
        /* Enable AllPortIsr */
        if(AllPortIsr_GetState() == ON)
        {
            AllPortIsr_Disable();
        }
        
        /* Disable deep sleep WDT counter */
        //WDT_DeepSleepWdtDisable();
    }
    
    sStandbyState.bStateReached = true;
}


//********************************************************************************
/*!
\author  KraemerE
\date    18.02.2019

\fn      StateExitLowVoltage

\brief   This state re-enables the low voltage peripherals to measure the temperature.
         After some measurements the state is leaved and entered via "StateEntryLowVoltage"
         or when the standby should be left the "StateExitSystem".

\param   none

\return  none
***********************************************************************************/
static void StateExitLowVoltage(void)
{    
    /* Change actual state */
    sStandbyState.eStandbyState = eStateExitLowVoltage;
    
    /* Enable measurement module */
    DR_Measure_Start();
    
    sStandbyState.bStateReached = true;
}


//********************************************************************************
/*!
\author  KraemerE
\date    18.02.2019

\fn      StateExitSystem

\brief   This state is called when the PCM temperature is above the set limit or
         the UIM requested to leave the stand by mode.

\param   none

\return  none
***********************************************************************************/
static void StateExitSystem(void)
{
    /* Change actual state */
    sStandbyState.eStandbyState = eStateExitSystem;
    
    /* Switch on system */
    
    const tRegulationValues* psRegVal = Aom_Regulation_GetRegulationValuesPointer();
    
    u8 ucOutputIdx;
    for(ucOutputIdx = 0; ucOutputIdx < DRIVE_OUTPUTS; ucOutputIdx++)
    {    
        Aom_Regulation_CheckRequestValues(psRegVal->sLedValue[ucOutputIdx].ucPercentValue,
                                            ON,
                                            psRegVal->bNightModeOnOff,
                                            psRegVal->sUserTimerSettings.bMotionDetectOnOff,
                                            psRegVal->sUserTimerSettings.ucBurningTime,
                                            false,
                                            psRegVal->sUserTimerSettings.bAutomaticModeActive,
                                            ucOutputIdx);
    }
    
    /* State was reached */
    sStandbyState.bStateReached = true;
    
    /* State was reached */
    sStandbyState.bStateReached = true;
}


/************************ externally visible functions ***********************/

//********************************************************************************
/*!
\author  KraemerE
\date    18.02.2019

\fn      Standby_Handler

\brief   Calls the linked standby-function. Checks afterwards for the next state.

\param   none

\return  eStandbyState - Returns the actual standby state
***********************************************************************************/
teStandbyState Standby_Handler(void)
{    
    /* Call the actual standby function */
    if(sStandbyState.pFctState)
    {
        sStandbyState.pFctState();
    }
    /* State active reached successfully */
    else
    {
        sStandbyState.eStandbyState = eStateActive;
        
        sStandbyState.bStateReached = true;
        
        /* Enable AllPortIsr */
        if(AllPortIsr_GetState() == ON)
        {
            AllPortIsr_Disable();
        }
    }
    
    /* Get the next state */
    CheckForNextState();
    
    return sStandbyState.eStandbyState;
}


//********************************************************************************
/*!
\author  KraemerE
\date    18.02.2019

\fn      Standby_ChangeState

\brief   Set a change request for the stand by

\param   eRequestedState - The requested state which should be switched to.

\return  none
***********************************************************************************/
void Standby_ChangeState(teStandbyState eRequestedState)
{
    /* Save requested state */
    sStandbyState.eReqState = eRequestedState;
}


//********************************************************************************
/*!
\author  KraemerE
\date    19.02.2019

\fn      Standby_GetStandbyState

\brief   Returns the acutal standby state

\param   none

\return  bool - Returns true when one of the standby states is active
***********************************************************************************/
teStandbyState Standby_GetStandbyState(void)
{
    return sStandbyState.eStandbyState;
}
