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
    tMsgVersion sMsgVersion;
    
    /* Clear the structures */
    memset(&sMsgVersion, 0, sizeof(sMsgVersion));
    
    /* Fill them */
    sMsgVersion.uiVersion = 0x0001;
    
    /* Start to send the packet */
    OS_Communication_SendResponseMessage(eMsgVersion, &sMsgVersion, sizeof(tMsgVersion), eNoCmd);
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
    tMsgUserTimer sMsgUserTimer;
    
    /* Clear the structures */
    memset(&sMsgUserTimer, 0, sizeof(sMsgUserTimer));

    /* Get the regulation values */
    tRegulationValues sRegulationValues;
    memset(&sRegulationValues, 0, sizeof(sRegulationValues));    
    Aom_Regulation_GetRegulationValues(&sRegulationValues);
    
    //Get the amount of saved timers
    u8 ucBinaryTimer = sRegulationValues.sUserTimerSettings.ucSetTimerBinary;
    
    //Check if any timer has been set
    if(ucBinaryTimer == 0)
    {
        //No timer has been set. Send empty UserTimer message
        OS_Communication_SendResponseMessage(eMsgUserTimer, NULL, 0, eCmdSet);
    }
    else
    {    
        u8 ucTimerIdx;
        for(ucTimerIdx = USER_TIMER_AMOUNT; ucTimerIdx--;)
        {    
            if(ucBinaryTimer & (0x01 << ucTimerIdx))
            {
                sMsgUserTimer.ucStartHour = sRegulationValues.sUserTimerSettings.sTimer[ucTimerIdx].ucHourSet;
                sMsgUserTimer.ucStopHour = sRegulationValues.sUserTimerSettings.sTimer[ucTimerIdx].ucHourClear;
                sMsgUserTimer.ucStartMin = sRegulationValues.sUserTimerSettings.sTimer[ucTimerIdx].ucMinSet;
                sMsgUserTimer.ucStopMin = sRegulationValues.sUserTimerSettings.sTimer[ucTimerIdx].ucMinClear;
                sMsgUserTimer.ucTimerIdx = ucTimerIdx;
                
                /* Start to send the packet */
                OS_Communication_SendResponseMessage(eMsgUserTimer, &sMsgUserTimer, sizeof(tMsgUserTimer), eCmdSet);  
            }
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
    tMsgInitOutputState sMsgUserOutput;
    
    /* Clear the structures */
    memset(&sMsgUserOutput, 0, sizeof(sMsgUserOutput));

    /* Initialize output arrays with invalid values */
    u8 ucOutputIdx;
    for(ucOutputIdx = 0; ucOutputIdx < _countof(sMsgUserOutput.asOutputs); ucOutputIdx++)
    {
        sMsgUserOutput.asOutputs[ucOutputIdx].ucBrightness = 0xFF;
        sMsgUserOutput.asOutputs[ucOutputIdx].ucLedStatus = 0xFF;
        sMsgUserOutput.asOutputs[ucOutputIdx].ucOutputIndex = 0xFF;
    }
    
    /* Get the regulation values */
    tRegulationValues sRegulationValues;
    memset(&sRegulationValues, 0, sizeof(sRegulationValues));    
    Aom_Regulation_GetRegulationValues(&sRegulationValues);
        
    for(ucOutputIdx = 0; ucOutputIdx < DRIVE_OUTPUTS; ucOutputIdx++)
    {
        sMsgUserOutput.asOutputs[ucOutputIdx].ucBrightness = sRegulationValues.sLedValue[ucOutputIdx].ucPercentValue;
        sMsgUserOutput.asOutputs[ucOutputIdx].ucLedStatus = sRegulationValues.sLedValue[ucOutputIdx].bStatus;
        sMsgUserOutput.asOutputs[ucOutputIdx].ucOutputIndex = ucOutputIdx;
    } 
    
    sMsgUserOutput.ucAutomaticModeActive = sRegulationValues.sUserTimerSettings.bAutomaticModeActive;
    sMsgUserOutput.ucMotionDetectionOnOff = sRegulationValues.sUserTimerSettings.bMotionDetectOnOff;
    sMsgUserOutput.ucNightModeOnOff = sRegulationValues.bNightModeOnOff;
    sMsgUserOutput.ucBurnTime = sRegulationValues.sUserTimerSettings.ucBurningTime;

    /* Start to send the packet */
    OS_Communication_SendResponseMessage(eMsgInitOutputStatus, &sMsgUserOutput, sizeof(tMsgInitOutputState), eCmdSet);
    
}

//********************************************************************************
/*!
\author     Kraemer E
\date       08.12.2019
\brief      Sends a settings done message to the slave.
\return     void 
***********************************************************************************/
static void SendUserSettingsDone(void)
{    
    /* Start to send the packet */
    OS_Communication_SendRequestMessage(eMsgInitDone, NULL, 0, eNoCmd);
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
    tMsgUpdateOutputState sMsgResponse;
    
    /* Clear the structures */
    memset(&sMsgResponse, 0, sizeof(sMsgResponse));
     
    /* Get the regulation structure */
    tRegulationValues sRegValues;
    Aom_Regulation_GetRegulationValues(&sRegValues);
    
    tsAutomaticModeValues* psAutomaticVal = Aom_System_GetAutomaticModeValuesStruct();
    sMsgResponse.slRemainingBurnTime = psAutomaticVal->slBurningTimeMs;
    sMsgResponse.ucMotionDetectionOnOff = psAutomaticVal->bMotionDetected;
    sMsgResponse.ucNightModeOnOff = psAutomaticVal->bInNightModeTimeSlot;
    sMsgResponse.ucAutomaticModeOnOff = psAutomaticVal->bInUserTimerSlot;
    
    u8 ucOutputIdx;
    for(ucOutputIdx = 0; ucOutputIdx < DRIVE_OUTPUTS; ucOutputIdx++)
    {
        /* Fill the message-payload */
        sMsgResponse.ucBrightness = sRegValues.sLedValue[ucOutputIdx].ucPercentValue;
        sMsgResponse.ucLedStatus = sRegValues.sLedValue[ucOutputIdx].bStatus;
        sMsgResponse.ucOutputIndex = ucOutputIdx;
        
        /* Start to send the packet */
        OS_Communication_SendRequestMessage(eMsgUpdateOutputStatus, &sMsgResponse, sizeof(tMsgUpdateOutputState), eCmdSet);
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
    
    Aom_Time_SetUserTimerSettings(&sUserTimer, psMsgUserTimer->ucTimerIdx);
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
   
    teMessageType eResponse = eTypeAck;
       
    /* Switch to message ID */
    switch(eMessageId)
    {
        case eMsgCurrentTime:
        {
            /* Cast Payload */
            tMsgCurrentTime* psMsgCurrentTime = (tMsgCurrentTime*)psMsgFrame->sPayload.pucData;

            /* Save new received time */
            Aom_Time_SetReceivedTime(psMsgCurrentTime->ucHour, psMsgCurrentTime->ucMinutes, psMsgCurrentTime->ulTicks);
            break;
        }        
        
        case eMsgHeartBeatOutput:
        {
            if(Aom_System_GetSystemStarted() == true)
            {
                /* Cast payload */
                tMsgHeartBeatOutput* psHeartBeat = (tMsgHeartBeatOutput*)psMsgFrame->sPayload.pucData;
                
                bool bValuesChanged = Aom_Regulation_CompareCustomValue(psHeartBeat->ucBrightness, (bool)psHeartBeat->ucLedStatus, psHeartBeat->ucOutputIndex);
                
                if(bValuesChanged == true)
                {
                    /* Values changed altough no Request output message was received. Therefore send an update back to
                       enforce value update of the slaves */
                    SendUpdateOutputState();
                }
            }
            else
            {
                //Send InitDone message. When an output request is received although
                //the System settings aren't done means that the slave already received
                //all user settings. Wait for InitDone message as response
                SendUserSettingsDone();
            }
            break;
        }
        
        case eMsgRequestOutputStatus:
        {
            /* Check if system is already active */
            if(Aom_System_GetSystemStarted() == true)
            {            
                if(eCommand == eCmdSet)
                {                        
                    /* Cast payload first */
                    tMsgRequestOutputState* psMsgReqOutputState = (tMsgRequestOutputState*)psMsgFrame->sPayload.pucData;
                    
                                                                   
                    /* Set new values in AOM */
                    Aom_Regulation_CheckRequestValues(psMsgReqOutputState->ucBrightness, 
                                       psMsgReqOutputState->ucLedStatus,
                                       psMsgReqOutputState->ucInitMenuActive, 
                                       psMsgReqOutputState->ucOutputIndex);          
                                        
                    /* Set system started information */
                    //Aom_System_SetSystemStarted(true);
                    
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
            }
            else
            {
                //Send InitDone message. When an output request is received although
                //the System settings aren't done means that the slave already received
                //all user settings. Wait for InitDone message as response
                SendUserSettingsDone();
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
                tMsgManualInit* psMsgManualInit = (tMsgManualInit*)psMsgFrame->sPayload.pucData;
                
                /* Check first for correct data. Exclusive ored !*/
                if(psMsgManualInit->ucSetMaxValue != psMsgManualInit->ucSetMinValue)
                {
                    /* Read the current values */
                    u16 uiVoltageAdc = Aom_Measure_GetAdcIsValue(eMeasureChVoltage, psMsgManualInit->ucOutputIndex);
                    u16 uiCurrentAdc = Aom_Measure_GetAdcIsValue(eMeasureChCurrent, psMsgManualInit->ucOutputIndex);
                    u16 uiPwmCompVal = 0; 
                    HAL_IO_PWM_ReadCompare(psMsgManualInit->ucOutputIndex, &uiPwmCompVal);
                    
                    if(psMsgManualInit->ucSetMaxValue)
                    {
                        Aom_Regulation_SetMaxSystemSettings(uiCurrentAdc, uiVoltageAdc, uiPwmCompVal, psMsgManualInit->ucOutputIndex);
                    }
                    else if(psMsgManualInit->ucSetMinValue)
                    {
                        Aom_Regulation_SetMinSystemSettings(uiCurrentAdc, uiVoltageAdc, uiPwmCompVal, psMsgManualInit->ucOutputIndex);
                    }
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
            }
            break;
        }
        #endif
        
        case eMsgSystemStarted:
        {
            if(Aom_System_GetSystemStarted() == false)
            {
                /* Slave started, send values from flash to the slave */
                SendUserTimerSettings();
                SendUserOutputSettings();
                SendUserSettingsDone();
            }
            else
            {
                eResponse = eTypeDenied;
            }
            break;
        }
        
        case eMsgInitDone:
        {
            /* Slave has started system. */
            Aom_System_SetSystemStarted(true);
            break;
        }
        
        case eMsgUserTimer:
        {
            if(eCommand == eCmdSet)
            {
                tMsgUserTimer* psMsgUserTimer = (tMsgUserTimer*)psMsgFrame->sPayload.pucData;
                HandleUserTimerSettings(psMsgUserTimer);
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
            break;
        }
        
        case eMsgVersion:
        {               
            /* Version should be send back */
            if(eCommand == eCmdGet)
            {
                /* Send message with data as ACK */
                SendSoftwareVersion();
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
            tMsgStillAlive* psStillAlive = (tMsgStillAlive*)psMsgFrame->sPayload.pucData;
            if(psStillAlive->bRequest == 0x00 && psStillAlive->bResponse == 0xFF)
            {
                eResponse = eTypeAck;
            }
            else
            {
                eResponse = eTypeDenied;
            }
            break;
        }
        
        case eMsgErrorCode:
        {
            /* Cast payload */
            tMsgFaultMessage* psFaultMsg = (tMsgFaultMessage*)psMsgFrame->sPayload.pucData;
            break;
        }
        
        case eMsgEnableNightMode:
        {
            /* Cast payload first */
            tsMsgEnableNightMode* psNightMode = (tsMsgEnableNightMode*)psMsgFrame->sPayload.pucData;            
            Aom_Regulation_SetNightModeStatus(psNightMode->ucNightModeStatus);
            break;
        }
        
        case eMsgEnableAutomaticMode:
        {
            /* Cast payload first */
            tsMsgEnableAutomaticMode* psAutoMode = (tsMsgEnableAutomaticMode*)psMsgFrame->sPayload.pucData;            
            Aom_Regulation_SetAutomaticModeStatus(psAutoMode->ucAutomaticModeStatus);
            break;
        }
        
        case eMsgEnableMotionDetect:
        {
            tsMsgEnableMotionDetectStatus* psMotionDetect = (tsMsgEnableMotionDetectStatus*)psMsgFrame->sPayload.pucData;            
            Aom_Regulation_SetMotionDectionStatus(psMotionDetect->ucMotionDetectStatus, psMotionDetect->ucBurnTime);
            break;
        }
        
        default:
            eResponse = eTypeDenied;
            break;
    }
    return eResponse;
}

