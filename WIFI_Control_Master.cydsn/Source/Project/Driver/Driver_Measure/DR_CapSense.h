/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#ifndef _CAPSENSE_H_
#define _CAPSENSE_H_

#ifdef __cplusplus
extern "C"
{
#endif   


#include "BaseTypes.h"
/* When CY_Sense_CapSense is defined use real functions instead of empty defines */
#ifdef CY_SENSE_CapSense_H
    void CapSense_Init(void);
    void CapSense_Handler(void);
#else
    #define CapSense_Init(void) 
    #define CapSense_Handler(void)
#endif


#ifdef __cplusplus
}
#endif    

#endif //_CAPSENSE_H_
