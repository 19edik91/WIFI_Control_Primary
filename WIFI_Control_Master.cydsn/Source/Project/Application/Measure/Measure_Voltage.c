/* ========================================
 *
 * Copyright Eduard Kraemer, 2019
 *
 * ========================================
*/

#include "Measure_Voltage.h"
#include "HAL_Config.h"
#include "Aom.h"

/****************************************** Defines ******************************************************/
#define MULTI_FOR_EASIER_CALC    100
#define VOLTAGE_RANGE            (VOLTAGE_DEFAULT_UPPER_LIMIT - VOLTAGE_DEFAULT_LOWER_LIMIT)/MULTI_FOR_EASIER_CALC
#define CALC_VOLTAGE_FROM_PERCENT(x) (VOLTAGE_DEFAULT_LOWER_LIMIT + ( VOLTAGE_RANGE * (x))) //result -> 2400 (= 24.00V)
#define RESISTOR_1               102000       //102kOhm
#define RESISTOR_2               5360         //5.36kOhm
#define ADC_REF_MILLIVOLT        ADC_INPUT_DEFAULT_VREF_MV_VALUE
#define BITSHIFT_10              1024

/********* Voltage calc *********/

/* The equation for the "constant" part in the voltage divider. The multiplied 10240 is used to get the value in
   1/mV without the floatin point. Also instead of dividing the result by 1000 a barrel-shift can be used! */
#define ADC2VOLT_DIVIDER     (((RESISTOR_1 + RESISTOR_2) * BITSHIFT_10)/(RESISTOR_2))
#define ADC2VOLT_ADC_STEP    ((ADC_REF_MILLIVOLT * BITSHIFT_10)/(ADC_MAX_VAL))

#define VOLT2ADC_DIVIDER    ((RESISTOR_2 * BITSHIFT_10)/(RESISTOR_1 + RESISTOR_2))
#define VOLT2ADC_ADC_STEP   ((ADC_MAX_VAL * BITSHIFT_10)/(ADC_REF_MILLIVOLT))

/* Requested ADC value: AdcVal = Ureq * (R2*AdcMaxVal)/((R1+R2)*AdcRefMilliVolt) 
    -> Umeas = AdcVal * DividerConst */
#define ADC_CONVERT_TO_MILLI_VOLT_S1(x) (((x) * ADC2VOLT_DIVIDER ) >> 10)
#define ADC_CONVERT_TO_MILLI_VOLT_S2(x) (((x) * ADC2VOLT_ADC_STEP) >> 10)

#define MILLI_VOLT_CONVERT_TO_ADC_S1(x) (((x) * VOLT2ADC_DIVIDER ) >> 10)
#define MILLI_VOLT_CONVERT_TO_ADC_S2(x) (((x) * VOLT2ADC_ADC_STEP) >> 10)


/****************************************** Variables ****************************************************/
static u32 ulVoltageLowerLimit[DRIVE_OUTPUTS] = {VOLTAGE_DEFAULT_LOWER_LIMIT};
static u32 ulVoltageUpperLimit[DRIVE_OUTPUTS] = {VOLTAGE_DEFAULT_UPPER_LIMIT};
static u32 ulVoltageRange[DRIVE_OUTPUTS] = {VOLTAGE_RANGE};
//static u32 ulVoltageLowerLimit[DRIVE_OUTPUTS] = {VOLTAGE_DEFAULT_LOWER_LIMIT, VOLTAGE_DEFAULT_LOWER_LIMIT, VOLTAGE_DEFAULT_LOWER_LIMIT};
//static u32 ulVoltageUpperLimit[DRIVE_OUTPUTS] = {VOLTAGE_DEFAULT_UPPER_LIMIT, VOLTAGE_DEFAULT_UPPER_LIMIT, VOLTAGE_DEFAULT_UPPER_LIMIT};
//static u32 ulVoltageRange[DRIVE_OUTPUTS] = {VOLTAGE_RANGE, VOLTAGE_RANGE, VOLTAGE_RANGE};
static u32 ulMeasure_SystemVoltage = 0;
static u32 ulMeasure_SystemVoltageRange = 0;

/****************************************** Function prototypes ******************************************/
/****************************************** loacl functiones *********************************************/
/****************************************** External visible functiones **********************************/

