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
#include "OS_ErrorHandler.h"
#include "HAL_IO.h"

#include "Aom_Regulation.h"
#include "Aom_Flash.h"
#include "Aom_Measure.h"
#include "Aom_System.h"

#include "Regulation_Data.h"
#include "Regulation_State_Init.h"
#include "Regulation_State_Root.h"
#include "DR_UserInterface.h"

#if (WITHOUT_REGULATION == false)
/****************************************** Defines ******************************************************/
#define AVG_BUFFER_SIZE     2
    
    
//Calculated by the period value of the PWM is 160. A second are 1000ms so to normalize the PWM period over
//the second I've calculated 1000ms/160 = 6.25ms.
#define REGULATION_CYCLES_MS    7  //Every 7 milliseconds a regulation cycle shall be handled.
    
typedef struct
{
    s16  siBuffer[AVG_BUFFER_SIZE];
    s16  siSum;
    s16  siAvg;
    u8   ucBufferIndex;
}tsMovingAverageValues;
    
/****************************************** Variables ****************************************************/
static u16 uiLedCompareVal[DRIVE_OUTPUTS];
    
static tsMovingAverageValues uiAvgCompVal[DRIVE_OUTPUTS];
static tsRegulationHandler sRegulationHandler[DRIVE_OUTPUTS];   
static tCStateDefinition* psStateHandler[DRIVE_OUTPUTS] = {NULL, NULL, NULL};

/****************************************** Function prototypes ******************************************/
static void RegulatePWM(u8 ucOutputIdx);


/****************************************** loacl functiones *********************************************/
//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Fills a moving average filter with values.
\return     uiAveragedCompareValue - Averaged value
\param      ucOutputIdx - The output index which shall be saved
\param      uiCompareValue - New value which shall be used for the ADC
***********************************************************************************/
static u16 PutInMovingAverage(u8 ucOutputIdx, u16 uiCompareValue)
{
    u16 uiAveragedCompareValue = 0;
    
    if(ucOutputIdx < DRIVE_OUTPUTS)
    {    
        tsMovingAverageValues* psAvgCompVal = &uiAvgCompVal[ucOutputIdx];
        
        /* Subtract the oldest entry from the sum */   
        psAvgCompVal->siSum -= psAvgCompVal->siBuffer[psAvgCompVal->ucBufferIndex];
          
        /* Save new value in buffer */
        psAvgCompVal->siBuffer[psAvgCompVal->ucBufferIndex] = uiCompareValue;
        
        /* Add new value to summation */
        psAvgCompVal->siSum += uiCompareValue;
        
        /* Increment buffer index and set back to zero, when limit is reached */
        if(++psAvgCompVal->ucBufferIndex == _countof(psAvgCompVal->siBuffer))
        {
            psAvgCompVal->ucBufferIndex = 0;
        }
        
        /* Get the averaged value */
        uiAveragedCompareValue = psAvgCompVal->siSum / AVG_BUFFER_SIZE;
    }
    
    return uiAveragedCompareValue;
}

#if PWM_ISR_ENABLE
//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\fn         PwmInterruptServiceRoutine()
\brief      Interrupt function which is called whenever the PWM reaches the TC.
            Writes new value in compare register.
