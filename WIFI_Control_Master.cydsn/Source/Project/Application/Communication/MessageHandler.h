//********************************************************************************
/*!
\author     Kraemer E
\date       30.01.2019

\file       MessageHandler.h
\brief      Handler for the serial communication messages

***********************************************************************************/

#ifndef _MESSAGEHANDLER_H_
#define _MESSAGEHANDLER_H_

#ifdef __cplusplus
extern "C"
{
#endif    
  
#include "Messages.h"

void MessageHandler_HandleSerialCommEvent(void);
void MessageHandler_SendFaultMessage(const u16 uiErrorCode);
bool MessageHandler_GetActorsConfigurationStatus(void);
void MessageHandler_Tick(u8 ucElapsedMs);
void MessageHandler_SendSleepOrWakeUpMessage(bool bSleep);
void MessageHandler_SendInitDone(void);
void MessageHandler_SendOutputState(void);
void MessageHandler_ClearAllTimeouts(void);

#ifdef __cplusplus
}
#endif    

#endif //_MESSAGEHANDLER_H_