//********************************************************************************
/*!
\author     Kraemer E.
\date       10.11.2019
\brief      Calculate voltage from the percentage
\return     ulVoltageValue - Returns a voltage value in mV
\param      ucPercentValue - Value in percent
\param      ucOutputIdx - Index of the output which shall be calculated
\param      bUseDefault - True to use the default voltage limits
***********************************************************************************/
u32 Measure_Voltage_CalculateVoltageFromPercent(u8 ucPercentValue, u8 ucOutputIdx, bool bUseDefault)
{
    u32 ulVoltageValue = 0;
    
    /* Check for valid output index */
    if(ucOutputIdx < DRIVE_OUTPUTS)
    {
        if(bUseDefault)
        {
            /* When no system voltage was calculated. Use the pre-defined values */
            if(ulMeasure_SystemVoltage == 0)
            {
                ulVoltageValue = CALC_VOLTAGE_FROM_PERCENT(ucPercentValue);
            }
            /* System voltage value was already calculated. Use the new limitations */
            else
            {
                ulVoltageValue = VOLTAGE_DEFAULT_LOWER_LIMIT + (ulMeasure_SystemVoltageRange * ucPercentValue);
            }
        }
        /* User wants to use his own "software" set limits */
        else
        {
            ulVoltageValue = ulVoltageLowerLimit[ucOutputIdx] + (ulVoltageRange[ucOutputIdx] * ucPercentValue);
        }
    }
    
    return ulVoltageValue;
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Calculate ADC value from the voltage value
\return     uiAdcValue - Returns the calculated ADC value.
\param      uiVoltage - Voltage value in millivolt
***********************************************************************************/
u16 Measure_Voltage_CalculateAdcValue(u32 uiVoltage)
{
    u16 uiAdcValue = 0;
    u32 ulTempVal = 0;
    
    /* Prevent an overflow in calculation */
    if(uiVoltage > VOLTAGE_DEFAULT_UPPER_LIMIT)
    {
        uiVoltage = VOLTAGE_DEFAULT_UPPER_LIMIT;
    }
    
    /* Calculate ADC value from voltage value */
    ulTempVal = MILLI_VOLT_CONVERT_TO_ADC_S1(uiVoltage);
    ulTempVal = MILLI_VOLT_CONVERT_TO_ADC_S2(ulTempVal);
    
    /* ADC value can't be greater than ADC max */
    if(ulTempVal > ADC_MAX_VAL)
    {
        ulTempVal = ADC_MAX_VAL;
    }
    
    /* Inverse ADC measurement */
//    uiAdcValue = ADC_MAX_VAL - ulTempVal;
    uiAdcValue = (u16)ulTempVal;
    return uiAdcValue;
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Calculate voltage on the measure input
\return     ulVoltageValue - Voltage value over the load in millivolt
\param      uiAdcValue - Returns the calculated ADC value.
***********************************************************************************/
u32 Measure_Voltage_CalculateVoltageValue(u16 uiAdcValue)
{
    u32 ulVoltageValue = 0;
    
    /* ADC value can't be greater than ADC max */
    if(uiAdcValue > ADC_MAX_VAL)
    {
        uiAdcValue = ADC_MAX_VAL;
    }
    
    /* Calculate voltage value */
    ulVoltageValue = ADC_CONVERT_TO_MILLI_VOLT_S1(uiAdcValue);
    ulVoltageValue = ADC_CONVERT_TO_MILLI_VOLT_S2(ulVoltageValue);
    
    return ulVoltageValue;
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       10.11.2019
\brief      Sets new voltage limits for calculating the voltage in dependency of
            the requested brightness
\return     none
\param      uiMinLimit - Sets the min limit
\param      uiMaxLimit - Sets the max limit
\param      ucOutputIdx - The output which shall get new limits
***********************************************************************************/
void Measure_Voltage_SetNewLimits(u32 ulMinLimit, u32 ulMaxLimit, u8 ucOutputIdx)
{
    if(ucOutputIdx < DRIVE_OUTPUTS)
    {
        ulVoltageLowerLimit[ucOutputIdx] = ulMinLimit;
        ulVoltageUpperLimit[ucOutputIdx] = ulMaxLimit;
        ulVoltageRange[ucOutputIdx] = (ulMaxLimit - ulMinLimit)/MULTI_FOR_EASIER_CALC;    
    }
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       20.12.2020
\brief      Returns the system voltage
\return     ulSystemVoltage - Returns the calculated system voltage in millivolt
\param      none
***********************************************************************************/
u32 Measure_Voltage_GetSystemVoltage(void)
{
    return ulMeasure_SystemVoltage;
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       20.12.2020
\brief      Calculates the voltage range and sets new software limits when
            they weren't changed yet.
\return     none
\param      ulSystemVoltage - The calculated system voltage
***********************************************************************************/
void Measure_Voltage_SetSystemVoltage(u32 ulSystemVoltage)
{    
    /* Calculate new voltage range */
    ulMeasure_SystemVoltageRange = (ulSystemVoltage - VOLTAGE_DEFAULT_LOWER_LIMIT)/MULTI_FOR_EASIER_CALC;    
    ulMeasure_SystemVoltage = ulSystemVoltage;
     
    /* Check if limits were already changed */
    u8 ucOutputIdx;
    for(ucOutputIdx = 0; ucOutputIdx < DRIVE_OUTPUTS; ucOutputIdx++)
    {
        if(ulVoltageUpperLimit[ucOutputIdx] == VOLTAGE_DEFAULT_UPPER_LIMIT)
        {
            //Limits weren't changed. Therefore overwrite the "software" limits with the new hardware limit
            Measure_Voltage_SetNewLimits(ulVoltageLowerLimit[ucOutputIdx], ulSystemVoltage, ucOutputIdx);
        }
    }
}