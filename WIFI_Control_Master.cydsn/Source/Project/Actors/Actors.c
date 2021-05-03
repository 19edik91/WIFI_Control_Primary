/* ========================================
 *
 * Copyright Eduard Kraemer, 2019 *
 * ========================================
*/

#include "Actors.h"
#include "Aom_Regulation.h"
#include "Aom_System.h"
#include "AutomaticMode.h"
#include "EventManager.h"

/****************************************** Defines ******************************************************/
/****************************************** Variables ****************************************************/
//Array holding registers for all sense pins
const tPinMapping sSenseMap[COUNT_OF_SENSELINES]=
{
    #define S_MAP(Senseline, PinMapping) PinMapping,
        SENSE_MAP
    #undef S_MAP
};

//Array holding registers for all output pins
const tPinMapping sOutputMap[COUNT_OF_OUTPUTS]=
{
    #define O_MAP(Senseline, PinMapping) PinMapping,
        OUTPUT_MAP
    #undef O_MAP
};

//Array holding functions for all PWM modules
const tsPwmFnMapping sPwmMap[COUNT_OF_PWM] =
{
    #define P_MAP(Name, PwmFn) PwmFn,
        PWM_MAP
    #undef P_MAP
};

static u8 ucOutputPortMasks[NUMBER_OF_PORTS];
static bool bPortMasksInitialized = false;



/****************************************** Function prototypes ******************************************/
static void InitializePortMasks(void);
static u8 GetPortMask(u8 ucPort, bool bOutputMask);
static void GPIOInterruptHandler(void);


/****************************************** loacl functions *********************************************/
//********************************************************************************
/*!
\author     KraemerE   
\date       10.07.2018  

\fn         InitializePortMasks (void)

\brief      Initializes the port mask array and check it with a checksum.

\return     none - 
***********************************************************************************/
static void InitializePortMasks(void)
{
    u8 ucPortIdx = 0;
    u16 uiChecksum = 0;
    for(ucPortIdx = NUMBER_OF_PORTS; ucPortIdx--;)
    {
        ucOutputPortMasks[ucPortIdx] = GetPortMask(ucPortIdx, true);
        uiChecksum += ucOutputPortMasks[ucPortIdx];
    }
    
    /* Check for correct mask */
    if(uiChecksum && uiChecksum < (NUMBER_OF_PORTS*0xFF))
    {
        bPortMasksInitialized = true;
        Aom_Regulation_SetActorsPortMask(&ucOutputPortMasks[0]);
    }
}

//********************************************************************************
/*!
\author     KraemerE   
\date       10.07.2018  

\fn         GetPortMask (u8 ucPort)

\brief      Uses the bitshift- and port-information  of the relay and triac mapping to create a
            register mask.

\return     ucPortMask - Returns the created port mask.

\param      ucPort - Portnumber which should be used to create a mask.
***********************************************************************************/
static u8 GetPortMask(u8 ucPort, bool bOutputMask)
{
    u8 ucPortMask = 0x00;    
    u8 ucIndex = 0;
    
    /* Get output pins only */
    if(bOutputMask)
    {        
        /* Get port mask for relays in this port */
        for(ucIndex = COUNT_OF_OUTPUTS; ucIndex--;)
        {
            if(sOutputMap[ucIndex].ucPort == ucPort 
                && sOutputMap[ucIndex].bCheckIO == true)
            {
                ucPortMask |= 0x01 << sOutputMap[ucIndex].ucBitShift;
            }
        }
    }
    
    /* Get input pins only */
    else
    {        
        /* Get port mask for digital sense lines in this port */
        for(ucIndex = COUNT_OF_SENSELINES; ucIndex--;)
        {
            if(sSenseMap[ucIndex].ucPort == ucPort
                && sOutputMap[ucIndex].bCheckIO == true)
            {
                ucPortMask |= 0x01 << sSenseMap[ucIndex].ucBitShift;
            }
        }
    }
    return ucPortMask;
}

