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
#include "RequestResponseHandler.h"
#include "Aom_Regulation.h"
#include "Aom_System.h"
#include "Aom_Time.h"
#include "Aom_Measure.h"
#include "Aom_Flash.h"
#include "HAL_IO.h"
//#include "Version\Version.h"
/****************************************** Defines ******************************************************/

/****************************************** Function prototypes ******************************************/
/****************************************** local functions *********************************************/



//********************************************************************************
/*!
\author     Kraemer E
\date       30.01.2019

\fn         SendSoftwareVersion

\brief      Sends the software version to the implemented version

\return     void 

***********************************************************************************/
static void SendSoftwareVersion(void)
{
    /* Create structure */
    tsMessageFrame sMsgFrame;  
    tMsgVersion sMsgVersion;
    
    /* Clear the structures */
    memset(&sMsgFrame, 0, sizeof(sMsgFrame));
    memset(&sMsgVersion, 0, sizeof(sMsgVersion));
    
    /* Fill them */
    sMsgFrame.sPayload.ucCommand = eCmdGet;
    sMsgFrame.sPayload.ucMsgId = eMsgVersion;
    sMsgVersion.uiVersion = 0x0001;
    sMsgFrame.sHeader.ucMsgType = eTypeAck;

    memcpy(&sMsgFrame.sPayload.ucData[0], &sMsgVersion, sizeof(sMsgVersion));

    /* Fill header and checksum */
    OS_Communication_CreateMessageFrame(&sMsgFrame);
    
    /* Start to send the packet */
    OS_Communication_SendMessage(&sMsgFrame);
}

//********************************************************************************
/*!
\author     Kraemer E
\date       08.12.2019

\fn         SendUserTimerSettings

\brief      Sends the saved timer settings

\return     void 

***********************************************************************************/
static void SendUserTimerSettings(void)
{
    /* Create structure */
    tsMessageFrame sMsgFrame;  
    tMsgUserTimer sMsgUserTimer;
    
    /* Clear the structures */
    memset(&sMsgFrame, 0, sizeof(sMsgFrame));
    memset(&sMsgUserTimer, 0, sizeof(sMsgUserTimer));
    
    /* Fill them */
    sMsgFrame.sPayload.ucCommand = eCmdSet;
    sMsgFrame.sPayload.ucMsgId = eMsgUserTimer;    
    sMsgFrame.sHeader.ucMsgType = eTypeRequest;

    /* Get the regulation values */
    tRegulationValues sRegulationValues;
    memset(&sRegulationValues, 0, sizeof(sRegulationValues));    
    Aom_Regulation_GetRegulationValues(&sRegulationValues);
    
    //Get the amount of saved timers
    u8 ucBinaryTimer = sRegulationValues.sUserTimerSettings.ucSetTimerBinary;
    
    u8 ucTimerIdx;
    for(ucTimerIdx = USER_TIMER_AMOUNT; ucTimerIdx--;)
    {    
        if(ucBinaryTimer & (0x01 << ucTimerIdx))
        {
            sMsgUserTimer.ucStartHour = sRegulationValues.sUserTimerSettings.sTimer[ucTimerIdx].ucHourSet;
            sMsgUserTimer.ucStopHour = sRegulationValues.sUserTimerSettings.sTimer[ucTimerIdx].ucHourClear;
            sMsgUserTimer.ucStartMin = sRegulationValues.sUserTimerSettings.sTimer[ucTimerIdx].ucMinSet;
            sMsgUserTimer.ucStopMin = sRegulationValues.sUserTimerSettings.sTimer[ucTimerIdx].ucMinClear;
            sMsgUserTimer.b7TimerIdx = ucTimerIdx;
            
            memcpy(&sMsgFrame.sPayload.ucData[0], &sMsgUserTimer, sizeof(sMsgUserTimer)); 
            
            /* Fill header and checksum */
            OS_Communication_CreateMessageFrame(&sMsgFrame);
            
            /* Start to send the packet */
            OS_Communication_SendMessage(&sMsgFrame);  
        }
    }
}

