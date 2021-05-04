//********************************************************************************
/*!
\author     Kraemer E
\date       30.01.2019

\file       MessageHandler.c
\brief      Handler for the serial communication messages

***********************************************************************************/
#include "BaseTypes.h"
#include "MessageHandler.h"
#include "OS_Serial_UART.h"
#include "OS_ErrorDebouncer.h"
#include "OS_Communication.h"
#include "ResponseDeniedHandler.h"

//#include "Version\Version.h"
/****************************************** Defines ******************************************************/

/****************************************** Function prototypes ******************************************/
/****************************************** local functions *********************************************/

/****************************************** External visible functiones **********************************/
//********************************************************************************
/*!
\author     KraemerE    
\date       30.01.2019  

\fn         HandleMessage 

\brief      Message handler which handles each message it got.

\return     void 

\param      pMessage - pointer to message
\param      ucSize   - sizeof whole message

***********************************************************************************/
teMessageType ResDeniedMsg_Handler(tsMessageFrame* psMsgFrame)
{
    /* Get payload */    
    const teMessageId eMessageId = OS_Communication_GetObject(psMsgFrame);   
    teMessageType eResponse = eNoType;
    
    //For other handling
    switch(eMessageId)
    {
        /* When sleep request is denied, stop the further handling of the sleep */
        case eMsgSleep:
        {
            OS_EVT_PostEvent(eEvtStandby, eEvtParam_ExitStandby, 0);
            eResponse = eTypeAck;
            break;
        }
        
        /* When wake-up is denied, than the slave is already awake */
        case eMsgWakeUp:
        {
            eResponse = eTypeAck;
            break;
        }
        
        default:
            break;
    }
    
    return eResponse;
}

