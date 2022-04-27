/* ========================================
 *
 * Copyright Eduard Kraemer, 2019
 *
 * ========================================
*/



#include "OS_EventManager.h"
#include "OS_ErrorHandler.h"

#include "DR_Regulation.h"
#include "DR_ErrorDetection.h"
#include "DR_Measure.h"

#include "Aom_Regulation.h"
#include "AutomaticMode.h"

/****************************************** Defines ******************************************************/
/****************************************** Variables ****************************************************/

/****************************************** Function prototypes ******************************************/
static bool ValidatePercentValue(u8* pucValue);
static void SetCustomValue(u8 ucBrightnessValue, bool bLedStatus, bool bInitMenuActive, u8 ucOutputIdx);

/****************************************** loacl functiones *********************************************/
#if (WITHOUT_REGULATION == false)
//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\fn         ValidatePercentValue()
\brief      Checks given value for a valid range (1..100%).
\return     bValid - true when valid value was set.
\param      pucValue - Pointer to the value which should be checked.
***********************************************************************************/
static bool ValidatePercentValue(u8* pucValue)
{
    bool bValid = false;    
    if(pucValue)
    {
        /* Check if value is smaller or equal zero */
        if(*pucValue < PERCENT_LOW)
        {
            *pucValue = PERCENT_LOW;
        }
        
        /* Check if request is greater than 100% */
        if(*pucValue > PERCENT_HIGH)
        {
            *pucValue = PERCENT_HIGH;
        }
        
        bValid = true;
    }    
    return bValid;
}
#endif

//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Writes new requested values from customer into AOM.
\return     none
\param      ucBrightnessValue - Value with the saved brightness percentage
***********************************************************************************/
static void SetCustomValue(u8 ucBrightnessValue, bool bLedStatus, bool bInitMenuActive, u8 ucOutputIdx)
{   
    /* Check first if output index is valid */
    if(ucOutputIdx < DRIVE_OUTPUTS)
    {        
        tLedValue* psLedVal = Aom_GetOutputsSettingsEntry(ucOutputIdx);
        
        /* Save LED status */
        psLedVal->bStatus = bLedStatus;
        
        //Get the parameter in dependency of the led status
        teEventParam eParam = (bLedStatus == OFF) ? eEvtParam_RegulationStop : eEvtParam_RegulationStart;

        /* Request a regulation state-change */
        OS_EVT_PostEvent(eEvtNewRegulationValue, eParam, ucOutputIdx);
        
        
        /* Check first if values are new values */
        if(ucBrightnessValue != psLedVal->ucPercentValue)
        {
            if(ValidatePercentValue(&ucBrightnessValue))
            {
                /* Set customised percent value */
                psLedVal->ucPercentValue = ucBrightnessValue;
                        
                /* Calculate requested voltage value */
                u16 uiReqVoltage = DR_Measure_CalculateVoltageFromPercent(ucBrightnessValue, bInitMenuActive, ucOutputIdx);
                
                /* Calculate requested ADC value */
                psLedVal->uiReqVoltageAdc = DR_Measure_CalculateAdcValue(uiReqVoltage,0);
                            
                /* Start with event */
                OS_EVT_PostEvent(eEvtNewRegulationValue, eEvtParam_RegulationValueStartTimer, ucOutputIdx);
            }
        }
    }
}

/****************************************** External visible functiones **********************************/