//********************************************************************************
/*!
\author     Kraemer E
\date       08.12.2019

\fn         SendUserOutputSettings

\brief      Sends the saved user output settings

\return     void 

***********************************************************************************/
static void SendUserOutputSettings(void)
{
    /* Create structure */
    tsMessageFrame sMsgFrame;  
    tMsgInitOutputState sMsgUserOutput;
    
    /* Clear the structures */
    memset(&sMsgFrame, 0, sizeof(sMsgFrame));
    memset(&sMsgUserOutput, 0, sizeof(sMsgUserOutput));
    
    /* Fill them */
    sMsgFrame.sPayload.ucCommand = eCmdSet;
    sMsgFrame.sPayload.ucMsgId = eMsgInitOutputStatus;    
    sMsgFrame.sHeader.ucMsgType = eTypeRequest;

    /* Get the regulation values */
    tRegulationValues sRegulationValues;
    memset(&sRegulationValues, 0, sizeof(sRegulationValues));    
    Aom_Regulation_GetRegulationValues(&sRegulationValues);
        
    u8 ucOutputIdx;    
    for(ucOutputIdx = 0; ucOutputIdx < DRIVE_OUTPUTS; ucOutputIdx++)
    {
        sMsgUserOutput.b7Brightness = sRegulationValues.sLedValue[ucOutputIdx].ucPercentValue;
        sMsgUserOutput.bLedStatus = sRegulationValues.sLedValue[ucOutputIdx].bStatus;
        sMsgUserOutput.b3OutputIndex = ucOutputIdx;
        
        sMsgUserOutput.bAutomaticModeActive = sRegulationValues.sUserTimerSettings.bAutomaticModeActive;
        sMsgUserOutput.bMotionDetectionOnOff = sRegulationValues.sUserTimerSettings.bMotionDetectOnOff;
        sMsgUserOutput.bNightModeOnOff = sRegulationValues.bNightModeOnOff;
        sMsgUserOutput.ucBurnTime = sRegulationValues.sUserTimerSettings.ucBurningTime;
                
        memcpy(&sMsgFrame.sPayload.ucData[0], &sMsgUserOutput, sizeof(sMsgUserOutput));

        /* Fill header and checksum */
        OS_Communication_CreateMessageFrame(&sMsgFrame);
        
        /* Start to send the packet */
        OS_Communication_SendMessage(&sMsgFrame);
    }
}

//********************************************************************************
/*!
\author     Kraemer E
\date       27.05.2020
\fn         SendUpdateOutputState
\brief      Sends the updated output values
\return     void 
***********************************************************************************/
static void SendUpdateOutputState(void)
{
    /* Create structure */
    tsMessageFrame sMsgFrame;  
    tMsgUpdateOutputState sMsgResponse;
    
    /* Clear the structures */
    memset(&sMsgFrame, 0, sizeof(sMsgFrame));
    memset(&sMsgResponse, 0, sizeof(sMsgResponse));
    
    /* Fill them */
    sMsgFrame.sPayload.ucCommand = eCmdSet;
    sMsgFrame.sPayload.ucMsgId = eMsgUpdateOutputStatus;
 
    /* Get the regulation structure */
    tRegulationValues sRegValues;
    Aom_Regulation_GetRegulationValues(&sRegValues);
    
    tsAutomaticModeValues* psAutomaticVal = Aom_System_GetAutomaticModeValuesStruct();
    sMsgResponse.slRemainingBurnTime = psAutomaticVal->slBurningTimeMs;
    sMsgResponse.bMotionDetectionOnOff = psAutomaticVal->bMotionDetected;
    sMsgResponse.bNightModeOnOff = psAutomaticVal->bInNightModeTimeSlot;
    sMsgResponse.bAutomaticModeOnOff = psAutomaticVal->bInUserTimerSlot;
    
    u8 ucOutputIdx;
    for(ucOutputIdx = 0; ucOutputIdx < DRIVE_OUTPUTS; ucOutputIdx++)
    {
        /* Fill the message-payload */
        sMsgResponse.b7Brightness = sRegValues.sLedValue[ucOutputIdx].ucPercentValue;
        sMsgResponse.bLedStatus = sRegValues.sLedValue[ucOutputIdx].bStatus;
        sMsgResponse.b3OutputIndex = ucOutputIdx;
        
        sMsgFrame.sHeader.ucMsgType = eTypeRequest;
        memcpy(&sMsgFrame.sPayload.ucData[0], &sMsgResponse, sizeof(sMsgResponse));

        /* Fill header and checksum */
        OS_Communication_CreateMessageFrame(&sMsgFrame);
        
        /* Start to send the packet */
        OS_Communication_SendMessage(&sMsgFrame);
    }
}