\return     none
\param      none
***********************************************************************************/
static void PwmInterruptServiceRoutine(void)
{   
    /* Stop the PWM */
    PWM_Stop();
        
    /* Clear TC interrupt */
    PWM_ISR_ClearPending();

    /* Read PWM status register -> clears also TC isr*/
//    (void)PWM_ReadStatusRegister();
        
    /* Write new PWM compare values into PWM module */
    PWM_COMPARE_WR(uiLedCompareVal);
    
    /* Start PWM */
    PWM_Start();
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\fn         GetCompareValueFactor()
\brief      This function calculates a multiplication factor for the compare and 
            period value in dependency of the AD-Difference between should and is value.
\return     ucMultiFactor - Factor for the Incr/Decrement.
\param      psLedValue - Pointer to the structure of the LED value
***********************************************************************************/
static u8 GetCompareValueFactor(tLedValue* psLedValue)
{
    u8 ucMultiFactor = 1;
    s16 siAdcDiff;
    
    /* Check for valid pointer */
    if(psLedValue)
    {
        /* Get absolute AD value difference */
        if(psLedValue->uiReqAdcVal < psLedValue->uiIsAdcVal)
        {
            siAdcDiff = psLedValue->uiIsAdcVal - psLedValue->uiReqAdcVal;
        }
        else
        {
            siAdcDiff = psLedValue->uiReqAdcVal - psLedValue->uiIsAdcVal;
        }
        
        /* Factor is difference dependent */
        ucMultiFactor = siAdcDiff >> 6; //Division trough 64
        
        /* Make it smoother */
        if(ucMultiFactor > 1)
        {
            ucMultiFactor--;
        }    
        else if(ucMultiFactor <= 0)
        {
            ucMultiFactor = 1;
        }
    }
    return ucMultiFactor;
}
#endif


//********************************************************************************
/*!
\author  KraemerE
\date    19.02.2019
\brief   Checks the next available state according to the requested state. 
\param   none
\return  none
***********************************************************************************/
static void CheckForNextState(u8 ucOutputIdx)
{        
    /* Use pointer for easier access */
    tsRegulationState* psRegulationState = &sRegulationHandler[ucOutputIdx].sRegState;
    tsRegAdcVal* psRegVal = &sRegulationHandler[ucOutputIdx].sRegAdcVal;
    
    /* Check if regulation should be switched off */
    if(psRegulationState->eReqState == eStateOff)
    {
        switch(psRegulationState->eRegulationState)
        {
            //Actual state is system entry
            case eStateEntry:
            case eStateActiveR:
            {
                //Next state should disable the regulation again
                psRegulationState->eNextState = eStateExit;
                break;
            }
                        
            case eStateExit:
            {
                //Leave regulation complete when light dimm down was reached
                if(psRegVal->bCantReach || psRegVal->bReached)
                    psRegulationState->eNextState = eStateOff;
                break;
            }
            
            case eStateOff:
            default:
                break;
        }
    }
    /* Check if regulation doesn't reached the requested state yet */
    else if(psRegulationState->eRegulationState != psRegulationState->eReqState)
    {
        switch(psRegulationState->eRegulationState)
        {
            case eStateOff:
            {
                //Next state enables the hardware and starts with a low output
                psRegulationState->eNextState = eStateEntry;
                break;
            }
            
            case eStateEntry:
            {
                //Next state is the regulation handling
                psRegulationState->eNextState = eStateActiveR;
                break;
            }
            
            case eStateActiveR:
            {
                //Next state should dimm down
                psRegulationState->eNextState = eStateExit;
                break;
            }
            
            
            case eStateExit:
            {                
                //Next state should restart again
                psRegulationState->eNextState = eStateEntry;
                break;
            }
            
            //This one isn't necessary because in this case the above if-statement would be used.
            //case eStateOff:            
            default:   
                break;
        }
    }
    
    /* When the requested state wasn't reached yet swtich the function pointer according the next state */
    if(psRegulationState->eReqState != psRegulationState->eRegulationState
        && psRegulationState->bStateReached == true)
    {
        /* Change callback for dependent state */
        switch(psRegulationState->eNextState)
        {
            case eStateActiveR:
            {
                psRegulationState->pFctState = psStateHandler[ucOutputIdx]->pFctActive;
                break;
            }
            
            case eStateEntry:
            {
                psRegulationState->pFctState = psStateHandler[ucOutputIdx]->pFctEntry;
                break;
            }
            
            case eStateExit:
            {
                psRegulationState->pFctState = psStateHandler[ucOutputIdx]->pFctExit;
                break;
            }            
            
            case eStateOff:
            {
                psRegulationState->pFctState = psStateHandler[ucOutputIdx]->pFctOff;
                break;
            }
            
            default:
                break;
        }
        
        /* Clear bit for next state */
        psRegulationState->bStateReached = false;
    }
}



//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\fn         RegulatePWM()
\brief      Regulate the PWM compare register, has to be triggered externally.
\return     none
\param      none
***********************************************************************************/
static void RegulatePWM(u8 ucOutputIdx)
{    
    /* Use pointer access for easier handling */
    tsRegAdcVal* psRegAdcVal = &sRegulationHandler[ucOutputIdx].sRegAdcVal;
    
    /* Calculate limits */
    s16 siAdcUpperLimit = psRegAdcVal->uiReqValue + ADC_LIMITS;
    s16 siAdcLowerLimit = psRegAdcVal->uiReqValue - ADC_LIMITS;    
    u16 uiReadPeriod = 0;
    
    /* Read actual compare values */
    HAL_IO_PWM_ReadCompare(ucOutputIdx, &uiLedCompareVal[ucOutputIdx]);
    HAL_IO_PWM_ReadPeriod(ucOutputIdx, &uiReadPeriod);
    
    /*************** Check for regulation ******************************************/
    if(psRegAdcVal->uiIsValue < siAdcLowerLimit)
    {
        if(uiLedCompareVal[ucOutputIdx] < uiReadPeriod)
        {
            ++uiLedCompareVal[ucOutputIdx];
        }
        else
            psRegAdcVal->bCantReach = true;
    }
    else if(psRegAdcVal->uiIsValue > siAdcUpperLimit)
    {
        // Is value is smaller than requested value
        if(uiLedCompareVal[ucOutputIdx] > 1)
        {
            --uiLedCompareVal[ucOutputIdx];
        }
        else
            psRegAdcVal->bCantReach = true;
    }
    else
    {
        // Requested value reached
        psRegAdcVal->bReached = true;
    }
    
    /* Average compare value */
    u16 uiAvgCompValue = PutInMovingAverage(ucOutputIdx, uiLedCompareVal[ucOutputIdx]);

    /* Don't change avg value when the ADC value has been reached */
    if(psRegAdcVal->bReached == false && psRegAdcVal->bCantReach == false)
    {
        /* Write new compare value into compare register */
        HAL_IO_PWM_WriteCompare(ucOutputIdx, uiAvgCompValue);   
    }
}


/****************************************** External visible functiones **********************************/

//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Initializes the Regulation values and PWM.
\return     none
\param      none
***********************************************************************************/
void DR_Regulation_Init(void)
{
    /* Read system settings from flash */
    u8 ucFlashSettingsRead = Aom_Flash_ReadSystemSettingsFromFlash();
    
    /* Get saved Regulation values from Flash */
    Aom_Flash_ReadUserSettingsFromFlash();    
    
    /* Initialize HAL */
    HAL_IO_Init();
    
    /* Initialize state machine handler */
    u8 ucOutputIdx;
    for(ucOutputIdx = 0; ucOutputIdx < DRIVE_OUTPUTS; ucOutputIdx++)
    {
        /* Get the state machine structure */
        psStateHandler[ucOutputIdx] = Regulation_State_RootLink(&sRegulationHandler[ucOutputIdx], ucOutputIdx);        
        
        const tLedValue* psLedVal = Aom_GetOutputsSettingsEntry(ucOutputIdx);
        
        /* Initialize state machine */
        sRegulationHandler[ucOutputIdx].sRegState.eRegulationState = eStateOff;
        sRegulationHandler[ucOutputIdx].sRegState.eReqState = psLedVal->bStatus == ON ? eStateActiveR : eStateOff;
        sRegulationHandler[ucOutputIdx].sRegState.pFctState = psStateHandler[ucOutputIdx]->pFctOff;
        sRegulationHandler[ucOutputIdx].sRegState.bStateReached = false;
       
        /* Get the initalized status of each output */
        sRegulationHandler[ucOutputIdx].sRegAdcVal.bInitialized = ucFlashSettingsRead & ( 0x01 << ucOutputIdx);
        
        /* Set to true because the state machine starts with state off.
        in this case the PWM is disabled manually */
        //sRegulationHandler[ucOutputIdx].bHardwareEnabled = true;
        
        /* Init PWM module */
        HAL_IO_PWM_Start(ucOutputIdx);
    }

    #if PWM_ISR_ENABLE
    /* Set PWM isr adress */    
    PWM_SetInterruptMode(PWM_INTR_MASK_TC);
    PWM_ISR_StartEx(PwmInterruptServiceRoutine);
    #endif
}


//********************************************************************************
/*!
\author  KraemerE
\date    18.02.2019
\brief   Calls the linked regulation-function. Checks afterwards for the next state.
\param   uiMilliSecElapsed - The time since the last call.
\return  ucIsAnyOutputActive - Zero when no output is active otherwise each bit represents
                               an output.
***********************************************************************************/
u8 DR_Regulation_Handler(u16 uiMilliSecElapsed)
{       
    static uint16 uiMilliSecCnt = 0;
    
    uiMilliSecCnt += uiMilliSecElapsed;
    
    u8 ucIsAnyOutputActive = 0;
    
    u8 ucOutputIdx;    
    for(ucOutputIdx = 0; ucOutputIdx < DRIVE_OUTPUTS; ucOutputIdx++)
    {        
        /* Use pointer for easier handling */
        tsRegAdcVal* psRegAdcVal = &sRegulationHandler[ucOutputIdx].sRegAdcVal;
        tsRegulationState* psRegState = &sRegulationHandler[ucOutputIdx].sRegState;
        
        /* Call the actual standby function */
        if(psRegState->pFctState)
        {
            psRegState->pFctState(ucOutputIdx);
        }
        
        /* Check if requested value has changed */
        if(psRegAdcVal->uiOldReqValue != psRegAdcVal->uiReqValue)
        {
            /* Save actual value and "restart" the PWM regulation */
            psRegAdcVal->uiOldReqValue = psRegAdcVal->uiReqValue;
            psRegAdcVal->bReached = false;
            psRegAdcVal->bCantReach = false;
        }
        
        /* Get the next state */
        CheckForNextState(ucOutputIdx);
        
        
        /* Regulate PWM */
        if(uiMilliSecCnt >= REGULATION_CYCLES_MS)
        {
            RegulatePWM(ucOutputIdx);
            uiMilliSecCnt = 0;
        }
        
        if(psRegState->eRegulationState != eStateOff)
        {
            ucIsAnyOutputActive |= (0x01 << ucOutputIdx);
        }
    }   
    
    return ucIsAnyOutputActive;
}


//********************************************************************************
/*!
\author  KraemerE
\date    18.02.2019
\brief   Set a change request for the stand by
\param   eRequestedState - The requested state which should be switched to.
\return  none
***********************************************************************************/
void DR_Regulation_ChangeState(teRegulationState eRequestedState, u8 ucOutputIdx)
{
    //if(eRequestedState == eStateInit)
    //{
    //    sRegulationHandler[ucOutputIdx].sRegAdcVal.bInitialized = false;
    //}
    
    if(ucOutputIdx < _countof(sRegulationHandler))
    {
        /* Save requested state */
         sRegulationHandler[ucOutputIdx].sRegState.eReqState = eRequestedState;
    }
}

//********************************************************************************
/*!
\author  KraemerE
\date    08.11.2020
\brief   Change the state machine
\param   eStateMachine - The desired state machine which shall be used
\param   ucOutputIdx - The output which shall change the state machine 
\return  none
***********************************************************************************/
void DR_Regulation_ChangeStateMachine(teRegulationStateMachine eStateMachine, u8 ucOutputIdx)
{
    if(eStateMachine == eStateMachine_Init
        && ucOutputIdx < _countof(sRegulationHandler))
    {
        sRegulationHandler[ucOutputIdx].sRegAdcVal.bInitialized = false;
    }
    
    #warning TODO: state machine change handling! Current state machine -> OFF and than change
}


//********************************************************************************
/*!
\author  KraemerE
\date    19.02.2019
\brief   Returns the acutal state
\param   none
\return  teRegulationState - The requested state is returned
***********************************************************************************/
teRegulationState DR_Regulation_GetActualState(u8 ucOutputIdx)
{
    return sRegulationHandler[ucOutputIdx].sRegState.eRegulationState;
}


//********************************************************************************
/*!
\author  KraemerE
\date    21.12.2019
\brief   Returns the requested state
\param   none
\return  teRegulationState - The requested state is returned
***********************************************************************************/
teRegulationState DR_Regulation_GetRequestedState(u8 ucOutputIdx)
{
    return sRegulationHandler[ucOutputIdx].sRegState.eReqState;
}


//********************************************************************************
/*!
\author  KraemerE
\date    21.12.2019
\brief   Returns the hardware enabled status
\param   none
\return  bHardwareEnabled - Information about the hardware enable status
***********************************************************************************/
bool DR_Regulation_GetHardwareEnabledStatus(u8 ucOutputIdx)
{
    return HAL_IO_GetPwmStatus(ucOutputIdx);
}


//********************************************************************************
/*!
\author  KraemerE
\date    06.05.2021
\brief   Returns the ESP Reset status. When false than the ESP is in running mode.
         otherwise the ESP is in HARDWARE-RESET Mode.
\param   none
\return  bool - True for active reset mode and false for normal mode.
***********************************************************************************/
bool DR_Regulation_GetEspResetStatus(void)
{
    bool bResetState = !HAL_IO_ReadOutputStatus(ePin_EspResetPin);
    return bResetState;
}

//********************************************************************************
/*!
\author  KraemerE
\date    06.05.2021
\brief   Sets the ESP into either a reset mode or into a normal mode.
\param   bReset - True to set the ESP into reset state. False to leash the reset pin.
\return  bool - True for High-State and false for LOW-State
***********************************************************************************/
void DR_Regulation_SetEspResetStatus(bool bReset)
{
    //Invert request because a HIGH-State is normal mode and a LOW-State is Reset mode.
    HAL_IO_SetOutputStatus(ePin_EspResetPin, !bReset);
    HAL_IO_SetOutputStatus(ePin_EspResetCopy, !bReset);
}

//********************************************************************************
/*!
\author  KraemerE
\date    08.05.2021
\brief   Puts modules into sleep mode
\param   none
\return  none
***********************************************************************************/
void DR_Regulation_ModulesSleep(void)
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
        
    DR_UI_SwitchOffHeartBeatLED();
    
    //UART is set to sleep in
    //"Serial_EnableUartWakeupInSleep()"    
}

