
#ifndef ERRORDETECTION_H_
#define ERRORDETECTION_H_

/********************************* includes **********************************/
#include "BaseTypes.h"

/***************************** defines / macros ******************************/
/****************************** type definitions *****************************/
/***************************** global variables ******************************/
/************************ externally visible functions ***********************/
u8   DR_ErrorDetection_GetFaultPorts(u8* pucGetPinFaults, u8* pucSetPinFaults, u8 ucGetArraySize, u8 ucSetArraySize);
void DR_ErrorDetection_CheckOutputPins(void);
bool DR_ErrorDetection_CheckPwmOutput(u8 ucOutputIdx);
void DR_ErrorDetection_CheckCurrentValue(void);
bool DR_ErrorDetection_GetOverCurrentStatus(void);
void DR_ErrorDetection_CheckAmbientTemperature(void);
#endif /* ERRORDETECTION_H_ */
