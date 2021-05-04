

/********************************* includes **********************************/
#include "HAL_IO.h"
#include "Project_Config.h"

#include "ErrorDetection.h"
#include "OS_ErrorDebouncer.h"
#include "Aom_Regulation.h"
#include "Aom_Measure.h"
#include "Measure_Current.h"

/************************* local function prototypes *************************/
static bool ValidateOutputPorts(tFaultOutputPortValues* psFaultOutputPortValues);


/************************* local data (const and var) ************************/


/************************ export data (const and var) ************************/
/****************************** local functions ******************************/
//********************************************************************************
/*!
\author     KraemerE   
\date       12.07.2018  
\fn         ValidateOutputPorts
\brief      Compares the data register with the state register of the choosen port. 
\return     none
\param      psFaultOutputPortValues - Invalid bit information about the triacs. Bitnumber is equal to the triac number
***********************************************************************************/
static bool ValidateOutputPorts(tFaultOutputPortValues* psFaultOutputPortValues)
{
    bool bPinFaultFound = false;
    
    if(psFaultOutputPortValues)
    {    
        u8 ucPortOutputStatusRegVal = 0;
        u8 ucPortOutputDataRegVal = 0;
        u8 ucPortIdx = 0;
        
        u16 uiInvalidOutputs = 0;
        
        /* Check if port masks are initialized */
        if(!Actors_GetPortPinMaskStatus())
        {
            Actors_Init();
        }

        for(ucPortIdx = NUMBER_OF_PORTS; ucPortIdx--;)
        {    
            /* Get the port mask */
            const u8* pucOutputPortMask = Aom_Regulation_GetActorsPortMask();
            
            /* Get interrupt save port register */
            CyGlobalIntDisable;
                ucPortOutputDataRegVal = Actors_ReadPortDataRegister(ucPortIdx);
                ucPortOutputStatusRegVal = Actors_ReadPortPinStateRegister(ucPortIdx);
            CyGlobalIntEnable;

            /* Mask both register */
            ucPortOutputDataRegVal &= pucOutputPortMask[ucPortIdx];
            ucPortOutputStatusRegVal &= pucOutputPortMask[ucPortIdx];
            
            /* Compare both register */
            if(!(ucPortOutputDataRegVal == ucPortOutputStatusRegVal))
            {
                /* Compared portregister is invalid */   
                u8 ucDiffRegVal = ucPortOutputDataRegVal ^ ucPortOutputStatusRegVal;
                
                /* Save the fault pins from this port */
                psFaultOutputPortValues->ucPinFaults[ucPortIdx] |= ucDiffRegVal;
                bPinFaultFound = true;                
                
                /* Check complete portregister */
                u8 ucBitIndex;
                for(ucBitIndex = 8; ucBitIndex--;)
                {
                    /* Get the invalid pin */
                    if((ucDiffRegVal >> ucBitIndex)&0x01)
                    {
                        /* Get the invalid relay or triac output */
                        u8 ucActorIdx;
                        for(ucActorIdx = COUNT_OF_OUTPUTS; ucActorIdx--;)
                        {
                            if(sOutputMap[ucActorIdx].ucPort == ucPortIdx)
                            {
                                if(sOutputMap[ucActorIdx].ucBitShift == ucBitIndex)
                                {
                                    uiInvalidOutputs |= 0x01 << ucActorIdx;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        /* Save pin validations */
        psFaultOutputPortValues->uiInvalidOutputs = uiInvalidOutputs;    
    }
    
    return bPinFaultFound;
}

/************************ externally visible functions ***********************/


//********************************************************************************
/*!
\author     KraemerE (Ego)
\date       13.07.2018
\fn         ErrorDetection_GetFaultPorts(u8* pucPinFaults, u8 ucArraySize)
\brief      Checks the Invalid-Variables and returns the affected connectors.
\param      pucGetPinFaults - Pointer to an array, where every array-index is equal to an port-index.
                              The value is equal to a bitfield of the port. Should be empty.
\param      pucSetPinFaults - Pointer to an array, where every array-index is equal to an port-index.
                              The value is equal to a bitfield of the port. Includes the values.
\param      ucGetArraySize - Size of the given array.
\param      ucSetArraySize - Size of the given array
\return     ucBitfieldFaultPorts - Returns a bitfield with the masked fault ports. 
***********************************************************************************/
u8 ErrorDetection_GetFaultPorts(u8* pucGetPinFaults, u8* pucSetPinFaults, u8 ucGetArraySize, u8 ucSetArraySize)
{
    u8 ucBitfieldFaultPorts = 0b0000000;
    
    if(pucGetPinFaults && pucSetPinFaults)
    {
        // Check given array size
        if(ucGetArraySize <= sizeof(ucSetArraySize))
        {
            memcpy(pucGetPinFaults, &pucSetPinFaults[0], ucGetArraySize);
        }
        
        //Save the fault ports in the bitfield
        u8 ucPortIdx;
        for(ucPortIdx = 0; ucPortIdx < NUMBER_OF_PORTS; ucPortIdx++)
        {
            if(pucSetPinFaults[ucPortIdx])
            {
                //Mask the bit number of the port
                ucBitfieldFaultPorts |= 0x01 << ucPortIdx;
            }
        }
    }
    
    return ucBitfieldFaultPorts;
}


//********************************************************************************
/*!
\author     KraemerE (Ego)
\date       12.07.2018
\fn         ErrorDetection_CheckOutputPins
\brief      uiInvalid-Variables will be written with the fault bit information.
            Example: uiInvalidOutputs = 0b0000.0000.0000.1001 => Pin_0 and Pin_3 fault.
\return     none
***********************************************************************************/
void ErrorDetection_CheckOutputPins(void)
{   
    tFaultOutputPortValues sFaultOutputPorts;
    memset(&sFaultOutputPorts, 0, sizeof(sFaultOutputPorts));
    
    //Returns the invalid sorted bitinformation of the output pins
    ValidateOutputPorts(&sFaultOutputPorts);
    
    //Check if there are any faults on the ports
    if(sFaultOutputPorts.uiInvalidOutputs)
    {
        /* Put error in queue */
        ErrorDebouncer_PutErrorInQueue(ePinFault);
    }
}

#if (WITHOUT_REGULATION == false) 
//********************************************************************************
/*!
\author     KraemerE (Ego)
\date       04.04.2019
\fn         ErrorDetection_CheckPwmOutput
\brief      Checks if the PWM module works probably by changing the compare value
            and reading the PWM pin.
            First check is for HIGH afterwards for LOW output
\return     bErrorFound - True when an error was found
***********************************************************************************/
bool ErrorDetection_CheckPwmOutput(u8 ucOutputIdx)
{
    bool bErrorFound = false;
#warning PWM-Test disabled. Need a second switch for OFF-State
#if 0
    /* Variables for testing */
    bool bLowCheck = false;
    bool bHighCheck = false;
    u8 ucPwmRetry = 0;
    
    /* Check if PWM is enabled, otherwise enable it */
    //if(((PWM_STATUS_REG >> PWM_RUNNING_STATUS_SHIFT) & 0x01) == false)
    //{
    //    PWM_Start();
    //}
    
    /* Change compare value of the PWM module. First check against HIGH */
    sPwmMap[ucOutputIdx].pfnWriteCompare(sPwmMap[ucOutputIdx].pfnReadPeriod());    
    CyDelay(10);
    for(ucPwmRetry = PWM_TEST_RETRY; ucPwmRetry--;)
    {
        if(Actors_ReadOutputStatus(ePwmOut_0 + ucOutputIdx) == ON)
        {
            bHighCheck = true;
            break;
        }
    }
    
    sPwmMap[ucOutputIdx].pfnWriteCompare(0);
    CyDelay(10);
    for(ucPwmRetry = PWM_TEST_RETRY; ucPwmRetry--;)
    {
        if(Actors_ReadOutputStatus(ePwmOut_0 + ucOutputIdx) == OFF)
        {
            bLowCheck = true;
            break;
        }
    }
    
    /* If an error was detected put it into the error queue */
    if(!(bLowCheck && bHighCheck))
    {
        ErrorDebouncer_PutErrorInQueue(ePinFault);
        bErrorFound = true;
    }
#endif
    return bErrorFound;
}

//********************************************************************************
/*!
\author     KraemerE (Ego)
\date       20.10.2019
\fn         ErrorDetection_CheckCurrentValue
\brief      Checks the current for border violation.
\return     none
***********************************************************************************/
void ErrorDetection_CheckCurrentValue(void)
{   
    /* Get the current value */
    u8 ucOutputIdx;
    for(ucOutputIdx = 0; ucOutputIdx < DRIVE_OUTPUTS; ucOutputIdx++)
    {    
        u16 uiIsCurrentVal = 0xFFFF;
        Aom_Measure_GetMeasuredValues(NULL, &uiIsCurrentVal, NULL, ucOutputIdx);
        
        /* Check for overlimit current */
        if(uiIsCurrentVal > MAX_MILLI_CURRENT_VALUE)
        {
            //Put over current fault into queue and use fault-0 as base
            ErrorDebouncer_PutErrorInQueue(eOverCurrentFault_0 + ucOutputIdx);
        }
    }
}

//********************************************************************************
/*!
\author     KraemerE (Ego)
\date       22.10.2020
\fn         ErrorDetection_CheckAmbientTemperature
\brief      Checks the ambient temperature for border violation.
\return     none
***********************************************************************************/
void ErrorDetection_CheckAmbientTemperature(void)
{
    /* Get the current value */
    s16 siTempValue = 0xFFFF;
    
    //Get temperature; Output index is irrelevant because there is only one NTC
    Aom_Measure_GetMeasuredValues(NULL, NULL, &siTempValue, 0);
    
    /* Check for overlimit current */
    if(siTempValue > MAX_AMBIENT_TEMPERATURE)
    {
        ErrorDebouncer_PutErrorInQueue(eOverTemperatureFault);
    }
}

#endif //(WITHOUT_REGULATION == false)