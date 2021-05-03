//********************************************************************************
/*!
\author     Kraemer E
\date       30.01.2019

\file       MessageHandler.c
\brief      Handler for the serial communication messages

***********************************************************************************/
#include "BaseTypes.h"
#include "MessageHandler.h"
#include "Messages.h"
#include "Serial.h"
#include "Actors.h"
#include "ErrorDebouncer.h"
#include "HelperFunctions.h"
#include "RequestResponseHandler.h"
#include "ResponseDeniedHandler.h"
#include "Aom_System.h"
#include "Aom_Measure.h"

//#include "Version\Version.h"
/****************************************** Defines ******************************************************/
#define INVALID_MESSAGES_MAX    10
#define ACTORS_TIMEOUT          60000   //60 seconds; after an communictaion fault is generated
#define SEND_ALIVE_TIMEOUT      30000   //30 seconds; After this time a send still alive message is send
#define SEND_ALIVE_INTERVAL     2000     //Send only every 2 seconds a message

/****************************************** Variables ****************************************************/
static u16 uiCommTimeout = 0;

/****************************************** Function prototypes ******************************************/
static inline void ResetCommTimeout(void);
static void CheckForCommTimeout(u8 ucElapsedTime);
static void SendStillAliveMessage(bool bRequest);
static void HandleMessage(tsMessageFrame* psMsgFrame);

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
        ErrorDebouncer_PutErrorInQueue(eCommunicationTimeoutFault);
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
    tsMessageFrame sMsgFrame;  
    tMsgStillAlive sMsgStillAlive;
        
    /* Clear the structures */
    memset(&sMsgFrame, 0, sizeof(sMsgFrame));
    memset(&sMsgStillAlive, 0, sizeof(sMsgStillAlive));  
    
    /* Fill them */
    sMsgFrame.sPayload.ucCommand = eCmdSet;
    sMsgFrame.sPayload.ucMsgId = eMsgStillAlive;
    sMsgStillAlive.bRequest  = (bRequest == true) ? 0xFF : 0;
    sMsgStillAlive.bResponse = (bRequest == false) ? 0xFF : 0;
    sMsgFrame.sHeader.ucMsgType = eTypeRequest;

    /* Fill frame payload */
    memcpy(&sMsgFrame.sPayload.ucData[0], &sMsgStillAlive, sizeof(tMsgStillAlive));
    
    /* Fill header and checksum */
    HF_CreateMessageFrame(&sMsgFrame);

    /* Start to send the packet */
    HF_SendMessage(&sMsgFrame);
}

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
static void HandleMessage(tsMessageFrame* psMsgFrame)
{
    bool bValidMessage = true;
    static u8 ucInvalidMessageCounter = 0;    

    /* Get payload */    
    const teMessageId eMessageId = HF_GetObject(psMsgFrame);
    const teMessageType eMsgType = HF_GetMessageType(psMsgFrame);
    
    teMessageType eResponse = eNoType;
    
    /* Check for valid message address */
    if(HF_ValidateMessageAddresses(psMsgFrame))
    {    
        /* Check if the message is a response or a request */
        switch(eMsgType)
        {
            case eTypeRequest:
            {
                eResponse = ReqResMsg_Handler(psMsgFrame);
                break;
            }
            
            case eTypeAck:
            {
                //Clear message from buffer
                HF_ResponseReceived(psMsgFrame);
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
        HF_SendResponseMessage(eMessageId, eResponse);
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
            ErrorDebouncer_PutErrorInQueue(eCommunicationFault);
        }
    }
    return;
}


/****************************************** External visible functiones **********************************/

//********************************************************************************
/*!
\author     KraemerE    
\date       30.01.2019  

\fn         MessageHandler_SendFaultMessage 

\brief      Creates a message with the fault code and sends it further

\return     none 

\param      uiErrorCode - The error code which should be send

***********************************************************************************/
void MessageHandler_SendFaultMessage(const u16 uiErrorCode)
{
    /* Create structure */
    tsMessageFrame sMsgFrame;  
    tMsgFaultMessage sMsgFault;

    /* Clear the structures */
    memset(&sMsgFrame, 0, sizeof(sMsgFrame));
    memset(&sMsgFault, 0, sizeof(sMsgFault));

    /* Fill them */
    sMsgFrame.sPayload.ucCommand = eCmdSet;
    sMsgFrame.sPayload.ucMsgId = eMsgErrorCode;
    sMsgFault.uiErrorCode = uiErrorCode;
    sMsgFrame.sHeader.ucMsgType = eTypeRequest;

    //    sMsgVersion.uiCrc = (u16)SelfTest_FlashCRCRead(ST_FLASH_SEGIDX_S1);
    memcpy(&sMsgFrame.sPayload.ucData[0], &sMsgFault, sizeof(sMsgFault));

    /* Fill header and checksum */
    HF_CreateMessageFrame(&sMsgFrame);

    /* Start to send the packet */
    HF_SendMessage(&sMsgFrame);
}


//********************************************************************************
/*!
\author     KraemerE    
\date       22.05.2019  

\fn         MessageHandler_SendSleepOrWakeUpMessage 

\brief      Creates a message which requests the sleep or wake up mode

\return     none 

\param      bSleep - True to send a sleep request otherwise sends a wake up request

***********************************************************************************/
void MessageHandler_SendSleepOrWakeUpMessage(bool bSleep)
{
    /* Create structure */
    tsMessageFrame sMsgFrame;  

    /* Clear the structures */
    memset(&sMsgFrame, 0, sizeof(sMsgFrame));

    /* Fill them */
    sMsgFrame.sPayload.ucCommand = eCmdSet;
    sMsgFrame.sPayload.ucMsgId = bSleep ? eMsgSleep : eMsgWakeUp;
    sMsgFrame.sHeader.ucMsgType = eTypeRequest;

    /* Fill header and checksum */
    HF_CreateMessageFrame(&sMsgFrame);

    /* Start to send the packet */
    HF_SendMessage(&sMsgFrame);
}

//********************************************************************************
/*!
\author     KraemerE    
\date       08.11.2019  

\fn         MessageHandler_SendInitDone 

\brief      Creates a message which sends just a response that the initialization
            is done

\return     none 

\param      none

***********************************************************************************/
void MessageHandler_SendInitDone(void)
{
    /* Create structure */
    tsMessageFrame sMsgFrame;  

    /* Clear the structures */
    memset(&sMsgFrame, 0, sizeof(sMsgFrame));

    /* Fill them */
    sMsgFrame.sPayload.ucCommand = eCmdSet;
    sMsgFrame.sPayload.ucMsgId = eMsgAutoInitHardware;
    sMsgFrame.sHeader.ucMsgType = eTypeRequest;

    /* Fill header and checksum */
    HF_CreateMessageFrame(&sMsgFrame);

    /* Start to send the packet */
    HF_SendMessage(&sMsgFrame);
}

//********************************************************************************
/*!
\author     Kraemer E
\date       30.01.2019

\fn         MessageHandler_HandleSerialCommEvent

\brief      Is called whenever an message was received. Used by the Event-Handler

\return     void 

***********************************************************************************/
void MessageHandler_HandleSerialCommEvent(void)
{
    //Buffer is bigger than message frame because a fault could lead to a
    //bigger message size and with this also to an error.
//    u8  ucRxBuffer[sizeof(tsMessageFrame)+sizeof(u32)];
    u8 ucRxBuffer[255];
    u8  ucRxCount = sizeof(ucRxBuffer);
    u32 ulCalcCrc32 = INITIAL_CRC_VALUE;    
    
    /* Clear buffer first */
    memset(&ucRxBuffer[0], 0, ucRxCount);
    
    /* Get message from the buffer */
    if(Serial_GetPacket(&ucRxBuffer[0], &ucRxCount))
    {
        /* Get the whole message first and cast it into the message frame */
        tsMessageFrame* psMsgFrame = (tsMessageFrame*)ucRxBuffer;
                
        /* Calculate the CRC for this message */
        ulCalcCrc32 = HF_CreateMessageCrc(&ucRxBuffer[0], HF_GetMessageSizeWithoutCrc(psMsgFrame));
        
        /* Check for correct CRC */
        if(psMsgFrame->ulCrc32 == ulCalcCrc32)
        {        
            /* Handle message */
            HandleMessage(psMsgFrame);
        }
        else
        {           
            /* Put into error queue */
            ErrorDebouncer_PutErrorInQueue(eMessageCrcFault);
        }
    }
}

//********************************************************************************
/*!
\author     KraemerE    
\date       11.04.2019  

\fn         MessageHandler_Tick 

\brief      This function handles the saved frames and resend them when the timeout
            has reached. When the retry counter is counted down to zero the message
            is discarded.

\return     none

\param      ucElapsedMs - The elapsed time in milliseconds since the last call

***********************************************************************************/
void MessageHandler_Tick(u8 ucElapsedMs)
{
    HF_Tick(ucElapsedMs);
    
    /* Check for communication faults only in active state;
       because in the standby state the slave is off */
    if(Aom_System_IsStandbyActive() == false && Aom_System_GetSlaveResetState() == false)
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
    tsMessageFrame sMsgFrame;  
    tMsgOutputState sMsgResponse;
    
    /* Clear the structures */
    memset(&sMsgFrame, 0, sizeof(sMsgFrame));
    memset(&sMsgResponse, 0, sizeof(sMsgResponse));
    
    /* Fill them */
    sMsgFrame.sPayload.ucCommand = eCmdGet;
    sMsgFrame.sPayload.ucMsgId = eMsgOutputState;
    sMsgFrame.sHeader.ucMsgType = eTypeRequest;
    
    /* Get the measured values */
    u8 ucOutputIdx;    
    for(ucOutputIdx = 0; ucOutputIdx < DRIVE_OUTPUTS; ucOutputIdx++)
    {
        Aom_Measure_GetMeasuredValues(&sMsgResponse.ulVoltage, &sMsgResponse.uiCurrent, &sMsgResponse.siTemperature, ucOutputIdx);
        sMsgResponse.ucOutputIndex = ucOutputIdx;        
        
        memcpy(&sMsgFrame.sPayload.ucData[0], &sMsgResponse, sizeof(sMsgResponse));

        /* Fill header and checksum */
        HF_CreateMessageFrame(&sMsgFrame);
        
        /* Start to send the packet */
        HF_SendMessage(&sMsgFrame);
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