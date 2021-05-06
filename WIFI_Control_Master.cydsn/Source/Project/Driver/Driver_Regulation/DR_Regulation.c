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
#include "Regulation_Data.h"
#include "Regulation_State_Init.h"
#include "Regulation_State_Root.h"

#if (WITHOUT_REGULATION == false)
/****************************************** Defines ******************************************************/


/****************************************** Variables ****************************************************/
static u16 uiLedCompareVal[DRIVE_OUTPUTS];
static u16 uiOldCompareValue[DRIVE_OUTPUTS];  
    
static tsRegulationHandler sRegulationHandler[DRIVE_OUTPUTS];   
static tCStateDefinition* psStateHandler[DRIVE_OUTPUTS] = {NULL, NULL, NULL};

/****************************************** Function prototypes ******************************************/
static void RegulatePWM(u8 ucOutputIdx);


/****************************************** loacl functiones *********************************************/
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
    HAL_IO_PWM_ReadCompare(ucOutputIdx, (u32*)&uiLedCompareVal[ucOutputIdx]);
    HAL_IO_PWM_ReadPeriod(ucOutputIdx, (u32*)&uiReadPeriod);
    
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
    
    /* Write new compare value only when changed */
    if(uiOldCompareValue[ucOutputIdx] != uiLedCompareVal[ucOutputIdx])
    {
        /* Write new compare value into compare register */
        HAL_IO_PWM_WriteCompare(ucOutputIdx, uiLedCompareVal[ucOutputIdx]);
        
        /* Save actual compare value */
        uiOldCompareValue[ucOutputIdx] = uiLedCompareVal[ucOutputIdx];        
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
        
        /* Initialize state machine */
        sRegulationHandler[ucOutputIdx].sRegState.eRegulationState = eStateOff;
        sRegulationHandler[ucOutputIdx].sRegState.eReqState = eStateOff;
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
\param   none
\return  ucIsAnyOutputActive - Zero when no output is active otherwise each bit represents
                               an output.
***********************************************************************************/
u8 DR_Regulation_Handler(void)
{       
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
        RegulatePWM(ucOutputIdx);
        
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
    
    /* Save requested state */
     sRegulationHandler[ucOutputIdx].sRegState.eReqState = eRequestedState;
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
    if(eStateMachine == eStateMachine_Init)
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
\brief   Toggles the heart-beat LED.
\param   none
\return  none
***********************************************************************************/
void DR_Regulation_ToggleHeartBeatLED(void)
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
\author  KraemerE
\date    06.05.2021
\brief   Toggles the error LED.
\param   none
\return  none
***********************************************************************************/
void DR_Regulation_ToggleErrorLED(void)
{
    bool bActualStatus = HAL_IO_ReadOutputStatus(eLedRed);
    
    /* Check if error timeout is running */
    if(OS_ErrorHandler_GetErrorTimeout() > 0)
    {   
        bActualStatus ^= 0x01;
    }
    else
    {
        //LED inputs are inverted
        bActualStatus = !OFF;
    }
    HAL_IO_SetOutputStatus(eLedRed, bActualStatus);
    
    #ifdef Pin_DEBUG_0
    Pin_DEBUG_Write(~Pin_DEBUG_Read());
    #endif
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
    bool bResetState = !HAL_IO_ReadOutputStatus(eEspResetPin);
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
    HAL_IO_SetOutputStatus(eEspResetPin, !bReset);
}


//********************************************************************************
/*!
\author  KraemerE
\date    06.05.2021
\brief   Checks first if a motion sensor is used. Afterwards the input pin of the
         sensor is read. When a HIGH level was detected the an event for reseting
         the burning time is created.
\param   none
\return  bMotionDetected - True for High-State and false for LOW-State
***********************************************************************************/
bool DR_Regulation_CheckSensorForMotion(void)
{
    bool bMotionDetected = false;
    
    /* Read PIR out for possible motion detection */                        
    const tRegulationValues* psReg = Aom_Regulation_GetRegulationValuesPointer();                        
    if(psReg->sUserTimerSettings.bMotionDetectOnOff)
    {                            
        /* Check if PIR has detected a change */
        bMotionDetected = HAL_IO_ReadDigitalSense(eSensePIR);
        
        if(bMotionDetected)
        {
            /* Reset "ON" timeout */
            OS_EVT_PostEvent(eEvtAutomaticMode_ResetBurningTimeout,0 ,0);
        }
    }
    
    return bMotionDetected;
}
#endif