//********************************************************************************
/*!
\author     Kraemer E
\date       30.01.2019

\file       MessageHandler.c
\brief      Handler for the serial communication messages

***********************************************************************************/
#include "BaseTypes.h"
#include "MessageHandler.h"
#include "OS_Messages.h"
#include "OS_Serial_UART.h"

#include "OS_ErrorDebouncer.h"
#include "OS_Communication.h"
#include "RequestResponseHandler.h"
#include "ResponseDeniedHandler.h"
#include "Aom_System.h"
#include "Aom_Measure.h"

//#include "Version\Version.h"
/****************************************** Defines ******************************************************/
#define INVALID_MESSAGES_MAX    10
#define ACTORS_TIMEOUT          15000   //15 seconds; after an communictaion fault is generated
#define SEND_ALIVE_TIMEOUT      10000   //10 seconds; After this time a send still alive message is send
#define SEND_ALIVE_INTERVAL     2000     //Send only every 2 seconds a message

/****************************************** Variables ****************************************************/
static u16 uiCommTimeout = 0;

/****************************************** Function prototypes ******************************************/
static inline void ResetCommTimeout(void);
static void CheckForCommTimeout(u8 ucElapsedTime);
static void SendStillAliveMessage(bool bRequest);
//static void HandleMessage(void* pvMsg);

/****************************************** local functions *********************************************/
//********************************************************************************
/*!
\author     KraemereE   
\date       26.10.2020 
\fn         ResetCommTimeout
\brief      When a valid message was received from the slave timeout will be reseted.
\return     void 
***********************************************************************************/
static inline void ResetCommTimeout(void)
{
    uiCommTimeout = 0;
}


//********************************************************************************
/*!
\author     KraemereE   
\date       26.10.2020 
\fn         ResetCommTimeout
\brief      When a valid message was received from the slave timeout will be reseted.
\param      ucElapsedTime - 
\return     void 
***********************************************************************************/
static void CheckForCommTimeout(u8 ucElapsedTime)
{
    //Use local count to send a still alive message every 500ms
    static u16 uiLocalCnt = 0;
    
    if(uiCommTimeout < ACTORS_TIMEOUT)
    {
        uiCommTimeout += ucElapsedTime;
                
        if(uiCommTimeout > SEND_ALIVE_TIMEOUT)
        {
            uiLocalCnt += ucElapsedTime;
            
            if(uiLocalCnt >= SEND_ALIVE_INTERVAL)
            {
                SendStillAliveMessage(true);
                uiLocalCnt = 0;
            }
        }
        else
        {
            uiLocalCnt = 0;
        }
    }
    else
    {
        OS_ErrorDebouncer_PutErrorInQueue(eCommunicationTimeoutFault);
        uiCommTimeout = ACTORS_TIMEOUT;
    }
}


//********************************************************************************
/*!
\author     KraemereE   
\date       26.10.2020 
\fn         SendStillAliveMessage()
\brief      Sends a message to the slave to check if its still alive
\param      bRequest - true when a request otherwise send a response
\return     none 
***********************************************************************************/
static void SendStillAliveMessage(bool bRequest)
{    
    /* Create structure */
    tMsgStillAlive sMsgStillAlive;
        
    /* Clear the structures */
    memset(&sMsgStillAlive, 0, sizeof(sMsgStillAlive));  

    sMsgStillAlive.bRequest  = (bRequest == true) ? 0xFF : 0;
    sMsgStillAlive.bResponse = (bRequest == false) ? 0xFF : 0;

    /* Start to send the packet */
    OS_Communication_SendRequestMessage(eMsgStillAlive, &sMsgStillAlive, sizeof(tMsgStillAlive), eNoCmd);
}

//********************************************************************************
/*!
\author     KraemerE    
\date       30.01.2019  
\brief      Message handler which handles each message it got.
\return     void 
\param      pMessage - pointer to message
\param      ucSize   - sizeof whole message
***********************************************************************************/
void MessageHandler_HandleMessage(void* pvMsg)
{
    tsMessageFrame* psMsgFrame = (tsMessageFrame*)pvMsg;
    
    bool bValidMessage = true;
    static u8 ucInvalidMessageCounter = 0;    

    /* Get payload */    
    const teMessageId eMessageId = OS_Communication_GetObject(psMsgFrame);
    const teMessageType eMsgType = OS_Communication_GetMessageType(psMsgFrame);
    
    teMessageType eResponse = eNoType;
    
    /* Check for valid message address */
    if(OS_Communication_ValidateMessageAddresses(psMsgFrame))
    {    
        /* Check if the message is a response or a request */
        switch(eMsgType)
        {
            case eTypeRequest:
            case eTypeResponse:
            {
                eResponse = ReqResMsg_Handler(psMsgFrame);
                break;
            }
            
            case eTypeAck:
            {
                //Clear message from buffer
                OS_Communication_ResponseReceived(psMsgFrame);
                break;
            }
            
            case eTypeDenied:
            {
                eResponse = ResDeniedMsg_Handler(psMsgFrame);               
                break;
            } 

            case eNoType:
            case eTypeUnknown:
            default:
                //invalid message
                break;
        }
    }
    else
    {   
        /* Invalid message received. Maybe UART TX & RX shorted */
        bValidMessage = false;
        
        /* Wrong address response with denied */
        eResponse = eTypeDenied;
    }
       
    /* Send response message */
    if(eResponse != eNoType)
    {
        OS_Communication_SendAcknowledge(eMessageId, psMsgFrame->sPayload.ucQueryID, eResponse);
    }

    /* Check for invalid messages */
    if(bValidMessage)
    {
        ResetCommTimeout();
        ucInvalidMessageCounter = 0;
    }
    else
    {
        if(++ucInvalidMessageCounter > INVALID_MESSAGES_MAX)
        {
            ucInvalidMessageCounter = INVALID_MESSAGES_MAX;
            OS_ErrorDebouncer_PutErrorInQueue(eCommunicationFault);
        }
    }
    return;
}


