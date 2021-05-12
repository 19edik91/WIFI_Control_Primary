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
#ifndef CYAPICALLBACKS_H
#define CYAPICALLBACKS_H
    
    /*Define your macro callbacks here */
    /*For more information, refer to the Writing Code topic in the PSoC Creator Help.*/
    #define CY_BOOT_START_C_CALLBACK    
    extern void CyBoot_Start_c_Calback(void);
    
    #define CY_BOOT_INT_DEFAULT_HANDLER_EXCEPTION_ENTRY_CALLBACK
    extern void CyBoot_IntDefaultHandler_Exception_EntryCallback(void); 
    
#endif /* CYAPICALLBACKS_H */   
/* [] */
