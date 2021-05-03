/* ========================================
 *
 * Copyright Eduard Kraemer, 2019
 *
 * ========================================
*/

#include "Measure_Voltage.h"
#include "Measure_Temperature.h"
#include "Measure_Current.h"
#include "Measure.h"
#include "Aom_Measure.h"

#if (WITHOUT_REGULATION == false)
/****************************************** Defines ******************************************************/

#define ADC_REF_MILLIVOLT        ADC_INPUT_DEFAULT_VREF_MV_VALUE
#define ADC_INPUT_CHANNEL0       0

#if (AMuxSeq_CHANNELS >= ADC_INPUT_SEQUENCED_CHANNELS_NUM)
    #define ADC_CHANNELS             AMuxSeq_CHANNELS
#else
    #define ADC_CHANNELS             ADC_INPUT_SEQUENCED_CHANNELS_NUM
#endif


/****************************************** Variables ****************************************************/
/* Fill MUX list with defined outputs correlations */
static tsAdMuxList sAdMuxList[] = 
{
    #define A_CH(ChannelName, sMeasureValue, MeasureType, OutputIndex) {{sMeasureValue}, MeasureType, OutputIndex},
        AD_MUX_LIST
    #undef A_CH
};

static u32 ulSystemVoltageOld;
static u32 ulSystemVoltageNew;
static u16 uiSystemVoltageAdc;
/****************************************** Function prototypes ******************************************/
static void MeasureInterrupt(void);
static void PutInMovingAverage(s16 siAdcValue, teAdMuxList eAMuxChannel);


