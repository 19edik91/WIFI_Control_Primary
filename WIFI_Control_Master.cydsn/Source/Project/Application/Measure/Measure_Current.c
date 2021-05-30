/* ========================================
 *
 * Copyright Eduard Kraemer, 2019
 *
 * ========================================
*/

#include "Measure_Current.h"
#include "HAL_IO_Config.h"
#include "Aom.h"

/****************************************** Defines ******************************************************/
#define SHUNT_RESISTOR_DIVISION   10        // 1/(0,1Ohm)
#define ADC_REF_MILLIVOLT        ADC_INPUT_DEFAULT_VREF_MV_VALUE
#define BITSHIFT_10              1024       //Bitshift in value
#define OP_AMP_GAIN                31       //Gain of the Op-AMP

/********* Current calc *********/
#define ADC2AMP_ADC_STEP    ((ADC_REF_MILLIVOLT * BITSHIFT_10)/(ADC_MAX_VAL))
#define AMP2ADC_ADC_STEP    ((ADC_MAX_VAL * BITSHIFT_10)/(ADC_REF_MILLIVOLT))

/* Requested ADC value: AdcVal = Ureq * (R2*AdcMaxVal)/((R1+R2)*AdcRefMilliVolt) 
    -> Umeas = AdcVal * DividerConst */
#define ADC_CONVERT_TO_MILLI_AMP_S1(x) (((x) * ADC2AMP_ADC_STEP ) >> 10)
#define ADC_CONVERT_TO_MILLI_AMP_S2(x) ((x) * SHUNT_RESISTOR_DIVISION)
#define ADC_CONVERT_TO_MILLI_AMP_S3(x) ((x) / OP_AMP_GAIN)

#define MILLI_AMP_CONVERT_TO_ADC_S1(x) ((x) * AMP2ADC_ADC_STEP )
#define MILLI_AMP_CONVERT_TO_ADC_S2(x) ((x) * OP_AMP_GAIN)
#define MILLI_AMP_CONVERT_TO_ADC_S3(x) (((x) / SHUNT_RESISTOR_DIVISION) >> 10)


/****************************************** Variables ****************************************************/
/****************************************** Function prototypes ******************************************/
/****************************************** loacl functiones *********************************************/

/****************************************** External visible functiones **********************************/

//********************************************************************************
/*!
\author     Kraemer E.
\date       15.10.2019
\brief      Calculate current from the ADC value
\return     uiCurrentValue - Current value in mA
\param      uiAdcValue - Returns the calculated ADC value.
***********************************************************************************/
u16 Measure_Current_CalculateCurrentValue(u16 uiAdcValue)
{
    u32 ulCurrentValue = 0;
    
    /* ADC value can't be greater than ADC max */
    if(uiAdcValue > ADC_MAX_VAL)
    {
        uiAdcValue = ADC_MAX_VAL;
    }
    
    /* Calculate current value */
    ulCurrentValue = ADC_CONVERT_TO_MILLI_AMP_S1(uiAdcValue);
    ulCurrentValue = ADC_CONVERT_TO_MILLI_AMP_S2(ulCurrentValue);
    ulCurrentValue = ADC_CONVERT_TO_MILLI_AMP_S3(ulCurrentValue);
    
    return (u16)ulCurrentValue;
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       20.10.2019
\brief      Calculate ADC value from the milli ampere value
\return     uiAdcValue - Requested current value in ADC digits
\param      uiMilliCurrent - Current in mA
***********************************************************************************/
u16 Measure_Current_CalculateAdcValue(u16 uiMilliCurrent)
{
    u32 ulAdcValue = 0;
        
    /* Calculate ADC value */
    ulAdcValue = MILLI_AMP_CONVERT_TO_ADC_S1(uiMilliCurrent);
    ulAdcValue = MILLI_AMP_CONVERT_TO_ADC_S2(ulAdcValue);
    ulAdcValue = MILLI_AMP_CONVERT_TO_ADC_S3(ulAdcValue);
    return (u16)ulAdcValue;
}