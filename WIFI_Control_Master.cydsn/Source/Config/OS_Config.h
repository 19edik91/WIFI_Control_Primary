/*
 * OS_SoftwareTimer_Config.h
 *
 *  Created on: 07.04.2021
 *      Author: kraemere
 */

#ifndef OS_CONFIG_H_
#define OS_CONFIG_H_

#include "CyLib.h"
    
#define MAX_EVENT_TIMER 8

#define SW_TIMER_10MS       10u
#define SW_TIMER_51MS       51u
#define SW_TIMER_251MS      251u
#define SW_TIMER_1001MS     1001u
    
#define EVT_SW_TIMER_10MS   (TIMER_TICK_OFFSET + SW_TIMER_10MS)
#define EVT_SW_TIMER_51MS   (TIMER_TICK_OFFSET + SW_TIMER_51MS)
#define EVT_SW_TIMER_251MS   (TIMER_TICK_OFFSET + SW_TIMER_251MS)
#define EVT_SW_TIMER_1001MS   (TIMER_TICK_OFFSET + SW_TIMER_1001MS)
    
    
#ifdef CY_PROJECT_NAME
#define EnterCritical()   CyDisableInts()
#define LeaveCritical(x)  CyEnableInts(x)
#endif
    
    
//#define VERBOSE   //Enable define for detailed messages

//Use of X-Macros for defining errors
/*              Error name                          |  Fault Code    |   Priority   |         Debouncer Count          */
#define STATE_LIST \
   ERROR(   eOutputVoltageFault_0                    ,    0xA001      ,       3     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eOutputVoltageFault_1                    ,    0xA002      ,       3     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eOutputVoltageFault_2                    ,    0xA003      ,       3     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eOutputVoltageFault_3                    ,    0xA004      ,       3     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eOverCurrentFault_0                      ,    0xA005      ,       2     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eOverCurrentFault_1                      ,    0xA006      ,       2     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eOverCurrentFault_2                      ,    0xA007      ,       2     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eOverCurrentFault_3                      ,    0xA008      ,       2     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eLoadMissingFault_0                      ,    0xA009      ,       2     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eLoadMissingFault_1                      ,    0xA00A      ,       2     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eLoadMissingFault_2                      ,    0xA00B      ,       2     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eLoadMissingFault_3                      ,    0xA00C      ,       2     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eOverTemperatureFault_0                  ,    0xA010      ,       2     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eOverTemperatureFault_1                  ,    0xA011      ,       2     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eOverTemperatureFault_2                  ,    0xA012      ,       2     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )\
   ERROR(   eOverTemperatureFault_3                  ,    0xA013      ,       2     ,      FAULTS_DEBOUNCE_CNT_DEFAULT )

//#define TMGR_PostEvent(param1,param2) EVT_PostEvent(eEvtTimer,(uiEventParam1)param1,(ulEventParam2)param2)

#endif /* OS_CONFIG_H_ */