//********************************************************************************
/*!
\author  KraemerE
\date    08.05.2021
\brief   Puts modules into sleep mode
\param   none
\return  none
***********************************************************************************/
void DR_Regulation_ModulesWakeup(void)
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
\date    08.05.2021
\brief   Set specific IOs into interrupt mode. They will create an interrupt which
         is catched by the AllPortIsr.
\param   none
\return  none
***********************************************************************************/
void DR_Regulation_SetWakeupInterrupts(void)
{    
    /* Set interrupt mode for UART-Rx-pin to falling edge */
    UART_rx_SetInterruptMode(UART_rx_0_INTR, UART_rx_INTR_FALLING);
    
    /* Set interrupt mode for PIR-pin to rising edge */
    Pin_PIR_SetInterruptMode(Pin_PIR_0_INTR, Pin_PIR_INTR_RISING);
}

//********************************************************************************
/*!
\author  KraemerE
\date    08.05.2021
\brief   Set specific IOs into interrupt mode. They will create an interrupt which
         is catched by the AllPortIsr.
\param   none
\return  none
***********************************************************************************/
void DR_Regulation_DeleteWakeupInterrupts(void)
{    
    /* disable interrupts on the UART RX pin. Interrupt request is cleared in the GPIO-Handler in Actors */
    UART_rx_SetInterruptMode(UART_rx_0_INTR, UART_rx_INTR_NONE);
                    
    /* Disable interrupts on PIR pin. Interrupt request is cleared in the GPIO Handler in Actors */
    Pin_PIR_SetInterruptMode(Pin_PIR_0_INTR, Pin_PIR_INTR_NONE);
}

//********************************************************************************
/*!
\author  KraemerE
\date    08.05.2021
\brief   Enters the deep sleep mode of this controller.
         Sleep is leaved with the next interrupt from a 
         wake up source (Watchdog-Timer or UART changes) 
\param   none
\return  none
***********************************************************************************/
void DR_Regulation_EnterDeepSleepMode(void)
{
    //Enter deep sleep mode
    CySysPmDeepSleep();
}


//********************************************************************************
/*!
\author  KraemerE
\date    08.05.2021
\brief   RX pin has been toggled during standby state
\param   none
\return  none
***********************************************************************************/
void DR_Regulation_RxInterruptOnSleep(void)
{
    OS_EVT_PostEvent(eEvtStandby_RxToggled, 0, 0);
}


#endif