//********************************************************************************
/*!
\author     Kraemer E
\date       08.12.2019

\fn         HandleUserTimerSettings

\brief      Saves the new timer in the AOM structure

\return     void 

***********************************************************************************/
static void HandleUserTimerSettings(tMsgUserTimer* psMsgUserTimer)
{   
    tsTimeFormat sUserTimer;
    sUserTimer.ucHourClear = psMsgUserTimer->ucStopHour;
    sUserTimer.ucHourSet = psMsgUserTimer->ucStartHour;
    sUserTimer.ucMinClear = psMsgUserTimer->ucStopMin;
    sUserTimer.ucMinSet = psMsgUserTimer->ucStartMin;
    
    Aom_Time_SetUserTimerSettings(&sUserTimer, psMsgUserTimer->b7TimerIdx);
}

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
teMessageType ReqResMsg_Handler(tsMessageFrame* psMsgFrame)
{
    /* Get payload */    
    const teMessageId eMessageId = OS_Communication_GetObject(psMsgFrame);
    const teMessageCmd eCommand = OS_Communication_GetCommand(psMsgFrame);    
   
    teMessageType eResponse = eNoType;
       
    /* Switch to message ID */
    switch(eMessageId)
    {
        case eMsgCurrentTime:
        {
            /* Cast Payload */
            tMsgCurrentTime* psMsgCurrentTime = (tMsgCurrentTime*)psMsgFrame->sPayload.ucData;

            /* Save new received time */
            Aom_Time_SetReceivedTime(psMsgCurrentTime->ucHour, psMsgCurrentTime->ucMinutes, psMsgCurrentTime->ulTicks);
            break;
        }        
        
        case eMsgRequestOutputStatus:
        {
            if(eCommand == eCmdSet)
            {                        
                /* Cast payload first */
                tMsgRequestOutputState* psMsgReqOutputState = (tMsgRequestOutputState*)psMsgFrame->sPayload.ucData;
                
                bool bInitMenuEnabled = false;
                
                /* Check if initialization menu is active */
                if(psMsgReqOutputState->bInitMenuActive != psMsgReqOutputState->bInitMenuActiveInv)
                {
                    bInitMenuEnabled = psMsgReqOutputState->bInitMenuActiveInv;
                }
                                                               
                /* Set new values in AOM */
                Aom_Regulation_CheckRequestValues(psMsgReqOutputState->b7Brightness, 
                                   psMsgReqOutputState->bLedStatus, 
                                   psMsgReqOutputState->bNightModeOnOff,
                                   psMsgReqOutputState->bMotionDetectionOnOff,
                                   psMsgReqOutputState->ucBurnTime,
                                   bInitMenuEnabled, 
                                   psMsgReqOutputState->bAutomaticModeActive,
                                   psMsgReqOutputState->b3OutputIndex);          
                
                /* Set message as acknowledged */
                eResponse = eTypeAck;
                
                /* Set system started information */
                Aom_System_SetSystemStarted(true);
                
                #if (WITHOUT_REGULATION == false)    
                    /* Send update output status */
                    SendUpdateOutputState();
                                        
                    ///* Send measured output values */
                    //SendOutputState();    
                #endif
            }
            else if( eCommand == eCmdGet)
            {
               //TODO: Send actual values back
            }
            break;
        }
        
        #if (WITHOUT_REGULATION == false) 
        case eMsgAutoInitHardware:
        {
            if(eCommand == eCmdSet)
            {
                /* Post init event */
                OS_EVT_PostEvent(eEvtInitRegulationValue, eEvtParam_InitRegulationStart, 0);
                
                /* Set message as acknowledged */
                eResponse = eTypeAck;
            }
            else
            {
                eResponse = eTypeDenied;
            }
            break;
        }
        
        case eMsgManualInitHardware:
        {
            if(eCommand == eCmdSet)
            {
                tMsgManualInit* psMsgManualInit = (tMsgManualInit*)psMsgFrame->sPayload.ucData;
                
                /* Check first for correct data. Exclusive ored !*/
                if(psMsgManualInit->bSetMaxValue != psMsgManualInit->bSetMinValue)
                {
                    /* Read the current values */
                    u16 uiVoltageAdc = Aom_Measure_GetAdcIsValue(eMeasureChVoltage, psMsgManualInit->ucOutputIndex);
                    u16 uiCurrentAdc = Aom_Measure_GetAdcIsValue(eMeasureChCurrent, psMsgManualInit->ucOutputIndex);
                    u16 uiPwmCompVal = 0; 
                    HAL_IO_PWM_ReadCompare(psMsgManualInit->ucOutputIndex, (u32*)&uiPwmCompVal);
                    
                    if(psMsgManualInit->bSetMaxValue)
                    {
                        Aom_Regulation_SetMaxSystemSettings(uiCurrentAdc, uiVoltageAdc, uiPwmCompVal, psMsgManualInit->ucOutputIndex);
                    }
                    else if(psMsgManualInit->bSetMinValue)
                    {
                        Aom_Regulation_SetMinSystemSettings(uiCurrentAdc, uiVoltageAdc, uiPwmCompVal, psMsgManualInit->ucOutputIndex);
                    }
                    
                    /* Set message as acknowledged */
                    eResponse = eTypeAck;
                }
            }
            else if(eCommand == eCmdGet)
            {
                //TODO..
                eResponse = eTypeDenied;
            }
            break;
        }
        
        case eMsgManualInitHwDone:
        {
            if(eCommand == eCmdSet)
            {
                /* System settings have changed */
                Aom_Flash_WriteSystemSettingsInFlash();
                
                /* Set message as acknowledged */
                eResponse = eTypeAck;
            }
            break;
        }
        #endif
        
        case eMsgSystemStarted:
        {
            /* Slave started, send values from flash to the slave */
            SendUserTimerSettings();
            SendUserOutputSettings();
            eResponse = eTypeAck;
            
            Aom_System_SetSystemStarted(true);
            break;
        }
        
        case eMsgUserTimer:
        {
            if(eCommand == eCmdSet)
            {
                tMsgUserTimer* psMsgUserTimer = (tMsgUserTimer*)psMsgFrame->sPayload.ucData;
                HandleUserTimerSettings(psMsgUserTimer);
                
                /* Set message as acknowledged */
                eResponse = eTypeAck;
            }
            break;
        }
        
        case eMsgWakeUp:
        {
            if(eCommand == eCmdSet)
            {                
                /* Generate a wake-up-event */
                OS_EVT_PostEvent(eEvtStandby_WakeUpReceived, 0, 0);
            }
            
            /* Set message as acknowledged */
            eResponse = eTypeAck;
            break;
        }
        
        case eMsgVersion:
        {               
            /* Version should be send back */
            if(eCommand == eCmdGet)
            {
                /* Send message with data as ACK */
                SendSoftwareVersion();
                
                /* Set message as acknowledged */
               //Response = eTypeResponseAck;
            }
            else
            {
                eResponse = eTypeDenied;
            }
            break;
        }
        
        case eMsgStillAlive:
        {
            /* Cast payload */
            tMsgStillAlive* psStillAlive = (tMsgStillAlive*)psMsgFrame->sPayload.ucData;
            if(psStillAlive->bRequest == 0x00 && psStillAlive->bResponse == 0xFF)
            {
                eResponse = eTypeAck;
            }
            else
            {
                eResponse = eTypeDenied;
            }
        }
        
        default:
            eResponse = eTypeDenied;
            break;
    }
    return eResponse;
}

