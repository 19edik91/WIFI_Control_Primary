/* ========================================
 *
 * Copyright Eduard Kraemer, 2019
 *
 * ========================================
*/
#include "DR_CapSense.h"
#include "Aom.h"

#ifdef CY_SENSE_CapSense_H
   
#if (WITHOUT_REGULATION == false)
/****************************************** Defines ******************************************************/
typedef enum
{
    eCS_SensorScan          = 0x01,
    eCS_WaitForScanComplete = 0x02,
    eCS_ProcessData         = 0x03
}teCapSenseState;

typedef struct
{
    bool bOldValue;
    bool bNewValue;
    u8 ucWidgetId;
}tsWidgetValue;

typedef struct
{
    tsWidgetValue sWidgetVal_BtnPlus;
    tsWidgetValue sWidgetVal_BtnMinus;
    tsWidgetValue sWidgetVal_BtnOnOff;
}tsWidgetButton;

typedef struct
{
    u8 ucLedBrightness;
    bool bOnOff;
}tsLedVal;

/****************************************** Variables ****************************************************/
static teCapSenseState eCS_State = eCS_SensorScan;
static tsWidgetButton sWidgetButtons;
static tsLedVal sLedVal;

/****************************************** Function prototypes ******************************************/

/****************************************** loacl functiones *********************************************/
//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\fn         MeasureInterrupt()
\brief      Interrupt function which should be called, whenever a ISR occurs.
\return     none
\param      none
***********************************************************************************/
void StateMachine(void)
{
    switch(eCS_State)
    {
        case eCS_SensorScan:
        {
            /* Initialize new scan process only when CapSens block is in idle */
            if(CapSense_IsBusy() == CapSense_NOT_BUSY)
            {
                /* Scan the widgets which are configured by the "CSDSetupWidget" API */
                CapSense_ScanAllWidgets();
                
                /* Change state for wait for scan complete */
                eCS_State = eCS_WaitForScanComplete;
            }
            break;
        }
        
        case eCS_WaitForScanComplete:
        {
            /* Wait until scan is done */
            if(CapSense_IsBusy() == CapSense_NOT_BUSY)
            {
                /* Switch to next state */
                eCS_State = eCS_ProcessData;
            }
            
            break;
        }
        
        case eCS_ProcessData:
        {
            /* Process data in all enabled widgets */
            CapSense_ProcessAllWidgets();
            
            /* Overwrite old data values */
            sWidgetButtons.sWidgetVal_BtnMinus.bOldValue = sWidgetButtons.sWidgetVal_BtnMinus.bNewValue;
            sWidgetButtons.sWidgetVal_BtnPlus.bOldValue = sWidgetButtons.sWidgetVal_BtnPlus.bNewValue;
            sWidgetButtons.sWidgetVal_BtnOnOff.bOldValue = sWidgetButtons.sWidgetVal_BtnOnOff.bNewValue;
            
            /* Get the button data */
            sWidgetButtons.sWidgetVal_BtnMinus.bNewValue = CapSense_IsWidgetActive(sWidgetButtons.sWidgetVal_BtnMinus.ucWidgetId);
            sWidgetButtons.sWidgetVal_BtnPlus.bNewValue = CapSense_IsWidgetActive(sWidgetButtons.sWidgetVal_BtnPlus.ucWidgetId);
            sWidgetButtons.sWidgetVal_BtnOnOff.bNewValue = CapSense_IsWidgetActive(sWidgetButtons.sWidgetVal_BtnOnOff.ucWidgetId);
            
            /* Check for changed values */
            if(sWidgetButtons.sWidgetVal_BtnMinus.bNewValue != sWidgetButtons.sWidgetVal_BtnMinus.bOldValue)
            {
                if(sWidgetButtons.sWidgetVal_BtnMinus.bNewValue)
                {
                    if(sLedVal.ucLedBrightness >= 10)
                        sLedVal.ucLedBrightness -= 10;
                }
            }
            
            if(sWidgetButtons.sWidgetVal_BtnPlus.bNewValue == true 
                && sWidgetButtons.sWidgetVal_BtnPlus.bNewValue != sWidgetButtons.sWidgetVal_BtnPlus.bOldValue)
            {
                if(sWidgetButtons.sWidgetVal_BtnPlus.bNewValue)
                {
                    if(sLedVal.ucLedBrightness <= 90)
                        sLedVal.ucLedBrightness += 10;
                }
            }
            
            if(sWidgetButtons.sWidgetVal_BtnOnOff.bNewValue != sWidgetButtons.sWidgetVal_BtnOnOff.bOldValue)
            {
                if(sWidgetButtons.sWidgetVal_BtnOnOff.bNewValue)
                    sLedVal.bOnOff ^= 0x01;
            }
            
            break;
        }
        
        default:
            break;
    }
}



/****************************************** External visible functiones **********************************/
//********************************************************************************
/*!
\author     Kraemer E.
\date       13.09.2020
\fn         CapSense_Init()
\brief      Initializes cap sense module
\return     none
\param      none
***********************************************************************************/
void CapSense_Init(void)
{
    /* Start cap-sense block */
    CapSense_Start();
    
    sWidgetButtons.sWidgetVal_BtnMinus.ucWidgetId = CapSense_CAPSENSEBTN_MIN_WDGT_ID;
    sWidgetButtons.sWidgetVal_BtnPlus.ucWidgetId = CapSense_CAPSENSEBTN_PLUS_WDGT_ID;
    sWidgetButtons.sWidgetVal_BtnOnOff.ucWidgetId = CapSense_CAPSENSEBTN_ONOF_WDGT_ID;
}
    
//********************************************************************************
/*!
\author     Kraemer E.
\date       13.09.2020
\fn         CapSense_Handler()
\brief      Handles the cap sense module
\return     none
\param      none
***********************************************************************************/
void CapSense_Handler(void)
{
    StateMachine();
}

#endif
#endif //CY_SENSE_CapSense_H