/* ========================================
 *
 * Copyright Eduard Kraemer, 2019
 *
 * ========================================
*/

#include "Aom.h"
#include "Actors.h"
#include "Measure.h"
#include "Flash.h"
#include "EventManager.h"
#include "Regulation.h"
#include "Standby.h"
#include "ErrorDetection.h"
#include "ErrorHandler.h"
#include "AutomaticMode.h"

/****************************************** Defines ******************************************************/
/****************************************** Variables ****************************************************/
static tRegulationValues sRegulationValues;

static tsSystemSettings sSystemSettings[DRIVE_OUTPUTS];

static tsConvertedMeasurement sConvertedMeasurement;
    
/* Variables to hold the received time from the ESP */
static tsCurrentTime sCurrentTime;
static tsAutomaticModeValues sAutomaticModeValues;

/****************************************** Function prototypes ******************************************/
/****************************************** loacl functiones *********************************************/
/****************************************** External visible functiones **********************************/

//********************************************************************************
/*!
\author     Kraemer E.
\date       28.10.2020
\brief      Get a new pointer to the system settings entry
\return     Address to the desired system settings entry
\param      ucDriveOutputIdx - Index of the desired output
***********************************************************************************/
tsSystemSettings* Aom_GetSystemSettingsEntry(u8 ucDriveOutputIdx)
{   
    if(ucDriveOutputIdx < DRIVE_OUTPUTS)
    {
        return &sSystemSettings[ucDriveOutputIdx];
    }
    else
    {
        return NULL;
    }    
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       28.10.2020
\brief      Get a new pointer to the regulation settings structure
\return     Address to the regulation settings structure
***********************************************************************************/
tRegulationValues* Aom_GetRegulationSettings(void)
{
    return &sRegulationValues;
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       28.10.2020
\brief      Get a new pointer to the output settings entry
\return     Address to the desired output settings entry
\param      ucDriveOutputIdx - Index of the desired output
***********************************************************************************/
tLedValue* Aom_GetOutputsSettingsEntry(u8 ucDriveOutputIdx)
{
    if(ucDriveOutputIdx < DRIVE_OUTPUTS)
    {
        return &sRegulationValues.sLedValue[ucDriveOutputIdx];
    }
    else
    {
        return NULL;
    }    
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       28.10.2020
\brief      Get a new pointer to the current time structure
\return     Address to the current structure
***********************************************************************************/
tsCurrentTime* Aom_GetCurrentTimePointer(void)
{
    return &sCurrentTime;
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       28.10.2020
\brief      Get a new pointer to the automatic mode structure
\return     Address to the automatic mode structure
***********************************************************************************/
tsAutomaticModeValues* Aom_GetAutomaticModeSettingsPointer(void)
{
    return &sAutomaticModeValues;
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       28.10.2020
\brief      Get a new pointer to the converted measurement structure
\return     Address to the automatic mode structure
***********************************************************************************/
tsConvertedMeasurement* Aom_GetConvertedMeasurementPointer(void)
{
    return &sConvertedMeasurement;
}
