

#ifndef ERRORHANDLER_H_
#define ERRORHANDLER_H_


#include "OS_Faults.h"
    
/********************************* includes **********************************/

/***************************** defines / macros ******************************/

/****************************** type definitions *****************************/

/***************************** global variables ******************************/


/************************ externally visible functions ***********************/
bool ErrorHandler_HandleActualError(teErrorList eFaultCode, bool bSetError);


#endif /* ERRORHANDLER_H_ */