/****************************************** loacl functiones *********************************************/
//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Interrupt function which should be called, whenever a ISR occurs.
\return     none
\param      none
***********************************************************************************/
static void MeasureInterrupt(void)
{    
    /* Clear isr flag */
    #if Measure_ISR    
        Measure_ISR_ClearPending();
    #else
        ADC_INPUT_IRQ_ClearPending();
        
        /* Read interrupt status register */
        u32 intr_status = ADC_INPUT_SAR_INTR_REG;
        
        /* Clear handled interrupt */
        ADC_INPUT_SAR_INTR_REG = intr_status;
    #endif
    
    /* Get the ADC value and put it in the average buffer */
    PutInMovingAverage(ADC_INPUT_GetResult16(ADC_INPUT_CHANNEL0), AMuxSeq_GetChannel());
    
    /* Switch multiplexer to the next channel */
    AMuxSeq_Next();
    
    /* Trigger the conversion */
    ADC_INPUT_StartConvert();
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Fills a moving average filter with values.
\return     none
\param      siAdcValue - New ADC value which should be saved.
***********************************************************************************/
static void PutInMovingAverage(s16 siAdcValue, teAdMuxList eAMuxChannel)
{
    tsMeasureValues* psMeasureVal = &sAdMuxList[eAMuxChannel].sMeasureValue;
    
    /* Subtract the oldest entry from the sum */   
    psMeasureVal->siAdcSum  -= psMeasureVal->siAdcBuffer[psMeasureVal->ucBufferIndex];
      
    /* Save new value in buffer */
    psMeasureVal->siAdcBuffer[psMeasureVal->ucBufferIndex] = siAdcValue;
    
    /* Add new value to summation */
    psMeasureVal->siAdcSum += siAdcValue;
    
    /* Increment buffer index and set back to zero, when limit is reached */
    if(++psMeasureVal->ucBufferIndex == BUFFER_LENGTH)
    {
        psMeasureVal->ucBufferIndex = 0;
    }
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Calculate average value from the ADC summation value
\return     uiAdcValue - Returns the calculated ADC value.
\param      psMeasureValue - Pointer to the measurement structure
***********************************************************************************/
static u16 CalculateAveragedAdcValue(const tsMeasureValues* psMeasureValue)
{
    u16 uiAdcValue = 0;

    /* Calculate temporary value first */
    s16 siTempVal = psMeasureValue->siAdcSum / BUFFER_LENGTH;
    
    /* Check if temp.val is negative and set to zero if it is */
    uiAdcValue = (siTempVal < 0) ? 0 : siTempVal; 
    
    return uiAdcValue;
}



/****************************************** External visible functiones **********************************/

//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Initialize the measurement module.
\return     none
\param      none
***********************************************************************************/
void Measure_Init(void)
{
    /* Clear measure values */   
    u8 ucMeasureValIdx = 0;
    for(ucMeasureValIdx = _countof(sAdMuxList); ucMeasureValIdx--;)
    {
        if((teAdMuxList)ucMeasureValIdx < eA_CH_INV)
        {
            memset(&sAdMuxList[ucMeasureValIdx].sMeasureValue, 0, sizeof(tsMeasureValues));
        }
    }

    #if Measure_ISR
        /* Set ISR address */
        Measure_ISR_StartEx(MeasureInterrupt);
        Measure_ISR_Enable();
    #else        
        AMuxSeq_Start();
        while(AMuxSeq_GetChannel < 0)
        {
            AMuxSeq_Next();
        }
        
        ADC_INPUT_Start();
        ADC_INPUT_StartConvert();        
        ADC_INPUT_IRQ_StartEx(MeasureInterrupt);
        //ADC_INPUT_SAR_INTR_MASK_REG |= ADC_INPUT_EOS_MASK;
    #endif
    
    /* Activate ADC module */   
    Measure_Start();
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Calculate voltage from the percentage
\return     uiVoltageValue - Returns a voltage value in double accuracy (= 1500 => 15.00V)
\param      ucPercentValue - Value in percent
***********************************************************************************/
u32 Measure_CalculateVoltageFromPercent(u8 ucPercentValue, bool bUseDefaultLimit, u8 ucOutputIdx)
{
    return Measure_Voltage_CalculateVoltageFromPercent(ucPercentValue, ucOutputIdx, bUseDefaultLimit);
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Calculate ADC value from the voltage value
\return     uiAdcValue - Returns the calculated ADC value.
\param      uiVoltage - Voltage value in double accuracy (2400)
***********************************************************************************/
u16 Measure_CalculateAdcValue(u32 ulVoltage, u16 uiCurrent)
{
    if(ulVoltage)
    {
        return Measure_Voltage_CalculateAdcValue(ulVoltage);
    }    
    else if(uiCurrent)
    {
        return Measure_Current_CalculateAdcValue(uiCurrent);
    }
    else
    {
        return 0xFFFF;
    }
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      Calculate voltage from the ADC value
\return     uiVoltage - Voltage value in double accuracy (2400)
\param      uiAdcValue - Returns the calculated ADC value.
***********************************************************************************/
u32 Measure_CalculateVoltageValue(u16 uiAdcValue)
{   
    return Measure_Voltage_CalculateVoltageValue(uiAdcValue);
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       08.11.2019
\brief      Calculate current from the ADC value
\return     uiCurrent - Current value in milli ampere
\param      uiAdcValue - Returns the calculated ADC value.
***********************************************************************************/
u16 Measure_CalculateCurrentValue(u16 uiAdcValue)
{   
    return Measure_Current_CalculateCurrentValue(uiAdcValue);
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       08.11.2019
\brief      Calculate temperature from the ADC value
\return     uiTemperature - Temp. value in double accuracy (2400)
\param      uiAdcValue - Returns the calculated ADC value.
***********************************************************************************/
u16 Measure_CalculateTemperatureValue(u16 uiAdcValue)
{   
    return Measure_Temperature_CalculateTemperature(uiAdcValue);
}
 

//********************************************************************************
/*!
\author     Kraemer E.
\date       20.01.2019
\brief      This module is called every millisecond.
\return     none
\param      none
***********************************************************************************/
void Measure_Tick(void)
{
    /* Save actual ADC values in AOM */
    u8 ucAdcChannelIdx;
    for(ucAdcChannelIdx = _countof(sAdMuxList); ucAdcChannelIdx--;)
    {    
        s16 siAvgValue = CalculateAveragedAdcValue(&sAdMuxList[ucAdcChannelIdx].sMeasureValue);
        
        /* Voltage ADC is calculated indirectly. The measured voltage is the voltage dissipation
           over the coil. Therefore the correct ADC value is "MaxAdcVal - ShuntAdcVal - CoilAdcVal = LedAdcVal.
           For easier calculation the ShuntAdcValue is ignored */
        if(sAdMuxList[ucAdcChannelIdx].eMeasureType == eMeasureChVoltage)
        {
            //Get system voltage
            ulSystemVoltageNew = Measure_Voltage_GetSystemVoltage();
            
            //Check if system voltage changed
            if(ulSystemVoltageNew != ulSystemVoltageOld)
            {
                //WHen changed, calculate new ADC value for system voltage
                uiSystemVoltageAdc = Measure_Voltage_CalculateAdcValue(ulSystemVoltageNew);
                ulSystemVoltageOld = ulSystemVoltageNew;
            }            
            
            siAvgValue = uiSystemVoltageAdc - siAvgValue;

            if(siAvgValue < 0)
                siAvgValue = 0;
        }
        
        /* Calculate average ADC value and save it into AOM */
        Aom_Measure_SetActualAdcValues(siAvgValue, sAdMuxList[ucAdcChannelIdx].eMeasureType, sAdMuxList[ucAdcChannelIdx].ucOutputIndex);
    }    
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       10.02.2019
\brief      Starts the ADC module.
\return     none
\param      none
***********************************************************************************/
void Measure_Start(void)
{
    // Start analog multiplexer. Starts with -1
    AMuxSeq_Start();
    while(AMuxSeq_GetChannel() < 0)
    {
        AMuxSeq_Next();
    }    
    
    // Set channel mask for no inputs
    ADC_INPUT_SetChanMask(0xFF);
    
    /* Enable ADC-Module */
    ADC_INPUT_Start();
    
    /* Enable the interrupt */
    //ADC_INPUT_IRQ_Enable();
    
    /* Start the first conversion. Every other conversion is
       started in the interrupt */
    ADC_INPUT_StartConvert();
}

//********************************************************************************
/*!
\author     Kraemer E.
\date       10.02.2019
\brief      Stops the ADC module.
\return     none
\param      none
***********************************************************************************/
void Measure_Stop(void)
{
    // Set channel mask for no inputs
    ADC_INPUT_SetChanMask(0x00);
    
    /* Stops the ADC module */
    ADC_INPUT_Stop();
    //ADC_INPUT_IRQ_Disable();
    
    /* Stop Analog multiplexer */
    AMuxSeq_Stop();
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       10.11.2019
\brief      Wrapper function for better modularity
\return     none
\param      uiMinLimit - Sets the min limit
\param      uiMaxLimit - Sets the max limit
\param      ucOutputIdx - The desired output which shall get new values
***********************************************************************************/
void Measure_SetNewVoltageLimits(u32 ulMinLimit, u32 ulMaxLimit, u8 ucOutputIdx)
{
    Measure_Voltage_SetNewLimits(ulMinLimit, ulMaxLimit, ucOutputIdx);
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       10.11.2019
\brief      Wrapper function for better modularity
\return     u32 - SystemVoltage in millivolt
\param      none
***********************************************************************************/
u32 Measure_GetSystemVoltage(void)
{
    return Measure_Voltage_GetSystemVoltage();
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       10.11.2019
\brief      Wrapper function for better modularity
\param     u32 - System voltage in millivolt
\return    none
***********************************************************************************/
void Measure_SetSystemVoltage(u32 ulSystemVoltage)
{
    Measure_Voltage_SetSystemVoltage(ulSystemVoltage);
}


//********************************************************************************
/*!
\author     Kraemer E.
\date       27.12.2020
\brief      Function which returns the averaged ADC value of the desired channel
\return     u16 - Averaged ADC value
\param      eAdcChannel - The ADC channel with the values 
***********************************************************************************/
u16 Measure_GetAveragedAdcValue(teAdMuxList eAdcChannel)
{
    return CalculateAveragedAdcValue(&sAdMuxList[eAdcChannel].sMeasureValue);
}
#endif