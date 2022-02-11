//********************************************************************************
/*!
\author     Kraemer E.
\date       05.02.2019

\file       DR_UserInterface.c
\brief      Module for the user interface handling like LEDs and GPIOs

***********************************************************************************/

#include "DR_UserInterface.h"
#include "DR_ErrorDetection.h"
#include "OS_EventManager.h"
#include "OS_ErrorDebouncer.h"
#include "OS_ErrorHandler.h"
#include "HAL_IO.h"

#include "Aom_Regulation.h"
#include "Aom_System.h"
#include "IR_Decoder.h"

/****************************************** Defines ******************************************************/
   
/****************************************** Variables ****************************************************/

/****************************************** Function prototypes ******************************************/
static void IR_Decoder_TimerIRQ(void);
static void IR_Decoder_InputIRQ(void);
static void IR_Decoder_Clear(void);

/****************************************** local functions *********************************************/
//********************************************************************************
/*!
\author     Kraemer E.
\date       01.02.2022
\brief      Interrupt service routine for the IR-Timer
\return     none
\param      none
***********************************************************************************/
static void IR_Decoder_TimerIRQ(void)
{
    //Variable to store the counter status register. Value is used to determine the event
    //which caused the interrupt (terminal count or capture).
    uint32_t ulIsrSource = Timer_IR_GetInterruptSource();
    
    // Check if counter interrupt ist caused by Capture/Compare.
    if((ulIsrSource & Timer_IR_INTR_MASK_CC_MATCH) == Timer_IR_INTR_MASK_CC_MATCH)
    {
        /* Clear interrupt */
        Timer_IR_ClearInterrupt(ulIsrSource & Timer_IR_INTR_MASK_CC_MATCH);
        
        /* Interprete the  capture value */
        IR_Decoder_InterpreteCaptureValue(Timer_IR_ReadCapture());
    }
    
    //Check if counter interrupt is caused by terminal count.
    if((ulIsrSource & Timer_IR_INTR_MASK_TC) == Timer_IR_INTR_MASK_TC)
    {
        /* Clear interrupt */
        Timer_IR_ClearInterrupt(ulIsrSource & Timer_IR_INTR_MASK_TC);
        
        //Init decoder after finishing
        IR_Decoder_Clear();
    }
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       01.02.2022
\brief      Input pin interrupt received. Start timer and set interrupt mode to none
\return     none
\param      none
***********************************************************************************/
static void IR_Decoder_InputIRQ(void)
{
    //Check if counter is running
    if((Timer_IR_ReadStatus() & Timer_IR_STATUS_RUNNING) == false)
    {    
        Timer_IR_Start();
        //Timer_IR_TriggerCommand(Timer_IR_MASK, Timer_IR_CMD_START);
    }
     
    Timer_IR_TriggerCommand(Timer_IR_MASK, Timer_IR_CMD_CAPTURE);
    Timer_IR_TriggerCommand(Timer_IR_MASK, Timer_IR_CMD_RELOAD);
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       01.02.2022
\brief      Resets the timer and pin mode of the IR decoder
\return     none
\param      none
***********************************************************************************/
static void IR_Decoder_Clear(void)
{
    /* Disable timer */
    Timer_IR_Stop();
    
    /* Set interrupt mode to trigger on falling and rising edge */    
    Pin_IR_IN_SetInterruptMode(Pin_IR_IN_0_INTR, Pin_IR_IN_INTR_BOTH);
    
    //Set IR-State back to start
    IR_Decoder_Init();
}

/****************************************** External visible functiones **********************************/

//********************************************************************************
/*!
\author     Kraemer E.
\date       01.02.2022
\brief      Initializes the driver user interface module
\return     none
\param      none
***********************************************************************************/
void DR_UI_Init(void)
{
    /* Enable AllPortIsr */
    if(AllPortIsr_GetState() == OFF)
    {
        AllPortIsr_Enable();
    }
    
    //Initialize the IR decoder
    IR_Decoder_Init();
    
    //Link the IR-Decoder Timer with the IR-Decoder module
    Timer_IR_ISR_StartEx(IR_Decoder_TimerIRQ);
}


//********************************************************************************
/*!
\author  KraemerE
\date    06.05.2021
\brief   Toggles the error LED.
\param   none
\return  none
***********************************************************************************/
void DR_UI_ToggleErrorLED(void)
{
    bool bActualStatus = HAL_IO_ReadOutputStatus(ePin_LedRed);
    
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
    HAL_IO_SetOutputStatus(ePin_LedRed, bActualStatus);
    
    #ifdef Pin_DEBUG_0
    Pin_DEBUG_Write(~Pin_DEBUG_Read());
    #endif
}


//********************************************************************************
/*!
\author  KraemerE
\date    06.05.2021
\brief   Toggles the heart-beat LED.
\param   none
\return  none
***********************************************************************************/
void DR_UI_ToggleHeartBeatLED(void)
{
    bool bActualStatus = HAL_IO_ReadOutputStatus(ePin_LedGreen);
    bActualStatus ^= 0x01;
    HAL_IO_SetOutputStatus(ePin_LedGreen, bActualStatus);
    
    #ifdef Pin_DEBUG_0
    Pin_DEBUG_Write(~Pin_DEBUG_Read());
    #endif
}


//********************************************************************************
/*!
\author  KraemerE
\date    09.06.2021
\brief   Switches off the heart-beat LED.
\param   none
\return  none
***********************************************************************************/
void DR_UI_SwitchOffHeartBeatLED(void)
{
    HAL_IO_SetOutputStatus(ePin_LedGreen, !OFF);
}


//********************************************************************************
/*!
\author  KraemerE
\date    06.05.2021
\brief   Checks first if a motion sensor is used. Afterwards the input pin of the
         sensor is read. When a HIGH level was detected the an event for reseting
         the burning time is created.
\param   none
\return  psAutoVal->bMotionDetected - True for High-State and false for LOW-State
***********************************************************************************/
void DR_UI_CheckSensorForMotion(void)
{
    /* Read PIR out for possible motion detection */                        
    const tRegulationValues* psReg = Aom_Regulation_GetRegulationValuesPointer();                        
    tsAutomaticModeValues* psAutoVal = Aom_System_GetAutomaticModeValuesStruct();
        
    if(psReg->sUserTimerSettings.bMotionDetectOnOff)
    {
        /* Check if PIR has detected a change */
        psAutoVal->bMotionDetected = HAL_IO_ReadDigitalSense(eSensePIR);

        if(psAutoVal->bMotionDetected)
        {
            /* Reset "ON" timeout */
            OS_EVT_PostEvent(eEvtAutomaticMode_ResetBurningTimeout,0 ,0);
        }
    }    
}

//********************************************************************************
/*!
\author  KraemerE
\date    01.02.2022
\brief   Interrupt service request for the infrared gpio. Triggers the IRQ of
         the IR_Decoder.
\param   none
\return  none
***********************************************************************************/
void DR_UI_InfraredInputIRQ(void)
{
    IR_Decoder_InputIRQ();
}