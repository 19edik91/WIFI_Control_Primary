/**
 * @file    OS_ErrorHandler.c
 */

/********************************* includes **********************************/
#include "OS_EventManager.h"
#include "ErrorHandler.h"

/***************************** defines / macros ******************************/

/************************ local data type definitions ************************/
/************************* local function prototypes *************************/


/************************* local data (const and var) ************************/

/************************ export data (const and var) ************************/

/****************************** local functions ******************************/

/************************ externally visible functions ***********************/


//***************************************************************************
/*!
\author     KraemerE
\date       04.05.2021
\brief      Handles explicit user specific errors in dependency of the fault code
\return     bErrorHandled - True when handled specific fault. Default case
                            will use OS-default handling.
\param      eFaultCode - Faultcode name which shall be handled
\param      bSetError - True when the fault shall be handled and send. False
                        when the fault shall only be send.
******************************************************************************/
bool ErrorHandler_HandleActualError(teErrorList eFaultCode, bool bSetError)
{    
    bool bErrorHandled = true;
    
    if(bSetError)
    {        
        switch(eFaultCode)
        {           
            case eCommunicationTimeoutFault:
            {
                OS_EVT_PostEvent(eEvtCommTimeout, 0, 0);
                bErrorHandled = false;
                break;
            }
            
            case eMuxInvalid:
            {
                while(1);
                break;
            }
            
            /* Currently all faults are handled with the same handling */   
            //case eOutputVoltageFault_0:         
            //case eOutputVoltageFault_1:         
            //case eOutputVoltageFault_2:          
            //case eOutputVoltageFault_3:          
            //case eOverCurrentFault_0:            
            //case eOverCurrentFault_1:            
            //case eOverCurrentFault_2:            
            //case eOverCurrentFault_3:            
            //case eLoadMissingFault_0:            
            //case eLoadMissingFault_1:            
            //case eLoadMissingFault_2:            
            //case eLoadMissingFault_3:            
            //case eOverTemperatureFault_0:        
            //case eOverTemperatureFault_1:        
            //case eOverTemperatureFault_2:        
            //case eOverTemperatureFault_3:
            default:
            {
                /* When not handled in here, use OS-default handling */
                bErrorHandled = false;
            
                break;
            }
        }
    }
    
    return bErrorHandled;
}
