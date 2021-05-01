/*
 * OS_SoftwareTimer_Config.h
 *
 *  Created on: 07.04.2021
 *      Author: kraemere
 */

#ifndef OS_CONFIG_H_
#define OS_CONFIG_H_

#define MAX_EVENT_TIMER 8
#define CY_PROJECT_NAME

#define SW_TIMER_10MS       10u
#define SW_TIMER_51MS       51u
#define SW_TIMER_251MS      251u
#define SW_TIMER_1001MS     1001u

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


//*******************************************
//* Set the selftest actions active         *
#define SELFTEST_S_ENABLE       1   // The selftests that run once at startup

#if ! SELFTEST_S_ENABLE             // Cyclic selftest may only run if startup test is activated !
    #define SELFTEST_C_ENABLE   0   // The selftests that run cyclically in main loop
#else
    #define SELFTEST_C_ENABLE   1
#endif

//*******************************************
//* Defines for inclusion of startup tests  *

#define EXEC_STARTUP_CPUREG             1u          // Execute CPU_Reg test
#define EXEC_STARTUP_CPUPC              1u          // Execute CPU_PC test
#define EXEC_STARTUP_TIMEBASE           1u          // Execute Interrupt test
#define EXEC_STARTUP_RAM                0u          // Execute Ram test
#define EXEC_STARTUP_STACK              1u          // Execute Stack test
#define EXEC_STARTUP_FLASH              0u          // Execute Flash test
#define EXEC_STARTUP_IO                 0u          // Execute IO test
#define EXEC_STARTUP_UREG               0u          // Execute UDB config register test
#define EXEC_STARTUP_ADC                0u          // Execute ADC test
#define EXEC_STARTUP_UART               0u          // Do NOT execute UART test

//*******************************************
//* Defines for inclusion of cyclic tests   *

#define EXEC_CYCLIC_CPUREG              1u          // Execute CPU_Reg test
#define EXEC_CYCLIC_CPUPC               1u          // Execute CPU_PC test
#define EXEC_CYCLIC_TIMEBASE            1u          // Execute Interrupt test
#define EXEC_CYCLIC_RAM                 0u          // Execute SRAM March tests
#define EXEC_CYCLIC_STACK               1u          // Execute Stack March tests
#define EXEC_CYCLIC_STACKOVF            0u          // Execute Stack Overflow test
#define EXEC_CYCLIC_FLASH               0u          // Execute Flash test
#define EXEC_CYCLIC_IO                  0u          // Do NOT execute IO test on LV
#define EXEC_CYCLIC_UREG                0u          // Execute UDB config register test
#define EXEC_CYCLIC_ADC                 0u          // Do NOT execute ADC test on LV
#define EXEC_CYCLIC_UART                0u          // Do NOT execute UART test on LV

#endif /* OS_CONFIG_H_ */
