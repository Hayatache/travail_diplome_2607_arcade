/*--------------------------------------------------------*/
//  Mc32DriverAdc.c
/*--------------------------------------------------------*/
//	Description :	Fonctions pour gestion ADC 10 bits
//                      en mode scan
//	Auteur 		: 	C. HUBER
//
//	Version		:	V1.0
//	Compilateur	:	XC32 V1.33 & Harmony 1.00
//
//      BSP             :  pic32mx_skes
//
// Revu / modifié:
//      création 18.09.2014 CHR
/*--------------------------------------------------------*/

#include "system_config.h"
#include "Mc32DriverAdc.h"
#include "peripheral/adc/plib_adc.h"



// Create the list of  channels to scan
// Bit a 1  SCAN Bit0 = AN0
//#define configscan  0x000C       // SCAN AN1 AN0 (Pots du kit)

//pour pic32mx230f256d
#define configscan  0x01F0


/*--------------------------------------------------------*/
// Fonction BSP_InitADC10
/*--------------------------------------------------------*/

void BSP_InitADC10(void)
{
    // Configure l'ADC

    PLIB_ADC_InputScanMaskAdd(ADC_ID_1, configscan) ;   // liste des AN à scanner
    PLIB_ADC_ResultFormatSelect(ADC_ID_1, ADC_RESULT_FORMAT_INTEGER_16BIT);
    PLIB_ADC_ResultBufferModeSelect(ADC_ID_1, ADC_BUFFER_MODE_TWO_8WORD_BUFFERS);
    PLIB_ADC_SamplingModeSelect(ADC_ID_1, ADC_SAMPLING_MODE_MUXA);

    PLIB_ADC_ConversionTriggerSourceSelect(ADC_ID_1, ADC_CONVERSION_TRIGGER_INTERNAL_COUNT);
    PLIB_ADC_VoltageReferenceSelect(ADC_ID_1, ADC_REFERENCE_VDD_TO_AVSS );
    PLIB_ADC_SampleAcquisitionTimeSet(ADC_ID_1, 0x1F);
    PLIB_ADC_ConversionClockSet(ADC_ID_1, SYS_CLK_FREQ, 32);

    // Rem CHR le nb d'échantillon par interruption doit correspondre au nb d'entrées
    // de la liste de scan
    PLIB_ADC_SamplesPerInterruptSelect(ADC_ID_1, ADC_5SAMPLES_PER_INTERRUPT);
    PLIB_ADC_MuxAInputScanEnable(ADC_ID_1);

    // Enable the ADC module
    PLIB_ADC_Enable(ADC_ID_1);

 }

/*--------------------------------------------------------*/
// Fonction ReadAllADC
/*--------------------------------------------------------*/

S_ADCResults BSP_ReadAllADC()
{
    S_ADCResults result;
    ADC_RESULT_BUF_STATUS BufStatus;

    // Auto start sampling
    // PLIB_ADC_SampleAutoStartEnable(ADC_ID_1);

    // Attente fin de conversion
    //    while (!PLIB_ADC_ConversionHasCompleted(ADC_ID_1))

    // Stop sample/convert
    PLIB_ADC_SampleAutoStartDisable(ADC_ID_1);

    // traitement avec buffer alterné
    BufStatus = PLIB_ADC_ResultBufferStatusGet(ADC_ID_1);
    if (BufStatus == ADC_FILLING_BUF_0TO7) {
        result.V_bat_RTC = PLIB_ADC_ResultGetByIndex(ADC_ID_1, 0);
        result.Joystick_1_Y = PLIB_ADC_ResultGetByIndex(ADC_ID_1, 1);
        result.Joystick_1_X = PLIB_ADC_ResultGetByIndex(ADC_ID_1, 2);
        result.Joystick_2_Y = PLIB_ADC_ResultGetByIndex(ADC_ID_1, 3);
        result.Joystick_2_X = PLIB_ADC_ResultGetByIndex(ADC_ID_1, 4);
    } else {
        result.V_bat_RTC = PLIB_ADC_ResultGetByIndex(ADC_ID_1, 8);
        result.Joystick_1_Y = PLIB_ADC_ResultGetByIndex(ADC_ID_1, 9);
        result.Joystick_1_X = PLIB_ADC_ResultGetByIndex(ADC_ID_1, 10);
        result.Joystick_2_Y = PLIB_ADC_ResultGetByIndex(ADC_ID_1, 11);
        result.Joystick_2_X = PLIB_ADC_ResultGetByIndex(ADC_ID_1, 12);
    }

    // Auto start sampling
    PLIB_ADC_SampleAutoStartEnable(ADC_ID_1);

    return result;
}
