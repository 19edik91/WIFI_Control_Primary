

#ifndef ERRORHANDLER_H_
#define ERRORHANDLER_H_

#include "Faults.h"
#include "Aom.h"
/********************************* includes **********************************/


/***************************** defines / macros ******************************/


/****************************** type definitions *****************************/
typedef struct
{
    teErrorList eFaultCode;
    u8 ucSendTimeout;
    bool bHandlingOnLV;
}tSendErrorDelay;

typedef struct
{
    u16 uiRetryTimeOut;
    u8  ucRetryCount;
    teErrorList eFaultCode;
}tRetry;

/***************************** global variables ******************************/


/************************ externally visible functions ***********************/
u16 ErrorHandler_GetErrorTimeout(void);
void ErrorHandler_OneSecRetryTimerTick(void);
void ErrorHandler_HandleActualError(teErrorList eFaultCode, bool bSetError);
bool ErrorHandler_SendErrorMessage(void);

#endif /* ERRORHANDLER_H_ */
