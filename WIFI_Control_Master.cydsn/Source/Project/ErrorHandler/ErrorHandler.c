/**
* @file    ErrorHandler.c
*
* @brief   TODO<Brief description of contents of file ErrorHandler.c>
*
* @author  $Author: kraemere $
*
* @date    $Date: 2019-01-17 11:49:38 +0100 (Do, 17 Jan 2019) $
*
* @version $Rev: 913 $
*
*/

/********************************* includes **********************************/
#include "ErrorHandler.h"
#include "MessageHandler.h"
#include "EventManager.h"
#include "Actors.h"
#include "Aom.h"

/***************************** defines / macros ******************************/
#define PERIPHERAL_BLOCK_COUNTER_2S  8  //8*250ms = 2s 
#define MAX_RETRY_COUNT             16
#define BITS_IN_SHORT               16
#define NUMBER_OF_SAVED_ERRORS      10
#define DELAY_IN_MILLISECONDS       1000
#define DELAY_FOR_ERROR_MESSAGE     DELAY_IN_MILLISECONDS/50    //Every 50Ms the counter will be decremented.
/************************ local data type definitions ************************/
/************************* local function prototypes *************************/
static void PutErrorMessageInBuffer(teErrorList eFaultCode);

/************************* local data (const and var) ************************/
static tSendErrorDelay sSendErrorDelay[NUMBER_OF_SAVED_ERRORS];
static tRetry sRetryData;
static u8 ucErrorInBuffer = 0;

/************************ export data (const and var) ************************/


/****************************** local functions ******************************/

//********************************************************************************
/*!
\author     KraemerE
\date       16.04.2018

\fn         PutErrorMessageInBuffer

\brief      Puts appearing errors in queue with an timed delay, to prevent an message overflow.

\param      eFaultCode - Faultcode which should be saved.

\return     none
***********************************************************************************/
static void PutErrorMessageInBuffer(teErrorList eFaultCode)
{
    bool bErrorFound = false;
    
    /* Check if faultcode is already saved */
    u8 ucErrorIdx; 
    for(ucErrorIdx = NUMBER_OF_SAVED_ERRORS; ucErrorIdx--;)
    {
        if(sSendErrorDelay[ucErrorIdx].eFaultCode == eFaultCode)
        {
            bErrorFound = true;
            break;
        }
    }
    
    /* If error wasn't found check for free slot */
    if(!bErrorFound)
    {
        for(ucErrorIdx = NUMBER_OF_SAVED_ERRORS; ucErrorIdx--;)
        {
            if(sSendErrorDelay[ucErrorIdx].eFaultCode == eNoError)
            {
                sSendErrorDelay[ucErrorIdx].eFaultCode = eFaultCode;
                sSendErrorDelay[ucErrorIdx].ucSendTimeout = DELAY_FOR_ERROR_MESSAGE;
                ucErrorInBuffer++;
                break;
            }
        }
    }
}



//********************************************************************************
/*!
\author     KraemerE (Ego)
\date       21.11.2018

\fn         SetRetryTimer

\brief      Sets the retry time for a tRetry-structure

\param      eFaultCode - Faultcode which should be saved.

\param      psRetry - Pointer to the retry structure which should be set.

\return     none
***********************************************************************************/
static void SetRetryTimer(tRetry* psRetry, teErrorList eFaultCode)
{
    /* Set retry counter*/
    if(psRetry->uiRetryTimeOut == 0 
        && psRetry->ucRetryCount < BITS_IN_SHORT)
    {
        psRetry->uiRetryTimeOut = (1 << ++(psRetry->ucRetryCount))-1;
        
        if(eFaultCode != eNoError)
            psRetry->eFaultCode = eFaultCode;
        
    }
    else if(psRetry->ucRetryCount == BITS_IN_SHORT)
    {
        psRetry->uiRetryTimeOut = (1 << psRetry->ucRetryCount)-1;
        
        if(eFaultCode != eNoError)
            psRetry->eFaultCode = eFaultCode;
    }
}

/************************ externally visible functions ***********************/