//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Compares received values with already saved values
\return     none
\param      ucBrightnessValue - Value with the saved brightness percentage
***********************************************************************************/
bool Aom_Regulation_CompareCustomValue(u8 ucBrightnessValue, bool bLedStatus, u8 ucOutputIdx)
{   
    bool bDifferentValue = false;
    
    /* Check first if output index is valid */
    if(ucOutputIdx < DRIVE_OUTPUTS)
    {        
        const tLedValue* psLedVal = Aom_GetOutputsSettingsEntry(ucOutputIdx);
        
        /* Compare values between heart beat and saved values */
        if(ucBrightnessValue != psLedVal->ucPercentValue
            || bLedStatus != psLedVal->bStatus)
        {
            bDifferentValue = true;
        }
    }
    
    return bDifferentValue;
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       19.04.2022
\brief      Change the current value of the output index by the requested change value.
\return     none
\param      scChangeValue - Signed value to change the current value in percent,
            relative to the current value
\param      ucOutputIdx - The output which shall be changed
***********************************************************************************/
void Aom_Regulation_ChangeValueRelative(s8 scChangeValue, u8 ucOutputIdx)
{
    tRegulationValues* psRegVal = Aom_GetRegulationSettings();
    
    if(ucOutputIdx < DRIVE_OUTPUTS)
    {
        //Allow changes only for active outputs
        if(psRegVal->sLedValue[ucOutputIdx].bStatus == true)
        {        
            //Add the signed change value to the current value.
            s8 scValue = psRegVal->sLedValue[ucOutputIdx].ucPercentValue + scChangeValue;
            
            //Normalize to percent 
            if(scValue < PERCENT_LOW)
            {
                scValue = PERCENT_LOW;
            }
            else if(scValue > PERCENT_HIGH)
            {
                scValue = PERCENT_HIGH;
            }
            
            //Save new values
            SetCustomValue(scValue, true, false, ucOutputIdx);
        }
    }
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       19.04.2022
\brief      Change the current value of the output index by the requested change value.
\return     none
\param      ucNewValue - Absolute value which is set directly
\param      ucOutputIdx - The output which shall be changed
***********************************************************************************/
void Aom_Regulation_ChangeValueAbsolute(u8 ucNewValue, u8 ucOutputIdx)
{
    tRegulationValues* psRegVal = Aom_GetRegulationSettings();
    
    if(ucOutputIdx < DRIVE_OUTPUTS)
    {
        //Allow changes only for active outputs
        if(psRegVal->sLedValue[ucOutputIdx].bStatus == true)
        {        
            ValidatePercentValue(&ucNewValue);
            
            //Save new values
            SetCustomValue(ucNewValue, true, false, ucOutputIdx);
        }
    }
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Writes new requested values from customer into AOM.
\return     none
\param      ucBrightnessValue - Value with the saved brightness percentage
***********************************************************************************/
void Aom_Regulation_CheckRequestValues(u8 ucBrightnessValue, bool bLedStatus, bool bInitMenuActive, u8 ucOutputIdx)
{   
    tsAutomaticModeValues* psAutomaticModeVal = Aom_GetAutomaticModeSettingsPointer();
    tRegulationValues* psRegVal = Aom_GetRegulationSettings();
    
    /* Check for over-current */
    //if(OS_ErrorHandler_GetErrorTimeout())
    //{
    //    /* Overcurrent was detected. Reduce requested brighntess value by half */
    //    ucBrightnessValue /= 2;
    //}
    
    /* Get new automatic mode state according to the set modes */
    teAutomaticState eAutoState = eStateDisabled;    
    if(psRegVal->sUserTimerSettings.bMotionDetectOnOff == ON && psRegVal->sUserTimerSettings.bAutomaticModeActive == ON)
    {
        eAutoState = eStateAutomaticMode_2;
    }
    else if(psRegVal->sUserTimerSettings.bMotionDetectOnOff == ON && psRegVal->sUserTimerSettings.bAutomaticModeActive == OFF)
    {
        eAutoState = eStateAutomaticMode_3;
    }
    else if(psRegVal->sUserTimerSettings.bMotionDetectOnOff == OFF && psRegVal->sUserTimerSettings.bAutomaticModeActive == ON)
    {
        eAutoState = eStateAutomaticMode_1;
    }
    //Update state machine of the automatic state
    AutomaticMode_ChangeState(eAutoState);
    
    /* Get automatic mode LED status dependent of the automatic mode */
    bool bAutomaticLedState = AutomaticMode_Handler();
    
    /* Overwrite user settings only when automatic mode is active */
    if(eAutoState != eStateDisabled)
    {
        bLedStatus = bAutomaticLedState;
    }
    
    /* Reduce brightness when night mode time slot is active */
    ucBrightnessValue = (psAutomaticModeVal->bInNightModeTimeSlot == true) ? PERCENT_LOW : ucBrightnessValue;   
    
    /* Set changed customer values */
    SetCustomValue(ucBrightnessValue, bLedStatus, bInitMenuActive, ucOutputIdx);
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       14.04.2022
\brief      Set automatic mode for regulation
\return     none
\param      bAutomaticModeStatus - True when automatic mode shall be enabled
***********************************************************************************/
void Aom_Regulation_SetAutomaticModeStatus(bool bAutomaticModeStatus)
{   
    tRegulationValues* psRegVal = Aom_GetRegulationSettings();
    
    if(bAutomaticModeStatus != psRegVal->sUserTimerSettings.bAutomaticModeActive)
    {
        psRegVal->sUserTimerSettings.bAutomaticModeActive = bAutomaticModeStatus;
        
        /* Post event to start the timer for saving the new regulation value into the flash */
        OS_EVT_PostEvent(eEvtNewRegulationValue, eEvtParam_RegulationValueStartTimer, eEvtParam_None);
    }
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       14.04.2022
\brief      Set night mode for regulation
\return     none
\param      bNightModeOnOff - True when night mode shall be enabled
***********************************************************************************/
void Aom_Regulation_SetNightModeStatus(bool bNightModeOnOff)
{   
    tRegulationValues* psRegVal = Aom_GetRegulationSettings();
    
    if(psRegVal->bNightModeOnOff != bNightModeOnOff)
    {
        psRegVal->bNightModeOnOff = bNightModeOnOff;
    
        /* Post event to start the timer for saving the new regulation value into the flash */
        OS_EVT_PostEvent(eEvtNewRegulationValue, eEvtParam_RegulationValueStartTimer, eEvtParam_None);
    }
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       14.04.2022
\brief      Set motion detection status for regulation
\return     none
\param      bMotionDetectionOnOff - True when motion detection shall be enabled.
***********************************************************************************/
void Aom_Regulation_SetMotionDectionStatus(bool bMotionDetectionOnOff, u8 ucBurnTime)
{
    tRegulationValues* psRegVal = Aom_GetRegulationSettings();
    
    if( psRegVal->sUserTimerSettings.bMotionDetectOnOff != bMotionDetectionOnOff
     || psRegVal->sUserTimerSettings.ucBurningTime != ucBurnTime)
    {
        psRegVal->sUserTimerSettings.bMotionDetectOnOff = bMotionDetectionOnOff;
        psRegVal->sUserTimerSettings.ucBurningTime = ucBurnTime;
    
        /* Post event to start the timer for saving the new regulation value into the flash */
        OS_EVT_PostEvent(eEvtNewRegulationValue, eEvtParam_RegulationValueStartTimer, eEvtParam_None);
    }
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Writes requested values from AOM into pointed structure.
\return     none
\param      psRegulationValues - Pointer to the structure.
***********************************************************************************/
void Aom_Regulation_GetRegulationValues(tRegulationValues* psRegulationValues)
{
    if(psRegulationValues)
    {
        memcpy(psRegulationValues, Aom_GetRegulationSettings(), sizeof(tRegulationValues));
    }
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       27.05.2020
\brief      Returns a pointer to the regualtion values.
\return     const tRegulationValues pointer
***********************************************************************************/
const tRegulationValues* Aom_Regulation_GetRegulationValuesPointer(void)
{
    return Aom_GetRegulationSettings();
}

#if (WITHOUT_REGULATION == false)
//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\fn         Aom_SetCalculatedVoltageValue()
\brief      Writes new requested values from customer into AOM.
\return     none
\param      psRegulationValues - Pointer with the calculated values
***********************************************************************************/
//void Aom_Regulation_SetCalculatedVoltageValue(tRegulationValues* psRegulationValues)
//{
//    if(psRegulationValues)
//    {
//        tRegulationValues* psRegVal = Aom_GetRegulationSettings();
//        psRegVal->sLedValue.uiIsVoltage = psRegulationValues->sLedValue.uiIsVoltage;        
//    }
//}


//********************************************************************************
/*!
\author     Kraemer E.
\date       22.10.2019
\brief      Sets the minimum values of the system
\return     none
***********************************************************************************/
void Aom_Regulation_SetMinSystemSettings(u16 uiCurrentAdc, u16 uiVoltageAdc, u8 ucOutputIdx)
{
    tsSystemSettings* psSystemSettings = Aom_GetSystemSettingsEntry(ucOutputIdx);    
    psSystemSettings->uiMinAdcCurrent = uiCurrentAdc;
    psSystemSettings->uiMinAdcVoltage = uiVoltageAdc;
    
    tsPwmData sPwmData; 
    DR_Regulation_GetPWMData(ucOutputIdx, &sPwmData);
    psSystemSettings->uiMinCompVal = sPwmData.uiCompareValue;
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       22.10.2019
\brief      Sets the maximum values of the system
\return     none
***********************************************************************************/
void Aom_Regulation_SetMaxSystemSettings(u16 uiCurrentAdc, u16 uiVoltageAdc, u8 ucOutputIdx)
{
    tsSystemSettings* psSystemSettings = Aom_GetSystemSettingsEntry(ucOutputIdx);
    psSystemSettings->uiMaxAdcCurrent = uiCurrentAdc;
    psSystemSettings->uiMaxAdcVoltage = uiVoltageAdc;
    
    tsPwmData sPwmData; 
    DR_Regulation_GetPWMData(ucOutputIdx, &sPwmData);
    psSystemSettings->uiMaxCompVal = sPwmData.uiCompareValue;
}

#endif

