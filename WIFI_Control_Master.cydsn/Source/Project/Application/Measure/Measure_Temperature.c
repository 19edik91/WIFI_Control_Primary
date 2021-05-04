/* ========================================
 *
 * Copyright Eduard Kraemer, 2019
 *
 * ========================================
*/

#include "Measure_Temperature.h"
#include "Aom.h"

/****************************************** Defines ******************************************************/
/****************************************** Variables ****************************************************/
typedef struct
{
    s16 siTemperature;
    u16 uiAdcValue;
}tsThermoTable;

/* Look-up-table for the NTC value 
   Precalculated values with a 10k-Ohm series resistor. The whole calculation
   and NTC-value table is found in 
   "file://wdmycloud/e_kraemer/Elektrotechnik/Software-Entwicklung/SVN-Projects/LED_Wifi_Documentation/trunk/Sonstiges/NTC"
    Note - Temperature is given with additional decimal (-50 = -5.0°C)
*/

static const tsThermoTable sThermoTableNTC[] =
{
    {-50	, 1765	},
    {0	    , 1448	},
    {50	    , 1180	},
    {100	, 958	},
    {150	, 777	},
    {200	, 630	},
    {250	, 511	},
    {300	, 416	},
    {350	, 340	},
    {400	, 279	},
    {450	, 229	},
    {500	, 190	},
    {550	, 158	},
    {600	, 132	},
    {650	, 110	},
    {700	, 93	},
    {750	, 78	},
    {800	, 67	},
    {850	, 57	}
};

/****************************************** Function prototypes ******************************************/
/****************************************** loacl functiones *********************************************/
/****************************************** External visible functiones **********************************/

//********************************************************************************
/*!
\author     Kraemer E.
\date       27.09.2019

\fn         Measure_Temperature_CalculateTemperature()

\brief      Calculate temperature value from the ADC value

\return     (u16)ulResult - Returns the calculated temperature value.

\param      uiNTCAdcValue - Measured ADC value from the NTC-Circuit
***********************************************************************************/
u16 Measure_Temperature_CalculateTemperature (u16 uiNTCAdcValue)
{
    u32 ulResult;
    u16 uiDelta;
    u16 uiIdx;
    u16 uiIdxFirst = _countof(sThermoTableNTC) - 1;
    u16 uiIdxSecond = 0;

    // check for lower temperature limit
    if(uiNTCAdcValue <= sThermoTableNTC[uiIdxFirst].uiAdcValue)
        return sThermoTableNTC[uiIdxFirst].siTemperature;

    // check for upper temperature limit
    if (uiNTCAdcValue >= sThermoTableNTC[uiIdxSecond].uiAdcValue)
        return sThermoTableNTC[uiIdxSecond].siTemperature;

    // binary search
    while((uiIdxFirst - uiIdxSecond) > 1)
    {
        /* Get the middle index between both */
        uiIdx = (uiIdxSecond + uiIdxFirst) / 2;

        // if value is in the second half
        if(uiNTCAdcValue >= sThermoTableNTC[uiIdx].uiAdcValue)
        {
            uiIdxFirst = uiIdx;
        }
        else
        {
            uiIdxSecond = uiIdx;
        }
    }

    /*
        Equation for the linear interpolation:
        X = ((Y1 - Y) * (X2-X1)/(Y2-Y1)) + X1
    */
    uiDelta = sThermoTableNTC[uiIdxSecond].uiAdcValue - sThermoTableNTC[uiIdxFirst].uiAdcValue;

    if(uiDelta)
    {
        ulResult = sThermoTableNTC[uiIdxSecond].uiAdcValue - uiNTCAdcValue;
        ulResult *= sThermoTableNTC[uiIdxFirst].siTemperature - sThermoTableNTC[uiIdxSecond].siTemperature;
        ulResult /= uiDelta;
        ulResult += sThermoTableNTC[uiIdxSecond].siTemperature;
    }
    else
    {
        ulResult = (sThermoTableNTC[uiIdxFirst].siTemperature - sThermoTableNTC[uiIdxSecond].siTemperature) / 2;
    }

    return (u16)ulResult;
}