//********************************************************************************
/*!
\author     KraemerE
\date       16.04.2018

\fn         ErrorHandler_SendErrorMessage

\brief      Sends time based error messages to POD. Time base should be 50ms.
            Timeout is set to DELAY_FOR_ERROR_MESSAGE (= 350ms)

\return     none
***********************************************************************************/
bool ErrorHandler_SendErrorMessage(void)
{
    static u8 ucSendIndex = 0;
    bool bMessageSend = false;
    
    if(ucErrorInBuffer)
    {
        u8 ucIndex = 0;
        u8 ucLoopCounter = 0;
        
        /* Decrement timeout counter for all entrys */
        for(ucIndex = NUMBER_OF_SAVED_ERRORS; ucIndex--;)
        {
            if(sSendErrorDelay[ucIndex].ucSendTimeout > 0)
            {
                sSendErrorDelay[ucIndex].ucSendTimeout--;
            }
        }
        
        /* Use bool to indicate if error was found */                
        for(;ucSendIndex < NUMBER_OF_SAVED_ERRORS; ucSendIndex++)
        {
            /* Increment loop counter to prevent an while */
            if(++ucLoopCounter >= NUMBER_OF_SAVED_ERRORS)
            {
                /* Complete buffer checked without succsses, leave the loop */
                bMessageSend = false;
                break;
            }
            
            /* Check if entry is an error */
            if(sSendErrorDelay[ucSendIndex].eFaultCode != eNoError)
            {               
                /* If timeout is reached send message to LV */
                if(sSendErrorDelay[ucSendIndex].ucSendTimeout == 0)
                {
                    u16 uiErrorValue = sErrorArray[sSendErrorDelay[ucSendIndex].eFaultCode].uiErrorValue;
                    
                    /* Put event in queue */
                    EVT_PostEvent(eEvtSendError, uiErrorValue, 0);
                    bMessageSend = true;
                    
                    /* Clear entry */
                    sSendErrorDelay[ucSendIndex].bHandlingOnLV = false;
                    sSendErrorDelay[ucSendIndex].eFaultCode = eNoError;
                    sSendErrorDelay[ucSendIndex].ucSendTimeout = 0;
                    
                    /* Decrement buffer counter */
                    ucErrorInBuffer--;
                }
            }
            
            /* Set the index to the start, when the end is reached */
            if(ucSendIndex == (NUMBER_OF_SAVED_ERRORS - 1))
            {
                ucSendIndex = 0;
            }
            
            /* Increment the send index and stop the for-loop when a message was send */
            if(bMessageSend)
            {
                ucSendIndex++;
                break;
            }
        }
    }
    
    return bMessageSend;
}


//********************************************************************************
/*!
\author     KraemerE
\date       02.02.2018

\fn         ErrorHandler_GetErrorTimeout

\param      none

\brief      Returns the timeout of the pending error.

\return     u16 - Returns the timeout which is pending
***********************************************************************************/
u16 ErrorHandler_GetErrorTimeout(void)
{    
    return sRetryData.uiRetryTimeOut;
}


//********************************************************************************
/*!
\author     KraemerE (Ego)
\date       10.10.2017

\fn         ErrorHandler_1SecRetryTimerTick

\param      none

\brief      Decrement every second the retry timer.

\return     none.
***********************************************************************************/
void ErrorHandler_OneSecRetryTimerTick(void)
{
    /* While retry timer is running decrement the timeout 
       and put the error message in the buffer */
    if(sRetryData.uiRetryTimeOut)
    {
        sRetryData.uiRetryTimeOut--;
        PutErrorMessageInBuffer(sRetryData.eFaultCode);            
    }
    else
    {
        sRetryData.uiRetryTimeOut = 0;
    }
}


//***************************************************************************
/*!
\author     KraemerE
\date       13.12.2017

\fn         void ErrorHandler_HandleActualError

\brief      Handles explicit errors in dependency of the fault code

\return     void

\param      FaultCode

******************************************************************************/
void ErrorHandler_HandleActualError(teErrorList eFaultCode, bool bSetError)
{    
    if(bSetError)
    {        
        switch(eFaultCode)
        {           
            case eCommunicationTimeoutFault:
            {
                EVT_PostEvent(eEvtCommTimeout, 0, 0);
                break;
            }
            
            /* Currently all faults are handled with the same handling */
            //case ePinFault:
            //case eInputVoltageFault:
            //case eOutputVoltageFault:
            //case eCommunicationFault:
            //case eOverCurrentFault:
            //case eOverTemperatureFault:            
            //case eLoadMissingFault:            
            default:
            {
                /* Start the timeout for the locked regulation */
                SetRetryTimer(&sRetryData, eFaultCode);
            
                /* Request a lower output voltage */
                //lower output power is checked with "GetErrorTimeout"
            }
                break;
        }
    }
           
    /* Put error in Buffer */
    PutErrorMessageInBuffer(eFaultCode);
}
