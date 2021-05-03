
#ifndef ERRORDETECTION_H_
#define ERRORDETECTION_H_

/********************************* includes **********************************/
#include "BaseTypes.h"
#include "Actors.h"

/***************************** defines / macros ******************************/
/****************************** type definitions *****************************/
/***************************** global variables ******************************/
/************************ externally visible functions ***********************/
u8 ErrorDetection_GetFaultPorts(u8* pucGetPinFaults, u8* pucSetPinFaults, u8 ucGetArraySize, u8 ucSetArraySize);
void ErrorDetection_CheckOutputPins(void);
bool ErrorDetection_CheckPwmOutput(u8 ucOutputIdx);
void ErrorDetection_CheckCurrentValue(void);
bool ErrorDetection_GetOverCurrentStatus(void);
void ErrorDetection_CheckAmbientTemperature(void);
#endif /* ERRORDETECTION_H_ */
