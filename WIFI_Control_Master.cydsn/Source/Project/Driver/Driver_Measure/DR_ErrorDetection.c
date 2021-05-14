

/********************************* includes **********************************/
#include "Project_Config.h"

#include "OS_ErrorDebouncer.h"

#include "DR_ErrorDetection.h"

#include "HAL_System.h"
#include "HAL_IO.h"

#include "Aom_Regulation.h"
#include "Aom_Measure.h"
#include "Measure_Current.h"

/************************* local function prototypes *************************/
static bool ValidateOutputPorts(tsFaultOutputPortValues* psFaultOutputPortValues);


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
static bool ValidateOutputPorts(tsFaultOutputPortValues* psFaultOutputPortValues)
{
    bool bPinFaultFound = false;
    
    if(psFaultOutputPortValues)
    {    
        u8 ucPortOutputStatusRegVal = 0;
        u8 ucPortOutputDataRegVal = 0;
        u8 ucPortIdx = 0;
        
        /* Check if port masks are initialized */
        if(!HAL_IO_GetPortPinMaskStatus())
        {
            HAL_IO_Init();
        }

        for(ucPortIdx = NUMBER_OF_PORTS; ucPortIdx--;)
        {    
            /* Get the port mask */
            const u8* pucOutputPortMask = HAL_IO_GetOutputPinMask();
            
            /* Get interrupt save port register */
            CyGlobalIntDisable;
                ucPortOutputDataRegVal = HAL_IO_ReadPortDataRegister(ucPortIdx);
                ucPortOutputStatusRegVal = HAL_IO_ReadPortStateRegister(ucPortIdx);
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
                        /* Get the invalid output */
                        u8 ucActorIdx;
                        for(ucActorIdx = COUNT_OF_OUTPUTS; ucActorIdx--;)
                        {
                            u8 ucOutputPort = 0xFF;
                            u8 ucOutputBit = 0xFF;                            
                            HAL_IO_GetPortAndBitshiftIndex(ucActorIdx, &ucOutputPort,&ucOutputBit);
                            
                            if(ucOutputPort == ucPortIdx && ucOutputBit == ucBitIndex)
                            {
                                psFaultOutputPortValues->eOutputFaultList[psFaultOutputPortValues->ucFaultCnt] = (teOutput)ucActorIdx;
                                psFaultOutputPortValues->ucFaultCnt++;
                            }
                        }
                    }
                }
            }
        }
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
u8 DR_ErrorDetection_GetFaultPorts(u8* pucGetPinFaults, u8* pucSetPinFaults, u8 ucGetArraySize, u8 ucSetArraySize)
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
void DR_ErrorDetection_CheckOutputPins(void)
{   
    tsFaultOutputPortValues sFaultOutputPorts;
    memset(&sFaultOutputPorts.ucPinFaults[0], 0, sizeof(sFaultOutputPorts.ucPinFaults));
    memset(&sFaultOutputPorts.eOutputFaultList[0], eInvalidOutput, sizeof(sFaultOutputPorts.eOutputFaultList));
    sFaultOutputPorts.ucFaultCnt = 0;
    
    //Returns the invalid sorted bitinformation of the output pins
    ValidateOutputPorts(&sFaultOutputPorts);
    
    //Check if there are any faults on the ports
    if(sFaultOutputPorts.ucFaultCnt)
    {
        /* Put error in queue */
        OS_ErrorDebouncer_PutErrorInQueue(ePinFault);
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
bool DR_ErrorDetection_CheckPwmOutput(u8 ucOutputIdx)
{
    bool bErrorFound = false;

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
    u32 ulPeriodVal = 0;    
    teIO_Return eReadRet = HAL_IO_PWM_ReadPeriod((tePWM)ucOutputIdx, &ulPeriodVal);
    teIO_Return eWriteRet = HAL_IO_PWM_WriteCompare((tePWM)ucOutputIdx, ulPeriodVal);
    
    if(eReadRet == eIO_Success && eWriteRet == eIO_Success)
    {    
        //HAL_System_Delay(10);
        for(ucPwmRetry = PWM_TEST_RETRY; ucPwmRetry--;)
        {
            if(HAL_IO_ReadOutputStatus((ePin_PwmOut_0 + ucOutputIdx)) == ON)
            {
                bHighCheck = true;
                break;
            }
        }
        
        /* Set compare value to LOW to check for LOW level */
        eWriteRet = HAL_IO_PWM_WriteCompare((tePWM)ucOutputIdx, 0);
        
        if(eWriteRet == eIO_Success)
        {
            //HAL_System_Delay(10);
            for(ucPwmRetry = PWM_TEST_RETRY; ucPwmRetry--;)
            {
                if(HAL_IO_ReadOutputStatus((ePin_PwmOut_0 + ucOutputIdx)) == OFF)
                {
                    bLowCheck = true;
                    break;
                }
            }
        }
        
        /* If an error was detected put it into the error queue */
        if(!(bLowCheck && bHighCheck))
        {
            OS_ErrorDebouncer_PutErrorInQueue(ePmwFault);
            bErrorFound = true;
        }
    }

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
void DR_ErrorDetection_CheckCurrentValue(void)
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
            OS_ErrorDebouncer_PutErrorInQueue(eOverCurrentFault_0 + ucOutputIdx);
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
void DR_ErrorDetection_CheckAmbientTemperature(void)
{
    /* Get the current value */
    s16 siTempValue = 0xFFFF;
    
    //Get temperature; Output index is irrelevant because there is only one NTC
    Aom_Measure_GetMeasuredValues(NULL, NULL, &siTempValue, 0);
    
    /* Check for overlimit current */
    if(siTempValue > MAX_AMBIENT_TEMPERATURE)
    {
        OS_ErrorDebouncer_PutErrorInQueue(eOverTemperatureFault);
    }
}

#endif //(WITHOUT_REGULATION == false)