/****************************************** External visible functiones **********************************/

//********************************************************************************
/*!
\author     KraemerE    
\date       30.01.2019  
\brief      Creates a message with the fault code and sends it further
\return     none 
\param      uiErrorCode - The error code which should be send
***********************************************************************************/
void MessageHandler_SendFaultMessage(const u16 uiErrorCode)
{
    /* Create structure */
    tMsgFaultMessage sMsgFault;

    /* Clear the structures */
    memset(&sMsgFault, 0, sizeof(sMsgFault));

    //Fill message 
    sMsgFault.uiErrorCode = uiErrorCode;

    /* Start to send the packet */
    OS_Communication_SendRequestMessage(eMsgErrorCode, &sMsgFault, sizeof(tMsgFaultMessage), eCmdSet);
}


//********************************************************************************
/*!
\author     KraemerE    
\date       22.05.2019  
\brief      Creates a message which requests the sleep or wake up mode
\return     none 
\param      bSleep - True to send a sleep request otherwise sends a wake up request
***********************************************************************************/
void MessageHandler_SendSleepOrWakeUpMessage(bool bSleep)
{
    /* Start to send the packet */
    teMessageId eMsgID = (bSleep ? eMsgSleep : eMsgWakeUp);
    OS_Communication_SendRequestMessage(eMsgID, NULL, 0, eCmdSet);
}

//********************************************************************************
/*!
\author     KraemerE    
\date       08.11.2019  
\brief      Creates a message which sends just a response that the initialization
            is done
\return     none 
\param      none
***********************************************************************************/
void MessageHandler_SendInitDone(void)
{
    /* Start to send the packet */
    OS_Communication_SendResponseMessage(eMsgInitDone, NULL, 0, eNoCmd);
}


//********************************************************************************
/*!
\author     KraemerE    
\date       11.04.2019  
\brief      This function handles the saved frames and resend them when the timeout
            has reached. When the retry counter is counted down to zero the message
            is discarded.
\return     none
\param      ucElapsedMs - The elapsed time in milliseconds since the last call
***********************************************************************************/
void MessageHandler_Tick(u8 ucElapsedMs)
{
    OS_Communication_Tick(ucElapsedMs);
    
    /* Check for communication faults only in active state;
       because in the standby state the slave is off */
    if(Aom_System_GetSlaveResetState() == false)
    {
        CheckForCommTimeout(ucElapsedMs);
    }
}

#if (WITHOUT_REGULATION == false)
//********************************************************************************
/*!
\author     Kraemer E
\date       08.11.2019
\fn         MessageHandler_SendOutputState
\brief      Sends the current measured values (temperature, voltage and current)
\return     void 
***********************************************************************************/
void MessageHandler_SendOutputState(void)
{
    /* Create structure */
    tMsgOutputState sMsgResponse;
    
    /* Clear the structures */
    memset(&sMsgResponse, 0, sizeof(sMsgResponse));

    /* Get the measured values */
    u8 ucOutputIdx;    
    for(ucOutputIdx = 0; ucOutputIdx < DRIVE_OUTPUTS; ucOutputIdx++)
    {
        Aom_Measure_GetMeasuredValues(&sMsgResponse.ulVoltage, &sMsgResponse.uiCurrent, &sMsgResponse.siTemperature, ucOutputIdx);
        sMsgResponse.ucOutputIndex = ucOutputIdx;        

        /* Start to send the packet */
        OS_Communication_SendResponseMessage(eMsgOutputState, &sMsgResponse, sizeof(tMsgOutputState), eCmdSet);
    }
}
#endif


//********************************************************************************
/*!
\author     Kraemer E
\date       26.10.2020
\fn         MessageHandler_ClearAllTimeouts
\brief      Clears all specificed timeouts
\return     void 
***********************************************************************************/
void MessageHandler_ClearAllTimeouts(void)
{
    ResetCommTimeout();
}

//********************************************************************************
/*!
\author     Kraemer E
\date       13.06.2021
\brief      Returns status of the communication timout
\return     bCommTimeoutPending - True when timeout is pending otherwise false 
***********************************************************************************/
bool MessageHandler_GetCommunicationTimeoutStatus(void)
{
    bool bCommTimeoutPending = (uiCommTimeout == ACTORS_TIMEOUT) ? true : false;
    
    return bCommTimeoutPending;
}

//********************************************************************************
/*!
\author     Kraemer E
\date       05.05.2021
\fn         MessageHandler_Init
\brief      Links the message handler function with the OS-Communication module
\return     void 
***********************************************************************************/
void MessageHandler_Init(void)
{    
    OS_Communication_Init(MessageHandler_HandleMessage);
}