//********************************************************************************
/*!
\author     Kraemer E
\date       03.10.2020
\fn         GPIOInterruptHandler
\brief      ISR function called by hardware whenever a GPIO detects an interrupt.
            Don't clear the specific interrupt request in the standby module.
            This handler is called when the critical section is left. When the IRQ
            is cleared before this module won't get any IRQs
\return     none
\param      none
***********************************************************************************/
static void GPIOInterruptHandler(void)
{            
    /* Check for PIR-pin triggered wake up */
    if((Pin_PIR_INTSTAT & (1 << Pin_PIR_0_SHIFT)) != 0)
    {
        /* Clear this interrupt */
        Pin_PIR_INTSTAT = (1 << Pin_PIR_0_SHIFT);
        
        /* Wake up system */
        EVT_PostEvent(eEvtStandby, eEvtParam_ExitStandby, 0);
        
        
        /* Read PIR out for possible motion detection */                        
        const tRegulationValues* psReg = Aom_Regulation_GetRegulationValuesPointer();                        
        if(psReg->sUserTimerSettings.bMotionDetectOnOff)
        {                            
            /* Check if PIR has detected a change */
            if(Actors_ReadDigitalSense(eSensePIR))
            {
                /* Reset "ON" timeout */
                AutomaticMode_ResetBurningTimeout();
                
                /* Set information about detected PIR motion */
                tsAutomaticModeValues* psAutoMode = Aom_System_GetAutomaticModeValuesStruct();
                psAutoMode->bMotionDetected = true;
            }
        }
    }
    
    /* check for RX-triggered wake */
    if((UART_rx_INTSTAT & (1 << UART_rx_0_SHIFT)) != 0)
    {
        /* clear this interrupt immediately to avoid further processing */
        UART_rx_INTSTAT = (1 << UART_rx_0_SHIFT);
        
        EVT_PostEvent(eEvtStandby, eEvtParam_StandbyStateMessageRec, 0);
    }
}

/****************************************** External visible functiones **********************************/
//********************************************************************************
/*!
\author     Kraemer E   
\date       13.02.2019  

\fn         Actors_Init

\brief      Initializes the Actors module. Actually only initializes the port mask

\return     none

\param      none
***********************************************************************************/
void Actors_Init(void)
{
    InitializePortMasks();
    
    /* Link All-Port-Isr to specific callback */
    AllPortIsr_StartEx(GPIOInterruptHandler);
    
    /* Set LED input */
    Actors_SetOutputStatus(eLedGreen, false);    
    Actors_SetOutputStatus(eLedRed, false);  
}


//********************************************************************************
/*!
\author     Kraemer E   
\date       12.02.2019  

\fn         Actors_ReadSense

\brief      Reads the state of the GPIO pin that are used for digital input.

\return     boolean - True for high and false for low

\param      eSenseLine - The sense line which should be read
***********************************************************************************/
bool Actors_ReadDigitalSense(teSenseLine eSenseLine)
{    
    return CY_SYS_PINS_READ_PIN(sSenseMap[eSenseLine].ulPortStateRegister, sSenseMap[eSenseLine].ucBitShift);    
}


//********************************************************************************
/*!
\author     Kraemer E   
\date       12.02.2019   

\fn         Actors_ReadOutputStatus

\brief      Reads the state of the GPIO pin that are used for digital output.

\return     boolean - True for high and false for low

\param      eOutput - The output which should be read
***********************************************************************************/
bool Actors_ReadOutputStatus(teOutput eOutput)
{    
    return CY_SYS_PINS_READ_PIN(sOutputMap[eOutput].ulPortStateRegister, sOutputMap[eOutput].ucBitShift);    
}


//********************************************************************************
/*!
\author     Kraemer E   
\date       12.02.2019   

\fn         Actors_SetOutputStatus

\brief      Set the state of the GPIO pin that are used for digital output.

\return     boolean - True for high and false for low

\param      eSenseLine - The sense line which should be read
***********************************************************************************/
void Actors_SetOutputStatus(teOutput eOutput, bool bStatus)
{   
    if(bStatus)
    {
        CY_SYS_PINS_SET_PIN(sOutputMap[eOutput].ulPortOutputDataRegister, sOutputMap[eOutput].ucBitShift);
    }
    else
    {
        CY_SYS_PINS_CLEAR_PIN(sOutputMap[eOutput].ulPortOutputDataRegister, sOutputMap[eOutput].ucBitShift);
    }  
}

//********************************************************************************
/*!
\author     LauserA   
\date       30.07.2018  

\fn         Actors_HAL_ReadPortPinStateRegister (u8 ucPort)

\brief      Reads the port pin state register of the choosen port

\return     ucPortDataRegisterValue - Value from the Register output

\param      ucPort - Portnumber which should be read.

***********************************************************************************/
u8 Actors_ReadPortPinStateRegister(u8 ucPort)
{
    u8 ucPortRegisterValue = 0;
    u32 ulDataRegisterAdress = 0;
    
    if (ucPort < NUMBER_OF_PORTS)
    {
        ulDataRegisterAdress = CYREG_GPIO_PRT0_PS + (CYDEV_GPIO_PRT0_SIZE * ucPort);
        ucPortRegisterValue = (u8)(*(reg32*)ulDataRegisterAdress);
    }
       
    return ucPortRegisterValue;
}


//********************************************************************************
/*!
\author     LauserA   
\date       30.07.2018  

\fn         Actors_HAL_ReadPortDataRegister (u8 ucPort)

\brief      Reads the port output data register of the choosen port

\return     ucPortDataRegisterValue - Value from the Register output

\param      ucPort - Portnumber which should be read.

***********************************************************************************/
u8 Actors_ReadPortDataRegister(u8 ucPort)
{
    u8 ucPortRegisterValue = 0;
    u32 ulDataRegisterAdress = 0;
    
    if (ucPort < NUMBER_OF_PORTS)
    {
        ulDataRegisterAdress = CYREG_GPIO_PRT0_DR + (CYDEV_GPIO_PRT0_SIZE * ucPort);
        ucPortRegisterValue = (u8)(*(reg32*)ulDataRegisterAdress);
    }
    
    return ucPortRegisterValue;
}


//********************************************************************************
/*!
\author     Kraemer E   
\date       12.02.2019   

\fn         Actors_GetPortPinMaskStatus

\brief      Returns the status of the port pin mask

\return     boolean - True for high and false for low

\param      none
***********************************************************************************/
bool Actors_GetPortPinMaskStatus(void)
{
    return bPortMasksInitialized;
}


//********************************************************************************
/*!
\author     Kraemer E   
\date       03.06.2019   

\fn         Actors_Sleep

\brief      Changes the drive mode of the pins into high impedance analog

\return     none

\param      none
***********************************************************************************/
void Actors_Sleep(void)
{
    u8 ucPinIdx = 0;
    
    for(ucPinIdx = 0; ucPinIdx < _countof(sOutputMap); ucPinIdx++)
    {
        CY_SYS_PINS_CLEAR_PIN(sOutputMap[ucPinIdx].ulPortOutputDataRegister, sOutputMap[ucPinIdx].ucBitShift);
        CY_SYS_PINS_SET_DRIVE_MODE(sOutputMap[ucPinIdx].ulPortConfigRegister, sOutputMap[ucPinIdx].ucBitShift, CY_SYS_PINS_DM_ALG_HIZ);
    }
}

//********************************************************************************
/*!
\author     Kraemer E   
\date       03.06.2019   
\brief      Changes the drive mode of the pins into initial drive mode before went
            to sleep
\return     none
\param      none
***********************************************************************************/
void Actors_Wakeup(void)
{
    
}

//********************************************************************************
/*!
\author     Kraemer E   
\date       15.11.2020   
\brief      Checks if the PWM module is active
\return     bPwmEnabled - True when register bit is set otherwise false
\param      ucOutputIdx - The output which shall be checked
***********************************************************************************/
bool Actors_GetPwmStatus(u8 ucOutputIdx)
{
    bool bPwmEnabled = false;
    if(ucOutputIdx < DRIVE_OUTPUTS)
    {
        switch(ucOutputIdx)
        {   
            case 0: 
                bPwmEnabled = (PWM_0_BLOCK_CONTROL_REG & PWM_0_MASK) ? true : false;
                break;
                
            case 1: 
                bPwmEnabled = (PWM_1_BLOCK_CONTROL_REG & PWM_1_MASK) ? true : false;
                break;
                
            case 2: 
                bPwmEnabled = (PWM_2_BLOCK_CONTROL_REG & PWM_2_MASK) ? true : false;
                break;                
        }
    }
    
    return bPwmEnabled;
}